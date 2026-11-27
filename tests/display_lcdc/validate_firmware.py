#!/usr/bin/env python3
"""Inspect a completed real 8773G firmware build after shared-LCDC migration."""
# SPDX-License-Identifier: Apache-2.0
import argparse
import json
from pathlib import Path
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('build', type=Path)
parser.add_argument('--nm', required=True)
parser.add_argument('--rgb', action='store_true')
args = parser.parse_args()
config = (args.build / 'zephyr/.config').read_text(encoding='utf-8')
assert 'CONFIG_REALTEK_DISPLAY_LCDC_SHARED=y' in config
commands = json.loads((args.build / 'compile_commands.json').read_text(encoding='utf-8'))
files = [c['file'].replace('\\', '/') for c in commands]
for kind in ('sdi', 'rgb'):
    assert any(f'/controller/lcdc/rtk_{kind}_lcdc.c' in f for f in files)
    assert not any(f'rtk_{kind}_8773g.c' in f for f in files)
assert any('/ports/rtl8773g.c' in f for f in files)
assert not any('/ports/rtl8773e.c' in f or '/ports/rtl8762g.c' in f for f in files)
assert not any('/device/legacy/' in f and '/lcd/' in f for f in files)
with (args.build / 'shared_lcdc_symbols.txt').open('w', encoding='utf-8') as output:
    subprocess.run([args.nm, str(args.build / 'zephyr/zephyr.elf')], stdout=output, check=True)
symbols = (args.build / 'shared_lcdc_symbols.txt').read_text(encoding='utf-8')
for name in ('rtk_display_submit', 'rtk_display_soc_clock_config', 'rtk_display_soc_irq_install'):
    assert name in symbols, name
if args.rgb:
    assert 'CONFIG_REALTEK_LCD_EK9716_800_480_RGB=y' in config
    assert 'rtk_rgb_update_framebuffer' in symbols
else:
    assert 'CONFIG_REALTEK_BUILD_GUI_410_502_DEMO=y' in config
    assert 'CONFIG_REALTEK_LCD_SH8601Z_410_502_QSPI=y' in config
    assert 'rtk_sdi_start_transfer' in symbols
print('PASS actual firmware shared LCDC graph and linked symbols:', args.build.name)
