# Arène collective q34 : six buffers, géométrie inchangée

27 septembre 2026, base `4badf8b7d`. Prototype CPU isolé, grille entière
1 mm, hors registre, `not_claimed`. **Ni raccord moteur, ni S2/S3, ni FULL,
ni GPU/GCP dans cette tranche.**

Le résultat utile est structurel : les facteurs et bandes de tous les
rectangles sont désormais rangés dans six tableaux collectifs, avec
préfixes vérifiés et plages d'écriture privées. On ne construit plus un
propriétaire à neuf buffers par rectangle. Les témoins et les crédits
géométriques sont exactement ceux du prototype direct précédent ; aucun
ancien tableau de cellules n'est fabriqué, même temporairement.

Lire [le design et sa preuve de plages](DESIGN.md), puis
[le contrat consommateur contre-audité](../b_q34_arena_review_20260927/README.md).
Les fichiers `arena.hpp` et `probe.cpp` constituent un prototype, pas un
nouveau chemin produit. Les sources de la capture r2 sont gelées.

## Résultat LiDAR mesuré

Trame **08/000000 sans sol, entière après masque figé**, 39 885 sites,
K5/s8, grille 1 mm. La segmentation n'est pas remesurée ici. Le tableau
ci-dessous concerne uniquement les 103 840 rectangles préparés ; les
1 024 326 rectangles fallback sont comptés, mais leurs descripteurs et
leur filtrage aval ne sont pas matérialisés par cette sonde.

| grandeur | résultat |
| --- | ---: |
| masse virtuelle P après filtre rectangle | 23 686 751 |
| résidu total E, fallback inclus | 9 122 704, inchangé |
| masse fallback incluse dans E | 1 492 845 |
| F = somme des tailles des facteurs préparés | 1 820 907 |
| classes / bandes | 592 028 / 331 733 |
| buffers finaux de l'arène | 6 |
| stockage final, en-tête et capacités inclus | 29 328 091 octets |
| pic des tableaux possédés, un / quatre workers | 41 659 906 / 41 660 866 octets |
| tableau des requêtes de l'appelant, séparé | 4 194 304 octets |
| propriétaire du nuage et index partagé, séparés | 11 579 408 octets |
| préparation un worker, AB / BA | 541,525 / 507,762 ms |
| préparation quatre workers, AB / BA | 277,111 / 276,215 ms |
| moyennes un / quatre workers | 524,643 / 276,663 ms, soit ×1,896 |

Le prototype direct précédent conservait 934 560 buffers et 101 118 254
octets de plans pour les mêmes rectangles. C'est une comparaison de
**représentations** épinglées, pas un chronométrage apparié ancien/nouveau
dans ce nouveau binaire. Ne pas soustraire leurs chronos de captures
différentes pour attribuer un gain causal à l'arène.

Les pics publiés ne sont **pas** le pic RSS du processus : ils comptent
les capacités des tableaux d'un constructeur isolé, hors piles/runtime
threads et allocateur. Le benchmark conserve simultanément ses arènes
un et quatre workers pour les comparer ; pendant la validation il ajoute
des plans directs temporaires. Les requêtes appelantes et l'index sont
séparés ci-dessus. Le futur consommateur et ses sorties ne sont pas inclus.

Les chronos comprennent copie privée des requêtes, validation, allocations,
préfixes, création/jointure des threads et construction complète. Ils
excluent la destruction des arènes. Comparaison et juge direct sont
chronométrés séparément ; les destructions n'ont pas de champ séparé.
Les arènes de la première paire sont détruites avant la deuxième et ce
coût entre dans `total`. Les dernières arènes `last_one`/`last_four` sont
détruites après l'impression de `total` : leurs destructions finales, comme
le nettoyage final de l'index et des entrées, sont **hors ce total**.
Deux paires alternées AB/BA, machine locale partagée,
sans affinité CPU dédiée : ce n'est ni une campagne statistique robuste ni
une promesse de gain sur 48 CPU. Le front et filtre rectangle CPU de cette
sonde prennent séparément 17 333,274 ms ; le total expérimental, avec quatre
constructions et les juges, vaut 19 566,600 ms. Rien ici ne vaut un temps de
tour ou un contrat 100 ms.

## Mesures 8k / 16k / 32k

Même binaire, K5/s8, recettes synthétiques épinglées, moyennes des deux
constructions de chaque configuration. L'identité complète des tableaux,
masses et compteurs géométriques est contrôlée entre un et quatre workers.

| régime | n | F préparé | E, fallback inclus | un worker (ms) | quatre workers (ms) |
| --- | ---: | ---: | ---: | ---: | ---: |
| uniforme | 8 000 | 784 | 435 709 | 0,231 | 0,949 |
| uniforme | 16 000 | 2 065 | 908 050 | 0,586 | 0,862 |
| uniforme | 32 000 | 3 970 | 1 876 820 | 1,821 | 6,552 |
| terrain | 8 000 | 1 127 | 140 079 | 0,535 | 1,096 |
| terrain | 16 000 | 1 742 | 285 967 | 0,519 | 1,040 |
| terrain | 32 000 | 2 273 | 591 278 | 0,671 | 1,236 |
| amas | 8 000 | 56 591 | 2 091 410 | 21,013 | 7,774 |
| amas | 16 000 | 113 366 | 7 787 691 | 42,171 | 17,190 |
| amas | 32 000 | 227 083 | 30 699 080 | 84,318 | 22,422 |

Le multiworker **régresse** pour les petits lots uniforme/terrain : ils
préparent peu de facteurs, et le coût de lancement/ordonnancement domine.
Une sélection future mono/multi selon le travail estimé serait une règle
d'ordonnancement, pas un quota de recherche ; elle n'est pas implémentée.
L'uniforme32k a en outre des chronos fortement chargés : publier la valeur,
ne pas en faire une preuve de complexité.

