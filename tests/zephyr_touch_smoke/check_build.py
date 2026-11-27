#!/usr/bin/env python3
"""Check a completed touch-only SDK build's config, compile graph and ELF symbols."""
# SPDX-License-Identifier: Apache-2.0
import argparse
import json
from pathlib import Path
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('build', type=Path)
parser.add_argument('--nm', required=True, help='ARM toolchain nm executable')
args = parser.parse_args()
config = (args.build / 'zephyr/.config').read_text(encoding='utf-8')
for name in ('REALTEK_DISPLAY', 'DISPLAY', 'REALTEK_HONEYGUI'):
    assert f'CONFIG_{name}=y' not in config, name
assert 'CONFIG_REALTEK_TOUCH=y' in config
commands = json.loads((args.build / 'compile_commands.json').read_text(encoding='utf-8'))
files = [entry['file'].replace('\\', '/') for entry in commands]
assert any('/input/touch/core/rtk_touch.c' in p for p in files)
for entry in commands:
    text = json.dumps(entry).replace('\\\\', '/')
    assert '/modules/display/' not in text, entry['file']
    assert '/device/output/display/' not in text, entry['file']
    assert '/device/zephyr/display/' not in text, entry['file']
elf = args.build / 'zephyr/zephyr.elf'
with (args.build / 'touch_symbols.txt').open('w', encoding='utf-8') as output:
    subprocess.run([args.nm, str(elf)], stdout=output, check=True)
symbols = (args.build / 'touch_symbols.txt').read_text(encoding='utf-8')
for name in ('rtk_touch_get_default', 'rtk_touch_get_data', 'rtk_touch_get_timeout_ms'):
    assert name in symbols, name
assert 'rtk_display_' not in symbols
chips = ('CHSC6417', 'CHSC6413', 'CST816D', 'CST820')
selected = [chip for chip in chips if f'CONFIG_REALTEK_TOUCH_{chip}_ZEPHYR=y' in config]
assert len(selected) <= 1, selected
for chip in chips:
    filename = f'touch_{chip if chip.startswith("CHSC") else chip.lower()}_zephyr.c'
    assert any('/controller/zephyr/' + filename in p for p in files) == (chip in selected)
has_driver = bool(selected)
assert ('get_raw_touch_data' in symbols) == has_driver
assert any('/ports/zephyr/rtk_touch_zephyr.c' in p for p in files) == has_driver
assert ('touch_bind' in symbols) == has_driver
print(f'PASS {args.build.name}: touch-only config, compile graph and linked ELF')
