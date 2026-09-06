# G4 SPOT : essais FULL à 50 000 points, refus avant les ordres FULL

6 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

La session réelle a utilisé la cible SPOT `devpod-gpu-exploration/us-central1-b/ehgp-v7-4fa0e0789a7d5bb06b787d35`, type `g4-standard-48`, avec 48 vCPU disponibles. Le pipeline demandait 48 threads ; les constructeurs FULL et les ordres K restaient séquentiels. Aucun calcul GPU FULL n'a été exécuté.

Le smoke à 8 points termine ses huit ordres effectifs. Les deux essais à **50 000 points sont de vrais processus distincts**, uniforme / graine 3 / coordonnées u16 / WSPD s=8 / lazy C=1 000 000 / P illimité :

| Tour demandée | Code du processus | Ordres FULL calculés | Refus de la sonde | Boules signalées |
| --- | ---: | ---: | --- | ---: |
| K=1..10 | 2 | 0 | `probe_rank_relevant_extra_shell` | 4 |
| K=1..5 | 2 | 0 | `probe_rank_relevant_extra_shell` | 3 |

Les deux terminaux portent `unsupported_degeneracy`, au stade `regularity`. Le contrôle global de coquille de la sonde a refusé ces entrées avant toute construction FULL. Cette observation ne démontre pas l'impossibilité mathématique d'une hiérarchie sur ces points ; la nécessité et la portée de ce contrôle sont une question distincte d'audit. Aucun refus n'est réétiqueté en succès ou en délai censuré.

Les bruts conservent environ 21,309 s et 5,597 s jusqu'au refus dans le chronomètre interne de la sonde, ainsi que les mesures GNU time et RSS. **Ce ne sont pas des latences de tours FULL terminées.** Les contrats 50k / 1 s et 100 ms ne sont pas qualifiés, et ces captures ne portent pas sur les dizaines de millions de points.

Les [reçus invité](guest/receipt.json) et [hôte](host/receipt.json) conservent respectivement `failed` et `worker_failed`. La collecte des bruts a réussi. L'arrêt ciblé par `stop_and_verify.sh` est clos avec code 0, pour la génération exacte `2026-09-06T06:19:11.593-07:00` ; son [stdout](host/guarded_stop.stdout) certifie `TERMINATED` et ne signale aucune autre VM active. La [relecture GCP filtrée](closure_readonly.json) confirme cette même génération et `TERMINATED`. Les horodatages conservés ne sont pas une mesure de facturation.

Le manifeste référence les 42 dépendances v7 réellement compilées, avec leurs octets déjà scellés dans les paquets voisins ; aucune source n'est recopiée. [publication.json](publication.json) épingle les scripts worker / contrôleur / gardes au commit du [contexte de lancement](launch_context.json). Les chemins absolus dans les captures sont historiques, pas des chemins de rejeu imposés. Le fichier `lifecycle.txt` garde son état historique `targeted_running` : le contrôleur n'a pas réécrit le témoin du start ; la preuve finale d'arrêt est le reçu hôte, le stop ciblé et la relecture filtrée.

Sont volontairement exclus : toute clé privée ou publique, le profil OS Login, les sorties complètes des descriptions GCE, les ELF, les archives tar et les snapshots dupliqués. Leurs éventuelles empreintes et les chemins des commandes restent dans le reçu hôte original, sans publier leur contenu. Les stdout/stderr des gardes demandés sont conservés tels quels.

Depuis ce paquet, avec les paquets voisins référencés : `python3 verify.py .` puis `python3 -O verify.py .` et `sha256sum -c SHA256SUMS`. Ce lecteur portable vérifie les octets, les sources référencées, les codes, les refus avant FULL et la fermeture ciblée déjà capturée. Il n'exécute aucun moteur, aucune commande GCP, aucun test de performance, et ne transforme pas les sorties en certificat géométrique.
