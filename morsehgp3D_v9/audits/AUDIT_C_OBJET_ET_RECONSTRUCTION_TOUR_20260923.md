# Audit C — but de l'algorithme et reconstruction de la tour des niveaux de densité K-NN

23 septembre 2026, auditeur C (troisième auditeur de la v9, arrivé ce matin).
Cadre : `phase=exploration_v9_hors_registre`, `backend=reference_cpu`,
`profile=quantized_u18_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`. GCP non utilisé.

Base lue : `origin/main` `0125dc18` (lecture), recoupée jusqu'à `af483513` ; le
commit sonde v12 `4530644b` ne touche ni `src/gen` ni `src/tower`. Méthode : neuf
lectures indépendantes (objet ; q2 ; q3/q4 ; chaîne et catalogue ; tour FULL ;
portes et CI ; mesures ; dossier d'audits ; pistes fermées et GPU), chacune
chargée de relire le code ligne à ligne et de citer ses preuves, puis une
**vérification adverse** de chaque constat par deux relecteurs (angle code,
angle portée et doublon avec A et B). Les petits contrôles exécutés l'ont été
sous `nice -n 19`, hors des campagnes de chronométrage du développeur. Aucun
chrono de cet audit n'est revendiqué.

**Révision 1 (09 h 13 UTC)** : corrections demandées par
l'[erratum de B](ERRATUM_B_AUDIT_C_OBJET_ET_EULER_20260923.md) (règle générique
limitée à la position générale, Euler présenté comme condition nécessaire,
statut exact des portes, mutants comparés clé par clé).

**Révision 2 (10 h 48 UTC)** : verdicts de la vérification adverse (129
contrôles sur 83 constats,
[`c_audit_20260923/verifications/`](c_audit_20260923/verifications/README.md))
intégrés au § 6 ; § 3.1 corrigé (la tour refuse, sous le contrat, toute
omission isolée d'ordre haut $p+u\leq K_{\max}$ ; angle mort conjoint avec
Euler mesuré à 8k) ; « machine à moitié inactive » retirée du § 5 ;
comptes de mutants et biais des chiffres K5 précisés.

**Révision 3 (11 h 25 UTC)** : après la
[contrelecture B des omissions](CONTRE_AUDIT_B_OMISSIONS_ET_PORTEE_REVISION_C_20260923.md),
la détection par la tour des omissions d'ordre haut $p+u\leq K_{\max}$
repose sur le lemme conditionnel de A (première cofacette), à qualifier
avant registre, la classe
haute une zone **potentiellement** aveugle, mesurée par la comparaison des
condensés des tours acceptées ; les « 129 contrôles » de la révision 2 sont
des verdicts argumentés d'agents, pas des tests exécutables.

Ce document répond à la première demande de l'utilisateur : **à quoi sert
l'algorithme, et comment la v9 reconstruit la tour complète**. L'étude des
implémentations alternatives pour le contrat est dans
[`AUDIT_C_ALTERNATIVES_CONTRAT_LIDAR_G4_20260923.md`](AUDIT_C_ALTERNATIVES_CONTRAT_LIDAR_G4_20260923.md).

## Verdict en dix lignes

1. **L'objet est clair et bien choisi** : pour chaque K, la hiérarchie des
   composantes connexes des niveaux supérieurs de la densité K-NN,
   c'est-à-dire de la multicouverture $L_K(r)=\lbrace x : \lvert P\cap B(x,r)\rvert\geq K\rbrace$,
   plus les applications verticales $L_K\subseteq L_{K-1}$. La v9 le calcule
   en deux temps : un **catalogue** exact des boules critiques, puis la
   **tour FULL** (port v7) qui en déduit naissances, multifusions, parents,
   contributions et verticales.
2. **Aucun défaut de correction** n'a été trouvé dans les chemins q2, q3/q4,
   chaîne et FULL (cinq lectures indépendantes, ligne à ligne).
3. **La tour est exacte relativement au catalogue.** La complétude du
   catalogue est prouvée arête par arête (notes v8, induction q4 de A,
   induction q3 écrite ici), testée sur de petits nuages seulement.
