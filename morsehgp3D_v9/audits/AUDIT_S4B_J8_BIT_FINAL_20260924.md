# S4b exploratoire : bit J8, seaux et coût du raffinement

**Addendum mutable du 24 septembre, 00:53 UTC.** Le constructeur a élargi
`dn`, `dp`, `a8`, `b8` à `uint16_t` dans le scratch : nouveau source SHA-256
`0c2d6d6e4d77df6d77efa211ead419e111e26b46353f546e777dd2dba279b852`,
binaire voisin SHA-256
`98377c871994e92c79b91e035614046556ba9f286c637b2f1e07a9bc812aa9a1`.
Les deux sondes corrigées sur 08/000000 sans sol, une arête q4 ouverte
sur vingt, donnent le multiensemble exact attendu **sur les arêtes
sondées** :

| K | SHA-256 du stdout scratch | `design_compare` | `d_cert` / `d_surv` | Couples graine–site préparés |
| ---: | --- | --- | ---: | ---: |
| 5 | `75b1540a841b38aaa54199bf1cc438ea9abd6ea3c005388ac20eb4ece6d22dea` | 7 771/7 771 égaux | 318 386 / 43 393 | 973 665 353 |
| 10 | `2a03a3f1132faf004fdf7106b7b512e4a492152b71ec2a9c47b0b86df7f9be6e` | 85 491/85 491 égaux | 1 398 173 / 222 836 | 5 479 669 161 |

Le `compare: ... mine=0 equal=0` demeure attendu, car `S4B_FULL` est
absent. Ces observations **clôturent le défaut de bit dans le source
scratch courant** et donnent un contrôle d'objet local positif ; aucun
reçu ne lie formellement compilation, source, binaire, entrée et commande.
Le source corrigé ne porte pas encore l'assertion de capacité
`J+1 <= digits(Word)` proposée ci-dessous.
Le pire cas de seau ci-dessous concerne **aussi** ce source corrigé. Ses
982 451/5 314 427 pas J8 diagnostiques ne sont pas du travail réellement
épargné : tous les couples graine–site du tableau sont préparés avant
la simulation de l'arrêt. Aucun port produit ni gain de chaîne n'en découle.

24 septembre 2026. **Préflight d'un prototype scratch, pas défaut du moteur
v9 publié.** Un exécutable `s4b_design8` était en cours sur 08/000000
sans sol/K5. Le source voisin dans le scratchpad `s4b/ep/` du constructeur
porte le SHA-256 :
`09e73353a6c69357fcd0c50b01d06ad0e6cfebc6d66e34fbc9879592461e3529`.
L'exécutable porte le SHA-256
`61b0659556b225deadfd58ea5aa78babd93f6f5ac8ffddd79076c1c7672a78ec` ;
aucun reçu ne lie encore formellement ces deux fichiers. Le source n'est
pas sur `main` à cette lecture. Ne pas promouvoir son chrono ou son
égalité d'objet éventuelle en preuve de S4b.

Dans le chemin **DESIGN** (`:470–505`), `J=8` construit neuf bornes
`g[0]..g[8]`, mais `dn`, `dp`, `a8` et `b8` sont des `uint8_t`. Le masque
`1U << 8` est tronqué lors de l'affectation à huit bits. Ainsi, pour
tout site, `((dn[t] >> 8) & 1U) == 0` et de même pour `dp` ; la dernière
lentille `lens[7]` reste nécessairement **zéro**. Comme `T=K−2>0` à
K5/K10, `all(lens[j] >= T)` ne peut jamais être vrai : aucun certificat
DESIGN J8 ne peut fermer une graine. Le dernier seau traite en outre
comme événements des sites qui auraient eu un signe stable à sa borne
droite ; l'attribution des racines et les profondeurs candidates ne
sont plus celles de la grille voulue. Cela peut changer travail **et**
objet, pas seulement une statistique.

