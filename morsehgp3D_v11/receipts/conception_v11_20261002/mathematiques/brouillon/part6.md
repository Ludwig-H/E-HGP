
## 9. Ce que la thèse énonce, et ce qui en diffère

Références : manuscrit, Parties I et II (pages PDF 35 à 134). Ni la thèse ni la v10 ne sont des autorités ; chaque ligne dit ce que ce document reprend, redémontre ou réfute.

| Thèse | Ici | Différence |
| --- | --- | --- |
| niveau = rayon $r$, boules fermées (déf. 7, 20) | niveau $a = r^{2}$, coupes fermée et ouverte | changement de variable croissant ; la coupe ouverte est ajoutée |
| $\Gamma_K$ : sommets = $K$-parties du complexe de Čech, adjacence « la réunion est un simplexe », sans borne de taille (déf. 21) ; les adjacences élémentaires suffisent (prop. 5) | $\Gamma_k(a)$ à adjacences élémentaires | aucune : la prop. 5 est l'argument d'échange de NERF-2 |
| $K$-polyèdres = amas discrets (th. 2) | NERF-2 et NERF-4, redémontrés ; coupe ouverte et naturalité verticale en plus | aucune sur la coupe fermée |
| amas discret $= X \cap \delta_r(C)$ ; « poser $C \cap X$ aurait été une erreur » (déf. 8, remarque 1) | `cover` suit la définition 8 ; `core` est $C \cap X$, objet différent et déclaré | `core` n'est pas l'amas discret de la thèse |
| position générale exigée pour les th. 4 à 7 (déf. 26) | aucune hypothèse | sous la déf. 26 toute coquille est régulière (appliquer la déf. 26 à $\sigma = S^{*}$) ; ici les coquilles étendues sont traitées par le quotient local |
| un simplexe $K$-séparant est de Gabriel (th. 4) | cas régulier de LOC-W | vrai sous la déf. 26 : une $(k+1)$-partie $\sigma$ non de Gabriel a une boule $b = B(\sigma)$ avec $p + m \geq k + 2$ et $m = q$, donc $k \leq p + q - 2$ : inerte |
| le $K$-graphe de Gabriel contient toutes les fusions utiles (prop. 6) ; le $K$-arbre couvrant minimal élagué redonne les $K$-polyèdres (th. 5) | **faux en général** : THESE-5 | remplacé par TOUR-E |
| les simplexes de Gabriel sont portés par la mosaïque de Delaunay d'ordre $K$ (th. 6, 7) | non utilisé | invariant d'architecture : la mosaïque d'ordre supérieur n'est jamais matérialisée |
| $K \leq \lvert X \rvert - 1$ au chapitre 8 ; un seul $K$ à la fois | tous les ordres $k \leq \min(K, n)$, $k = n$ compris, et les applications verticales | la tour est un ajout du projet |
| partition stricte par vote pondéré (§ 9.1, prop. 7) | hors de ce document | une sélection fixée donne une partition, pas des partitions emboîtées |

**THESE-5 (le théorème 5 du manuscrit est faux en général).** [faux] Énoncé de la thèse (déf. 28 à 30, prop. 6, th. 5) : sous la position générale de la déf. 26, pour tout $r$, les ensembles de points des composantes non réduites à un sommet du $K$-graphe de Gabriel élagué à $r$ sont les $K$-polyèdres de Čech non réduits à une $K$-partie isolée ; ce graphe a pour sommets les $K$-parties qui sont facettes d'au moins un simplexe de Gabriel à $K + 1$ points (simplexe dont la plus petite boule ouverte ne contient aucun point hors de lui) et relie entre elles les facettes de chaque tel simplexe.

Contre-exemple exact, $K = 2$, cinq points du plan : $A = (0,100,0)$, $C = (200,100,0)$, $z = (101,10,0)$, $y = (130,15,0)$, $w = (103,400,0)$.

