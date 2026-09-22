# Contrelecture B — grandes coquilles et quotient local sans table 2^u

22 septembre 2026. Proposition mathématique pour le futur catalogue FULL
v9, **non implémentée et non chronométrée**. Elle ne qualifie ni le moteur
entier 1 mm, ni la tour sur G4. Son point de comparaison est le
[`ShellTable` v7](../../morsehgp3D_v7/src/forest/local_plateau.hpp),
limité à `u=|U|≤12` et construit sur `2^u` masques. La
[contrelecture générale](CONTRELECTURE_B_CELLULES_ET_CATALOGUE_20260922.md)
sépare déjà catalogue de boules et flux de toutes les présentations.

## Ce que la tour réduite doit conserver

Pour une boule exacte de centre `c`, rayon `R>0`, intérieur global `I`
de cardinal `p` et coquille globale `U` de `u` **positions géométriques
distinctes**, considérer le
rang K avec `1≤t=K−p≤u`. Le cas `K≤p` suit déjà une branche analytique
séparée. Dans la v7, un *sommet réduit strict* est un
`t`-sous-ensemble `S⊂U` dont la boule minimale a un rayon **strictement
plus petit** que `R`. Deux tels sommets sont reliés si leur union est
contenue dans une coface stricte de `t+1` sites. C'est exactement le
graphe parcouru par le DSU de `ShellTable::rank`.

Le [`visit_block` du constructeur FULL](../../morsehgp3D_v7/src/forest/full_ball_tower.hpp)
ne lit, pour ce bloc, que **un représentant par composante de ce graphe**
et la **contribution non couverte** de la coquille/intérieur. Il ne lit
pas `reduced_members`, la liste de tous les sommets stricts, ni les
statistiques DSU. Ces listes sont utiles à l'oracle borné et aux tests,
mais ne sont pas une interface de sortie obligatoire pour la v9.
Déterminer exactement les composantes et leur couverture reste
indispensable ; supprimer la liste ne suffit pas à les trouver.

## Équivalence géométrique exacte

Translater le centre en zéro et noter `v_i=p_i−c` pour chaque site de
coquille. Tous vérifient `|v_i|=R`. Pour tout `S⊂U` non vide,

`S est strict ⇔ 0∉conv({v_i : i∈S}) ⇔ il existe n tel que n·v_i>0 pour tout i∈S`.

Preuve de la première équivalence : si zéro est dans l'enveloppe convexe,
des poids non négatifs de somme 1 donnent, pour tout autre centre `d`,
une moyenne des distances carrées égale à `R²+|d−c|²` ; le rayon minimal
ne peut diminuer. Si zéro n'y est pas, la séparation stricte fournit
`n·v_i>0` ; déplacer `c` d'un petit `εn` diminue **toutes** les distances
de `S`, donc son rayon minimal. La seconde équivalence est le théorème de
séparation pour un ensemble fini compact. Les égalités restent du côté
**non strict** : aucun `n·v_i=0` ne suffit à certifier un sommet strict.

Sur la sphère des directions **orientées** `n` (ne pas identifier `n` et
`−n`), chaque site trace un grand cercle
`n·v_i=0`. Les `u` cercles, après regroupement des cercles confondus,
découpent la sphère en **O(u²) régions ouvertes** même avec
intersections multiples. Dans une région `C`, le signe de chaque produit
scalaire est constant ; poser `P_C={i:n·v_i>0}`. Tous les
`t`-sous-ensembles de `P_C` sont stricts et forment une seule composante
locale : on passe de l'un à l'autre par échanges d'un site, sous une
coface de `t+1` toujours contenue dans `P_C`.

## Graphe compact des régions : proposition et preuve

Garder les régions avec `|P_C|≥t`. Relier deux régions **adjacentes par
une arête** si `|P_C∩P_D|≥t`. Les composantes de ce graphe des régions
correspondent exactement aux composantes du graphe des sommets stricts
de `ShellTable::rank(t)` :

