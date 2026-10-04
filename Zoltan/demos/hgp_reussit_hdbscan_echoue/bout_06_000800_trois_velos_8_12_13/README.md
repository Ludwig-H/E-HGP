# Trois vélos (trame 06/000800)

Catégorie : [HGP réussit, HDBSCAN échoue](../README.md). Bout de scène SemanticKITTI, séquence 06, trame 000800, réduit aux seuls points de ses objets : ni sol, ni fond, ni autre objet ; mesures G4 `claudebouts1` (tous les bouts, commit f1a53fe1c) et `claudebouts2` (blocs publiés, commit b72fe8771).

| objet | classe | points |
| --- | --- | --- |
| A | vélo | 86 |
| B | vélo | 101 |
| C | vélo | 77 |

Écarts (plus courte distance entre les points de deux objets) : A–B : 0,04 m, B–C : 0,40 m. Sites au millimètre : 264.

| k | HDBSCAN, meilleur IoU par objet (A / B / C) | HGP, meilleur IoU par objet | IoU moyen HDBSCAN / HGP | issue |
| --- | --- | --- | --- | --- |
| 2 | **0,46** / 0,70 / 1,00 | 0,64 / 0,63 / 1,00 | 0,72 / 0,76 | HGP réussit, HDBSCAN échoue |
| 3 | **0,46** / 0,69 / 1,00 | 0,64 / 0,58 / 1,00 | 0,72 / 0,74 | HGP réussit, HDBSCAN échoue |
| 5 | **0,46** / 0,56 / 1,00 | 0,64 / 0,69 / 1,00 | 0,67 / 0,78 | HGP réussit, HDBSCAN échoue |
| 10 | 0,50 / 0,52 / 0,87 | 0,64 / 0,70 / 1,00 | 0,63 / 0,78 | les deux réussissent |

En gras : objet à 0,5 ou moins : aucun groupe de la hiérarchie ne le recouvre à plus de la moitié.

<!-- video:début -->
## Vidéo : HGP contre HDBSCAN, k = 5

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="bout_06_000800_trois_velos_8_12_13_hgp_hdbscan_k5_sombre_instant_cle.png">
  <img alt="Instant clé, k = 5, r = 16,0 cm : HGP, A, B et C retrouvés, encore séparés ; HDBSCAN, A et B déjà réunis" src="bout_06_000800_trois_velos_8_12_13_hgp_hdbscan_k5_clair_instant_cle.png">
</picture>

Vidéo de 48 s, 1920 × 1080 : [thème sombre](bout_06_000800_trois_velos_8_12_13_hgp_hdbscan_k5_sombre.mp4) · [thème clair](bout_06_000800_trois_velos_8_12_13_hgp_hdbscan_k5_clair.mp4) ; image finale : [sombre](bout_06_000800_trois_velos_8_12_13_hgp_hdbscan_k5_sombre_bilan.png) · [clair](bout_06_000800_trois_velos_8_12_13_hgp_hdbscan_k5_clair_bilan.png).

Mêmes 264 points, même ordre k = 5 : à gauche la hiérarchie de points HGP de `morsehgp3D_v11` (Hʳₖ₊₁), à droite l'arbre de HDBSCAN (scikit-learn 1.7.2, `min_samples` = 5). Le niveau r croît pour les deux à la fois et s'arrête à chaque événement des groupes qui suivent les objets (mêmes textes que les bandeaux de la vidéo) :

| r | HGP | HDBSCAN |
| --- | --- | --- |
| 8,7 cm |  | ✓ C retrouvé · IoU 0,65 |
| 9,9 cm |  | ✓ B retrouvé · IoU 0,505 |
| 10,7 cm | A et B encore séparés | ✗ A et B réunis : A jamais retrouvé |
| 13,7 cm | ✓ C retrouvé · IoU 0,73 |  |
| 13,8 cm | ✓ B retrouvé · IoU 0,505 |  |
| 16,0 cm | ✓ A, B et C retrouvés, encore séparés | ✗ A et B déjà réunis |
| 18,9 cm | ✓ A et B réunis, chacun retrouvé avant |  |
| 19,8 cm |  | ✗ A, B et C réunis : A jamais retrouvé |
| 25,0 cm | ✓ A, B et C réunis, chacun retrouvé avant |  |

Meilleur IoU de chaque objet : HGP A 0,64, B 0,69, C 1,00 ; HDBSCAN A 0,46, B 0,56, C 1,00. Légende, convention de niveau et contrôle des calculs : [README de `demos/`](../../README.md#vidéos-hgp-contre-hdbscan-des-bouts) ; nombres : [`resultats_duel_k5.json`](resultats_duel_k5.json).

<!-- video:fin -->

## Images

Vue de dessus, tournée selon l'axe principal. Trois panneaux : vérité (A bleu, B orange, C violet) ; meilleur groupe de HDBSCAN pour l'objet clé ; meilleur groupe de HGP pour le même objet. Vert : point de l'objet dans le groupe ; rouge : point d'un autre objet dans le groupe ; bleu : point de l'objet hors du groupe ; gris : autres points.

k = 2, objet clé A :

![k = 2](k2.png)

k = 3, objet clé A :

![k = 3](k3.png)

k = 5, objet clé A :

![k = 5](k5.png)

## Données

`bout.json` décrit le bout (trame, empreintes sha256 de la trame, des étiquettes et des fichiers du bout, instances) et donne les meilleurs IoU à chaque ordre. Les points ne sont pas versionnés (CC BY-NC-SA) : `data/` est ignoré par git. Pour les refaire à l'identique depuis les archives officielles :

```sh
python3 Zoltan/demos/tools/chercher_bouts.py --cache CACHE --out Zoltan/demos/hgp_reussit_hdbscan_echoue/bout_06_000800_trois_velos_8_12_13 \
    --rebuild Zoltan/demos/hgp_reussit_hdbscan_echoue/bout_06_000800_trois_velos_8_12_13/bout.json
```

<!-- plat:debut -->

## Sortie plate (clusters)

mcs = 20, racine exclue, aucune complétion. Panneaux, de gauche à droite puis de haut en bas : vérité ; HDBSCAN (`sklearn` tel quel) ; HGP, EOM z = 1 ; HGP, EOM z = 2 ; HGP, feuilles. Un cluster apparié à un objet suivi (IoU > 1/2) prend la couleur de l'objet ; les autres clusters ont des couleurs pâles ; le bruit est gris clair.

![Sortie plate à k = 5](plat_k5.png)

| Sortie (k = 5) | Clusters | Objets retrouvés | Objets fusionnés |
| --- | --- | --- | --- |
| HDBSCAN (`sklearn` tel quel) | 2 | 1 / 3 | 2 |
| HGP, EOM z = 1 | 4 | 3 / 3 | 0 |
| HGP, EOM z = 2 | 4 | 3 / 3 | 0 |
| HGP, feuilles | 5 | 2 / 3 | 0 |

Données : arbres exportés par `morsehgp3D_v11/bench/points_flat_dump.py` (session G4 `claudeflat0`), tête certifiée `points_flat.py` ; outil : `tools/rendre_plat.py` ; décision : `morsehgp3D_v11/docs/SORTIE_PLATE.md`.

<!-- plat:fin -->
