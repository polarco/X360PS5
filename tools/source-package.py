#!/usr/bin/env python3
"""Archive project sources and pinned upstream trees, including available submodules."""
import io
import json
import subprocess
import tarfile
from pathlib import Path

root=Path(__file__).resolve().parents[1]
version=(root/'VERSION').read_text().strip()
output=root/'dist'/f'X360PS5-{version}-sources.tar.gz'
output.parent.mkdir(exist_ok=True)
lock=json.loads((root/'dependencies.lock.json').read_text())
with tarfile.open(output,'w:gz',compresslevel=5) as target:
    for path in sorted(root.iterdir()):
        if path.name in {'.git','.deps','build','dist','logs'} or path.name.startswith('ClaudeContext'):
            continue
        target.add(path,arcname='X360PS5/'+path.name,filter=lambda info: None if '__pycache__' in info.name else info)
    def archive(repo, revision, prefix):
        process=subprocess.Popen(['git','-C',str(repo),'archive',revision],stdout=subprocess.PIPE)
        with tarfile.open(fileobj=process.stdout,mode='r|') as source:
            for member in source:
                data=source.extractfile(member) if member.isfile() else None
                member.name=prefix+'/'+member.name
                target.addfile(member,data)
        if process.wait(): raise RuntimeError(f'git archive failed: {repo}')
    for name,entry in lock.items():
        repo=root/'.deps'/name
        archive(repo,entry['revision'],'X360PS5/.deps/'+name)
        if name=='xenia':
            lines=subprocess.check_output(['git','-C',str(repo),'submodule','status','--recursive'],text=True).splitlines()
            for line in lines:
                if line.startswith('-'): continue # unused, uninitialized upstream dependency
                sha,path,*_=line[1:].split()
                archive(repo/path,sha,'X360PS5/.deps/xenia/'+path)
print(output)
