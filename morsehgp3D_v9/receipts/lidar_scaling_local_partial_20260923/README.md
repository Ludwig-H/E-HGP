# Reçu partiel : pente LiDAR locale (interrompu)

23 septembre 2026. **GCP non utilisé.** Hôte local partagé à huit cœurs, W8,
s = 8, leviers par défaut. Binaire `mhgp9_tower_probe` construit depuis
`28f0c284` (empreinte dans `probe_binary.sha256`). Script :
`morsehgp3D_v9/bench/run_lidar_scaling.py` (sous-nuages emboîtés de 8 000,
16 000 et 32 000 sites par distance horizontale au centre médian, puis les
sept morceaux figés du reçu v8 `lidar_ground_20260921`). Les nuages dérivés
ne sont pas versionnés ; chaque JSON porte l'empreinte SHA-256 de son entrée.

**Mesure interrompue** par l'arrêt du codespace : 08/000000 à K5 complet (dix
cas, `SUMMARY_s00_k5.json`), K10 limité aux emboîtés 8 000 et 16 000. Aucune
pente K10, aucune autre trame. Diagnostic de croissance seulement, pas un
contrat ni une mesure G4 ; à reprendre en entier avant toute conclusion.