1. Chaque région gardée représente une famille connexe de sommets
   stricts. Deux régions reliées partagent un `t`-sous-ensemble strict ;
   leurs familles sont donc dans la même composante.
2. Chaque arête du graphe v7 vient d'une coface stricte `T` de taille
   `t+1`. Par la séparation ci-dessus, une direction positive pour tout
   `T` existe ; une région ouverte voisine contient alors `T`, donc ses
   deux faces dans la même famille locale.
3. Si un même `t`-sous-ensemble `S` appartient à deux régions non
   adjacentes, son domaine de directions positives est l'intersection
   de demi-espaces ouverts `n·v_i>0`. Ce cône est convexe et, sur la
   sphère, connexe par normalisation d'un segment entre deux directions.
   Une petite perturbation du chemin évite les sommets de l'arrangement
   sans quitter ces inégalités **strictes**. Les régions successives
   gardent donc `S` et sont reliées selon la règle `|P_C∩P_D|≥t`.

Ainsi un algorithme exact d'arrangement et d'adjacence peut fournir
**un `t`-sous-ensemble représentant par composante**, sans construire
les `binomial(u,t)` sommets du quotient. Pour une composante, l'union des
coquilles couvertes est simplement l'union des `P_C` de ses régions :
si `|P_C|≥t`, chaque site de `P_C` appartient à un `t`-sous-ensemble
local. La contribution de coquille est `U` moins l'union des couvertures
de toutes les composantes ; l'intérieur contribue si aucune composante
stricte n'existe et `p>0`. Le maximum de cardinal strict `h` du code v7
est `max_C |P_C|`.

### Deux calculs plus simples que le quotient complet

**Arité minimale `q_min`.** Sous la précondition qu'une présentation
positive de cette BallKey a déjà été certifiée, `0∈conv(U)` et
`q_min∈{2,3,4}`. Une paire diamétrale `p_i+p_j=2c`, retrouvée par
hash/tri de coordonnées exactes, donne `q_min=2`. Sinon, `q_min=3`
exactement lorsqu'un plan passant par `c` contient une famille de sites
dont l'enveloppe convexe contient `c` : sans paire diamétrale, un
triangle de cette famille le contient avec trois poids positifs. On
peut grouper les plans canoniques définis par les `O(u²)` paires de
directions non collinéaires, puis tester l'enveloppement en 2D de chaque
groupe. La somme de tailles des groupes est `O(u²)` : un plan de `r≥2`
sites possède `binomial(r,2)` paires qui le définissent, et chaque paire
n'appartient qu'à un plan. Un tri exact donne
une cible `O(u² log u)` **à vérifier en implémentation**. Si aucun tel
plan n'existe, Carathéodory et la présentation positive fournie donnent
`q_min=4`. Cette route ne prouve pas encore le coût du catalogue global
ni les largeurs entières nécessaires aux plans rationnels.

**Contribution de coquille pour les grandes tailles.** Si
`u≥2t−1`, alors **chaque** site de `U` appartient à un `t`-sous-ensemble
strict et `contribution_shell=∅`, indépendamment du nombre de
composantes. Pour un site `i`, parcourir les directions génériques de
l'équateur `n·v_i=0`. Chacun des `u−1−a` autres sites non antipodaux
est positif pendant la moitié de ce parcours ; l'unique antipode
éventuel (`a∈{0,1}`) n'y est jamais strictement positif. Il existe donc
une direction générique avec au moins la moyenne `(u−1−a)/2` des autres
sites positifs. **Choisir d'abord cette direction**, puis l'incliner
assez peu vers `v_i` pour conserver tous ses signes non nuls : `i`
devient positif et l'antipode reste négatif. Le nombre de sites positifs
atteint au moins `ceil(u/2)≥t`. Pour `K≤10`, on a `t≤10` :
`u≥19` suffit à vider cette contribution. **Cela ne fusionne aucune
composante et ne supprime aucun parent.**

