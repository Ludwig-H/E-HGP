# MEB locale : proposition, certificat régulier et ordinal de référence

5 septembre 2026, audit de conception indépendant, sans modification moteur.
**Proposition non intégrée** : le delta E désigne uniquement la
précontenance q2, pas ce prototype par pivots. La variante à deux budgets
du § 6.2 est une solution future, non implémentée ni qualifiée.

La [contre-fixture archivée](../receipts/meb_pivot_budget_counterexample_20260905/README.md)
conserve le test négatif exécuté au § 9, ses sources historiques et ses
sorties complètes. Ce résultat ne promeut pas le prototype.

```text
phase=exploration_v7_hors_registre
backend=cpu_reference
profile=quantized_u16_input_only
mode=audit_independant_math_and_architecture
public_status=not_claimed
```

Verdict : **le raccourci est justifié localement** si son certificat exact
établit un support strictement positif, la contenance de tous les sites
et une coquille locale composée exactement de ce support. Le support est
alors l'unique candidat que la référence D peut accepter. Son ordinal se
calcule sans réénumération, ce qui conserve les compteurs logiques et les
refus de D, y compris au milieu de l'énumération. Un certificat incomplet,
échoué ou non régulier impose le repli D inchangé, pas un verdict nouveau.

**Verrou distinct : le plafond actuel porte sur les supports essayés.**
Le prototype ordinal seul ne conserve pas cette autorité physique. La
preuve d'équivalence des valeurs publiques ne l'autorise donc pas à entrer
tel quel dans le moteur. Le § 6.1 donne une contre-fixture, et le § 6.2
décrit une solution future avec un deuxième budget prospectif explicite.

Cette conclusion ne dépend pas de l'algorithme de proposition. La première
variante recommandée par le constructeur emploie des pivots entiers bornés,
ce qui n'ajoute aucun contrat flottant. Aucun prototype moteur ou benchmark
n'a été produit ici. À la demande du constructeur, une contre-fixture C++
isolée a ensuite été compilée et exécutée ; son autorité bornée est au § 9.

## 1. Référence, domaine et état observé

La référence D est `silent_detail::Builder::miniball`, header SHA256
`5214a9a7f2b6f53b1c59c803d414e109c9a660f15ab9448d88aec90300160c71`,
au HEAD `e6d33698e62ebecf74dff01c16d7de17149d7a4e`.

Le domaine est celui des appels admis : entre 2 et 11 sites distincts,
indices valides d'un `CloudIndex` valide, coordonnées dans [0,65535],
primitives non mutées et formes construites depuis ces mêmes positions.
Les appels productifs satisfont n>=2 car K=1 est traité avant la construction
des facettes. La méthode locale publique en C++ ne valide pas elle-même
un tableau arbitraire ; cette note n'étend pas son domaine.

D examine toutes les paires, puis tous les triplets, puis tous les
quadruplets, chacun dans l'ordre lexicographique des **positions dans
`sites[]`**. Ces positions ne sont pas les valeurs d'indices Morton ni
nécessairement les PointId triés pour un appel local de test.
Chaque candidat est chargé avant son examen, même s'il est ensuite
colinéaire, obtus, coplanaire, non positif ou rejeté pour non-contenance.

`accept` n'écrit `LocalBall` qu'après contenance de tous les sites. Puis D
s'arrête au premier accepté et compte sa coquille. Une coquille trop grande
rend `silent_local_nonessential_shell` **en conservant la boule déjà écrite**.
Un refus budget avant accept laisse au contraire la boule initiale intacte.
Le nombre de MEB est incrémenté une seule fois à l'entrée, refus compris.
Un succès ne réinitialise pas implicitement les autres statistiques, les
événements, le statut ou la raison fournis à la méthode.

## 2. Certificat suffisant et preuve de l'unicité du support

Soit P l'ensemble local. La proposition ne porte que des indices de sites,
jamais l'autorité d'un centre, rayon ou niveau approché. On canonise les
indices de support dans l'ordre croissant de leurs positions dans `sites[]`.
Le certificat exige :

