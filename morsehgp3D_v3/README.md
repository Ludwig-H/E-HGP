# MorseHGP3D v3

MorseHGP3D v3 explore une construction exacte et industrielle de la hiérarchie
Morse/HGP en dimension trois, sans matérialiser la mosaïque de Delaunay d'ordre
supérieur.

Cadre courant :

```text
phase=exploration_v3_hors_registre
backend=cpu_reference_bounded_oracles_and_g4_diagnostic
profile=quantized_u16_input_only
mode=audit_independant_math_and_architecture
public_status=not_claimed
```

La v3 n'est ni promue dans le registre officiel, ni qualifiée GPU, ni déclarée
exacte sur son domaine public. Une session G4 SPOT documentée au `HEAD=35fcea8`
a utilisé ses 48 processeurs logiques comme ressource CPU, sans kernel CUDA ;
le reçu certifie l'arrêt ciblé `TERMINATED` et ne qualifie aucun SLO.

## Verdict actuel

Le contrat n'est pas rempli. Pour `n=50000` et `K_max=10`, la cible principale
est `p95 warm_e2e<100 ms` et la cible secondaire `p95 warm_e2e<1 s` sur un G4,
sortie complète et synchronisation comprises. Aucun échantillon qualifiable ne
reçoit l'une ou l'autre.

Le pin historique `2b89ea1` introduit enfin une première tranche
`BallForm -> PrimitiveSphereKey -> census I_B/U_B -> SphereRun`. C'est le bon
ordre architectural, mais **l'étape 0A n'est pas reçue pour u16** : les
constructeurs q3/q4 rabattent des numérateurs de 67 à 81 bits vers `int64`, puis
créent des carrés jusqu'à environ 162 bits dans `i128` avant réduction. Les
huit CTests du pin, puis les dix du successeur, ne couvrent que `coord<=64` ;
leur juge de Gram dépasse lui-même 128 bits sur des fixtures u16, ne recertifie
ni la positivité ni la clé primitive, et le mutant de clé reste corrélé.

Le producteur au pin `3c11bc8`, inchangé au `HEAD=35fcea8`, ajoute un probe
nommé stage 0B. Il ne ferme pas 0B : il
compare Kruskal à Floyd--Warshall sur le même hypergraphe de `PointId`, dans
une unique DSU. Il n'émet ni dix forêts par ordre, ni lots, coverage,
verticales ou payload. Cette DSU est structurellement fausse dès `k=2` lorsque
deux générateurs distincts partagent un point mais moins de `k` identités.

Le verdict live est dans
[`audits/AUDIT_ETAT_COURANT.md`](audits/AUDIT_ETAT_COURANT.md). L'audit ciblé de
la tranche est
[`audits/AUDIT_BALL_EVENT_V0_2B89EA1_20260813.md`](audits/AUDIT_BALL_EVENT_V0_2B89EA1_20260813.md).
Le contre-rejeu du faux 0B est
[`audits/AUDIT_CONTRE_RECEPTION_STAGE_0B_3C11BC8_20260813.md`](audits/AUDIT_CONTRE_RECEPTION_STAGE_0B_3C11BC8_20260813.md).

## Route active

La prochaine chaîne à fermer est :

```text
0A  BallForm -> BallKey -> census -> BallEvent exact
0B  oracle exhaustif borné -> lots -> dix forêts -> verticales -> payload
1   CKPairTape q2 -> porteurs aigus -> OwnedCK-WST3/WST4, toujours factorisés
2   certifier profondeur par blocs avant fill, puis rang/census et
    F2/F3/C4_carrier/F4/M4_apex/T4_site
3   porter la même tranche sur device, puis mesurer warm_e2e sur G4
4   ouvrir séparément tout nouveau profil numérique
```

Une source incomplète peut être comparée dans le sink de référence, mais ne
publie jamais un succès. Il n'existe pas de watermark monotone par ancre : les
runs sont scellés, triés et mergés par niveau exact avant le premier commit
d'un lot. « Streamé » signifie mémoire résidente bornée, jamais fold en ligne
sur une source non scellée.

## Contrat d'identité

Les couches restent distinctes :

- `PrimitiveSphereKey` : cinq coefficients primitifs de
  `A||z||^2+B dot z+C`, avec `A>0`, avant census ;
- `BallKey` : identité de nuage, profil et schéma exact ajoutés à la clé
  primitive ;
- `SupportKey` : vrais `PointId` triés, jamais positions Morton ou indices
  d'un buffer ;
- `BallEvent` : `BallKey`, supports, owners, niveau exact, `I_B/U_B`, lanes,
  provenance, complétude du census et disposition transactionnelle.

