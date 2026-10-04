# Trois vélos (trame 00/001472)

Catégorie : [HGP réussit, HDBSCAN échoue](../README.md). Bout de scène SemanticKITTI, séquence 00, trame 001472, réduit aux seuls points de ses objets : ni sol, ni fond, ni autre objet ; mesures G4 `claudebouts1` (tous les bouts, commit f1a53fe1c) et `claudebouts2` (blocs publiés, commit b72fe8771).

| objet | classe | points |
| --- | --- | --- |
| A | vélo | 82 |
| B | vélo | 139 |
| C | vélo | 161 |

Écarts (plus courte distance entre les points de deux objets) : A–B : 0,36 m, B–C : 0,07 m. Sites au millimètre : 382.

| k | HDBSCAN, meilleur IoU par objet (A / B / C) | HGP, meilleur IoU par objet | IoU moyen HDBSCAN / HGP | issue |
| --- | --- | --- | --- | --- |
| 2 | 0,84 / **0,47** / 0,59 | 0,84 / 0,56 / 0,57 | 0,63 / 0,65 | HGP réussit, HDBSCAN échoue |
| 3 | 0,84 / **0,43** / 0,57 | 0,84 / **0,43** / 0,57 | 0,61 / 0,61 | les deux échouent |
| 5 | 0,84 / **0,41** / 0,57 | 0,84 / 0,72 / 0,60 | 0,61 / 0,72 | HGP réussit, HDBSCAN échoue |
| 10 | 0,84 / **0,43** / 0,57 | 0,84 / 0,76 / 0,57 | 0,61 / 0,72 | HGP réussit, HDBSCAN échoue |

En gras : objet à 0,5 ou moins : aucun groupe de la hiérarchie ne le recouvre à plus de la moitié.

## Images

Vue de dessus, tournée selon l'axe principal. Trois panneaux : vérité (A bleu, B orange, C violet) ; meilleur groupe de HDBSCAN pour l'objet clé ; meilleur groupe de HGP pour le même objet. Vert : point de l'objet dans le groupe ; rouge : point d'un autre objet dans le groupe ; bleu : point de l'objet hors du groupe ; gris : autres points.

k = 2, objet clé B :

![k = 2](k2.png)

k = 5, objet clé B :

![k = 5](k5.png)

k = 10, objet clé B :

![k = 10](k10.png)

## Données

`bout.json` décrit le bout (trame, empreintes sha256 de la trame, des étiquettes et des fichiers du bout, instances) et donne les meilleurs IoU à chaque ordre. Les points ne sont pas versionnés (CC BY-NC-SA) : `data/` est ignoré par git. Pour les refaire à l'identique depuis les archives officielles :

```sh
python3 Zoltan/demos/tools/chercher_bouts.py --cache CACHE --out Zoltan/demos/hgp_reussit_hdbscan_echoue/bout_00_001472_trois_velos_40_42_59 \
    --rebuild Zoltan/demos/hgp_reussit_hdbscan_echoue/bout_00_001472_trois_velos_40_42_59/bout.json
```
