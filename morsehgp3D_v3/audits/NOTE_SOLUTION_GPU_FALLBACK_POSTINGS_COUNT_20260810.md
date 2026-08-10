# Solution GPU exacte pour le `query_mask` hybride par comptage de postings

Date : 10 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, `backend=gpu_product_candidate`,
`profile=quantized_u16_input_only`, `mode=mathematical_and_architectural_proposal`,
`public_status=not_claimed`.

## Décision proposée

Le premier kernel GPU candidat du chemin produit ne doit pas porter l'oracle
exhaustif `face-owner`. Il peut accélérer le `query_mask` du fold hybride par un
comptage des recouvrements dans les postings. Cette forme :

- n'énumère aucune `k`-face;
- ne matérialise aucune mosaïque ni graphe de Johnson;
- rend un vrai `GeneratorId` incident, jamais un représentant DSU;
- laisse à l'hôte le DSU, le pruning dynamique, les témoins et le commit
  atomique du lot;
- reste exacte relativement au catalogue même lorsque la source n'est pas
  complète; seul le statut public du transcript reste alors partiel.

Interroger tous les nouveaux générateurs est une excellente porte de
correction bornée, mais pas une architecture d'échelle. Le produit construit un
`query_mask` reçu et calcule sa masse exacte avant admission. Il ne se réduit au
seul fallback qu'après réception d'un forest batch d'arêtes réelles couvrant
collectivement tous les fast omis et certifiant le sous-graphe induit par les
anciens et ces fast; les connexions aux requêtes sont produites par celles-ci. Le bit
`principal_support` live ne suffit pas dans un lot ex
æquo : tant qu'aucun certificat intra-lot n'existe, tout lot ayant au moins deux
générateurs de rang au moins `k` doit les interroger tous. Les fast restent dans
la CSR candidate dans tous les cas.

## Identité exacte

Fixer un ordre `k`. Le posting visible `P_x` contient tous les générateurs
actifs de rang au moins `k` qui contiennent le point `x`, y compris tout le lot
courant mais aucun lot futur. Pour un nouveau générateur `M`, émettre une
occurrence `(M,N)` pour chaque `x` dans `M` et chaque `N` dans `P_x`.

La multiplicité du run `(M,N)` est exactement `|M intersection N|`. Garder les
runs de longueur au moins `k` produit donc exactement les arêtes du graphe de
seuil. Cette preuve ne dépend ni de la géométrie, ni de `q_min`, ni de la
complétude de la source.

Pour la porte tout-requête, il suffit d'interroger les générateurs de rang au
moins `k` du lot courant : une arête ancien--ancien a déjà été rejouée, et toute
arête qui devient disponible au lot possède au moins une extrémité courante.
Les requêtes du lot rendent donc toutes les arêtes ancien--nouveau et
nouveau--nouveau; les doublons du second type sont dédupliqués ou absorbés par
le DSU.

Dans le mode count dirigé, le self-run `(M,M)` est conservé jusqu'au reçu : pour
chaque requête logique de rang au moins `k`, il existe une fois et sa longueur
vaut `rank(M)`. En mode slab, il appartient uniquement à l'intervalle contenant
`ActivationId(M)`. Les modes canoniques effectuent le même contrôle directement
sur la CSR sans écrire la clé self. Cela certifie la requête du `query_mask`,
pas les candidats fast qui ne sont pas interrogés; masse et digest de la CSR
certifient séparément que ceux-ci restent visibles.

Un falsificateur combinatoire indépendant a rejoué 20 000 familles aléatoires,
lots ex æquo inclus : 951 824 comparaisons de paires et 395 938 slabs candidats,
zéro écart entre le seuil des runs, l'intersection directe et l'union des
slabs. C'est un diagnostic positif à transformer en porte permanente; la preuve
de multiplicité ci-dessus reste l'autorité.

## Préflight quantitatif : tout interroger est NO-GO

