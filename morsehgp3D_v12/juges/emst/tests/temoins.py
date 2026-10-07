#!/usr/bin/env python3
"""Temoins graves du juge JUG-EMST : attendus exacts ecrits ici, independants du juge et de l'oracle borne.

    python3 temoins.py --juge <mhgp12_jug_emst> --travail <dossier> [--mutant binaire|departage]

Chaque temoin fixe les naissances (ordre lexicographique des positions), les fusions (niveau exact d2 / 4, enfants)
et l'arbre couvrant minimal sous l'ordre strict (d2, plus petit site, plus grand site), puis les empreintes
recalculees ici. S'y ajoutent les refus (usage : code 2 ; entree invalide, dont les doublons de la decision D8 :
code 3) et des vidages MHGP11FUL1 fabriques ici a l'ordre un (identite : 0 ; ecart : 1 ; vidage invalide : 3).

Codes : 0 conforme ; 1 desaccord ; 3 plancher viole. Avec --mutant : 4 si au moins un controle echoue (mutant tue),
0 sinon (mutant survivant). Python 3.10 nu, aucun assert (meme code sous python3 -O).
"""
import argparse
import os
import sys
from fractions import Fraction

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import commun as C  # noqa: E402

M = (1 << 32) - 1
CONTROLES = 139  # nombre exact de controles d'un passage conforme (plancher contre le vert par vacuite)
B21 = (1 << 31) - 1
B31 = 1 << 31