Le fold contractuel ne doit dépendre ni de `__int128` natif, ni du nombre de
limbs du profil. Le probe courant viole encore cette frontière : son
comparateur lit directement `PrimitiveSphereKey` et effectue ses contrôles
après des multiplications signées susceptibles de déborder. Une fois la
frontière reçue, un futur profil binary64 pourra changer `ExactKernel` et le
codec sans réécrire le fold. La cardinalité seule ne motive pas binary64 : la
grille u16 3D contient $2^{48}$ sites distincts ; l'index dense et le `PointId`
sont des codecs séparés.

## Prochaines réparations P0

Avant d'appeler `0A` fermé :

1. construire directement les polynômes q3/q4 sans centre rabattu en `int64`
   et employer une autorité BigInt/rationnelle sur tout `[0,65535]^3` ;
2. juger indépendamment dépendance affine, positivité, clé primitive, niveau,
   census et owner sur des `PointId` non denses ;
3. ajouter epoch/profile/schema, statuts typés, marqueurs de complétude et
   `SupportRecord` atomique ;
4. appliquer `count -> preflight -> fill -> validate -> publish`, avec zéro
   payload sur cap moins un, erreur numérique ou dégénérescence non admise ;
5. borner les générateurs de fixtures et refuser leur capacité plus un ;
6. différencier toute la sortie de `0A`, puis fermer `0B` par générateurs et
   ordres jusqu'au `BenchmarkOutputContract-v1`.

## Source factorisée q2/q3/q4

La source retenue exploite Callahan--Kosaraju sans développer son produit
cartésien :

```text
CKPairTape(A,B)                          toutes les paires, exact-once
  -> OwnedCK-WST3(A,B,C)                carrier de l'arête maximale
  -> OwnedCK-WST4(A,B,C,D)              second carrier/apex
  -> BallKey/RLE -> rang/census -> fold
```

Pour chaque rectangle CK, une boule `B_R` contenant `A union B` fixe un niveau
Morton. Tout troisième ou quatrième sommet d'un support dont `ab` est l'arête
maximale appartient à `2B_R`; la constante deux est sharp. En écrivant
`a=m-h`, `b=m+h`, `x=m+q` autour du centre de `B_R`, les endpoints donnent
`||m||^2+||h||^2<=R^2`, tandis que la maximalité donne
`||q+h||,||q-h||<=2||h||`, donc `||q||<=sqrt(3)||h||` et `||x||<=2R`.
Intersecter avec `B(c_A,U_AB+r_A)` et `B(c_B,U_AB+r_B)`, où `U_AB` majore
les distances endpoint, resserre encore le domaine sans perdre de carrier.
Les cellules non vides de ce niveau donnent une extension ternaire complète,
puis leurs couples non ordonnés une extension quaternaire complète. L'owner
longueur/`EdgeKey` rend les sorties exact-once.
Cette preuve exige que `CKPairTape` partitionne réellement les paires non
ordonnées et que les cellules half-open forment une antichaîne. Un raffinement
remplace atomiquement le parent par tous ses enfants disjoints. Pour la
diagonale q4, `binom(C,2)` devient tous les `binom(C_i,2)` et tous les
`C_i×C_j`, `i<j`; pour `C×D`, tous les `C_i×D_j`. Le carrier primaire oriente
la sweep mais ne crée jamais une seconde copie du `CellPair`.
Une paire n'est q2 propre que si `D=||b-a||^2>0` : les positions dupliquées
sont rejetées, quotientées ou filtrées exactement avant cette promotion.

q3 recertifie `E+X-D>0` et l'indépendance affine. q4 ne signifie pas « quatre
faces aiguës » : l'autorité est la stricte positivité des quatre
barycentriques du circumcentre. Une face aiguë adjacente à l'arête maximale
sert seulement à choisir un carrier géométrique primaire. La jointure teste
`Acute(x) OR Acute(y)`, jamais `AND` : un q4 positif peut n'avoir qu'une seule
face aiguë adjacente. Si les deux le sont, le plus petit `PointId` aigu est le
primaire et supprime le doublon ; l'autre sommet reste un apex arbitraire.

Le chemin q4 élimine d'abord les `CarrierBlock` sans face aiguë, avant de
former les couples de cellules. Les blocs `ALL_ACUTE` restent symboliques :
WST4 est formé avant toute expansion par face. Pour une face exacte résiduelle, les centres vivent sur
une droite et la puissance de chaque apex y est affine : une sweep 1D par lots
égaux remplace l'arrangement 2D comme candidat principal. Les comparaisons
rationnelles peuvent dépasser `i128` sous u16. Un site dont le dénominateur de
sweep est nul contribue une puissance constante négative, nulle ou positive ;
il n'est jamais jeté.

Les masques de rang restent indépendants. Une fixture u16 de 64 points possède
un q4 régulier de rang 4, alors que ses six arêtes q2 et ses quatre faces q3 ont
toutes rang 12. `OwnedCK-WST4` doit donc consommer la relation aiguë q3
**pré-rang**, jamais les événements q3 retenus. Le tape carrier est construit
dès que `q3_open || q4_open`, même si la lane de rang q3 est fermée.

