#!/usr/bin/env python3
"""Gel des scenes synthetiques de l'experience E1 (dev et test) : grille, verite, etiquettes MAP, manifeste sha256.

    python3 bench/points_scenes_freeze.py --out /workspaces/E-HGP/build/v11-persist/e1_scenes
        [--only dev|test] [--sizes 8000,16000,32000] [--reps 3] [--check-only] [--replay-sample 16]

Cadre : phase=exploration_v11_hors_registre, backend=cpu_reference, profile=quantized_u18_input_only,
public_status=not_claimed. Specification : build/v11-points-select/juge/SPEC.md, par. 3.2 (plan synthetique, graines,
scenes gelees, ensembles dev et test) et par. 4.1. Generation locale en O(n) par scene, AUCUN clustering.

Plan (SPEC par. 3.2). Cellules : 8 familles x {medium, hard} x {3, 8, 20} groupes x {8 000, 16 000, 32 000} points,
bruit 5 % ; familles et niveaux nommes du generateur epingle (FAMILIES, SEPARATION de
morsehgp3D_v11/receipts/full_points_20261003/experiment/vendor_scenes.py, sha256 61ea9abc..., verifies a
l'execution). Dev : 48 cellules x {8 000 ; 16 000} x 2 repetitions = 192 scenes. Test : 144 cellules x R (R = 3 par
defaut, --reps) = 432 scenes.

Graines (chaine exacte). Pour une scene de l'ensemble E (prefixe 'dev_v11e1' ou 'test_v11e1'), de cellule
c = {"family", "groups", "level", "n", "noise_fraction"} et de repetition r (1, 2, ...) :
    texte = prefixe + '|' + json.dumps(c, sort_keys=True, separators=(',', ':')) + '|' + str(r)
    h     = sha256(texte encode en ASCII)
    graine = int.from_bytes(h[0:8], 'big') >> 1          (63 bits : le generateur exige 0 <= graine < 2^63)
Exemple : 'test_v11e1|{"family":"spherical","groups":8,"level":"hard","n":16000,"noise_fraction":0.05}|1'.
Toutes les graines des deux ensembles (au complet, quel que soit --only) doivent etre distinctes entre elles et
distinctes des graines du developpement passe (9341, 9342 de bench/points_campaign.py ; 9331 a 9333 du recu
full_points_20261003 ; 11 a 15 du point de calibration du generateur) : sinon refus, code 2.

Grille : quantize18 du generateur (pas isotrope sur 18 bits, doublons de position retires, premiere occurrence
gardee, ordre du generateur conserve). PointId = rang dans l'ordre du fichier. Etiquettes MAP : points_map.scene,
calculees sur les MEMES flottants avant quantification, puis restreintes aux sites gardes ; la porte de rejeu de
points_map refuse toute scene dont l'empreinte flottante rejouee differe du generateur.

Fichiers, dans <out>/<ensemble>/ :
    <nom>.sites.u32le   n x 4 uint32 petit-boutiste : x, y, z, PointId
    <nom>.truth.i32le   n int32 : groupe >= 0, -1 = bruit vrai
    <nom>.map.i32le     n int32 : etiquette MAP, -1 = classe bruit
    manifest.json       par scene : nom, ensemble, famille, niveau, groupes, n demande, n sites, repetition,
                        graine (entier et hex), texte de graine, spec complete, sha256 et octets de chaque fichier,
                        niveau de Bayes (mIoU_h du MAP contre la verite) et sa strate ; sha256 du generateur, de ce
                        script, de points_map.py et de points_flat_metrics.py ; versions.
Archives, dans <out>/ : <ensemble>_<taille>.tar (dev_8000.tar, ..., test_32000.tar), tar USTAR non compresse et
deterministe (mtime 0, uid/gid 0, mode 0644, membres tries), membres a nom simple (aucun sous-dossier), manifeste
de l'archive inclus sous le nom <ensemble>_<taille>.manifest.json (sous-ensemble du manifeste de l'ensemble) ;
au plus 3 x 144 + 1 = 433 membres par archive (limite de session G4 : 512 fichiers). <out>/archives.json recense
chaque archive (sha256, octets, membres).

Le manifeste porte aussi, par cellule, le niveau de Bayes moyen sur les repetitions et la strate de cette moyenne
(`cells`) ; les strates des scenes d'une meme cellule peuvent differer. Lecture cote campagne : read_scene(dossier,
entree) verifie sha256 et taille avant de rendre (xyz, PointId, verite, MAP) ; elle n'importe pas le generateur.

Controles : relecture de chaque fichier et de chaque membre d'archive contre son sha256 ; grille < 2^18, sites
distincts, PointId = rang, etiquettes dans [-1, groupes) ; rejeu d'un echantillon de scenes (--replay-sample)
regenere les trois fichiers et les compare octet pour octet. Codes : 0 conforme ; 2 refus (graines, generateur,
porte de rejeu) ; 3 controle echoue.

BLAS : les produits matriciels du generateur dependent du BLAS ; lance en script, on fixe un seul fil
(OPENBLAS_NUM_THREADS, OMP_NUM_THREADS, MKL_NUM_THREADS = 1, si non deja fixes) avant d'importer numpy, pour qu'un
rejeu sur la meme machine soit identique (1 et 8 fils donnent ici les memes empreintes) ; la configuration est
consignee. La VM ne regenere jamais : elle lit les fichiers geles.
Aucune decision par assert ; Python >= 3.10.
"""
import os

