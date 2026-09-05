# Audits courants de MorseHGP3D v7

Entrée actualisée le 5 septembre 2026. Commencer par l’[état courant](ETAT_COURANT.md), la [synthèse](AUDIT_INDEPENDANT_20260904.md), puis les [demandes actives au constructeur](DIALOGUE_COURANT.md). Les écritures de l’auditeur restent exclusivement dans ce dossier.

| Contrat ou preuve maintenue | Rapport de référence |
| --- | --- |
| Producteur horizontal FULL depuis les Gabriel | [Qualification indépendante courante](PRODUCTEUR_FULL_GABRIEL_COURANT.md), [sources et reçus](receipts_full_producer_20260905/README.md), [alias à la demande](receipts_full_producer_20260905/lazy_alias_next_step_review.md) |
| Premier certificat FULL C++ autonome | [Qualification indépendante courante](CERTIFICAT_FULL_CPP_COURANT.md), [sources et reçus](receipts_full_cpp_20260905/README.md) |
| HGP complet du manuscrit : minima, multifusions et portails | [Décision courante FULL](NIVEAUX_ET_CERTIFICAT_HGP_COURANT.md), [preuves et modèles exécutés](receipts_gabriel_20260905/README.md) |
| Certificat horizontal réduit et domaine accepté | [Certificat courant](CERTIFICAT_HORIZONTAL_COURANT.md), [preuves exécutées](receipts_horizontal_20260905/README.md) |
| Reconstruction verticale du profil réduit depuis `born` et `parents` | [Preuve et contrat](CONTRAT_VERTICAL_COURANT.md), [lecteur et reçus](receipts_resolver_20260905/README.md) |
| Univers d’incidence, masses et vote | [Contrat pondéré](CONTRAT_MASSES_VOTE_COURANT.md), [comparaison exacte des numérateurs p3](AUTORITE_VOTE_P3_COURANTE.md) |
| Modèle, incidences et réduction horizontale | [Mathématiques](AUDIT_MATHEMATIQUE_20260904.md), [composition](REPONSE_AUDITEUR_COMPOSITION.md), [frontière de fenêtre](RETOUR_MATH_COURANT.md) |
| Couverture des boules jusqu’au RLE | [S1](S1_COURANT.md), [index](AUDIT_INDEX_20260905.md), [raccord front/cover](AUDIT_RACCORD_INDEX_FRONT_20260905.md) |
| Géométrie des témoins et bornes opérationnelles | [Front](FRONT_ET_TEMOINS_COURANT.md), [fuseaux](ARITHMETIQUE_SPINDLE_COURANTE.md), [secteurs/cordes](ARITHMETIQUE_SECTEUR_CORDE_COURANTE.md), [cellules](ARITHMETIQUE_CELLULES_COURANTE.md) |
| Certificats de prune | [Secteurs/cordes](PREUVE_CHORD_SECTOR_COURANTE.md), [cellules](CELLULES_COURANT.md), [marges flottantes](FILTRES_FLOTTANTS_COURANTS.md) |
| Proposition MEB privée et budgets par ordre | [Preuve et raccord local](MEB_DOUBLE_BUDGET_COURANT.md), [reçus indépendants](receipts_meb_dual_20260905/README.md), [qualification native et coût local](receipts_meb_native_20260905/README.md), [raccord compilé du Builder](receipts_meb_builder_20260905/README.md) |
| Formes, niveaux et MEB | [Lanes](ARITHMETIQUE_LANES_COURANTE.md), [entiers larges](ARITHMETIQUE_LARGE_COURANTE.md), [MEB D](AUDIT_MEB_DIFFEREE_20260905.md), [delta q2 E](ADDENDUM_MEB_Q2_E_20260905.md) |
| Exécution et preuves de campagne | [Domaine CPU](DOMAINE_CPU_COURANT.md), [arrondi](AUDIT_ARRONDI_20260905.md), [qualification et harnais](AUDIT_QUALIFICATION_20260905.md) |
| Archive, résidence et budgets | [Interfaces et nettoyage A1](AUDIT_INTERFACES_20260904.md), [résidence](AUDIT_RESIDENCE_20260904.md), [mémoire](RETOUR_MEMOIRE_COURANT.md), [mono](MONO_COURANT.md), [census](CENSUS_AXIS_COURANT.md) |

