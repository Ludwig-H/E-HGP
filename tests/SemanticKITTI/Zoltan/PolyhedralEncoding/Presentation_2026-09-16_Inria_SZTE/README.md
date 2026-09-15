# Représenter le LiDAR 3D par une hiérarchie HGP

**Version courte : 10 diapositives, couverture comprise, sans annexes.**

Louis Hauseux · 16 septembre 2026  
Réunion avec **Zoltán Kató** et **Josiane Zerubia**.  
**Inria / Szegedi Tudományegyetem**.

[PDF](HGP_LiDAR_Inria_SZTE_2026-09-16.pdf) · [Source LaTeX](main.tex)

## Contenu

| Pages | Sujet |
|---|---|
| 1–3 | Couverture et deux figures introductives de la soutenance, allégées |
| 4–5 | Plan d'appui et limite du support pour les géométries non convexes |
| 6–7 | Support quadratique, distance et propriétés utiles |
| 8 | Centre de boîte, normalisation isotrope, grille 8³ et métadonnées |
| 9–10 | Oracle d'IoU d'instances, puis comparaison avec un petit réseau |

Cette version remplace les 34 pages précédentes. Les preuves, annexes, détails d'architecture et d'entraînement, ainsi que la métaphore alphabet/grammaire ont été retirés. L'ancienne version reste dans l'historique Git, sans présentation parallèle.

## Précisions conservées hors des slides

La base de cette révision est la conversation préparatoire fournie par l'auteur et la présentation existante ; aucune nouvelle revue de littérature n'est ajoutée.

- La réalisation géométrique est distincte des seuls supports Q2/Q3/Q4. Pour le pilote K = 2 : union des segments de la composante, sans remplir les triangles.
- Centre de boîte et demi-plus-grand-côté ; un seul facteur sur les trois axes, **aucune rotation locale**. Traiter séparément les éléments ponctuels, pour lesquels cette taille vaut zéro.
- Les sondes sont les centres des cellules de [-1,1]³ : `b_ijk = (-1+(2i+1)/8, -1+(2j+1)/8, -1+(2k+1)/8)`, pour `i,j,k = 0,…,7`. Ce code fini n'est pas déclaré injectif.
- La stabilité et la composition par minimum concernent des fonctions dans un même repère. Un minimum direct entre tableaux renormalisés indépendamment n'est pas justifié.
- L'IoU concerne les masques de points annotés, non des volumes. Le maximum par instance est un oracle diagnostique, pas une segmentation simultanément réalisable ni une preuve de gain appris. Les labels ignorés sont exclus ; le budget de régions doit être contrôlé.
- La comparaison A/B/C sépare sélection des régions et réalisation géométrique ; elle n'isole pas à elle seule l'effet des liens parent–enfant. Réseau, attributs, données et budgets sont appariés. Garder les séquences de réglage séparées de l'évaluation.
- Recalculer HGP et les codes après sous-échantillonnage. Ce test ne simule pas à lui seul un autre capteur.

**Protocole proposé seulement : aucune nouvelle expérience ni aucun gain mesuré.** Aucun moteur Morse HGP ni registre de qualification n'est modifié.

## Provenance

Les deux figures introductives et le thème proviennent de `Ludwig-H/Manuscrit-de-th-se/Soutenance/soutenance/`, source `main.tex` au blob `bb66a9230aa6c3f8fefb8661909421020953a9d6`. Leur texte a été allégé. La définition HGP vient des parties I–II du manuscrit ; la réalisation géométrique reste un choix du prototype.

Le dossier `theme/` conserve l'adaptation autonome du thème Inria 2024 déjà employée dans cette présentation. Aucun fichier de police n'est distribué.

Logos : Inria, SVG du thème de soutenance ; SZTE, `moraszk/moraweb/static/icon/szte.png`, blob `0c27d77127a186ef3ac3b6d0d30932c50bb5c513`, marges transparentes retirées. Les droits sur les marques restent à leurs institutions.

Références présentes dans les sources préparatoires :

- Prokudin, Lassner, Romero, *Efficient Learning on Point Clouds with Basis Point Sets*, ICCV 2019 : https://arxiv.org/abs/1908.09186
- Behley et al., *SemanticKITTI*, ICCV 2019 : https://semantic-kitti.org/dataset.html
- Brown et al., *Language Models are Few-Shot Learners*, NeurIPS 2020 ; Kirillov et al., *Segment Anything*, ICCV 2023, pour la figure introductive.

## Compilation et vérification

Dépendances : LuaLaTeX, latexmk, Beamer, TikZ, Babel français, Python 3 et CairoSVG.

```sh
make
make clean
```

`make clean` conserve le PDF. Sur Overleaf : importer le dossier complet, choisir LuaLaTeX et compiler `main.tex`. Les figures sont éditables en TikZ. `prepare_assets.py` prépare les PNG localement, sans réseau.

Le workflow existant `hgp-szeged-presentation.yml` recompile et publie le PDF sur `main`. Vérification locale : 10 pages rendues, aucun avertissement `Overfull` ou `Missing character`. Ce contrôle du document ne valide pas les expériences proposées.
