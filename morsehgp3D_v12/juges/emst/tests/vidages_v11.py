#!/usr/bin/env python3
"""Porte LiDAR du juge JUG-EMST : l'ordre un des vidages MHGP11FUL1 de la v11 gelee (ac081a06f), dont les empreintes
sont epinglees au paragraphe 4 de docs/MESURE.md, est identique a l'arbre de fusion de l'EMST exact.

    MHGP12_DATA_DIR=<dossier des .u32le et .ids.u32le> MHGP12_EMST_VIDAGES=<dossier des vidages> \\
        python3 vidages_v11.py --juge <mhgp12_jug_emst>

Vidages lus : <cas>.k5.ful1 (exiges, les six cas) et <cas>.k10.ful1 (juges s'ils sont la). Commande de la v11 :
mhgp11_full_bench <cas>.u32le <cas>.ids.u32le <vidage> <K> <feuille> 256 0 4294967295 8589934592 <fils> 802811
(feuilles 16 a K5, 24 a K10). Aucune donnee ni aucun vidage dans le depot : comptes et empreintes seulement.

Sur chaque nuage, en plus, des transformations explicites (fichiers temporaires effaces a la fin) :
  - profils 24 et 32 : homothetie de rapport 2^j (le plus grand qui tienne dans le profil) ; memes enfants, niveaux
    multiplies par 4^j, voie u128 au profil 32 ;
  - translation de 2^31 : voie u128, sha256_fusions inchangee ;
  - permutation (ordre d'entree renverse, identifiants compris) : memes empreintes, vidage toujours identique.

Codes : 0 conforme ; 1 ecart (juge, comptes, empreinte d'ordre un ou transformation) ; 3 vidage absent ou
d'empreinte differente de l'epingle ; 77 sautee (variables d'environnement absentes). Python 3.10 nu, aucun assert.
"""
import argparse
import hashlib
import os
import sys
import tempfile
from array import array
from fractions import Fraction

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import commun as C  # noqa: E402

# Empreintes des vidages (MESURE.md, paragraphe 4).
EPINGLES = {
    ('lidar_ng00', 5): '3a2bfb4f9f48b4b0cc5b0318d9fcf4887906638e034b2c97dda1918e3170a6fe',
    ('lidar_ng01', 5): '5212a2ced81bf69bd2abfb935a3b14a35158df25339285a0d25a9d5d5c09e091',
    ('lidar_ng02', 5): '78feb765e21c8e4582762a25bd0dd36a455747d3f0df80523e19f7ba4be0e207',
    ('lidar_ng00', 10): '61a4245b91d9a4fdad012f0a2e26a63c3e48db1d46a180db756f2c4e4aa77295',
    ('lidar_ng01', 10): '838a447e0b92e13e42f7f69a84fd536d5d46d26463d3f4cc7c688475878fe0de',
    ('lidar_ng02', 10): '81f89995eaccb497cfe92abb81213bebd0d4ce5076210570d86aa9456cf7ff2e',
    ('uniform_u18_n8000', 5): 'f87dbb19dd928311fcb65d5a98d30b4fb50eddf7de1d26708bbc278f46dcc7cf',
    ('uniform_u18_n16000', 5): '141bc7d6523154cff1dd22b288e4155259e3d97a82877205887642bad8286889',
    ('uniform_u18_n32000', 5): 'a7563907a5c9f1b273776b811d8a559eb80713eb161460edd948664f5433811b',
}
# Ordre un attendu (premier passage du juge, 7 octobre 2026) : sites, fusions, multifusions (au moins trois enfants),
# niveaux distincts, sha256_arbre. Il ne depend pas de K.
ORDRE_UN = {
    'uniform_u18_n8000': (8000, 7999, 0, 7999,
                          'ae5aaf27cc31f675537240b63f8c2db8dfdb45c2b27a3c7bd4c2b9238c88e39b'),
    'uniform_u18_n16000': (16000, 15999, 0, 15996,
                           '65ded228df429c71799e3c65721c4ab8925d3243566e02ad520ce586d11e6d28'),
    'uniform_u18_n32000': (32000, 31998, 1, 31991,
                           'f46e4cb477d11088531a203479cd772d94ff479c5ec0549ceffd9c6e62d8db31'),
    'lidar_ng00': (39885, 39796, 87, 22511, 'fbe9969e6b5d6ae9716cbeef526c44a59b8d95b2ee8df9325c85de184e92dd0d'),
    'lidar_ng01': (35551, 35461, 87, 23344, '674989e0ea6dd2f318abc4c8c49b5928f66773030cda65e9a1eb82c16db35db1'),
    'lidar_ng02': (45845, 45563, 272, 21528, '0e0ed701e5e88ece5493468702fa798525e01fda37b62903310f7aa11623ac4d'),
}


def sha256_fichier(chemin):
    h = hashlib.sha256()
    with open(chemin, 'rb') as entree:
        for bloc in iter(lambda: entree.read(1 << 20), b''):
            h.update(bloc)
    return h.hexdigest()


def lire_mots(chemin):
    mots = array('I')
    with open(chemin, 'rb') as entree:
        mots.frombytes(entree.read())
    if sys.byteorder != 'little':
        mots.byteswap()
    return mots


def ecrire_mots(chemin, mots):
    copie = array('I', mots)
    if sys.byteorder != 'little':
        copie.byteswap()
    with open(chemin, 'wb') as sortie:
        copie.tofile(sortie)


