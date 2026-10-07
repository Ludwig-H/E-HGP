#!/usr/bin/env python3
"""Trames SemanticKITTI sans sol pour morsehgp3D_v12 (famille a, regime principal).

    python3 -I prepare_semantickitti.py --archives DIR --frames DIR [DIR ...] --control DIR --out DIR --work DIR
        [--labels] [--only SEQ_FRAME ...]

--archives : dossier contenant patchwork_sources.tar.gz et eigen3.tar.gz (empreintes epinglees, voir
             v12data/patchwork.py) ; en local : /workspaces/E-HGP/build/v11-persist/data_points3/.
--frames   : dossiers ou chercher les scans (noms `<seq>_<trame>.bin` ou `sequences/<seq>/velodyne/<trame>.bin`,
             etiquettes `.label` a cote ou sous `labels/`) ; toutes les copies d'une meme trame doivent avoir la meme
             empreinte (sinon refus).
--control  : dossier `.../sequences/08/velodyne/` contenant 000000.bin, 000100.bin et 000200.bin : controle bout a
             bout contre les entrees du contrat v11 ng00, ng01, ng02 (convention v8 : translation au minimum de la
             trame brute ENTIERE, sites en ordre lexicographique, identifiant = indice du retour brut).
             Les empreintes attendues sont gravees ci-dessous (build/v11-full-data-20261002/manifest.json).

Chaine (identique a la v11, morsehgp3D_v11/bench/points_lidar_prepare.py, sauf la translation et l'ordre) :
1. sonde Patchwork++ v8 compilee depuis les archives epinglees ; masque de 08/000000 = 9db3fe5c... sinon code 3 ;
2. par trame : retours de sol retires (masque = 1 ; inconnus 0 et hors sol 2 gardes), grille 1 mm exacte depuis le
   float32, positions distinctes en ordre lexicographique (identifiant = indice du retour brut), translation au
   minimum de chaque axe des sites GARDES (v12 ; la translation est publiee) ;
3. ecrit `<nom>.u32le`, `<nom>.ids.u32le` (et `<nom>.labels.u32le` avec --labels : etiquette du premier retour), un
   manifeste par trame et `manifest.json` (toutes les trames) + `manifest_v12set.json` (selection stratifiee).
Codes : 0 conforme ; 2 entree refusee ; 3 controle de sonde ou de chaine faux.
Aucune donnee KITTI dans le depot (CC BY-NC-SA 3.0) : seulement empreintes et comptes.
"""
from __future__ import annotations

import argparse
import re
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import numpy as np  # noqa: E402

from v12data import common, patchwork  # noqa: E402
from v12data.formats import kitti_bin  # noqa: E402

DATASET = dict(name='SemanticKITTI (scans KITTI odometry velodyne + etiquettes SemanticKITTI)',
               sensor='Velodyne HDL-64E, vehicule (Karlsruhe), une trame = un tour (10 Hz)',
               licence='CC BY-NC-SA 3.0 (KITTI) ; etiquettes SemanticKITTI CC BY-NC-SA 4.0',
               licence_url='https://www.cvlibs.net/datasets/kitti/ ; http://www.semantic-kitti.org/',
               access='archives officielles (data_odometry_velodyne.zip 80 Go, data_odometry_labels.zip 179 Mo) ; '
                      'trames deja en cache local (telechargement partiel par Zoltan/demos/tools/kitti.py)')