Pour une requête `M`, sa masse exacte est la somme des longueurs visibles des
postings de ses membres. Poser `L_k` égal à la somme des rangs, `W_diff,k` égal
à la masse d'intersection des paires de lots distincts et `W_same,k` à celle
des paires d'un même lot. Le chemin tout-requête émet exactement
`H_k=L_k+W_diff,k+2*W_same,k=L_k+P_post,k+W_same,k` hits : une paire du même
lot est vue dans les deux directions. Pour un `query_mask` quelconque, le
préflight somme directement les longueurs visibles pour ses seules requêtes;
un couple d'un même lot contribue zéro, une ou deux fois selon le masque. Cette
masse `H_count_directed` doit être calculée exactement, jamais estimée par une fraction
globale du nombre de fallback. Un harnais indépendant sur les trois
catalogues de la session G4 donne :

Le mode canonique dépendant du masque a une autre masse exacte
`H_count_canonical`, self séparé :
la somme des poids query--ancien, plus query--fast omis, plus une seule fois
chaque paire non ordonnée query--query. Il émet zéro nonquery--nonquery. Ce total ne se
déduit ni d'une fraction de `P_post`, ni de la formule dirigée ci-dessus; le
préflight publie donc le mode et ses quatre catégories séparées.

| n | hits `count_directed` tout-requête, self inclus | incidences `face-owner` | rapport |
| ---: | ---: | ---: | ---: |
| 64 | 226 854 316 | 3 030 554 | 74,9 |
| 200 | 1 892 861 494 | 17 282 892 | 109,5 |
| 400 | 5 747 389 371 | 44 258 951 | 129,9 |

Ces hits sont exacts pour `smax=11`, `K=5`, graine `20260810` et coordonnées
`40/58/73`. La seconde passe, corrigée pour compter deux fois les paires d'un
même lot, utilisait `order_k_flats.hpp` d'empreinte `b3ba750d938e`, une source
temporaire d'empreinte `3ccf1170b422` et un binaire d'empreinte
`fa9164255ee9`; ce diagnostic hors dépôt n'est pas une porte permanente.
Le chemin tout-requête réintroduit donc le mur de postings, aggravé par les
ordres multiples. Il reste un oracle GPU borné. Le candidat produit doit
publier `H_query(mode)` avant lancement et refuser, rester CPU demand-driven ou
choisir une autre forme si cette masse ne tient pas dans le contrat.

## Réduction exacte avant comptage : cover-filter

Même sur une requête admise, parcourir tous les membres de `M` est inutile.
Poser `r=rank(M)` et choisir `t` entre 1 et `k`. Choisir un sous-ensemble `A` de
`M` de taille `r-k+t`, puis interroger seulement les postings des
`t`-sous-ensembles de `A`. Si `|M intersection N|>=k`, alors
`|A intersection N|>=t`; le candidat `N` apparaît donc dans au moins un de ces
postings. La réciproque n'est pas supposée : après tri/unique des candidats,
le device recalcule directement `|M intersection N|` et garde seulement le
seuil exact.

Après staging du lot entier, le flux cover peut se canoniser sans perdre
l'atomicité. Pour une requête `M`, garder le candidat visible `N` si et seulement
si `N` n'appartient pas au `query_mask` courant **ou** si
`ActivationId(N)<ActivationId(M)`. Deux requêtes sont ainsi possédées une fois
par la plus tardive; une arête requête--fast omis est toujours possédée par la
requête, quel que soit leur ordre; seules les arêtes sans extrémité requête
restent à la famille certifiée du sidecar. Cela inclut ancien--fast omis et
fast--fast; ancien--ancien est déjà acquis. Le self échoue aux deux clauses et
se contrôle séparément. Un simple filtre `N<M` est faux avec un masque partiel :
il perd l'arête lorsque la requête est l'extrémité précoce et le fast omis
l'extrémité tardive.

