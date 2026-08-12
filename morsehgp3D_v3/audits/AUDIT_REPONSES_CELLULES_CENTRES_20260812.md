# Audit des cellules de centres — réponses à Claude et route sparse corrigée

Date : 12 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Cet audit répond à
[`QUESTIONS_CLAUDE_CELLULES_CENTRES_20260812.md`](QUESTIONS_CLAUDE_CELLULES_CENTRES_20260812.md),
corrige la génération proposée dans
[`NOTE_ARCHITECTURE_GPU_LISTES_CELLULES_CENTRES_20260812.md`](NOTE_ARCHITECTURE_GPU_LISTES_CELLULES_CENTRES_20260812.md)
et ne modifie aucun fichier de code.

## 1. Verdict court

| question | verdict | conséquence |
| --- | --- | --- |
| L1, census sur la liste | **reçu** | `p'+q<=smax` certifie simultanément `beta<=R_q` et la complétude du census; le test rationnel `beta<=R_q` est inutile dans cette branche |
| census commun sur `A_2` | **reçu** | exact avec le seuil et l'arité propres au support, si le support a bien été généré dans `A_q` |
| L2, `U_B` identifie la boule | **reçu sémantiquement** | exact dans un snapshot authentifié pour une boule munie d'un support minimal positif; impropre comme clé physique chaude avant census |
| cliques d'intervalles | **filtre exact, pas réduction sparse** | pire cas `Theta(m^4)`; la version bitset évite les retests mais pas les cliques nombreuses |
| premier GO LiDAR sur terrain/multi-écho | **refusé comme SLO officiel** | série diagnostique utile, mais `uniform` et le mélange équilibré de huit amas restent bloquants selon le plan de tests |
| doublons de coordonnées | **refus explicite reçu provisoirement** | `unsupported_degeneracy`, sans jitter; l'agrégation exige un quotient séparé sur les `PointId` et multiplicités |
| génération q2 puis q3 puis q4 | **réfutée si elle dépend des supports inférieurs retenus** | les lanes q3 et q4 doivent être générativement indépendantes |

Le théorème de listes imbriquées est exact. Il ne suffit toutefois pas à faire
de la machine une source sparse : ni le nombre de cellules, ni la taille des
listes, ni le nombre de cliques ne possède aujourd'hui de borne compatible avec
50 000 points sous une seconde.

## 2. L1 est exact, avec deux précautions

Pour une cellule `C`, une arité `q`, `t_q=smax-q+1`, le seuil `R_q(C)` et la
liste `A_q(C)`, supposons que le centre d'un support minimal positif soit owner
de `C`. Si `beta>R_q(C)`, les `t_q` témoins vérifiant `u_C<=R_q(C)` sont tous
dans `A_q(C)` et strictement intérieurs. Le compte restreint `p'` vérifie donc
`p'>=t_q`. La contraposée est exactement :

$$p'+q\leq s_{\max}\Longrightarrow\beta\leq R_q(C)\Longrightarrow I_B\cup U_B\subseteq A_q(C).$$

Il n'est donc pas nécessaire de former le produit croisé large comparant
`beta` à `R_q`. Deux limites doivent rester explicites :

- un support q doit être généré avec tous ses membres dans `A_q`; le census peut
  ensuite être élargi à `A_2` parce que `A_q` est inclus dans `A_2`;
- le shell d'une branche rejetée tôt est incomplet et ne peut jamais être
  publié. Une branche survivante doit matérialiser le shell entier avant de
  produire `U_B`, une classe extra-shell ou un reçu de dégénérescence.

La porte Source S est `p+q<=smax`. La propriété
`p+|U_B|<=smax` est la classification distincte
`accepted_closed_rank`. Une boule vérifiant
`p+q<=smax<p+|U_B|` n'est pas un rejet sain silencieux : elle doit entrer dans
un quotient de plateau reçu ou terminer explicitement en
`unsupported_degeneracy`.

### Correction du resserrement `tight`

Le théorème précédent est global lorsque les domaines actifs sont emboîtés et
que `A_q(enfant)` est recalculé depuis `A_q(parent)`. Le prototype live fait
plus : il filtre sur la cellule dyadique, resserre ensuite le domaine vers
`tight=C intersection bbox(P)`, recalcule des seuils sur ce domaine, puis
transmet le pool obtenu à des enfants dyadiques qui ne sont pas nécessairement
inclus dans `tight`. Il ne peut donc pas appeler ses seuils et listes les
`R_q(D)` et `A_q(D)` globaux, ni publier `R_child<=R_parent` entre ces deux
domaines non emboîtés.

La complétude se répare sans rescan. Si un pool hérité `P` contient déjà
`I_B union U_B`, si le domaine compact actif contient le centre de `B`, et si
`B` a exactement `p` intérieurs, alors la `(p+1)`-ième statistique calculée
seulement sur `P` vérifie encore :

$$\beta_B\leq R_{p,P}\quad\Longrightarrow\quad I_B\cup U_B\subseteq D_{p,P}.$$

La preuve est la même contradiction par `p+1` intérieurs. De plus,
`c_B in conv(U_B)` place le centre dans `bbox(P)` et dans tous les slabs k-DOP
fermés dérivés de `P`. Le filtre conserve donc le census pertinent et
l'induction continue. Ce **pool-relative conservation invariant** reçoit la
complétude de la source, mais pas l'identité des listes avec un rescan global.
Il faut séparer dans les reçus le digest du pool, le domaine actif et la cellule
owner, puis graver un enfant débordant du `tight` parent.

## 3. Renforcement prioritaire : stratifier par budget d'intérieurs

