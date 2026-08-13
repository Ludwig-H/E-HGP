# Proposition consolidée MorseHGP3D v3

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Ce document fixe la route candidate actuelle. Il ne promeut aucune phase et ne
remplace ni les spécifications sous `docs/`, ni le verdict mutable
[`audits/AUDIT_ETAT_COURANT.md`](audits/AUDIT_ETAT_COURANT.md). Une brique est
« reçue » seulement lorsque son autorité indépendante, ses mutants, sa
complétude, ses statuts d'échec et ses ressources sont exercés.

## 1. Objectif et interdits d'architecture

La cible secondaire est `p95 warm_e2e<1 s` pour `n=50000`, `K_max=10`, sur un
seul G4; la cible principale reste `100 ms`. Le chronomètre officiel couvre
entrée, construction, source, census, fold, dix forêts, verticales, payload,
synchronisation et sortie exigée par `BenchmarkOutputContract-v1`.

Le chemin produit ne construit jamais :

- mosaïque de Delaunay d'ordre supérieur ;
- arrangement global de plans ou de droites ;
- matrice globale de cofaces ;
- catalogue résident de tous les supports ;
- tableau indexé par toutes les paires, triplets ou quadruplets.

Un oracle exhaustif à petit `n` est obligatoire pour juger le chemin produit,
mais il ne devient jamais son ordonnance.

## 2. Identités et statuts avant toute optimisation

### 2.1 Identités

- `PointId` est une identité stable, distincte de l'index dense, de Morton et
  de `GenerationRank`.
- `SupportKey` est le tuple trié des vrais `PointId` du support.
- L'owner d'un support maximise la longueur carrée de ses arêtes puis choisit,
  en cas d'égalité, la plus petite `EdgeKey=(min PointId,max PointId)`.
- La `BallKey` géométrique est formée **avant** le census. Elle contient
  l'identité du nuage, le profil, le schéma et les cinq coefficients primitifs
  de `A||z||^2+B dot z+C`, normalisés par pgcd et signe `A>0`.
- `I_B` et `U_B` appartiennent au `BallEvent` ou au `SphereRun`; ils ne créent
  pas une seconde clé post-census.
- `PrimitiveSphereKey` peut nommer un codec interne des cinq coefficients. Il
  ne remplace ni l'epoch du nuage, ni le schéma de la `BallKey` persistante.

Le code historique où une clé contient `shell_min` n'est pas l'ABI produit :
une donnée issue du census rendrait le RLE pré-census circulaire.

### 2.2 Statuts transactionnels

Chaque exécution termine dans un état typé :

```text
complete_regular
unsupported_degeneracy
resource_exhausted
numeric_failure
incomplete_continuation
invalid_input
```

Un cap, une allocation refusée, un overflow ou une continuation non consommée
ne publie jamais un préfixe de payload. La séquence est
`count -> preflight -> fill -> validate -> publish` et les identités
`planned=filled=consumed`, `pending=0` sont bloquantes.

## 3. Étape zéro : recevoir l'objet avant la parcimonie

La chaîne de réception est :

```text
0A  BallForm exhaustive -> BallKey/RLE -> census -> BallEvent exact
0B  BallEvent -> spool scellé -> tri/merge exact -> lots -> fold
    -> dix forêts -> verticales -> BenchmarkOutputContract-v1
```

L'émission par ancre n'a aucun watermark monotone. Le chemin peut écrire des
runs spillables, mais il scelle la source et merge globalement les niveaux
avant le premier commit d'un lot. « Streamé » signifie mémoire résidente
bornée, jamais fold en ligne sur une source encore ouverte.

### 3.1 Statut de `BallFormToBallEvent-v0`

Le pin `2b89ea1` fournit un candidat borné `coord<=64`. Il n'est pas reçu pour
le profil u16 :

- des numérateurs q3/q4 de 67 à 81 bits sont rabattus vers `int64` ;
- des carrés jusqu'à environ 162 bits sont formés en `i128` avant le pgcd ;
- la positivité n'est pas jugée indépendamment ;
- l'owner ne sépare pas index dense et `PointId` ;
- le mutant de clé s'auto-déclare fautif sans clé de référence ;
- le census est exécuté par support avant le groupement par sphère ;
- `kUnsupported` est compté, mais le diagnostic publie encore `accord=OUI` et
  sort zéro ;