Un falsificateur masqué indépendant confirme cette frontière : sur 20 000
familles, le filtre brut `N<M` perd au moins une arête dans 12 750 cas, tandis
que le prédicat dépendant du masque n'a aucun écart. Une porte exhaustive
supplémentaire a validé 186 972 configurations de masques et 14 383 choix
admissibles de `(A,t)`. Ces diagnostics doivent devenir des fixtures
permanentes; le lemme de couverture reste l'autorité. Un rejeu indépendant de
30 000 familles avec spanning forest non-requête retrouve zéro écart du filtre
exact et 4 303 pertes de partition pour `N<M`.

### Certificat des fast omis

Le filtre générique émet volontairement zéro arête `R--R`. Après staging et
attaches fast, poser `R=anciens_visibles union fast_omis`. Le sidecar doit donc
certifier, avec un sous-DSU ou forest constitué uniquement d'arêtes réelles dont
les deux extrémités sont dans `R`, munies de témoins de `k` points, que sa
partition égale celle du graphe de seuil induit par `R`. Une restriction de
labels d'un DSU ayant déjà utilisé des chemins via une requête ne suffit pas.
Cette condition est globale au lot : vérifier seulement les signatures
rencontrées par une requête laisserait échapper une composante tout-fast.

Le certificat peut être dérivé sans énumérer ce graphe sous un contrat hostile
précis : chaque handle porte une boule `MEB(U)` exacte et son saturé fermé
complet lié au nuage; le catalogue contient exactement un handle par boule
exacte, même avec plusieurs supports; la `BallKey` résout ses collisions; la
complétude par ordre garantit l'existence de `Sat(F)` pour toute
`k`-signature visible; niveaux et scratch ancien sont reçus exactement; chaque
fast omis atteint par de vraies arêtes toutes ses composantes strictes
incidentes. La provenance `q_min` est requise si cette dernière exhaustivité est
dérivée des modes fast.

Deux handles distincts d'un même niveau qui partagent une `k`-signature ont
alors un carrier strict ancien : l'égalité de rayon ferait des deux boules la
même miniboule, puis la canonicalité one-handle-per-ball les identifierait. Le
cas `rank=k` n'a même aucun voisin visible distinct. Le seul bit
`principal_support` ne porte pas ces obligations; le live retourne encore le
premier match de boule au lieu de refuser un doublon. En leur absence, élargir
`query_mask` à tout le lot actif ou refuser l'omission. La preuve complète est
reprise dans la note owner liée ci-dessous.

Le premier produit peut prendre `t=1` et les `r-k+1` points de plus petit
posting visible, ordonnés par `(degré_visible,PointId)`. Il réutilise la CSR de
points, réduit exactement la masse et
remplace le RLE de multiplicité par un unique puis une intersection courte.
Pour `t>1`, choisir `A` qui minimise la somme exacte des degrés de ses
signatures, puis le plus petit `A` lexicographique en `PointId` en cas d'égalité,
est encore exact, mais exige une CSR supplémentaire de paires ou de triples :
ne la construire que si son propre préflight gagne réellement. Le témoin `F`
reste lui aussi trié canoniquement en `PointId`.
Sans le filtre canonique, le self brut apparaît `C(|A|,t)` fois puis une fois
après unique; avec le filtre dépendant du masque, il n'est pas émis et cette
valeur est contrôlée
séparément depuis les signatures de `A`. Dans les deux cas, ne pas la comparer
à l'identité `rank(M)` du compteur par points.

Sur le flux tout-requête, où le filtre dépendant du masque se réduit à `N<M`,
self exclus, le choix exact du meilleur `A` et de `t<=3` donne :

| n | choix de t pour k=1/2/3/4/5 | hits cover sur cinq ordres |
| ---: | --- | ---: |
| 64 | 1/1/1/1/1 | 151 187 730 |
| 200 | 1/1/1/2/3 | 1 290 965 625 |
| 400 | 1/1/1/2/3 | 3 898 171 415 |

Le diagnostic temporaire avait l'empreinte `f21c733fdb63`. Le gain est exact
mais insuffisant en tout-requête; il devient utile seulement après mesure du
`query_mask`.

