#!/usr/bin/env python3
"""Rejeu du contre-exemple de contact de l'auditeur continu (uniform_majority_contact_20260930) sous MM_kappa.

Cadre : phase=exploration_v10_hors_registre, backend=cpu_reference, profile=quantized_u18_input_only,
mode=interaction_auditeur, public_status=not_claimed. GCP non utilise. Aucun moteur modifie.
Fixture (auditeur) : a=(0,0,0), b=(10,0,0), z=(9-eps,3,0), y=(0,0,9), K=2 ; ici homothetie S=1024 et eps = e/S,
e dans {0, 1} (jitter d'une unite de grille). Mesure : hauteur de reunion u(a, b) (rayon) sous plusieurs regles.
Codes : 0 conforme ; 1 attente contredite.
"""
import json
import os
import sys

sys.dont_write_bytecode = True
LIB = '/workspaces/E-HGP/build/v10-verrou-points/fixtures_cibles/lib'
MMC = '/workspaces/E-HGP/build/v10-verrou-points/revision_cible/majorites_continues'
sys.path.insert(0, LIB)
sys.path.insert(0, MMC)
import regles as RG  # noqa: E402
import mmc  # noqa: E402

S = 1024


def scene(e):
    pts = {'a': (0, 0, 0), 'b': (10 * S, 0, 0), 'z': (9 * S - e, 3 * S, 0), 'y': (0, 0, 9 * S)}
    return RG.Scene(RG.normaliser_points(pts), 2, nom='contact_e%d' % e, gamma='auto')


def main():
    out = {'cadre': 'exploration_v10_hors_registre, interaction_auditeur, public_status=not_claimed', 'S': S,
           'cas': []}
    regles_lib = ['maj_bande_unif[1/8]', 'maj_bande_unif[1/4]', 'P_2', 'core', 'cover_A1']
    for e in (0, 1):
        sc = scene(e)
        gf = mmc.GammaForest(sc.P, sc.K)
        a, b = sc.noms.index('a'), sc.noms.index('b')
        ligne = {'e': e}
        for r in regles_lib:
            h = sc.regle(r)
            u = h.hauteur(a, b)
            ligne[r] = [float(u) / S, u.texte() if hasattr(u, 'texte') else str(u)]
        for kappa in (2, 3):
            for eta in ('1', '1/4'):
                from fractions import Fraction
                rg = mmc.mm_gamma(sc, gf, Fraction(kappa), Fraction(eta))
                u = rg.h.hauteur(a, b)
                ligne['MMg[k=%d,e=%s]' % (kappa, eta)] = [float(u) / S, u.texte() if hasattr(u, 'texte') else str(u)]
                rc = mmc.mm_catalogue(sc, Fraction(kappa), Fraction(eta))
                u = rc.h.hauteur(a, b)
                ligne['MMc[k=%d,e=%s]' % (kappa, eta)] = [float(u) / S, u.texte() if hasattr(u, 'texte') else str(u)]
        out['cas'].append(ligne)
    print(json.dumps(out, indent=1, ensure_ascii=False))
    return 0


if __name__ == '__main__':
    sys.exit(main())
