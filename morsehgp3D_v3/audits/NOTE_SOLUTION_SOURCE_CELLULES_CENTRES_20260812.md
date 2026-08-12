# Note de solution — Source S par listes imbriquées de cellules de centres

Date : 12 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Cette note spécifie le snapshot historique `34371880...` et les invariants
durables de la machine; elle ne décrit pas automatiquement chaque successeur de
`prototype/centre_cell_source.cpp`. Elle consomme
[`NOTE_ARCHITECTURE_GPU_LISTES_CELLULES_CENTRES_20260812.md`](NOTE_ARCHITECTURE_GPU_LISTES_CELLULES_CENTRES_20260812.md)
et les corrections de
[`AUDIT_REPONSES_CELLULES_CENTRES_20260812.md`](AUDIT_REPONSES_CELLULES_CENTRES_20260812.md).
Elle ne revendique **ni** borne de complexité, **ni** GO pour G4, **ni** statut
public. Le verdict live reste
[`AUDIT_ETAT_COURANT.md`](AUDIT_ETAT_COURANT.md).

## 1. Contrat visé et portée reçue

Pour un nuage u16 de sites deux à deux distincts et une fenêtre `smax`, le
contrat de la machine est d'énumérer **tous** les supports minimaux positifs
`U` d'arité `q` dans
`{2,3,4}` dont la boule circonscrite `B` vérifie `p+q<=smax`, avec `p=|I_B|`, et
publie pour chacun le census **global exact** `I_B` et `U_B`, ainsi que la
classification `accepted_closed_rank` contre `extra_shell`.

Le snapshot source `34371880...`, CMake `f663ada0...`, ELF `f927e47b...` passe
`22/22` CTests ciblés en `106,22 s`. Cela reçoit les fixtures et mutants
raccordés, pas encore la complétude générale de l'implémentation : le juge
partage les lifts et `power_of`, et reste borné. Le théorème ci-dessous reçoit
le schéma sous ses invariants; le statut live demeure celui de l'audit courant.
Le source live a changé après ce pin; aucun résultat `22/22` ne lui est
transféré sans reconstruction et nouvelle porte. Son statut appartient
exclusivement à l'audit courant.

Elle ne produit ni `BallActivation`, ni facettes du cœur, ni gateways, ni
resolver, ni MSF, ni fold, ni verticales, ni `BenchmarkOutputContract-v1`. Elle
ne contient aucun noyau CUDA.

## 2. Le lemme budget--cellule, tel qu'implémenté

Pour une cellule `C` de fermeture compacte `K_C`, poser

$$l_{C}(x)=\min_{c\in K_{C}}\left\Vert x-c\right\Vert^{2},\qquad u_{C}(x)=\max_{c\in K_{C}}\left\Vert x-c\right\Vert^{2}.$$

Pour un budget `h>=0`, `R_h(C)` est la `(h+1)`-ième plus petite valeur de `u_C`
et `A_h(C)={x : l_C(x)<=R_h(C)}`. Si une boule positive possédée par `C` a
exactement `p` intérieurs stricts, alors `beta<=R_p(C)` et `I_B union U_B` est
inclus dans `A_p(C)`.

La cellule dyadique reste seule autorité pour la propriété half-open et pour la
subdivision. Une première passe sur sa fermeture globale conserve l'enveloppe
de budget maximal. Seulement ensuite, les bornes et seuils terminaux emploient
la boîte **resserrée** `K_C inter bbox(A(C))`. Pour tout support pertinent déjà
conservé, `U` est inclus dans le pool et `c` appartient à `relint conv(U)` : la
boîte resserrée contient donc encore son centre. Les seuils suivants sont
relatifs au pool hérité; ils conservent le census pertinent, mais ne doivent pas
être appelés les seuils globaux d'un enfant dyadique qui déborde de cette boîte.

La preuve utile pour éviter toute circularité est dichotomique. Poser
`H=smax-2`, le budget maximal commun du snapshot, et `S_B=I_B union U_B`. Pour
toute boule candidate positive dont le support a survécu le long d'un chemin,
le pool courant `P` maintient :

`S_B subseteq P` **ou** `|I_B intersection P|>=H+1`.