Une autre généralisation exacte ne résout pas le mur : compter les postings de
signatures de taille fixe `t` donne la multiplicité
`C(|M intersection N|,t)` et le seuil `C(k,t)`. Le même diagnostic temporaire a
essayé chaque `t` de 1 à `k` et choisi le meilleur par ordre :

| n | hits points `t=1` | meilleur comptage adaptatif | tâches owner `t=k` |
| ---: | ---: | ---: | ---: |
| 64 | 226 854 316 | 214 618 361 | 3 030 554 |
| 200 | 1 892 861 494 | 1 668 077 445 | 17 282 892 |
| 400 | 5 747 389 371 | 4 914 636 211 | 44 258 951 |

Les petites signatures `t=2/3` ne gagnent pas sur ces catalogues; les gains
arrivent seulement près de `t=k`, où continuer à compter matérialise encore
les cliques. Il faut alors prendre directement un owner de la signature, pas
trier des milliards de paires dirigées.

Cette réduction reste candidate-based. La voie plus forte ne construit aucun
flux de candidats : elle intersecte à la demande les `k` postings de chaque
signature de requête et rend directement un carrier réel. Preuve,
kernel warp, transaction et portes sont dans
[`NOTE_SOLUTION_GPU_OWNER_DEMAND_DRIVEN_20260810.md`](NOTE_SOLUTION_GPU_OWNER_DEMAND_DRIVEN_20260810.md).

## Données et pipeline device

Attribuer à chaque générateur un `ActivationId` dense, canonique, ordonné par
niveau exact; chaque lot est un intervalle contigu. Construire les postings
triés en `ActivationId`. Pour éviter `K` copies persistantes, compacter et
traiter un ordre à la fois, puis libérer sa CSR avant l'ordre suivant.
La CSR candidate contient **tous** les générateurs visibles de rang au moins
`k`, fast compris. Le masque des requêtes peut exclure un fast certifié; le
masque des candidats ne le peut pas.

Un job device reçoit des segments de postings visibles et émet des clés 64 bits
`(uint64_t(query_id) << 32) | uint64_t(candidate_id)`, sous le contrat vérifié
`G<=UINT32_MAX`. Convertir avant le décalage est obligatoire; décaler un entier
32 bits de 32 serait indéfini.

En mode dirigé, toutes les occurrences et le self sont émis. En mode canonique,
le prédicat dépendant du masque est appliqué avant l'écriture de la clé; le self
est contrôlé sans l'émettre : chaque requête doit être retrouvée dans tous ses
postings de points en count et dans toutes ses signatures de `A` en cover. Le
manifeste seul ne détecterait pas une CSR tronquée; attendu, observé et checksum
self restent donc publiés dans les quatre modes.

Le pipeline `count_*` :

1. trie les occurrences point `(M,x,N)` par clé `(M,N)`;
2. réduit chaque run et vérifie que sa longueur est `|M intersection N|`;
3. garde les runs de longueur au moins `k`;
4. contrôle les self-runs en mode dirigé puis retire `M==N`.

Le pipeline `cover_*` est différent : une occurrence est `(M,T,N)`, où `T` est
une `t`-signature de `A`. Il trie puis rend uniques les seules paires `(M,N)`,
recalcule directement `|M intersection N|` sur les membres canoniques et garde
le seuil au moins `k`. La multiplicité d'un candidat dans les signatures de
`A` n'a aucune raison d'atteindre `k`; lui appliquer le RLE du compteur serait
faux. En mode dirigé, le self apparaît `C(|A|,t)` fois avant unique et une fois
après; en mode canonique il n'est pas émis.

Les deux pipelines projettent ensuite **après le seuil exact** vers le label de
composante gelé, réduisent localement par `(M,label)` en choisissant le plus
petit vrai candidat `N`, puis font un merge hôte global des jobs et slabs avant
le tri des carriers.

Un bloc par segment remplit une tranche de sortie préfixée par scan. Il n'y a
ni compteur atomique global, ni tableau dense `queries*generators`, ni
projection en racines **avant** le seuil exact. Seul le quotient tardif lit les
labels gelés.

