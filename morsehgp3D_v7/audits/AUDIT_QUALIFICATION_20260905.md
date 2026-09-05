# Qualifications indépendantes historiques C/D/E/F

5 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

**Les qualifications ci-dessous restent attachées à leurs sources et binaires capturés.** Elles concernent le moteur réduit historique, pas le producteur FULL. Cette note conserve l'autorité propre de l'audit ; les contrats et tableaux de performance relèvent des documents développeur liés. Aucun nouveau moteur, CTest ou benchmark n'est exécuté pour cette consolidation. GCP non utilisé.

## Résultats confirmés

| Campagne | Complète Release | Ciblée Release / ASan-UBSan | Preuve indépendante |
| --- | ---: | ---: | --- |
| C arithmétique | 316 | 24 / 24 | [Inspection C/D](receipts_20260905/qualification_independent.json) |
| D MEB différée | 323 | 32 / 32 | Même inspection ; [reconstruction d'audit D](receipts_20260905/release/summary.json) distincte |
| E précontenance q2 | 324 | 33 / 33 | [Inspection E](receipts_front_compiled_20260905/qualification/e_tests_live.json) |
| F pile de témoins | 339 | 48 / 48 | [Inspection F](receipts_vertical_20260905/f_qualification/review.json) |

Les inspecteurs comparent directement inventaires, XML, blocs LastTest, commandes, sources et binaires. Les noms attendus doivent être uniques et exhaustifs ; tout `failure`, `error`, `skipped` ou statut non exécuté est rejeté. Un mutant constitue une porte passée lorsque son refus attendu est observé. Les résumés constructeur ne servent pas d'oracle.

L'inspection C/D vérifie huit XML, 140 sources D et 37 binaires de sa Release complète. Les sceaux publics ferment respectivement 48, 75 et 64 fichiers pour D complet, MEB ciblée et arithmétique, puis 24 fichiers Boost hors manifeste. Les trois projections ajoutant un LF sont déclarées. Ces nombres décrivent les captures historiques, pas le worktree actuel.

## Intégrité et autorité indépendante

La branche facultative Boost de la porte entière est effectivement compilée : le dump contient `INTEGER_GATE_BOOST=1`, `BOOST_VERSION=108300`, et le depfile inclut `cpp_int.hpp`. Les huit portes integer utilisent cette autorité ; les seize portes lanes restent OBig et littéraux. Cet acquis ne fournit pas un second pipeline.

[verify_qualification_20260905.py](verify_qualification_20260905.py) n'importe pas le juge JUnit constructeur. Ses autotests [normal](receipts_20260905/qualification_selftest_normal.json) et [optimisé](receipts_20260905/qualification_selftest_optimized.json) passent un positif et neuf corruptions. Son inspection complète dépend aussi des artefacts locaux `build/` nommés : leur absence interdit ce rejeu local, sans invalider rétroactivement le reçu public.

La fraîcheur des sources est une autorité différente, portée par [verify_current.py](verify_current.py) et [son manifeste](validation_current.json). Elle ne requalifie ni un binaire ni une campagne et ne fusionne pas les variantes historiques.

## D/E : nouvelle contrelecture des campagnes closes

Les [captures mono D/E](receipts_front_compiled_20260905/qualification/capture_manifest.json) conservent 66 fichiers, 581 074 octets. Le [lecteur indépendant](receipts_front_compiled_20260905/qualification/replay_live.json) reconstitue les chaînes de digests et lit stdout, stderr et GNU time sans parseur constructeur. Les six succès appariés à 8k concordent sur les dix cardinalités et onze digests forêt/global. Les candidats avant préfiltre varient avec s ; l'égalité porte sur les projections imprimées, sans archive complète comparée.

La liste CTest E est exactement D plus `mhgp7_meb_lazy_q2_reject_shell`, rejeté au code 4 attendu. Ses trois campagnes sont capturées dans [e_tests_capture.json](receipts_front_compiled_20260905/qualification/e_tests_capture.json), avec 140 sources et 37/9/9 binaires stables. Les deux inspecteurs portables ont chacun un positif et neuf rejets, effectifs [normalement et sous -O](receipts_front_compiled_20260905/qualification/inspector_checks.json). Leurs options `--live` ajoutent le contrôle des originaux locaux.

## Qualification F désormais fermée

Les [résultats F](receipts_vertical_20260905/f_qualification/results_live.json) relient 143 sources et références, 39/11/11 binaires et les 339/48/48 portes. Les préfixes JUnit explicitement tronqués sont confrontés aux journaux complets. Les 51 dépendances du CLI forment une couverture distincte.

Le mutant de double crédit observe trois témoins nominaux contre huit fautifs et rend 4. Dans le bras instrumenté sans observateur d'allocations, les compteurs nuls ne prouvent pas l'absence d'allocation. Les [rejeux optimisés](receipts_vertical_20260905/f_qualification/captured_optimized.json) conservent les dix corruptions rejetées. Le certificat horizontal de l'auditeur demeure attribué à E ; ces résultats F ne deviennent pas une qualification FULL.

## Harnais et classification des campagnes

Trois autorités locales restent utiles :

- [Classification](receipts_20260904/campaign_current.json) : sept tests normal/-O, dix motifs aux frontières K2/K10 et un vrai refus MEB sur onze points, code 2, zéro succès moteur. Motif inconnu ou invariant : `invalid` ; timeout : `censored`. Les [deux CTests enregistrés](receipts_20260904/campaign_registration_current.json) passent.
- [Lanceur apparié](receipts_20260904/paired_runner_delta_current.json) : quatre portes normal/-O, 24 positifs de parseur, 228 rejets et seize campagnes factices. La route historique exige `verified_events_only` ; rôles des bras, versions, K et s sont contrôlés séparément.
- [Sonde CI](receipts_iteration3/sonde_ci_current.json) : 23 scènes normal/-O sur faux binaires. Le refus d'environnement demeure actif ; ces scènes ne sont pas des runs GitHub.

## F mono et paliers : observations closes

La [revue indépendante](receipts_resolver_20260905/qualification/review.json) ferme 69 fichiers publics et 61 projections depuis les originaux. Son [lecteur portable](receipts_resolver_20260905/qualification/verify_observations.py) passe un positif et onze rejets en [normal/-O](receipts_resolver_20260905/qualification/inspector_checks.json).

Les mesures et limites sont dans [RESULTATS_MONO_F_20260905.md](../docs/RESULTATS_MONO_F_20260905.md) : trois paires E/F à 8k, succès F à 16k, refus F à 32k/K9 avec stdout vide. `observations_completed` clôt le reçu, même quand le moteur refuse. `silent_core_record_budget` borne les occurrences avant dédoublonnage ; `core=0` n'indique pas zéro travail. Temps jusqu'au refus, proxy de payload, espace virtuel et RSS restent distincts. Ces observations uniques sur hôte partagé ne qualifient aucun gain statistique ni SLO.

## Autorités des mesures et du cloud

Les mesures antérieures B/C renvoient à [RESULTATS_MONO_20260904.md](../docs/RESULTATS_MONO_20260904.md) et à leur [contrelecture](receipts_iteration3/constructor_receipts_review.json). Les [résultats G4](../docs/RESULTATS_G4_20260904.md) et le [constat de fermeture daté](../receipts/gcp_handoff_20260905.json) sont historiques, sans inventaire live ni transfert GCC11/CUDA vers GCC13 local.

Les qualifications privées MEB, native puis Builder, ont leur [note indépendante distincte](MEB_DOUBLE_BUDGET_COURANT.md) et leurs reçus propres. Elles ne changent aucun total C/D/E/F ci-dessus. La comptabilité et les coûts ne se déduisent pas des seuls digests géométriques.
