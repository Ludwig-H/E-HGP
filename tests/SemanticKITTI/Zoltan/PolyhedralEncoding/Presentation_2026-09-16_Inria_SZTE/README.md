# Représenter le LiDAR 3D par une hiérarchie Hypergraph-Percol

**Louis Hauseux · Zoltán Kató · Josiane Zerubia**  
**Inria / Szegedi Tudományegyetem · 16 septembre 2026**

[PDF](HGP_LiDAR_Inria_SZTE_2026-09-16.pdf) · [Source principale](main.tex) · [Citations et bibliographie](references.tex)

## Plan

**21 diapositives : 1 garde, 4 transitions, 14 pages de contenu (dont le tableau et le pseudo-code), 2 pages de bibliographie. Sans annexes.** Chaque partie commence par une véritable diapositive de transition, avec numéro, titre et filet Inria. Son nom reste indiqué sur les pages de contenu.

| Partie | Transition | Contenu |
|---|---|---|
| Garde | — | Page 1 : trois noms à égalité ; logos Inria et SZTE |
| I. Introduction | Page 2 | Pages 3–5 : modèles 3D, hypothèse capteur/portée, hiérarchie de polyèdres |
| II. Questions | Page 6 | Page 7 : les cinq questions de la réunion |
| III. Quelques bons points | Page 8 | Page 9 : un seul tableau récapitulatif |
| IV. Éléments de réponse | Page 10 | Pages 11–19 : primitives, support, distance, encodage, premiers tests et pseudo-code |
| Bibliographie | — | Pages 20–21 : références complètes avec liens |

Les anciennes pages 2 et 3 ont été inversées. Les titres demandés sont « Vers un modèle de fondation pour la 3D ? » et « Hypothèse : le nuage est un artefact du capteur ». La figure de portée de la soutenance est conservée. La page séparée sur les quatre cubes a été retirée ; l'identité du support avec celui de l'enveloppe convexe figure sur la page de définition du support.

## Citations

`references.tex` reprend les conventions de la soutenance : `DeclareRef`, `DeclareMyRef`, `citb`, `reffoot` et `bibligne`. Les références apparaissent entre crochets, leur notice complète est donnée en pied de page lors de la première citation, puis dans la bibliographie finale. Les citations du corps renvoient à la bibliographie ; les titres des références pointent vers leur source. La thèse est distinguée en rouge, comme les travaux de l'auteur dans la soutenance.

Les tentatives 3D citées dans l'introduction sont Sonata, Utonia et Vernata. Leur présence ne signifie ni que leurs représentations sont des polyèdres HGP, ni que leur supériorité dans notre protocole a été établie.

## Quelques bons points : base et portée du tableau

Le tableau de `points_encourageants.tex` reprend quatre thèmes de la conversation jointe par l'auteur : Superpoint Transformer (superpoints et contexte multi-échelle), Basis Point Sets (codage par distances utilisable par un petit réseau), et PolyhedronNet (géométrie et attributs). Le texte intégral de la conversation n'est pas publié.

La colonne « Résultat publié » rapporte ces précédents ; la colonne « Pour notre projet » indique des pistes à tester, non des gains HGP démontrés. Les références primaires ont été contrôlées le 16 septembre 2026 :