if __name__ == '__main__':  # en script seulement : un import (campagne) ne touche pas aux fils du BLAS
    for _var in ('OPENBLAS_NUM_THREADS', 'OMP_NUM_THREADS', 'MKL_NUM_THREADS'):
        os.environ.setdefault(_var, '1')

import argparse  # noqa: E402
import hashlib  # noqa: E402
import io  # noqa: E402
import json  # noqa: E402
import platform  # noqa: E402
import sys  # noqa: E402
import tarfile  # noqa: E402
import time  # noqa: E402

import numpy as np  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import points_flat_metrics as fm  # noqa: E402
import points_map as pm  # noqa: E402

SCHEMA = 'ehgp.v11.e1_scenes.manifest.v1'
FAMILIES = ('spherical', 'anisotropic', 'heteroscedastic', 'unbalanced', 'shells', 'bridge', 'hierarchical',
            'filaments')
LEVELS = ('medium', 'hard')
GROUPS = (3, 8, 20)
SIZES = (8000, 16000, 32000)
NOISE = 0.05
ENSEMBLES = dict(dev=dict(prefix='dev_v11e1', tag='d', sizes=(8000, 16000), reps=2),
                 test=dict(prefix='test_v11e1', tag='t', sizes=SIZES, reps=None))
PAST_SEEDS = frozenset([9341, 9342, 9331, 9332, 9333, 11, 12, 13, 14, 15])
SUFFIXES = (('sites', '.sites.u32le'), ('truth', '.truth.i32le'), ('map', '.map.i32le'))
MAX_MEMBERS = 512


class Refusal(Exception):
    pass


def sha_bytes(data):
    return hashlib.sha256(data).hexdigest()


def sha_file(path):
    h = hashlib.sha256()
    with open(path, 'rb') as stream:
        for block in iter(lambda: stream.read(1 << 20), b''):
            h.update(block)
    return h.hexdigest()


def cell_text(prefix, cell, rep):
    return prefix + '|' + json.dumps(cell, sort_keys=True, separators=(',', ':')) + '|' + str(rep)


def seed_of(text):
    digest = hashlib.sha256(text.encode('ascii')).digest()
    return int.from_bytes(digest[:8], 'big') >> 1, digest.hex()


