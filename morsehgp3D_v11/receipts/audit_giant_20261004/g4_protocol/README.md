# Complément indépendant : protocole G4 v11

Source Git : `0f5e8a207f2974e262cd40a8882b97af1da396af`. Lecture statique de README/controller/worker et des deux gardes start/stop épinglées. Aucun lancement GCP, natif, compilation, fit ou campagne. Aucun fichier produit/audit actif modifié.

Les 13 clauses de [protocol_matrix.json](protocol_matrix.json) couvrent verrou, cible/génération, sources, double garde, échéances, descendants, refus, archives et verdict final. Les cinq sources sont figées avant lecture et recoupées après ; leurs SHA correspondent aux blobs Git. Les empreintes start/stop correspondent aussi aux pins du contrôleur.

Aucun nouveau défaut causal du protocole établi. Le contrôleur enfant détient le verrou avant les opérations cloud ; la génération est recertifiée et la fermeture reste tentée après une récupération échouée. Un arrêt non certifié et une génération étrangère active sont distingués. Le worker publie les refus/censures, puis un manifeste exhaustif et l'archive hachée avant sa sentinelle de fin. Le lecteur recoupe paquet, source, plan, génération et fichiers.

| Limite à conserver | Ancrage |
| --- | --- |
| Le verrou est commun seulement si les acteurs utilisent le même inode. Un autre `sessions-root` nécessite la liaison explicite au verrou canonique. | controller:1359–1377, README:106–115 |
| La sursouscription des timeouts est avertie ; les coupes et omissions restent des tentatives partielles. | controller:1027–1045, worker:193–218 |
| `wall_seconds` se termine avant la fermeture du groupe. Un résidu tué puis fermé n'est pas une preuve d'isolation pendant la mesure. | worker:229–285 |
| `completed` est le verdict de l'exécution gardée du plan ; un collecteur doit qualifier lui-même ses sous-cas, sorties et binaires recompilés. | worker:357–410, controller:1866–1908 |
| Le code 0 du lancement détaché ne suffit pas : attendre `DONE` et le reçu final. | controller:2074–2122, README:25 |

La comparaison de génération et la mutation GCE ne sont pas une transaction atomique ; la garantie repose aussi sur le verrou partagé. Cette limite est explicitement reconnue dans le contrôleur, pas une nouvelle reproduction d'arrêt étranger.

Contrôle autonome de cette capture : `python3 review.py` et `python3 -O review.py`, 87 contrôles d'intégrité/AST/arithmétique, mêmes sorties, code 0. Le lecteur n'importe et n'exécute aucun contrôleur/worker. Premier échec conservé : sa regex initiale rejetait le commentaire de la constante `PACK_RESERVE_SECONDS` ; correction du seul lecteur, sans défaut produit. Les mocks historiques du parent ne sont pas rejoués ni recomptés ici.

Pour `maxRun=4200`, le calcul de politique donne un arrêt invité de 55 minutes, worker à `D−660`, cutoff SSH à `D−360`, réserve d'archivage 120 secondes. Ce sont des réserves, pas des mesures. Le plafond général de l'API est distinct du plafond exact attesté pour une session.

Les 360 coupes récentes de 107 à 17 593 sites restent leur cohorte propre (information de coordination, aucun résultat brut dupliqué ici). Cette capsule n'acquiert ni contrat 100 ms/FULL, ni GPU, ni qualification nouvelle de clustering plat, ni trame LiDAR entière.

Inventaires : [LEDGER.json](LEDGER.json) couvre tous les payloads ; [SHA256SUMS](SHA256SUMS) couvre aussi le ledger et exclut seulement ce `SHA256SUMS` racine. Aucun ancien reçu modifié.