- **Superpoints.** Robert, Raguet et Landrieu, *Efficient 3D Semantic Segmentation with Superpoint Transformer*, ICCV 2023, sections 4.1–4.2 : segmentation supervisée sur KITTI-360 et DALES. Ce n'est ni un modèle de fondation ni une hiérarchie K-NN HGP.
- **Hiérarchie.** La discussion mentionne le bénéfice des régions multi-échelles. La vérification du **tableau 4** précise le chiffre ajouté à la slide : ne garder qu'un niveau de partition donne **−5,1 points de mIoU sur KITTI-360 validation**, par rapport à leur meilleur modèle (63,5). Il s'agit de leur ablation, pas d'une borne ni d'une prévision de gain pour HGP. Les auteurs n'observent pas de bénéfice supplémentaire avec trois niveaux ou davantage. Source : https://arxiv.org/html/2306.08045v1#S4.T4
- **Distances.** Prokudin, Lassner et Romero, *Efficient Learning on Point Clouds with Basis Point Sets*, ICCV 2019, résumé et section 4.3 : un réseau entièrement connecté sur les distances atteint une précision comparable à PointNet en classification ModelNet40. Les données sont des objets, pas des scènes LiDAR extérieures. Le passage des points aux arêtes/faces HGP reste notre proposition. Source : https://arxiv.org/html/1908.09186v1
- **Attributs.** Yu, Zhang et Zhao, *PolyhedronNet*, ICLR 2025, section 5.5, tableau 3 : l'exactitude sur ShapeNet-P est de 0,627 avec attributs de faces, contre 0,578 lorsqu'ils sont masqués. Le tableau de la présentation conserve seulement l'observation qualitative déjà présente dans l'échange. Ce résultat concerne des polyèdres d'objets déjà construits ; il ne valide ni notre reconstruction ni une invariance par rotation pour le LiDAR. Source : https://arxiv.org/html/2502.01814v1#S5.T3

Le tableau ne reprend aucun chiffre d'oracle comme preuve d'un gain appris et ne compare pas entre eux des scores de jeux ou de tâches différents.

## Encodage proposé et pseudo-code final

`encodage_polyedres.tex` contient six pages simples : objet géométrique, fonction support, support quadratique, distances et fusions, normalisation et grille, limites du code. `encodage_pseudocode.tex` fournit la dernière page de contenu, avant la bibliographie. Les transitions, l’introduction, les questions, le tableau des résultats encourageants et les deux tests sont conservés.

La distinction est celle retenue dans la discussion : les primitives restent la référence géométrique explicite ; un code de taille fixe est calculé pour les nœuds que le réseau utilise. Une primitive ne doit pas être remplacée dans le stockage par 512 valeurs. Les primitives partagent leurs sommets et sont distinguées par leur type et leurs indices. Le pseudo-code ne construit pas cette réalisation depuis les sorties de Morse HGP : il la reçoit en entrée.

### Convention de la méthode

Entrée : une famille finie non vide de points, segments, triangles remplis et/ou tétraèdres pleins, à coordonnées finies. Une surface de tétraèdre doit être fournie comme quatre triangles, pas comme un tétraèdre plein. Les primitives dégénérées représentent leur enveloppe convexe de dimension inférieure ; la fonction de distance doit traiter ce cas.

Les 512 sondes sont les centres des cellules du cube, rangées dans l’ordre des triplets `(i,j,k)` avec `k` variant le plus vite : `b_ijk = (-1+(2i+1)/8, -1+(2j+1)/8, -1+(2k+1)/8)`, pour `i,j,k = 0,…,7`. La même base est utilisée pour tous les morceaux. Les distances ne sont jamais triées.

Le centre est celui de la boîte englobante des sommets des primitives et la taille est la moitié de son plus grand côté. Pour `s > 0`, les trois coordonnées sont divisées par le même scalaire, sans rotation. La distance est évaluée dans ces coordonnées locales, directement aux primitives entières ; il n’y a ni relèvement explicite à calculer ni soustraction de grands carrés en coordonnées physiques.

Pour `s = 0`, la réalisation est un singleton : la convention est `P_normalisé = {0}`, `D[j] = norme(b_j)`, centre `c`, taille physique `0`. Elle donne un vecteur de même dimension sans division par zéro. Les morceaux presque ponctuels restent un cas de précision à calibrer avant une implémentation de production ; aucun seuil implicite ne leur est appliqué dans ce pseudo-code.

La sortie brute est `(D,c,s)` : 512 distances, 3 coordonnées et 1 taille, soit 516 scalaires hors attributs. Ce n’est pas une compression garantie, ni un code universellement injectif. Les attributs gardent leur localisation et leurs masques de validité ; les relations HGP sont une structure séparée, pas des nombres arbitraires concaténés au code.

