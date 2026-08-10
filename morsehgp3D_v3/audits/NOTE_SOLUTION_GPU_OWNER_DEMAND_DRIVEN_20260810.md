# Solution GPU exacte du `query_mask` par owner demand-driven

Date : 10 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, `backend=gpu_product_candidate`,
`profile=quantized_u16_input_only`, `mode=mathematical_and_architectural_proposal`,
`public_status=not_claimed`.

## Décision proposée à Claude

Le flux `face-owner` exécuté sur G4 confirme que les signatures et les tris sont
une charge GPU favorable, mais sa forme courante matérialise toutes les
incidences. Le compteur de postings par points évite les faces, mais son mode
tout-requête émet 226,9 millions à 5,75 milliards de hits dès `n=64..400`.

Un prochain kernel utile peut être différent. Pour chaque `k`-signature d'une
requête du **masque générique reçu**, il intersecte directement les `k` postings
de points triés et rend leur premier générateur visible commun. Ce masque ne se
réduit aux fallback qu'après certification des omissions fast.
Il produit ainsi un carrier réel sans matérialiser la clique, le flux des hits ou
une table globale de signatures. Le DSU, les témoins publics et le commit du lot
restent sur l'hôte.

Cette route réutilise l'unranking déjà reçu du kernel `face-owner`, mais remplace
son tri global par des intersections à arrêt précoce. Elle est exacte
relativement au catalogue fourni; l'autorité publique du transcript dépend
toujours du sidecar et du reçu de source.

## Théorème de l'owner à la demande

Fixer l'ordre `k` et un lot `b`. Pour chaque point `x`, `P_x^b` est le posting
trié en `ActivationId` de tous les générateurs visibles de rang au moins `k`, y
compris **tout** le lot `b` et aucun lot futur. Tous les générateurs, fast comme
fallback, restent dans ces postings.

Pour une requête `M` du lot et chaque sous-ensemble `F` de `M` de taille `k`,
définir `owner_b(F)` comme le plus petit `ActivationId` de l'intersection des
`P_x^b` pour `x` dans `F`. Cette intersection n'est jamais vide : elle contient
`M`. Émettre l'arête réelle `M--owner_b(F)` lorsque les deux handles diffèrent.

Pour deux générateurs visibles `M,N`, `|M intersection N|>=k` si et seulement
s'ils partagent au moins une telle signature `F`. Tous les générateurs qui
contiennent `F` sont reliés à son premier owner : un générateur d'un lot ancien
l'a été lors de son activation, et les générateurs d'un même lot voient le même
owner après staging atomique du lot entier. L'étoile ainsi obtenue a exactement
les composantes de la clique portée par `F`. L'union sur toutes les signatures
donne donc exactement les composantes du graphe de seuil, à chaque coupe stricte
et fermée.

Cette compression à un owner exige `|F|=k`. Avec une signature plus petite
`t<k`, deux porteurs de `F` ne sont pas nécessairement voisins : choisir un
owner unique pourrait alors masquer le vrai porteur qui atteint le seuil.

La version **tout-requête** n'utilise aucune hypothèse de complétude. En
revanche, conserver le minimum global avec un masque partiel est faux : dans un
lot sans ancien carrier, une requête précoce `A` et un fast omis plus tardif `B`
peuvent partager `F`; `owner(F)=A` ne produit alors aucune arête et perd
`A--B`.

Une extension exacte existe. Après staging et attaches fast, poser `R` égal
aux anciens visibles et aux fast omis, et `Q` égal au `query_mask`. Exiger un
certificat batch global : un sous-DSU ou forest constitué uniquement d'arêtes
réelles dont les deux extrémités sont dans `R` doit avoir les mêmes composantes
que le graphe de seuil induit par `R`. Restreindre les labels d'un DSU ayant déjà
utilisé un chemin via `Q` ne certifie pas ce sous-graphe. Pour chaque signature
`F` d'une requête, choisir comme ancre le
plus petit porteur dans `R` s'il existe, sinon le plus petit porteur dans `Q`;
chaque requête porteuse distincte de l'ancre émet vers elle. Si une ancre `R`
existe, tous les porteurs non-requêtes de `F` sont déjà connectés par le
certificat batch et toutes les requêtes rejoignent cette classe; sinon on
obtient l'étoile habituelle entre requêtes. Toutes les arêtes émises partagent
le vrai `F`. L'union de ces étoiles a donc exactement les composantes du graphe
de seuil. Le certificat est global au batch, pas nécessairement par signature,
mais il doit être lié au sidecar validé; `smax>=n` et le bit local
`principal_support` ne suffisent pas.

