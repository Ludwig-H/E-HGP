# Audits courants de MorseHGP3D v7

Entrée actualisée le 5 septembre 2026. Commencer par l’[état courant](ETAT_COURANT.md), la [synthèse](AUDIT_INDEPENDANT_20260904.md), puis les [demandes actives au constructeur](DIALOGUE_COURANT.md). Les écritures de l’auditeur restent exclusivement dans ce dossier.

| Contrat ou preuve maintenue | Rapport de référence |
| --- | --- |
| Cache FULL paresseux, digest et admission | [Qualification courante](CACHE_FULL_COURANT.md), [preuves et contrôles de reçus](receipts_full_lazy_20260905/README.md) |
| Campagne FULL mono et coût des alias | [Résultats, bornes et suite constructive](MONO_FULL_COURANT.md), [paquets, juge et analyse indépendante](receipts_full_mono_20260905/README.md) |
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

Le [manifeste courant](validation_current.json) épingle les preuves communes et les variantes complètes D/E/F/G/H/I/J. Le code 0 affiche la variante reconnue **et sa portée** : 323 portes D exécutées indépendamment, 324 portes E du constructeur contre-vérifiées et certificat horizontal propre, 339 portes F et deux campagnes ciblées de 48 portes contre-vérifiées, avec conservation statique de pile ; G, certificat FULL structurel qualifié par ses propres sondes C++ et ses reçus dédiés ; H, producteur horizontal relatif qualifié sur ses catalogues indépendants, budgets et portails, avec ses propres reçus 7+7 CTests ; I, publication `98bb6578` du même producteur et campagne mono scellée contre-vérifiée ; J, port lazy `13c6cc72` qualifié indépendamment, 14+14 CTests propres, admission n=8 de la sonde avec digests et supplément first-C contre-vérifiés. Il ne transfère aucun résultat entre variantes et ne réexécute aucun test. Il compare une liste de fichiers épinglés : un nouveau fichier FULL hors de cette liste n’est pas qualifié par son code 0. La preuve FULL et ses modèles d’audit ont leur attribution séparée. Le code 1 demande une actualisation de sources ; le code 2 rejette un manifeste invalide. La compatibilité `--manifest` conserve l’accès aux anciens formats. Les 30 [tests de ce contrôleur](test_verify_current.py) passent normalement et sous `-O`.

G ajoute la cible structurelle et ses deux portes CMake, ainsi que les deux sources FULL ; les 142 autres pins F restent identiques. H ajoute le producteur relatif, son oracle et ses deux portes, ainsi que les textes et le probe relus. I conserve le code H publié dans `98bb6578`, actualise README/PASSATION et ajoute les pièces effectivement contre-vérifiées du mono. Le contrat de performance reste épinglé par variante. J ajoute les sources lazy, les contrats relus, les 198 pièces publiques de qualification ciblée et les 469 pièces d’admission micro. Sa provenance est le worktree capturé après `6f4b4de5`, distincte d’un commit produit. Les grandes campagnes concurrentes et leur document de résultats ne sont pas qualifiés par cette variante. Les six anciennes variantes restent intactes ; les observations EAGER ne sont pas réattribuées à lazy.

## Entretien du dossier

Une conclusion active a un rapport de référence. Douze notes transitoires ou dépassées ont été fusionnées et retirées ; le [registre de consolidation](receipts_front_20260905/documentation_retirement.json) donne pour chacune son remplacement, le commit historique et son hash. Les fixtures permanentes et reçus bruts, y compris échecs et refus, restent à leur emplacement reproductible. Les répertoires `.work*` sont des temporaires ignorés, pas des autorités publiques.

Les [reçus D/E](receipts_20260905/README.md), [preuves compilées du front](receipts_front_compiled_20260905/README.md), qualifications MEB et [rejeux verticaux/p3](receipts_resolver_20260905/README.md) conservent leurs attributions. Les demandes satisfaites sont retirées des entrées actives. Le stockage, le lecteur et le [producteur par portails](PRODUCTEUR_FULL_GABRIEL_COURANT.md) ont leurs qualifications séparées ; les [paliers mono FULL](MONO_FULL_COURANT.md) ont maintenant leur contre-vérification propre. Le [cache qualifié](CACHE_FULL_COURANT.md), la liaison par digest et les contrôles du juge quittent la liste des demandes ouvertes. La priorité active est leur comparaison de coût appariée, puis le raccord à l’export industriel sous autorité terminale. Les suppléments vertical et pondéré du [certificat FULL](NIVEAUX_ET_CERTIFICAT_HGP_COURANT.md) restent distincts.

Statut public : `not_claimed`. Aucun registre officiel modifié. GCP non utilisé.
