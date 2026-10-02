#!/usr/bin/env python3
"""Generate provenance; revisions remain separate from runtime verification."""
import json
from pathlib import Path
import sys
root = Path(__file__).resolve().parents[1]
out = Path(sys.argv[1]); out.mkdir(parents=True, exist_ok=True)
lock = json.loads((root / 'dependencies.lock.json').read_text())
revisions = ';'.join(f'{k}={v["revision"]}' for k,v in lock.items())
text = '#pragma once\n'
for key,value in [('VERSION',(root/'VERSION').read_text().strip()),('ENVIRONMENT',sys.argv[2]),('DEPENDENCIES',revisions)]:
    text += f'#define X360_{key} {json.dumps(value)}\n'
(out/'build_info.hpp').write_text(text)