- La déf. 26 est satisfaite : aucun point hors de $\sigma$ n'est sur la frontière de $B(\sigma)$, pour chacune des parties $\sigma$ d'au moins deux points.
- Les triangles de Gabriel sont exactement $Czy$ (niveau $17901/4$), $Azy$ ($24125/4$) et $ACw$ ($10001440081/360000$). La paire $AC$ a pour plus petite boule sa boule diamétrale, de niveau 10000, qui contient $z$ et $y$ strictement : les triangles $ACz$ et $ACy$ ont cette même boule et ne sont pas de Gabriel.
- Le graphe de Gabriel a donc, à tout niveau $\geq 10001440081/360000$, deux composantes : $\lbrace Cz, Cy, zy, Az, Ay \rbrace$, de points $\lbrace A, C, z, y \rbrace$, et $\lbrace AC, Aw, Cw \rbrace$, de points $\lbrace A, C, w \rbrace$. Il n'est jamais connexe : le « $K$-arbre couvrant minimal » n'existe pas comme arbre.
- $\Gamma_2$ n'a qu'une composante, de points $\lbrace A, C, z, y, w \rbrace$, à partir du même niveau ; son arbre de fusion a huit nœuds, la racine étant la fusion ternaire de niveau $10001440081/360000$.

Le désaccord est permanent. Vérifié par l'oracle de définition (pièce `these_th5.py`). La fixture E5 du registre racine (`gabriel-point-set-counterexample-5-points-v1`, points $(0,0,7)$, $(0,9,6)$, $(1,4,0)$, $(0,0,1)$, $(4,1,2)$) donne un désaccord de même nature, limité à l'intervalle de niveaux $\left[ 83886/3563, 24 \right)$.

*Où la preuve de la proposition 6 échoue.* La récurrence porte sur les **ensembles de points** des composantes. Quand un simplexe non de Gabriel $\sigma$ naît au niveau $r$, la preuve constate que ses facettes nées au même niveau « ne changent pas l'ensemble de points de la composante » : c'est vrai au niveau $r$. Mais l'hypothèse de récurrence ne retient pas que ces facettes **appartiennent désormais à cette composante**. Quand l'une d'elles est plus tard la facette active d'un simplexe de Gabriel, $\Gamma_K$ réunit sa composante aux autres, alors que le graphe de Gabriel n'y voit qu'un sommet neuf. L'égalité des ensembles de points n'est pas un invariant de récurrence. Ici la facette est $AC$, née au niveau 10000 dans $ACz$ et $ACy$, puis active dans $ACw$.

*Ce qui est vrai à la place.* TOUR-E : les cellules de fenêtre (ici la jonction $ACw$, de représentants $AC$, $Aw$, $Cw$) et la **descente** de chaque représentant. Le représentant $AC$ a deux sites strictement intérieurs à sa plus petite boule : saut vers $\lbrace z, y \rbrace$, naissance de niveau $433/2$, dont l'ancêtre au niveau de $ACw$ est la bonne composante.

## 10. Multiplicités

Une entrée peut porter des positions répétées : un site $x$ a un poids $w(x) \geq 1$, $W = \sum_x w(x)$. Un **point** est une copie $(x, i)$, $1 \leq i \leq w(x)$. La sémantique visée est « un doublon est un point » : $D_k(y)$ compte les copies ; une $k$-partie est un ensemble de $k$ copies ; le niveau d'une partie, les enveloppes convexes et la séparabilité se lisent sur ses positions ; $p$ et $m$ deviennent des nombres de copies ; $q$ et $S^{*}$ restent comptés en positions.

**MULT-1 (ce qui reste vrai pour des copies).** [démontré par transfert] Avec ce dictionnaire, les énoncés suivants restent vrais et leurs preuves s'appliquent mot pour mot, parce qu'elles n'emploient que la plus petite boule d'un ensemble de positions et des échanges d'éléments d'une partie, jamais le fait que deux éléments d'une partie ont des positions distinctes : CAD-2, CAT-2, NERF-1 à NERF-4, LOC-1 à LOC-4, LOC-W (une partie portée par moins de $q$ positions est séparable), TOUR-A (i) à (iii), TOUR-B (0) à (2), TOUR-C, TOUR-D, TOUR-E, TOUR-F, TOUR-G (1) et (4), TOUR-J, TOUR-J2, INV-RACINE, INV-PLATEAU, PTS-CORE, PTS-COVER, PTS-ENC, PTS-LAM ; et, en remplaçant « nombre de sites » par « poids », GEN-D à GEN-G, avec l'admission $p + q \leq K + 1$ et les seuils $\theta_r$ en poids. CAT-1, CAT-3, GEN-Z et LOC-5 portent sur des positions et ne changent pas.

