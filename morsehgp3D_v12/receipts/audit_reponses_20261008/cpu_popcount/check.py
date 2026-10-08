#!/usr/bin/env python3
"""Lecture d'objets existants et de JSON K ; aucun build ni execution native."""
import argparse
import collections
import hashlib
import json
from pathlib import Path
import re
import statistics
import subprocess
import tarfile
import tempfile


def need(ok, why):
    if not ok:
        raise ValueError(why)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, required=True)
    parser.add_argument('--build', type=Path, required=True, help='Build LOCAL epingle, jamais construit ici')
    parser.add_argument('--archive', type=Path, required=True, help='Resultats K, metadonnees uniquement')
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    here = Path(__file__).parent
    capture = json.loads((here/'capture.json').read_text())
    source = {}
    for pin in capture['sources']:
        body = subprocess.check_output(['git', '-C', str(args.repo), 'show', pin['commit']+':'+pin['path']])
        need(sha(body) == pin['sha256'], 'source differente')
        source[pin['path']] = body
    before = {n: sha((args.build/n).read_bytes()) for n in capture['local_artifacts']}
    need(before == capture['local_artifacts'], 'artefact local different')
    obj = args.build/'CMakeFiles/mhgp12.dir/src/catalogue/leaves.cpp.o'
    nm = subprocess.check_output(['nm', '-C', '-u', str(obj)], text=True)
    need(' U __popcountdi2' in nm, 'symbole logiciel absent')
    asm = subprocess.check_output(['objdump', '-drwC', str(obj)], text=True).splitlines()
    func, count, example = '', collections.Counter(), None
    for i, line in enumerate(asm):
        match = re.match(r'[0-9a-f]+ <(.*)>:', line)
        if match:
            func = match.group(1)
        if 'R_X86_64_PLT32' in line and '__popcountdi2' in line:
            count[func] += 1
            if 'phase<2, 32u' in func and 'Narrow' in func and 'WriteSink<32u>' in func and \
                    any('0x180' in part and 'or ' in part for part in asm[max(0,i-3):i]):
                example = dict(function=func, instructions=[re.sub(r'<.*>', '<cible>', part).strip()
                                                          for part in asm[max(0,i-2):i+10]])
    need(example is not None, 'temoin G3 absent')
    flags = (args.build/'CMakeFiles/mhgp12.dir/flags.make').read_text()
    effective_flags = next(line.split('=', 1)[1].strip() for line in flags.splitlines() if line.startswith('CXX_FLAGS ='))
    need('-march' not in effective_flags and '-mpopcnt' not in effective_flags, 'flags locaux modifies')
    cache = (args.build/'CMakeCache.txt').read_text().splitlines()
    settings = {key: next(line.split('=', 1)[1] for line in cache if line.startswith(key+':'))
                for key in ('CMAKE_BUILD_TYPE', 'MHGP12_COORD_BITS', 'MHGP12_ENABLE_CUDA', 'MHGP12_MARCH')}
    need(settings == dict(CMAKE_BUILD_TYPE='Release', MHGP12_COORD_BITS='21', MHGP12_ENABLE_CUDA='OFF', MHGP12_MARCH=''),
         'configuration locale differente')
    path = 'morsehgp3D_v12/src/catalogue/simt.hpp'
    with tempfile.TemporaryDirectory(prefix='audit-popcount-text-') as temp:
        dest = Path(temp)/path
        dest.parent.mkdir(parents=True)
        dest.write_bytes(source[path])
        subprocess.run(['git', 'apply', '--check', str(here/'proposition.patch')], cwd=temp, check=True)
        subprocess.run(['git', 'apply', str(here/'proposition.patch')], cwd=temp, check=True)
        need(sha(dest.read_bytes()) == capture['proposal_sha256'], 'proposition differente')
    archive_before = args.archive.read_bytes()
    need(sha(archive_before) == capture['archive_sha256'], 'archive K differente')
    med = statistics.median
    frames = {}
    with tarfile.open(args.archive, 'r:gz') as tar:
        for case in ('ng00', 'ng01', 'ng02'):
            rows = []
            for rep in range(3):
                name = f'results/cmd/000_mes_full/files/full/brut/cpu_{case}_r{rep}.jsonl'
                process = [json.loads(line) for line in tar.extractfile(name).read().splitlines()]
                full = [row for row in process if row['phase'] == 'full']
                need(len(full) == 5 and all(row['voie'] == 'cpu' and row['coord_bits'] == 21 and row['threads'] == 48
                                          and row['kmax'] == 5 for row in full), 'regime K different')
                rows.extend(full[1:])
            keys = ('parcours', 'feuilles', 'emission', 'fin_etage')
            residual = [row['etapes_ns']['C'] - sum(row['c_ns'][key] for key in keys) for row in rows]
            need(min(residual) >= 0, 'somme CPU hors mur')
            frames[case] = dict(warm_passes=len(rows), catalogue_ns=med(row['etapes_ns']['C'] for row in rows),
                                stages_ns={key: med(row['c_ns'][key] for row in rows) for key in keys},
                                median_shares_percent={key: med(100*row['c_ns'][key]/row['etapes_ns']['C'] for row in rows)
                                                      for key in keys},
                                unassigned_ns=med(residual),
                                if_finish_free_ns=med(row['etapes_ns']['C']-row['c_ns']['fin_etage'] for row in rows))
    result = dict(local_only=True, native_executed=False, compilation_executed=False, patch_application=True,
                  static_popcount_relocations=sum(count.values()), functions_with_relocations=len(count),
                  count_body_relocations=sum(v for k,v in count.items() if '::count_body(' in k),
                  flags=effective_flags, settings=settings, g3_example=example, session_k_cpu=frames)
    need({n: sha((args.build/n).read_bytes()) for n in capture['local_artifacts']} == before,
         'artefact local modifie pendant lecture')
    need(args.archive.read_bytes() == archive_before, 'archive modifiee')
    if args.check:
        need(result == capture['result'], 'resultat different')
    print(json.dumps(result, sort_keys=True, indent=2))


if __name__ == '__main__':
    main()