4. **Nouveau juge global** : l'invariant d'Euler par ordre K
   ([note](NOTE_C_INVARIANT_EULER_20260923.md)) est une **condition nécessaire**
   de complétude, calculable à l'échelle : il détecte une partie des omissions
   du catalogue (des omissions de contributions opposées peuvent se compenser)
   et ne prouve donc pas la complétude. Il passe sur les 18 coupes LiDAR
   8k/16k/32k ; un protocole « Kmax+2 » ajoute une cohérence entre deux
   exécutions pour les deux ordres supérieurs (contrat K5 seulement : le
   domaine actuel s'arrête à K10).
5. Les preuves invoquées par la v9 (extension non régulière, voies q3/q4) **ne
   sont pas inscrites au registre**, et le README surévalue leur statut.
6. La CI v9 est rouge depuis 01 h 49 (deux causes hors moteur), la porte T2
   de chaîne ne juge que K = 10, où l'élagage est presque vide, et aucune
   porte ne tourne aux tailles d'intérêt.
7. Contrat : meilleurs temps G4 CPU 3,67 s à K5 et 9,58 s à K10 ; même
   parfaitement réparti, le travail actuel demande 1,9 s et 5,8 s sur
   48 fils. L'architecture actuelle ne tient pas 1 s sans réduire le nombre
   d'opérations.

## 1. Le but : l'objet mathématique

### 1.1 Densité K-NN, niveaux et multicouverture

Le manuscrit (§ 6.3, définitions 7–8 et 20–22, théorème 2, texte p. 59–61)
estime la densité en $y$ par l'inverse du volume de la boule qui atteint le
K-ième voisin, et considère ses niveaux supérieurs. À rayon $r$ fixé :
$L_K(r)=\lbrace y : \lvert B(y,r)\cap X\rvert\geq K\rbrace$, boules **fermées**.
Écrit avec la distance au K-ième voisin $d_K$, c'est le sous-niveau
$\lbrace d_K\leq r\rbrace$, soit l'ensemble des points couverts par au moins K
boules de rayon $r$ : la **K-multicouverture**. Les amas de forte densité
sont ses composantes connexes.

Le théorème 2 (`theorem_external`) dit que ces composantes correspondent
exactement aux K-polyèdres du complexe de Čech : les K-sous-ensembles de
rayon de miniboule au plus $r$, reliés quand leur union de $K+1$ points est
encore couverte par une boule de rayon $r$ (graphe $\Gamma_K$, déf. 21).

### 1.2 La tour

La **hiérarchie en r** est l'arbre de fusion de ces composantes quand $r$
croît, facettes isolées comprises. La **tour** empile ces hiérarchies pour
$K=1..K_{\max}$ et les relie par les applications induites par
$L_K(r)\subseteq L_{K-1}(r)$ (spécification, `docs/SPECIFICATION_MORSEHGP3D.md`
l. 85–103 ; le manuscrit traite chaque K séparément). C'est la partie $H_0$
de la bifiltration de multicouverture, restreinte à $K\leq K_{\max}$.
La v9 publie, pour chaque K, une forêt de naissances, de multifusions à
parents pré-lot et de contributions de couverture datées, et l'image
verticale de chaque nœud à son niveau fermé
(`src/tower/forest/full_ball_tower.hpp` l. 97–102).

### 1.3 Où l'objet change : les boules critiques

Une boule $B$ est décrite par son centre $c$, ses $p$ sites strictement
intérieurs $I$, sa coquille $U$ de $u$ sites et la taille $q_{\min}$ de son
plus petit **support positif** ($S\subseteq U$, centre dans l'intérieur
relatif de l'enveloppe de $S$, donc $2\leq q_{\min}\leq4$).
**En position générale** (coquille réduite au support minimal, $u=q$),
Reani–Bobrowski, inscrits au registre : $c$ est critique pour $d_K$ si et
seulement si $p<K\leq p+q$ ; l'indice vaut $\mu=p+q-K$ et la multiplicité
locale $\binom{q-1}{\mu}$. Seuls $\mu=0$ et $\mu=1$ changent $H_0$ :

| rôle pour $H_0$ | ordre | effet |
| --- | --- | --- |
| naissance | $K=p+q$ | une nouvelle composante, qui couvre $I\cup U$ |
| multifusion | $K=p+q-1$ | les $q$ bras $(I\cup U)\setminus\lbrace s\rbrace$ se rejoignent : jusqu'à $q-1$ fusions en une fois |
| inerte pour $H_0$ | $K\leq p+q-2$ | cycles et cavités seulement |

**Coquille étendue** ($u>q_{\min}$) : ces règles ne s'appliquent plus telles
quelles (erratum de B,
[contrelecture](ERRATUM_B_AUDIT_C_OBJET_ET_EULER_20260923.md)). Contre-exemple :
les quatre sommets d'un carré de côté 2 ($p=0$, $u=4$, $q_{\min}=2$). À $K=3$,
aucune boule plus petite ne couvre trois sommets : le centre est une
**naissance**, pas une fusion de quatre composantes ; à $K=2$, en revanche, les
quatre lentilles des côtés fusionnent en une. Les contributions d'Euler de ce
bloc valent $(1,-3,1,1)$ pour $K=1..4$. Le produit ne s'appuie pas sur la règle
générique pour ces blocs : il emploie le quotient local `ShellTable`
(`src/tower/forest/local_plateau.hpp`).

Un triangle aigu à $K=p+2$ fusionne ainsi **trois** régions d'un coup ; un
tétraèdre à $K=p+3$ en fusionne quatre. Sur LiDAR, les multifusions ont en
moyenne 2,31 à 2,49 parents de K2 à K10 (R7b, `vm/probe_4`).

D'où les deux règles de la v9, conformes au manuscrit et au registre :
l'**admission** $p+q_{\min}\leq K_{\max}+1$ (`src/chain/tower_chain.cpp`
l. 492–494) et le **calendrier** d'une boule, ordres
$[p+q_{\min}-1,\min(K_{\max},p+u)]$ (`full_ball_tower.hpp` l. 965–966).
Hors position générale, l'inertie d'une boule avec $p+s\geq K+2$ pour un
support minimal de taille $s$ est le théorème 4.2 du registre
(`proved_here`) : prendre $s=q_{\min}$ est le choix sûr
(`src/tower/forest/local_plateau.hpp` l. 108).

### 1.4 Pourquoi on ne replie pas le seul graphe de Gabriel

Le théorème 4 réduit les fusions aux cofaces de Gabriel (position
générale). Mais la proposition 6 et le théorème 5 du manuscrit, qui
réduiraient toute la hiérarchie au graphe des facettes de Gabriel, sont
**faux en général** (registre, fixture E5) : à un niveau donné, une facette
peut s'attacher silencieusement par des cofaces non Gabriel. La v7 puis la
v9 résolvent donc chaque facette par une **descente géométrique** jusqu'à
une boule du catalogue déjà fermée (§ 2.5). La lecture « objet » l'a
reproduit exactement : au niveau 83886/3563, $\Gamma_2$ complet donne
$\lbrace ABCDE\rbrace$ et le graphe de Gabriel seul $\lbrace ABC, ACDE\rbrace$.

### 1.5 Statut des énoncés

| énoncé | statut |
| --- | --- |
| Th. 2, prop. 5, th. 4 (position générale) | `theorem_external` |
| fenêtre, indice, multiplicité de Reani–Bobrowski ; seuls $s\in\lbrace K,K+1\rbrace$ changent $H_0$ | `theorem_external` |
| lemme des porteurs Gabriel ; th. 4.2 (inertie) | `proved_here`, sans position générale |
| prop. 6, th. 5, fold v4, repli des seules cofaces Gabriel | `false_in_general` |
| naissances FULL = minima Gabriel ; minima + multifusions reconstruisent FULL | `conditional_theorem`, **sous régularité** |
| extension non régulière codée (quotient de coquille, contributions datées, descente) | **aucune entrée au registre** |
| complétude du générateur q2 (notes v8) et q3/q4 (A, cette note) | **aucune entrée au registre** |
| invariant d'Euler par ordre (cas générique) | proposé `proved_here` ([note](NOTE_C_INVARIANT_EULER_20260923.md)) |

### 1.6 Écarts entre l'objet normatif et l'objet v9

- **Position générale** : fausse sur toutes les trames du contrat. R7b compte
  135 à 572 coquilles étendues à K5 et 280 à 1 301 à K10 par trame, coquille
  maximale 5. Chaque condensé publié dépend donc de l'**extension non
  régulière**, qui n'a pas d'entrée au registre. Elle est testée par le juge
  Γ borné, et une esquisse de preuve en huit étapes (connexité du bloc fermé,
  facettes strictes, unicité de la boule, effet d'un bloc, inertie,
  représentants réduits, terminaison de la descente, verticales) a été
  rédigée et contrôlée par un oracle exact sur 33 nuages dégénérés
  (1 408 événements, 732 blocs à coquille étendue, deux mutants tués).
- **Inégalités** : conformes (intérieur strict, coquille à puissance nulle,
  niveaux fermés, coupes ouvertes et fermées distinguées).
- **Coquilles de plus de 12 sites** : refus transactionnel
  `unsupported_degeneracy`, jamais une troncature.
- **Sites dupliqués** : la spécification compte les distances avec
  multiplicité, la v9 refuse les doublons ; sans effet sur les trois trames
  (aucune fusion de quantification à 1 mm), à trancher pour des trames
  multi-échos.
- **Identités** : la chaîne publie des rangs d'entrée ; la tour sait gérer des
  identités externes, pas la chaîne.

## 2. La reconstruction, pas à pas

Vue d'ensemble de `run_tower_chain` (`src/chain/tower_chain.cpp` l. 285–603) :

```text
points u18 distincts
  -> préparation (tri 54 bits, unicité) -> index du générateur
  -> q2 : front WSPD -> témoins -> Pool/frère -> census exact par paire
  -> q3/q4 : produits WSPD développés -> témoins par paire (cache)
             -> cœur diamétral -> cover -> voies mortes -> atlas q4
             -> graines q3/q4 -> census q3 / balayage q4 -> présentations
  -> fusion des présentations (tri d'échantillonnage, dédoublonnage)
  -> index de la tour -> recensement exact de chaque clé (I, U, q_min)
  -> tour FULL : validation -> programmes par K -> cibles statiques
             -> lots par niveau exact -> populations -> verticales -> forêts
```

### 2.1 Préparation et index

`prepare_cloud` copie les points, refuse toute coordonnée hors de
$[0,2^{18})$ et les doublons (clé 54 bits triée). L'index du générateur est
un arbre de bissection au milieu de la plus grande étendue, $2n-1$ nœuds en
préordre, profondeur au plus 54. La tour construit plus loin un **second**
index (arbre radix sur Morton 54 bits) : 20 à 27 ms en série pour les deux.

### 2.2 Voie q2 : boules à support diamétral

Une boule de support minimal $\lbrace a,b\rbrace$ porte un K-simplexe de
Gabriel dès que $K+1\geq p+2$ : les boules utiles sont celles de
$p\leq K_{\max}-1$, le seuil de rejet est $K_{\max}$ témoins stricts.

1. **Front WSPD** (`src/gen/wspd/front.cpp` l. 75–391). Chaque paire non
   ordonnée tombe dans exactement un produit, à son plus bas ancêtre
   commun ; un produit disjoint $A\times B$ est émis quand
   $\mathrm{gap}^2\geq s^2\max(\mathrm{diag}_A^2,\mathrm{diag}_B^2)$
   ($s\geq8$). La masse des produits rejetés et émis égale $\binom{n}{2}$,
   vérifiée à l'exécution. **La séparation n'entre dans aucun certificat
   q2** : elle ne règle que la granularité.
2. **Témoins universels** (`front.cpp` l. 214–357) : un site $z$ hors de
   $A\cup B$ est crédité si le minimum exact de $H=(z-a)\cdot(b-z)$ sur les
   boîtes est $>0$, c'est-à-dire s'il est strictement intérieur à toutes les
   boules diamétrales du produit. $K_{\max}$ rangs distincts rejettent le
   produit. Fenêtre de $2K_{\max}$ rangs autour d'un pivot et héritage des
   témoins par les enfants (théorème H des notes v8).
3. **Rectangle terminal** : crédits Pool ($p\geq h_a+h_b$ par populations
   disjointes), certificat du frère ($K$ sites strictement intérieurs).
4. **Census exact par paire** (`q2_census.cpp` l. 397–547) : descente de
   l'index avec bornes exactes de $4H$, crédit d'un nœud entier si le
   minimum est $>0$, saut si le maximum est $\leq0$, saturation à $K_{\max}$.
   Une paire acceptée relance une collecte complète des intérieurs et de la
   coquille, puis vérifie compte = intérieurs.

Complétude : couverture des paires, sûreté des rejets (front, Pool, frère)
et exactitude du census sont **prouvées** dans les notes v8
(`P0_FRONT_REEL`, `P0_TEMOINS_HERITES_Q2`, `P0_POOL_TERMINAL_Q2`,
`P0_CERTIFICAT_FRERE_Q2`, `P0_FRONT_ET_CENSUS_Q2`), jamais consolidées en
v9 ni au registre ; **testées** jusqu'à 20 sites exhaustivement, 320 sites
pour l'héritage, 14 pour la chaîne. Arithmétique : tous les intermédiaires
q2 restent sous $2^{44}$, donc la voie q2 est **exacte en binary64**, point
utile pour un portage GPU (7 315 contrôles exacts, lecture q2).

### 2.3 Voies q3/q4 : triangles aigus et tétraèdres positifs

Une boule q3 ou q4 a pour **arête propriétaire** la plus longue arête de son
support (départage par identifiants). Son centre est proche du milieu $m$ :
$\lVert c-m\rVert^2\leq D/12$ en q3, $\leq D/8$ en q4 (lemme du « citron »,
$D=\lvert ab\rvert^2$). Seuils : $p<K-1$ en q3, $p<K-2$ en q4.

1. **Produits WSPD développés** en paires $(a,b)$, après un filtre de
   témoins sur les boîtes du rectangle.
2. **Filtre de paire** exact (`q34_witness_search.cpp` l. 105–213) et
   **cache** des nœuds admis de la dernière recherche de même $a$.
3. **Cœur diamétral** puis **cover** (boule fermée
   $\lvert 2z-a-b\rvert^2\leq4\lvert b-a\rvert^2$, qui contient toute boule
   possédée) et **certificat de voie morte** : une cellule de centres est
   fermée si elle est hors du disque admissible, ou si $T$ sites sont
   strictement intérieurs à ses quatre coins ($T=K-1$ en q3, $K-2$ en q4).
4. **Atlas des centres q4** (`q4_local.cpp`, `q4_local_partition.cpp`) sur
   le domaine positif : cellules `Outside`, `Deep` (compte certifié
   $\geq K-2$), fragments exacts (sites intérieurs, extérieurs, actifs).
5. **q3** : graines aiguës possédées cherchées dans l'index global, centre
   exact localisé dans l'atlas, rejet si le compte certifié atteint $K-1$,
   sinon census sur la feuille exacte ou sur l'index global.
6. **q4** : graines en balayage le long de leur droite dans les feuilles
   vivantes ; événements triés par racines exactes ; émission de la
   première présentation valide et canonique d'un groupe de racines égales.

Complétude : l'induction q4 « support propriétaire → graine canonique →
cellule ni `Outside` ni `Deep` → événement émis » est
[écrite par A](Q4_INDUCTION_ATLAS_EVENEMENTS_20260923.md) ; la lecture q3/q4
de cet audit l'a relue et a écrit l'**induction q3 symétrique** (front,
filtres, cœur, cover, énumération, atlas, census, émission), avec un lemme
neuf : un centre q3 n'est **jamais** dans une cellule `Outside`, sauf si la
lentille de l'arête ne contient qu'un site. Ce lemme explique pourquoi
`q3_atlas.outside_domain` vaut exactement la même chose à K5 et à K10 dans
R7b (20 845 / 17 683 / 21 868). Bornes 18 bits : toutes vérifiées par
script exact ; la garde `certified_cell` à $2^{117}$ est **sans marge**
($512M^6=2^{117}(1-2^{-18})^6$) : un passage à 19 bits lèverait une
exception, jamais une erreur silencieuse.

### 2.4 Catalogue : fusion, recensement, garanties

Les présentations de chaque ouvrier sont triées par (clé, arité, support),
réparties par séparateurs de clés, puis rassemblées et dédoublonnées
(`gather_presentations`, l. 88–153). Pour chaque clé distincte, la chaîne
recalcule clé et niveau depuis le support (formules v7), recense
**exactement** $I$ et $U$ sur tout le nuage, exige profondeur et coquille
égales à celles émises, recalcule $q_{\min}$ sur une coquille étendue,
vérifie la fenêtre, et refuse sans troncature une coquille de plus de
12 sites.

Ce que garantit `complete_relative` (lecture de code) : chaque clé émise est
exacte ; $q_{\min}$ est vrai (coquille étendue : recalcul ; coquille
régulière : positivité prouvée par la tour) ; chaque ligne est dans la
fenêtre ; la tour est la tour FULL exacte **de ce catalogue**. Ce qu'il ne
garantit pas : une **clé entièrement omise** (aucune présentation) ; avec
`run_tower=false`, la positivité des coquilles régulières n'est pas
vérifiée alors que le statut reste `complete_relative`.

### 2.5 Tour FULL : du catalogue à la hiérarchie

**Validation** (`validate_catalogue`, l. 877–1002) : domaine, unicité des
clés, index par adressage ouvert, puissances $<0$ sur $I$ et $=0$ sur $U$,
recalcul de la clé pour les boules régulières, témoin MEB pour les coquilles
étendues, tri total par niveau exact (filtre double certifié, puis U320),
puis **programmes** : chaque boule entre dans `programs[K]` pour chaque
ordre de son calendrier.

**Représentants** (`visit_block_at`, l. 1463–1504). Boule régulière (plus de
99,99 % sur LiDAR) : à $K=p+u$, naissance sans représentant, contribution
$I\cup U$ ; à $K=p+u-1$, les $u$ facettes $I\cup U\setminus\lbrace s\rbrace$.
Coquille étendue : le quotient local `ShellTable::rank(K)` donne une
composante stricte par classe et une contribution.

**Résolution d'un représentant** $F$ ($K$ sites, strictement antérieur) :

1. MEB exacte de $F$ : proposition Welzl « move-to-front » en `double`, qui
   ne décide rien, vérifiée par formes et puissances entières puis
   canonisée sur le bord exact ; sinon énumération de référence (première
   paire maximale, puis triples et quadruples).
2. Si la boule est au catalogue et admissible à K, c'est le terminal.
3. Sinon, un **intrus** strictement intérieur existe forcément (sinon $F$
   serait faible et sa boule absente du catalogue : refus
   `full_ball_static_missing_weak_terminal`) ; il remplace le premier site du
   support. $F$ et $F'$ sont adjacents dans $\Gamma_K$ par la coface
   $F\cup\lbrace z\rbrace$, et le couple (rayon, taille de coquille
   sélectionnée) décroît strictement dans l'ordre lexicographique. Imposer
   une décroissance stricte du seul rayon serait faux (fixture
   `actual_equal_radius_descent`).
4. Semis : une facette égale à la population $I\cup U$ d'une boule
   d'ordre $p+u$ est résolue sans MEB.

La voie **statique** (défaut de la chaîne) collecte toutes les requêtes d'un
ordre, les trie par clé et résout chaque clé une fois ; la voie temporelle
résout pendant les lots. Les deux donnent le même terminal (porte T2 à
0/1/4 fils).

**Lots** : `programs[K]` est parcouru par lots de niveau **exactement**
égal ; tous les représentants d'un lot sont résolus sur l'état pré-lot,
puis groupés par racines pré-lot communes. Zéro parent : naissance ; un
parent : continuation (émise seulement si elle apporte une contribution) ;
au moins deux : multifusion vers un nouveau nœud. Les ancres des boules du
lot ne sont publiées qu'après sa fermeture. Deux boules de même niveau ne
partagent aucune facette nouvelle (unicité de la miniboule) : le groupement
par racines suffit.

**Verticales** : une naissance a pour image l'ancre de la **même boule** à
$K-1$ (elle existe, car $K\geq p+q_{\min}$) ; une multifusion, la racine à
$K-1$ de l'image de ses parents, qui doivent coïncider (contrôle de
naturalité).

