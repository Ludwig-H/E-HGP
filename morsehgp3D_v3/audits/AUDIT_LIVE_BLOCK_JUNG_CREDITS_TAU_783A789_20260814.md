# Contre-audit live : `BlockJungDual64`, crédits d'identité et profondeur par transversal

Date : 14 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

> [!CAUTION]
> Ce rapport conserve le snapshot historique `783a789` et le premier delta
> live. Le commit `5809bd2` a depuis absorbé le packing réparé et ses trois
> CTests. Les défauts de juge partiel et d'options vacuaires décrits ici restent
> vrais au pin `5809bd2`; un worktree ultérieur tente de les réparer sans être
> encore repinné. Le verdict courant et le lemme de miniboule unique sont dans
> [`AUDIT_ETAT_COURANT.md`](AUDIT_ETAT_COURANT.md) et
> [`AUDIT_MINIBOULE_UNIQUE_RESIDUEL_SHALLOW_5809BD2_20260814.md`](AUDIT_MINIBOULE_UNIQUE_RESIDUEL_SHALLOW_5809BD2_20260814.md).

## 0. Snapshot et verdict court

Le pin de code relu est `HEAD=783a78934fef97b2c2836ee49c6f03ddd18d2e08`.
Le worktree est mouvant : Claude modifie concurremment `CMakeLists.txt` et
`prototype/wspd_wavefront_probe.cpp`. Les hashes intermédiaires observés entre
`11:30` et `11:39 UTC` ne sont pas des autorités de réception. Le premier delta
ajoutait directement des crédits de groupes ; le delta courant exclut les
feuilles déjà créditées, rend les groupes deux à deux disjoints, conserve leurs
indices, ajoute deux mutants et appelle le juge primal. Ce rapport audite donc
le contrat mathématique et les fautes à tuer, pas une version finale attribuée
à Claude.

Verdict : la primitive `BlockJungDual64` du `HEAD` est mathématiquement sûre
pour **une base pondérée fixe**, mais le premier raccord live de ses crédits
était faux. Le raccord courant répare ce défaut comme packing disjoint et ses
trois nouvelles CTests passent. Il n'est pas encore reçu : un cap de juge
insuffisant laisse des claims non jugés tout en publiant `fenetre_finale=OUI`,
`OK` et le code zéro ; les options peuvent aussi être vacuaires. Pour atteindre
la vraie profondeur et éviter un coût prohibitif, la route recommandée reste de
garder les bases comme hyperarêtes et de fermer sur `tau(F)>=8`, avec recherche
bornée et génération par coupes.

Le contrat G4 reste entièrement ouvert. La dernière session G4 du 14 août n'a
pas atteint sa rampe : elle a échoué pendant CTest, puis la cible précise a été
certifiée `TERMINATED`. Aucun chiffre de cette session ne mesure 50 000 points.

## 1. Ce que `BlockJungDual64` prouve réellement

Pour une base de témoins `z` et des poids positifs **communs à toute la tuile**,
poser `L=sum w`, `Z=sum w z`, `Q=sum w ||z||^2`,
`A0=-L(a dot b)+(a+b) dot Z-Q` et
`C0=L(a cross b)-a cross Z-Z cross b`. La lane q4 est certifiée par
`A0>0 && 2*A0^2>||C0||^2`; la lane q3 remplace `2` par `3`.

À `b` fixé, `(A0,C0)` est affine en `a`, et réciproquement. L'intérieur du
cône de Lorentz correspondant est convexe. Les 64 couples de coins d'un
produit de deux AABB sont donc nécessaires et suffisants pour la même base et
les mêmes poids sur **l'enveloppe AABB continue**. Reproposer des poids à chaque
coin détruirait cette quantification.

Cette preuve reçoit mathématiquement un certificat suffisant `ALL`. Au commit,
elle ne reçoit pas encore :

- un proposant complet de poids pour les groupes de deux ou trois IDs ;
- la profondeur d'une tuile ;
- un wrapper persistant par vrais `PointId` pour les deux ledgers ;
- un chemin device ;
- un gain de coût transitif jusqu'au payload.

Les tests du `HEAD` vérifient l'identité avec la forme ponctuelle et des boîtes
réduites à des points. Ils ne constituent pas encore un oracle indépendant sur
de petits produits AABB non dégénérés.