# entrees du contrat v11 (build/v11-full-data-20261002/manifest.json)
CONTROL = {
    '000000': dict(name='lidar_ng00', raw='92e945f37a6cd4a58acc8aa15b275af2e271ecf69c0a44a311d888524473c451',
                   coords='0baa4de14c95838ef7bd18d5a98551ca513ed830ec1eeee84f649fa97c95abaf',
                   ids='c73a41965f6f2e1042b5f3ba876d12ae0811b7f24aa73c886aa07830974e33c6', count=39885,
                   translation=[79602, 79917, 27639]),
    '000100': dict(name='lidar_ng01', raw='4dfb5701db9ff36bd592746e08e8fafd4d1c63cba1ef66d2c2791e405b800e80',
                   coords='ba15adc6907d58e50bf28bca92305210c1efdde6efdf46c782aa1eec2318036f',
                   ids='bb699c2511a87618813a32657399539fe90e8f1e4f1187656ac88c03f2fee6a4', count=35551,
                   translation=[80208, 79533, 5901]),
    '000200': dict(name='lidar_ng02', raw='132f437349e17c00a8a31539f10a3214d9391ae1251293e7794a26540412b5cd',
                   coords='a4bbc86d00f92627b869fdc34aa260353bf1b821eff7c992ad93beb2a13308af',
                   ids='121e76f3ef1fcedb05cb65da9a95bfb84fe4b8305076a7485483fff113d128cc', count=45845,
                   translation=[80271, 22224, 13694]),
}
CONTRACT_FRAMES = [('08', '000000'), ('08', '000100'), ('08', '000200')]
PER_SEQUENCE = 8  # selection v12set : au plus 8 trames par sequence, rangs regulierement espaces par taille


def discover(dirs):
    frames, labels = {}, {}
    pat_flat = re.compile(r'^(\d\d)_(\d{6})\.(bin|label)$')
    for root in dirs:
        for path in sorted(Path(root).rglob('*')):
            if not path.is_file():
                continue
            m = pat_flat.match(path.name)
            if m:
                key, ext = (m.group(1), m.group(2)), m.group(3)
            else:
                m = re.match(r'^(\d{6})\.(bin|label)$', path.name)
                parts = path.parts
                if not m or len(parts) < 3 or parts[-2] not in ('velodyne', 'labels') or \
                        not re.match(r'^\d\d$', parts[-3]):
                    continue
                key, ext = (parts[-3], m.group(1)), m.group(2)
            (frames if ext == 'bin' else labels).setdefault(key, []).append(path)
    return frames, labels


def unique_copy(paths):
    digests = {}
    for p in paths:
        digests.setdefault(common.sha256_file(p), []).append(p)
    if len(digests) != 1:
        raise RuntimeError('copies divergentes : %s' % {k: [str(x) for x in v] for k, v in digests.items()})
    digest, copies = next(iter(digests.items()))
    return copies[0], digest, len(copies)


def control_chain(binary, control_dir: Path, work: Path) -> dict:
    """Rejoue ng00, ng01, ng02 a l'octet (convention v8 : translation sur la trame brute entiere)."""
    results, ok = {}, True
    for frame, expect in CONTROL.items():
        path = control_dir / (frame + '.bin')
        raw_sha = common.sha256_file(path)
        xyzi = kitti_bin.read_velodyne(path)
        mask, mask_sha, _ = patchwork.ground_mask(binary, path, work)
        q_all = common.quantize_float_mm(xyzi[:, :3])
        lo_raw = q_all.min(axis=0)
        u_all = (q_all - lo_raw).astype(np.uint32)
        keep = np.flatnonzero(mask != 1)
        d = common.distinct_sites(u_all[keep])
        sites = u_all[keep][d['first']]
        ids = keep[d['first']].astype('<u4')
        got = dict(raw=raw_sha, coords=common.sha256_array(sites.astype('<u4')), ids=common.sha256_array(ids),
                   count=int(len(sites)), translation=[-int(v) for v in lo_raw.tolist()])
        same = all(got[k] == expect[k] for k in ('raw', 'coords', 'ids', 'count', 'translation'))
        ok = ok and same
        results[expect['name']] = dict(frame='08/' + frame, identical=same, mask_sha256=mask_sha, **got)
    return dict(ok=ok, cases=results)