La projection tardive est sûre si les labels proviennent d'un snapshot où
chaque classe est déjà réellement connectée. Ils sont gelés une fois pour tout
le lot, après les attaches fast certifiées, et restent immuables pendant tous
les jobs. Le label public est canonique, par exemple le plus petit
`ActivationId` réel de la classe, et non le représentant interne choisi par le
DSU. Une seule vraie arête `M--N` suffit alors à rattacher `M` à cette classe.
Le tableau `frozen_root[N]`, son domaine et son digest font partie de l'entrée
reçue; l'hôte recalcule en outre ce label canonique pour chaque carrier rendu.
Si deux classes fusionnent au replay, les carriers supplémentaires sont
redondants, jamais faux. Projeter les postings avant le comptage resterait
interdit, car cela inventerait des intersections entre des générateurs
différents.

## Chunking sans perte de compte

Pour chaque requête, calculer en entier vérifié sa masse visible après le
prédicat d'ownership du mode : somme des tranches de postings de points en
count, ou de signatures `T` en cover. Empaqueter des requêtes entières tant que leur somme tient
dans l'arène. Une coupure arbitraire dans ce flux serait fausse : elle pourrait
séparer entre deux jobs les occurrences count d'un même candidat ou les
apparitions cover qui doivent être rendues uniques.

Si une requête seule dépasse la capacité, la découper par intervalles contigus
d'`ActivationId` candidat. Deux recherches binaires bornent chaque posting pour
un intervalle. Toutes les occurrences d'un candidat `N` appartiennent alors à
un seul slab, donc toute son information reste entière. Un slab d'un seul
candidat contient au plus `rank(M)` occurrences en count et `C(|A|,t)` en
cover; avec le rang produit borné, le progrès est garanti dès que le plus petit
job CUB tient.

Les intervalles doivent former une partition exacte, sans trou ni recouvrement.
Le dernier candidat, les bornes `lower_bound` et les lots ex æquo sont des
portes permanentes. Une même racine peut apparaître dans plusieurs slabs : la
réduction locale ne suffit donc pas. Le merge hôte final choisit le plus petit
vrai candidat sur **tous** les slabs de la requête. Geler les labels pendant le
lot rend ce flux canonique et indépendant du packing; les rafraîchir entre jobs
changerait légitimement les carriers et interdirait un digest byte à byte.

## Mémoire et admission

Pour `H` occurrences, la phase count utilise deux buffers de clés pour le
radix-sort, un compteur par run et réemploie les buffers devenus libres pour la
sortie unique puis la sélection : environ `20*H` octets explicites avant
workspace. C'est spécifique au count et ce n'est pas le pic du job complet. Le
cover garde deux buffers de clés, puis jusqu'à `U` paires uniques, leurs poids
d'intersection, flags et sortie; il n'alloue aucun compteur de run utilisé comme
seuil. Son manifeste est calculé depuis ce layout réel, pas depuis `20*H`.
Si `S` paires passent le seuil,
le quotient device demande typiquement deux buffers de clés `(M,racine)` et
deux buffers de valeurs candidates, soit environ `24*S` octets, plus labels et
workspace. Les deux phases doivent réutiliser la même arène et le manifeste
publie leur maximum réel; aucun coefficient unique n'est une borne portable.
À `k=1`, `S` peut être du même ordre que `H`; la seconde phase peut donc être
plus chère que les `20*H` octets explicites du count.

Une première porte simplifiée peut copier les paires seuil vers l'hôte et y
faire le quotient. Elle reçoit plus facilement la sémantique, mais ne devient
pas le produit tant que la masse D2H n'est pas admise. La variante device
ci-dessus est la cible après cette porte.

Le contrat mémoire réel exige :