# (nom, points en ordre d'entree, naissances attendues, fusions attendues, arbre couvrant attendu, arithmetique)
TEMOINS = (
    ('WIT-TRI-EQ triangle equilateral : une fusion ternaire',
     [(0, 0, 0), (1, 1, 0), (1, 0, 1)],
     [(0, 0, 0), (1, 0, 1), (1, 1, 0)],
     [(Fraction(1, 2), (0, 1, 2))],
     [(2, 0, 1), (2, 0, 2)], 'u64'),
    ('alignes4 : quatre points alignes a pas egal, une fusion quaternaire',
     [(3, 0, 0), (0, 0, 0), (2, 0, 0), (1, 0, 0)],
     [(0, 0, 0), (1, 0, 0), (2, 0, 0), (3, 0, 0)],
     [(Fraction(1, 4), (0, 1, 2, 3))],
     [(1, 0, 1), (1, 1, 2), (1, 2, 3)], 'u64'),
    ('WIT-SIX deux triangles de la these (section 6.1) : deux fusions ternaires simultanees',
     [(268, 3000, 0), (268, 1000, 0), (2000, 2000, 0), (4000, 2000, 0), (5732, 3000, 0), (5732, 1000, 0)],
     [(268, 1000, 0), (268, 3000, 0), (2000, 2000, 0), (4000, 2000, 0), (5732, 1000, 0), (5732, 3000, 0)],
     [(Fraction(999956), (0, 1, 2)), (Fraction(999956), (3, 4, 5)), (Fraction(1000000), (6, 7))],
     [(3999824, 0, 2), (3999824, 1, 2), (3999824, 3, 4), (3999824, 3, 5), (4000000, 2, 3)], 'u64'),
    ('u32_coins : coins de [0, 2^32)^3, d2 = 2 (2^32 - 1)^2 au-dela de u64',
     [(M, M, M), (0, 0, 0), (M, 0, 0), (0, M, 0), (0, 0, M)],
     [(0, 0, 0), (0, 0, M), (0, M, 0), (M, 0, 0), (M, M, M)],
     [(Fraction(M * M, 4), (0, 1, 2, 3)), (Fraction(M * M, 2), (4, 5))],
     [(M * M, 0, 1), (M * M, 0, 2), (M * M, 0, 3), (2 * M * M, 1, 4)], 'u128'),
    ('u32_diagonale : coins opposes de [0, 2^32)^3, d2 = 3 (2^32 - 1)^2',
     [(M, M, M), (0, 0, 0)],
     [(0, 0, 0), (M, M, M)],
     [(Fraction(3 * M * M, 4), (0, 1))],
     [(3 * M * M, 0, 1)], 'u128'),
    ('seuil_u64 : coordonnees < 2^31, voie u64',
     [(B21, B21, B21), (1, 0, 0), (0, 0, 0)],
     [(0, 0, 0), (1, 0, 0), (B21, B21, B21)],
     [(Fraction(1, 4), (0, 1)), (Fraction((B21 - 1) ** 2 + 2 * B21 ** 2, 4), (2, 3))],
     [(1, 0, 1), ((B21 - 1) ** 2 + 2 * B21 ** 2, 1, 2)], 'u64'),
    ('seuil_u128 : une coordonnee = 2^31, voie u128',
     [(B31, B31, B31), (1, 0, 0), (0, 0, 0)],
     [(0, 0, 0), (1, 0, 0), (B31, B31, B31)],
     [(Fraction(1, 4), (0, 1)), (Fraction((B31 - 1) ** 2 + 2 * B31 ** 2, 4), (2, 3))],
     [(1, 0, 1), ((B31 - 1) ** 2 + 2 * B31 ** 2, 1, 2)], 'u128'),
    ('cube_unite : douze aretes egales, une fusion a huit enfants',
     [(x, y, z) for z in (1, 0) for y in (0, 1) for x in (1, 0)],
     [(x, y, z) for x in (0, 1) for y in (0, 1) for z in (0, 1)],
     [(Fraction(1, 4), (0, 1, 2, 3, 4, 5, 6, 7))],
     [(1, 0, 1), (1, 0, 2), (1, 0, 4), (1, 1, 3), (1, 1, 5), (1, 2, 6), (1, 3, 7)], 'u64'),
    ('chaine : pas croissants, fusions binaires',
     [(0, 0, 0), (6, 0, 0), (1, 0, 0), (3, 0, 0)],
     [(0, 0, 0), (1, 0, 0), (3, 0, 0), (6, 0, 0)],
     [(Fraction(1, 4), (0, 1)), (Fraction(1), (2, 4)), (Fraction(9, 4), (3, 5))],
     [(1, 0, 1), (4, 1, 2), (9, 2, 3)], 'u64'),
    ('deux_paires : deux fusions au meme niveau, rangees par plus petite naissance',
     [(10, 0, 0), (11, 0, 0), (0, 0, 0), (1, 0, 0)],
     [(0, 0, 0), (1, 0, 0), (10, 0, 0), (11, 0, 0)],
     [(Fraction(1, 4), (0, 1)), (Fraction(1, 4), (2, 3)), (Fraction(81, 4), (4, 5))],
     [(1, 0, 1), (1, 2, 3), (81, 1, 2)], 'u64'),
    ('un_point : une naissance, aucune fusion',
     [(5, 6, 7)], [(5, 6, 7)], [], [], 'u64'),
)


class Juge(object):
    def __init__(self, binaire, travail):
        self.binaire, self.travail = binaire, travail
        self.echecs, self.controles = [], 0

    def verifier(self, ok, quoi, genre='autre'):
        self.controles += 1
        if not ok:
            self.echecs.append((genre, quoi))
            print('ECHEC [%s] %s' % (genre, quoi))
        return ok

    def chemin(self, nom):
        return os.path.join(self.travail, nom)


