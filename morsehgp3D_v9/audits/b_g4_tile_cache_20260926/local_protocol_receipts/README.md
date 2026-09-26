# Rejeu local du protocole S1/S2 — 26 septembre 2026

Base : `d0e711e23617a7fa2838cac9b370ff125d4be5fb`.
Capture fraîche avant la session G4 du cache par tuiles ; protocole et
moteur inchangés. **GCP non utilisé par ce rejeu.**

Les deux commandes complètes passent :

- `python3 -B gcp-migration/gpu_filter_selftest_v9.py -v` : 13 tests,
  code 0 ; 58,124 s de mur externe, 57,320 s rapportées par unittest.
- `python3 -B -O gcp-migration/gpu_filter_selftest_v9.py -v` : 13 tests,
  code 0 ; 57,942 s de mur externe, 57,428 s rapportées par unittest.

[`receipt.json`](receipt.json) conserve les commandes exactes, la version
Python et les SHA-256 de tous les fichiers suivis de `gcp-migration/`
avant et après les deux exécutions : ils sont inchangés. Les sorties
brutes sont les fichiers `normal.*` et `optimized.*`. Le lanceur
[`run.py`](run.py) refuse d'écraser une capture existante.

Ces portes exercent le **faux cloud** : succès référence/cache, mutant de
masque, écarts de résultats, schémas et temps incohérents, lecture des
anciens formats, refus avant mesure et arrêt ciblé simulé après les
pannes. Elles ne compilent pas CUDA, ne qualifient aucune géométrie GPU,
ne donnent aucun temps G4 et ne certifient l'arrêt d'aucune VM réelle.