Sans ce certificat batch, le masque doit contenir **tous** les générateurs de
rang au moins `k` de tout lot ayant au moins deux générateurs actifs à cet
ordre, fast compris. La taille brute du lot est trop conservatrice : les
membres de rang inférieur à `k` ne portent aucune `k`-face. L'état normatif
s'appelle `query_mask`; `fallback_mask` n'en est qu'un cas certifié.

### Déduire le certificat batch de la géométrie validée

Claude peut éviter un nouvel oracle global, mais « `BallKey` injective + source
complète » est trop faible pour une entrée hostile. Le sidecar doit recevoir
ensemble les obligations suivantes :

- chaque handle porte une boule exacte `B=MEB(U)`, avec `U` inclus dans ses
  membres, et ses membres sont exactement le saturé fermé `X intersect B` lié
  au digest du nuage;
- la `BallKey` est exacte, ses collisions sont résolues et le catalogue contient
  exactement un handle par boule; plusieurs supports minimaux d'une même boule
  ne créent jamais plusieurs générateurs;
- `source_complete_for_order[k]` signifie la fermeture carrier : pour toute
  `k`-signature visible `F`, l'unique générateur `Sat(F)` de `MEB(F)` existe;
- les niveaux et les lots utilisent l'égalité exacte, et le scratch ancien est
  déjà certifié;
- pour chaque fast omis de rang supérieur à `k`, de vraies arêtes avec témoins
  atteignent **chaque** composante stricte incidente. Si ce dernier bit est
  dérivé des modes principal/redondant, sa provenance `q_min` est elle aussi
  reçue.

Pour une arête fast--fast `M--N` du lot, choisir `F` dans leur intersection. Si
`MEB(F)` avait le niveau courant, les boules de `M` et `N` seraient deux
miniboules de `F`; l'unicité euclidienne donne la même boule, puis la canonicalité
du catalogue le même handle. Le carrier est donc strict et la fermeture source
fournit `Sat(F)` dans l'ancien scratch; les deux fast rejoignent sa composante.
Pour une arête fast--ancien, le même carrier est au plus au niveau de l'ancien.
Toutes les attaches sont réelles et couvrent chaque incidence : le forest sur
`R` et le graphe induit ont donc les mêmes composantes, pour n'importe quelle
arité ex æquo.

Si `rank(M)=k`, `M` n'a aucun voisin visible distinct : un tel `N` contiendrait
`M`; minimalité et visibilité imposeraient l'égalité des niveaux, puis
unicité, saturé complet et handle canonique donneraient `N=M`. Sous ces
obligations, le minimum global redevient suffisant, car toute signature mixte
possède un carrier strict antérieur. Sinon employer l'ancre préférant `R` avec
certificat batch explicite, ou élargir `query_mask` à tout le lot actif.

Le live n'a pas encore ce contrat : son index de centre est suivi d'une
comparaison exacte du rayon, mais `lookup_ball` rend le premier match au lieu de
refuser deux handles de la même boule. Le futur sidecar doit fermer ce cas avant
de lever `solo_batch`.

Le corollaire est néanmoins positivement observé sur le vrai producteur v3.
Un probe CPU local sur vingt catalogues `smax=n=11` sous prétention de
complétude, graines `0..19`, compte
3 443 handles et zéro paire de handles pour la même boule exacte. Parmi 1 296
paires générateur--ordre d'un même lot, 78 atteignent le seuil; leurs 213
incidences `(paire-ordre,F)` ont toutes un `Sat(F)` présent exactement une fois et
strictement antérieur, sans manque ni doublon. Les 2 248 cas `rank=k` ont testé
47 548 comparaisons à un candidat visible distinct de `M` et zéro voisin au seuil. Le binaire
temporaire avait l'empreinte
`73600135bbbbd3d903c96c11c9b6a8a71f9c62da29ba7b372babafc9942b422b`.
Ce diagnostic crédite la route sur ce corpus; il ne remplace pas les rejets
hostiles de doublon, saturé incomplet et fermeture carrier manquante.

