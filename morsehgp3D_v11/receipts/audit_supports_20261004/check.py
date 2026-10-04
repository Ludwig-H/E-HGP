"""Replay the closed portable supports audit; never invoke native/cloud tools."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parent


def need(condition, message):
    if not condition:
        raise RuntimeError(message)


def inventory(directory):
    expected = {}
    for line in (directory / 'SHA256SUMS').read_text().splitlines():
        digest, name = line.split('  ', 1)
        relative = Path(name)
        need(not relative.is_absolute() and '..' not in relative.parts, 'invalid inventory path')
        need(name not in expected, 'duplicate inventory entry')
        expected[name] = digest
    actual = {p.relative_to(directory).as_posix() for p in directory.rglob('*')
              if p.is_file() and p != directory / 'SHA256SUMS'}
    need(actual == set(expected), 'inventory membership changed')
    for name, digest in expected.items():
        need(hashlib.sha256((directory / name).read_bytes()).hexdigest() == digest, 'inventory hash changed: ' + name)
    return len(expected)


inventory(ROOT)
source = json.loads((ROOT / 'SOURCE.json').read_text())
tasks = [('carrier', 'check.py', 'normal.stdout', 75),
         ('qb', 'check.py', 'normal.json', 1090),
         ('plateau', 'check.py', 'normal.json', 139),
         ('incidences', 'check_incidences.py', 'stdout_normal.json', 61)]
checks = 0
for name, script, saved, expected in tasks:
    directory = ROOT / name
    need(hashlib.sha256((directory / 'SHA256SUMS').read_bytes()).hexdigest() == source['children'][name], 'child closure changed')
    inventory(directory)
    for flags in ([], ['-O']):
        run = subprocess.run([sys.executable, '-B', *flags, '-S', str(directory / script)], cwd=directory,
                             capture_output=True, timeout=30)
        need(run.returncode == 0 and not run.stderr, 'portable replay failed: ' + name)
        need(run.stdout == (directory / saved).read_bytes(), 'portable replay changed: ' + name)
        need(json.loads(run.stdout)['checks'] == expected, 'check count changed: ' + name)
    checks += expected

for flags in ([], ['-O']):
    run = subprocess.run([sys.executable, '-B', *flags, '-S', str(ROOT / 'carrier' / 'mutant_nonstrict.py')],
                         capture_output=True, timeout=30)
    need(run.returncode == 1 and b'unperturbed carrier is two diameters' in run.stderr,
         'non-strict support mutation was not rejected causally')

print(json.dumps({'status': 'ok', 'portable_checks': checks, 'children': len(tasks),
                  'scalar_mutation_rejected': True, 'native_executed': False}, sort_keys=True))