Deux réserves d'ABI restent bloquantes. `make_base` additionne bien les poids en
`i128` et refuse `L>65535`, mais `dual_lane` les additionne encore en `i64` sans
ce cap. Surtout, `bjd_lane_box` rend actuellement `kLaneNone` lorsque la base est
invalide. Le callsite live ne lit que `retour>=q4`, donc cette invalidité échoue
ouvertement aujourd'hui ; l'ABI typée permettrait néanmoins à un futur
consommateur de confondre « base invalide » et verdict géométrique exact `NONE`.
La réception exige un type séparé `ALL_GROUP/MIXED/INVALID_OR_UNKNOWN`, ou une
valeur `UNKNOWN` explicite pour toute invalidité ; `NONE` reste réservé à une
réfutation réellement calculée.

Le commentaire de `prototype/jung_dual.hpp` inverse aussi le minimax. La bonne
identité est `min_w max_z Phi_z(w) = max_lambda min_w sum_z lambda_z Phi_z(w)`,
pas « maximum sur le disque du minimum sur le groupe ». Les coefficients codés
sont cohérents avec la bonne identité : c'est une faute de documentation de
preuve, pas une réfutation de la primitive.

## 2. Réfutation du premier raccord live

Le premier delta observé collectait jusqu'à 24 feuilles, cherchait des paires
`BlockJungDual64`, marquait seulement les membres des groupes déjà retenus, puis
faisait `++cred[2]` et `++ccred[2]`. Il n'excluait pas les `PointId` déjà
crédités comme témoins universels par le central, le spindle ou `SOC64`.

Or une unité de groupe signifie seulement : pour chaque centre admissible, au
moins un membre du groupe est intérieur. Elle ne peut s'ajouter à des crédits
singleton que si son support est disjoint de tous les singletons déjà imposés.
Un compteur ne prouve pas cette union.

### Fixture permanente `seven_collinear_plus_reused_pair`

Prendre `a=(0,0,0)`, `b=(10,0,0)` et les sept `PointId` distincts
`z_i=(i,0,0)`, `1<=i<=7`. Pour tout centre `c=(5,u,v)` d'une sphère passant
par `a,b`, la différence entre le rayon carré et la distance carrée à `z_i`
vaut `i*(10-i)>0`. Les sept témoins sont donc universels, et la vraie profondeur
du pool vaut exactement sept.

Après sept crédits singleton, toute paire de ces mêmes témoins est encore une
base couvrante. L'ancienne addition scalaire publiait huit crédits sans huitième
identité. La fixture doit fermer à sept dans le chemin sain et tuer :

- `bjd-reutilise-temoin` : un groupe réemploie un singleton crédité ;
- `bjd-groupes-chevauchants` : deux groupes partagent un `PointId` ;
- `bjd-meme-point-deux-fois` : deux positions de banque désignent le même ID.

Cette fixture est exigée par le présent audit mais n'est pas encore implémentée
dans le delta observé. Les nouveaux tests statistiques `eight_clusters` ne la
remplacent pas.

Le correctif courant exige maintenant que la feuille soit libre dans les deux
vues et que les groupes soient deux à deux disjoints. Ajouter ensuite la même
base à `cred` et `ccred` est donc sûr dans cette version, mais conservateur :
des pools séparés seraient nécessaires pour exploiter un crédit propre à une
seule vue. Le delta compare encore des rangs du tableau `sp` et ne sérialise pas
un reçu de vrais `PointId`/proof-spans. L'unicité des rangs suffit dans ce
probe, pas pour l'ABI produit.

### Fixture source u16, pas seulement un ledger abstrait

Une version qui mord directement la source q4 prend les `PointId` ordonnés
`a,b,p,q` aux positions suivantes :

```text
a=(0,1000,1000)       b=(1000,1000,1000)
p=(500,300,1500)      q=(500,300,500)
c=(500,650,1000)      R2=372500
```

Les quatre vecteurs relatifs à `c` ont tous la norme carrée `372500` et leur
somme est nulle : le centre a les poids stricts `1/4`. Les arêtes `ab` et `pq`
ont longueur carrée `1000000`, les quatre autres `990000`; le tie-break
`EdgeKey` donne donc l'owner `ab`. Ajouter six témoins distincts
`x_i=(497+i,1000,1000)`, `0<=i<6`. Ils sont sur la corde ouverte `ab`, donc
strictement intérieurs à toute sphère par `a,b`, et en particulier à la sphère
du support. Elle a exactement `I=6` et `U=4` dans ce nuage.

