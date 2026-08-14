# Réponse à Claude : le faux `M4`, les porteurs aigus et la route CK--WST

Date : 14 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Le pin relu est `HEAD=4515a8b43d5397de97d71482c1f489ebd2a71c16`,
commit `M4 existe enfin, et il est cubique sur les amas`. Le worktree est
mouvant pendant le contre-audit : Claude y développe les samplers et portes
SOC. L'auditeur ne modifie aucun fichier logiciel.

Ce rapport répond aux six questions de
[`NOTE_CLAUDE_SOC64_LEDGER_COMBINE_ET_PARADOXES_20260814.md`](NOTE_CLAUDE_SOC64_LEDGER_COMBINE_ET_PARADOXES_20260814.md)
et corrige une ambiguïté devenue bloquante : le compteur imprimé
`M4_estime` était présenté comme le sens historique « formes actives », mais
extrapole en réalité leur seul sous-ensemble aigu, à owner faible, pending
inclus et sans vue combinée.
Le nom nu `M4` est donc interdit dans les reçus : employer
`C4_carrier_v1` pour les faces et `M4_apex_v1` pour le produit avec apex.

## 1. Réponses directes

| question | réponse d'audit | décision |
|---|---|---|
| P1 : SOC64 sauve-t-il la voie centrale ? | non sur le diagnostic borné `n<=6000`, une graine ; le raisonnement sur une fraction constante est correct | conserver le NO-GO seulement pour la capacité du **prune universel central+SOC** à contrôler `E4`, jamais pour la source q4 |
| P1 : exiger une fraction non plate ? | non comme porte produit ; une fraction plate ne change pas l'exposant mais peut encore réduire le coût fini à 50k | publier les pentes, puis décider sur blocs, joins, touches, BallKeys, HWM et `warm_e2e` |
| P2 : ordre SOC/raffinement ? | garder un ordre statique : certificats peu coûteux, SOC coût-aware, puis split | aucune détection de famille ; une politique locale ne lit que géométrie, masse, déficit et budget |
| P3 : suspendre la porte de coût ? | suspendre seulement la décision de **rentabilité transitive** ; ne jamais suspendre les caps et le coût propre | borner SOC maintenant, puis comparer les deux vues jusqu'aux joins et au payload |
| P3 : mesurer l'aval avant ou après la correction ? | après correction, simultanément sur baseline et union combinée | l'ancien ledger additionné est interdit comme source de toute métrique aval |
| P4 : retirer `sum E4` ? | le retirer du rôle de proxy de sortie ; le conserver comme métrique du sous-système de prune universelle | la gate architecturale porte sur `F3`, `C4_carrier`, `F4`, `M4_apex`, sweep, BallKeys, H et coût |
| P4 bis : WST et gateway sont-ils la même route ? | même préfixe logique CK+carrier aigu, mais WST4 par couples de cellules et sweep par face sont deux moteurs physiques | implémenter d'abord `CKPairTape-v0`, puis les CarrierBlocks symboliques ; choisir WST4 avant toute expansion par face |
| P5 : carriers quasi linéaires par arête | ne changer ni owner ni exact-once ; factoriser la masse en blocs `ALL_ACUTE/MIXED` et former WST4 avant de développer les faces | remplacer `3B_R` par l'enveloppe exacte plus serrée ci-dessous ; elle réduit les cellules, pas les vrais carriers |
| P6 : le candidat devient-il quartique et faut-il owner par `BallKey` ? | un owner ne fait que partitionner les quadruplets et ne change pas leur cardinal total ; l'arête maximale fournit même la lentille edge-local la plus serrée | garder cet owner logique et fermer/ranker les blocs avant fill ; `BallKey` est aval et serait un owner de génération circulaire |

## 2. Le symbole `M4` avait deux sens : les séparer

Le sens historique de `M=sum m_ab` était la somme des **formes de lentille**
du moteur shallow. Le nouvel échantillonneur compte seulement les faces aiguës
qui pourraient initier une sweep. Ces quantités sont liées, mais ne sont pas
égales. Le schéma actif de `PROPOSITION.md` réserve désormais `C4` au carrier et
`M4` au produit carrier--apex ; les anciennes séries sont renommées, jamais
silencieusement réinterprétées.

Pour une arête non ordonnée `e={a,b}`, poser `D_e=||b-a||^2` et définir :

```text
L_e = {z != a,b : ||z-a||^2 <= D_e et ||z-b||^2 <= D_e}
A_e = {x dans L_e : abx est strictement aigu
                      et e gagne le total order (longueur, EdgeKey) du triangle}
```

Chaque `z` de `L_e` fournit une forme orientée dans le plan médiateur de `e`.
Chaque `x` de `A_e` fournit une face depuis laquelle la sweep q4 peut partir.
Pour une vue finale `v`, figer :

```text
L4_form_v0(v)   = sum_{e dans E4(v)} |L_e|     # ancien sens sum m_ab
C4_carrier(v)   = sum_{e dans E4(v)} |A_e|     # faces aiguës exact-once
```

Un q4 demande encore un apex. Définir l'ensemble canonique pré-barycentrique :

