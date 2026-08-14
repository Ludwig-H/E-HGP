# Contrat de source — énumérer les aigus, tester le rang

Date : 14 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Cette note inscrit un contrat de source dicté par l'utilisateur, **après audit**.
Elle ne certifie aucun logiciel. Les autorités restent
[`AUDIT_ETAT_COURANT.md`](AUDIT_ETAT_COURANT.md) et les contre-audits qu'elle
cite. Ce qu'elle ajoute : l'énoncé du contrat, ce qu'il a de juste et pourquoi,
les précisions de vocabulaire qui le rendent non ambigu, sa formulation exacte
en trois énumérateurs indépendants, et ce qu'il ne borne pas.

Cette note répond aussi définitivement à Q14 : **aucune structure de Delaunay
n'est autorisée**, ordre un inclus. La structure de proximité permise est une
partition WSPD/Callahan--Kosaraju et ses généralisations ternaires et
quaternaires, construites directement depuis l'index Morton. Aucune sortie,
fermeture, capacité ou preuve de complétude d'une arité ne source l'arité
suivante.

## 0. L'énoncé reçu

> Il suffit d'énumérer les paires `a,b` de q2 par WSPD dont la boule de diamètre
> `ab` contient moins de `s_max` points. Puis les triangles aigus en
> généralisant WSPD de telle sorte que la miniboule contienne moins de `s_max`
> points. Puis les tétraèdres aigus dont la miniboule = circumsphère contient
> moins de `s_max` points.

## 1. Ce que le contrat dit juste

**Il n'y a pas d'autre événement.** Pour `S` affinement indépendant,
`2 <= |S| <= 4`, de circumcentre `o = sum_s lambda_s s`, l'identité de variance
`sum_i lambda_i d(p_i,y)^2 = r^2 + d(o,y)^2` donne : si tous les `lambda_s > 0`,
alors `B(o,R)` est **l'unique** miniboule de `S` et `S` son support minimal
positif. C'est le lemme reçu de
[`AUDIT_MINIBOULE_UNIQUE_RESIDUEL_SHALLOW_5809BD2_20260814.md`](AUDIT_MINIBOULE_UNIQUE_RESIDUEL_SHALLOW_5809BD2_20260814.md).
Donc « miniboule = circumsphère » n'est pas une hypothèse commode : c'est
**exactement** la condition de positivité, et elle est décidable exactement sur
les entiers.

Ses trois spécialisations sont celles du contrat :

| arité | prédicat de positivité | boule |
|---|---|---|
| q2 | toujours vrai dès `D > 0` | boule de diamètre `ab` |
| q3 | triangle **strictement aigu** | circumboule du triangle, ambiante dans `R^3` |
| q4 | tétraèdre **bien centré** (quatre poids `> 0`) | circumboule du tétraèdre |

**La couche q2 a bien une source exact-once par WSPD.** La partition
Callahan--Kosaraju factorisée écrit `binom(X,2)` comme une réunion disjointe de
rectangles `A_i x B_i` : chaque paire non ordonnée apparaît exactement une fois,
en `O(s^3 n)` rectangles. C'est l'acquis de
[`AUDIT_SOURCE_CK_WST_Q2_Q3_Q4_35FCEA8_20260814.md`](AUDIT_SOURCE_CK_WST_Q2_Q3_Q4_35FCEA8_20260814.md).

**Le filtre de rang est le seul qui morde.** Sur `uniform`, l'acuité seule
retient `7,3` à `7,8 %` des quadruplets non dégénérés ; c'est le rang qui ramène
`Theta(n^4)` candidats à quelques dizaines de supports par point.

## 2. Deux corrections de vocabulaire, obligatoires

**(a) « tétraèdre aigu » ne veut pas dire « à faces aiguës ».** Les deux
implications sont réfutées et gravées : il existe un q4 positif à deux faces
obtuses, et un tétraèdre à quatre faces aiguës dont une barycentrique vaut
`-1/12` (`tetrahedron_face_filter_counterexamples.json`). Le prédicat q4 est, et
reste, la stricte positivité des quatre poids du circumcentre — `c8::bien_centre`
par Cramer, sans jamais former le centre.

**(b) « aigu » est strict.** Un `lambda` nul n'est pas un cas limite bénin : le
support minimal devient un sous-ensemble propre, l'arité **dégrade**, et
l'événement est déjà émis par ce sous-support. Fixture gravée :
`(0,0,0)`, `(4,0,0)`, `(2,2,0)`, `(2,0,2)` sont cosphériques de centre `(2,0,0)`
et rayon `2` ; `ab` est un diamètre, `lambda_c = lambda_d = 0`, l'événement
**reste q2**. Un contrat qui écrirait `>=` compterait deux fois le même
événement et perdrait l'unicité de la `SupportKey`.

