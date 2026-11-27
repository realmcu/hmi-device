#!/usr/bin/env python3
"""Host execution of early init failure using real LCD defs and trapping SDK stubs."""
# SPDX-License-Identifier: Apache-2.0
import argparse
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
HERE=Path(__file__).resolve().parent
DEVICE=HERE.parents[1]
DISPLAY=DEVICE/'output/display'
LCD=DEVICE.parent/'display/hardware/lcdc'

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--gcc',default=shutil.which('gcc'))
    parser.add_argument('--nm',default=shutil.which('nm'))
    args=parser.parse_args()
    with tempfile.TemporaryDirectory(prefix='display-init-failure-') as temporary:
        root=Path(temporary)
        types=(HERE/'syntax_sdk.h').read_text(encoding='utf-8')
        types+='\n'+'\n'.join(f'#define BIT{i} (1u << {i})' for i in range(32))+'\n#endif\n'
        (root/'syntax_sdk.h').write_text(types,encoding='utf-8')
        (root/'rtl876x.h').write_text('#include "syntax_sdk.h"\n',encoding='utf-8')
        flags=['-std=c11','-Wall','-Wextra','-Werror','-Wno-pointer-to-int-cast','-Wno-int-to-pointer-cast','-ffunction-sections','-fdata-sections',
               '-I',str(root),'-I',str(DISPLAY/'core'),'-I',str(DISPLAY/'ports'),'-I',str(LCD/'inc'),'-I',str(LCD/'src/device/rtl8773e')]
        objects=[]
        undefined=set()
        for kind in ('sdi','rgb'):
            obj=root/f'{kind}.o';objects.append(obj)
            subprocess.run([args.gcc,*flags,'-c',str(DISPLAY/f'controller/lcdc/rtk_{kind}_lcdc.c'),'-o',str(obj)],check=True)
            result=subprocess.run([args.nm,'-u',str(obj)],capture_output=True,text=True,check=True)
            undefined.update(re.findall(r'\bU\s+((?:LCDC|DBIC|EDPI)_\w+)',result.stdout))
        traps='#include <assert.h>\n'+'\n'.join(f'void {name}(void) {{ assert(!"Unexpected hardware call: {name}"); }}' for name in sorted(undefined))+'\n'
        (root/'traps.c').write_text(traps,encoding='utf-8')
        exe=root/'failure.exe'
        subprocess.run([args.gcc,*flags,str(HERE/'init_failure_test.c'),str(root/'traps.c'),*[str(x) for x in objects],'-Wl,--gc-sections','-o',str(exe)],check=True)
        subprocess.run([str(exe)],check=True)
    print('PASS early-init failure path; trapping fake LCD functions, NOT actual HAL/runtime validation')

if __name__=='__main__':main()