- 2<=q<=4, positions distinctes et toutes dans [0,n) ;
- q2 : deux positions distinctes, boule diamétrale exacte ;
- q3 : les trois produits scalaires d'acuité strictement positifs puis G>0,
  exactement comme D ;
- q4 : déterminant non nul et `q4_center_strictly_inside` vrai,
  exactement comme D ;
- puissance exacte <=0 sur chaque point de P ;
- puissance exacte nulle sur les q sites proposés et strictement négative
  sur tous les autres points de P.

Le dernier test peut se coder par un compteur de coquille égal à q,
à condition que les q incidences du support soient effectivement garanties
par les formes certifiées. Un tableau de membership ou un contrôle explicite
de chaque site du support rend cette prémisse facile à tester.

Les gardes q2/q3/q4 fournissent un simplexe affinement indépendant S, dont
le centre c est une combinaison barycentrique strictement positive des
sommets. Ils sont sur une même sphère de rayon R, contenant P. Pour un autre
centre y, l'identité de variance donne :

$\sum_{s\in S}\lambda_s\lVert s-y\rVert^2=R^2+\lVert c-y\rVert^2$, avec $\lambda_s>0$ et $\sum_s\lambda_s=1$.

Toute boule contenant S a donc un rayon au moins R. En cas d'égalité,
son centre est c. La boule certifiée est ainsi l'unique MEB de S et de P.
Cette preuve établit l'optimalité, pas seulement la contenance.

Soit T un autre support candidat que D pourrait accepter. Les mêmes
gardes de positivité impliquent que sa boule est également l'unique MEB
de P. Elle est donc la même boule. Tous les sites de T sont dans la coquille
locale, qui vaut exactement S : T est inclus dans S. Comme S est affinement
indépendant, les coordonnées barycentriques de c sur S sont uniques ;
elles sont toutes strictement positives. c ne peut donc appartenir à
l'enveloppe convexe d'un sous-ensemble propre T de S. Par suite T=S.

Il n'existe donc **aucun accepté antérieur** à S dans D, quelle que soit
l'arité et l'ordre des autres candidats. D atteint exactement S s'il a
le budget nécessaire, puis réussit son contrôle de coquille. Le certificat
n'a pas besoin de refaire les prédicats des candidats sautés.

## 3. Ordinal fermé et calcul sûr

Écrire les positions canoniques $0\leq h_0<\cdots<h_{q-1}<n$ et convenir
que $\binom{a}{b}=0$ lorsque b>a pour les arguments non négatifs utilisés.
L'ordinal R, **indexé à partir de un**, dans l'énumération complète D est :

$R=\sum_{p=2}^{q}\binom{n}{p}-\sum_{t=0}^{q-1}\binom{n-1-h_t}{q-t}$.

Preuve : tous les supports d'arité inférieure contribuent d'abord. À arité
q, compter les combinaisons ayant leur première différence lexicographique
en position t donne la somme des blocs dont l'élément à cette position
précède h_t. L'identité de Pascal, sommée sur ces blocs, donne le rang
lexicographique à partir de zéro :

$\binom{n}{q}-1-\sum_{t=0}^{q-1}\binom{n-1-h_t}{q-t}$.

L'ajout d'un et des arités précédentes produit R. Tous les candidats sont
comptés, pas uniquement les supports géométriquement admissibles.

Une table entière constexpr C[n][k], n<=11 et k<=4, calculée par Pascal,
évite division, multiplication large et arrondi. Son maximum est 330.
La somme des trois arités vaut au plus 550. Les arguments n-1-h_t et q-t
sont sûrs **après** validation des indices. Accumuler séparément les deux
sommes puis soustraire évite des soustractions unsigned intermédiaires
non justifiées. Vérifier ou tester 1<=R<=550 et la monotonie du rang.

Exemples n=11 : première paire R=1, dernière paire R=55, premier triplet
R=56, dernier triplet R=220, premier quadruplet R=221, dernier quadruplet
R=550. Ces frontières exercent les offsets d'arité.