Un falsificateur abstrait a validé le filtre et l'ancre sur 186 972
configurations masquées. Une seconde passe, avec anciens, permutations
intra-lot et seulement un spanning forest certifié du sous-graphe non-requête,
donne 146 504 partitions identiques au graphe complet. Le minimum global sous
masque partiel échoue sur le cas `A--B` ci-dessus; ces diagnostics doivent
devenir une porte permanente, pas remplacer la preuve. Un rejeu indépendant de
30 000 familles donne encore zéro écart pour l'ancre préférant `R`, contre
8 802 pertes de partition pour le minimum global sous masque partiel.

## Kernel proposé

Le payload commun nomme le handle rendu `carrier`. Le terme `owner` est réservé
au minimum global; en mode `prefer-R`, l'ancre peut être plus tardive que la
requête et ne satisfait pas les invariants de `FaceOwnerDeviceEdge` actuel.

Une tâche est le couple `(M,combination_rank)`. Le device :

1. reconstruit `F` par l'unranking combinatoire déjà utilisé par
   `faceowner_device_kernel.cu`;
2. en mode minimum global, prend dans les `k` postings les préfixes visibles bornés par
   `ActivationId(M)`, avec filtre `rank>=k` déjà matérialisé par la CSR de
   l'ordre;
3. choisit le préfixe le plus court comme driver, avec `PointId` comme
   départage canonique;
4. parcourt ses `ActivationId` dans l'ordre et teste leur présence dans les
   autres tranches par intersection gallopante ou recherche binaire;
5. s'arrête au premier candidat commun et rend
   `(batch,M,carrier,combination_rank)`.

Le premier hit du driver est bien le minimum global, puisque le driver est
trié selon l'ordre d'activation canonique. Un warp peut tester un tile de
candidats en parallèle, mais il doit sélectionner le premier lane valide du
premier tile valide; un arrêt au premier thread arrivé serait faux. L'absence
de carrier est un refus interne, jamais une sortie vide, car le self `M` est la
borne terminale garantie. Sous `rank<=32` et `k<=10`, le rang local de
combinaison tient dans un `uint32_t` car `C(32,10)=64 512 240`; l'hôte
reconstruit alors `F` depuis les membres canoniques de `M`. Le cache doit en
revanche comparer la clé complète en `PointId` : ce rang local dépend de `M` et
n'identifie pas une signature globale.

La variante à masque partiel effectue deux recherches logiques. Elle cherche
d'abord le premier porteur commun appartenant à `R`, sur tout le lot visible;
seulement en son absence elle rend le premier porteur commun de `Q` jusqu'au
self `M`. Elle choisit de nouveau le posting global complet le plus court,
départagé par `PointId`. Avec une seule CSR et un bitset, la v0 sûre scanne une
fois ce driver entier, mémorise le premier hit `Q` et continue jusqu'au premier hit `R`
ou jusqu'à la fin. Couper à `M` serait faux : l'ancre fast omise peut avoir un
`ActivationId` supérieur. Deux vues indexées `R/Q` permettent deux drivers plus
courts, mais leur construction et leur mémoire entrent alors dans le budget.

Les tâches sont indépendantes et se chunkent donc à leurs frontières, sans la
contrainte délicate des runs coupés du compteur de hits. Après chaque chunk,
trier et réduire `(M,carrier)` en gardant le plus petit témoin `F`; merger ensuite
tous les chunks sur l'hôte avant le replay. Une signature répétée entre
requêtes peut être mise en cache, mais ce cache est une optimisation et non une
précondition de correction.

