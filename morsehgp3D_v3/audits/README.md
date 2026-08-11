# Index des audits MorseHGP3D v3

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Ce dossier est volontairement réduit aux autorités encore utiles. Les anciennes
notes de livraison, réponses, benchmarks sans reçu et snapshots remplacés ont
été supprimés du dépôt actif; leur historique reste consultable dans Git. Un titre,
un message de commit ou un CTest vert ne vaut jamais réception.

## Verdict live

- [`AUDIT_ETAT_COURANT.md`](AUDIT_ETAT_COURANT.md) est l'unique verdict mutable :
  il épingle le `HEAD`, les empreintes utiles, les contre-exemples, les tests
  qualifiables et les portes ouvertes.

Le résumé est [`../README.md`](../README.md) et l'architecture durable est
[`../PROPOSITION.md`](../PROPOSITION.md). En cas de dérive temporelle, corriger
ces trois fichiers dans le même passage; les spécifications et le registre des
preuves sous `docs/` restent supérieurs.

## Snapshots et preuves conservés

| objet | portée exacte |
| --- | --- |
| [`AUDIT_Q2_SELFJOIN_8A39C53.md`](AUDIT_Q2_SELFJOIN_8A39C53.md) | preuve locale q2, réfutation du différentiel compensable de `8a39c53` et profil de coût du snapshot; tout successeur est jugé dans l'audit courant |
| [`AUDIT_DELTA_CBAC109_SIDECAR_ET_SOURCE_20260811.md`](AUDIT_DELTA_CBAC109_SIDECAR_ET_SOURCE_20260811.md) | contre-exemples du sidecar `cbac109` et contrat de frontière; le statut du successeur est uniquement live |
| [`AUDIT_JUNG_ANCHOR_389A742.md`](AUDIT_JUNG_ANCHOR_389A742.md) | contre-fixture permanente à une ancre de Jung insuffisamment certifiée |
| [`NOTE_COEUR_UNIVERSEL_JUNG_ANCRES_Q3_Q4_20260811.md`](NOTE_COEUR_UNIVERSEL_JUNG_ANCRES_Q3_Q4_20260811.md) | preuve des certificats cœur/profondeur q3/q4, hypothèses, égalités fail-open et limites industrielles |
| [`NOTE_CLAUDE_SESSION_G4_MASSONLY_50K_20260811.md`](NOTE_CLAUDE_SESSION_G4_MASSONLY_50K_20260811.md) | provenance de la session G4 mass-only et arrêt de la cible; déclaration de session, pas verdict produit |

## Preuves statiques citées par les prototypes

Ces fichiers ne décrivent pas le `HEAD`; ils conservent un théorème, une
contre-fixture ou le contrat d'une gate encore citée par le code :

| objet | portée |
| --- | --- |
| [`AUDIT_CONNECTIVITE_ORDER_K_A8111F0.md`](AUDIT_CONNECTIVITE_ORDER_K_A8111F0.md) | connectivité shallow conditionnelle de l'arrangement |
| [`AUDIT_ORDER_K_FLATS_9C587E6.md`](AUDIT_ORDER_K_FLATS_9C587E6.md) | contre-fixtures permanentes de `order_k_flats` |
| [`AUDIT_SOURCE_DIRECTE_24AD3D37.md`](AUDIT_SOURCE_DIRECTE_24AD3D37.md) | invariants et contre-exemples de la source directe |
| [`AUDIT_VOIE_MULTIPLICITES_ORDER_K.md`](AUDIT_VOIE_MULTIPLICITES_ORDER_K.md) | propriétaire shallow avec multiplicités |
| [`NOTE_GATE_D_PREMIERES_INCIDENCES_DU_COEUR.md`](NOTE_GATE_D_PREMIERES_INCIDENCES_DU_COEUR.md) | dichotomie des premières incidences du cœur |
| [`NOTE_GATE_D_UNE_ATTACHE_PAR_FACETTE_COEUR.md`](NOTE_GATE_D_UNE_ATTACHE_PAR_FACETTE_COEUR.md) | attache canonique conditionnelle par facette cœur |
| [`NOTE_PARENT_LOCAL_REVERSE_SEARCH_GATE_D.md`](NOTE_PARENT_LOCAL_REVERSE_SEARCH_GATE_D.md) | parent local conditionnel de reverse search |
| [`NOTE_POSITIVE_INDEX_KD_EXACT_ET_CERTIFICAT_PINCEAU.md`](NOTE_POSITIVE_INDEX_KD_EXACT_ET_CERTIFICAT_PINCEAU.md) | prédicats d'index spatial exact et contre-fixture flottante |
| [`check_gate_d_fold_f0.py`](check_gate_d_fold_f0.py) | gate Python F0 enregistrée par CMake; son succès reste local à ses fixtures |

## Reçus

Les reçus G4 mass-only sont dans
[`../receipts/g4_massonly_20260811/`](../receipts/g4_massonly_20260811/).

| fichier | SHA-256 | portée |
| --- | --- | --- |
| `cell_50k_raw.txt` | `6b355d0d9c7bf01dbdeb1d14dc442cab75570e6be044dcd50f314d79b9010afe` | masses de cellules, aucun tuple ni pipeline |
| `mask_scale_raw.txt` | `d82e43c7f4b32a5731cfdb2bbb9edf22cd7cecef0fdc73e84d1457277d61c740` | scaling count-only, aucun fold |

Le dossier
[`../receipts/selfjoin_q2_20260811/`](../receipts/selfjoin_q2_20260811/)
contient trois journaux CPU diagnostiques :

| fichier | SHA-256 | portée |
| --- | --- | --- |
| `scale_counters_raw.txt` | `2685ceb387f46cb0be2f0a04f7b1ad8afbcaa41c521dad20328c7a4cb5332bc5` | snapshot de l'ancien binaire, 15 runs nuls et le contre-exemple 12 500 rouge |
| `scale_counters_correctif_12500_raw.txt` | `3ade1bc74dd2f129a9c26079fe8c52195946e8ccd479c587e462e2d40144149d` | autre binaire et autre contrat local; diagnostic correctif séparé, pas réécriture du reçu rouge |
| `anchor_core_counters_raw.txt` | `6f7938c53da21a55e8e8072d66dc2cea400a2bea2628845f578b1dcf5dfc70a7` | campagne terminée 400/1 200/2 400; en-tête source incomplet et portes core non reçues |

Leurs compteurs peuvent falsifier une route; leurs temps sous charge ne sont
ni un benchmark reçu ni `warm_e2e`.

## Autorités externes

- [`SPECIFICATION_MORSEHGP3D.md`](../../docs/SPECIFICATION_MORSEHGP3D.md) :
  contrat et SLO.
- [`STATUT_PREUVES_ET_HEURISTIQUES.md`](../../docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md) :
  registre des preuves et réfutations.
- [`INCIDENCES_SILENCIEUSES_GAMMA.md`](../../docs/math/INCIDENCES_SILENCIEUSES_GAMMA.md) :
  inertie H0.
- [`CATALOGUE_PAIRES_DIAMETRALES_EXACT.md`](../../docs/math/CATALOGUE_PAIRES_DIAMETRALES_EXACT.md) :
  architecture q2 Yao/LBVH.
- [`RNG_JUNG_CLIQUES_ET_NIVEAUX_PEUPROFONDS.md`](../../docs/math/RNG_JUNG_CLIQUES_ET_NIVEAUX_PEUPROFONDS.md) :
  Jung et limites des graphes low-rank.

GCP non utilisé.
