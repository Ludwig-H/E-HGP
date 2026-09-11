# Réutiliser une population complète après échange

11 septembre 2026. `phase=exploration_v7_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

Ce complément de la [voie statique CPU](RESOLUTION_STATIQUE_CPU_20260911.md)
est intégré dans le header `6763a877…`. Le défaut reste le cache temporel ;
`--static-threads=1` ou `4` active la voie statique, sans nouvelle option.
Les résultats antérieurs sur `33e7d05e…` restent des références distinctes.

## Pourquoi une MEB peut être supprimée

Après remplacement du premier site du support par le premier intrus strict,
le résolveur trie la nouvelle facette F'. Il consulte maintenant la table
de semis déjà construite pour cet ordre K. Un hit exige l'égalité de la
**clé entière** de F' avec toute la population fermée d'une boule B du census,
intérieur et coquille compris. Un support seul ou une coquille partielle
ne satisfait pas cette condition.

Le [lemme de l'auditeur](../audits/receipts_static_followup_20260911/README.md)
établit alors MEB(F')=B et une diminution stricte du rayon. En effet, un
support positif de B impose son rayon et F' est contenu dans B. Si le rayon
restait égal à celui de la boule précédente, l'unicité de la MEB les
identifierait ; le site retiré appartiendrait alors à la population complète
de B, donc à F', contradiction. Le terminal suivant est exactement la même
BallId, admise à K=p+u et strictement avant le bloc consommateur.

La MEB finale, la matérialisation de sa clé et son lookup catalogue ne sont
donc plus exécutés. Le choix de l'intrus, du site retiré, les échanges, les
ancres temporelles et le calendrier des parents ne changent pas. Aucune
table globale, mosaïque Delaunay ou liste exhaustive de facettes Gamma n'est
ajoutée ; la table de semis existante reste partagée et immuable.

## Compter le travail payé

Les champs par K `post_seed_queries`, `post_seed_hits` et
`post_seed_terminals` séparent les essais Q, hits H et terminaux T des semis
initiaux S. Pour U clés uniques et D échanges, sur une exécution complète :
`Q=D`, `H=T≤min(D,U−S)` et `MEB=U−S+D−H`. Un terminal évité n'est jamais
compté comme un `anchor_hit` payé. Les compteurs de dernière descente et
de longueur de chaîne restent inclus. Le tag de comptage est
`static_exact_sort_unique_complete_population_seeds_after_exchange_v2`.

Le coût ajouté est une recherche dichotomique exacte par échange,
O(K log S), sans nouvelle allocation proportionnelle aux requêtes. Au plus
une dernière MEB est supprimée par chaîne. Les trois tableaux de statistiques
augmentent légèrement la taille des workers ; « sans nouvelle table » ne
signifie pas une identité octet pour octet de toute la résidence.

## Preuves locales et premiers résultats

Le [paquet de preuves](../receipts/post_exchange_seed_20260911/README.md)
conserve 106 commandes et leurs sources. Le protocole privé a d'abord payé
la MEB suivante pour vérifier chaque hit
contre sa clé, son niveau et sa BallId, puis a comparé le raccourci à ce bras
d'observation et au bras sans lookup. Les nœuds, parents, contributions,
banques et verticales sont comparés physiquement, pas seulement par digest.

La fixture à cinq points ABEZW exerce un vrai hit. Le carré avec quatre
points intérieurs conserve le rayon après échange, mais sa facette K4 n'est
pas sa population complète de huit points : il doit manquer le lookup.
Les portes propres au patch passent O2 et ASan/UBSan/LSan ROOT, dans les
quatre modes cache, sans cache, statique un et quatre threads. Les portes
produit statiques passent chacune 292 528 contrôles, 34 nuages, 150 ordres,
3 916 coupes, 87 230 vérifications verticales et 5 704 comparaisons physiques ;
Q=20 et H=T=4 sur ces fixtures. La porte nominale reste inchangée.

Micro uniforme u16, graine 3, s8, toute la tour K1..10, un thread amont et
un statique. Ces mesures portent sur le prototype d'observation épinglé,
pas sur un benchmark du binaire actif :

| n | MEB avant | MEB après | Supports évités | Hits exacts |
| ---: | ---: | ---: | ---: | ---: |
| 200 | 51 563 | 48 618 | 282 985 | 2 945 |
| 400 | 132 750 | 125 199 | 723 253 | 7 551 |
| 800 | 314 605 | 296 672 | 1 749 123 | 17 933 |
| 1 000 | 406 134 | 382 929 | 2 300 411 | 23 205 |

Environ 5,7 % de MEB et 6,5–6,8 % de supports en moins ; un hit pour environ
5,8 recherches ajoutées. Les buffers des bras de cette micro coïncident.
Leur ordre fixe, la référence retenue en mémoire et l'hôte partagé interdisent
d'en déduire une accélération chronométrique qualifiée.

Deux essais de mutant à clé partielle ont d'abord survécu aux seules sorties
Gamma : une mauvaise BallId pouvait rejoindre la même composante. Ces échecs
du protocole sont conservés ; l'observation MEB/BallId/niveau renforcée tue
ensuite cette faute. Omettre le dernier échange est également réfuté.
Le contrôle d'un hit à rayon égal est testé sur un état synthétique non
géométrique, et n'est jamais présenté comme une contre-fixture du lemme.

Le [build CMake actif](../receipts/post_exchange_active_cmake_20260911/README.md)
passe les 24 CTests pertinents en Release strict, pas toute la suite du dépôt.
Le probe n200 retrouve tous les champs non temporels du propre O2 ; ses
sources, ses dépendances utilisées et son binaire sont épinglés avant/après.
Le [nouveau raccord réel census→K10](../receipts/full_t2_post_exchange_20260911/README.md)
passe aussi O2/SAN sur le header actif, avec 120 hits et 228 recherches :
sa capture est distincte de l'ancien T2. Elle conserve 54 tours par build,
13 000 coupes et 8 103 948 vérifications verticales, avec 18 paires de travail
statique un/quatre threads. Les hits proviennent de spatial12 ; la coquille14
exerce les recherches qui doivent manquer, et la ligne12 seulement les semis
initiaux. Les quatre mutants T2 ne remplacent pas les trois mutations propres
au raccourci, conservées dans son paquet.
## Triplet mono-thread actif : 8k, 16k, 32k

Le [nouveau triplet](../receipts/post_exchange_scale_20260911/README.md) est
clos sur le binaire Release actif : trois processus indépendants, uniforme
u16 graine 3, s8, toute la tour K1..10 et ses verticales retenues, un thread
amont et un statique. Aucun autre compilateur ou benchmark du chantier
pendant ce créneau ; les charges extérieures de l'hôte ne sont pas contrôlées.

| n | MEB après | MEB évitées | Supports testés après | Total pipeline (s) | FULL seul (s) | Pic RSS (Gio) |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 8 000 | 3 947 627 | 237 557 | 340 615 272 | 141,366 | 58,977 | 2,039 |
| 16 000 | 8 278 207 | 501 258 | 717 184 296 | 318,968 | 134,329 | 4,223 |
| 32 000 | 17 199 233 | 1 045 620 | 1 494 426 349 | 694,459 | 292,341 | 8,687 |

La réduction supplémentaire du travail est de 5,68/5,71/5,73 % pour les MEB
et 6,58/6,60/6,64 % pour les supports. Les 35 champs et digests de sortie,
ainsi que R/U/S par K, coïncident avec les captures statiques antérieures.
À ces tailles, ce n'est pas une comparaison physique de chaque objet de
sortie : les qualifications bornées ci-dessus fournissent ce contrôle séparé.
Les comptes Q/H/T vérifient le travail réellement supprimé, sans faux hit.

En doublant n, le nombre de MEB est multiplié par 2,097 puis 2,078 ; les
supports par 2,106 puis 2,084. Ces exposants locaux proches de 1,05–1,07
ne concernent que cette famille uniforme. Le pic RSS mesure le processus
entier et ne démontre pas un gain mémoire. Les durées sont une observation
par taille, sans répétitions : les références anciennes utilisaient quatre
threads statiques sur hôte partagé, donc aucune accélération chronométrique
causale n'est déduite de leur comparaison avec ces nouveaux temps.

s10/s12 n'ont pas été remesurés à ces tailles sur ce nouveau header ; leur
comparaison antérieure reste attribuée à ses sources. Aucun temps 50k
nouveau, aucun terminal GPU actif :
les contrats de toute la tour sous 1 s, puis 100 ms, et le régime de plusieurs
dizaines de millions de points ne sont pas acquis. GCP non utilisé ici.
