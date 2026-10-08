#!/usr/bin/env python3
"""Temoin abstrait d'historique altere : preuve par non-lecture, aucun C++ execute."""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import re
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent
N = (1 << 32) - 1
UNREAD = ('attach_rank', 'survivor_events', 'event_rank', 'event_node', 'event_cell')


def require(ok, message):
    if not ok:
        raise ValueError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def fixture():
    return dict(k=1, births=2, root=2, birth_key=[0, 1], birth_node=[0, 1],
                rank=[0, 0, 1], parent=[2, 2, N], minleaf=[0, 1, 0], children=([0, 0, 0, 2], [0, 1]),
                lower=[], cell_node=[2], event_cell=[0], retained_cell=[0], retained_ball=[0],
                retained_rank=[1], branches=([0, 2], [0, 1]), attach_parent=[N, 0], attach_rank=[0, 1],
                survivor_events=([0, 1, 1], [0]), event_rank=[1], event_node=[2])


def csr(off, val, rows):
    return len(off) == rows + 1 and off[0] == 0 and off[-1] == len(val) and all(a <= b for a, b in zip(off, off[1:]))


def parent_cut(f, leaf, rank):
    while f['parent'][leaf] != N and f['rank'][f['parent'][leaf]] <= rank:
        leaf = f['parent'][leaf]
    return leaf


def history_cut(f, leaf, rank):
    require(leaf < f['births'] and f['rank'][leaf] <= rank, 'requete hors domaine')
    x = leaf
    while f['attach_parent'][x] != N and f['attach_rank'][x] <= rank:
        x = f['attach_parent'][x]
    off, val = f['survivor_events']
    begin, end = off[x], off[x + 1]
    lo, hi = begin, end
    while lo < hi:
        mid = lo + (hi - lo) // 2
        if f['event_rank'][val[mid]] <= rank:
            lo = mid + 1
        else:
            hi = mid
    return x if lo == begin else f['event_node'][val[lo - 1]]


def proposed_guard(f):
    nb, nn = f['births'], len(f['rank'])
    ne = nb - 1
    off, val = f['survivor_events']
    return (len(f['attach_rank']) == nb and all(len(f[k]) == ne for k in ('event_cell', 'event_rank', 'event_node'))
            and len(val) == ne and csr(off, val, nb)
            and all(nb <= v < nn and f['rank'][v] == f['event_rank'][e]
                    and f['event_cell'][e] < len(f['cell_node']) for e, v in enumerate(f['event_node']))
            and all(e < ne for e in val))


def proof(source):
    text = (source / 'src/tower/forest_validate.cpp').read_text()
    text = re.sub(r'//[^\n]*|/\*.*?\*/', '', text, flags=re.S)
    require(all(re.search(r'\b' + k + r'\b', text) is None for k in UNREAD), 'historique maintenant lu')
    fields = set(re.findall(r'\b(?:f|up|low)\.(\w+)', text)) - {'nodes'}
    require(fields <= set(fixture()), 'lecture non modelisee')
    fields.add('rank')  # OrderForest::nodes() = rank.size().
    base = fixture()
    # Verification directe de ce seul arbre : deux feuilles de rang 0, racine 2 de rang 1,
    # deux aretes coherentes et triees ; profondeur d'attache 1 = floor(log2(2)), branche ouverte {0,1}.
    require(base['parent'] == [2, 2, N] and base['rank'] == [0, 0, 1] and base['minleaf'] == [0, 1, 0]
            and csr(*base['children'], 3) and base['children'] == ([0, 0, 0, 2], [0, 1])
            and base['attach_parent'] == [N, 0] and csr(*base['branches'], 1)
            and base['branches'] == ([0, 2], [0, 1]) and not base['lower'], 'temoin de base')
    cases = [('base', base, 0)]
    empty = copy.deepcopy(base)
    for key in ('event_cell', 'event_rank', 'event_node'):
        empty[key] = []
    empty['survivor_events'] = ([0, 0, 0], [])
    cases.append(('evenements_vides_csr_bien_formee', empty, 0))
    leaf = copy.deepcopy(base)
    leaf['event_node'][0] = 0
    cases.append(('evenement_vers_naissance', leaf, 0))
    late = copy.deepcopy(base)
    late['attach_rank'][1] = 2
    cases.append(('attache_tardive_residuelle', late, 1))
    results = []
    for name, f, leaf in cases:
        unchanged = all(f[k] == base[k] for k in fields)
        require(unchanged, 'mutation lue par le validateur')
        actual, expected = history_cut(f, leaf, 1), parent_cut(f, leaf, 1)
        results.append(dict(case=name, checked_projection_unchanged=unchanged,
                            event_count=len(f['event_cell']), required_events=1, query=[leaf, 1],
                            history_result=actual, parent_cut_result=expected,
                            proposed_guard=proposed_guard(f)))
    require([r['history_result'] for r in results] == [2, 0, 0, 1] and
            [r['proposed_guard'] for r in results] == [True, False, False, True], 'modele different')
    return dict(unread_fields=list(UNREAD), observed_fields=sorted(fields), cases=results,
                current_validator_acceptance='inference statique sur projection inchangee',
                cpp_executed=False, product_output_corruption_observed=False, gcp_used=False)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--prototype', type=Path, required=True)
    args = parser.parse_args()
    cap = json.loads((HERE / 'capture.json').read_text())
    source = args.prototype / 'repo5/morsehgp3D_v12'
    for rel, expected in cap['files_sha256'].items():
        require(sha(source / rel) == expected, 'source modifiee : ' + rel)
    result = proof(source)
    with tempfile.TemporaryDirectory(prefix='audit-forest-guard-') as d:
        root = Path(d)
        rel = Path('morsehgp3D_v12/src/tower/forest_validate.cpp')
        (root / rel).parent.mkdir(parents=True)
        (root / rel).write_bytes((source / 'src/tower/forest_validate.cpp').read_bytes())
        for flags in (['--check'], []):
            subprocess.run(['git', 'apply', *flags, str(HERE / 'gardes_minimales.patch')], cwd=root, check=True)
        require(sha(root / rel) == cap['proposed_validate_sha256'], 'patch different')
    require(result == cap['result'], 'resultat different')
    for rel, expected in cap['files_sha256'].items():
        require(sha(source / rel) == expected, 'source modifiee pendant lecture : ' + rel)
    print(json.dumps(result, sort_keys=True, indent=2))


if __name__ == '__main__':
    main()