- tailles et produits en entier vérifié;
- requête préalable des workspaces de chaque primitive CUB;
- une arène device plafonnée contenant tous les buffers du job;
- publication de son high-water réel;
- vérification de la limite `num_items` de la version CUB;
- marge explicite pour contexte CUDA, CSR persistante et sorties hôte;
- route CPU exacte avant toute mutation si le plus petit job ne tient pas, ou
  refus si l'appelant exige le GPU.

La CSR persistante d'un ordre doit entrer dans le même manifeste. Le budget ne
peut jamais être décidé par une constante d'octets par hit sans workspace.

## Replay hôte transactionnel

À la coupe stricte du lot, l'hôte gèle un proxy distinct pour chaque racine
ancienne, active tous les générateurs du lot dans un DSU scratch, applique les
attaches fast certifiées, puis fige les labels de composante transmis au
device. Il lance tous les jobs, merge leurs carriers, puis rejoue dans un ordre
canonique. Pour chaque paire rendue `(M,N)` :

1. valider les deux handles, leur visibilité et leurs rangs;
2. recalculer l'intersection courte des membres et exiger au moins `k` points;
3. conserver les `k` premiers points communs comme témoin d'arête réel;
4. exiger que `frozen_root[N]` soit le label canonique hôte reçu pour ce snapshot;
5. appeler `find(M)` et `find(N)` au moment du replay;
6. unir seulement si les composantes diffèrent.

Le pruning est dynamique et local au replay. Un `seen_roots` global au lot est
faux, et compresser les postings par racine avant le comptage invente des
intersections. Les doublons directionnels nouveau--nouveau peuvent rester au
premier jalon : le DSU les absorbe sans risque.

Après tous les jobs, classer naissances, continuations et multifusions avec les
proxies stricts gelés, vérifier les identités, puis publier le lot en une fois.
Toute erreur CUDA, allocation ou validation jette le scratch; aucun fragment du
lot ne devient observable. Un repli CPU recommence le lot entier.

## Reçu minimal

Publier par ordre, lot et job :

- mode exact `count_directed`, `count_mask_canonical`, `cover_directed` ou
  `cover_mask_canonical`, avec `t` et digest des ensembles `A` en mode cover;
- bits reçus de boule/saturé exacts, handle unique par boule, fermeture carrier,
  lots/scratch exacts et exhaustivité des attaches fast;
- masse de la CSR et requêtes; en count, hits de points, runs et somme des
  longueurs; en cover, hits de signatures, candidats uniques, intersections
  directes et rejets sous `k`;
- preuves self attendues/observées et checksum; longueurs des self-runs en
  count dirigé, `C(|A|,t)` avant unique et un self après unique en cover dirigé;
- paires au seuil avant quotient, racines gelées et carriers après quotient;
- avant quotient, paires au seuil query--ancien, query--fast omis,
  query--query et nonquery--nonquery; après quotient, carriers par racine;
- digest du domaine `R`, mode et provenance du certificat batch, nombre et
  digest de ses arêtes réelles `R--R`, partitions induites attendue/observée et
  statut de validation;
- jobs, requêtes lourdes, slabs et couverture exacte de leurs intervalles;
- paires prunées, unions tentées et réussies;
- octets persistants, capacité d'arène, high-water et workspaces CUB;
- temps H2D, expansion, tri, RLE count ou unique/intersection cover,
  sélection, D2H et replay CPU;
- digests du sidecar, de l'ordre d'activation, de la CSR et du flux de paires.

