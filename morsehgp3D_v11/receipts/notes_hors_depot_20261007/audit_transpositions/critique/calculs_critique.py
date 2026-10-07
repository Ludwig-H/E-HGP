#!/usr/bin/env python3
"""Recalcul des chiffres de CRITIQUE_COMPLETUDE.md (critique de completude de l'audit des transpositions v11).

Lecture seule, bibliotheque standard seule. Sources :
  - archives G4 versees (git show origin/main:..., worktree build/v11-claude-20261003) : claudeab1, claudeab4,
    claudeab7, claudeprof1, pts4_review_20261003 ;
  - sessions G4 non versees, lues en place dans /workspaces/.ehgp-sessions/ : claudecat1 (3 oct. 19 h 45 UTC),
    claudeab8 (4 oct. 13 h 36 - 14 h 08 UTC).
Rien n'est ecrit sur disque ; les archives sont lues en memoire. Usage : python3 -B calculs_critique.py
"""
import gzip
import io
import json
import math
import os
import statistics as st
import subprocess
import sys
import tarfile

WT = '/workspaces/E-HGP/build/v11-claude-20261003'
SESS = '/workspaces/.ehgp-sessions'
PIPE = 'morsehgp3D_v11/receipts/developpement_20261003/pipeline_g4/sessions'
NGS = ('ng00', 'ng01', 'ng02')


def git_bytes(path):
    return subprocess.run(['git', '-C', WT, 'show', 'origin/main:' + path], check=True,
                          stdout=subprocess.PIPE).stdout


def phases(text):
    out = {}
    for line in text.splitlines():
        line = line.strip()
        if line.startswith('{'):
            obj = json.loads(line)
            out[obj.get('phase')] = obj
    return out


class Archive:
    """Prises t_<bras>_lidar_<trame>_w<W>_r<i>.stdout d'une archive versee ou d'un dossier local."""

    def __init__(self, tar_bytes=None, folder=None):
        self.files = {}
        if tar_bytes is not None:
            with tarfile.open(fileobj=io.BytesIO(tar_bytes), mode='r:gz') as tar:
                for member in tar.getmembers():
                    base = os.path.basename(member.name)
                    if member.isfile() and base.endswith('.stdout'):
                        self.files[base] = tar.extractfile(member).read().decode()
        else:
            for base in os.listdir(folder):
                if base.endswith('.stdout'):
                    with open(os.path.join(folder, base)) as handle:
                        self.files[base] = handle.read()

    def take(self, name):
        doc = phases(self.files[name])
        full, dom = doc['full'], doc['domain']
        return dict(full=full['wall_ns'] / 1e6, sp=dom['single_pass_ns'] / 1e6, prefix=dom.get('prefix_ns', 0) / 1e6,
                    reg=full['phases']['regular_ns'] / 1e6, pub=full['phases']['publish_ns'] / 1e6,
                    forest=full['forest_ns'] / 1e6, cpu=full['cpu_seconds'], status=full['status'],
                    lanes=full.get('parallel', {}).get('descent_lanes'))

    def arm(self, arm, ng, w='w48', reps=5):
        return [self.take(f't_{arm}_lidar_{ng}_{w}_r{r}.stdout') for r in range(reps)]


def med(rows, key):
    return st.median(row[key] for row in rows)


def fmt(values):
    return '[' + ', '.join(f'{v:.1f}' for v in values) + ']'


def fit(pairs):
    xs = [math.log(a) for a, _ in pairs]
    ys = [math.log(b) for _, b in pairs]
    mx, my = st.mean(xs), st.mean(ys)
    return sum((x - mx) * (y - my) for x, y in zip(xs, ys)) / sum((x - mx) ** 2 for x in xs)