**Ordonnancement** (`run_orders_parallel`, l. 458–507) : phase 0 (cibles
statiques, ordre par ordre, parallèle dans un ordre), phase A (lots, un
ordre par tâche, séquentielle dans un ordre), phase B (populations), phase C
(verticales), puis encodage. En cas d'échec, le plus petit K l'emporte et
aucune tour partielle n'est publiée.

### 2.6 Pourquoi la tour est juste si le catalogue est complet

Tout changement de $\Gamma_K$ au niveau $L$ est porté par des boules de
niveau $L$ dans leur fenêtre (fenêtre et quotient local) ; aucune facette
nouvelle n'est partagée entre deux boules ; un représentant par composante
stricte suffit ; la descente termine sur un terminal fermé et admissible ;
le groupement par racines pré-lot donne les composantes du lot ; les
naissances sont les K-ensembles de boules minimales ; les verticales
découlent de l'inclusion. Sources : `BALL_ANCHORS` v7 §§ 1–5, registre
(`conditional_theorem` sous régularité), esquisse B1–B8 de cet audit pour le
domaine étendu. Le juge Γ borné le vérifie jusqu'à $n\leq14$ ; le filtre
flottant de l'ordre des niveaux a une erreur relative maximale de
$2^{-51,39}$ sur 200 000 cas exacts, sous la borne annoncée de $2^{-49}$.

