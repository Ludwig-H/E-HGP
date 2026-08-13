# Contre-audit de `BallFormToBallEvent-v0` : le noyau géométrique est utile, l'étape 0A n'est pas close

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## 0. Pin, périmètre et verdict

Le pin relu est `HEAD=2b89ea127d979a60981e6741470f8d8bb49c63d6`, après les commits
`b1995a9` et `2b89ea1`. Les empreintes SHA-256 sont :

- `prototype/ball_event.hpp` : `89dd043b947556682f42b807f901039d98aeae06b586afe3813f4687e04db108` ;
- `prototype/ball_event_probe.cpp` : `4c39691a107f7c82f03e437ce11cb68c0c14e8782659b5b45fafb92db7919bf4` ;
- `CMakeLists.txt` : `e0e452597d37e11b27c0625b3be7544a567b5698123d97c10236bbfd3c72b2bd`.

Pendant le contre-audit, Claude a repris concurremment
`prototype/rect_front.hpp` et `prototype/wspd_wavefront_probe.cpp`. Ces deltas
ne sont ni attribués au pin ci-dessus, ni relus ici. L'auditeur n'a modifié
aucun logiciel.

Verdict : **le pin reçoit un noyau borné de construction de sphère et de census
par support, pas `BallForm -> BallEvent exact et politique de dégénérescence`
au sens de l'étape 0A.** Le statut correct est `PARTIEL` :

- reçu localement : accord de deux paramétrisations du centre et du signe de
  puissance, sur les supports que le sujet a déjà déclarés positifs et sous
  `coord<=64` ;
- ouvert : complétude et positivité des `BallForm`, identité `PointId`,
  canonicalisation indépendante de la `BallKey`, RLE avant census, activation
  des lanes, statut transactionnel de dégénérescence et véritable `BallEvent`.

Le message de commit, huit CTests verts et le compteur `refus_domaine` ne
ferment pas ces propriétés absentes.

## 1. Rejeu

Les commandes suivantes ont été rejouées :

```text
cmake -S morsehgp3D_v3 -B build/v3 -DCMAKE_BUILD_TYPE=Release
cmake --build build/v3 --parallel --target mhgp3v_wspd_wavefront_probe mhgp3v_ball_event_probe
ctest --test-dir build/v3 --output-on-failure -R '^mhgp3v_(wspd_wavefront_(fixtures_rang|fixtures_owner|q3_)|ball_event_)'
ctest --test-dir build/v3 --output-on-failure -R '^mhgp3v_ball_event_'
```

Le premier filtre rend `16/16` en `0,64 s`; le second rend `8/8` en `0,13 s`.
Le nominal cosphérique imprime :

```text
ball_event n=14 famille=grid coord=32 | formes=1456 degenerees=86 non_positives=1098 | supports=272 (arite2=91 arite3=115 arite4=66) | spheres_uniques=254 regulieres=196 refus_domaine=58 | cospheres=15 supports_sur_cospheres=33 | desaccords=0
ball_event accord=OUI
```

Le code de sortie est zéro malgré les `58` dispositions `kUnsupported`.
L'injection `cle-non-reduite` imprime `mutant_killed=1 fautes=262` et sort
quatre, mais ce résultat n'est pas une comparaison indépendante de clés, comme
le montre le § 3.3.

## 2. Ce qui est effectivement reçu

### 2.1 Deux constructions différentes du même centre borné

Le sujet construit les sphères q2, q3 et q4 par milieu, formule fermée du pied
et Cramer en coordonnées. Le juge reconstruit le centre par le système de Gram.
Pour chaque support accepté par le sujet, les deux routes s'accordent sur la
dégénérescence affine et sur le côté rationnel de chaque point du petit nuage.
Le mutant qui compte la coquille comme intérieur est bien pris par la
comparaison `I_B/U_B` qui reste armée.

