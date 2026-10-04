# Trois vélos (trame 08/001182)

Catégorie : [HGP réussit, HDBSCAN échoue](../README.md). Bout de scène SemanticKITTI, séquence 08, trame 001182, réduit aux seuls points de ses objets : ni sol, ni fond, ni autre objet ; mesures G4 `claudebouts1` (tous les bouts, commit f1a53fe1c) et `claudebouts2` (blocs publiés, commit b72fe8771).

| objet | classe | points |
| --- | --- | --- |
| A | vélo | 73 |
| B | vélo | 132 |
| C | vélo | 444 |

Écarts (plus courte distance entre les points de deux objets) : A–B : 0,09 m, A–C : 0,74 m, B–C : 0,03 m. Sites au millimètre : 649.

| k | HDBSCAN, meilleur IoU par objet (A / B / C) | HGP, meilleur IoU par objet | IoU moyen HDBSCAN / HGP | issue |
| --- | --- | --- | --- | --- |
| 2 | 0,82 / 0,56 / 0,94 | 0,68 / **0,49** / 0,99 | 0,78 / 0,72 | HGP échoue, HDBSCAN réussit |
| 3 | 0,81 / 0,55 / 0,94 | 0,79 / 0,56 / 0,98 | 0,77 / 0,78 | les deux réussissent |
| 5 | 0,77 / 0,52 / 0,93 | 0,78 / 0,51 / 0,99 | 0,74 / 0,76 | les deux réussissent |
| 10 | 0,67 / **0,37** / 0,95 | 0,77 / 0,51 / 1,00 | 0,66 / 0,76 | HGP réussit, HDBSCAN échoue |

En gras : objet à 0,5 ou moins : aucun groupe de la hiérarchie ne le recouvre à plus de la moitié.

## Images

Vue de dessus, tournée selon l'axe principal. Trois panneaux : vérité (A bleu, B orange, C violet) ; meilleur groupe de HDBSCAN pour l'objet clé ; meilleur groupe de HGP pour le même objet. Vert : point de l'objet dans le groupe ; rouge : point d'un autre objet dans le groupe ; bleu : point de l'objet hors du groupe ; gris : autres points.

k = 10, objet clé B :

![k = 10](k10.png)

## Données

`bout.json` décrit le bout (trame, empreintes sha256 de la trame, des étiquettes et des fichiers du bout, instances) et donne les meilleurs IoU à chaque ordre. Les points ne sont pas versionnés (CC BY-NC-SA) : `data/` est ignoré par git. Pour les refaire à l'identique depuis les archives officielles :

```sh
python3 Zoltan/demos/tools/chercher_bouts.py --cache CACHE --out Zoltan/demos/hgp_reussit_hdbscan_echoue/bout_08_001182_trois_velos_55_56_57 \
    --rebuild Zoltan/demos/hgp_reussit_hdbscan_echoue/bout_08_001182_trois_velos_55_56_57/bout.json
```