- aucun `BallEvent` versionné, niveau exact, lane ou manifeste transactionnel
  n'est construit.

L'autorité reçue localement se limite à deux constructions rationnelles du
centre et au signe de puissance sur les supports conservés par le sujet.

### 3.2 Réparation arithmétique

Pour un centre rationnel `N/den` et un point `p` de la sphère, ne pas former
`den^2` et `N^2` avant réduction. Employer directement :

```text
A = den
B = -2*N
C = 2*N dot p - den*||p||^2
```

Pour q3, la forme compacte de § 5.3 évite entièrement le centre gonflé. Pour
q3 et q4, la positivité se décide sur les numérateurs barycentriques
Gram--Cramer, avec une preuve de largeur sur tout le cube u16. Toute conversion
rétrécissante est précédée d'un preflight exact ou supprimée.

### 3.3 Ordre RLE/census obligatoire

```text
formes positives
  -> BallKey + SupportRecord
  -> count/sort/RLE BallKey
  -> range-count saturé par BallKey unique
  -> census complet par BallKey survivante
  -> joindre I_B/U_B à tous les supports incidents
```

La porte exige `census_calls=unique_BallKeys`, deux supports pour une clé et un
mutant `census-par-support`. L'oracle juge séparément dépendance affine,
positivité, clé primitive, niveau, owner, `I_B/U_B` et activation de lane.

## 4. Fenêtre d'arêtes certifiée

Pour chaque lane, `E_q(a)` contient les seconds endpoints dont la paire n'est
pas fermée par assez de crédits universels à IDs disjoints. Une vraie arête
maximale canonique de support doit rester dans cette fenêtre. Le reporter porte
des spans de `GenerationRank`, jamais une table de `PairId`.

Les fates sont exclusifs :

```text
input_mass = closed_mass + open_mass + pending_mass
```

`pending_mass=0` est nécessaire à une fenêtre finale. Deux range-adds par span
ouvert puis un scan calculent les degrés orientés et leur somme sans expansion
des paires. Une égalité avec l'expansion PairId est exigée sur petit `n`.

Après la fenêtre, `EdgeActiveCarrierCounter` mesure avant tout fill :

```text
M3 = sum over open q3 edges of active q3 carriers
M4 = sum over open q4 edges of active forms
```

Une fenêtre d'arêtes sparse ne borne ni `M3`, ni `M4`, ni les sphères uniques.

## 5. Générateur q3 recommandé

### 5.1 Réduction exacte arête-owner × porteur

Pour `d=b-a`, `u=x-a`, poser :

```text
D = d dot d
E = u dot u
F = d dot u
X = D+E-2F
V = 2u-d
```

Si `ab` est maximale faible, donc `D>=E` et `D>=X`, les deux angles adjacents
à `ab` sont strictement aigus. La positivité q3 équivaut alors à :

```text
V dot V > D
```

car `V dot V-D=2(E+X-D)`. L'égalité est un triangle droit. Chaque q3 est donc
une incidence canonique `owner EdgeKey(ab) × PointId(x)`, pas un triplet libre.

### 5.2 Rôle d'une WSPD/WSSD aiguë

Une WSPD/WSSD ternaire peut proposer des blocs `ALL/NONE/MIXED` ou compresser
le broad phase. Elle n'est pas l'autorité de la source :

- un bloc `ALL_ACUTE(A,B,C)` peut représenter une masse cubique ;
- la frontière droite peut raffiner jusqu'aux points ;
- l'acuité ne décide ni profondeur, ni shell, ni owner, ni `BallKey` ;
- une représentation linéaire ne borne pas l'expansion ni la sortie.

Le bon usage est une ablation sur le même ledger transitif `E3, M3, BallKeys,
census, supports, fold`, jamais une conclusion depuis le nombre de cellules.

### 5.3 Pied unique et puissance exacte

Pour un carrier owner, poser :

```text
G = D*E-F^2
W = E*(D-F)*d + D*(E-F)*u
```

`G>0` est l'indépendance affine. Le centre q3 vaut `a+W/(2G)`. Pour
`v=z-a`, le prédicat de puissance mis à l'échelle est :