Le cache global immuable `F -> minimum visible` ne vaut que pour le mode
tout-requête ou le corollaire à carrier strict. Dans le mode général « préférer
`R` », l'ancre dépend du lot, du `query_mask` et de son digest : l'entrée doit
être scratch et liée à `(batch,query_mask_digest,anchor_mode)`. Réutiliser sans
ces clés un owner global peut réintroduire exactement l'arête manquante du cas
ex æquo. Comme la v0 ne publie aucun miss avant la barrière finale du lot, un
tel cache scratch ne serait jamais relu : le désactiver en mode `prefer-R`.
Seuls le minimum global et le corollaire à carrier strict bénéficient d'abord
du cache persistant. Des phases intra-lot déterministes pourraient l'étendre
plus tard, avec un nouveau contrat de transaction.

## Travail et mémoire admis

Le nombre exact de tâches est `I_query,k=sum_M C(rank(M),k)` sur le masque de
requêtes reçu. En mode minimum global, une borne du nombre de candidats driver
avant lancement est
`D_prefix,k=sum_(M,F) min_(x in F) |P_x intersect (-infinity,M]|`, la somme
portant sur les **tâches** et non sur les seules signatures distinctes. Une
même signature portée par plusieurs requêtes doit donc être comptée plusieurs
fois hors hit de cache. Le temps doit encore inclure les recherches dans
les `k-1` autres listes; `D_prefix` n'est donc pas à lui seul une borne en
cycles. Publier cette masse, le nombre réel de candidats examinés et les
comparaisons de membership.

Cette borne se préflight sans énumérer les signatures. Pour une requête de rang
`r`, trier les degrés préfixés `d_1<=...<=d_r`; sa contribution vaut
`sum_(i=1)^(r-k+1) C(r-i,k-1)*d_i`. C'est exactement la somme des plus courts
drivers de toutes ses `k`-signatures. Une borne v0 des probes binaires multiplie
encore ce total par `(k-1)*ceil(log2(G_visible+1))`; elle est grossière mais
fail-closed pour le dispatcher.

La formule fermée des drivers a été comparée à l'énumération explicite sur
10 000 vecteurs de degrés aléatoires, tous les ordres admissibles jusqu'à
`r=12`, sans aucun écart. Cette porte combinatoire doit accompagner son
implémentation en entier vérifié.

Sous le certificat batch et l'ancre préférant `R`, la borne v0 sans index
supplémentaire est `D_preferR`, somme, par tâche, des longueurs du plus court
driver global visible. Ce n'est pas `D_prefix` : un carrier non-requête peut
avoir un `ActivationId` supérieur à celui de `M`. Les seules cardinalités
`P_x intersect R` et `P_x intersect Q`
ne bornent pas les lectures nécessaires pour sauter l'autre domaine. Si des
vues select `R/Q` sont construites, leurs plus courtes longueurs deviennent des
bornes distinctes `D_RQ`, mais leur construction, leurs octets et leurs lectures
doivent être admis. Publier le mode et sa borne, puis séparément entrées du driver scannées, candidats
`R/Q` testés et comparaisons de membership. Cette route n'est admise que si ce
préflight et le certificat batch tiennent; sinon le dispatcher revient au
masque sûr tout-requête du lot.

Le device conserve la CSR d'un seul ordre, les descripteurs des requêtes, un
chunk de sorties et les workspaces de compaction. Il ne conserve ni les
hits `H_query(mode)`, ni les incidences de tous les générateurs, ni une table
`signature -> owner`. Les sorties ont au plus une entrée par tâche avant
déduplication. Le pic reste à recevoir par une arène plafonnée et les tailles de
workspace CUB; aucune constante d'octets par tâche ne constitue une borne.

Le dispatcher exact compare avant toute mutation :

- `H_query(mode)` pour le cover/count par postings, avec un champ distinct pour
  `count_directed`, `count_mask_canonical`, `cover_directed` et
  `cover_mask_canonical`;
- `I_query` et la borne du mode d'ancre : `D_prefix` pour le minimum global,
  `D_preferR` avec une CSR unique, ou `D_RQ` avec des vues sélectives;
- les limites de la route CPU demand-driven.

