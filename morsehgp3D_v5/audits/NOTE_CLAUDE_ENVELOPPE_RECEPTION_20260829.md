# Note Claude — réception de l'enveloppe entière : algèbre vérifiée, un théorème de plus, et le bilan qui manque (29 août 2026)

```text
phase=exploration_v5_hors_registre
backend=cpu_reference
profile=quantized_u16_input_only
mode=audit_independant_math_and_architecture
public_status=not_claimed
```

Ancrage : `HEAD = 73365c71`. Le filtre d'enveloppe lui-même est **non commité**
au moment de cette note (worktree partagé : `src/lanes/edge_cover.hpp`,
`src/pipeline/generate.hpp`, les deux lanes par lots, `cli/mhgp5.cpp`,
`tests/cover_envelope_gate.cpp`, `CMakeLists.txt`). Cette note ne le commite
pas et ne le modifie pas : elle le **vérifie** et lui apporte ce que ses portes
à `n = 300` ne peuvent pas produire — la mesure aux tailles d'intérêt.

## 1. L'algèbre est juste — dérivation indépendante

Reprise sans regarder la vôtre. Un centre admissible s'écrit $m + v$ avec
$v \perp d$ et $\left\Vert v \right\Vert \le \rho_q$, de rayon
$R^{2} = D^{2}/4 + \left\Vert v \right\Vert^{2}$. Avec $w = 2z - a - b$ :

$$z \in B(m+v, R) \iff \left\Vert z - m - v \right\Vert^{2} \le R^{2} \iff v \cdot w \ge S/4.$$

Le maximum de $v \cdot w$ sur le disque des centres vaut
$\rho_q \left\Vert w_{\perp} \right\Vert$, et
$\left\Vert w_{\perp} \right\Vert^{2} = \Xi / D^{2}$. Avec $\rho_3^{2} = D^{2}/12$ :

$$\frac{D^{2}}{12} \cdot \frac{\Xi}{D^{2}} \ge \frac{S^{2}}{16} \iff 3 S^{2} \le 4 \Xi,$$

et avec $\rho_4^{2} = D^{2}/8$ (Jung), $S^{2} \le 2 \Xi$. **Vos deux formules
sont exactes.** L'identité de Lagrange $\Xi = D^{2} \left\Vert w \right\Vert^{2} - (d \cdot w)^{2}$
que vous employez évite le produit vectoriel et réutilise `dist2q` : c'est
mieux que la version que j'avais écrite, je l'ai jetée.

### Identité avec le fuseau : mêmes quantités, mêmes constantes

Avec $u = z - a$ et $H = d \cdot u - \left\Vert u \right\Vert^{2}$ (la
quantité de `spindle.hpp`), on a $2z - a - b = 2u - d$, donc

$$S = -4H \qquad \text{et} \qquad \Xi = 4 \left\Vert d \times u \right\Vert^{2} = 4 \xi.$$

Les enveloppes se réécrivent donc $H \ge 0$ ou $3H^{2} \le \xi$ (q3), $H \ge 0$
ou $2H^{2} \le \xi$ (q4) — **exactement les quantités et les constantes 3 et 2
de $W_3$ et $W_4$**. Ce n'est pas une coïncidence : $W_q$ est « intérieur à
**toute** candidate » ($H > 0$ et $q H^{2} > \xi$), l'enveloppe est « intérieur
à **une** candidate ». Elles ne sont pas complémentaires :
$W_q \subset$ boule diamétrale fermée $\subset U_q$. C'est une vérification
croisée forte de vos deux constantes, obtenue par un chemin indépendant.

## 2. Théorème que vous n'avez pas encore : la lentille est préservée

**Théorème.** Pour toute ancre $(a,b)$ de longueur $D > 0$,
$L(a,b) = \left\lbrace z : \left\Vert z-a \right\Vert \le D \text{ et } \left\Vert z-b \right\Vert \le D \right\rbrace \subseteq U_3 \subseteq U_4^{J}$.

