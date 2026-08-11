# Index des audits MorseHGP3D v3

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Cet index est volontairement réduit aux autorités encore utiles. Les snapshots
historiques conservés sont explicitement étiquetés; aucun statut ancien n'est
recopié dans les documents live. Une référence de prototype vers un fichier
supprimé est un défaut documentaire à inventorier dans l'audit courant, pas une
autorité ressuscitée. Un titre, un message de commit ou un CTest vert ne vaut
jamais réception.

## Verdict live

- [`AUDIT_ETAT_COURANT.md`](AUDIT_ETAT_COURANT.md) est l'unique verdict mutable :
  il distingue le `HEAD`, le delta éventuel du worktree, les empreintes utiles,
  les contre-exemples, les tests qualifiables et les portes ouvertes.

Le résumé est [`../README.md`](../README.md) et l'architecture durable est
[`../PROPOSITION.md`](../PROPOSITION.md). Ces deux documents ne doivent pas
dupliquer un statut de commit : ils renvoient au verdict live. Les
spécifications et le registre des preuves sous `docs/` restent supérieurs.

## Snapshots et preuves conservés

| objet | portée exacte |
| --- | --- |
| [`AUDIT_Q2_SELFJOIN_8A39C53.md`](AUDIT_Q2_SELFJOIN_8A39C53.md) | preuve locale q2, réfutation du différentiel compensable de `8a39c53` et profil de coût du snapshot; tout successeur est jugé dans l'audit courant |
| [`AUDIT_DELTA_CBAC109_SIDECAR_ET_SOURCE_20260811.md`](AUDIT_DELTA_CBAC109_SIDECAR_ET_SOURCE_20260811.md) | contre-exemples du sidecar `cbac109` et contrat de frontière; le statut du successeur est uniquement live |
| [`AUDIT_JUNG_ANCHOR_389A742.md`](AUDIT_JUNG_ANCHOR_389A742.md) | contre-fixture permanente à une ancre de Jung insuffisamment certifiée |
| [`NOTE_COEUR_UNIVERSEL_JUNG_ANCRES_Q3_Q4_20260811.md`](NOTE_COEUR_UNIVERSEL_JUNG_ANCRES_Q3_Q4_20260811.md) | preuve des certificats cœur/profondeur q3/q4, hypothèses, égalités fail-open et limites industrielles |
| [`NOTE_CERTIFICAT_HELLY_DISQUE_JUNG_20260811.md`](NOTE_CERTIFICAT_HELLY_DISQUE_JUNG_20260811.md) | preuve du certificat de couverture du disque par groupes disjoints de trois témoins au plus, décision exacte et limites de complexité |
| [`AUDIT_DEGRE_GABRIEL_KISSING_SMAX11_20260811.md`](AUDIT_DEGRE_GABRIEL_KISSING_SMAX11_20260811.md) | réfutation exacte de toute borne de degré Gabriel par le kissing number ou `smax`; deux constructions u16 aux rangs 2 et 11, baseline de Poisson et conséquences industrielles; statut logiciel exclusivement dans l'audit live |
| [`NOTE_CLAUDE_SESSION_G4_MASSONLY_50K_20260811.md`](NOTE_CLAUDE_SESSION_G4_MASSONLY_50K_20260811.md) | provenance de la session G4 mass-only et arrêt de la cible; déclaration de session, pas verdict produit |
| [`NOTE_SOLUTION_SOURCE_Q2_YAO48_LBVH_U16_20260811.md`](NOTE_SOLUTION_SOURCE_Q2_YAO48_LBVH_U16_20260811.md) | spécification de la route produit q2 : Morton/LBVH, coupe Yao48 stricte fail-open, classifieur terminal, census fermé, ledger, juge indépendant et gate d'exposant; statut logiciel exclusivement dans l'audit live |
| [`NOTE_SOLUTION_P1A_CENTER_COVER_MASSONLY_20260811.md`](NOTE_SOLUTION_P1A_CENTER_COVER_MASSONLY_20260811.md) | spécification auditée du falsificateur P1a q4 mass-only : domaine de Jung, 64 patchs rationnels, témoins collectifs, ledger bijectif et protocole direct `n=32` vers 50 k; statut logiciel exclusivement dans l'audit live |
| [`AUDIT_REEMPLOI_YAO48_P1A_LIGNE_ENREGISTREE_20260811.md`](AUDIT_REEMPLOI_YAO48_P1A_LIGNE_ENREGISTREE_20260811.md) | inventaire du prior art CUDA Yao48/LBVH et P1a dans `morsehgp3d/`, limites de qualification et propositions exactes de réemploi; différentiel, jamais autorité v3 |
| [`AUDIT_REEMPLOI_EMST_YAO48_LIGNE_ENREGISTREE_20260811.md`](AUDIT_REEMPLOI_EMST_YAO48_LIGNE_ENREGISTREE_20260811.md) | théorème Yao-1 contenant l'EMST canonique, prior art LBVH/Kruskal enregistré, rejet CPU et contrat de mutualisation exacte avec q2; blueprint, jamais preuve de débit |
| [`AUDIT_RECU_YAO48_ECHELLE_2E49DCF_20260811.md`](AUDIT_RECU_YAO48_ECHELLE_2E49DCF_20260811.md) | audit de la rampe CPU mono-binaire q2 à 12,5/25/50 k; trois familles structurées rouges, temps non qualifiables et ordonnance courante NO-GO avant G4 |
| [`AUDIT_REPONSES_G4_Q2_YAO1_20260811.md`](AUDIT_REPONSES_G4_Q2_YAO1_20260811.md) | réponses closes à Claude : diagnostic CPU G4 admissible mais non pertinent maintenant, causalité q2 non prouvée, banque exacte de onze, certificat dual `Q--W` et ordre de réception Yao-1 |

