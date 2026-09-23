# Contre-audit B — prévol de la session G4 R4

23 septembre 2026, lecture **seule** de la session R4 déjà lancée par le
développeur, dans `/workspaces/.ehgp-sessions/v9tower.20260923T024248Z`.
L'auditeur n'a ni lancé de commande GCP ni modifié la session. Au moment
du prévol, son état local est `targeted_running` : **aucun résultat de
cas ni reçu final R4 n'est encore disponible**. Ne pas transférer les
temps R3 à ce snapshot.

Le paquet annonce le commit produit `a1d7a9bc379f914daa64a1ccc11d54f86060a25d`,
arbre `8026450d…`, `protocol_source=commit` ; l'arbre et les hashes
archive/manifeste concordent avec Git et la copie hôte. Les octets du
cache `q34_witness_search.cpp` et de `tower_probe.cpp` concordent aussi
entre commit, manifeste et archive. Le snapshot contient donc le
correctif public du cache et la sonde v6, **pas** un worktree sale.

Le plan v4 contient 13 cas sur les trois trames entières sans sol, grille
isotrope 1 mm, de la séquence SemanticKITTI 08 : 39 885, 35 551 et
45 845 sites. Pour chaque trame et `K=5/10`, il prévoit une paire
adjacente cache ON/OFF à `s=8`, `W=48`, tour statique 48 ; atlas saturant,
census q3 sur feuille et voies mortes restent ON dans les deux cas.
Un treizième cas répète 08/000000/K10/cache ON. La tour FULL est
demandée. C'est bien une **ablation isolée du cache dans le plan**,
pas encore une comparaison de résultats ou de temps.

La garde avant upload est marquée vérifiée ; `guarded_start` a terminé
code 0, le plan plafonne chaque cas à 600 s, le travail utile à 1 500 s,
avec arrêt invité à 40 min et durée GCP maximale de 60 min. Vérifier
ensuite les deux preuves d'arrêt et le reçu final. Le snapshot contient
le correctif v6 des principaux défauts de réception ; sa
[lacune résiduelle sur certains champs de garde archivés](CONTRE_AUDIT_B_G4_RECEPTION_V5_20260923.md)
reste distincte des gardes externes.

À réception, contrôler sans extrapolation : 13 statuts individuels,
préflight non vacant, hashes et commandes, `input`, générateur,
catalogue, ordres, digest, masses de `cover_builds`, `cover_sites` et
`dead_form_sites` **identiques par paire**, recherches et nœuds du cache,
CPU/mur/RSS, puis arrêt ciblé `TERMINATED`. Une paire locale sur une
séquence ne prouve ni une borne sous-quadratique 8k/16k/32k, ni la
cible de 1 s, ni une exécution GPU, ni le contrat sur trames brutes.

## Issue observée : capture sans mesure

Le reçu hôte local a finalement `status=capture_failed`,
`FULL_executed=false`, `worker_exit_code=255` et
`capture_error="ValueError: fixed G4 target/status"`. La commande SSH
du worker a expiré sur le port 22 **avant tout reçu invité ou toute
sonde**. Le contrôle hôte suivant a trouvé la VM `STOPPING`, puis
`guarded_stop` code 0 a certifié `TERMINATED` et aucune autre VM
`project=e-hgp` active. Le reçu local conserve `lifecycle.txt` à
`targeted_running` : ce texte n'est pas la preuve d'arrêt, qui est
séparée. Le descriptif capturé indique `provisioningModel=SPOT`,
`automaticRestart=false`, action `STOP` ; l'arrêt précoce est
**compatible** avec une interruption SPOT, mais ces traces ne prouvent
pas sa cause exacte.

La tentative n'a donc produit **aucun résultat R4** de cache ON/OFF,
aucun temps de tour et aucun test du contrat. Le snapshot et le plan
préparatoires restent contrôlés ; ne pas transformer cet échec de
connexion en régression ou en succès de l'algorithme. Aucune relance
GCP par l'auditeur.