def prepare_frame(binary, key, path, label_path, out_dir: Path, work: Path, with_labels: bool):
    seq, frame = key
    xyzi = kitti_bin.read_velodyne(path)
    n_raw = len(xyzi)
    mask, mask_sha, report = patchwork.ground_mask(binary, path, work)
    if len(mask) != n_raw:
        raise RuntimeError('masque de taille fausse pour %s_%s' % key)
    keep = np.flatnonzero(mask != 1)
    q_raw = common.quantize_float_mm(xyzi[:, :3])
    raw_dup = n_raw - len(common.distinct_sites((q_raw - q_raw.min(axis=0)).astype(np.uint32))['first'])
    labels = None
    if with_labels and label_path is not None:
        labels = kitti_bin.read_labels(label_path, n_raw)[keep]
    name = 'kitti_ng_%s_%s' % (seq, frame)
    # sites distincts parmi les retours gardes (convention v11) ; multiplicites publiees s'il y a des doublons (D8)
    result = common.prepare_outputs(out_dir, name, q_raw[keep], ids=keep, labels=labels, variants=('distinct',),
                                    primary='distinct')
    m = result['measures']
    m.update(n_raw_returns=int(n_raw), n_without_ground_returns=int(len(keep)),
             duplicate_returns_without_ground=int(m['duplicate_points']), duplicate_returns_raw_frame=int(raw_dup),
             n_sites=int(m['n_distinct']))
    conversion = dict(source_type='float32 (metres, repere capteur)',
                      ground=dict(method='Patchwork++', upstream_commit=patchwork.COMMIT, mask_sha256=mask_sha,
                                  mask_policy='remove_only_1_keep_0_and_2', counts=report.get('counts'),
                                  labels_used=False),
                      site_policy='distinct positions among kept returns, lexicographic order, '
                                  'id = smallest raw return index at that position',
                      translation_scope='kept sites (v12) ; absolute mm = u + translation_mm (sensor frame)',
                      subsampling=False, whole_frame_after_ground_mask=True)
    source = dict(sequence=seq, frame=frame, velodyne=dict(file=path.name, sha256=common.sha256_file(path)),
                  labels=(dict(file=label_path.name, sha256=common.sha256_file(label_path))
                          if label_path is not None else None))
    manifest = common.scene_manifest(name, 'semantickitti_sans_sol', DATASET, source, conversion, result,
                                     __file__)
    common.write_json(out_dir / (name + '.manifest.json'), manifest)
    return manifest


def select_v12set(manifests):
    """Selection stratifiee : par sequence, au plus PER_SEQUENCE trames a des rangs regulierement espaces de la
    taille (n sites), min et max compris ; plus les trames du contrat (08/000000, 000100, 000200)."""
    by_seq = {}
    for m in manifests:
        by_seq.setdefault(m['source']['sequence'], []).append(m)
    chosen = []
    for seq in sorted(by_seq):
        items = sorted(by_seq[seq], key=lambda m: (m['measures']['n_distinct'], m['source']['frame']))
        k = min(PER_SEQUENCE, len(items))
        ranks = sorted({round(i * (len(items) - 1) / (k - 1)) for i in range(k)}) if k > 1 else [0]
        chosen.extend(items[r] for r in ranks)
    names = {m['name'] for m in chosen}
    for seq, frame in CONTRACT_FRAMES:
        for m in manifests:
            if (m['source']['sequence'], m['source']['frame']) == (seq, frame) and m['name'] not in names:
                chosen.append(m)
                names.add(m['name'])
    return sorted(chosen, key=lambda m: m['name'])


