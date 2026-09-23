# FULL v9 — géométrie préparatoire indépendante des fermetures par K

23 septembre 2026. Lecture du moteur `5ab4326c` et du [premier reçu G4](../receipts/g4_tower_r1_20260922/README.md), produit avec le paquet `e28296bb`. Profil mesuré : trame SemanticKITTI 08/000000 entière **sans sol**, grille optionnelle 1 mm, u18, CPU de la VM G4 ; ni GPU, ni float32 brut, ni contrat acquis. Cette note précise l'unité de travail à essayer après le [contre-audit B](CONTRE_AUDIT_B_PARALLELISME_Q34_FULL_20260922.md), qui proposait déjà des fenêtres à état pré-niveau figé.

## Unité indépendante exacte

Pour un ordre `K`, une facette de `K` sites triés et le plus petit niveau `before` de ses consommateurs, la résolution **géométrique statique** produit un `BallId` du catalogue. `static_terminal` lit seulement l'index, les `BallData`, `by_key` et les semis `seeds_K` tirés des populations complètes `I∪U` ; son travail/statistiques et sa pile de recherche sont privés ([source](../src/tower/forest/full_ball_tower.hpp#L577-L632)). `visit_block` et `ShellTable::rank` extraient les facettes depuis le catalogue immuable ([source](../src/tower/forest/full_ball_tower.hpp#L924-L967)). Les programmes et l'index par clé sont fixés avant la boucle des ordres ([source](../src/tower/forest/full_ball_tower.hpp#L518-L529)). Aucun de ces calculs ne lit les ancres ou le DSU de la forêt.

`before` demeure une **validation obligatoire** : la MEB initiale, chaque MEB de la chaîne et le semis final doivent être strictement antérieurs au consommateur. La chaîne a des rayons non croissants ; sur une résolution valide, ses embranchements et son `BallId` ne dépendent pas de la valeur précise de `before`. Le tri par clé conserve la première occurrence de la facette dans le programme chronologique, donc son niveau est le minimum des consommateurs ([source](../src/tower/forest/full_ball_tower.hpp#L755-L790)). Valider ce minimum couvre les occurrences ultérieures, sans supprimer leurs contrôles de chronologie. Les semis sont propres à `K` ; ils ne se transportent jamais d'un ordre à l'autre.

Ainsi, les jobs `(K, facette, niveau_min)` peuvent être calculés dans n'importe quel ordre, **y compris pendant la fermeture d'un autre K**, puis rendre seulement un `BallId`. La fermeture reste chronologique par K et par lot de niveau égal : elle traduit ce `BallId` en `anchors[target]`, normalise la racine et publie les ancres seulement après le lot entier ([source](../src/tower/forest/full_ball_tower.hpp#L860-L867), [fermeture](../src/tower/forest/full_ball_tower.hpp#L990-L1080)). Un job préparatoire ne publie donc jamais une racine ni une ancre. L'extraction concurrente exige de sortir `current_k` mutable du `Builder` vers un contexte de job immuable ; partager l'objet actuel entre plusieurs K serait une course. Les échecs doivent conserver le résultat transactionnel : aucune tour partielle publiée, tous les workers rejoints et leur travail payé compté.

## Ce que le G4 permet de dire

Sur 08/000000/K10, W48, l'option statique existante réduit le temps de tour de **75,90 à 34,14 s** et le total de chaîne de **111,68 à 70,00 s**, à digest identique ; une répétition par cas. Elle résout déjà les clés distinctes d'un ordre en parallèle. Le reçu publie **17 389 031 représentants** sur toute la tour et **7 426 215 nœuds FULL** sur les dix ordres, dont **5 884 465** aux seuls K6..10. Il ne sépare pas, par ordre, extraction, tri/dédoublonnage, géométrie, second `visit_block`, fermeture des lots et encodage ; les 34,14 s ne sont donc pas un potentiel de gain attribuable au préchargement.

Le préchargement naïf de tous les ordres accroîtrait la résidence : à ce volume, conserver simultanément une requête de 56 octets et une cible `u32` par représentant représente déjà environ **1,04 Go** de capacités logiques, hors semis, groupes, catalogue et surcapacités. Ce produit ne doit pas être extrapolé aux dizaines de millions de sites. Une expérience sûre borne strictement en octets une fenêtre de facettes et son lookahead entre K, garde index/catalogue en lecture seule, et exerce une pression de retour quand les cibles attendent la fermeture. Si le découpage sépare deux occurrences d'une même facette, recalculer reste exact mais peut augmenter le travail ; mesurer `représentants / clés uniques` et les répétitions entre fenêtres. Découper **à l'intérieur d'un lot de niveau égal** ne permet jamais d'en publier les ancres avant que tous ses blocs soient résolus.

## Porte expérimentale

Ajouter au reçu, pour chaque K, temps mur/CPU d'extraction et quotient, tri/unique/semis, résolution géométrique, scatter des `BallId`, second `visit_block`, fermeture/DSU ; mesurer l'encodage final séparément. Publier nombre de facettes, clés uniques, semis, chaînes et visites d'intrus, nombre de lots et taille du plus gros lot, attentes des workers, capacités simultanées et pic RSS par phase. Comparer ensuite W48 statique actuel à une seule fenêtre bornée, puis à un lookahead limité, avec **mêmes** entrée/catalogue/options, digest et comptes de chaque ordre, travail géométrique et trois répétitions. Un gain n'est acquis que sur le temps de chaîne et les octets vivants mesurés, pas par la seule augmentation du nombre de jobs.

Un raccourci par `BallData::interior()` pour les recherches d'intrus de clé cataloguée est possible seulement aux ordres inférieurs : à `K=Kmax=10`, tout enregistrement du catalogue satisfait `p+q_min-1≤10`, tandis qu'une facette de sa MEB ne peut avoir `K>p+u`. Une clé trouvée est donc déjà admissible ; les recherches d'intrus restantes portent sur des clés **absentes** du catalogue. Ce raccourci ne peut pas expliquer ni éliminer le coût K10 dominant et ne doit pas être vendu comme solution aux 403,4 M visites de nœuds d'intrus cumulées du reçu G4.

## Première révision des ordres K concurrents, relue le 23 septembre

Le premier snapshot du constructeur dans `full_ball_tower.hpp` (SHA-256
**`0552ad3e5ef6…`**, antérieur au port publié) ajoute un chemin
`run_orders_parallel()` pour `static_threads>1`, sans résolveur externe.
La séparation A/B/C est mathématiquement saine à ce stade : chaque K
ferme ses propres lots avec des `BallId` géométriques préparés avant la
construction ; les populations sont ensuite numérotées dans l'ordre K,
lot, action, contribution, puis les images verticales sont calculées à
partir des historiques clos du K inférieur. Le contenu du catalogue,
les programmes et les tables de coquille restent immuables pendant les
phases parallèles ; les ancres, historiques, brouillons et compteurs sont
privés par K. Le traitement de chaque lot de même niveau reste atomique.
Sur une copie exacte de ce snapshot, les trois portes FULL ciblées
`--selftest`, `--static-1`, `--static-4` passent en Clang O2 ; la dernière
compare 38 nuages et 168 ordres au chemin séquentiel, dont 6 256
comparaisons de payload. La porte `--static-4` passe aussi sous TSan et
Clang ASan/UBSan/LSan sur le même snapshot ;
la chaîne locale de 1 500 sites conserve le digest
`73490cf88c02af30` pour `static=0/1/4/8`. Ces essais ne sont ni un reçu
LiDAR 40k, ni une mesure G4 du nouveau port.

Le **ledger d'échec** a un trou précis. `order_lots` et `order_images`
accumulent leur travail dans `OrderState::st`, mais `merge_order_stats`
n'est appelé qu'après la réussite des deux `parallel_items`. Si l'un
échoue, les workers sont joints et la tour partielle n'est pas publiée,
mais le travail déjà payé est perdu des statistiques du résultat. Une
injection locale `std::bad_alloc` juste après la fermeture d'un lot K1
sur une vraie fixture q2 à deux sites rend
`resource_exhausted/full_ball_allocation_failed`, `orders=0`, et
`anchor_blocks=contributions=births=0` dans le résultat, alors que
`OrderState::st` avait déjà incrémenté ces compteurs. C'est une
reproduction **instrumentée sur copie temporaire**, pas un échec naturel
du produit. Fusionner les statistiques locales après jointure aussi en
cas d'exception, sans double fusion ni publication partielle ; conserver
un drapeau « travail connu » si la réduction elle-même ne peut être
attestée. Une porte injectée après une action et une autre durant les
images doivent vérifier ce contrat.

La **résidence simultanée** est aussi mesurable avant toute promesse de
gain. Sur 08/000000/K10 du [reçu R4b](../receipts/g4_tower_r4b_20260923/README.md),
`B=5 512 670` boules et `N=7 426 215` nœuds FULL cumulés. À la fin de
la phase A, les dix `anchors` privés occupent logiquement `4·10·B =
220 506 800` octets ; les historiques (`ExactLevel` 48 octets et `next`
8 octets par nœud) et `birth_ball` (4 octets) occupent `60·N =
445 572 900` octets. Ce sont **666 079 700 octets, 635,2 Mio**, sans
capacités, brouillons, populations, catalogue, banque ni autres buffers.
Le chemin séquentiel ne garde qu'un historique inférieur et l'historique
courant à la fois ; ce chiffre est le plancher logique du **nouveau**
chemin à cette phase, pas une hausse RSS directement mesurée. Préparer
toutes les cibles statiques avant la phase A ajoute temporairement jusqu'à
`4·17 389 031 ≈ 69,6 Mo` de cibles, mais elles sont libérées ordre par
ordre. Mesurer le pic RSS et les capacités coexistantes par phase, puis
envisager une fenêtre de K ou une libération progressive des historiques
si les images verticales coûtent peu. À 48 fils, les phases A/C créent au
plus `min(K,48)` tâches (5 ou 10 ici), chaque ordre déroulant ses lots en
série ; publier leur mur, CPU et charge par K pour distinguer gain et
contention mémoire. Aucun transfert de ces tailles au régime 30 M sans
mesure de `B` et `N` n'est justifié.

### Port publié `684d8fc7` : banque de populations déplacée

Le port publié (`full_ball_tower.hpp` SHA-256
`89f1f96af53d…`, `full_coverage_certificate.hpp` SHA-256
`08033ed0b869…`) conserve la séparation par K et construit les lignes
de populations par IDs de première rencontre, dans des slots distincts.
Elle valide ensuite les lignes en parallèle et **déplace** leurs deux
vecteurs vers la banque immuable, au lieu de recopier toutes les lignes
à la fermeture. C'est une économie de résidence temporaire pertinente
pour les gros catalogues ; aucun pic RSS de cette révision n'est encore
mesuré. Les portes ciblées Release (`full_coverage` : 823 vérifications,
30 refus, 20 allocations ; FULL `--static-4` : 301 980 vérifications),
la chaîne de 1 500 sites et un mini-gate move/copy à 0/1/4 fils passent.
FULL `--static-4` et le mini-gate passent aussi sous Clang
ASan/UBSan/LSan et TSan, sur copies exactes en `/tmp` ; aucun reçu
LiDAR/G4 ne porte sur cette révision.

Le nouveau point d'entrée **public**
`build_full_coverage_populations(domain, vector&&, threads)` n'a pas
encore la même clôture d'échec que l'overload par copie : son
`parallel_ranges` est hors `try`, et le `try` ne capte pas
`std::length_error`. Un lancement de thread échoué par le failpoint
`MHGP9_TESTING/launch_fail_after=0` fait effectivement remonter
`std::system_error` à l'appelant direct, au lieu d'un
`FullCoveragePopulationResult` en `resource_exhausted` ; la chaîne
`build_full_ball_tower` intercepte cette exception plus haut. Encadrer
la validation et l'affectation par la même traduction d'exceptions que
l'overload existant, avec une raison de lancement dédiée et une porte
directe injectée. La promesse du commentaire « première ligne invalide
dans l'ordre » n'est pas observable : l'annulation atomique peut arrêter
avant cette ligne, même si le statut générique reste le bon. Le défaut
de compteurs privés perdus sur exception en phase A/C est **inchangé**
sur cette révision.

### Révision `133c8653` : API réparée, bilan d'échec encore incomplet

`133c8653` encadre maintenant la validation et le lancement de l'overload
public à banque déplacée et traduit `std::system_error`, `bad_alloc` et
`length_error` en statut. La porte produit de lancement injecté et la
contre-épreuve directe passent sur les SHA publiés
`full_ball_tower.hpp` `31184775…` et
`full_coverage_certificate.hpp` `3a2cda98…` : le défaut d'API décrit
ci-dessus est **clos**. Aucun reçu G4/RSS apparié ne porte encore sur le
port FULL concurrent.

Le ledger en cas d'échec reste ouvert : les statistiques privées
`OrderState::st` ne sont fusionnées qu'après réussite de **toutes** les
phases A, B et C. L'injection après un lot K1 laisse encore les compteurs
de travail à zéro. Une fonction de fusion unique, appelée une seule fois
après la jointure même lors des sorties d'exception, préserverait le
travail connu sans publier de tour partielle. Une panne d'allocation des
`drafts` après fusion ne doit pas fusionner une seconde fois.

La révision choisit le plus petit K **à l'intérieur de chaque phase** :
elle arrête après l'échec de A sans lancer C. Une injection A/K3 et C/K2
sur une copie temporaire rend A/K3. C'est une politique déterministe,
mais pas « comme la boucle séquentielle » pour toutes les phases. Si la
priorité globale K est contractuelle, calculer encore C pour les ordres
K<f ayant réussi A avant de rendre l'échec A/K=f ; sinon corriger le
commentaire et la porte pour annoncer une priorité par phase.

La limite `u32` ajoutée à `order_new_node` ne constitue finalement pas
un nouveau verrou indépendant des entrées admises. Pour K≥2, chaque
nouveau nœud correspond à un groupe non vide de blocs ; chaque bloc est
une boule distincte de `program[K]`, qui contient chaque BallId au plus
une fois. Ainsi `N_K≤|program[K]|≤B≤UINT32_MAX`. Pour K1, les `n`
singletons initiaux sont les seules naissances, puis tout nouveau nœud
fusionne au moins deux composantes de ces singletons :
`N_1≤2n−1≤UINT32_MAX−2` car `n≤INT32_MAX`. La garde reste utile contre
les débordements et les incohérences ; c'est surtout le nombre de boules
`B` et la mémoire de leurs programmes, catalogues et forêts qu'il faut
mesurer au contrat massif.

### Clôture publiée `84c74a5e` des deux écarts d'échec

Le produit (`full_ball_tower.hpp` SHA-256 `6d474e92…`) choisit désormais
le premier échec dans l'ordre global K : si A échoue à K=f, il calcule
encore les images C des K<f dont les lots ont réussi, puis relance la
première `Failure` par K. Une fusion unique des `OrderState::st` s'exécute
aussi après la jointure lors d'un échec ; le résultat reste transactionnel.
La porte `order_failure_priority_gate.cpp` (SHA `67e00a7d…`) passe dans un
build Release isolé : **19 contrôles**, dont 12 pannes concurrentes,
comparaison statique 1/4/8 fils et digest complet
`73490cf88c02af30`. Deux mutants compilés, priorité par phase et perte
des compteurs sur échec, sortent avec les causes attendues. Les défauts
de ledger et de priorité des révisions précédentes sont donc clos au
niveau de cette porte ; R5 a été exécuté sur `aae9da0e` **avant** ce
correctif, sans nouveau chrono G4/RSS.

`merge_once()` marque la fusion avant ses additions contrôlées. Un
débordement `u64` de ces additions pourrait masquer l'exception initiale
et laisser un bilan partiel ; aucun cas admis proche du régime LiDAR ne
le démontre, et `parallel_orders+=K` ne peut déborder dans un appel neuf
avec K≤10. C'est une limite de robustesse extrême à garder distincte
des deux défauts causaux corrigés.
