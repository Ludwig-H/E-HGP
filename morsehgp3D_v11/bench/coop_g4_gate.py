#!/usr/bin/env python3
"""Porte G4 de la feuille cooperative (un warp par feuille ; note aux auditeurs, section O ; audit d117de397).

    python3 bench/coop_g4_gate.py --bench B_CUDA/mhgp11_full_bench --out DIR [--sanitizer compute-sanitizer]

Binaire construit avec MHGP11_ENABLE_CUDA et MHGP11_COORD_BITS=21. Deux nuages synthetiques (aucune donnee LiDAR) :
A = 3 000 sites uniformes du cube 2^16 (random.Random(20261004), celui de tests/tower/full_leaf_lanes.py) ; B = 400 sites
du cube 2^21 (random.Random(20261006)), dont des feuilles a K = 10 sont non resolues (chemins i128 refuses, repli CPU).
Configurations K = 5 feuilles de 16 et K = 10 feuilles de 24, quatre fils. Modes : cpu 16379, gpu 81915 (un fil par
feuille), gpu_coop 212987 (un warp par feuille), lot_coop 180219 (emulation hote). Exige : dumps et registres du
catalogue identiques au CPU ; lot GPU et lot cooperatif de memes taches, boules, incidences et non resolues ; des
non resolues sur B a K = 10 ; les deux chemins d'ecriture (cases copiees, feuilles rejouees) exerces. Puis
Compute Sanitizer (memcheck, racecheck, synccheck, voir SANITIZED) sur gpu_coop : zero erreur et meme dump.
Bibliotheque standard seule, aucune assertion. Codes : 0 conforme ; 1 refus.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import shutil
import struct
import subprocess
import sys
import time

MODES = {'cpu': 16379, 'gpu': 81915, 'gpu_coop': 212987, 'lot_coop': 180219}
CONFIGS = (('5', '16'), ('10', '24'))
# Compute Sanitizer : memcheck sur A a K = 5 (27 000 feuilles) et B a K = 10 ; racecheck et synccheck sur B (K = 5,
# puis K = 10 avec feuilles non resolues et seconde passe d'ecriture).
SANITIZED = (('memcheck', 'A', '5', '16'), ('memcheck', 'B', '10', '24'), ('racecheck', 'B', '5', '16'),
             ('racecheck', 'B', '10', '24'), ('synccheck', 'B', '5', '16'), ('synccheck', 'B', '10', '24'))


def refuse(reason, report, out):
    report['verdict'] = 'refus : ' + reason
    (out / 'coop_g4_gate.json').write_text(json.dumps(report, indent=1, sort_keys=True) + '\n')
    print('coop_g4_gate_verdict refus : ' + reason)
    sys.exit(1)


def phases(text):
    found = {}
    for line in text.splitlines():
        try:
            event = json.loads(line)
        except ValueError:
            continue
        if isinstance(event, dict) and 'phase' in event:
            found[event['phase']] = event
    return found


def cloud(root, name, seed, bits, sites):
    rng = random.Random(seed)
    points = sorted({(rng.getrandbits(bits), rng.getrandbits(bits), rng.getrandbits(bits)) for _ in range(sites)})
    xyz, ids = root / (name + '.xyz'), root / (name + '.ids')
    xyz.write_bytes(b''.join(struct.pack('<III', *p) for p in points))
    ids.write_bytes(b''.join(struct.pack('<I', i) for i in range(len(points))))
    return xyz, ids, len(points)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--bench', type=Path, required=True)
    ap.add_argument('--out', type=Path, required=True)
    ap.add_argument('--sanitizer', default='')
    args = ap.parse_args()
    out = args.out
    out.mkdir(parents=True, exist_ok=True)
    work = out / 'work'
    work.mkdir(exist_ok=True)
    report = {'schema': 'ehgp.v11.coop_g4_gate.v1', 'bench': str(args.bench), 'runs': [], 'sanitizer': []}
    if not args.bench.is_file():
        refuse('binaire absent', report, out)
    clouds = {'A': cloud(work, 'A', 20261004, 16, 3000), 'B': cloud(work, 'B', 20261006, 21, 400)}
    dump = work / 'dump'

    def probe(name, kmax, leaf, mode, wrapper=(), timeout=600):
        xyz, ids, _ = clouds[name]
        dump.unlink(missing_ok=True)
        argv = list(wrapper) + [str(args.bench), str(xyz), str(ids), str(dump), kmax, leaf, '256', '0',
                                str(2**32 - 1), str(1 << 32), '4', str(mode)]
        start = time.monotonic()
        run = subprocess.run(argv, stdin=subprocess.DEVNULL, capture_output=True, text=True, timeout=timeout)
        got = phases(run.stdout)
        digest = hashlib.sha256(dump.read_bytes()).hexdigest() if dump.is_file() else None
        return run, got, digest, round(time.monotonic() - start, 3)

    seen = {}
    for name in clouds:
        for kmax, leaf in CONFIGS:
            for mode_name, mode in MODES.items():
                run, got, digest, seconds = probe(name, kmax, leaf, mode)
                status = got.get('exit', {}).get('status')
                domain = got.get('domain', {})
                entry = dict(cloud=name, kmax=kmax, leaf=leaf, mode=mode_name, code=run.returncode, status=status,
                             digest=digest, seconds=seconds, work=domain.get('catalogue_work'),
                             batch=domain.get('leaf_batch'))
                report['runs'].append(entry)
                if run.returncode != 0 or status != 'ok' or digest is None:
                    refuse('%s K%s %s : sortie %d %s %s' % (name, kmax, mode_name, run.returncode, status,
                                                            run.stderr[-400:]), report, out)
                seen[(name, kmax, mode_name)] = entry
            reference = seen[(name, kmax, 'cpu')]
            for mode_name in MODES:
                entry = seen[(name, kmax, mode_name)]
                if entry['digest'] != reference['digest'] or entry['work'] != reference['work']:
                    refuse('%s K%s %s : dump ou registre different du CPU' % (name, kmax, mode_name), report, out)
            gpu, coop, emulated = (seen[(name, kmax, m)]['batch'] for m in ('gpu', 'gpu_coop', 'lot_coop'))
            for key in ('jobs', 'records', 'population', 'unresolved'):
                if not gpu[key] == coop[key] == emulated[key]:
                    refuse('%s K%s : lot %s different (%s %s %s)' % (name, kmax, key, gpu[key], coop[key],
                                                                     emulated[key]), report, out)
    if seen[('B', '10', 'gpu_coop')]['batch']['unresolved'] == 0:
        refuse('aucune feuille non resolue sur B a K = 10', report, out)
    last = seen[('A', '10', 'gpu_coop')]['batch']
    if not (0 < last['fill_jobs'] < last['jobs'] and last['copied_jobs'] > 0):
        refuse('chemins d ecriture non exerces', report, out)
    sanitizer = args.sanitizer or shutil.which('compute-sanitizer') or ''
    for candidate in ('/usr/local/cuda/bin/compute-sanitizer', '/usr/local/cuda-12.9/bin/compute-sanitizer'):
        if not sanitizer and Path(candidate).is_file():
            sanitizer = candidate
    if not sanitizer:
        refuse('compute-sanitizer introuvable', report, out)
    for tool, name, kmax, leaf in SANITIZED:
        wrapper = [sanitizer, '--tool', tool, '--error-exitcode', '9', '--print-limit', '20']
        run, got, digest, seconds = probe(name, kmax, leaf, MODES['gpu_coop'], wrapper, timeout=600)
        text = run.stdout + run.stderr
        clean = 'ERROR SUMMARY: 0 errors' in text or 'RACECHECK SUMMARY: 0 hazards' in text
        entry = dict(tool=tool, cloud=name, kmax=kmax, code=run.returncode, seconds=seconds, clean=clean,
                     same_dump=digest == seen[(name, kmax, 'cpu')]['digest'], tail=text[-600:])
        report['sanitizer'].append(entry)
        if run.returncode != 0 or not clean or not entry['same_dump']:
            refuse('%s %s K%s : %d %s' % (tool, name, kmax, run.returncode, text[-400:]), report, out)
    report['verdict'] = 'conforme'
    (out / 'coop_g4_gate.json').write_text(json.dumps(report, indent=1, sort_keys=True) + '\n')
    print('coop_g4_gate_verdict conforme runs%d sanitizer%d unresolved_B%d' % (
        len(report['runs']), len(report['sanitizer']), seen[('B', '10', 'gpu_coop')]['batch']['unresolved']))


if __name__ == '__main__':
    main()
