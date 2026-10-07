#!/usr/bin/env python3
"""Boreas (vehicule, Velodyne Alpha Prime 128 faisceaux, Toronto ; CC BY 4.0) -> format v11 a 1 mm (famille b).

    python3 -I prepare_boreas.py --raw DIR --out DIR --archives DIR_PATCHWORK --work DIR [--check]

Seau public AWS Open Data `s3://boreas` (HTTPS anonyme, sans compte). Une fenetre de trames CONSECUTIVES d'une
sequence, accumulees dans le repere ENU local de la sequence avec les poses lidar de verite terrain
(`applanix/lidar_poses.csv`, post-traitement Applanix centimetrique) : c'est l'analogue, fait ici et sans compte,
des nuages statiques accumules par fenetre de KITTI-360 (qui exige un compte).
Chaine par trame (deterministe a l'octet, aucune libm, aucun BLAS) :
1. lecture float32 (x, y, z, intensite, laser, t) ; t = temps du point relatif au temps de la trame (milieu du tour) ;
2. compensation du mouvement au premier ordre vers le temps de la trame : p' = p + t (v + w x p), v = C^T v_enu et
   w = (angvel_x, angvel_y, angvel_z) du releve (convention du devkit pyboreas, `remove_motion`, ici par point et non
   par 21 paquets) ; erreur du second ordre < 1 cm en ligne droite ;
3. pose : p_enu = C p' + r, C = roll(r) pitch(p) yaw(y) du devkit (`get_transform`), sin et cos deterministes
   (v12data/dettrig.py), produits et sommes float64 elementaires dans un ordre fixe ;
4. grille 1 mm exacte ; identifiant = decalage de la trame dans la fenetre + indice du point dans la trame.
Variantes : `tout` et `sans_sol` (sonde Patchwork++ v8 epinglee appliquee a chaque trame dans son repere capteur,
hauteur de capteur 2,2 m mesuree sur la premiere trame, inconnus gardes comme pour SemanticKITTI). Scenes emboitees :
1, 10 et 50 trames depuis la meme trame de depart ; plus une scene « itineraire » (une trame toutes les 200, soit
20 s, arrets sautes) qui couvre plusieurs kilometres : donnee reelle au-dela de 21 bits (qualification u24, D6).
Doublons au mm : `<nom>` brut + `<nom>.distinct` (D8).
--check : controle de la convention de pose et du signe de la compensation (recouvrement de voxels de 5 cm entre
trames voisines : les trois signes de compensation et la rotation transposee compares) ; refuse si la convention
retenue n'est pas la meilleure.
"""
from __future__ import annotations

import argparse
import csv
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import numpy as np  # noqa: E402

from v12data import common, dettrig, lasconv, patchwork  # noqa: E402

