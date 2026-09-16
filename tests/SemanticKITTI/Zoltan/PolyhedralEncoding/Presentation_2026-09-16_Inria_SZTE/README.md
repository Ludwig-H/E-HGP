# Représenter le LiDAR 3D par une hiérarchie Hypergraph-Percol

**Louis Hauseux · Zoltán Kató · Josiane Zerubia**  
**Inria / Szegedi Tudományegyetem · 16 septembre 2026**

[PDF](HGP_LiDAR_Inria_SZTE_2026-09-16.pdf) · [Source principale](main.tex) · [Citations et bibliographie](references.tex)

## Plan

**14 diapositives, couverture comprise, dont deux de bibliographie. Sans annexes.** Les trois parties sont des sections Beamer et des signets PDF ; leur nom est affiché en haut de chaque diapositive. Il n'y a pas de pages intercalaires supplémentaires.

| Partie | Pages | Contenu |
|---|---|---|
| Garde | 1 | Trois noms à égalité ; logos Inria et SZTE |
| I. Introduction | 2–4 | Modèles de fondation 3D ; hypothèse capteur/portée ; hiérarchie de polyèdres |
| II. Questions | 5 | Les cinq questions de la réunion, conservées comme questions |
| III. Quelques débuts de réponse | 6–12 | Support, relèvement quadratique, distance, codage dans le cube, comparaison des échantillonnages, premiers tests |
| Bibliographie | 13–14 | Références complètes, avec liens |

Les anciennes pages 2 et 3 ont été inversées. Les titres demandés sont « Vers un modèle de fondation pour la 3D ? » et « Hypothèse : le nuage est un artefact du capteur ». La figure de portée de la soutenance est conservée. La page séparée sur les quatre cubes a été retirée ; l'identité du support avec celui de l'enveloppe convexe figure sur la page de définition du support.

## Citations

`references.tex` reprend les conventions de la soutenance : `DeclareRef`, `DeclareMyRef`, `citb`, `reffoot` et `bibligne`. Les références apparaissent entre crochets, leur notice complète est donnée en pied de page lors de la première citation, puis dans la bibliographie finale. Les citations du corps renvoient à la bibliographie ; les titres des références pointent vers leur source. La thèse est distinguée en rouge, comme les travaux de l'auteur dans la soutenance.

Les tentatives 3D citées dans l'introduction sont Sonata, Utonia et Vernata. Leur présence ne signifie ni que leurs représentations sont des polyèdres HGP, ni que leur supériorité dans notre protocole a été établie.

## Relèvement et sondes : précision mathématique

La discussion préparatoire fournie par l'auteur définissait le support quadratique et retenait finalement le centre de boîte, la normalisation isotrope et une grille cartésienne volumique. Le texte de cette conversation n'est pas publié ici. La relation avec les directions unitaires de dimension quatre ci-dessous est une explicitation algébrique de cette définition, non une nouvelle méthode attribuée à BPS.

Pour un compact non vide P, on relève **tous ses points**, et non seulement les sommets de ses primitives :

$$C_P=\mathrm{conv}\lbrace (x,\lVert x\rVert^2):x\in P\rbrace.$$

$$H_P(b)=h_{C_P}(2b,-1)=\max_{x\in P}(2\langle b,x\rangle-\lVert x\rVert^2)=\lVert b\rVert^2-d_P(b)^2.$$

Après une même normalisation de la réalisation en dimension trois, deux choix numériques sont possibles : échantillonner le support du convexe relevé sur S³ (la sphère unité de R⁴), ou échantillonner la distance dans le cube de R³. Il ne faut pas confondre S³ avec la sphère S² de l'espace physique.

La positive homogénéité du support donne, exactement :

$$u(b)=\frac{(2b,-1)}{\sqrt{1+4\lVert b\rVert^2}},\qquad h_{C_P}(u(b))=\frac{\lVert b\rVert^2-d_P(b)^2}{\sqrt{1+4\lVert b\rVert^2}}.$$

Ainsi, aux positions et directions correspondantes, les deux tableaux contiennent la même information à une transformation connue près. **Une grille uniforme du cube n'est pas une distribution uniforme sur S³.** Le cube ne couvre qu'une portion de l'hémisphère inférieur. Plus généralement, tout u=(a,t) de S³ avec t<0 correspond à b=a/(-2t). Le bord t=0 correspond à des sondes qui s'éloignent vers l'infini ; l'autre hémisphère interroge d'autres maxima quadratiques.

Il n'est pas nécessaire de connaître toute S³ pour identifier P : le champ complet sur un domaine contenant P le détermine par son ensemble de zéros. En particulier, les directions associées à tous les points du cube suffisent pour un P contenu dans ce cube. En revanche, **aucun tableau fini n'est déclaré injectif sur tous les compacts**.

Le relèvement n'est pas une normalisation concurrente : on peut d'abord centrer et normaliser P, puis choisir l'un ou l'autre échantillonnage. Un segment se relève en un arc de parabole ; le convexe relevé n'est donc pas en général le polytope des seuls sommets relevés. Pour le code par distances, il n'est pas construit explicitement.

