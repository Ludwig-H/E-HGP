# Note de solution — Source S par listes imbriquées de cellules de centres

Date : 12 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Cette note spécifie la machine implémentée dans
`prototype/centre_cell_source.cpp` et ses portes. Elle consomme
[`NOTE_ARCHITECTURE_GPU_LISTES_CELLULES_CENTRES_20260812.md`](NOTE_ARCHITECTURE_GPU_LISTES_CELLULES_CENTRES_20260812.md)
et les corrections de
[`AUDIT_REPONSES_CELLULES_CENTRES_20260812.md`](AUDIT_REPONSES_CELLULES_CENTRES_20260812.md).
Elle ne revendique **ni** borne de complexité, **ni** GO pour G4, **ni** statut
public. Le verdict live reste
[`AUDIT_ETAT_COURANT.md`](AUDIT_ETAT_COURANT.md).

## 1. Ce que la machine produit

Pour un nuage u16 de sites deux à deux distincts et une fenêtre `smax`, elle
énumère **tous** les supports minimaux positifs `U` d'arité `q` dans
`{2,3,4}` dont la boule circonscrite `B` vérifie `p+q<=smax`, avec `p=|I_B|`, et
publie pour chacun le census **global exact** `I_B` et `U_B`, ainsi que la
classification `accepted_closed_rank` contre `extra_shell`.

Elle ne produit ni `BallActivation`, ni facettes du cœur, ni gateways, ni
resolver, ni MSF, ni fold, ni verticales, ni `BenchmarkOutputContract-v1`. Elle
ne contient aucun noyau CUDA.

## 2. Le lemme profondeur--cellule, tel qu'implémenté

Pour une cellule `C` de fermeture compacte `K_C`, poser

$$l_{C}(x)=\min_{c\in K_{C}}\left\Vert x-c\right\Vert^{2},\qquad u_{C}(x)=\max_{c\in K_{C}}\left\Vert x-c\right\Vert^{2}.$$

Pour `p>=0`, `R_p(C)` est la `(p+1)`-ième plus petite valeur de `u_C` et
`A_p(C)={x : l_C(x)<=R_p(C)}`. Si une boule positive possédée par `C` a
exactement `p` intérieurs stricts, alors `beta<=R_p(C)` et `I_B union U_B` est
inclus dans `A_p(C)`.

La cellule dyadique reste seule autorité pour la propriété half-open et pour la
subdivision. Les bornes, les seuils et tous les filtres emploient la boîte
**resserrée** `K_C inter bbox(A(C))` : c'est un convexe plus petit qui contient
encore tous les centres possédés, puisque `c` appartient à `relint conv(U)`.

## 3. Les deux lemmes soumis et leur sort

`L1` — le census restreint est exact dès qu'il est accepté — est reçu. La porte
de rejet est `p'+q>smax` avec `q` l'arité du support minimal. La comparaison
rationnelle `beta<=R_p(C)`, qui déborde `i128`, n'est jamais formée. Le rang
fermé `p+|U_B|<=smax` reste une **classification** distincte : la classe
`p+q<=smax<p+|U_B|` est publiée sous le nom `extra_shell`, jamais effacée.

`L2` — la coquille fermée détermine la boule — est reçu comme identité
sémantique **post-census** seulement. La clé chaude implémentée est le centre
rationnel réduit par pgcd, complété par un test de puissance exact pour séparer
deux rayons de même centre. L'audit propose mieux : le 5-uplet homogène
primitif `H=(D, C-2Da, D||a||^2 - C.a)` issu de la forme liftée, disponible
avant tout census et de taille fixe. Cette migration n'est pas faite.

## 4. Exact-once entre profondeurs

`tau_C(x)=min{p : x in A_p(C)}` et `e(U)=max_{x in U} tau_C(x)`. Le census part
du bucket `A_e`. Si le compte intérieur partiel `r` vérifie `r<=e`, le census
est global; et comme un membre au moins n'est pas dans `A_{e-1}`, on a
`beta>R_{e-1}`, donc `r>=e`, donc **`r=e` exactement**. C'est un invariant
vérifié à l'exécution : sa violation rend le code 3. Si `r>e`, l'indice est
promu à `r` et seuls les nouveaux buckets sont scannés. Si `r>smax-q`, le
support est `above_support_window` et **aucun shell partiel n'est publié**.

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
   diagonales. Sans elle l'octree ne coupe jamais : `A(C)` contient toujours au
   moins `t` sites, même au milieu du vide.
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

