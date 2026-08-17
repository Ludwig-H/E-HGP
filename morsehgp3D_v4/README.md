# morsehgp3D_v4 — reprise de zéro

Cadre : `phase=exploration_v4_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

La v4 remplace `morsehgp3D_v3/` comme chantier actif. Elle repart de zéro sur
le code, pas sur les leçons : les théorèmes v3 relus, les bugs documentés
(WSPD quadratique par cap, scission par population, tie-break Morton 16 bits)
et les fixtures gravées sont repris comme fondations — voir
`docs/MATHEMATIQUES.md` § statuts et `docs/ARCHITECTURE.md` § 2.2.

## Contrats

Forêt HGP complète K = 1..10, événements et niveaux exacts, en **< 100 ms sur
une G4** (secondaire : K = 5, < 1 s) ; nuages jusqu'à des **dizaines de
millions de points** ; pipeline GPU-friendly avec fallback CPU parallèle.
Aucun claim tant que les portes ne le prouvent pas : `public_status` reste
`not_claimed`.

## Parcours de lecture

1. [`docs/MATHEMATIQUES.md`](docs/MATHEMATIQUES.md) — l'objet (manuscrit,
   Défs 20–31, Théorèmes 2–7), la réduction événements-boules q2/q3/q4, les
   fuseaux `W_q`, l'élagage `h_coeur/h_a/h_b`, les statuts de chaque énoncé,
   les questions ouvertes Q1–Q5.
2. [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) — une seule structure
   spatiale (arbre radix sur positions uniques ; « Morton » n'est qu'une clé
   de tri), le pipeline en six étapes, la descente ternaire qui tue, l'état
   d'implémentation.
3. [`docs/PLAN_DE_TESTS.md`](docs/PLAN_DE_TESTS.md) — tailles 8000/16000/
   32000 (64000 en extension), familles v3 bit à bit, s = 6/8/10, invariants,
   mutants, fixtures reprises.
4. [`audits/README.md`](audits/README.md) — le contrat de l'auditeur
   mathématique (seul dossier où il écrit).

## Construire et tester

```bash
cmake -S morsehgp3D_v4 -B build/v4 -DCMAKE_BUILD_TYPE=Release
cmake --build build/v4 --parallel
ctest --test-dir build/v4 --output-on-failure
```

Préfixe des cibles et tests : `mhgp4_`. Portes à code de sortie exact
(0 conforme, 1 juge, 2 refus, 3 invariant, 4 mutant tué), crash par signal
refusé partout. Probe d'échelle :

```bash
./build/v4/mhgp4_wspd_scaling_probe --family=uniform --n=8000 --s=8 --seed=3
```

## Arborescence

```text
docs/        MATHEMATIQUES.md, ARCHITECTURE.md, PLAN_DE_TESTS.md
audits/      dossier de l'auditeur (cycle NOTE/QUESTION/AUDIT/REPONSE)
src/core     types u16/i64/i128, Morton 48 bits
src/cloud    familles de nuages (port bit à bit v3) + contre-familles
src/tree     arbre radix de Karras sur positions uniques (LA structure)
src/wspd     front par vagues, prédicat de séparation entier, ledger
src/events   (à venir) fuseaux W_q, certificats, instruction q2/q3/q4
src/forest   (à venir) BallKey/census, dix forêts, rendu § 9.1
oracle/      (à venir) juge indépendant, arithmétique volontairement autre
bench/       probes counter-only (campagnes d'échelle)
tests/       selftests, fixtures gravées, portes
```