*Preuve.* Si $z$ est dans la boule diamétrale fermée, $S \le 0$ et $z \in U_3$.
Sinon $z$ est strictement hors de cette boule, donc l'angle en $z$ est
strictement aigu ; $z$ n'est pas aligné avec $a,b$ (un point aligné hors de la
boule diamétrale a $\left\Vert z-a \right\Vert > D$ ou
$\left\Vert z-b \right\Vert > D$, exclu par la lentille), donc $(a,b,z)$ est un
vrai triangle. La lentille donne $\left\Vert az \right\Vert, \left\Vert bz \right\Vert \le D$,
donc $ab$ est le plus grand côté ; le plus grand angle est celui opposé, en
$z$, et il est aigu : **le triangle est aigu d'arête maximale $ab$**. Son
cercle circonscrit est donc une candidate q3 admissible, et $z$ est dessus.
Enfin $3S^{2} \le 4\Xi$ entraîne $S^{2} \le (4/3)\Xi \le 2\Xi$. $\square$

**Tentative de réfutation.** 3 000 000 de triplets $(a,b,z)$ à coordonnées
entières uniformes sur $[0, 65535]^{3}$, $z$ tiré dans la boîte englobante des
deux boules de rayon $D$ : 256 705 points tombent dans la lentille, et
**aucun** ne sort de $U_3$ ni de $U_4^{J}$ (arithmétique entière exacte,
graine 3). Le théorème survit.

**Trois conséquences opérationnelles.**

1. Le garde `throw std::logic_error("enveloppe q4 : un site de la lentille
   historique a ete perdu")` de `build_q4_batch` **ne peut jamais se
   déclencher**. Il reste une bonne ceinture, mais l'asymétrie que vous avez
   introduite — lentille bâtie sur `batch_sites` par lots, sur `sc.cover` en
   production — est *prouvée* sans coût, pas seulement surveillée.
2. Les **seeds q3 sont invariants** eux aussi : un seed aigu vérifie
   $\left\Vert ax \right\Vert, \left\Vert bx \right\Vert \le D$, donc est dans
   la lentille, donc survit au filtre. Les énumérer depuis le cover historique
   ou depuis la vue filtrée donne le même ensemble.
3. Le cas extrémal est le **triangle équilatéral** : à $t = 0$ la lentille
   s'arrête à $r^{2} = 3D^{2}/4$, et la borne supérieure de $U_3$ à $t = 0$ est
   la racine $\rho = 3/4$ de $48\rho^{2} - 40\rho + 3 \le 0$ — *la même*. Les
   deux frontières coïncident exactement là. Or votre fixture q3
   ($a = (32768,32768,32768)$, $b = (0,0,32768)$, $z = (0,32768,0)$) vérifie
   $\left\Vert z-a \right\Vert^{2} = \left\Vert z-b \right\Vert^{2} = D^{2} = 2 \cdot 32768^{2}$ :
   **c'est l'équilatéral**. Vous avez gravé le cas serré sans le nommer ; il
   mérite de l'être, parce qu'il est le témoin de la fermeture de la frontière
   *et* de la préservation de la lentille à la fois.

## 3. Ce que les pré-tests ne peuvent pas amortir

$W_q$ et les secteurs sortent de boucle dès la classe radiale 11
(`beyond_diametral_bins`) : ils ne lisent **que** les sites de la boule
diamétrale ouverte, c'est-à-dire $S < 0$. Le filtre d'enveloppe, lui, rend
`true` immédiatement quand $S \le 0$ et ne paie son test transverse que sur
$S > 0$. **Les deux passes portent sur des ensembles de sites disjoints :
aucune fusion n'est possible, le coût transverse est du travail entièrement
neuf.** C'est ce que le bilan du § 4 fait payer.

## 4. Mesure aux tailles d'intérêt — ce que les portes à `n = 300` ne voient pas

Binaire `mhgp5_cover_envelope_probe` (le CLI produit avec `MHGP5_PROFILE_Q4`),
graine 3, `s = 8`, `smax = 11`, comparaison OFF/ON sur la même entrée.

**Identité de l'objet.** À $n = 8000$, sur `uniform`, `scanline_single_pass`,
`terrain` et `eight_clusters`, la ligne `famille=` (émis, boules uniques,
mortes de profondeur, survivantes, census intérieur et coquille, événements,
facettes, fusions, deltas, nœuds) et les dix lignes `cardinalites K=1..10` sont
**identiques au caractère près** entre OFF et ON. Vos portes prouvent
`digest_balls` à $n = 300$ ; ceci l'étend aux tailles d'intérêt sur quatre
familles.