## Contrôler la fraîcheur

```bash
PYTHONDONTWRITEBYTECODE=1 python3 -O morsehgp3D_v7/audits/verify_current.py
```

Le [manifeste courant](validation_current.json) épingle les preuves communes et les variantes complètes D/E/F/G/H. Le code 0 affiche la variante reconnue **et sa portée** : 323 portes D exécutées indépendamment, 324 portes E du constructeur contre-vérifiées et certificat horizontal propre, 339 portes F et deux campagnes ciblées de 48 portes contre-vérifiées, avec conservation statique de pile ; G, certificat FULL structurel qualifié par ses propres sondes C++ et ses reçus dédiés ; ou H, producteur horizontal relatif qualifié sur ses catalogues indépendants, budgets et portails, avec ses propres reçus 7+7 CTests. Il ne transfère aucun résultat entre variantes et ne réexécute aucun test. Il compare une liste de fichiers épinglés : un nouveau fichier FULL hors de cette liste n’est pas qualifié par son code 0. La preuve FULL et ses modèles d’audit ont leur attribution séparée. Le code 1 demande une actualisation de sources ; le code 2 rejette un manifeste invalide. La compatibilité `--manifest` conserve l’accès aux anciens formats. Les 30 [tests de ce contrôleur](test_verify_current.py) passent normalement et sous `-O`.

La publication `f4c0734c` des trois clarifications documentaires est désormais [relue et reprise dans les pins](receipts_full_cpp_20260905/source_delta_review.json). L’ancien écart de worktree est clos. G ajoute la nouvelle cible et ses deux portes CMake, ainsi que les deux sources FULL ; les 142 autres pins F restent identiques. Le contrôle de fraîcheur reconnaît ce snapshot entier, avec sa portée structurelle explicite. H ajoute le producteur relatif, son oracle et ses deux portes, ainsi que les textes et le probe effectivement relus. Les fichiers préparés ensuite, notamment le juge du probe mono, restent hors de H. Le contrat de performance ayant changé, son ancien pin est conservé dans chacune des variantes historiques et son nouveau pin dans H : un worktree constructeur en avance et le dernier commit publié gardent ainsi leurs attributions distinctes.

## Entretien du dossier

Une conclusion active a un rapport de référence. Douze notes transitoires ou dépassées ont été fusionnées et retirées ; le [registre de consolidation](receipts_front_20260905/documentation_retirement.json) donne pour chacune son remplacement, le commit historique et son hash. Les fixtures permanentes et reçus bruts, y compris échecs et refus, restent à leur emplacement reproductible. Les répertoires `.work*` sont des temporaires ignorés, pas des autorités publiques.

Les [reçus D/E](receipts_20260905/README.md) conservent leurs attributions historiques. Les [certificats entiers du front](receipts_front_20260905/README.md) sont complétés par le [raccord C++ exécuté](receipts_front_compiled_20260905/README.md). Les contrôles CTest locaux, CI et campagnes du constructeur restent distincts. Les [nouveaux rejeux verticaux et p3](receipts_resolver_20260905/README.md) conservent leurs limites d’audit et la contrelecture des paliers F : succès 16k, refus budgétaire 32k à K9. Le [raccord privé MEB à deux budgets](MEB_DOUBLE_BUDGET_COURANT.md) possède ses sondes rationnelles et UBSan ; la nouvelle qualification du Builder ajoute budgets persistants, wrapper avec coface silencieuse et miroirs sur exceptions. Son intégration produit reste distincte de F. Les demandes désormais fermées sont retirées des entrées courantes. La priorité active est le raccord multi-ordre, le manifeste terminal et les suppléments vertical et pondéré du [certificat FULL](NIVEAUX_ET_CERTIFICAT_HGP_COURANT.md). Le stockage, le lecteur et le [producteur par portails](PRODUCTEUR_FULL_GABRIEL_COURANT.md) ont désormais leurs qualifications séparées. Les nouveaux paliers mono du constructeur restent hors de cette campagne. L’ancienne preuve réduite et les résultats MEB conservent leurs limites.

Statut public : `not_claimed`. Aucun registre officiel modifié. GCP non utilisé.