```text
P_x(z) = G*||v||^2-v dot W
```

`P_x(z)<0` signifie intérieur strict et `P_x(z)=0` coquille. Les cinq
coefficients pré-census sont :

```text
A = G
B = -(2G*a+W)
C = G*||a||^2+a dot W
```

Ils sont réduits par pgcd/signe puis intégrés à la `BallKey`. Sous u16, les
évaluations exigent environ 105 bits signés; le chemin device emploie des limbs
explicites, jamais un `i64` implicite.

### 5.4 `Q3FootPowerRange-v0`

Après RLE, une wavefront LBVH traite `(BallRun,NodeKey,count<=9)`. Sur une AABB
entière, `P_x` est somme de trois quadratiques convexes. Par axe, le minimum
entier se trouve aux deux entiers voisins de `W_i/(2G)` clipés et le maximum à
une extrémité.

- `max P<0` crédite toute la population du nœud ;
- `min P>=0` élague le nœud pour le rang strict ;
- sinon la tâche se scinde ;
- le neuvième intérieur rejette q3 sous `smax=11` ;
- une survivante paie un seul census complet.

Ce backend est le v0 parce qu'il est simple à juger, pas parce qu'il domine
asymptotiquement tous les cas. Le ledger publie tâches run×nœud, populations
créditées, feuilles, rejets au neuvième, opérations larges, octets et HWM.

### 5.5 Préfixes Morton et niveaux shallow

Une AABB serrée ne donne aucun packing. Pour un terminal reçu avec `Dlo>0` et
`kappa=Dhi/Dlo` borné, choisir un niveau Morton dyadique commun dont la maille
est comparable à `sqrt(Dlo)/s3`. Le nombre de cellules alignées rencontrant la
région carrier est conditionnellement :

```text
O(s3^3 * kappa^(3/2)) par terminal
```

Cette borne ne contrôle ni le nombre de terminaux, ni leur population, ni
`M3`. Les cellules half-open, limites de profondeur et cas `Dlo=0` sont
explicitement délégués.

Si les visites LBVH sont rouges, `Q3FootLevelLocate-v1` construit seulement les
`k+1` bas niveaux des formes P et hauts niveaux des formes N. La complexité
combinatoire des niveaux est `O(mk)`, mais le coût conserve les point-locations
des `f` pieds, les bundles pondérés et les concurrences. Sans sweep reçu, une
cible honnête inclut `f log m`.

### 5.6 Limite de sortie

Des configurations réelles en dimension trois possèdent un nombre quadratique
de triangles Delaunay critiques, aigus et vides. Cette obstruction interdit une
promesse universelle de catalogue q3 sous-quadratique; elle ne donne pas encore
un pire cas `50000` sur grille u16 fixe. Une fixture u16 cocyclique de 384
points porte déjà `2322560` supports aigus pour une seule sphère. Le RLE sauve
le census, jamais une provenance de supports explicitement exigée.

Le contrat q3 est donc output- et resource-sensitive. Sur les familles sparse,
la route ci-dessus peut être rapide; sur une sortie lourde, elle fournit un
preflight exact, un quotient reçu ou `resource_exhausted`.

## 6. Crédits universels : LP, rectangles et cages

### 6.1 Théorème LP projectif

Translater l'ancre en zéro. Pour `s_i=z_i-a`, `q_i=||s_i||^2`, `d=b-a` et
`D=||d||^2`, définir :

$$\kappa_P(d)=\min\left\lbrace \sum_i\alpha_iq_i:\sum_i\alpha_is_i=d,\ \alpha_i\geq0\right\rbrace.$$

Le pool `P` place au moins un intérieur dans toute sphère passant par `a,b` si
et seulement si `d` appartient au cône positif de `P` et `kappa_P(d)<D`.
L'égalité reste ouverte. Un optimum basique emploie au plus trois IDs.

Ce théorème décide `UniversalSphereDepth-1`, propriété plus forte que les
seules sphères Morse. Sur un pool capé, le succès est sûr et l'échec fail-open.
La complétude exige le pool mondial authentifié.

Si un groupe `G` crédite une fois, la profondeur universelle vérifie exactement :

