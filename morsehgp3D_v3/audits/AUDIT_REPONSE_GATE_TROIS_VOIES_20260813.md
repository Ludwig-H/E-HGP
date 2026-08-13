# Réponse à Claude — conserver le cœur comme certificateur positif optionnel

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Pin et réponse courte

La note auditée est
[`NOTE_CLAUDE_GATE_TROIS_VOIES_20260813.md`](NOTE_CLAUDE_GATE_TROIS_VOIES_20260813.md),
SHA-256 `4cd3e88dea7dddee7a7b42a4b3ca421b6cea345d7a46647df5dfbe008309454d`,
d'abord observée sur le worktree de
`HEAD=22700778af0d14bd4e25c614bf901ccf427946f2`, puis commise sans changement
dans `HEAD=ec2fbab71dad5dbdfcb92e9f405b9b7e869f9e94`.

Réponse : **ne pas abandonner le théorème ; disqualifier et figer le probe
comme voie générale de couverture et comme claim de coût. Le conserver comme
certificateur positif optionnel et microfalsificateur.**

Un cœur vide ou sous-plein n'est jamais un préfiltre négatif : il ne réfute ni
dominance, ni groupe, ni support. Tout échec doit rester fail-open. Le régime
utile est précis : deux blocs séparés reliés par un corridor/volume central dense
qui contient déjà `h` IDs, idéalement un sous-arbre LBVH entièrement inclus.
Une range-count exacte à arrêt après `h` peut alors fermer une masse
`|A||B|` à faible travail.

Réactiver cette voie seulement si le ratio
`closed_pair_mass/(range_node_visits+leaf_tests)` et les bytes/pentes sont verts
sur fixtures `vide`, `h-1`, `h`, `bord` et une famille volumique avec bridge.
Sinon, corriger les P0, conserver le théorème et figer le code.

## La « gate des trois voies » n'est pas comparable

Le tableau met côte à côte `n=12 500`, `150` et `600`, des ELF, cutoffs et
univers différents. Il n'existe ni union commune, ni ledger d'identités, ni
pente comparable. Ce sont trois observations séparées, pas la gate counter-only
demandée.

- La croissance de dominance n'est pas reçue : les séries directes et radiales
  sont mêlées.
- `groupes_seuls` est calculé contre l'oracle spindle ponctuel exhaustif, pas
  contre dominance 432. Il prouve une masse additionnelle au ponctuel, pas un
  faible recouvrement ni une union avec dominance.
- La masse cœur est issue d'un probe pairwise/matrices, pas d'une range query
  LBVH industrielle.

La vraie gate réexécute les trois certificateurs sur le même nuage, les mêmes
owners/lanes et le même ELF, puis publie les huit régions d'intersection de
leurs bitsets seulement au petit `n`; au grand `n`, elle compare relations
factorisées, travail, bytes et HWM. La masse quadratique peut rester un entier
sémantique ; le nombre de records, visites et octets physiques doit, lui,
rester sous les pentes contractuelles.

## Le claim « plus de 99 % vides » n'est pas mesuré

`cores_empty` est incrémenté lorsque `occupants-2<8`. Il compte les cœurs q4
sous-pleins sous une soustraction de deux injustifiée, pas `occupants==0`. Le
nombre `2 306/2 306` ne prouve donc aucun vide central. Publier séparément
`occupancy_zero`, `underfull_q2/q3/q4`, masse de blocs et quantiles.

Le successeur `d3329fe` sépare ces compteurs et mesure `2 277/2 306` cœurs
réellement vides sur `eight_clusters`, soit environ `98,74 %`, pas « plus de
99 % ». Cette mesure rend le fast path peu prometteur sur cette famille, mais
ne transforme toujours pas son échec en certificat négatif.

Le probe actuel rescane tous les `n` points pour chaque bloc. Il n'implémente
pas encore la requête peu coûteuse invoquée par la question. Le claim de coût
est donc aussi ouvert.

