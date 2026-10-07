# Réduction filtrée certifiée de A_k et budget géométrique : expérience

6 octobre 2026, de 22 h 46 au 7 octobre 00 h 15 UTC environ (heures lues par `date -u`). Rôle : expérimentateur « réduction
filtrée certifiée et budget géométrique » du workflow « généraliser le complexe alpha à l'ordre K ».

```text
phase=exploration_v11_hors_registre
backend=cpu_reference (prototype Python en aval de la tour, au plus 2 fils, nice 10)
profile=quantized_u21_input_only (grille de 1 mm)
public_status=not_claimed
```

GCP non utilisé. Aucun commit, aucun push, aucune écriture hors de ce dossier (le harnais, `approche_mosaique` et le
cache du harnais sont importés ou lus, jamais modifiés ; la permutation de stabilité est faite en mémoire). Aucune
donnée SemanticKITTI hors de `build/`. Dossier : 62 Mo.

Étiquettes : [P] prouvé (preuve rédigée par l'auditeur ou ici) ; [V] vérifié borné (petit nuage, arithmétique exacte) ;
[M] mesuré (fichier cité) ; [E] estimé (échantillonnage, borne inférieure) ; [C] conjecture.

## 0. En bref

1. **Le prototype recommandé par l'auditeur (28d70f8ab) fonctionne et se certifie.** Sur 8 jeux × 4 ordres
   (k = 1, 2, 3, 5 ; k = 10 sur l'anneau de 52 points), plus six points, deux contre-épreuves et une échelle k = 1 / k = 2, chaque mosaïque passe un
   contrôle STRICT de complétude, chaque journal de paires est accepté par un vérificateur global indépendant
   (dates exactes rationnelles de toutes les paires des journaux de référence), et tous les mutants sont refusés
   par la vérification complète ; un mutant de dates passe un ÉCHANTILLON de 50 dates : un échantillon de dates est
   un juge, pas un certificat [M].
   k = 1 redonne exactement le complexe alpha (gudhi, précision exacte : mêmes simplexes, niveaux à 1 ulp) [M].
2. **La réduction est réelle mais bornée par les sommets, et plus faible sur LiDAR à l'échelle.** Complexe entier,
   petits jeux : L/K = 0,14 à 0,31 (facteur 3 à 7), à 1,2 à 4 fois le plancher 2V/K imposé par la protection des
   sommets ; boules de la trame 08/000000 de 8 000 à 32 000 sites : L/K = 0,44 à 0,53 à k = 1 et 0,38 à 0,47 à
   k = 2 (facteur 2 à 2,6) [M]. Par nœud et par niveau, là où la question se pose : |L_{r,v}| / |A_{k,v}(r)| médian 0,36
   à 1,00 (facteur 1 à 3) [M]. À k = 5, L garde encore 77 à 190 cellules par site et des milliers de cellules par
   nœud d'objet ; à k = 2 sur LiDAR, 54 à 67 par site.
3. **Elle ne réduit pas le rendu.** Les « faces exposées + strates isolées » de L valent 0,55 à 2,7 fois celles de
   A (médiane 0,98 sur 41 nœuds d'objet au milieu de leur vie) : l'effondrement transforme un solide en une « âme »
   de triangles et d'arêtes libres, qu'il faut toutes dessiner [M]. Pour un jeton compact, la réponse est **non** : la réduction
   certifiée à sommets protégés ne suffit pas.
4. **Les garanties de l'auditeur se vérifient sans exception** sur 876 couples (nœud, niveau) : d_H(L, A)
   estimé <= D_v(r) (rapport médian 0,04 à 0,42) ; D_v(r)/r <= 4/k (et <= 2 à k = 1) ; sup_{y ∈ C_v} d(y, L) <= r ;
   nombres de Betti de L égaux à ceux de A (873 couples mesurés, dont 673 non triviaux, b1 jusqu'à 15) après
   correction d'une faute de MESURE (§ 3.3) ;
   diagrammes de persistance identiques sur toute la filtration, jusqu'à 32 000 sites ; couverture exacte par les
   labels identique ; emboîtement en r et vers le parent [M]. Ce sont des théorèmes [P] ; les mesures n'y ajoutent
   qu'une absence de faute d'implantation.
5. **Les variantes ne changent presque rien, sauf la protection partielle.** Les priorités (dimension croissante,
   grand diamètre, labels seuls) donnent L à ±3 % de la référence. La protection partielle ancrée (sommet retiré
   seulement avec son arête de même naissance, ancré à un sommet gardé à <= r/2, sans cascade) retire 21 à 45 % des
   sommets à k >= 2 (aucun à k = 1), gagne 9 à 37 % de taille, coûte une borne d_H <= D_v + r/2 (vérifiée) et la
   perte de la couverture portée par les sommets (aucun site perdu ici, mais rien ne le garantit) [M].
6. **La forme n'est pas contrôlée par l'homotopie** : sur les vélos, la part du disque d'ouverture de la roue
   couverte par L tombe jusqu'à 47 points sous celle de A, sans aucun changement de nombres de Betti : un disque
   plein s'effondre en arbre. Le dual réduit peut « ouvrir » visuellement une roue que la tour dit pleine [M].

## 1. Ce qui a été implanté (exactement le prototype de l'auditeur)

Fichiers : `noyau.py` (complexe, contrôle strict, effondrements, sphères exactes, vérificateur, arbre des
composantes), `campagne.py` (jeux, nœuds, niveaux, mesures, rendus), `fixtures.py` (contre-épreuves gravées,
oracle 2D exact, six points, F4), `stabilite.py`, `echelle.py`, `verif_dates_echelle.py`, `persistance.py`,
`persistance_echelle.py`, `betti_cellulaire.py`, `temoin_qp.py`, `synthese.py`, `montage.py`, files `file_*.sh`.
Résultats :
`resultats/*.json`, `resultats/*_journal_reference.npy` (journaux des paires), `TABLEAUX.md`, `png/`, `planches/`,
`journaux/`.

- **Mosaïque** : `approche_mosaique/mosaique.construire` (importée, non modifiée) sur le jeu ENTIER, ordre k :
  cellules P_{I,U,k}, faces de toutes dimensions identifiées par leurs labels (I', U'), niveaux R = a_σ (rationnels
  exacts arrondis correctement), incidences 3→2, 2→1, 1→0. En aval de la tour, borné ; jamais dans le calcul de la
  hiérarchie. Coût déclaré au § 5.
- **Contrôle STRICT de complétude** (`noyau.controle_strict`), plus fort que le certificat de volume qui avait laissé
  passer le trou de 29 220 mm^3 : (1) chaque 2-face borde 1 ou 2 cellules ; (2) une 2-face intérieure a ses deux
  cellules de part et d'autre de son plan (orientation entière exacte) ; (3) une 2-face de bord est sur le bord de
  l'enveloppe des barycentres (tous les sommets d'un même côté, entiers) ; (4) rapport des volumes dans ]0,75 ; 1,25[.
  (1)-(3) rendent le degré du recouvrement constant, (4) le fixe à 1 : sous-division sans trou ni recouvrement.
  Résultat : **complet sur toutes les mosaïques construites** [M].
- **Effondrements** (`noyau.effondrer`) : paires (σ, τ), σ facette de τ, σ libre dans le complexe ENTIER courant
  (une seule cofacette vivante ; τ alors maximale par la propriété du losange, contrôlée), a_σ = a_τ ; tous les
  sommets protégés ; tas de priorité (−dim τ, diamètre de τ, identifiant de τ, identifiant de σ) ; l'identifiant
  d'une face est son rang dans le tri lexicographique des clés (dim, |I'|, I', U') : « labels exacts triés ».
  Comme les paires ont la même naissance et que la liberté est prise dans le complexe entier, la même suite est une
  suite d'effondrements de A_k(r) sur L_r = L ∩ A_k(r) **pour tout r** [P, auditeur § 3 et 4.2].
- **Vérificateur global indépendant** (`noyau.Verificateur`) : il ne lit que les labels (I', U'), les dimensions et
  les coordonnées entières ; il recalcule (V1) les sommets de chaque cellule par les seuls labels (0 écart avec la
  construction sur toutes les mosaïques), (V2) valide chaque incidence par inclusion des ensembles de sommets et le
  nombre de facettes par forme, (V3) rejoue le journal : unicité, σ facette de τ, toutes deux vivantes, σ libre au
  sens fort (aucune AUTRE cellule vivante de toute dimension ne contient ses sommets), sommets protégés,
  (V4) **dates exactes** : a_f = min des niveaux propres admissibles des cellules contenant f, sphères entières
  recalculées par un autre algorithme (système de Gram et adjointe, pas le produit vectoriel de la construction),
  admissibilité exacte (I' dedans ou dessus, aucun autre site strictement dedans, test entier), égalité en
  `Fraction`, (V5) fermeture par faces (incidences validées, et aucune cellule retirée n'est contenue dans une
  vivante), (V6) acyclicité (composantes fortement connexes du diagramme de Hasse modifié).
  Limite d'indépendance : la formule a_σ = min des niveaux propres admissibles des cofaces est la même que celle de
  la construction (déduite de l'audit) ; elle est corroborée à k = 1 par gudhi, à k >= 2 par la bijection des
  composantes avec FULL (mhgp11) et par un témoin numérique de minimisation convexe directe (§ 7), pas par un
  certificat exact à multiplicateurs.
- **Nœuds** : arbre des composantes de A_k(r) (coupes fermées, plateaux en bloc, convention FULL), contrôlé contre
  la tour FULL en cache quand elle existe : **0 écart de nombre de composantes à chaque niveau** sur toutes les paires
  (jeu, k) où une sortie supports d'ordre k existait [M]. Sélection oracle (vérité terrain, borne et non méthode) :
  meilleur nœud par objet et par partie (IoU des sites couverts par les labels à la fin de vie), chaîne d'ancêtres
  de la partie jusqu'à l'objet (au plus 6 intermédiaires), parent du nœud d'objet ; au plus 14 nœuds par jeu
  (8 au-dessus de 300 000 cellules). Niveaux : naissance (coupe fermée), milieu géométrique sqrt(r_b r_d), juste
  avant la mort (coupe OUVERTE a_σ < d_v^2, auditeur § 5) ; racine : borne déclarée 2 r_b.
- **Attribution** : composante = sommets reliés par les arêtes actives au sommet représentant du nœud ; cellules
  par leurs sommets ; jamais par position.

## 2. Contre-épreuves gravées (fixtures, coordonnées exactes) — `fixtures.py`, `resultats/fixtures.json`

- **F1, A=(0,6), B=(4,6), C=(2,7), D=(2,0), k = 1** [V] : oracle 2D exact : a(AB) = a(ABC) = 25/4, a(ABD) = 100/9.
  L'algorithme global ne propose AUCUNE paire (ABD critique bloque AB). Une réduction faite au seul niveau
  r^2 = 25/4 propose (AB, ABC) ; le vérificateur global la REFUSE (`non_libre` : AB est encore facette de ABD).
- **F2, chaîne de 8 segments de 10 mm, toutes cellules nées au même niveau** [V] : sans protection, la cascade
  atteint d_H(L, A) = 80 mm = N l ; sommets protégés : 0 paire ; protection partielle ancrée (θ r = 20 mm >= l) :
  deux sommets retirés aux deux bouts, d_H = 10 mm = l, pas de cascade.
- **F3, six points du § 6.1** (deux triangles quasi équilatéraux de côté 2 m sur la grille de 1 mm, 1732 = arrondi
  de 1000 √3 ; plan, oracle 2D exact, plongé dans le plan vertical pour le rendu) [V] : à k = 2, sept nœuds
  naissent vers r = 1 m (les sept segments du manuscrit, Fig. 6.2 ; la grille sépare 999,98 et 1000,00 mm), puis
  {A,B,C} et {D,E,F} à r = 1154,7 mm (2r/√3) coexistent avec {C,D} jusqu'à 1931,8 mm : exactement les trois
  2-polyèdres de la Fig. 6.5. Juste avant la fusion générale, A_2 = deux triangles et un sommet ; L = deux chemins
  de deux arêtes et un sommet ; l'**ombre de couverture** redonne conv{A,B,C}, [C,D], conv{D,E,F}, qui se touchent
  en C et en D sans fusionner (`planches/planche_six_points_k2.png`). Vérificateur : accepté à k = 1, 2, 3.
- **F4, faute de mesure gravée** (pas une contradiction mathématique) : deux pyramides à base carrée collées par leur
  base, triangulées indépendamment avec deux diagonales différentes du carré commun : gudhi trouve b2 = 1 (le bord
  du tétraèdre abcd est dans la réunion) ; l'homologie cellulaire Z/2 du complexe polyédral donne (1, 0, 0, 0).
  C'est le mécanisme des six faux écarts de Betti du piéton (§ 3.3).
- **Mutants** (sur chaque jeu, `res['mutants']`) : paire de dates inégales où σ est libre (seule erreur : `dates` ;
  vérifiée sur toutes les dates), paire non libre, sommet protégé retiré, paire hors facette (arête, tétraèdre) :
  **refusés sur tous les jeux** [M]. Le cinquième, une paire de bord de dates inégales préfixée au journal et
  vérifiée sur un ÉCHANTILLON de 50 dates, **est passé une fois** (vélo à 5 m, k = 1) : la paire fautive n'était pas
  dans l'échantillon et le reste du journal restait valide. Refusé par la vérification de toutes les dates
  (`resultats/mutant_dates_velo05m_k1.json`). Leçon : seules les vérifications à dates complètes sont des
  certificats ; c'est le cas de tous les journaux de référence des jeux et, après coup, des boules LiDAR (§ 5),
  sauf 16 000 et 32 000 sites à k = 2 (échantillon de 20 000 dates).

## 3. Résultats (tableaux complets : `TABLEAUX.md`)

### 3.1 Taille : la réduction obtenue (la question posée)

Complexe ENTIER (la plage de filtration entière), référence, tous sommets protégés [M] (`TABLEAUX.md`, tableau 1) :

| k | L/K (min–max sur les jeux) | plancher 2V/K | gardé : arêtes / 2-faces / 3-cellules | L par point du nuage | sommets de K par point |
| --- | --- | --- | --- | --- | --- |
| 1 | 0,23 – 0,31 | 0,07 – 0,11 | 0,39–0,47 / 0,14–0,25 / 0,00–0,11 | 4 – 8 | 1 |
| 2 | 0,18 – 0,25 | 0,11 – 0,13 | 0,27–0,35 / 0,07–0,16 / 0,000–0,071 | 16 – 32 | 5,5 – 7,3 |
| 3 | 0,16 – 0,22 | 0,11 – 0,12 | 0,24–0,30 / 0,05–0,12 / 0,000–0,057 | 34 – 72 | 12 – 19 |
| 5 | 0,14 – 0,19 | 0,11 – 0,12 | 0,21–0,27 / 0,03–0,09 / 0,000–0,038 | 77 – 190 | 31 – 61 |

- Sur ces petits jeux, presque tous les tétraèdres et octaèdres s'effondrent (il en reste 0 à 11 % ; 21 à 38 % sur
  les boules LiDAR du § 5) ; il reste surtout les sommets (tous), un quart des arêtes et quelques pour cent des
  2-faces. À k = 5, L n'est plus qu'à 1,2 à 1,7 fois le plancher :
  **la protection des sommets est la contrainte dominante**, et le nombre de sommets de la mosaïque croît comme
  k n environ (31 à 61 par point à k = 5).
- Par nœud et par niveau (meilleurs nœuds oracle d'objets et de parties, leurs ancêtres, trois niveaux) :
  |L_{r,v}| / |A_{k,v}(r)| médian 0,65 à 1,00 à k = 1 (les composantes vivantes y sont surtout des graphes : rien à
  effondrer), 0,51 à 0,95 à k = 2, 0,44 à 0,85 à k = 3, 0,36 à 0,49 à k = 5 [M]. Le nœud d'objet au milieu de sa vie
  garde des milliers de cellules : vélo à 10 m, k = 5, 43 526 → 14 624 ; piéton à 5 m, k = 3, 132 979 → 34 489 ;
  vélo réel (découpe), k = 5, 41 955 → 17 537.
- Coût du rendu « faces exposées + strates isolées » du nœud d'objet au milieu de sa vie (tableau 5 de
  `TABLEAUX.md`) : primitives dessinées A → L de 0,55 à 2,7 fois (médiane 0,98 sur 41 nœuds) ; par exemple vélo à 10 m, k = 5 : 4 615 → 4 884 ;
  piéton, k = 3 : 6 068 → 11 688 ; vélo réel, k = 5 : 2 980 → 5 803 ; anneau à 5 m, k = 5 : 1 717 → 1 016 [M]. Les
  faces exposées du solide A disparaissent, remplacées par des 2-faces et arêtes LIBRES (une « âme » de dimension 2
  et 1) qu'il faut toutes dessiner. **La réduction des cellules ne réduit pas le rendu.**

### 3.2 Géométrie (R3) : les bornes de l'auditeur tiennent, avec de la marge

- D_v(r)/r (diamètre maximal exact des cellules de la composante, sommes entières) : maximum 2,00 à k = 1, 1,88 à
  k = 2, 1,29 à k = 3, 0,77 à k = 5, 0,396 à k = 10 ; **aucun dépassement de 4/k (2 à k = 1)** sur les 876 couples,
  cellules dégénérées comprises [M]. La borne
  générique 4r/k est donc presque atteinte : ce n'est pas une borne lâche sur ces nuages.
- d_H(L_{r,v}, A_{k,v}(r)) estimé (borne inférieure : centres et points aléatoires de toutes les cellules retirées,
  distances exactes en double au polyèdre de L) : toujours <= D_v(r) ; rapport médian d_H/D_v de 0,04 à 0,42 ;
  d_H/r au plus 0,92 à k = 1, 0,48 à k = 2, 0,33 à k = 3, 0,19 à k = 5 et 0,06 à k = 10 [E].
- sup d(y, L_{r,v}) / r sur des échantillons de C_v(r) (tirés à moins de r des sommets, retenus si d_k(y) <= r et
  si leur k-ensemble de plus proches voisins est un sommet de la composante : attribution par label) : <= 1 partout
  (égalité 1,00 à k = 1, cas limite), médiane 0,54 à 1,00 [E]. Conforme à d_H(L, C) <= r sans terme additionnel.

### 3.3 Topologie (R1), emboîtement (R2), couverture

- **Nombres de Betti égaux** pour A_{k,v}(r) et L_{r,v} sur tous les couples mesurés (873 ; 3 omis au-delà de
  250 000 cellules, borne de coût déclarée), dont 673 non triviaux (b1 jusqu'à 15 sur le vélo, b2 > 0) [M].
  Homologie cellulaire Z/2 pour 17 couples (jeu, k) sur 33, homologie triangulée (gudhi) pour les 16 autres,
  égale dans tous les cas après la correction ci-dessous. Six couples (piéton, k = 2 et 3) avaient d'abord donné A = (1, 0, 1),
  L = (1, 0, 0) : **faute de MESURE**, pas de la réduction. La triangulation pour gudhi découpait deux cellules
  dégénérées (polytopes à 5 sommets et plus) voisines par des Delaunay indépendants, d'où une sphère parasite le long
  d'une facette quadrilatère commune. Refait en homologie CELLULAIRE sur Z/2 (bord = somme des facettes, aucune
  triangulation, `betti_cellulaire.py`) : A = L = (1, 0, 0, 0) dans les six cas, puis sur les 84 couples du piéton
  relancés à k = 2 et 3 [M]. Les relances utilisent désormais cette homologie cellulaire ; fixture F4.
- **Diagrammes de persistance identiques** (H0, H1, H2, gudhi, filtration par a_σ, toute la plage de r) entre la
  mosaïque complète et L sur 8 jeux × ordres (anneau à 10 m k = 1, 2, 5 ; vélo à 10 m k = 1, 2, 3 ; découpe k = 2 ;
  anneau à 5 m k = 3 ; jusqu'à 797 paires H1 et 210 paires H2) et, à l'échelle, sur les boules LiDAR de 8 000 et
  32 000 sites à k = 1 (complexe simplicial, sans triangulation ajoutée : 13 350 et 51 476 paires H1, 3 997 et
  15 354 paires H2, `resultats/persistance_echelle_*.json`) [M] : l'équivalence filtrée certifiée se voit sur toute
  la filtration, pas seulement aux coupes choisies.
- Emboîtement : L_v(naissance) ⊆ L_v(milieu) ⊆ L_v(avant mort) ⊆ L_parent(naissance) vrai sur tous les contrôles [M]
  (L est fixe et A_k(r) croît : c'est une conséquence, pas une propriété à espérer).
- Couverture exacte par les labels des sommets de L = celle de A sur tous les couples [M] (sommets protégés).
- Composantes : 0 écart avec la tour FULL (mhgp11) sur toutes les paires (jeu, k) où sa sortie d'ordre k existait.

### 3.4 Ouvertures : l'homotopie ne contrôle pas la forme

Part du disque d'ouverture (roues synthétiques, 0,24 m) à moins de 10 mm du polyèdre : celle de L est toujours <=
celle de A (L ⊂ A), mais l'écart atteint 0,47 (vélo à 10 m, k = 1), 0,45 (vélo occulté), 0,29 à 0,38 à k = 2 et 3,
sans aucun changement des nombres de Betti [M]. Un disque rempli dans A s'effondre en un arbre d'arêtes dans L :
L « ouvre » visuellement la roue alors que la région dense ne l'ouvre pas. C'est le contre-exemple concret à
l'emploi du dual réduit pour juger une ouverture (le verdict d'ouverture doit se lire sur H1 de A, ou sur A).

### 3.5 Stabilité (R4)

`stabilite.py`, `resultats/stabilite_*.json` (anneaux à 5 et 10 m k = 2, vélo à 10 m k = 2 et 3) [M] :

- **Réétiquetage** (permutation des points en mémoire, mêmes PointId) : la mosaïque est identique (mêmes cellules
  par familles de PointId), L ne l'est pas : Jaccard 0,91 à 0,97, et sa taille peut changer de quelques cellules
  (vélo, k = 2). Le départage par labels triés est reproductible, pas canonique, comme l'auditeur l'annonçait ; les
  garanties (homotopie filtrée, bornes) ne dépendent pas de ce choix.
- **Déplacement apparié** (chaque point bouge d'au plus 1 mm par coordonnée, δ <= √3 mm, sans fusion de sites) :
  distance d'étranglement des diagrammes H0 entre 0,99 et 1,48 mm, toujours <= δ (théorème de stabilité, ici
  vérifié) ; L/K varie de moins de 0,005. Le nombre de composantes à un niveau FIXE n'est le même qu'à 77 à 87 % des
  niveaux critiques : la stabilité est un entrelacement en r, pas une identité à r fixé. La géométrie du dessin
  n'a pas été comparée (pas de borne attendue, auditeur § 1.1).


### 3.6 Rendus nommés (même caméra par jeu : tous les points du jeu en fond, vue tirée de leur seule ACP)

`planches/planche_<jeu>_k<k>_objet<o>.png` : en haut à gauche A_k (faces exposées et strates isolées), en haut à
droite **dual réduit** L (sommets protégés), en bas à gauche L à protection partielle, en bas à droite **ombre de
couverture** des labels de L (conv de la réunion des Q des sommets de chaque cellule maximale). Constats [M] :
le dual réduit est une « feuille » de triangles qui garde le contour (tous les sommets) et les trous de H1, mais
perd le remplissage et ouvre des jours sans trou topologique ; l'ombre est un solide épais, sans garantie
d'homotopie, qui redonne exactement les sites couverts. Six points (`planches/planche_six_points_k2.png`) :
l'ombre redonne les trois 2-polyèdres de la Fig. 6.5 du manuscrit, qui se touchent sans fusionner.


## 4. Variantes : ce que chacune gagne ou perd

| Variante | Taille du complexe entier (écart relatif à la référence) | Par nœud | Ce qu'elle gagne | Ce qu'elle perd |
| --- | --- | --- | --- | --- |
| référence (dim. décroissante, petit diamètre, labels) | L/K 0,14–0,31 (petits jeux) | L/A méd. 0,36–1,00 | — | — |
| dimension croissante | 0,0 % sur les 31 couples (jeu, k) ; mêmes cellules à k = 1 seulement | identique en taille | rien | rien |
| grand diamètre d'abord | −1,3 % à +3,4 % | L/A méd. ±3 % | rien | rien de mesurable : d_H/d_H réf. médian 0,95 à 1,12 (le facteur 1,2–2 prédit est réfuté) |
| labels seuls (dimensions mêlées) | −3,0 % à +2,2 % | — | rien | rien |
| protection partielle ancrée, θ = 1/2 | −9 % à −37 % à k >= 2 ; 0 à k = 1 | L/A méd. 0,32 à 0,95 | 21 à 45 % des sommets retirés à k >= 2 | borne certifiée d_H(L, A) <= D_v + θ r au lieu de D_v (vérifiée sur tous les couples ; d_H/r estimé jusqu'à 0,48 à k = 2 et 0,25 à k = 5) ; d_H(L, C) <= (1 + θ) r ; la couverture n'est plus portée par les seuls sommets (0 site perdu mesuré, sans garantie) |

- Les priorités ne comptent pas : la liberté et l'égalité exacte des dates dictent presque tout l'appariement. Le
  résultat reste reproductible (identifiants triés) mais **pas canonique** : sous une permutation des points (même
  mosaïque, mêmes cellules par PointId), L change (Jaccard 0,91 sur l'anneau à 10 m, k = 2, tailles égales) [M].
- La protection partielle est la seule variante qui gagne vraiment, et elle n'est possible qu'à k >= 2 (à k = 1,
  a_v = 0 < a_e). Elle a deux certificats nouveaux : une ancre par sommet retiré, jamais retirée elle-même (pas de
  cascade : fixture F2), et la longueur d'ancrage <= θ sqrt(a) vérifiée exactement par le vérificateur. Elle ne
  change pas le verdict : retirer 40 % des sommets laisse encore 45 à 115 cellules par point à k = 5.


## 5. Échelle et coût (R5)

`echelle.py`, `resultats/echelle_n*_k*.json` : boules des n plus proches sites du centre de masse de
`trame_08_000000` (sans sol, grille de 1 mm), mosaïque complète, contrôle strict, réduction de référence,
vérificateur global (liberté, unicité, fermeture, acyclicité sur toutes les paires ; dates exactes sur un
échantillon déclaré de 20 000 paires, puis **sur TOUTES les paires** pour k = 1 à 8 000, 16 000, 32 000 sites
(62 146, 105 092, 224 771 dates) et k = 2 à 8 000 sites (351 321 dates), `resultats/verif_dates_echelle_*.json`,
avec la fermeture complète) [M]. Un échantillon de dates est un juge, pas un certificat : sur le vélo à 5 m, k = 1,
un mutant de dates inégales placé en tête du journal passe un échantillon de 50 dates et est refusé par la
vérification complète (`resultats/mutant_dates_velo05m_k1.json`).

| n | k | K (cellules ; par site) | L (par site) | L/K | plancher 2V/K | 3-cellules gardées | strict | vérifié | t mosaïque / effondrement / vérification (s) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 8 000 | 1 | 220 633 ; 27,6 | 96 341 ; 12,0 | 0,437 | 0,073 | 26 % | oui | oui | 6,8 / 0,5 / 9,4 |
| 16 000 | 1 | 445 071 ; 27,8 | 234 887 ; 14,7 | 0,528 | 0,072 | 38 % | oui | oui | 21,1 / 0,9 / 13,6 |
| 32 000 | 1 | 892 681 ; 27,9 | 443 139 ; 13,8 | 0,496 | 0,072 | 34 % | oui | oui | 40,3 / 2,4 / 27,7 |
| 8 000 | 2 | 1 136 487 ; 142,1 | 433 845 ; 54,2 | 0,382 | 0,104 | 21 % | oui | oui | 39,4 / 3,0 / 26,9 |
| 16 000 | 2 | 2 296 865 ; 143,6 | 1 068 371 ; 66,8 | 0,465 | 0,104 | 32 % | oui | oui (dates : échantillon) | 89,9 / 6,7 / 56,3 |
| 32 000 | 2 | 4 607 991 ; 144,0 | 1 975 847 ; 61,7 | 0,429 | 0,104 | 27 % | oui | oui (dates : échantillon) | 270,6 / 13,2 / 86,8 |

- **Sur LiDAR réel à l'échelle, la réduction est deux fois plus faible que sur les petits jeux synthétiques** :
  L/K = 0,44 à 0,53 à k = 1 (contre 0,23 à 0,31), 0,38 à 0,47 à k = 2 sur 8 000 à 32 000 sites (contre 0,18
  à 0,25), un cinquième à un tiers des 3-cellules restent, et le rapport ne baisse pas avec n. À k = 2, L garde
  54 à 67 cellules par site ; la mosaïque d'ordre 2 de 32 000 sites a 4,6 millions de cellules (270 s en Python).
  L'effondrement global part de l'enveloppe et s'arrête sur les cellules critiques ; un nuage LiDAR étendu
  (bâtiments, végétation) en contient beaucoup plus [C pour la cause]. Les tailles par site sont stables en n
  (27,7 cellules de K, 12 à 15 de L) : coût linéaire.
- L'effondrement est négligeable (environ 3 µs par cellule en Python) ; la mosaïque complète (45 à 60 µs par
  cellule) et le certificat (30 à 110 µs par cellule) dominent. À k = 5, avec environ 1 000 cellules par site, une
  trame de 40 000 sites serait à 4·10^7 cellules : **non mesuré à l'échelle à k = 5**, et hors de toute enveloppe de
  100 ms par construction (mosaïque complète). Rien de ceci ne qualifie un coût produit.
- Petits jeux, k = 5 (`TABLEAUX.md`) : les deux nappes (1 139 sites, 1,2 million de cellules) : K = 1 198 851
  cellules, L = 170 647 (L/K = 0,142), effondrement 4,4 s, vérification complète 130 s (514 102 dates exactes),
  mosaïque 100 s.


## 6. Prédictions gelées (PREDICTIONS.md, sha256 d3a471de…) contre mesures

| Prédiction | Mesure | Verdict |
| --- | --- | --- |
| P1a k = 1 : L/K entre 0,30 et 0,55 ; 3-cellules gardées 10–35 % | petits jeux : 0,23–0,31 et 0–11 % ; LiDAR 8 000–32 000 : 0,44–0,53 et 26–38 % | **réfutée** sur les petits jeux, **tenue** à l'échelle LiDAR |
| P1b k = 2, 3, 5 : L/K entre 0,25 et 0,60, au moins 2 fois le plancher | petits jeux 0,14–0,25 (1,2–1,7 fois le plancher à k = 5) ; LiDAR 8 000–32 000, k = 2 : 0,38–0,47 (3,7–4,5 fois) | **réfutée** sur les petits jeux, tenue sur LiDAR à k = 2 |
| P1c faces de L par point à k = 5 : 150–500 | 77–190 | **réfutée en partie** (plus bas) |
| P1d par nœud au milieu de vie : L/A entre 0,25 et 0,70 | médianes 0,36–1,00 (k = 1 : 0,65–1,00) | **réfutée à k = 1**, tenue à k >= 2 |
| P1e la réduction ne suffit pas à un jeton compact | des milliers de cellules par nœud d'objet ; rendu non réduit | **confirmée** |
| P2a vérificateur : 0 refus | 0 refus sur tous les journaux | confirmée |
| P2b mutants refusés | 4 types refusés partout ; le 5e (vérifié sur 50 dates tirées) passe une fois, refusé à dates complètes | confirmée pour le vérificateur complet ; **un échantillon de dates n'est pas un certificat** |
| P2c fixture A, B, C, D | conforme | confirmée |
| P2d chaîne en cascade | conforme (N l sans protection, 0 protégé, l en partielle) | confirmée |
| P3a d_H <= D_v partout ; d_H/D médian 0,2–0,6 | partout ; médianes 0,04–0,42 | confirmée (un jeu sous 0,2) |
| P3b D_v/r <= 4/k sur au moins 99 % | 100 % | confirmée |
| P3c sup d(C, L)/r <= 1 ; médiane 0,4–0,9 | <= 1 ; médianes 0,54–1,00 (1,00 à k = 1) | confirmée sauf la médiane à k = 1 |
| P4a Betti égaux partout | égaux (après correction de la mesure) | confirmée ; voir § 3.3 |
| P4b emboîtement 100 % ; P4c couverture identique 100 % | 100 % ; 100 % | confirmées |
| P4d ouvertures : écart A − L <= 10 points | jusqu'à 47 points | **réfutée** |
| P5a priorités à ±10 % ; grand diamètre : d_H × 1,2–2 | ±3 % ; d_H × 0,95–1,12 | taille confirmée, **d_H réfutée** |
| P5b partielle : 0 sommet à k = 1 ; 5–40 % à k = 5 ; perte de couverture 0–5 % | 0 ; 39–45 % ; 0 % | confirmée sauf la borne haute (45 %) |
| P6a 1,2 million de faces : effondrement 20–120 s, vérificateur 60–400 s | effondrement 4,4 s ; vérificateur 130 s | effondrement **réfuté** (plus rapide), vérificateur confirmé |
| P6b pas de conclusion de coût à l'échelle à k = 5 | échelle mesurée seulement à k = 1 et k = 2 (8 000 à 32 000) | confirmée |


## 7. Échecs, limites, ce qui n'est pas acquis

- **Pas d'échelle LiDAR à k >= 3.** La mosaïque complète d'ordre k est construite en Python
  sur le nuage entier (oracle d'exploration, en aval, borné) : environ 60 µs par cellule pour la construire,
  4 µs pour l'effondrer, 110 µs pour vérifier (`TABLEAUX.md`). À k = 5 sur une trame (40 000 sites, environ
  1 000 cellules par site), il faudrait 4·10^7 cellules : hors de portée de ce prototype et sans rapport avec 100 ms.
  Le coût est dans la mosaïque et dans le certificat, pas dans l'effondrement. Rien ici n'entre dans le calcul de
  la hiérarchie (invariant respecté) ; un constructeur local depuis FULL n'a pas été écrit.
- **Indépendance partielle du vérificateur.** Il recalcule sommets, liberté, sphères, admissibilité et dates sans
  lire la construction, mais il emploie la même caractérisation a_σ = min des niveaux propres admissibles des
  cofaces. Elle est corroborée à k = 1 par gudhi (complexe alpha exact) et à k >= 2 par la bijection des composantes
  avec FULL (niveaux des sommets et des arêtes seulement), et par un **témoin numérique indépendant**
  (`temoin_qp.py`) : minimisation convexe directe de d_k^2 sur la face duale F_σ (programme quadratique, SLSQP) sur
  100 cellules par dimension (anneau à 10 m k = 2 et 5, vélo à 10 m k = 3) : écart relatif à R au plus 8·10^-13 sur
  toutes les cellules où le solveur converge ; il échoue sur 1 à 35 % des cellules (surtout les sommets, formulés
  avec une variable d'épigraphe), qui ne sont pas comptées. Ce n'est pas le certificat exact à multiplicateurs
  rationnels proposé par l'auditeur (non fait ici).
- **Estimations.** d_H(L, A) est une borne inférieure par échantillonnage ; sup d(C, L) est estimé sur un
  échantillon de C ; seul D_v(r) est exact. Les ouvertures utilisent la mesure du harnais (disque à 10 mm).
- **Faute de mesure corrigée** : Betti triangulés faux sur six couples du piéton (cellules dégénérées voisines
  triangulées indépendamment) ; corrigé par l'homologie cellulaire (§ 3.3). Les diagrammes de persistance du § 3.3
  utilisent encore la triangulation ; ils sont égaux sur les huit cas essayés.
- **Rendu** : un rendu a échoué sur le piéton à k = 5 (tétraèdre plat d'une triangulation de Delaunay de points
  cosphériques passé à `Polyedre.frontiere`, qui le refuse) ; le JSON n'avait pas été écrit. Corrigé (JSON écrit
  avant les rendus, rendus protégés, tétraèdres plats écartés) et relancé.
- **Sélection** : nœuds choisis par la vérité terrain (borne, pas méthode), au plus 14 par jeu ; niveaux : naissance,
  milieu géométrique sqrt(r_b r_d), coupe ouverte avant la mort ; racine : borne déclarée 2 r_b.
- **Robustesse (R4) partielle** : seulement réétiquetage et déplacement apparié de 1 mm par coordonnée (sans fusion de
  sites) sur trois petits jeux ; les épreuves « modifications de population » et « échantillonnage avec contrat de
  masse » de l'auditeur ne sont pas faites ici.
- La découpe réelle n'a servi qu'aux images et aux mesures ; aucun réglage n'a été choisi dessus.
- k = 10 : seulement l'anneau de 52 points (K = 78 363 cellules, 1 507 par site ; L/K = 0,138 pour un plancher de
  0,118 ; vérifié ; `resultats/synth_anneau_perce_10m_k10.json`) : la tendance « L colle au plancher des sommets »
  se poursuit.


## 8. Conclusion

**Réponse à la question « réduit-elle assez ? » : non, pas pour un jeton compact ; oui comme certificat.**

1. Le prototype de l'auditeur est correct, vérifiable et bon marché à effondrer : effondrements polyédraux directs
   sur la plage entière, sommets protégés, journal, vérificateur global indépendant, dates exactes. Il conserve
   tout ce que la hiérarchie exige : identités de FULL, couverture exacte, emboîtements, type d'homotopie à chaque
   niveau (diagrammes de persistance identiques), et donne des bornes géométriques certifiées par composante
   (d_H(L, A) <= D_v(r) <= 4r/k, d_H(L, C) <= r), qui tiennent avec de la marge.
2. Mais le plancher est le nombre de sommets de la mosaïque d'ordre k, qui croît comme k n (31 à 61 sommets par
   site à k = 5). L atteint déjà 1,2 à 1,7 fois ce plancher : il n'y a plus rien à gagner par une meilleure priorité.
   Par nœud, le gain est d'un facteur 1 à 3 ; un nœud d'objet garde des milliers de cellules.
3. Le dual réduit n'est pas un meilleur dessin : le rendu des faces exposées ne baisse pas (0,55 à 2,7 fois celui
   de A, médiane 0,98),
   et la forme peut tromper (une roue pleine s'effondre en arbre et paraît ouverte, sans changement de topologie).
   « Homotopie et emboîtement ne contrôlent pas la forme » : mesuré.
4. Usage recommandé : garder A_k comme objet (événements, labels, dates), dessiner ses faces exposées, et employer L
   comme **modèle filtré réduit pour la topologie** : il donne les mêmes diagrammes de persistance avec 2 à 9 fois
   moins de simplexes (5 à 9 sur les petits jeux à k >= 2, 2 à 2,3 sur les boules LiDAR à k = 1), donc b1 / b2 par nœud et le long de sa vie (trous de roue, cavités) à moindre coût. Pour un
   jeton compact, il faut renoncer à garder les sommets de la mosaïque : protection partielle avec budget
   (gain limité à 40 % des sommets par la règle sans cascade), représentant porté par les sites (ombre de
   couverture : pas d'homotopie certifiée) ou modèle approché de taille O(n) (multicouverture creuse d'Alonso),
   chacun avec son contrat déclaré. Aucun de ces choix n'est qualifié par cette expérience.