def plan(ensemble, reps_test):
    """Scenes planifiees d'un ensemble (au complet) : liste de dicts (nom, cellule, repetition, graine...)."""
    conf = ENSEMBLES[ensemble]
    reps = conf['reps'] if conf['reps'] is not None else reps_test
    out = []
    for n in conf['sizes']:
        for family in FAMILIES:
            for level in LEVELS:
                for groups in GROUPS:
                    cell = dict(family=family, groups=groups, level=level, n=n, noise_fraction=NOISE)
                    for rep in range(1, reps + 1):
                        text = cell_text(conf['prefix'], cell, rep)
                        seed, digest = seed_of(text)
                        name = '%s_%s_%s_g%d_n%d_r%d' % (conf['tag'], family, level, groups, n, rep)
                        out.append(dict(name=name, ensemble=ensemble, family=family, level=level, groups=groups,
                                        n_requested=n, repetition=rep, seed=seed, seed_hex='%016x' % seed,
                                        seed_text=text, seed_sha256=digest, spec=dict(cell, seed=seed)))
    return out


def check_seeds(reps_test):
    """Graines distinctes entre ensembles et du developpement passe ; noms uniques. Refus sinon."""
    seen = {}
    for ensemble in ENSEMBLES:
        for item in plan(ensemble, reps_test):
            if item['seed'] in PAST_SEEDS:
                raise Refusal('graine %d du developpement passe (%s)' % (item['seed'], item['name']))
            if item['seed'] in seen:
                raise Refusal('collision de graines : %s et %s' % (seen[item['seed']], item['name']))
            seen[item['seed']] = item['name']
    names = list(seen.values())
    if len(set(names)) != len(names):
        raise Refusal('noms de scene non uniques')
    return len(seen)


def check_generator():
    vs = pm.generator()  # verifie le sha256 epingle
    if tuple(vs.FAMILIES) != FAMILIES:
        raise Refusal('familles du generateur %s' % (vs.FAMILIES,))
    for level in LEVELS:
        if level not in vs.LEVELS or any(level not in vs.SEPARATION[f] for f in FAMILIES):
            raise Refusal('niveau %s absent du generateur' % level)
    return vs


def scene_payload(spec):
    """Les trois fichiers d'une scene (octets), plus les mesures du manifeste. Porte de rejeu dans pm.scene."""
    sc = pm.scene(spec)
    grid = np.asarray(sc['grid'], dtype=np.uint32)
    n = len(grid)
    sites = np.empty((n, 4), dtype='<u4')
    sites[:, :3] = grid
    sites[:, 3] = np.arange(n, dtype=np.uint32)
    truth = np.asarray(sc['truth'], dtype=np.int64)
    mp = np.asarray(sc['map'], dtype=np.int64)
    data = dict(sites=sites.tobytes(), truth=truth.astype('<i4').tobytes(), map=mp.astype('<i4').tobytes())
    bayes, per_group, _ = fm.miou_hungarian(truth, mp)
    info = dict(n_sites=n, dropped_duplicates=sc['dropped'], step=sc['step'],
                float_points_sha256=sc['meta']['digest'], separation=float(sc['meta']['separation']),
                truth_groups=int(len(np.unique(truth[truth >= 0]))), truth_noise=int(np.sum(truth < 0)),
                map_noise=int(np.sum(mp < 0)), map_groups=int(len(np.unique(mp[mp >= 0]))),
                map_min_margin=float(np.min(sc['margin'])), map_margin_below_1e6=int(np.sum(sc['margin'] < 1e-6)),
                bayes_level=bayes, bayes_stratum=fm.bayes_stratum(bayes),
                bayes_iou_per_group=[round(x, 6) for x in per_group])
    return data, info


def write_atomic(path, data):
    tmp = path + '.tmp'
    with open(tmp, 'wb') as stream:
        stream.write(data)
    os.replace(tmp, path)


def validate_arrays(item, data):
    """Controles de contenu d'une scene : grille 18 bits, sites distincts, PointId = rang, etiquettes bornees."""
    errors = []
    sites = np.frombuffer(data['sites'], dtype='<u4').reshape(-1, 4)
    truth = np.frombuffer(data['truth'], dtype='<i4')
    mp = np.frombuffer(data['map'], dtype='<i4')
    n = len(sites)
    if len(truth) != n or len(mp) != n or n != item['n_sites']:
        errors.append('%s : longueurs' % item['name'])
        return errors
    if int(sites[:, :3].max()) >= (1 << 18):
        errors.append('%s : grille hors de 18 bits' % item['name'])
    if not np.array_equal(sites[:, 3], np.arange(n, dtype=np.uint32)):
        errors.append('%s : PointId different du rang' % item['name'])
    if len(np.unique(sites[:, :3], axis=0)) != n:
        errors.append('%s : sites non distincts' % item['name'])
    g = item['groups']
    for label, arr in (('verite', truth), ('MAP', mp)):
        if int(arr.min()) < -1 or int(arr.max()) >= g:
            errors.append('%s : etiquettes %s hors de [-1, %d)' % (item['name'], label, g))
    return errors