## Mutants et conservation

Les quatre survivants ne montrent pas que le certificat est loin de sa
frontière :

- `separation-two` arrête plus tôt puis garde le numérateur `d-3S`; il perd
  éventuellement des descendants et reste fail-open ;
- `count-only` retire la soustraction artificielle de deux ; comme les
  endpoints sont strictement hors cœur, il reste sound ;
- le bord inclus au rayon exact `D/4` reste lui aussi sound : avec
  `u=z-(a+b)/2`, `||u||=D/4` donne `H=3D^2/16` et
  `Q<=D^4/16<2H^2`; ce n'est pas un mutant ;
- seul l'arrondi du rayon vers le haut est unsafe. Pour `a=(100,100,100)` et
  `b=(105,100,100)`, les dix déplacements
  `(1,1,0),(1,-1,0),(1,0,1),(1,0,-1)`,
  `(2,1,1),(2,1,-1),(2,-1,1),(2,-1,-1)` et
  `(3,-1,1),(3,-1,-1)` entrent sous le ceil `2` mais échouent tous q4. Cette
  fixture doit tuer l'injection restante.

L'identité scalaire `paires_couvertes=C(n,2)` est nécessaire, pas suffisante :
une omission peut être compensée par un doublon. Le juge borné doit exiger une
multiplicité exactement `1` par `PairId`; le produit conserve une partition de
records disjoints et sa preuve structurelle.

Enfin, le cutoff direct dominance porte la frontière uniforme worst-case de la
cellule, atteinte par les rayons extrêmes, pas la frontière exacte du spindle
pour chaque paire de directions.

## Déblocage mathématique prioritaire : crédits cellulaires sans triples

Le prochain jalon ne doit pas factoriser le greedy de triples actuel. Le
théorème de groupe vaut pour un ensemble fini `G` de taille quelconque : si la
direction cible appartient à son cône positif et si chaque membre satisfait la
puissance H2 strictement, `G` fournit un intérieur, même si aucun membre ne le
fait universellement seul. Carathéodory borne à trois un sous-groupe pour une
direction **fixée** ; il n'impose pas d'énumérer ces sous-groupes pour couvrir
une cellule entière.

Soit une cellule simpliciale `C=cone(r0,r1,r2)` dont les rayons sont sur la
section de hauteur `T`. Pour un site relatif `s`, poser :

$$m_C(s)=\min_{0\leq j<3}r_j\mathbin{\cdot}s.$$

Si `m_C(s)>0`, le site devient H2-actif sur tout le suffixe de hauteur `x` dès
que :

$$x\,m_C(s)>T\left\lVert s\right\rVert^2.$$

L'événement entier exact est donc
`X_s=floor(T||s||^2/m_C(s))+1`. Dans le pool actif, choisir
`w=r0+r1+r2`, couper chaque rayon par `w dot u=1` et construire l'enveloppe
convexe 2D canonique des directions de sites. Le cône du pool contient toute
la cellule exactement lorsque les trois rayons normalisés appartiennent à
cette enveloppe. Pour chacun des trois rayons, un carrier de rang un, deux ou
trois suffit ; l'union emploie au plus neuf `PointId` et forme un crédit de la
cellule entière. Retirer ses IDs puis recommencer donne jusqu'à
`h=smax+1-q` crédits disjoints. Un échec glouton reste `UNKNOWN` : aucun packing
maximal n'est requis pour la sûreté.

Ce certificat se rejoue sans division. Les orientations projectives ont le
signe des déterminants `det(s_i,s_j,s_k)` puisque les dénominateurs sont
positifs. Chaque crédit doit publier ses IDs, vérifier H2 stricte pour chacun et
faire passer **les trois rayons extrêmes** par Cramer. Un groupe qui contient
seulement une direction représentative ne certifie jamais toute la cellule.
Les égalités coniques et carriers de rang un/deux sont légitimes ; une égalité
H2 reste toujours résiduelle.

