# Revue de porte — phase 0

## Décision

La phase 0 est fermée. La porte d'entrée de la phase 1 est satisfaite pour le backend `reference_cpu`, les profils `hgp_reduced` et `full_pi0`, et le mode `certified`.

Cette décision ne démontre pas M.1 et n'autorise aucun code CUDA. Elle signifie uniquement que l'objet, les statuts, les schémas et les résultats contractuels sont assez précis pour construire l'oracle CPU indépendant.

## Artefacts reçus

| exigence | preuve de réception |
|---|---|
| énoncé candidat M.1 et obligations ouvertes | [contrat M.1](M1_RECONSTRUCTION.md), H1–H8, O1–O9 et CE-M1-01 à CE-M1-12 |
| profils distincts | [exemples contractuels](EXEMPLES_CONTRACTUELS.md) et [matrice de traçabilité](MATRICE_ENONCE_TEST_CERTIFICAT.md) |
| schémas versionnés | [`morsehgp3d-contract-v1.schema.json`](../../schemas/morsehgp3d-contract-v1.schema.json), 20 types requis et 56 définitions |
| unités, indexation et canonisation | [conventions v1](SCHEMA_CONVENTIONS_V1.md) |
| cinq sorties attendues | [`tests/fixtures/contracts`](../../tests/fixtures/contracts/) |
| validation et round-trip | [`tools/check_contracts.py`](../../tools/check_contracts.py) et [`tests/contracts`](../../tests/contracts/) |
| preuve Gabriel | [catalogue critique, section 3](../math/CATALOGUE_CRITIQUE_3D.md#3-équivalence-exacte-avec-gabriel) |
| germes d'indice un | [contrat M.1, sections 2 à 4](M1_RECONSTRUCTION.md#2-notation) |
| morphismes verticaux comprimés | [spécification, section 11](../SPECIFICATION_MORSEHGP3D.md#11-morphismes-verticaux) et type `VerticalMap` du schéma |
| position générale et doublons | [spécification, section 12](../SPECIFICATION_MORSEHGP3D.md#12-domaine-exact-et-dégénérescences) et `InputSemantics` |

L'artefact logiciel principal est enregistré par le commit `b305a39` (`feat: figer les contrats MorseHGP3D v1`).

## Évaluation de la porte de sortie

- `hgp_reduced` et `full_pi0` ont des contrats, identifiants et bases de preuve distincts.
- `public_status` est une fonction vérifiée des champs du certificat; les scores de benchmark n'interviennent jamais dans cette décision.
- `full_pi0` ne peut pas publier `exact` avec `proof_basis=m1_conditional_contract`.
- Un mode `budgeted` ne peut pas publier `exact`.
- Les unités publiques sont fermées et les niveaux sont des rayons carrés rationnels canoniques.
- Les objets critiques refusent les champs inconnus.
- Les cinq exemples couvrent l'ordre un, une naissance isolée, une fusion binaire, une multifusion et un recouvrement d'ordre deux.
- M.1 reste `proof_obligation` jusqu'à la phase 12.
- Aucun code CUDA n'a été introduit.

La porte de sortie décrite par la feuille de route est donc satisfaite.

## Commandes de réception

```bash
python tools/check_docs.py
python tools/check_contracts.py
python -m unittest discover -s tests/contracts -p 'test_*.py'
python tools/check_references.py
python tools/check_scope.py
python tools/check_implementation_status.py
python tools/check_gcp_workflows.py
python -m unittest discover -s tests/gcp -p 'test_*.py'
bash -n gcp-migration/*.sh
```

Résultats enregistrés le 14 juillet 2026 : 20 documents actifs, 20 types contractuels, 20 exemples de schéma, cinq fixtures, neuf tests de contrat et onze tests de sécurité GCP validés.

## Limites reportées à leurs phases

- la preuve O1–O9 de M.1 reste en phase 12;
- l'oracle exhaustif et ses tests aléatoires commencent en phase 1;
- les prédicats filtrés commencent en phase 2A;
- la validation octet par octet des sérialisations C++/Python précède CUDA;
- les dégénérescences non génériques restent hors statut `exact` tant que leur phase n'est pas fermée.

## GCP

GCP n'a pas été utilisé pour la phase 0. Aucun inventaire global ni mutation n'était requis.