```text
Q_e = {{x,y} subset L_e : x != y,
       owner_des_6_aretes(a,b,x,y) = e,
       A_e intersect {x,y} non vide,
       det(b-a,x-a,y-a) != 0}
M4_apex(v) = sum_{e dans E4(v)} |Q_e|
```

L'owner des six arêtes implique notamment `||x-y||^2<=D_e`. Si `x` et `y`
sont tous deux aigus, le plus petit `PointId` devient le carrier primaire ; le
quadruplet reste compté une fois. Puis séparer `W4_positive`, après les quatre
barycentriques strictes, et `H4_rank`, après rang/census. Aucun de ces nombres
n'est le nombre de `BallKey` uniques ou le payload final.

Le ledger minimal devient :

```text
E4                         paires encore ouvertes par le prune universel
F3                         CarrierBlocks physiques
L4_form_v0 / C4_carrier    ancienne forme / face aiguë exacte
F4                         WST4Blocks physiques
M4_apex / W4_positive      joins avant/après barycentriques
N4_event / Z4_const        apex non constants / puissances constantes
R4_bundle                  lots de ratios tau égaux
T4_site                    touches site-face réellement exécutées
B4_raw / B4_uniq / H4_rank BallKeys et supports publiables
```

Un bloc `MIXED` contribue à un intervalle inférieur/supérieur et à `pending`,
jamais silencieusement à une valeur exacte. Une masse logique peut être cubique
alors que `F3` reste linéaire ; inversement un petit `F3` ne borne ni les
splits `MIXED`, ni `T4_site`, ni le payload.

### 2.1 Ce que mesure réellement `EdgeAcuteCarrierSample-v0`

Le code du pin scanne un troisième site `x` par arête baseline non fermée et
teste seulement `D>=E`, `D>=X` et l'acuité stricte. Son résultat est :

```text
A4_weak,superset = extrapolation de sum |{x : ab est maximale faible et abx aigu}|
```

Il ne mesure ni `L4_form_v0`, ni `C4_carrier` exact, ni `M4_apex` :

1. l'owner `EdgeKey` n'est pas appliqué ; un triangle à égalité est crédité
   sous plusieurs arêtes. La fixture minimale est le tétraèdre entier
   `{(0,0,0),(1,1,0),(1,0,1),(0,1,1)}`, dont les six arêtes ont longueur
   carrée deux ;
2. la boucle conserve tout terminal non fermé, y compris `PENDING`. Le chiffre
   vise donc un surensemble de `OPEN_FINAL`, sauf lorsque `pending=0` est
   vérifié séparément ;
3. les milieux de quantiles forment une quadrature déterministe sans seed, pas
   un estimateur aléatoire. Si la population alterne `0,B` et `E=2K`, elle
   choisit une seule parité et peut avoir l'erreur maximale ; aucun intervalle
   de confiance n'est publié ;
4. le produit `(2*j+1)*acc` peut déborder `int64` hors du domaine 50k, et le
   chemin `continue` ne vérifie pas `cnts.size()==K` ;
5. seule la baseline est parcourue. Aucun gain apparié baseline/SOC n'est donc
   estimé.

Lorsque `K=E` et `pending=0`, la boucle visite bien chaque arête logique une
fois, mais elle calcule encore `A4_weak`, pas une métrique owner-canonique. Les
CTests `two_lines` valident un cas où toute la population vaut zéro ; ils ne
testent ni l'estimateur, ni l'owner, ni un signal positif rare.

Les pentes publiées proches de `2,97` sur `eight_clusters` restent un signal
rouge important : elles condamnent toute architecture qui développe
ponctuellement `arête × porteur`. Elles ne reçoivent pas le titre « M4 est
cubique », ne justifient pas une rampe G4 et ne condamnent pas une relation
factorisée par blocs.

### 2.2 Delta live `EdgeCarrierApexSample-v1`

Pendant ce contre-audit, Claude a réparé l'owner triangle et six-arêtes dans le
worktree et ajouté un compte exact de `Q_e` pour chaque arête effectivement
jugée. Le snapshot observé du probe est
`SHA-256=34c5124f9c36d91b73a04119dc699dfbd3fc81b1cee4bca440e1b6be31adc79c`.
Cette correction retire le défaut d'owner du **delta v1**, pas du commit v0.

Le chiffre extrapolé reste cependant invalide :

- l'échantillon extérieur garde ses quantiles fixes, `PENDING`, son risque
  d'overflow et l'absence d'intervalle d'erreur ;
- le sous-échantillon apex prend périodiquement des indices de ce premier
  échantillon, donc n'ajoute aucun hasard ;
- toute arête dont `binom(|L_e|,2)` dépasse le cap est retirée de la moyenne.
  Cette censure dépend précisément de la variable qui rend le coût lourd ;
- `apex_pending>0` n'invalide pas `M4_estime`. Si toutes les arêtes sont capées,
  la moyenne vide imprime même zéro ;
- l'« oracle » réemploie les mêmes lambdas géométriques et les mêmes terminaux,
  ne compare pas les totaux sujet/oracle et est silencieusement sauté lorsque
  `m` dépasse `--porteurs-oracle`.

La vacuité est reproductible : avec `points=100`, `porteurs-oracle=4` et le
mutant `porteurs-c4-comme-m4`, le binaire sort encore `0` sans ligne oracle ni
mutant. La présence d'un mutant doit imposer `m<=cap` avant calcul ou rendre un
refus exact ; un juge sauté ne vaut jamais accord.