Il choisit la voie admise la moins chère. Si aucune ne tient, il refuse ou
reste CPU selon le contrat appelant. La version tout-requête sur les catalogues
G4 possède exactement `3 030 554`, `17 282 892` et `44 258 951` tâches à
`n=64/200/400`; ce sont de bons paliers de qualification, pas une extrapolation
50 k.

Le nombre de tâches n'est pas le nombre de candidats lus. Un diagnostic du
driver simple, binaire temporaire d'empreinte `0b981c802329`, donne :

| n | k | candidats examinés avant le premier owner |
| ---: | ---: | ---: |
| 64 | 1/2/3/4/5 | 62 243 / 13 115 996 / 67 342 812 / 159 653 839 / 226 372 675 |
| 200 | 1/2/3/4/5 | 329 920 / 155 727 450 / 845 110 168 / 2 091 999 751 / 3 088 842 316 |
| 400 | 1/2/3/4/5 | 833 925 / 460 227 691 / 2 570 310 591 / 6 527 185 468 / 9 829 608 549 |

Le tout-requête est donc lui aussi NO-GO en travail, malgré sa petite mémoire.
Cette route ne devient attractive que si le sidecar rend le masque rare et
si `J_query` mesuré ou la borne propre au mode d'ancre sont admis. Une intersection
gallopante peut battre le driver linéaire, sans supprimer cette obligation.

Un bridge opposé maintient une table incrémentale `F -> premier owner` : il
traite les incidences de **tous** les générateurs, mais répond ensuite en temps
constant. À `n=400`, les cinq ordres ont 44 258 951 incidences et 1 823 707
signatures distinctes au total; traité ordre par ordre, le maximum persistant
est 1 232 705 entrées. Cette table évite les listes d'incidences et les cliques,
mais matérialise bien un dictionnaire global de `k`-faces. Elle n'est admissible
qu'avec un préflight de ses clés complètes, qui dépassent 128 bits au profil
50 k/K=10, et ne doit pas être renommée « sans faces ».

Entre les deux, un cache owner de capacité fixe ne change jamais la sémantique.
Sur hit, il rend l'owner immuable de `F`; sur miss ou collision, il refait
l'intersection exacte. Une éviction ne coûte que du temps si la clé complète
est vérifiée. La v0 GPU sûre garde le cache **read-only pendant tout le lot** :
elle collecte les misses `(F,owner)`, les trie/réduit, puis reconstruit le cache
seulement après la barrière validée du lot, dans un ordre déterministe. Aucun
rebuild n'a lieu entre chunks. Une insertion
concurrente dans une clé de 135 bits pourrait sinon exposer une paire clé/owner
déchirée et rendre hits/misses dépendants de l'ordonnancement GPU. Une variante
mutable exige états `LOCKED/READY`, acquire/release et double lecture stable;
elle n'appartient pas au premier jalon. Le cache du lot reste en scratch et
n'est committé qu'avec le lot; son reçu publie hits, misses et `J_miss`.

Un cache direct-mapped de `2^20` entrées sur le diagnostic tout-requête
`n=400` donne :

| k | taux de hit | candidats examinés sur miss |
| ---: | ---: | ---: |
| 1 | 99,952 % | 400 |
| 2 | 99,524 % | 6 754 524 |
| 3 | 97,661 % | 111 855 186 |
| 4 | 93,239 % | 649 780 579 |
| 5 | 85,950 % | 1 793 565 358 |

Le binaire temporaire avait l'empreinte `359780da96e1`. C'est un résultat très
encourageant pour la localité, pas un layout produit ni un taux CUDA admis : le
simulateur séquentiel publiait immédiatement ses insertions, tandis que la v0
read-only ne bénéficie dans un kernel que des owners committés aux barrières
antérieures. Il faut remesurer séparément hits hérités et répétitions
intra-lot sous le vrai découpage. Les clés tiennent sur 64 bits à `n=400`,
tandis qu'une signature `n=50 000,k=10` demande 135 bits. Une collision doit
donc être un miss après comparaison de la clé complète, jamais un hit sur le
seul hash.

