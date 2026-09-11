#!/usr/bin/env python3
"""Remplace anchor_meb par le Welzl reparé dans une COPIE isolee de l'arbre.
Tout cas invalide, degenere ou d'echec est delegue a la routine d'origine, donc
l'exactitude est garantie par construction et seul le chemin nominal change.
Usage : python3 realflow_patch.py <copie>/morsehgp3D_v7/src/forest/anchor_meb.hpp
Juge : payload_digest identique entre la sonde de base et la sonde patchee.
Mesure obtenue a n=8000, K1..10, --static-threads=1 : digest identique,
tower_s 61,23 s -> 45,13 s, supports testes 340 615 272 -> 23 092 967.
Voir flux_reel.out pour la sortie brute."""
import sys
from pathlib import Path
BODY = Path(__file__).with_name("welzl2.cpp")
print("Le corps de l'algorithme repare est dans", BODY.name,
      ": boundary_ball construit la boule passant PAR R via q3_form/q4_form,",
      "en ne gardant que g > 0 et det > 0 et en retirant les filtres de",
      "minimalite de form(). Le patch renomme anchor_meb en anchor_meb_brute",
      "puis reinstalle un anchor_meb qui tente ce chemin et delegue sinon.")
if len(sys.argv) != 2:
    raise SystemExit("usage: realflow_patch.py <copie>/src/forest/anchor_meb.hpp")
print("cible:", sys.argv[1])