Un contrôle indépendant en JavaScript entier, exécuté pour cette lecture,
a comparé les **1 507 supports** de tous les n de 2 à 11 à une énumération
lexicographique directe ; tous les ordinaux concordent. Ce contrôle ne
teste ni un futur helper C++ ni le moteur et n'en constitue aucun reçu.

## 4. Plafond prospectif, priorité du refus et commit de la boule

Soient c la valeur initiale de `meb_supports` et L son plafond. Le nombre
R est l'ordinal local, pas un compteur global. Le transfert équivalent à D
est le suivant, sans calcul potentiellement débordant c+R avant le garde :

```text
si c >= L :
  refuser silent_meb_support_budget ; compteur c inchangé ; boule inchangée
sinon si R > L - c :
  mettre compteur à L ; refuser silent_meb_support_budget ; boule inchangée
sinon :
  mettre compteur à c + R ; seulement ensuite écrire la boule certifiée
  rendre le même succès que D
```

Si R>L-c, D consomme exactement L-c candidats sans en accepter aucun, puis
refuse au candidat suivant. Puisqu'il précède ou égale S, la boule initiale
est restée intacte. Si R=L-c, la charge de S réussit et D termine : utiliser
une comparaison >= à cet endroit serait faux. Si c>L dans un appel local
artificiel, D refuse sans ramener c à L : préserver aussi ce cas.

L'addition autorisée ne dépasse pas L<=UINT64_MAX. Aucun cast signé ne
sert à calculer le budget restant. Les cas c=UINT64_MAX, L=UINT64_MAX,
c=L-1 et les caps nuls doivent être exercés. `meb_calls` doit suivre
exactement l'incrément D une seule fois ; le repli ne doit pas en ajouter
un second. Les compteurs publics non concernés restent inchangés.

Un contrôle indépendant JS utilisant BigInt a comparé **839 245 transitions**
à une simulation répétant la charge D, dont des états à proximité de
UINT64_MAX et c>L : booléen et valeur finale concordent. Ceci reste un
contrôle de la formule, non une qualification de l'implémentation C++.

La proposition et le certificat doivent utiliser des temporaires et ne
modifier ni le `LocalBall` appelant ni les statistiques ni statut/raison.
Sur certificat manqué, le repli D repart du même état. Sur certificat
valide mais budget insuffisant, ne jamais exposer la boule calculée.
Le cap déjà épuisé peut être testé avant toute proposition.

**Sens des compteurs.** Après optimisation, `meb_supports` sera le coût
logique de référence D, conservé pour compatibilité des caps et reçus,
pas le nombre réel de supports dont les prédicats auront été exécutés.
La proposition et sa certification font un travail nouveau borné, qui
n'est pas un nombre fictif de tests géométriques à ajouter à ce compteur.
Documenter cette distinction ; mesurer séparément les pivots, propositions,
certifications, replis et candidats effectivement testés. Aucun gain ne
se déduit de l'égalité numérique du compteur historique de 802 millions.

## 5. Pourquoi la coquille complète est impérative : contre-fixture

Prendre, dans cet ordre local : a=(0,0,0), b=(2,0,0), c=(2,2,0), d=(0,2,0).
Les deux diagonales ac et bd sont des supports q2 positifs dont la boule
contient les quatre points. Le shell local contient les quatre sommets.

D rejette ab, accepte ac à l'ordinal 2, écrit sa boule et son support
`{sites[0], sites[2], 0, 0}`, puis rend
`kUnsupportedDegeneracy / silent_local_nonessential_shell`.
Une proposition bd sans garde de coquille aurait l'ordinal 5. Elle peut
changer à tort le support, promouvoir un succès, ou, si elle garde le
contrôle final mais décompte l'ordinal 5, rendre un refus budget au cap 2
alors que D rend le refus scientifique avec une boule déjà renseignée.