Contre-exemple à cette confusion : sur un grand cercle unité, prendre
les quatre axes `(±1,0,0)`, `(0,±1,0)` et autant de points rationnels
distincts que voulu sur l'arc ouvert du premier quadrant. Au rang
`t=2`, la paire `{(-1,0,0),(0,-1,0)}` est stricte mais isolée : ajouter
un axe positif introduit une paire antipodale ; ajouter un point de
l'arc met les trois directions hors de tout demi-cercle ouvert.
La coquille est arbitrairement grande et sa contribution peut être
vide, pourtant plusieurs représentants de composantes restent requis.

## Portée, pièges et gate de preuve

- `O(u²)` borne le **nombre de régions combinatoires**, pas encore le
  temps ni la mémoire de leur construction. Recalculer un bitset de `u`
  signes ou une couverture de `u` bits à chaque région recréerait
  `O(u³)` travail ; il faut compter mises à jour de signes, intersections,
  DSU, couvertures, exactitude des ordres et octets réellement conservés.
- Les cercles coïncidents (sites antipodaux), les multiplicités aux
  intersections, les coquilles coplanaires et les régions de mesure nulle
  exigent un arrangement exact. Seules les **régions ouvertes** fournissent
  les certificats stricts ; un chemin dans le cône faisable peut être
  perturbé hors des croisements parce qu'il garde une marge stricte sur
  le sous-ensemble `S`. Traverser le cercle commun de deux antipodes
  inverse leurs deux signes à la fois ; la règle d'intersection des
  `P_C` reste nécessaire. Quotienter les directions par `n∼−n` serait
  incorrect, car cela identifie un signe et son complément.
- Les vecteurs `v_i` ont un centre rationnel issu de la `BallKey` ; ni
  les types u16/192 bits de la v7 ni les bornes u18 de l'atlas ne
  qualifient d'office les déterminants et ordres du nouvel arrangement.
  La grille 1 mm est le profil principal v9 ; float32 reste secondaire.
- `q_min`, l'arité minimale d'un support positif de la **boule entière**,
  n'est pas déterminée par une seule région `P_C`. Il faut la calculer
  exactement et conserver `I/U` globaux avant de fournir la boule au
  constructeur FULL. La découverte de **toutes les BallKey utiles** et
  les parents globaux restent des verrous distincts.
- Le masque `representative_shell` de la v7 est un `u16` : pour une
  coquille non bornée, l'interface FULL doit porter des IDs de
  représentants (ou un handle possédé), sans indexer silencieusement au-delà
  du seizième bit. Le quotient local ne prouve ni la production des
  BallKey, ni les incidences ou parents globaux.
- Le coût quadratique en `u` peut encore être rédhibitoire si une coquille
  contient une fraction macroscopique des points ; cette proposition ne
  prouve aucune borne sous-quadratique globale en `n`. Elle remplace
  seulement un **obstacle exponentiel artificiel** par une cible
  combinatoire de dimension fixe à instruire.

Gate initial : pour chaque coquille v7 admissible `u≤12`, comparer à
`ShellTable` **la partition entière des `t`-sous-ensembles stricts**
(via oracle borné), `h`, la couverture et les représentants acceptés par
`visit_block`, pas seulement le nombre de composantes. Ajouter antipodes,
points coplanaires, cercles confondus, intersections multiples, puis la
fixture coquille30 déjà disponible et la paire isolée du carré avec arc.
Au-delà de `u=12`, ne pas prétendre à une comparaison exhaustive avec
`ShellTable` : vérifier plutôt des identités géométriques et partitions
sur des familles à réponse connue, plus des échantillons exacts de
sous-ensembles. Après ce gate, mesurer séparément
arrangement, calcul des signes, DSU, q_min et résolution des parents.
Une implémentation GPU n'est pertinente qu'après preuve et bilan de
travail CPU local sur ces objets.

### Contrôle exploratoire indépendant du 22 septembre