## 3. La complétude à l'échelle

### 3.1 Ce que la tour détecte déjà (corrigé en révisions 2 et 3)

La descente d'un représentant qui atteint une boule sans intrus absente du
catalogue lève un refus (`full_ball_*missing_weak_terminal`). La révision 1
en tirait qu'à $K=1$ une arête d'arbre couvrant minimal omise fausse les
niveaux sans refus ; la vérification adverse (constat L4-02) l'a réfutée
**sous le contrat** : une boule de fenêtre $[p+q_{\min}-1,\,p+u]$ est une
naissance à son ordre haut $p+u$, et une arête de Gabriel ($p=0$, fenêtre
$[1,2]$) est donc aussi en jeu à l'ordre 2 dès que $K_{\max}\geq2$.

**Lemme conditionnel** (A, [première cofacette](LEMME_PREMIERE_COFACETTE_OMISSION_20260923.md)) :
*une omission isolée d'une boule d'ordre haut $p+u\leq K_{\max}$, ou
plusieurs omissions toutes de cette classe, sont refusées par la tour,
le catalogue étant complet par ailleurs.* Mon premier argument « le nœud
$I\cup U$ doit fusionner, et sa première référence vient d'une descente qui
atteint sa clé » a une lacune relevée par
[B](CONTRE_AUDIT_B_OMISSIONS_ET_PORTEE_REVISION_C_20260923.md) : FULL
construit programmes et nœuds **depuis le catalogue amputé**, si bien que la
racine unique ne force pas, à elle seule, une référence à la clé manquante.
A comble ce trou : la boule $C=\mathrm{MEB}(S\cup\lbrace z\rbrace)$ de plus petit
rayon, $S=I\cup U$, émet $S$ comme représentant et sa résolution refuse.
Reste à qualifier avant registre la couture sur coquilles étendues,
égalités de niveau et voies statique et temporelle. Observations : oracle borné du vérificateur,
3 382 retraits sur 3 382 refusés (6 à 8 sites) ; sonde d'omission de C à
8k (trois familles synthétiques et trois coupes LiDAR sans sol à K5, deux
cas à K7, deux à K10, tirage déterministe à pas régulier par strate) :
575 retraits sur 575 refusés dans les strates d'ordre haut
$p+u\leq K_{\max}$. La sonde retire des clés **déjà émises** : elle ne
cherche aucune clé que le générateur n'aurait jamais émise. Détail :
[`c_omission_20260923/`](c_omission_20260923/README.md).

