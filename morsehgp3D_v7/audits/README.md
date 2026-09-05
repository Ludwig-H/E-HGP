# Audits courants de MorseHGP3D v7

Entrée actualisée le 5 septembre 2026. Commencer par l’[état courant](ETAT_COURANT.md), la [synthèse](AUDIT_INDEPENDANT_20260904.md), puis les [demandes actives au constructeur](DIALOGUE_COURANT.md). Les écritures de l’auditeur restent exclusivement dans ce dossier.

| Contrat ou preuve maintenue | Rapport de référence |
| --- | --- |
| Modèle, incidences et réduction horizontale | [Mathématiques](AUDIT_MATHEMATIQUE_20260904.md), [composition](REPONSE_AUDITEUR_COMPOSITION.md), [frontière de fenêtre](RETOUR_MATH_COURANT.md) |
| Couverture des boules jusqu’au RLE | [S1](S1_COURANT.md), [index](AUDIT_INDEX_20260905.md), [raccord front/cover](AUDIT_RACCORD_INDEX_FRONT_20260905.md) |
| Géométrie des témoins et bornes opérationnelles | [Front](FRONT_ET_TEMOINS_COURANT.md), [fuseaux](ARITHMETIQUE_SPINDLE_COURANTE.md), [secteurs/cordes](ARITHMETIQUE_SECTEUR_CORDE_COURANTE.md), [cellules](ARITHMETIQUE_CELLULES_COURANTE.md) |
| Certificats de prune | [Secteurs/cordes](PREUVE_CHORD_SECTOR_COURANTE.md), [cellules](CELLULES_COURANT.md), [marges flottantes](FILTRES_FLOTTANTS_COURANTS.md) |
| Formes, niveaux et MEB | [Lanes](ARITHMETIQUE_LANES_COURANTE.md), [entiers larges](ARITHMETIQUE_LARGE_COURANTE.md), [MEB D](AUDIT_MEB_DIFFEREE_20260905.md), [delta q2 E](ADDENDUM_MEB_Q2_E_20260905.md) |
| Exécution et preuves de campagne | [Domaine CPU](DOMAINE_CPU_COURANT.md), [arrondi](AUDIT_ARRONDI_20260905.md), [qualification et harnais](AUDIT_QUALIFICATION_20260905.md) |
| Archive, résidence et budgets | [Interfaces et nettoyage A1](AUDIT_INTERFACES_20260904.md), [résidence](AUDIT_RESIDENCE_20260904.md), [mémoire](RETOUR_MEMOIRE_COURANT.md), [mono](MONO_COURANT.md), [census](CENSUS_AXIS_COURANT.md) |

## Contrôler la fraîcheur

```bash
PYTHONDONTWRITEBYTECODE=1 python3 -O morsehgp3D_v7/audits/verify_current.py
```

Le [manifeste courant](validation_current.json) épingle les preuves communes et les variantes complètes D/E. Le code 0 affiche la variante reconnue **et sa portée** : 323 portes D exécutées indépendamment, ou 324 portes E du constructeur contre-vérifiées et sondes locales propres. Il ne transfère aucun résultat entre variantes et ne réexécute aucun test. Le code 1 demande une actualisation de sources ; le code 2 rejette un manifeste invalide. La compatibilité `--manifest` conserve l’accès aux anciens formats. Les 30 [tests de ce contrôleur](test_verify_current.py) passent normalement et sous `-O`.

## Entretien du dossier

Une conclusion active a un rapport de référence. Douze notes transitoires ou dépassées ont été fusionnées et retirées ; le [registre de consolidation](receipts_front_20260905/documentation_retirement.json) donne pour chacune son remplacement, le commit historique et son hash. Les fixtures permanentes et reçus bruts, y compris échecs et refus, restent à leur emplacement reproductible. Les répertoires `.work*` sont des temporaires ignorés, pas des autorités publiques.

Les [reçus D/E](receipts_20260905/README.md) conservent leurs attributions historiques. Les [certificats entiers du front](receipts_front_20260905/README.md) sont complétés par le [raccord C++ exécuté](receipts_front_compiled_20260905/README.md). Les contrôles CTest locaux, CI et campagnes du constructeur restent distincts. Les demandes désormais fermées sont retirées des entrées courantes.

Statut public : `not_claimed`. Aucun registre officiel modifié. GCP non utilisé.
