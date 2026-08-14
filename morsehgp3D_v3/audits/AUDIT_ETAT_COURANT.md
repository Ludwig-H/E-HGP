# Audit courant de MorseHGP3D v3

Date : 14 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

> **Alerte au `HEAD=069d903`.** Les commits `89774d0`, `e3f1925` et
> `88a9ba8` ajoutent respectivement `Corner8BallDepth`, un broad phase WST3 et
> son produit WST4 ; `22d1cb0` en publie la note de coût. Le successeur
> `3703097` avait branché un filtre d'acuité au signe inversé. `a73161c`
> applique maintenant l'identité exacte `E+X-D=-2H`, garde `H<0`, ajoute la
> fixture du tétraèdre régulier et rétracte le gain `1,62x`. La fixture vérifie
> les deux valeurs `H=-1` et les six longueurs égales ; elle ne recalcule pas
> indépendamment les quatre barycentriques `1/4` ni l'owner sur de vrais
> `PointId`. Le même commit stabilise une option `--echelle=num/den`
> réciproque de son contrat : sa porte `1/256` reste verte par surcouverture et
> ne distingue pas le mutant `256/1`.
> `069d903` ajoute ensuite un diagnostic Corner8 sur un préfixe du produit brut.
> Il confirme que la séparation aide certains certificats, mais ne reçoit pas
> la thèse « séparation nécessaire » : un support q4 valide peut avoir deux
> apex proches, et la diagonale `C=D` doit être raffinée par `Sym2`, pas jetée.
>
> Le théorème de la
> miniboule unique est reçu uniquement pour un **support minimal positif
> complet**. Corner8 est un certificat `ALL_INTERIOR` conditionnel sur un vrai
> support q4 déjà authentifié ; ses `6/6` portes ne jugent ni positivité, ni
> owner, ni IDs, et son oracle ponctuel recopie le prédicat du sujet. Les
> `5/5` portes WST3 et la porte WST4 reçoivent une couverture dans le rectangle
> owner choisi par leur juge, pas une source physique exact-once : le tape brut
> conserve les ancres non-owner et les diagonales, tie-breake par rang Morton et
> rejette à tort les positions dupliquées. Le verdict détaillé et les réponses
> Q6--Q9 sont dans le nouveau contre-audit support-complet.
>
> Le commit `069d903` raccorde Corner8 seulement comme diagnostic après
> matérialisation du WST3 et parcours explicite des couples WST4. Il ne retire
> aucune tâche aval, n'a ni CTest ni juge causal, et saute désormais les
> couples `C--D` non séparés : c'est une fermeture partielle fail-open, pas une
> source complète. Son helper d'orientation centré n'est pas transmis à
> `corner8_block`, qui recalcule l'ancien intervalle.
>
> Les défauts antérieurs restent actifs : les portes HC saines sont à regex et
> ne jugent pas chaque promotion ; `--fenetre-exhaustive` contourne les gates ;
> `cred+reste` ne majore que le ledger singleton baseline et perd encore une
> fermeture SOC combinée à `n=16` et deux fermetures BJD q4 à `n=64`. La porte
> borne ne compare ni fates ni masses à une exécution OFF.
> Le verdict exact sur l'idée de miniboule unique est
> [`AUDIT_MINIBOULE_UNIQUE_RESIDUEL_SHALLOW_5809BD2_20260814.md`](AUDIT_MINIBOULE_UNIQUE_RESIDUEL_SHALLOW_5809BD2_20260814.md) : l'événement q2 canonique est bien
> la boule diamétrale ; q3 et q4 ont chacun une boule canonique une fois
> leur support complet. Sur une ancre partielle, le sandwich `U<=D<=C` choisit
> entre fermeture, `tau(F)` et passage immédiat aux complétions.
> La dernière session G4 a échoué avant sa rampe et la cible a été certifiée
> `TERMINATED` : elle ne fournit aucune mesure 50k.

## Snapshot

Le dernier commit stable relu est
`HEAD=069d90329c0562963ea8b4b40eec2a0102918456`, commit
`Louis avait raison sur s : la separation gouverne la decidabilite`. Il
absorbe le raccord Midball de `a58d020`, HC et la borne de `c1e2e3b`, Corner8
de `89774d0`, WST3 de `e3f1925`, WST4 de `88a9ba8`, la correction du signe
aigu, l'échelle rationnelle et le diagnostic Corner8 postérieur au broad phase.
Le delta logiciel mobile de Claude porte à nouveau sur
`prototype/corner8_ball.hpp` et `prototype/wst3_probe.cpp`; il expérimente un
certificat bisigne. Hors présentes écritures documentaires, aucun logiciel n'a
été modifié par l'auditeur.

Le commit stable ajoute une échelle rationnelle `num/den` dont le commentaire
annonce `diag2<=num/den*rayon2`, mais compare actuellement
`diag2*num<=rayon2*den`. Numérateur et dénominateur sont inversés : augmenter
le rapport raffine dans le code alors que l'API annonce des cellules plus
grosses. La comparaison conforme au commentaire est
`diag2*den<=rayon2*num`, en i128. Le cas historique `1/1` ne distingue pas ce
mutant ; une paire de portes `4/1` et `1/4` est nécessaire.

Le rejeu causal `uniform,n=1000` donne `6 159 060` blocs WST4 à `1/1`,
`35 494` à `1/64` et `65 167 274` à `64/1`. À `1/4096,n=60`, l'arrêt racine
rend encore le juge owner vert par pure surcouverture et publie
`3 132 900=binom(60,2)^2` tuples candidats. Cela ne reçoit aucune sélectivité
aval.

Empreintes SHA-256 au commit `069d903` :

- `CMakeLists.txt` :
  `a4d6f7ed0e1746a5a7a2f88921fe138d3eb01ad550077b253abc72c918b16f48` ;
- `prototype/wst3_probe.cpp` :
  `f6d1203eb97f62a2695956bfadd6aea0fee56b0eec353c8a8423c3f5ecbafc3f` ;
- `prototype/corner8_ball.hpp` :
  `ee4d03e320e678e29428b9263620e9795ec66d83034fd3fef53df70424d2ea36` ;
- `prototype/corner8_probe.cpp` :
  `9f29100613543cdf1f246243ec7b8112b4c84826618c4a053b3332361c49115f` ;
- `prototype/wspd_wavefront_probe.cpp` :
  `6330b79586066c575c59e4480a17fdf864fdef77b261a3a7f33c66bd68ed9c5b` ;
- `prototype/cloud_families.hpp` :
  `f825334096c80407c57e2ca05f6f59f6ae3dd6313746beb8e73d689e9082dded` ;
- `prototype/midball_block.hpp` :
  `921e649f0ebbbfb7a8034bedaeeb0a14a2eaaadf9eadb092f4f8c3cdbfd9403b` ;
- `prototype/midball_probe.cpp` :
  `587ecc58ebdd592245449fef901be9d2fac3f4b9287752648554c48f2e7dcc49` ;
- `prototype/jung_dual_probe.cpp` :
  `d05a997c1c10e1e02918f3a585e7cdee45fda6814edf4930ccbae9b4030fd4e6` ;
- `prototype/block_jung_dual.hpp` :
  `99d960eb42e452a7cec126428945c1a56bf8984c7b138d18d3ea837f9a56ae5d` ;
- `receipts/soc64_actif_g4_20260814/transcript.txt` :
  `18e6dd0f1aeafca51639805761a15855acbd08f4446ef9b4cbe7132db64977eb`.

Le commit stable reçoit la primitive BJD, un packing disjoint sûr sur ses
campagnes causales, huit portes BJD ciblées, treize portes Midball, cinq portes
HC, une porte de réfutation de la borne, six portes Corner8 et huit portes
WST3/WST4 enregistrées. Les verts ciblés ne ferment pas les défauts de composition et
d'identité ci-dessus. Il ne reçoit ni projection CK--WST distinct-ID/owner,
ni profondeur `tau(F)`, ni ledger persistant de vrais `PointId`, ni primitive
device, ni payload. Son `--fenetre-exacte` n'est exact que pour la miniboule q2
échantillonnée sous domaine régulier ; ses lanes q3/q4 publient un majorant par
témoins universels. Le préfixe SplitMix fixe ne reçoit pas les hypothèses
probabilistes des crochets Hoeffding.
Les titres de commits restent des claims, jamais des verdicts reçus. GCP non
utilisé par le présent auditeur.

## Verdict

Le contrat G4 reste ouvert. Il n'existe encore ni source u16 reçue, ni stage
`0B` produit, ni `BenchmarkOutputContract-v1` complet et éligible, ni campagne
p95 à 50 000 points. Le squelette du contrat de benchmark existe, mais ses
étages critiques restent incomplets ou absents.

`SOC64 --actif` et `BlockJungDual64` sont des diagnostics amont. Aucun ne
traverse `BallEvent -> 0B -> payload`; ils ne qualifient donc ni exactitude
industrielle ni SLO.

Le snapshot contient au moins sept probes/campagnes utiles, mais aucune chaîne produit
reçue :