Cette parité est utile : elle reçoit le noyau `SphereEquationAndCensus-v0` dans
le domaine explicite de la sonde. Elle ne reçoit pas encore la sélection des
supports ni l'événement aval.

### 2.2 Groupement géométrique en mémoire

La forme primitive réduite par pgcd et signe positif permet au `std::map` de
rassembler plusieurs supports cosphériques. La famille `grid` exerce réellement
des runs à supports multiples et des coquilles supplémentaires. C'est un bon
plancher de non-vacuité pour le futur RLE.

Le type C++ `PrimitiveSphereKey` doit toutefois être lu comme un codec interne
des cinq coefficients de la `BallKey` géométrique, pas comme une seconde
identité sémantique. La `BallKey` produit inclut aussi schéma et identité du
nuage; `I_B/U_B` appartiennent au `BallEvent` ou au `SphereRun` et ne créent pas
une nouvelle clé après census.

## 3. Pourquoi l'étape 0A reste ouverte

### 3.1 Le juge ne juge pas la positivité

`be_positive` appartient au sujet. Lorsqu'il retourne faux, le probe incrémente
`non_positives` et sort immédiatement de la forme. Le juge de Gram ne calcule
jamais indépendamment les coordonnées barycentriques strictes et ne compare
jamais l'ensemble des supports positifs.

Un mutant qui inverse une coordonnée barycentrique, accepte une égalité ou
rejette un vrai support peut donc modifier le catalogue sans produire de
`desaccords`. Le nombre `1098 non_positives` montre seulement que le filtre a
mordu, pas qu'il a décidé juste.

Porte minimale : le juge forme ses propres barycentriques rationnelles pour
q3/q4, compare le bit positif pour **chaque** sous-ensemble affine indépendant
et tue séparément les mutants `>0` contre `>=0`, signe d'un cofacteur et centre
sur une face.

### 3.2 Les indices denses sont appelés `PointId`

La sonde ne porte aucun tableau de labels. Les ensembles `S`, les
`SupportKey`, les `I_B/U_B` et `EdgeKey` sont tous des indices de vecteur. Le
mutant `owner-index` choisit la première arête du tuple; il ne rejoue pas la
faute historique `GenerationRank` contre `PointId`.

Le juge owner rappelle en outre `be_owner`, le helper du sujet. Il peut tuer la
première-arête injectée, mais pas une faute commune comme un tie-break sur la
plus grande `EdgeKey` ou sur l'ordre Morton.

Porte minimale : ajouter des `PointId` indépendants des positions denses,
permuter stockage et labels séparément, construire l'owner du juge sans le
helper sujet et comparer l'`EdgeKey` attendue, pas seulement un nombre de
fautes. L'équivariance sous relabeling est distincte de l'invariance sous
permutation du stockage.

### 3.3 Le mutant de clé est auto-déclaré mort

Le juge rationnel ne construit aucune clé primitive de référence. Sous
`--inject=cle-non-reduite`, la sonde ajoute inconditionnellement
`runs.size()` au nombre de fautes. Toute exécution non vide tue donc ce mutant,
même sans comparer la clé injectée à une canonicalisation indépendante.

La puissance reste inchangée par une échelle positive; l'accord de signes ne
peut pas recevoir le pgcd. Il faut comparer les octets ou coefficients
canoniques produits par une deuxième route, exercer deux écritures rationnelles
de la même sphère et vérifier les tuples attendus des fixtures déjà documentées.
Le mutant ne doit mourir que par une différence d'objet observable.

### 3.4 Le census précède le RLE dans l'exécution réelle

La boucle `for z=0..n-1` est exécutée dans `traite(support)`, avant
`runs[key]`. Le pin paie donc un census complet pour chaque support, puis les
regroupe. Sur le nominal `grid`, il effectue `272` scans pour `254` sphères;
sur une cosphère lourde, il paierait le scan pour chaque support incident.

L'étape promise est l'inverse :