def tar_bytes(members):
    """Archive tar USTAR deterministe depuis une liste (nom, octets)."""
    buf = io.BytesIO()
    with tarfile.open(fileobj=buf, mode='w', format=tarfile.USTAR_FORMAT) as tar:
        for name, data in members:
            info = tarfile.TarInfo(name)
            info.size = len(data)
            info.mtime = 0
            info.mode = 0o644
            info.uid = info.gid = 0
            info.uname = info.gname = ''
            tar.addfile(info, io.BytesIO(data))
    return buf.getvalue()


def header(reps_test):
    def own(path):
        return sha_file(path)
    blas = None
    try:
        blas = np.show_config(mode='dicts').get('Build Dependencies', {}).get('blas')
    except Exception:  # noqa: BLE001 - consigne seulement
        blas = None
    return dict(schema=SCHEMA, public_status='not_claimed', phase='exploration_v11_hors_registre',
                profile='quantized_u18_input_only',
                generator=dict(path='morsehgp3D_v11/receipts/full_points_20261003/experiment/vendor_scenes.py',
                               sha256=pm.GENERATOR_SHA256),
                code=dict(points_scenes_freeze=own(os.path.abspath(__file__)),
                          points_map=own(os.path.join(HERE, 'points_map.py')),
                          points_flat_metrics=own(os.path.join(HERE, 'points_flat_metrics.py'))),
                seed_chain=("seed = int.from_bytes(sha256((prefix + '|' + json.dumps(cell, sort_keys=True, "
                            "separators=(',', ':')) + '|' + str(rep)).encode('ascii')).digest()[:8], 'big') >> 1 ; "
                            "cell = {family, groups, level, n, noise_fraction} ; rep from 1"),
                prefixes=dict((e, ENSEMBLES[e]['prefix']) for e in ENSEMBLES), past_seeds_refused=sorted(PAST_SEEDS),
                plan=dict(families=list(FAMILIES), levels=list(LEVELS), groups=list(GROUPS), noise_fraction=NOISE,
                          dev=dict(sizes=list(ENSEMBLES['dev']['sizes']), reps=ENSEMBLES['dev']['reps']),
                          test=dict(sizes=list(SIZES), reps=reps_test)),
                files=dict(sites='n x 4 uint32 little-endian : x, y, z, PointId (rang dans le fichier)',
                           truth='n int32 little-endian : groupe >= 0, -1 bruit vrai',
                           map='n int32 little-endian : etiquette MAP, -1 classe bruit'),
                grid='quantize18 du generateur epingle (pas isotrope 18 bits, doublons retires, ordre conserve)',
                environment=dict(python=platform.python_version(), numpy=np.__version__,
                                 scipy=__import__('scipy').__version__, machine=platform.machine(),
                                 blas=blas, blas_threads=dict((v, os.environ.get(v)) for v in
                                                              ('OPENBLAS_NUM_THREADS', 'OMP_NUM_THREADS',
                                                               'MKL_NUM_THREADS'))))


def read_scene(folder, item):
    """Lecture verifiee d'une scene gelee (entree `item` d'un manifeste) : refuse si un sha256 ou une taille differe.
    Rend (xyz uint32 (n, 3), PointId uint32 (n,), verite int64 (n,), MAP int64 (n,)), dans l'ordre du fichier."""
    data = {}
    for role, _ in SUFFIXES:
        entry = item['files'][role]
        with open(os.path.join(folder, entry['name']), 'rb') as stream:
            data[role] = stream.read()
        if len(data[role]) != entry['bytes'] or sha_bytes(data[role]) != entry['sha256']:
            raise Refusal('%s : sha256 ou taille differents du manifeste' % entry['name'])
    sites = np.frombuffer(data['sites'], dtype='<u4').reshape(-1, 4)
    return (np.ascontiguousarray(sites[:, :3]), sites[:, 3].copy(),
            np.frombuffer(data['truth'], dtype='<i4').astype(np.int64),
            np.frombuffer(data['map'], dtype='<i4').astype(np.int64))