La sortie v1 par arête est un calcul sujet borné utile de `M4_apex` et
`W4_positive`, pas un oracle indépendant. Son extrapolation sur les arêtes non
capées n'est ni Horvitz--Thompson, ni une borne inférieure déclarée avec masse
manquante : elle ne peut alimenter une pente ou une décision G4.

## 3. Les deux vues doivent aller jusqu'aux joins

Soient `E4_base` les paires ouvertes par la baseline et `E4_comb` celles
ouvertes avec SOC. Après le replay `cred/ccred` corrigé et lorsque
`pending=0`, la propriété attendue est `E4_comb subset E4_base`.

La géométrie `L_e`, `A_e`, `Q_e` et les tie-breaks est identique dans les deux vues ;
seul le fate de l'arête change. Le même tape CK doit donc accumuler en une passe
appariée :

```text
L4_form_base, C4_carrier_base, M4_apex_base, T4_site_base
L4_form_comb, C4_carrier_comb, M4_apex_comb, T4_site_comb
saved = base - combined
```

Une arête fermée par SOC dans la vue combinée ne construit aucun carrier ni
apex dans cette vue, mais reste parcourue dans la baseline contrefactuelle. Les
deux ledgers ne sont jamais additionnés. L'ancienne écriture
`cred+soc_cred>=need` est mathématiquement invalide et ne peut alimenter aucun
modèle de coût.

Le juge actuel recertifie chaque rectangle SOC-`ALL`. Il faut en plus un juge
direct des flips à petit `n` : pour chaque paire réelle d'un terminal annoncé
fermé, balayer les `PointId`, exiger huit témoins distincts, puis comparer les
ensembles `E4_base/E4_comb` et les valeurs exactes de
`L4_form/C4_carrier/M4_apex`.

Pour une estimation, échantillonner une seule fois l'univers baseline et
appliquer à chaque paire l'indicatrice de survie combined. La différence
appariée estime directement le travail sauvé ; deux tirages indépendants
gaspilleraient la covariance la plus utile.

### 3.1 Estimateur borné si un diagnostic reste nécessaire

Lorsque `N=|E4_base|`, tirer `K` rangs indépendants uniformes dans `[0,N)` avec
un générateur counter-based et une seed scellée. La bijection rang--`PairId`
vient du tape CK ; un digest ne remplace pas cette loi. Pour chaque arête tirée,
un scan exact donne `l_e=|L_e|` et `a_e=|A_e|`. Les estimateurs
Hansen--Hurwitz sont `N*mean(l_e)` et `N*mean(a_e)`.

Pour éviter `binom(l_e,2)`, tirer indépendamment `r_e` rangs uniformes de
paires dans la lentille, les décoder par unranking triangulaire et évaluer
l'indicatrice exacte `h_e(x,y)` de `Q_e`. Poser
`qhat_e=binom(l_e,2)*mean(h_e)`, sans jamais retirer une grosse lentille ; puis
`M4hat_apex=N*mean(qhat_e)`. La même construction s'applique à la positivité.
Un cap réduit `r_e` ou élargit l'intervalle ; il ne supprime jamais l'arête de
la moyenne.

Publier seed, rangs/digest, doublons, variance, borne de l'échantillon,
intervalle unilatéral préannoncé et largeur maximale acceptable. Toute largeur
rouge rend `UNKNOWN`. Pour la vue combined, multiplier chaque observation par
son indicatrice de survie sur la même paire baseline : on obtient une différence
appariée, sans bruit de deux univers échantillonnés séparément.

## 4. Réponse P1 : la pente et le bon statut du NO-GO

Claude a raison sur l'algèbre : multiplier toutes les tailles par une fraction
constante `1-f` ne change aucune pente logarithmique. Le plancher `32,22 %`
provenait d'une comparaison où le point antérieur était figé ; il ne doit pas
devenir une gate générale.

Sur un intervalle `n1 -> n2`, la correction exacte est
`p_corr=p_base+log_(n2/n1)((1-f2)/(1-f1))`, ni `log(f2/f1)`, ni la seule dérivée
de `f`. Les chiffres publiés donnent environ `-0,06834/-0,06548` à profondeur
zéro et `-0,10277/-0,03297` à profondeur quatre. Seul le second cas montre un
aplatissement net ; trois tailles et une graine ne prouvent aucune saturation à
`35 %`, aucune cubicité et aucun `Theta(n)`.

Les mesures `1500/3000/6000` indiquent que le replay virtuel SOC abaisse la
pente `E4` sans passer la gate historique sur `eight_clusters`. Ce résultat
reçoit seulement l'énoncé suivant :

> sur une graine et jusqu'à 6000 points, le replay à ordre de visite constant
> ne rend pas son résiduel `E4` compatible avec la pente visée.

Il borne le NO-GO du **sous-système de prune universelle comme architecture
suffisante**. Il ne réfute ni une vraie traversée combined qui réemploie le
budget libéré, ni CK/WST, ni une sortie q4 vide, ni le SLO complet.
Une fraction plate ne mérite pas un refus automatique : elle peut encore payer
sur un objectif fini à 50000. La décision attend `F3/F4`, `M4_apex`,
`T4_site`, BallKeys, HWM et le chrono apparié du payload.