BUCKET = 'https://boreas.s3.amazonaws.com/'
SEQUENCE = 'boreas-2020-11-26-13-58'
START, COUNT = 4500, 50  # ligne droite, 10,7 a 13,2 m/s, 61 m parcourus (choix : voir RAPPORT.md)
WINDOWS = (1, 10, 50)
ROUTE_STEP, ROUTE_MIN_GAP_M = 200, 30.0  # scene itineraire : une trame toutes les 200 (20 s), arrets sautes
SENSOR_HEIGHT_M = 2.2  # 10e centile de z a 4-12 m sur la trame 1606417564274582 : -2,199 m
POSES_SHA256 = 'c1bfd60f5400e613aac248a146b492535dcf9adb2ea036356a098aa69ac1887f'  # epingle le 7 oct. 2026
FRAME_SHA256 = {  # epingles au premier telechargement (7 oct. 2026) : fenetre f4500 et itineraire s200
    '1606417097502930': '9158150711120daaa375a8d57363605eea46f906d7ae8d02e3a271e0d7ff5b05',
    '1606417118244019': '676bd699afcdad3396749ccce58a0b24d0937320b22148915d727c6d025b6072',
    '1606417138985230': '20c81e5dfee2702c3b4b3bbeee8e1b43e1f60b4754b156e38f719df419ef311e',
    '1606417159725586': '4576e4f2215b2f63cdeeee404de2586f0c834f813db1c72fb60f7b6211900b72',
    '1606417180465943': 'ff40d206e6ebe7886ef2a569f8ff93d6a388174711e9081c11bfad2a1bc1026f',
    '1606417201207886': 'a8d61e477cdce230bc71c4b7001dd4f340110eea12a8c34f124a42684d23e0fc',
    '1606417222052289': '3b64e048f2b8270ae2cc6d009fb32ab0ad106505f11edb6be75198dd9cf7336f',
    '1606417242792608': 'c48cc1a71ebd54275004f9b637f09b9601b1d11e8dfda1fda117b46e2b342ecd',
    '1606417263533165': '61af14d766ab62d077d92850d38cd5aac3533717c3a7603414bc97755bf5cb5e',
    '1606417284273609': '42facfb325905b31681ccb9e7b754094cb14240b661c58469b98ccdc245e5b60',
    '1606417346496002': '8b9db211b9244ca9fc5a98d19cfcd93d2958d8e5b7a05cd7b07bf463dd183af5',
    '1606417367237297': '3b9613f9147a04f71ae994d942908b4c6d0cd5bbad5560a3f737d628d578bfc1',
    '1606417387977539': '72931b36c97de1aae36e6a53b8ca035f7a1a62903ae54680ee9dba4cb734d74b',
    '1606417408718559': 'c3b5d8ced7e604ceb22b41e1fa263d9de81574e705fc79fae2de892e271b6f03',
    '1606417429459412': 'b4f98adca773cac7b1bb4a0e9f1a972f56944a3ad3d05e9a3e30b7578bb6e686',
    '1606417450199768': '233e2e60b72a9957bcf16fbfeb7dcda3a60075dd0262af22bc40b02adac88c95',
    '1606417470940643': '1eae591c0f382faa200ddb441f9e24fea4ada815aece42756194812444fcd205',
    '1606417491681732': '175d6e64d510dfedde2277aee5dd9024969d13914fb01f7da539c6a69d4871b6',
    '1606417512422714': 'b10225c588fb4e42297fcd69aa537e1a186134d9834de875b2d7d36c29facf4c',
    '1606417533163986': 'de49b12a525f936d25b52e06193afcff63c1ffd219cfb0c742838e6372a012b6',
    '1606417553904282': 'c0348b2f05409d02b5d355541b0bd11bd0cb0a5f32f43f7c6892d5cc5bc289b8',
    '1606417564274582': '8246e632180fd4cb370700ca71b3d3c6c00741e128cc506a8023fc74a325d5ba',
    '1606417564378220': '759936d9806a90806740ebb177dff614f5f507d9b7d9383e7892bf6dbf4e08c0',
    '1606417564481949': '476a0fbc1415b4a73de27c4015b019405064f1e10cbbabff40c3c855c327166c',
    '1606417564585678': 'b304824bc86d42027381f589b940547f82275787986404c4aa37c11e6010d513',
    '1606417564689407': '2df4791a548ad1f8aae8e58f5bb2e09fc0b9608cbeb6207c5dc1a82ce3ea7dc6',
    '1606417564793121': 'a9145bb87e3a1b2ae60cb8b49cbf98490c71c48ff16e107457e983b8fb7d5282',
    '1606417564896835': '6e9fa788b6399e1e9f847883392514ca49c2f464e0d4e644d40b400ff4fcd5dd',
    '1606417565000626': 'c5c0aab9b1da9b6a7502cc74defab059f578a1fa8742c3ef007af1a35a4eafef',
    '1606417565104416': 'ea8881577fdfe28dffd9554844b704c568f6ac12fb8c10fe2f14a009c0211793',
    '1606417565208161': 'a675c5a96cf7027d0d584fafcf216ed5118fd87566ce1fb7eba9733eadbe4d13',
    '1606417565311890': '57d41afe049530d0016ba699069f8303fec5189eb35e332fffdac4c0243c0aae',
    '1606417565415726': '166ceccf8d277ee230e8a575418175025f5a2aeda660445ed7fc6efd9dc32a2e',
    '1606417565519486': 'd7b33d80f4b31e881dd91252a4229cb9674d14858291988d026df059ab8e1b72',
    '1606417565623200': '3b1a8ccd33ff9cabce0a02c1e61a4c76a4ac78e13f4c48aaf551ecd751f18e85',
    '1606417565726914': '2598ccaa2b3d60eaa019a005df6efd400647f29b1c97046fe9f68a4346ed5d5d',
    '1606417565830536': 'd860bd6f50b649786bcf1d5d4e5be4e4b3d27d0aa021295e184a5b4559931a21',
    '1606417565934158': '22dd763861ee9a5c87a825e543ed4367d2b8fdf69d5a710bebc033bebb858be8',
    '1606417566037781': 'c831a27cd347a91b286251a5fb73ec35ee46ac15cccf36333cdb103f2134c25d',
    '1606417566141419': '644f6ff82a8ca724235799cdee4f23a745ffbe11d1c572bad1632fa14c79d838',
    '1606417566245072': '473ef0fc2039a6c3c62695bb229ae7612505aa9a5127d8021e33eea448de5049',
    '1606417566348740': 'c090c59377342baef5c0eba5d1db2cf78f3fc86247aecbb25f6221cfd2dfbb0d',
    '1606417566452362': '9fed222391eb578e661dbfe84a03b67fac8db44885187c04d166bf891b28c663',
    '1606417566556091': '6ae1862e32dc3186d021286ad685e8411f555c435f5a5b38fcc3300e6e4c66fc',
    '1606417566659805': 'a4235b2e72cce712adbe2748b78fda525b550c5ed729ba2fd0fb866088538778',
    '1606417566763443': '9f1d3eccf76813b7ecf17b9f083779ce44835794e6bbff786039ee09fc45c284',
    '1606417566867142': '7f45e0373477e7f46ab66822f10aee3b6c8fa55137ca4fe3a1e562f745cc6e67',
    '1606417566970871': 'c857fe654bb581548b3b70d6e66747f468df73f3379761cd02299a82b46d527f',
    '1606417567074600': 'ff24ada1cbcf14d479ab3dae138b8df0367e3a75af19b15f6bfdd619a5bca6d2',
    '1606417567178406': '55ce951fc84054aa1fa15be9accd11fa295e23eab2a9754f9fcd3d60e4f4d5ea',
    '1606417567282135': '1ef9520a9ee97b7e47dc2462f5a6b1470e87b1ea4811fccdfebdfef794f3edab',
    '1606417567385864': '2526d8228caf6b2bbd099fb6a5019acc9b451df06b8771f5abff02bc753bc959',
    '1606417567489655': '7c1aba8dadef8a384c370832e8a1eb57f4934cf0143c00dc63fd3ea1dfd5fd75',
    '1606417567593369': '7e6269af85f3c0482b03098a64806525c7a63a6fecb7de794e3900906eaee160',
    '1606417567697083': '7990e9a48e7ab3aa898063bee851405e5c897651407adace0e5fda03f7d07602',
    '1606417567800797': '3e8383d4cdb605a1658edbd58f06a5b47d40cf4b15dbba3ca8f5ff091b71ba36',
    '1606417567904495': '91519ea0e277a2d498ab7f3d7429ec95ad0f0ff4f062efe380b920da1ee57b04',
    '1606417568008209': 'f1eeacc6401ee9f8d8e615d948ff67b5a08a89a1e24ae693d239ecd9d7ff5909',
    '1606417568111939': 'ff0a0ca851f5707ebd3132732f05ed332f22fd515b661f9dfee7f52464bd79c7',
    '1606417568215653': '0650301b9765452f3384e24e3489c2094dba983519a09c40186ec13f83996803',
    '1606417568319306': 'f67b3977237bfd40d1de4229e0a318c3705e22a2103ced67b9d377d30dbbf9d3',
    '1606417568422958': 'bbea3db7a224a8f248e33da7cfbfeb9ce67120d9761e504db09fd07392276bb5',
    '1606417568526611': 'f7d7aef3cce8d8346eefea36e3f534c9fd0c911625df0e3227dc77f20d08dae4',
    '1606417568630234': 'df9964500d5a7dfcfbc8476917f79fa2a08aa1a21e0056a05aeadfd28bb2ee5b',
    '1606417568733872': '14480bbed0bc7ede89ab918ef1cbc836b8a05d35a9d8daa3c6289cee10190ee0',
    '1606417568837601': '19aba8182797d18673d83230876032037ff23c1638964832c2dc51cd25e7378d',
    '1606417568941315': '1e0e4b57cb8abb8b0701b2e5e8b300bfb50ef549fc021873e344038ef0e3fdb1',
    '1606417569044953': 'a6de88ea4788490918fc7e1221976e5c1b1758a745d09501b6798bd42ba557ec',
    '1606417569148605': '1c8a3b0106dbcdaf200e97d873516dbc242ac14e017235251039695989a26cb6',
    '1606417569252350': 'ead68a40fc7f9912a82d9eebd749096ccb15de726769ebce64400e5c7c9c6700',
    '1606417569356140': '2a2cf11850089045088c24893abf0ab2cec19813e7bc8fcc6cec482e655b9f9a',
    '1606417574645126': 'd0e22173ea7eb3048e4c132f94470fcc6f2629c31f96297e06e48d75cbff1830',
    '1606417595386429': 'a6157413853a7001201cbc836778f6708acaa2279ca2c15fb37342a685cf1c65',
    '1606417616127091': 'a775143f8c373f3c773a3c2f2f0155870238acd0c47197d8d301bc56f775b2b7',
    '1606417636868332': '3a9b6ffb9ee9e6459829c5e39e2d84f415f4bbd051f525eb3df6af308075af2b',
    '1606417657609100': '4f76bee66851e5ed2b89f6baaaad18b5b46aa4d67d2bf89d1cc1f5f9ba01f094',
    '1606417678350906': '4475f8423aa22bc1156468474c2421ddca0aad30339885589539e6f7a28ccabb',
    '1606417699091065': 'de7ff32ff835791b9ff439d6f89ba82cd0fa440c00b780a2f27c013657bfc070',
    '1606417719831909': '6c8aff4154fda6a66af43a25692b59126da3364ed02c527d51aa3b320f7adc59',
    '1606417740572998': 'd45b89e85f79be992041f66580ad6ce303f9277edb9c34ae7288cc75892e5916',
    '1606417761313751': '8197c5ec279b00eb4192d1f5b5dce31e946fde58d75de43d5ff03255a9b97e8d',
    '1606417782054718': 'f312d70922bd66ac16008da12a06a16766592d0587aaa27834297227ba941fb5',
    '1606417802795624': '228486d277889c4a5f3ae1274430899404588d1169d683127bd4093428969687',
    '1606417823536438': 'fd7f999e0e53b6b9a83e850890661f6e82eb53e1ab98051cb565830ae5982fa7',
    '1606417844277527': '1ecbe9414b0b53dc4ce15162077b30e7817461833788ef4a9f7b8179de01bf45',
    '1606417865018189': '62f19f5c76d008590f75eeed87be7fb0f3ab9e558110cb115c7e80cfd819364c',
    '1606417885759430': 'd9d2106960e78f01194e52d6dc31cdaa83f7512f5e2e1051b0ccfe31c9b83d15',
    '1606417968722839': '900e11fcfe1e548da346d086c9eb921d98d40cda6fef93353a6f83a1622b4bff',
    '1606417989463440': 'a5fd79f9da41f373bf43598de896e9681e57701b5279afff720fbfe442171cf7',
    '1606418010204193': 'bc4afc20e17a4eda66d82a68f713d96f10451635773c59bdd08e1faf8915c7f0',
    '1606418030945007': '0b5e0080e83d40297e7b6a3f1d1a80a17dc3a550d8bbc49d73fb2b8f91d00d31',
    '1606418051685303': '27aa60e9d6b24fa2f3caf43c3e35c7fc98aede338790fc1d8bb7a9b6a08e42bd',
    '1606418093166687': '67d768c4e622b44c16e21badae8d77dedb263e534f03ed54ab6a91de40767f65',
    '1606418113907837': '712b58542d6dc4a58aeeea9730e068e5ae15204048d50afac69dfc04eec644aa',
}
DATASET = dict(name='Boreas autonomous driving dataset (UTIAS)',
               sensor='MLS vehicule : Velodyne Alpha Prime 128 faisceaux, 10 Hz, poses Applanix post-traitees',
               licence='CC BY 4.0', licence_url='https://www.boreas.utias.utoronto.ca/ ; '
                                                'https://registry.opendata.aws/boreas/',
               access='seau public AWS Open Data s3://boreas, HTTPS anonyme (boreas.s3.amazonaws.com), sans compte',
               density='environ 215 000 retours par trame ; accumulation de trames consecutives')