```text
formes positives -> BallKey + SupportKey -> sort/RLE BallKey
                  -> un range-count/census par BallKey unique
                  -> joindre tous les supports au BallEvent
```

Le ledger bloquant doit établir `census_calls=unique_BallKeys`, avec au moins
deux supports pour une clé, puis tuer le mutant `census-par-support`.

### 3.5 Aucun `BallEvent` ni activation de lane n'est encore décidé

Le seul record est `SphereRun`. Il ne contient ni `CloudEpoch/CloudDigest`, ni
schéma de sérialisation, ni `source_complete`, ni activations q2/q3/q4, ni
`q_min`, ni masque d'ordres, ni niveau exact, ni statut de transaction. Le
champ `rank=|I_B|+|U_B|` ne remplace pas les décisions par support
`|I_B|+|S|<=smax`.

Le type `Disposition::kPlateauPending` n'est jamais produit. Le probe classe
les runs en `kRegular/kUnsupported`, imprime `accord=OUI` et sort zéro. Il
n'exerce donc ni le refus `unsupported_degeneracy`, ni l'annulation atomique
d'un manifeste, ni une sortie lossless de plateau.

Le mode diagnostic peut légitimement inventorier les dispositions et sortir
zéro, mais il doit alors se nommer `diagnostic_classify`. Une porte d'admission
doit, elle, rendre le statut contractuel attendu et prouver qu'aucun payload
partiel n'a été engagé.

### 3.6 Le juge de Gram n'est pas une autorité u16

Le dépassement de largeur ne concerne pas seulement le sujet. Pour
`M=65535` et le triangle
`(M,0,0),(0,M,0),(0,0,M)`, le numérateur de rayon carré construit par le juge
vaut `96*M^10`, soit environ 167 bits. Les carrés du juge q4 montent encore
plus haut. `i128` ne couvre donc pas le profil `quantized_u16_input_only`, même
après suppression des casts `long long` du sujet.

La réparation 0A exige une autorité BigInt/rationnelle ou des formes
homogènes dont chaque largeur est prouvée. Un oracle qui déborde avec le sujet
ne peut pas recertifier son domaine public. Cette fixture doit comparer clé,
positivité et census attendus, puis tuer un backend i128 sans preflight.

### 3.7 Le générateur de fixture peut ne jamais terminer

La commande admise suivante ne termine pas :

```text
timeout 2 ./build/v3/mhgp3v_ball_event_probe \
  --family=clusters --points=5 --coord=4
```

Elle rend `124`, car la famille n'offre que quatre positions distinctes et la
boucle de rejet attend un cinquième point sans budget. Chaque famille doit
publier sa capacité exacte ou conservative, refuser `invalid_input` avant
tirage et borner ses essais. La porte exerce capacité, capacité plus un et une
graine hostile.

## 4. Réparation minimale remise à Claude

Ne pas jeter ce noyau. Le fermer verticalement en six pas bornés :

1. ajouter au juge indépendant positivité q2/q3/q4 et owner sur vrais
   `PointId` ;
2. faire produire au juge la `BallKey` canonique complète et comparer la valeur
   exacte, y compris pgcd, signe, schéma et identité du nuage ;
3. séparer `enumerate/forms -> count/sort/RLE -> census unique -> join`, avec
   les identités `planned=filled=consumed` et `census_calls=unique_BallKeys` ;
4. construire les activations par `SupportKey`, avec `p=|I_B|`, arité réelle,
   owner et lanes q2/q3/q4, puis comparer l'ensemble complet au BallForm oracle ;
5. distinguer explicitement `diagnostic`, `regular_success`,
   `unsupported_degeneracy`, `resource_exhausted` et `incomplete`, tous
   transactionnels ;
6. seulement ensuite raccorder spool, tri global, lots, fold, dix forêts,
   verticales et `BenchmarkOutputContract-v1` pour fermer 0B.

