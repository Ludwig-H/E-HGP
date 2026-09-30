"""One-time closure of this new packet; no native execution or old receipt write."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys

sys.dont_write_bytecode = True
root = Path(__file__).resolve().parent
commands = []
for mode, flags in (('normal', []), ('optimized', ['-O'])):
    argv = [sys.executable, '-B', *flags, str(root/'verify.py'), '--inner-only']
    result = subprocess.run(argv, capture_output=True, text=True, check=False, timeout=30)
    for stream in ('stdout', 'stderr'):
        with (root/('reader_'+mode+'.'+stream)).open('x') as file:
            file.write(getattr(result, stream))
    commands.append(dict(argv=argv, mode=mode, exit_code=result.returncode))
    if result.returncode != 0 or result.stderr:
        raise ValueError('reader closure refused: '+mode)
with (root/'closure.json').open('x') as file:
    file.write(json.dumps(dict(status='PASS', commands=commands, native_executions=0,
                               GCP_used=False), sort_keys=True, indent=2)+'\n')
files = sorted(path for path in root.rglob('*') if path.is_file())
with (root/'SHA256SUMS').open('x') as file:
    file.write(''.join(hashlib.sha256(path.read_bytes()).hexdigest()+'  '+
                       str(path.relative_to(root))+'\n' for path in files))
print(json.dumps(dict(status='CLOSED', files=len(files), commands=commands), sort_keys=True))
