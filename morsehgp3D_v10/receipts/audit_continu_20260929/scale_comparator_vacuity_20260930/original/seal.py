import hashlib
import json
import pathlib
import shutil
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parent
sources = [pathlib.Path('/tmp/mhgp10-integ-r2/scratch/mut_scale_python3.log'),
           pathlib.Path('/tmp/mhgp10-integ-r2/scratch/mut_scale_python3.10.log')]
provenance = []
for source in sources:
    data = source.read_bytes()
    before = hashlib.sha256(data).hexdigest()
    if b'bilan mutants=23 tues=21/21 equivalents=2/2 temoin=survit motifs_absents=0' not in data or not data.endswith(b'code=0\n'):
        raise RuntimeError('subset is not terminal: ' + str(source))
    copy = ROOT / source.name
    shutil.copyfile(source, copy)
    after = hashlib.sha256(source.read_bytes()).hexdigest()
    if before != after or before != hashlib.sha256(copy.read_bytes()).hexdigest():
        raise RuntimeError('moving subset receipt')
    provenance.append({'source': str(source), 'copy': copy.name, 'sha256': before,
                       'scope': '23 scale mutations, 21 killed and 2 equivalents; not full 95-mutant campaign'})
(ROOT / 'subset_provenance.json').write_text(json.dumps(provenance, indent=2, sort_keys=True) + '\n')
paths = sorted(path for path in ROOT.rglob('*') if path.is_file() and path.name not in ['SHA256SUMS', 'verify_normal.txt', 'verify_optimized.txt'])
(ROOT / 'SHA256SUMS').write_text(''.join(hashlib.sha256(path.read_bytes()).hexdigest() + '  ' + path.relative_to(ROOT).as_posix() + '\n' for path in paths))
for name, flags in [('normal', []), ('optimized', ['-O'])]:
    result = subprocess.run([sys.executable, '-B'] + flags + [str(ROOT / 'verify.py')], capture_output=True, text=True, timeout=10)
    (ROOT / ('verify_' + name + '.txt')).write_text(result.stdout + result.stderr + 'code=%d\n' % result.returncode)
    print(result.stdout, end='')
    if result.returncode:
        raise RuntimeError('reader failed')
print('SEALED manifest=' + hashlib.sha256((ROOT / 'SHA256SUMS').read_bytes()).hexdigest())