Deux groupes disjoints, par exemple `{x_0,x_1}` et `{x_2,x_3}`, sont chacun
couvrants mais n'ajoutent aucune identité aux six singletons déjà crédités. La
somme fautive annonce huit et élimine un q4 régulier ; l'hypergraphe formé des
six arêtes singleton et de ces deux arêtes a toujours `tau=6`. Cette fixture
doit devenir une fonction permanente, pas rester remplacée par les comptes
d'une famille pseudo-aléatoire.

### État du correctif live observé

Après exclusion des feuilles déjà créditées et disjonction des groupes, les
trois nouvelles portes `mhgp3v_bjd_*` passent localement : nominal sans faux,
mutant de réutilisation tué et mutant de chevauchement tué, avec les codes
attendus. Le replay consolidé `SOC/Jung/BJD` du worktree donne `10/10` en
`1,21 s` sur la machine courante ; ce temps n'est pas un invariant. C'est une
réparation causale du packing, pas encore de `Depth=tau(F)`.

Le statut de juge reste faux sous cap. La commande :

```text
build/v3/mhgp3v_wspd_wavefront_probe --family=eight_clusters --points=200 --sep-euclid=8/1 --tight --vwave --window=512 --window-ledger --soc64-actif --bjd-groupes=8 --juge-bjd=1
```

rend code zéro et `OK` avec `groupes=158`, `sautes=98`, puis
`fermetures juges=7` et `sautes=10`. Un reçu partiel doit publier
`PARTIEL/UNKNOWN` et un code non nul dans une gate qui exige la complétude. De
même, `--bjd-groupes=8` sans `--vwave` rend code zéro avec `essais=0` et
`couvrants=0`; le preflight d'options doit refuser les modes vacuaires.

## 3. Réparation minimale sûre : packing candidat

Pour conserver provisoirement la sémantique scalaire, chaque vue maintient :

- les intervalles/IDs crédités comme singletons universels ;
- les supports exacts des groupes retenus ;
- un test d'appartenance par vrai `PointId`, jamais par position de banque ;
- le digest du reçu de bloc et des poids.

Un groupe ajoute un crédit seulement s'il est disjoint de l'union entière de
cette vue. Un `ALL` sur un nœud interne consomme toute sa plage de `PointId` :
ne pas descendre vers ses feuilles ne les rend pas libres. Les endpoints
`a,b`, les doublons d'identité et la politique des positions dupliquées sont
également exclus explicitement.

Un rang Morton est actuellement une identité injective à l'intérieur du run
parce que le probe rejette les positions dupliquées. Il ne devient pas pour
autant un `PointId` persistant. Le reçu, les digests et les mutants de
permutation doivent porter `spid` ou l'identité publique authentifiée.

Le filtrage `D=0` ne permet pas de perdre une multiplicité de témoins : une
implémentation peut bucketiser une position géométrique, mais conserve tous les
`PointId` et leur multiplicité dans les pools et produits. Seule la paire
endpoint dégénérée est omise.

Cette réparation prouve seulement un packing `nu(F)`. Elle est sûre parce que
`nu(F)<=tau(F)<=Depth`, mais elle reste incomplète. La fixture `u<p<d` déjà
documentée interdit de confondre échec du packing et profondeur insuffisante.

## 4. Réparation recommandée : hypergraphe et `tau(F)`

Soit `F` une famille de groupes de un à trois vrais `PointId`, chacun muni d'un
reçu uniforme prouvant que ses intérieurs couvrent tout le domaine de centres
de la proof-tile. Pour tout centre, l'ensemble des témoins intérieurs frappe
chaque arête de `F`. Ainsi `tau(F)` minore la profondeur réelle. Fermer dès
`tau(F)>=h` est donc sûr même si `F` est incomplet.

Pour une **paire d'endpoints fixe**, si `F` contient toutes les bases de Helly
couvrantes du pool sur son domaine convexe bidimensionnel, l'égalité
`Depth=tau(F)` suit dans les deux sens :

1. les intérieurs de tout centre forment un transversal, donc
   `tau(F)<=Depth` ;
2. si `T` frappe toutes les bases, les régions mauvaises des IDs hors `T` ont
   une intersection commune par Helly, donc un centre possède au plus `|T|`
   intérieurs et `Depth<=tau(F)`.

