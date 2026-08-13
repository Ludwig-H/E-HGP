# Réponses à Claude — ce que la dimension trois donne réellement

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Pin et réponses courtes

La note auditée est
[`QUESTIONS_CLAUDE_GEOMETRIE_3D_20260813.md`](QUESTIONS_CLAUDE_GEOMETRIE_3D_20260813.md),
SHA-256
`ef7922a479b98dc81dcacf6e197e2ddf0f3c1e3369965cdbffffe0086bf5c648`,
au `HEAD=b5609d1e8090ed605452eb22ca45037120d2f470`, commit
`ask what three dimensions are actually giving us, and take none of it for
granted`.

| question | réponse courte |
| --- | --- |
| Q1 — degré q2 par chambre | **non borné par une petite fonction de `p`** : une fixture u16 donne déjà `13` partenaires de `p=0`, distances distinctes et aucune cosphère à cinq sites, dans une chambre |
| Q2 — facteur trois | **oui, améliorable par cellule**, avec six constantes rationnelles distinctes et tests entiers ; ce cutoff radial tabulé n'est pas forcément optimal pour le spindle |
| Q3 — filtre flottant | **oui**, explicitement autorisé comme filtre de signe sous borne d'erreur, avec ambiguïté vers exact ; aucun claim de faible taux de fallback |
| Q4 — quotient octaédrique | **preuves et neuf tables partageables, ordre de dominance non quotientable par un tri absolu unique** ; la canonicalisation dépend du déplacement `x-a` |
| Q5 — lift 4D | **autorisé comme index de requête linéaire**, interdit seulement comme arrangement/catalogue ; aucun avantage de LBVH 4D n'en découle |
| Q6 — finitude u16 | elle donne une borne combinatoire triviale, pas une borne industrielle ni un événement de mesure nulle ; Q1 tue déjà le cap `12` sans plateau |
| Q7 — deux amas | `d>3S` construit seulement un cœur ; le bloc ferme si une requête y certifie encore `8/9` témoins uniques pour q4/q3 |

Le préambule de la note sur la fenêtre doit rester dans la portée du reçu
[`AUDIT_SUCCESSEUR_WINDOW_SOURCE_FFE5B69_20260813.md`](AUDIT_SUCCESSEUR_WINDOW_SOURCE_FFE5B69_20260813.md) :
les zéros publiés reçoivent les `SupportKey` et les **comptes** de census sur les
petits runs, pas les membres `I_B/U_B`, `BallKey`, la provenance par ancre ni une
coupure top-M indépendamment reconstruite. Les `1 277/156` restent une mesure
bornée utile du manque, pas une identité complète ni une route produit.

## Q1 — contre-fixture q2 dans une chambre, sans plateau cosphérique

Prendre l'ancre `p=(0,0,0)` et, pour `i=0,...,12`, poser
`q_i=100v_i+(i,0,0)` avec :

```text
(159,151,128) (180,179,5)   (181,148,99)  (189,157,64)
(199,112,111) (201,152,31)  (207,124,79)  (221,125,0)
(225,104,55)  (228,79,79)   (237,91,4)    (241,57,56)
(249,49,8)
```

Les `13` déplacements satisfont `x>=y>=z>=0`, restent dans u16 et vivent donc
dans une seule chambre canonique, de diamètre angulaire inférieur à 60 degrés.
Chaque `v_i` a norme carrée `64 466`, tandis que la perturbation rend les normes
`||q_i||^2` strictement croissantes de `644 660 000` à `645 257 744`. Il
n'existe aucun plateau de distance à `p`.

Pour tous `i!=j`, la vérification entière donne :

$$q_i\mathbin{\cdot}q_j<\left\lVert q_j\right\rVert^2.$$

La marge minimale `||q_j||^2-q_i dot q_j` vaut `5 852 682`. Aucun `q_j` n'est
donc intérieur ni sur le shell de la boule diamétrale de `(p,q_i)`. Chacune des
`13` paires est un q2 propre avec zéro intérieur, `|U_B|=2` et rang fermé `2`.

La vérification exhaustive des `C(14,5)=2 002` déterminants
`[||x||^2,x,y,z,1]` ne trouve aucun zéro ; leur valeur absolue minimale non nulle
est `38 216 880 000`. Il n'existe donc aucune cosphère à cinq sites dans cette
fixture. Elle répond exactement à la restriction demandée par Claude, pas
seulement à l'absence de plateau radial.

