# LiDAR 3D guidé par HGP : représentation et premiers tests

**Louis Hauseux · 16 septembre 2026**  
Réunion avec **Zoltán Kató** et **Josiane Zerubia**.  
**Inria / Szegedi Tudományegyetem**.

[Ouvrir le PDF](HGP_LiDAR_Inria_SZTE_2026-09-16.pdf) · [Source principale](main.tex)

## Contenu

34 diapositives : 28 pour l'exposé, 6 d'annexes mathématiques et bibliographiques.

| Pages | Sujet |
|---|---|
| 1–3 | Couverture et deux diapositives introductives de la soutenance |
| 4–5 | Hiérarchie HGP, réalisation géométrique et premier choix filaire à K = 2 |
| 6–9 | Fonction support, plans d'appui, propriétés convexes et limites non convexes |
| 10–15 | Distance non signée, support quadratique, relèvement et calcul sur primitives |
| 16–20 | Centrage, normalisation isotrope sans rotation, grille 8³, métadonnées et appartenances |
| 21–28 | Oracles d'instances, fidélité, stabilité, petit réseau et expériences séparant les gains |
| 29–34 | Preuves, changement de repère, contre-exemple Q2 et références |

L'exposé présente une **proposition de représentation et de protocole**. Aucun score nouveau, aucune expérience SemanticKITTI exécutée et aucune qualification de performance de Morse HGP ne sont revendiqués.

## Choix repris de la discussion préparatoire

La conversation transmise par l'auteur le 15 septembre 2026 constitue la référence pour la progression et les décisions finales. Le texte intégral de cette conversation n'est pas publié dans le dépôt.

- Aucun repère tourné propre à chaque polyèdre ; conservation des axes communs à la scène.
- Centre de boîte et **demi-longueur du plus grand côté**, non demi-diagonale. Un seul facteur de normalisation ; centre, taille et résolution conservés séparément.
- Sondes cartésiennes fixes **dans le volume**, non sur une sphère et non adaptatives pour le premier essai. La présentation précise la convention de placement : centres des 8³ cellules.
- `H_P(b) = max_x (2<b,x> - ||x||²) = ||b||² - d_P(b)²`. Le calcul se fait directement par distances aux primitives, sans construire de convexe en dimension quatre.
- Pour le premier pilote K = 2 : union des segments des arêtes actives de la composante. Ce choix ne prétend pas reconstruire automatiquement une surface matérielle.
- Premiers tests sans modèle de fondation : oracles, fidélité, stabilité et petit MLP.

Les sections antérieures de la conversation évoquant Steiner, demi-diagonale, rotation locale, sondes sphériques/adaptatives ou adaptateur Utonia ne remplacent pas ces décisions finales.

## Précisions scientifiques apportées dans les diapositives

Le code fini n'est pas déclaré injectif. Pour une grille de centres dans [-1,1]³, le rayon de couverture vaut sqrt(3)/8 ; la borne additive sur la distance de Hausdorff vaut sqrt(3)/4. La comparaison concerne les géométries dans le même repère normalisé, pas automatiquement leur distance physique.

Les minima entre champs d'enfants sont exacts aux **mêmes positions physiques**. Un minimum coordonnée par coordonnée de grilles centrées et normalisées indépendamment n'est pas valide. L'annexe donne le transport exact.

L'IoU porte sur les **indices de retours annotés**, après exclusion des labels ignorés et des identifiants d'instance invalides. Ce n'est pas une IoU de solides reconstruits. Les oracles par instance ne constituent pas une segmentation réalisable simultanément ; l'appariement un-à-un et le budget de propositions doivent être évalués séparément.

Les comparaisons distinguent trois facteurs : construction des régions (HGP / K = 1 / grilles), réalisation géométrique (points / primitives), utilisation des relations parent–enfant (mêmes états, même petit réseau). Le tableau factoriel précise le dernier principe de la discussion ; il ne prétend pas être une expérience déjà réalisée.

## Sources et provenance

Sources de thèse et de soutenance : dépôt `Ludwig-H/Manuscrit-de-th-se`, branche `main`, consulté pour cette préparation.

