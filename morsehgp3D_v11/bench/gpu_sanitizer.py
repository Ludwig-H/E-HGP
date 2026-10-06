#!/usr/bin/env python3
"""Compute Sanitizer sur la voie GPU des feuilles (G4) : memcheck, racecheck et synccheck, dump egal au CPU.

    python3 bench/gpu_sanitizer.py --bench B_CUDA/mhgp11_full_bench --out DIR [--sanitizer compute-sanitizer]

Binaire construit avec MHGP11_ENABLE_CUDA (MHGP11_COORD_BITS=21). Nuages synthetiques seulement (aucune donnee
LiDAR) : A = 3 000 sites uniformes du cube 2^16 (random.Random(20261004), celui de tests/tower/full_leaf_lanes.py),
B = 400 sites du cube 2^21 (random.Random(20261006), feuilles non resolues a K = 10, rejouees par le CPU). Pour
chaque nuage, K = 5 feuilles de 16 et K = 10 feuilles de 24 : la voie CPU (16379) donne le dump de reference ; la voie
GPU avec reservoir chaine (81915) et sans (212987 = 81915 + 131072, debordements rejoues) doit rendre le meme dump et
le meme registre, puis chaque outil de Compute Sanitizer rejoue la voie GPU avec reservoir sur A a K = 10 (chaines du
reservoir exercees) et sur B a K = 10 (non resolues) : zero erreur, meme dump. Exige aussi, sur A a K = 10, des blocs
du reservoir pris et aucune feuille rejouee avec lui, des feuilles rejouees sans lui. Bibliotheque standard seule,
aucune assertion. Codes : 0 conforme ; 1 refus.
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
import tempfile
import time

MODES = {'cpu': 16379, 'gpu': 81915, 'gpu_rejoue': 212987}
CONFIGS = (('5', '16'), ('10', '24'))
SANITIZED = (('memcheck', 'A'), ('memcheck', 'B'), ('racecheck', 'A'), ('racecheck', 'B'), ('synccheck', 'A'))


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
    return xyz, ids


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--bench', type=Path, required=True)
    ap.add_argument('--out', type=Path, required=True)
    ap.add_argument('--sanitizer', default='')
    args = ap.parse_args()
    out = args.out
    out.mkdir(parents=True, exist_ok=True)
    # Nuages et dumps hors du dossier de sortie (un dump K10 pese 272 Mo : il depassait le plafond des resultats de
    # la session G4 reservoir2) ; seul gpu_sanitizer.json est publie.
    work = Path(tempfile.mkdtemp(prefix='mhgp11-gpu-sanitizer-'))
    report = {'schema': 'ehgp.v11.gpu_sanitizer.v1', 'bench': str(args.bench), 'runs': [], 'sanitizer': []}

    def finish(verdict):
        shutil.rmtree(work, ignore_errors=True)
        report['verdict'] = verdict
        (out / 'gpu_sanitizer.json').write_text(json.dumps(report, indent=1, sort_keys=True) + '\n')
        print('gpu_sanitizer_verdict ' + verdict)
        return 0 if verdict == 'conforme' else 1

    if not args.bench.is_file():
        return finish('refus : binaire absent')
    clouds = {'A': cloud(work, 'A', 20261004, 16, 3000), 'B': cloud(work, 'B', 20261006, 21, 400)}
    dump = work / 'dump'

    def probe(name, kmax, leaf, mode, wrapper=(), timeout=600):
        xyz, ids = clouds[name]
        dump.unlink(missing_ok=True)
        argv = list(wrapper) + [str(args.bench), str(xyz), str(ids), str(dump), kmax, leaf, '256', '0',
                                str(2**32 - 1), str(1 << 32), '4', str(mode)]
        start = time.monotonic()
        run = subprocess.run(argv, stdin=subprocess.DEVNULL, capture_output=True, text=True, timeout=timeout)
        digest = hashlib.sha256(dump.read_bytes()).hexdigest() if dump.is_file() else None
        return run, phases(run.stdout), digest, round(time.monotonic() - start, 3)

    seen = {}
    for name in clouds:
        for kmax, leaf in CONFIGS:
            for mode_name, mode in MODES.items():
                run, got, digest, seconds = probe(name, kmax, leaf, mode)
                domain = got.get('domain', {})
                entry = dict(cloud=name, kmax=kmax, mode=mode_name, code=run.returncode, digest=digest,
                             status=got.get('exit', {}).get('status'), work=domain.get('catalogue_work'),
                             batch=domain.get('leaf_batch'), seconds=seconds)
                report['runs'].append(entry)
                if run.returncode != 0 or entry['status'] != 'ok' or digest is None or entry['work'] is None:
                    return finish('refus : %s K%s %s : sortie %d %s' % (name, kmax, mode_name, run.returncode,
                                                                        run.stderr[-300:]))
                seen[(name, kmax, mode_name)] = entry
            for mode_name in MODES:
                a, b = seen[(name, kmax, mode_name)], seen[(name, kmax, 'cpu')]
                if a['digest'] != b['digest'] or a['work'] != b['work']:
                    return finish('refus : %s K%s %s : dump ou registre different du CPU' % (name, kmax, mode_name))
    chained, replayed = seen[('A', '10', 'gpu')]['batch'], seen[('A', '10', 'gpu_rejoue')]['batch']
    if not (chained['fill_jobs'] == 0 and chained['spare_record_chunks'] > 0 and replayed['fill_jobs'] > 0):
        return finish('refus : chemins d ecriture non exerces (%s, %s)' % (chained['fill_jobs'], replayed['fill_jobs']))
    if seen[('B', '10', 'gpu')]['batch']['unresolved'] == 0:
        return finish('refus : aucune feuille non resolue sur B a K = 10')
    sanitizer = args.sanitizer or shutil.which('compute-sanitizer') or ''
    for candidate in ('/usr/local/cuda/bin/compute-sanitizer', '/usr/local/cuda-12.9/bin/compute-sanitizer'):
        if not sanitizer and Path(candidate).is_file():
            sanitizer = candidate
    if not sanitizer:
        return finish('refus : compute-sanitizer introuvable')
    for tool, name in SANITIZED:
        wrapper = [sanitizer, '--tool', tool, '--error-exitcode', '9', '--print-limit', '20']
        run, got, digest, seconds = probe(name, '10', '24', MODES['gpu'], wrapper, timeout=900)
        text = run.stdout + run.stderr
        clean = 'ERROR SUMMARY: 0 errors' in text or 'RACECHECK SUMMARY: 0 hazards' in text
        entry = dict(tool=tool, cloud=name, code=run.returncode, seconds=seconds, clean=clean,
                     same_dump=digest == seen[(name, '10', 'cpu')]['digest'], tail=text[-600:])
        report['sanitizer'].append(entry)
        entry['status'] = got.get('exit', {}).get('status')
        entry['same_work'] = got.get('domain', {}).get('catalogue_work') == seen[(name, '10', 'cpu')]['work']
        if run.returncode != 0 or not clean or not entry['same_dump'] or entry['status'] != 'ok' or not entry['same_work']:
            return finish('refus : %s %s : %d %s' % (tool, name, run.returncode, text[-300:]))
    return finish('conforme')


if __name__ == '__main__':
    sys.exit(main())