Identités obligatoires : en count, la somme des longueurs des runs vaut le
nombre de hits, mais cette identité interne ne suffit pas; chaque occurrence
`(M,x,N)` appartient à un seul job ou slab. En cover, chaque occurrence
`(M,T,N)` appartient à un seul job ou slab, le nombre d'uniques concorde avec
le sort/unique et chaque poids publié est recalculé depuis les membres. Dans les
deux cas, `H_job` égale la masse préflight et le dernier offset du scan, et la
somme des slabs égale la masse du mode. En `count_directed`, chaque requête
possède un self-run de longueur `rank(M)` dans l'unique slab contenant son ID;
en `cover_directed`, sa longueur brute vaut `C(|A|,t)` et l'unique contient un
self. Après seuil et avant quotient, les deux modes `*_directed` voient chaque
paire au seuil query--query dans les deux directions, chaque paire au seuil
query--ancien et query--fast omis une fois, et aucune paire
nonquery--nonquery. En mode canonique, le
contrôle CSR self indépendant donne les mêmes valeurs attendues sans écrire de
clé; chaque **paire au seuil** query--ancien, query--fast et query--query apparaît
exactement une fois avant quotient, nonquery--nonquery zéro fois. Après quotient,
plusieurs paires au seuil vers une même racine peuvent légitimement devenir un
seul carrier. La
sortie est triée, unique, visible et
non-self; chaque paire fournit un témoin réel de `k` points; les intervalles
d'une requête lourde partitionnent exactement les candidats. Le reçu porte
aussi masse et digest de la CSR, digest du `query_mask` et digest de
`frozen_root`.

## Fixtures et mutants

Fixtures minimales :

- comptes `k-1`, `k` et `k+1`;
- chaîne nouveau--nouveau `A={0,1}`, `B={0,1,2,3}`, `C={2,3}` à `k=2`;
- deux anciens dans une même racine dont aucun ne contient seul les deux points
  du nouveau;
- deux nouveaux rejoignant séparément la même racine ancienne;
- représentant DSU non incident;
- dernier générateur du lot indispensable;
- candidat futur partageant `k` points mais encore invisible;
- ancien fallback et nouveau fast reliés : l'omission de la requête fast doit
  être justifiée par ses attaches certifiées;
- lot sans ancien carrier : une requête `A` plus précoce et un fast omis `B`
  partagent exactement `k` points; le filtre brut `N<M` perd `A--B`, tandis que
  le filtre dépendant du masque doit la rendre;
- boule nulle à deux handles, saturé incomplet et carrier strict supprimé sous
  faux bit de complétude : tous doivent refuser avant construction du masque;
- identifiants clairsemés et frontière haute d'`ActivationId`;
- requête lourde forcée en plusieurs slabs, avec voisins sur chaque frontière;
- `k=32`, intersections 31 et 32;
- job vide avec `k` invalide;
- échec injecté après le dernier job, qui doit laisser zéro publication.

Mutants prioritaires : seuil `k-1` ou `k+1`; clé sans identifiant de requête;
lot courant omis; futur inclus; projection en racines avant comptage;
représentant DSU rendu au lieu du vrai candidat; `seen_roots` partagé;
coupure brute par nombre de hits; dernier segment ou dernier posting omis;
borne de slab décalée; self conservé; doublon au-delà du seuil; scratch non
réinitialisé; offset tronqué en 32 bits; mauvais `frozen_root` fusionnant deux
classes; labels rafraîchis entre jobs; réduction seulement locale sans merge
global; filtre brut `N<M` sous masque partiel; fast retiré du masque candidat;
label DSU non canonique utilisé comme `frozen_root`; pic `k=1` avec `S` proche de `H`;
handle de boule dupliqué; saturé incomplet; fermeture carrier manquante; seuil
RLE appliqué au cover; publication partielle après erreur.

## Porte G4

Avant l'escalier de masse, comparer le flux device à un oracle CPU indépendant
par intersections de listes, puis rejouer **uniquement** la sortie GPU et
comparer partitions, records et marqueurs à G2 et au compteur CPU. Exiger la
même sortie avec une grande arène, plusieurs packs et des slabs minuscules;
tuer tous les mutants; répéter byte à byte; passer `memcheck`, `racecheck`,
`initcheck` et `synccheck`; injecter les erreurs CUDA et le budget moins un
octet.

Cette porte reçoit le compteur/cover comme backend exact du `query_mask`
lorsque sa masse est admise. Le kernel exhaustif `face-owner` reste son
falsificateur borné et son outil de profilage. Si `H_query(mode)` est trop grand, l'owner
demand-driven est le candidat prioritaire : il rend une arête par signature
sans flux de hits ni table globale de faces.