def main():
    ab = {s: Archive(git_bytes(f'{PIPE}/{s}/results.tar.gz')) for s in ('claudeab1', 'claudeab4', 'claudeab7')}
    cat1 = Archive(folder=f'{SESS}/v11.20261003.claudecat1/results/extracted/results/cmd/000_ab/files')
    ab8 = Archive(folder=f'{SESS}/v11.20261004.claudeab8/results/extracted/results/cmd/000_ab/files/ab')

    print('1. Meme binaire de base a45daff3a (sha256 0089f43e... dans les trois ab_report.json), W48, cinq prises')
    for ng in NGS:
        meds = {s: med(ab[s].arm('base', ng), 'full') for s in ab}
        print(f'   {ng} medianes FULL par session {{{", ".join(f"{s}: {v:.1f}" for s, v in meds.items())}}} '
              f'max/min {max(meds.values()) / min(meds.values()):.3f}')
    print('   resolution reguliere de la base, 15 prises (bimodale) :')
    for ng in NGS:
        regs = sorted(r['reg'] for s in ab for r in ab[s].arm('base', ng))
        print(f'   {ng} {fmt(regs)}')
    print('   passe unique de la base, 15 prises :')
    for ng in NGS:
        sps = sorted(r['sp'] for s in ab for r in ab[s].arm('base', ng))
        print(f'   {ng} {fmt(sps)}')

    print('2. Moteur b87285378 (bras new de claudeab7, bras base de claudecat1 : meme binaire f5513318...)')
    for ng in NGS:
        rows = ab['claudeab7'].arm('new', ng) + cat1.arm('base', ng)
        full = [r['full'] for r in rows]
        print(f'   {ng} n={len(rows)} FULL min {min(full):.1f} med {st.median(full):.1f} max {max(full):.1f} '
              f'sd(log) {st.pstdev(math.log(x) for x in full):.3f} ; passe unique {min(r["sp"] for r in rows):.0f}-'
              f'{max(r["sp"] for r in rows):.0f}')

    print('3. Resolution reguliere W48 : voie liee sans pipeline (claudeab1 new) contre pipeline (claudeab4/7 new)')
    for ng in NGS:
        flat = ab['claudeab1'].arm('new', ng)
        piped = ab['claudeab4'].arm('new', ng) + ab['claudeab7'].arm('new', ng)
        print(f'   {ng} sans pipeline {fmt(sorted(r["reg"] for r in flat))} (lanes {flat[0]["lanes"]}), forets med '
              f'{med(flat, "forest"):.1f} ; pipeline {fmt(sorted(r["reg"] for r in piped))}, forets med '
              f'{med(piped, "forest"):.1f}')
    print('   prises sans queue de publication (< 5 ms), bras new de claudeab4/7 :')
    for ng in NGS:
        piped = ab['claudeab4'].arm('new', ng) + ab['claudeab7'].arm('new', ng)
        low = [(round(r['full'], 1), round(r['pub'], 1)) for r in piped if r['pub'] < 5]
        print(f'   {ng} {len(low)}/10 {low} ; queue min {min(r["pub"] for r in piped):.1f} ms')

    print('4. claudecat1 (non versee) : filtre G1 binary64 sans branche + compteurs de feuille, contre b87285378')
    for ng in NGS:
        b1, n1 = cat1.arm('base', ng, 'w1', 1)[0], cat1.arm('new', ng, 'w1', 1)[0]
        b, n = cat1.arm('base', ng), cat1.arm('new', ng)
        print(f'   {ng} W1 FULL {b1["full"]:.0f} -> {n1["full"]:.0f} ({n1["full"] / b1["full"]:.3f}), passe unique '
              f'{b1["sp"]:.0f} -> {n1["sp"]:.0f} ({n1["sp"] / b1["sp"]:.3f}), preambule {b1["prefix"]:.1f} -> '
              f'{n1["prefix"]:.1f} ({n1["prefix"] / b1["prefix"]:.2f}) ; W48 med FULL {med(b, "full"):.1f} -> '
              f'{med(n, "full"):.1f}')

    print('5. claudeab8 (non versee, terminee 14 h 08) : base 17514012b, q3 = 56216392e, new = lemme R')
    for ng in NGS:
        row = {a: ab8.arm(a, ng, 'w1', 1)[0] for a in ('base', 'q3', 'new')}
        print(f'   {ng} W1 passe unique base {row["base"]["sp"]:.0f} q3 {row["q3"]["sp"]:.0f} '
              f'({row["q3"]["sp"] / row["base"]["sp"] - 1:+.1%}) new {row["new"]["sp"]:.0f} '
              f'(lemme R seul {row["new"]["sp"] / row["q3"]["sp"] - 1:+.1%}) ; FULL W1 new/base '
              f'{row["new"]["full"] / row["base"]["full"] - 1:+.1%}')
        b, q, n = ab8.arm('base', ng), ab8.arm('q3', ng), ab8.arm('new', ng)
        rq = st.median(q[i]['full'] / b[i]['full'] for i in range(5))
        rn = st.median(n[i]['full'] / b[i]['full'] for i in range(5))
        print(f'      W48 medianes FULL base {med(b, "full"):.1f} q3 {med(q, "full"):.1f} new {med(n, "full"):.1f} ; '
              f'medianes des rapports apparies q3/base {rq:.3f} new/base {rn:.3f}')
    every = []
    for arch, arms in ((ab['claudeab4'], ('base', 'new')), (ab['claudeab7'], ('base', 'new')),
                       (cat1, ('base', 'new')), (ab8, ('base', 'q3', 'new'))):
        for a in arms:
            for ng in NGS:
                every += [(r['full'], a, ng) for r in arch.arm(a, ng) if r['status'] == 'ok']
    every.sort()
    print('   plus courtes prises FULL W48 statut ok (claudeab4/7, claudecat1, claudeab8) :',
          [(round(f, 1), a, ng) for f, a, ng in every[:4]])

    print('6. claudeprof1 : -march, W48, trois prises non appariees, CPU du processus (s)')
    prof = Archive(git_bytes(f'{PIPE}/claudeprof1/results.tar.gz'))
    for ng in NGS:
        cpu = {a: [prof.take(f't_{a}_lidar_{ng}_r{r}.stdout')['cpu'] for r in range(3)] for a in ('base', 'v3', 'v4')}
        print(f'   {ng} ' + ' ; '.join(f'{a} {min(v):.2f}-{max(v):.2f}' for a, v in cpu.items()))

    print('7. PTS4 (claudepts4, K1..10, 4 fils par processus, 22 processus simultanes, hors contrat)')
    blob = gzip.decompress(git_bytes('morsehgp3D_v11/receipts/pts4_review_20261003/case_metadata.json.gz'))
    rows = {}
    for sess_rows in json.loads(blob).values():
        for rec in sess_rows:
            name = rec['member'].split('/')[-1].replace('.json', '')
            exp = json.loads(rec['json_text']).get('export', {})
            if '/lidar' in rec['member'] and name.startswith('c08_') and exp.get('status') == 'ok':
                nodes5 = next((o['nodes'] for o in exp.get('orders', []) if isinstance(o, dict) and o.get('k') == 5), None)
                rows[name] = (exp['sites'], exp['balls'], exp['full_ns'] / 1e9, nodes5)
    vals = list(rows.values())
    print(f'   trames c08 : {len(vals)} ; > 60 000 sites : {sum(1 for v in vals if v[0] > 60000)}')
    print(f'   boules ~ n^{fit([(v[0], v[1]) for v in vals]):.2f} ; noeuds ordre 5 ~ n^{fit([(v[0], v[3]) for v in vals]):.2f} ;'
          f' full_ns ~ n^{fit([(v[0], v[2]) for v in vals]):.2f} ; full_ns ~ boules^{fit([(v[1], v[2]) for v in vals]):.2f}')
    ref = [v[2] for v in vals if 38000 <= v[0] <= 42000]
    band = {k: v for k, v in rows.items() if 50000 <= v[0] <= 60000}
    print(f'   temps median des trames de 38-42 k sites : {st.median(ref):.1f} s (n = {len(ref)}) ; 50-60 k : '
          + ', '.join(f'{k} {v[2]:.1f} s' for k, v in sorted(band.items()))
          + f' ; rapport median {st.median(v[2] for v in band.values()) / st.median(ref):.2f}, max '
          f'{max(v[2] for v in band.values()) / st.median(ref):.2f}')
    return 0


if __name__ == '__main__':
    sys.exit(main())