Le même argument devient plus sélectif si la liste est indexée par un budget
`h` d'intérieurs plutôt que seulement par l'arité. Pour `h>=0`, prendre la
`(h+1)`-ième plus petite valeur de `u_C`, notée `R_h(C)`, et poser
`D_h(C)={x:l_C(x)<=R_h(C)}`. La lettre `D` distingue cette enveloppe de
l'ancienne liste `A_q` indexée par l'arité; on a précisément
`A_q=D_(smax-q)`. L'indice `h` n'est pas encore la profondeur vraie de la
boule : cette profondeur `p` n'est connue qu'au point fixe du census.

**Lemme budget--cellule.** Si une boule positive owner de `C` a exactement
`p` sites strictement intérieurs, alors `beta<=R_p(C)` et
`I_B union U_B` est inclus dans `D_p(C)`.

La preuve tient en une ligne : si `beta>R_p(C)`, les `p+1` témoins ayant
`u_C<=R_p(C)` seraient strictement intérieurs, contradiction.

Les hypothèses exactes sont : fermeture de cellule compacte, bornes `l=min` et
`u=max` exactes sur cette fermeture, `p+1<=n`, statistique avec multiplicité de
`PointId`, filtre fermé `l<=R_p`, intérieur strict et aucune troncature des ex
æquo. Si `p+1>n`, la branche prend `D_p=X` et reste fail-open. L'owner emploie
la cellule half-open, mais les bornes emploient toujours sa fermeture.

Les propriétés d'implémentation sont favorables :

- sous subdivision, `D_p(enfant)` est inclus dans `D_p(parent)` par la même
  monotonie `l` croissante, `u` et `R_p` décroissants;
- dans une cellule, `D_0` est inclus dans `D_1`, puis dans les listes suivantes;
- au centre singleton d'une boule ayant exactement `p` intérieurs,
  `R_p=beta` et `D_p=I_B union U_B`;
- sous la porte régulière qui impose `U_B=U`, la liste singleton possède donc
  `p+q<=11` membres.

Pour q3, les budgets utiles sont `h=0..8`; pour q4, `h=0..7`. Ils tiennent dans
une seule CSR `D_8`, chaque site portant son premier `p` d'appartenance. Une
réduction top-9 produit tous les seuils. Si q2 est aussi traité par cette
machine, il faut ajouter `p=9`, `D_9` et un top-10; la route Yao-1/q2 séparée
permet de laisser ce coût hors de la source q3/q4.

Définir `tau_C(x)=min{h:x in D_h(C)}` et, pour un support proposé `U`, son
indice d'entrée **immuable**
`e0(U)=max(tau_C(x):x in U)`. Comme `U` est inclus dans `D_p`, on a `e0<=p`.
La promotion emploie un curseur distinct `h`, initialisé à `e0`, et le compte
total `r_h=|I_B intersection D_h|` après scan complet du préfixe courant.
L'invariant exact est `h<=r_h<=p` : pour `e0>0`, un membre de `U` absent de
`D_(e0-1)` impose `beta>R_(e0-1)` et fournit `e0` témoins stricts; le cas
`e0=0` est trivial avec `D_-1` vide.

Si `r_h<=h`, la contraposée `beta>R_h => r_h>=h+1` ferme le census; elle donne
`r_h=p=h` et `I_B union U_B` inclus dans `D_h`. Si `r_h>h`, poser
`h_new=r_h` et scanner seulement les buckets `D_(h_new) minus D_h`; l'invariant
est préservé, `h` croît strictement et la terminaison survient en au plus
`p-e0` promotions. Un compte total `r_h>smax-q` prouve
`above_support_window`. Une sortie anticipée est permise uniquement au
`(smax-q+1)`-ième intérieur; elle ne publie aucun shell partiel. Tous les
contacts de puissance nulle rencontrés avant et pendant les promotions sont
accumulés, faute de quoi le census fermé final serait incomplet.

La promotion reste dans la **même cellule figée**, avec le même cloud/epoch,
les mêmes `R_h`, `tau` et buckets. La migrer vers un enfant oblige à tout
recalculer. L'exact-once spatial exige en outre une partition terminale commune
à tous les budgets d'une arité : une cellule ne peut pas émettre pour `h=0`
pendant que la lane `h=1` subdivise le même domaine. À défaut, il faut un ledger
global par `SupportKey`, plus coûteux. Dans une feuille commune, chaque
q-sous-ensemble de `D_(h_max)`, avec `h_max=smax-q`, est proposé une fois et étiqueté par son seul `e0`;
un producteur par buckets doit imposer `max tau(U)=e0` et un anchor canonique.
L'identité exacte
`sum_e0 count(max tau(U)=e0)=C(|D_(h_max)|,q)` montre aussi que cette
stratification ne rend pas la génération directe sparse : elle évite les
doublons entre budgets et réduit le census.

Une fixture u16 tue les arbres de budgets indépendants. Prendre
`A=(10,10,10)`, `B=(20,10,10)`, `C=(15,18,10)` et `W=(15,12,10)`. Le support
`ABC` a le centre `(15,199/16,10)`, le rayon carré `7921/256`, le shell `ABC`
et le seul intérieur `W`. Dans la cellule racine
`[10,20] x [10,18] x {10}`, les quatre bornes inférieures valent zéro, donc
`D_0=X` et `e0=0`; une lane `h=0` terminale peut proposer puis promouvoir ce
support. Dans la cellule singleton du centre, `D_0={W}`, `D_1=X` et `e0=1`;
une lane `h=1` subdivisée le propose une seconde fois. La décision
`split/terminal` doit donc porter sur le maximum de travail de tous les budgets
et s'appliquer collectivement à la cellule.