1. `BallFormToBallEvent-v0` forme des sphères et des census sur petit domaine ;
2. `PointHypergraphBottleneckClosureProbe-v0` compare Kruskal et Floyd sur un
   même hypergraphe de points ;
3. le raffinement local réduit le résiduel `E4` du certificateur central, au
   prix d'un front et d'un nombre de recertifications supérieurs ;
4. la rampe à 50 000 points rejette la configuration centrale mesurée sur
   `eight_clusters`, sans réfuter les certificateurs rectangles corrélés ;
5. `q4_brute_oracle` énumère les 4-sous-ensembles à petit `n`, avec un census
   et des prédicats encore corrélés/incomplets sur le shell ;
6. Corner8 certifie conditionnellement des blocs de témoins q4 `ALL` ;
7. WST3/WST4 compte une couverture candidate avant projection owner/positivité.

Les noms « 0A fermé », « stage 0B » et « le raffinement paie » dépassent les
preuves disponibles.

## Nouvelle source candidate : `CKPairTape -> WST3 -> WST4`

L'audit reçoit le contrat mathématique comme proposition exacte et
GPU-factorisable. Le probe courant n'en implémente que le `CandidateCover` :

- une WSPD Callahan--Kosaraju canonique partitionne toutes les paires en
  `O(s^3 n)` rectangles physiques sans les développer ; après filtrage des
  seules paires endpoint `D=0`, elle source tous les q2 propres. Un bucket de
  positions dupliquées conserve obligatoirement la multiplicité et les vrais
  `PointId` dans les pools témoins et les produits ; les paires vers toute
  troisième position gardent cette multiplicité ;
- pour chaque rectangle `R=(A,B)`, les cellules Morton d'une échelle liée à sa
  boule `B_R` et rencontrant `2B_R` couvrent tout carrier d'un support dont
  `ab` est l'arête maximale ; l'intersection avec les deux enveloppes endpoint
  resserre encore ce domaine. Après projection distinct-ID, owner
  longueur/`EdgeKey` et positivité, on obtient un unique
  `OwnedCK-WST3(A,B,C)` ;
- après la même projection, les couples non ordonnés de ces cellules donnent
  un unique `OwnedCK-WST4(A,B,C,D)` ; q4 recertifie directement les quatre
  barycentriques du circumcentre. Le compteur brut antérieur à cette projection
  n'est pas `Owned`.

Les nombres de blocs **initiaux** conditionnels sont `O(s^3 n)`,
`O(s^3*eta^-3*n)` et `O(s^3*eta^-6*n)` sous une vraie propriété
fair/compressed-split et `0<eta<=1`. Ils ne bornent ni les raffinements `MIXED`,
ni les masses logiques `|A||B|`, `|A||B||C|` et `|A||B||C||D|`. Chaque bloc
reste paresseux jusqu'à un consommateur factorisé reçu ou au preflight atomique
de sa vraie sortie.

Une fixture u16 de 64 points interdit toute cascade de rang : un q4 régulier a
rang 4 alors que ses six arêtes q2 et ses quatre faces q3 ont toutes rang 12.
`WST4` doit donc consommer les carriers aigus géométriques pré-rang, jamais les
événements q3 retenus. La relation est construite lorsque
`q3_open || q4_open`, même si la lane de rang q3 est déjà fermée. Le rapport et le recalcul exact sont dans
[`AUDIT_SOURCE_CK_WST_Q2_Q3_Q4_35FCEA8_20260814.md`](AUDIT_SOURCE_CK_WST_Q2_Q3_Q4_35FCEA8_20260814.md).

Pour une paire ponctuelle, `JungDiskDepth9/8` restreint les centres au disque
imposé par Jung dans le plan médiateur. Des groupes disjoints de trois IDs au
plus peuvent y fermer q3/q4 même lorsque le LP sur tout le plan échoue. Cette
preuve ne ferme pas un rectangle CK : le plan et les demi-plans varient avec
`a,b`. Une contre-fixture `2×2` est désormais documentée ; il faut scinder vers
des microtiles rejoués ou prouver un `BlockJungDiskDepth` uniforme.

## Miniboule canonique : oui au support complet, non à la cascade

Pour un support minimal positif affinement indépendant fixé, la miniboule et
donc la boule canonique de son événement complet sont uniques : diamètre q2,
centre/rayon donnés par le circumcercle intrinsèque du triangle aigu q3 mais
boule et census ambiants en dimension trois,
circumsphère du tétraèdre bien centré q4. Pour les arités inférieures à quatre,
une famille de sphères ambiantes reste incidente au support sans être portée
minimalement par lui. Le circumdisque
planaire q3 est le cœur des sphères incidentes, pas l'événement ambiant. Une
arité supérieure conserve toutefois des centres différents tant que son support
n'est pas complet. Deux fixtures séparées montrent qu'une paire q2 de rang
douze peut porter respectivement un q3 de rang trois ou un q4 de rang quatre ;
une face q3 de rang douze peut porter un q4 de rang quatre. Un troisième point
de shell ne change pas nécessairement l'arité minimale : le triangle droit
reste q2.

Pour un domaine continu `K` contenant le centre canonique, les quantités
`U=singletons universels`, `D=profondeur collective minimale` et
`C=profondeur au centre canonique` vérifient `U<=D<=C`. L'ordre exact est donc :

```text
C<h      -> sauter ce certificateur, sans décider les cofaces
U>=h     -> fermer
U<h<=C   -> tau(F), sweep ou split
support complet -> census de son unique boule canonique
```

Le sampler du commit mesure `U`, pas `D`. Ses pentes proches de `1,09` sont des
pentes d'un majorant échantillonné ; `U<h` n'exhibe aucune sphère peu profonde.
La fixture Jung à huit groupes ferme q4 avec `U=0`, réfutant le non-converse et
l'inclusion Delaunay affirmée dans la note de Claude. Le résultat reste utile
pour éviter des appels Jung lorsque `C<h` et pour fermer immédiatement lorsque
`U>=h`, à condition de rester blockwise. Un scan de 50 000 témoins pour chacun
des `1 392 028` blocs historiques `s=2` dépasserait 69 milliards de tests.

## `MidballBlockDepth` stable : théorème q2 reçu, ABI et raccord ouverts

Le minimum de `H(a,b,z)=(z-a) dot (b-z)` implémenté au pin est exact sur
l'AABB continue et `hmin>0` constitue donc un `ALL` q2 sûr. Son maximum teste
les entiers voisins du point stationnaire : il est exact sur le réseau u16, pas
sur l'enveloppe continue. Avec `A={0}`, `B={1}`, `C=[0,1]`, le code trouve
`hmax=0` et `NONE`, alors que le maximum continu vaut `1/4`. Ce verdict reste
sûr pour les vrais sites quantifiés ; il doit être typé `NONE_LATTICE_U16` ou
remplacé par le numérateur exact d'échelle quatre avant toute réutilisation
continue.

Le header ne préflighte ni domaine u16, ni `lo<=hi`, ni paire propre, ni
identités et n'expose aucun `INVALID/UNKNOWN`. Il duplique
`rect_h_interval`, dont le calcul et les fixtures portent eux aussi un domaine
réseau insuffisamment typé dans l'API. Les 13/13 CTests affichés verts ne ferment
pas ces points : trois portes saines emploient `PASS_REGULAR_EXPRESSION` ;
`--selftest=1` imprime `accord=OUI` puis rend le code `3`, démontrant que le
texte précède le plancher. La fixture de non-hérédité ne recertifie pas encore
l'acuité q3 ni les barycentriques positives q4.

Le raccord stable ALL-only est l'ordonnance saine : ajouter uniquement un gain
`ALL`, sans remplacer le `NONE` du certificateur central. Au snapshot courant,
il compile et affiche 13/13 CTests ciblés verts, mais la porte saine WSPD à regex
reste fail-open. Le source appelle la primitive complète, mais GCC Release
élimine déjà le maximum inutilisé : le bloc machine de promotion contient 24
multiplications. Une ABI min-only reste utile pour l'autorité et le device, pas
comme gain CPU présumé. Sur
`eight_clusters,n=1500,s=8`, une ablation alternée réduit les lectures de
`7,41 %` et le résiduel q2 de `29,95 %`, avec `pending=0`, mais augmente la
médiane de vague de `14,1 %` sur la machine partagée. Cette hausse ne peut pas
être attribuée à `axis_max`, absent du binaire. La porte suivante est une
autorité partagée min-only, un juge de chaque promotion et un différentiel
causal profilé de lectures/temps.

Le juge live reste ouvert : ses compteurs sont globaux à une liste de tailles,
son produit de cap `na*nb*m` peut déborder i64, il juge les fermetures finales
plutôt que chaque promotion et le retour de `--fenetre-exhaustive` contourne ses
planchers. Le plancher de gain doit être une option de campagne. La seule
dominance reçue est `central ALL => Midball ALL`; Midball peut au contraire
promouvoir un `central NONE`, par exemple sur `A=[0,8],B=[10,100],C={9}`.

