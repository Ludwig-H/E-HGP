# Revue de porte — phase 0

## Décision

La phase 0 corrective v2 est fermée. La porte d'entrée de la phase 1 est satisfaite pour le backend `reference_cpu`, les profils `hgp_reduced` et `full_pi0`, et le mode `certified`. La phase 1 est ouverte uniquement pour requalifier l'oracle et ses campagnes contre Gamma exhaustif; elle n'est pas encore fermée.

Le contre-exemple exact `gabriel-point-set-counterexample-5-points-v1` invalide la base `reduced_manuscript_theorem_5` du contrat v1. La cible scientifique reste la tour des K-polyèdres : le contrat v2 définit donc `hgp_reduced` exact par Gamma exhaustif sur le backend CPU de référence. Le flot Gabriel brut ne fournit qu'une connectivité positive conditionnelle. Cette correction ne démontre pas M.1 et n'autorise aucun code CUDA.

## Artefacts reçus

| exigence | preuve de réception |
|---|---|
| énoncé candidat M.1 et obligations ouvertes | [contrat M.1](M1_RECONSTRUCTION.md), H1–H8, O1–O9 et CE-M1-01 à CE-M1-12 |
| profils distincts | [exemples contractuels](EXEMPLES_CONTRACTUELS.md) et [matrice de traçabilité](MATRICE_ENONCE_TEST_CERTIFICAT.md) |
| schémas versionnés | [`morsehgp3d-contract-v2.schema.json`](../../schemas/morsehgp3d-contract-v2.schema.json), 21 types requis dont `GammaCoface`, avec [v1 archivée](../../schemas/morsehgp3d-contract-v1.schema.json) |
| unités, indexation et canonisation | [conventions v2](SCHEMA_CONVENTIONS_V2.md) |
| cinq sorties attendues | [`tests/fixtures/contracts`](../../tests/fixtures/contracts/) |
| validation et round-trip | [`tools/check_contracts.py`](../../tools/check_contracts.py) et [`tests/contracts`](../../tests/contracts/) |
| obstruction Gabriel | [preuve des incidences silencieuses](../math/INCIDENCES_SILENCIEUSES_GAMMA.md), [registre des preuves, verrou V0](../math/STATUT_PREUVES_ET_HEURISTIQUES.md#v0--contre-exemple-à-la-réduction-de-gabriel) et fixture exacte permanente |
| germes d'indice un | [contrat M.1, sections 2 à 4](M1_RECONSTRUCTION.md#2-notation) |
| morphismes verticaux comprimés | [spécification, section 11](../SPECIFICATION_MORSEHGP3D.md#11-morphismes-verticaux) et type `VerticalMap` du schéma |
| position générale et doublons | [spécification, section 12](../SPECIFICATION_MORSEHGP3D.md#12-domaine-exact-et-dégénérescences) et `InputSemantics` |

Le commit `b305a39` (`feat: figer les contrats MorseHGP3D v1`) est l'archive historique. Le commit `5351704` (`feat: définir le contrat MorseHGP3D v2`) fournit le schéma actif, les validateurs, le migrateur canonique, les fixtures et la documentation corrective.

## Évaluation de la porte de sortie

- `hgp_reduced` et `full_pi0` ont des contrats, identifiants et bases de preuve distincts.
- `hgp_reduced` exact exige `hgp-reduced-v2`, `gamma_exhaustive_reference` et `effective_backend=reference_cpu`.
- `gabriel_positive_connectivity` exige `partial_refinement`, `require_exact=false` et un statut conditionnel ou de budget.
- `public_status` est une fonction vérifiée des champs du certificat; les scores de benchmark n'interviennent jamais dans cette décision.
- `full_pi0` ne peut pas publier `exact` avec `proof_basis=m1_conditional_contract`.
- Un mode `budgeted` ne peut pas publier `exact`.
- Les unités publiques sont fermées et les niveaux sont des rayons carrés rationnels canoniques.
- Les objets critiques refusent les champs inconnus.
- Les cinq exemples couvrent l'ordre un, une naissance isolée, une fusion binaire, une multifusion et un recouvrement d'ordre deux; leur migration v2 passe les tests actifs et atteint un point fixe canonique.
- M.1 reste `proof_obligation` jusqu'à la phase 12.
- Aucun code CUDA n'a été introduit.

La porte de sortie G0 est satisfaite. Cette décision ferme uniquement les formats et la sémantique contractuelle v2. La phase 1 doit encore reconstruire `hgp_reduced` depuis Gamma, conserver Gabriel comme voie partielle séparée et requalifier sa campagne avant que G2 ou la phase 2A puissent être ouverts.

## Commandes de réception

```bash
python tools/check_docs.py
python tools/check_contracts.py
python -m unittest discover -s tests/contracts -p 'test_*.py'
python tools/rekey_contract_fixtures_v2.py
python -m unittest discover -s tests/oracle -p 'test_*.py'
python tools/run_oracle_campaign.py --ci --manifest /tmp/morsehgp3d-oracle-ci-v2.json
python tools/check_references.py
python tools/check_scope.py
python tools/check_implementation_status.py
python tools/check_gcp_workflows.py
python -m unittest discover -s tests/gcp -p 'test_*.py'
bash -n gcp-migration/*.sh
```

Résultats enregistrés le 14 juillet 2026 sur le commit `5351704` : 24 documents actifs, 21 définitions de contrat, 21 exemples de schéma, cinq fixtures v2 au point fixe canonique, 18 tests de contrat et 81 tests d'oracle validés. La campagne CI bornée a terminé ses trois cas avec `status=passed`; sa propre `matrix_gate` reste fausse et ne constitue donc pas une preuve de fermeture de la phase 1. Les cinq références, les 20 entrées du registre, les workflows GCP en lecture seule, les 11 tests de sécurité GCP et la syntaxe des scripts ont aussi été validés.

## Limites reportées à leurs phases

- la preuve O1–O9 de M.1 reste en phase 12;
- la requalification Gamma de l'oracle exhaustif et de ses tests aléatoires appartient à la phase 1;
- les prédicats filtrés commencent en phase 2A;
- la validation octet par octet des sérialisations C++/Python précède CUDA;
- les dégénérescences non génériques restent hors statut `exact` tant que leur phase n'est pas fermée.

## GCP

GCP n'a pas été utilisé pour la phase 0. Aucun inventaire global ni mutation n'était requis.
