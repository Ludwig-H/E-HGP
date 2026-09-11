# Audit indépendant v7

Lire l’[état courant](ETAT_COURANT.md), puis le [dialogue actif](DIALOGUE_COURANT.md). La [coordination entre auditeurs](COORDINATION_AUDITEURS.md) répartit les écritures. Les décisions reprises par le développeur sont dans le [dossier principal](../PASSATION.md) et les [fausses pistes](../docs/FAUSSES_PISTES.md) ; ce dossier conserve leurs preuves indépendantes.

| Sujet courant | Qualification indépendante |
| --- | --- |
| Raccord par ancres et cache ad7ffd28 | [Rejeu sur octets commités, corpus et quatre blocs nommés 50k](receipts_cache_commit_20260911/README.md) ; second auditeur ; deux verrous clos sur CPU, qualification relative au census fourni |
| Comptabilité des lots privés | [Débordement reproduit et correction vérifiée](receipts_batch_work_20260911/README.md) ; consommation cumulative hôte 83f1/993786f3 désormais contre-vérifiée, sans exécution device |
| Noyau MEB | [Piste du second auditeur](NOTE_CLAUDE_COEUR_MEB_20260911.md) ; [refus valide sur K7 et repli vérifié O2/SAN](receipts_meb_boundary_20260911/README.md), aucun nouveau temps de tour |
| Graphe filtré et vraies hiérarchies | [Réduction aux naissances, forêts minimales, contributions et verticales](receipts_filtered_graph_20260911/README.md) ; preuve et modèle borné, sans nouveau raccord produit |
| Phase statique CPU | [SAN et preuves des raccourcis](receipts_static_followup_20260911/README.md) ; semis après échange désormais intégré et mesuré, support survivant encore non mesuré |
| Gardes du journal réservé, publié | [Dix cas valides, neuf rejets, six lectures, neuf mutants et pannes d’allocation](receipts_journal_guards_20260910/README.md) ; preuves indépendantes conservées du prototype incrémental |
| Journal incrémental privé | [Préfixes et lots mixtes O2/SAN, trois mutants causaux](receipts_incremental_prefix_20260911/README.md) ; [preuve et écart net de résidence](receipts_incremental_review_20260911/README.md), premier raccord FULL testé puis non retenu pour ses coûts |
| Résidence de la nouvelle tour retenue | [Durées de vie, borne des brouillons et format des naissances](receipts_tower_cost_review_20260910/README.md) ; calculs statiques, pas gains mesurés |
| Journal daté v2 et parents exportés | [184 cas O2/SAN, 2 976 coupes et mutant du tableau de parents](receipts_coverage_cpp_20260910/README.md) ; [lacune du test constructeur corrigée](../receipts/coverage_parent_array_20260910/README.md), raccord FULL distinct |
| Modèle FULL et taille de sortie | [Décision et domaine](NIVEAUX_ET_CERTIFICAT_HGP_COURANT.md), [borne à K fixé et témoins u16](receipts_probe_meb_review_20260906/full_output_growth.md) |
| Priorité 50k : plateaux non réguliers | [Trois cas globaux décidés](receipts_plateaux_full_20260906/GLOBAL_PARENTS.md), [C++ local publié](../receipts/local_plateau_20260906/README.md), [preuves et ancres](receipts_plateaux_full_20260906/README.md) ; raccord [publié](../docs/TOUR_FULL_PAR_BOULES.md), contrelecture attribuée au second auditeur |
| Vraie hiérarchie K-NN et tour sur les minima | [Descente et prototype K1..4](receipts_gabriel_vertices_20260906/README.md), [ancres partagées et choix de retrait](receipts_shared_anchors_20260906/README.md) |
| Génération WSPD et histogrammes | [Sélection stable et saturation](receipts_phase_selection_20260906/README.md), [blocs](receipts_block_histograms_20260906/README.md), [terminal unique et cœur q2 positif](receipts_terminal_count_20260906/README.md) |
| Admission du payload CPU | [Raccord contre-lu et fixture à sept points](receipts_census_followup_20260906/README.md), [preuve des gardes](receipts_phase_selection_20260906/README.md) ; pas borne RSS |
| Cache, lots et normalisation | [Verdict](CACHE_FULL_COURANT.md), [normalisation v2](receipts_full_successor_20260905/README.md), [lot unitaire](receipts_full_singleton_20260905/README.md) |
| EAGER et composant structurel | [Producteur relatif](PRODUCTEUR_FULL_GABRIEL_COURANT.md), [lecteur](CERTIFICAT_FULL_CPP_COURANT.md) |
| Mono : mesures courantes et diagnostics conservés | [Captures courantes, bornes et refus MEB](MONO_FULL_COURANT.md) |
| MEB filtrée dans FULL | [Qualification indépendante jusqu’à K10](receipts_full_meb_20260906/README.md), [preuves et réutilisation terminale proposée](MEB_DOUBLE_BUDGET_COURANT.md) |
| Lecteurs MEB et historique des formats | [Bornes de Work et suivi des formats](receipts_probe_meb_review_20260906/README.md) |
| Intégrations différées | [Questions secondaires regroupées](QUESTIONS_SECONDAIRES.md) |