Cette égalité ne se transfère pas automatiquement à une proof-tile dont les
endpoints varient. Pour une famille de bases reçues uniformément sur la tuile,
seule `tau(F)<=Depth` est générale et suffit à fermer. Cette écriture traite correctement les recouvrements. Sept arêtes singleton et
une paire formée de deux de leurs IDs gardent `tau(F)=7` : la fausse huitième
unité disparaît sans règle spéciale.

### Solveur borné

Pour q4, décider `tau(F)>=8` revient à réfuter l'existence d'un transversal de
taille sept. Choisir une arête non frappée et brancher sur chacun de ses au plus
trois sommets donne au plus `3^7=2187` feuilles avant mémoïsation. Pour q3,
`h=9` donne au plus `3^8=6561`. Des bitsets sur un pool capé suffisent ; aucune
géométrie n'est rejouée dans ce solveur.

Les singletons universels forment un ensemble obligatoire `U`. Tant que la
lane n'est pas fermée, `|U|<h`; il suffit donc de conserver au plus `h-1` vrais
IDs. Toute hyperarête rencontrant `U` est déjà frappée et peut être retirée ; le
solveur teste alors `tau(F_res)>=h-|U|`. Un gros nœud `ALL` ferme immédiatement
si sa population distincte atteint le seuil.

## 5. Génération par coupes, pas balayage de toutes les paires

Avec un pool de 24 feuilles, un seul tour naïf teste `binom(24,2)=276` paires.
Huit recherches gloutonnes successives peuvent retester jusqu'à
`276+231+190+153+120+91+66+45=1172` paires. Chaque appel de bloc paie un centre
et jusqu'à 64 couples de coins, soit jusqu'à 76 180 appels au prédicat ponctuel
BJD par terminal. Le chemin q4 positif représente de l'ordre de 2,74 millions
de multiplications larges par terminal, estimation non instrumentée qui dépend
du précalcul de la base. Placé **après** la descente complète, ce travail n'économise aucune
lecture du front. Une baisse de `E4` dans cette ordonnance peut donc augmenter
fortement `warm_e2e`.

La boucle candidate recommandée est :

```text
U = singletons universels reçus ; F = hyperarêtes uniformes reçues
répéter sous un cap explicite :
  si tau(F_res) >= h-|U| : ALL_DEPTH
  sinon extraire un transversal R de taille < h-|U|
  résoudre exactement l'HPI de P minus (U union R) sur une paire représentante
  si contre-centre : R réfute la profondeur de cette paire et guide le split
  si intersection vide : extraire une base de Helly G minimale, |G|<=3
  vérifier G et ses poids par BlockJungDual64 sur toute la tuile
  si ALL uniforme : ajouter l'hyperarête à F ; sinon split/UNKNOWN, jamais NONE
```

Le proposant primal doit rendre une base active de taille au plus trois et un
reçu de poids vérifiable. `BlockJungDual64` n'est exécuté que sur les bases que
la séparation demande, non sur toutes les paires du pool. Un cache clé par
`(RectId,SupportKey,WeightKey,lane)` évite les rejeux entre q3/q4 et entre
enfants qui héritent d'un reçu valide.

Tant que `F` ne contient que singletons et paires, le solveur q4 branche sur au
plus deux sommets et descend au plus `2^7=128` feuilles. Une base ternaire
n'entre qu'après minimisation du conflit : ses trois sous-paires doivent être
testées. À un contre-centre provisoire, une requête nearest-neighbor AABB exacte
sur `P minus (U union R)` ne fournit qu'une contrainte violée ; elle accélère
l'HPI mais ne constitue pas une hyperarête. La boucle continue jusqu'à un vrai
contre-centre certifié ou une base de Helly, puis seulement BJD vérifie cette
base uniformément. Cap ou requête indécise donne `UNKNOWN/split`.

### Préfiltre exact `BJD-BilinearBounds`

Une base fixe admet un préfiltre beaucoup moins cher que 64 prédicats complets.
Pour chaque axe `i`, poser `g_i(x,y)=-L*x*y+Z_i*(x+y)`. Ses extrema sur
`A_i×B_i` sont aux quatre coins et les trois axes sont indépendants ;
`Amin=sum_i min(g_i)-Q` est donc le **vrai** minimum de `A0` sur `A×B`.