def jouer_temoin(j, rang, temoin):
    nom, points, naissances, fusions, emst, voie = temoin
    nuage, arbre, aretes = j.chemin('t%d.u32le' % rang), j.chemin('t%d.arbre' % rang), j.chemin('t%d.emst' % rang)
    C.ecrire_nuage(nuage, points)
    code, sortie, erreur = C.lancer(j.binaire, [nuage, '--arbre', arbre, '--emst', aretes])
    if not j.verifier(code == C.OK and sortie is not None, '%s : code %s %s' % (nom, code, erreur.strip()), 'code'):
        return
    lus_n, lus_f = C.lire_arbre(arbre)
    j.verifier(lus_n == naissances, '%s : naissances %r' % (nom, lus_n), 'arbre')
    j.verifier(lus_f == fusions, '%s : fusions %r, attendu %r' % (nom, lus_f, fusions), 'arbre')
    j.verifier(C.lire_emst(aretes) == emst, '%s : arbre couvrant %r' % (nom, C.lire_emst(aretes)), 'emst')
    j.verifier(sortie['sha256_arbre'] == C.empreinte_arbre(naissances, fusions), '%s : sha256_arbre' % nom, 'arbre')
    j.verifier(sortie['sha256_fusions'] == C.empreinte_fusions(len(naissances), fusions), '%s : sha256_fusions' % nom,
               'arbre')
    j.verifier(sortie['sha256_emst'] == C.empreinte_emst(len(naissances), emst), '%s : sha256_emst' % nom, 'emst')
    j.verifier(sortie['arithmetique'] == voie, '%s : voie %s' % (nom, sortie['arithmetique']), 'autre')
    multi = sum(1 for _l, e in fusions if len(e) >= 3)
    j.verifier((sortie['naissances'], sortie['fusions'], sortie['multifusions']) == (len(naissances), len(fusions), multi),
               '%s : comptes %r' % (nom, (sortie['naissances'], sortie['fusions'], sortie['multifusions'])), 'arbre')
    # Meme arbre par la voie u128 forcee (deux arithmetiques, un seul objet).
    code, force, _e = C.lancer(j.binaire, [nuage, '--force-u128'])
    j.verifier(code == C.OK and force is not None and force['sha256_arbre'] == sortie['sha256_arbre'] and
               force['sha256_emst'] == sortie['sha256_emst'], '%s : voie u128 forcee' % nom, 'autre')


def refus(j):
    """Usage (code 2) et entrees invalides (code 3), dont les doublons (decision D8)."""
    vide, court, doublon, ids = j.chemin('vide.u32le'), j.chemin('court.u32le'), j.chemin('doublon.u32le'), j.chemin('x.ids')
    open(vide, 'wb').close()
    with open(court, 'wb') as f:
        f.write(b'\0' * 13)
    C.ecrire_nuage(doublon, [(1, 2, 3), (4, 5, 6), (1, 2, 3)])
    coins = j.chemin('t3.u32le')
    C.ecrire_ids(ids, [7, 8])
    cas = (([], C.USAGE), (['--inconnue'], C.USAGE), ([coins, '--bits', '18'], C.USAGE), ([coins, coins], C.USAGE),
           ([coins, '--ids'], C.USAGE), ([coins, '--bits', '21'], C.INVARIANT), ([coins, '--bits', '24'], C.INVARIANT),
           ([coins, '--bits', '32'], C.OK), ([vide], C.INVARIANT), ([court], C.INVARIANT),
           ([doublon], C.INVARIANT), ([j.chemin('absent.u32le')], C.INVARIANT), ([coins, '--ids', ids], C.INVARIANT))
    for arguments, attendu in cas:
        code, _s, erreur = C.lancer(j.binaire, arguments)
        j.verifier(code == attendu, 'refus %r : code %s, attendu %s' % (arguments, code, attendu), 'refus')
        if arguments == [doublon]:
            j.verifier('doublon' in erreur, 'refus du doublon : message %r' % erreur.strip(), 'refus')