**MULT-2 (ce qui change).**

1. *Niveau nul.* Un site de poids $w$ est une boule de rayon nul ($I = \emptyset$, $U$ = ses $w$ copies, $q = 1$) : à chaque ordre $k \leq w$ elle porte une naissance de niveau 0, dont la composante est l'ensemble des $k$-parties de ses copies ; sa fenêtre est $\left[ 1, \min(K, w) \right]$. TOUR-A (iv) et TOUR-B (3) sont remplacés par cet énoncé ; dans TOUR-D une partie de niveau nul est un sommet de cette naissance.
2. *Coquilles régulières.* LOC-R et TOUR-G (2) ne valent que pour une coquille sans position répétée. Sinon la cellule se décide par le quotient sur les copies, et les ordres à événement d'une fenêtre ne forment **pas** un intervalle. Fixtures : `paire_31`, sites $(12,12,12)$ de poids 3 et $(14,12,12)$ de poids 1 : la boule diamétrale, de niveau 1, est une jonction à l'ordre 1, une continuation avec gain de couverture aux ordres 2 et 3, une naissance à l'ordre 4. `triangle_311`, sites $(0,0,0)$ de poids 3, $(6,0,0)$ et $(3,5,0)$ de poids 1 : la boule circonscrite, de niveau $289/25$, est sans événement à l'ordre 1 (inerte) et à l'ordre 3 (un seul morceau), une jonction aux ordres 2 et 4, une naissance à l'ordre 5.
3. *Euler.* Dans INV-EULER, $n \left[ k = 1 \right]$ devient le nombre de sites de poids $\geq k$, et $e_k(b)$ somme sur les parties $B$ de copies de $U$ ; la preuve est la même.
4. *Points.* Toutes les copies d'un site ont les mêmes dates et les mêmes propriétaires (échanger deux copies d'un site est un automorphisme de tous les objets).

**MULT-3 (ce qui reste à rédiger ou à décider).** Tant que ces points ne sont pas levés, la tour **refuse** une entrée pondérée (`unsupported_degeneracy`), et le produit applique ce refus au plus tôt.

1. Décision de l'utilisateur : « un doublon est un point », ou dédoublonnage déclaré à l'entrée.
2. Contre-lecture de MULT-1 énoncé par énoncé par un tiers. État : relecture de l'auteur de ce document ; force brute exacte sur 36 nuages à positions répétées (pièce `verif_enonces.py`, mode copies) ; l'oracle de `reference/` juge ses deux étages l'un contre l'autre sur des nuages à doublons ; aucun binaire ne confirme une tour pondérée (la v10 la refuse).
3. Quotient local et identité d'Euler en temps borné quand les poids sont grands : regroupement des parties de copies par ensemble de positions. Non rédigé ici.
4. Enregistrements du catalogue (poids, drapeau de coquille pondérée, boules de rayon nul) et contrat des entrées de points pour des copies : à écrire avec le module qui les sert.

## 11. Statuts, obligations, pièces

### 11.1 Table des statuts

Statuts au sens du registre racine. « Oracle » désigne la référence bornée de `reference/` (étage A, définition ; étage B, constructif ; porte `mhgp11_reference_fast` et ses mutants). « P1 » à « P4 » sont les pièces du § 11.3. « Porte à créer » : contrôle exigé du moteur, qui n'existe pas encore.