Cette règle ne fusionne pas les arités : les générateurs q3 et q4 restent
indépendants, et le nombre de supports proposés reste l'inconnue industrielle
principale.

Ce lemme donne une **source candidate exacte par arité `q` et budgets `h`**, pas
une borne de temps. Une profondeur dyadique fixée à 26 ne garantit pas à elle
seule que chaque `D_h` sera petite. Toute branche doit finir par l'un des statuts reçus
`pruned`, `terminal_exact`, `exact_fallback` ou `resource_exhausted`.

### Stabilisation terminale, résultat conditionnel

Pour des domaines compacts emboîtés contenant un centre `c` et de diamètre
tendant vers zéro, la liste globale `D_p` se stabilise exactement sur tous les
sites dont la distance à `c` est au plus le rayon du cutoff, ex æquo inclus.
La finitude du nuage fournit l'écart positif jusqu'au rayon suivant.

Sous l'hypothèse globale supplémentaire « aucun cinq sites cosphériques », le
cutoff a multiplicité au plus quatre. À `smax=11`, on obtient alors les bornes
terminales sûres `13/12/11` pour q2/q3/q4, soit 78 paires, 220 triplets et 330
quadruplets. `RelevantGP` ne suffit pas à garantir cette hypothèse : une grande
cosphère non critique peut rester au cutoff. Ces nombres sont donc des
conditions de terminalisation, jamais des caps d'exactitude.

## 4. Correction de complétude : arités indépendantes

La note antérieure proposait de former q3 depuis des bras q2 certifiés, puis q4
depuis un triangle q3 reçu. Cette implication est fausse si « certifié » ou
« reçu » signifie « support inférieur retenu par `p+q<=smax` ». Les lanes de
profondeur ne sont pas emboîtées entre arités.

Les deux constructions ci-dessous ont été rejouées le 12 août avec un solveur
`Fraction` autonome : équidistances, barycentriques, intérieurs stricts et
shells exacts ont été recalculés sans appeler les prédicats du dépôt.

### Fixture q3 sans aucune arête q2 pertinente

Prendre `A=(10,10,10)`, `B=(20,10,10)`, `C=(15,18,10)`, puis ajouter les trente
témoins suivants :

- pour `AB` : `(11,8,z)` avec `z=8..12`, `(11,9,8)`, `(11,9,12)`, puis
  `(12,7,z)` avec `z=8..10`;
- pour `AC` : `(8,13,10)`, `(8,14,z)` avec `z=9..11`, `(8,15,10)`,
  `(9,11,10)`, puis `(9,12,z)` avec `z=8..11`;
- pour `BC` : `(16,18,z)` avec `z=9..11`, `(17,16,6)`, `(17,16,14)`,
  `(17,17,7)`, `(17,17,13)`, puis `(17,18,z)` avec `z=8..10`.

Les boules diamétrales `AB`, `AC`, `BC` ont chacune exactement dix intérieurs
et leurs deux extrémités pour shell : `p+q=12>11`. Le triangle `ABC` a pour
centre `(15,199/16,10)`, rayon carré `7921/256`, barycentriques
`(89/256,89/256,39/128)`, aucun intérieur et exactement `A,B,C` sur le shell.
Il est pertinent alors qu'aucune de ses arêtes ne l'est.

### Fixture q4 sans aucune facette q3 pertinente

Prendre le tétraèdre
`(20,20,20),(60,60,20),(60,20,60),(20,60,60)`, puis les deux groupes :

- `G01={(21,55,65),(21,57,64),(21,58,63),(21,59,62),(22,53,67),`
  `(22,55,66),(22,56,65),(22,57,65),(22,58,64)}`;
- `G23={(21,21,18),(21,22,17),(21,23,16),(21,25,15),(22,21,17),`
  `(22,22,16),(22,23,15),(22,24,15),(22,25,14)}`.

Le tétraèdre a pour centre `(40,40,40)`, rayon carré `1200`, poids `1/4`, aucun
intérieur et son seul support q4 sur le shell. Chacune de ses quatre faces a
exactement neuf intérieurs et trois sommets sur le shell, donc `p+q=12>11`.
Le q4 pertinent ne peut être engendré depuis une facette q3 retenue.

Ces deux fixtures doivent devenir permanentes avant tout refactor. Le prototype
observé au snapshot décrit en section 8 évite déjà ce piège : ses cliques
géométriques q3/q4 ne dépendent pas du succès de `try_support` à l'arité
inférieure. Il faut préserver cet invariant.

## 5. Producteur terminal GPU proposé

La route la plus simple à falsifier est maintenant :

1. calculer top-9, `tau_C` et les buckets imbriqués `D_p` par
   `count/scan/fill`;
2. subdiviser selon une estimation du travail maximal de tous les budgets,
   avec une décision terminale commune par arité;
3. lorsque `|D_(h_max)|<=M`, énumérer une fois et séparément les triplets q3 et
   quadruplets q4, puis attacher leur `e0=max tau(U)`;
4. décider Gram--Cramer, indépendance et barycentriques exactes, puis owner;
5. produire `(GeometricBallKey,SupportKey,e0,cell_id)`, radix/RLE par clé
   géométrique en conservant **tous** les supports et choisir un représentant
   rejouable;
6. pour ce représentant seulement, partir de `h=e0`, promouvoir par compte
   intérieur total et scanner les nouveaux buckets jusqu'à fermeture ou
   dépassement; attacher le même `p` à la boule, puis appliquer `p+q<=smax`
   séparément à chacun de ses supports.

