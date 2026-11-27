#!/usr/bin/env python3
"""Source preservation and CMake graph regression for shared LCDC (not hardware)."""
# SPDX-License-Identifier: Apache-2.0
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent
DEVICE = HERE.parents[1]
DISPLAY = DEVICE / 'output/display'


def run(command, expected=0):
    result = subprocess.run([str(x) for x in command], check=False)
    assert (result.returncode == 0) == (expected == 0), (command, result.returncode)


def kconfig(temp):
    import kconfiglib
    symbols = ('REALTEK_DISPLAY','REALTEK_DISPLAY_ENABLE_LCDC','SOC_SERIES_RTL87X2G',
               'SOC_SERIES_RTL8773E','SOC_SERIES_RTL87X3E','SOC_SERIES_RTL87X3G','SOC_SERIES_RTL8773G')
    mock = temp/'Kconfig'
    mock.write_text('\n'.join(f'config {name}\n    bool "{name}"' for name in symbols)
                    + f'\nsource "{(DISPLAY/"Kconfig").as_posix()}"\n', encoding='utf-8')
    for soc in symbols[2:]:
        conf = kconfiglib.Kconfig(str(mock), warn_to_stderr=False)
        conf.syms['REALTEK_DISPLAY'].set_value(2)
        conf.syms[soc].set_value(2)
        shared = conf.syms['REALTEK_DISPLAY_LCDC_SHARED']
        default = soc in ('SOC_SERIES_RTL87X3G','SOC_SERIES_RTL8773G')
        assert (shared.str_value=='y') == default, soc
        shared.set_value(2)
        assert shared.str_value=='y' and conf.syms['REALTEK_DISPLAY_ENABLE_LCDC'].str_value=='y', soc
        conf.syms['REALTEK_DISPLAY'].set_value(0)
        assert shared.str_value=='n', soc
    conf = kconfiglib.Kconfig(str(mock), warn_to_stderr=False)
    conf.syms['REALTEK_DISPLAY'].set_value(2)
    conf.syms['REALTEK_DISPLAY_LCDC_SHARED'].set_value(2)
    assert conf.syms['REALTEK_DISPLAY_LCDC_SHARED'].str_value=='n'
    manifest = (DEVICE.parent/'display/Kconfig').read_text(encoding='utf-8')
    definition = manifest.split('config REALTEK_DISPLAY_USE_PANEL_MANIFEST',1)[1].split('\nconfig ',1)[0]
    assert 'depends on REALTEK_DISPLAY_LCDC_SHARED' in definition
    print('PASS shared Kconfig SoC defaults/opt-in/display dependency/LCDC select/manifest dependency')


def register_layouts():
    root = DEVICE.parent / 'display/hardware/lcdc/src/device'
    for kind in ('dbic','dma','dma_ll','edpi','handler','dbib','rlspi'):
        values = []
        for soc in ('rtl8773g','rtl8773e','rtl87x2g'):
            text = (root/soc/f'rtl_lcdc_{kind}_def.h').read_text(encoding='utf-8')
            text = re.sub(r'/\*.*?\*/|//[^\n]*','',text,flags=re.S)
            text = re.sub(r'^\s*#include[^\n]*|^\s*#define DISPLAY_CTRL_REG_BASE[^\n]*','',text,flags=re.M)
            text = re.sub(r'typedef\s+enum\s*\{[^{}]*\}\s*LCDC_BUS_CLOCK\s*;','',text,flags=re.S)
            values.append(re.sub(r'\s+','',text))
        assert values[0] == values[1] == values[2], kind
    print('PASS three-family real LCD register definitions equal except SDK includes/base alias/clock enum')