**Zone de détection potentiellement aveugle.** Pour une coquille régulière,
Euler voit une omission isolée si et seulement si $p\leq K_{\max}-3$, et la
tour la refuse (lemme conditionnel) si $p+q\leq K_{\max}$. Restent les boules de
**fusion seule à l'ordre $K_{\max}$** de type q2 à $p=K_{\max}-1$ et q3 à
$p=K_{\max}-2$ : 26 à 29 % du catalogue à K5, 17 à 18 % à K7, 10 à 11 % à
K10 sur 8k. Ni Euler ni le statut de la tour n'y sont systématiquement
sensibles : 2 retraits sur 312 y sont refusés, par la connexité finale.
Les retraits acceptés ne sont pas pour autant des tours inchangées :
à 8k, dans cette zone, 10 des 68 retraits q2 acceptés et 26 des 67 retraits q3 acceptés changent le condensé FULL (tours fausses publiées `complete_relative`, invisibles à Euler) ; les autres gardent un condensé égal (fusions vraisemblablement redondantes).

À K5, une exécution de contrôle à $K_{\max}+1$ ou $K_{\max}+2$ **avec
tour**, plus l'égalité clé par clé de la restriction
$p+q_{\min}\leq K_{\max}+1$, renforce le diagnostic : ces boules y
deviennent des naissances d'ordre au plus $K_{\max}+1$. Elle ne remplace
pas un juge indépendant si une omission est commune aux deux exécutions,
et elle ne se transporte pas à K10 : K11 est hors domaine
(`kBallInteriorMax = 9`). Pour la famille q2, un juge d'échantillon
indépendant du générateur existe désormais (même dossier) : 204 683 boules
q2 attendues autour de sites tirés, toutes présentes, dont 16 506 à $p=9$ à
K10. Pour la famille q3 à $p=K_{\max}-2$, un juge d'échantillon
indépendant (élagage exact par la demi-boule diamétrale, clé canonique,
niveau, coquille, intérieurs et arité recoupés) trouve présentes les
286 706 incidences q3 admissibles de sa campagne v5, dont 55 297 clés
régulières de cette famille ; ses portes v6 tuent tous leurs mutants
avec marqueur causal. Après l'audit A du juge v6, les portes v7
(`results/gates_v7/`) visent la seule strate longue régulière au rang
critique : 92, 179 et 17 triangles distincts sur les sites isolés de s00,
s01, s02, clé retirée déclarée manquante, mutants tués dans la strate. Ce
sont des échantillons, pas une certification. Deux omissions
conjointes (une naissance et la seule fusion qui la référence) restent
hors de portée de ces contrôles, comme d'Euler.

