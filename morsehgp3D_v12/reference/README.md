# Référence exacte bornée de la v12

**Port de la v11.** Ce dossier est le port explicite de `reference/` de la v11 gelée (`ac081a06f`), à l'identique
hormis les noms : paquet `hgp12_ref`, portes `mhgp12_reference_*`, variable `MHGP12_V10_FROZEN_DIR`. Les faits,
compteurs et empreintes décrits ci-dessous sont ceux qu'a établis la v11 ; les portes de la v12 les rejouent
(aucune qualification n'est héritée). Le format `hgp11_supports_oracle` des sorties canoniques de l'oracle des
supports est gardé tel quel : il entre dans les empreintes gravées de `test_supports.py`. Les portes natives citées
plus bas (`tests/supports/`, `tests/tower/`) sont celles de la v11 : la v12 n'a pas encore ces modules.

Python 3.10 nu (`fractions`, `itertools`), aucune dépendance, aucun flottant dans une décision. Ce dossier établit la
**vérité sur de petits nuages** ; il jugera le catalogue et la tour du moteur. Il ne dépend d'aucun module C++.

```text
phase=exploration_v12_hors_registre   mode=fondations   public_status=not_claimed
```

## Principe : la vérité est la définition, pas un algorithme

Deux étages sans idée commune, qui remplissent le même enregistrement canonique (`hgp12_ref/model.py`) :

| Étage | Fichier | Ce qu'il est | Arithmétique |
| --- | --- | --- | --- |
| **A, définition** | `hgp12_ref/definition.py` | le graphe $\Gamma_k$ de la thèse, exhaustif : sommets = $k$-parties au niveau du rayon carré de leur boule minimale, arêtes = $(k+1)$-parties au niveau de la leur | `Fraction`, élimination de Gauss ; boule minimale par énumération des supports (pas Welzl) |
| **B, constructive** | `hgp12_ref/constructive.py`, `hgp12_ref/intgeom.py` | la voie du moteur : catalogue critique par force brute, cellules, morceaux (Gordan), descente, Kruskal par plateaux | entiers Python, formules du moteur ($c = a + N / D$) |

L'étage A ne connaît ni boule critique, ni support canonique, ni morceau, ni descente, ni ordre de Morton. L'étage B ne
connaît pas $\Gamma_k$. `hgp12_ref/judge.py` exige que **B égale A** partout où A est calculable, champ par champ.

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

## Sérialisations au format des dumps de la v10 (`hgp12_ref/dumps.py`)

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

## Oracle borné des supports d'ordre K (`hgp12_ref/supports.py`, tranche S1)

```text
phase=exploration_v12_hors_registre   backend=cpu_reference   quantification=quantized_u21_input_only   public_status=not_claimed
```

Vérité bornée de la sortie `supports` décidée le 4 octobre 2026 (`build/v11-persist/sortie_supports/`,
`DECISIONS_UTILISATEUR.md` puis `SPECIFICATION_FINALE.md`, paragraphes 2, 8.2 et 9.1). Le module ne s'appuie que sur
l'étage A (`definition.py` et les enregistrements de `model.py`) ; la porte le charge **sans le paquet** (ni
`__init__`, ni `constructive`, ni `judge`), comme `tests/tower/forest_oracle.py` de la v11. Positions distinctes
seulement : le moteur refuse les entrées pondérées.