## 3. Précision formelle : « puis » n'est pas une cascade de rang

Dans l'énoncé reçu, « puis » énumère les trois cas ; il ne spécifie aucun flux
de données entre eux. Il faut donc interdire explicitement une autre lecture :
retenir des paires, étendre ces seules sorties en triangles, puis étendre ces
seules sorties en tétraèdres. Cette architecture serait fausse et elle est déjà
réfutée par une fixture permanente.

**Non-hérédité du rang.** La fixture de 64 points u16 du contre-audit
(`T = {(20,20,20),(60,60,20),(60,20,60),(20,60,60)}`, plus `G01`, `G23`, `W` et
quarante points) porte un q4 de rang `4` dont **les six arêtes et les quatre
faces ont rang 12**. Une seconde fixture, `a=(10,10,10)`, `b=(20,10,10)`,
`x=(12,6,6)`, `y=(12,6,14)`, donne un q3 de rang 12 pour un q4 de rang 4. Avec
`s_max = 8`, la couche q2 du contrat supprime les six arêtes, et le q4 de rang
`4` est définitivement perdu. Les mutants `q4_requires_retained_q3`,
`q4_requires_q2_clique` et `q4_uses_q3_depth_mask` gravent cette faute.

**Mesure indépendante.** Sur `uniform` à densité `10^-3`, en énumérant par ancre
d'arête diamétrale et en comparant les deux règles d'ancre, la perte est de
`1,22 / 1,23 / 1,74 %` des q3 et `1,05 / 1,16 / 1,41 %` des q4 à `n = 60 / 100 /
140`. Ce n'est pas une queue négligeable : c'est une perte **systématique**, qui
interdit tout `public_status=exact`.

## 4. L'ancre propre à chaque énumérateur : un fuseau, pas une sortie q2

L'objet correct existe déjà dans le dépôt, sous le nom de **fuseau**
(`prototype/spindle_cone.hpp`). Pour une paire distincte `(a,b)`, poser
`d=b-a`, `L=||d||`, `m=(a+b)/2`, `z=m+u`,
`H=(b-z).(z-a)=L^2/4-||u||^2` et
`Xi=||d x (z-a)||^2=L^2||u_perp||^2` :

| lane | fuseau | signification |
|---|---|---|
| q2 | `H > 0` | intérieur de la boule de diamètre `ab` |
| q3 | `H > 0` et `3 H^2 > Xi` | |
| q4 | `H > 0` et `2 H^2 > Xi` | |

Ces domaines sont emboîtés : `W4 < W3 < W2`. Leur propriété est celle-ci.

**Théorème d'ancre.** Soit `S` un support positif affinement indépendant
d'arité `q` dans `{2,3,4}`, dont `(a,b)` réalise le diamètre, et
`B(c,rho)` sa miniboule. Alors `W_q(a,b)` est inclus dans l'intérieur strict de
`B(c,rho)`.

*Preuve.* Écrire `c=m+w`, avec `w` orthogonal à `d`. Comme `a` et `b` sont sur
la coquille, `rho^2=L^2/4+||w||^2`. La positivité fait de cette circumboule la
miniboule. Jung dans l'espace affine de dimension `q-1` donne respectivement
`||w||<=T_q`, avec `T_2=0`, `T_3=L/sqrt(12)` et `T_4=L/sqrt(8)` ; il dit que
les centres réalisables sont **contenus** dans ces disques, pas qu'ils les
parcourent tous. Pour `z=m+u`, on a
`||z-c||^2-rho^2=-H-2 u_perp.w<=-H+2 T_q||u_perp||`. Sous `H>0`, les
conditions `3H^2>Xi` et `2H^2>Xi` sont exactement les inégalités strictes qui
rendent ce dernier membre négatif pour q3 et q4 ; q2 emploie `T_2=0`. Ainsi
chaque point du fuseau ouvert est strictement intérieur. Fin de preuve.

Poser `d_q=s_max-q`, maximum d'intérieurs accepté sous `RelevantGP`, et
`h_q=d_q+1=s_max-q+1`, premier compte qui tue la lane. Comme
`C_q=|P inter W_q(a,b)|<=|I_B|` et `|U_B|>=q`, la condition `C_q>=h_q`
implique un rang fermé strictement supérieur à `s_max`. L'ancre n'est donc
explorée que si `C_q<h_q`. À `s_max=11`, les seuils sont `10/9/8`. Cette
condition est nécessaire, jamais suffisante ; le verdict final reste le census
complet `(I_B,U_B)` et sa disposition.

