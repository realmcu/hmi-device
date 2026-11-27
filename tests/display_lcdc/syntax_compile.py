#!/usr/bin/env python3
"""Compile shared controllers against real three-SoC LCD defs and SDK type stubs.
No actual E/2G SDK, linker, hardware or register values are validated.
"""
# SPDX-License-Identifier: Apache-2.0
from pathlib import Path
import argparse
import shutil
import subprocess
import tempfile

HERE=Path(__file__).resolve().parent
DEVICE=HERE.parents[1]
LCD=DEVICE.parent/'display/hardware/lcdc'
DISPLAY=DEVICE/'output/display'

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--gcc', default=shutil.which('arm-zephyr-eabi-gcc') or 'E:/Programs/Zephyr-SDK/zephyr-sdk-0.16.8/arm-zephyr-eabi/bin/arm-zephyr-eabi-gcc.exe', help='ARM GCC; PATH first, then the SDK path used in this workspace')
    args=parser.parse_args()
    with tempfile.TemporaryDirectory(prefix='display-lcdc-syntax-') as temporary:
        root=Path(temporary)
        (root/'utils').mkdir()
        types=(HERE/'syntax_sdk.h').read_text(encoding='utf-8')
        types+='\n'+'\n'.join(f'#define BIT{i} (1u << {i})' for i in range(32))+'\n#endif\n'
        (root/'syntax_sdk.h').write_text(types,encoding='utf-8')
        for relative in ['rtl876x.h','address_map.h','utils/rtl_utils.h']:
            (root/relative).write_text('#include "syntax_sdk.h"\n',encoding='utf-8')
        for soc,symbol in [('rtl8773g','CONFIG_SOC_SERIES_RTL87X3G'),('rtl8773e','CONFIG_SOC_SERIES_RTL8773E'),('rtl87x2g','CONFIG_SOC_SERIES_RTL87X2G')]:
            for source in ['controller/lcdc/rtk_sdi_lcdc.c','controller/lcdc/rtk_rgb_lcdc.c','core/rtk_display.c']:
                cmd=[args.gcc,'-std=c11','-Wall','-Wextra','-Werror','-fsyntax-only',f'-D{symbol}=1',
                     '-I',str(root),'-I',str(DISPLAY/'core'),'-I',str(DISPLAY/'ports'),
                     '-I',str(LCD/'inc'),'-I',str(LCD/'src/device'/soc),str(DISPLAY/source)]
                subprocess.run(cmd,check=True)
                print('PASS real LCD defs + SDK-stub syntax',soc,source)
    print('PASS syntax-only: NOT real E/2G SDK or runtime validation')

if __name__=='__main__':main()
