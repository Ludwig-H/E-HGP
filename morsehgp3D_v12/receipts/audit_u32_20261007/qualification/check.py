#!/usr/bin/env python3
"""Lecture bornée Git et JSON ; aucune construction, campagne ou donnée réelle."""
import hashlib
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[3]
REPO = ROOT.parent
PIN = 'c3de9d73d8999f2f1e31a0f592b829efc6e7a4da'
PORT_FIX = '95247cf4b'
RENAMES = (('mhgp11', 'mhgp12'), ('MHGP11', 'MHGP12'),
           ('hgp11_', 'hgp12_'), ('ehgp.v11.', 'ehgp.v12.'))
ANCHORS = (
    'docs/PORTS.md', 'docs/PROVENANCE.md', 'CMakeLists.txt',
    'reference/tests.cmake', 'reference/test_dump_v10.py', 'cmake/gates.cmake',
    'cmake/run_expect.cmake', 'tests/mutants/run_mutants.py',
    'receipts/developpement_20261007/numerique_socle.md',
    'receipts/developpement_20261007/numerique_socle_RAPPORT.md',
    'receipts/developpement_20261007/numerique_socle_CHANGEMENTS.md',
    'receipts/audit_claude_socle_t0_20261007/verifier_ports.py',
) + tuple('tests/mutants/' + x + '.json' for x in ('core', 'num', 'index', 'cloud', 'io', 'sched'))


def git(*args):
    return subprocess.check_output(['git', '-C', str(REPO), *args])


def show(pin, path):
    return git('show', pin + ':' + path)


def require(condition, reason):
    if not condition:
        raise RuntimeError(reason)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def anchors():
    result = {}
    for path in ANCHORS:
        data = (ROOT / path).read_bytes()
        require(data == show(PIN, 'morsehgp3D_v12/' + path), 'source non épinglée : ' + path)
        result[path] = sha(data)
    return result


def port_table(pin):
    counts = dict(rows=0, sha_ok=0, copies=0, renames=0, adapted=0)
    differences = []
    for line in show(pin, 'morsehgp3D_v12/docs/PORTS.md').decode().splitlines():
        if not line.startswith('| `'):
            continue
        cells = [c.strip() for c in line.strip('|').split('|')]
        if len(cells) < 4 or not cells[2].startswith('`'):
            continue
        new, old, digest, state = [c.strip('`') for c in cells[:4]]
        src = show('ac081a06f', 'morsehgp3D_v11/' + old)
        dst = show(pin, 'morsehgp3D_v12/' + new)
        counts['rows'] += 1
        require(sha(src) == digest, 'empreinte source divergente : ' + old)
        counts['sha_ok'] += 1
        if state.startswith('copie'):
            counts['copies'] += 1
            expected = src
        elif state.startswith('renommage seul'):
            counts['renames'] += 1
            text = src.decode()
            for a, b in RENAMES:
                text = text.replace(a, b)
            expected = text.encode()
        else:
            counts['adapted'] += 1
            continue
        if expected != dst:
            differences.append(new)
    return dict(counts=counts, differences=differences)


def socle_blobs(pin):
    result = {}
    prefix = 'morsehgp3D_v12/'
    for row in git('ls-tree', '-r', pin, prefix).decode().splitlines():
        meta, path = row.split('\t')
        path = path[len(prefix):]
        if path.startswith(('src/', 'tests/', 'cmake/', 'tools/', 'bench/index_io')) or path == 'CMakeLists.txt':
            result[path] = meta.split()[2]
    return result


def main():
    before = anchors()
    initial = port_table('a0091e2b7')
    fixed = port_table(PORT_FIX)
    current = port_table(PIN)
    require(initial['differences'] == ['reference/tests.cmake'], 'témoin CST-0115 changé')
    require(fixed['differences'] == [], 'correction historique CST-0115 non confirmée')
    require(show(PORT_FIX, 'morsehgp3D_v12/reference/tests.cmake') ==
            show(PIN, 'morsehgp3D_v12/reference/tests.cmake'), 'portes référence modifiées')
    x, y = socle_blobs('26b53648c'), socle_blobs('3d6c92c1f')
    require(len(x) == 191 and x == y, 'raccord de base divergent')
    manifests = {}
    for name in ('core', 'num', 'index', 'cloud', 'io', 'sched'):
        m = json.loads((ROOT / ('tests/mutants/' + name + '.json')).read_text())
        manifests[name] = dict(count=len(m['mutants']), floor=m['plancher'],
                               construction_expected=sum(x.get('attendu') == 'construction' for x in m['mutants']))
    changes = []
    for row in git('diff', '--numstat', '6a38f7e4b^', '6a38f7e4b', '--', 'morsehgp3D_v12').decode().splitlines():
        added, removed, path = row.split('\t')
        rel = path[len('morsehgp3D_v12/'):]
        if rel.startswith(('src/', 'tests/', 'cmake/', 'tools/', 'bench/index_io')) or rel in (
                'CMakeLists.txt', 'README.md', 'docs/ARCHITECTURE.md'):
            changes.append((int(added), int(removed), rel))
    require(len(changes) == 82, 'nombre de fichiers de la tranche changé')
    require(before == anchors(), 'sources modifiées pendant la lecture')
    out = dict(schema='audit_u32_qualification.v1', pin=PIN,
               source_sha256=before, checker_sha256=sha(Path(__file__).read_bytes()),
               ports_initial=initial, ports_fixed_95247cf4b=fixed, ports_current=current,
               base_26b53648c_equals_3d6c92c1f=dict(files=191, different=0),
               integrated_numeric_patch=dict(files=len(changes), added=sum(x[0] for x in changes),
                                             removed=sum(x[1] for x in changes)),
               mutant_manifests=manifests, executions_replayed=0,
               historical_logs_recovered=False, cst0024_cause_established=False,
               qualification_g4=False, hashes_closed=True)
    print(json.dumps(out, ensure_ascii=False, indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