Le choix constructif est donc adaptatif : owner demand-driven si le masque
et `J_query` sont petits; cache ou dictionnaire owner si `I_all` et leur
capacité tiennent; cover/count si `H_query(mode)` tient; sinon trie CPU
demand-driven ou refus. Aucun de ces coûts ne peut être déduit de la seule
fraction de générateurs fallback.

## Diagnostic du masque hybride actuel

Le masque est effectivement rare sur une petite campagne complète. Avec
`HEAD=f6cb562`, `saturated_fold_hybrid.hpp=3147bb0564d`, le binaire Release
`c33bd671758c` et les vingt graines `0..19` de
`--points 11 --coord 23 --smax 11 --max-order 5 --join hybrid --compare-joins 1` :

- 20/20 folds concordent avec G2;
- 10 529 passages générateur--ordre prennent le fast principal, 290 le
  fallback et 2 690 la réduction redondante;
- le fallback représente 2,15 % de tous les passages et 2,68 % des seuls
  passages événementiels principal+fallback;
- les comptes par graine vont de 0 à 37 fallback, pour 38 392 postings lus au total et un
  maximum de 8 617 sur un nuage.

Le fallback live n'est toutefois pas encore le masque sûr. Un second préflight
hors dépôt inclut tous les actifs des lots qui en ont au moins deux à l'ordre
courant. Sur cette même campagne, il donne `597/13 509=4,419 %` de requêtes,
`I_query=2 042`, `H_count_directed=25 394`, `D_prefix=34 559` et
`J_query=10 809`; aucun
non-principal n'apparaît hors de ces lots ex æquo. Le binaire avait l'empreinte
`165dc75b7565`. C'est un résultat positif pour un `query_mask` encore rare, mais
pas une prévision 50 k : `n=11` est minuscule et la prétention de complétude
vient encore de l'appelant. Le prochain reçu utile n'est donc pas un chrono GPU,
mais le digest de ce masque **par ordre et lot**, avec les quatre masses. Les
seuls totaux actuels ne permettent pas encore au dispatcher de choisir sa voie.

## Transaction hôte

À la coupe stricte, l'hôte crée les proxies des racines anciennes, active tout
le lot dans un scratch et applique les attaches fast certifiées. La CSR visible
et le masque de requêtes sont ensuite gelés jusqu'à la fin de tous les chunks.
Pour chaque réponse device, l'hôte :

1. valide les handles, le lot, les rangs et la visibilité;
2. reconstruit `F` depuis `(M,combination_rank)`, exige sa taille `k` et son
   inclusion dans les membres du carrier;
3. recalcule sur la porte bornée que le carrier est le minimum global en mode
   ordinaire, ou le premier non-requête puis premier requête en mode masqué;
4. rejoue l'arête seulement si les composantes scratch diffèrent.

Les naissances, continuations, multifusions, témoins et marqueurs sont classés
avec les proxies stricts gelés. Une erreur CUDA, un carrier absent ou une
validation fausse jette le scratch du lot courant; un repli CPU recommence ce
lot depuis sa coupe stricte, sans poursuivre depuis un lot partiellement muté.

## Reçu minimal

Publier par ordre et par lot :

- digests des points, du catalogue, du sidecar, de l'ordre d'activation, de la
  CSR visible et du `query_mask`;
- bits reçus `exact_ball_and_full_saturate`, `unique_handle_per_ball`,
  `carrier_closure_for_order[k]`, lots exacts et scratch ancien validé;
- digest du domaine `R`, mode/provenance du certificat batch, arêtes réelles
  `R--R`, témoins et digests des partitions induites attendue/observée;
- générateurs visibles, fast, fallback et masse des postings;
- par fast omis : rang, provenance `q_min`, mode, carriers réels et témoins,
  racines strictes incidentes/attachées et égalité de leurs digests;
- `I_query` prédit/réel par rang, mode d'ancre et borne associée
  `D_prefix`, `D_preferR` ou `D_RQ`, prédite et observée;
- tâches, chunks, combinaisons émises, carriers self/non-self et types d'ancre, candidats
  réellement testés, avances gallopantes et comparaisons de membership;