def preservation():
    baseline = json.loads((HERE / 'baseline.json').read_text(encoding='utf-8'))
    for relative, record in baseline.items():
        # Core types gain clock options; controller/core implementation changes are intended.
        if relative.startswith('output/display/boards/') or '/panel/' in relative or relative.endswith(('rtk_display.h', 'rtk_sdi.h', 'rtk_rgb.h')):
            data = (DEVICE / relative).read_bytes().replace(b'\r\n', b'\n')
            assert hashlib.sha256(data).hexdigest() == record['sha256_lf'], relative
    for kind in ('sdi', 'rgb'):
        source = DISPLAY / f'controller/lcdc/rtk_{kind}_lcdc.c'
        text = source.read_text(encoding='utf-8')
        functions = sorted(re.findall(r'^\w[\w\s*]*\s+(rtk_\w+)\s*\([^;]*?\)\s*\{', text, re.M))
        old = baseline[f'output/display/core/rtk_{kind}_8773g.c']['functions']
        assert functions == old, (kind, functions, old)
        assert 'rtl876x_' not in text and 'LCDC_Clock_Sel' not in text and 'Pad_HighSpeed' not in text, kind
    print('PASS API function-set preservation, board/panel bytes unchanged, shared sources no G HAL calls')


HARNESS = '''cmake_minimum_required(VERSION 3.20)
project(display_graph C)
set(ZEPHYR_BASE "mock")
function(zephyr_library_named name)
  add_library(${name} STATIC "${CMAKE_SOURCE_DIR}/dummy.c")
endfunction()
add_library(realtek_display_driver_library INTERFACE)
add_library(guiLib INTERFACE)
add_subdirectory("@DEVICE@" device)
set(report "target=absent\n")
if(TARGET realtek_device_driver_library)
  set(report "target=present\n")
  foreach(prop SOURCES INCLUDE_DIRECTORIES LINK_LIBRARIES)
    get_target_property(value realtek_device_driver_library ${prop})
    string(APPEND report "${prop}=${value}\n")
  endforeach()
endif()
file(WRITE "${CMAKE_BINARY_DIR}/graph.txt" "${report}")
'''


def graphs(temp, cmake, gcc):
    harness = temp / 'graph'
    harness.mkdir()
    (harness / 'dummy.c').write_text('int display_test_dummy;\n', encoding='utf-8')
    (harness / 'CMakeLists.txt').write_text(HARNESS.replace('@DEVICE@', DEVICE.as_posix()), encoding='utf-8')
    base = dict(CONFIG_REALTEK_DISPLAY='y', CONFIG_REALTEK_TOUCH='n', CONFIG_REALTEK_KEY_BUTTON='n')
    socs = {'8773g': 'RTL87X3G', '8773e': 'RTL8773E', '8762g': 'RTL87X2G'}
    cases = []
    for soc, symbol in socs.items():
        for shared in ('y', 'n'):
            cases.append((f'{soc}-{shared}', soc, {f'CONFIG_SOC_SERIES_{symbol}': 'y',
                          'CONFIG_REALTEK_DISPLAY_LCDC_SHARED': shared,
                          'CONFIG_REALTEK_LCD_SH8601Z_410_502_QSPI': 'y'}, shared == 'y'))
    cases.append(('8773g-default', '8773g', {'CONFIG_SOC_SERIES_RTL87X3G': 'y'}, True))
    cases.append(('8773g-alias', '8773g', {'CONFIG_SOC_SERIES_RTL8773G': 'y'}, True))
    cases.append(('8773e-alias', '8773e', {'CONFIG_SOC_SERIES_RTL87X3E': 'y', 'CONFIG_REALTEK_DISPLAY_LCDC_SHARED': 'y'}, True))
    cases.append(('touch-only', None, {'CONFIG_REALTEK_DISPLAY': 'n', 'CONFIG_REALTEK_TOUCH': 'y',
                                      'CONFIG_SOC_SERIES_RTL87X3G': 'y'}, False))
    for name, soc, overrides, shared in cases:
        build = temp / name
        configs = dict(base, **overrides)
        if soc == '8773g':
            configs['CONFIG_BOARD_RTL87X3G_SAMPLE'] = 'y'
        run([cmake, '-S', harness, '-B', build, '-G', 'Ninja', f'-DCMAKE_C_COMPILER={gcc}'] + [f'-D{k}={v}' for k,v in configs.items()])
        report = (build / 'graph.txt').read_text(encoding='utf-8').replace('\\', '/')
        sources = next(line for line in report.splitlines() if line.startswith('SOURCES='))
        for kind in ('sdi', 'rgb'):
            assert (f'/controller/lcdc/rtk_{kind}_lcdc.c' in sources) == shared, (name, sources)
        assert '/core/rtk_sdi_8773g.c' not in sources and '/core/rtk_rgb_8773g.c' not in sources
        assert ('/core/rtk_display.c' in sources) == shared, (name, sources)
        assert ('/legacy/' in sources and '/lcd/' in sources) == (soc == '8773e' and not shared), (name, sources)
        if soc != '8773g':
            assert '/zephyr/display/rtl87x3g-display-lcd-qspi.c' not in sources, (name, sources)
        if shared:
            port_sources = [p for p in sources.split(';') if '/ports/' in p and p.endswith('.c')]
            assert len(port_sources) == 1, (name, port_sources)
            assert soc in port_sources[0], (name, port_sources)
        assert ('/boards/rtl87x3g/board_display.c' in sources) == (soc == '8773g' and shared), (name, sources)
        if name == 'touch-only':
            assert '/output/display/' not in report and '/zephyr/display/' not in report
        print('PASS graph', name)
    invalid = {'unsupported': {}, 'multiple': {'CONFIG_SOC_SERIES_RTL87X3G':'y','CONFIG_SOC_SERIES_RTL8773E':'y'}}
    for name, extra in invalid.items():
        configs = dict(base, CONFIG_REALTEK_DISPLAY_LCDC_SHARED='y', **extra)
        run([cmake,'-S',harness,'-B',temp/f'invalid-{name}','-G','Ninja',f'-DCMAKE_C_COMPILER={gcc}'] + [f'-D{k}={v}' for k,v in configs.items()], expected=1)
        print('PASS rejects shared', name, '(expected CMake error)')