## Choix du pilote et limites

- La réalisation P_v reste distincte de la hiérarchie définie dans la thèse et des seuls certificats Q2/Q3/Q4. Le pilote K=2 peut utiliser l'union des segments de la composante, sans remplir les triangles.
- Le centre est celui de la boîte, l'échelle est la moitié de son plus grand côté. **Aucune rotation locale**, et un seul facteur sur les trois axes. Centre et taille physiques restent disponibles. Les cas ponctuels sont traités séparément.
- Les 512 sondes sont les centres des 8³ cellules de [-1,1]³. Elles sont **dans le volume ambiant**, non échantillonnées sur la surface du polyèdre. Leurs distances portent sur les primitives entières.
- Les identités de stabilité et de composition par minimum concernent des fonctions dans un même repère. Un minimum entre tableaux normalisés indépendamment n'est pas une opération exacte justifiée.
- L'IoU porte sur des masques de points annotés. Exclure les labels ignorés et les instances invalides ; contrôler le budget de régions. Le meilleur nœud par instance est un oracle diagnostique, pas une segmentation réalisable simultanément ni un gain appris.
- Le petit test A/B/C sépare choix des régions et réalisation géométrique. Le contrôle avec/sans échanges parent–enfant, à mêmes régions, codes et budgets, concerne les liens hiérarchiques. Garder le réglage séparé de l'évaluation. Répéter plusieurs graines.
- Le contrôle de robustesse conserve 50 % puis 25 % des retours, en recalculant la hiérarchie et les codes. Il ne simule pas à lui seul un autre capteur.

Les attributs physiques, une adaptation légère de type LoRA et l'architecture complète restent des questions à discuter, pas des décisions arrêtées ni des résultats. **Aucune expérience SemanticKITTI nouvelle ni aucun gain mesuré ne sont revendiqués.** Aucun moteur Morse HGP, registre ou workflow n'est modifié. GCP non utilisé.

## Sources et provenance

Source de la présentation avant révision : `main.tex`, blob `cc05a8a5e3311a86ac18b6c793a5771d803a5ed2`, identique à l'archive locale fournie. Le schéma de portée conserve le blob `9cca660433532a16af3cf736cf9383184d3ba9e6` de `Ludwig-H/Manuscrit-de-th-se/Soutenance/soutenance/figs/verrou_portee.tex`. La figure introductive est adaptée pour ajouter les trois références 3D.

Système de citations et titre de la thèse : `Ludwig-H/Manuscrit-de-th-se/Soutenance/soutenance/main.tex`, blob `bb66a9230aa6c3f8fefb8661909421020953a9d6`. Les parties I–II du manuscrit définissent la hiérarchie ; le support quadratique vient de la discussion préparatoire, et non d'un théorème attribué au manuscrit.

Références extérieures vérifiées le 16 septembre 2026 :

- Sonata : https://arxiv.org/abs/2503.16429 ; CVPR 2025.
- Utonia : https://arxiv.org/abs/2603.03283v2 ; ICML 2026.
- Vernata : https://arxiv.org/abs/2608.06919v1 ; prépublication du 7 août 2026, annoncée IROS 2026 dans la notice.
- BPS : https://arxiv.org/abs/1908.09186 ; notice ICCV 2019, p. 4332–4341, du CVF et du Max Planck Institute.
- SemanticKITTI : https://semantic-kitti.org/dataset.html ; référence Behley et al., ICCV 2019.
- LoRA : https://arxiv.org/abs/2106.09685 ; référence ICLR 2022 déjà utilisée dans la soutenance. La page OpenReview n'était pas consultable sans vérification de navigateur lors de cette révision.
- Schneider : https://doi.org/10.1017/CBO9781139003858 ; 2e édition, dates de publication 2013 données par la notice Cambridge. Le millésime retenu ici suit cette notice.

`theme/` conserve l'adaptation autonome du thème Inria 2024. Aucun fichier de police n'est distribué. Logos : Inria, SVG de la soutenance ; SZTE, `moraszk/moraweb/static/icon/szte.png`, blob `0c27d77127a186ef3ac3b6d0d30932c50bb5c513`, marges transparentes retirées. Les droits sur les marques restent à leurs institutions.

## Compilation et contrôle

Dépendances : LuaLaTeX, latexmk, Beamer, TikZ, Babel français, Python 3 et CairoSVG.

```sh
make
make clean
```

`make clean` conserve le PDF. Sur Overleaf : importer le dossier complet avec ses images préparées, choisir LuaLaTeX et compiler `main.tex`. `prepare_assets.py` ne fait aucun appel réseau. Les figures restent éditables en TikZ.

Contrôles de cette révision : compilation, 14 pages rendues et inspectées, absence de débordements `Overfull` et de glyphes `Missing character`, vérification des trois sections et des renvois bibliographiques. Le workflow existant recompile ensuite le PDF sur `main`. La vérification du document ne valide pas les expériences proposées.
