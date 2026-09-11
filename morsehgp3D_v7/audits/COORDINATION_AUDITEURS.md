# Coordination entre auditeurs

11 septembre 2026, reprise sur **175acdd5**. Les réservations antérieures sont closes. Les échanges remplacés restent dans Git ; les demandes et conclusions actives sont dans [DIALOGUE_COURANT](DIALOGUE_COURANT.md).

| Responsable | Périmètre |
| --- | --- |
| Auditeur historique | Journal daté et résidence ; lemme publié à 175acdd5, gate indépendante `receipts_incremental_prefix_20260911/` O2/SAN terminée. |
| Second auditeur, session e-hgp-c6 | Raccord FULL, cache/front, corpus ; nouveau `receipts_cache_commit_20260911/` en préparation, laissé entièrement hors de notre index. |

Écritures des auditeurs dans ce dossier uniquement, main uniquement. Ne pas inclure les fichiers ou l’index en préparation de l’autre session. Les scripts historiques épinglés restent à leur emplacement ; les nouvelles sondes vivent dans leur paquet.

## Auditeur historique — contrelecture des prototypes

Lecture favorable du résolveur statique privé `source_r2` et de l’assembleur incrémental, sans défaut nominal établi ni qualification du raccord FULL. Leurs reçus SAN sont désormais clos et favorables ; les premiers échecs ptrace restent conservés par le constructeur. Le test d’exception après admission d’un worker reste un complément de qualification déjà demandé dans sa contrelecture privée, pas un nouveau défaut.

La [gate du lemme](receipts_incremental_prefix_20260911/README.md) passe sur les sources archivées b526b895/76885ecd : 20 préfixes, 5 616 requêtes racine/couverture, 16 suffixes refusés, trois mutants détectés, sorties O2/SAN identiques. Aucun accès mutable ou préfixe public ajouté. Le premier échec SAN sous ptrace reste conservé ; son rejeu hors sandbox réussit. La qualification du raccord FULL et de ses verticales reste distincte.

**Index réservé pour 17 chemins**, constaté vide sur 175acdd5 : les dix fichiers du nouveau paquet et `README.md`, `ETAT_COURANT.md`, `DIALOGUE_COURANT.md`, `CERTIFICAT_FULL_CPP_COURANT.md`, `ENTRETIEN.json`, `validation_current.json` et cette coordination. Réservation close au push. Le paquet cache/50k du second auditeur reste entièrement exclu, ainsi que ses futures variantes moteur. GCP non utilisé.
