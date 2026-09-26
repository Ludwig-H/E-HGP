# Capture G4 non récupérée — preuve d'échec close

Autorité : `SUMMARY.json` est recalculé depuis les trois reçus hôte,
leurs commandes et les confirmations GCE arrêtées. Aucun probe brut
n'a été récupéré : zéro cas FULL/GPU qualifié, aucun chrono exploitable.
Les 26 cas sont ceux du plan, pas 26 résultats localement validés.

L'échec initial de capture est conservé (`host/`). La récupération R1
a été refusée avant démarrage pour expiration insuffisante de la clé
OS Login (`failed_recovery/`). R2 a été arrêtée après un échec silencieux
du pack (`recovery_allocated/`) ; sa précondition bloquante exacte n'est
pas établie. Aucune disparition de données n'est déduite.

Budget : original 309,542 s + R2 121,113 s = 430,655 s d'allocation GCE ;
R1 ne crée aucune génération. Les deux générations allouées sont certifiées
TERMINATED. Aucun montant facturé n'est estimé.

Relecture hors ligne depuis la racine, normal puis avec `-O` :

```sh
python3 -B morsehgp3D_v9/audits/b_q3_payload_g4_20260926/read_failed.py \
  morsehgp3D_v9/receipts/g4_q3_payload_failed_capture_20260926 \
  --snapshot /workspaces/E-HGP/build/v9-q3-payload-snapshot-f9f273bb0/snapshot.tar.gz
```

Lecteur LIVE : snapshot privé épinglé nécessaire. `SHA256SUMS` couvre
tous les fichiers publiés sauf lui-même. Ni archive, ni nouvelle donnée
KITTI, ni clé SSH, ni réponse OS Login brute n'est publiée. Les reçus
restent inchangés ; seuls les logs sélectionnés et états GCE sont expurgés.
