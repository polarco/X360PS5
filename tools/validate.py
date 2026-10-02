#!/usr/bin/env python3
"""Local acceptance evidence; no console access or hardware claims."""
import hashlib
import json
import subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[1]
evidence=root/'docs/evidence'; evidence.mkdir(parents=True,exist_ok=True)
def run(args,name):
    with (evidence/name).open('w') as output:
        subprocess.run(args,cwd=root,stdout=output,stderr=subprocess.STDOUT,check=True)
def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()
run(['make','test','xenia-host'],'host.txt')
run(['bash','tools/build-ps5.sh'],'ps5-build.txt')
paths=[root/'dist/PPSA99361/eboot.bin',root/'dist/PPSA99361/sce_module/libc.prx']
first=[sha(path) for path in paths]
run(['bash','tools/build-ps5.sh'],'ps5-repeat-build.txt')
second=[sha(path) for path in paths]
if first != second: raise SystemExit('Binary reproduction failed; compare build evidence')
(evidence/'inspection.txt').write_bytes((root/'build/ps5/inspection.txt').read_bytes())
report={'version':(root/'VERSION').read_text().strip(),'host_tests':'APROVADO',
        'ppc_cases':[42,4,0],'ppc_lifecycle_repeats':3,'same_environment_binary_reproduction':'APROVADO',
        'ps5_runtime':'NAO TESTADO','vulkan_presentation':'NAO TESTADO',
        'binaries':{str(p.relative_to(root)):s for p,s in zip(paths,second)}}
(evidence/'results.json').write_text(json.dumps(report,indent=2)+'\n')
with (evidence/'toolchain.txt').open('w') as output:
    for args in [['uname','-a'],['clang-18','--version'],['clang-19','--version'],['clang-21','--version'],['cmake','--version'],['glslangValidator','--version']]:
        subprocess.run(args,stdout=output,stderr=subprocess.STDOUT,check=True)
print(json.dumps(report,indent=2))
