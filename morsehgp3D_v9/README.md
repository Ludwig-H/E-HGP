# Morse HGP 3D v9 — ouverture après l'audit général de la v8

Ouverture demandée le 22 septembre 2026, sur `main` uniquement.

```text
phase=exploration_v9_hors_registre
backend=reference_cpu (premier moteur v9 : generateur v8 + tour v7, 18 bits)
profile=quantized_u18_input_only (grille 1 mm, contrat temps)
mode=ouverture_audit_v8_et_v7
public_status=not_claimed
```

La v9 succède à la v8 comme chantier actif. Elle contient l'audit général de
la v8 (et de ce qui était bon en v7), le plan, l'héritage, les fausses pistes,
et depuis le 22 septembre au soir un premier moteur : la chaîne générateur
exact → catalogue recoupé → tour FULL, jugée par le juge T2 (voir la
[passation](PASSATION.md) et la [provenance](docs/PROVENANCE.md)). La v8 et la
v7 restent des sources différentielles et des réservoirs de fixtures, jamais
des autorités implicites.

## Objectif

Calculer la **tour HGP FULL** (définie et prouvée en v7 : minima Gabriel,
multifusions, parents, verticales, extension non régulière) d'une trame
SemanticKITTI **sans sol** de 30 000 à 60 000 sites, grille 1 mm, moteur entier
exact à 18 bits, en moins de **1 s** puis **100 ms** sur G4, à **K = 5** puis
**K = 10**, avec parallélisation multi-CPU puis GPU. Les décisions de
l'utilisateur et les points encore ouverts sont dans la
[synthèse](docs/AUDIT_V8_SYNTHESE.md) § 2 et § 9.

## Verdict de la v8 en quelques lignes

- La v8 livre un **générateur exact de candidats** q2/q3/q4 en entier, parallèle
  sur CPU, élargi à 18 bits, avec des certificats de rejet prouvés et une
  discipline de reçus exemplaire. 129 des 132 tests passent au commit audité
  12294241 ; les trois autres sont désactivés par construction.
- Elle **ne livre pas la tour** : ni catalogue canonique, ni forêts, ni parents.
  Le contrat n'est donc pas mesurable ; le flux seul coûte 6 à 100 fois le
  budget d'une seconde, dominé par l'atlas q4.
- Aucun code GPU. Quatre sessions G4, toutes CPU, toutes arrêtées et certifiées.
- La v7 avait la tour FULL (50k uniforme : 419 s à K10, 34 s à K5) : la v9
  réunit le générateur de la v8 et l'aval de la v7, mesurés de bout en bout.

## Commencer ici

1. [Passation](PASSATION.md) : état exact au 22 septembre, travail non commis
   d'autres acteurs, première tranche.
2. [Audit général de la v8](docs/AUDIT_V8_SYNTHESE.md) : verdict, contrats,
   chiffres, défauts, questions à l'utilisateur.
3. [Plan de la v9](docs/PLAN_V9.md) : phases, portes, règles de travail.
4. [Héritage v7 et v8](docs/HERITAGE_V7_V8.md) : ce qu'il faut porter, avec
   pins, et les fixtures à graver.
5. [Fausses pistes](docs/FAUSSES_PISTES.md) : ce qu'il ne faut pas rouvrir.
6. [Rapports détaillés de l'audit](docs/audit_v8/README.md) : douze lentilles
   contre-vérifiées.
7. [Reçu de l'audit](receipts/audit_v8_20260922/README.md) : inventaire épinglé
   et suite CTest rejouée au commit audité.

Auditeurs : [état courant](audits/ETAT_COURANT.md) et canal
[`audits/COORDINATION_MORSEHGP3D_V9.md`](../audits/COORDINATION_MORSEHGP3D_V9.md).

## Organisation

Structure attendue, calquée sur les versions précédentes : `src/`, `tests/`,
`oracle/`, `bench/`, `cmake/`, `docs/`, `audits/` (propriété des auditeurs
indépendants), `receipts/` (captures immuables). Les dossiers de code sont des
emplacements réservés. Conventions : C++20, `-Wall -Wextra -Wpedantic -Werror`,
namespace `mhgp9`, cibles et tests `mhgp9_*`, macros `MHGP9_*`, portes Python
sans `assert` (valides sous `python3 -O`).

Commandes (Boost obligatoire : `libboost-dev`, ou `-DBOOST_ROOT=<préfixe>`
d'un `libboost1.83-dev` extrait, voir le [plan](docs/PLAN_V9.md) V9-0) :

```bash
cmake -S morsehgp3D_v9 -B build/v9 -DCMAKE_BUILD_TYPE=Release
cmake --build build/v9 --parallel
ctest --test-dir build/v9 --output-on-failure -L gate
./build/v9/mhgp9_tower_probe <trame.u32le> K workers [--s=8] [--static=T] [--no-tower]
```

Options CMake : `MHGP9_SANITIZE` (ASan/UBSan), `MHGP9_TSAN`.