Le nombre de blocs initiaux vaut conditionnellement `O(s^3 n)`,
`O(s^3*eta^-3*n)` et `O(s^3*eta^-6*n)` pour q2/q3/q4, avec `0<eta<=1` et une
vraie propriété fair/compressed-split. Ces bornes ne couvrent pas tous les
raffinements `MIXED`. Leur masse logique peut rester quadratique ou pire. Les
blocs sont paresseux jusqu'à un consommateur factorisé reçu ou au preflight
atomique d'une vraie sortie. Le rapport complet
et son contre-audit sont
[`audits/AUDIT_SOURCE_CK_WST_Q2_Q3_Q4_35FCEA8_20260814.md`](audits/AUDIT_SOURCE_CK_WST_Q2_Q3_Q4_35FCEA8_20260814.md)
et
[`audits/AUDIT_DEBLOCAGE_Q4_PORTEUR_AIGU_SOC64_LIVE_35FCEA8_20260814.md`](audits/AUDIT_DEBLOCAGE_Q4_PORTEUR_AIGU_SOC64_LIVE_35FCEA8_20260814.md).
Les réponses à Claude et le contre-audit des compteurs porteurs/apex sont dans
[`audits/AUDIT_REPONSE_M4_PORTEURS_AIGUS_4515A8B_20260814.md`](audits/AUDIT_REPONSE_M4_PORTEURS_AIGUS_4515A8B_20260814.md).
Le contre-audit v2 et la réponse entière `BlockBallDepth8` sont dans
[`audits/AUDIT_CONTRE_RECEPTION_M4_V2_DEPTHBLOCK_5BFC5C8_20260814.md`](audits/AUDIT_CONTRE_RECEPTION_M4_V2_DEPTHBLOCK_5BFC5C8_20260814.md).

## Déblocages mathématiques prêts après `0A`

Quatre pistes sont assez précises pour être implémentées dans les composants
existants, avec échec fail-open.

### `SOC64` et `CORNER512`

Pour `e=z-a`, `t=b-z`, `H=e dot t`, `E=||e||^2`, `X=||t||^2`, q3 exige
`H>0 && 4H^2>EX` et q4 `H>0 && 3H^2>EX`. Le domaine est séparément convexe en
`e,t` et en `a,b,z`.

- Si les 64 couples de coins de `(C-A)×(B-C)` passent, tout le rectangle est
  `ALL`. Un échec est `UNKNOWN`.
- Les 512 triples de coins de `A×B×C` caractérisent exactement `ALL` pour
  l'enveloppe AABB continue. Un coin fictif échouant ne vaut pas `NONE` pour les
  seuls `PointId` stockés.

Le `JungSpindleRect-v0` actuellement branché n'est pas `SOC64/CORNER512` : il
combine des extrema séparés de `D,V,T`. Son diagnostic `n=6000,s=8` gagne
environ trois centièmes de point seulement. Cela réfute cette combinaison sur
les boîtes grossières mesurées, pas les deux théorèmes corrélés. Leur primitive
et 16 CTests bornés sont verts au HEAD ; l'ablation WSPD et son coût transitif
restent non reçus.

La prochaine ablation recommandée est `SOC64-shadow-q4` sur un échantillon
déterministe de tâches `central-MIXED`, avec cap propre, early exits, masse
créditable et aucun changement de fate. Si le signal est positif, le brancher
**avant** le raffinement local : un certificat corrélé évite une scission,
alors qu'un certificat rejoué après la scission paie déjà son front.

Le premier raccord live du 14 août additionnait les crédits SOC d'un ancêtre
aux crédits baseline du même sous-arbre et comptait deux fois les mêmes IDs.
Le shadow courant corrige cette faute : il compare les ledgers baseline et
union combinée, place SOC après les fallbacks et arrête une branche combinée à
son premier `ALL`. Un replay borné mesure `41` fermetures de masse `95`, contre
`127` et `316` avec la somme fautive. Il doit encore recevoir une
comparaison PointId de l'union. Le juge direct des flips tue désormais le mutant
`sum_instead_of_union` lorsqu'il énumère tout, mais annonce encore `accord=OUI`
si son cap saute des flips. Cinq portes CTest intégrées passent, sans garder la
non-vacuité du juge ni un oracle d'union par vrais IDs. À `n=2000`, le
shadow non capé a déjà soumis `3 809 028` tâches et ajouté environ `2,1 s` à la
vague sur une machine partagée. Le HEAD fournit désormais un cap qui publie
`MINORANT_CAP` ; ce cap borne le diagnostic sans recevoir la politique de
sélection ni la rentabilité transitive. Ne pas lancer sa rampe 50k. Un retour inférieur
à `floor=q4` signifie seulement `UNKNOWN_BELOW_FLOOR`, pas une lane exacte.

