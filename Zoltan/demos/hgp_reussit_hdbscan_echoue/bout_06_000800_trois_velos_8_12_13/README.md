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
