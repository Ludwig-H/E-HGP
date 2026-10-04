from pathlib import Path
import hashlib
import json
import subprocess
import sys

ROOT = Path(__file__).resolve().parent
def need(ok, message):
    if not ok:
        raise SystemExit(message)

def inventory(folder):
    recorded = {}
    for line in (folder / 'SHA256SUMS').read_text().splitlines():
        digest, name = line.split('  ', 1)
        rel = Path(name)
        need(not rel.is_absolute() and '..' not in rel.parts, 'invalid inventory path')
        need(name not in recorded, 'duplicate inventory path')
        recorded[name] = digest
    actual = {}
    for path in sorted(folder.rglob('*')):
        need(not path.is_symlink(), 'symlink payload')
        if path.is_file() and path != folder / 'SHA256SUMS':
            actual[str(path.relative_to(folder))] = hashlib.sha256(path.read_bytes()).hexdigest()
    need(actual == recorded, 'inventory/content mismatch: ' + folder.name)

inventory(ROOT)
for name in ('tower', 'qb', 'evidence'):
    inventory(ROOT / name)
before = json.loads((ROOT / 'io/BEFORE.json').read_text())
after = json.loads((ROOT / 'io/AFTER.json').read_text())
need(before['sources'] == after['sources'] and after['same_captured_sources'], 'io review drift')
for name, digest in before['sources'].items():
    need(hashlib.sha256((ROOT / 'io/source' / name).read_bytes()).hexdigest() == digest, 'io copied source pin')

jobs = [
    ('tower', 'check_gap.py', 'gap_normal.json', 'gap_optimized.json', 77),
    ('qb', 'check.py', 'normal.json', 'optimized.json', 6300),
    ('evidence', 'check_identity_and_reader.py', 'stdout_normal.json', 'stdout_optimized.json', 79),
]
counts = {}
for name, script, normal, optimized, expected in jobs:
    outputs = []
    for flags, saved in (([], normal), (['-O'], optimized)):
        argv = [sys.executable, '-B', '-S'] + flags + [script]
        result = subprocess.run(argv, cwd=ROOT / name, capture_output=True, timeout=45)
        need(result.returncode == 0 and not result.stderr, 'child replay refused: ' + name)
        need(result.stdout == (ROOT / name / saved).read_bytes(), 'child output mismatch: ' + name)
        count = json.loads(result.stdout)['checks']
        need(count == expected, 'child guard count mismatch: ' + name)
        outputs.append(result.stdout)
    need(outputs[0] == outputs[1], 'normal/optimized differ: ' + name)
    counts[name] = expected

report = {'status': 'PASS', 'child_guards': counts, 'total_child_guards': sum(counts.values()),
          'normal_optimized_identical': True, 'native_executed': False,
          'scope': 'closed bounded stdlib/Fraction/AST capsules; no native or cloud qualification'}
need(report == json.loads((ROOT / 'RESULTS.json').read_text()), 'parent result mismatch')
print(json.dumps(report, sort_keys=True, indent=2))