Le HEAD essaie maintenant SOC au nœud courant même si le central rend `NONE`.
Ce gain partiel est sûr, mais la vue combinée ne descend toujours pas lorsque
SOC rend `UNKNOWN` sous ce `NONE`. `cmask` reste subordonné au masque baseline.
Un replay borné révèle `14383` témoins exacts cachés sous ce prune et fait
tomber SOC/exact de `96,76 %` à `86,10 %` sur la cohorte élargie. La réparation
est un parcours physique commun à deux masques logiques indépendants : baseline
descend sur `central-MIXED`; combined teste SOC sur tout central non-`ALL` et
descend sur `SOC-UNKNOWN`, avec son propre cap et son propre `PENDING`.
`--none-descend` tel qu'il existe ne propage pas ce masque combiné.

### `JungDiskDepth`, puis LP projectif

Pour une paire ponctuelle owner `ab`, les centres q3 et q4 ne parcourent pas
tout le plan médiateur : avec `y=2c`, ils restent dans les disques exacts de
Jung `||y-d||^2<=D/3` et `||y-d||^2<=D/2`. Dans ce plan 2D fixe, un groupe d'au
plus trois IDs peut certifier qu'au moins un témoin est intérieur pour tout
centre admissible. Neuf groupes disjoints ferment q3, huit ferment q4 avant la
création des carriers de cette paire.

Cette preuve ne ferme pas un rectangle CK : ses endpoints font varier le plan,
le disque et les demi-plans. Il faut scinder jusqu'à une paire/microtile rejoué,
ou prouver un futur `BlockJungDiskDepth` uniforme. Une fixture `2×2` ferme q4
sur la paire basse alors qu'aucun de ses huit témoins n'est même q2 intérieur
sur la paire haute ; tout transfert depuis un représentant reste interdit.

Un candidat de certificat `ALL` uniforme est `BlockJungDualTile`. Écrire `m=(a+b)/2`,
`h=(b-a)/2`, `c=m+w`, avec `w dot h=0` et
`||w||<=kappa||h||`, où `kappa^2=1/3` pour q3 et `1/2` pour q4. Un groupe
ferme une paire si et seulement s'il existe des poids rationnels `lambda_z`
non négatifs de somme un tels que, avec
`alpha=||h||^2-sum lambda_z||m-z||^2` et `p=m-sum lambda_z z`, on ait :

```text
q3 : alpha>0 et 3*alpha^2 > 4*(||h||^2||p||^2-(p dot h)^2)
q4 : alpha>0 et   alpha^2 > 2*(||h||^2||p||^2-(p dot h)^2)
```

Helly borne une base à trois IDs. Un certificat de bloc conserve une même base
et des poids rationnels. À poids fixes, le test exact des 64 couples de coins
décrit ci-dessous prouve les polynômes sur tout `A×B`, sans Bernstein ; un
échec provoque un split fail-open. Tester les coins avec des poids reproposés
séparément reste faux. Pour un témoin singleton, ce dual redonne exactement
`SOC64`.
Cette version bloc est sûre mais incomplète : `for all pair exists lambda`
n'implique pas `exists lambda for all pair`. Un échec ou un dénominateur trop
large rend `MIXED/UNKNOWN`, jamais `NONE`.

La forme entière directement alignée sur le prototype pose
`W=sum w_z`, `D=||b-a||^2`, `A=W*D-sum w_z||a+b-2z||^2`,
`P=W*(a+b)-2*sum w_z*z` et
`R=D||P||^2-(P dot (b-a))^2`. Elle teste q3 par
`A>0 && 3A^2>4R` et q4 par `A>0 && A^2>2R`. Sous u16, la largeur i128 annoncée
exige `W<=65535` vérifié avant toute somme. Le prototype live code ces seuils,
mais ne préflighte pas encore `W` et ne constitue ni une primitive CUDA reçue
ni une profondeur de bloc.

Le HEAD ajoute un juge primal BigInt indépendant pour une base fixe ; les
`13/13` CTests Jung ciblés passent et le collectif `k=2` est exercé. Il manque
une fixture nécessairement ternaire, le calcul de profondeur `d`, le preflight
u16 et le raccord `OPEN_FINAL`.

Le lift rectangle à poids fixes est désormais résolu mathématiquement. Poser
`A0=-W*(a dot b)+(a+b) dot Z-Q` et
`C0=W*(a cross b)-a cross Z-Z cross b`. q4 vaut
`A0>0 && 2A0^2>||C0||^2`, q3 remplace `2` par `3`. Pour un endpoint fixé,
`(A0,C0)` est affine dans l'autre et chaque lane est un cône de Lorentz convexe.
Les `8×8=64` couples de coins caractérisent donc exactement `ALL` sur
l'enveloppe `A×B`. `BlockJungDual64` rend `ALL` ou `MIXED`, tient en i128 sous
`1<=W<=65535`, et repropose les poids après split sans jamais émettre les
paires. Les contrôles `A4=4*A0` et `R=4*||C0||^2` fixent les facteurs ; cette
équivalence porte seulement sur le reçu à poids communs, pas sur l'existence
d'un poids différent par paire. Le widening précède `a+b`, les produits et les
normes ; le preflight de `W` somme en type large ou saturant avant tout cast.