## `HCBlockDepth` au HEAD : formule sûre, intégration non reçue

Le delta HC emploie `H=(z-a) dot (b-z)` et
`C=(b-z) cross (z-a)`. Les conditions q3 `3H^2>||C||^2` et q4
`2H^2>||C||^2` sont exactes ; `Hmin` exact et une majoration par composantes de
`C` donnent un `ALL` sûr mais conservateur. Ce n'est ni la source des supports
q3/q4 ni une nouvelle autorité exacte : `corner512_all_lane` reçoit déjà
l'enveloppe continue complète. Le commentaire du commit ne perd pas seulement
la corrélation entre les deux produits d'une composante : il perd aussi la
simultanéité entre les trois maxima de composantes, puis entre `hmin` et le
pire `C`. Une fixture publiée isole la perte entre composantes ; les deux
autres pertes sont identifiées algébriquement et doivent encore devenir des
fixtures permanentes.

Les cinq CTests HC du commit passent, avec les deux mutants tués. Les portes
saines `mhgp3v_hc_selftest` et `mhgp3v_hc_vwave` restent toutefois à
`PASS_REGULAR_EXPRESSION` ; `--selftest-hc=1` imprime `accord=OUI` avant son
plancher puis rend code `3`. Le raccord recalcule HC jusque trois fois par
nœud, n'a aucun juge de chaque promotion, hérite ses compteurs entre tailles et
est contourné par `--fenetre-exhaustive`. `--hc --midball` finit
structurellement code `3`, HC ayant absorbé tous les gains q2 avant le plancher
Midball. À `eight_clusters,n=200`, les lectures baissent
`247966 -> 226535`, mais la vague one-shot monte `119,8 -> 214,2 ms` ; à
`n=3000`, le commit rapporte `31 538 327 -> 27 552 424` lectures mais une
médiane CPU `8,3 -> 13,7 s`. Ces diagnostics reçoivent la réduction logique,
pas un chemin chaud ni le SLO.

## `--borne-sup` au HEAD : invariant utile, raccord multivue réfuté

Pour une seule vue de crédits singleton, l'idée est exacte. Si les tâches
empilées forment une antichaîne, poser pour chaque lane
`P=cred+sum(pop(task))`. Un `ALL` transfère la population de la pile au crédit,
un `NONE` la retire, et un `MIXED` remplace le parent par deux enfants disjoints
de même population totale. Ainsi `P` ne croît pas ; `P<h` prouve que cette
source singleton ne fermera plus la lane.

La première révision, hash `90640885`, soustrayait le parent `MIXED` sans
réinsérer ses enfants. Elle transformait donc cette borne supérieure en
sous-estimateur. Fixture causale `uniform,n=16,coord=64,seed=1,window=1024` :
la baseline ferme une q2 après `3171` lectures ; la révision annonce zéro
fermeture après `117` lectures, `351` lanes mortes et aucune troncature. La
révision désormais commise réinsère bien les deux populations ; cette fixture
retrouve la fermeture et passe de `3171` à `2177` lectures. Le premier défaut
doit rester un mutant permanent `drop-mixed-children`.

Le raccord reste néanmoins faux sur les modes acceptés :

- `reste/mort` ne suit que `cred/mask`. Il efface ensuite `m`, donc aussi
  `cmask`, alors que la vue combinée peut avoir `ccred>cred`. Sur
  `terrain,n=16,coord=64,seed=3,--soc64-shadow`, la vue combinée passe d'une
  fermeture à zéro avec la borne ;
- BJD propose des crédits collectifs **après** la boucle à partir des feuilles
  vues. La borne des singletons ne majore pas ces groupes et l'arrêt précoce
  réduit leur banque. `terrain,n=64,coord=64,seed=3,BJD8` passe ainsi de
  `84` à `82` q4 fermées et de `338` à `319` groupes avec code zéro ;
- `visites_evitees` ne compte ni les tâches encore empilées au break, ni leurs
  descendants, et les compteurs sont globaux aux tailles. Ce n'est pas encore
  une mesure du travail supprimé.
- `--climb` initialise le potentiel sur les seuls frères remontés et omet la
  feuille `pos0` ; la mortalité porte alors sur un parcours incomplet, pas sur
  tous les témoins géométriquement disponibles.

La CTest `mhgp3v_borne_sup_refutation` passe, mais ne vérifie qu'une ligne de
compteurs par regex : aucune exécution OFF n'est comparée et les combinaisons
fausses restent acceptées. La réparation minimale sépare `reste/mort` par vue et garde le parcours vivant
sur l'union de leurs bits. Jusqu'à une borne composée authentifiée, le mode doit
refuser BJD et tout proposant post-boucle. Les portes permanentes comparent
fates, masses et fermetures exactes à la baseline sans troncature, mordent
`drop-mixed-children`, le cas `cred=0,ccred=7,reste=1,h=8`, la fixture BJD
ci-dessus, les fenêtres capées et plusieurs tailles. Aucun gain de la révision
commise n'est recevable avant cette parité. Le snapshot falsifié, la portée
exacte du lemme et les obligations sont détaillés dans
[`AUDIT_LIVE_BORNE_SUP_CREDITS_A58D020_20260814.md`](AUDIT_LIVE_BORNE_SUP_CREDITS_A58D020_20260814.md).

## `Corner8BallDepth` au HEAD : lemme `ALL` reçu, événement non raccordé

Pour un support q4 fixé et positif, `sigma*J(S,z)` est strictement convexe en
`z`. Ses huit coins donnent donc un certificat `ALL_INTERIOR` complet sur une
AABB témoin. Les largeurs `|O|<2^51` et `|J|<2^87` tiennent sous u16 lorsque
les signes sont jugés séparément. Les `6/6` portes ciblées passent.

La primitive ne préflighte toutefois ni boîtes u16, ni quatre IDs distincts,
ni positivité barycentrique, ni owner, ni shell ou `BallKey`. Son oracle de
bloc réemploie les prédicats ponctuels du sujet. Trois nominales sont à regex ;
`--selftest=1` imprime `accord=OUI` puis rend code `3`. Deux mutants déterminants
restent hors porte, `kProduitOJ` n'est pas implémenté et le mutant nommé
`corners-outside-implies-none` produit en réalité un faux `ALL`.

La fixture de 4096 supports prouve un bloc q4 utile, pas le raccord : avant
owner/positivité/census, sa télémétrie est `domain_mass_closed`. Un bloc `ALL`
paie environ 1508 multiplications d'extrémités `i128`, et non huit opérations.
Le détail, les fixtures `drop-corner`, support non positif et q3 ambiant sont
dans
[`AUDIT_CONTRE_RECEPTION_SUPPORT_COMPLET_CORNER8_WST34_22D1CB0_20260814.md`](AUDIT_CONTRE_RECEPTION_SUPPORT_COMPLET_CORNER8_WST34_22D1CB0_20260814.md).

## `WST3/WST4` au HEAD : candidate cover, pas source `Owned` reçue

Le filtre d'acuité rouge de `3703097` est réparé au `HEAD=a73161c`. Pour
`H=(x-a) dot (b-x)`, une face est aiguë en `x` lorsque `H<0`. La classification
exacte est `Hmax<0 => ALL_ACUTE`, `Hmin>=0 => NONE_ACUTE`, sinon `MIXED`.
Le juge WST4 contrôle cependant toujours les couples bruts et ignore le booléen
`aigu`; la nouvelle fixture tue le signe inversé, mais ne reçoit ni owner, ni
positivité q4, ni la relation filtrée.

Le ratio stable `--echelle=num/den` reste inversé entre son commentaire et sa
comparaison entière. Le code teste `diag2*num<=rayon2*den`, donc un rapport plus
grand raffine, alors que l'API annonce `diag2<=num/den*rayon2`, où un rapport
plus grand grossit les cellules. Les mesures étiquetées `1/256` décrivent le
comportement du code, pas la grandeur documentée.

Le lemme arête maximale--lentille est correct et les campagnes bornées couvrent
chaque triplet/quadruplet dans le rectangle de l'owner choisi : WST3 `5/5` et
WST4 `1/1`. Le juge ne cherche cependant aucune copie sous les ancres non-owner,
ne compte pas les diagonales du tape, tie-breake avec les rangs Morton plutôt
qu'avec les vrais `PointId` et refuse les positions dupliquées. Sujet et juge
partagent ces deux dernières conventions.

Ainsi les blocs courants sont une `candidate-cover exact-once après projection
distinct-ID et owner`, pas encore `OwnedCK-WST3/WST4`. La projection distinct-ID, owner et
positivité doit rester factorisée avant le produit. L'arrêt LBVH à échelle
grossière donne une surcouverture sûre mais aucune borne de cellules par
rectangle ; les pentes et la constante proche de 32 sont des diagnostics finis.
La masse WST4 proche de `n^4` et `187` millions de blocs à `n=8000` rendent le
produit brut rouge, sans mesurer encore `M4_apex`.

