# Vidéos pour les réseaux sociaux

Versions courtes du duel HGP contre HDBSCAN, faites pour un écran de téléphone. Un dossier par réseau.

## LinkedIn

`LinkedIn/hgp_vs_hdbscan_dark.mp4` (fond sombre) et `LinkedIn/hgp_vs_hdbscan_light.mp4` (fond clair) : 1080 × 1350
(portrait 4:5), anglais, 82 s. Quatre scènes à la suite, dans cet ordre :

1. `06_000800_pieton_deux_velos_2_8_12/sans_sol`, k = 10 ;
2. `08_002852_deux_velos_6_51/sans_sol`, k = 5 ;
3. `08_001170_deux_velos_43_57/sans_sol`, k = 5 ;
4. `00_002140_pieton_velo_1_6/instances`, k = 10.

Chaque scène : les objets, puis HGP (en haut) et HDBSCAN (en bas) balaient l'échelle $\varepsilon$ (le rayon, comme le paramètre de DBSCAN) **en même temps** jusqu'au dernier événement qui décide du résultat, avec une courte pause
et un bandeau d'une ligne aux événements importants (« A found ✓ », « A, B found ✓ » quand tous sont retrouvés et
encore séparés, « A + B merged too early ✗ » quand des objets fusionnent avant d'avoir été retrouvés), puis le
meilleur nœud de chaque hiérarchie pour chaque objet, encadré en vert s'il retrouve l'objet (IoU > 0,5), en rouge
sinon. Une hiérarchie qui a retrouvé tous ses objets se fige au dernier de leurs maxima (son
panneau affiche cet $\varepsilon$) pendant que l'autre continue. Seuls textes : HGP, HDBSCAN, $\varepsilon$ et les clusters. Mêmes données et mêmes meilleurs nœuds que les vidéos longues
(scènes `data/duel_k<k>_en.js`, `best_sites` de `tools/duel_scene.py`).

Lecteur : `player/social.html` et `player/social.js` ; rendu :

```text
node tools/render_social.cjs --out reseaux_sociaux/LinkedIn/hgp_vs_hdbscan_dark.mp4 --theme sombre SCENE1.js ... SCENE4.js
```