```text
C_0(P,d) = true
C_h(P,d) iff, for every z in G, C_(h-1)(P without z,d)
```

Avec des bases de taille au plus trois, q4 demande au plus 3280 appels LP,
q3 9841 et q2 29524. Ce sont des nombres d'appels d'oracle borné, pas une borne
du reporter ni un hot path par paire. Rangs un/deux, stricte, shell, suppressions
par `PointId` et coût du solveur exact restent explicites.

### 6.2 `SOC64` et `CORNER512`

Pour `e=z-a`, `t=b-z`, `H=e dot t`, `E=||e||^2`, `X=||t||^2` :

```text
q2 : H>0
q3 : H>0 et 4H^2>EX
q4 : H>0 et 3H^2>EX
```

À `t` fixé puis à `e` fixé, chaque lane est une section convexe de cône de
Lorentz. Si les 64 couples de coins du produit relaxé
`(C-A)×(B-C)` passent, le vrai rectangle est `ALL`. Un échec reste `UNKNOWN`,
car les deux différences partagent réellement le même `z`.

Pour `a,b` fixés, le spindle est aussi convexe en `z`. Le prédicat est donc
séparément convexe en `a,b,z`; les 512 triples de coins caractérisent exactement
`ALL` sur l'enveloppe AABB continue. Un coin fictif échouant ne donne jamais
`NONE` sur les seuls points du nœud.

Le classifieur `JungSpindleRect-v0` du pin `7d2efcb` est une autre borne : il
combine des extrema séparés de `D,V,T`. Son disjonctif est sûr mais n'a gagné
qu'environ trois centièmes de point dans le diagnostic déclaré à `n=6000,s=8`.
Cette mesure ne réfute ni `SOC64` ni `CORNER512`, qui ne sont pas encore
implémentés; elle réfute l'espoir que les extrema décorrélés suffisent sur des
boîtes grossières.

Même exact pour `ALL`, `CORNER512` ne prouve aucune parcimonie. Compter ses
early exits, opérations larges, tâches et gain transitif avant de le porter.

### 6.3 Cages de quatre à six sites

Une cage ancre-globale minimale est une base positive de dimension trois et
peut contenir quatre, cinq ou six sites. Un constructeur tétra-only est un fast
path exact mais incomplet. Une cellule de Voronoï locale à six facettes a au
plus huit sommets; sa fleur fournit donc un nombre constant de formes
quadratiques vérifiables sur un `BNode`.

Huit cages à unions d'IDs disjointes ferment q4, neuf ferment q3. Une banque
bornée est un proposer : échec, égalité, reuse d'ID, rang inférieur, overflow
ou cap délègue toujours. `0` intérieur au hull inversé prouve seulement un
rayon borné; une cible est fermée lorsque son point inversé est strictement
dans le hull, ou lorsque toutes les facettes transformées passent.

Le tri exact des rayons rationnels peut être plus large que les formes
ponctuelles; multiprécision ou majorant conservateur puis replay des formes est
requis. Une cage minimisée doit recalculer sa fleur. Aucune loi globale de coût
ne découle de la seule existence de cages.

## 7. q4 : shallow local seulement après les portes E4/M4

Pour une arête `ab`, tout site actif induit une forme affine dans le plan
médiateur. q2 interroge le point central; q3 le pied unique d'une ligne; q4 les
intersections shallow de deux formes. Les lanes partagent un codec, pas leurs
sorties.

Le moteur q4 candidat construit localement les niveaux nécessaires des formes
P et N et émet seulement les intersections `P-P`, `N-N` et `P-N` dont le rang
peut satisfaire la lane. Il traite atomiquement lignes confondues, bundles
pondérés et concurrences; il rejoue indépendance, positivité, owner, Jung,
rang, `BallKey` et census.

Il ne construit aucun arrangement global. Il n'est instancié que si `E4`,
`M4`, tâches et HWM sont verts. Le code exhaustif actuel reste son oracle, pas
son modèle GPU.

## 8. Plateaux, fold et sortie

Le profil u16 n'exclut pas les cosphères. La politique candidate régulière est
fail-closed : un extra-shell **pertinent pour une lane admise** retourne
`unsupported_degeneracy` tant qu'aucun quotient complet n'est reçu. Cette
fermeture de `RelevantGP` reste elle-même à prouver.