| Objet | Calcul | Contrôle |
| --- | --- | --- |
| $W_K$ | boules minimales des $K$- et $(K+1)$-parties, dédoublonnées par (centre, niveau) exacts, $p+q\leq K+1$ | lemme W : toute liaison de Gabriel (Déf. 28) a sa boule dans $W_K$, leur nombre égale la somme des `gabriel_cofaces` ; une liaison non Gabriel n'est jamais séparante (Th. 4 sans position générale) ; témoins E5 et D2 du point 2 (`window_tree`, deux lectures) |
| $\mathrm{att}(b)$ | `Definition.node_at` à la coupe fermée $\lambda_b$, sur **toutes** les $K$-parties de $P_b$ | lemme A : un seul nœud (T3) |
| $\mathrm{ant}(b)$, rôle | nœuds de la coupe ouverte des $K$-parties strictes ; naissance, fusion ou interne par les niveaux | lemme B (rangs, naissance de même centre), lemme C (traces comprimées, enfants, règle du parent, union des branches d'une fusion égale à ses enfants) |
| $\mathcal{Q}_b$ | parties de $U_b$ de 2 à 4 sites, affinement indépendantes, poids barycentriques de $c_b$ strictement positifs (Gram, `Fraction`) | lemme F par force brute : parties non séparables **minimales** de $U_b$, séparabilité lue sur $\beta(A)<\lambda_b$, toutes arités ; chaque support redonne sa boule (M1) |
| comptes | `kparties_reliees`, `compressed_parts`, `strict_traces`, `cofaces` par boule et par support, `components`, comptes de Gabriel, par énumération brute des parties de $P_b$ | lemme G : formules de la spec, M2 sur chaque $(K+1)$-partie, inégalités et cas réguliers |
| polyèdres | sites des $K$-parties de chaque composante (Déf. 21) à chaque coupe d'événement | lemme H : égaux à l'union des $P_b$ des boules rattachées au sous-arbre, de niveau au plus la coupe, et aux seules boules fortes à $K\geq 2$ ; à $K=1$, les feuilles |
| instantanés | boules du sous-arbre en ordre canonique | tranche contiguë ; instantané daté = préfixe ; supports dans le $K$-polyèdre |

`Supports(points).canonical(k, ids)` rend la sortie canonique (JSON trié, sites désignés par leurs coordonnées,
schéma dans l'en-tête du module) que les différentiels natifs S3 et S6 reliront. Elle ne dépend pas de l'ordre des
points ; un réétiquetage ne change que la colonne `ids`.

**Établi par exécution le 4 octobre 2026** (`test_supports.py`, Python 3.12.1 et 3.10.21, normal et `-O`, sorties
identiques, et sous deux graines de hachage) : 50 faits. Ce sont 24 faits gravés sur les fixtures de la spec
(paragraphe 2.9) et des audits `1bf4be68f`, `de4ab58a8` et `aef7182b3`, tous conformes aux attendus de la spec,
22 empreintes de fixtures (les 28 autres fixtures ont aussi leur empreinte gravée), `sphere50` ($m=84$), le témoin
de Hausdorff des cercles $n\in\lbrace 3,4,5,1023\rbrace$, le témoin E5 du lemme W, point 2, dans ses deux lectures,
et le témoin D2 de l'auditeur. D2 est une trace stricte née au niveau 64, après le niveau 41 de rang $r_b-1$, de la
boule faible de niveau $1681/25$ ; c'est aussi un second témoin du lemme W, point 2. La garde « q4 du cube à
$K=1$ » est un fait nommé. Puis 210 nuages (50 fixtures, 160 nuages de 5 à 10 points des dix familles à positions
distinctes, graine 31), 951 ordres, 15 062 boules, 16 943 supports, 12 441 nœuds, 48 074 couples (nœud, coupe)
jugés par le lemme H, 17 058 liaisons de Gabriel et 40 053 liaisons non Gabriel jugées, 2 551 coquilles étendues,
530 boules à plusieurs supports, 2 736 boules internes, 271 cellules passagères, 1 890 fusions d'au moins trois
enfants, 226 coquilles étendues portant un support d'arité supérieure à $q_{\min}$ : aucun écart. 70 nuages sont
rejoués permutés (permutation effective, jamais l'identité, compteur gravé), réétiquetés et translatés de
$(5,11,17)$ : même sortie, à la colonne `ids` et à la translation près. Environ 20 à 30 s de CPU selon la version de
Python. Une exception levée dans un fait ou dans cette contre-épreuve est comptée comme un écart, jamais comme une
trace Python.

Treize mutants de l'oracle sont tués, chacun par sa cause. Sept visent l'objet (coupes, fenêtre, supports, cofaces,
populations). Six prouvent la **vivacité** d'un contrôle : W.4, restriction du lemme H aux fortes, union des
branches d'une fusion (C.3), règle du parent, vie d'une boule interne, M1. Si l'un de ces contrôles devenait
tautologique ou disparaissait, son mutant survivrait, ou ne s'appliquerait plus, et sa porte échouerait.

La somme `kparties` de la ligne de compteurs (54 659) compte des incidences $(b,F)$, pas des $K$-parties distinctes.

**Ce que cet oracle n'établit pas** : aucune propriété du moteur natif ; ni coût ni échelle ; pas la stabilité du
carrier (le cercle à quatre points la réfute, fixture gravée) ; pas le refus natif `support_shell_capacity` (seul
$m=84$ est constaté sur `sphere50`) ; pas l'identification de $\pi_{0}(L_K)$ à $\pi_{0}(\Gamma_K)$, invoquée comme
pour l'étage A. Le support canonique $S^{*}$ (premier en ordre des `SiteIdx`) est une convention du format natif :
l'oracle ne connaît pas l'ordre de Morton, et les différentiels natifs retrient par (postordre, niveau, centre). La
suite s'arrête à $K\leq 5$ et à des coquilles de 12 sites au plus : $K\geq 6$ et les coquilles de 13 à 24 sites,
jusqu'au plafond natif, n'ont pas de porte bornée ; la fermeture $N_j$ de l'oracle parcourt $2^{m}$ masques.

**Ajouts du 5 octobre 2026** (intégration L1, apports des auditeurs `9cbf805c6` et `238734f1d`) :
- **Budget de la force brute du lemme F.** `_minimal_nonseparable` examine au plus $2^{m}-1$ parties candidates :
  chacune est une partie non vide de $U_b$, formée une seule fois. Au-delà du budget `MINIMAL_BUDGET`
  ($2^{16}-1$, donc $m\leq 16$), elle lève `BudgetRefusal` **avant tout calcul** : refus explicite, jamais un
  résultat partiel. Le fait gravé `minimal_budget` fixe la borne à l'égalité, sur la diagonale du carré ($m=4$ :
  refus au budget 14, parties minimales égales à $\mathcal{Q}_b$ au budget 15), et le refus sur `sphere50` ($m=84$) :
  la porte rapide compte désormais 51 faits.
- **Primitives sur sphere5** (`test_supports.py --suite=primitives`, porte `mhgp12_reference_supports_primitives`).
  Les 24 points entiers de $x^2+y^2+z^2=5$, translatés de $(2,2,2)$, forment une coquille mixte au plafond natif.
  $\mathcal{Q}_b$ est calculé par Gram (12 diamètres, 24 triangles, 792 tétraèdres), puis $N_j$ pour $j\leq 4$ par
  combinaisons (`closure_upto` : 12, 288 et 3 906), sans les $2^{24}$ masques. Les comptes de $K=1$ à $K=3$ viennent
  des formules du lemme G, et les incidences par support valent 4 068 à $K=3$. La force brute du lemme F refuse
  explicitement. Environ deux secondes : label `fast`. Cette porte ne qualifie pas tout S1 à 24 sites ; la porte
  native correspondante était, dans la v11, `mhgp11_supports_unit_sphere5`.
- **Différentiels natifs contre cet oracle (v11).** `mhgp11_supports_fraction` (tranche S6a, `tests/supports/`) et
  `mhgp11_tower_attach_fraction` (tranche S3, `tests/tower/attach_fraction.py`). Le second confronte l'arbre d'ordre K
  seul et le rattachement natifs (sonde `mhgp11_tower_attach_probe`, domaine préparé à l'ordre K) à
  `canonical(k, ids)`, projeté sur les champs de S3, sur les nuages de la suite et à ses ordres. Il y ajoute le petit
  témoin à $K$ élevé de l'auditeur (`morsehgp3D_v11/receipts/audit_native_integration_20261005/qb/normal.json` :
  quatre coins d'un carré de côté 20 et huit sites intérieurs) à $K=1..12$ ; l'oracle le traite en moins d'une
  seconde, lemmes A à H contrôlés. C'est le seul cas borné de l'arbre et du rattachement à $K\geq 6$, mais il ne lève pas la limite
  ci-dessus pour les coquilles de 13 à 24 sites.

## Familles gravées (`hgp12_ref/families.py`)

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
| `mhgp12_reference_fast` | oracle, fast | 15 faits gravés (dont l'attendu d'intervalles), puis B égale A sur la suite rapide ; compteurs exacts ; 16 s de CPU |
| `mhgp12_reference_fast_split` | oracle, fast | la même suite en 3 processus : mêmes compteurs (tranches, rapports, somme) |
| `mhgp12_reference_refusal` | oracle, fast | usage faux : code 2 |
| `mhgp12_reference_resolution_v12` | oracle, fast | règle de résolution de la v12 ([`CONTRAT_TOUR.md`](../docs/CONTRAT_TOUR.md) § 4.1, ajoutée le 7 octobre 2026, `Reference(..., resolution='v12_*')`) : chaque représentant s'arrête sur la première cellule de fenêtre (`LEM-T3`), une cible « cellule » se lit sur la cellule déjà traitée, cellules inertes comprises (pont de `LEM-T4`) ; pour trois politiques de saut (plus proches du centre, plus petits identifiants de $I$ comme la v11, candidats voisins du levier `G-L3`), résultat canonique de chaque ordre identique à celui de l'étage B sur la suite rapide ; compteurs exacts et planchers (168 cibles sur des cellules inertes) ; fait gravé du repli de `G-L3` sur le census ; ligne `resolution_v12_ok nuages=342 ordres=1362 politiques=3 cibles=18392 cellules=8448 inertes=1113 faits=1` ; six secondes |
| `mhgp12_reference_resolution_v12_mutant_<nom>` (4) | oracle, fast, mutant | `inertes_omises`, `element_sans_racine`, `arret_sous_fenetre`, `plateau_coupe` : code 4 |
| `mhgp12_reference_resolution_v12_refusal` | oracle, fast | usage faux : code 2 |
| `mhgp12_reference_mutant_<nom>` (25) | oracle, fast | code 4 pour un mutant réel, 0 et `mutant_survives` pour un équivalent déclaré |
| `mhgp12_reference_full_<i>` (16) | oracle, long | tranche $i$ de la suite complète |
| `mhgp12_reference_full` | oracle, long | somme des tranches, faits, planchers ; exige les 16 tranches |
| `mhgp12_reference_diff_v10`, `_large`, `_refusal` et 10 mutants | diff_v10, fast | identité d'octets avec le binaire figé (petits nuages depuis A et B ; 24 à 32 points depuis B) ; absentes sans `MHGP12_V10_FROZEN_DIR` |
| `mhgp12_reference_supports` | oracle, fast | oracle des supports (tranche S1) : 51 faits (50 le 4 octobre, plus le budget du lemme F), puis 210 nuages, lemmes A à H et W, invariance (permutation effective, réétiquetage, translation), compteurs exacts et planchers de la spec ; ligne `reference_supports_ok nuages=210 ordres=951 boules=15062 supports=16943 noeuds=12441 coupes=48074` ; 20 à 30 s de CPU |
| `mhgp12_reference_supports_refusal` | oracle, fast | usage faux : code 2 |
| `mhgp12_reference_supports_primitives` | oracle, fast | primitives sur sphere5 (24 sites) : $\mathcal{Q}_b$ par Gram, $N_j$ pour $j\leq 4$ par combinaisons, comptes de $K=1$ à $K=3$, refus explicite de la force brute du lemme F ; ligne `reference_supports_primitives_ok sites=24 supports=828 q2=12 q3=24 q4=792 N2=12 N3=288 N4=3906 refus=2` ; deux secondes |
| `mhgp12_reference_supports_mutant_<nom>` (13) | oracle, fast | `att_coupe_ouverte`, `ant_coupe_fermee`, `fenetre_forte`, `premier_support_seul`, `triangle_droit_admis`, `cofaces_ordre_k`, `populations_naissances_seules` (lemme A, C, périmètre, F, F, G, H), puis les six mutants de vivacité `w4_gabriel_juge`, `h_fortes_etroites`, `c3_une_fusion`, `regle_parent_inversee`, `interne_vie_inversee`, `m1_support_inverse` (W.4, H, C.3, règle du parent, vie d'une interne, M1) : code 4, tué par sa cause |

## Usage

```python
from hgp12_ref import Definition, Reference, dumps, judge

points = [(0, 0, 7), (0, 9, 6), (1, 4, 0), (0, 0, 1), (4, 1, 2)]
truth = Definition(points).order(2)            # étage A : OrderResult de l'ordre 2
ref = Reference(points, kmax=4)                # étage B : catalogue (ref.balls), puis ref.order(k)
ecarts, _a, _b = judge.compare_cloud(points, 4)  # [] si B égale A aux ordres 1 à 4
ouverte, fermee = judge.cut_at(truth, 9)       # coupes au niveau 9 : (nœud vivant, couverture, cœur)
texte = dumps.tower_dump(ref, entry='core')    # dump de mhgp10_tower, octet pour octet

from hgp12_ref.supports import Supports
carre = Supports([(0, 0, 0), (2, 0, 0), (2, 2, 0), (0, 2, 0)])
boules = carre.order(2).balls                  # W_2 : quatre naissances, puis la diagonale de rôle fusion
sortie = carre.canonical(2)                    # sortie canonique (dict JSON) de l'ordre 2
```

`python3 test_supports.py --dump=<fixture>` écrit la sortie canonique d'une fixture à tous ses ordres.

## Provenance

Port explicite de `reference/hgp10_ref.py` et `reference/test_ref.py` du raccord R2 de la v10 (commit `865f5e6`),
des formules de `src/arith/geometry.hpp`, `geometry.cpp` et `src/catalogue/support.hpp`, des règles de
`src/catalogue/generator.cpp` (admission, `emitted_level`), de `src/tower/tower.cpp` (cellules, descente, Kruskal,
verticales, attaches) et des formats de `cli/mhgp10_catalogue.cpp` et `cli/mhgp10_tower.cpp`. Empreintes et
adaptations : `docs/PROVENANCE.md` de la v11 ; port de la v11 à la v12 : `docs/PROVENANCE.md` de la v12.
