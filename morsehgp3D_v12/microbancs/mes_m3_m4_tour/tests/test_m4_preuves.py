#!/usr/bin/env python3
"""Porte native des preuves de MES-M4 (constat CST-0214) sur le carre de l'auditeur, sans donnee reelle.

Le generateur du carre K1..4 est celui du recu audit_socle_microbancs_20261007/tour/check_dumps.py (quatre sites
A=(0,0,0), B=(2,0,0), D=(0,2,0), C=(2,2,0) en identifiants de Morton 0..3 ; Cat_4 : quatre boules diametrales des
cotes au niveau 1 et le cercle au niveau 2 ; a K4, l'image basse de la naissance du cercle est une naissance). Le
binaire reel mhgp12_mes_m4 est joue sur des vidages ecrits ici. Attendus (avant correction : valide 0, fausse 1,
absente 0 avec zero naissance jugee) :
  - verticales correctes : code 0, six naissances jugees par LEM-T6 (4 + 1 + 1), zero ecart ;
  - une image fausse : code 1 ;
  - FLOWER absente aux ordres 2 a 4, ou au seul ordre 3 : refus explicite (ligne « refus », code 3), jamais un succes ;
  - entrees hors domaine (genre ou ordre d'en-tete, cle de naissance hors du catalogue, boule de cellule hors du
    catalogue, decalages decroissants) : refus, code 3 ;
  - le mutant sans contraction, s'il est fourni, sur le carre valide : code 1 (ecart d'identite), jamais 0.

Usage : python3 -S -O tests/test_m4_preuves.py --binaire <mhgp12_mes_m4> [--mutant <..._sans_contraction>]
Bibliotheque standard ; aucune garde par assert. Codes : 0 conforme, 1 ecart, 2 usage.
"""
import hashlib
import json
import subprocess
import sys
import tempfile
from pathlib import Path

sys.dont_write_bytecode = True  # aucune trace dans l'arbre des sources (hachees par les pilotes)
sys.path.insert(0, str(Path(__file__).resolve().parent))
from carre import make  # noqa: E402  (generateur partage du carre)


CAS = [
    # (nom, mode FLOWER, alteration, code attendu, naissances jugees attendues ou None)
    ('verticales_correctes', 'valid', None, 0, 6),
    ('image_fausse', 'wrong', None, 1, None),
    ('flower_absente_ordres_2_a_4', 'absent', None, 3, None),
    ('flower_absente_ordre_3', 'absent_ordre_3', None, 3, None),
    ('ordre_en_tete_faux', 'valid', 'ordre_en_tete_faux', 3, None),
    ('genre_de_foret_faux', 'valid', 'genre_de_foret_faux', 3, None),
    ('cle_hors_catalogue', 'valid', 'cle_hors_catalogue', 3, None),
    ('boule_de_cellule_hors_catalogue', 'valid', 'boule_de_cellule_hors_catalogue', 3, None),
    ('decalages_decroissants', 'valid', 'decalages_decroissants', 3, None),
]


def jouer(binaire, dossier):
    proc = subprocess.run([str(binaire), str(dossier), '--repetitions', '1', '--fils-contraction', '2'],
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, timeout=60)
    lignes = []
    for ligne in proc.stdout.splitlines():
        if ligne.startswith('{'):
            lignes.append(json.loads(ligne))
    return proc.returncode, lignes


def main(argv):
    binaire = mutant = None
    i = 1
    while i < len(argv):
        if argv[i] == '--binaire' and i + 1 < len(argv):
            binaire = Path(argv[i + 1])
        elif argv[i] == '--mutant' and i + 1 < len(argv):
            mutant = Path(argv[i + 1])
        else:
            print(__doc__)
            return 2
        i += 2
    if binaire is None or not binaire.is_file():
        print(__doc__)
        return 2
    ecarts, resultats = [], []
    with tempfile.TemporaryDirectory(prefix='m4-preuves-') as tmp:
        tmp = Path(tmp)
        for nom, mode, alteration, attendu, jugees in CAS:
            dossier = tmp / nom
            dossier.mkdir()
            make(dossier, mode, alteration)
            code, lignes = jouer(binaire, dossier)
            refus = [l for l in lignes if l.get('phase') == 'refus']
            fin = [l for l in lignes if l.get('phase') == 'fin']
            t6 = sum(l.get('lem_t6', {}).get('naissances_jugees', 0) for l in lignes if l.get('phase') == 'ordre')
            ligne = {'cas': nom, 'code': code, 'attendu': attendu, 'naissances_jugees_lem_t6': t6,
                     'refus': refus[0]['raison'] if refus else None,
                     'vidages_sha256': {p.name: hashlib.sha256(p.read_bytes()).hexdigest()[:16]
                                        for p in sorted(dossier.iterdir())}}
            if code != attendu:
                ecarts.append('%s : code %d, attendu %d' % (nom, code, attendu))
            if not fin or fin[-1].get('code') != code:
                ecarts.append('%s : ligne de fin absente ou discordante' % nom)
            if attendu == 3 and not refus:
                ecarts.append('%s : refus sans ligne explicite' % nom)
            if jugees is not None and t6 != jugees:
                ecarts.append('%s : %d naissances jugees, %d attendues' % (nom, t6, jugees))
            resultats.append(ligne)
        if mutant is not None:
            dossier = tmp / 'mutant'
            dossier.mkdir()
            make(dossier, 'valid')
            code, lignes = jouer(mutant, dossier)
            ecart = any(l.get('phase') == 'ordre' and not l['identite']['identiques'] for l in lignes)
            resultats.append({'cas': 'mutant_sans_contraction_carre', 'code': code, 'ecart_d_identite': ecart})
            if code != 1 or not ecart:
                ecarts.append('mutant sans contraction : code %d, ecart %s (attendu : code 1 par ecart)' % (code, ecart))
    for r in resultats:
        print(json.dumps(r, ensure_ascii=False, sort_keys=True))
    print(json.dumps({'porte': 'm4_preuves', 'binaire_sha256': hashlib.sha256(binaire.read_bytes()).hexdigest(),
                      'cas': len(resultats), 'ecarts': ecarts, 'optimise': sys.flags.optimize}, ensure_ascii=False))
    return 0 if not ecarts else 1


if __name__ == '__main__':
    sys.exit(main(sys.argv))