def vidages(j):
    """Vidages MHGP11FUL1 fabriques ici a l'ordre un, sur WIT-SIX, u32_coins et le triangle."""
    six, coins, tri = TEMOINS[2], TEMOINS[3], TEMOINS[0]

    def juger(temoin, rang, octets, attendu, quoi, ids=None, extra=()):
        nom = j.chemin('v_%s.ful1' % quoi)
        with open(nom, 'wb') as f:
            f.write(octets)
        nuage = j.chemin('t%d.u32le' % rang)
        C.ecrire_nuage(nuage, temoin[1])
        arguments = [nuage, '--vidage', nom] + list(extra)
        if ids is not None:
            C.ecrire_ids(j.chemin('v.ids'), ids)
            arguments += ['--ids', j.chemin('v.ids')]
        code, sortie, erreur = C.lancer(j.binaire, arguments)
        j.verifier(code == attendu, 'vidage %s : code %s, attendu %s (%s)' % (quoi, code, attendu, erreur.strip()),
                   'vidage')
        return sortie

    ids6 = [100 + i for i in range(6)]
    ids_entree = [ids6[sorted(six[1]).index(p)] for p in six[1]]  # PointId de chaque ligne d'entree
    base = C.vidage_ful1(six[2], six[3], ids6)
    sortie = juger(six, 2, base, C.OK, 'six_identique', ids=ids_entree)
    j.verifier(sortie is not None and sortie.get('vidage', {}).get('verdict') == 'identique', 'verdict identique',
               'vidage')
    juger(six, 2, C.vidage_ful1(six[2], six[3], ids6, kmax=5), C.OK, 'six_kmax5_ordre1_seul')
    juger(six, 2, C.vidage_ful1(six[2], six[3], ids6, formes={6: (3999824, 4, 2), 8: (2000000, 2, 0)}), C.OK,
          'six_niveaux_non_reduits')
    juger(six, 2, C.vidage_ful1(six[2], six[3], ids6, formes={6: (3999825, 4, 0)}), C.DESACCORD, 'six_niveau_faux')
    swap = [(six[3][0][0], (0, 1, 3)), (six[3][1][0], (2, 4, 5)), six[3][2]]
    juger(six, 2, C.vidage_ful1(six[2], swap, ids6), C.DESACCORD, 'six_enfants_echanges')
    bouge = list(six[2])
    bouge[5] = (5732, 3001, 0)
    juger(six, 2, C.vidage_ful1(bouge, six[3], ids6), C.DESACCORD, 'six_site_deplace')
    juger(six, 2, base, C.DESACCORD, 'six_pointid_faux', ids=[ids_entree[1], ids_entree[0]] + ids_entree[2:])
    juger(six, 2, C.vidage_ful1(six[2], six[3], ids6, poids={0: 2}), C.INVARIANT, 'six_poids_2')
    juger(six, 2, b'MHGP11FUL0' + base[10:], C.INVARIANT, 'six_signature')
    juger(six, 2, base[:-5], C.INVARIANT, 'six_tronque')
    juger(six, 2, C.vidage_ful1(six[2][:5], [(Fraction(999956), (0, 1, 2)), (Fraction(1000000), (3, 4, 5))], ids6),
          C.DESACCORD, 'six_cinq_sites')
    juger(coins, 3, C.vidage_ful1(coins[2], coins[3], [1, 2, 3, 4, 5], bits=32), C.OK, 'coins_u32_identique')
    juger(coins, 3, C.vidage_ful1(coins[2], coins[3], [1, 2, 3, 4, 5], bits=24), C.INVARIANT, 'coins_hors_profil_24')
    chaine = [(Fraction(1, 2), (0, 1)), (Fraction(1, 2), (2, 3))]
    juger(tri, 0, C.vidage_ful1(tri[2], chaine, [0, 1, 2]), C.DESACCORD, 'triangle_binarise')


def main(argv):
    parser = argparse.ArgumentParser(description='Temoins graves du juge JUG-EMST')
    parser.add_argument('--juge', required=True)
    parser.add_argument('--travail', required=True)
    parser.add_argument('--mutant', choices=('binaire', 'departage'))
    try:
        args = parser.parse_args(argv[1:])
    except SystemExit:
        return C.USAGE
    os.makedirs(args.travail, exist_ok=True)
    j = Juge(os.path.abspath(args.juge), args.travail)
    for rang, temoin in enumerate(TEMOINS):
        jouer_temoin(j, rang, temoin)
    refus(j)
    vidages(j)
    genres = sorted(set(g for g, _q in j.echecs))
    print('temoins=%d controles=%d echecs=%d genres=%s' % (len(TEMOINS), j.controles, len(j.echecs), ','.join(genres)))
    if args.mutant:
        print('mutant %s : %s' % (args.mutant, 'TUE' if j.echecs else 'SURVIVANT'))
        return C.MUTANT_TUE if j.echecs else C.OK
    if j.controles != CONTROLES:
        print('plancher : %d controles, %d attendus' % (j.controles, CONTROLES))
        return C.INVARIANT
    return C.DESACCORD if j.echecs else C.OK


if __name__ == '__main__':
    sys.exit(main(sys.argv))