- `Manuscrit_de_these/Manuscrit these Louis Hauseux/PartI/` : Single-Linkage, Hartigan, hiérarchies et extraction.
- `PartII/ChapI_et_II_fusionnes.tex` : régions témoins et correspondance K-NN.
- `PartII/ChapIII.tex`, `ChapIV.tex`, `ChapV.tex` : percolation, boules minimales, Gabriel et hiérarchies pratiques.
- `Soutenance/soutenance/main.tex`, blob `bb66a9230aa6c3f8fefb8661909421020953a9d6`.
- Introduction 1 : `figs/alphabet_fondation.tex`, blob `3ffac4ad7fe2bed1c3ff4f03129e61b9f0c184ca`.
- Introduction 2 : `figs/hierarchie_surfaces.tex`, blob `a0d8fff49a7ccd75cf59e7a16094ddb209e4cdde`.

Les deux diapositives d'introduction reprennent leur texte et leur dessin TikZ ; les commentaires et espaces des sources ont été condensés. Les numéros, la date et les références de pied de page sont adaptés au nouvel exposé. Leur formulation de perspective est historique, non une affirmation que les modèles 3D préentraînés n'existent pas.

Le dossier `theme/` est une adaptation autonome et allégée des fichiers du **thème Inria 2024 utilisé dans la soutenance**, conservant couleurs, marges, angle dégradé, filet et pied de page. La couverture est adaptée à la réunion bilatérale ; les fonctions de pages de section et de remerciement inutilisées ne sont pas recopiées. Police de substitution standard de TeX lorsque les fontes Inria ne sont pas installées ; **aucun fichier de police n'est distribué**.

Références externes, limitées aux antécédents utilisés :

- Prokudin, Lassner, Romero, *Efficient Learning on Point Clouds with Basis Point Sets*, ICCV 2019 : https://arxiv.org/abs/1908.09186
- Behley et al., *SemanticKITTI: A Dataset for Semantic Scene Understanding of LiDAR Sequences*, ICCV 2019 : https://semantic-kitti.org/dataset.html
- Profil institutionnel de Kató Zoltán : https://www.inf.u-szeged.hu/users/kato-zoltan
- Nom institutionnel et identité graphique : https://u-szeged.hu/egyetemi-arculat

### Logos

Inria : SVG du thème de la soutenance, `theme/imgs/Inria-logo-rouge.svg` (source originale : blob `7781c12d3d6725db074d66bdd32805ff92a53072`). Dégradé et angle également issus de ce thème.

SZTE : logo institutionnel figurant dans le site du Móra Ferenc Szakkollégium, dépôt `moraszk/moraweb`, `static/icon/szte.png` (branche `master`, blob **`0c27d77127a186ef3ac3b6d0d30932c50bb5c513`**). Les octets de la source ont été vérifiés contre ce hash Git. Seules les marges transparentes ont été retirées, sans redessiner ni recolorer le logo. Le nom hongrois est écrit à côté dans la couverture. Les droits sur les marques restent à leurs institutions respectives.

`assets/szte.png.b64` est la source PNG encodée sans perte pour son transport textuel ; `prepare_assets.py` la décode localement. Aucune connexion réseau n'est utilisée lors de la compilation.

## Compilation reproductible

Dépendances : Python 3 + CairoSVG, LuaLaTeX, latexmk, Beamer, TikZ et Babel français.

Sur Debian/Ubuntu :

```sh
sudo apt-get install latexmk texlive-luatex texlive-latex-extra texlive-fonts-recommended texlive-lang-french python3-cairosvg
make
```

Le résultat est `HGP_LiDAR_Inria_SZTE_2026-09-16.pdf`. `make clean` supprime uniquement les fichiers auxiliaires, pas le PDF. Les figures originales et nouvelles sont éditables en TikZ. Sur Overleaf, importer le dossier complet avec ses PNG préparés, choisir LuaLaTeX et compiler `main.tex`.

## Contrôle de livraison

Compilation LuaLaTeX, vérification des débordements et inspection des 34 pages rendues. Les démonstrations des bornes utilisées sont incluses en annexe. La reproductibilité du PDF ne constitue pas une exécution des expériences proposées.