Le dernier seau (`:499–537`) contient alors tous les sites du cover avec
`B≠0` dans `evj` et peut comparer chaque candidat à tout ce seau : le prototype
retombe potentiellement sur un balayage quadratique par graine. Une longue
durée de ce processus ne mesurerait donc pas le design J8 corrigé.

Le diagnostic préliminaire `J=8` calculé plus haut dans ce même fichier
emploie des masques `uint64_t` sur la grille `JMAX=32` : ses comptes ne
sont **pas** invalidés par ce défaut local au chemin DESIGN. Sa grille
`-mubar + floor(2*mubar*i/32)` peut toutefois différer d'une unité,
du côté négatif, de la grille symétrique du DESIGN (`:327`, `:472`). Ses
comptes J8 ne sont donc pas exactement ceux du design, même une fois le
masque corrigé. De plus, le prototype prépare `P`, `B` et 33 signes
pour **tous** les couples graine–site avant de simuler l'arrêt anticipé
(`:323–343`), après avoir déjà payé le pipeline produit et le cœur S3 :
ses pas de warp J8 sont un modèle, pas une économie de travail réalisée.

Le processus observé n'a pas `S4B_FULL` dans son environnement. Sans cette
variable, le flux de référence `mine` n'est pas construit
(`:564–628`) alors que le code de sortie compare `mine` au produit
(`:685`, `:761`). Un code 1 dans cette configuration ne juge donc pas le
DESIGN ; seul `design_compare` compare ses sorties au produit, et il faut
conserver cet oracle au rejeu.

Correction minimale : employer au moins `uint16_t` pour les quatre
stockages de signes DESIGN, et verrouiller `J+1 <= digits(Word)` par
assertion de compilation. Une fixture à signe strictement positif et
une à signe strictement négatif en `g[8]` doivent exercer le bit 8 ;
exiger `d_cert>0` sur la sonde qui certifie avec J8, puis comparer le
multiensemble q4 complet (support, clé, profondeur, tous les IDs de
coquille) au produit, y compris les racines sur bornes et les ex æquo.
Mesurer ensuite les coûts réels de préparation et des seaux survivants,
avec le même échantillon d'arêtes avant/après correction. Aucun résultat
GPU/G4 ou borne sous-quadratique ne découle de cette porte scratch.
Ce chemin consomme les survivantes **après** le certificat S3 : même
correct, il peut réduire le q4 aval et l'atlas, mais il ne supprime pas les
formes du cœur déjà calculées. La pente défavorable de ce cœur dans les
quarts LiDAR exige donc une voie amont distincte et une mesure intégrée.

## Après correction du bit : un seau vivant peut rester quadratique

Le masque élargi rétablit l'exactitude du découpage, mais ne borne pas le
coût du DESIGN. Dans `:499–537`, chaque candidat d'un seau vivant compare
sa racine à **tous** ses événements avant le test `depth>=T` ; `decided`
écarte seulement les racines **égales**. Il y a donc `cands×events`
comparaisons possibles pour une seule graine, indépendamment du nombre
de groupes finalement émis.

Une famille entière dans le domaine u18 le réalise. Pour
`1≤L≤13107`, prendre `a=(0,0,0)`, `b=(20L,0,0)`,
`x=(10L,12L,0)` et les `m=floor(L/2)+1` points
`y_z=(10L,0,z)`, `11L≤z≤floor(23L/2)`, avec les IDs de `y_z`
supérieurs à celui de `x`. `ab` est l'arête propriétaire et le triangle
`abx` est strictement aigu. Le cover **complet** de `Q34EdgeCover::make`
a pour rayon `20L` autour du milieu de `ab` : il contient `x` et tous
les `y_z`. Le calcul entier du seed donne

`P(y_z)=57600L⁴(z²−100L²)>0`, `B(y_z)=240L²z>0`,
`µ_z=P/B=240L²(z−100L²/z)`.