Fixtures prioritaires : triangle aigu, droit et obtus; tétraèdre positif puis
centre sur une face; deux écritures rationnelles d'une même sphère; vrais
labels `PointId` permutés; deux supports pour une `BallKey` avec un seul census;
huit puis neuf intérieurs stricts; extra-shell avec code
`unsupported_degeneracy`; cap moins un sans payload; permutation, tuilage et
sérialisation stable.

## 5. Conséquence pour le générateur q3

Le constat ne retire rien à la bonne solution q3. Une fois l'autorité aval
fermée, la source candidate reste :

```text
fenêtre E3 certifiée et finale
  -> owner-edge × carrier aigu, avec masse M3 préflightée
  -> BallKey géométrique primitive + RLE
  -> Q3FootPowerRange LBVH capé au neuvième intérieur
  -> un census par BallKey survivante
  -> BallEvent puis fold
```

Une WSSD aiguë peut proposer ou compresser les blocs `edge×carrier`; elle ne
borne ni `M3`, ni les sphères uniques, ni la sortie. `SOC64`, `CORNER512` et le
LP projectif améliorent surtout la fenêtre universelle; ils ne remplacent pas
le pied q3 ni son rang. Le bon générateur q3 reste donc conditionnellement
sparse et output-sensitive, jamais sparse par la seule acuité.

## 6. Contre-audit de l'autre auditeur

Les quatre résultats mathématiques de
`AUDIT_REPONSE_PLAN_VERTICAL_SOC64_LP_1AA487D_20260813.md` ont été revérifiés
indépendamment :

- `SOC64` est une équivalence pour le produit relaxé indépendant
  `Ebox×Tbox`; son succès est un `ALL` sûr du vrai rectangle et son échec reste
  `UNKNOWN` ;
- `CORNER512` est bien un théorème pour `ALL` sur l'enveloppe AABB continue du
  prédicat spindle, par convexité séparée en `a`, `b` et `z`; il ne décide ni
  la présence de points, ni la source Morse, et l'échec reste `UNKNOWN` ;
- `LP1` caractérise exactement la propriété « toute sphère passant par `a,b`
  possède un intérieur dans le pool complet »; sur un pool capé, seul son
  succès est globalement sûr ;
- la récurrence de suppression donne exactement la profondeur universelle
  `h`, sous stricte, suppression par `PointId`, rangs un/deux/trois et pool
  mondial authentifié.

Deux formulations de l'autre audit sont resserrées. « Oracle pairwise complet
du résiduel » doit se lire **oracle complet de profondeur universelle des
sphères par la paire**, propriété plus forte que les seules sphères Morse; un
échec reste fail-open pour la source. Et la borne `3280 LP` compte des appels,
pas leur coût ni celui du reporter global.

Sa correction des cages est conservée : une base positive minimale 3D peut
avoir quatre à six sites. Une tétra-cage reste un fast path exact, jamais une
preuve d'`UNDERFULL` général. Le cutoff par hull exige que le point cible
inversé soit strictement dans le hull; `0` intérieur seul ne ferme aucune
cible donnée. Les largeurs du tri de rayons et toute loi de coût des cages
restent à recevoir.

Enfin, l'autre audit a depuis repris le statut « étape 0A close ». Le présent
contre-rejeu le corrige : ses théorèmes géométriques restent valides, mais les
absences des §§ 3.1 à 3.5 interdisent encore ce statut de réception.

## 7. Décision

Conserver `ball_event.hpp` comme candidat de noyau borné et oracle de
construction, corriger ses juges et son ordre RLE/census, puis fermer la vraie
tranche. Ne pas lancer de campagne de parcimonie ou de G4 sur ce probe.

Le contrat `50 000`, G4, p95 sous une seconde reste entièrement ouvert : aucun
kernel résident, aucune source E3/E4 complète, aucun fold officiel et aucun
`BenchmarkOutputContract-v1` ne sont produits par ce pin.

GCP non utilisé.