La réparation de performance est une vraie grille Morton au niveau lié à
`r_R`, puis `ALL/NONE_ACUTE`, profondeur et owner uniforme **avant** tout
`sum k_t^2`. Le count de masse des paires de cellules emploie
`binom(sum |C_i|,2)` en temps linéaire par rectangle et en u128, jamais une
boucle `u<v`. La sortie reconvertit encore l'u128 en `double` à six chiffres
pour l'affichage : le reçu exact doit publier l'entier. Les questions Q6--Q9 et
la gate coût appariée sont résolues dans le contre-audit ci-dessus.

Le raccord Corner8 commis n'économise encore aucun produit : il est
exécuté après le count WST4 et après le juge, sur `par_terminal` déjà
matérialisé. Il ne supprime aucune tâche aval et ne possède ni CTest de jonction,
ni juge causal, ni refus sur support non owner/non positif. Son crédit de huit
spans témoins disjoints est une voie `ALL` potentiellement sûre sous les
préconditions de Corner8, mais ses compteurs `fermes/masse_fermee` portent la
candidate-cover brute, pas des supports q4 authentifiés. Un cap `--corner8`
tronque en outre le diagnostic; il ne doit jamais être interprété comme une
fermeture complète.

La diagonale `C=D` contient le choix `x=y`, donc son intervalle d'orientation
contient nécessairement zéro. Elle exige un raffinement paresseux exact de
`binom(C,2)` en `LL/LR/RR`; la supprimer perdrait les q4 dont les deux sommets
restants partagent la cellule. Le worktree saute plus largement tout couple
`C--D` non séparé. C'est sûr pour un diagnostic de fermetures partielles, mais
incomplet ; malgré son commentaire « tous les couples », il ne teste ni
`A--C/A--D` ni `B--C/B--D`. Le prétest d'orientation centré est sûr, mais
`corner8_block` recalcule ensuite l'ancien intervalle au lieu de consommer son
signe : la promesse « une seule fois » et le gain associé ne sont pas reçus.
L'enclosure centrée contient explicitement un terme `e1` linéaire dans les
rayons ; son commentaire « erreur quadratique » est faux. `sep_ok(C,D)` reste
seulement un filtre suffisant de coût. Une arête-owner longue peut avoir deux
apex proches ; une source q4 exacte doit raffiner `C×D` ou laisser `PENDING`,
jamais supprimer la complétion faute de séparation.

Le worktree suivant essaie de contourner le signe uniforme par deux ledgers de
témoins, `J<0` et `J>0`. Le principe peut être sûr : les supports `O>0`
consomment huit vrais `PointId` du premier ledger, les supports `O<0` huit du
second, et `O=0` n'est pas q4. La réception exige toutefois un oracle de cette
jonction, des reçus d'identités séparés, le passage causal avant le produit et
la suppression du préfixe biaisé. Sa télémétrie mélange encore les strates de
signe, peut publier un taux supérieur à 100 %, emploie la mauvaise masse sur la
diagonale et reconvertit u128 en `double`. Aucun taux de ce delta mobile ne
qualifie le SLO.

La rampe CPU locale `12500/25000/50000`, `--echelle=1/256` au sens du code et
Corner8 désactivé, rend la gate physique rouge. Sur uniform, les deux dernières
pentes des visites sont `1,907/1,036`; sur huit amas, `F3`, visites et `F4`
finissent respectivement à `1,756`, `2,050` et `2,193`. À 50 000, la masse
candidate vaut environ `3*binom(n,3)` en q3 et `6*binom(n,4)` en q4 : l'échelle
coarse compresse presque l'univers brut sous plusieurs ancres, elle ne le
sélectionne pas. Le prochain gain doit donc fermer ou projeter owner/positivité
sur les descripteurs avant toute expansion ; une nouvelle rampe de préfixes
Corner8 ne répondrait pas à ce verrou.

## P0 : `0A` reste ouvert sur u16

- les constructeurs q3/q4 rabattent vers `int64` des numérateurs qui atteignent
  respectivement 81 et 67 bits sur des fixtures u16 valides ;
- les chemins de clé et de positivité forment ensuite des intermédiaires au-delà
  de 128 bits ;
- `be_level_num` et `be_level_cmp` multiplient en `i128` signé avant de tester
  l'overflow. Sous UBSan, le mutant de clé existant déclenche un comportement
  indéfini à `ball_event.hpp:290` ;
- le vert Release de ce mutant n'est pas causal : `desaccords=0`,
  `fold_desaccords=0`, puis `fautes=263`, dont 262 proviennent de
  `runs.size()` ajouté sans comparaison à une vérité et une de l'erreur
  numérique ;
- le juge partage encore des identités, calculs et filtres avec le sujet ;
- `PointId`, epoch, profil, schéma, lanes, complétude et publication
  transactionnelle ne forment pas encore une ABI reçue ;
- le census reste payé par support avant le RLE par `BallKey`.

Une nouvelle forme homogène terminale enlève toutefois le besoin mathématique
du chemin q4 à 174 bits : avec `T=det3(v1,v2,v3)` et
`Q=sum_i ||v_i||^2*(v_j cross v_k)`, les quatre numérateurs de Cramer tiennent
sous 106 bits et la BallForm sous 87 bits sur u16. La formule complète, ses
bounds et fixtures sont maintenant dans `PROPOSITION.md`. Elle n'est pas encore
implémentée ni jugée par une autorité BigInt ; elle transforme le P0 q4 en
réparation i128/deux-limbs actionnable, pas en réception de `0A`.

Autorité, valeurs exactes et fixtures :
[`AUDIT_BALL_EVENT_V0_2B89EA1_20260813.md`](AUDIT_BALL_EVENT_V0_2B89EA1_20260813.md).

## P0 : le fold live n'est pas `0B`

Le sujet écarte chaque `SphereRun` dont la disposition n'est pas `kRegular`,
puis le sujet et son juge consomment les mêmes runs, membres, niveaux et rangs.
Le nominal grid peut ainsi imprimer `refus_domaine=99` puis
`ball_event accord=OUI fold=OUI`.

Le calcul unionne `I_B union U_B` dans une seule DSU de `PointId`. Il juge une
primitive de goulot pour `K=1`, pas Morse/HGP aux ordres `1..10`. Dès `k=2`,
deux générateurs qui partagent un seul point doivent rester séparés, alors que
la DSU de points les fusionne. Manquent notamment :

- générateurs/facettes par ordre et lanes ;
- lots de niveaux égaux avec racines pré-lot gelées ;
- naissances, multifusions, coverage et continuations ;
- neuf applications verticales et payload officiel ;
- preflight complet et zéro sortie sur événement unsupported.

Le juge Floyd est en outre un oracle borné : ses deux matrices d'entiers
coûteraient environ 20 Go à 50 000 points et sa fermeture ferait
`1,25e14` triplets. Il ne peut devenir l'architecture produit.

Contre-audit et fixture `k=2` :
[`AUDIT_CONTRE_RECEPTION_STAGE_0B_3C11BC8_20260813.md`](AUDIT_CONTRE_RECEPTION_STAGE_0B_3C11BC8_20260813.md).

## Raffinement local : levier reçu comme diagnostic seulement

À `n=3000`, `s=8`, `window=512`, une exécution locale a mesuré :

| famille | profondeur | masse q4 ouverte | terminaux | recertifications |
|---|---:|---:|---:|---:|
| `uniform` | 0 | 1 027 538 | 885 188 | 108 858 186 |
| `uniform` | 4 | 464 599 | 1 221 936 | 193 020 841 |
| `eight_clusters` | 0 | 4 045 644 | 363 018 | 31 538 327 |
| `eight_clusters` | 4 | 2 597 699 | 1 182 988 | 199 169 436 |

Le ledger terminal est exclusif et final dans ces six rejeux. En revanche les
anciens compteurs `bank.closed`, `pending_lane` et `mass_closed_q2` comptent des
tentatives parentes avant leur scission, puis leurs enfants : ils peuvent
dépasser 100 %. Séparer impérativement `AttemptStats` de `TerminalLedger`.

Le prochain jalon utile est `ProofCarryingLocalRefinement-v0` : un enfant
hérite les spans `ALL/NONE` du parent et ne rejoue que sa frontière `MIXED`.
Mesurer ensuite le coût transitif `E4 -> F3/C4_carrier -> F4/M4_apex ->
BallKeys -> census -> fold`, pas la seule pente de `E4`.

Audit complet et défauts de la recette G4 :
[`AUDIT_CONTRE_RAFFINEMENT_LOCAL_ET_SESSION_G4_3C11BC8_20260813.md`](AUDIT_CONTRE_RAFFINEMENT_LOCAL_ET_SESSION_G4_3C11BC8_20260813.md).

## Rampe 50 000 : no-go borné, pas pivot automatique vers les cages

