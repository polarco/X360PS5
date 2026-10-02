#!/usr/bin/env python3
"""Package an inspected diagnostic with explicit limitations and source material."""
import hashlib
import json
from pathlib import Path
import zipfile
root=Path(__file__).resolve().parents[1]
version=(root/'VERSION').read_text().strip()
app=root/'dist/PPSA99361'
source=root/'dist'/f'X360PS5-{version}-sources.tar.gz'
required=[app/'eboot.bin', app/'sce_module/libc.prx', app/'sce_sys/param.json', root/'build/ps5/inspection.txt',source]
for path in required:
    if not path.is_file(): raise SystemExit(f'Missing build/inspection: {path}')
out=root/'dist'/f'X360PS5-{version}-diagnostic.zip'
with zipfile.ZipFile(out,'w',compression=zipfile.ZIP_DEFLATED) as z:
    for path in sorted(app.rglob('*')):
        if path.is_file(): z.write(path,path.relative_to(app.parent))
    for name in ['README.md','CHANGELOG.md','docs/TESTADOR.md','docs/REFERENCES.md','docs/VALIDACAO.md','docs/ARQUITETURA.md','dependencies.lock.json','THIRD_PARTY_NOTICES.md','LICENSE']:
        z.write(root/name,name)
    z.write(root/'build/ps5/inspection.txt','evidence/inspection.txt')
    for path in (root/'docs/evidence').glob('*'):
        if path.is_file() and path.name!='inspection.txt': z.write(path,'evidence/'+path.name)
    for dependency in (root/'.deps').iterdir():
        if not dependency.is_dir(): continue
        for path in dependency.iterdir():
            if path.is_file() and path.name.upper().startswith(('LICENSE','COPYING','NOTICE')):
                z.write(path,'licenses/'+dependency.name+'/'+path.name)
def sha(path):
    with path.open('rb') as stream: return hashlib.file_digest(stream,'sha256').hexdigest()
digest=sha(out)
(root/'dist/SHA256SUMS').write_text(''.join(f'{sha(path)}  {path.relative_to(root/"dist").as_posix()}\n' for path in [out,source,app/'eboot.bin',app/'sce_module/libc.prx']))
print(f'{out}\nSHA256 {digest}')