C'est une contre-fixture minimale à la prétendue unicité d'un support
positif contenant sans shell régulier : avec deux ou trois positions
distinctes il n'existe pas deux bases positives différentes de ce type.
Au cap 0/1, D refuse le budget avant toute boule ; au cap >=2, il rend
le refus scientifique décrit. Cette fixture doit devenir permanente si
le raccourci est implémenté. Elle figure déjà comme scène de carré dans
le corpus MEB D ; la nouvelle porte doit injecter explicitement bd pour
exercer l'autorité du certificat et la priorité de refus.

Les triplets rectangles, supports coplanaires, centre q4 sur une face,
extra-shells, propositions partielles ou invalides ne reçoivent pas une
heuristique de réparation de la décision : ils vont au repli D. Ce repli
préserve aussi la sentinelle de refus, le premier support accepté et
le niveau exact littéral des cas dégénérés.

## 6. Proposition par pivots entiers bornés

La variante soumise par le constructeur utilise une paire initiale
distincte, maintient un support positif Q de taille au plus quatre avec
sa forme entière brute, puis choisit un point local strictement extérieur.
Elle résout la MEB de T=Q union {z}, donc au plus cinq sites, par les mêmes
tests stricts q2/q3/q4 et contenance exacte ; il existe au plus 25 candidats
(10 paires, 10 triplets, 5 quadruplets). Elle répète au plus 16 pivots,
puis impose le repli D si elle n'a pas obtenu le certificat final.

Pour un pivot réussi, la nouvelle boule contient l'ancienne base Q et
l'intrus z. Son rayon est strictement supérieur à l'ancien : un rayon
inférieur est impossible par optimalité de l'ancienne MEB de Q ; à rayon
égal, l'unicité démontrée au § 2 imposerait l'ancienne boule, qui ne contient
pas z. La nouvelle base positive constitue donc un état valable pour le
pivot suivant. Il n'est pas nécessaire de comparer des niveaux construits
dans le code de proposition pour exploiter cette progression.

On peut tolérer une coquille non essentielle sur T dans la **proposition** :
le support positif choisi suffit à conserver son rôle de base de MEB.
Rejeter ces coquilles intermédiaires est également sûr, mais ajoute des
replis ; il ne faut pas confondre cette option avec le certificat final,
qui exige toujours la coquille régulière sur tous les sites de P.

Si tous les points sont contenus mais que la coquille finale est trop
grande, replier D. Si le solveur de cinq sites ne produit rien, refuse son
propre budget interne, rencontre une entrée non prise en charge ou atteint
16 pivots, replier D. Aucun de ces événements de proposition ne devient
directement un statut public. Un échec interne doit être distingué d'un
échec de certification dans les observations de test.

La limite 16 est un paramètre de coût, pas un théorème de convergence en
16 étapes. Toutes les formes proviennent de positions u16, donc les bornes
existantes q2/q3/q4 restent applicables. Un éventuel ordre de proposition
différent est sans autorité sur les sorties : après succès, reconstruire
avec les positions canoniques du support de P. La proposition n'ajoute
aucune structure globale, Γ exhaustif ou mosaïque de Delaunay. Son état
est de taille fixe sur <=11 sites et elle n'a besoin ni de tas ni d'oracle
global. Son surcoût peut régresser sur les cas où D accepte la première
paire ; mesurer ce cas séparément est obligatoire.

### 6.1. Revue du prototype entier et contre-fixture de budget physique

Le constructeur a soumis `build/v7_meb_pivot_prototype/pivot.hpp`, SHA256
`d6dbba195eb17d7ae8f765b8295a374ccd43e39f88371afef86b03c3779b8ec5`.
Ce fichier a été relu entièrement avant la contre-fixture exécutée au § 9.
Sur le domaine interne fermé q2/q3/q4 et n2..11 :

- `form` trie les positions du support et construit les bons prédicats ;
- `small_ball` n'écrit le candidat qu'après contenance sur Q union {z} ;
- `propose` examine la coquille sur tous les sites quand aucun extérieur
  n'a été trouvé, et borne explicitement le nombre de pivots ;