Ces racines sont distinctes et croissent de `(5040/11)L³` à
`(15480/23)L³`. La borne de la sonde donne
`mubar=ceil(sqrt(21491200L⁶/2))` et le seau J8 `j=4` est
`[g4,g5]=[0,floor(mubar/4)]`, avec `g5≥819L³` : **toutes** les
racines y sont strictement intérieures, tandis que `lens[4]=0<T`.
Chaque `y_z` reste donc candidat et compare `m` événements : exactement
`m²` appels dans ce seau. À `L=10`, les six profondeurs sont
`0,1,2,3,4,5` : K5 n'en garde que trois, après **36** comparaisons.
À la borne u18 `L=13107`, le même raisonnement permet `m=6554` et
**42 954 916** comparaisons pour un seul seed.

Ce sont de vrais supports q4 positifs : le centre de `abxy_z` vaut
`(10L,11L/6,(z²−100L²)/(2z))`, avec poids barycentriques
`λx=11/72`, `λy=(z²−100L²)/(2z²)` et
`λa=λb=(1−λx−λy)/2`, tous strictement positifs. Le premier `y_z`
donne une boule de profondeur zéro dans ce nuage ; un certificat q4
globalement sûr ne peut donc éliminer `ab` avant la famille si cette
arête est présentée. Cette construction prouve un pire cas **local** du
chemin DESIGN, pas une pente LiDAR observée ni un pire cas déjà mesuré de
toute la chaîne. La [fixture exacte
rejouable](s4b_j8_bucket_fixture_20260924.py) contrôle, en Python
normal et sous `-O`, `L=8,10,16,64,256,13107` : racines, cover,
propriétaire, positivité et `lens[4]=0`. Les comparaisons qui découlent
de la boucle sont respectivement `5²,6²,9²,33²,129²,6554²` ; trois
groupes restent peu profonds à K5 dès que `m≥3`.

**Observation produit locale non scellée, distincte du scratch J8.** À `L=10`,
la fixture énumère désormais les **126 tétraèdres** du nuage de neuf
sites en arithmétique rationnelle : les seuls supports q4 strictement
positifs sont `abxy_110` à `abxy_115`, de profondeurs `0..5` ; il y en a
trois admissibles à K5 et six à K10. Le binaire produit CPU v20 du
[reçu S4a local](s4a_ground_hot_quarter_20260923/README.md), SHA-256
`eea3040cf4150599ca7ab5c71a4f0e3738f2975483584527f339d3d2600d08e9`,
a été rejoué sur les neuf triples u32le dans l'ordre écrit plus haut
(SHA-256 de l'entrée
`040c44bf4e4a542081aa4ac4ce7830c308788f4426d0873287eecb033c6d7ef0`).
À `--s=8 --static=8 --grid=1mm --catalogue-digest`, W1, le second bras
ajoute `q34_batch_filter=1`, `q34_batch_certificates=1` et garde les
deux leviers GPU à zéro. Les bras moteur et S2/S3 CPU par lots rendent
chacun `complete_relative`, **3/6**
présentations q4 à K5/K10 et les mêmes condensés de catalogue et de tour
entre bras : `0bd6d9370c47804e` / `ab4d011dd514fd81` à K5,
`2e5756b94e14ec8b` / `84f479f2dda58041` à K10. Les stdout temporaires
des quatre rejeux portent les SHA-256, dans l'ordre K5 moteur/lot puis
K10 moteur/lot : `7351014ebc011788acaf5e9263373bda3f415489c8a028cce596a8064fc0343c`,
`1ac17cd96cd0cbbeafa3127d168b5e445a2399ae83b922e39509667d0e84c8b2`,
`b0c78ce1561c51c81d47fba22cbcbd91a544e6d30fc460a8b743781a69cc7998`,
`64191007e93ea0f1f98824c82a1a048d53c4e74312bb267cd41388a3f88bac56`.
Le produit ne publie pas ici les supports individuels : l'accord de
compte avec l'oracle n'en est pas un juge nominatif. Il établit que le
petit adversaire parcourt effectivement la voie q4 du produit ; **aucun
DESIGN J8 n'est porté ni chronométré** par ce rejeu.

