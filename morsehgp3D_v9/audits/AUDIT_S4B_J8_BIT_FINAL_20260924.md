# S4b exploratoire : le neuvième signe de la grille J8 est perdu

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