| Id | Énoncé | Statut | Preuve | Contrôle | Fixture d'égalité |
| --- | --- | --- | --- | --- | --- |
| CAD-S | séparation (Gordan) | `theorem_external` | classique | — | — |
| CAD-1 | plus petite boule englobante | `proved_here` | § 1.2 | P1 | — |
| CAD-2 | régions témoins | `proved_here` | § 1.3 | oracle A | — |
| CAD-3 | niveaux rationnels, ordre exact | `proved_here` | § 1.4, CAT-3 | porte à créer (module `num`) | `circle25_pair`, `double_collision` |
| CAT-1 | supports | `proved_here` | § 2.2 | P1 | `triangle_rectangle`, `carre` |
| CAT-2 | boule minimale d'une partie d'une boule | `proved_here` | § 2.2 | P1 | — |
| CAT-3 | centre et niveau exacts | `proved_here` | § 2.2 | P2 ; porte à créer (`num`) | `tetra_face_obtuse` |
| CAT-ADM | l'admission $p + q \leq K + 1$ suffit | `proved_here` | LOC-W, TOUR-B | oracle (B contre A) | `ligne_024` ; `triangle_rectangle` (non nécessaire) |
| GEN-D, GEN-DLOC | dominance | `proved_here` | § 3.1 | P2 | `dominance_face`, `dominance_coin` |
| GEN-L | restriction des listes | `proved_here` | § 3.1 | P2 | — |
| GEN-C | recensement local | `proved_here` | § 3.2 | P2 | `triangle_rectangle_interieur` |
| GEN-A, GEN-P | ajustement, partition des centres | `proved_here` | § 3.3 | P2 | `carre` (centres sur la face haute de la boîte englobante) |
| GEN-M | masques de dominance | `proved_here` | § 3.4 | P2 | — |
| GEN-Z | droite des équidistants | `proved_here` | § 3.4 | P2 | `droite_sommet`, `droite_arete` |
| GEN-S | survie du support canonique | `proved_here` | § 3.5 | P2 | `ligne_024`, `triangle_aigu_interieur`, `tetra_centre`, `tetra_face_obtuse` |
| GEN-E | émission unique | `proved_here` | § 3.5 | P2 | `carre`, `cube` |
| GEN-G | théorème du générateur | `proved_here` | § 3.6 | P2 ; porte à créer (catalogue contre oracle) ; INV-RESTR | toutes les précédentes |
| GEN-F | listes inhérentes | `proved_here` | § 3.7 | — | `sphere_24` |
| GEN-COUT (1) | sortie non linéaire dans le pire cas | `proved_here` | § 3.7 | oracle B (catalogue par force brute) | `arcs_enlaces_16` |
| GEN-COUT (coût) | linéarité en la sortie, loi de la marge de feuille | `experimental_target` | aucune | portes d'échelle à créer | `amas_coins_14` (v10) |
| NERF-1 à NERF-4 | nerf, coupes fermée et ouverte, naturalité, couverture | `proved_here` (coupe fermée : aussi `theorem_external`, thèse th. 2 et prop. 5) | § 4 | oracle A (par construction) ; P1 | — |
| LOC-1, LOC-2 | échanges, structure stricte | `proved_here` | § 5.2 | P1 | — |
| LOC-3 | morceaux (surjection, raffinement exhaustif) | `proved_here` | § 5.3 | P1 ; oracle B | `morceaux_non_injectifs` |
| LOC-4, LOC-5 | quotient par famille couvrante | `proved_here` | § 5.3 | porte à créer (quotient contre énumération brute) | `carre`, `cube`, `cercle_12` |
| LOC-W | fenêtre de rang | `proved_here` | § 5.4 | P1 | `triangle_scalene` (boule du triangle : inerte à l'ordre $p + q - 2 = 1$, jonction à l'ordre $p + q - 1 = 2$) |
| LOC-R | coquille régulière | `proved_here` | § 5.4 | P1 | — |
| LOC-GEO | lecture géométrique des morceaux | `proved_here` | § 5.4 | — | — |
| TOUR-A | arbre de fusion | `proved_here` | § 6.1 | oracle A | — |
| TOUR-B | classification des événements | `proved_here` | § 6.2 | P1 ; oracle (B contre A) | `triangle_rectangle`, `carre`, `morceaux_non_injectifs` |
| TOUR-C | plateaux atomiques | `proved_here` | § 6.3 | P1 ; mutants de la référence | `carre` (ordre 1), `ligne_024` (ordre 1) |
| TOUR-D | descente | `proved_here` | § 6.4 | P1 ; INV-NEUTRE | `ligne_024` (terminal non unique), `these_th5_plan` (saut) |
| TOUR-E | exactitude relative au catalogue | `conditional_theorem` (relatif à H2) | § 6.5 | oracle (B contre A) ; porte à créer (tour contre oracle, arbre étiqueté) | `triangle_scalene` (catalogue amputé) |
| TOUR-F | verticales à la coupe fermée | `proved_here` | § 6.6 | oracle (B contre A) | `carre` (ordres 2 et 3) |
| TOUR-G | couvertures | `proved_here` ; (3) `false_in_general` pour « union des naissances » | § 6.7 | P1 | `gain_de_couverture`, `internal_k3` |
| TOUR-J, TOUR-J2 | construction sans lots | `proved_here` | § 6.8 | P3 | `carre` (ordre 1) |
| INV-EULER | identité d'Euler | `proved_here` ; « Euler certifie le catalogue » : `false_in_general` | § 7.1 | P1 ; porte d'échelle à créer | `carre`, `triangle_scalene`, `euler_compensation` |
| INV-EMST | ordre 1 = lien simple | `proved_here` | § 7.2 | porte d'échelle à créer (scikit-learn) | `carre` (ordre 1) |
| INV-RESTR, INV-RACINE, INV-PLATEAU, INV-VIVANT, INV-NEUTRE | invariants linéaires | `proved_here` | § 7.3 | portes d'échelle à créer | — |
| PTS-CORE, PTS-COVER | entrées `core` et `cover` | `proved_here` | § 8.2 | P1 ; oracle | `ligne_024`, `cover_tie_n4` |
| PTS-ENC, PTS-LAM, PTS-ANC, PTS-NP, PTS-MONO, PTS-K1 | encadrement, laminarité, monotonies | `proved_here` | § 8.2 | P1 (PTS-ENC) | `ligne_012` |
| PTS-STAB | stabilité de `core`, constante 2 | `proved_here` | § 8.2 | — | `stab_x`, `stab_y` |
| PTS-MR | encadrement par l'atteignabilité mutuelle | `proved_here` | § 8.2 | — | — |
| PTS-DEP | pas de propriétaire unique équivariant | `proved_here` | § 8.3 | — | `ligne_024` |
| PTS-X1 à PTS-X5 | stabilité, verticalité, respect du cœur de `cover` ; partition des amas discrets | `false_in_general` | § 8.3 | oracle A | `f2_gauche`, `f2_droite`, `ligne_01269`, `firstcov_k3_n6`, `internal_k3`, `ligne_024` |
| THESE-5 | th. 5 et prop. 6 du manuscrit | `false_in_general` | § 9 | P4 ; oracle A | `these_th5_plan`, `e5` |
| MULT-1, MULT-2 | énoncés pour des copies | `proved_here` par transfert, contre-lecture exigée avant usage | § 10 | P1 (mode copies) ; oracle sur nuages à doublons | `paire_31`, `triangle_311` |

