# Référence exacte bornée de la v11

Python 3.10 nu (`fractions`, `itertools`), aucune dépendance, aucun flottant dans une décision. Ce dossier établit la
**vérité sur de petits nuages** ; il jugera le catalogue et la tour du moteur. Il ne dépend d'aucun module C++.

```text
phase=exploration_v11_hors_registre   mode=fondations   public_status=not_claimed
```

## Principe : la vérité est la définition, pas un algorithme

Deux étages sans idée commune, qui remplissent le même enregistrement canonique (`hgp11_ref/model.py`) :

| Étage | Fichier | Ce qu'il est | Arithmétique |
| --- | --- | --- | --- |
| **A, définition** | `hgp11_ref/definition.py` | le graphe $\Gamma_k$ de la thèse, exhaustif : sommets = $k$-parties au niveau du rayon carré de leur boule minimale, arêtes = $(k+1)$-parties au niveau de la leur | `Fraction`, élimination de Gauss ; boule minimale par énumération des supports (pas Welzl) |
| **B, constructive** | `hgp11_ref/constructive.py`, `hgp11_ref/intgeom.py` | la voie du moteur : catalogue critique par force brute, cellules, morceaux (Gordan), descente, Kruskal par plateaux | entiers Python, formules du moteur ($c = a + N / D$) |

L'étage A ne connaît ni boule critique, ni support canonique, ni morceau, ni descente, ni ordre de Morton. L'étage B ne
connaît pas $\Gamma_k$. `hgp11_ref/judge.py` exige que **B égale A** partout où A est calculable, champ par champ.

**Ce que les deux étages partagent** : les types de `model.py` (des enregistrements, aucun calcul) et la convention de
numérotation qu'il énonce. La numérotation canonique, les parents, la recherche d'ancêtre et la lecture d'une coupe
sont écrits trois fois : dans A, dans B et dans le juge. Un troisième attendu, `interval_oracle.py`, n'importe rien
du paquet : sur des points alignés il prédit composantes, points couverts, points de cœur et images verticales par
de simples intervalles, et relit les résultats sur leurs seuls enregistrements (réponse à la nuance d'indépendance
de l'audit courant du 2 octobre 2026).

Les doublons sont permis : deux points de même position sont deux points ($D_k$ compte les multiplicités). L'étage A
travaille sur les copies ; l'étage B sur des sites pondérés, avec le quotient de Gordan général dès qu'une coquille
porte un site de poids supérieur à 1.

## Modèle canonique (un `OrderResult` par ordre $k$)

- `nodes` : arbre de fusion. Naissances d'abord, triées par (niveau, centre de la boule de naissance) ; puis fusions
  N-aires, triées par (niveau, plus petite naissance du sous-arbre). Plateaux atomiques : une fusion a au moins deux
  enfants, tous nés strictement avant elle. Niveaux : rayons carrés, `Fraction`.
- `lower` : application verticale, nœud de l'ordre $k - 1$ vivant à la coupe fermée du niveau du nœud.
- `core` : par point, $D_k(x)$ et le nœud vivant à cette coupe fermée dont la composante contient $x$ (C∩X).
- `cover` : par point, $\alpha_k(x)^{2}$ (plus petit rayon carré d'une $k$-partie contenant $x$) et **l'ensemble** des
  nœuds vivants qui couvrent $x$ à ce niveau. Le choix d'un seul nœud est une règle du produit, pas la vérité.
- `cuts` : à chaque niveau d'événement, coupes ouverte et fermée ; par nœud vivant, masque des points couverts (amas
  discret : points à distance au plus $r$ de la composante) et masque des points de cœur.

Numérotation intrinsèque : ni rang de Morton ni support. Les indices de points sont ceux du nuage d'entrée.

## Ce que chaque étage établit, et jusqu'où

Temps CPU sur un cœur du codespace, 2 octobre 2026 (machine chargée : seuls les temps CPU sont lus).

| Étage | Domaine | Coût mesuré |
| --- | --- | --- |
| A | $n \leq 14$ points pour $K \leq 10$ ; $n \leq 20$ pour $K \leq 3$ ; coordonnées entières quelconques | $n = 12$, $K = 5$ : 0,4 s ; $n = 14$, $K = 5$ : 0,9 s ; $n = 14$, $K = 10$ : 4,0 s |
| B | quelques dizaines de sites ; coordonnées dans $[0, 2^{21})$ (clé de Morton du moteur) ; familles gravées dans $[0, 2^{18})$ | $n = 24$, $K = 5$ : 0,9 s ; $n = 32$, $K = 10$ : 9,6 s ; $n = 48$, $K = 5$ : 16,8 s |
| juge (A, B, cohérence, comparaison) | celui de A | $n = 8$, $K = 5$ : 0,09 s ; $n = 12$, $K = 8$ : 1,1 s ; $n = 13$, $K = 10$ : 2,1 s ; $n = 18$, $K = 4$ : 2,4 s |

**Établi par exécution le 2 octobre 2026** (commandes dans `tests.cmake`) :

- suite rapide, 342 nuages (34 fixtures gravées, 11 familles à graine fixe, 34 nuages à doublons), ordres 1 à 4 ou
  plus : B égale A sur 1 362 ordres, 48 234 coupes, 13 029 nœuds, dont 1 580 fusions d'au moins trois enfants, 142
  niveaux portant plusieurs fusions, 760 boules à coquille étendue, 264 à coquille pondérée, 595 entrées `cover` à
  plusieurs composantes, 145 sauts de descente. Compteurs exacts gravés dans `test_ref.py`, identiques sous toute
  graine de hachage, sous `python3 -O`, en un ou trois processus.
- attendu d'intervalles : 251 multiensembles alignés d'au plus 5 points (220 à doublons), tous les ordres, les deux
  étages : 41 112 coupes ouvertes et fermées, 39 012 composantes, 25 762 images verticales, aucun écart. L'auditeur
  a obtenu les mêmes nombres avec son propre script.
- 22 mutants tués et 3 mutants équivalents acceptés (`ref_mutants.py`) ; parmi les tués, les trois fautes que les
  portes de la v10 publiée laissaient passer (audit L06 : multifusion binarisée, attache à la coupe ouverte, image
  verticale sans remontée) et une jonction manquante qui déplace une fusion sans rompre aucun invariant (audit L01).
- différentiel contre le binaire figé de la v10 : 190 nuages, 190 dumps de catalogue et 640 dumps de tour (`core` et
  `cover`, sérialisés depuis B et, jusqu'à 9 points, depuis A), 26 530 lignes, aucun écart ; 10 mutants de
  sérialisation tués. Grands nuages, étage B seul : 6 nuages de 24 à 32 points, $K$ jusqu'à 10, 6 110 boules,
  12 dumps de tour, 16 128 nœuds, 1 256 sauts de descente, aucun écart.
- recoupes hors porte : l'étage A égale l'oracle indépendant de l'audit L03 sur 150 nuages (6 417 coupes, 6 330
  entrées) ; deux tranches d'un quatre-vingt-seizième de la suite complète (59 nuages chacune, jusqu'à 16 points,
  $K \leq 10$) sont conformes, la seconde avec le code final sous Python 3.10.