Pour borner ce poste, basculer les gros seaux vers un **tri exact des
racines puis un balayage des entrées/sorties par groupe**, comme la
primitive familiale existante, avec `lens[j]` comme profondeur initiale.
Le seuil doit tenir compte de `cands×events` et du coût du tri ; les
événements à la borne et toutes les coquilles restent dans la comparaison
multiensemble avec le produit. Publier maxima et sommes par arête de
`cands×events`, comparaisons réellement exécutées, mémoire et temps de
chaîne sur les demi-scènes, quarts et densités LiDAR avant toute
projection sous-quadratique. Même un coût `O(events log events)` **par
seed** ne borne pas la somme sur toutes les graines.

## Raffinement octaire scratch : un carré non compté et des rescans entiers

Le scratch CPU `s4b/syn/syn_probe.cpp` du 24 septembre, source SHA-256
`e182f165c657e9f16162b21ac1853ed6366c9d53a3be8769148e3857560f0dee`,
est **distinct** du DESIGN J8 ci-dessus et de tout port produit. Sur la
trame **sans sol entière 08/000000**, les sorties du raffinement `REFINE=1`,
`J=8`, `R=4`, `stride=1` égalent le multiensemble q4 du produit dans ce
rejeu local : 158 496/158 496 à K5 et 1 732 548/1 732 548 à K10,
avec zéro faute déclarée. Les stdout scratch K5/K10 ont respectivement
les SHA-256 `b780c49a33612162e56a9ed31af0288d8d83d6281e117f861640a5535cb2cb85`
et `661e5bd448077517af3ebcbccdcaaa236336a6cdb70ca14787ccff6a31403128`.
`compare_steps` baisse de 636 128 à 423 540 (K5), et de 8 488 182 à
4 529 432 (K10) par rapport au même scratch sans raffinement. Ces
compteurs ne mesurent pas tout le travail du raffinement.

Dans `resolve`, une fois le groupe de racines égales `grp` construit,
chaque candidat encore actif appelle `std::find(grp.begin(), grp.end(), c2)`.
Si les `m` événements sont tous candidats d'un même groupe, il y a
exactement `1+⋯+m=m(m+1)/2` comparaisons de positions, indépendamment
de leur ordre. Elles sont absentes de `compare_steps` et `grp_steps2` ;
le raffinement ne sépare pas des racines **exactement égales**. La
[fixture entière autonome](s4b_refined_group_fixture_20260924.py) énumère
les sites de trois nuages u18 translatés : `m=60,282,1439` donnent
respectivement **1 830, 39 903, 1 036 080** recherches d'indices, face
à **2, 9, 45** pas de comparaison déclarés pour le groupe. Elle passe
en Python normal et `-O`.

Construction à l'échelle entière `L` : `a=(-24L,0,7L)`, `b=(24L,0,7L)`,
`x=(0,15L,-20L)` ; les `y` sont les points entiers sur la sphère centrée
en zéro de rayon `25L`, satisfaisant
`max(|y−a|²,|y−b|²,|y−x|²)≤|ab|²`, `B(y)≠0` et `P(y)>0`.
La translation commune `(25L,25L,25L)` met tous les sites dans u18
pour `L=1,10,100`. Chaque `abxy` appartient au cover complet et a `ab`
pour plus longue arête (IDs `a,b,x,y` dans cet ordre pour départager les
ex æquo) ; la graine `abx` est aiguë. Les événements ont
la même racine exacte `P/B=−10080L³`, strictement dans le seau vivant
J8 `j=3`. Les lentilles valent `[m,m,m,0,0,0,0,0]` : ni K5 ni K10 ne
ferme cette graine avant le groupe. Le site `(0,−15L,−20L)` donne une
boule q4 strictement positive centrée en zéro, avec poids
`(10/27,10/27,7/54,7/54)` ; tous les `y` sont sur sa coquille.
L'ancien code FULL refuse les coquilles au-dessus de 12 sites ; cette
fixture démontre le coût **du générateur q4/scratch**, pas une sortie FULL
valide ni une croissance LiDAR.