La dissection live à `eight_clusters,n=1500` trouve huit témoins singleton
exacts pour `89,5 %` de 200 PairId ouverts tirés par masse, contre `26,5 %` de
200 rectangles hachés. Ce signal favorise les microtiles et les preuves
uniformes, mais les deux taux ne se soustraient pas : PairId mass-weighted et
rectangles non pondérés ne sont ni appariés ni munis d'un intervalle, et le
second test n'exerce aucun groupe Jung.

L'ordre de descente live `--ordre-proche` réduit le pending sur ce même nuage
aux petits budgets, mais à finalité `window=256/512` il rend le même
`E4=1071162` que l'ordre Morton. Il compresse donc le budget du certificateur
central ; il ne réduit pas son résiduel géométrique et ne remplace ni Jung
collectif ni `BlockBallDepth8`.

La primitive entière associée ne décide pas elle-même l'existence de ces
poids : elle vérifie un vecteur rationnel fourni. Son contrat exige `D>0`, de
un à trois IDs authentifiés, des poids positifs, une somme capée et le profil
u16 ; tout échec du proposant reste `UNKNOWN`. Le wrapper, et non le tableau de
coordonnées, porte la disjonction des groupes. Une réception `k>1` exige un
juge exact de la faisabilité sur le disque continu ; un accord singleton avec
`SOC64` ou quelques centres tirés ne suffit pas.

Ce juge reste de taille constante par base : avec `u_z=a+b-2z` et `s=2w`, les
centres non couverts satisfont `2*s dot u_z>=D-||u_z||^2` dans le plan
`s dot (b-a)=0`. Le point de norme minimale de trois demi-plans au plus est
l'origine, une projection sur un bord ou l'intersection de deux bords. Le test
strict est `3*r^2>D` pour q3 et `2*r^2>D` pour q4. Une base couvrante `G`
certifie ensuite récursivement `Depth(P,h)` par tous les
`Depth(P minus {z},h-1)`, `z` dans `G`; le DAG porte au plus trois enfants par
niveau. Les groupes disjoints restent le fast path, cette récurrence le juge
borné. Sous `smax=11`, elle demande au plus `3280` recherches de base pour
`h=8` et `9841` pour `h=9`. Ces nombres viennent de la profondeur, pas des noms
q4/q3. L'oracle entier tient en i256 après réduction par
`g_i=D-||u_i||^2` et
`K_ij=D*(u_i dot u_j)-(u_i dot d)*(u_j dot d)` ; le replay Gram brut peut
dépasser 256 bits et reste GMP.

Le packing disjoint n'est pas complet. Une fixture u16 possède six témoins
universels, sept groupes couvrants disjoints au maximum, mais une profondeur
q4 exactement huit : `u=6<p=7<d=8`. Le hot path tente le packing, puis un
`ProofSpanDAG` de suppressions capé ; un cap produit `PENDING`. Sur un bloc CK,
chaque base proposée doit garder une marge uniforme ou provoquer un split du
proof-tile, jamais l'expansion des `PairId`.

