# Démos : là où le clustering d'instance sans sémantique échoue

Petites vidéos pour les présentations Inria / SZTE. Chacune balaie une
hiérarchie concurrente de Morse HGP 3D sur une trame SemanticKITTI, suit
trois objets et montre, boîtes à l'appui, le niveau où la hiérarchie les
fusionne à tort — ou, pour le témoin, où elle réussit. Chaque vidéo existe
en deux thèmes, comme Percolia.com : **sombre** (fond marine) et **clair**
(fond blanc).

```text
phase=demonstration_hors_registre
backend=reference_cpu (HDBSCAN et ALPINE réimplémentés ; Morse HGP 3D non exécuté)
profile=float32_brut (trames SemanticKITTI 08, sol retiré par Patchwork++ v8 ou conservé)
mode=illustration
public_status=not_claimed
```

**Aucun résultat Morse HGP 3D n'est montré ni revendiqué ici.** Les vidéos
établissent seulement des échecs mesurés des méthodes concurrentes, sur des
trames identifiées. Que la tour HGP fasse mieux sur ces mêmes trames reste
à mesurer quand la v9 sera prête ; ces trames en sont les cas d'essai
désignés.

## Catalogue

Meilleur IoU atteignable par **un nœud quelconque** de l'arbre, pour chaque
objet (A / B / C). L'IoU suit l'évaluation panoptique de SemanticKITTI : les
points « void » (non étiqueté, aberrant, autre structure, autre objet) en
sont exclus. Un objet dont le meilleur IoU est ≤ 0,5 ne peut donc être
compté vrai positif par **aucune** extraction de la hiérarchie : 0,5 est le
seuil d'appariement de la qualité panoptique (PQ).