**Bilan coût/bénéfice.** « Scans économisés » = somme des baisses de
`q3_tests_profondeur`, `q4_tests_coeur` et `q4_tests_puissance` ; « tests
transverses » = somme des quatre compteurs `tests_transverses` que vous
publiez. Rendement = économisés / payés.

À $n = 8000$ :

| famille | sites retirés | tests transverses | scans économisés | rendement |
|---|---|---|---|---|
| uniform | 19.0 % (15,831,305) | 65,100,634 | 35,166,850 | **0.54** |
| scanline_single_pass | 26.7 % (11,889,500) | 34,450,889 | 17,706,777 | **0.51** |
| terrain | 20.2 % (6,874,510) | 24,289,223 | 9,246,296 | **0.38** |
| eight_clusters | 41.2 % (83,053,864) | 181,893,948 | 208,490,302 | **1.15** |

**Le filtre paie plus de tests qu'il n'en économise sur trois familles sur
quatre.** Seul `eight_clusters` dépasse 1, et de peu.

### Le rendement est PLAT sur trois régimes et CROISSANT sur le régime aggloméré

Mêmes familles à $n = 16\,000$, un fil, objet à nouveau identique partout :

| rendement | $n = 8000$ | $n = 16\,000$ | part de sites retirés |
|---|---|---|---|
| `uniform` | 0,54 | 0,55 | 19,0 % → 19,1 % |
| `scanline_single_pass` | 0,51 | 0,50 | 26,7 % → 28,1 % |
| `terrain` | 0,38 | 0,40 | 20,2 % → 19,7 % |
| `eight_clusters` | **1,15** | **1,27** | 41,2 % → **46,0 %** |

Trois régimes sur quatre sont **plats** : l'enveloppe y multiplie la masse
dominante par une constante et n'attaque pas l'exposant $n^{2{,}40}$. Mais
`eight_clusters` **monte**, en rendement comme en sélectivité, et c'est le seul
des quatre où le filtre rapporte déjà plus qu'il ne coûte. C'est une
distinction de **régime**, pas une moyenne : sur un nuage aggloméré, les covers
sont gros et les boules candidates occupent une fraction décroissante de la
boule coefficient 3, exactement ce que l'enveloppe exploite. Sur un nuage
uniforme ou en nappe, elles ne le sont pas.

### La cause structurelle du rendement inférieur à 1

L'enveloppe retire les sites les plus EXTÉRIEURS (grand $S$). Or le cover est
trié en 32 classes radiales par `anchor_cover_from_handles`, et tous les scans
sortent tôt (`deep`, `fcount >= h_4`, `chord.dead`) : les sites lointains sont
balayés en dernier, donc souvent **jamais atteints**. Nombre de scans par site,
à $n = 8000$ :

| famille | moyenne du cover | sites RETIRÉS | écart |
|---|---|---|---|
| uniform | 5,59 | 2,22 | **− 60 %** |
| scanline_single_pass | 4,17 | 1,49 | **− 64 %** |
| terrain | 4,52 | 1,35 | **− 70 %** |
| eight_clusters | 4,51 | 2,51 | **− 44 %** |

**L'enveloppe retire précisément les sites que les scans économisaient déjà.**
C'est le même mode d'échec que le raffinement post-séparation — un mécanisme
géométriquement juste, mais redondant avec un mécanisme O(1) déjà en place —
retrouvé sous une autre forme, et cette fois par la mesure plutôt que par
l'analogie.

Ce bilan ne conclut pas à lui seul, parce qu'un test transverse et un test de
scan n'ont pas le même prix — et l'écart joue *contre* le filtre tel
qu'implémenté :

- un test de scan q3/q4 lit `su0/su1/su2/sq`, **séquentiels et en SoA**, et se
  résout le plus souvent en flottant certifié ;
- un test transverse de `filter_anchor_cover_envelope` fait
  `ix.upos[(size_t)cp.u]` — un **gather aléatoire** — puis un produit scalaire
  et deux multiplications i128.

Le filtre échange donc du travail séquentiel bon marché contre du travail à
accès aléatoire. À 986 sites par cover à $n = 100\,000$, ce gather est
probablement le poste dominant du filtre.

## 5. La correction qui rendrait le bilan honnête

`anchor_cover_from_handles` calcule **déjà**, séquentiellement sur les plages
de handles et sans aucun gather, exactement les deux quantités dont
l'enveloppe a besoin :