Placer la promotion avant le RLE répéterait précisément le strict-count pour
tous les supports d'une même boule et contredirait « un seul census par
`BallKey` ». Un support rejeté à une arité ne tombstone jamais toute la boule :
un support d'arité inférieure de la même clé peut rester pertinent.

Dans le cas idéal régulier, `M=11` donne au plus `C(11,3)=165` triplets ou
`C(11,4)=330` quadruplets par cellule et lane. `M=12..16` peut être testé comme
seuil de terminal, mais n'est jamais un cap d'exactitude. Une liste plus grande
est subdivisée, traitée par un producteur exact alternatif ou refusée pour une
ressource physique réelle.

Les filtres suivants sont sûrs, mais secondaires :

- k-DOP ou séparation convexe stricte de la cellule et de `conv(D_p)`;
- graphe local éphémère de bissecteurs, en bitsets registre/shared pour
  `M<=64`, puis triangles par intersection de deux lignes et q4 par intersection
  de trois lignes;
- pour q4, condition nécessaire des boules équatoriales et condition du
  cylindre appliquées à un triple géométrique, sans exiger que ce triple soit un
  support q3 positif;
- Jung et Helly comme prunes fail-open de blocs, jamais comme générateurs
  uniques.

Un q4 well-centered doit être résolu directement : ses faces peuvent être
obtuses. Le théorème des boules équatoriales et la condition du cylindre sont
des filtres nécessaires établis par VanderZee, Hirani, Guoy, Zharnitsky et
Ramos dans
[`Geometric and Combinatorial Properties of Well-Centered Triangulations`](https://arxiv.org/abs/0912.3097).

Un fait complémentaire évite une confusion : tout tétraèdre propre positif
possède **au moins deux** faces triangulaires aiguës, donc positives. Le résultat
classique est démontré dans la solution du problème 3653 de
[`Crux Mathematicorum 38(8), pages 341--343`](https://cms.math.ca/wp-content/uploads/crux-pdfs/CRUXv38n8.pdf).
L'argument suivant fournit déjà au moins une face. Après translation du
circumcentre en zéro, écrire ses demi-espaces
`n_i dot x<=h_i`, avec normales sortantes unitaires et `h_i>0`. Pour une face
d'indice `i` minimisant `h_i`, la projection `z=h_i n_i` de zéro vérifie
`n_j dot z<h_j` pour toute autre face; elle est donc dans l'intérieur relatif
de la face `i` et en est le circumcentre. Cela rend complète une génération q4
depuis **tous** les triangles positifs géométriques et tous leurs apex
compatibles. Cela ne rend pas complète une génération depuis les seuls q3
pertinents : la profondeur de cette face n'est pas bornée par celle du q4. La
génération q4 directe reste préférable tant que tous les triangles positifs ne
peuvent pas être produits sparsement.

Pour exploiter ce résultat sans énumérer tous les K4, le lieu des centres
équidistants d'une face aiguë `F` est la droite rationnelle normale au plan de
`F` passant par son circumcentre. Un q4 owner de la cellule impose que cette
droite rencontre son domaine actif. Le test droite--cellule exact précède les
apex; chaque q4 choisit sa plus petite face aiguë canonique. Le carrier reste
géométrique et indépendant du verdict shallow q3.

### Critère de split mesurable

Avant les bitsets, un potentiel exact est disponible. Trier les intervalles par
borne gauche et noter `a_i` le nombre d'intervalles antérieurs dont la borne
droite atteint la borne gauche de `i`. Le nombre exact de q-cliques
d'intervalles vaut `sum_i C(a_i,q-1)`, calculable par sweep en `O(m log m)`.
Cette quantité borne le travail avant les prunes de bissecteurs et distingue
une liste longue ordonnée d'une liste réellement ambiguë.

Compter ou estimer `E`, `T`, `Q` après les bitsets, puis comparer
`W_terminal=c_E E+c_T T+c_Q Q+c_G G+c_C C` à
`W_split=c_b B+c_s sum(m_child)+sum(W_child)+overhead`. Le split est choisi
seulement avec une marge stable, par exemple
`W_split<(1-epsilon)W_terminal`. Le choix binaire sur l'axe qui réduit le plus
les listes doit être mesuré contre l'octree : trois niveaux binaires peuvent
lire jusqu'à quatorze listes parentes, contre huit enfants lus une fois par un
octree, mais ils peuvent aussi éviter sept enfants presque identiques.

Le ledger publie `E/T/Q`, lifts, candidats positifs, census, nombre de cellules,
lectures parentes, octets et high-water pour chaque lane. Deux pentes
successives supérieures à `1,35` sur la rampe contractuelle ferment
l'ordonnance avant CUDA.

## 6. `U_B` : identité sémantique, pas clé chaude

L2 est correct après une reformulation. Le lieu des points équidistants de
`U_B` est un espace affine dont la direction est orthogonale à la direction de
`aff(U_B)`. Un support minimal positif `U` impose au centre d'appartenir à
`relint conv(U)`, donc à `aff(U_B)`. L'intersection est unique; le rayon est
alors imposé par n'importe quel membre du shell.

Les préconditions sont :

- `q>=2` et existence d'un support minimal positif rejouable;
- `U_B` constitué de `PointId` dans un dataset et un `cloud_epoch`
  authentifiés;
- census global terminé, égalités comprises.

`U_B` trié convient donc comme certificat d'identité **post-census**. Il ne
convient pas au RLE chaud : il n'existe qu'après ce census, sa taille peut être
`Theta(n)` et le former pour chaque support répète précisément le travail que
le RLE doit supprimer.

La clé chaude doit être géométrique et de taille fixe. Depuis la forme liftée
`D||y-a||^2+C dot (y-a)=0`, développer donne le 5-uplet homogène
`H=(D,C-2Da,D||a||^2-C dot a)`. Après normalisation `D>0` puis division des
cinq coefficients par leur pgcd, le tuple primitif est identique pour tous les
supports d'une même sphère et disponible avant census. Une empreinte radix plus
courte peut router les candidats, mais toute collision est tranchée par le
tuple exact et le rejeu d'un support; elle n'est pas une autorité. Les bornes
u16 q2/q3/q4 doivent être publiées avant de choisir i128 ou des limbs device.
Le prior art v3 contient déjà
`ExactCenterKey` et l'égalité de niveau `sphere_cmp_beta` en `BigInt<6>` dans
`prototype/validated_hybrid_sidecar.hpp`; il faut mesurer leur adaptation
device au lieu de réintroduire une clé variable.

Pipeline recommandé : `SupportKey + GeometricBallKey`, radix/RLE, strict-count
une fois par boule, puis census fermé CSR une fois par survivante. `U_B` sert
ensuite de vérification sémantique et de certificat de plateau.

## 7. Les cliques d'intervalles ne ferment pas Q1

Une autre formulation réduit le coût des bornes, sans changer le théorème.
Fixer une jauge dyadique `c_0` commune à tout l'arbre et poser

$$s_x(c)=\left\Vert x-c_0\right\Vert^2-2\langle x-c_0,c-c_0\rangle.$$

Comme `||x-c||^2=s_x(c)+||c-c_0||^2`, le second terme est commun aux sites :
les rangs, égalités et preuves de témoins se calculent avec les extrema affines
de `s_x`. Pour une boule de centre `c_B`, définir toutefois le niveau translaté
`theta_B=beta_B-||c_B-c_0||^2`. Les extrema affines se comparent à `theta_B`,
jamais directement à `beta_B` : `theta_B>R_h` fournit `h+1` témoins stricts,
tandis que `theta_B<=R_h` enferme la boule fermée dans `{x:L_C(x)<=R_h}`.
Le graphe d'ambiguïté relie `x,y` si zéro appartient à l'intervalle
exact de `s_x-s_y`, donc si leur bissecteur coupe la cellule. Tout support q
induit une q-clique. Cette jauge doit rester fixe dans tout l'arbre; la changer
par enfant demanderait une nouvelle preuve de nesting. Le choix par signes des
coefficients calcule ces extrema sur une AABB; un k-DOP utilisé comme domaine
actif demanderait ses sommets ou un programme linéaire exact et reste donc un
prune séparé dans le blueprint courant.

Une famille de `m` intervalles peut être une clique complète. Dire que
l'énumération coûte le nombre de cliques ne remplace donc pas le pire cas
`C(m,4)`. Les bitsets suppriment les retests de bissecteurs et sont une bonne
optimisation mécanique; ils ne rendent pas le graphe sparse.

Pour un Poisson homogène 3D, le rayon contenant en moyenne dix points vérifie
`r_10/s0=(30/(4*pi))^(1/3)=1,337`. Avec le halo proposé
`r_10+sqrt(3) delta`, le modèle de paires est proportionnel à
`delta^-3(r_10+sqrt(3)delta)^6`; son optimum continu est
`delta=r_10/sqrt(3)=0,772 s0`, et non `0,83 s0`. La discrétisation peut
expliquer l'écart, mais même `1,4e4 n` signifie 700 millions de contrôles de
paires à 50 000 points, avant lifts, census, fold et payload. Ce chiffre ne
justifie aucun GO sous une seconde.

La piste Delaunay par suppressions ou permutations est un proposer seulement.
Sous généricité, un support de taille `q` ayant `p` intérieurs devient vide dans
un préfixe aléatoire si ses `q` sommets précèdent ses `p` intérieurs, avec
probabilité exacte `1/C(p+q,q)`: au pire `1/165` pour q3 et `1/330` pour q4 dans
la fenêtre. Une famille déterministe séparant tous les couples `(U,I)` rendrait
la couverture exacte, mais multiplie les passes et réintroduit une structure de
Delaunay; elle n'est pas admise dans le chemin chaud.

Un résultat externe donne néanmoins une excellente porte d'oracle. Pour un
ensemble fini générique de dimension trois, un point générique appartient à au
plus `C(p+3,3)` tétraèdres dont la circumsphère contient exactement `p` sites,
avec égalité dans le `p`-hull. Le théorème 2.3 de Edelsbrunner, Garber et
Saghafian est publié dans
[`On Spheres with k Points Inside`](https://doi.org/10.4230/LIPIcs.SoCG.2025.43).
Cette multiplicité donne un invariant de petit oracle : pour un point de requête
générique, compter les tétraèdres p-hefty qui le contiennent et vérifier la
borne. Elle ne borne ni le nombre de centres owner d'une cellule de volume non
nul, ni le nombre total de tétraèdres, ni le coût pour les découvrir.

Une seconde référence fournit une baseline quantitative beaucoup plus forte
pour `uniform`. L'équation (7) d'Edelsbrunner et Nikitenko compte, dans un
processus de Poisson stationnaire homogène 3D d'intensité `rho`, les simplexes
de support `q=u+1`, de type de visibilité `v`, ayant exactement `p` points
strictement intérieurs et leur centre dans `Omega`. À rayon non borné :

`E[N_(v,u,p)(Omega)] = C^3_(v,u) binom(p+u-1,u-1) rho |Omega|`.

Dans le cas critique positif `v=u`, les constantes 3D publiées sont
`C^3_(1,1)=4`, `C^3_(2,2)=3+3*pi^2/16` et
`C^3_(3,3)=3*pi^2/16`. La fenêtre `smax=11` autorise respectivement
`p=0..9`, `p=0..8` et `p=0..7`. La Source S positive attendue vaut donc :

`[40 + 45(3+3*pi^2/16) + 120(3*pi^2/16)] rho |Omega| = (175+495*pi^2/16) rho |Omega|`,

soit environ `480,340886 rho |Omega|`. L'analogie bulk
`rho |Omega| ~= 50 000` donne environ **24,017 millions de supports** :
`2,000 millions` en q2, `10,914 millions` en q3 et `11,103 millions` en q4.
Une architecture qui matérialise chaque support doit donc déjà absorber ce
trafic avant déduplication, census, tri et fold; elle doit mesurer le débit de
records et privilégier la fusion en ligne vers le consommateur H0 plutôt qu'un
catalogue hôte. Ce nombre est une baseline de charge moyenne bulk pour cette
architecture de Source S, pas un minorant sur tout algorithme exact du H0.

La formule tronquée donne aussi la loi conditionnelle
`rho nu_3 R^3 ~ Gamma(p+u,1)`. Elle peut prédire la distribution de profondeur
et dimensionner les queues de cellules; sa queue est non bornée et aucun
quantile ne peut devenir un prune exact. Voir
[`Poisson--Delaunay Mosaics of Order k`](https://doi.org/10.1007/s00454-018-0049-2)
et les constantes de
[`Expected Sizes of Poisson--Delaunay Mosaics`](https://doi.org/10.1017/apr.2017.20).

Ces identités portent sur un Poisson continu en espace entier, en position
générique presque sûrement, et comptent les centres dans `Omega`. Remplacer
`rho |Omega|` par le nombre fixé de points d'une boîte finie est une
approximation bulk, pas l'identité publiée; les termes de bord, la
quantification u16, les doublons, les plateaux, les nuages surfaciques et les
amas restent hors hypothèses. Elles ne bornent ni les cliques ou transits
visités pour découvrir les supports, ni la mémoire ou le temps de
l'algorithme. Le reçu doit donc publier par `(q,p)` au moins
`unique_balls/n`, `candidate_supports/unique_balls` et
`cell_views/unique_balls`, ainsi que les octets de records avant/après fold.

## 8. Audit pincé des prototypes successifs

### Snapshot livré à 13:05 UTC

Relevé à `2026-08-12 13:05:15 UTC` :

- `HEAD=8c00ab07695ef353e673ab73a778a6f260c87509`;
- `CMakeLists.txt` SHA-256
  `dee88c760e0ddc2d53406a07de8ff1a1a2d1685e89d49a3c5af4ef14063eacd0`;
- `prototype/centre_cell_source.cpp` SHA-256
  `14787923af4e10f03a33126d6055ceb5e7033fe63a2c231df977b76fe2b85257`;
- binaire Release SHA-256
  `8e09984e2c199036c617ffd15f87db5b55f6d398c642ccdf80cb55bdb3a6558e`;
- la configuration recense `468` CTests, dont huit `centre_cell`.

Le source et le binaire ont changé pendant les audits. Les octets
`14787923...` et `8e09984e...` n'ont pas été archivés et ne sont plus présents
dans le worktree; les résultats ci-dessous sont donc une **observation
historique non reproductible**, jamais un verdict live. Les empreintes
empêchent seulement de les attribuer par erreur au successeur.

### Porte ciblée

Sur le binaire historique, la commande

```bash
ctest --test-dir build/v3 --output-on-failure -R '^mhgp3v_centre_cell_'
```

a rendu `7/8` en `6,21 s`. La porte
`mhgp3v_centre_cell_fixture_equality` échoue avant d'exécuter la fixture : CMake
passe `--fixtures`, tandis que le parseur n'accepte que
`--fixtures=egalite|proprietaire|toutes`. Les quatre accords aléatoires ne
remplacent pas cette égalité ciblée.

### Densité observée

Sur ce même binaire historique, `uniform`, `seed=11`, `smax=11`, `leaf=48`,
`pair_cap=64`, sans juge :

| n | cellules terminales | paires | triplets | quadruplets | tests census | supports |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 40 | 1 | 780 | 9 880 | 91 390 | 350 516 | 3 986 |
| 50 | 71 | 22 988 | 119 827 | 454 960 | 450 035 | 5 677 |
| 60 | 120 | 37 308 | 178 491 | 557 135 | 538 299 | 7 474 |

À `n=40`, les trois compteurs sont exactement `C(40,2)`, `C(40,3)` et
`C(40,4)`. À `n=50`, la subdivision engendre `454 960` vues q4, contre
`C(50,4)=230 300` quadruplets globaux. Le filtre `rank_prefix` ne retire aucun
candidat dans ces trois runs. La machine peut donc payer davantage que
l'exhaustif global avant même 60 points; elle reste un oracle branch-and-bound,
pas une source sparse admise.

### Défauts et acquis statiques

- le graphe bitset courant est la bonne optimisation locale, mais son commentaire
  `O(nombre de cliques) au lieu de C(m,q)` est un surclaim;
- les boucles q3/q4 partent des cliques géométriques et ne dépendent pas du
  verdict du support inférieur : cet invariant de complétude doit rester;
- `int in_ids[24], sh_ids[24]` puis `exit(3)` au-delà de 24 est incompatible
  avec une coquille pertinente de taille arbitraire. Le produit exige une CSR à
  offsets 64 bits ou un refus transactionnel reçu, jamais un invariant interne;
- le groupement par `U_B` est effectué après un census par support, donc il ne
  supprime aucun census dupliqué;
- le target est CPU seulement; aucun kernel CUDA `centre_cell` ni producteur du
  payload officiel n'existe;
- le juge de ce target partage `ball_front.hpp`, les lifts et `power_of` avec le
  sujet. Son accord d'identités est utile, mais il n'est pas un juge
  arithmétiquement indépendant.

Une fixture u16 minimale expose le tampon shell : prendre le centre
`(10,10,10)` et les trente points de rayon cinq obtenus par les six
permutations/signes de `(5,0,0)` et les vingt-quatre de `(4,3,0)`. La paire
antipodale `(5,10,10),(15,10,10)` a `p=0,q=2`, mais `|U_B|=30`. Elle est
pertinente à `smax=11`, extra-shell, et `D_0` conserve les trente égalités à
toute profondeur. Cette fixture doit tuer tout cap shell et rappelle que même
la stratification par `p` peut garder une liste `Theta(n)`.

La représentation dyadique demande aussi une correction de bits dans la note
d'architecture. Avec les numérateurs de cellule à l'échelle `2^d`, la borne est
`l,u<=3(65535*2^d)^2<2^(34+2d)`, non `2^34`. Elle reste sous `i128` à
`d<=26`, mais pas sous un entier device 64 bits. Une implémentation GPU exige
des limbs 96/128 bits ou une autre représentation reçue, avec fixtures
`coord=65535,max_depth=26`.

La matrice `adj` courante est dense en bits, de taille quadratique en `top`, et
`list_bytes_high_water` est déclaré sans être alimenté ni publié. Avant toute
allocation device, un preflight d'octets doit choisir bitset pour les petits
terminaux, CSR sparse lorsque cela gagne réellement, ou
`resource_exhausted`.

Un audit statique du snapshot antérieur `300099a6...` n'a trouvé aucune faute
d'aliasing DFS, d'ownership half-open, de largeur sous `u16,max_depth<=26`, ni
de faux prune k-DOP. Ce résultat ne se transfère pas automatiquement aux octets
ci-dessus; il indique les invariants à conserver et à graver.

### Successeur testé à 13:31 UTC

Claude a corrigé plusieurs défauts avant la fin du contre-audit. Le successeur
testé est pincé par :

- `prototype/centre_cell_source.cpp` SHA-256
  `fd73409257bf61e3b41d872f92e3b4eedda5eaeda836399beec2c4aacb4959b1`;
- binaire Release SHA-256
  `b2b430bb91a449ba0d8279316b58ebd99b683aebba044ef351068fdff49c50cd`;
- `CMakeLists.txt` SHA-256
  `dee88c760e0ddc2d53406a07de8ff1a1a2d1685e89d49a3c5af4ef14063eacd0`.

Le source `fd734092...` et l'ELF `b2b430bb...` ont tous deux été remplacés avant
la consolidation; aucun transcript brut n'a été archivé. Cette section est donc
une **observation historique entièrement non reconstructible depuis le worktree
courant**, pas un verdict live.

Sur ces octets, le CLI acceptait `--fixtures`, les deux contre-fixtures
inter-arités étaient exécutées dans `--fixtures=toutes`, le shell utilisait des
vecteurs et les candidats étaient groupés par centre exact puis rayon exact
**avant** census. La porte `^mhgp3v_centre_cell_` a rendu `8/8` en `3,18 s`.

Cette porte reste incomplète du point de vue mutant. Le test d'égalité vérifie
directement quatre distances, mais n'injecte pas `drop-ties` dans la machine de
cellules. Sur `terrain,n=60,smax=6,seed=3`, ce mutant rendait le code 3 tout en
reproduisant les 918 identités du juge : il survivait. En revanche, les
campagnes manuelles tuaient `strata-stop`, et
`--fixtures=toutes --inject=arity-cascade` tuait la dépendance inter-arités avec
833 supports manquants. CMake ne raccordait que `shrink-list` comme mutant de
la machine.

La rampe suivante employait les defaults du snapshot `fd734092...` :
`leaf=4,pair_cap=256,max_depth=22`,
`uniform,seed=3,smax=11`, sans juge. Les secondes sont des observations locales
partagées, jamais un benchmark; seuls les comptes servent à la gate.

| n | cellules | lectures parentes | IDs candidats | lifts | rejets owner | census | tests census | supports |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 100 | 3 409 | 127 044 | 73 728 | 1 053 389 | 898 944 | 46 121 | 523 852 | 16 428 |
| 200 | 9 345 | 366 632 | 207 234 | 2 912 908 | 2 490 394 | 126 365 | 1 424 819 | 41 637 |
| 400 | 24 081 | 970 264 | 532 586 | 7 240 129 | 6 203 556 | 323 189 | 3 584 629 | 103 978 |

Les deux pentes successives de `cells_created`, `parent_candidate_reads`,
`candidate_ids` et `census_scans` dépassent 1,35. À `n=400`, 85,7 % des lifts
meurent seulement à l'owner et le groupement économise 87 scans, tandis que
323 189 census sont lancés pour 103 942 boules acceptées. Une extrapolation
strictement linéaire vers 50 k — diagnostic, pas loi asymptotique — donne
environ 3,0 millions de cellules, 905 millions de lifts et 448 millions de
tests census. Le port littéral reste donc `NO-GO` malgré les réparations de
correction.

La prochaine réduction doit intervenir avant le lift et l'owner : scores
affines à jauge fixe, split guidé par le graphe de bissecteurs, carrier aigu q4
avec test droite--cellule et clé primitive de sphère. Le regroupement actuel
par centre compare encore quadratiquement les rayons concentriques; la clé
primitive supprime cette classe de collision.

## 9. Réponses Q2 et Q3

`terrain` et `scanline_overlap_multiecho` sont de bons diagnostics LiDAR. Ils ne
peuvent pas devenir seuls bloquants pour le SLO. La section 14.5 du plan de
tests impose au minimum six régimes et évalue les objectifs sur le Poisson
uniforme **et** le mélange équilibré de huit amas. Le générateur v3 courant ne
possède pas encore ce second régime. De plus, « les centres restent dans un
tube autour de la surface » est une heuristique sans hypothèses de reach,
d'épaisseur et d'échantillonnage : la positivité donne seulement
`c in conv(U)`, et un support spatialement étendu peut éloigner son centre de la
surface locale.

Pour les doublons de coordonnées, le refus explicite est la politique exacte
la plus petite aujourd'hui. Une future politique `duplicate_policy=aggregate`
doit spécifier les multiplicités, les rangs, les `PointId`, les niveaux nuls et
le payload avant d'être utilisée. Sur GPU, la détection se fusionne naturellement
avec le radix Morton résident; un second sort hôte n'est pas l'architecture
produit.

## 10. Join sparse des générateurs saturés en H0

Une fois les boules groupées, il faut éviter un second coût quadratique entre
plateaux. À niveau `beta` et ordre `k` fixés, soit `S_g` le saturé fermé porté
par le générateur `g`. Le graphe abstrait relie `g` et `h` lorsque
`|S_g intersection S_h|>=k`.

Avant d'énumérer des postings, une fermeture plus forte vaut pour le seul H0
normalisé. Pour une boule `B` et `S_B=X intersection B`, tout
`(k+1)`-sous-ensemble de `S_B` a une miniboule de niveau au plus `beta(B)`.
Deux k-sous-ensembles qui diffèrent d'un label sont donc reliés à la coupe
fermée; le graphe induit contient tout le graphe de Johnson `J(|S_B|,k)`, qui
est connexe. Un seul token `(GeometricBallKey,beta,S_B)`, un support positif
canonique rejouable et les racines strictes pré-lot peuvent ainsi représenter
l'effet interne du plateau sans énumérer ses cofaces ni tous ses supports.

Cette fermeture ne restitue ni Gamma facetté, ni les verticales, ni la
chronologie ouverte. Elle est précisément la raison pour laquelle le chemin H0
normalisé ne doit pas conserver un catalogue quartique de supports portés par
une grande cosphère.

Il suffit de produire un posting `(k,F,g)` pour chaque k-sous-ensemble
`F` de `S_g`, de radix-trier par `(beta,k,F)`, puis d'unir les générateurs de
chaque run en étoile. Deux générateurs partagent un posting exactement quand
leur intersection contient au moins k labels; les composantes sont donc
identiques à celles du join pairwise, sans matérialiser toutes ses arêtes.

Le travail devient `P_k=sum_g C(|S_g|,k)`, et non la somme des carrés des degrés
des postings. Sous `|S_g|<=11`, le maximum par générateur est `C(11,5)=462`
pour un ordre fixé et la somme sur `k=1..10` vaut 2 046. Les racines strictes
sont gelées avant le lot, puis les unions sont commises atomiquement.

Ce théorème porte seulement le quotient/fold H0 au lot considéré. Il ne produit
ni le payload Gamma facetté, ni les incidences futures; un resolver ou un index
`F -> handle` reste nécessaire. Si `|S_g|` est extra-shell et non borné, les
postings redeviennent combinatoires : employer le token de fermeture pour H0,
ou une route Gamma explicitement admise, jamais les générer par défaut.

## 11. Lookup de gateway sans requête négative

Soit `F` une facette du cœur, avec `|F|<=10`, `B_F` sa miniboule, `q` la taille
d'un support minimal, `r=|F intersection int(B_F)|` et
`j=|(X minus F) intersection int(B_F)|`. Une table terminale complète de toutes
les boules pertinentes `p+q<=11` donne une dichotomie exacte :

- si `GeometricBallKey(B_F)` est présent, son census donne `J_F` exactement;
- s'il est absent, alors `r+j+q>=12`, tandis que `r+q<=|F|<=10`; donc `j>=2`.

Aucune requête globale ne sert donc à certifier `J_F=0` ou `J_F=1`. La branche
absente lance seulement une recherche garantie positive, arrêtée après deux
intrus; un échec à en trouver deux réfute la complétude de la table. Les
co-minimiseurs de la branche `J_F=0` restent issus du premier bucket des
postings `DirectQ -> FacetKey`. Ce lemme retire les recherches négatives du
gateway, sous l'hypothèse explicite d'une table Source S complète.

## 12. Décision avant G4

`NO-GO` pour un benchmark G4 de latence de cette ordonnance. Les raisons sont
structurelles : plusieurs compteurs ont deux pentes rouges, environ 905
millions de lifts sous la seule extrapolation linéaire à 50 k, absence de CUDA
et absence de `BenchmarkOutputContract-v1`. Les rouges historiques de fixture,
du shell fixe et du RLE post-census ont été réparés dans le successeur; ils ne
doivent plus être présentés comme des défauts live.

Le prochain reçu utile reste CPU et déterministe : invariant pool-relative,
mutant `drop-ties` réellement tué dans la machine, clé primitive de sphère,
scores affines, carriers aigus q4, oracle arithmétiquement indépendant, puis
rampe de compteurs sur `12 500/25 000/50 000`. Installer immédiatement le
squelette de `BenchmarkOutputContract-v1` et l'interface verticale avec statut
`incomplete`; G4 n'est justifiée qu'après passage de la gate de travail et
existence du producteur device complet.

GCP non utilisé.
