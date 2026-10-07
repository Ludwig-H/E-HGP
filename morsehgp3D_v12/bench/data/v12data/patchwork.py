"""Retrait du sol Patchwork++ (sonde v8 epinglee, amont 3e6903a1), compilee depuis deux archives :
`patchwork_sources.tar.gz` (sonde + six fichiers amont, empreintes epinglees verifiees) et `eigen3.tar.gz`
(en-tetes Eigen, outil de construction seulement). Memes drapeaux que le constructeur v8 et que
morsehgp3D_v11/bench/points_lidar_prepare.py. Controle obligatoire : masque de 08/000000 = 9db3fe5c...
"""
from __future__ import annotations

import json
import subprocess
import tarfile
from pathlib import Path

import numpy as np

from .common import sha256_file

COMMIT = '3e6903a1d5537a4cc2ace897b0bbb98a92d6014c'
MASK_08_000000 = '9db3fe5c3f5c4c5d955dde9fcaf61f52ac304f4c23aa68076fd934b589caaf8f'
VELODYNE_08_000000 = '92e945f37a6cd4a58acc8aa15b275af2e271ecf69c0a44a311d888524473c451'
ARCHIVES = {'patchwork_sources.tar.gz': '5a847165802ac0afddd31256a4e6cc49833e155fdbfdb7f743ecf27843118565',
            'eigen3.tar.gz': 'f0d2b098d6aec0bd337c5e9e563b58b93b98e3b205a103c46dae5f907f738d5a'}
PINNED = {  # morsehgp3D_v8/bench/build_patchwork_ground.py (UPSTREAM) et morsehgp3D_v11/bench/points_lidar_prepare.py
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


def _run(argv, timeout=900):
    done = subprocess.run([str(a) for a in argv], capture_output=True, text=True, timeout=timeout)
    if done.returncode != 0:
        raise RuntimeError('%s : code %d %s' % (argv[0], done.returncode, done.stderr[-800:]))
    return done.stdout


def _safe_extract(archive: Path, dest: Path) -> None:
    dest.mkdir(parents=True, exist_ok=True)
    with tarfile.open(archive) as tar:
        for member in tar.getmembers():
            target = (dest / member.name).resolve()
            if not str(target).startswith(str(dest.resolve())) or member.issym() or member.islnk():
                raise RuntimeError('membre suspect dans %s : %s' % (archive, member.name))
        tar.extractall(dest)


def build_probe(archives_dir: Path, work: Path, compiler: str = 'g++') -> tuple[Path, dict]:
    for name, digest in ARCHIVES.items():
        if sha256_file(archives_dir / name) != digest:
            raise RuntimeError('archive non epinglee : ' + name)
    eigen, sources = work / 'eigen', work / 'patchwork'
    _safe_extract(archives_dir / 'eigen3.tar.gz', eigen)
    _safe_extract(archives_dir / 'patchwork_sources.tar.gz', sources)
    for relative, digest in PINNED.items():
        if sha256_file(sources / relative) != digest:
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
        _run([compiler, *common, *extra, '-c', source, '-o', obj])
        objects.append(obj)
    binary = work / 'mhgp8_patchwork_ground_probe'
    _run([compiler, '-O3', *objects, '-o', binary])
    version = _run([compiler, '--version']).splitlines()[0]
    return binary, dict(upstream_commit=COMMIT, sources=PINNED, archives=ARCHIVES, compiler=version,
                        flags=common, binary_sha256=sha256_file(binary))


def ground_mask(binary: Path, frame_bin: Path, work: Path) -> tuple[np.ndarray, str, dict]:
    """Masque u8 par retour : 0 inconnu, 1 sol, 2 hors sol (encodage de la sonde v8)."""
    out = work / (frame_bin.stem + '.mask')
    report = json.loads(_run([binary, '--input', frame_bin, '--output', out]).strip().splitlines()[-1])
    mask = np.fromfile(out, np.uint8)
    digest = sha256_file(out)
    out.unlink()
    return mask, digest, report
