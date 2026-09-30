"""Read-only exact oracle and archived native judges. Also works under -O."""
import hashlib
import json
import sys
from pathlib import Path
import reference as r

ROOT = Path(__file__).resolve().parent


def cases():
    for name, d, ms, zs in r.fixtures():
        a = r.expand(d)
        for m in ms:
            for z in zs:
                for version, tree in [('original', d), ('expanded', a)]:
                    # Both selection methods and root modes on the three
                    # targeted API controls; random native rows only EOM/root.
                    options = [(True, 'eom')]
                    if not name.startswith('random_'):
                        options = [(single, method) for single in (False, True) for method in ('eom', 'leaf')]
                    for single, method in options:
                        yield (name, m, z, version, single, method), tree


def input_text(rows):
    lines = [str(len(rows))]
    for (name, m, z, version, single, method), d in rows:
        levels = sorted(set(d['birth']) | set(d['entry']))
        rank = {beta: i for i, beta in enumerate(levels)}
        off, val = [0], []
        for cs in d['children']:
            val.extend(cs); off.append(len(val))
        lines.append('%d %d %d %d %d %d %d %d' %
                     (len(levels), len(d['birth']), len(val), len(d['target']), m, z, single, method == 'leaf'))
        for v in ([float(beta) for beta in levels], [rank[b] for b in d['birth']], off, val,
                  [4294967295 if p is None else p for p in d['parent']], d['target'],
                  [rank[e] for e in d['entry']], d['weight']):
            lines.append(' '.join(map(str, v)))
    return '\n'.join(lines)+'\n'


def native_tree(row):
    n = len(row['parent'])
    r.require(all(len(row[k]) == n for k in ('birth', 'stability', 'mass')), 'native cluster arrays')
    return dict(clusters=[dict(parent=None if row['parent'][c] == 4294967295 else row['parent'][c],
                birth=row['birth'][c], stability=row['stability'][c], mass=row['mass'][c]) for c in range(n)],
                point_cluster=row['point_cluster'], point_lambda=row['point_lambda'])


def near(a, b):
    from math import isfinite
    return type(a) in (int, float) and isfinite(a) and abs(a-float(b)) <= 2e-12*max(1,abs(float(b)))


def same_numeric(actual, truth):
    a, ap, al = r.canonical(actual)
    b, bp, bl = r.canonical(truth)
    r.require(set(a) == set(b) and ap == bp and len(al) == len(bl), 'native topology/membership')
    for key, row in a.items():
        expected = b[key]
        r.require(row[0] == expected[0] and row[2] == expected[2] and
                  near(row[1], expected[1]) and near(row[3], expected[3]), 'native birth/mass/stability')
    r.require(all(near(x,y) for x,y in zip(al,bl)), 'native point exits')


def label_blocks(labels):
    blocks, noise = {}, []
    for x, c in enumerate(labels):
        (noise if c == -1 else blocks.setdefault(c, [])).append(x)
    return sorted(tuple(xs) for xs in blocks.values()), tuple(noise)


def judge():
    exact_rows = cut_checks = old_diff = 0
    named = []
    for name, d, ms, zs in r.fixtures():
        a = r.expand(d)
        for beta in sorted(set(d['birth']) | set(d['entry'])):
            for closed in (True, False):
                r.require(r.partition(d,beta,closed) == r.partition(a,beta,closed), 'point-cut conservation')
                cut_checks += 1
        for m in ms:
            for z in zs:
                truth = r.oracle(d,m,z)
                expanded = r.static_head(a,m,z)
                r.require(r.canonical(truth) == r.canonical(expanded), 'cohort expansion exact mismatch')
                for single in (False, True):
                    for method in ('eom', 'leaf'):
                        r.require(r.selected(truth,single,method) == r.selected(expanded,single,method), 'exact selection')
                diff = r.canonical(truth) != r.canonical(r.static_head(d,m,z))
                old_diff += diff
                if not name.startswith('random_'):
                    named.append(dict(case=name,mcs=m,z=z,old_differs=diff,
                        exact_stabilities=[str(c['stability']) for c in truth['clusters']],
                        exact_point_lambda=list(map(str,truth['point_lambda'])),
                        exact_eom_without_root=r.selected(truth,False,'eom')))
                exact_rows += 1
    # Causal control: preserving cohort/parent equal ranks yields incorrect
    # static mass selection even though all closed cuts are unchanged.
    mutant_d = next(d for name,d,_,_ in r.fixtures() if name == 'cohort_at_geometric_split_API')
    bad = r.expand(mutant_d,flatten=False)
    r.require(r.canonical(r.static_head(bad,2,2)) != r.canonical(r.oracle(mutant_d,2,2)), 'plateau mutant survived')
    rows = list(cases())
    r.require((ROOT/'native.stdin').read_text() == input_text(rows), 'native command/input identity')
    r.require(json.loads((ROOT/'case_identity.json').read_text()) == [list(key) for key,d in rows],
              'native case identity')
    native_results = {}
    for backend in ('release','ubsan'):
        output = [json.loads(line) for line in (ROOT/(backend+'.stdout')).read_text().splitlines()]
        r.require(len(output) == len(rows), 'native row count')
        targeted_labels = 0
        for (key,d), row in zip(rows,output):
            name,m,z,version,single,method = key
            same_numeric(native_tree(row),r.static_head(d,m,z))
            if not name.startswith('random_'):
                r.require(label_blocks(row['label']) == r.selected(r.static_head(d,m,z),single,method), 'targeted native selection')
                targeted_labels += 1
        native_results[backend] = dict(rows=len(rows),targeted_selection_rows=targeted_labels)
    # This is a native wrong-value control compiled from unchanged published
    # head, fed the intentionally unflattened adapter output.
    bad_row = json.loads((ROOT/'plateau_mutant.stdout').read_text())
    expected_mutant_input = input_text([(('plateau_mutant',2,2,'unflattened',False,'eom'),bad)])
    r.require((ROOT/'plateau_mutant.stdin').read_text() == expected_mutant_input, 'plateau input identity')
    same_numeric(native_tree(bad_row),r.static_head(bad,2,2))
    mutant_expected = r.selected(r.oracle(mutant_d,2,2),False,'eom')
    mutant_actual = label_blocks(bad_row['label'])
    r.require(mutant_expected == ([],tuple(range(6))) and
              mutant_actual == ([(0,1,2),(3,4,5)],()), 'native plateau wrong-value control')
    return dict(status='COHORT_REFERENCE_PASS',exact_condensations=exact_rows,exact_cut_checks=cut_checks,
                original_static_differences=old_diff,exact_selection_checks=4*exact_rows,
                native=native_results,plateau_wrong_value_exit_code=0,
                plateau_native_labels=mutant_actual,plateau_exact_labels=mutant_expected,named=named,
                scope='fixed API trees and one archived K3 geometric cover; no new native generator, vote or statistics')


if __name__ == '__main__':
    ledger = ROOT/'SHA256SUMS'
    if ledger.exists():
        for line in ledger.read_text().splitlines():
            expected, rel = line.split('  ',1)
            r.require(hashlib.sha256((ROOT/rel).read_bytes()).hexdigest() == expected, 'archive hash '+rel)
    print(json.dumps(judge(),sort_keys=True,indent=2))