Pour une permutation cyclique `(i,p,q)`, poser
`f_pq(x,y)=L*x*y-x*Z_q-Z_p*y`. Alors
`C_i=f_pq(a_p,b_q)-f_qp(a_q,b_p)`. Les deux couples de variables sont
disjoints, donc les bornes coordonnées exactes sont
`Ci_lo=min(f_pq)-max(f_qp)` et `Ci_hi=max(f_pq)-min(f_qp)`, avec quatre coins
par extrema. Si `Mi=max(abs(Ci_lo),abs(Ci_hi))` et
`Nmax=sum_i Mi^2`, les implications sûres sont :

```text
Amin > 0 et 2*Amin^2 > Nmax  -> ALL q4
Amin > 0 et 3*Amin^2 > Nmax  -> ALL q3
sinon                         -> fallback exact 64 coins ou MIXED
```

`Amin<=0` réfute seulement l'universalité de **cette base et ces poids** ; il ne
donne jamais `NONE` pour la géométrie ni pour une autre pondération. Le filtre
paie 36 valeurs bilinéaires scalaires et peut éviter jusqu'à 64 formations
complètes de `(A0,C0)`. La perte de corrélation entre les trois composantes de
`C0` ne crée que des faux `MIXED`. Préconditions : u16 et `lo<=hi` vérifiés,
`1<=L<=65535` accumulé large, mêmes IDs/poids sur la tuile, widening avant tout
produit et comparaison stricte. Les mutants obligatoires remplacent `Amin` par
`Amax`, la somme des `Mi^2` par leur maximum, les quatre coins par la diagonale,
ou `>` par `>=`; un oracle petit-produit exige toujours
`FAST_ALL => BJD64_ALL`.

Le fallback 64 coins ne doit pas reformer 64 dot/cross complets. Comme
`(A0,C0)` est bilinéaire dans les six coordonnées d'endpoint, précalculer en
`i128` sa valeur au coin bas, les six différences premières et les neuf
différences mixtes `Delta a_i Delta b_j`. Chaque couple de coins se reconstruit
ensuite par additions exactes, avant les seuls carrés/comparaisons de cône. Une
parité bit-à-bit avec l'évaluation directe, y compris aux égalités, reçoit cette
optimisation ; ce n'est pas une nouvelle approximation.

Pour une base de deux IDs, le proposant peut lui aussi éviter une banque
arbitraire. Avec `lambda` comme poids du premier témoin, chaque couple de coins
préimage l'intérieur du cône de Lorentz par une droite affine : il donne un
intervalle ouvert de `lambda`. Intersecter exactement les 64 intervalles avec
`(0,1)`, puis choisir par Stern--Brocot un rationnel réduit de dénominateur au
plus 65 535, propose le poids commun ; le replay BJD reste l'autorité. La fixture
`a=(0,0,0), b=(6,0,0), z1=(1,0,1), z2=(2,2,0)` exige notamment un poids `3:1`
et tue le seul essai `(1,1)`. Pour trois IDs, la même recherche vit dans un
convexe de dimension deux et reste un proposant capé, jamais un nouvel oracle.

Pour agir sur le temps, cette porte doit précéder la multiplication
carrier--apex et, si possible, arrêter une proof-tile avant la descente complète.
La brancher après le front ne constitue qu'une ablation de couverture.

Le verrou physique historique est le replay témoin depuis chaque rectangle, pas
la construction de l'arbre. Une route d'implémentation à falsifier est une vague
persistante jointe `(PairBlock,WitnessNode,lane,proof_state)` : chaque split
hérite des verdicts `ALL/NONE` et des hyperarêtes déjà reçues, et seul le
résiduel `MIXED` descend. Cette route ne matérialise ni arrangement global ni
mosaïque de Delaunay d'ordre supérieur. Elle n'est reçue que si elle réduit les
recertifications et un coût aval réel, pas seulement `E4`.

## 6. Portes exigées avant tout claim de gain

### Exactitude

- la fixture collinéaire de sept IDs ci-dessus ;
- un `ALL` ancêtre dont une feuille réapparaît dans la banque ;
- deux ledgers dont les unions de preuves diffèrent ;
- groupes de supports `{1,2}`, `{2,3}` et singletons, comparés à `tau(F)` ;
- une configuration où `nu(F)<tau(F)` et la profondeur ferme quand même ;
- endpoints, `PointId` non denses, coordonnées dupliquées, permutation Morton ;
- produits AABB `2x2`, `2x3` et `3x3` contre le juge primal sur toutes les
  vraies paires ;
