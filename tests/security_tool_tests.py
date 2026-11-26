import importlib.util, io, tempfile, zipfile, contextlib, re
from pathlib import Path
root=Path(__file__).resolve().parent.parent
spec=importlib.util.spec_from_file_location('sensitive',root/'tools/Check-Sensitive.py');scanner=importlib.util.module_from_spec(spec);spec.loader.exec_module(scanner)
assert scanner.sensitive('a.txt',b'-----BEGIN '+b'PRIVATE KEY-----')
assert scanner.sensitive('a.txt',b'ghp_'+b'A'*36)
assert scanner.sensitive('backup.pfx',b'')
assert not scanner.sensitive('README.md',b'UPDATE_SIGNING_KEY is configured in GitHub Secrets; never commit a key.')
with tempfile.TemporaryDirectory() as directory:
    path=Path(directory)/'bad.zip'
    with zipfile.ZipFile(path,'w') as z:z.writestr('DesktopPet_v1.3.0/config.json','private user data')
    try:scanner.check([path]);raise AssertionError('unsafe package accepted')
    except SystemExit as error:assert 'config.json' in str(error) and 'private user data' not in str(error)
workflow=(root/'.github/workflows/build.yml').read_text()
for uses in re.findall(r'uses:\s*([^\s#]+)',workflow):assert re.fullmatch(r'actions/[a-z-]+@[a-f0-9]{40}',uses),uses
assert 'persist-credentials: false' in workflow
assert 'artifact-ids:' in workflow and '--source-digest $env:GITHUB_SHA' in workflow and '--verify-tag' in workflow
assert 'pull_request_target:' not in workflow
assert '$object.sha -cne $env:GITHUB_SHA' in workflow
print('PASS: sensitive-material scanner positive/negative/ZIP redaction and workflow pin/provenance gates')
