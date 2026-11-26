#!/usr/bin/env python3
"""Developer/CI check. Report paths, never secret values. No runtime dependency."""
import re, subprocess, sys, zipfile
from pathlib import Path
PATTERNS=[re.compile(rb'-----BEGIN (?:[A-Z0-9]+ )?PRIVATE KEY-----'),
          re.compile(rb'(?:gh[pousr]_[A-Za-z0-9]{30,255}|github_pat_[A-Za-z0-9_]{60,255})'),
          re.compile(rb'MIGHAgEAMBMGByqGSM49AgEGCCqGSM49AwEHBG0wawIBAQQg[A-Za-z0-9+/]{70,}'),
          re.compile(bytes.fromhex('308187020100301306072a8648ce3d020106082a8648ce3d03010704'))]
DENIED={'.key','.pem','.pfx','.p12','.der','.env'}
def sensitive(name,data):
    p=Path(name)
    return p.suffix.lower() in DENIED or p.name.lower().startswith(('config.json','signing-key','.env.')) or any(pattern.search(data) for pattern in PATTERNS)
def check(paths):
    errors=[];count=0
    for path in paths:
        if path.suffix.lower()=='.zip':
            with zipfile.ZipFile(path) as archive:
                for entry in archive.infolist():
                    count+=1
                    if entry.file_size>16*1024*1024:errors.append(str(path)+'!'+entry.filename);continue
                    if sensitive(entry.filename,archive.read(entry)):errors.append(str(path)+'!'+entry.filename)
        else:
            count+=1
            if sensitive(path.name,path.read_bytes()):errors.append(str(path))
    if errors:raise SystemExit('Sensitive material rejected (paths only):\n'+'\n'.join(errors))
    print(f'PASS: {count} source/package entries scanned for private keys, tokens and user config (pattern-based, not a proof of absence).')
if __name__=='__main__':
    root=Path(__file__).resolve().parent.parent
    if len(sys.argv)>1:paths=[Path(p) for p in sys.argv[1:]]
    else:
        result=subprocess.run(['git','ls-files','-z'],cwd=root,capture_output=True)
        if not result.returncode:paths=[root/p for p in result.stdout.decode().split('\0') if p and (root/p).is_file()]
        else:
            ignored={'build-local','dist','local-only','custom-pets','.git','.vs','__pycache__'}
            paths=[p for p in root.rglob('*') if p.is_file() and not any(part in ignored or part.startswith('build') for part in p.relative_to(root).parts) and p.suffix.lower() not in {'.zip','.exe','.obj','.pdb'}]
    check(paths)
