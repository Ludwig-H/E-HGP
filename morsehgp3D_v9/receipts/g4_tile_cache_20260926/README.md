# G4 — cache de témoins S2 par tuiles, 26 septembre 2026

**14/14 cas terminés, 42 passages GPU, aucune divergence de masque.**
Code et protocole épinglés à `d0e711e23617a7fa2838cac9b370ff125d4be5fb`.
Trois trames entières sans sol de la séquence 08, grille entière 1 mm.
Le plan sépare K5/K10 et WSPD s8/10/12. Ce n'est pas un reçu FULL.

Sur 08/000000 K5/s8, médianes chaudes : paires GPU
29,0435 → 25,4410 ms ; filtre complet 64,0655 → 60,7645 ms.
Les gains nets du filtre sont d'environ 1–4 ms sur les six configurations.
La [lecture détaillée](../../audits/AUDIT_B_CACHE_S2_G4_20260926.md)
donne les limites, la mémoire et les décisions proposées au développeur.

## Preuves et frontières

- `vm/` : reçus, commandes, codes de sortie, sorties brutes, sources et
  dépendances compilées épinglées. Les octets sont ceux de la capture,
  sans réécriture. `preflight.u32le` est une fixture synthétique, pas KITTI.
- `host/` : identité de génération, commandes et arrêt ciblé certifié,
  journaux de garde explicitement expurgés, relecture GCE après arrêt.
- `PACKAGE.json`, `source_manifest.json`, `plan.json` : reconstruction du
  paquet depuis son commit. Le snapshot contenant des coordonnées LiDAR
  reste hors de la v9 ; aucun fichier de trame n'est dupliqué ici.
- `SUMMARY.json` : tableau recalculé par le lecteur. Chaque passage est
  conservé ; le minimum historique n'est pas le seul temps publié.
- `SHA256SUMS` : inventaire des fichiers publiés, hors cet inventaire lui-même.

La comparaison CPU porte sur chaque masque de paire de la dernière passe ;
l'égalité des masques et compteurs entre répétitions contrôle les autres.
Il y a 291 690 754 masques comparés en cumul sur les 14 processus, avec
répétitions et bras volontairement inclus : ce ne sont pas autant de
paires géométriques distinctes. Le compactage des survivants, le
catalogue, FULL, le brut, plusieurs séquences et les courbes de croissance
CUDA ne sont pas qualifiés par ce reçu.

Les temps du filtre sont des events CUDA avec transferts selon le
périmètre de la sonde. Index, front CPU et juges CPU n'y sont pas inclus.
Les temps mur de processus sont également conservés dans les commandes.

## Relecture

Depuis la racine du dépôt, reconstruire un paquet neuf depuis le commit
épinglé, puis relire sans appel GCP :

```bash
python3 gcp-migration/gpu_filter_snapshot_v9.py \
  --commit d0e711e23617a7fa2838cac9b370ff125d4be5fb \
  --plan morsehgp3D_v9/receipts/g4_tile_cache_20260926/plan.json \
  --output /tmp/mhgp9-tile-relecture-paquet-neuf
python3 -B morsehgp3D_v9/audits/b_g4_tile_cache_20260926/analyze_receipt.py \
  morsehgp3D_v9/receipts/g4_tile_cache_20260926 \
  --snapshot /tmp/mhgp9-tile-relecture-paquet-neuf/snapshot.tar.gz
```

Les scripts du protocole doivent rester identiques aux empreintes de ce
commit ; sinon utiliser un worktree détaché de celui-ci pour reconstruire
le paquet. Les lecteurs normal et `-O` ont donné le même résumé. Depuis
ce dossier, `sha256sum --check SHA256SUMS` contrôle l'archive publiée.
La copie publiée ne prétend pas vérifier les hashes des journaux hôte
originaux expurgés (`shutdown_raw_hashes_verified=false`) ; ils ont été
vérifiés avant publication. Les sorties VM restent intégrales.

## Session et coût borné

Cible : `devpod-gpu-exploration/us-central1-b/ehgp-v7-4fa0e0789a7d5bb06b787d35`,
`g4-standard-48`, SPOT, RTX PRO 6000 Blackwell Server Edition.
Génération `2026-09-26T12:20:06.956-07:00`, dernier arrêt
`2026-09-26T12:24:29.680-07:00`. Arrêt ciblé certifié, puis lecture
GCE indépendante : **TERMINATED, même génération**. Environ 4 min 23 s
entre les horodatages GCE ; ce n'est pas une estimation tarifaire.
Worker complet environ 103,71 s, compilation CUDA 9,58 s, aucune installation.
Une seule session payante. Aucun contrat 100 ms FULL n'est acquis.
