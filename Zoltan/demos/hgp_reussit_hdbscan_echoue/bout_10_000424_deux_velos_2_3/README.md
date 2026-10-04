# Deux vélos (trame 10/000424)

Catégorie : [HGP réussit, HDBSCAN échoue](../README.md). Bout de scène SemanticKITTI, séquence 10, trame 000424, réduit aux seuls points de ses objets : ni sol, ni fond, ni autre objet ; mesures G4 `claudebouts1` (tous les bouts, commit f1a53fe1c) et `claudebouts2` (blocs publiés, commit b72fe8771).

| objet | classe | points |
| --- | --- | --- |
| A | vélo | 163 |
| B | vélo | 98 |

Écarts (plus courte distance entre les points de deux objets) : A–B : 0,04 m. Sites au millimètre : 261.

| k | HDBSCAN, meilleur IoU par objet (A / B) | HGP, meilleur IoU par objet | IoU moyen HDBSCAN / HGP | issue |
| --- | --- | --- | --- | --- |
| 2 | 0,78 / 0,51 | 0,88 / 0,51 | 0,64 / 0,70 | les deux réussissent |
| 3 | 0,78 / **0,50** | 0,78 / **0,50** | 0,64 / 0,64 | les deux échouent |
| 5 | 0,78 / **0,50** | 0,79 / 0,51 | 0,64 / 0,65 | HGP réussit, HDBSCAN échoue |
| 10 | 0,79 / **0,46** | 0,81 / **0,50** | 0,62 / 0,65 | les deux échouent |

En gras : objet à 0,5 ou moins : aucun groupe de la hiérarchie ne le recouvre à plus de la moitié.

## Images

Vue de dessus, tournée selon l'axe principal. Trois panneaux : vérité (A bleu, B orange, C violet) ; meilleur groupe de HDBSCAN pour l'objet clé ; meilleur groupe de HGP pour le même objet. Vert : point de l'objet dans le groupe ; rouge : point d'un autre objet dans le groupe ; bleu : point de l'objet hors du groupe ; gris : autres points.

k = 5, objet clé B :

![k = 5](k5.png)

## Données

`bout.json` décrit le bout (trame, empreintes sha256 de la trame, des étiquettes et des fichiers du bout, instances) et donne les meilleurs IoU à chaque ordre. Les points ne sont pas versionnés (CC BY-NC-SA) : `data/` est ignoré par git. Pour les refaire à l'identique depuis les archives officielles :

```sh
python3 Zoltan/demos/tools/chercher_bouts.py --cache CACHE --out Zoltan/demos/hgp_reussit_hdbscan_echoue/bout_10_000424_deux_velos_2_3 \
    --rebuild Zoltan/demos/hgp_reussit_hdbscan_echoue/bout_10_000424_deux_velos_2_3/bout.json
```

<!-- plat:debut -->

## Sortie plate (clusters)

mcs = 20, racine exclue, aucune complétion. Panneaux, de gauche à droite puis de haut en bas : vérité ; HDBSCAN (`sklearn` tel quel) ; HGP, EOM z = 1 ; HGP, EOM z = 2 ; HGP, feuilles. Un cluster apparié à un objet suivi (IoU > 1/2) prend la couleur de l'objet ; les autres clusters ont des couleurs pâles ; le bruit est gris clair.

![Sortie plate à k = 5](plat_k5.png)

| Sortie (k = 5) | Clusters | Objets retrouvés | Objets fusionnés |
| --- | --- | --- | --- |
| HDBSCAN (`sklearn` tel quel) | 2 | 2 / 2 | 0 |
| HGP, EOM z = 1 | 2 | 2 / 2 | 0 |
| HGP, EOM z = 2 | 2 | 2 / 2 | 0 |
| HGP, feuilles | 4 | 2 / 2 | 0 |

Données : arbres exportés par `morsehgp3D_v11/bench/points_flat_dump.py` (session G4 `claudeflat0`), tête certifiée `points_flat.py` ; outil : `tools/rendre_plat.py` ; décision : `morsehgp3D_v11/docs/SORTIE_PLATE.md`.

<!-- plat:fin -->