def set_manifest(manifests, probe, control, scope):
    cases = []
    for m in manifests:
        outs = m['outputs']
        raw = outs['distinct'] if outs.get('distinct', {}).get('name') == m['name'] else outs['raw']
        cases.append(dict(name=m['name'], coordinates=raw['coordinates'], point_ids=raw['point_ids'],
                          count=raw['count'], duplicate_sites=raw['duplicate_sites'], sha256=raw['sha256'],
                          ids_sha256=raw['ids_sha256'], coordinate_encoding=raw['coordinate_encoding'],
                          point_id_encoding=raw['point_id_encoding'], profile=m['measures']['minimal_profile'],
                          bits_needed=m['measures']['bits_needed'], extent_mm=m['measures']['extent_mm'],
                          unit_site_weights=True,
                          provenance=dict(dataset='SemanticKITTI', kind='lidar', sequence=m['source']['sequence'],
                                          frame=m['source']['frame'],
                                          velodyne_sha256=m['source']['velodyne']['sha256'],
                                          mask_sha256=m['conversion']['ground']['mask_sha256'],
                                          n_raw_returns=m['measures']['n_raw_returns'],
                                          n_without_ground_returns=m['measures']['n_without_ground_returns'],
                                          translation_mm=m['measures']['translation_mm'],
                                          duplicate_returns_without_ground=m['measures'][
                                              'duplicate_returns_without_ground'],
                                          multiplicities=raw.get('mult', {}).get('file'),
                                          labels_used=False, subsampling=False)))
    sizes = sorted(c['count'] for c in cases)
    seqs = sorted({c['provenance']['sequence'] for c in cases})
    return dict(schema=common.SCHEMA_SET, scope=scope, family='semantickitti_sans_sol', cases=cases,
                summary=dict(frames=len(cases), sequences=seqs, sites_min=sizes[0], sites_max=sizes[-1],
                             sites_median=sizes[len(sizes) // 2] if len(sizes) % 2 else
                             (sizes[len(sizes) // 2 - 1] + sizes[len(sizes) // 2]) / 2,
                             frames_over_60000=sum(1 for s in sizes if s > 60000),
                             frames_under_30000=sum(1 for s in sizes if s < 30000),
                             per_sequence={s: sum(1 for c in cases if c['provenance']['sequence'] == s)
                                           for s in seqs}),
                probe=probe, control=control, generator=common.generator_info(__file__),
                created_utc=common.utc_now(), public_status='not_claimed')


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--archives', type=Path, required=True)
    parser.add_argument('--frames', type=Path, nargs='+', required=True)
    parser.add_argument('--control', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--work', type=Path, required=True)
    parser.add_argument('--labels', action='store_true')
    parser.add_argument('--only', nargs='*', default=None, help='SEQ_TRAME a preparer (defaut : toutes)')
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    args.work.mkdir(parents=True, exist_ok=True)
    started = time.monotonic()
    common.selftest_quantize()
    binary, probe = patchwork.build_probe(args.archives, args.work)
    mask_path = args.control / '000000.bin'
    if common.sha256_file(mask_path) != patchwork.VELODYNE_08_000000:
        common.log('trame de controle 08/000000 inattendue')
        return 2
    _, mask_sha, _ = patchwork.ground_mask(binary, mask_path, args.work)
    probe['mask_08_000000'] = mask_sha
    probe['mask_ok'] = mask_sha == patchwork.MASK_08_000000
    if not probe['mask_ok']:
        common.log('controle de sonde FAUX : masque %s' % mask_sha)
        return 3
    control = control_chain(binary, args.control, args.work)
    common.log('controle ng00-02 : %s' % ('identique' if control['ok'] else 'ECART'))
    if not control['ok']:
        common.write_json(args.out / 'control_failure.json', control)
        return 3
    frames, labels = discover(args.frames)
    keys = sorted(frames)
    if args.only:
        wanted = {tuple(s.split('_')) for s in args.only}
        keys = [k for k in keys if k in wanted]
    manifests = []
    for i, key in enumerate(keys):
        path, digest, copies = unique_copy(frames[key])
        label_path = unique_copy(labels[key])[0] if key in labels else None
        m = prepare_frame(binary, key, path, label_path, args.out, args.work, args.labels)
        m['source']['local_copies'] = copies
        manifests.append(m)
        if i % 25 == 0:
            common.log('%d/%d %s_%s : %d sites' % (i + 1, len(keys), key[0], key[1], m['measures']['n_distinct']))
    common.write_json(args.out / 'manifest.json', set_manifest(manifests, probe, control, 'all_local_frames'))
    chosen = select_v12set(manifests)
    common.write_json(args.out / 'manifest_v12set.json',
                      set_manifest(chosen, probe, control, 'v12set_stratified_by_sequence_and_size'))
    common.log('%d trames, selection v12set %d, %.1f s' % (len(manifests), len(chosen), time.monotonic() - started))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