### 11.2 Obligations restantes

1. **OBL-1.** Contre-lecture par un tiers de toutes les preuves de ce document, en priorité LOC-3, LOC-5, TOUR-B, TOUR-D, TOUR-E, TOUR-G (1) et INV-EULER ; inscription au registre racine après contre-lecture.
2. **OBL-2.** Exactitude des prédicats : chaque expression de CAT-3, GEN-DLOC, GEN-Z et du test « centre dans le pavé » doit porter son budget de bits (architecture § 3) ; aucun énoncé d'ici ne vaut pour un prédicat approché.
3. **OBL-3.** Grandes coquilles : limite unique et déclarée du nombre de sites d'une coquille, commune au catalogue et à la tour ; borne du nombre de membres de la famille de LOC-5 et du coût du quotient ; juge indépendant au-delà de 14 sites (l'oracle de définition s'arrête là).
4. **OBL-4.** Coût du générateur : aucune borne du nombre de nœuds, de feuilles ni de recensements ; aucune loi en fonction de la marge entre taille de feuille et $K$. Cible expérimentale, sous budget de nœuds.
5. **OBL-5.** Longueur des descentes : aucune borne autre que le nombre de niveaux.
6. **OBL-6.** Taille de la sortie par site sur les familles du contrat : mesurée, non bornée (GEN-COUT (1) interdit toute borne linéaire générale).
7. **OBL-7.** Complétude à l'échelle : INV-EULER est nécessaire, pas suffisant ; restent à écrire le refus d'une boule de fenêtre absente (TOUR-E, portée) et un juge d'échantillon par recensement brut.
8. **OBL-8.** Hiérarchie de points : aucune continuité démontrée hors `core` ; la convention de propriétaire unique de `cover` est une décision de produit (PTS-DEP).
9. **OBL-9.** Multiplicités : MULT-3.
10. **OBL-10.** Signaler à l'auteur de la thèse l'énoncé à corriger (th. 5, prop. 6) et proposer TOUR-E comme énoncé de remplacement.

