# Trois voitures (trame 08/002560)

Catégorie : [HGP et HDBSCAN réussissent](../README.md). Bout de scène SemanticKITTI, séquence 08, trame 002560, réduit aux seuls points de ses objets : ni sol, ni fond, ni autre objet ; mesures G4 `claudebouts1` (tous les bouts, commit f1a53fe1c) et `claudebouts2` (blocs publiés, commit b72fe8771).

| objet | classe | points |
| --- | --- | --- |
| A | voiture | 2969 |
| B | voiture | 3427 |
| C | voiture | 2491 |

Écarts (plus courte distance entre les points de deux objets) : A–B : 0,92 m, B–C : 0,57 m. Sites au millimètre : 8887.

| k | HDBSCAN, meilleur IoU par objet (A / B / C) | HGP, meilleur IoU par objet | IoU moyen HDBSCAN / HGP | issue |
| --- | --- | --- | --- | --- |
| 2 | 1,00 / 0,99 / 1,00 | 1,00 / 0,99 / 1,00 | 1,00 / 1,00 | les deux réussissent |
| 3 | 1,00 / 0,99 / 1,00 | 1,00 / 0,99 / 1,00 | 1,00 / 1,00 | les deux réussissent |
| 5 | 1,00 / 0,99 / 1,00 | 1,00 / 0,99 / 1,00 | 1,00 / 1,00 | les deux réussissent |
| 10 | 1,00 / 0,99 / 1,00 | 1,00 / 0,99 / 1,00 | 1,00 / 1,00 | les deux réussissent |

En gras : objet à 0,5 ou moins : aucun groupe de la hiérarchie ne le recouvre à plus de la moitié.

## Images

Vue de dessus, tournée selon l'axe principal. Trois panneaux : vérité (A bleu, B orange, C violet) ; meilleur groupe de HDBSCAN pour l'objet clé ; meilleur groupe de HGP pour le même objet. Vert : point de l'objet dans le groupe ; rouge : point d'un autre objet dans le groupe ; bleu : point de l'objet hors du groupe ; gris : autres points.

k = 2, objet clé B :

![k = 2](k2.png)

k = 3, objet clé B :

![k = 3](k3.png)

k = 5, objet clé B :

![k = 5](k5.png)

k = 10, objet clé B :

![k = 10](k10.png)

## Données

`bout.json` décrit le bout (trame, empreintes sha256 de la trame, des étiquettes et des fichiers du bout, instances) et donne les meilleurs IoU à chaque ordre. Les points ne sont pas versionnés (CC BY-NC-SA) : `data/` est ignoré par git. Pour les refaire à l'identique depuis les archives officielles :

```sh
python3 Zoltan/demos/tools/chercher_bouts.py --cache CACHE --out Zoltan/demos/hgp_reussit_hdbscan_reussit/bout_08_002560_trois_voitures_169_170_171 \
    --rebuild Zoltan/demos/hgp_reussit_hdbscan_reussit/bout_08_002560_trois_voitures_169_170_171/bout.json
```