## Preuves conservées

Ces notes portent des arguments encore cités par le dossier principal ; elles ne sont pas des demandes de qualification à recommencer.

- Géométrie : [S1](S1_COURANT.md), [front](FRONT_ET_TEMOINS_COURANT.md), [secteurs/cordes](PREUVE_CHORD_SECTOR_COURANTE.md), [cellules](CELLULES_COURANT.md), [filtres](FILTRES_FLOTTANTS_COURANTS.md), [lanes](ARITHMETIQUE_LANES_COURANTE.md), [fuseaux](ARITHMETIQUE_SPINDLE_COURANTE.md).
- Exécution : [index](AUDIT_INDEX_20260905.md), [domaine CPU](DOMAINE_CPU_COURANT.md), [qualifications D/E/F](AUDIT_QUALIFICATION_20260905.md), [prétest q2 E](ADDENDUM_MEB_Q2_E_20260905.md), [MEB à deux budgets](MEB_DOUBLE_BUDGET_COURANT.md).
- Hiérarchie : [certificat réduit E](CERTIFICAT_HORIZONTAL_COURANT.md), [verticales](CONTRAT_VERTICAL_COURANT.md), [incidences pondérées](CONTRAT_MASSES_VOTE_COURANT.md), [comparateur p3](AUTORITE_VOTE_P3_COURANTE.md).
- Reçus anciens sans note active redondante : [D/E](receipts_20260905/README.md), [front compilé](receipts_front_compiled_20260905/README.md), [mono](receipts_20260904/mono_current.json), [census axis](receipts_iteration3/axis_execution.json), [résidence](receipts_20260904/residence_current.json), [composition](receipts_20260904/math_current_repro.json), [frontière de fenêtre](receipts_20260904/math_window_repro.json), [archive](receipts_20260904/archive_delta_current.json).

## Fraîcheur et entretien

Les sondes et juges datés du 4 au 6 septembre restent à la racine : 39 d’entre eux dérivent leurs chemins de `__file__` et tous sont épinglés par des reçus scellés, un déplacement casserait des preuves. Toute nouvelle sonde vit dans le paquet de reçu qui la consomme ; aucun fichier épars nouveau à la racine. Les répertoires de travail locaux `.work*` sont ignorés par Git et supprimés une fois leur paquet publié.

```bash
python3 -B -O morsehgp3D_v7/audits/verify_current.py
```

Le [manifeste](validation_current.json) vérifie des sources et preuves épinglées, affiche leur portée et ne réexécute aucun test. Code 0 : une variante entière correspond ; 1 : sources ou documents à actualiser ; 2 : manifeste invalide. Un fichier non épinglé n’est pas qualifié par ce contrôle. Les variantes D à Q gardent leurs autorités distinctes ; P et Q épinglent les contrelectures du second auditeur, sans réception générale du raccord ni réattribution aux sources courantes.

Le [registre d’entretien](ENTRETIEN.json) donne les notes supprimées, leurs remplacements et leur version Git antérieure. Les sources, reçus scellés, contre-fixtures et échecs restent intacts. Les questions sans incidence immédiate sont raccourcies dans un seul fichier. Un nouvel audit ou push doit apporter une décision, une preuve, une correction ou un entretien utile ; aucune publication de routine sans contenu pertinent.

Écritures uniquement ici, sur `main`. `public_status=not_claimed`. GCP non utilisé.