Un calcul exact **en mémoire**, non archivé comme gate de production, a
comparé pour six coquilles entières et `t=1..4` les partitions complètes
des sous-ensembles stricts : oracle direct par Carathéodory (supports de
taille ≤4) et cofaces strictes ; indépendamment, motifs de signes
réalisables par séparation stricte, cercles confondus regroupés et graphe
des régions. Les **24 partitions et couvertures** ont coïncidé. Le tableau
donne les tailles des composantes, non les seuls nombres de régions :

| Coquille | `u` | Régions | `t=1` | `t=2` | `t=3` | `t=4` |
|---|---:|---:|---|---|---|---|
| Octaèdre, axes `±5` | 6 | 8 | `[6]` | `[12]` | `[1×8]` | vide |
| Coplanaire, axes `±5` et `(±3,±4,0)` | 8 | 8 | `[8]` | `[24]` | `[24]` | `[1×8]` |
| Tétraèdre entier régulier | 4 | 14 | `[4]` | `[6]` | `[1×4]` | vide |
| Carré et deux points rationnels d'arc | 6 | 8 | `[6]` | `[1,12]` | `[10]` | `[1×3]` |
| Mixte 3D de rayon `5` | 8 | 24 | `[8]` | `[26]` | `[43]` | `[38]` |
| Cube `{±1}³` | 8 | 14 | `[8]` | `[24]` | `[32]` | `[1×14]` |

La ligne carré+arc prend les axes `±(1313,0,0)`, `±(0,1313,0)` et
`(1287,260,0)`, `(1212,505,0)` ; elle conserve bien la composante de
paire isolée. Ce contrôle ciblé renforce la contrelecture mais **ne
remplace ni le code de test archivé ni les fixtures v7/u18 à porter**.

### Cible d'implémentation à contre-auditer

Une construction explicite de l'arrangement peut éviter les bitsets de
`u` sites **par région**. Après regroupement des grands cercles
coïncidents, leurs sommets sont les directions rationnelles
`±(v_i×v_j)` pour les normales indépendantes ; grouper les intersections
multiples, trier exactement les sommets le long de chaque cercle et
construire les adjacences orientées. Le nombre de sommets, arêtes et
régions est `O(u²)` ; la cible de tri est `O(u² log u)` comparaisons
exactes, **sous réserve d'une construction DCEL prouvée** pour les cas
dégénérés, y compris un seul cercle. Pour `m≤u` cercles distincts,
chaque paire a deux intersections orientées, donc
`Σ_x binomial(k_x,2)=m(m−1)` pour les multiplicités `k_x≥2` des sommets ;
la somme des incidences à trier reste `O(u²)` même si beaucoup de
cercles concourent. Les directions de sommet se gardent en coordonnées
homogènes rationnelles : aucune racine carrée n'est requise. Digones,
arcs antipodaux et `m=1` demandent un traitement explicite.

Sur un parcours DFS des régions, un ensemble ordonné **mutable et
réversible** des sites positifs ne change que pour le cercle traversé :
au plus deux sites antipodaux basculent, puisque les positions sont
distinctes. On maintient `|P_C|` et `h` sans recalculer les `u` signes.
Pour chaque région active (`|P_C|≥t`), les `t` plus petits indices
positifs donnent le plus petit masque numérique strict contenu dans
`P_C`. Un DSU des régions actives fusionne les voisines seulement si
`|P_C∩P_D|≥t`, en gardant le minimum de ces représentants ; ce minimum
par composante reproduit le choix du balayage des masques de la v7.
Comparer par valeur du **masque numérique** (ordre colex sur les indices),
pas par ordre lexicographique croissant des listes : `{0,3}` vaut `9`,
alors que `{1,2}` vaut `6`. Le stockage naïf d'un candidat de `t` IDs
par région prend `O(tu²)` mots ; pour `K≤10`, c'est `O(u²)` avec une
constante physique à mesurer.
Une paire antipodale échange deux signes sans changer `|P|` : calculer
**l'intersection**, pas le minimum des deux comptes. Pour une arête qui
traverse un groupe `G` de `|G|∈{1,2}` sites dont les signes basculent,
`|P_C∩P_D|=(|P_C|+|P_D|−|G|)/2` : le test se fait sans bitset.
Les opérations DSU
coûtent `O(α(u²))` amorti, non une constante stricte.