Pour une union, on prend le minimum des distances aux primitives. Lorsqu’un parent possède son propre centre et sa propre taille, ses sondes ne sont pas celles des enfants : le pseudo-code est réappliqué à sa réalisation, plutôt que de prendre le minimum de leurs tableaux. Un index spatial peut accélérer les requêtes, mais aucun coût accéléré ni résultat de performance n’est revendiqué ici.

Le choix de grille 8³ reste une référence d’essai. Les résolutions 4³ et 16³ servent à vérifier le compromis précision–mémoire–temps. Une petite cavité ou structure peut être manquée par les sondes d’un grand parent ; ses primitives et les niveaux fins sont conservés.

## Relèvement et sondes : précision mathématique

La discussion préparatoire fournie par l'auteur définissait le support quadratique et retenait finalement le centre de boîte, la normalisation isotrope et une grille cartésienne volumique. Le texte de cette conversation n'est pas publié ici. La relation avec les directions unitaires de dimension quatre ci-dessous est une explicitation algébrique de cette définition, non une nouvelle méthode attribuée à BPS.

Pour un compact non vide P, on relève **tous ses points**, et non seulement les sommets de ses primitives :

$$C_P=\mathrm{conv}\lbrace (x,\lVert x\rVert^2):x\in P\rbrace.$$

$$H_P(b)=h_{C_P}(2b,-1)=\max_{x\in P}(2\langle b,x\rangle-\lVert x\rVert^2)=\lVert b\rVert^2-d_P(b)^2.$$

Après une même normalisation de la réalisation en dimension trois, deux choix numériques sont possibles : échantillonner le support du convexe relevé sur S³ (la sphère unité de R⁴), ou échantillonner la distance dans le cube de R³. Il ne faut pas confondre S³ avec la sphère S² de l'espace physique.

La positive homogénéité du support donne, exactement :

$$u(b)=\frac{(2b,-1)}{\sqrt{1+4\lVert b\rVert^2}},\qquad h_{C_P}(u(b))=\frac{\lVert b\rVert^2-d_P(b)^2}{\sqrt{1+4\lVert b\rVert^2}}.$$

Ainsi, aux positions et directions correspondantes, les deux tableaux contiennent la même information à une transformation connue près. **Une grille uniforme du cube n'est pas une distribution uniforme sur S³.** Le cube ne couvre qu'une portion de l'hémisphère inférieur. Plus généralement, tout u=(a,t) de S³ avec t<0 correspond à b=a/(-2t). Le bord t=0 correspond à des sondes qui s’éloignent vers l’infini. Pour t≥0, la fonction maximisée `x ↦ <a,x> + t ||x||²` est convexe ; son maximum sur P est donc le même que sur son enveloppe convexe 3D. Cet hémisphère ne distingue pas les géométries qui partagent cette enveloppe. Cette observation algébrique motive le choix pratique des sondes de distance, sans affirmer qu’un placement fini soit optimal.

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

Base de cette révision : dépôt au commit `6429235149dc3179d2c260b579748689b8d0d2f8`, `main.tex` au blob `2e1ea13959bbe98622a899ddd16fe2aaf497f097`, identique à l’archive locale fournie. La partie encodage transpose la recommandation acceptée dans l’échange suivant : primitives conservées, code de distances calculé pour le réseau. Le reste de la présentation et les références ne font pas l’objet d’une nouvelle revue de littérature. Les transitions reprennent le principe de la page de section du thème de soutenance (numéro, titre centré et filet). Le schéma de portée conserve le blob `9cca660433532a16af3cf736cf9383184d3ba9e6` de `Ludwig-H/Manuscrit-de-th-se/Soutenance/soutenance/figs/verrou_portee.tex`. La figure introductive est adaptée pour ajouter les trois références 3D.

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

Contrôles de cette révision : compilation, 21 pages rendues et inspectées, absence de débordements `Overfull` et de glyphes `Missing character`, vérification des quatre transitions (pages 2, 6, 8 et 10), du tableau unique et des douze références bibliographiques. Le workflow existant recompile ensuite le PDF sur `main`. La vérification du document ne valide pas les expériences proposées.