**Ce que cette précision change.** L'ancre d'une lane d'arité `q` n'est pas
« une paire retenue en q2 », c'est une paire créée dans l'énumérateur `q` et
dont le fuseau d'arité `q` est peu peuplé. Ce filtre est **nécessaire** ; il ne
prétend pas être suffisant. Il
survit précisément aux fixtures de non-hérédité : le fuseau q4 d'une ancre est
strictement inclus dans la circumboule du q4. Pour un q4 de rang fermé `4`, on
a même `C_4<=|I_B|=0`, quand bien même la boule diamétrale de son arête maximale
contient douze points.

**Le prédicat de fuseau possède une primitive `ALL` exacte.** À `a` et `z` fixés,
l'ensemble des cibles `b` telles que `z` soit un témoin universel est un **cône
de Lorentz ouvert et convexe** d'apex `z` et d'axe `z-a` ; une AABB de cibles y
est incluse **si et seulement si** ses huit coins y sont. Cela certifie un
target-node conditionnellement aux singletons `(a,z)`. Cela ne décide ni un
rectangle CK complet `A x B` où `a` varie, ni un verdict `NONE`, ni son compte
de rang : ces produits exigent corrélation ou split fail-open.

## 5. Ce que « généraliser WSPD » demande encore

Le contrat demande, pour q3 et q4, une partition analogue à CK dont chaque bloc
puisse être **décidé en rang**. C'est le verrou central, et il n'est pas ouvert :
cinq certificats de bloc — masque central, `SOC64`, `BlockJungDual64`,
`HCBlockDepth`, `Corner8BallDepth` — échouent dès que le bloc porte de la masse ;
deux d'entre eux sont exacts, mais seulement au sens `ALL`, jamais `NONE`. Aucun
certificat de profondeur uniforme sur un rectangle CK n'est connu.

Ce qui est en revanche disponible pour ordonner **à l'intérieur du seul
énumérateur q4** ses quadruplets exact-once :

**Lemme du préfixe ternaire q4 aigu.** Tout q4 bien centré d'arête maximale
`ab` possède au moins un préfixe `abx` ou `aby` **strictement aigu** ; jamais
nécessairement deux
(fixture `p0=(8,2,12)`, `p1=(1,3,9)`, `p2=(4,0,0)`, `p3=(10,5,1)`, de poids
`(1459/3750, 977/11250, 3613/11250, 761/3750)`). Le porteur **primaire** est le
plus petit `PointId` parmi les porteurs aigus.

Ce triple appartient exclusivement à `Lane4` : ce n'est ni un événement q3, ni
un record produit par `Lane3`, ni une preuve de complétude q3. Le lemme donne
un ordre interne de la boucle q4, **pré-rang**. La liste est construite par le
générateur q4 lui-même, sans lire aucune condition de profondeur d'une autre
lane. Y appliquer le verdict de `Lane3` serait la faute interdite à la section
3, sous un autre nom.

## 6. Ce que le contrat ne borne pas

1. **L'acuité ne borne pas la sortie.** Il existe des nuages u16 à nombre
   quadratique de supports aigus : `384` points cocycliques donnent
   `2 322 560` supports aigus pour **une seule** sphère. Toute réalisation du
   contrat doit donc porter une politique de dégénérescence explicite — `RLE`
   par `BallKey`, quotient de plateau, ou `unsupported_degeneracy` — jamais un
   buffer dimensionné sur une hypothèse de position générale.
2. **L'acuité n'est pas sélective au niveau du bloc.** Le filtre d'acuité
   grossier conserve plus de `99,998 %` des blocs q3 et q4 à `n = 50 000`, sur
   `uniform` comme sur `eight_clusters`. Elle sert à décider un support, pas à
   élaguer un rectangle.
3. **L'acuité n'est ni un certificat de densité ni un certificat de pénurie.**
   Sur la contre-famille `two_lines` (`A_i = (i,0,0)`, `B_j = (0,j,H)`), aucun
   triangle n'est aigu — la source q3/q4 est **vide** — et pourtant les `n^2/4`
   paires croisées possèdent chacune une sphère vide, donc aucun certificat de
   pénurie fondé sur les paires ne les ferme. Le contrat est correct sur cette
   famille, mais il n'y donne aucune borne de travail.
4. **Le coût réel est dans « tester le rang ».** Le rang est un census ambiant
   en dimension trois, par `BallKey` et après `RLE` ; c'est lui, et non
   l'énumération des aigus, qui reste à rendre sous-quadratique.

## 7. Le contrat, réparé et normatif