Un `SphereRun` interne lossless conserve `BallKey`, `I_B/U_B`, supports ou
handle de provenance et disposition. Il ne rend pas un plateau publiable. Un
quotient saturé H0 doit encore recevoir joins par intersection, lots gelés,
coverage, dix forêts et verticales contre Gamma exhaustif. Si le contrat exige
chaque `SupportKey`, ni quotient non reçu, ni streaming ne supprime la borne de
sortie.

Le fold consomme des références exactes opaques, pas les coordonnées ni les
limbs natifs. Dix millions de points n'imposent pas binary64 : la grille u16 3D
contient `2^48` sites. Le codec d'index dense peut devenir u32 séparément. Un
futur profil binary64 exact est une phase distincte derrière `ExactKernel` et
une sérialisation BigInt/dyadique variable.

## 9. Ordre de réalisation vers le G4

1. Fermer 0A sur tout u16 : largeurs, vrais `PointId`, juge indépendant,
   `BallKey`, RLE avant census, lanes et statuts transactionnels.
2. Fermer 0B sur petit `n` jusqu'au payload complet, sous permutations,
   tilings, caps et reprises.
3. Substituer seulement la source : fenêtre E3/E4 finale, puis portes M3/M4.
4. Implémenter q3 `owner-edge -> BallKey -> Q3FootPowerRange`; comparer WSSD
   aiguë seulement comme broad phase sur le même coût transitif.
5. Implémenter q4 shallow local seulement après E4/M4 verts.
6. Porter la tranche entière par count--scan--fill, tâches persistantes, SoA,
   radix/RLE et arithmétique large reçue.
7. Exécuter des microgates `1500/3000/6000`, puis
   `12500/25000/50000` uniquement si tâches, octets, HWM et sorties passent.
8. Qualifier trente répétitions chaudes à `50000` avec le payload officiel.

La porte composée publie au minimum :

```text
E3/E4, max par ancre, CLOSED/OPEN/PENDING
M3/M4, blocs, tâches, splits, visites
BallKeys brutes/uniques, supports, census, shells
sortie H, opérations larges, octets/HWM
temps par phase et warm_e2e
commandes, seeds, HEAD, diff, hashes, codes de sortie
```

Une pente seule, un taux de fermeture, un `OK` CPU ou une extrapolation de
bande passante ne qualifie aucun SLO.

## 10. Fixtures permanentes prioritaires

- owner équilatéral/isocèle sous permutation indépendante stockage/PointId ;
- q3 aigu, droit, obtus, `G=0`, `P=-1/0/+1`, seuil huit/neuf ;
- triangle et tétraèdre u16 maximaux, clés et barycentriques attendus ;
- deux supports pour une `BallKey`, un census, deux `SupportRecord` ;
- cosphère 384 sans troncature et extra-shell pertinent fail-closed ;
- partenaire q3 au-delà d'un cutoff de rang 128 ;
- `SOC64` succès axial et faux-échec du produit relaxé ;
- `CORNER512` coin omis, égalité, axe dégénéré et largeur 70 bits ;
- LP rang un/deux/trois, égalité `kappa=D`, pool capé et IDs supprimés ;
- cage octaédrique six-sites, tétra-only, reuse d'ID et rayon mal arrondi ;
- caps exacts puis moins un, continuation, permutation, tuilage et reprise ;
- comparaison lot par lot des dix forêts, coverage et verticales.

## 11. Non-claims

Il existe une voie q3 exacte, GPU-factorisable et conditionnellement sparse :
`owner-edge × carrier -> pied -> BallKey/RLE -> profondeur batchée`. Il n'existe
aucune preuve que « triangles aigus » fournisse seul une source sparse, ni une
garantie universelle sous-quadratique.

`SOC64`, `CORNER512`, LP projectif et cages sont des certificateurs ou
falsificateurs précis. Aucun n'a encore démontré le contrat 50000/G4. Le
contrat reste ouvert jusqu'à production du même `BenchmarkOutputContract-v1`
complet dans le p95 déclaré.

GCP non utilisé pour cette proposition.