- `materialize` utilise les formes canoniques mises en cache, les mêmes
  constructions D et un `LocalBall` initialisé à zéro ;
- `miniball` laisse le résultat public intact avant certificat, préserve
  le compteur d'appels dans les deux voies et traite correctement la
  frontière logique de budget avec la boule sentinelle intacte.

Ces helpers ne constituent pas une API validant des structures Candidate
arbitraires : `power`, `form`, `ordinal` et `materialize` supposent arités
et indices internes déjà valides. Une future injection de candidats de test
doit passer par un validateur plutôt qu'appeler ces helpers hors domaine.

Le défaut du prototype est contractuel et concret. La documentation
`morsehgp3D_v7/docs/INCIDENCES_SILENCIEUSES.md`, § 4, dit que les plafonds
couvrent les « supports locaux essayés ». `RESIDENCE_MASSIVE.md` confirme
la charge prospective. Le nouveau `Work::candidates` n'est pourtant pas
borné prospectivement ; la forme initiale est même comptée après construction.
Le plafond ordinal legacy n'est appliqué qu'après la proposition terminée.

Contre-fixture minimale : P=((0,0,0),(2,2,0),(2,0,2)), dans cet ordre.
C'est un triangle équilatéral entier aigu, avec shell régulier de trois
points. D essaie les trois paires puis le triplet : R=4. Le proposeur
construit la paire initiale, puis `small_ball` réessaie les trois paires
et construit le triplet : **cinq candidats** contre quatre charges legacy.
Au plafond L=4, le prototype réussit malgré cinq candidats essayés ; au
plafond L=1, il essaie encore les cinq candidats avant de rendre le refus
logique que D rend après une seule paire. L'admission cap nul ne suffit pas.

Borner seulement chaque proposition par le budget legacy restant ne ferme
pas le contrat : après épuisement de cette proposition, le repli D peut
consommer à nouveau ce budget. Sans cache/réemploi ou autorité de budget
distincte, la somme dépasse le plafond annoncé. Conserver la proposition
en overlay et intégrer d'abord le delta q2, qui ne change pas le parcours,
est cohérent tant que ce contrat n'est pas effectivement implémenté.

### 6.2. Solution future : deuxième budget prospectif, version explicite

La variante suivante a été proposée par le constructeur après ce constat :
conserver `max_meb_supports` et `meb_supports` comme plafond et ordinal
logiques de D, et ajouter `max_meb_proposal_supports` avec un compteur
effectif `meb_proposal_supports`. Elle est cohérente sous ces obligations :

1. Le nouveau compteur est propre à la même tentative/ordre que le compteur
   legacy et persiste entre toutes les MEB du Builder. Il n'est jamais
   remis à zéro à chaque proposition ni lors d'un repli.
2. Chaque candidat essayé par la proposition, initial compris, reçoit
   une charge prospective réussie **avant** sa construction ou son test.
   Un candidat rejeté par l'acuité ou le rang consomme aussi sa charge.
3. Quand ce budget est épuisé, le proposeur s'arrête immédiatement et D
   reprend depuis l'état legacy initial. Cet épuisement n'est ni un succès,
   ni un refus scientifique, ni un nouveau refus public : c'est un repli.
4. Le compteur effectif de proposition reste conservé après repli. Les
   compteurs legacy, la boule et le statut n'ont pas été touchés par
   l'échec de proposition. Les charges futures de D restent prospectives.
5. Si le plafond legacy est déjà atteint, refuser avant la proposition,
   comme dans le prototype. Si le nouveau budget est déjà atteint,
   aller directement au repli, avant même la recherche de paire extrême.
6. Le plafond interne de 16 pivots et la limite locale <=11 restent présents.
   Des budgets globaux ne doivent pas devenir des boucles non bornées en
   supprimant ces gardes structurelles.

