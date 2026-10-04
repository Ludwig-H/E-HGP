#!/usr/bin/env python3
"""Preparation sur G4 de trames SemanticKITTI pour la campagne points, exactement comme le criblage de Zoltan/.

    python3 bench/points_lidar_prepare.py --src SRC --data DATA --work DIR --scenes DIR --out DIR
        [--rows criblage_rows.json] [--prefix c08_] [--role voisin]

DATA (dossier plat de la session) : <seq>_<trame>.bin et .label (telechargement partiel des archives officielles
par Zoltan/demos/tools/kitti.py, empreintes du criblage), criblage_rows.json (lignes de
Zoltan/demos/recherche/criblage_08.jsonl), eigen3.tar.gz (en-tetes Eigen, outil de construction seulement),
et en option des scenes deja preparees (<nom>_sites.u32le, <nom>_labels.u32le, points_manifest_extra.json).

1. Sonde de sol Patchwork++ v8 compilee depuis DATA/patchwork_sources.tar.gz (morsehgp3D_v8/bench/
   patchwork_ground_probe.cpp et les six fichiers amont du recu v8 lidar_ground_20260921, empreintes epinglees
   verifiees avant compilation), drapeaux du constructeur v8 ; controle : masque de
   08/000000 egal a l'empreinte v8 9db3fe5c... ; controle bout a bout : la trame 08/000040 preparee ici rend
   exactement les sites deja prepares par le criblage v10 (empreinte donnee).
2. Par trame : sol retire (masque != 1), garde de rejeu (points bruts, points sans sol, instances d'au moins
   40 points et leurs effectifs, egaux au criblage ; tout ecart publie), grille 1 mm floor(x/h + 1/2) exacte depuis
   le float32, sites distincts, label de la premiere occurrence, translation au minimum de chaque axe.
3. Ecrit SCENES/<nom>_sites.u32le, _labels.u32le et points_manifest.json (role --role, « voisin » par defaut).
   Aucune coordonnee dans OUT : seulement les recus. Trames hors criblage (population P08 de l'E1) : --rows designe
   leurs lignes (n_raw et empreintes seulement : la garde de rejeu ne porte que sur les champs enregistres) ; la
   ligne du criblage de 08/000040 y reste pour le controle bout a bout de la sonde.
Codes : 0 conforme ; 1 garde de rejeu en ecart (publie, scenes ecrites quand meme) ; 3 controle de sonde faux.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tarfile
import time

import numpy as np

COMMIT = '3e6903a1d5537a4cc2ace897b0bbb98a92d6014c'
MASK_000000 = '9db3fe5c3f5c4c5d955dde9fcaf61f52ac304f4c23aa68076fd934b589caaf8f'
SITES_000040 = '0e46a18b42542b548925f7d9f99f0aebf93e892e61b9cdacc822e79b3ca6a6ed'
THING = (10, 11, 13, 15, 16, 18, 20, 30, 31, 32, 252, 253, 254, 255, 256, 257, 258, 259)
LIMIT = (1 << 21) - 1


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def run(argv, cwd=None, timeout=900):
    done = subprocess.run([str(a) for a in argv], cwd=cwd, capture_output=True, text=True, timeout=timeout)
    if done.returncode != 0:
        raise RuntimeError('%s : code %d %s' % (argv[0], done.returncode, done.stderr[-800:]))
    return done.stdout


PINNED = {  # empreintes epinglees par morsehgp3D_v8/bench/build_patchwork_ground.py (UPSTREAM) et la sonde de Git
    'third_party/LICENSE': 'b2ae335f9e2c0e9b7262226c8bb903743d4376bfca271ed8adbbfe9ad5755f7c',
    'third_party/cpp/patchworkpp/src/patchworkpp.cpp': 'f7037532eea5027994f672fbdca34be9b7be1033fd8d6f23cf6e41d0912f9b2a',
    'third_party/cpp/patchworkpp/include/patchwork/patchworkpp.h':
        '3803a5cd4c1064c71fa09f3388369e04ca912d18cdc2b8f52a796a6bbfc362f8',
    'third_party/cpp/common/src/plane_fit.cpp': 'a9b507089e6946954136a23b4e5ceb91e599fdca4da9ea6b2dc5f97fd8e9370c',
    'third_party/cpp/common/include/patchwork/plane_fit.h':
        '0f89a35178aa247bbdd80b36b983e8fd4ca85b338b9f8d882af99235b33fc7c2',
    'third_party/cpp/common/include/patchwork/types.h': '4265f49849f598d731908d4ce5ad05a78362fdda8518cd7f712046173bf31816',
    'patchwork_ground_probe.cpp': 'f1add2faf285f0fbf634438519d700de3c839a02732b13cf1ead53dec3cef2bd',
}


def build_probe(src, data, work):
    """Sonde v8 compilee depuis l'archive des sources epinglees (le paquet de session ne porte que la v11)."""
    del src
    eigen, sources = work / 'eigen', work / 'patchwork'
    with tarfile.open(data / 'eigen3.tar.gz') as archive:
        archive.extractall(eigen)
    with tarfile.open(data / 'patchwork_sources.tar.gz') as archive:
        archive.extractall(sources)
    for relative, digest in PINNED.items():
        if sha(sources / relative) != digest:
            raise RuntimeError('source non epinglee : ' + relative)
    include = next(p.parent.parent for p in eigen.rglob('Core') if p.parent.name == 'Eigen')
    third, probe = sources / 'third_party', sources / 'patchwork_ground_probe.cpp'
    common = ['-std=c++20', '-ffp-contract=off', '-fno-fast-math', '-DEIGEN_DONT_PARALLELIZE',
              '-DMHGP8_PATCHWORK_COMMIT="%s"' % COMMIT, '-isystem', str(include),
              '-isystem', str(third / 'cpp/patchworkpp/include'), '-isystem', str(third / 'cpp/common/include'),
              '-O3', '-DNDEBUG', '-Wall', '-Wextra', '-Wpedantic']
    units = [('patchworkpp', third / 'cpp/patchworkpp/src/patchworkpp.cpp', []),
             ('plane_fit', third / 'cpp/common/src/plane_fit.cpp', []), ('probe', probe, ['-Werror'])]
    objects = []
    for name, source, extra in units:
        obj = work / (name + '.o')
        run(['g++', *common, *extra, '-c', source, '-o', obj])
        objects.append(obj)
    binary = work / 'mhgp8_patchwork_ground_probe'
    run(['g++', '-O3', *objects, '-o', binary])
    return binary, dict(sources=PINNED, eigen_sha256=sha(data / 'eigen3.tar.gz'), binary_sha256=sha(binary))