Pour la contribution globale, si `u≥2t−1`, le lemme ci-dessus la donne
vide sans visite des couvertures. Sinon, pour la tour `K≤10`, on a
`u≤18` et un masque de 32 bits suffit à accumuler exactement l'union
des `P_C` actifs. Il y a alors au plus `u(u−1)+2≤308` régions ; même
un balayage direct de couverture coûte au plus `18×308=5 544` tests
de sites par rang. Un Fenwick ou autre ordre-statistique sans allocation
par région peut maintenir les positifs et sortir `t` indices en
`O(t log u)` par région active. Cela suggère, **en nombre d'opérations
combinatoires et pour K fixé ≤10**, une cible
`O(u² log u + t u² log u)` avec `O(u²)` mémoire, plus le calcul séparé
de `q_min` et les représentants réellement émis. Ce n'est pas encore
une borne prouvée pour un code ni une borne globale en nombre de points :
largeurs arithmétiques, construction exacte des adjacences, population
des coquilles et découverte des BallKey restent à qualifier. Une grande
coquille de taille proportionnelle à `n` peut encore coûter `Ω(n²)`.

**Suivi du commit `8054540c`.** La [contrelecture A](CONTRE_AUDIT_A_MATH_MOTEUR_20260922.md)
factorise les normales des plans de `q_min=3` en u18, bornées en i128,
et publie un [oracle ciblé](check_qmin_planes_u18_20260922.py) passé
avec et sans `-O` (quatre fixtures et 755 sous-coquilles sans antipodes).
Cela ferme une partie du calcul de `q_min` sous présentation positive,
mais **pas** les prédicats ni les largeurs de l'arrangement des grands
cercles, les composantes ou le catalogue FULL.

**Gate du tri circulaire, distinct de l'identité de plans.** Poser
`w_i=2A(p_i−c)` et `N_ij=w_i×(p_j−p_i)` pour les intersections du grand
cercle de `i`. L'identité exacte
`N_ij×N_ik=D w_i`, où `D=N_ij·(p_k−p_i)`, évite de matérialiser le
produit vectoriel potentiellement plus large que `D`. Mais si le tri
projette les normales sur les deux coordonnées restantes **dans l'ordre
croissant** après suppression de l'axe `r` (`w_i[r]≠0`), son déterminant
orienté vaut `(-1)^r D w_i[r]` : l'axe `r=1` **inverse le signe**.
Fixture entière sur une sphère unité : `c=(1,1,1)`,
`p_i=(1,2,1)`, `p_j=(2,1,1)`, `p_k=(1,1,2)`, `A=1` donnent
`w_i=(0,2,0)`, `N_ij=(0,0,-2)`, `N_ik=(2,0,0)`, `D=-2`, mais le
déterminant projeté en `(x,z)` vaut `+4`. Une convention cyclique
d'axes est possible, à condition de la fixer et de la tester.

Même avec ce signe corrigé, un déterminant relatif ne définit **pas**
un ordre total autour de `2π`. Le comparateur doit d'abord classer les
deux demi-plans dans le plan orienté, puis employer ce déterminant pour
les rayons dans une même moitié. `D=0` exige de séparer rayon identique,
rayon antipodal et intersection multiple, après exclusion des normales
nulles des cercles coïncidents. Le contrôle local d'identité algébrique
ne constitue donc pas un gate de tri ou de DCEL : imposer des tests
`r=0,1,2`, changement de demi-plan, `D=0` dans ses deux sens,
coïncidences et intersections multiples, puis comparer l'ordre et les
adjacences à un oracle exact borné.