## 7. Mesure du filtre d'enveloppe

Sur `terrain`, `n=2000`, `smax=11`, `pair_cap=256`, même sortie
`supports_total=134 300` avant et après :

| version | lifts | temps CPU utilisateur |
| --- | ---: | ---: |
| sans filtre d'enveloppe | `31 246 503` | `6,95 s` |
| avec filtre d'enveloppe | `12 156 467` | `4,85 s` |

`hull_pruned=19 090 036` sur `31 246 503` tests. Les temps proviennent d'une
machine partagée à deux cœurs et ne sont pas un benchmark.

## 8. Portes permanentes

Cinq accords de juge sur les identités `(support, I_B, U_B, rang fermé)` :
`uniform`, `terrain`, `scanline_single_pass`, `scanline_overlap_multiecho` et
la **grille gravée**. Cette dernière existe parce qu'aucun nuage aléatoire ne
produit l'égalité entière `l_C(x)=R_p(C)` : sans elle le mutant `drop-ties`
survit partout, ce qui a été vérifié sur les quatre familles.

Quatre fixtures gravées : `egalite`, `proprietaire`, `arite3`, `arite4`,
`coquille`. La dernière est la contre-fixture de coquille de l'audit : trente
points de rayon cinq autour de `(10,10,10)`, paire antipodale à `p=0, q=2` et
`|U_B|=30`. Elle rend `shell_high_water=30` et classe extra-shell. Le tampon
`[24]` et son `exit(3)` sont supprimés.

Sept mutants tués : `drop-ties`, `owner-closed`, `rank-closed`,
`tight-threshold`, `bisector-strict`, `shrink-list`, `arity-cascade`. Le mutant
`strata-stop`, qui arrête le census au premier bucket sans promotion, est
enregistré mais n'est pas encore rattaché à une porte.

Refus à code 2 : argument inconnu, suffixe numérique (`--points=60junk`), juge
au-delà de 220 points, doublons exacts par le nuage gravé `grave_doublon` — les
générateurs du dépôt dédupliquent, donc aucune famille n'exerce ce refus.
Plancher de couverture à code 3.

## 9. Ce qui reste ouvert

1. Le juge partage `ball_front.hpp`, les lifts et `power_of` avec le sujet.
   L'accord d'identités est utile, mais ce n'est pas un juge arithmétiquement
   indépendant. `oracle/locality_census_judge.cpp` doit être étendu.
2. Le test **droite--cellule** pour q4, légitimé par le fait que tout tétraèdre
   propre positif possède au moins deux faces aiguës, n'est pas implémenté.
   C'est le filtre net attendu pour la lane la plus coûteuse.
3. Le critère de split exact `sum_i C(a_i,q-1)` par sweep n'est pas implémenté :
   le critère courant compte seulement les paires d'intervalles compatibles.
4. La jauge dyadique commune `s_x(c)` n'est pas implémentée. Elle rendrait les
   bornes affines et réduirait la largeur; la borne actuelle est
   `l,u<=3(65535\cdot 2^{d})^{2}<2^{34+2d}`, sous `i128` jusqu'à `d<=26` mais
   pas sous un entier device 64 bits.
5. La matrice `adj` est un bitset dense quadratique en `top`; aucun préflight
   d'octets ne choisit entre bitset, CSR sparse et `resource_exhausted`.
6. Le régime « mélange équilibré de huit amas » de la section 14.5 du plan de
   tests n'existe pas dans le générateur v3.
7. Aucun noyau CUDA, aucun producteur du payload officiel, aucune session G4.

## 10. Non-claims

La rampe contractuelle `12 500/25 000/50 000` est en cours et n'est pas encore
publiée. Aucun compteur ne vaut GO. La baseline Poisson--Delaunay d'ordre `k`
de l'audit — de l'ordre de `24` millions de supports pour un régime volumique
uniforme à 50 000 points — indique que le régime volumique et le régime
surfacique ne sont pas comparables et qu'aucune conclusion d'un régime ne
s'étend à l'autre.

GCP non utilisé.
