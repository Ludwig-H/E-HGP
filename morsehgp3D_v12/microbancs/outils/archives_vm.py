#!/usr/bin/env python3
"""Recupere, pendant une session G4 gardee, l'archive de resultats d'une session PRECEDENTE restee sur le disque
persistant de la VM, quand son rapatriement a echoue (par exemple faute de place sur le codespace : session
v12.20261008.mesb1, rapatriement refuse avec 878 Mo libres).

Le controleur garde sur la VM, pour chaque session passee, results.tar.gz, SHA256SUMS, worker.exit et worker.log
($HOME/ehgp-v12/ehgp-v12.XXXXXXXXXX/) ; il n'en elague que les parties regenerables (gcp-migration/v12_session.py).
Cet outil copie ces fichiers dans --sortie, verifie l'empreinte SHA-256 de results.tar.gz contre SHA256SUMS et ecrit
une ligne JSON de compte rendu. Il ne lit rien d'autre et n'ecrit que dans --sortie.

Usage : archives_vm.py --repertoire ehgp-v12.XXXXXXXXXX --sortie DOSSIER [--racine DOSSIER (defaut ~/ehgp-v12)]
Codes : 0 copie verifiee ; 1 empreinte differente ; 2 usage, repertoire ou fichier absent.
Python 3.10 nu, aucun assert.
"""
import argparse
import hashlib
import json
import os
import re
import shutil
import sys

NAME = re.compile(r'ehgp-v12\.[A-Za-z0-9]{10}')
FILES = ('results.tar.gz', 'SHA256SUMS', 'worker.exit', 'worker.log')


def sha256_file(path):
    digest = hashlib.sha256()
    with open(path, 'rb') as handle:
        for block in iter(lambda: handle.read(1 << 20), b''):
            digest.update(block)
    return digest.hexdigest()


def main(argv):
    parser = argparse.ArgumentParser()
    parser.add_argument('--repertoire', required=True)
    parser.add_argument('--sortie', required=True)
    parser.add_argument('--racine', default=os.path.expanduser('~/ehgp-v12'))
    args = parser.parse_args(argv[1:])
    folder = os.path.join(args.racine, args.repertoire)
    if not NAME.fullmatch(args.repertoire) or not os.path.isdir(folder) or os.path.islink(folder):
        print(json.dumps(dict(statut='refus', raison='repertoire absent ou mal nomme', repertoire=args.repertoire)))
        return 2
    archive = os.path.join(folder, 'results.tar.gz')
    sums = os.path.join(folder, 'SHA256SUMS')
    if not os.path.isfile(archive) or os.path.islink(archive) or not os.path.isfile(sums) or os.path.islink(sums):
        print(json.dumps(dict(statut='refus', raison='results.tar.gz ou SHA256SUMS absent', repertoire=args.repertoire)))
        return 2
    os.makedirs(args.sortie, exist_ok=True)
    copied = {}
    for name in FILES:
        source = os.path.join(folder, name)
        if os.path.isfile(source) and not os.path.islink(source):
            shutil.copyfile(source, os.path.join(args.sortie, name))
            copied[name] = dict(octets=os.path.getsize(source), sha256=sha256_file(source))
    with open(sums, encoding='ascii', errors='replace') as handle:
        declared = {line.split()[1].lstrip('*'): line.split()[0] for line in handle if len(line.split()) == 2}
    expected = declared.get('results.tar.gz')
    ok = expected is not None and expected == copied['results.tar.gz']['sha256']
    print(json.dumps(dict(statut='ok' if ok else 'ecart', repertoire=args.repertoire, fichiers=copied,
                          sha256_declare=expected), sort_keys=True))
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main(sys.argv))