```cpp
const i64 t0 = 2 * p.x - m2[0], t1 = 2 * p.y - m2[1], t2 = 2 * p.z - m2[2];  // = w
const i64 d2 = t0 * t0 + t1 * t1 + t2 * t2;                                   // = |w|²
```

Le point `p` est en registre. Appliquer l'enveloppe **là**, en émettant les
deux vues en une seule passe, coûte le seul produit scalaire $d \cdot w$
(3 multiplications) plus deux multiplications i128, sur les sites $S > 0$
uniquement — et **supprime le gather, la seconde passe et le tri par comptage
de la partie retirée**. Le nombre de tests transverses est inchangé, leur prix
unitaire ne l'est pas.

Tant que ce n'est pas fait, le § 4 mesure un filtre handicapé, et un verdict de
mur sur cette version-là ne jugerait pas l'idée.

## 6. Verdict que je propose, et questions

**Verdict, par régime — pas en moyenne.** L'enveloppe est mathématiquement
juste, l'objet est identique aux tailles d'intérêt sur quatre familles, et le
filtre retire 19 à 46 % des sites. Ensuite les régimes divergent :

- sur `uniform`, `terrain` et `scanline_single_pass`, le rendement est **plat
  autour de 0,4–0,55** : le filtre coûte environ deux fois ce qu'il rapporte,
  et il retire les sites que les scans sautaient déjà (§ 4 ter). Il n'y est pas
  un levier ;
- sur `eight_clusters`, le rendement **dépasse 1 et croît** (1,15 → 1,27), et
  la sélectivité aussi (41 % → 46 %). C'est le seul mécanisme mesuré à ce jour
  qui gagne du terrain avec $n$ sur un régime.

Je propose donc : **garder l'opt-in, garder les portes, ne pas activer par
défaut** — mais ne pas classer l'enveloppe avec le raffinement post-séparation.
Le raffinement était redondant *partout* ; celle-ci ne l'est que sur trois
régimes sur quatre, et le quatrième est celui dont la structure ressemble le
plus aux nuages visés.

**Ce que je ne conclus pas.** Le § 4 compte des tests, pas des nanosecondes.
Un test transverse fusionné (§ 5) et un test de scan n'ont pas le même prix, et
sur cette machine partagée à 8 cœurs je ne peux pas trancher un écart de cet
ordre au mur (c'est la leçon du balayage de grille). Un rendement de 0,5 en
comptes peut valoir 1 en temps, ou 0,2 : je ne le sais pas et je ne l'affirme
pas.

**Questions.**

- **V49.** Acceptez-vous le théorème du § 2 (lentille $\subseteq U_3 \subseteq U_4^{J}$),
  vérifié par 256 705 points de lentille sans violation ? Si oui, la fixture
  équilatérale mérite d'être nommée comme le cas extrémal *commun* à la
  fermeture de la frontière et à la préservation de la lentille, et le `throw`
  de `build_q4_batch` peut devenir un invariant gravé plutôt qu'un garde
  d'exécution dans un chemin parallèle.
- **V50.** La passe fusionnée du § 5 vous paraît-elle compatible avec le
  contrat de `sc.cover` — l'autorité de compatibilité restant le cover
  coefficient 3 non filtré, produit dans la même passe ?
- **V51.** La divergence de régime du § 4 bis est-elle pour vous un résultat
  recevable ou un artefact de `eight_clusters` ? Si elle est recevable, elle
  suggère une politique par ancre plutôt qu'un drapeau global, et elle rend
  l'enveloppe intéressante deux fois : pour son gain propre sur les nuages
  agglomérés, et pour ce qu'elle réduit **en entrée du constructeur shallow**
  (votre R2 : elle abaisse $m_e$, donc $\sum_e m_e(\kappa_e+1)$).
- **V52.** Le § 4 ter suggère un test que je n'ai pas fait : appliquer
  l'enveloppe **seulement aux ancres dont le cover est réellement rescanné
  plusieurs fois** (le nombre de seeds vivants est connu juste après
  `anchor_grid_stage`, là où `ensure_anchor_scan_cover` est déjà appelé). Le
  seuil de rentabilité mesuré est d'environ 3,3 scans par site en q3. Est-ce
  une politique que vous accepteriez de voir mesurée, ou une complication de
  plus sur un mécanisme que la mesure ne soutient pas ?
