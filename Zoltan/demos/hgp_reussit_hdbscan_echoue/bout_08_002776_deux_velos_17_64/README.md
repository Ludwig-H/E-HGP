# Deux vélos (trame 08/002776)

Catégorie : [HGP réussit, HDBSCAN échoue](../README.md). Bout de scène SemanticKITTI, séquence 08, trame 002776, réduit aux seuls points de ses objets : ni sol, ni fond, ni autre objet ; mesures G4 `claudebouts1` (tous les bouts, commit f1a53fe1c) et `claudebouts2` (blocs publiés, commit b72fe8771).

| objet | classe | points |
| --- | --- | --- |
| A | vélo | 198 |
| B | vélo | 95 |

Écarts (plus courte distance entre les points de deux objets) : A–B : 0,13 m. Sites au millimètre : 293.

| k | HDBSCAN, meilleur IoU par objet (A / B) | HGP, meilleur IoU par objet | IoU moyen HDBSCAN / HGP | issue |
| --- | --- | --- | --- | --- |
| 2 | 0,75 / **0,42** | 0,69 / **0,49** | 0,59 / 0,59 | les deux échouent |
| 3 | 0,75 / **0,41** | 0,69 / 0,51 | 0,58 / 0,60 | HGP réussit, HDBSCAN échoue |
| 5 | 0,75 / **0,36** | 0,70 / 0,53 | 0,56 / 0,61 | HGP réussit, HDBSCAN échoue |
| 10 | 0,70 / **0,45** | 0,70 / **0,49** | 0,57 / 0,59 | les deux échouent |

En gras : objet à 0,5 ou moins : aucun groupe de la hiérarchie ne le recouvre à plus de la moitié.

## Images

Vue de dessus, tournée selon l'axe principal. Trois panneaux : vérité (A bleu, B orange, C violet) ; meilleur groupe de HDBSCAN pour l'objet clé ; meilleur groupe de HGP pour le même objet. Vert : point de l'objet dans le groupe ; rouge : point d'un autre objet dans le groupe ; bleu : point de l'objet hors du groupe ; gris : autres points.

k = 3, objet clé B :

![k = 3](k3.png)

k = 5, objet clé B :

![k = 5](k5.png)

## Données

`bout.json` décrit le bout (trame, empreintes sha256 de la trame, des étiquettes et des fichiers du bout, instances) et donne les meilleurs IoU à chaque ordre. Les points ne sont pas versionnés (CC BY-NC-SA) : `data/` est ignoré par git. Pour les refaire à l'identique depuis les archives officielles :

```sh
python3 Zoltan/demos/tools/chercher_bouts.py --cache CACHE --out Zoltan/demos/hgp_reussit_hdbscan_echoue/bout_08_002776_deux_velos_17_64 \
    --rebuild Zoltan/demos/hgp_reussit_hdbscan_echoue/bout_08_002776_deux_velos_17_64/bout.json
```