def ground(binary, frame_bin, work):
    out = work / (frame_bin.stem + '.mask')
    report = json.loads(run([binary, '--input', frame_bin, '--output', out]).strip().splitlines()[-1])
    return np.fromfile(out, np.uint8), sha(out), report


def quantize_exact(values):
    """floor(x / h + 1/2), h = 1 mm, exact depuis le flottant (formule du harnais et du criblage)."""
    out = np.empty(values.size, np.int64)
    for i, v in enumerate(values.reshape(-1).tolist()):
        num, den = (0.0 if v == 0 else v).as_integer_ratio()
        out[i] = (2 * num * 1000 + den) // (2 * den)
    return out.reshape(values.shape)


def prepare_frame(binary, data, work, row):
    seq, frame = row['seq'], row['frame']
    frame_bin = data / ('%s_%s.bin' % (seq, frame))
    xyzi = np.fromfile(frame_bin, np.float32).reshape(-1, 4)
    label = np.fromfile(data / ('%s_%s.label' % (seq, frame)), np.uint32)
    mask, mask_sha, report = ground(binary, frame_bin, work)
    keep = mask != 1
    X, L = xyzi[keep, :3].astype(np.float64), label[keep]
    sem, inst = (L & 0xFFFF).astype(np.int64), (L >> 16).astype(np.int64)
    lbl = np.where(np.isin(sem, THING) & (inst > 0), L.astype(np.int64), -1)
    keys, counts = np.unique(lbl[lbl >= 0], return_counts=True)
    got = {(int(k) & 0xFFFF, int(k) >> 16): int(c) for k, c in zip(keys, counts) if c >= 40}
    want = {(i['sem'], i['inst']): i['points'] for i in row.get('instances', [])}
    gaps = []  # garde de rejeu : seulement sur les champs enregistres (une trame hors criblage n'en a que n_raw)
    if len(xyzi) != row['n_raw']:
        gaps.append(dict(field='n_raw', recorded=row['n_raw'], replayed=len(xyzi)))
    if 'n_without_ground' in row and len(X) != row['n_without_ground']:
        gaps.append(dict(field='n_without_ground', recorded=row['n_without_ground'], replayed=len(X)))
    if 'instances' in row and got != want:
        gaps.append(dict(field='instances', recorded=sorted(want.items()), replayed=sorted(got.items())))
    q = quantize_exact(X)
    uniq, first = np.unique(q, axis=0, return_index=True)
    sites = uniq - uniq.min(axis=0)
    if int(sites.max()) > LIMIT:
        raise ValueError('etendue hors u21')
    return sites.astype('<u4'), L[first].astype('<u4'), dict(
        seq=seq, frame=frame, n_raw=int(len(xyzi)), n_without_ground=int(len(X)), sites=int(len(sites)),
        duplicates=int(len(X) - len(sites)), mask_sha256=mask_sha, ground_counts=report.get('counts'),
        velodyne_sha256=sha(frame_bin), replay=dict(identical=not gaps, gaps=gaps))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('--src', '--data', '--work', '--scenes', '--out'):
        parser.add_argument(name, type=Path, required=True)
    parser.add_argument('--rows', default='criblage_rows.json', help='lignes des trames (dossier DATA)')
    parser.add_argument('--prefix', default='c08_', help='prefixe des noms de scene')
    parser.add_argument('--role', default='voisin', help='role des scenes dans le manifeste')
    args = parser.parse_args()
    for path in (args.work, args.scenes, args.out):
        path.mkdir(parents=True, exist_ok=True)
    started = time.monotonic()
    binary, build = build_probe(args.src, args.data, args.work)
    _, mask_sha, _ = ground(binary, args.data / '08_000000.bin', args.work)
    rows = json.loads((args.data / args.rows).read_text())
    control = next((r for r in rows if r['frame'] == '000040'), None)
    receipts, scenes, status = [], [], 0
    probe_ok = mask_sha == MASK_000000
    if control is not None:
        sites, _, _ = prepare_frame(binary, args.data, args.work, control)
        probe_ok = probe_ok and hashlib.sha256(sites.tobytes()).hexdigest() == SITES_000040
    if not probe_ok:
        status = 3
    else:
        for row in rows:
            if row['frame'] == '000040':
                continue
            sites, labels, receipt = prepare_frame(binary, args.data, args.work, row)
            name = args.prefix + row['frame']
            sites.tofile(args.scenes / (name + '_sites.u32le'))
            labels.tofile(args.scenes / (name + '_labels.u32le'))
            receipts.append(dict(name=name, **receipt))
            scenes.append(dict(name=name, kind='criblage_voisin' if args.role == 'voisin' else args.role,
                               role=args.role, sites=receipt['sites'], sequence=row['seq'], frame=row['frame'],
                               sites_sha256=sha(args.scenes / (name + '_sites.u32le')),
                               labels_sha256=sha(args.scenes / (name + '_labels.u32le'))))
            status = max(status, 0 if receipt['replay']['identical'] else 1)
        extra = args.data / 'points_manifest_extra.json'
        if extra.is_file():
            for entry in json.loads(extra.read_text())['scenes']:
                for suffix in ('_sites.u32le', '_labels.u32le'):
                    shutil.copyfile(args.data / (entry['name'] + suffix), args.scenes / (entry['name'] + suffix))
                scenes.append(entry)
    (args.scenes / 'points_manifest.json').write_text(json.dumps(dict(scenes=scenes), indent=1) + '\n')
    summary = dict(status=status, probe=dict(build, mask_000000=mask_sha, mask_expected=MASK_000000,
                                             sites_000040_checked=control is not None, ok=probe_ok),
                   frames=receipts, scenes=len(scenes), seconds=time.monotonic() - started)
    (args.out / 'prepare.json').write_text(json.dumps(summary, indent=1, sort_keys=True) + '\n')
    print('points_lidar_prepare status%d scenes%d sonde_ok%d ecarts%d' % (
        status, len(scenes), int(probe_ok), sum(1 for r in receipts if not r['replay']['identical'])))
    return status


if __name__ == '__main__':
    raise SystemExit(main())