- parent `k=2` sans intervalle de poids commun, deux enfants chacun `ALL`, avec
  partition exact-once et somme des masses enfants égale à la masse parent ;
- égalité stricte, base invalide, somme des poids 65 535 et 65 536 ;
- cap du juge ou du proposant donnant `UNKNOWN/PARTIAL`, jamais `accord=OUI` ;
- option BJD sans `--vwave`, juge sans groupes et mutant sans juge rejetés avant
  calcul ;
- mode `SOC64 --actif` jugé directement sur ses fermetures, car le shadow saute
  actuellement un verdict déjà promu `ALL` par le chemin actif ;
- `--judge-vwave` refusé avec SOC/BJD tant qu'il ne recompose que les témoins
  singleton du central.

Le juge doit recalculer la profondeur ou `tau(F)`. La fixture actuelle
`u=6,p=7,d=8` ne suffit pas si `d` est imprimé comme littéral.

### Coût

Publier par famille et taille : proof-tiles soumises, pools, bases proposées,
bases vérifiées, couples de coins, nœuds du solveur transversal, taux de cache,
splits, fermetures avant/après descente, opérations `i128`, octets, HWM et temps
par phase. Les comparaisons causales gardent le même front, les mêmes seeds et
les mêmes caps.

No-go avant G4 50k si l'un des points suivants tient à `1500/3000/6000` :

- le nombre de tests de bases croît comme le nombre de terminaux fois la taille
  quadratique du pool ;
- les fermetures arrivent seulement après avoir payé la descente entière ;
- le temps BJD dépasse le temps transitif économisé ;
- `pending!=0`, un juge saute des flips, ou le payload reste absent.

La microgate appariée exige en outre
`bjd_faux=bjd_unknown=bjd_sautes=ferm_faux=ferm_sautes=0`, des juges et
fermetures non vacuaires, puis deux exposants successifs au plus `1,35` pour les
recertifications, preuves larges, octets et HWM. Un tier rapide propose au plus
16 bases par tuile et un tier exceptionnel au plus 64 ; cap atteint signifie
`UNKNOWN/split`. Le solveur transversal ne dépasse pas 3280 nœuds bruts en q4
ni 9841 en q3. Enfin poser `T_proof=T_proposer+T_BJD+T_tau+T_SOC` et mesurer le
vrai aval évité sur les mêmes tuiles : le GO diagnostic demande
`T_saved/T_proof>=2` à 3000 et 6000 sur `uniform` et `eight_clusters`, avec la
baisse d'au moins un compteur aval. Ces seuils d'ingénierie ne qualifient pas le
SLO officiel.

### Preuves de coût déjà disponibles

Le replay live sain donne une comparaison causale sans dépendre du chronomètre
de la machine partagée. À `uniform,n=600`, la baseline et BJD paient les mêmes
`6892939` lectures/recertifications ; BJD ajoute `263349` essais et environ
`4,32` millions de couples de coins. `E4` baisse de `119521` à `90033`, signal
géométrique utile, mais aucune descente n'est évitée. À `uniform,n=3000`,
`SOC64 --actif` paie encore `102151070` lectures, plus `42584307` appels SOC,
`235010080` couples et `675385834` opérations larges pour abaisser `E4` à
`611908`. Ces nombres imposent une fermeture de tuile **avant** la descente et
une gate physique, pas une nouvelle rampe fondée sur le seul résiduel.

Le replay apparié à `n=1500` confirme le no-go de l'ordonnance actuelle. Sur
`uniform`, BJD réduit la masse q4 ouverte de `269817` à `235959` (`-12,55 %`),
mais garde exactement `32387961` lectures/recertifications et fait passer la
médiane de trois temps CPU utilisateur de `3,344 s` à `3,527 s` (`+5,47 %`).
Sur `eight_clusters`, la masse passe de `922141` à `914118` (`-0,87 %`), avec
exactement `9366805` lectures, tandis que la médiane passe de `0,945 s` à
`1,022 s` (`+8,15 %`). Les médianes portent sur trois processus distincts et la
machine partagée ; elles diagnostiquent le signe du surcoût, pas un benchmark
produit. La commande complète, l'ordre alterné et les sorties de `/usr/bin/time`
n'ont pas été conservés dans un reçu ; les compteurs, pas ces médianes, portent
la comparaison causale reproductible. Le greedy égal-poids branché après descente est donc **NO-GO comme hot
path sous cette ordonnance** ; conserver ses fixtures et mutants reste utile.

