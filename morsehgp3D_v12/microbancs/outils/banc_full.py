#!/usr/bin/env python3
"""Outils communs des pilotes G4 de la sonde FULL residente (bench/full_probe.cpp) : lancement borne d'un processus
dans son propre groupe, releve d'environnement (cmake, nvcc, GPU vide ou non, hote), construction Release de la sonde,
extrait du CMakeCache, empreinte SHA-256 d'un fichier, deballage controle d'une archive tar plate.
Python 3.10 nu, aucun assert. Bibliotheque seulement (aucun point d'entree).
"""
import hashlib
import json
import os
import platform
import signal
import subprocess
import tarfile
import time

CMAKE_KEYS = ('CMAKE_BUILD_TYPE', 'CMAKE_CXX_COMPILER', 'CMAKE_CUDA_COMPILER', 'CMAKE_CUDA_ARCHITECTURES',
              'CMAKE_CXX_FLAGS', 'CMAKE_CUDA_FLAGS', 'MHGP12_')


def sha256_file(path):
    digest = hashlib.sha256()
    with open(path, 'rb') as handle:
        for block in iter(lambda: handle.read(1 << 22), b''):
            digest.update(block)
    return digest.hexdigest()


def run(argv, delay, raw_out=None, raw_err=None):
    """Lance argv dans son propre groupe ; rend (code, sortie, erreur, secondes) ; code 'expire' apres le delai (tout
    le groupe est tue)."""
    t0 = time.monotonic()
    proc = subprocess.Popen([str(a) for a in argv], stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                            start_new_session=True)
    try:
        out, err = proc.communicate(timeout=delay)
        code = proc.returncode
    except subprocess.TimeoutExpired:
        try:
            os.killpg(proc.pid, signal.SIGKILL)
        except ProcessLookupError:
            pass
        out, err = proc.communicate()
        code = 'expire'
    for path, data in ((raw_out, out), (raw_err, err)):
        if path is not None:
            with open(path, 'wb') as handle:
                handle.write(data)
    return code, out.decode('utf-8', 'replace'), err.decode('utf-8', 'replace'), time.monotonic() - t0


def environment(nvcc, with_gpu):
    """Releve d'environnement ; gpu_apps vaut '' quand le GPU est connu vide, None si nvidia-smi manque."""
    env = {}
    commands = [('cmake', ['cmake', '--version'])]
    if with_gpu:
        commands += [('nvcc', [nvcc, '--version']),
                     ('gpu', ['nvidia-smi', '--query-gpu=name,driver_version,persistence_mode,memory.total',
                              '--format=csv,noheader']),
                     ('gpu_apps', ['nvidia-smi', '--query-compute-apps=pid', '--format=csv,noheader'])]
    for name, argv in commands:
        try:
            code, out, _err, _s = run(argv, 60)
            env[name] = out.strip() if code == 0 else None
        except OSError:
            env[name] = None
    env['noyau'] = platform.release()
    env['fils_hote'] = os.cpu_count()
    try:
        with open('/proc/meminfo', encoding='ascii') as handle:
            env['memoire_hote'] = next((line.split(':', 1)[1].strip() for line in handle
                                        if line.startswith('MemTotal:')), None)
    except OSError:
        env['memoire_hote'] = None
    return env


def environment_ok(env):
    """Environnement complet et GPU connu vide (chaine vide, jamais absente)."""
    return env.get('gpu_apps') == '' and all(env.get(k) is not None for k in ('cmake', 'nvcc', 'gpu'))


def build(src, folder, nvcc, jobs, log_path, bits=21):
    """Construction Release avec CUDA de mhgp12_full_probe ; rend le chemin de la sonde ou None."""
    os.makedirs(folder, exist_ok=True)
    configure = ['cmake', '-S', os.path.join(src, 'morsehgp3D_v12'), '-B', folder, '-DCMAKE_BUILD_TYPE=Release',
                 '-DMHGP12_COORD_BITS=%d' % bits, '-DMHGP12_ENABLE_CUDA=ON', '-DCMAKE_CUDA_COMPILER=' + nvcc]
    for argv in (configure, ['cmake', '--build', folder, '-j', str(jobs), '--target', 'mhgp12_full_probe']):
        code, out, err, _s = run(argv, 3600)
        with open(log_path, 'a', encoding='utf-8') as log:
            log.write('$ %s\n%s%s' % (' '.join(argv), out, err))
        if code != 0:
            return None
    probe = os.path.join(folder, 'mhgp12_full_probe')
    return probe if os.path.isfile(probe) else None


def cmake_extract(folder):
    """Lignes du CMakeCache qui fixent compilateurs, options et profil ; None si le cache manque."""
    try:
        with open(os.path.join(folder, 'CMakeCache.txt'), encoding='utf-8', errors='replace') as handle:
            return [line.rstrip('\n') for line in handle if line.startswith(CMAKE_KEYS)]
    except OSError:
        return None


def unpack(archive, folder):
    """Archive tar plate (fichiers simples a nom simple, sans lien) ; rend le manifeste du paquet (dict), ou None."""
    os.makedirs(folder, exist_ok=True)
    try:
        with tarfile.open(archive) as tar:
            members = tar.getmembers()
            for m in members:
                if not m.isfile() or '/' in m.name or m.name in ('', '.', '..') or m.name.startswith('.'):
                    return None
            for m in members:
                with open(os.path.join(folder, m.name), 'wb') as out:
                    out.write(tar.extractfile(m).read())
        with open(os.path.join(folder, 'bundle_manifest.json'), encoding='utf-8') as handle:
            manifest = json.load(handle)
    except (OSError, ValueError, KeyError, TypeError, tarfile.TarError):
        return None
    return manifest if isinstance(manifest, dict) else None