<!-- catalogue:début -->
| démo | trame | objets | HDBSCAN K = 5 | HDBSCAN K = 10 | ALPINE sans sémantique | vidéos |
| --- | --- | --- | --- | --- | --- | --- |
| [01 vélos garés en rang](01_velos_en_rang/) | 08/001176, sans sol | quatre vélos garés en deux paires ; trois suivis | 0,67 / **0,41** / 0,75 | 0,62 / **0,40** / 0,64 | 0,91 / 0,82 / 0,89 | K5 [sombre](01_velos_en_rang/01_velos_en_rang_hdbscan_K5_sombre.mp4) · [clair](01_velos_en_rang/01_velos_en_rang_hdbscan_K5_clair.mp4) ; K10 [sombre](01_velos_en_rang/01_velos_en_rang_hdbscan_K10_sombre.mp4) · [clair](01_velos_en_rang/01_velos_en_rang_hdbscan_K10_clair.mp4) |
| [02 vélos contre une façade](02_velos_contre_facade/) | 08/000882, sans sol | trois vélos contre un mur | **0,31** / **0,18** / 0,60 | **0,15** / **0,15** / 0,52 | 0,52 / **0,24** / 0,52 | K5 [sombre](02_velos_contre_facade/02_velos_contre_facade_hdbscan_K5_sombre.mp4) · [clair](02_velos_contre_facade/02_velos_contre_facade_hdbscan_K5_clair.mp4) ; K10 [sombre](02_velos_contre_facade/02_velos_contre_facade_hdbscan_K10_sombre.mp4) · [clair](02_velos_contre_facade/02_velos_contre_facade_hdbscan_K10_clair.mp4) ; ALPINE [sombre](02_velos_contre_facade/02_velos_contre_facade_alpine_bev_sombre.mp4) · [clair](02_velos_contre_facade/02_velos_contre_facade_alpine_bev_clair.mp4) |
| [03 piéton près d'une façade](03_pieton_contre_facade/) | 08/000048, sans sol | un piéton près d'une façade et d'un groupe, un piéton isolé, un vélo | **0,44** / 1,00 / 0,98 | **0,44** / 1,00 / 0,98 | 0,71 / 1,00 / 0,98 | K5 [sombre](03_pieton_contre_facade/03_pieton_contre_facade_hdbscan_K5_sombre.mp4) · [clair](03_pieton_contre_facade/03_pieton_contre_facade_hdbscan_K5_clair.mp4) ; K10 [sombre](03_pieton_contre_facade/03_pieton_contre_facade_hdbscan_K10_sombre.mp4) · [clair](03_pieton_contre_facade/03_pieton_contre_facade_hdbscan_K10_clair.mp4) |
| [04 vélos en rang, sol conservé](04_velos_en_rang_avec_sol/) | 08/001176, **sol conservé** | les trois vélos de 01 | **0,22** / **0,29** / **0,33** | **0,23** / **0,24** / **0,31** | (sans objet : ALPINE suppose le sol retiré) | K5 [sombre](04_velos_en_rang_avec_sol/04_velos_en_rang_avec_sol_hdbscan_K5_sombre.mp4) · [clair](04_velos_en_rang_avec_sol/04_velos_en_rang_avec_sol_hdbscan_K5_clair.mp4) ; K10 [sombre](04_velos_en_rang_avec_sol/04_velos_en_rang_avec_sol_hdbscan_K10_sombre.mp4) · [clair](04_velos_en_rang_avec_sol/04_velos_en_rang_avec_sol_hdbscan_K10_clair.mp4) |
| [05 témoin](05_temoin_voitures_en_file/) | 08/002554, sans sol | trois voitures garées à 0,7–1,2 m l'une de l'autre | 0,86 / 0,98 / 0,99 | 0,83 / 0,98 / 0,99 | 0,88 / 0,99 / 0,97 | K5 [sombre](05_temoin_voitures_en_file/05_temoin_voitures_en_file_hdbscan_K5_sombre.mp4) · [clair](05_temoin_voitures_en_file/05_temoin_voitures_en_file_hdbscan_K5_clair.mp4) ; ALPINE [sombre](05_temoin_voitures_en_file/05_temoin_voitures_en_file_alpine_bev_sombre.mp4) · [clair](05_temoin_voitures_en_file/05_temoin_voitures_en_file_alpine_bev_clair.mp4) |
<!-- catalogue:fin -->

**Bouts de scène où HGP réussit** : [`bouts_hgp/`](bouts_hgp/README.md) rassemble douze bouts SemanticKITTI réduits
aux seuls points de deux ou trois objets proches (vélos, vélos et piétons). Sur chacun, au même ordre k, la hiérarchie
de HDBSCAN manque un objet que la hiérarchie de points de HGP (morsehgp3D_v11) retrouve. Ce dossier, lui, montre des
résultats Morse HGP 3D, mesurés sur G4 (`public_status=not_claimed`) ; les démos 01 à 05 n'en montrent aucun.

Pour **tout** K de 1 à 10 (courbe du bilan de chaque vidéo), le vélo B de
01, les vélos A et B de 02, le piéton A de 03 et les trois vélos de 04
restent sous 0,5 : aucune valeur de `min_samples` ne les isole. Chaque dossier contient
ses vidéos dans les deux thèmes (`*_sombre.mp4`, `*_clair.mp4`), deux images
fixes par vidéo et par thème (`*_instant_cle.png`, l'instant clé : la
première fusion de branches suivies, la première absorption de fond pour 03
et 04, le dernier appariement pour 05 à K = 5, et pour 05 ALPINE la fusion de
deux voitures, sous le seuil voiture de l'article ; `*_bilan.png`, carte
finale), `demo.json` (la spécification et le texte) et
`resultats_*.json` (les nombres calculés, sans coordonnées).

## Ce qu'on voit dans une vidéo

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="01_velos_en_rang/01_velos_en_rang_hdbscan_K5_sombre_instant_cle.png">
  <img alt="Instant clé de la démo 01, K = 5" src="01_velos_en_rang/01_velos_en_rang_hdbscan_K5_clair_instant_cle.png">
</picture>

- **Introduction (4 s)** : la vérité terrain, en boîtes pointillées, et une
  courte rotation de caméra. La scène est tournée pour que les objets
  soient **alignés sur les axes**, la rangée le long de x et le capteur du
  côté du spectateur. Les boîtes sont donc des boîtes alignées sur les axes
  qui épousent les objets.
- **Balayage** : le niveau r de la hiérarchie croît en échelle
  logarithmique. Pour chaque objet on suit **la branche qui passe par son
  meilleur nœud**, à partir de son point le plus dense. On affiche sa boîte
  (trait plein), qui **grossit avec la hiérarchie**, et ses points. Le
  balayage marque une pause, avec une légende, à chaque événement :
  appariement (IoU > 0,5), absorption de points hors objet, fusion de deux
  branches suivies. Une branche fusionnée passe au rouge. Sa boîte est alors
  tracée en tirets espacés, aux couleurs mêlées des objets qu'elle contient et
  du rouge. Les points de l'objet qu'elle a pris sont **cernés** d'un anneau
  rouge, et les points de fond pris par une branche sont des **carrés
  creux** : rouges si la branche est fusionnée, de la couleur de l'objet
  sinon ; gris pour les points void, que la PQ ignore (ils ne comptent ni
  dans la précision ni dans l'IoU affichées). Chaque état se lit donc aussi
  par sa forme, pas seulement par sa couleur, ce qui compte pour les
  spectateurs daltoniens.
- **Panneaux de droite** : la vue de dessus (BEV) et la **hiérarchie
  restreinte aux trois branches**. Axe log r ; tirets = fragmenté ; trait
  épais = apparié ; double trait rouge = fusionné (curseur ✗) ; connecteurs
  verticaux = fusions de branches. Elle se dessine au fur et à mesure du
  balayage.
- **Bilan (8 s)** : meilleur nœud de chaque objet, meilleure coupe
  horizontale (combien d'objets un même r apparie à la fois), et la courbe
  du meilleur IoU pour HDBSCAN K = 1…10 et ALPINE (un marqueur par objet :
  disque A, carré B, triangle C).

## Méthodes montrées

- **HDBSCAN, arbre complet.** `min_samples = K` (K = 5 et 10, les ordres de
  la tour v9), `min_cluster_size = 1`. Avec 1, rien n'est condensé : on
  balaie l'arbre du lien simple de la distance d'atteignabilité mutuelle
  `max(core_K(a), core_K(b), |a - b|)`. Convention de K : celle de
  scikit-learn, **le point compté**, comme dans le plan de qualité du dépôt
  (`docs/PERFORMANCE_MORSEHGP3D.md`). Une boule de rayon `core_K(x)` contient
  alors K points, comme une boule d'ordre K de la tour. La bibliothèque
  `hdbscan` compte sans le point : elle reçoit `min_samples = K - 1`. Un
  niveau r de cet arbre est DBSCAN(eps = r, min_samples = K) sans ses
  points de bord. Toute extraction HDBSCAN (EOM, feuilles,
  `cluster_selection_epsilon`) rend des nœuds de cet arbre. K = 1 est le
  clustering euclidien 3D ; il figure dans la courbe du bilan.
- **ALPINE sans sémantique** (Sautier et al., 3DV 2026, `valeoai/Alpine`,
  commit `15d7fb3`). La méthode garde x, y (vue de dessus), relie chaque
  point à ses k = 32 plus proches voisins, garde les arêtes de longueur
  < t et prend les composantes connexes. Sans sémantique, il n'y a plus
  qu'une classe, donc un seul t, et le découpage par boîte, qui exige la
  classe, disparaît. Le sol doit être retiré (Patchwork++ ici). Balayer t
  donne une hiérarchie exactement emboîtée : l'arbre du lien simple du
  graphe kNN. Les seuils t de l'article (0,61 m vélo, 0,94 m piéton,
  1,8 m voiture) sont marqués sur l'axe. Le code public fixe k par
  mégarde à la dernière clé de classe ; nous prenons k = 32, la valeur de
  l'article.

Recherche bibliographique résumée : ALPINE (fig. 2 et 4 : voitures voisines,
échec de D&M), ElC-OIS (fig. 5 : vélo contre poteau, cycliste contre voiture,
voiture lointaine fragmentée, échecs de HDBSCAN, du clustering euclidien et
de CVC), 3DUIS (propositions Patchwork + HDBSCAN `min_cluster_size = 20`).
Aucun de ces articles ni de ces dépôts ne donne l'identifiant des trames de
ses figures. Seule 3DUIS livre une trame, 08/000000. La hiérarchie y réussit :
chacune des sept instances d'au moins 40 points a un nœud d'IoU ≥ 0,92, quel
que soit K (1, 5 ou 10). Ce n'est pas une trame difficile.
Les trames ci-dessus ont donc été **cherchées**, pas reprises.

## Comment les trames ont été trouvées

[`recherche/`](recherche/README.md) : criblage de 299 trames de la séquence
08 (une sur huit parmi celles qui ont au moins un vélo, une moto, un piéton,
un cycliste ou un motard, et deux voitures, de 60 points ou plus chacun), plus
117 trames autour des candidates. Chaque
trame est traitée entière, sans sol : de 32 000 à 101 000 points (126 000
avec le sol pour la démo 04), dans les tailles d'intérêt du dépôt. Le
critère est le meilleur IoU d'un nœud pour K = 1, 5, 10, au sens de la PQ.
Sur les instances d'au moins 50 points (le seuil `min_points` de la PQ), à
toutes distances :

<!-- stats:début -->
| classe | instances | aucun nœud > 0,5 à K = 5 | à K = 10 | pour K = 1, 5 et 10 |
| --- | ---: | ---: | ---: | ---: |
| voiture | 2 188 | 0 | 0 | 0 |
| piéton | 231 | 7 | 6 | 5 |
| autre véhicule | 140 | 2 | 2 | 2 |
| vélo | 138 | 22 | 36 | 17 |
| moto | 96 | 1 | 2 | 1 |
| cycliste | 94 | 1 | 1 | 0 |
| camion | 23 | 0 | 0 | 0 |
| motard | 8 | 0 | 0 | 0 |
<!-- stats:fin -->

À lire honnêtement : sur trame sans sol, l'arbre HDBSCAN **contient
presque toujours** les voitures (aucun échec sur 2 188), et le témoin 05 le
montre. Ses échecs se concentrent sur les petits objets au contact d'un
voisin : vélos garés (16 % à K = 5, 26 % à K = 10), piétons contre un mur ou
un groupe (3 %). Des trois cas de la figure 5 d'ElC-OIS, seul « vélo contre
une structure » se retrouve ici ; « cycliste contre voiture » et « voiture
lointaine fragmentée » n'échouent pas dans ce criblage (ElC-OIS mesurait
sous un masque sémantique). Garder le sol aggrave les échecs : dans la démo
04, les trois vélos échouent, contre un seul sans sol (démo 01).

## Limites

- La vérité terrain sert **seulement** à choisir et à noter les objets.
  Aucune hiérarchie ne la voit.
- Les « échecs » sont relatifs à l'annotation SemanticKITTI : une instance
  mal découpée ferait un faux échec. Chaque scène retenue a été vérifiée à la
  main, en examinant la composition de son meilleur nœud.
- Le meilleur nœud est une borne **optimiste** pour HDBSCAN : il suppose un
  oracle qui choisirait, objet par objet, le meilleur niveau. Toute
  extraction au même K, EOM compris, rend des nœuds de cet arbre : elle fait
  au mieux aussi bien. Le réglage de 3DUIS (`min_cluster_size = 20`, soit
  K = 21, sur un MST approché) n'est pas évalué ici.
- ALPINE sans sémantique n'est pas ALPINE : l'article n'en définit aucune
  variante sans classe, et la nôtre est la plus fidèle possible. Son arbre
  en vue de dessus n'est pas toujours moins bon que celui de HDBSCAN. En 01,
  il sépare les trois vélos vers t ≈ 0,1 m, six fois sous son seuil vélo.
  En 03, il isole le piéton A (0,71). Il n'y a donc de vidéo ALPINE que pour
  02, où il n'isole pas le vélo B, et pour le témoin 05, où son seuil
  voiture fusionne deux voitures.
- Une scène de piétons (08/000055) a été écartée : un piéton voisin y est
  annoté sans identifiant d'instance, et le meilleur nœud de HDBSCAN recouvre
  presque exactement leur union. Une autre vue de la démo 03 (08/000058) a
  été abandonnée : au sens de la PQ, le piéton y est apparié de justesse.
  Voir [`recherche/`](recherche/README.md).
- Une trame, un instant : aucune conclusion sur la séquence ni sur la
  fréquence des échecs au-delà du tableau ci-dessus.

## Insérer dans une présentation

Les vidéos sont en **MP4 H.264** (profil High, yuv420p, 1920 × 1080,
30 i/s, sans son, `+faststart`), le format lu partout : PowerPoint, Keynote,
Google Slides, LibreOffice Impress, navigateurs. Chacune dure 38 à 57 s ;
les pauses sont intégrées, inutile de toucher au lecteur.

Deux thèmes, comme Percolia.com, avec les mêmes couleurs de fond et de
texte : `*_sombre.mp4` (fond marine `#071b2e`) pour des diapositives
sombres, `*_clair.mp4` (fond blanc) pour des diapositives claires, dont le
thème Inria de `PolyhedralEncoding/`. Les couleurs des objets A (bleu),
B (ambre), C (vert d'eau) et de la fusion (rouge) sont réglées par thème :
contraste d'au moins 3:1 sur les panneaux et écart CIEDE2000 d'au moins 20
entre elles, en vision normale comme en deutéranopie et en protanopie
simulées. `tools/test_scene_contract.py` le vérifie.

Beamer (LuaLaTeX, comme `PolyhedralEncoding/`) : le PDF ne contient pas la
vidéo, il la lance dans le lecteur externe. On garde l'image fixe comme
affiche et comme repli pour l'impression :

```latex
\usepackage{multimedia}
% ...
\movie[width=\textwidth,height=0.5625\textwidth,externalviewer]
  {\includegraphics[width=\textwidth]{demos/01_velos_en_rang/01_velos_en_rang_hdbscan_K5_clair_instant_cle.png}}
  {demos/01_velos_en_rang/01_velos_en_rang_hdbscan_K5_clair.mp4}
```

Pour une démonstration en direct, [`player/index.html`](player/index.html)
lit les mêmes scènes, avec lecture, pause (espace) et curseur :
`player/index.html?scene=../01_velos_en_rang/data/scene_hdbscan_K5.js`.
Comme sur Percolia.com, le bouton rond en haut à droite bascule le thème
(☀️ vers le clair, 🌙 vers le sombre). Le thème sombre est celui par défaut,
et le choix est mémorisé dans le navigateur. `&theme=clair` ou
`&theme=sombre` dans l'adresse impose un thème sans le mémoriser.
Les scènes sont régénérées localement, voir plus bas.

## Reproduire

Dépendances : Python 3.11 avec `numpy`, `scipy`, `hdbscan` (0.8.44 testé),
`pypatchworkpp` (1.4.1 testé) et `imageio-ffmpeg` ; Node.js avec Playwright et
son Chromium pour le rendu.

```bash
pip install numpy scipy hdbscan pypatchworkpp imageio-ffmpeg
python3 Zoltan/demos/tools/build_scene.py Zoltan/demos/01_velos_en_rang      # trame lue à distance, scènes + resultats_*.json
node Zoltan/demos/tools/render_video.cjs Zoltan/demos/01_velos_en_rang hdbscan_K5   # Playwright + ffmpeg, thèmes sombre et clair
python3 -O -m unittest discover -s Zoltan/demos/tools -p 'test_*.py'          # oracles bornés
python3 Zoltan/demos/tools/search_frames.py --seq 08 --step 8 --out criblage.jsonl
```

`tools/kitti.py` lit les trames dans les archives officielles (KITTI
odometry velodyne, 84,8 Go ; labels SemanticKITTI, 179 Mo) par requêtes
HTTP partielles, sans les télécharger ; `demo.json` épingle le sha256 de
chaque trame. `tools/ground.py` reprend exactement les paramètres
Patchwork++ de la v8 : sur 08/000000 le masque est identique octet pour
octet à celui de `morsehgp3D_v8/receipts/lidar_ground_20260921/`, et un
test le vérifie quand cette trame est dans le cache local. `tools/hierarchy.py` porte l'arbre, le meilleur nœud et
le suivi des branches ; ses oracles (Prim dense, énumération de tous les
nœuds) sont dans `tools/test_hierarchy.py`. Le rendu est déterministe : une
image ne dépend que du temps, de la scène et du thème. Les scènes ne portent
que des rôles de couleur (objet A, B, C, fusion, texte) ; `player/player.js`
les résout dans la palette du thème.

## Licences

Code et texte : MIT, comme la racine. Les **vidéos et images** sont des
œuvres dérivées de KITTI (CC BY-NC-SA 3.0) et de SemanticKITTI (CC BY-NC-SA
4.0). Elles restent non commerciales, partagées dans les mêmes conditions,
avec attribution : Geiger et al., CVPR 2012 ; Behley et al., ICCV 2019.
Aucun octet KITTI, aucune coordonnée de point n'est versionné : `_cache/`
et `*/data/` sont ignorés par git. ALPINE est réimplémenté d'après
l'article, sans importer son code. La police Inter de la page du lecteur
(`player/fonts/`, sous-ensemble latin) est sous licence SIL OFL 1.1, jointe
(`player/fonts/OFL.txt`) ; les vidéos ne l'utilisent pas.