La session G4 CPU du commit publie quarante `code=0` et une provenance
substantielle. Les pentes q4 post-calculées sont arithmétiquement correctes.
Pour `eight_clusters/r4`, elles valent `1.898146 / 1.909154 / 1.911063`, avec
`pending=0` aux quatre tailles. Le seuil numérique préannoncé `1,7` justifie
donc l'arrêt d'ingénierie de
`CentralBall209 + DVT scalaire + s=8 + window=512 + raffinement r=4` sur cette
famille et cette graine. Le protocole plus large demandait plusieurs graines ;
ce n'est pas une réception asymptotique.

La portée s'arrête là :

- le script lance une taille par processus et ne calcule aucune pente ;
- `--max-slope=9` est inerte et aucune gate `sum_E4` n'est armée ;
- la complétude compte les lignes `code=` sans tester explicitement leur
  valeur. Un code non nul fait actuellement disparaître la ligne sous
  `errexit`, donc le fichier est rejeté, mais le diagnostic promis est perdu ;
- `terrain` contient du pending q4 à 25 000 et 50 000 ;
- le filtre retire recertifications, temps et HWM, donc le « prix exact » du
  front n'est pas mesuré.

`uniform` seule ne peut qualifier le SLO : `TEST_PLAN` §14.5 et G6 exigent
Poisson uniforme et le mélange équilibré de huit amas. `sum E4` reste la gate
du certificateur universel mesuré, mais la contre-famille `two_lines` interdit
d'en faire une gate de la source : elle conserve une masse quadratique avec
zéro carrier aigu et zéro q4.

Réponses complètes aux trois questions de Claude :
[`AUDIT_REPONSE_CRITERE_MORT_SOC64_LP_35FCEA8_20260813.md`](AUDIT_REPONSE_CRITERE_MORT_SOC64_LP_35FCEA8_20260813.md).
Le détail contractuel du reçu et des continuations est dans
[`AUDIT_CONTRE_RECU_RAMPE_RAFFINEMENT_G4_35FCEA8_20260813.md`](AUDIT_CONTRE_RECU_RAMPE_RAFFINEMENT_G4_35FCEA8_20260813.md).

## `C4/M4/H4` : le mur avant rang est réel, les claims du HEAD ne le sont pas

Le schéma courant sépare `C4_carrier` pour les faces aiguës,
`M4_apex` pour les quadruplets canoniques avant barycentriques,
`W4_positive` après positivité et `H4_rank` après census. L'owner maximal doit
rester dans la route proposée : changer seulement l'owner ne réduit ni les
vrais supports ni les 4-ensembles à attribuer. L'arête maximale fournit un
diamètre qui borne immédiatement les cinq autres distances. Une autre broad
phase exigerait une nouvelle preuve de couverture ; une `BallKey` est aval et
serait un owner de génération circulaire.

Le sampler v2 améliore v1 : il exclut `PENDING` et ne censure plus les grosses
lentilles. Le HEAD a remplacé la fausse barre `2 sigma` par une demi-largeur
Hoeffding correcte **conditionnellement** à des tirages indépendants et
uniformes. Cette loi n'est pas reçue : multiply-high reste sans rejet exact,
les deux streams SplitMix à seed fixe n'ont pas de contrat d'indépendance et le
delta n'est pas réparti sur les décisions simultanées. `W4` n'a pas d'intervalle
propre. Le champ est honnêtement renommé `repetitions_consecutives`, mais le
contrôle direct ne compare toujours pas le décodeur rang--`PairId` à un mode
exhaustif déterministe. La vue combinée SOC reste absente.

`--rang` peut sortir code zéro sans `--porteurs` et sans aucune ligne rang. Sur
`two_lines,n=40`, il peut aussi juger zéro bien-centré puis réussir. Son taux
est conditionné par arête et par positif, sans poids `binom(|L_e|,2)` : ce
n'est pas `H4/W4`. Surtout, il compte seulement `I_B`. Le rang fermé vaut
`|I_B|+|U_B|`; sous le régime régulier q4 il faut `I<=7` **et** `U=4`, ou un
refus des extra-shells pertinents. Le libellé `rang<=7` est donc faux.

Le nouvel énumérateur brut confirme seulement des comptes exhaustifs à petit
`n`. Il recopie Gram--Cramer et in-sphere du sujet : l'énumération est
indépendante, les prédicats ne le sont pas. Son claim
`M4=Theta(n^4)` « pour n'importe quel nuage » est réfuté par sa propre famille
`two_lines`, où `M4=0`. Les tailles `60/90/120` ne prouvent ni `H4` linéaire ni
une marge de deux ordres ; à `eight_clusters,n=120`, le rapport moyen publié au
seuil sept est seulement `6,6`. Les cas `n<4` produisent encore `NaN`, et aucun
cap n'empêche le coût `O(n^5)`.

Le mur mathématique existe néanmoins. Le support strict
`a=(20,20,20)`, `b=(30,30,30)`, `x=(19,31,31)`, `y=(31,19,31)` possède owner
`ab` unique, deux faces aiguës et barycentriques `(47,3,55,55)/160`. Un produit
de voisinages réels conserve ces inégalités et montre qu'une masse logique
quartique peut exister avant rang. L'ancienne instanciation par cubes unitaires
u16 n'était cependant pas uniforme : seuls `2093/4096` supports passaient.

La fixture de bloc exacte mise à l'échelle utilise
`A=(20000,20000,20000)+{0,1}^3`,
`B=(30000,30000,30000)+{0,1}^3`,
`C=(19000,31000,31000)+{0,1}^3`,
`D=(31000,19000,31000)+{0,1}^3` et
`Z=(20000,20000,30000)+{0,1}^3`. Ses `4096` supports ont owner `AB`, une
barycentrique minimale `13217143/721310286>0` et les huit IDs de `Z`
strictement intérieurs. Après fixation du signe d'orientation, le déterminant
in-sphere normalisé est convexe en `z`; ses huit coins suffisent donc pour
certifier `ALL_INTERIOR`. La bonne cible est une preuve de profondeur **par
bloc avant fill**, pas une énumération plus rapide des quadruplets.

Le count M4 n'exige pas davantage de fill. Initialiser `M4_pending` à la masse
distinct-ID non décidée, calculée par Möbius sur les quinze partitions de
`A/B/C/D`. Pour une masse `m`, `ALL_Q` fait
`M4_pending-=m; M4_L+=m`, `NONE_Q` fait `M4_pending-=m` et `MIXED_Q` remplace
atomiquement le parent par ses enfants ; `M4_U=M4_L+M4_pending`.
`M4_L>B_fill` rejette seulement le fill ponctuel. `M4_U<=B_fill` reçoit sa
capacité, mais count final, offsets et publication exigent encore
`M4_pending=0`. L'identité exacte par arête/`PlaneKey` reste un microkernel
endpoint borné : à 50 000 points, un RLE global matérialiserait `1249975000`
arêtes et `62496250050000` incidences `(e,z)`. Les compteurs soustraits restent
complets ; les saturer séparément peut transformer `B+2-(B+1)=1` en zéro.
`M4_raw` pré-profondeur et `residual_output` après profondeur sont deux ledgers :
une fermeture de profondeur crédite `domain_mass_closed`, pas `M4_closed`.

La route reçue comme proposition est : proposer des bases sur l'arête, les
vérifier par `BlockJungDual64`, puis fermer seulement si `tau(E_Q)>=8` ;
`FaceAxisJungDepth8` après la porte aiguë, puis
`Corner8BallDepth/BlockBallDepth8` après la cellule apex. Un reçu de profondeur
`ALL` ferme un bloc, `MIXED` scinde et la sweep ne reçoit que le résiduel
preflighté. `2B_R` est une enveloppe extérieure sharp lorsque seule la
boule contenant les endpoints est connue ; elle réduit les cellules, pas la
masse réelle. Une grande masse logique peut tenir dans peu de blocs, mais elle
n'est utile que si le consommateur de profondeur reste lui-même factorisé.
Sur une face fixe, ce consommateur est désormais explicite : poser
`p=min(8,n_permanents)`, puis conserver les `8-p` seuils gauches les plus grands
et les `8-p` seuils droits les plus petits donne un noyau d'au plus 16 IDs qui
préserve exactement le seuil `Depth>=8`. Un scan
top-k `O(n)` et un replay des égalités remplacent la sweep et le DAG général ;
une égalité uniforme est groupée comme shell, seul un ordre indécis scinde.
Les bouts irrationnels montent vers 207 bits sous u16 et exigent i256/quatre
limbs ; le lift bloc garde sa propre preuve de largeur.

La dissection `n=1500` trouve huit témoins singleton exacts pour `89,5 %` de
200 PairId q4 ouverts tirés par masse, contre `26,5 %` de 200 rectangles hachés.
Elle révèle du potentiel pairwise, sans mesurer la perte causale : les deux
échantillons ne sont ni appariés ni pondérés dans la même unité, et le second
n'exerce aucun groupe Jung. L'option live `--ordre-proche` réduit fortement le
pending aux fenêtres 32--128, mais à finalité 256/512 le résiduel reste exactement
`E4=1071162` dans les deux ordres. C'est une compression de budget du
certificateur central, pas une réduction M4.