### 3.2 Invariant d'Euler

Pour tout $K\leq K_{\max}-2$, toute boule qui contribue au bilan d'Euler de
$d_K$ est admissible dans le catalogue ; la somme des contributions vaut 1
(plus $n$ à $K=1$). C'est une **condition nécessaire** : elle détecte les
omissions dont les contributions ne se compensent pas, elle ne certifie pas
chaque clé. Détails, preuve, oracle et résultats dans la
[note dédiée](NOTE_C_INVARIANT_EULER_20260923.md). Sur les 18 coupes
LiDAR emboîtées (trois trames, 8k/16k/32k, K5 et K10), toutes les sommes
vérifiables valent 1. Le protocole « Kmax+2 » (exécuter à $K_{\max}+2$,
vérifier Euler jusqu'à $K_{\max}$, puis comparer le catalogue $K_{\max}$ à la
restriction $p+q_{\min}\leq K_{\max}+1$ du catalogue $K_{\max}+2$) passe sur
08/000000 8k à K5 : 342 181 boules de part et d'autre, **même nombre et même
somme commutative de hachés 64 bits** (ce n'est pas une comparaison clé par
clé ; une omission commune aux deux exécutions passerait). Il ne couvre pas
le contrat K10, qui demanderait un générateur à K12.

Les 35 mutants compilés du générateur (`tests/gen/mutants.json`) ont été liés
à la chaîne sur 08/000000 8k (détail et fichiers dans la
[note Euler](NOTE_C_INVARIANT_EULER_20260923.md)) : **9 sont tués par
l'invariant simple** (aucun n'est vu par les contrôles de la chaîne), 3 par le
protocole Kmax+2 seulement, 11 sont refusés par la chaîne, et 12 laissent le
catalogue **identique clé par clé** sur cette coupe. Les trois mutants que
l'invariant simple laisse passer n'omettent, dans cette exécution K5, que des
boules de profondeur 3, qui ne comptent qu'aux ordres 4 et 5.

### 3.3 Autres juges disponibles

- $K=1$ : les niveaux des fusions K1 publiés doivent égaler les
  $d^2/4$ d'un arbre couvrant minimal exact indépendant (single linkage).
- Invariant K-NN pour q2 : tout site strictement intérieur à la boule
  diamétrale de $\lbrace a,b\rbrace$ est plus proche de $a$ que $b$, donc
  $p(a,b)\leq\min(\rho_a(b),\rho_b(a))$ ; toute paire de rang de voisinage
  au plus $K_{\max}-1$ doit être émise (vérifié sur 3 312 paires).
- Juge d'échantillon : graines tirées, supports de 2 à 4 sites parmi leurs
  voisins, recensement exact indépendant, présence exigée au catalogue si la
  boule est dans la fenêtre.

## 4. Portes, oracles et CI

**Ce qui est solide.** Les juges T2 calculent en rationnels Boost, une
arithmétique distincte de celle du produit (i128, U192, U320). Le modèle Γ
suit mot pour mot la définition 21 du manuscrit. Les 35 mutants compilés du
générateur sont exigeants (code **et** stderr exacts). CTest inscrit
128 portes : 127 exécutées et une désactivée. L'étape CTest de la CI passait
(capture de la lecture L6 à `0125dc18`, exécution `35833313204`) alors que le
workflow entier était déjà rouge à cause du selftest (point suivant).

**Ce qui manque** (vérifié par cet audit) :

- **CI rouge depuis 01 h 49**, pour deux causes hors moteur, signalées au
  développeur (`af483513`) : `git rev-parse HEAD~1` sur clone superficiel
  dans `gcp-migration/tower_selftest_v9.py:1075`, puis, depuis `06f71037`, un
  chemin absolu du worktree du développeur archivé dans le cas de la porte
  `mhgp9_lidar_scaling_reader_*`. 138 échecs sur les 200 dernières
  exécutions.
- **La porte T2 de chaîne ne juge que $K_{\max}=10$**
  (`tests/chain/chain_census_tower_gate.cpp` l. 174–211). Sur 12 à
  14 sites, la fenêtre de rang y admet presque tout : 1 à 3 boules rejetées
  par fixture contre 21 à 76 à K5. Les leviers qui élaguent (voies mortes,
  cœur, cache, saturation) n'y sont presque jamais jugés, et le repli K5 du
  contrat n'est jugé par oracle nulle part au niveau de la chaîne.
- **Aucune porte à la taille d'intérêt** ne juge la complétude : les labels
  `scale8000/16000/32000` ne portent que sur un diagnostic de l'atlas à une
  seule arête (`CMakeLists.txt` l. 294–299). L'invariant d'Euler est prêt à
  y être raccordé.
