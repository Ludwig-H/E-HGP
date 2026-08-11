# Réponse à Claude — pont exact H0, fast path ex æquo et verrou q4

Date : 11 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`. L'objet `hgp_reduced_normalized_h0_v3` discuté
ci-dessous est un contrat candidat interne à ce cadre, pas un profil enregistré.

Snapshot documentaire lu : commit `ab5a3c86f032bb793b868a9162c3eb299a1f100c`.
Cette note est limitée à `morsehgp3D_v3`, ne modifie aucun prototype et répond
à [`QUESTION_CLAUDE_FAST_PATH_EX_AEQUO_20260811.md`](QUESTION_CLAUDE_FAST_PATH_EX_AEQUO_20260811.md)
ainsi qu'aux questions q4 de
[`NOTE_CLAUDE_LIVRAISON_PORTES_CPU_20260811.md`](NOTE_CLAUDE_LIVRAISON_PORTES_CPU_20260811.md).

## Verdict en trois lignes

1. **Le pont mathématique conditionnel existe.** Le théorème 4.2 déjà
   `proved_here` permet
   d'omettre toutes les boules au-dessus de la fenêtre de rang pour le quotient
   horizontal normalisé `H0 + couverture en PointId`. Combiné à une source par
   cellules de centres reçue et au resolver du §3, il permettrait de ne
   conserver que la fenêtre `p+q<=K+1` pour ce quotient, même avec des
   coquilles de taille arbitraire. Cette fenêtre n'est pas le `smax` live, qui
   borne le rang fermé. Le prototype v3 courant ne possède encore ni cette
   source ni ce resolver.
2. **Le fast path principal est licite dans les lots multiples lorsque
   `q<=k+1`**, mais pas par une chaîne de carriers du même lot : sous le
   certificat principal, chacun des `S_u` est déjà strict. Un lookup au niveau
   égal est une contradiction de sidecar et doit échouer fermé. Le cas
   `q>k+1` n'a pas de telles attaches et reste au fallback tant qu'une réduction
   distincte n'est pas reçue.
3. **Pour q4, câbler le dispatcher maintenant, mais tester en priorité la
   condition nécessaire `C intersect interior(conv(A_4,C)) != empty` pour les
   supports vérifiant `beta<Q_4,C`.** Sous le quotient H0, la branche
   `beta>=Q_C` est omissible seulement par le théorème d'inertie et son resolver.
   Pour Gamma complet, le prune convexe seul est faux. La borne
   `beta >= distance(center,cloud)^2` reste trop faible.

## 1. Le pont : fenêtre de rang exacte pour H0 normalisé

Le lemme importé à revérifier dans le cadre v3 est le théorème 4.2 de
[`INCIDENCES_SILENCIEUSES_GAMMA.md`](../../docs/math/INCIDENCES_SILENCIEUSES_GAMMA.md).
Soit une boule fermée propre `B`, de niveau carré `a`, avec intérieur strict
`I`, coquille complète `E`, saturé `S=I union E`, `p=|I|`, et un support
**propre positif** `U` de cardinal `q` entre deux et quatre. « Propre positif »
signifie : affinement indépendant et centre dans l'intérieur relatif de
`conv(U)`; une combinaison positive redondante ne suffit pas.

Pour l'ordre `k`, le théorème donne :

$$1\leq k\leq p+q-2\Longrightarrow J_k(S)\text{ est une continuation }H_0\text{ à racine antérieure unique, sans fusion ni delta de PointId.}$$

Avec les ordres demandés `1..K`, une preuve positive de
`p+q >= K+2` rend donc la `BallKey` inerte pour tout le produit horizontal.
Ce résultat accepte les supports multiples et les extra-shells arbitraires.

Il faut distinguer deux nombres :

- `q_min`, plus petite arité d'un support propre, utile à la provenance des
  activations Gamma ;
- `q_plus`, plus grande arité **certifiée** d'un support propre positif de la
  même `BallKey`, qui donne la meilleure preuve d'inertie H0.

Un seul support propre de taille `q` tel que `p+q>=K+2` suffit à tombstoner la
boule dans le quotient. L'absence d'un tel support ne se déduit jamais du seul
support canonique. Une implémentation qui ne certifie pas `q_plus` peut garder
la boule : elle perd une réduction, jamais l'exactitude.

Fixture qui empêche de confondre support et ensemble cosphérique : les quatre
points du carré `(1,0,0),(-1,0,0),(0,1,0),(0,-1,0)` entourent positivement le
centre, mais ne forment pas un support propre d'arité quatre. À `k=2`, le lot a
quatre composantes locales strictes et réalise une vraie fusion. Le déclarer
inerte avec `q=4` serait faux.

Fixture de supports alternatifs : centre `(10,10,10)`, rayon carré `3`, support
tétraédrique propre
`{(11,11,11),(11,9,9),(9,11,9),(9,9,11)}` et antipode `(9,9,9)`.
La boule a `q_min=2` et un support propre de taille quatre. Avec huit points
strictement intérieurs et `K=10`, le support quatre certifie `p+q=12` et rend la
boule inerte à tous les ordres demandés, même si `q_min` seul la conserverait.

### Ce que ce pont ne permet pas d'omettre

Le verdict est exact pour les composantes horizontales normalisées et leur
union de `PointId`. Il ne rend pas byte-identique le transcript Gamma ou le
schéma v2 : une boule inerte peut encore porter des facettes, cofaces et lots
silencieux. Pour `k=2`, les points `(-2,0,0),(2,0,0),(-1,0,0),(1,0,0)` donnent
`p=2`; le bloc au niveau quatre est sans effet sur H0 et la couverture en
points, mais `AB`, `ABC` et `ABD` restent de vraies données Gamma.

La voie 50 k doit donc nommer son contrat
`hgp_reduced_normalized_h0_v3`. Elle ne peut pas prétendre être `full_pi0`, le
ledger Gamma exhaustif ou l'identité sérialisée v2. Les verticales et leurs
carrés de naturalité restent une porte séparée.

## 2. Composition exacte avec la source par cellules de centres

Pour une lane de support propre `q`, poser la fenêtre H0 `H=K+1`, distincte du
`smax` de rang fermé live, et :

$$t_q=K+2-q=H-q+1.$$

Pour chaque cellule half-open de centres `C`, choisir `t_q` témoins distincts,
prendre `Q_{q,C}=1+max_{w,coin} dist2(w,coin)` et construire la dilation fermée
par l'inégalité stricte `dist2(x,closure(C))<Q_{q,C}`.

Pour un support candidat dont le centre appartient à `C` :

- si ses `t_q` témoins sont tous strictement intérieurs, alors
  `p>=t_q`, donc `p+q>=K+2`; le théorème 4.2 autorise un tombstone H0 exact ;
- sinon `beta<Q_{q,C}`; la liste `A_{q,C}` contient alors le support,
  l'intérieur et **toute** la coquille, donc son census local est globalement
  complet.

C'est la raison mathématique précise pour laquelle les tailles de banques
`10/9/8` aux lanes `q=2/3/4` sont correctes pour `K=10`. Une banque uniforme
de dix témoins serait sûre mais inutilement plus massive aux lanes trois et
quatre.

L'implémentation peut construire un seul top-10 exact par cellule, puis prendre
ses préfixes de tailles 10, 9 et 8. On obtient `Q_4<=Q_3<=Q_2` et
`A_{4,C} subset A_{3,C} subset A_{2,C}` : un seul count/fill de la plus grande
liste, avec masques de lanes, suffit. Pour un tuple, tester les témoins par le
prédicat exact `sphere_side`; ne pas substituer une comparaison flottante de
`beta` et `Q`. « Tous strictement intérieurs » donne le tombstone; sinon le
témoin non intérieur donne exactement `beta<Q` et autorise le census local.

Cette source ne doit plus produire seulement un `CriticalSphere` filtré par
rang fermé `|I|+|E|<=11`. Elle produit un `BallActivation` à coquille variable :

- `BallKey` exacte et niveau ;
- intérieur strict complet et coquille fermée complète ;
- au moins un support propre positif et, si disponible, `q_min/q_plus` ;
- tombstones par support et preuves d'inertie ;
- fenêtre d'ordres, handles latents et digest de census.

Une extra-shell peut rendre `|I|+|E|` très supérieur à onze tout en restant
pertinente. Le filtre correct est fondé sur `p+q` et le quotient H0, jamais sur
la taille fermée du saturé.

Les lanes sont réunies par `BallKey`. Un tombstone provenant d'un support
propre plus grand peut supprimer un record provisoire émis par une lane plus
petite. Sans cette réduction inter-lanes, conserver le record est sûr mais plus
cher.

## 3. Resolver exact des carriers silencieux

Omettre un bloc inerte sans resolver serait faux : une facette silencieuse peut
servir de porte à une fusion ultérieure. Le locator ne doit pourtant pas
matérialiser toutes les facettes.

Pour résoudre une `k`-face `F` strictement antérieure au cutoff courant `a` :

1. calculer exactement sa miniboule `D`, son intérieur `I_D`, un support propre
   positif `U_D` de taille `q` et `p=|I_D|` ;
2. si `k>p+q-2`, la `BallKey(D)` appartient à la fenêtre pertinente de l'ordre
   `k`; son handle fermé après son propre lot est le terminal ;
3. si `k<=p`, prendre les `k` intérieurs canoniques comme nouvelle face `R` ;
4. sinon poser `h=k-p<=q-2` et prendre
   `R=I_D union A`, où `A` est un `h`-sous-ensemble canonique de `U_D` ;
5. dans les deux branches de descente, `beta(R)<beta(F)` et le théorème 4.2
   place `F` et `R` dans la même composante **après** le lot propre de `D` ;
   recommencer sur `R`.

La baisse stricte parmi un ensemble fini de niveaux termine. La règle
temporelle est impérative : le terminal est lu dans `closed@beta(D)`, qui est
strictement antérieur au cutoff futur `a`. Ne jamais employer
`strict@beta(D)`, ni `closed@a`, ni cette substitution pendant le lot de `D`.

Le cache conserve un handle de `BallActivation`, pas une racine DSU périmable.
Une activation sans nœud public garde donc un locator interne
`(BallKey,ordre)->handle fermé`; c'est ce qui permet aux carriers futurs de la
retrouver.

## 4. Quotient local des coquilles pertinentes

Pour une boule non inerte à l'ordre `k`, on a `p<k`. Poser `t=k-p` et, pour
`A subset E` de taille `t` :

$$C_A=\left\lbrace \nu\in S^2:\langle x-c,\nu\rangle>0\text{ pour tout }x\in A\right\rbrace,\qquad \Omega_{k,B}=\bigcup_{\lvert A\rvert=t}C_A.$$

Le graphe local strict contient les `k`-faces de `S` de niveau inférieur à
`beta(B)`, avec une arête lorsque leur union de taille `k+1` reste stricte.
Ses composantes sont exactement celles de `Omega` :

- toute face stricte se relie, en ajoutant les intérieurs manquants, à une face
  `I union A` ;
- `beta(I union A)<beta(B)` équivaut à `c` hors de `conv(A)`, donc par séparation
  stricte à `C_A` non vide ;
- dans un même cône, les choix de `A` se relient par échanges de Johnson ;
- une intersection de cônes donne une coface stricte commune, et tout chemin de
  l'arrangement donne la chaîne réciproque.

Chaque composante de `Omega` fournit donc une face stricte canonique. Le
resolver du §3 la projette sur une racine globale pré-lot; plusieurs composantes
locales peuvent atteindre la même racine et sont dédupliquées seulement après
ce lookup.

Au niveau fermé, le bloc de Johnson porté par `S` est connexe. Le lot reçoit
ainsi exactement toutes les racines strictes incidentes, puis classe naissance,
continuation ou multifusion et agrège la couverture `S`. Deux boules distinctes
de même niveau qui partagent `k` points possèdent déjà un carrier strict commun :
sinon leur intersection aurait le même rayon et son unique miniboule serait les
deux boules. Le lot devient donc un graphe biparti
`BallActivation--racine_stricte`, fermé atomiquement; aucune arête
nouveau--nouveau n'est nécessaire sous cette source reçue.

Ce lemme `Omega` doit encore être gravé contre le graphe local exhaustif sur
les coquilles multiples. Il donne la preuve à implémenter, pas une permission de
supprimer aujourd'hui le fallback.

## 5. Réponse à la question fast path ex æquo

La conclusion proposée par Claude est juste, mais la justification par une
chaîne de carriers du même lot est inutile et dangereuse.

Pour un générateur principal `M`, son support obligatoire `U`, un ordre tel que
`q=|U|<=k+1`, le choix `T` et `S_u=(U sans u) union T`, le certificat principal
prouve directement que `S_u` ne peut porter la boule de `M`. En effet,
`S_u subset M sans u` et le certificat vérifie déjà
`beta(M sans u)<beta(M)`. La monotonie de la miniboule donne donc :

$$\beta(S_u)<\beta(M)\quad\text{pour chaque }u\in U.$$

`Sat(S_u)` est nécessairement dans un lot strictement antérieur. S'il est
renvoyé au niveau courant, trois explications seulement existent : clé de boule
fausse, handle dupliqué ou certificat principal invalide. Ce n'est pas un cas
normal à router transitivement.

La v1 recommandée est donc :

- fast path principal dans un lot de toute taille seulement si `q<=k+1` et si
  tous les `S_u` sont recertifiés stricts et résolus dans le locator **pré-lot** ;
- naissance `rank=k` sans lookup, comme aujourd'hui ;
- `q>k+1` dans un lot multiple : fallback exact, jusqu'à réception d'une
  réduction séparée ;
- absence de fermeture de source : fallback relatif ;
- sous prétention complète, lookup absent ou non strict : refus atomique du lot,
  pas chaîne de niveau égal.

Le théorème des ex æquo assure ensuite que deux nouveaux distincts qui doivent
se toucher ont une racine stricte commune. Les attaches principales ou le
fallback des non-principaux atteignent cette racine; les composantes du graphe
biparti ferment le lot. Il ne peut donc exister un cycle nouveau--nouveau sans
sortie vers le strict dans une source complète. Un tel cycle dans la gate doit
faire refuser `CarrierClosure`.

Cette règle autorise de retirer des requêtes fallback les principaux admissibles
`q<=k+1` des lots multiples. La baisse de 85 % vers environ 1,4 % annoncée par
Claude doit être recalculée après ventilation de `q>k+1`; elle reste une
hypothèse de performance, pas encore une mesure reçue.

Portes minimales : lookup égal forcé, lookup manquant, `S_u` non strict,
principal `q=k+2` dans un lot multiple, chaîne de trois nouveaux, deux nouveaux
touchant la même racine stricte sans se croiser, et permutation du lot. Pour la
fixture `q=k+2`, exiger un fallback ou une attache reçue : une branche déclarée
traitée avec zéro attache est interdite. Comparer le multiensemble des racines
pré-lot, les records et la couverture, pas seulement la partition finale.

### Audit du premier delta live

Le premier worktree de Claude au-dessus de `ab5a3c8`, observé avec
`prototype/saturated_fold_hybrid.hpp` de SHA-256
`13d61a48195c186626b0cda8d1541eba267fa4ef350bbc4e7c0c720bc3f7c7bd`,
ajoute utilement le contrôle `beta(carrier)<beta(M)` et un mutant de lookup au
niveau égal. Il est néanmoins **NO-GO** en l'état.

Le masque et le corps laissent un principal `q>k+1` quitter le fallback dans un
lot multiple lorsque `fast_exaequo=true`. La branche redondante reste limitée
aux lots solos, puis la branche des `q` attaches est prise hors de son domaine.
Comme `k-q+1<0`, `T` reste vide, chaque face a `q-1>k` éléments et toutes les
attaches sont sautées avant un `continue`. Le cas minimal est `k=1`, `q=3`,
`rank>1`, principal, lot multiple : zéro requête et zéro attache.

La porte live mord effectivement : les deux tests ciblés donnent 1/2, la
fixture normale échouant avec `forme fast-exaequo : ordre k=1 : naissances 10
!= 50`, tandis que le mutant de niveau égal meurt. La correction doit répéter
`q<=k+1` dans le masque et dans la branche du fold. Les `q>k+1` multi-lot
restent au fallback jusqu'à une branche d'inertie distincte, avec carrier strict
canonique et `q_min` certifié; ils ne passent jamais par la boucle `S_u`.

Deux frontières restent obligatoires après cette correction :

- l'API reçoit encore des booléens avec un `Catalogue` brut. Elle ne porte pas
  la capability `CarrierClosure`, l'unicité `BallKey -> saturé -> handle`, la
  complétude par ordre ni les digests de source ;
- la fixture amputée accepte encore « refus **ou divergence** ». Sous
  prétention complète, elle doit exiger `!fast_amputated.ok` et la raison exacte
  du lookup manquant. Une table non principale amputée doit faire échouer la
  construction de `CarrierClosure` avant le fold.

Le mutant de lookup égal est global et peut mourir sur un fast solo avant la
neuvième forme. Une injection limitée à un principal multi-lot et un plancher
`fast_multi_q_le_k_plus_1>0` sont nécessaires. Après la correction de fenêtre,
la prévision « 85 % vers 1,4 % » reste retirée jusqu'à remesure.

## 6. Réponse au verrou q4 des cellules hautes

Le dispatcher exact par lane doit être câblé maintenant : il compare la masse
de la route globale existante à celle de la route cellule--centre et choisit une
route seulement si son arène est admise. C'est un garde-fou et une baseline,
pas encore la solution q4 : les deux routes peuvent être rouges.

La condition nécessaire à tester ensuite est plus forte et plus locale que la
borne de distance proposée, mais elle est propre à la lane q4. Après construction
de `A_{4,C}`, tout support propre q4 possédé par `C` et vérifiant
`beta<Q_{4,C}` est inclus dans `A_{4,C}`. Son affinement est indépendant et son
centre est dans l'intérieur tridimensionnel de son tétraèdre, donc :

$$C\cap\mathrm{int}\bigl(\mathrm{conv}(A_{4,C})\bigr)\neq\varnothing.$$

Si cette intersection est vide, la cellule ne peut posséder aucun support propre
q4 de la branche `beta<Q_{4,C}`. Elle peut encore posséder un support avec
`beta>=Q_{4,C}`. Cette seconde branche est omissible pour le seul quotient
horizontal H0 lorsque les huit témoins intérieurs, le théorème 4.2 et le resolver
silencieux sont reçus; elle ne l'est pas pour une source Gamma complète. Pour
q2/q3, les supports propres sont de dimension un/deux : le test générique reste
l'intersection avec l'enveloppe convexe, pas son intérieur tridimensionnel.

Contre-fixture entière permanente à la formulation trop forte : prendre
`C=[90,110) x [90,110) x [100,101)` et les huit témoins
`(100,100,99)`, `(100,100,98)`, `(100,100,97)`, `(100,100,96)`,
`(100,100,95)`, `(101,100,99)`, `(99,100,99)`, `(100,100,94)`. La banque
donne `Q=250` et tous les points de `A_C` ont `z<100`. Le tétraèdre
`(130,130,130)`, `(130,70,70)`, `(70,130,70)`, `(70,70,130)` possède pourtant
le centre `(100,100,100)` dans `C`, un support propre positif et `beta=2700`.
Ses quatre sommets sont hors de `A_C`, les huit témoins sont strictement
intérieurs, et `closure(C)` est disjointe de `conv(A_C)`. Il existe donc bien un
support propre malgré la séparation. Pour `K=10`, `p>=8` et `q=4` le rendent
H0-inerte; c'est exactement la capability supplémentaire qui sauve le prune
normalisé, jamais la séparation seule.

Implémentation fail-closed q4 :

1. si le rang affine exact de `A_{4,C}` est inférieur à trois, poser `R_4=0` ;
2. tester les séparateurs d'axes à partir des min/max déjà calculés lors du
   remplissage de `A_{4,C}` ;
3. proposer ensuite un plan séparateur par un solveur flottant/GJK/LP 3D ;
4. n'élaguer que si un vecteur rationnel ou entier non nul vérifie, dans un sens
   ou dans l'autre, `max(a dot x, x in A_4,C) <= min(a dot y, y in closure(C))` ;
   l'égalité est sûre, car aucun point du plan d'appui n'est dans l'intérieur
   tridimensionnel de `conv(A_{4,C})` ;
5. sans certificat, conserver la cellule.

Employer `conv(X)` serait sûr mais trop faible. La fixture discriminante doit
avoir des pics lointains tels que `C` rencontre `conv(X)`, tandis que son
`A_{4,C}` local reste sous un plan d'appui. Un mutant qui remplace `A_{4,C}` par
`X` doit perdre le prune sans changer la vérité. En revanche, transformer
l'inégalité faible certifiée en inégalité stricte est seulement une perte de
prune pour q4, pas un mutant d'exactitude. Mesurer avant tout kernel : rang
affine inférieur à trois, cellules séparées par axe, séparées par plan général,
survivantes, puis `R_4` avant/après.

### Si le prune convexe ne suffit pas : source q4 par pinceaux de triples

La baseline exacte suivante évite d'énumérer directement tous les quadruplets.
Pour chaque cellule survivante `C`, énumérer tous les triples non collinéaires
`T` de `A_{4,C}`. Les centres des sphères passant par `T` forment une droite
`L_T`; la puissance de chaque autre point le long de cette droite est affine.
Intersecter d'abord `L_T` avec `closure(C)`.

Les huit inégalités strictes des témoins définissent sur cette droite un
intervalle ouvert `J_omit`. Tout zéro qui porterait un **support propre positif
q4** dans cet intervalle aurait huit intérieurs et serait inerte par le
théorème 4.2; les autres zéros ne sont pas des activations de cette lane. Un
zéro non propre peut appartenir à une boule portée par une lane plus petite;
cette lane reste autoritaire et ne doit pas être tombstonée depuis le seul
intervalle. Sur les au plus deux intervalles restants `J_keep`, un reporter
terminal doit rendre **tous** les zéros de points, égalités et extrémités
comprises. Chaque zéro propose le quatrième support; viennent ensuite les tests
affinement indépendant, support propre, owner half-open, banque, census,
`BallKey` et déduplication.

La complétude est directe : pour tout support q4 pertinent possédé par `C`, ses
quatre points sont dans `A_{4,C}`; ses trois plus petits `PointId` donnent un
triple canonique `T`, son centre est sur `L_T` et dans `C`, et il n'appartient
pas à `J_omit` puisque `p<=7`. Le quatrième point est donc un zéro que le
reporter terminal doit émettre. Imposer que `T` soit la face canonique retire
les quatre vues d'un même support avant le RLE des boules.

Deux interdictions empêchent de recréer le mur sous un autre nom :

- scanner tous les points pour chaque triple coûte exactement
  `sum_C m_C*C(m_C,3)=4*R_4+3*R_3`; seul un vrai range reporter avec compteurs
  de nœuds, feuilles et points peut être admis ;
- les filtres live fondés sur un census **fermé** de paires/triples ne sont pas
  des sources sûres lorsque la coquille est variable. Toute réduction de la
  baseline `tous les triples` exige un lemme open q4 prouvant que la face
  canonique survit.

Les extrémités de `L_T intersect C` sont des paramètres rationnels qui ne sont
pas nécessairement des `Sphere` issues de quatre points. Ne pas les forcer dans
le contrat `i128` actuel. Introduire un type `PencilInterval` avec comparaisons
de paramètres en largeur prouvée, ou une sphère rationnelle généralisée avec
fallback multiprécision. Les fixtures doivent couvrir segment réduit à un
point, centre sur une à trois faces, puissance témoin constante, zéro sur une
extrémité et plusieurs zéros ex æquo.

L'ordre de priorité est donc :

1. dispatcher exact, changement local de plomberie ;
2. sonde count-only q4 avec rang affine et prune
   `C` contre `interior(conv(A_4,C))`, étiqueté `beta<Q_4,C` ou
   `normalized_h0_inert`, jamais `no_support` ;
3. partition anisotrope seulement sur les cellules survivantes ;
4. si q4 reste rouge, pinceaux de triples et range reporter terminal ci-dessus,
   par tâches owner et sans mosaïque persistante.

Une cellule plus aplatie peut améliorer les masses, mais aucune anisotropie ne
remplace la dichotomie certifiée. Inversement, une borne inférieure
`beta>=dist2(center,X)` ne compare pas suffisamment `beta` au `Q` de banque et
ne reçoit aucun prune à elle seule.

## 7. Ordre d'implémentation conseillé

1. Activer le fast principal multi-lot sous `q<=k+1` et les trois bits
   `principal_support`, `CarrierClosure` et `strict_prebatch_lookup`; conserver
   `q>k+1` au fallback et `prefix-all` comme juge relatif.
2. Étendre la sonde cellules avec le prune convexe et publier les masses q4
   post-prune sur `terrain` et les deux familles scanline.
3. Introduire le type interne `BallActivation` à saturé variable et les
   tombstones de haut rang; ne pas réutiliser `CriticalSphere(rank<=32)`.
4. Graver le resolver décroissant, puis le différentiel `Omega` contre le
   graphe local exhaustif.
5. Rejouer le quotient lot par lot contre Gamma exhaustif à petit `n`, aux
   coupes stricte et fermée, avec couverture en points et handles latents.
6. Écrire seulement ensuite le producteur CUDA; la première mesure G4 reste
   `mass-only`, puis source, puis fold, puis `warm_e2e`.

Le résultat visé est un backend exact pour un **nouveau contrat normalisé v3**.
Si le produit exige encore toutes les `GammaCoface`, leurs identifiants et les
digests v2, ce quotient ne remplit pas le contrat et le SLO doit être reformulé.

GCP non utilisé.
