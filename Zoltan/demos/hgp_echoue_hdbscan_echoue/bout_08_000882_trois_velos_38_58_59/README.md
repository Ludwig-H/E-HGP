# Trois vélos (trame 08/000882)

Catégorie : [HGP et HDBSCAN échouent](../README.md). Bout de scène SemanticKITTI, séquence 08, trame 000882, réduit aux seuls points de ses objets : ni sol, ni fond, ni autre objet ; mesures G4 `claudebouts1` (tous les bouts, commit f1a53fe1c) et `claudebouts2` (blocs publiés, commit b72fe8771).

| objet | classe | points |
| --- | --- | --- |
| A | vélo | 52 |
| B | vélo | 98 |
| C | vélo | 147 |

Écarts (plus courte distance entre les points de deux objets) : A–B : 0,09 m, A–C : 0,15 m, B–C : 0,69 m. Sites au millimètre : 297.

| k | HDBSCAN, meilleur IoU par objet (A / B / C) | HGP, meilleur IoU par objet | IoU moyen HDBSCAN / HGP | issue |
| --- | --- | --- | --- | --- |
| 2 | **0,40** / 0,70 / 0,99 | **0,40** / 0,70 / 0,97 | 0,69 / 0,69 | les deux échouent |
| 3 | **0,37** / 0,71 / 0,99 | **0,40** / 0,71 / 0,97 | 0,69 / 0,69 | les deux échouent |
| 5 | **0,45** / 0,72 / 0,97 | **0,40** / 0,70 / 0,97 | 0,71 / 0,69 | les deux échouent |
| 10 | **0,35** / 0,71 / 0,92 | **0,41** / 0,70 / 0,96 | 0,66 / 0,69 | les deux échouent |

En gras : objet à 0,5 ou moins : aucun groupe de la hiérarchie ne le recouvre à plus de la moitié.

## Images

Vue de dessus, tournée selon l'axe principal. Trois panneaux : vérité (A bleu, B orange, C violet) ; meilleur groupe de HDBSCAN pour l'objet clé ; meilleur groupe de HGP pour le même objet. Vert : point de l'objet dans le groupe ; rouge : point d'un autre objet dans le groupe ; bleu : point de l'objet hors du groupe ; gris : autres points.

k = 2, objet clé A :

![k = 2](k2.png)

k = 3, objet clé A :

![k = 3](k3.png)

k = 5, objet clé A :

![k = 5](k5.png)

k = 10, objet clé A :

![k = 10](k10.png)

## Données

`bout.json` décrit le bout (trame, empreintes sha256 de la trame, des étiquettes et des fichiers du bout, instances) et donne les meilleurs IoU à chaque ordre. Les points ne sont pas versionnés (CC BY-NC-SA) : `data/` est ignoré par git. Pour les refaire à l'identique depuis les archives officielles :

```sh
python3 Zoltan/demos/tools/chercher_bouts.py --cache CACHE --out Zoltan/demos/hgp_echoue_hdbscan_echoue/bout_08_000882_trois_velos_38_58_59 \
    --rebuild Zoltan/demos/hgp_echoue_hdbscan_echoue/bout_08_000882_trois_velos_38_58_59/bout.json
```