Le diagnostic documentaire SOC à `eight_clusters,n=2000` rapporte
`3 809 028` tâches, `18 871 452` prédicats et une vague passée de `2,83 s` à
`4,94 s`. Ce n'est pas un transcript brut ni un benchmark, mais le surcoût
`+74,6 %` interdit déjà une extrapolation aveugle. À attribution purement
linéaire, 50 000 points représenteraient environ 95 millions d'appels et plus
de 52 secondes CPU ; cette extrapolation est une alerte, jamais une prévision.

Les reçus bruts `rampe_raffinement_g4_20260813` à `s=8,r0,n=50000` contiennent
`9 182 111` terminaux q4 ouverts sur `uniform` et `7 961 883` sur
`eight_clusters`. Même **un** appel BJD par terminal atteindrait donc jusqu'à
596,8 et 517,5 millions de prédicats ponctuels. Le balayage naïf du pool de 24
est hors route avant même de compter ses succès.

Le reçu `chaine_complete_g4_20260813` localise le vrai coût CPU du diagnostic
actuel à `s=3,n=50000` : l'arbre prend `3,5 ms`, mais la vague paie
`624 377 753` recertifications et `18,437 s` sur `uniform`, puis
`430 666 842` et `12,799 s` sur `eight_clusters`. Environ 43 % des visites sont
des descentes pures et 50 % finissent `NONE`; seulement environ 7 % créditent
`ALL`. `BlockJungDual64` placé après cette vague ne retire aucune de ces
visites.

Enfin, les fronts `s=8` à 50 000 sont 24,47 fois et 17,05 fois plus grands que
les fronts historiques `s=2` sur `uniform` et `eight_clusters`. La baseline de
coût reste donc proche de `s=2`, avec raffinement local reçu, pas `s=8` global.
Ces rapports portent sur des diagnostics CPU et ne qualifient aucun kernel.

Le squelette `BenchmarkOutputContract-v1` inventorie actuellement seize étages
critiques : zéro complet, six incomplets et dix absents. Il publie
`warm_e2e_mesurable=non` et `slo_eligible=non`. Le statut exact n'est donc pas
« SLO lent » mais **chronométrage officiel encore inéligible**.

## 7. État réel de la dernière session G4

Le reçu `receipts/soc64_actif_g4_20260814/transcript.txt` est un reçu d'échec
sûr : la session au `HEAD=110fe76` et worktree sale a construit le probe, puis
CTest a signalé plusieurs échecs/non-exécutions. La rampe n'a pas commencé. Le
trap a arrêté exactement `devpod-gpu-exploration/europe-west4-ai1a/ehgp-blackwell-spot-ai1a`
et confirmé `TERMINATED`.

Le `HEAD=783a789` corrige **partiellement** le script de session, sans le rendre
recevable :

- la liste de build omet encore `mhgp3v_jung_dual_judge`, alors que le regex
  CTest sélectionne sa porte ; un build propre doit encore s'arrêter avant la
  rampe ;
- ce regex ne sélectionne pas non plus les nouvelles portes `mhgp3v_bjd_*` ;
- `check_rampe_pentes.py` tourne sous `set +e`, son code est imprimé puis
  ignoré ; une pente rouge, du pending ou un code de run non nul peut donc
  conduire à `SESSION_TERMINEE rc=0` ;
- le script ne passe pas `--exige-fenetre-finale` à l'analyseur. Cette option
  doit elle-même signifier une finalité complète : aucun claim SOC/BJD sauté ou
  inconnu, en plus de `pending=0` et des codes zéro ;
- l'analyseur ne sait lire que `sum_E`, `masse_ouverte` et `max_E`, et le script
  gate `sum_E` à `1,70`. Il ne peut donc pas appliquer le seuil officiel `1,35`
  aux tests de boîtes, à la frontière, aux supports feuilles, aux octets ou à la
  HWM ;
- chaque job peut exécuter quatre timeouts de `3000 s` en série, soit jusqu'à
  `12000 s`, au-delà des coupe-circuits invité `4800 s` et GCE `5400 s` ; la
  cohérence annoncée des durées est fausse ;
- la rampe force `s=8,r0`, alors que les fronts reçus sont 24,47 et 17,05 fois
  ceux de `s=2` sur `uniform` et les amas ; sans aval officiel, elle mesure
  seulement un résiduel contre le coût du certificateur ;