def cells_of(scenes):
    """Niveau de Bayes par cellule (famille, niveau, groupes, n) : moyenne, extremes et strate de la moyenne ; les
    strates des scenes d'une meme cellule peuvent differer (la convention de strate se fixe au preenregistrement)."""
    by = {}
    for item in scenes:
        key = '%s_%s_g%d_n%d' % (item['family'], item['level'], item['groups'], item['n_requested'])
        by.setdefault(key, []).append(item)
    out = []
    for key in sorted(by):
        levels = [x['bayes_level'] for x in by[key]]
        mean = float(np.mean(levels))
        out.append(dict(cell=key, scenes=[x['name'] for x in by[key]], bayes_mean=mean, bayes_min=min(levels),
                        bayes_max=max(levels), stratum_of_mean=fm.bayes_stratum(mean),
                        scene_strata=sorted(set(x['bayes_stratum'] for x in by[key]))))
    return out


def freeze(args):
    check_generator()
    total_seeds = check_seeds(args.reps)
    sizes = set(args.sizes)
    started = time.monotonic()
    report = dict(seeds_checked=total_seeds, ensembles={})
    ensembles = [args.only] if args.only else list(ENSEMBLES)
    for ensemble in ensembles:
        folder = os.path.join(args.out, ensemble)
        os.makedirs(folder, exist_ok=True)
        items = [item for item in plan(ensemble, args.reps) if item['n_requested'] in sizes]
        complete = set(ENSEMBLES[ensemble]['sizes']) <= sizes
        scenes, t0, nbytes, timing = [], time.monotonic(), 0, {}
        for index, item in enumerate(items):
            t1 = time.monotonic()
            data, info = scene_payload(item['spec'])
            item = dict(item, **info)
            errors = validate_arrays(item, data)
            if errors:
                raise RuntimeError('; '.join(errors))
            item['files'] = {}
            for role, suffix in SUFFIXES:
                path = os.path.join(folder, item['name'] + suffix)
                write_atomic(path, data[role])
                item['files'][role] = dict(name=item['name'] + suffix, sha256=sha_bytes(data[role]),
                                           bytes=len(data[role]))
                nbytes += len(data[role])
            timing.setdefault(item['n_requested'], []).append(time.monotonic() - t1)
            scenes.append(item)
            if (index + 1) % 24 == 0 or index + 1 == len(items):
                print('  %s %d/%d scenes, %.1f s' % (ensemble, index + 1, len(items), time.monotonic() - t0),
                      flush=True)
        manifest = dict(header(args.reps), ensemble=ensemble, sizes=sorted(sizes & set(ENSEMBLES[ensemble]['sizes'])),
                        complete=complete, scenes=scenes, cells=cells_of(scenes))
        write_atomic(os.path.join(folder, 'manifest.json'),
                     (json.dumps(manifest, indent=1, sort_keys=True) + '\n').encode('utf-8'))
        report['ensembles'][ensemble] = dict(
            scenes=len(scenes), seconds=round(time.monotonic() - t0, 2), data_bytes=nbytes,
            seconds_per_scene=dict((str(k), dict(mean=round(float(np.mean(v)), 4), max=round(float(np.max(v)), 4)))
                                   for k, v in sorted(timing.items())))
        for size in sorted(sizes & set(ENSEMBLES[ensemble]['sizes'])):
            sub = [s for s in scenes if s['n_requested'] == size]
            arch = '%s_%d' % (ensemble, size)
            sub_manifest = dict(header(args.reps), ensemble=ensemble, archive=arch + '.tar', sizes=[size],
                                complete=True, scenes=sub, cells=cells_of(sub))
            members = [(arch + '.manifest.json',
                        (json.dumps(sub_manifest, indent=1, sort_keys=True) + '\n').encode('utf-8'))]
            for s in sorted(sub, key=lambda x: x['name']):
                for role, _ in SUFFIXES:
                    with open(os.path.join(folder, s['files'][role]['name']), 'rb') as stream:
                        members.append((s['files'][role]['name'], stream.read()))
            if len(members) > MAX_MEMBERS:
                raise Refusal('%s : %d membres > %d' % (arch, len(members), MAX_MEMBERS))
            write_atomic(os.path.join(args.out, arch + '.tar'), tar_bytes(members))
    report['seconds'] = round(time.monotonic() - started, 2)
    return report


