#!/usr/bin/env python3
"""Fixture abstraite T seulement ; ni execution C++, ni catalogue realise."""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import subprocess

HERE = Path(__file__).resolve().parent
NONE = 2**32 - 1
CELL = 2**31


def need(ok, why):
    if not ok:
        raise ValueError(why)


def fixture():
    rows = [[2] for _ in range(513)]
    rows[0], rows[1], rows[511], rows[512] = [2, 3], [2, 4], [0, 1], [0, 2]
    off = [0]
    for row in rows:
        off.append(off[-1] + len(row))
    return dict(k=1, birth_key=list(range(5)), birth_rank=[0]*5,
                cell_ball=list(range(100, 613)), cell_rank=list(range(1, 514)),
                rep_offsets=off, targets=[x for row in rows for x in row])


def validate(f):
    nb, nc = len(f['birth_key']), len(f['cell_ball'])
    need(0 < nb < CELL and nc < CELL and 1 <= f['k'] <= 12, 'domain')
    need(len(f['birth_rank']) == nb and len(f['cell_rank']) == nc, 'rank lengths')
    off = f['rep_offsets']
    need(len(off) == nc+1 and off[0] == 0 and off[-1] == len(f['targets']), 'offsets')
    for keys, ranks in ((f['birth_key'], f['birth_rank']), (f['cell_ball'], f['cell_rank'])):
        need(all(a < b for a, b in zip(keys, keys[1:])), 'keys')
        need(all(a <= b for a, b in zip(ranks, ranks[1:])), 'ranks')
    for t in range(nc):
        need(off[t] < off[t+1], 'empty cell')
        for code in f['targets'][off[t]:off[t+1]]:
            need(type(code) is int and 0 <= code < NONE, 'target code')
            x = code & (CELL-1)
            ranks = f['cell_rank'] if code & CELL else f['birth_rank']
            need(x < len(ranks) and ranks[x] < f['cell_rank'][t], 'target date/domain')
    return nb, nc


def rows(f):
    o = f['rep_offsets']
    return [f['targets'][o[t]:o[t+1]] for t in range(len(f['cell_ball']))]


def partition_oracle(f, hint):
    # Reference par ensembles, sans parents/tailles/attaches.
    blocks = [set([i]) for i in range(5)]
    cuts = {}
    for t, row in enumerate(rows(f)):
        target = ([hint, 1] if t == 511 else row)
        selected = [b for b in blocks if any(x in b for x in target)]
        blocks = [b for b in blocks if b not in selected] + [set.union(*selected)]
        if t in (1, 511, 512):
            cuts[t] = sorted(sorted(b) for b in blocks)
    return cuts


def kernel(f, hint):
    validate(f)
    up, size, top, minimum = list(range(5)), [1]*5, list(range(5)), list(range(5))
    attach, attach_rank, events, event_cells = [NONE]*5, [0]*5, [], []
    cuts, retained = {}, []

    def find(x):
        while up[x] != x:
            x = up[x]
        return x

    for t, row in enumerate(rows(f)):
        before = len(events)
        x = find(hint if t == 511 else row[0])
        for leaf in row[1:]:
            y = find(leaf)
            if x == y:
                continue
            need(len(events)+1 < 5, 'event overflow')
            survivor, loser = (y, x) if size[x] < size[y] else (x, y)
            events.append(dict(rank=f['cell_rank'][t], a=top[x], b=top[y],
                               minimum=min(minimum[x], minimum[y]), survivor=survivor))
            event_cells.append(t)
            up[loser] = survivor
            attach[loser], attach_rank[loser] = survivor, f['cell_rank'][t]
            size[survivor] += size[loser]
            minimum[survivor] = events[-1]['minimum']
            top[survivor] = CELL | (len(events)-1)
            x = survivor
        if len(events) != before:
            retained.append(t)
        if t in (1, 511, 512):
            cuts[t] = sorted(sorted(i for i in range(5) if find(i) == root)
                             for root in sorted({find(i) for i in range(5)}))
    need(len(events) == 4 and len({find(i) for i in range(5)}) == 1, 'final root')
    for e in events:
        a = events[e['a'] & (CELL-1)]['survivor'] if e['a'] & CELL else e['a']
        b = events[e['b'] & (CELL-1)]['survivor'] if e['b'] & CELL else e['b']
        loser = b if a == e['survivor'] else a
        need(attach[loser] == e['survivor'], 'history attachment')
    need(cuts == partition_oracle(f, hint), 'independent partition oracle')
    return dict(cuts=cuts, events=events, event_cells=event_cells, retained=retained,
                attach=attach, attach_rank=attach_rank, final_root=find(0))


