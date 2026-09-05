# Certificats et entretien du front — 5 septembre 2026

Cadre : exploration v7 hors registre, CPU de référence, entrée u16, audit indépendant, `public_status=not_claimed`. GCP non utilisé.

| Contrôle léger | Autorité et résultat |
| --- | --- |
| [Fuseaux](spindle_bounds.json) | Modèle entier Python : 92 corrections de racine, 125 identités, contre-fixture de mauvais arrondi, majorants des intermédiaires |
| [Secteurs/cordes](secteur_corde_arithmetic.json) | 368 directions, rayon inscrit/D² au moins 1/6, 53 racines dont 13 corrigées ; aucun C++ exécuté |
| [Cellules](cell_width.json), [mode optimisé](cell_width_optimized.json) | Majorants, huit frontières de garde i64, 17 frontières de racine, trois variantes fautives détectées ; aucun C++ exécuté |
| [Fraîcheur normale](freshness_normal.json), [optimisée](freshness_optimized.json) | 30 scènes chacune : D/E entiers acceptés, mélanges refusés, codes 0/1/2 et schémas v1/v2 |
| [Consolidation documentaire](documentation_retirement.json) | Douze notes retirées de l’entrée active, hash historique et rapport de remplacement conservés |

Les trois preuves mathématiques sont accessibles depuis le [sommaire courant](../README.md). La suite CTest D de 323 portes reste dans son [reçu distinct](../receipts_20260905/release/summary.json) ; ces nouveaux contrôles ne s’y additionnent pas et ne certifient pas une suite complète E.

Le [manifeste courant](../validation_current.json) reconnaît des snapshots entiers et affiche leur portée de qualification. Les reçus datés antérieurs, y compris leurs échecs de préparation, ne sont pas réécrits pour les rendre artificiellement courants.