Noter A le nombre réel de candidats tentés par les replis D et P le
nombre réel de candidats tentés par les propositions. Les replis D
incrémentent le compteur legacy à chaque tentative ; une réussite rapide
ne fait qu'y ajouter des charges virtuelles positives. Ainsi A est au
plus la somme des charges legacy, tandis que P est exactement le nouveau
compteur prospectif. Pour une tentative partant de compteurs nuls :

$A+P\leq\text{meb\_supports}+\text{meb\_proposal\_supports}\leq L_{\mathrm{D}}+L_{\mathrm{P}}$.

Cette borne ferme un nombre de **candidats/formes tentés**, pas un nombre
d'instructions, une durée, les distances de sélection de la paire, les
prédicats de contenance ou une RAM. Ces opérations supplémentaires ont
leurs limites locales explicites (n<=11, q<=4, T<=5, pivots<=16). La
matérialisation finale clé/niveau d'un candidat accepté doit être décrite
comme la finalisation de ce candidat, non comme une forme spéculative
supplémentaire cachée. Si une implémentation reconstruit plusieurs formes
distinctes à la finalisation, sa comptabilité doit l'annoncer ou les charger.

Les plafonds par ordre de la tour ne sont pas un plafond commun K1..10 :
la borne de la tour est la somme des limites par ordre réellement demandé.
Pour des appels locaux démarrant avec des compteurs non nuls, raisonner
sur leurs incréments effectifs ; ne pas supposer que le travail historique
injecté dans un test correspond à une exécution mesurée.

La nouvelle règle doit avoir un identifiant documentaire/receipt explicite,
par exemple `meb_work_accounting=reference_ordinal_plus_proposal_v1`, avec
le nom final décidé par le constructeur. Les anciens reçus D restent
épinglés à leur comptabilité d'origine ; ils ne prouvent pas le nouveau
budget. Ajouter les nouveaux champs/caps dans le schéma, les CLI/API et
les reçus réellement consommateurs, sans changement silencieux sous une
seed ou une valeur de s. Les digests géométriques peuvent rester stables,
mais les reçus et leur interprétation ont une version nouvelle.

Les portes spécifiques doivent exercer P=0, P=1, P=4 et P=5 sur le triangle
ci-dessus, l'épuisement au milieu d'un `small_ball`, l'épuisement cumulé
au milieu d'une deuxième MEB et le repli sans double compteur. Elles
doivent vérifier avant chaque construction que la charge a eu lieu,
et réfuter le mutant « charge après construction ». Une comparaison
des seuls compteurs finaux ne verrait pas ce défaut prospectif.

Cette variante ne justifie pas le prototype actuel : il manque encore
le deuxième compteur prospectif, sa limite, les tests et les documents
consommateurs. Elle offre une voie compatible avec une autorité de coût
explicite, sans forcer une mutation silencieuse de l'ancien plafond.

## 7. Conservation des représentations et raccords compilés

Reconstituer la boule finale **avec les expressions D**, depuis le support
canonisé dans l'ordre de `sites[]` :

- q2 : `q2_ball_key` et `promote_level(q2_exact_level(...))` ;
- q3 : `q3_form`, `q3_ball_key`, `promote_level(q3_exact_level(...))` ;
- q4 : `q4_form`, `ball_key_reduce(q4_ball_form(...))`, `q4_level_raw`.

Ne pas synthétiser le niveau depuis une `BallKey` réduite : le niveau q4
est volontairement non réduit. L'égalité rationnelle seule ne démontre
pas l'identité des trois limbs du numérateur et du dénominateur. Le même
tuple canonique avec les mêmes helpers conserve littéralement ces champs,
les zéros du support inutilisé, et surtout `support[0]`, employé ensuite
pour retirer un sommet pendant la descente silencieuse.

Les permutations d'un même tétraèdre conservent algébriquement |det| et
R², donc aussi sa représentation brute ; cette identité n'autorise pas
à ignorer l'ordre contractuel du tableau de support ni à substituer une
autre base sur une coquille non régulière.

Avant intégration, obligations testables :

