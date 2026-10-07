#!/usr/bin/env python3
"""Le juge JUG-EMST contre l'oracle borne de la v12 (reference/hgp12_ref, etage A : la definition) a l'ordre un.

    python3 oracle_rapide.py --juge <mhgp12_jug_emst> --reference <morsehgp3D_v12/reference> --travail <dossier>

Sur la suite rapide de l'oracle (families.fast_suite() : 34 fixtures gravees et 11 familles a graine fixe, dont
grilles, droites, plans, cocirculaires, cospheriques et doublons), pour chaque nuage :
  - nuage a doublons : refus exige (code 3, decision D8) ;
  - sinon l'arbre du juge doit egaler l'ordre un de Definition(points) : naissances (centres), niveaux exacts,
    enfants, numerotation canonique ; sha256_arbre recalcule ici depuis l'oracle ;
  - meme nuage translate de (2^31, 2^31, 2^31) : voie u128, naissances translatees, fusions identiques (empreinte
    sha256_fusions inchangee : l'invariance par translation est jugee par translation explicite) ;
  - vidage MHGP11FUL1 a l'ordre un ecrit ici depuis l'arbre de l'ORACLE : le juge doit le declarer identique.
Les compteurs de couverture sont exacts (suite deterministe) : toute derive est un plancher viole.

Codes : 0 conforme ; 1 desaccord ; 2 usage ; 3 plancher viole. Python 3.10 nu, aucun assert.
"""
import argparse
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import commun as C  # noqa: E402

DECALAGE = 1 << 31
# Compteurs exacts de la suite rapide (premier passage le 7 octobre 2026 ; a regraver seulement si la suite de
# l'oracle change, en connaissance de cause).
ATTENDU = dict(nuages=342, refuses=34, juges=308, naissances=1846, fusions=1312, multifusions=158, arite_max=8,
               niveaux=1215, niveaux_plusieurs=87, vidages=308, translates=308)


def main(argv):
    parser = argparse.ArgumentParser(description='JUG-EMST contre l\'oracle borne, ordre un')
    parser.add_argument('--juge', required=True)
    parser.add_argument('--reference', required=True)
    parser.add_argument('--travail', required=True)
    parser.add_argument('--graver', action='store_true', help='affiche les compteurs sans les juger')
    try:
        args = parser.parse_args(argv[1:])
    except SystemExit:
        return C.USAGE
    sys.path.insert(0, os.path.abspath(args.reference))
    try:
        from hgp12_ref import Definition, families
    except ImportError as erreur:
        print('usage : oracle introuvable sous %s (%s)' % (args.reference, erreur))
        return C.USAGE
    os.makedirs(args.travail, exist_ok=True)
    juge = os.path.abspath(args.juge)
    nuage, arbre, vidage = (os.path.join(args.travail, nom) for nom in ('n.u32le', 'n.arbre', 'n.ful1'))
    compte = dict((cle, 0) for cle in ATTENDU)
    ecarts = []

    def ecart(nom, quoi):
        ecarts.append('%s : %s' % (nom, quoi))
        print('ECART %s : %s' % (nom, quoi))

    for cloud in families.fast_suite():
        compte['nuages'] += 1
        points = [tuple(int(c) for c in p) for p in cloud.points]
        C.ecrire_nuage(nuage, points)
        code, sortie, erreur = C.lancer(juge, [nuage, '--arbre', arbre])
        if len(set(points)) != len(points):
            if code != C.INVARIANT or 'doublon' not in erreur:
                ecart(cloud.name, 'doublons : code %s, refus attendu (code 3)' % code)
            compte['refuses'] += 1
            continue
        if code != C.OK or sortie is None:
            ecart(cloud.name, 'code %s %s' % (code, erreur.strip()))
            continue
        compte['juges'] += 1
        ordre = Definition(points).order(1)
        naissances = [tuple(int(c) for c in nd.center) for nd in ordre.nodes if not nd.children]
        fusions = [(nd.level, tuple(nd.children)) for nd in ordre.nodes if nd.children]
        if any(nd.level != 0 or any(c.denominator != 1 for c in nd.center) for nd in ordre.nodes if not nd.children):
            ecart(cloud.name, 'naissance d\'ordre un de l\'oracle hors des sites')
        lus_n, lus_f = C.lire_arbre(arbre)
        if lus_n != naissances:
            ecart(cloud.name, 'naissances %r, oracle %r' % (lus_n, naissances))
        if lus_f != fusions:
            ecart(cloud.name, 'fusions %r, oracle %r' % (lus_f, fusions))
        if sortie['sha256_arbre'] != C.empreinte_arbre(naissances, fusions):
            ecart(cloud.name, 'sha256_arbre different de celui de l\'oracle')
        compte['naissances'] += len(naissances)
        compte['fusions'] += len(fusions)
        compte['multifusions'] += sum(1 for _l, e in fusions if len(e) >= 3)
        compte['arite_max'] = max([compte['arite_max']] + [len(e) for _l, e in fusions])
        niveaux = [l for l, _e in fusions]
        compte['niveaux'] += len(set(niveaux))
        compte['niveaux_plusieurs'] += sum(1 for l in set(niveaux) if niveaux.count(l) > 1)
        # Translation explicite : voie u128, memes fusions, naissances translatees.
        C.ecrire_nuage(nuage, [(x + DECALAGE, y + DECALAGE, z + DECALAGE) for x, y, z in points])
        code, decale, erreur = C.lancer(juge, [nuage])
        if code != C.OK or decale is None or decale['arithmetique'] != 'u128' or \
                decale['sha256_fusions'] != sortie['sha256_fusions'] or \
                decale['sha256_arbre'] != C.empreinte_arbre([(x + DECALAGE, y + DECALAGE, z + DECALAGE)
                                                             for x, y, z in naissances], fusions):
            ecart(cloud.name, 'translation de 2^31 : code %s %s' % (code, erreur.strip()))
        else:
            compte['translates'] += 1
        # Vidage a l'ordre un ecrit depuis l'oracle, PointId arbitraires (decroissants).
        C.ecrire_nuage(nuage, points)
        ids = [1000 - i for i in range(len(naissances))]
        with open(vidage, 'wb') as f:
            f.write(C.vidage_ful1(naissances, fusions, ids))
        code, jugement, erreur = C.lancer(juge, [nuage, '--vidage', vidage])
        if code != C.OK or jugement is None or jugement['vidage']['verdict'] != 'identique':
            ecart(cloud.name, 'vidage de l\'oracle : code %s %s' % (code, erreur.strip()))
        else:
            compte['vidages'] += 1
    print('oracle_rapide ' + ' '.join('%s=%d' % (cle, compte[cle]) for cle in ATTENDU))
    if args.graver:
        return C.OK
    if ecarts:
        print('desaccords : %d' % len(ecarts))
        return C.DESACCORD
    derives = [cle for cle in ATTENDU if compte[cle] != ATTENDU[cle]]
    if derives:
        print('plancher viole : %s' % ', '.join('%s=%d (attendu %d)' % (c, compte[c], ATTENDU[c]) for c in derives))
        return C.INVARIANT
    return C.OK


if __name__ == '__main__':
    sys.exit(main(sys.argv))
