# FULL G4 — intérieurs q3 transportés jusqu'au catalogue

Autorité : probes bruts dans `vm/`, reçus hôte fermés dans `host/`,
`SUMMARY.json` entièrement recalculé par le lecteur. Sources : `f9f273bb0a3e9c0c4d15b531d28efcfe9b82451b`.
Capture : `completed`, 26 cas rejugés : 18 GPU et 8 engine CPU ; 19 comparaisons exactes et 9 paires ON/OFF.

Profil grille entière **1 mm/u18**, toute la tour **K=1..Kmax explicite**.
00/01/02 désignent 08/000000, 08/000100 et 08/000200 sans sol :
39 885 / 35 551 / 45 845 sites après les masques figés. b00 est la
trame brute entière 08/000000, 123 389 sites. Ces trames appartiennent
toutes à la même séquence 08.

Les paires ON/OFF gardent trois digests et le travail producteur identiques.
Les temps ci-dessous couvrent la chaîne vers la tour explicite sur entrée
préparée en mémoire ; lecture fichier, segmentation et digests sont séparés.
Tous les processus ont `frames=1` : aucune mesure chaude. Les valeurs sont
les médianes de deux processus par bras pour 00/K5/s8 seulement, et la
mesure unique par bras ailleurs ; ce ne sont pas les premiers essais chronologiques.

| Scène | K | s | Médiane OFF (ms) | Médiane ON (ms) |
|---|---:|---:|---:|---:|
| 00 | 5 | 8 | 927.789 | 922.663 |
| 00 | 10 | 8 | 2955.836 | 2950.635 |
| 01 | 5 | 8 | 767.701 | 749.028 |
| 02 | 5 | 8 | 964.710 | 969.243 |
| 00 | 5 | 10 | 975.267 | 950.940 |
| 00 | 5 | 12 | 973.686 | 965.854 |
| b00 | 5 | 8 | 1964.634 | 1942.923 |
| b00 | 10 | 8 | 6047.587 | 5938.304 |

Sur 00/K5/s8, les deux deltas appariés ON−OFF sont −11,474 ms puis
+1,222 ms ; sur 02, +4,533 ms. Le census médian 00 diminue de 102,5415
à 82,8245 ms, mais le gain FULL n'est pas stable. Le défaut reste **OFF**.

Ces observations ne qualifient ni les 100 ms, ni plusieurs séquences,
ni une borne sous-quadratique globale. Le masque sans-sol figé ne remplace
pas le contrat sur la trame brute. Les résultats des échecs antérieurs
restent distincts dans `../g4_q3_payload_failed_capture_20260926/`.

Allocation GCE de cette capture : 306.768 s ; avec les échecs liés : **737,423 s**. Aucun montant facturé estimé.
Génération `2026-09-26T13:54:14.979-07:00` ; arrêt `2026-09-26T13:59:21.747-07:00`.
Le contrôle GCE après arrêt confirme `TERMINATED` sur cette même génération.

Depuis la racine, lire normalement puis ajouter `-O` :

```sh
python3 -B morsehgp3D_v9/audits/b_q3_payload_g4_20260926/readback.py \
  morsehgp3D_v9/receipts/g4_q3_payload_20260926 \
  --snapshot /workspaces/E-HGP/build/v9-q3-payload-snapshot-f9f273bb0/snapshot.tar.gz
```

Le snapshot privé et le protocole épinglé sont nécessaires à cette relecture LIVE.
`SHA256SUMS` couvre les fichiers publiés sauf lui-même. Aucun snapshot,
nouveau nuage KITTI, archive ou clé SSH n'est publié.
