# Arène collective et encodage FULL linéaire : résultats et décisions

27 septembre 2026, suite de `b9fcc3d63`. Cadre :
`phase=exploration_v9_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u18_input_only`, `mode=collective_and_first_parent_prototypes`,
`public_status=not_claimed`. Prototypes séparés du moteur ; GCP non utilisé.

## Ce qui est maintenant acquis

Le [prototype collectif q3/q4](../audits/b_q34_collective_arena_20260927/README.md)
remplace les petits vecteurs par **six tableaux pour tout le lot**.
Il prépare les mêmes témoins et crédits, une seule fois, puis compte,
préfixe et écrit les classes et bandes dans des plages disjointes.
Les ancres d'un gros facteur sont réellement partagées entre workers.
Il n'alloue aucune liste des P paires du produit, ni des E paires résiduelles.

Sur la trame 08/000000 sans sol, K5/s8, 39 885 sites :

| Grandeur de préparation | Résultat |
| --- | ---: |
| Rectangles préparés | 103 840 |
| Sites parcourus dans les facteurs, F | 1 820 907 |
| Buffers finaux collectifs | 6 |
| Capacité finale de l'arène | 29,328 Mo |
| Pic des tableaux possédés, un constructeur W4 | 41,661 Mo |
| Préparation W1 / W4, moyennes des deux ordres appariés | 524,64 / 276,66 ms |
| Paires avant / après Pool, fallbacks compris | 23,687 / 9,123 millions |

La capacité finale des [plans directs précédents](../receipts/q34_direct_bands_20260927/README.md)
était 101,118 Mo sur cette même entrée : **−71,0 %**, à crédits et résidu
égaux. Cette comparaison de stockage ne prouve pas un gain de temps entre
captures. Les chronos W1/W4 sont appariés dans la nouvelle sonde ; l'hôte
est partagé et une campagne FULL a tourné concurremment. Aucun facteur
d'accélération stable ni temps GPU n'en est déduit.

Le pic annoncé est celui des tableaux d'un constructeur isolé, pas le RSS
du benchmark : les deux arènes comparées y coexistent. Index/nuage
(11,579 Mo), requêtes de l'appelant (4,194 Mo), front, piles, allocateur et
infrastructure des threads sont distincts. Les 1 024 326 rectangles
fallbacks ne sont pas stockés dans ces six tableaux : leur consommation
reste à raccorder et à compter.

La [capture r2](../receipts/q34_collective_arena_20260927/README.md) ferme
23 commandes et dix mesures : Release, ASan/UBSan/LSan, deux mutants
par build, 96 lots et 17 488 rectangles de gate. Les seize compteurs
géométriques sont identiques aux plans directs, ainsi qu'entre W1 et W4.
L'échec LSan sous ptrace de r1 est conservé. Une contrelecture a trouvé
un trou de couverture dans un test de refus d'ordinal, pas une erreur de
géométrie : son premier rectangle était déjà invalide. Le complément
séparé part de rectangles valides et contrôle les motifs exacts : neuf
commandes Release/sanitizers closes, 78 refus de construction et 18 de
requêtes, 18 contrôles d'alias. Ne pas transformer les 72 refus initiaux
en 72 branches indépendantes. La
[contrelecture ciblée](../audits/b_q34_collective_arena_review_20260927/README.md)
et le [contrat du consommateur](../audits/b_q34_arena_review_20260927/README.md)
restent séparés de ces résultats exécutés.

## Ce que les mesures de croissance disent réellement

Uniforme, terrain et amas sont mesurés à 8k/16k/32k. Sur amas, F et les
tests des coins doublent presque exactement à chaque doublement de n ;
les tableaux finaux font 0,323 / 0,632 / 1,250 Mo. Mais E reste
2,091 / 7,788 / 30,699 millions : **×3,724 puis ×3,942**, inchangé.
La préparation n'a donc pas simplement déplacé un carré dans ses propres
histogrammes ; elle n'a pas non plus supprimé le résidu quasi quadratique.