def rotation(row) -> list:
    r, p, y = float(row[7]), float(row[8]), float(row[9])
    cr, sr, cp, sp, cy, sy = dettrig.cos(r), dettrig.sin(r), dettrig.cos(p), dettrig.sin(p), dettrig.cos(y), \
        dettrig.sin(y)
    roll = [[1.0, 0.0, 0.0], [0.0, cr, sr], [0.0, -sr, cr]]
    pitch = [[cp, 0.0, -sp], [0.0, 1.0, 0.0], [sp, 0.0, cp]]
    yaw = [[cy, sy, 0.0], [-sy, cy, 0.0], [0.0, 0.0, 1.0]]
    return dettrig.matmul3(dettrig.matmul3(roll, pitch), yaw)


def frame_to_enu(points: np.ndarray, row, sign: float = 1.0, transpose: bool = False) -> np.ndarray:
    c = rotation(row)
    if transpose:  # temoin du controle de convention seulement
        c = [list(col) for col in zip(*c)]
    ve, vn, vu = float(row[4]), float(row[5]), float(row[6])
    v = [(c[0][i] * ve + c[1][i] * vn) + c[2][i] * vu for i in range(3)]  # C^T v_enu
    w = [float(row[12]), float(row[11]), float(row[10])]
    x, y, z = (points[:, i].astype(np.float64) for i in range(3))
    t = points[:, 5].astype(np.float64) * sign
    cx = y * w[2]
    cx -= z * w[1]
    cy = z * w[0]
    cy -= x * w[2]
    cz = x * w[1]
    cz -= y * w[0]
    px = x + t * (cx + v[0])
    py = y + t * (cy + v[1])
    pz = z + t * (cz + v[2])
    out = np.empty((len(points), 3), dtype=np.float64)
    for i, ti in enumerate((float(row[1]), float(row[2]), float(row[3]))):
        acc = px * c[i][0]
        acc += py * c[i][1]
        acc += pz * c[i][2]
        acc += ti
        out[:, i] = acc
    return out