Réponses aux six questions, preuves, contre-fixtures et portes :
[`AUDIT_REPONSE_M4_PORTEURS_AIGUS_4515A8B_20260814.md`](AUDIT_REPONSE_M4_PORTEURS_AIGUS_4515A8B_20260814.md).
Le contre-audit du sampler v2, du brute-force, la réponse entière à la question
7 et les microgates `JungDual/BlockBallDepth8` sont dans
[`AUDIT_CONTRE_RECEPTION_M4_V2_DEPTHBLOCK_5BFC5C8_20260814.md`](AUDIT_CONTRE_RECEPTION_M4_V2_DEPTHBLOCK_5BFC5C8_20260814.md).

## `JungDepth` et `BlockJungDual64` au HEAD stable : théorème fixe reçu, intégration ouverte

La forme entière duale `A/P/R` est correcte sous u16, paire propre et
`sum(weights)<=65535`, mais elle vérifie seulement les poids fournis. Son échec
reste `UNKNOWN`. `BlockJungDual64::make_base` préflighte la somme en `i128`, mais
`dual_lane` l'additionne encore en `i64` sans ce cap ; aucun des deux headers ne
porte les `PointId`, et le mutant étroit utilise un overflow signé. Le wrapper
futur authentifie donc profil, cap, IDs et disjonction. Il peut exposer une
primitive nommée `verify_dual_weights_lane`, mais ce symbole n'existe pas au
`HEAD` et ne serait jamais un décideur de couverture.

Le HEAD ajoute le bon juge indépendant pour une base fixe. Dans le plan
`s dot (b-a)=0`, il minimise exactement `||s||^2` sur les demi-plans mauvais,
en BigInt, puis compare strictement `3*r^2>D` ou `2*r^2>D`. Les formes
`K_ij`, `Delta` et `N` sont algébriquement cohérentes et les fixtures q4 de
taille un/deux passent. Les `13/13` CTests Jung ciblés sont verts. Il manque
toutefois le cas Helly réellement ternaire, les normales projetées nulles et le
preflight u16. Une fixture exacte nécessaire prend
`a=(0,100,100)`, `b=(100,100,100)`, puis
`z1=(5,90,100)`, `z2=(5,100,90)`, `z3=(0,110,110)` : les régions mauvaises
`Y>=75/2`, `Z>=75/2`, `Y+Z<=20` ont chaque intersection par paire non vide dans
le disque q4, mais leur triple est vide.

Le titre du commit « le primal retire encore un espoir » n'est pas reçu.
`--primal` ne remplace que les groupes de taille deux d'un greedy disjoint sur
`--voisins`; la taille trois emploie encore une banque finie. Il mesure donc
`p`, jamais la profondeur `d`. Aucun CTest ne lance ce mode et le claim « quatre
mesures identiques » est faux sur le binaire du même HEAD : à
`eight_clusters,n=600`, les fermetures passent `169 -> 170`, et à `n=1500`,
`189 -> 190`. Même sur sa cible réduite, le primal récupère déjà une base.

La fixture `u<p<d` du probe ne calcule pas `d` : elle imprime le littéral huit
après avoir cherché seulement singletons/paires et extrait greedily `p`.
Dupliquer géométriquement `g1` comme troisième gadget laisse cette porte publier
`u=6,p=7,d=8`, alors que le vrai minimum tombe à sept. Le prochain juge doit
calculer `d` ou son équivalent combinatoire, jamais le graver.

Cet équivalent est maintenant exact. Soit `E_q(P)` l'hypergraphe de rang trois
des bases Helly couvrantes. Les intérieurs de tout centre frappent chaque base,
et tout transversal des bases laisse, par Helly, un contre-centre sur les IDs
restants. Donc :

```text
Depth_q(P) = tau(E_q(P))
```

Le packing courant n'est que `nu(E)<=tau(E)`. Un branch-and-cut alterne un
solveur bitset de transversal et le juge primal : un transversal `R`,
`|R|<h`, donne soit un contre-centre sur `P minus R`, soit une nouvelle base
disjointe de `R`. Chaque base géométrique n'est vérifiée qu'une fois ; le replay
GPU devient combinatoire.

Le lift uniforme de chaque hyperarête est lui aussi résolu. Pour une base et
des poids fixes, poser
`A0=-W*(a dot b)+(a+b) dot Z-Q` et
`C0=W*(a cross b)-a cross Z-Z cross b`. q4 teste
`A0>0 && 2*A0^2>||C0||^2`, q3 remplace `2` par `3`. Le couple `(A0,C0)` est
affine séparément en `a` et `b`, et chaque lane est un cône de Lorentz convexe.
Les 64 couples de coins caractérisent donc exactement `ALL` sur l'enveloppe
`A×B`. `BlockJungDual64` ajoute au branch-and-cut seulement les bases qui
passent uniformément ; les autres provoquent un split. Sous u16 et
`1<=W<=65535`, les identités `A4=4*A0`, `R=4*||C0||^2` et les bornes
`|A0|<2^50`, `|C0_i|<2^49` placent ses comparaisons dans i128. Cette combinaison
`primal proposer -> dual64 verifier -> tau(E)>=8` est le premier certificat
constructif fail-open de `BlockJungDepth8` sans tableau de `PairId`. Son pire
cas peut encore visiter toutes les feuilles du produit.
Ces bornes supposent un widening avant les sommes, normes et produits, ainsi
qu'un preflight de `W` en accumulation large ou saturante.

Aucun artefact durable ne permet d'identifier un second auteur ou modèle : Git
attribue les deux flux documentaires au même auteur. Le présent contre-audit
redérive néanmoins le résultat. À base et poids communs fixés, `(A0,C0)` est
séparément affine et la lane est un cône de Lorentz strict convexe ; la double
interpolation des 64 coins est donc correcte sur l'enveloppe AABB continue. La
réserve `for all pair exists lambda(pair)` contre
`exists lambda for all pair` reste impérative. Les constantes 80/99 sont
cohérentes avec Montejano--Oliveros, théorème 3.1, puis la borne stricte de Tuza
`eta(3,h)<(h+1)^2`. Cette redérivation valide les énoncés, pas une indépendance
organisationnelle ni l'intégration live.

La réception logicielle de bloc reste ouverte. Les tests du header comparent
les deux formes sur des couples ponctuels puis des AABB dégénérées en points.
Le commit ajoute trois CTests d'intégration WSPD, dont deux mutants de ledger,
mais toujours aucun produit AABB non dégénéré, changement de poids par coin ou
preflight coordonnées/boîtes/IDs du wrapper. `bjd_lane_box` paie en outre un
prétest intérieur puis jusqu'à 64 coins : le pire cas annoncé est 65 prédicats,
pas 64.

Son ABI confond encore invalidité et réfutation : `!base.valide` retourne
`kLaneNone`. Le callsite q4 live reste fail-open parce qu'il teste seulement
`retour>=q4`, mais un consommateur générique de lanes pourrait publier un faux
`NONE`. La réception exige `ALL_GROUP/MIXED/INVALID_OR_UNKNOWN`. Le commentaire
de `jung_dual.hpp` inverse par ailleurs le minimax ; la bonne identité est
`min_w max_z Phi_z(w)=max_lambda min_w sum_z lambda_z Phi_z(w)`. Les formules
codées suivent la bonne identité, donc cette faute documentaire ne réfute pas la
primitive.

Un wrapper de profondeur reçu devra conserver des ensembles authentifiés de
`PointId`, pas seulement `cred/ccred`. Le packing minimal exige chaque groupe disjoint des
autres groupes et de tous les singletons/spans déjà crédités dans la même vue.
La route plus complète garde les groupes comme hyperarêtes et teste
`tau(F)>=h`, ce qui traite leurs recouvrements sans double crédit. Un cap de
juge donne `PARTIEL/UNKNOWN`; il ne transforme jamais un raccord non jugé en
accord.

Le packing live est désormais sûr sous ses invariants de probe, mais il est
NO-GO comme hot path dans son ordonnance actuelle. À `n=1500`, les lectures
restent exactement identiques avec et sans BJD (`32387961` sur `uniform`,
`9366805` sur les amas), tandis que les médianes CPU utilisateur augmentent de
`5,47 %` et `8,15 %`. Le gain de masse q4 vaut respectivement `12,55 %` et
seulement `0,87 %`. Le certificat doit donc fermer avant la descente. Le
préfiltre `BJD-BilinearBounds` redérivé dans le contre-audit est sûr : 36 extrema
bilinéaires donnent le minimum exact de `A0` et une surborne de `||C0||^2` ; un
succès implique le verdict des 64 coins, un échec retombe sur ceux-ci ou
`MIXED`.