def ports(temp, gcc):
    stub = temp / 'sdk-stubs'
    stub.mkdir()
    for name in ('rtl_lcdc.h', 'rtl876x_pinmux.h', 'rtl876x_nvic.h', 'rtl876x_rcc.h',
                 'platform_utils.h', 'vector_table.h', 'rtl_pinmux.h', 'rtl_rcc.h', 'rtl_nvic.h', 'utils.h'):
        (stub / name).write_text('#include "fake_hal.h"\n', encoding='utf-8')
    (stub / 'irq_hook.h').write_text('int fake_irq_install(void(*handler)(void));\n#define RTK_DISPLAY_SOC_IRQ_INSTALL(handler) fake_irq_install(handler)\n', encoding='utf-8')
    for soc, number in (('8773g',3),('8773e',2),('8762g',1)):
        for hook in ((False,) if number==3 else (False,True)):
            exe = temp / f'port-{soc}-{hook}.exe'
            cmd = [gcc,'-std=c11','-Wall','-Wextra','-Werror',f'-DTEST_SOC={number}',
                   '-I',HERE,'-I',stub,'-I',DISPLAY/'ports','-I',DISPLAY/'core']
            if hook:
                cmd += ['-DTEST_IRQ_HOOK=1','-include',stub/'irq_hook.h']
            cmd += [HERE/'port_test.c',DISPLAY/f'ports/rtl{soc}.c','-o',exe]
            run(cmd)
            run([exe])
            print('PASS port', soc, 'hook' if hook else 'default')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--gcc', default=shutil.which('gcc'))
    parser.add_argument('--cmake', default=shutil.which('cmake'))
    parser.add_argument('--only', choices=('preservation','graphs','ports','kconfig','all'), default='all')
    args = parser.parse_args()
    if args.only in ('preservation','all'):
        register_layouts()
        preservation()
    with tempfile.TemporaryDirectory(prefix='display-lcdc-tests-') as temp:
        if args.only in ('kconfig','all'): kconfig(Path(temp))
        if args.only in ('graphs','all'): graphs(Path(temp), args.cmake, args.gcc)
        if args.only in ('ports','all'): ports(Path(temp), args.gcc)
    print('PASS shared LCDC source/graph tests (no real E/2G HAL validation)')

if __name__ == '__main__': main()
