# Reçu : croissance LiDAR locale, trois trames, K5 et K10 (runner v2)

23 septembre 2026, 07:3x–08:03 UTC. **GCP non utilisé.** Cadre :
`exploration_v9_hors_registre`, `backend=reference_cpu`,
`profile=quantized_u18_input_only`, `public_status=not_claimed`. Hôte local
partagé à huit cœurs, W8, s = 8, leviers par défaut (tous ON). Charge moyenne
de l'hôte 9 à 13 pendant la campagne (lecteurs d'une revue multi-agents en
parallèle) : les **temps** sont indicatifs, les **compteurs** sont
déterministes et font foi pour les exposants.

Binaire `mhgp9_tower_probe` construit depuis `4530644b` (sonde v12, arbres
`src` `ceba3397` et `bench` `c2c617e0`, worktree `src`/`bench` propre),
empreinte `e1ba126f…` dans `probe_binary.sha256` ; une copie du binaire est
conservée hors dépôt (`build/v9-scaling/mhgp9_tower_probe.e1ba126f`) pour un
rejeu. Runner : `morsehgp3D_v9/bench/run_lidar_scaling.py` v2
(`mhgp9_lidar_scaling_v2`) : sous-nuages **emboîtés** 8 000 ⊂ 16 000 ⊂ 32 000
sites par distance horizontale au centre médian (emboîtement vérifié, IDs de
sites épinglés), puis la trame entière et six morceaux spatiaux du reçu v8
`lidar_ground_20260921`, vérifiés contre son `MANIFEST.json`. Chaque sortie
de sonde est validée (schéma v12, `complete_relative`, `K_effective`, ordres
1..K, options et leviers). Aucun octet KITTI n'est versionné : chaque JSON
porte les SHA-256 de ses points et de ses IDs.

Commande (depuis la racine du worktree) :

```text
for k in 5 10; do for sc in 01 00 02; do
  python3 morsehgp3D_v9/bench/run_lidar_scaling.py --probe build/v9-dev/mhgp9_tower_probe \
    --scene $sc --k $k --workers 8 --repeat 0 --work build/v9-scaling/work \
    --out build/v9-scaling/s${sc}_k${k}_w8_r0
done; done
```

Six campagnes, 66 cas, **aucun échec**, tous `complete_relative` (relatif au
catalogue recoupé : pas une preuve de complétude géométrique des clés absentes).

## Exposants de croissance emboîtés

`p = log₂(m(2n)/m(n))` sur 8k→16k puis 16k→32k d'**une seule géométrie**
emboîtée par trame : deux doublements, pas une borne asymptotique. Tableau
complet (valeurs, exposants, trame entière) dans `SLOPES.json`.

| cas | chaîne 8k/16k/32k/entier (s) | p chaîne | p CPU | p paires dév. | p visites témoins paire | p `core_sites` | p tests uniformes cœur | p `cover_sites` | p atlas | p boules |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| s00 K5 (39885) | 5,60 / 12,41 / 23,92 / 25,61 | 1,15 / 0,95 | 1,22 / 0,83 | 1,81 / 0,85 | 1,64 / 0,73 | 2,94 / 0,44 | 2,41 / 1,16 | 1,38 / 0,97 | 1,36 / 1,15 | 0,90 / 0,80 |
| s01 K5 (35551) | 5,79 / 11,41 / 18,89 / 17,39 | 0,98 / 0,73 | 1,15 / 0,77 | 0,81 / 0,42 | 1,28 / 0,46 | 0,83 / 0,22 | 0,92 / 0,38 | 1,23 / 0,50 | 1,10 / 0,78 | 0,99 / 0,96 |
| s02 K5 (45845) | 3,59 / 7,32 / 20,57 / 28,41 | 1,03 / 1,49 | 1,19 / 1,52 | 1,27 / 2,27 | 1,26 / 2,12 | 1,44 / 3,05 | 1,55 / 2,41 | 1,38 / 1,93 | 1,54 / 1,57 | 0,94 / 0,90 |
| s00 K10 (39885) | 18,81 / 42,03 / 75,16 / 89,89 | 1,16 / 0,84 | 1,12 / 0,82 | 1,68 / 0,87 | 1,46 / 0,77 | 2,51 / 0,55 | 2,13 / 1,00 | 1,31 / 0,92 | 1,31 / 1,06 | 0,85 / 0,75 |
| s01 K10 (35551) | 14,97 / 30,96 / 53,85 / 66,17 | 1,05 / 0,80 | 1,09 / 0,82 | 0,78 / 0,52 | 1,10 / 0,52 | 0,83 / 0,26 | 0,93 / 0,41 | 1,21 / 0,52 | 1,07 / 0,75 | 0,99 / 1,03 |
| s02 K10 (45845) | 12,64 / 27,34 / 71,36 / 94,33 | 1,11 / 1,38 | 1,18 / 1,42 | 1,14 / 2,09 | 1,26 / 1,91 | 1,27 / 2,86 | 1,38 / 2,30 | 1,40 / 1,90 | 1,46 / 1,57 | 0,92 / 0,89 |

## Lecture

- Les boules du catalogue restent sous-linéaires (p = 0,75 à 1,03) et le temps
  de chaîne croît de ×1,7 à ×2,8 par doublement : aucun temps quadratique sur
  ces six cas.
- Le **cœur diamétral** porte le seul signal superquadratique, reproduit sur
  deux trames et deux K : `core_sites` p = 2,94 (s00 K5, 8k→16k), 2,51 (s00
  K10), 3,05 (s02 K5, 16k→32k), 2,86 (s02 K10). Le doublement fautif change
  avec la trame. Les sites par cœur passent de 81 à 266 (s00 K5) et de 72 à
  191 (s02 K5) : le disque agrandi absorbe une structure où des paires
  longues survivent au filtre de témoins avec des boules diamétrales très
  peuplées. Les paires développées et les visites témoins par paire suivent
  le même doublement (p ≈ 1,7 à 2,3).
- Les parcours cachés publiés par la sonde v12 (témoins rectangle, graines
  q3, domaine et balayage q4) restent entre p = 0,68 et 1,32 sur les six
  cas, sauf l'atlas (p ≤ 1,57).
- Priorité qui en découle : remplacer l'énumération des sites du cœur par un
  certificat sur les nœuds de l'index (compter sans énumérer), puis réduire
  les paires longues développées avant le cœur. Diagnostic de croissance
  seulement : pas un contrat, pas une mesure G4, pas une qualification
  sous-quadratique.

## Contenu

`out/<cas>/` : résumé `SUMMARY_*.json` (lignes, pentes, provenance) et les
onze JSON de sonde de chaque campagne ; `campaign.log` (une ligne de pentes
par campagne, marqueur `CAMPAIGN_DONE`) ; `SLOPES.json` (dérivé des
résumés) ; `probe_binary.sha256` ; `SHA256SUMS`.