La propriété est vraie à la racine `P=X`. Lors d'un filtre
`D_(H,P)(K)`, si `beta<=R_(H,P)`, tous les membres déjà conservés de la boule
restent; sinon les `H+1` témoins de borne supérieure sont strictement
intérieurs et conservés. Le resserrement garde le centre puisque le support
positif `U` survit et `c_B in conv(U) subseteq bbox(P)`. À une fermeture locale
`r_h<=h<=H`, la seconde branche est impossible : elle fournirait encore
`H+1>h` intérieurs dans la liste scannée. Le census local fermé est donc bien
le census global. Cet invariant, et non l'identité avec des listes recalculées
depuis `X`, reçoit le resserrement implémenté.

## 3. Les deux lemmes soumis et leur sort

`L1` — le census restreint est exact dès qu'il est accepté — est reçu sous
l'invariant de pool précédent. La porte
de rejet est `p'+q>smax` avec `q` l'arité du support minimal. La comparaison
rationnelle `beta<=R_p(C)`, dont la largeur worst-case peut dépasser `i128`,
n'est jamais formée. Le rang
fermé `p+|U_B|<=smax` reste une **classification** distincte : la classe
`p+q<=smax<p+|U_B|` est publiée sous le nom `extra_shell`, jamais effacée.

`L2` — la coquille fermée détermine la boule — est reçu comme identité
sémantique **post-census** seulement. La clé chaude implémentée est le centre
rationnel réduit par pgcd, complété par un test de puissance exact pour séparer
deux rayons de même centre. L'audit propose mieux : le 5-uplet homogène
primitif `H=(D, C-2Da, D||a||^2 - C.a)` issu de la forme liftée, disponible
avant tout census et de taille fixe. Cette migration n'est pas faite.

## 4. Exact-once entre budgets

Dans une cellule figée, poser `tau_C(x)=min{h : x in A_h(C)}` et l'entrée
immuable `e0(U)=max_{x in U} tau_C(x)`. Le census emploie un curseur distinct
`h=e0`. Après le scan complet de `A_h`, noter `r_h` le compte intérieur total.
L'invariant est `h<=r_h<=p`. Si `r_h<=h`, le census est global et
`r_h=p=h`. Si `r_h>h`, poser `h=r_h` et scanner seulement les nouveaux buckets.
Si `r_h>smax-q_min` pour un run de boule, aucun support du run n'est pertinent
et **aucun shell partiel n'est publié**. Les contacts nuls rencontrés dans tous
les buckets sont accumulés.

L'exact-once demande une partition terminale commune à tous les budgets d'une
arité. Le code courant possède un seul arbre et énumère une fois chaque tuple
dans `A_(h_max)`, où `h_max=smax-q`; il calcule ensuite `e0=max tau(U)`. Deux
arbres indépendants pourraient émettre le même support à deux cellules de
résolution différente. La fixture à ajouter est
`A=(10,10,10),B=(20,10,10),C=(15,18,10),W=(15,12,10)` : `ABC` entre à `e0=0`
dans la racine, mais à `e0=1` au singleton de son centre.

Le champ `Pending.e` conserve actuellement `e0`; une variable locale est
ensuite promue. Au premier tour, la garde `interior<e_start` vérifie la borne
basse non structurelle; après chaque promotion, `h` prend l'ancien compte et le
nouveau compte ne peut qu'augmenter. La sortie `interior<=h` implique donc bien
`interior==h` par le flot nominal. Une assertion finale explicite serait
redondante mais utile comme défense. Historiquement, `strata-stop` n'était pas
raccordé à CTest sur ce snapshot; le CMake successeur l'enregistre désormais,
sans que cette inscription transfère un résultat d'exécution.

## 5. Lanes d'arité indépendantes — l'invariant de complétude

Les lanes q3 et q4 partent de cliques **géométriques** et ne consultent jamais
le verdict de l'arité inférieure. Les deux contre-fixtures de l'audit sont
gravées et **vérifiées numériquement par le juge exhaustif de ce dépôt** :

| fixture | résultat mesuré |
| --- | --- |
| `arite3`, 33 points | `ABC=1 AB=0 AC=0 BC=0`; le juge accorde `1 391` supports |
| `arite4`, 22 points | `TET=1 faces=0000`; le juge accorde `379` supports |