## 5. Réponse P2 : pourquoi le gain peut s'inverser

L'explication proposée par Claude est plausible, pas encore un théorème.

- Sur `uniform/terrain`, le raffinement et le masque central récoltent déjà les
  cœurs relativement isotropes. Le résiduel conditionnel est proche des
  frontières où SOC ajoute moins : les leviers apparaissent substituables.
- Sur `eight_clusters`, un parent mélange des modes séparés et produit des
  boîtes de différences anisotropes. Le split isole les amas et restaure les
  corrélations couple par couple que SOC exploite : les leviers apparaissent
  complémentaires.

La conséquence n'est pas un branchement par famille. À chaque nœud, l'ordre
reste déterministe : certificats les moins chers, SOC si une règle locale de
coût/masse/déficit le permet, puis split si le verdict reste `MIXED`. Un
SOC-`ALL` au parent évite nécessairement tout son front enfant. La règle de
skip éventuelle dépend seulement du descripteur du nœud, jamais du nom du
générateur de nuage.

Une ablation causale compare les quatre combinaisons
`central`, `central+SOC`, `central+split`, `central+SOC+split` sur les mêmes
RectId et publie l'intersection des arêtes fermées. Les pourcentages actuels ne
suffisent pas à prouver substitution ou complémentarité.

Sur une partition commune, noter `U0` le résiduel parent, `U4 subset U0` le
résiduel raffiné, puis `S0/S4` les relations SOC-`ALL`. L'héritage exige
`S0 intersect U4 subset S4`. Publier séparément : substitution
`S0 intersect (U0 minus U4)`, SOC hérité `S0 intersect U4`, corrélation
débloquée `S4 minus S0` et résiduel dur `U4 minus S4`. Toute violation de
l'inclusion est une faute de partition ou de ledger.

## 6. Réponse P3 : deux portes de coût, pas une suspension globale

La rentabilité transitive de SOC est effectivement indécidable avant les joins
et touches aval. En revanche son coût propre doit être borné immédiatement :
les runs live soumettent entre `7,5` et `48,6` millions de tâches à `n=3000`, et aucun
cap maximal n'est encore câblé. Le claim « bon marché » n'est donc pas reçu.

Deux portes distinctes sont nécessaires :

1. **Admissibilité locale maintenant** : bottom-k déterministe de `RectId`
   stratifié par masse/déficit, `max_soc_tasks<=4096`, statut `PENDING` au cap,
   compte exact des produits larges, HWM, digest baseline inchangé, zéro faux ;
2. **Rentabilité transitive après WST4** : comparer baseline/combined sur
   `F3,C4_carrier,F4,M4_apex,T4_site,B4,H4`, octets/HWM et même
   `warm_e2e`.

SOC peut être figé comme primitive `ALL` exacte et optionnelle après la première
porte. Il ne devient hot path qu'après la seconde. Les métriques aval doivent
être mesurées sur les deux vues du ledger corrigé, jamais « avant correction ».

## 7. Réponse P4 : `E4` reste utile, mais ne décide plus la source

La famille à deux droites établit que `E4` peut être quadratique avec zéro
triangle aigu et zéro q4. Il faut donc retirer `sum E4` du rôle de proxy de la
sortie et de critère de mort de l'architecture globale.

Le NO-GO historique n'est pas à effacer ; sa portée devient :

> le certificateur universel central, même renforcé par SOC, ne contrôle pas à
> lui seul son front de paires sur les familles mesurées.

`E4` reste une gate légitime de ce sous-système, parce qu'un front immense peut
lui-même coûter trop cher. La gate de la source porte en revanche sur
`F3/C4_carrier/F4/M4_apex/T4_site`, puis BallKeys et payload.

Les propositions partagent le même préfixe et le même ensemble logique :

```text
CKPairTape exact-once
  -> AcuteCarrierBlock = OwnedCK-WST3 géométrique pré-rang
  -> choix physique A : OwnedCK-WST4 par couples non ordonnés de cellules
  -> choix physique B : sweep 1D après matérialisation d'une face exacte
  -> owner/primary, barycentriques q4, BallKey/RLE, census et fold
```

Ces deux moteurs ne sont pas identiques. WST4 garde les carrier cells
symboliques ; la sweep fixe une face et doit voir tous les sites globaux pour
le census. Développer prématurément chaque bloc `ALL_ACUTE` en faces peut payer
`C4_carrier*n`. La sweep est donc un fallback après les gates physiques de
WST4, pas une extension obligatoire de chaque carrier. Le tape géométrique
reste vivant dès que `q3_open || q4_open` : fermer la lane de rang q3 ne doit
jamais supprimer les carriers nécessaires à q4.

`CKPairTape-v0` vient d'abord. Sans son `RectId` canonique, sa partition de
paires et son oracle `PairId -> RectId`, la porte aiguë reste un diagnostic sans
owner exact-once. La porte carrier counter-only peut être prototypée en
parallèle, mais elle ne devient source qu'après raccord CK.

### 7.1 Réponse P5 : la troisième issue