Sur uniforme et terrain, très peu de facteurs justifient un Pool ; le
temps de lancement des workers domine et W4 régresse. Ce résultat doit
guider le choix du calendrier, sans tronquer le travail. Aucun nouveau
test spatial de croissance LiDAR, K10 ou s10/12 n'est exécuté par cette
capture d'arène ; les tests des prototypes antérieurs restent distincts.

## FULL : supprimer le tri est exact, mais pas un gain CPU suffisant

Le [nouvel encodeur](../audits/b_full_first_parent_20260927/README.md)
vérifie globalement l'absence d'actions à un seul parent. Dans ce cas,
chaque action crée un nœud ; son ID et ses offsets parents sont déjà
connus. Une table conserve la première occurrence de chaque parent.
Un parent ne peut être consommé par deux fusions : ce contrôle remplace
le tri des incidences sans retirer la validation des parents vivants.
Les continuations restent légales et déclenchent le repli général.

Sur la voie sans continuations, le travail devient O(B+A+P+C), avec
16A octets de scratch sur ce build ; le repli garde son tri général.
Sur LiDAR00, la somme des scratchs demandés par ordre passe ainsi
de 98,672 à 24,668 Mo ; ce n'est pas un pic RSS de la tour.
Les niveaux exacts, nœuds, parents, successeurs, contributions et premier
motif de refus sont conservés. Qualification : 19 294 comparaisons par
build, 805 entrées acceptées, 8 842 refusées, quatre mutants spécialisés,
Release/ASan/UBSan/LSan et lecteurs normal/−O. La
[contrelecture](../audits/b_full_first_parent_review_20260927/README.md)
confirme le lemme et les priorités de refus ; ce code reste scalaire.

La [nouvelle mesure sur vrais drafts](../receipts/full_first_real_drafts_20260927/README.md)
conserve les objets exacts, mais ne montre pas de gain LiDAR convaincant :

| Entrée K1..5 | Natif / first-parent, ms |
| --- | ---: |
| LiDAR00 sans sol | 168,12 / 171,08 |
| Uniforme 8k | 63,78 / 62,38 |
| Uniforme 16k | 133,11 / 131,79 |
| Uniforme 32k | 308,25 / 282,47 |

Ce sont des sommes de médianes par ordre de trois réencodages, pas des
murs de tour parallèle. Les allocations et sorties sont payées ; les
comparaisons et destructions sont hors intervalle d'encodage. La chaîne
instrumentée paie aussi les copies de drafts. Ne pas comparer directement
ces chronos à l'ancienne campagne de l'encodeur trié sous une autre charge.
Les quatre entrées vérifient ici l'absence de continuations ; ce n'est pas
une propriété démontrée de tous les LiDAR.

## Décisions pour le développement

1. **Publier ces prototypes et leurs limites**, sans les activer dans le
   moteur sur la seule base de leur exactitude structurelle.
2. **Garder l'arène collective** comme base du raccord parallèle q3/q4.
   Le consommateur doit fonctionner par vagues complètes à mémoire bornée,
   et ne pas reconstruire les anciens tableaux de taille P ou E.
   Comparer bandes et listes ordonnées sur le coût préparation+S2+ordre.
3. **Garder la voie FULL linéaire pour son port parallèle**, pas comme
   une optimisation CPU acquise. Réduction minimum des occurrences,
   réduction du premier défaut, puis écritures disjointes ; ni le premier
   worker arrivé ni une simple suppression des contrôles ne conviennent.
4. Avant G4, payer aussi le mapping rang→ID original du Pool, distinguer
   P logique/E réellement testé, et préserver l'ordre avant S3. Les
   verticales et la construction du draft FULL ne disparaissent pas.

Le dernier résultat G4 demeure la référence publiée autour de 923 ms
K5 sur 00 sans sol. Ces nouveaux reçus ne rapprochent pas encore le chrono
de production de 100 ms : ils valident et sélectionnent les structures du
prochain port. Aucun nouveau contrat FULL/G4 ni sous-quadratique global.