- Python 3.10.21 (celui de la VM G4, pris dans le cache de paquets du codespace) : porte rapide, porte différentielle
  et mutants rendent les mêmes codes et les mêmes compteurs que sous Python 3.12 ; le calcul y est 1,4 fois plus lent.

**Non exécuté ici** : la suite complète (5 617 nuages, environ 35 minutes de CPU sous Python 3.10 d'après les
tranches jouées, en 16 portes), réservée à la VM G4. Ses planchers sont des minorants ; ses totaux exacts seront
gravés après le premier passage.

## Ce que la référence n'établit pas

- Rien au-delà de ses tailles : ni coût, ni échelle, ni comportement à 8 000 points. Les petites tailles sont un
  oracle de correction ; à l'échelle, ce sont des invariants globaux et un juge d'échantillon qui jugent.
- L'étage A s'appuie sur l'identification de $\pi_0(L_k(a))$ à $\pi_0(\Gamma_k(a))$ (théorème 2 du manuscrit ;
  argument du nerf de convexes, redémontré dans les audits L01 et L03 de la v10). Il ne la re-vérifie pas.
- Les multiplicités ne sont jugées qu'entre A et B, et au catalogue contre la v10 : la tour de la v10 refuse les
  doublons, aucun binaire ne confirme la tour pondérée. La sémantique « un doublon est un point » est celle de la
  spécification ($D_k$ compte les multiplicités) ; son adoption par le moteur reste à décider.
- Le régime des longues descentes à grand $K$ (des millions de sauts sur une trame LiDAR) n'est qu'effleuré :
  145 sauts sur la suite rapide, jugés par A ; 1 256 sur six nuages de 24 à 32 points, où seul le binaire figé de
  la v10 confirme B. Un saut exige au moins $k$ points strictement intérieurs à une boule minimale.
- Les coquilles de plus de 14 sites (le moteur de la v10 refusait au-delà de 24) sont hors de portée de l'étage A.
- Aucun profil au-delà de 18 bits n'est exercé.

## Sérialisations au format des dumps de la v10 (`hgp11_ref/dumps.py`)

`catalogue_dump(ref)` et `tower_dump(ref, tower, entry)` rendent, octet pour octet, les fichiers `--dump` de
`mhgp10_catalogue` et de `mhgp10_tower` figés au commit `c764e121a` (29 septembre 2026). Le différentiel annonce ses
objets :