- **Mutants absents** : aucun mutant produit sur le cœur FULL (lots de même
  niveau, coupes ouvertes et fermées, verticales), sur les recoupements de la
  chaîne (clé, profondeur, coquille, $q_{\min}$, doublon, fenêtre, coquille
  de plus de 12) ni sur la voie q2. Le registre `kMutants` déclare 132
  noms, dont 18 seulement ont un site (19 lignes `MHGP9_MUTANT`) ; aucun
  n'est activable (`mutants_enable` sans appelant, pas de `--inject=`) ;
  12 sont sur le chemin produit, 6 dans du code sans appelant (mutants
  équivalents : supprimer ce code plutôt que le muter). Le mutant
  `admitted_lane_recounted_in_children` est désactivé alors qu'un site
  unique à deux lignes existe.
- **Concurrence** : ni ASan/UBSan ni TSan en CI ; bit-identité testée
  jusqu'à 8 fils pour la chaîne et la tour, 4 pour le générateur ; aucune
  permutation d'entrée au-delà de 30 sites.

## 5. Mesures et écart au contrat

Recalcul sur les 109 exécutions G4 brutes (R1 à R7b), périmètre constant :

| session | 000100 K5 | 000000 K5 | 000200 K5 | 000100 K10 | 000000 K10 | 000200 K10 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| R1 (tour mono) | 15,05 | 18,81 | 29,25 | 82,31 | 111,68 | 125,44 |
| R5 | 4,26 | 5,96 | 7,38 | 12,07 | 17,00 | 19,28 |
| R7b hors digest | 3,67 | 5,56 | 6,44 | 9,58 | 13,91 | 15,28 |

- **Où va le temps** (R7b, moyenne des répétitions MEB ON) : q3/q4 69–74 %
  à K5 et 55–64 % à K10 ; tour 15–20 % à K5 et 25–33 % à K10 ; q2 4–8 % ;
  plomberie (préparation, index, fusion, recensement, libérations) 0,17 à
  0,28 s à K5 et 0,70 à 1,00 s à K10, dont 0,18 à 0,43 s **non attribués**
  à K10 (destruction supposée du catalogue et des index).
- **Écart au contrat** : même parfaitement réparti sur 48 fils, le travail
  CPU de R7b demande 1,81–2,60 s à K5 et 5,58–7,75 s à K10. Il faut le
  diviser par 1,8–2,6 (K5) et 5,6–7,7 (K10) pour 1 s, par 18–77 pour
  100 ms. **Aucun réglage d'ordonnancement ne ferme cet écart.**
- **Occupation et temps système** : 19 à 28 fils occupés en moyenne sur
  48 fils logiques, soit 24 cœurs physiques en SMT 2 (ce n'est pas une
  machine à moitié inactive : de 24 à 48 fils, le mur ne baisse que de 9 à
  14 %) ; 10 à 13 %
  de CPU système à K5, apparu avec le certificat de voies mortes (ablation
  R3 : 6,74 contre 1,68 s de temps système) ; 0,44 à 1,97 M commutations
  volontaires par exécution ; file q3/q4 unique sous mutex ; environ
  100 équipes de fils créées et jointes par chaîne K10 (`pool.hpp` n'est pas
  un pool persistant). La sonde ne publie ni CPU par phase ni temps par
  ouvrier, bien que le générateur les calcule.
- **La tour plafonne à 24 fils** : phase A séquentielle dans un ordre et
  limitée à $K_{\max}$ tâches, phase 0 séquentielle entre ordres ; ×1,02 à
  ×1,12 de W24 à W48.
- **q2 et le recensement n'ont pas bougé depuis R1** ; q2 est près de son
  plancher SMT (×1,07 à ×1,28 de 24 à 48 fils). À K10, q2 et recensement
  font à eux seuls 0,77 à 1,27 s.
- **Précisions sur les chiffres publiés** : les meilleurs K5 de 000000 et
  000200 (5,31 et 6,40 s) sont des minima sur quatre essais pris en
  configuration **MEB OFF** ; la configuration par défaut donne 5,63 et
  6,51 s en moyenne, soit un biais optimiste de 2 à 6 % (les chiffres K10
  sont bien des minima MEB ON). Les reçus G4 n'ont qu'une ou deux répétitions, sans
  échauffement ni p95, et ne publient ni la grille ni le masque sans sol.
- **Représentativité** : trois trames d'une seule séquence, 35,6 k à
  45,8 k sites ; rien entre 46 k et 60 k. Dans la campagne locale emboîtée,
  la densité de sortie baisse avec la taille (48,6 → 38,7 nœuds par site à
  K5 sur 000000) : la sous-linéarité des boules est un effet du recadrage
  radial, pas une propriété de l'algorithme.
- **Sortie** : 31 à 33 boules et 37 à 39 nœuds par site à K5, 120 à 138 et
  163 à 186 à K10 ; 770 à 968 Mio de sortie K10 plus un catalogue de
  0,98 à 1,24 Go à l'ABI actuelle. À 100 ms, K10 exigerait 18 à 22 Go/s
  d'écriture : une représentation compacte sera nécessaire, mais le verrou
  reste l'**amplification de travail** (313 à 387 tests ponctuels d'atlas
  par candidat q3/q4 émis, 15 à 55 graines q4 par q4 émise).

## 6. Constats principaux

Constats consolidés des neuf lectures, dédoublonnés et recoupés avec les
notes de A et B. Colonne « vérif. » : **C** = revérifié directement par
l'auditeur C ; **V** = confirmé par la vérification adverse ; **V\*** =
confirmé avec correction (le texte ci-dessous est corrigé) ; **D** = déjà
signalé par A, B ou le développeur. Verdicts bruts (129 contrôles sur 83
constats) : [`c_audit_20260923/verifications/`](c_audit_20260923/verifications/README.md).