Correction linéaire locale : marquer chaque `cand` par sa position dans
`ev`, puis balayer une fois `grp` pour former `gc` avec les seules
positions encore actives ; les marquer consommées. Le choix final du
plus petit ID valide et le tri des IDs de coquille restent inchangés.
Comparer ensuite à nouveau le multiensemble complet, y compris les
coquilles et cas de racines sur borne. Une permutation de `gc` ne doit
pas être interprétée comme une modification de l'objet émis.

Un autre coût manque à la somme des pas déclarés : chaque graine non
certifiée balaie **tout son cover** pour calculer `Lsize`, puis le
rebalaie **une fois par seau vivant** pour bâtir `ev`. Le scratch compte
873 097 survivantes/3 135 073 chunks à K5 et
4 490 527/27 565 075 à K10. Comme `used=ceil(n/32)`, la somme des
tailles de covers des survivantes vaut au moins
`32·surv_chunks−31·surv`, soit 73 256 329 et 742 876 063 sites.
`Lsize` plus **un seul** seau vivant imposent donc déjà au moins
**146 512 658 / 1 485 752 126** visites de sites ; les 3 668 184 /
20 406 103 seaux effectivement vivants contiennent respectivement
2 795 087 / 15 915 576 rescans **en plus** du premier seau par graine.
En incluant aussi le premier passage de signes, le scratch exécute donc
au moins **222 564 074 / 2 244 543 765** itérations de sites pour
ces survivantes seules. Ce plancher ne concerne pas un algorithme S4b
futur et ne compte pas les graines certifiées ni l'amont.
Le `filt_steps=ceil(buffered/32)` ne les représente pas. Un ledger utile
séparerait `full_rescan_sites`, `live_bucket_scan_sites`, comparaisons de
tri, tests d'appartenance `grp` et lectures/écritures des événements,
avec maxima par arête et par graine. La comparaison doit porter sur le
**mur de chaîne S2–S4b/FULL**, et sur les demi-scènes, quarts et trois
densités emboîtées déjà définies ; ni `probe_s` CPU ni les pas simulés
ne prédisent seuls le GPU G4 ou une pente sous-quadratique.

## Premier port HostGroup WIP : un bit valide d'un groupe contamine le suivant

Le plan local `build/s4b-prototypes/PLAN_S4B_SYNTHESIS.md` du
24 septembre, SHA-256
`b00c14d2fcde5897200d691a1b363c59c7424b45ebf63df8d824bd79554c772d`,
prévoit déjà un tampon d'événements et des masques de groupe : s'ils sont
portés fidèlement, ils peuvent supprimer les deux coûts cachés **du
simulateur** signalés ci-dessus. Les projections de 18,93 M/119,51 M pas
et de 27–69/120–320 ms GPU restent des modèles tant que la voie portée
et ses compteurs physiques ne sont pas reçus.

Une première version **non publiée**, `build/v9-open-worktree/morsehgp3D_v9/src/gpu/q4_lanes.hpp`
(SHA-256 `51bee1f570047adc5f49af0d565001e6b025bbddc0e90285fdd9b980fd833f2b`),
utilise ces masques mais présente un défaut d'objet différent. Elle
ne porte encore ni raffinement octaire, ni découpage en tâches, ni
fusion q3/q4, ni noyau de clés séparé : aucun chrono de cette version
ne qualifierait le schéma S4b.3 du plan. Le port efface `q4_valid_bit`
au **début du seau** (`:374`), puis le pose pour
les candidats positifs d'un groupe (`:449–463`). Les réductions de
`valid_id` et `chosen` (`:465–486`) parcourent tout le seau en ne
vérifiant que ce bit. Au groupe suivant du **même seau**, l'ID valide
du groupe précédent reste donc éligible. Les comptes `groups=emitted=2`
peuvent rester corrects alors que support et clé sont erronés.