## Lemmes conditionnels, contre-fixtures et portes citées

Ces fichiers ne décrivent pas le `HEAD`. Ils conservent une contre-fixture, un
lemme dont les hypothèses restent explicites ou le contrat d'une porte encore
citée par le code. Leur présence dans cet index ne les promeut pas en preuve
formelle enregistrée :

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

Deux reçus à la racine de `receipts/` sont conservés comme diagnostics datés,
jamais comme portes v3 actuelles :

| fichier | SHA-256 | portée |
| --- | --- | --- |
| [`census_tukey_shallow_20260808.json`](../receipts/census_tukey_shallow_20260808.json) | `aba8abc5e479a8900a2c83aa0cc5618a3e0a05bc9a59963572c140738a5ea128` | minorant heuristique par 4 096 directions aléatoires; `git_commit=unavailable`, aucune complétude exacte |
| [`oracle_campaign_20260808.json`](../receipts/oracle_campaign_20260808.json) | `2579cd5a8eee14bc6e3d7e6ef83bdf052faacbcf90d2636e37e5c29c0c755bca` | différentiel exhaustif borné du sujet v2 à `n=8/11`; ne reçoit aucun worktree v3 |

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

Le dossier
[`../receipts/yao48_scale_20260811/`](../receipts/yao48_scale_20260811/)
contient la rampe q2 CPU auditée séparément :

| fichier | SHA-256 | portée |
| --- | --- | --- |
| `scale_counters_raw.txt` | `acf8e89248131cc7fdce3246f559d380acbee4ce67548ac9fb5e26efdd67d889` | douze ledgers count-only fermés sur un binaire dont la provenance a été reconstruite; aucun payload ni temps qualifiable |
| `exponents_derived.txt` | `f2d9783211d884fef821a45961d428ee645bad656685d1520337957f54d2776f` | exposants arithmétiquement justes; masses de couverture et secondes exclues de la gate de travail, arrondi `1,35` ambigu pour une valeur brute strictement rouge |

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