1. Tester le helper d'ordinal C++ sur les 1 507 combinaisons, offsets q2/q3/q4,
   et mutations ciblées d'offset, ordre, +1 et convention C(a,b)=0.
2. Tester le transfert budget avec un compteur initial non nul, tous les
   caps autour de R, c>=L et UINT64_MAX ; comparer la boule sentinelle et
   l'intégralité des statistiques, statut/raison et booléen à D.
3. Tester un candidat fourni par injection indépendamment du proposeur :
   chaque base certifiée q2/q3/q4, permutations de sites, indices invalides,
   arité invalide, doublons, mauvais support, contenance fausse, shell extra,
   centre non positif, déterminant nul, et la diagonale bd du carré.
4. Garder une référence D épinglée compilée séparément et comparer les
   représentations littérales sur tous les caps, chaque refus compris.
   Puis répéter sans MHGP7_TESTING et sous ASan/UBSan pour la même route.
5. Exercer de façon non vide : succès de proposition dans chaque arité,
   plusieurs pivots, repli par plafond interne, repli par shell, repli par
   proposition manquée, acceptation immédiate q2 et support ordinal 550.
6. Vérifier que le test n'injecte ni connaissance du support de référence
   ni ordre de recherche idéal dans le chemin mesuré. Ne pas désactiver
   le raccourci uniquement sous MHGP7_TESTING. Les compteurs privés de
   matérialisation D changent naturellement de sens ; les requalifier
   explicitement, sans faire passer une non-vacuité devenue vide.
7. Les mutants D `silent-meb-q3-reject-shell`, `silent-meb-q4-reject-shell`
   et eager doivent conserver une portée causale déclarée. Une fast route
   qui les contourne peut rendre les anciennes portes vacantes ; ne pas
   attribuer ces verts par héritage. Tester les mutations des gardes de
   certificat dans la vraie route produit et les replis dans D.
8. Rejouer Gamma borné indépendant, descente, archivage transactionnel,
   API et refus : l'équivalence locale ne vérifie pas à elle seule les
   mécanismes de purge et de publication de la composition.
9. Geler les octets C/D/nouveau CLI avant une paire mono n=8000, mêmes
   coordonnées, seed, K1..10, s, caps et digests. Rapporter compteurs D
   logiques et coûts réels de proposition séparément ; ne pas transférer
   un microbenchmark de MEB à la tour complète ou à G4.

Les théorèmes conditionnels S1 et les audits arithmétiques actuels ne
sont pas rouverts par ce raccourci. Celui-ci conserve leur séparation
entre proposition, décision exacte et publication. Les conditions
globales de complétude du catalogue direct, index/front, domaine compilé,
normalisation horizontale, verticale et produit industriel restent leurs
obligations distinctes. Aucun statut exact public n'est promu.

## 8. Sources relues et autorité de la note

Les SHA256 ci-dessous ont été relevés le 5 septembre 2026 à 06:03:45 UTC.
Les chemins sont relatifs à la racine du dépôt :

```text
5214a9a7f2b6f53b1c59c803d414e109c9a660f15ab9448d88aec90300160c71  morsehgp3D_v7/src/forest/silent_incidence.hpp
11049293b7ad2f7139e1699c11b2506e8af2c11a04f22dc951f33ba10406c52f  morsehgp3D_v7/src/lanes/q2.hpp
4155a1c39193b68c47504e247a36e1bbf28b2c9ecbeeb50d6285d974519563fe  morsehgp3D_v7/src/lanes/q3.hpp
58aac9bd57ac1a9b19ad156f6397941f67df1379e29215c50fcf268268491c4a  morsehgp3D_v7/src/lanes/q4.hpp
acd6641e0616c926f6ce8afb6e294ae9982dcf9c518fa807cc9cfd713da7f34c  morsehgp3D_v7/src/lanes/level.hpp
913e9e89ebf40b7a64d54ea0608e2a76e8ea85f0a61d5a6657a04cfeb537aeca  morsehgp3D_v7/src/core/types.hpp
ae1efbe7a415972176b4df6b90f9d24672348a99fa574f7387069471e1d8fff5  morsehgp3D_v7/audits/S1_COURANT.md
2092d49b2bcb63c393b22da190a06b64d629e84894b457adca285f47d16defbb  morsehgp3D_v7/audits/ARITHMETIQUE_LANES_COURANTE.md
```