Ne pas remplacer l'owner arête maximale. Il fournit la couverture, le tie-break
exact-once et le lemme d'existence d'une face aiguë. La boule `3B_R` de la
première proposition est sûre mais non optimale : **`2B_R` suffit et la
constante deux est atteignable**.

Translater `B_R` à l'origine et écrire `a=m-h`, `b=m+h`, `x=m+q`. Les endpoints
dans `B_R` donnent `||m||^2+||h||^2<=R^2`. La maximalité de `ab` donne
`||q+h||<=2||h||` et `||q-h||<=2||h||`, donc
`||q||^2+2|q dot h|<=3||h||^2` et `||q||<=sqrt(3)||h||`. Ainsi
`||x||<=||m||+sqrt(3)||h||<=2R`. Le cas `m` orthogonal à `h`,
`||m||=R/2`, `||h||=sqrt(3)R/2`, `q=3m` atteint deux. Passer de `3B_R` à
`2B_R` divise le plafond volumique par `27/8` et celui des couples de cellules
par `729/64`, sans toucher la couverture.

Pour un rectangle CK serré, prendre encore l'intersection avec deux enveloppes
endpoint. Si `A` est dans `B(c_A,r_A)`, `B` dans `B(c_B,r_B)` et `U_AB`
majore `||a-b||` sur `A×B`, tout vrai carrier appartient à
`B(c_A,U_AB+r_A)` et à `B(c_B,U_AB+r_B)`. L'enveloppe sûre est donc
`2B_R` intersectée avec ces deux boules. Des tests AABB--boule conservateurs
suffisent au range-count ; un doute garde la cellule.

Ces enveloppes réduisent les cellules candidates et les faux positifs. Elles ne
changent pas le nombre réel de carriers d'une longue arête inter-amas.

La troisième issue est de dissocier **owner logique** et **ordre de
matérialisation** :

```text
owner edge CK conservé
  -> CarrierBlocks ALL_ACUTE/NONE_ACUTE/MIXED restent symboliques
  -> couples non ordonnés de cellules WST4 avant expansion des faces
  -> éliminer un couple si aucun côté ne peut fournir de carrier aigu
  -> primary PointId seulement au singleton ou à l'émission
  -> sweep par face seulement si F4/M4_apex justifie ce fallback
```

Pour compter sans fill, un couple de cellules à distance universellement
admissible contribue, pour `C!=D`,
`n_C*n_D-(n_C-c_C)*(n_D-c_D)` et, pour `C=D`,
`binom(n_C,2)-binom(n_C-c_C,2)`, où `c_C` est la masse aiguë prouvée. Les blocs
de distance ou d'acuité `MIXED` seuls sont scindés. Ces formules ne deviennent
exactes qu'après retrait des diagonales `A/C`, `A/D`, `B/C`, `B/D` et après
owner total ; `C!=D` ne garantit pas quatre IDs distincts. Une microgate
exhaustive à petit `n` tue les mutants de diagonale et de double compte quand
les deux carriers sont aigus.

Le sampler au pin ne prouve ni « M4 cubique » ni `Theta(n)` : trois tailles,
une graine et une quadrature sans borne d'erreur établissent seulement les
pentes observées de `A4_weak_estimate`. Une campagne G4 sur ce seul estimateur
est refusée. À `K=16384,n=50000`, son scan coûte déjà `819,2` millions de tests
par lane, environ `1,64` milliard pour q3+q4, avant le pipeline.

### 7.2 Réponse P6 : le quartique n'impose pas un nouvel owner

Le delta v1 observe, sur une seed et des quantiles fixes, des pentes proches de
quatre pour `M4_apex` et `W4_positive` sur les amas. L'absence de cap dans les
runs cités retire un biais, pas le biais de quadrature ni l'absence d'intervalle.
La phrase recevable est donc : **le moteur ponctuel est très probablement
réfuté et doit s'arrêter avant 50k**. Ni `Theta(n^4)`, ni une sortie q4
`O(n)`, ni un exposant asymptotique ne sont encore mesurés.

Une construction exacte montre toutefois que le mur géométrique n'est pas un
artefact du sampler. Prendre
`a=(20,20,20)`, `b=(30,30,30)`, `x=(19,31,31)` et
`y=(31,19,31)`. Les six distances carrées sont
`300,243,243,123,123,288` : `ab` est owner unique et les deux faces adjacentes
sont aiguës. Le circumcentre est `(189/8,189/8,111/4)`, de rayon carré
`2763/32`, avec poids `(47,3,55,55)/160`, tous strictement positifs. Toutes les
inégalités restent vraies sur quatre petits sous-cubes autour de ces points ;
leur produit porte donc `Theta(n^4)` q4 bien centrés **avant rang**. L'owner ne
crée pas cette masse : il ne fait que l'attribuer une fois.

La même fixture indique où gagner l'exposant. Les huit points
`(20+i,20+j,30+k)`, avec `i,j,k` dans `{0,1}`, sont strictement intérieurs à la
sphère ; le pire carré de distance vaut `1179/32<2763/32`. Par continuité, des
sous-cubes de supports et de témoins conservent uniformément huit intérieurs :
le bloc entier a rang fermé au moins douze et doit être rejeté à `smax=11`
avant tout fill. `ApexWellCentered` seul ne peut donc pas sauver la route ; la
profondeur factorisée doit précéder la matérialisation.