Helly avec tolérance borne en plus le payload ponctuel. Pour tout
`R`, `|R|<=h-1`, la profondeur exige l'intersection vide des mauvais convexes
hors `R`. Il existe donc un `ToleranceKernel` de taille
`eta(3,h)<(h+1)^2` : par intégralité, au plus **80 IDs pour q4** et **99 pour
q3**, sans affirmer l'optimalité de ces constantes. Ce théorème est existentiel
et pointwise : il ne borne ni la découverte, ni le nombre de proof-tiles, et
`for all pair exists kernel` ne donne aucun noyau commun à un rectangle CK.
Sur une tuile, les mêmes bases doivent être vérifiées uniformément ou provoquer
un split. Après une face aiguë fixe, la dimension un est plus forte : les top-k
seuils donnent constructivement un noyau d'au plus 16 IDs pour q4.

## SOC64 actif au HEAD : baisse locale du résiduel, aucune campagne G4 reçue

Le faible gain du classifieur scalaire `D/V/T` ne réfute que des extrema
décorrélés. Il ne réfute ni le spindle ponctuel, ni les rectangles corrélés, ni
le disque de Jung.

Pour `e=z-a`, `t=b-z`, `H=e dot t`, `E=||e||^2`, `X=||t||^2`, q4
exige `H>0` et `3H^2>EX`. Si les 64 couples de coins de
`(C-A) times (B-C)` passent, tout le rectangle est `ALL`; le premier échec est
seulement `UNKNOWN`. La fixture axiale où toutes les différences sont
colinéaires positives ferme q4 par `SOC64` alors que la borne scalaire échoue.

Le commit `110fe76` ajoute `--soc64-actif` comme disjonctif q4 : un succès
change le fate sans double-compter ses descendants. Son message publie, à
`n=3000`, `uniform 22,842 % -> 13,602 %`,
`terrain 12,603 % -> 6,781 %` et
`eight_clusters 89,933 % -> 73,767 %`. Aucun transcript de commande ni reçu de
sortie associé dans le dépôt ne permet de transformer ces chiffres locaux en
pente ou en résultat G4.

Le branchement demeure incomplet pour la vue combinée : sous `central-NONE`,
un échec SOC au nœud courant ne descend pas vers d'éventuels descendants
SOC-`ALL`. Cela perd des fermetures mais ne crée pas de faux `ALL`. Les ledgers
baseline, SOC et futurs groupes Jung gardent des preuves d'identités et des
statuts `PENDING` séparés.

La session `soc64_actif_g4_20260814` n'a produit aucune rampe. Elle a construit
seulement `mhgp3v_wspd_wavefront_probe`, puis un regex CTest plus large a
demandé des exécutables absents ; CTest a rendu `rc=8`. Le transcript conserve
l'échec et `stop_and_verify.sh` certifie exactement
`devpod-gpu-exploration/europe-west4-ai1a/ehgp-blackwell-spot-ai1a` en
`TERMINATED`. Ce reçu qualifie l'arrêt, pas SOC64, une pente ou le SLO. Le
script inchangé jusqu'au commit `a58d020` est seulement corrigé partiellement et n'a pas de
reçu d'exécution : il omet encore `mhgp3v_jung_dual_judge`, sélectionné par son
regex CTest, et ce regex ignore les portes `mhgp3v_bjd_*`. Il avale le code de
l'analyseur, omet `--exige-fenetre-finale`, gate le seul `sum_E` à `1,70` au
lieu des compteurs physiques à `1,35`, et autorise `4*3000 s` par job sous des
coupe-circuits de `4800/5400 s`. Sa rampe `s=8,r0` reste CPU-only, sans aval
officiel et sans vue SOC complète sous `central-NONE`.

Prochaine porte : stabiliser le worktree, construire toutes les cibles réellement
demandées, rejouer localement les mutants et l'oracle d'union par `PointId`,
puis mesurer les mêmes tâches à `1500/3000/6000` avec `pending=0`, coût, HWM et
sorties. Aucune nouvelle rampe 50 000 avant cette porte.

`JungDiskDepth8` restreint le plan des centres au disque
`||y-d||^2<=D/2`. Une fixture à huit groupes disjoints ferme ce disque alors
que le LP global échoue même à profondeur un. Sous `smax=11`, le LP à 3280
appels correspond à `h=8`; il reste un oracle pairwise, jamais le hot path ni
le diagnostic d'une « vraie pénurie » sur le domaine Morse.

Preuves, largeurs, fixtures et cascade :
[`AUDIT_REPONSE_PLAN_VERTICAL_SOC64_LP_1AA487D_20260813.md`](AUDIT_REPONSE_PLAN_VERTICAL_SOC64_LP_1AA487D_20260813.md).

## Contre-audit croisé des flux documentaires

Dans ce corpus, « l'autre auditeur » est une relation locale au document et ne
désigne aucune identité globale vérifiable. Par exemple,
`AUDIT_REPONSE_PLAN_VERTICAL_SOC64_LP_1AA487D_20260813.md` section 0.1 relit
explicitement `AUDIT_REPONSE_ROUTE_VERTICAL_SLICE_1AA487D_20260813.md`, tandis
que d'autres rapports relisent PLAN ou une proposition non nommée. Lorsqu'un
lien explicite existe, il est cité ; sinon la provenance est indéterminée. Les
conclusions sont reçues ou rejetées par preuve, fixture et pin, jamais par
autorité personnelle.

`AUDIT_REPONSE_ROUTE_VERTICAL_SLICE_1AA487D_20260813.md` a correctement
découvert l'owner décidé sur
`GenerationRank` plutôt que `PointId`; Claude l'a réparé localement au commit
`f516198`. Il a aussi correctement relevé l'incomplétude de `0A` et la nécessité
des lots/verticales.

Les propositions LP/cages de
`AUDIT_REPONSE_PLAN_VERTICAL_SOC64_LP_1AA487D_20260813.md` sont recevables après les restrictions intégrées
dans la proposition consolidée : complétude LP seulement sur le pool mondial,
largeur du constructeur distincte du vérificateur `F`, traitement des bases de
rang inférieur, et recalcul de la fleur après réduction d'une cage. Les comptes
`32/36` ne valent que pour des tétra-cages : les maxima six-sites deviennent
`64/72`; `P=48` est une capacité q4, pas une existence. Pour une base positive
minimale 3D, le seuil angulaire suffisant est `delta>=4h-3`, soit `29` pour
huit cages, et n'est jamais une condition nécessaire. Son oracle de profondeur
ne prouve pas l'absence d'un support source et son arbre à 3280 LP pour `h=8`
ne doit pas être présenté comme GPU/hot path.

Sa restriction « WSSD compacte ne borne pas la sortie » est correcte. Sa
conclusion « WSSD seulement broad phase » est trop étroite : avec partition
CK, owner total, certificats `[L,U]` et blocs paresseux, `OwnedCK-WST` devient
une source factorisée exacte. Elle ne devient toujours pas une liste sparse.
De même, le théorème des faces aiguës q4 est utile uniquement avant le rang ;
la fixture 64 points interdit toute cascade depuis les événements q3 retenus.

Le présent contre-audit du probe live resserre à son tour les formulations
de `AUDIT_SOURCE_CK_WST_Q2_Q3_Q4_35FCEA8_20260814.md` : les
conditions précédentes sont un contrat, pas l'état du tape `22d1cb0`. Le juge
choisit l'owner puis ignore les autres ancres, tandis que le producteur brut ne
projette ni owner, ni distinct-ID et partage un tie-break Morton.
`AUDIT_SOURCE_CK_WST_Q2_Q3_Q4_35FCEA8_20260814.md` avait raison de demander une
broad phase et de refuser une borne de sortie ; son angle mort était la couture
entre l'owner abstrait et les records réellement comptés. Le statut live reste
`CandidateCover`.

`AUDIT_CONTRE_RECEPTION_M4_V2_DEPTHBLOCK_5BFC5C8_20260814.md` corrige également
les formulations antérieures : `2B_R` est sharp seulement à information de boule
contenante fixée ; un poids `lambda` commun rend `BlockJungDual` sûr mais
incomplet à cause du swap `for all pair exists lambda`; et un glouton arbitraire
de groupes sur l'axe n'est pas complet. La formulation est maintenant plus
simple : les extrema top-k donnent directement le noyau tolérant exact, sans
matching global. Ces corrections sont intégrées à la proposition et aux fixtures. Pour
`BlockBallDepth`, une profondeur moyenne élevée ne prouve pas encore que huit
mêmes groupes couvrent uniformément un bloc : cette efficacité reste une gate
falsifiable.

Le même contre-audit valide les signes et seuils stricts du primal Jung,
mais corrige deux nouveaux raccourcis : i256 n'est sûr qu'après la réduction
`g_i/K_ij`, et les comptes `3280/9841` appartiennent aux profondeurs `h=8/9`
sous `smax=11`. Il réfute aussi l'interprétation du diagnostic feuilles : seul
le témoin est singleton, `A/B` restent des boîtes, et `pending=0` n'implique
pas que les sous-arbres `central-NONE` ont été visités. Ces corrections sont
maintenant dans la proposition et dans l'audit M4 actif.