| Dans le dump | Statut |
| --- | --- |
| boules admises, $I$, $U$, $q_{\min}$, poids, drapeaux ; arbres, niveaux exacts, images verticales, entrées `core` | **objet** : indépendant de toute convention, égal entre A, B et la v10 |
| ordre des sites (Morton), ordre des boules (niveau, puis $S^{*}$), numérotation des nœuds (naissances dans l'ordre des boules, fusions par niveau puis plus petite naissance) | convention de numérotation |
| écriture non réduite `num den` d'un niveau : celle de la première boule du rang, dans la forme de sa première présentation (`emitted_level` de la v10) | convention d'écriture ; `reduce_tower_levels` la retire (trois mutants montrent qu'elle seule distingue alors les dumps) |
| entrée `cover` : composante de la **première** boule couvrante dans l'ordre des boules | règle de départage de la v10, dépendante du rang de Morton ; la vérité est l'ensemble `Entry.nodes` |
| admission d'une boule à coquille pondérée par $p \leq K - 1$ | règle de la v10 (`admission='v10'`) ; la règle unique $p + q_{\min} \leq K + 1$ (`admission='single'`) donne la même tour et un catalogue plus petit sur un nuage à doublons |

La tour d'un nuage à doublons n'a pas de dump : la v10 la refuse, `tower_dump` lève `ValueError`.

## Familles gravées (`hgp11_ref/families.py`)

Coordonnées entières dans $[0, 2^{18})$. Générateur écrit dans le fichier (SplitMix64) : les nuages ne dépendent pas
de la version de Python.

- **Fixtures** (34) : E5 de la v10 ; les deux triangles de la thèse § 6.1 (ponts de 2000, 1998 et 1700) ; carré,
  octaèdre, cube, tétraèdre et son centre ; points alignés (égalité `cover`, entrée exactement à une fusion,
  non-verticalité de `cover`) ; égalité exacte de première couverture à l'ordre 3 ; extrêmes du domaine ; collisions de niveaux (même niveau écrit par une paire et par un
  triangle ; deux niveaux exacts distincts de même double ; boule $q_{\min} = 3$ écrite dans la forme d'un tétraèdre) ;
  multiplicités (paire (3, 1), triangles aigus (2, 1, 1) et (3, 1, 1), octaèdre et carré pondérés, un site de poids 4).
- **Familles à graine fixe** (11) : `generic`, `generic_u18`, `corner_u18`, `grid3`, `grid4`, `coplanar`,
  `collinear`, `clusters`, `cocircular`, `cospherical`, `duplicates`.

## Portes (`tests.cmake`)

Codes : 0 conforme, 1 désaccord, 2 refus avant calcul, 3 plancher ou invariant violé, 4 mutant tué.

| Porte | Labels | Ce qu'elle établit |
| --- | --- | --- |
| `mhgp11_reference_fast` | oracle, fast | 15 faits gravés (dont l'attendu d'intervalles), puis B égale A sur la suite rapide ; compteurs exacts ; 16 s de CPU |
| `mhgp11_reference_fast_split` | oracle, fast | la même suite en 3 processus : mêmes compteurs (tranches, rapports, somme) |
| `mhgp11_reference_refusal` | oracle, fast | usage faux : code 2 |
| `mhgp11_reference_mutant_<nom>` (25) | oracle, fast | code 4 pour un mutant réel, 0 et `mutant_survives` pour un équivalent déclaré |
| `mhgp11_reference_full_<i>` (16) | oracle, long | tranche $i$ de la suite complète |
| `mhgp11_reference_full` | oracle, long | somme des tranches, faits, planchers ; exige les 16 tranches |
| `mhgp11_reference_diff_v10`, `_large`, `_refusal` et 10 mutants | diff_v10, fast | identité d'octets avec le binaire figé (petits nuages depuis A et B ; 24 à 32 points depuis B) ; absentes sans `MHGP11_V10_FROZEN_DIR` |

## Usage

```python
from hgp11_ref import Definition, Reference, dumps, judge

points = [(0, 0, 7), (0, 9, 6), (1, 4, 0), (0, 0, 1), (4, 1, 2)]
truth = Definition(points).order(2)            # étage A : OrderResult de l'ordre 2
ref = Reference(points, kmax=4)                # étage B : catalogue (ref.balls), puis ref.order(k)
ecarts, _a, _b = judge.compare_cloud(points, 4)  # [] si B égale A aux ordres 1 à 4
ouverte, fermee = judge.cut_at(truth, 9)       # coupes au niveau 9 : (nœud vivant, couverture, cœur)
texte = dumps.tower_dump(ref, entry='core')    # dump de mhgp10_tower, octet pour octet
```

## Provenance

Port explicite de `reference/hgp10_ref.py` et `reference/test_ref.py` du raccord R2 de la v10 (commit `865f5e6`),
des formules de `src/arith/geometry.hpp`, `geometry.cpp` et `src/catalogue/support.hpp`, des règles de
`src/catalogue/generator.cpp` (admission, `emitted_level`), de `src/tower/tower.cpp` (cellules, descente, Kruskal,
verticales, attaches) et des formats de `cli/mhgp10_catalogue.cpp` et `cli/mhgp10_tower.cpp`. Empreintes et
adaptations : `docs/PROVENANCE.md`.
