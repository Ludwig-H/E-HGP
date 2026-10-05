"""Aides des portes de l'executable mhgp11 (hors produit) : entrees u32le, appels du CLI et de la sonde de reference
bench/full_probe.cpp, lecture de la ligne JSON, nuages deterministes. Python 3.10 nu, aucun assert.
"""
import hashlib
import json
import os
import random
import struct
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, '..', '..'))
sys.path.insert(0, os.path.join(ROOT, 'tests', 'support'))
sys.path.insert(0, os.path.join(ROOT, 'bench'))
import mhgp11_gate  # noqa: E402
import mhgp11_formats as formats  # noqa: E402

UNLIMITED = (1 << 64) - 1
ENGINE_MASK = 16379  # parametres fixes de l'api : masque des sondes (src/api/internal.hpp)
NONE = (1 << 32) - 1


def write_inputs(folder, points, ids, order=None, prefix=''):
    """Ecrit <prefix>points.u32le et <prefix>ids.u32le dans l'ordre `order` (indices) ; rend les deux chemins."""
    order = tuple(range(len(points))) if order is None else tuple(order)
    xyz = os.path.join(folder, prefix + 'points.u32le')
    names = os.path.join(folder, prefix + 'ids.u32le')
    with open(xyz, 'wb') as handle:
        handle.write(b''.join(struct.pack('<III', *points[i]) for i in order))
    with open(names, 'wb') as handle:
        handle.write(b''.join(struct.pack('<I', ids[i]) for i in order))
    return xyz, names


def sha256_of(path):
    return formats.sha256_file(path)


def cli_argv(cli, xyz, names, directory, k, workers=None, extra=()):
    argv = [cli, '--sortie=full', '--points=' + xyz, '--ids=' + names, '--dossier=' + directory, '--k=%d' % k]
    if workers is not None:
        argv.append('--fils=%d' % workers)
    return argv + list(extra)


def json_lines(text):
    """Lignes JSON d'une sortie ; une ligne illisible donne None a sa place."""
    rows = []
    for line in (text or '').splitlines():
        try:
            rows.append(json.loads(line))
        except ValueError:
            rows.append(None)
    return rows


def run_cli(argv, timeout=600, **options):
    """Lance le CLI ; rend (issue, lignes JSON de la sortie standard)."""
    result = mhgp11_gate.run(argv, timeout=timeout, **options)
    return result, json_lines(result.stdout)


def run_bench(bench, xyz, names, dump, k, workers, timeout=600):
    """Sonde de reference : meme tour, dump MHGP11FUL1 (bench/full_probe.cpp, masque 16379, feuilles 16 a 256)."""
    argv = [bench, xyz, names, dump, str(k), '16', '256', '0', str(NONE), str(UNLIMITED), str(workers),
            str(ENGINE_MASK)]
    result = mhgp11_gate.run(argv, timeout=timeout)
    return result, json_lines(result.stdout)


def uniform_u18(count, seed=20261002):
    """Famille uniform_u18 des bancs (random.Random(graine).getrandbits(18), x puis y puis z), PointId 0..n-1."""
    rng = random.Random(seed)
    points = [(rng.getrandbits(18), rng.getrandbits(18), rng.getrandbits(18)) for _ in range(count)]
    return points, list(range(count))


def lidar(name):
    """Trame du dossier MHGP11_DATA_DIR : (points, ids) ; jamais copiee hors de ce dossier."""
    folder = mhgp11_gate.require_data_dir()
    with open(os.path.join(folder, name + '.u32le'), 'rb') as handle:
        raw = handle.read()
    with open(os.path.join(folder, name + '.ids.u32le'), 'rb') as handle:
        names = handle.read()
    points = [struct.unpack_from('<III', raw, 12 * i) for i in range(len(raw) // 12)]
    ids = [struct.unpack_from('<I', names, 4 * i)[0] for i in range(len(names) // 4)]
    return points, ids


def distinct_ids(count, rng, include_extremes=True):
    """count PointId distincts, non denses ; 0xFFFFFFFF et 0 en tete si demande."""
    chosen = [NONE, 0] if include_extremes else []
    seen = set(chosen)
    while len(chosen) < count:
        value = rng.getrandbits(32)
        if value not in seen:
            seen.add(value)
            chosen.append(value)
    result = chosen[:count]
    rng.shuffle(result)
    return result


def tree_digest_of(directory):
    with open(os.path.join(directory, formats.MANIFEST), 'rb') as handle:
        return formats.read_manifest(handle.read())['tree_k_sha256']


def file_sha(path):
    with open(path, 'rb') as handle:
        return hashlib.sha256(handle.read()).hexdigest()
