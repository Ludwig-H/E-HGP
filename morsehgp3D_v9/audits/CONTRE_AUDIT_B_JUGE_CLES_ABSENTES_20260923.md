# Contrelecture B — premier juge des clés jamais émises

23 septembre 2026. Code `683fa46e`,
[`chain_absent_keys_gate.cpp`](../tests/chain/chain_absent_keys_gate.cpp).
Lecture du code et du journal CTest local du développeur ; pas de
nouveau test lourd ni GCP par B. Cadre : diagnostic CPU hors registre,
pas qualification de complétude ou de temps de la tour.

## Avancée réelle

Le juge ne part plus seulement des clés que le générateur a sorties.
Il choisit des supports de 2 à 4 sites, calcule leur miniboule exacte,
recense intérieur/coquille sur un index reconstruit et exige la présence
de toute clé candidate **admissible** dans le catalogue. C'est un test
unilatéral utile : une clé admissible trouvée absente démontre un défaut
du générateur (sous la correction des primitives de l'oracle). Les
retraits plantés exercent ce chemin. Il ne change pas le statut produit
`complete_relative` et ne tourne pas dans la chaîne chronométrée.

Le journal local non versionné
`build/v9-open-worktree/build/v9-exp/Testing/Temporary/LastTest.log`
rapporte, sur `uniform`, `terrain`, `clusters` à K5 et K10 :

| n | présentations candidates | présentations admissibles, présentes | occurrences de manques plantés | durée du test local |
| ---: | ---: | ---: | ---: | ---: |
| 2 000 | 225 000 | 77 051 | 625 | 21,30 s |
| 8 000 | 225 000 | 77 177 | 572 | 152,48 s |

Ces nombres sont des **appels sur supports** : ni `admissible` ni les
625/572 détections ne dédupliquent les `BallKey`. Ils ne désignent donc
pas 77 000 boules distinctes ni 572 clés manquantes distinctes. Le
journal indique 4 529/20 745 clés retirées des catalogues K5 à 2k/8k ;
ce sont des essais de non-vacuité, pas des omissions naturelles.
La capture n'est pas un reçu autonome épinglé ; le code est versionné,
les chiffres ci-dessus sont une lecture du journal local.

## Couverture exacte de l'échantillon

À ces deux tailles, `stride=n/500` fixe **500 ancres** par nuage.
Chaque ancre propose 10 supports q2, 45 q3 et seulement 20 q4 : les
q4 choisissent trois sites parmi les **six** premiers voisins, non
parmi les dix. Soit 75 présentations par ancre et
`500×75×3 familles×2 K=225 000` par construction, même quand n
quadruple. Ce nombre constant ne mesure aucune pente de génération
ni ne couvre les supports longs. Les familles sont synthétiques et
déterministes ; aucun fichier SemanticKITTI, avec ou sans sol, aucune
trame brute entière, autre séquence, s10/s12 ou GPU ne passe cette porte.
La chaîne de référence est lancée avec s8, deux workers et **sans FULL**.
Les recettes de `front_fixtures.hpp` restent dans le domaine de
coordonnées u16 (masque 65535, coins ≤51023) : elles n'exercent pas
le haut du domaine entier u18, malgré le type numérique v9.

L'indépendance est celle du **choix des candidats** vis-à-vis de la
WSPD et du générateur. L'oracle réutilise `anchor_meb`, `ball_census`,
`ShellTable`, `BallKey` et l'index du produit ; un défaut commun dans
une de ces primitives peut donc survivre. `ball_census` rend soit
`kInteriorOverflow` (profondeur excessive), soit `kShellOverflow` ;
le juge saute actuellement les deux sans les compter séparément.
Une coquille >12 n'est pas synonyme d'une boule hors fenêtre : elle
peut relever d'un refus explicite du domaine produit. Le commentaire
« outside the window » est trop large.

Le plancher de la porte est seulement `all.admissible≥1000` **au total**
et `planted_found>0` **au total**, sans obligation par famille, ordre,
arité, profondeur p ou longueur de support. Enlever tous les 60 000
appels q4 possibles à 8k laisserait encore au moins 17 177
présentations admissibles avant autres effets, au-dessus du plancher ;
la non-vacuité pourrait encore être portée par q2/q3. Le code ne
certifie donc pas qu'il exerce les q4, ni les strates de fin de fenêtre
q2 `p=Kmax−1` et q3 `p=Kmax−2`. La proximité est un biais connu,
particulièrement pour les ancres longues du LiDAR.

## Prochaine porte utile

Conserver ce juge, mais compter **clés uniques**, statuts de census
séparés et couverture par `(q,p,diamètre, famille, K)`, avec des
planchers non vacants par strate visée. Planter au moins une clé connue
de fin de fenêtre q2/q3 et une q4, puis exiger leur découverte, pas
seulement un nombre global de manques. Ajouter un tirage déterministe
de supports longs et les trames SemanticKITTI entières 1 mm de
plusieurs séquences, brutes puis sans sol, hors temps de contrat.
Cette porte restera échantillonnée ; une preuve ou un oracle plus large
reste requis avant d'élever `complete_relative` en exactitude globale.