1. **Objet.** La source est l'ensemble des couples `(S, B(o,R))` où `S` est un
   support **positif strict** d'arité `2`, `3` ou `4` et où
   `|P inter int B(o,R)| <= d_q`, avec `d_q=s_max-q`. Sous rang fermé
   `s_max=11`, les maxima acceptés sont `d_2=9`, `d_3=8`, `d_4=7`. Les premiers
   comptes rejetés sont `h_q=d_q+1`, soit `10/9/8`. En particulier,
   `s_max-2=9` est le **seuil strict de mort q3** : `Lane3` accepte `I_B<9`,
   jamais `I_B<=9`. Rien d'autre n'est un événement.
2. **Ancre.** Chaque support est possédé par son **arête diamétrale** `(a,b)`,
   départagée en cas d'égalité par le plus petit couple de vrais `PointId`.
3. **Filtre d'ancre.** Une ancre est explorée pour la lane `q` si
   `|P inter W_q(a,b)|<h_q`. Ce filtre est nécessaire, il est
   bloc-exact par les huit coins, et il ne dépend d'aucun résultat de la lane
   `q-1`.
4. **Trois énumérateurs.** La partition neutre de paires et l'index Morton
   immuable peuvent être mutualisés, puis trois queues autonomes sont créées :
   `Lane2(Pair2)`, `Lane3(PairAnchor3,Third3)` et
   `Lane4(PairAnchor4,Q4Seed3,Fourth4)`. Leurs records, verdicts, caps,
   continuations et preuves de complétude sont disjoints.
5. **Énumération interne.** `Lane3` parcourt son propre troisième sommet dans
   la lentille `B(a,D) inter B(b,D)` et retient le triangle s'il est strictement
   aigu et si `(a,b)` est son arête maximale canonique. `Lane4` construit son
   propre `Q4Seed3`; le plus petit `PointId` parmi ses préfixes aigus choisit la
   provenance primaire, le quatrième sommet parcourt la lentille, et `(a,b)`
   doit rester l'arête maximale canonique du quadruplet. `Q4Seed3` n'est jamais
   lu dans la sortie de `Lane3`.
6. **Univers d'intériorité.** Tout point strictement intérieur est à distance au
   plus `0,966 D` du milieu `m` ; la requête de census se borne exactement à
   `|z - m| <= D`.
7. **Sorties.** Le contrat n'est réalisé que s'il publie `BallKey`, `SupportKey`
   avec `I_B` et `U_B`, le shell complet, l'owner rejouable sur de vrais
   `PointId`, et le statut de fenêtre. Un compteur n'est pas une source.

## 8. Ce qui est mesuré, et ce qui ne l'est pas

L'énumération des points 2 à 6 a été réalisée dans un instrument hors dépôt et
**coïncide exactement** avec le brute force `C(n,4)` du dépôt : `q4 = 2563`,
`6267`, `10981` à `n = 60`, `100`, `140` sur `uniform` à densité `10^-3`. La
coïncidence porte sur le compte, pas sur les identités : elle valide l'ancre, le
porteur primaire et les deux règles de départage, pas un payload.

Ne sont mesurés ni un débit, ni un `warm_e2e`, ni une pente, ni une borne
mémoire. Aucun `public_status` ne change. Le contrat `50 000` points reste
entièrement ouvert, et l'interdiction de rampe G4 avant porte locale verte reste
en vigueur.

## 9. Portes exigées avant toute réception

1. fixture de non-hérédité rejouée **par le filtre d'ancre** : le q4 de rang `4`
   doit survivre alors que la boule diamétrale de son arête maximale porte
   douze points ;
2. mutant `ancre-boule-diametrale` qui remplace `W_q` par `W_2` : il doit être
   **tué** par la fixture précédente, à code de sortie exact ;
3. mutant `porteur-aigu-filtre-par-rang` : il doit être tué par la même fixture ;
4. fixture `lambda` nul (`(0,0,0)`, `(4,0,0)`, `(2,2,0)`, `(2,0,2)`) : l'arité
   publiée doit être `2`, jamais `4` ;
5. fixture cocyclique : la sortie doit être `RLE` par `BallKey` ou
   `unsupported_degeneracy`, jamais un buffer par support ;
6. `two_lines` : source q3/q4 **vide**, et le compteur de travail publié ;
7. équivariance par permutation des `PointId` à géométrie constante ;
8. positions dupliquées conservées, seule la paire endpoint `D = 0` filtrée ;
9. comparaison au brute force du dépôt sur les deux familles obligatoires, aux
   tailles où il est calculable, avec plancher de non-vacuité.

GCP non utilisé à la date de cette note.