| # | gravité | constat | vérif. | action proposée |
| --- | --- | --- | --- | --- |
| 1 | haute | CI v9 rouge depuis 01 h 49 (selftest `HEAD~1`, puis chemin absolu dans la porte de pente) | C | corriger les deux causes ; mutation « chemin étranger » |
| 2 | haute | aucun juge de complétude du catalogue aux tailles d'intérêt | C | invariant d'Euler dans la sonde, le lecteur G4 et une porte `scale8000` ; protocole Kmax+2 |
| 3 | haute | travail CPU ×1,8–2,6 (K5) et ×5,6–7,7 (K10) trop grand pour 1 s même à 48 fils parfaits | C | réduire le nombre d'opérations avant de paralléliser ; étude d'alternatives |
| 4 | haute | la tour FULL ne passe pas au-delà de 24 fils (phase A : un fil par ordre ; phase 0 séquentielle entre ordres) ; R8 : lots de K10 0,80–1,19 s sur un seul fil | D, V | préparer K=Kmax d'abord et lancer A(Kmax) aussitôt (A(K) ne dépend que de sa phase 0) ; phase A maigre ; phase C parallèle ; lemme max-ID |
| 5 | basse | aucune préparation GPU du poste dominant (déjà connu) ; les 27 annotations `MHGP9_HD` sont toutes dans `tower/` et incohérentes (aides non annotées, points de mutant hôtes) | D, V\* | flux plat par feuille d'atlas, filtres certifiés, porter q2 en binary64 exact |
| 6 | moyenne | T2 de chaîne limitée à $K_{\max}=10$ : élagage presque vide, K5 jamais jugé | C, V | paramétrer K ∈ {1,2,3,5,10}, planchers de rejets, mutant de seuil tué |
| 7 | moyenne | les refus de recoupement de la chaîne n'ont aucune porte causale | V | mutants compilés de présentation, fixture u13 de A |
| 8 | moyenne | aucun mutant produit sur le cœur FULL, la recoupe et q2 ; `kMutants` : 132 noms, 18 avec site, aucun activable, 6 dans du code mort | V | convertir les 12 sites du chemin produit en mutants compilés, supprimer le code mort, purger `kMutants` |
| 9 | moyenne | extension non régulière, complétude q2 et q3/q4 absentes du registre ; README « prouvée » trop fort | C | entrées au registre (B1–B8, inductions q3/q4, Euler) ; corriger README l. 23 |
| 10 | moyenne | plomberie 0,70–1,00 s à K10, dont 0,18–0,43 s non attribués | V (chiffres) | publier `release_ms`, arènes, index unique, recensement fusionné |
| 11 | moyenne | 10–13 % de CPU système à K5, lié au certificat de voies mortes (ablation R3) ; environ 100 équipes de fils créées par chaîne K10 ; file q3/q4 sous mutex unique, commentaires faux (attente 35–49 % à K5 en R8) | V\*, D | pool persistant, files par ouvrier, `getrusage` par phase |
| 12 | moyenne | q2 et recensement figés depuis R1 (0,78–1,29 s à eux deux à K10, R7b et R8) ; grand-livre q2 non publié (3 compteurs sur plusieurs dizaines) | D, V | ablations `{4,all,true}`, charge utile réduite, publier le grand-livre |
| 13 | basse | protocole de mesure du plan non tenu (répétitions, seuil de charge, p95, octets de tour) ; aucune trame entre 45,8 k et 60 k | V\*, D | session G4 W1..W48, boucle à chaud, trames déclarées à l'avance |
| 14 | basse | bénéfice net de la preuve de voie morte sur cover complet jamais mesuré, aucun levier ne l'isole ; elle prouve 19–27 % des voies atteintes et ferme 21–24 % des covers | V\* | levier séparé, puis ablation appariée |
| 15 | basse | `run_tower=false` publie `complete_relative` sans positivité vérifiée des coquilles régulières | V | positivité dans le recensement ou statut distinct |
| 16 | basse | meilleurs K5 publiés mêlant MEB OFF et ON (biais optimiste 2–6 %) | V | publier moyenne et configuration par défaut |
| 17 | haute | zone de détection potentiellement aveugle pour Euler et la tour : boules de fusion seule à l'ordre Kmax (q2 à p=Kmax−1, q3 à p=Kmax−2), 26 à 29 % (K5), 17 à 18 % (K7) et 10 à 11 % (K10) du catalogue à 8k ; parties q2 et q3 jugées par échantillon indépendant (v5 et portes v6, tout présent) | C | K5 : contrôle Kmax+1 avec tour et restriction clé par clé ; K10 : domaine d'audit K11 ou juge d'échantillon dédié (R-20) |

## 7. Recommandations au développeur, par ordre

1. **Tout de suite, coût faible** : réparer la CI (constat 1) ; publier
   `euler_by_k` dans la sonde et le faire exiger par le lecteur G4 ;
   ajouter la porte `scale8000` Euler ; faire tourner T2 aux $K_{\max}$
   1, 2, 3, 5 et 10 ; publier `getrusage` par phase et les temps par
   ouvrier que le générateur calcule déjà.
2. **Preuves** : inscrire au registre l'extension non régulière (esquisse
   B1–B8 et son oracle), les inductions q3/q4, la complétude q2 consolidée et
   l'invariant d'Euler ; corriger le README. Ajouter les mutants de la
   recoupe de chaîne et du cœur FULL.
3. **Même objet, moins de travail** : la tour (phase A maigre, pipeline qui
   lance l'ordre $K_{\max}$ en premier, phase C parallèle, pool persistant) ;
   la plomberie (arènes, index unique, recensement fusionné au balayage des
   plages) ; q2 (`{4,all,true}`, charge utile réduite) ; q3/q4 (ablation de
   la preuve sur cover, restructuration par feuille d'atlas, certificat avant
   expansion en cours chez le développeur).
4. **Au-delà** : l'étude d'implémentations alternatives de l'auditeur C
   (note séparée, en cours) et le GPU. L'invariant d'Euler fournit un juge
   **nécessaire**, peu coûteux et applicable à l'échelle, pour tout générateur
   alternatif ; il ne remplace ni les oracles bornés, ni une comparaison clé
   par clé avec le générateur actuel sur les coupes 8k–32k.

## Annexe — preuves et reproduction

Les neuf rapports de lecture, leurs scripts de contrôle (Python exact et
petits harnais C++) et les verdicts de la vérification adverse sont archivés
dans [`c_audit_20260923/`](c_audit_20260923/README.md). Les sondes de
l'invariant d'Euler sont dans [`c_euler_20260923/`](c_euler_20260923/README.md).
Aucune donnée SemanticKITTI n'y est versionnée.