La preuve est autonome par identité de variance et coordonnées
barycentriques ; aucune référence externe ni bibliothèque de MEB n'est
introduite. Les contrôles JS mentionnés sont des diagnostics de conception
à contenu explicite, pas des reçus produit reproductibles versionnés.
Les sources moteur, rapports de l'auditeur externe et Git n'ont pas été
modifiés. **GCP non utilisé.**

## 9. Contre-fixture compilée isolée : plafond physique confirmé

Sur demande explicite du constructeur, `counter_budget.cpp` inclut le
prototype entier épinglé au § 6.1, sans changer une source produit.
Il compare D et le prototype sur le triangle du même paragraphe, aux
plafonds 4 et 1, avec un `LocalBall` sentinelle non nul. Les terminaux
booléen/statut/raison, toutes les statistiques existantes, clés, supports
et niveaux littéraux sont identiques ; le nombre de candidats réellement
essayés par la proposition est pourtant cinq dans chaque cas.

Sortie brute, sans troncature :

```text
counter_budget_case limit=4 reference_supports=4 proposal_candidates=5 terminal_equal=1 sentinel_preserved=0 reference_ok=1 reason=complete_relative_to_supplied_regular_direct_catalogue
counter_budget_case limit=1 reference_supports=1 proposal_candidates=5 terminal_equal=1 sentinel_preserved=1 reference_ok=0 reason=silent_meb_support_budget
counter_budget=counterexample_confirmed cases=2 proposition_candidates=5 reference_ordinal=4 public_status=not_claimed
```

La compilation est C++20, -O0, -Wall -Wextra -Wpedantic -Werror, sans
MHGP7_TESTING ; elle dure 2,277 s et rend 0. L'exécution rend 0 en 0,004 s,
confirmant la contre-fixture, non la conformité du prototype. Il ne s'agit
pas d'une mesure de performance. Pas de sanitizer ni de promotion produit.

Le runner `run_counter_budget.py` crée exclusivement un nouveau dossier
`counter_budget_run_20260905`, refuse tout dossier préexistant, impose une
échéance cumulée de 30 s avec destruction ciblée du groupe en cas de délai,
conserve chaque argv/stdout/stderr/rc et les hashes avant/après du prototype,
du test, du runner et des headers produit. Les pins sont tous stables.
Le fichier de dépendances de compilation et le binaire sont également
hashés. Aucune commande Git mutante ou GCP n'est exécutée.

```text
03d8468d8ee6e5e10e9b167034e1e7f20361d3bfb754e32a7b06e32334ad1124  counter_budget.cpp
a26f1b5ca18be0406e5a2f9db7c25c0613f66b0a73fdc4c0477d81331d829ca0  run_counter_budget.py
4d3f5de9147ac2135d54975f9ab0943ee0e9d77547d9b81a9ec326cb13f2ac4d  counter_budget_run_20260905/receipt.json
876f7933970a74f2a0f0315c6c1bced7c592daa21ffde34222f5dfdb3abc26fb  counter_budget_run_20260905/execute.stdout
7ab9a69271570aea12d9b36dc75cb7a900afeac0bb9ecf1aaa305b5b25c6979a  counter_budget_run_20260905/counter_budget
```

Le code source et le reçu de cette contre-fixture sont désormais conservés
dans l'archive publique liée en tête de note, sans binaire. Leurs chemins
d'origine sous `build/` sont historiques : les copies ne sont pas un runner
directement exécutable depuis l'archive. Le README déclare les conditions
de reproduction. Aucun fichier produit, tests ou CMake n'a été modifié.