def overlap(a: np.ndarray, b: np.ndarray, voxel: float = 0.05) -> float:
    """Part des voxels de 5 cm de b deja occupes par a (points hors sol des deux trames)."""
    def keys(p):
        k = np.floor(p / voxel).astype(np.int64)
        return (k[:, 0] << 42) ^ (k[:, 1] << 21) ^ k[:, 2]
    ka, kb = np.unique(keys(a)), np.unique(keys(b))
    return float(np.isin(kb, ka).mean())


def select_route(rows: list) -> list:
    """Une trame toutes les ROUTE_STEP, en sautant celles a moins de ROUTE_MIN_GAP_M de la precedente retenue
    (arrets du vehicule) : regle deterministe, sans tirage."""
    chosen, last = [], None
    for k in range(0, len(rows), ROUTE_STEP):
        e, n = float(rows[k][1]), float(rows[k][2])
        if last is None or ((e - last[0]) ** 2 + (n - last[1]) ** 2) >= ROUTE_MIN_GAP_M ** 2:
            chosen.append(k)
            last = (e, n)
    return chosen


def load_frames(binary, rows, seq_dir: Path, work: Path):
    frames, enu, ground, counts = [], [], [], []
    for row in rows:
        name = row[0] + '.bin'
        dl = common.download(BUCKET + SEQUENCE + '/lidar/' + name, seq_dir / 'lidar' / name,
                             expected_sha256=FRAME_SHA256.get(row[0]))
        frame = dict(file=name, sha256=dl['sha256'], bytes=dl['bytes'])
        pts = np.fromfile(seq_dir / 'lidar' / name, '<f4').reshape(-1, 6)
        enu.append(frame_to_enu(pts, row))
        tmp = work / ('boreas_' + name)
        np.ascontiguousarray(pts[:, :4]).astype('<f4').tofile(tmp)  # format KITTI pour la sonde
        out = work / (tmp.stem + '.mask')
        report = json.loads(patchwork._run([binary, '--input', tmp, '--output', out,
                                            '--sensor-height', SENSOR_HEIGHT_M]).strip().splitlines()[-1])
        mask = np.fromfile(out, np.uint8)
        frame['mask_sha256'] = common.sha256_file(out)
        frame['ground_counts'] = report.get('counts')
        tmp.unlink()
        out.unlink()
        ground.append(mask == 1)
        counts.append(len(pts))
        frames.append(frame)
    return frames, enu, ground, counts