Le [gate HostGroup direct](s4b_valid_group_gate_20260924.cpp) fournit
`a=(0,0,0)`, `b=(200,0,0)`, `x=(100,120,0)`,
`y₃=(100,0,110)`, `y₄=(100,0,111)`, K5, IDs dans cet ordre.
Les deux racines q4 positives et possédées sont **distinctes mais dans
le même seau J8** (`5040000/11` et `18568000/37`, entre 0 et 819512) ;
leurs profondeurs sont 0 et 1. Compilé avec
`g++ -std=c++20 -O1 -Wall -Wextra -Wpedantic -Werror`, en incluant
les headers WIP du développeur sans les modifier, le gate rend code 1
(même résultat en `-O2`) :

```sh
g++ -std=c++20 -O1 -Wall -Wextra -Wpedantic -Werror \
  -I build/v9-open-worktree/morsehgp3D_v9/src/gpu \
  -I build/v9-open-worktree/morsehgp3D_v9/src \
  morsehgp3D_v9/audits/s4b_valid_group_gate_20260924.cpp \
  -o /tmp/mhgp9_s4b_valid_group_gate_20260924
/tmp/mhgp9_s4b_valid_group_gate_20260924
```

```text
status=0 groups=2 emitted=2 records=2
record0 support=0,1,2,3 depth=0 shell=4
record1 support=0,1,2,3 depth=1 shell=4
```

Le second support attendu est `0,1,2,4`, avec une autre clé. Une
correction sans balayage supplémentaire consiste à effacer
`q4_valid_bit` de **chaque événement** dans le passage de comparaison
du groupe courant (`:416–426`), avant le test de positivité ; conserver
`q4_decided_bit`. Rejouer le gate puis l'égalité nominative multiensemble
sur deux groupes du même seau et les fixtures cosphériques. Il s'agit
du **port WIP**, pas d'un défaut observé du produit publié ou de R15.

Le port WIP a bien remplacé les rescans **du cover** par un tampon. Son
ledger n'inclut toutefois pas encore tous ses parcours réels : chaque
seau vivant rebalaye le tampon entier pour reconstruire `q4.list`
(`:350–365`), tandis que `filter_steps` ne compte que le premier
filtrage et `bucket_events` les seules admissions. Chaque groupe fait
ensuite deux réductions min sur toute la liste, une comparaison, et,
s'il est peu profond, un passage positivité/réduction puis un choix
final sur la liste (`:394–486`) ; `compare_steps` n'en compte qu'un.
Ajouter des compteurs `bucket_scan_steps`, `minimum_scan_steps` et
`presentation_scan_steps`, leurs maxima par tâche, et les confrontations
host/device avant d'utiliser 18,93 M/119,51 M pas scratch pour une
projection de temps ou d'équilibrage GPU. Cela n'invalide pas la voie à
tampons : le coût de ces listes peut être réduit et mesuré sans changer
la preuve géométrique.

Le découpage annoncé en tâches `(arête, ≤32 graines)` a un raccord de
sortie à préciser avant le port GPU. Le contrat actuel `Q34LanesBatch`
donne **une seule plage contiguë** `record_begin[j], record_count[j]`
par arête (`src/gen/pipeline/wspd_q34.hpp:431–435`) ; le contrôle et le
juge parcourent cette plage (`wspd_q34.cpp:1268–1298,1377–1383`). Si
plusieurs warps réservent atomiquement des records de tâches distinctes,
leurs plages peuvent s'entrelacer avec celles d'autres arêtes. Il faut
soit un répertoire de plages par tâche puis un compactage déterministe
par arête, soit un premier passage de comptes suivi d'une réservation
unique par arête. Ne publier une arête que lorsque **toutes** ses tâches
sont décidées et écarter toutes ses plages au repli ; inclure ce
compactage, son stockage et la synchronisation dans le coût K-B/clé.
Le tri ultérieur des présentations ne répare pas une plage attribuée à
la mauvaise arête. Le port WIP actuel `edge_lanes` traite encore l'arête
entière et n'exerce donc pas ce problème de tâches multiples.
