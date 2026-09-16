# Représenter le LiDAR 3D par une hiérarchie Hypergraph-Percol

**Louis Hauseux · Zoltán Kató · Josiane Zerubia**  
**Inria / Szegedi Tudományegyetem · 16 septembre 2026**

[PDF](HGP_LiDAR_Inria_SZTE_2026-09-16.pdf) · [Source principale](main.tex) · [Bibliographie](references.tex)

## Plan

**18 pages : 1 garde, 4 transitions, 11 pages de contenu et 2 pages de bibliographie. Sans annexes.**

| Partie | Transition | Contenu |
|---|---|---|
| Garde | — | Page 1 : trois noms à égalité, logos Inria et SZTE |
| Introduction | Page 2 | Pages 3–5 : modèles 3D, hypothèse capteur, hiérarchie |
| Questions | Page 6 | Page 7 : les cinq questions |
| Quelques bons points | Page 8 | Page 9 : tableau récapitulatif |
| Éléments de réponse | Page 10 | Pages 11–15 : géométrie et encodage ; page 16 : oracle d’instances |
| Bibliographie | — | Pages 17–18 |

## Révision : exemples de Gabriel et allègement

La slide « Quel objet encoder ? » présente **cinq points au total par ensemble de Gabriel**, avec uniquement le support géométrique en bleu. Les autres points sont gris ; aucune arête ne les relie au support.

| Support Q de la boule minimale | Réalisation bleue | Points strictement intérieurs |
|---|---|---|
| 2 points | Arête diamétrale | 3 |
| 3 points | Triangle aigu | 2 |
| 4 points | Tétraèdre | 1 |

Le dernier dessin représente l’union de ces trois primitives, et non l’enveloppe convexe de leur union. Le cardinal illustré est |A| = 5 : il décrit le nombre demandé de points du dessin, sans modifier la convention d’ordre K du moteur.

`figs/gabriel_supports.tex` utilise une projection orthographique commune. Les supports à trois et quatre points sont équilatéral et régulier. Les trois boules de rayon 1 peuvent être placées tangentes dans un même nuage de 13 sites : centres (-2,0,0), (0,0,0) et (1,sqrt(3),0), un sommet partagé à chaque tangence. Chaque boule contient exactement les cinq points de son ensemble. Vérifications numériques locales : comptage des sites, points de support sur la sphère, autres points strictement intérieurs, centre dans l’intérieur relatif du support et recherche exhaustive de la plus petite boule sur les cinq sites. Les coordonnées sont indiquées en commentaire de la figure.

Ce dessin suit le choix explicite de l’auteur : colorer les supports, pas l’enveloppe convexe de tous les points de chaque population. Il illustre une union géométrique, sans prétendre définir un événement de fusion HGP ni reconstruire une surface matérielle.

Les phrases indiquées par l’auteur ont été retirées des slides. Les pages « Mesurer un apport avec un petit modèle » et « Encoder un polyèdre : pseudo-code » sont supprimées, ainsi que le fichier du pseudo-code. L’introduction, les quatre transitions, les questions, le tableau et la bibliographie sont conservés.

La fonction support est suivie directement de la distance aux primitives. La citation `[BPS 19]` reste attachée à la diapositive de normalisation, avec sa notice en pied de page et son entrée bibliographique.

## Conventions conservées hors des slides

Les fonctions de distance s’appliquent à la réalisation P déclarée : points, segments, triangles remplis, tétraèdres pleins ou unions de ces objets. La surface d’un tétraèdre et son volume sont deux réalisations différentes. La géométrie reste conservée explicitement ; le vecteur de distances est un résumé pour le réseau.

Le centre est celui de la boîte englobante de P ; l’échelle est la moitié de son plus grand côté. Pour une taille positive, les trois coordonnées sont divisées par le même scalaire, sans rotation. Les 512 sondes sont les centres des cellules du cube [-1,1]³, dans l’ordre `(i,j,k)`, avec `k` variant le plus vite : `b_ijk = (-1+(2i+1)/8, -1+(2j+1)/8, -1+(2k+1)/8)`, pour `i,j,k = 0,…,7`. Leurs distances ne sont pas triées.

Le cas ponctuel de taille nulle doit être traité séparément : géométrie normalisée {0}, distances `D[j] = norme(b_j)`, centre physique c et taille 0. Les identités sur les champs complets concernent des compacts non vides dans un même repère. Un minimum entre tableaux centrés et normalisés indépendamment n’est pas une mise à jour exacte. Le code fini reste une approximation ; attributs, relations HGP et niveaux fins sont conservés séparément.

L’IoU porte sur les indices de points annotés, non des volumes. Les labels ignorés et les instances invalides sont exclus. Le meilleur nœud par instance est un oracle, pas une segmentation simultanément réalisable ni un gain appris. Le budget de régions doit être contrôlé dans toute expérience comparative.

## Citations et provenance

`references.tex` conserve le système de la soutenance : citations entre crochets, liens vers la bibliographie et notices complètes en pied de page. La thèse est distinguée en rouge. Les douze références sont inchangées par cette révision.

Le tableau de `points_encourageants.tex` conserve les précédents vérifiés lors des révisions antérieures : Superpoint Transformer (superpoints et hiérarchie), BPS (distances) et PolyhedronNet (attributs). L’ablation à un seul niveau de Superpoint Transformer perd 5,1 points de mIoU sur KITTI-360 validation ; elle ne prédit aucun gain HGP. BPS est évalué notamment sur des objets ModelNet40 ; PolyhedronNet utilise des polyèdres déjà construits. Ces résultats ne sont pas assimilés à des expériences sur notre représentation.

Base de cette révision : `ff7191355aa555e251854362d9d671088c3c5c69`, source `main.tex` au blob `d6d0c8db4b6d83db2a12648a8d04d76e196f829f`, identique à l’archive locale fournie. Les figures introductives et les conventions de citations viennent de `Ludwig-H/Manuscrit-de-th-se/Soutenance/soutenance/` ; le schéma de portée conserve le blob `9cca660433532a16af3cf736cf9383184d3ba9e6`. Le manuscrit, parties I–II, est la référence pour la hiérarchie, non pour une validation de notre futur modèle de fondation.

`theme/` conserve l’adaptation du thème Inria 2024. Aucun fichier de police n’est distribué. Logos : Inria, SVG de la soutenance ; SZTE, `moraszk/moraweb/static/icon/szte.png`, blob `0c27d77127a186ef3ac3b6d0d30932c50bb5c513`, marges transparentes retirées. Les droits restent à leurs institutions.

## Compilation et contrôle

Dépendances : LuaLaTeX, latexmk, Beamer, TikZ, Babel français, Python 3 et CairoSVG.

```sh
make
make clean
```

`make clean` conserve le PDF. Sur Overleaf : importer le dossier complet, choisir LuaLaTeX et compiler `main.tex`. `prepare_assets.py` ne fait aucun appel réseau. Les figures restent éditables en TikZ.

Contrôles : 18 pages rendues, absence de débordements `Overfull` et de glyphes `Missing character`, quatre transitions, douze références bibliographiques, dessins de Gabriel conservés. Le workflow existant recompile et publie le PDF sur `main`.

Aucune nouvelle expérience SemanticKITTI ni aucun gain HGP mesuré. Aucun moteur, registre de qualification ou workflow n’est modifié. GCP non utilisé.