- le parcours combiné reste incomplet sous `central-NONE`, et la recette est
  explicitement CPU-only : même une exécution verte ne qualifierait ni la vue
  complète ni le contrat G4 ;
- le reçu précédent annonce un worktree sale sans en conserver la liste ni le
  diff complet.

Aucun reçu ne prouve l'exécution du script corrigé. Les taux locaux SOC64 à
`n=3000` ne sont ni des pentes 50k, ni du `warm_e2e`, ni une mesure GPU. Ne pas
relancer avant correction locale et dry-run de la sélection CTest.

## 8. Contre-audit croisé des documents antérieurs

`AUDIT_CONTRE_RECEPTION_M4_V2_DEPTHBLOCK_5BFC5C8_20260814.md` avait raison sur
quatre points structurants :

- les poids doivent rester fixes sur les 64 coins ;
- pour une paire fixe, `Depth=tau(E)` est la quantité mathématique pertinente ;
  sur une tuile variable, `tau(F)` reste seulement une minoration sûre ;
- la profondeur doit fermer un bloc avant fill ;
- un échec d'un certificat uniforme impose `MIXED/UNKNOWN` ou un split.

Deux durcissements sont nécessaires :

1. l'équivalence des 64 coins est actuellement davantage prouvée dans les
   documents que jugée indépendamment sur des boîtes non dégénérées ; les
   selftests point/boîte-point ne reçoivent pas la quantification live ;
2. son angle mort était le raccord live des identités et l'absence de tests
   AABB non dégénérés. Il recommandait déjà `Depth=tau(E)` sur le domaine
   pair-level, le branch-and-cut et le packing comme seul fast path ; la faute d'addition scalaire appartient au
   premier raccord live, pas à cette proposition.

`AUDIT_SOURCE_CK_WST_Q2_Q3_Q4_35FCEA8_20260814.md` garde une proposition
CK--WST de source factorisée conditionnelle, pas une borne de masse logique.
Le présent déblocage ne rend pas cette masse sparse ; il fournit un moyen exact
de la fermer symboliquement avant son développement. Aucun artefact Git ne
prouve que ces flux viennent d'identités distinctes ; leurs énoncés sont jugés
par contenu et snapshot.

La borne de noyau tolérant a été redérivée depuis les sources primaires : le
théorème 3.1 de [Montejano--Oliveros](https://doi.org/10.1007/s00454-010-9296-6)
emploie `eta(d+1,t+1)` et la borne stricte de
[Tuza](https://doi.org/10.1016/0095-8956(85)90043-7) donne
`eta(3,h)<(h+1)^2`, donc au plus 80 IDs en q4 et 99 en q3. Cela valide la borne
ponctuelle existentielle, pas un noyau commun aux rectangles CK ni son coût de
découverte.

## 9. Ordre remis à Claude

1. typer toute base invalide `UNKNOWN`, corriger le commentaire minimax, terminer
   le reçu d'identité, refuser les juges partiels et graver les fixtures
   collinéaires avant toute mesure ;
2. séparer complètement baseline et combiné, y compris leurs pools et leurs
   hyperarêtes ;
3. remplacer le crédit glouton par le solveur borné `tau(F)>=h`, en gardant le
   packing comme ablation ;
4. générer les bases et poids par coupes, appliquer `BJD-BilinearBounds`, puis
   rejouer les 64 coins seulement sur le résiduel ;
5. ouvrir en parallèle une piste **counter-only** CK/porteur aigu/BJD--tau sur
   petit `n`, contre vérité exhaustive, sans attendre que 0A/0B autorisent un
   claim produit ; partir du front coarse proche de `s=2` et, lorsqu'un parent
   n'a pas de poids commun, scinder le facteur indiqué par ses deux coins
   incompatibles tout en héritant les hyperarêtes déjà uniformes ;
6. déplacer le certificat avant la descente dans une vague jointe héritant les
   preuves, puis mesurer le coût causal à `1500/3000/6000` ; arrêter la piste si
   elle ne réduit pas les recertifications ou si ses tests croissent
   quadratiquement avec le pool ;
7. ne relancer une rampe G4 que lorsque la recette locale est verte, finale et
   porte au moins les compteurs physiques bloquants ; aucune qualification SLO
   avant `BenchmarkOutputContract-v1` complet.

GCP non utilisé par l'auditeur.
