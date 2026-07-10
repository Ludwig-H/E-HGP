# Backend `power_cover_3d_cuda`

## Contrat mathématique

Pour chaque observation (x_i\in\mathbb R^3), le backend résout sur un support local dur de taille `m_reg` :

\[
q_i^\kappa\in\arg\min_{q\in\Delta}
\sum_j q_j\lVert x_i-x_j\rVert^2,
\qquad H(q)\ge\log\kappa.
\]

Il construit ensuite :

\[
z_i=\sum_jq_{ij}x_j,
\qquad
a_i=\sum_jq_{ij}\lVert x_j-z_i\rVert^2,
\]

et la distance de puissance additive

\[
d_i^\kappa(y)^2=\lVert y-z_i\rVert^2+a_i.
\]

Le champ d’intérêt est le rayon

\[
g(y)=r_{K,\kappa}(y)
=\sqrt{K\text{-ième plus petite valeur de }
\{\lVert y-z_i\rVert^2+a_i\}_i}.
\]

Les trois paramètres ne sont pas interchangeables :

- `K` définit l’ordre de la couverture et vaut 10 pour la cible ;
- `kappa` fixe l’entropie effective ;
- `m_reg` fixe le support géométrique dur du problème entropique.

Avec

\[
s_i=\sqrt{\lVert x_i-z_i\rVert^2+a_i},\qquad s=\max_i s_i,
\]

le champ régularisé et le champ K-NN original sont `s`-entrelacés en rayon.

## Garde top-K de puissance

Random Ball Cover fournit les (C+1) voisins euclidiens d’une requête (y) parmi les
centres (z_i), sous le contrat algorithmique de cuML qui est contrôlé empiriquement à
l'exécution contre cuVS brute force. Les (C) premiers sont évalués avec leurs poids.
Si `guard` désigne le suivant, tout site omis vérifie en arithmétique exacte :

\[
\lVert y-z_i\rVert^2+a_i
\ge d_{\rm guard}(y)^2+\min_j a_j.
\]

Le K-ième candidat est accepté sous l'enveloppe numérique float32 déclarée lorsque sa
borne supérieure est sous cette borne inférieure. Cette condition n'est pas présentée
comme une preuve par arithmétique dirigée. Sinon, (C) double jusqu’au plafond configuré.
Au plafond :

- mode strict : exception `PowerKNNCertificationError` ;
- mode non strict : valeur accompagnée de `certified=False`.

Le notebook audite en plus les valeurs RBC sur un échantillon contre le brute force
exact de cuVS. Cette vérification empirique protège contre une régression ou une
incompatibilité de bibliothèque, mais ne prouve ni les requêtes non échantillonnées ni
la borne d'erreur flottante.

## Filtration cubique

Pour un cube (Q), de centre (c_Q) et de demi-diagonale (h_Q), la 1-lipschitzianité de (g) donne :

\[
g(c_Q)-h_Q\le g(y)\le g(c_Q)+h_Q.
\]

L’implémentation ajoute une enveloppe globale `delta_num` pour l’évaluation float32 des centres. Sur une grille uniforme, les activations sont donc :

```text
outer_Q = g(c_Q) - H - delta_num
inner_Q = g(c_Q) + H + delta_num
```

La borne totale vis-à-vis du champ K-NN original est :

\[
s+2(H+\delta_{\rm num}).
\]

Les cubes sont fermés. Deux cubes qui se touchent par une face, une arête ou un sommet sont adjacents : la connexité est 26-connexe en 3D, 8-connexe sur un nuage plan et 2-connexe sur une ligne.

## MSF cubique implicite

Une arête de cubes ((u,v)) naît au poids

\[
w(u,v)=\max(g_u,g_v).
\]

Un arbre couvrant minimal conserve les composantes à tous les seuils. La référence CPU traite implicitement les voisins dans l’ordre total `(poids, edge_id)`. Le chemin GPU utilise Borůvka avec des kernels CuPy `RawKernel` : chaque sommet inspecte seulement les 13 directions canoniques, chaque composante choisit sa meilleure arête, puis une Union-Find par CAS contracte les composantes.

Pour `256^3` cubes :

- 16 777 216 sommets ;
- 216 338 940 arêtes implicites ;
- environ 256 Mio de résultat compact ;
- environ 448 Mio de pic device modélisé pour le MSF ;
- environ 3,22 Gio de liste explicite évités.

Les deux forêts intérieure et extérieure partagent la même topologie sur une grille uniforme : elles sont obtenues par translation constante des naissances et fusions. Cela évite de lancer Borůvka deux fois.

## Mémoire cible 30 M

Avec la configuration par défaut `m_reg=64`, chunks de régularisation de 262 144,
chunks de puissance séparés de 65 536 et candidats plafonnés à 1 024, l’estimateur
analytique compte environ :

- 3,02 Gio persistants, y compris l'entrée appelant, sa copie normalisée et une
  provision de 64 octets par point pour l’index spatial ;
- 0,95 Gio de buffers connus pour un chunk de régularisation ;
- 1,26 Gio au plafond du chunk de puissance, grâce au kernel fusionné qui ne crée
  aucun tenseur de coordonnées `Q x C x 3` ;
- 4,27 Gio de pic analytique conservatif pour les tableaux explicitement modélisés.

Ce chiffre n’est pas une mesure. La coexistence de l'entrée float32 et de sa copie
normalisée est incluse, mais une entrée float64 ajoute 12 octets par point. Les
buffers internes RAPIDS/CUB, l'espace de travail de partition, RMM, NVRTC et le driver
ne sont pas entièrement modélisés et doivent être mesurés. Le notebook refuse une
estimation supérieure à 85 % de la VRAM libre, partage l'allocateur RMM entre CuPy et
cuML et enregistre le pic NVML.

## Sorties

`PowerCoverHierarchy` contient :

- les sites `(Z, a, s_i)`, les résidus entropiques agrégés et la taille de
  l'échantillon déterministe utilisé pour les quantiles de `s_i` (maximum exact) ;
- le champ cubique et les statuts de certification ;
- un MSF de base ;
- les MSF intérieur et extérieur ;
- les intervalles de fusion lorsque toutes les requêtes passent l'enveloppe déclarée ;
- un `AccuracyReport` machine-lisible ;
- les temps synchronisés par étage et le budget mémoire analytique.

Les labels de cubes à une coupe sont disponibles. En revanche, le backend ne prétend pas encore produire l’appartenance canonique des 30 M observations. Celle-ci exige de certifier l’intersection entre chaque boule de puissance et une composante continue/cubique. Une affectation par cellule serait un post-traitement heuristique et doit rester déclarée comme telle.

## Validation locale

La suite CPU couvre notamment :

- Gibbs, limites `kappa=1` et `kappa=m_reg`, minima ex aequo et invariance d’unités ;
- régularisation indépendante des chunks ;
- top-K de puissance contre brute force bloqué et escalade du garde ;
- oracle HGP exhaustif sans filtre Gabriel ;
- min-ball additive et résidus primal/dual ;
- équivalence `K=1` avec Single-Linkage au rayon distance/2 ;
- MSF cubique contre composantes brute force à tous les seuils ;
- 26-connexité, dimensions dégénérées, égalités et déterminisme ;
- covariances par translation et homothétie ;

La construction du wheel et la présence des sous-packages font l'objet d'une
validation de packaging séparée de la suite CPU.

Le kernel CUDA et le débit 30 M ne sont pas validés localement, faute de GPU NVIDIA. Leur statut dépend des cellules d’acceptation du notebook Blackwell.