def transformations(juge, base, vidage, reference, travail):
    """Ecarts (liste de textes) des transformations explicites d'un nuage."""
    ecarts = []
    mots, ids = lire_mots(base + '.u32le'), lire_mots(base + '.ids.u32le')
    arbre0 = os.path.join(travail, 'base.arbre')
    code, sortie0, erreur = C.lancer(juge, [base + '.u32le', '--arbre', arbre0])
    if code != C.OK or sortie0 is None or sortie0['sha256_arbre'] != reference:
        return ['nuage de base : code %s %s' % (code, erreur.strip())]
    naissances0, fusions0 = C.lire_arbre(arbre0)
    largeur = max(mots).bit_length()
    nuage, arbre = os.path.join(travail, 't.u32le'), os.path.join(travail, 't.arbre')
    for bits in (24, 32):
        j = bits - largeur
        ecrire_mots(nuage, [m << j for m in mots])
        code, sortie, erreur = C.lancer(juge, [nuage, '--bits', str(bits), '--arbre', arbre])
        if code != C.OK or sortie is None:
            ecarts.append('profil %d : code %s %s' % (bits, code, erreur.strip()))
            continue
        naissances, fusions = C.lire_arbre(arbre)
        voie = 'u128' if bits == 32 else 'u64'
        if sortie['arithmetique'] != voie or \
                naissances != [tuple(c << j for c in p) for p in naissances0] or \
                fusions != [(niveau * Fraction(1 << (2 * j)), enfants) for niveau, enfants in fusions0]:
            ecarts.append('profil %d (homothetie 2^%d) : arbre non homothetique, voie %s' % (bits, j,
                                                                                              sortie['arithmetique']))
    ecrire_mots(nuage, [m + (1 << 31) for m in mots])
    code, sortie, erreur = C.lancer(juge, [nuage])
    if code != C.OK or sortie is None or sortie['arithmetique'] != 'u128' or \
            sortie['sha256_fusions'] != sortie0['sha256_fusions']:
        ecarts.append('translation de 2^31 : code %s %s' % (code, erreur.strip()))
    n = len(ids)
    renverse = array('I')
    for i in range(n - 1, -1, -1):
        renverse.extend(mots[3 * i:3 * i + 3])
    ecrire_mots(nuage, renverse)
    ecrire_mots(nuage + '.ids', ids[::-1])
    code, sortie, erreur = C.lancer(juge, [nuage, '--ids', nuage + '.ids', '--vidage', vidage])
    if code != C.OK or sortie is None or sortie['sha256_arbre'] != sortie0['sha256_arbre'] or \
            sortie['sha256_emst'] != sortie0['sha256_emst']:
        ecarts.append('permutation renversee : code %s %s' % (code, erreur.strip()))
    return ecarts


def main(argv):
    parser = argparse.ArgumentParser(description='JUG-EMST sur les vidages de la v11 gelee')
    parser.add_argument('--juge', required=True)
    try:
        args = parser.parse_args(argv[1:])
    except SystemExit:
        return C.USAGE
    donnees, vidages = os.environ.get('MHGP12_DATA_DIR'), os.environ.get('MHGP12_EMST_VIDAGES')
    if not donnees or not vidages:
        print('sautee : MHGP12_DATA_DIR et MHGP12_EMST_VIDAGES requis')
        return C.SAUTE
    juge = os.path.abspath(args.juge)
    ecarts, juges, transformes = 0, 0, 0
    for (cas, k), epingle in sorted(EPINGLES.items(), key=lambda e: (e[0][1], e[0][0])):
        vidage = os.path.join(vidages, '%s.k%d.ful1' % (cas, k))
        if not os.path.isfile(vidage):
            if k == 5:
                print('%s K%d : vidage absent (%s)' % (cas, k, vidage))
                return C.INVARIANT
            continue
        empreinte = sha256_fichier(vidage)
        if empreinte != epingle:
            print('%s K%d : empreinte du vidage %s, epingle %s' % (cas, k, empreinte, epingle))
            return C.INVARIANT
        base = os.path.join(donnees, cas)
        code, sortie, erreur = C.lancer(juge, [base + '.u32le', '--ids', base + '.ids.u32le', '--vidage', vidage])
        juges += 1
        attendu = ORDRE_UN[cas]
        obtenu = None if sortie is None else (sortie['sites'], sortie['fusions'], sortie['multifusions'],
                                              sortie['niveaux_distincts'], sortie['sha256_arbre'])
        if code != C.OK or obtenu != attendu:
            ecarts += 1
            print('ECART %s K%d : code %s, ordre un %r, attendu %r (%s)' % (cas, k, code, obtenu, attendu,
                                                                          erreur.strip()))
            continue
        print('%s K%d : identique, sites=%d fusions=%d multifusions=%d niveaux=%d sha256_arbre=%s emst_ms=%.1f '
              'total_ms=%.1f' % ((cas, k) + obtenu + (sortie['temps_ms']['emst'], sortie['temps_ms']['total'])))
        if k == 5:
            with tempfile.TemporaryDirectory(prefix='jug_emst_') as travail:
                fautes = transformations(juge, base, vidage, attendu[4], travail)
            for faute in fautes:
                print('ECART %s : %s' % (cas, faute))
            ecarts += len(fautes)
            transformes += 1
    print('vidages_v11 juges=%d transformes=%d ecarts=%d' % (juges, transformes, ecarts))
    if ecarts:
        return C.DESACCORD
    if juges < 6 or transformes != 6:
        print('plancher : au moins six vidages juges et six nuages transformes')
        return C.INVARIANT
    return C.OK


if __name__ == '__main__':
    sys.exit(main(sys.argv))
