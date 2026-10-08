# Vidéos pour les réseaux sociaux

Versions courtes du duel HGP contre HDBSCAN, faites pour un écran de téléphone. Un dossier par réseau.

## LinkedIn

`LinkedIn/hgp_vs_hdbscan_dark.mp4` (fond sombre) et `LinkedIn/hgp_vs_hdbscan_light.mp4` (fond clair) : 1080 × 1350
(portrait 4:5), anglais, environ 101 s. Quatre scènes à la suite, dans cet ordre :

1. `06_000800_pieton_deux_velos_2_8_12/sans_sol`, k = 10 ;
2. `08_002852_deux_velos_6_51/sans_sol`, k = 5 ;
3. `08_001170_deux_velos_43_57/sans_sol`, k = 5 ;
4. `00_002140_pieton_velo_1_6/instances`, k = 10.

Déroulé d'une scène :

- **Les objets** : la vérité terrain, nommée (« A · bicycle »), immobile un instant puis vue en 3D par une petite
  orbite de la caméra (26° autour, 10° de descente), qui finit sur la vue du balayage.
- **Le balayage** : HGP (en haut) et HDBSCAN (en bas) balaient l'échelle $\varepsilon$ (le rayon, comme le paramètre
  de DBSCAN) **en même temps**, jusqu'au dernier événement qui décide du résultat.
  - **Ralentis** : le balayage freine à l'approche de chaque événement, s'y arrête, puis repart doucement.
  - **Surbrillance** : pendant l'arrêt, le reste du panneau s'estompe et les clusters concernés ressortent, avec un
    éclair bref puis un anneau qui pulse (vert : objet retrouvé ; rouge : fusion parasite).
  - **Commentaires**, formulation des vidéos longues, sans les IoU :
    - « A recovered ✓ » au maximum de l'IoU d'un objet ;
    - « A, B and C recovered ✓ / still separate » quand tous le sont ;
    - « ✗ B and C now merged / B and C never recovered separately » pour une fusion parasite ;
    - « A merges with the building » quand un objet déjà retrouvé absorbe le fond.
  - Une hiérarchie qui a retrouvé tous ses objets se fige au dernier de leurs maxima (son panneau affiche cet
    $\varepsilon$) pendant que l'autre continue.
- **La fin** : le meilleur nœud de chaque hiérarchie pour chaque objet, encadré en vert s'il retrouve l'objet
  (IoU > 0,5), en rouge sinon, et le bilan (« 2 / 2 objects recovered »).

Mêmes données et mêmes meilleurs nœuds que les vidéos longues (scènes `data/duel_k<k>_en.js`, `best_sites` et pauses
de `tools/duel_scene.py`).

Lecteur : `player/social.html` et `player/social.js` ; rendu :

```text
node tools/render_social.cjs --out reseaux_sociaux/LinkedIn/hgp_vs_hdbscan_dark.mp4 --theme sombre SCENE1.js ... SCENE4.js
```
