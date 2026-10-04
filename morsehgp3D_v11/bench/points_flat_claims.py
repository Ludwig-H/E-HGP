#!/usr/bin/env python3
"""Regles de revendication des preenregistrements E1, bibliotheque standard seule (porte CTest en Python nu).

    python3 bench/points_flat_claims.py --selftest

holm : ajustement de Holm-Bonferroni (step-down, monotone). h_l1_claim : p de Holm strictement < 0,05.
h_l2_claim (P08, H_L2, non-inferiorite en IoU, marge 0,02) : p de Holm strictement < 0,05 ET borne basse de l'IC
a 95 % strictement > -0,02 ; la p-valeur et l'IC restent deux criteres separes, aucun n'implique l'autre. Les
fixtures de frontiere viennent de la contrelecture du 4 octobre (receipts/audit_selfreview_20261004, flat_verdict) :
p Holm 0,0300969903 avec IC [-0,021 ; -0,019] et une borne basse exactement egale a -0,02 ne revendiquent pas.
Codes : 0 conforme ; 1 ecart.
"""
import argparse
import sys

ALPHA = 0.05
MARGIN = 0.02


def holm(pvalues):
    order = sorted(range(len(pvalues)), key=lambda i: pvalues[i])
    adjusted = [0.0] * len(pvalues)
    running = 0.0
    for rank, i in enumerate(order):
        running = max(running, min(1.0, (len(pvalues) - rank) * pvalues[i]))
        adjusted[i] = running
    return adjusted


def h_l1_claim(p_holm):
    return p_holm is not None and p_holm < ALPHA


def h_l2_claim(p_holm, ci_low, margin=MARGIN):
    return p_holm is not None and ci_low is not None and p_holm < ALPHA and ci_low > -margin


def selftest():
    problems = []

    def need(ok, why):
        if not ok:
            problems.append(why)

    need(not h_l2_claim(0.0300969903, -0.021), 'IC basse -0,021 revendiquee')
    need(not h_l2_claim(0.0300969903, -0.02), 'IC basse egale a -0,02 revendiquee')
    need(h_l2_claim(0.0300969903, -0.0199), 'IC basse -0,0199 refusee')
    need(not h_l2_claim(0.05, 0.01), 'p de Holm egale a 0,05 revendiquee')
    need(h_l2_claim(0.0499, 0.01), 'p 0,0499 et IC positive refusee')
    need(not h_l2_claim(None, 0.01) and not h_l2_claim(0.01, None), 'valeur absente revendiquee')
    need(h_l1_claim(0.0499) and not h_l1_claim(0.05) and not h_l1_claim(None), 'H_L1 a la frontiere')
    got = holm([0.01, 0.04, 0.03, 0.005])
    need(got == [0.03, 0.06, 0.06, 0.02], 'Holm %s' % got)
    need(holm([0.5, 0.6]) == [1.0, 1.0], 'Holm plafonne a 1')
    need(holm([]) == [], 'Holm vide')
    for problem in problems:
        print('ecart : ' + problem)
    print('points_flat_claims_verdict %s checks10' % ('conforme' if not problems else 'ecart'))
    return 0 if not problems else 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--selftest', action='store_true')
    args = parser.parse_args()
    if not args.selftest:
        parser.error('--selftest seulement')
    return selftest()


if __name__ == '__main__':
    sys.exit(main())
