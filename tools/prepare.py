#!/usr/bin/env python3
"""Resolve locked sources and stage a reproducible build without touching the PS5."""
import json
from pathlib import Path
import subprocess
import sys
import shutil

ROOT=Path(__file__).resolve().parents[1]
def git(path,*args):
    return subprocess.check_output(['git','-C',str(path),*args],text=True).strip()
def fetch():
    lock=json.loads((ROOT/'dependencies.lock.json').read_text())
    for name,entry in lock.items():
        path=ROOT/'.deps'/name
        if not (path/'.git').exists():
            path.mkdir(parents=True,exist_ok=True)
            subprocess.run(['git','init',str(path)],check=True)
            git(path,'remote','add','origin',entry['url'])
        try: git(path,'cat-file','-e',entry['revision']+'^{commit}')
        except subprocess.CalledProcessError: git(path,'fetch','--depth','1','origin',entry['revision'])
        if subprocess.run(['git','-C',str(path),'rev-parse','--verify','HEAD'],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL).returncode:
            git(path,'-c','core.autocrlf=false','checkout','--detach',entry['revision'])
        elif name not in ('boilerplate','PS5_PayloadSDK') and git(path,'rev-parse','HEAD') != entry['revision']:
            raise RuntimeError(f'{name}: checkout differs from lock; preserve local work and use a separate checkout of '+entry['revision'])

def verify():
    lock=json.loads((ROOT/'dependencies.lock.json').read_text())
    for name in ('xenia','PS5_Mesa','PS5_Vulkan'):
        path=ROOT/'.deps'/name
        if git(path,'rev-parse','HEAD') != lock[name]['revision']:
            raise RuntimeError(f'{name}: revision mismatch')
        # Ignore only line endings, including Windows checkout conversions.
        # Native Git avoids very slow per-file DrvFS metadata round trips.
        command=['git','-C',str(path)]
        if str(path).startswith('/mnt/') and shutil.which('git.exe'):
            windows=subprocess.check_output(['wslpath','-w',str(path)],text=True).strip()
            command=['git.exe','-C',windows]
        subprocess.run(command+['-c','core.filemode=false','-c','core.safecrlf=false','diff','--quiet','--ignore-space-at-eol','--ignore-submodules=dirty','HEAD','--'],check=True)

def stage():
    root=ROOT
    generated=root/'build/ps5/generated'; generated.mkdir(parents=True,exist_ok=True)
    lock=json.loads((root/'dependencies.lock.json').read_text())
    b=root/'.deps/boilerplate'
    def source(path): return git(b,'show',lock['boilerplate']['revision']+':'+path)+'\n'
    h=source('src/demo_renderer.hpp')
    cpp=source('src/demo_renderer.cpp')
    # Keep the upstream bounded font/canvas and VideoOut setup, but redraw each frame.
    old='    for (;;)\n        (void)sceKernelUsleep(1000000);'
    new='''    unsigned frame = 0;
    for (;;) {
        unsigned index = (++frame) & 1;
        draw(index ? second : first);
        flush_range(index ? second_frame : mapped, frame_bytes);
        if (sceVideoOutSubmitFlip(video, index, 1, frame + 1) < 0)
            halt("X360PS5: flip failed");
        (void)sceVideoOutWaitVblank(video);
        (void)sceKernelUsleep(16000);
    }'''
    if old not in cpp: raise RuntimeError('pinned renderer loop changed')
    (generated/'demo_renderer.hpp').write_text(h)
    before,after=cpp.rsplit(old,1)
    (generated/'demo_renderer.cpp').write_text(before+new+after)
    metadata=json.loads(source('sce_sys/param.json'))
    metadata['titleId']='PPSA99361'
    metadata['contentId']='IV0000-PPSA99361_00-X360PS5DIAG000001'
    metadata['titleName']='X360PS5 Diagnostics'
    major,minor,patch=map(int,(root/'VERSION').read_text().strip().split('.'))
    metadata['contentVersion']=f'{major:02d}.{minor:03d}.{patch:03d}'
    if 'conceptId' in metadata: metadata['conceptId']='99361'
    if 'localizedParameters' in metadata:
        for value in metadata['localizedParameters'].values():
            if isinstance(value,dict) and 'titleName' in value: value['titleName']='X360PS5 Diagnostics'
    (generated/'param.json').write_text(json.dumps(metadata,indent=2)+'\n')
    subprocess.run([sys.executable,str(root/'tools/build_info.py'),str(generated),'PS5'],check=True)

if __name__=='__main__':
    if '--fetch' in sys.argv: fetch()
    verify()
    stage()