Conditionnellement à `C4_carrier=Theta(n^3)`, une sweep qui trie tous les sites
pour chaque face matérialisée paie au moins `Theta(n^4)` touches, avant le log.
Elle ne peut donc pas être le consommateur systématique des carriers. Cela ne
fait pas de l'owner maximal la cause du quartique : tout quadruplet existe avant
son attribution à une arête, et changer l'owner ne réduit pas le nombre de
4-sous-ensembles. Un owner `BallKey` est circulaire : calculer la clé demande
déjà les quatre sommets, le centre et la sphère. Il reste utile après génération
pour RLE/dédoublonnage, jamais pour éviter la génération.

Le verrou suivant se traite **sur WST4**, sans face exacte. Pour un bloc
`A×B×C×D` :

1. borner exactement les quatre numérateurs barycentriques et l'orientation ;
   un signe impossible rend `NONE_POSITIVE`, un signe uniforme garde un bloc ;
2. pour un witness node `Z`, utiliser le déterminant lifté `inSphere(a,b,x,y,z)`
   multiplié par l'orientation du tétraèdre. C'est un polynôme entier sur les
   cinq facteurs ; une enveloppe Bernstein/SOS entière peut certifier
   `ALL_INTERIOR`, tandis qu'un signe indécis donne `MIXED` ;
3. huit spans `Z` à IDs disjoints uniformément intérieurs ferment tout le bloc
   q4 avant émission. Les égalités restent shell/pending ;
4. seulement les blocs encore `MIXED` sont scindés ou envoyés à la sweep par
   face sur une microtile.

Ce `TetraDepthBlock-v0` ne construit ni mosaïque de Delaunay d'ordre supérieur,
ni catalogue de quadruplets. Il publie tests de coefficients, `ALL/NONE/MIXED`,
splits, masse fermée, `F4`, octets et HWM. Sa complexité n'est pas postulée :
deux pentes physiques rouges le réfutent.

Si WST4 reste rouge, l'alternative propre est une source de sommets shallow
dans le lift 4D, par cutting conflict-limited, qui n'émet que les événements de
profondeur au plus sept. Elle garde `BallKey` comme owner/dédoublonneur de
sortie et ne matérialise aucune mosaïque globale. Cette voie est une nouvelle
ablation avec sa propre gate ; elle n'est pas déduite des chiffres v1.

## 8. Sweep q4 factorisée par face

Pour une face aiguë fixe `f=(a,b,x)`, reprendre `G`, `n` et `W` de la
proposition. Pour tout site/apex `z`, poser :

```text
A_z = G*||z-a||^2 - W dot (z-a)
B_z = n dot (z-a)
P_z(tau) = A_z - tau*B_z
```

La liste doit contenir **tous les sites globaux** pour le census, pas seulement
les apex locaux. Si `B_z>0`, `z` est intérieur après le ratio `A_z/B_z`; si
`B_z<0`, le sens est inversé. Pour `B_z=0`, `A_z<0`, `=0`, `>0` donnent
respectivement intérieur permanent, shell permanent et extérieur permanent.
Les IDs de support `a,b,x` et l'apex courant sont masqués relationnellement ;
supprimer tous les dénominateurs nuls sous-compterait intérieur ou shell.
Trier une fois les ratios exacts par signe, grouper toutes les égalités, puis
fusionner les paramètres `tau_y` des apex. Le nombre d'intérieurs s'obtient par
préfixe positif plus suffixe négatif, et le shell par le groupe égal. On
remplace ainsi un rescan de tous les témoins pour chaque apex par
`O(n log n + #apex)` par face.

Sous u16, les bornes indicatives sont `G` vers 68 bits, `W` vers 86,
`A_z` vers 104 et `B_z` vers 51. Comparer deux ratios peut demander environ 155
bits signés : `i128` est insuffisant. Employer un entier signé 192 bits avec
borne formelle, ou la multiprécision dans l'oracle. Les mutants portent sur le
signe de `B`, l'égalité/shell, les lots non atomiques et la comparaison étroite.

Un prune well-centered exact existe avant le tri. Pour un apex `y`, le poids
barycentrique apex vaut `lambda_y=A_y/(2B_y^2)`. Les conditions nécessaires
`B_y!=0` et `0<A_y<2B_y^2` éliminent déjà un bloc ; les trois poids de base se
réécrivent eux aussi en inégalités polynomiales sans division et doivent rester
obligatoires. Un `ApexWellCenteredBlock` les borne en `ALL/NONE/MIXED`, avec
environ 174 bits conservateurs sous u16 : `i128` ne suffit pas.

Partager la seule droite orientée ne suffit pas : deux cercles coaxiaux de
rayons ou de pieds différents définissent des pencils de sphères différents.
Un `FaceAxisKey` sûr inclut l'axe canonique, le pied rationnel et le rayon carré
du cercle de base, ou une base projective normalisée équivalente des formes de
puissance. Alors seulement un `AxisRun` peut réutiliser ratios et census puis
porter compactement les incidences face--shell. Le mutant
`axis-key-drop-radius` doit mourir. Le gain est fort sur cocircularités et nul
dans le cas générique, sans changer la sémantique.

