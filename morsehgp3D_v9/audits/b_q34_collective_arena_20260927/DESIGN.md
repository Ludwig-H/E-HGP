# Arène collective q34 — interface proposée puis prototype CPU

27 septembre 2026, base `4badf8b7d`. Hors registre, CPU de référence,
grille entière 1 mm, `not_claimed`. Aucun moteur ni GCP modifié.
Le contrat consommateur est contre-audité séparément dans
[la note de raccord](../b_q34_arena_review_20260927/README.md).

## Ce qui doit disparaître

Les plans directs précédents gardent huit vecteurs de facteurs et un
vecteur de bandes par rectangle. La trame 08/000000 K5 en conserve
934 560 buffers sur ses plans préparés. Une simple collection de ces
objets, même parallèle, ne constitue pas une arène collective.

Le nouvel objet doit avoir **six buffers finaux pour tout le lot** :
métadonnées rectangles, métadonnées facteurs, rangs groupés, crédits groupés,
classes et bandes. Les coordonnées et l'index spatial restent partagés.
Le lot peut être un front complet ou un segment explicitement déclaré ;
le segment n'est pas une qualification de trame entière.

| objet | champs essentiels | octets proposés |
| --- | --- | ---: |
| facteur | origine spatiale u64, offset rangs/crédits u64, offset classes u64, taille u32, nombre classes u16 | 32 |
| rectangle | ordinal producteur u64, offset bandes u64, nombre bandes u32, masque u8 | 24 |
| classe | début/fin locaux u32, crédits packed u8 | 12 |
| bande | classe A locale u32, début/fin B locaux u32 | 12 |
| rang groupé | rang **local au facteur**, pas ID original | 4 |
| crédit groupé | q3 dans quatre bits, q4 dans quatre bits | 1 |

Les offsets collectifs sont u64. Les valeurs locales sont vérifiées avant
conversion u32 : aucun écrêtage de données. Un éventuel domaine dépassant
cette représentation demande une représentation plus large, pas un
sous-échantillonnage. Les IDs originaux des pools temporaires restent u64.
L'ordinal producteur distingue les rectangles qui ne sont pas préparés.
Sa croissance stricte contrôle l'identité et l'ordre des requêtes ; elle
ne prouve ni la disjonction entre rectangles ni la couverture d'une WSPD.
Ces propriétés restent à la charge du producteur certifié. Un même couple
de facteurs présenté sous deux ordinaux différents serait traité deux fois.
Le départage de projection utilise l'**ID original**, pas le rang spatial.
Le CPU lit la permutation du même index. Un futur port GPU devra transférer
ce mapping global une fois (4n octets si son domaine u32 est validé, sinon
représentation plus large) : l'entrée GPU courante ne le livre pas déjà.
Une proposition pourra stocker son rang après sélection, mais le tri doit
avoir conservé l'ID original et la correspondance bijective.

## Phases et propriété des écritures

1. Copier les requêtes, retenir le même index immuable. Valider leurs
   nœuds, facteurs disjoints, masques et ordinaux croissants. Tailles puis
   préfixes vérifiés donnent F et les plages de pools.
2. Sélectionner les mêmes TopK par facteur, dans un buffer temporaire
   collectif. Le tri total projection décroissante/ID original croissant
   est inchangé. Aucun vecteur privé par rectangle.
3. Découper les plages d'ancres en tâches grainées. Chaque tâche lit le
   pool figé et écrit un intervalle exclusif de crédits originaux. Chaque
   test géométrique est exécuté **une seule fois**. Le grain est un choix
   d'ordonnancement, jamais un quota de recherche.
4. Compter les classes occupées avec des histogrammes privés aux workers,
   pas une table de 100 cases pour chaque facteur. Préfixer les nombres
   de classes pour allouer leur unique tableau.
5. Relire les crédits, produire les classes dans l'ordre lexicographique
   puis disperser stablement les rangs/crédits groupés dans les plages
   préallouées. Cette relecture est un travail linéaire **payé**, pas un
   nouveau census ni une seconde préparation géométrique.
6. Compter bandes et masses à partir des seules classes. Préfixer les
   comptes, allouer une fois, puis rejouer le petit regroupement pour
   écrire les bandes. Comparer les comptes avant/après ; jamais fabriquer
   d'anciennes cellules pour connaître ces tailles.
7. Joindre tous les workers, clore les tailles, puis publier un propriétaire
   const. Toute exception interrompt la publication et joint les threads.

Le constructeur ne reçoit aucun vecteur de sortie externe et ne publie
aucun pointeur mutable. Le déplacement d'un vecteur fourni par l'appelant
ne sert **pas** de preuve d'immuabilité. Les vues const conservent leur
propriétaire pendant toutes les tâches et toutes les vagues consommatrices.
Copie/déplacement du propriétaire désactivés dans le premier prototype.