def check(args):
    """Relecture : fichiers et membres d'archive contre les sha256 ; contenus ; rejeu d'un echantillon."""
    errors, counts = [], dict(files=0, members=0, replayed=0, scenes=0)
    archives = []
    for ensemble in ENSEMBLES:
        path = os.path.join(args.out, ensemble, 'manifest.json')
        if not os.path.exists(path):
            continue
        with open(path) as stream:
            manifest = json.load(stream)
        if manifest.get('schema') != SCHEMA:
            errors.append('%s : schema' % path)
            continue
        if manifest['generator']['sha256'] != pm.GENERATOR_SHA256:
            errors.append('%s : generateur' % path)
        by_name = {}
        for item in manifest['scenes']:
            counts['scenes'] += 1
            by_name[item['name']] = item
            data = {}
            for role, _ in SUFFIXES:
                entry = item['files'][role]
                with open(os.path.join(args.out, ensemble, entry['name']), 'rb') as stream:
                    data[role] = stream.read()
                counts['files'] += 1
                if sha_bytes(data[role]) != entry['sha256'] or len(data[role]) != entry['bytes']:
                    errors.append('%s : sha256 ou taille' % entry['name'])
            errors.extend(validate_arrays(item, data))
            seed, _ = seed_of(item['seed_text'])
            if seed != item['seed'] or item['spec'].get('seed') != seed:
                errors.append('%s : graine' % item['name'])
        for size in manifest['sizes']:
            arch = '%s_%d' % (ensemble, size)
            tar_path = os.path.join(args.out, arch + '.tar')
            if not os.path.exists(tar_path):
                errors.append('%s absente' % tar_path)
                continue
            seen = set()
            with tarfile.open(tar_path) as tar:
                for member in tar.getmembers():
                    name = member.name
                    if not member.isfile() or '/' in name or name.startswith('.'):
                        errors.append('%s : membre %r' % (arch, name))
                        continue
                    blob = tar.extractfile(member).read()
                    counts['members'] += 1
                    seen.add(name)
                    if name == arch + '.manifest.json':
                        sub = json.loads(blob.decode('utf-8'))
                        want = sorted(s['name'] for s in manifest['scenes'] if s['n_requested'] == size)
                        if sorted(s['name'] for s in sub['scenes']) != want:
                            errors.append('%s : manifeste d archive incomplet' % arch)
                        continue
                    stem, role = name.split('.')[0], name.split('.')[1]
                    item = by_name.get(stem)
                    if item is None or sha_bytes(blob) != item['files'][role]['sha256']:
                        errors.append('%s : membre %s different du manifeste' % (arch, name))
            expected = 1 + 3 * sum(1 for s in manifest['scenes'] if s['n_requested'] == size)
            if len(seen) != expected or len(seen) > MAX_MEMBERS:
                errors.append('%s : %d membres, attendu %d' % (arch, len(seen), expected))
            archives.append(dict(archive=arch + '.tar', sha256=sha_file(tar_path), bytes=os.path.getsize(tar_path),
                                 members=len(seen)))
        # rejeu d'un echantillon deterministe : une scene par (taille, famille) au plus, jusqu'a --replay-sample
        sample, keys = [], set()
        for item in sorted(manifest['scenes'], key=lambda s: s['seed_sha256']):
            key = (item['n_requested'], item['family'])
            if key not in keys and len(sample) < args.replay_sample:
                keys.add(key)
                sample.append(item)
        for item in sample:
            try:
                read_scene(os.path.join(args.out, ensemble), item)
            except Refusal as error:
                errors.append(str(error))
            data, _ = scene_payload(item['spec'])
            counts['replayed'] += 1
            for role, _ in SUFFIXES:
                if sha_bytes(data[role]) != item['files'][role]['sha256']:
                    errors.append('%s : rejeu different (%s)' % (item['name'], role))
    return errors, counts, archives