Helly avec tolérance comprime en outre tout succès ponctuel. Pour les ensembles
fermés `B_z` de centres où `z` n'est pas intérieur, `Depth(P,h)` signifie qu'il
n'existe, pour **aucun** ensemble `R` de `h-1` IDs au plus, un point commun à
tous les `B_z` restants. Il existe
donc toujours un sous-pool qui certifie déjà la profondeur, de taille
`eta(3,h)<(h+1)^2` : au plus **80 IDs pour q4** et **99 pour q3**. Un
`ToleranceKernel` porte ces IDs et le vérificateur rejoue exactement leur
arrangement dans le disque. Ce résultat borne le payload, pas sa recherche ni
le nombre de tuiles ; pour un rectangle, le même noyau doit être prouvé sur
tout `A×B` ou provoquer un split. Le détail et la source primaire sont dans
[`PROPOSITION.md`](PROPOSITION.md#noyau-de-helly-avec-tolérance-et-hypergraphe-exact).

La profondeur possède surtout une réduction combinatoire exacte. Former
l'hypergraphe de rang trois dont chaque hyperarête est une base Helly couvrante
donne `d=tau(E)`, le nombre transversal minimal ; le packing actuel n'est que
`p=nu(E)<=d`. Un branch-and-cut alterne alors un petit solveur de transversal
bitset et une HPI qui rend soit un contre-centre, soit une nouvelle base
disjointe. Le device vérifie chaque base géométrique une fois, puis rejoue la
preuve de `tau(E)>=8/9` sans refaire la géométrie à chaque branche.
Le rang de cet hypergraphe décroît avec le domaine des centres : trois IDs sur
le disque pair-level 2D, deux sur l'axe de face 1D, un à BallKey fixée. C'est le
même mécanisme de profondeur à travers q3 et q4.

Après une face aiguë, une seconde porte collective travaille en dimension un.
Sur le segment de centres `J_f` compatible avec `K_4(ab)`, chaque témoin porte
la forme affine `P_z(tau)=A_z-tau*B_z`; il est intérieur lorsque `P_z<0`.
La profondeur fixe-face se décide exactement sans sweep globale : après `p`
témoins permanents, conserver les `8-p` seuils `tau<alpha` les plus grands et
les `8-p` seuils `tau>beta` les plus petits. Leur replay groupe les égalités
shell et forme un `AxisToleranceKernel` d'au plus **16 IDs**. C'est un scan
`O(n)` à mémoire `O(8)`, et la spécialisation constructive de
`eta(2,8)=16`. La version bloc vérifie signes, ordre et marges uniformément,
sinon scinde. La chaîne devient `Jung edge 2D -> carrier aigu -> noyau axe 1D
-> WST4 symbolique -> BlockBallDepth8 sur carrier×apex -> résiduel`. La sweep
par face n'est autorisée qu'après preflight ; il n'est pas nécessaire
d'énumérer les q4 pour commencer à prouver leur rang.

Le preflight M4 reste lui aussi factorisé. Chaque atome WST4 porte une masse de
quatre IDs distincts calculée par Möbius sur les quinze partitions des quatre
facteurs. Initialiser `M4_pending` à la masse non décidée ; pour une masse `m`,
`ALL_Q` fait `M4_pending-=m; M4_L+=m`, `NONE_Q` fait `M4_pending-=m` et
`MIXED_Q` remplace atomiquement le parent par ses enfants. Ainsi
`M4_U=M4_L+M4_pending`. `M4_L>B_fill` rejette seulement le fill ponctuel ;
`M4_U<=B_fill` certifie sa capacité, mais count final, offsets et publication
exigent encore `M4_pending=0`. Sinon continuer ou passer au shallow. L'identité exacte par arête/`PlaneKey` reste un
microkernel endpoint borné : à 50 000 points, un catalogue global aurait déjà
`1249975000` arêtes et `62496250050000` incidences `(e,z)`. Aucun de ces termes
soustraits n'est saturé séparément avant le résultat positif.

Deux ledgers restent distincts : `M4_raw_[L,U]` est pré-profondeur, tandis que
`residual_output_[L,U]` suit le fill après profondeur et positivité. Une
fermeture `ALL_INTERIOR` crédite `domain_mass_closed` et produit zéro sortie
résiduelle ; elle ne rend jamais `M4_raw=0`.

Le LP global reste un oracle utile, mais son échec ne prouve plus une pénurie
sur le disque Morse : une fixture à huit groupes ferme `JungDiskDepth8` alors
que le LP sur tout le plan échoue dès la profondeur un.

Pour `s_i=z_i-a`, `d=b-a`, `D=||d||^2`, `q_i=||s_i||^2`, poser :

$$\kappa_G(d)=\min\left\lbrace \sum_i\alpha_iq_i:\sum_i\alpha_is_i=d,\ \alpha_i\geq0\right\rbrace.$$

`G` crédite un intérieur sur toute sphère par `a,b` si et seulement si
`d` appartient au cône positif de `G` et `kappa_G(d)<D`. Un optimum basique
emploie au plus trois IDs. Huit extractions disjointes donnent un fast path q4;
un arbre de suppressions fournit un oracle complet de profondeur universelle
relativement au pool, jusqu'à 3280 appels LP pour `h=8`, donc q4 sous
`smax=11`. Cette propriété porte sur
toutes les sphères par la paire, pas seulement les supports Morse; un échec
reste fail-open pour la source. Ce dernier n'est pas un hot path.

### Pelages inversés collectifs

`OriginOnionDepth-h` inverse une banque autour de l'ancre, retire
successivement les sommets de `conv({0} union P)`, puis teste si la cible
inversée reste strictement dans la dernière coque sans chute de rang. Chaque
couche fournit alors un ID intérieur distinct dans toute sphère par la paire.
Une facette se rejoue sur un BNode par le test séparable
`v||d||^2-u dot d>=1`, sous environ 87 bits. `h=8/9/10` ferme q4/q3/q2.

Ce fast path reste universel. Sur la famille u16 à deux droites, tous les
certificats universels peuvent laisser `n^2/4` paires alors que tous les
triangles sont obtus et la vraie source q3/q4 est vide. La porte par carrier
aigu doit y rendre zéro sans développer de `PairId`.

### Cages de quatre à six sites

Une positive basis inclusion-minimale 3D peut avoir quatre, cinq ou six sites. Les cages
tétra-only sont donc incomplètes. Une cage de six facettes possède au plus huit
sommets de fleur. `SixRoleCageProposer` reste une ablation counter-only ; chaque
groupe doit être validé exactement, et réduire une cage impose de recalculer sa
fleur.

Les preuves, limites de largeur et contre-fixtures sont consolidées dans
[`audits/AUDIT_REPONSE_PLAN_VERTICAL_SOC64_LP_1AA487D_20260813.md`](audits/AUDIT_REPONSE_PLAN_VERTICAL_SOC64_LP_1AA487D_20260813.md).

## Raffinement local : signal utile, coût non reçu

Le raffinement des seuls terminaux q4 non fermés réduit réellement `E4` à
`n=3000`, `s=8` : `eight_clusters` passe de `4 045 644` à `2 597 699` arêtes
résiduelles et `uniform` de `1 027 538` à `464 599`. Mais à profondeur quatre,
les recertifications passent respectivement de `31 538 327` à `199 169 436`
et de `108 858 186` à `193 020 841`. Sans `F3/C4_carrier/F4/M4_apex`,
BallKeys, census et fold, il
est faux de conclure que le levier « paie ».

La télémétrie de tête double-compte les parents ensuite scindés et imprime
jusqu'à `380,15 %` de masse q2 fermée. Le ledger terminal
`CLOSED/OPEN/PENDING` reste cohérent ; les compteurs de tentatives doivent être
séparés de l'objet final.

La réparation algorithmique proposée est `ProofCarryingLocalRefinement` : un
enfant hérite des CNodes témoins déjà `ALL`, des `NONE` et de leurs IDs ; seuls
les `MIXED` sont rejoués. Cela évite de repartir de la racine à chaque split et
se prête à `count--scan--fill` avec continuations persistantes.

La recette G4 a depuis été exécutée sur CPU. Les quarante processus rendent
zéro, mais `terrain` conserve des continuations q3/q4 à 25 000 et 50 000 : ses
`sum_E4` sont des surensembles, pas des fenêtres finales. Sur
`eight_clusters`, `pending=0` et les pentes restent proches de `1,9` après
profondeur quatre ; cela réfute la configuration centrale mesurée, pas tous les
certificateurs rectangle. Le rapport `1,62` contre `1,57` compare en outre des
unités différentes et ne mesure pas un prix. Détails et réponses :
[`audits/AUDIT_REPONSE_CRITERE_MORT_SOC64_LP_35FCEA8_20260813.md`](audits/AUDIT_REPONSE_CRITERE_MORT_SOC64_LP_35FCEA8_20260813.md)
et
[`audits/AUDIT_CONTRE_RECU_RAMPE_RAFFINEMENT_G4_35FCEA8_20260813.md`](audits/AUDIT_CONTRE_RECU_RAMPE_RAFFINEMENT_G4_35FCEA8_20260813.md).

## Dégénérescences et sortie lourde

Le profil de coordonnées u16 n'exclut pas les cosphères. Le domaine candidat
utilise une politique `RelevantGP` fail-closed : un extra-shell pertinent rend
`unsupported_degeneracy` tant qu'aucun quotient complet n'est reçu. Cette
fermeture du domaine reste elle-même à recevoir.

Un `SphereRun` interne conserve l'identité et le census pour garder la décision
réversible. Il n'autorise pas un plateau public. Un quotient saturé ne devient
valide qu'après reconstruction des lots, dix forêts, coverage et verticales.
Si le contrat exige chaque `SupportKey`, une cosphère lourde est une borne de
sortie ; ni RLE ni streaming ne suppriment ce travail.

## Porte de coût

Une pente `sum_E4` ne qualifie rien seule. Chaque campagne publie au minimum :

- masses exclusives `CLOSED/OPEN/PENDING`, avec `pending=0` pour une fenêtre
  finale ;
- `E3/E4`, maximum par ancre, `F3`, `C4_carrier=edge×carrier aigu`, `F4`,
  `M4_apex=edge×carrier-primaire×apex`, puis `W4_positive/H4_rank` ;
- `N4_event`, `Z4_const`, `R4_bundle` et `T4_site` séparés ;
- `BallKey` brutes/uniques, supports, census et tailles de shell ;
- sorties `H`, octets, HWM, opérations larges et temps par phase ;
- commandes, seeds, commit, diff, binaire et codes de sortie.

Les diagnostics CPU existants ne sont pas des modèles G4. Aucun cutoff kNN
n'est exact : des supports positifs gardent un partenaire arbitrairement loin
en rang. Aucun arrangement global, aucune mosaïque Delaunay d'ordre supérieur
et aucun catalogue exhaustif ne deviennent le chemin produit.

Au HEAD `cec4a4f`, le sampler v2 retire `PENDING` et la censure des grosses
lentilles, puis remplace `2 sigma` par une demi-largeur Hoeffding correcte sous
des tirages i.i.d. uniformes. Son implémentation ne reçoit pas encore cette loi :
multiply-high reste sans rejet, les streams SplitMix n'ont pas de contrat
d'indépendance, le delta n'est pas réparti sur les décisions simultanées et
`W4` n'a pas d'intervalle. Le contrôle ne compare pas le décodage à un mode
exhaustif déterministe. `--rang` peut en outre réussir sans `--porteurs`, ignore
les extra-shells et mesure un échantillon conditionnel non pondéré, pas `H4/W4`.

Le nouveau `q4_brute_oracle` reçoit seulement une énumération exhaustive
bornée. Son claim `M4=Theta(n^4)` pour tout nuage est faux : sa propre famille
`two_lines` donne `M4=0`. Ses prédicats recopient le Gram--Cramer/in-sphere du
sujet, son `H4` teste seulement les intérieurs et oublie le shell, et les cas
vides impriment encore certains `NaN`. Les cinq CTests verts ne réparent pas
ces défauts.
Une construction ouverte en coordonnées réelles prouve néanmoins qu'une masse
q4 bien centrée peut être quartique **avant rang**. L'ancienne instanciation par
cubes unitaires u16 n'était pas uniforme (`2093/4096` supports seulement). La
fixture exacte mise à l'échelle utilise les nœuds
`A=(20000,20000,20000)+{0,1}^3`,
`B=(30000,30000,30000)+{0,1}^3`,
`C=(19000,31000,31000)+{0,1}^3`,
`D=(31000,19000,31000)+{0,1}^3` et
`Z=(20000,20000,30000)+{0,1}^3`, avec quarante `PointId` distincts. Ses `4096`
supports sont q4 positifs et les huit IDs de `Z` sont uniformément intérieurs. Par convexité du déterminant
in-sphere normalisé en `z`, huit coins certifient `ALL_INTERIOR`; un seul bloc
du futur classifieur doit donc fermer avant fill. Les coins ne certifient pas le verdict inverse
`NONE`.

Le HEAD contient aussi le juge direct des flips SOC et `JungDual`. Le premier
tue le mutant de somme lorsqu'il énumère tout, mais imprime encore `accord=OUI`
si son cap laisse des flips non jugés. Le second a une identité entière correcte
sous `sum(weights)<=65535` ; il ne fait cependant qu'essayer sept pondérations,
n'impose pas son cap dans le header et possède un mutant de largeur à overflow
signé. Le juge primal indépendant reçoit une base `k=2`, mais pas encore le cas
ternaire, la profondeur ni la tuile uniforme. Le HEAD inclut aussi les diagnostics de
feuilles et d'ordre de descente ; leurs cohortes et coûts restent à recevoir.

Le raffinement local réduit effectivement `E4`, mais ses parents et enfants
sont encore mélangés dans plusieurs compteurs de tentative. Son coût doit être
jugé après séparation `AttemptStats/TerminalLedger` et avec héritage des preuves
`ALL/NONE`. La campagne CPU G4 a calculé ses pentes après coup, coupe les
métriques physiques et n'exige pas la finalité ; elle n'est pas une campagne de
qualification. Voir
[`audits/AUDIT_CONTRE_RAFFINEMENT_LOCAL_ET_SESSION_G4_3C11BC8_20260813.md`](audits/AUDIT_CONTRE_RAFFINEMENT_LOCAL_ET_SESSION_G4_3C11BC8_20260813.md)
et son contre-reçu au pin `35fcea8`.

## Construire et tester

Depuis la racine du dépôt :

```bash
cmake -S morsehgp3D_v3 -B build/v3 -DCMAKE_BUILD_TYPE=Release
cmake --build build/v3 --parallel
ctest --test-dir build/v3 --output-on-failure
python3 tools/check_docs.py
```

CUDA reste opt-in avec `-DMHGP3V_ENABLE_CUDA=ON`. Une session GCP éventuelle
doit suivre exclusivement les scripts gardés et les coupe-circuits décrits par
`AGENTS.md`.

## Arborescence documentaire

- [`PROPOSITION.md`](PROPOSITION.md) : proposition technique et mathématique
  consolidée ;
- [`audits/AUDIT_ETAT_COURANT.md`](audits/AUDIT_ETAT_COURANT.md) : unique
  verdict mutable ;
- [`audits/README.md`](audits/README.md) : index court des audits actifs et des
  dépendances historiques encore citées par le logiciel ;
- `oracle/` : juges bornés indépendants ;
- `prototype/` : candidats et probes, sans autorité produit implicite ;
- `receipts/` : diagnostics et reçus, dont le statut est fixé par l'audit.