Le prototype historique `JungDual` code correctement la forme entière ponctuelle
`A/P/R` et ses seuils q3/q4 sous la précondition u16 et `sum w<=65535`. Son
`dual_lane` ne préflighte pas cette somme et les paramètres zéro rendent
l'ablation vacuaire. Le nouveau `BlockJungDual64::make_base` corrige le cap pour
sa propre base, sans durcir l'ancienne ABI. Le juge primal séparé reçoit désormais le collectif `k=2`, mais aucune
fixture `k=3`, aucune profondeur `tau(E)` et aucun rectangle CK uniforme. Le
raccord `--primal` reste un packing greedy et son claim de gain nul est réfuté
par les replays `169 -> 170` et `189 -> 190`. Son succès représente une paire
ponctuelle, jamais encore une proof-tile uniforme ni un chemin device.

Le contre-audit détaillé corrige aussi les anciennes confusions entre q2 et q4,
cutoff kNN, taux empirique et borne structurelle.

## Rejeux ponctuels

```text
ball_event Release ciblé : 10/10 verts
suite ciblée ball_event/WSPD/Gamma/postings/saturated : 87/87 verts
faces q4 exactes : 5/5 ; q3 côtés hors rang : 6/6
fixtures centre_cell arité 3/4 + mutant cascade : 3/3
SOC64 isolé : 16/16 ; WSPD--SOC intégré : 5/5, oracle d'union incomplet
WSPD--SOC/porteurs/two_lines/cap au HEAD : 17/17 en 33,18 s
q4_brute au HEAD : 5/5 en 2,40 s, prédicats et shell non indépendants
sous-suite SOC/WSPD/porteurs/two_lines/q4/Jung/diag/ordre : 56/56 en 25,79 s
contre-calcul BigInt ad hoc Corner8 (pas une CTest) : 4096/4096,
  marge owner=11892000,
  min bary=13217143/721310286, pire J_U=-79011820908103787995
dissection de perte live : 1/1 en 0,80 s, populations non appariées
flip direct cap 1000 : accord=OUI, juges=47, sautes=3, faux=0 (statut faux)
mutant somme cap complet : code 4, juges=168, sautes=0, faux=25
JungDual UBSan étroit : overflow signé à jung_dual.hpp:157
Jung ciblé au HEAD : 13/13 verts en 0,31 s, collectif k=2 reçu, k=3/tau(E)/OPEN_FINAL absents
BJD header au HEAD : seulement point + boîtes dégénérées ; produits non dégénérés absents
BJD intégré au pin 5809bd2 : 3/3 CTests verts, nominal et deux mutants de ledger
rejeu consolidé précommit SOC/Jung/BJD : 10/10 verts en 1,21 s sur cette machine
pin 5809bd2, BJD cap 1 : code 0/OK avec groupes sautes=98 et fermetures sautees=10 (statut faux)
pin 5809bd2, BJD sans vwave : code 0/OK, essais=0, couvrants=0 (mode vacuaire)
HEAD SOC actif + juge shadow : zéro verdict actif jugé ; raccord actif sans autorité intégrée
HEAD judge-vwave + SOC/BJD : code 1, 149 fermetures dites « sans 10 » ; juge central incompatible avec les preuves collectives q4
rejeu local historique Jung/BJD : 16/16 verts en 1,45 s, dont 13 Jung et 3 BJD intégrés
pin 694920a BJD ciblé : 8/8 verts ; 0,26 s présent rejeu, 0,36 s précédent
  PARTIEL code 3, sans-vwave code 2, mutants code 4
pin 694920a exige-q4-ouvert seul : code 0/OK ; collinear_seven points=200 exécute n=9
HEAD 8fd6f59 BJD ciblé : 8/8 verts ; exige-q4-ouvert sans juge et collinear n=200 refus code 2
HEAD collinear_seven sain : zéro groupe, zéro fermeture, 55 visites de feuilles déjà créditées rejetées
HEAD midball standalone : 9/9 affichés verts en 0,95 s présent rejeu ;
  deux portes saines à regex, doublon de rect_h_interval, réception ouverte
HEAD midball --selftest=1 : accord=OUI imprimé puis plancher code 3
HEAD fenetre-exacte n=200, S=1000 : 198000 scans ; q2 exact échantillonné, q3/q4 majorants ; aucune CTest dédiée
HEAD exhaustif n=200 : 19900 paires, U<h=3790/10059/10937, 3184359 tests
HEAD exhaustif --points=8,9 : n=9 sauté par retour après les 28 paires de n=8
HEAD exhaustif + plancher BJD impossible ou mutant : code 0, gates court-circuitées
pin a58d020 WSPD 63c79bd6 / CMake 4c6cb24e : build vert ; 13/13 affichés verts
  n=1500 amas : lectures -7,41 %, résiduel q2 -29,95 %, vague médiane +14,1 %
  multi-n : compteurs hérités ; cap i64 et bypass exhaustif du juge ouverts
révision borne-sup 90640885 : zéro fermeture q2/q3/q4, pending=0, fenêtre finale OUI ; réfuté
révision commise ec5ec3d4 : fates/masses appariés sans SOC/BJD/climb ; lectures -0,0274 %
  ledgers combinés, BJD, climb, CTest de parité et mutant de conservation ouverts
HEAD Corner8 : 6/6 ciblés ; bloc=4096 supports/32768 témoins, oracle ponctuel corrélé
  selftest=1 imprime accord=OUI puis code 3 ; drop-corner/norme hors porte ; produit-OJ absent
pin 22d1cb0 WST3 : 5/5 ciblés ; uniform n=120, 280840 triangles, zéro manquant/doublon owner
pin 22d1cb0 WST4 : 1/1 ciblé ; uniform n=60, 487635 quadruplets, zéro manquant/doublon owner
  copies non-owner, diagonales, vrai PointId, doublons de position et coût non jugés
HEAD 069d903 : huit portes WST3/WST4 enregistrées, dont signe aigu et échelle ; 8/8 ciblées vertes
worktree bisigne : 12/12 portes préexistantes WST3/WST4/Corner8 vertes en 11,76 s ; aucune n'exerce la jonction
catalogue HEAD : 835 CTests après reconfiguration ; aucune suite complète 835/835 rejouée ici
ablation BJD n=1500 uniform : masse q4 -12,55 %, CPU user médian +5,47 %, lectures identiques
ablation BJD n=1500 amas : masse q4 -0,87 %, CPU user médian +8,15 %, lectures identiques
session G4 SOC actif : CTest rc=8, aucune rampe, cible TERMINATED
ablation primal amas n=600 : 169 -> 170 fermetures ; n=1500 : 189 -> 190
--rang=10 sans --porteurs : code 0, aucune ligne rang
two_lines n=40 avec rang : code 0, bien_centres_juges=0
suite complète : interrompue après 28/734 terminés, tous verts
grid n=16 : refus_domaine=99 puis fold=OUI
clusters n=5, coord=4 : timeout après 2 s, capacité non preflightée
UBSan mutant clé : overflow signé à ball_event.hpp:290
```

Ces rejeux falsifient des claims précis ; ils ne qualifient ni exactitude
publique, ni performance.

## Ordre bloquant

```text
réparer 0A u16 et isoler les juges de mutants
  -> raccorder BallEvent aux autorités Gamma par ordres/lots/verticales
  -> recevoir 0B et le payload borné
  -> recevoir CKPairTape q2 et ses certificats [L,U]
  -> Midball ALL min-only, domaine/IDs préflightés, avant toute descente q2
  -> triage canonique U<=D<=C par paire/microtile
  -> SOC64 union-disjointe + JungDiskDepth9/8 seulement sur U<h<=C
  -> primal proposer -> BlockJungDual64 uniforme -> branch-and-cut tau(E)>=h
  -> CandidateWST3 coarse dans 2B_R-lentille dès q3_open || q4_open
  -> normaliser vrais PointId, distinct-ID, owner et ALL/NONE_ACUTE par bloc
  -> profondeur carrier avant de former paresseusement le résiduel WST4
  -> positivité q4 puis Corner8BallDepth/BlockBallDepth8 avant tout fill
  -> raffinement porteur de preuves sur les tâches encore MIXED
  -> mesurer F2/F3/C4_carrier/F4/M4_apex/W4/H4/T4, BallKeys, census, H
  -> seulement alors portage device et campagne G4 50k
```

Cet ordre bloque toute réception produit, pas la falsification architecturale.
En parallèle, une piste `counter-only` peut recevoir sur petit `n`, contre
vérité exhaustive, `CKPairTape -> carrier aigu -> BlockJungDual64/tau(F) ->
AxisKernel/BlockBallDepth`. Elle ne promeut ni 0A, ni 0B, ni le statut public ;
elle permet de mesurer les fermetures **avant** descente, les splits, `F4/M4`,
les nœuds de transversal, les octets et la HWM avant d'investir dans un portage
G4.

La proposition consolidée est
[`../PROPOSITION.md`](../PROPOSITION.md). L'index court des audits est
[`README.md`](README.md).

GCP non utilisé par le présent auditeur ; la session échouée de Claude est
certifiée `TERMINATED`.