La sortie naturelle est un record
`(AnchorId,CellId,lane,X,TargetSuffixNodeKeys,CreditKeys)`, pas des `PairId`.
Dans une première version, `AnchorId` reste une feuille : les vecteurs
`s=z-a` changent avec l'ancre. Le passage à un bloc d'ancres exige des extrema
communs qui recertifient H2 et les trois inclusions coniques, jamais le partage
du pool d'une ancre représentative.

## Raccord factorisé avec la dominance

Commencer par la partition canonique des paires non ordonnées
`(N,N) -> (L,L),(L,R),(R,R)`. Sur un rectangle `A times B`, certifier d'abord
que toutes les différences et tous les témoins communs appartiennent à la même
cellule half-open. Si `ell` est sa hauteur linéaire, `zeta_h` le h-ième témoin
absolu commun, `alpha=min_A ell` et `beta=min_B ell`, alors le rapport radial
minimal sur le rectangle est :

$$r_{AB}=\frac{\beta-\alpha}{\zeta_h-\alpha}.$$

La preuve exige les gardes explicites `zeta_h>max_A ell` et
`beta>zeta_h`. Dans ce domaine, le rapport croît séparément avec `ell(a)` et
`ell(b)` ; les deux minima donnent donc un minorant sûr. Le cutoff direct de la
cellule peut fermer le rectangle entier. En cas d'échec, scinder `A` ou `B` et
ne créer une tâche `(a,CellId)` qu'au résiduel feuille. Les deux orientations
sont évaluées sur le même `RectId` : fermeture par `OR`, résiduel par `AND`,
sans joindre des listes de paires.

Le cœur intervient avant ces splits seulement si ses bornes et un minorant
d'occupation viennent de la traversée déjà en cours. Il ne reçoit ni WSPD ni
index dédié tant que son ratio masse fermée/travail n'est pas vert.

Un cœur sous-plein peut néanmoins produire des **crédits partiels** : chacun de
ses occupants est un témoin individuel commun à toutes les paires du bloc. Ces
crédits peuvent compléter dominance et groupes coniques si le ledger conserve
les `PointId` et impose la disjonction des ensembles membres. Il faut donc
publier l'histogramme `0,1,...,>=h` et les IDs, pas seulement jeter les classes
`1..h-1`.

## Ordre recommandé

1. Corriger `smax`, les IDs/reçus et les portes des probes dominance/groupes.
2. Construire la dominance par rectangles, puis les crédits cellulaires par
   événements d'activation et enveloppes 2D ; garder singleton/paire/triple
   comme carriers reçus et le transverse comme raffinement, jamais comme
   catalogue global.
3. En parallèle seulement, conserver une microgate cœur `vide/h-1/h/bord` avec
   vraie range query et arrêt après `h`.
4. N'évaluer une WSPD qu'en ablation contre les rectangles LCA/dual-tree, et ne
   la retenir avec le cœur que si masse fermée par visite, bytes, HWM et deux
   pentes sont verts. Une WSPD choisit les blocs ; elle ne peuple pas le cœur.
5. Garder filtre FP et lift comme optimisations postérieures, jamais comme
   raccourcis de complétude.

Le cœur est donc un **fast path positif opportuniste**, jamais une couverture,
un préfiltre négatif ou une priorité avant la factorisation dominance+groupes.
G4 reste NO-GO.

Le commit `d3329fea4b595b7bbd283e509b0fa1955fcc3b06` répond déjà à une partie de
cet audit : retrait des deux faux mutants séparation/count, suppression du
`-2`, compteurs vide/sous-plein distincts et multiplicité exacte des `PairId`.
Le P0 `smax`, le faux mutant de bord, la fixture ceil, les IDs du cœur et la
range query factorisée restent ouverts.

GCP non utilisé.