def summary(out):
    """Niveaux de Bayes par strate (et par famille x niveau), depuis les manifestes."""
    rows = {}
    for ensemble in ENSEMBLES:
        path = os.path.join(out, ensemble, 'manifest.json')
        if not os.path.exists(path):
            continue
        with open(path) as stream:
            manifest = json.load(stream)
        strata, cells = {}, {}
        for item in manifest['scenes']:
            strata[item['bayes_stratum']] = strata.get(item['bayes_stratum'], 0) + 1
            key = '%s/%s' % (item['family'], item['level'])
            cells.setdefault(key, []).append(item['bayes_level'])
        cell_strata = {}
        mixed = 0
        for cell in manifest.get('cells', []):
            cell_strata[cell['stratum_of_mean']] = cell_strata.get(cell['stratum_of_mean'], 0) + 1
            mixed += len(cell['scene_strata']) > 1
        rows[ensemble] = dict(scenes=len(manifest['scenes']), strata=strata, cells=len(manifest.get('cells', [])),
                              cell_strata_of_mean=cell_strata, cells_with_mixed_scene_strata=mixed,
                              by_family_level=dict((k, dict(mean=round(float(np.mean(v)), 4),
                                                            min=round(float(np.min(v)), 4),
                                                            max=round(float(np.max(v)), 4), scenes=len(v)))
                                                   for k, v in sorted(cells.items())))
    return rows


def main():
    parser = argparse.ArgumentParser(description='Gel des scenes synthetiques E1')
    parser.add_argument('--out', required=True)
    parser.add_argument('--only', choices=('dev', 'test'))
    parser.add_argument('--sizes', default=','.join(map(str, SIZES)))
    parser.add_argument('--reps', type=int, default=3, help='repetitions du test (R, SPEC par. 3.6.5)')
    parser.add_argument('--check-only', action='store_true')
    parser.add_argument('--replay-sample', type=int, default=16)
    args = parser.parse_args()
    try:
        args.sizes = sorted(set(int(x) for x in args.sizes.split(',') if x))
        if not args.sizes or any(s not in SIZES for s in args.sizes):
            raise Refusal('--sizes dans %s' % (SIZES,))
        if not 1 <= args.reps <= 16:
            raise Refusal('--reps dans 1..16')
        os.makedirs(args.out, exist_ok=True)
        report = {}
        if not args.check_only:
            report['freeze'] = freeze(args)
    except (Refusal, ValueError) as error:
        print('refus :', error)
        return 2
    t0 = time.monotonic()
    errors, counts, archives = check(args)
    report['check'] = dict(counts, seconds=round(time.monotonic() - t0, 2), errors=errors[:50],
                           error_count=len(errors))
    index = os.path.join(args.out, 'archives.json')
    previous = {}
    if os.path.exists(index):
        with open(index) as stream:
            previous = dict((a['archive'], a) for a in json.load(stream).get('archives', []))
    for a in archives:
        previous[a['archive']] = a
    listing = dict(schema=SCHEMA + '.archives', archives=sorted(previous.values(), key=lambda a: a['archive']))
    write_atomic(index, (json.dumps(listing, indent=1, sort_keys=True) + '\n').encode('utf-8'))
    report['archives'] = archives
    report['bayes'] = summary(args.out)
    print(json.dumps(report, indent=1, sort_keys=True))
    print('points_scenes_freeze', 'conforme' if not errors else 'NON CONFORME (%d)' % len(errors))
    return 0 if not errors else 3


if __name__ == '__main__':
    sys.exit(main())