Le travail de préparation des amas double presque exactement : ratios F
×2,003/×2,003, tests de coins ×2,003/×2,002. Mais **leur résidu E reste
presque quadratique**, ×3,724/×3,942. Les ratios E uniforme sont
×2,084/×2,067 et terrain ×2,042/×2,068. Ce sont des diagnostics empiriques
de ces recettes, pas une preuve générale de pipeline sous-quadratique.
Une seule trame LiDAR est mesurée ici : aucune nouvelle croissance LiDAR,
ni qualification de plusieurs scènes, de K10 ou s10/s12 n'en découle.

## Exactitude, comptage et parallélisme

Les mêmes 16 compteurs de sélection/géométrie que `direct.hpp` sont
comparés par rectangle et agrégés. Les propositions sont départagées par
ID original, jamais par rang spatial. Les crédits sont calculés une seule
fois ; le regroupement relit ensuite leur tableau trois fois, explicitement
`grouping_reads=3F`. Les bandes sont comptées puis émises en deux passages
sur les classes. Les additions des compteurs et préfixes restent contrôlées.

Les tâches d'ancres découpent aussi les gros facteurs ; grain128 pour les
mesures, grains7/1 dans le gate. En LiDAR : 208 674 tâches d'ancres,
facteur maximum942. Sélection TopK et regroupement sont encore une tâche
par facteur ; une réduction GPU intrafacteur n'est pas implémentée. Le
travail public et les sorties sont identiques entre un et quatre workers.
Les threads sont joints avant publication ou propagation d'une exception.
Les histogrammes100 cases sont privés aux workers, jamais multipliés par
tous les facteurs. Les pools temporaires O(K·2R) sont libérés avant les
bandes finales. Pas de buffer caché de P ou E paires dans le constructeur.

Les ordinals strictement croissants identifient les requêtes : ils ne
certifient pas à eux seuls une WSPD ni la disjonction inter-rectangles.
Le type u32 est contrôlé uniquement pour les tailles et offsets locaux ;
les offsets collectifs sont u64. Dépassement de domaine = erreur explicite,
pas troncature, sous-échantillonnage ou plafonnement de recherche.

## Portes et reçus

[Capture r2](../../receipts/q34_collective_arena_20260927/r2/summary.json) :
23 commandes, dix mesures. Release et Clang ASan/UBSan/LSan donnent le même
gate : 96 lots, 17 488 rectangles, 34 976 facteurs, 38 536 rangs,
36 406 classes, 17 968 bandes, 24 940 paires et 37 612 IDs de propositions.
Deux mutants dans chaque build sont tués causalement : scatter de rangs
incorrect et masque6 substitué au masque exact. Huit exceptions injectées
dans des workers sont propagées après jointure. Pas de gate TSan : ne pas
confondre preuve de plages et qualification par détecteur de courses.

Les 72 refus du gate initial ne couvraient pas 72 branches indépendantes :
le test d'ordinal dupliqué était masqué par un premier rectangle déjà
invalide. Ce défaut du **test**, trouvé indépendamment en contre-audit,
n'est pas effacé ni corrigé dans les sources gelées. Le complément autonome
dans `supplement/` part de rectangles valides et vérifie les motifs exacts,
ainsi que des permutations non triviales et des domaines de requête.
Son statut est publié séparément dans le reçu de complément.

[Ce complément](../../receipts/q34_collective_arena_20260927/supplement/summary.json)
est clos : neuf commandes, Release et Clang ASan/UBSan/LSan, lectures LIVE
normale et `-O`. Il vérifie 78 refus avec motif exact depuis une requête
initialement valide, 18 refus de domaines de query et 18 contrôles d'alias.
Ses 18 plans positifs couvrent les masques2/4/6 et produisent 622 rangs
réellement déplacés, comparés au prototype direct ; 763 IDs diffèrent des
rangs spatiaux. Ordinaux dupliqués/décroissants, K2/mask4 et K2/mask6 sont
refusés causalement ; ordinaux croissants et K2/mask2 sont exercés
positivement. Ce complément ne remplace pas les gates géométriques de r2.

La première capture reste en échec conservé : LeakSanitizer ne peut pas
fonctionner sous ptrace. R2 emploie les mêmes sources et de nouveaux builds
hors ce contexte, avec `detect_leaks=1`, sans écraser le premier essai.
Sources, bibliothèques immuables réutilisées, commandes, sorties et binaires
sont hachés. Les reçus sont LIVE : ils requièrent ces builds et entrées
locales, et ne deviennent pas une archive autonome par simple copie.
Les lectures r2 normale et `-O` passent indépendamment du complément.
Le [contrôle du lecteur](../../receipts/q34_collective_arena_20260927/r2/checks/checks.json)
enregistre aussi huit corruptions causales rejetées : commande manquante,
arguments/code retour faux, groupe ouvert, source ou entrée modifiée,
fausse déclaration GPU et capture non close. Aucune assertion Python
désactivable par `-O` ne porte ces verdicts.

## Suite recommandée

Conserver la propriété collective, les préfixes et le découpage du travail.
Comparer avant port la voie bandes + restauration d'ordre des seules
survivantes avec la voie listes B stables par classe A étudiée séparément.
Le consommateur doit balayer jusqu'à épuisement en vagues à mémoire bornée,
sans tableau global de masques P ou E, et payer le fallback, les sorties
et l'ordre nécessaire à S3. Il faut ensuite une gate S2 exacte
endpoints+masques, le raccord FULL et des mesures G4. L'arène rend ce
raccord plus praticable ; elle ne règle pas seule le résidu quadratique,
la segmentation, la tour explicite ni le contrat 100 ms.