Le mutant `arity-cascade`, qui conditionne q3 au succès de q2 et q4 au succès
de q3, est **tué** par `arite3` : `judge_mine=558` contre `judge_truth=1391`,
`missing=833`, code 4.

## 6. Filtres exacts employés

Tous sont des refus exacts; aucun n'est une heuristique flottante.

1. **Séparation convexe** par boîte resserrée puis quatre directions
   diagonales. Le seul seuil ne peut pas certifier le vide puisque `A(C)`
   contient toujours au moins `t` sites; la séparation apporte ce refus sans
   prétendre être nécessaire ni rendre l'octree sparse.
2. **Recouvrement d'intervalles** : les membres d'un support de rayon carré
   `beta` vérifient `l_C<=beta<=u_C`, donc leurs intervalles s'intersectent.
3. **Bissecteur exact** : `g(c)=||c-a||^2-||c-b||^2` est affine, ses extrema sur
   un pavé sont aux coins. Strictement plus fort que 2. Le graphe des paires
   admissibles est matérialisé **une fois par cellule** en bitset; les cliques
   sont ensuite des intersections de lignes de bits.
4. **Enveloppe du support** : `c` appartient à `relint conv(U)`, donc la bbox de
   `U` rencontre la cellule. Testé incrémentalement avant tout lift. Ce filtre
   ne se propage pas au sous-arbre : ajouter un point élargit la bbox.
5. **Propriétaire half-open** avant tout census.
6. **Positivité** par barycentriques strictes.

Le commentaire de surclaim « `O(nombre de cliques)` au lieu de `C(m,q)` » est
retiré : le pire cas reste `C(m,4)` et les bitsets suppriment les retests, pas
les cliques.

## 7. Mesure historique du filtre d'enveloppe

Sur `terrain`, `n=2000`, `smax=11`, avec l'option historique
`pair_cap=256` depuis renommée `work_cap`, même sortie
`supports_total=134 300` avant et après :

| version | lifts | temps CPU utilisateur |
| --- | ---: | ---: |
| sans filtre d'enveloppe | `31 246 503` | `6,95 s` |
| avec filtre d'enveloppe | `12 156 467` | `4,85 s` |

`hull_pruned=19 090 036` sur `31 246 503` tests. Les deux sources, ELF,
commandes et sorties brutes ne sont pas archivés; ce relevé est donc historique
et non reproductible depuis le worktree courant. L'égalité du seul compte de
supports ne remplace pas une comparaison d'identités. Les temps proviennent
d'une machine partagée à deux cœurs et ne sont pas un benchmark.

## 8. Portes permanentes

Cinq accords de juge sur les identités `(support, I_B, U_B, rang fermé)` :
`uniform`, `terrain`, `scanline_single_pass`, `scanline_overlap_multiecho` et
la **grille gravée**. Cette dernière existe parce qu'aucun nuage aléatoire ne
produit l'égalité entière `l_C(x)=R_p(C)` : sans elle le mutant `drop-ties`
survit partout, ce qui a été vérifié sur les quatre familles.

Cinq fixtures gravées : `egalite`, `proprietaire`, `arite3`, `arite4`,
`coquille`. La dernière est la contre-fixture de coquille de l'audit : trente
points de rayon cinq autour de `(10,10,10)`, paire antipodale à `p=0, q=2` et
`|U_B|=30`. Elle rend `shell_high_water=30` et classe extra-shell. Le tampon
`[24]` et son `exit(3)` sont supprimés.

Sur le snapshot pincé `34371880...`, sept mutants étaient tués : `drop-ties`,
`owner-closed`, `rank-closed`, `tight-threshold`, `bisector-strict`,
`shrink-list`, `arity-cascade`. Dans le CMake successeur `d0738d1e...`,
`strata-stop` et sa variante uniforme sont désormais rattachés, pour neuf
portes mutantes et vingt-quatre CTests `centre_cell` au total. Leur présence
dans le registre ne transfère pas le résultat historique `22/22`; le source et
le binaire successeurs doivent être pincés et exécutés ensemble.

Refus à code 2 : argument inconnu, suffixe numérique (`--points=60junk`), juge
au-delà de 220 points, doublons exacts par le nuage gravé `grave_doublon` — les
générateurs du dépôt dédupliquent, donc aucune famille n'exerce ce refus.
Plancher de couverture à code 3.