### 11.3 Pièces

Scripts exacts (fractions), hors dépôt, sous `build/v11-persist/mathematiques/pieces/`. Ils ne prouvent rien : ils ont cherché un contre-exemple à chaque énoncé, sans en trouver.

| Pièce | Ce qu'elle confronte à la définition | Volume du 2 octobre 2026 |
| --- | --- | --- |
| P1 `verif_enonces.py` | CAT-1, CAT-2, LOC-1 à LOC-3, LOC-W, LOC-R, TOUR-B, TOUR-C, TOUR-D, TOUR-G, PTS-CORE, PTS-COVER, PTS-ENC, INV-EULER, INV-RACINE, par force brute sur $\Gamma_k$, sans aucun import du dépôt | 67 nuages de sites distincts (13 fixtures, 54 nuages de neuf familles dégénérées ou génériques, 4 à 8 sites, tous les ordres) et 36 nuages à positions répétées : 0 écart |
| P2 `verif_generateur.py` | modèle exécutable du générateur (GEN-D à GEN-G, poids compris) contre le catalogue de la définition ; GEN-DLOC contre les coins ; GEN-Z contre l'intersection exacte | 33 nuages, 396 exécutions, 2 299 boules attendues : 0 écart |
| P3 `verif_cartesienne.py` | TOUR-J contre TOUR-C sur des hypergraphes à rangs répétés | 3 000 essais, 5 564 nœuds dont 3 893 à trois enfants ou plus : 0 écart |
| P4 `these_th5.py` | THESE-5 par l'oracle de définition | désaccord permanent confirmé ; E5 : désaccord à un niveau |
| `fixtures_oracle.py`, `fixtures_catalogue.py` | valeurs exactes des fixtures, lues dans la référence | — |

### 11.4 Fixtures citées

Coordonnées entières, dans le domaine du profil à 18 bits. La liste complète à graver dans les portes, avec l'attendu exact de chaque fixture, est tenue à part (`FIXTURES.md` des pièces).

