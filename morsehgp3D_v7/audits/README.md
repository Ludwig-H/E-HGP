# Audit indépendant v7

Lire l’[état courant](ETAT_COURANT.md), puis le [dialogue actif](DIALOGUE_COURANT.md). Les contrats, décisions d’architecture et résultats déjà repris par le développeur sont dans le [dossier principal](../PASSATION.md) et les [fausses pistes](../docs/FAUSSES_PISTES.md). Ce dossier garde leur contrôle indépendant et les preuves qui lui sont propres.

| Sujet courant | Qualification indépendante |
| --- | --- |
| Modèle FULL | [Décision et domaine](NIVEAUX_ET_CERTIFICAT_HGP_COURANT.md) |
| Cache, lots et normalisation | [Verdict](CACHE_FULL_COURANT.md), [normalisation v2](receipts_full_successor_20260905/README.md), [lot unitaire](receipts_full_singleton_20260905/README.md) |
| EAGER et composant structurel | [Producteur relatif](PRODUCTEUR_FULL_GABRIEL_COURANT.md), [lecteur](CERTIFICAT_FULL_CPP_COURANT.md) |
| Mono : EAGER clos, successeurs et coût MEB | [Bornes, attribution et diagnostic des successeurs](MONO_FULL_COURANT.md) |
| Proposeur MEB privé | [Qualifications et filtres démontrés des pivots](MEB_DOUBLE_BUDGET_COURANT.md) |
| Intégrations différées | [Questions secondaires regroupées](QUESTIONS_SECONDAIRES.md) |

## Preuves conservées

Ces notes portent des arguments encore cités par le dossier principal ; elles ne sont pas des demandes de qualification à recommencer.

- Géométrie : [S1](S1_COURANT.md), [front](FRONT_ET_TEMOINS_COURANT.md), [secteurs/cordes](PREUVE_CHORD_SECTOR_COURANTE.md), [cellules](CELLULES_COURANT.md), [filtres](FILTRES_FLOTTANTS_COURANTS.md), [lanes](ARITHMETIQUE_LANES_COURANTE.md), [fuseaux](ARITHMETIQUE_SPINDLE_COURANTE.md).
- Exécution : [index](AUDIT_INDEX_20260905.md), [domaine CPU](DOMAINE_CPU_COURANT.md), [qualifications D/E/F](AUDIT_QUALIFICATION_20260905.md), [prétest q2 E](ADDENDUM_MEB_Q2_E_20260905.md), [MEB privée à deux budgets](MEB_DOUBLE_BUDGET_COURANT.md).
- Hiérarchie : [certificat réduit E](CERTIFICAT_HORIZONTAL_COURANT.md), [verticales](CONTRAT_VERTICAL_COURANT.md), [incidences pondérées](CONTRAT_MASSES_VOTE_COURANT.md), [comparateur p3](AUTORITE_VOTE_P3_COURANTE.md).
- Reçus anciens sans note active redondante : [D/E](receipts_20260905/README.md), [front compilé](receipts_front_compiled_20260905/README.md), [mono](receipts_20260904/mono_current.json), [census axis](receipts_iteration3/axis_execution.json), [résidence](receipts_20260904/residence_current.json), [composition](receipts_20260904/math_current_repro.json), [frontière de fenêtre](receipts_20260904/math_window_repro.json), [archive](receipts_20260904/archive_delta_current.json).

## Fraîcheur et entretien

```bash
python3 -B -O morsehgp3D_v7/audits/verify_current.py
```

Le [manifeste](validation_current.json) vérifie des sources et preuves épinglées, affiche leur portée et ne réexécute aucun test. Code 0 : une variante entière correspond ; 1 : sources ou documents à actualiser ; 2 : manifeste invalide. Un fichier non épinglé n’est pas qualifié par ce contrôle. Les variantes D à L gardent leurs autorités distinctes.

Le [registre d’entretien](ENTRETIEN.json) donne les notes supprimées, leurs remplacements et leur version Git antérieure. Les sources, reçus scellés, contre-fixtures et échecs restent intacts. Les questions sans incidence immédiate sont raccourcies dans un seul fichier. Un nouvel audit ou push doit apporter une décision, une preuve, une correction ou un entretien utile ; aucune publication de routine sans contenu pertinent.

Écritures uniquement ici, sur `main`. `public_status=not_claimed`. GCP non utilisé.