## 9. Ce qui reste ouvert

1. Le juge partage `ball_front.hpp`, les lifts et `power_of` avec le sujet.
   L'accord d'identités est utile, mais ce n'est pas un juge arithmétiquement
   indépendant. `oracle/locality_census_judge.cpp` doit être étendu.
2. Les successeurs postérieurs implémentent un test **droite--cellule** q4 depuis une face
   canonique non colinéaire. Sa sûreté vient du lieu équidistant de cette face;
   elle ne dépend ni de son acuité, ni de sa pertinence dans la lane q3. La
   variante a réduit les quadruplets et lifts sur un petit diagnostic, mais a
   ralenti le CPU; elle reste optionnelle et non reçue sur device.
3. Le snapshot postérieur `fd043fe...` implémentait le sweep
   `sum_i C(a_i,q-1)`. Il compte
   exactement les cliques du graphe d'intervalles scalaires, pas celles du
   graphe 3D de bissecteurs; sa pondération reste un modèle de coût. Le
   snapshot historique `HEAD=02e709b`, source `dbaa2e0...`, implémente les vrais
   `E2/T3/T4/Q4` avec cuts par
   lane et plafond 96, mais les désactive de fait au défaut
   `probe_factor=1`. Il passe `28/28`, mais ces portes n'exercent donc pas la
   sonde. Le successeur `HEAD=3ffff85`, source `d2039ba...`, corrige la garde
   d'incidence et a été observé à `30/30`, dont deux portes de sonde; celles-ci
   n'apportent toutefois ni vérité indépendante `E/T/Q`, ni fixture saturée,
   ni reçu durable. Aucun résultat ne se transfère au pin historique
   `34371880...` ou au worktree d'ablation postérieur. Son statut détaillé
   appartient à l'audit courant.
4. La jauge dyadique commune `s_x(c)` n'est pas implémentée. Elle rendrait les
   bornes affines et réduirait la largeur; la borne actuelle est
   `l,u<=3(65535\cdot 2^{d})^{2}<2^{34+2d}`, sous `i128` jusqu'à `d<=26` mais
   pas sous un entier device 64 bits.
5. La matrice `adj` est un bitset dense quadratique en `top`; aucun préflight
   d'octets ne choisit entre bitset, CSR sparse et `resource_exhausted`.
6. Le régime « mélange équilibré de huit amas » de la section 14.5 du plan de
   tests n'existe pas dans le générateur v3.
7. Aucun noyau CUDA, aucun producteur du payload officiel, aucune session G4.
8. Le groupement avant census trie les centres, puis compare encore
   quadratiquement les rayons concentriques et omet ces comparaisons exactes du
   ledger. La clé homogène primitive doit supprimer ce sous-produit avant toute
   qualification de débit.
9. Les successeurs `005b786...` et `64cf6fe...` ajoutent un ledger fermé par
   arité, un histogramme non causal et un diagnostic de déduplication par lots.
   Le worktree postérieur tente un RLE `SupportKey` local avant lift. Aucun de
   ces états n'est reçu par le `22/22` historique; leur contre-audit est
   [`AUDIT_LEDGER_CAUSES_LIFTS_238CF12_20260812.md`](AUDIT_LEDGER_CAUSES_LIFTS_238CF12_20260812.md).

## 10. Non-claims

La campagne diagnostique de taille `12 500/25 000/50 000` sur `terrain` est
mixte et irrecevable comme rampe mono-binaire; ses blocs 12 500 et 25 000 sont
fermés sur `5b422644...`, tandis que 50 000 a démarré sur `8fdfc8af...` sous le
même en-tête.
Elle n'est pas une rampe contractuelle, car les familles bloquantes `uniform`
et `eight_clusters` n'y sont pas reçues. Aucun compteur ne vaut GO. La baseline
Poisson--Delaunay d'ordre `k`
de l'audit — de l'ordre de `24` millions de supports pour un régime volumique
uniforme à 50 000 points — indique que le régime volumique et le régime
surfacique ne sont pas comparables et qu'aucune conclusion d'un régime ne
s'étend à l'autre.

GCP non utilisé.