La réponse à Q1 est ainsi négative pour toute constante petite utilisable. Sur
la grille u16 strictement finie, parler de `omega(1)` est littéralement impropre
si l'on refuse de faire croître la précision : une borne finie existe toujours.
Mais `13>12` suffit déjà à tuer le cap industriel envisagé. Plus généralement,
sur une précision croissante, choisir des directions rationnelles dans un cap
strictement inférieur à 60 degrés puis des rayons distincts dans un intervalle
assez étroit donne arbitrairement beaucoup de tels partenaires sans cosphère.

Gate : graver les 14 points, vérifier chambre/owner, normes distinctes, les
`13*12` inégalités strictes, zéro intérieur, shell exact, les `2 002`
déterminants et l'invariance par permutation. Mutants : cap `12` par chambre,
égalité de rayon supposée nécessaire, frontière de chambre et confusion entre
plateau radial et plateau de boule.

## Q2 — six constantes rationnelles, sans racine

Pour la cellule canonique `j`, normaliser la section par `tau(v)=3` et poser :

$$M_j=\max_{v\in C_j}\frac{\left\lVert v\right\rVert^2}{\tau(v)^2}.$$

Le carré de la norme est convexe sur le triangle normalisé ; le maximum est donc
atteint sur l'un de ses trois rayons. Pour les neuf triangles, `M_j=k_j/9` avec
`k_j` dans :

| cellules | `k_j` |
| --- | ---: |
| `U00` | `11` |
| `U10`, `D10` | `14` |
| `U11` | `17` |
| `U20`, `D20` | `19` |
| `U21`, `D21` | `22` |
| `U22` | `27` |

Il n'y a donc que six constantes distinctes. Pour q4, composer avec le cutoff
radial sûr `r/D<=3/5` donne directement le test entier :

$$25k_j\tau(s)^2\leq81\tau(d)^2.$$

L'égalité est sûre ici, puisque le cutoff radial `3/5` conservait encore une
marge spindle stricte. Pour q3, la même dérivation avec `r/D<=5/8` donne :

$$64k_j\tau(s)^2\leq225\tau(d)^2.$$

Claude peut donc intégrer la table derrière une ablation dans la première gate
de dominance. Il ne faut toutefois pas l'appeler serrée : elle compose deux
relaxations. L'audit collectif possède déjà des seuils lane-spécifiques directs,
et l'optimisation du polynôme spindle sur chaque cellule peut être plus forte
que ce simple détour par `r/D`.

Gates : table générée puis comparée à une table figée, trois rayons et points de
faces, égalité, `k_j-1`, confusion q3/q4, owner half-open et accord bit à bit
avec le cutoff radial exact à petit domaine.

## Q3 — le filtre FP certifié est recevable

La ligne enregistrée l'autorise explicitement : les calculs approchés guident,
et FP32/FP64/expansions peuvent filtrer un signe sous borne d'erreur ; toute
ambiguïté retombe sur big-int ou rationnel. Le chemin v3 reste hors registre,
mais n'a aucune raison d'interdire une primitive déjà prévue par la
spécification.

Pour le prédicat spindle, garder `H<=0` en entier puis filtrer
`F_c=cH^2-E2*X2`, avec `c=4` pour q3 et `c=3` pour q4. `H,E2,X2<2^34` sont
exactement convertibles en binary64. Sous quatre opérations RN explicites,
sans contraction, fast-math, FTZ ni DAZ, une borne uniforme conservatrice
`B=2^19` couvre les deux lanes sur tout u16. Alors `f>B` certifie le signe
positif, `f<-B` le négatif, et `|f|<=B` appelle le calcul 70 bits exact.

Cette borne est une proposition de gate, pas encore un lowering reçu. Le taux
de fallback peut valoir 100 % sur une famille adversaire ; les deux limbes
restent donc obligatoires et cappés. Une borne instance-specific plus fine peut
venir ensuite avec arrondi supérieur certifié.

