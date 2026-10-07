#!/usr/bin/env python3
"""Porte native bornee : vrai Driver courant contre son en-tete avant lot 2.

Deux compilations mono de la meme petite porte, aucun nuage ni GPU.
Tout est construit dans un repertoire temporaire detruit a la sortie.
"""
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[4]
M5 = ROOT / 'morsehgp3D_v12/microbancs/mes_m5_parcours'
M2 = ROOT / 'morsehgp3D_v12/microbancs/mes_m2_feuille'
PIN = 'f601b36ac'


def need(condition, message):
    if not condition:
        raise RuntimeError(message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    compiler = shutil.which('g++')
    need(compiler is not None, 'g++ absent')
    sources = [M5 / 'host/driver_selftest.cpp'] + sorted((M5 / 'include').rglob('*.hpp'))
    sources += sorted((M2 / 'include').rglob('*.hpp'))
    pins = {str(p.relative_to(ROOT)): sha(p.read_bytes()) for p in sources}
    for path, expected in pins.items():
        data = subprocess.check_output(['git', 'show', PIN + ':' + path], cwd=ROOT)
        need(sha(data) == expected, 'source hors pin : ' + path)
    relative = 'morsehgp3D_v12/microbancs/mes_m5_parcours/include/mhgp12/traversal/driver.hpp'
    old = subprocess.check_output(['git', 'show', '2b2113264^:' + relative], cwd=ROOT)
    results = []
    with tempfile.TemporaryDirectory(prefix='audit-m5-driver-') as temporary:
        folder = Path(temporary)
        header = folder / 'old_include/mhgp12/traversal/driver.hpp'
        header.parent.mkdir(parents=True)
        header.write_bytes(old)
        for version in ('after', 'before'):
            binary = folder / version
            command = [compiler, '-std=c++20', '-O1', '-Wall', '-Wextra', '-Wpedantic', '-Werror']
            if version == 'before':
                command += ['-I', str(folder / 'old_include')]
            command += ['-I', str(M5 / 'include'), '-I', str(M2 / 'include'),
                        str(M5 / 'host/driver_selftest.cpp'), '-o', str(binary)]
            compiled = subprocess.run(command, capture_output=True, text=True, timeout=120)
            need(compiled.returncode == 0, compiled.stderr)
            executed = subprocess.run([str(binary)], capture_output=True, text=True, timeout=15)
            rows = [json.loads(s) for s in executed.stdout.splitlines()]
            need(rows[-1]['cas'] == 29 and rows[-1]['ecarts'] == (0 if version == 'after' else 21), rows[-1])
            need(executed.returncode == (0 if version == 'after' else 1), 'code inattendu')
            results.append({'version': version, 'code': executed.returncode, 'cas': rows[-1]['cas'],
                            'ecarts': rows[-1]['ecarts'], 'cas_en_ecart': [r for r in rows[:-1] if not r['conforme']]})
    need(pins == {str(p.relative_to(ROOT)): sha(p.read_bytes()) for p in sources}, 'sources modifiees')
    print(json.dumps({'pin': PIN, 'compiler': subprocess.check_output([compiler, '--version'], text=True).splitlines()[0],
                      'flags': '-std=c++20 -O1 -Wall -Wextra -Wpedantic -Werror',
                      'sources_sha256': pins, 'ancien_driver_sha256': sha(old), 'resultats': results},
                     ensure_ascii=False, sort_keys=True, indent=2))


if __name__ == '__main__':
    main()