- mode d'ancre, digest du certificat batch, capacité du cache, digests des
  clés/owners committés, hits hérités de lots validés, répétitions intra-lot,
  misses, collisions, évictions et `J_miss`;
- arêtes avant/après déduplication, unions tentées/réussies et témoins validés;
- octets persistants, capacité/high-water de l'arène et chaque workspace;
- H2D, intersection, compaction, D2H, validation et rejeu CPU séparés.

Identités obligatoires : une tâche par combinaison prédite, exactement un carrier
par tâche, zéro carrier d'un lot futur, somme des chunks égale à `I_query`, chaque
arête possède son vrai témoin `F`, et le rejeu device seul rend les mêmes
partitions, records et marqueurs que G2. `carrier<=M` n'est exigé que dans le mode
minimum global; l'ancre non-requête d'un lot ex æquo peut légitimement être plus
tardive que `M` et doit alors satisfaire le digest du masque et le certificat
batch.

## Fixtures et mutants

Fixtures permanentes : ancien--nouveau; trois générateurs d'un même lot portant
la même signature; owner strict ancien; owner courant plus petit; candidat
futur de plus petit `GeneratorId`; ex æquo de niveau; identifiants clairsemés;
`k=1`; `k=rank`; unranking `k=7..10` et frontières `C(32,k)`; driver le plus
court en première/dernière position; owner au
dernier élément du dernier tile; nouveau fallback contre ancien fast; nouveau
fast contre ancien fallback; lot sans ancien carrier où un fallback `A` de plus
petit ID et un fast omis `B` partagent exactement une signature — l'owner self
de `A` perdrait `A--B`; cache vidé entre chunks avec flux identique;
échec du dernier chunk sans publication; un point avec deux handles de la même
boule nulle, rejet `duplicate_ball_handle`; saturé incomplet `{0,2}` face à
`{0,1,2}` sur une ligne; carrier singleton strict retiré malgré un faux bit de
complétude; cosphère à supports multiples mais un seul handle; trois boules ex
æquo reliées par carriers stricts.

Mutants : CSR sans le lot courant; candidat futur inclus; CSR sans les fast;
ordre par `GeneratorId` brut plutôt que `ActivationId`; unranking décalé;
rang local de combinaison utilisé comme clé globale du cache;
dernier élément du driver omis; arrêt au premier lane arrivé; recherche binaire
sur borne ouverte; mauvais filtre de rang; self accepté sans chercher un owner
antérieur; minimum global conservé sous masque partiel; première passe coupée à
`M`; tâche fast omise sans certificat batch; cache réutilisé sous un autre
digest de masque; hit de cache accepté sans comparer
la clé complète; lecture déchirée clé/owner; publication `READY` sans
release/acquire; entrée d'un lot avorté visible; owner cache futur sans garde
de visibilité; déduplication locale sans merge global; handle de boule dupliqué;
saturé incomplet; fermeture carrier manquante; lots formés par égalité flottante;
publication avant validation.

## Porte CPU puis G4

La première porte compare **chaque carrier/ancre de chaque tâche** à une intersection
CPU indépendante des postings, puis rejoue uniquement les arêtes device et
compare partitions, records et marqueurs à G2. Elle exige le même flux avec un,
deux et plusieurs chunks, tue tous les mutants et exerce les deux cas mixtes du
sidecar. Sous CUDA, ajouter `memcheck`, `racecheck`, `initcheck`, `synccheck`,
les erreurs injectées et le budget moins un octet.

Un falsificateur combinatoire indépendant a comparé après chaque lot le graphe
complet et les étoiles au premier owner sur 20 000 familles aléatoires,
`k<=5`, ex æquo inclus : 66 066 états de niveau, zéro désaccord de partition.
Ce résultat doit devenir une porte permanente; la preuve ci-dessus reste
l'autorité.

Le G4 n'est utile qu'après cette porte et après correction du CTest CUDA
actuellement inversé. Le kernel `face-owner` existant reste alors l'oracle de
masse et de flux; l'owner demand-driven devient le candidat du fallback qui ne
matérialise aucune table globale de `k`-faces.