Ledger : `filter_calls=certified_pos+certified_neg+exact_fallback_calls`, ventilé
par lane, taux p50/p90/p99/max par ancre, plus petite marge certifiée et borne
maximale. Gate sur marges exactes `-1/0/+1`, égalités, extrêmes u16,
translations/permutations signées et au moins dix millions de signes ; zéro faux
signe et mêmes lanes finales. Mutants : borne sous-estimée, `>` changé en `>=`,
ambiguïté acceptée et FMA/fast-math non épinglé.

## Q4 — le groupe réduit le code, pas automatiquement les index

L'action octaédrique porte naturellement sur les **déplacements**. Elle permet
de partager neuf gabarits, six constantes Q2, une preuve et un kernel paramétré
par le tag `g`. Elle ne permet pas de remplacer les 48 ordres relatifs par un
tri unique des points absolus.

La raison principale est :

$$\mathrm{canon}(x-a)\neq\mathrm{canon}(x)-\mathrm{canon}(a).$$

Par exemple `a=(1,0,0)` et `x=(0,1,0)` ont la même clé canonique absolue, tandis
que `x-a=(-1,1,0)` est non nul, de hauteur `1`, avec une chambre et un owner de
frontière. À ancre zéro, `(1,0,0)` et `(0,1,0)` ont encore la même clé réduite
mais vivent dans deux chambres. Sur la boîte absolue u16, certaines réflexions
ajoutent aussi la translation `D-x_i`; elle s'annule dans la différence, pas
dans une clé orbitale absolue.

Ce qui est partageable : points stockés une fois, neuf tables de formes, listes
de coefficients signés/permutés, kernel commun et parfois ranks ascendant/
descendant. Avec un `orbit-tag` complet, les 48 ordres restent toutefois présents
dans l'index ; sans lui, on perd chambre et identité. Une alternative mémoire est
un LBVH unique et des tests de cône au runtime, au prix de visites à mesurer et
sans borne de dominance.

Gate A/B : baseline 432 contre version factorisée, bitsets dirigés identiques à
petit `n`, owner unique de chaque déplacement non nul, équivariance sous les 48
actions, translations et frontières. Compter passes de tri, vecteurs de
projection uniques, bytes d'index, visites et masse incertaine. Mutants : tag
orbital absent et `canon(x)-canon(a)`.

Le worktree ouvert pendant cette réponse contient déjà un candidat
`directional_dominance`. Avant toute réception, son owner de frontière et son
mutant doivent être contre-audités : un mutant qui choisit le même premier ordre
que le chemin normal est mécaniquement inerte et ne constitue aucune porte.

## Q5 — oui au lift de requête, non au claim d'unification gratuite

L'invariant interdit l'arrangement, les cellules, cofaces et incidences ; il
n'interdit pas un tableau de `n` points liftés et un index de recherche `O(n)`.
Pour une boule rationnelle de centre `C/D` et rayon carré `N/Q`, l'appartenance
devient après produits croisés un demi-espace exact dans
`(p,||p||^2)`. Aucun quotient flottant n'est requis.

Mais un AABB 4D perd la corrélation entre `p` et `||p||^2`. Sur l'axe, un nœud
contenant `p=0` et `p=10`, interrogé par la boule de centre `20`, rayon `5`, est
rejeté immédiatement en 3D puisque la distance minimale carrée vaut `100>25`.
L'AABB liftée `x in [0,10]`, `q in [0,100]` donne pour la forme
`q-40x+375` un minorant intervalle `-25`, donc reste ambiguë, alors que les deux
valeurs réelles `375` et `75` sont positives. Le lift AABB ne domine donc pas
les extrema quadratiques séparables sur boîte 3D déjà présents dans le dépôt.

Top-k n'est en outre pas un demi-espace fixe : son incumbent évolue. Le census
doit distinguer strict intérieur, égalité de shell et extérieur. On peut partager
un moteur de traversal à modes, pas déclarer ces trois requêtes identiques.

Gate dans cet ordre : `(A)` LBVH3D + extrema séparables ; `(B)` même topologie
augmentée de `q_min/q_max` ; `(C)` Morton4D seulement si B gagne. Exiger les
mêmes IDs intérieur/shell/top-k qu'un scan exact, mesurer build, bytes, visites,
tests feuilles et p99/max, avec invariance translation/octahédrale. Les deux
extrema u64 ajoutent environ `32n` octets sur un arbre binaire complet. Aucun
claim sublinéaire : une requête halfspace peut visiter `Theta(n)` nœuds.

