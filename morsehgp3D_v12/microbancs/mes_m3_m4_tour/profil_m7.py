#!/usr/bin/env python3
"""MES-M7 sur G4 : profil par composante de la resolution (option --profil-resolution de mhgp12_vidage, README § 5.5).

Joue mhgp12_vidage dans un processus neuf par cas, avec --journal aucun, --chrono-resolution R et --profil-resolution,
garde ses lignes JSON (temps des bras, profil par ordre, controle du vidage) et efface les vidages binaires, qui ne
sont jamais publies (donnees KITTI). Mesure publiee, sans regle d'adoption (PLAN.md, MES-M7).

Usage : profil_m7.py --vidage BINAIRE --donnees DIR --sortie DIR --cas ng00:5,ng01:5,ng02:5,ng00:10 [--fils 48]
                     [--passes 3] [--delai 1500]
  ngXY designe <donnees>/lidar_ngXY.u32le et .ids.u32le ; feuilles de 16 a K <= 5, de 24 au-dela (MESURE.md § 4).
Sorties : <sortie>/<cas>_k<K>.jsonl (lignes du vidage), <sortie>/profil_m7.json (code, secondes, empreinte des lignes,
lignes de profil par ordre). Codes : 0 toutes les prises rendues avec un profil conforme ; 1 une prise sans profil
conforme (code du vidage non nul, controle du vidage en echec ou ligne de profil absente) ; 2 usage ou entree absente.
Bibliotheque standard seule (Python 3.10 nu).
"""
import argparse
import hashlib
import json
import os
import shutil
import subprocess
import sys
import time


def run_case(binary, data, out_dir, frame, k, threads, passes, delay):
    xyz = os.path.join(data, 'lidar_%s.u32le' % frame)
    ids = os.path.join(data, 'lidar_%s.ids.u32le' % frame)
    if not os.path.isfile(xyz) or not os.path.isfile(ids):
        return dict(cas=frame, k=k, code=None, raison='entree absente')
    leaf = 16 if k <= 5 else 24
    work = os.path.join(out_dir, 'travail_%s_k%d' % (frame, k))
    os.makedirs(work, exist_ok=True)
    argv = [binary, xyz, ids, frame, str(k), str(leaf), str(threads), work, '--journal', 'aucun',
            '--chrono-resolution', str(passes), '--profil-resolution']
    start = time.monotonic()
    try:
        done = subprocess.run(argv, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=delay)
        code, out, err = done.returncode, done.stdout, done.stderr
    except subprocess.TimeoutExpired as expired:
        code, out, err = 'expire', expired.stdout or b'', expired.stderr or b''
    seconds = time.monotonic() - start
    shutil.rmtree(work, ignore_errors=True)  # vidages KITTI : jamais publies
    log = os.path.join(out_dir, '%s_k%d.jsonl' % (frame, k))
    with open(log, 'wb') as handle:
        handle.write(out)
    if err:
        with open(os.path.join(out_dir, '%s_k%d.err' % (frame, k)), 'wb') as handle:
            handle.write(err[-65536:])
    profiles, controls = [], []
    for raw in out.decode('utf-8', 'replace').splitlines():
        raw = raw.strip()
        if not raw.startswith('{'):
            continue
        try:
            line = json.loads(raw)
        except ValueError:
            continue
        text = json.dumps(line, sort_keys=True)
        if 'profil' in text:
            profiles.append(line)
        if isinstance(line, dict) and isinstance(line.get('controle_vidage'), dict):
            controls.append(line['controle_vidage'])
    entry = dict(cas=frame, k=k, feuille=leaf, fils=threads, passes=passes, code=code, secondes=round(seconds, 3),
                 lignes_sha256=hashlib.sha256(out).hexdigest(), lignes_profil=len(profiles),
                 controles=len(controls), controles_conformes=sum(1 for c in controls if c.get('conforme') is True))
    # une ligne de profil par ordre 2..K et la ligne de fin ; un controle du vidage par ordre, tous conformes
    entry['conforme'] = (code == 0 and len(profiles) >= k and len(controls) == k - 1 and
                         entry['controles_conformes'] == len(controls))
    return entry


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('--vidage', required=True)
    parser.add_argument('--donnees', required=True)
    parser.add_argument('--sortie', required=True)
    parser.add_argument('--cas', required=True)
    parser.add_argument('--fils', type=int, default=48)
    parser.add_argument('--passes', type=int, default=3)
    parser.add_argument('--delai', type=int, default=1500)
    try:
        args = parser.parse_args(argv[1:])
        cases = []
        for item in args.cas.split(','):
            frame, k = item.split(':')
            cases.append((frame, int(k)))
    except (SystemExit, ValueError):
        return 2
    if not os.path.isfile(args.vidage) or not cases or args.passes < 1 or args.fils < 1:
        return 2
    os.makedirs(args.sortie, exist_ok=True)
    binary_sha = hashlib.sha256(open(args.vidage, 'rb').read()).hexdigest()
    entries = []
    for frame, k in cases:
        entry = run_case(args.vidage, args.donnees, args.sortie, frame, k, args.fils, args.passes, args.delai)
        entries.append(entry)
        print(json.dumps(entry, sort_keys=True), flush=True)
        with open(os.path.join(args.sortie, 'profil_m7.json'), 'w', encoding='utf-8') as out:
            json.dump(dict(mesure='MES-M7', vidage_sha256=binary_sha, prises=entries), out, indent=1, sort_keys=True)
    if any(e.get('raison') == 'entree absente' for e in entries):
        return 2
    return 0 if all(e['conforme'] for e in entries) else 1


if __name__ == '__main__':
    sys.exit(main(sys.argv))