La preuve des écritures disjointes est une preuve de plages : les
préfixes de tailles fixent un intervalle par facteur ; les sous-tâches
d'ancres le partitionnent ; le comptage par classe puis les curseurs
stables partitionnent chaque facteur groupé ; le préfixe de bandes donne
un intervalle par rectangle. Aucun append concurrent sur `std::vector`,
aucun pointeur traversant une réallocation.

## Parallélisme réel et limites du premier port CPU

Les tâches d'ancres sont partagées même à l'intérieur d'un gros facteur.
La sélection TopK et les balayages de regroupement restent initialement
une tâche par facteur : ils sont parallèles entre facteurs, mais ce premier
port n'en qualifie pas une réduction multi-CTA GPU. Publier le facteur
maximum, les tâches par étage et le travail 1/4 workers ; ne pas appeler
chaque étape massivement parallélisée sur la seule foi des tableaux.

Extension compatible, pas encore revendiquée : TopK de tuiles puis fusion
segmentée associative sous le même ordre total. Pour les seuls gros
facteurs, histogrammes par tuiles puis préfixes par classe permettent un
scatter stable parallèle ; leur mémoire temporaire doit être mesurée.
Il n'est pas nécessaire de stocker 100 cases pour **tous** les facteurs,
notamment les nombreux facteurs minuscules.

## Travail, mémoire et consommation

La sélection et les prédicats restent O(KF), avec leurs compteurs propres.
Le regroupement ajoute O(F + 100R + C_B + C_A K log K + D), avec K≤10
dans cette interface : ne pas présenter le nombre de classes C_A≤K²
comme une preuve que toutes les boucles sur les lignes coûteraient O(K²).
Les relectures de crédits, classes et bandes sont comptées séparément.
Les crédits d'origine sont lus trois fois : compter les classes,
recompter pour les offsets, puis disperser ; `grouping_reads=3F`.
Compter deux fois le
petit regroupement pour obtenir puis remplir les préfixes ne signifie pas
refaire les prédicats ; le lecteur doit le vérifier.

Les compteurs de travail et préfixes u64 utilisent les additions contrôlées
historiques. Un dépassement est une erreur explicite, jamais un retour de
résultat tronqué ni une limite de recherche. Les histogrammes locaux u32
sont bornés par la taille u32 déjà contrôlée de leur facteur.

Mémoire finale nominale : `24R + 64R + 5F + 12C + 12D`, plus en-tête et
capacités réelles. Scratch explicite : copie requêtes, pools, offsets,
crédits originaux avant dispersion, tâches d'ancres, registres privés et
histogrammes des workers. Le pic doit inclure les phases où crédits
originaux et crédits groupés coexistent ; ne pas sommer seulement les
buffers finaux. Les pools O(K·2R) sont libérés avant les bandes finales.
Index et nuage sont déclarés à part. Les pics publiés portent uniquement
sur les capacités des tableaux : ni RSS, ni piles et allocations internes
des threads, ni surcoût de l'allocateur. La copie de requêtes interne est
incluse ; le tableau de requêtes de l'appelant est déclaré séparément.

Cette arène ne doit pas être suivie d'un tableau global de P ni même de E
masques de paires : utiliser des vagues de tuiles à mémoire bornée, traitées
jusqu'à épuisement, avec les rectangles fallback conservés. La capacité
d'une vague ne limite jamais le domaine exploré. Conserver l'ordinal
cartésien original pour trier les seules survivantes S avant S3 ; les coûts
de clés/tri/scatter font partie du raccord, pas de cette préparation.
L'alternative de listes B déjà ordonnées par classe A est examinée par
l'auditeur principal ; elle ne fait pas partie de ce premier objet.

## Portes avant toute promotion

Comparer à `direct.hpp` pour chaque facteur : propositions, crédits,
classes, permutation stable ; comparer toutes les bandes, masses et masques
sur petits produits et toutes les cellules de crédits sur grandes mesures.
Les tests géométriques doivent être identiques ; les compteurs de
regroupement, volontairement changés, restent distincts. Comparer 1/4
workers, plusieurs grains, rejets et exceptions de workers. Tester un lot
vide, des facteurs singleton et des masques q3 seul/q4 seul/mixte ; inclure
des mutations de scatter et de masque. Un rectangle non vide entièrement
rejeté n'est pas une fixture positive de ce Pool local sans cœur externe :
sa paire croisée de distance minimale survit, car un témoin local certifié
strict engendrerait une paire croisée plus courte. Le lot vide teste donc
la représentation vide, sans revendiquer cette branche inaccessible.
Release et ASan/UBSan/LSan sont requis ; sans porte
TSan, ne pas déclarer une qualification par détecteur de courses.

Aucun résultat de tour, GPU, contrat 100 ms ni nouvelle borne globale
sous-quadratique ne découle de ce format.