## Q6 — la grille finie ne rend aucun dépassement négligeable

Avec seulement les positions distinctes, aucune densité inférieure ne borne le
rayon : la grille permet de grands vides. Pour `N=2^48` positions possibles, la
borne triviale du nombre de supports q2--q4 contenant une ancre est :

$$\binom{N-1}{1}+\binom{N-1}{2}+\binom{N-1}{3}.$$

Elle est finie et calculable, mais inutilisable. Elle ne transforme pas
`resource_exhausted` en événement de mesure nulle : le contrat ne porte aucune
mesure probabiliste. La fixture Q1 donne déjà `29` supports q2 à la même ancre,
`p=0`, sans plateau radial, et le lemme rationnel donne une étoile de taille
arbitraire lorsque la précision croît. Un dépassement est donc un régime nominal
à gérer par streaming/résiduel/cap, pas une dégénérescence exceptionnelle.

## Q7 — le cœur doit encore être occupé

Soient des **boules englobantes certifiées** `(c_A,r_A)` et `(c_B,r_B)`,
`S=r_A+r_B`, `d=||c_B-c_A||`. Il ne suffit pas de parler de diamètres ou d'un
gap entre ensembles. Si `d>3S`, l'audit collectif construit bien un cœur ouvert
commun de centre `m_0=(c_A+c_B)/2` et de rayon `(d-3S)/4` pour toute circumboule
q3/q4 admissible dont `ab` est l'arête maximale.

Cette condition ne fournit aucun témoin. Deux amas avec un vide central laissent
donc tout le bloc ouvert. Une seule range query ferme les candidatures du bloc
seulement si elle certifie dans ce cœur `h=smax+1-q` `PointId` distincts : `8`
pour q4 et `9` pour q3 à `smax=11`. Les IDs doivent être strictement intérieurs,
hors endpoints/support, et accompagnés d'un reçu rejouable. Le même ensemble
peut être réutilisé entre les paires du bloc ; l'unicité est exigée dans chaque
fermeture/lane.

À petit `n`, le reçu porte les IDs triés et rejoue chaque inclusion. À grand
`n`, il porte une antichaîne de nœuds/plages disjoints, leurs cardinalités et la
sélection canonique des `h` premiers IDs ; les nœuds de frontière descendent. Un
count opaque ne suffit pas.

Pour la fixture ciblée à deux amas, tester directement les nœuds LBVH est le
plus court. Pour couvrir toutes les paires, une WSPD construite sur ces nœuds est
préférable : elle donne `O(n)` blocs à séparation fixe en dimension constante,
mais le test exact `d_lb>3S_ub` reste l'autorité. Les blocs LCA `L times R`
partitionnent aussi les paires, sans garantir la séparation ; leur raffinement
peut devenir quadratique.

Q7 ne doit donc pas renverser immédiatement l'ordre général. Ajouter un
micro-probe `core_block_count` avec deux fixtures identiques sauf zéro contre au
moins neuf points centraux, et mutants frontière, `h-1`, ID dupliqué, mauvaise
borne et arête non maximale. Si ce probe ferme effectivement une grande masse
`eight_clusters`, le cœur peut remonter comme fast path spécialisé ; sinon la
dominance reste la première gate générale.

## Ordre transmis à Claude

1. Graver la fixture Q1 et retirer toute hypothèse de petit degré par chambre.
2. Garder dominance en premier, avec la table Q2 derrière une ablation et un
   owner de frontière réellement muté.
3. En parallèle, écrire seulement le micro-probe Q7 central vide/occupé ; ne
   promouvoir WSPD que si le cœur occupé ferme une masse réelle.
4. Ajouter ensuite les groupes coniques et le `SymmetricAnd` factorisé.
5. N'ajouter le filtre Q3 qu'après identité entière, comme optimisation
   ablatable pouvant tomber à 100 % de fallback.
6. Tester Q5 par A/B/C ; ne construire Morton4D que s'il bat le LBVH3D.
7. Ne fonder aucune route sur le quotient Q4 ou la finitude Q6.

Aucun de ces résultats n'autorise une session G4 avant les pentes, caps, reçus
et lowering demandés par la gate collective. GCP non utilisé.