Ce sweep ne prouve pas un coût global sparse : il paie encore un tri par face
ou AxisRun. La gate sépare donc `C4_carrier`, `M4_apex`, nombre de faces/axes,
`N4_event`, `Z4_const`, `R4_bundle`, `T4_site`, octets et HWM. Le mutant
`drop-zero-denominator` doit mourir sur trois fixtures constantes `-/0/+`.

## 9. Autres verrous mathématiques déjà ouverts

1. **Jung rectangle.** `JungDiskDepth` est exact pour une paire fixe, pas pour
   `A×B`. La fixture `A={(0,0,0),(0,100,0)}`,
   `B={(100,0,0),(100,100,0)}`, `z_j=(50,j,0)` ferme q4 sur la paire basse mais
   aucun `z_j` n'est même q2 intérieur sur la paire haute. Scinder vers une
   paire/microtile ou prouver un `BlockJungDiskDepth` uniforme.
2. **Frontière du shell.** `max H<=0` exclut l'intérieur q2 ; seul `max H<0`
   exclut aussi le shell. Pour la puissance q3/q4, `min P>=0` et `min P>0`
   jouent les deux rôles. `U_closed` conserve toute égalité.
3. **Non-cascade.** WST4 consomme la relation q3 géométrique pré-rang. La
   fixture u16 de 64 points porte q4 au rang 4 et ses dix sous-supports aux
   rangs 12.
4. **Source et preuve.** Garder une WSPD coarse canonique comme owner et un
   `ProofSpanDAG` lane-local qui raffine seulement le résiduel `MIXED`. Remplacer
   chaque `RectId` source par tous ses descendants recrée le front de dizaines
   de millions de terminaux observé.
5. **Sortie.** CK/WST borne des blocs physiques initiaux, pas la masse logique
   ni les plateaux. Toute expansion réelle reste preflightée, quotientée par
   une politique reçue ou refusée atomiquement.

### 9.1 Candidat exact pour `BlockJungDiskDepth`

Un verrou mathématique supplémentaire peut être levé sans promouvoir un test
de coins. Écrire `m=(a+b)/2`, `h=(b-a)/2`, `c=m+w`, avec `w dot h=0` et
`||w||<=kappa*||h||`. On a `kappa^2=1/2` pour q4 et `1/3` pour q3. Pour un
témoin `z`, la marge intérieure vaut :

```text
Phi_z(w)=||h||^2-||m-z||^2-2*w dot (m-z)
```

Par minimax sur le disque de `w` et le simplexe de poids `lambda_z`, un groupe
couvre la paire si et seulement s'il existe des poids rationnels non négatifs de
somme un tels que, avec
`alpha=||h||^2-sum lambda_z||m-z||^2` et
`p=m-sum lambda_z z`, les conditions suivantes tiennent strictement :

```text
q4 : alpha>0 et alpha^2 > 2*(||h||^2||p||^2-(p dot h)^2)
q3 : alpha>0 et 3*alpha^2 > 4*(||h||^2||p||^2-(p dot h)^2)
```

Helly redonne une base d'au plus trois IDs pour une paire. Pour un bloc, porter
ces IDs et poids dans `BlockJungDualTile`, puis vérifier les polynômes sur tout
`A×B` par une enveloppe Bernstein/SOS entière ; un verdict `MIXED` scinde
`A/B`. C'est une piste de classifieur uniforme, pas encore un théorème de coût.
Une borne initiale à coût constant utilise
`alpha=-a dot b+(a+b) dot zbar-qbar` et une enveloppe de `h cross p`. Pour un
témoin singleton, le dual redonne exactement les critères `SOC64` q3/q4 ; il
en est donc la généralisation collective.

La quantification doit rester explicite : `for all (a,b) exists lambda(a,b)`
n'implique pas l'existence d'un même `lambda` pour tout le rectangle. Une base
et des poids communs vérifiés uniformément sont donc un certificat `ALL` sûr,
mais incomplet ; leur échec reste `MIXED`. La rationalité vient de la densité
et de la marge stricte, sans borne automatique sur le dénominateur. Après un
cap de largeur ou de dénominateur, le seul retour permis est `UNKNOWN`.

Un test de coins reste insuffisant : avec trois points verticaux dans chacun de
`A` et `B`, aux ordonnées `0,50,100`, et les deux témoins `(50,0,0)` et
`(50,100,0)`, les quatre couples extrêmes sont couverts alors que la paire
médiane ne possède que deux témoins de shell. Le mutant `corners-only` doit
mourir.

### 9.2 Jung sur l'axe d'une face, puis profondeur du bloc q4

Pour une face aiguë fixe `f`, les centres q4 admissibles sont dans le segment
`J_f=axe(f) intersect K_4(ab)`. Avec le paramétrage de la section 8, écrire
`c_0=a+W/(2G)`, `m=(a+b)/2`, `h=(b-a)/2`. Alors
`J_f={tau:tau^2<=T_f}` avec
`T_f=4G*(||h||^2/2-||c_0-m||^2)>0`. Les bouts sont généralement
irrationnels : toute comparaison élevée au carré certifie d'abord son signe.
Un groupe de témoins couvre toute la face exactement lorsque :

```text
J_f intersect intersection_z {tau : A_z-tau*B_z>=0} = empty
```

