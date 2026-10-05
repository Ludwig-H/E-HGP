# Session G4 S : cinq lots TSan clos, qualification encore partielle

5 octobre 2026. Cadre : `exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`. La session `v11.20261005.claudequals`, source publiee `d26328fe2216133ba40385a9673325bbdcf03ea7`, est fermee a 18:51 UTC. `DONE=3`, recu `failed_remote`, worker code1. L'arret cible de la generation exacte est certifie ; archive verifiee sans eviction ni troncature.

Les verdicts CTest termines donnent cinq lots TSan u21 entierement conformes : `scale32000_cli` 4/4, `scale32000` 9/9, `lidar_ng00` 17/17, `lidar_ng01` 15/15, `lidar_ng02` 15/15, soit **60 portes**. Les portes points sur les trois trames, normales et sous Python -O, ont chacune un verdict Passed explicite.

Les autres lots restent incomplets ou en echec : ASan/UBSan u24 large 34/55, 21 sans resultat ; ASan rest 42/45, trois refus `input_unreadable` ; TSan rest 28/40, trois echecs et neuf sans resultat. Les coupures sont les echeances globales des lots, apres compilation ; aucun echec geometrique n'est etabli. Les trois echecs dans chaque instrumentation sont `num_roots_cost_uniform_u18_n8000/16000/32000_k5`. La session n'a livre que les six fichiers LiDAR et `manifest.json`, sans les entrees uniformes exigees. ASan conserve les trois raisons IO explicites ; le journal LastTest de TSan a seulement son en-tete apres la coupure, donc cette meme cause y est inferee des fichiers absents et des sondes, pas d'un diagnostic runtime conserve. La preparation suivante doit livrer ces fichiers ; la disponibilite de `data_complet` est verifiee separement par l'auditeur principal.

Cette session selectionne seulement les portes echelle/LiDAR. **Elle ne qualifie pas `num_roots` court ni `points_unit_sort_refusal`**, absents de ses inventaires. Elle ne joue pas les quatre differentiels complets `points_vs_python`, ni le nouveau chemin L2b. Le plan dedie Python epingle a ete adopte localement ; ce n'est pas un resultat de session.

La matrice suivante `8b2ca400ea215ffbf6821fe2a1a73b40d8095b2f` repartit les portes normales en **11 lots** (4 ASan, 7 TSan). Sur chacun des inventaires S effectivement observes de 100 portes, l'union est disjointe et couvre exactement les **68 portes sans suffixe `_opt`**, dont les six portes normales points echelle/LiDAR. Les **32 jumelles `_opt`** sont explicitement exclues du nouveau perimetre. Les planchers de comptes sont satisfaits sur cette projection. Ce decoupage est prepare ; aucun nouveau delai ni succes d'execution n'est prouve.

La preuve conserve seulement les champs utiles du recu et des resultats, les verdicts termines et empreintes. Le rejeu verifie les SHA de l'archive, du paquet et du plan, le raccord recu/worker/source, la fermeture ciblee et sept sources du paquet contre Git. Il rapproche les verdicts Passed/Failed et les absences avec chaque resultat. Il derive ensuite la projection de la nouvelle matrice. Aucun compte cloud, secret ni octet LiDAR n'est copie.

```sh
python3 -B replay.py /workspaces/E-HGP
python3 -B -O replay.py /workspaces/E-HGP
```

Dependances locales : commits Git epingles et session close dans `/workspaces/.ehgp-sessions/v11.20261005.claudequals`. Un autre chemin de session peut etre fourni en deuxieme argument. Capsule non autonome sans cette archive. Aucun build, test natif ni appel GCP par l'auditeur.

Le contrôle local séparé [data_preparation.json](data_preparation.json) confirme les six fichiers synthétiques dans `data_complet` : tailles et SHA256 conformes au manifeste. Le répertoire `data` les omet toujours. Seules ces métadonnées sont conservées, aucun fichier de coordonnées ou d’identifiants.
