# Deux vélos (trame 08/002852)

Catégorie : [HGP réussit, HDBSCAN échoue](../README.md). Bout de scène SemanticKITTI, séquence 08, trame 002852, réduit aux seuls points de ses objets : ni sol, ni fond, ni autre objet ; mesures G4 `claudebouts1` (tous les bouts, commit f1a53fe1c) et `claudebouts2` (blocs publiés, commit b72fe8771).

| objet | classe | points |
| --- | --- | --- |
| A | vélo | 157 |
| B | vélo | 122 |

Écarts (plus courte distance entre les points de deux objets) : A–B : 0,05 m. Sites au millimètre : 279.

| k | HDBSCAN, meilleur IoU par objet (A / B) | HGP, meilleur IoU par objet | IoU moyen HDBSCAN / HGP | issue |
| --- | --- | --- | --- | --- |
| 2 | 0,81 / 0,73 | 0,87 / 0,85 | 0,77 / 0,86 | les deux réussissent |
| 3 | 0,80 / 0,68 | 0,86 / 0,84 | 0,74 / 0,85 | les deux réussissent |
| 5 | 0,74 / **0,44** | 0,85 / 0,83 | 0,59 / 0,84 | HGP réussit, HDBSCAN échoue |
| 10 | 0,73 / **0,44** | 0,85 / **0,50** | 0,58 / 0,67 | les deux échouent |

En gras : objet à 0,5 ou moins : aucun groupe de la hiérarchie ne le recouvre à plus de la moitié.

## Images

Vue de dessus, tournée selon l'axe principal. Trois panneaux : vérité (A bleu, B orange, C violet) ; meilleur groupe de HDBSCAN pour l'objet clé ; meilleur groupe de HGP pour le même objet. Vert : point de l'objet dans le groupe ; rouge : point d'un autre objet dans le groupe ; bleu : point de l'objet hors du groupe ; gris : autres points.

k = 5, objet clé B :

![k = 5](k5.png)

## Données

`bout.json` décrit le bout (trame, empreintes sha256 de la trame, des étiquettes et des fichiers du bout, instances) et donne les meilleurs IoU à chaque ordre. Les points ne sont pas versionnés (CC BY-NC-SA) : `data/` est ignoré par git. Pour les refaire à l'identique depuis les archives officielles :

```sh
python3 Zoltan/demos/tools/chercher_bouts.py --cache CACHE --out Zoltan/demos/hgp_reussit_hdbscan_echoue/bout_08_002852_deux_velos_6_51 \
    --rebuild Zoltan/demos/hgp_reussit_hdbscan_echoue/bout_08_002852_deux_velos_6_51/bout.json
```