L'ensemble de droite est un intervalle ; Helly en dimension un laisse au plus
deux IDs dans une base. Huit groupes de `PointId` disjoints donnent donc huit
intérieurs distincts pour tout centre de `J_f` et ferment toutes les extensions
q4 de la face avant apex/sweep. Une égalité de bornes est un shell et reste un
échec. Le reçu stocke les signes de `B`, les ratios extrêmes ou une comparaison
au bord de `J_f`; une version `FaceAxisJungDepth8Block` ne rend `ALL` qu'après
vérification uniforme sur `A×B×C`, sinon elle scinde.

Extraire successivement une paire compatible par un glouton arbitraire n'est
pas exact pour la profondeur. Une preuve de profondeur huit utilise soit la
récurrence leave-out, soit le maximum matching du graphe-chaîne des demi-droites.
Les fixtures permanentes incluent deux événements opposés égaux, qui ne donnent
aucun crédit au point shell, les trois cas constants `B=0`, et un cas où le
premier choix glouton détruit un matching de taille deux.

Après ajout d'une cellule apex, `BlockBallDepth8(A,B,C,D;G_j)` est encore plus
fort : il restreint le même axe au sous-intervalle réellement atteint par `D`
et certifie huit groupes sur toutes les BallForms du bloc. Le déterminant
in-sphere ponctuel tient dans `i128` sous u16, mais une AABB ne devient pas un
polytope levé par ses seuls coins ; il faut borner séparément `||p||^2`,
l'orientation et le signe, ou employer Bernstein/SOS exact. `ALL` ferme le
bloc, `MIXED` scinde et les égalités restent shell.

La hiérarchie proposée est ainsi : `BlockJungDual` sur l'arête, Jung 1D après
la porte aiguë, `BlockBallDepth8` après la cellule apex, puis seulement le
résiduel sous budget vers sweep ou fill. Elle généralise le même certificat
collectif de q2/q3 à q4 sans construire de mosaïque d'ordre supérieur.

## 10. Portes remises à Claude

Ordre recommandé :

1. publier le compteur live comme `C4_carrier_quadrature` et ses apex comme
   `M4_apex_quadrature`, jamais comme mesures ; séparer `OPEN_FINAL/PENDING` ;
2. si l'estimation reste utile, remplacer quantiles fixes et censure des grosses
   lentilles par le schéma emboîté de la section 3.1, avec oracle exhaustif
   réellement comparé à petit `n` ;
3. recevoir `CKPairTape-v0`, `D>0`, exact-once et identité de masse ;
4. recevoir `BlockJungDualTile` comme certificat `ALL` uniforme fail-open, puis
   `OwnedCK-WST3/AcuteCarrierBlock` dans l'enveloppe
   `2B_R`--lentille, avec `F3`, masses `ALL/NONE/MIXED`, pending et la famille
   à deux droites (`C4_carrier=0`) ;
5. recevoir `OwnedCK-WST4` symbolique avec `F4`, `M4_apex_L/U`, diagonales,
   owner des six arêtes, carrier primaire, puis `FaceAxisJungDepth8Block` avant
   toute face/apex matérialisée ;
6. ajouter `TetraPositiveBlock/BlockBallDepth8` sur WST4, avec huit groupes
   intérieurs disjoints, égalités shell et la fixture exacte à quatre sous-cubes
   positifs mais fermés par le cinquième ;
7. raccorder la sweep 1D seulement sur le résiduel qui justifie ce fallback,
   avec constantes `B_z=0`, lots égaux et largeur 155 bits ;
8. comparer alors baseline et SOC combiné jusqu'à
   `M4_apex/T4_site/BallKeys/H`, sous caps déterministes ;
9. aucune rampe G4 50000 avant passage des microgates et preflight du payload.

## 11. État du contre-audit live

La primitive SOC isolée passe `16/16` CTests ; les portes ponctuelles du cône
passent `39/39`. Le replay union `cred/ccred` est cohérent dans la source relue,
recompile et préserve les sorties baseline sur deux microgates avec et sans
raffinement. La somme réfutée surcompte effectivement les fermetures.

Le HEAD câble désormais cinq portes WSPD SOC, dont un shadow jugé et le témoin
de surcompte brut ; elles passent dans le rejeu courant. Il manque encore un
juge direct des flips distinct-ID, les mutants d'union/descendant/fallback et
le cap maximal. Le delta live corrige `SocStats.wide` en comptant trois ou
quatre produits source selon l'early exit ; ce n'est pas un compteur
d'instructions machine.

La sous-suite live `wspd_soc64|porteurs|two_lines` passe `15/16`. L'unique rouge
est causalement documentaire : le code imprime désormais `C4_estime=0`, tandis
que la regex `mhgp3v_two_lines_zero_porteur_aigu` attend encore
`M4_estime=0`. Les nouvelles portes dites « oracle porteurs » partagent encore
les lambdas géométriques, les terminaux et les fates du sujet ; le mutant peut
être sauté lorsque `m>--porteurs-oracle`. Elles ne constituent pas encore un
oracle PointId indépendant de `C4/M4`.

La formulation recevable est donc : **primitive `ALL` exacte, replay union
plausible et premières portes intégrées ; coût transitif, flips et source
C4/M4 non reçus**.

GCP non utilisé.
