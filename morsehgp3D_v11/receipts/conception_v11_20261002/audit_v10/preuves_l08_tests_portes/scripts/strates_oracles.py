"""Mesure (lecture seule) des strates reellement exercees par les deux oracles de HEAD.
Rejoue les MEMES nuages (meme graine, meme generateur importe) et lit les compteurs JSON des CLI.
Usage : python3 strates_oracles.py BUILD_DIR SRC_V10
"""
import json
import os
import random
import subprocess
import sys
import tempfile
from collections import Counter, defaultdict

build, src = sys.argv[1], sys.argv[2]
sys.path.insert(0, os.path.join(src, 'reference'))
sys.path.insert(0, os.path.join(src, 'tests', 'oracle'))
import hgp10_ref as R  # noqa: E402
from test_catalogue_oracle import clouds  # noqa: E402


def write(P, path):
    with open(path, 'wb') as f:
        for p in P:
            for v in p:
                f.write(int(v).to_bytes(4, 'little'))


def last_json(out):
    js = [l for l in out.splitlines() if l.startswith('{')]
    return json.loads(js[-1]) if js else None


def main():
    res = {}
    with tempfile.TemporaryDirectory() as tmp:
        srcf = os.path.join(tmp, 'in.u32le')
        # ---- oracle catalogue : 40 nuages x K in (1,2,3,5)
        rnd = random.Random(20260928)
        cat = defaultdict(Counter)
        maxcoord = 0
        sizes = Counter()
        for t, P in enumerate(clouds(40, rnd)):
            kind = t % 4
            maxcoord = max(maxcoord, max(max(p) for p in P))
            sizes[len(P)] += 1
            for K in (1, 2, 3, 5):
                write(P, srcf)
                dump = os.path.join(tmp, 'd.txt')
                r = subprocess.run([os.path.join(build, 'mhgp10_catalogue'), srcf, '--k=%d' % K, '--threads=2',
                                    '--dump=' + dump], capture_output=True, text=True)
                js = last_json(r.stdout)
                c = cat[kind]
                c['checks'] += 1
                c['balls'] += js['balls']
                c['multi_leaf'] += js['leaves'] > 1
                c['leaves_max'] = max(c['leaves_max'], js['leaves'])
                c['nodes_max'] = max(c['nodes_max'], js['nodes'])
                c['extended'] += js['extended']
                c['max_shell'] = max(c['max_shell'], js['max_shell'])
                c['stalled_leaves'] += js['stalled_leaves']
                for q in ('q2', 'q3', 'q4'):
                    c[q] += sum(js['by_q_p'][q])
                c['K%d_multi_leaf' % K] += js['leaves'] > 1
        res['catalogue'] = {str(k): dict(v) for k, v in cat.items()}
        res['catalogue_max_coord'] = maxcoord
        res['catalogue_sizes'] = dict(sorted(sizes.items()))
        # ---- oracle tour : 24 nuages P[:12] x K in (1,3,5) + E5 K=4
        rnd = random.Random(20260929)
        tow = defaultdict(Counter)
        cases = [(t % 4, P[:12], K) for t, P in enumerate(clouds(24, rnd)) for K in (1, 3, 5)]
        cases.append(('E5', [(0, 0, 7), (0, 9, 6), (1, 4, 0), (0, 0, 1), (4, 1, 2)], 4))
        tsizes = Counter()
        for kind, P, K in cases:
            write(P, srcf)
            tsizes[len(P)] += 1
            dump = os.path.join(tmp, 't.txt')
            r = subprocess.run([os.path.join(build, 'mhgp10_tower'), srcf, '--k=%d' % K, '--threads=2',
                                '--dump=' + dump], capture_output=True, text=True)
            js = last_json(r.stdout)
            c = tow[str(kind)]
            c['checks'] += 1
            c['balls'] += js['balls']
            c['meb_fallbacks'] += js['stages']['meb_fallbacks']
            for part in ('join', 'point', 'vertical'):
                s = js['stages'][part]
                c[part + '_resolves'] += s['resolves']
                c[part + '_steps'] += s['steps']
                c[part + '_knn_jumps'] += s['knn_jumps']
                c[part + '_level_exact'] += s['level_exact']
                c[part + '_jump_exact'] += s['jump_exact']
                c[part + '_memo_hits'] += s['memo_hits']
                c[part + '_seed_hits'] += s['seed_hits']
                c[part + '_census_cat'] += s['census_cat']
                c[part + '_closed_balls'] += s['closed_balls']
            for o in js['orders']:
                c['nodes'] += o['nodes']
                c['births'] += o['births']
                c['merges'] += o['merges']
                c['joins'] += o['joins']
            # multifusions >= 3 enfants et noeuds par ordre, depuis le dump
            kids = None
            for line in open(dump):
                t = line.split()
                if t[0] == 'order':
                    if kids is not None:
                        c['multifusions_ge3'] += sum(1 for v in kids.values() if v >= 3)
                        c['fusions'] += len(kids)
                    kids = Counter()
                elif t[0] == 'node' and int(t[2]) >= 0:
                    kids[int(t[2])] += 1
            if kids is not None:
                c['multifusions_ge3'] += sum(1 for v in kids.values() if v >= 3)
                c['fusions'] += len(kids)
        res['tower'] = {k: dict(v) for k, v in tow.items()}
        res['tower_sizes'] = dict(sorted(tsizes.items()))
    print(json.dumps(res, indent=1, sort_keys=True))


main()