def write_scene(args, base, kind, rows, frames, enu, ground, counts, poses_dl, probe, check):
    entries = []
    q = common.quantize_float_mm(np.concatenate(enu))
    ids = np.arange(int(sum(counts)), dtype=np.int64)
    g = np.concatenate(ground)
    for variant, keep in (('tout', np.ones(len(q), dtype=bool)), ('sans_sol', ~g)):
        name = base if variant == 'tout' else base + '_sans_sol'
        idx = np.flatnonzero(keep)
        result = common.prepare_outputs(args.out, name, q[idx], ids=ids[idx])
        source = dict(bucket=BUCKET, sequence=SEQUENCE, frames=frames,
                      poses=dict(file='applanix/lidar_poses.csv', sha256=poses_dl['sha256'],
                                 rows=[r[0] for r in rows]))
        conversion = dict(source_type='float32 x y z i laser t (metres, sensor frame)', accumulation=kind,
                          motion_compensation='first order per point towards the frame time: '
                                              'p + t (v + w x p), pyboreas body rate convention',
                          pose='p_enu = C p + r, C = roll(r) pitch(p) yaw(y) (pyboreas get_transform), '
                               'deterministic sin/cos, float64 elementwise, fixed order',
                          ground=dict(method='Patchwork++ (probe v8, upstream %s)' % patchwork.COMMIT,
                                      sensor_height_m=SENSOR_HEIGHT_M, policy='remove only 1, keep 0 and 2',
                                      per_frame=True) if variant == 'sans_sol' else None,
                          ids='frame offset in the scene + point index in the frame', variant=variant,
                          pose_check=check)
        manifest = common.scene_manifest(name, 'multi_millions', DATASET, source, conversion, result,
                                         __file__, extra=dict(probe=probe if variant == 'sans_sol' else None))
        common.write_json(args.out / (name + '.manifest.json'), manifest)
        entries.append(lasconv.set_entry(manifest))
        me = result['measures']
        common.log('%s : %d points, %d distincts, %d bits' % (name, me['n_points'], me['n_distinct'],
                                                               me['bits_needed']))
    return entries


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--raw', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--archives', type=Path, required=True)
    parser.add_argument('--work', type=Path, required=True)
    parser.add_argument('--check', action='store_true')
    parser.add_argument('--no-route', action='store_true', help='sans la scene itineraire (50 trames de plus)')
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    args.work.mkdir(parents=True, exist_ok=True)
    common.selftest_quantize()
    seq_dir = args.raw / SEQUENCE
    poses_dl = common.download(BUCKET + SEQUENCE + '/applanix/lidar_poses.csv', seq_dir / 'lidar_poses.csv',
                               expected_sha256=POSES_SHA256)
    all_rows = list(csv.reader(open(seq_dir / 'lidar_poses.csv')))
    if all_rows[0][:4] != ['GPSTime', 'easting', 'northing', 'altitude']:
        raise RuntimeError('en-tete de lidar_poses.csv inattendu')
    all_rows = all_rows[1:]
    binary, probe = patchwork.build_probe(args.archives, args.work)
    control = Path(args.archives) / '08_000000.bin'
    if control.is_file():
        _, mask0, _ = patchwork.ground_mask(binary, control, args.work)
        probe['mask_08_000000'] = mask0
        if mask0 != patchwork.MASK_08_000000:
            raise RuntimeError('controle de sonde faux')
    rows = all_rows[START:START + COUNT]
    frames, enu, ground, counts = load_frames(binary, rows, seq_dir, args.work)
    check = None
    if args.check:
        check = {}
        for label, sign, transpose in (('sign_+1', 1.0, False), ('sign_+0', 0.0, False), ('sign_-1', -1.0, False),
                                       ('rotation_transposed', 1.0, True)):
            vals = []
            for k in range(0, COUNT - 1, 7):
                a = frame_to_enu(np.fromfile(seq_dir / 'lidar' / frames[k]['file'], '<f4').reshape(-1, 6),
                                 rows[k], sign, transpose)
                b = frame_to_enu(np.fromfile(seq_dir / 'lidar' / frames[k + 1]['file'], '<f4').reshape(-1, 6),
                                 rows[k + 1], sign, transpose)
                vals.append(overlap(a[~ground[k]], b[~ground[k + 1]]))
            check[label] = float(np.mean(vals))
        common.log('controle de pose et de compensation : %s' % check)
        if not check['sign_+1'] >= max(check.values()):
            raise RuntimeError('convention de pose ou de compensation refutee par le recouvrement : %s' % check)
    tag = SEQUENCE.replace('boreas-', '').replace('-', '')
    entries = []
    for n in WINDOWS:
        entries += write_scene(args, 'boreas_%s_f%d_n%d' % (tag, START, n),
                               '%d consecutive frames from frame %d, local ENU frame of the sequence' % (n, START),
                               rows[:n], frames[:n], enu[:n], ground[:n], counts[:n], poses_dl, probe, check)
    all_frames = list(frames)
    if not args.no_route:
        picks = select_route(all_rows)
        r_rows = [all_rows[k] for k in picks]
        r_frames, r_enu, r_ground, r_counts = load_frames(binary, r_rows, seq_dir, args.work)
        all_frames += r_frames
        entries += write_scene(args, 'boreas_%s_route_s%d' % (tag, ROUTE_STEP),
                               'route: one frame every %d (frame indices %s), frames closer than %g m to the '
                               'previous pick skipped, local ENU frame' % (ROUTE_STEP, picks, ROUTE_MIN_GAP_M),
                               r_rows, r_frames, r_enu, r_ground, r_counts, poses_dl, probe, check)
    manifest_path = args.out / 'manifest.json'
    old = json.loads(manifest_path.read_text()) if manifest_path.is_file() else {}
    cases = {c['name']: c for c in old.get('cases', [])}
    cases.update({e['name']: e for e in entries})
    out = dict(schema=common.SCHEMA_SET, family='multi_millions', dataset=DATASET,
               cases=[cases[k] for k in sorted(cases)], generator=common.generator_info(__file__),
               created_utc=common.utc_now(), public_status='not_claimed')
    if old.get('crops'):
        out['crops'] = old['crops']
    common.write_json(manifest_path, out)
    pins = dict(poses=poses_dl['sha256'], frames={f['file'][:-4]: f['sha256'] for f in all_frames})
    common.write_json(args.work / 'boreas_pins.json', pins)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