def main(repo):
    cap = json.loads((HERE/'capture.json').read_text())
    for path, expected in cap['sources'].items():
        for pin in (cap['source_pin'], cap['publication_pin']):
            blob = subprocess.check_output(['git', '-C', str(repo), 'show', pin+':morsehgp3D_v12/'+path])
            need(hashlib.sha256(blob).hexdigest() == expected, 'source '+path)
    old = subprocess.check_output(['git', '-C', str(repo), 'show', cap['publication_pin']+':'+cap['prior_model_path']])
    need(hashlib.sha256(old).hexdigest() == cap['prior_model_sha256'], 'prior model')
    # Reutilisation du graphe deja publie : pas de nouveau modele C++ integral.
    ns = {'__name__': 'prior', '__file__': str(HERE/'prior_not_materialized.py')}
    exec(compile(old, cap['prior_model_path'], 'exec'), ns)
    graph, bridge = ns['verify'](), ns['verify'](leaf_bridge=True)
    need(graph['accepted'] and graph['events'] == 47 and not bridge['accepted'], 'prior graph')
    f = fixture()
    nb, nc = validate(f)
    good, bad = kernel(f, 0), kernel(f, 2)
    need(good['cuts'][511] != bad['cuts'][511], 'prefix differs')
    need(good['attach'][0] == bad['attach'][0] == 2, 'same future write')
    need(good['event_cells'] == bad['event_cells'] == [0, 1, 511, 512], 'padding inert')
    grain, window, kernel_slice, hint_next = 256, 32, 0, 0
    claimed = max(hint_next, kernel_slice+1)
    count = (nc+grain-1)//grain
    need(claimed == 1 and claimed < count and claimed <= kernel_slice+window, 'claim')
    need(claimed*grain <= 511 < (claimed+1)*grain and 512 == (claimed+1)*grain, 'boundary')
    # Petits controles negatifs des gardes dont depend le raccord.
    mutations = {}
    for name in ('empty', 'birth_date', 'target_domain', 'future_cell', 'no_target', 'disconnected'):
        m = copy.deepcopy(f)
        if name == 'empty': m['rep_offsets'][4] = m['rep_offsets'][3]
        elif name == 'birth_date': m['birth_rank'] = [1]*5
        elif name == 'target_domain': m['targets'][0] = 5
        elif name == 'future_cell': m['targets'][0] = CELL | 512
        elif name == 'no_target': m['targets'][0] = NONE
        else: m['targets'][-1] = 0
        try:
            kernel(m, 0)
        except ValueError:
            mutations[name] = 'refused'
        else:
            raise ValueError('mutation accepted '+name)
    return dict(births=nb, cells=nc, representatives=len(f['targets']),
                slice_sizes=[256, 256, 1], claimed_slice=claimed, processed_cells=0,
                hint_window=window, flags_assumed=['KernelOpen done', 'kernel busy', 'slice1 done+leaves', 'not closed'],
                reference=good, future_hint=bad, negative_controls=mutations,
                prior_graph_events=47, prior_relaxed_accepted=graph['accepted'],
                prior_leaf_bridge_rejected=not bridge['accepted'],
                native_execution=False, cpp_complete_execution_proved=False,
                geometric_catalogue_constructed=False)


if __name__ == '__main__':
    ap = argparse.ArgumentParser()
    ap.add_argument('repo', type=Path)
    ap.add_argument('--check', action='store_true')
    args = ap.parse_args()
    result = main(args.repo)
    encoded = json.dumps(result, indent=2, sort_keys=True)+'\n'
    if args.check:
        need(json.loads(encoded) == json.loads((HERE/'results.json').read_text()), 'stored result')
    print(encoded, end='')