| Fixture | Sites |
| --- | --- |
| `ligne_024`, `ligne_012`, `ligne_01269` | sur l'axe des $x$ : abscisses $(0, 2, 4)$ ; $(0, 1, 2)$ ; $(0, 1, 2, 6, 9)$ |
| `f2_gauche`, `f2_droite` | abscisses $(0, 999, 2000)$ ; $(0, 1001, 2000)$ |
| `stab_x`, `stab_y` | abscisses $(1, 11)$ ; $(0, 12)$ |
| `triangle_rectangle` ; `triangle_rectangle_interieur` | $(0,0,0)$, $(3,0,0)$, $(0,4,0)$ ; avec en plus $(1,1,0)$ |
| `triangle_scalene` | $(0,0,0)$, $(6,0,0)$, $(2,5,0)$ |
| `triangle_aigu_interieur` | $(0,0,0)$, $(6,0,0)$, $(3,5,0)$, $(3,2,0)$ |
| `carre` | $(0,0,0)$, $(2,0,0)$, $(0,2,0)$, $(2,2,0)$ |
| `cube` | les huit points de $\lbrace 0, 2 \rbrace^{3}$ |
| `tetra_centre` | $(0,0,0)$, $(2,2,0)$, $(2,0,2)$, $(0,2,2)$, $(1,1,1)$ |
| `tetra_face_obtuse` | $(0,4,4)$, $(1,2,0)$, $(1,2,4)$, $(4,4,0)$ |
| `circle25_pair` | $(15,10,3)$, $(7,14,3)$, $(7,6,3)$, $(40,40,40)$, $(50,40,40)$ |
| `double_collision` | $(0,0,0)$, $(78404,0,0)$, $(39202,55440,0)$, $(150000,150000,150000)$, $(198046,200290,195586)$ : niveaux exacts $1728896403$ et $1728896403 + 1/768398400$, de même valeur en binaire64 |
| `cercle_12`, `sphere_24` | les 12 points entiers de $x^{2} + y^{2} = 25$, translatés de $(5,5,0)$ ; les 24 points entiers de $x^{2} + y^{2} + z^{2} = 5$, translatés de $(2,2,2)$ |
| `amas_coins_14` | $(1,7,1)$, $(3,1,5)$, $(3,1,7)$, $(3,3,3)$, $(4,5,0)$, $(5,5,0)$, $(6,0,6)$, $(262137,262136,6)$, $(262138,262140,5)$, $(262138,262141,4)$, $(262139,262136,0)$, $(262140,262140,4)$, $(262141,262140,3)$, $(262143,262140,5)$ |
| `morceaux_non_injectifs` | $(0,0,0)$, $(2,0,0)$, $(4,0,0)$, $(2,3,0)$ |
| `gain_de_couverture` | $(8,9,0)$, $(5,10,0)$, $(2,9,0)$, $(5,0,0)$ |
| `euler_compensation` | $(0,5,0)$, $(8,9,0)$, $(8,1,0)$, $(35,5,0)$, $(45,5,0)$ |
| `firstcov_k3_n6` | $(2,4,4)$, $(2,8,5)$, $(2,9,1)$, $(3,7,0)$, $(6,9,6)$, $(8,10,7)$ |
| `internal_k3` | $(15,4,0)$, $(5,4,0)$, $(7,8,0)$, $(7,0,0)$, $(1,4,0)$, $(0,4,1)$ |
| `cover_tie_n4` | $(7,6,10)$, $(6,7,10)$, $(10,5,10)$, $(5,10,10)$ |
| `these_th5_plan` | $(0,100,0)$, $(200,100,0)$, $(101,10,0)$, $(130,15,0)$, $(103,400,0)$ |
| `e5` | $(0,0,7)$, $(0,9,6)$, $(1,4,0)$, $(0,0,1)$, $(4,1,2)$ |
| `arcs_enlaces_16` | $(130000,0,0)$, $(129970,2786,0)$, $(129881,5570,0)$, $(129731,8351,0)$, $(129523,11129,0)$, $(129255,13902,0)$, $(128927,16668,0)$, $(128540,19427,0)$, $(30,0,2786)$, $(119,0,5570)$, $(269,0,8351)$, $(477,0,11129)$, $(745,0,13902)$, $(1073,0,16668)$, $(1460,0,19427)$, $(1906,0,22177)$ |
| `paire_31`, `triangle_311` | poids entre crochets : $(12,12,12)$ [3], $(14,12,12)$ [1] ; $(0,0,0)$ [3], $(6,0,0)$ [1], $(3,5,0)$ [1] |
| `dominance_face`, `dominance_coin` | prédicat GEN-DLOC : pavé $\left[ 10, 14 \right] \times \left[ 0, 4 \right]^{2}$, $z = (6,2,2)$ ne domine pas $x = (22,2,2)$ (égalité sur une face) et domine $x = (23,2,2)$ ; pavé $\left[ 0, 4 \right]^{3}$, $z = (0,0,0)$ ne domine pas $x = (8,8,8)$ (égalité en un coin) et domine $x = (8,8,9)$ |
| `droite_sommet`, `droite_arete` | prédicat GEN-Z : sites $(0,0,1)$, $(4,0,1)$, $(2,2,3)$, la droite touche le pavé $\left[ 2, 3 \right] \times \left[ 1, 2 \right] \times \left[ 1, 2 \right]$ en son seul sommet $(2,1,1)$ et manque $\left[ 2, 3 \right] \times \left[ 1, 2 \right] \times \left[ 2, 3 \right]$ ; sites $(0,0,0)$, $(2,0,0)$, $(0,2,0)$, la droite longe une arête de $\left[ 1, 2 \right]^{2} \times \left[ 0, 1 \right]$ et manque $\left[ 1, 2 \right] \times \left[ 2, 3 \right] \times \left[ 0, 1 \right]$ |
