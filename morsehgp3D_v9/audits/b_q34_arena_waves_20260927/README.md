# Consommateur de l'arène q34 par vagues : porte S2 exacte close

27 septembre 2026, base `fd1a2c7ee`. Prototype CPU isolé, grille entière
1 mm, hors registre, `not_claimed`. **Aucun moteur ni GCP modifié ; pas de
raccord FULL ni de contrat temps dans cette tranche.**

L'arène collective précédente avait supprimé les vecteurs par rectangle.
Ce nouveau consommateur évite de recréer ensuite des tableaux de masques
sur toutes les P paires brutes ou toutes les E candidates. Il filtre les
candidates par vagues temporaires, garde seulement les S survivantes puis
les remet dans l'ordre original exigé par la suite native.

[Design et preuve](DESIGN.md),
[qualification](../../receipts/q34_arena_waves_20260927/summary.json),
[contrôles du lecteur](../../receipts/q34_arena_waves_20260927/checks/checks.json).
L'arène de référence `b_q34_collective_arena_20260927/arena.hpp` est
réutilisée explicitement et reste figée. Aucun ancien `Plan` ni tableau de
cellules n'est construit sur le chemin candidat.

## Ce qui est transporté et conservé

`Prepared` copie les rectangles d'entrée avant validation et conserve le
même index immuable. Il exécute le filtre rectangle Affine une fois, puis
prépare le Pool uniquement pour les rectangles éligibles. Les singletons
et les rectangles dont les seuls témoins locaux ne pourraient fermer
aucune voie restent des **fallbacks intégraux**, pas des oublis.

Une liste de segments contient les bandes de l'arène et chaque rectangle
fallback. Son préfixe est en u64 ; sa taille dépend du nombre de bandes et
de rectangles, jamais du nombre E/taille_tuile. Une vague de capacité Q
décode des plages successives, calcule leur masque exact par voie et
appelle le filtre ponctuel Affine natif une seule fois par candidate.
Chaque appel repart de zéro : aucun crédit Pool n'est ajouté au census.

Après compaction, une survivante transporte ses rangs, son masque et
`raw_base[r] + (a_rank−A.first)*|B| + (b_rank−B.first)`.
Le tri des seules S survivantes restaure l'ordre avant la production d'un
`Q34FilterBatch`. Endpoints et masque suivent la même permutation. Ce lot
est comparé au natif, mais n'est pas encore injecté dans le moteur S3/FULL.

La convention des masses est explicite :

| grandeur | signification |
| --- | --- |
| `raw_base` | préfixe de tous les produits d'entrée, **avant** filtre rectangle, y compris les futurs rectangles fermés |
| P/P3/P4 | paires/voies encore ouvertes après filtre rectangle, avant Pool |
| E/E3/E4 | candidates/voies réellement présentées à S2 après Pool, fallback inclus |
| S | paires dont le masque après S2 est non nul |

Le champ natif `expanded_pairs` reste P. Les compteurs physiques exposés
par le curseur valent E, E3 et E4, non P. Les rejets natifs par voie sont
reconstitués par `(Pq−Eq) + rejets_S2_sur_Eq`. Les visites géométriques
ancien/nouveau n'ont pas à être égales ; les sorties et les nombres de
voies rejetées, oui. La forme actuelle du moteur qui assimile encore P aux
requêtes réelles devra être adaptée explicitement lors d'un futur port.

## Mémoire, exhaustion et exceptions

Stockage O(R+F+C+D+Q+S), index et nuage à part. Le scratch par candidate
est contenu dans Q slots ; la sortie grandit seulement avec S. Q est une
capacité mémoire, pas un quota : le curseur parcourt tous les segments,
même avec Q=1. Les tests emploient aussi Q>E sur de petits cas ; ce choix
de test ne transforme pas E en taille de buffer globale imposée.

`Cursor::array_bytes_peak` compte les capacités des tableaux du curseur :
slots, keyedS, ancien+nouveau keyedS simultanés pendant une réallocation,
puis keyedS+sortie nativeS simultanés lors de la conversion. Ce n'est **pas**
le pic de construction de `Prepared`, son stockage retenu, le propriétaire
du nuage/index, les piles, les en-têtes/allocations de runtime ou le RSS.
`Prepared::retained_bytes` est un poste séparé ; aucun pic complet de
pipeline en octets n'est revendiqué ici.

Une vague conserve ses masques jusqu'à réservation de la sortie puis
dispersion. Si la réservation échoue, le curseur reste pending : sa reprise
ne refait pas S2. La gate injecte `bad_alloc` **au point pré-commit**, y
compris lorsque la capacité courante aurait suffi : c'est une injection de
contrôle, pas une panne forcée dans l'allocateur standard. Deux échecs
successifs puis la validation de la vague doivent garder tous les compteurs
géométriques inchangés. Cette reprise limitée n'est pas un journal général.
Une exception pendant le filtrage, le tri ou la conversion finale interdit
la publication d'une sortie complète ; filtrage et finalisation défaillants
empoisonnent le curseur. Pas de reprise de `finish()` après une telle panne.

## Portes closes

Quinze commandes de qualification, Release et Clang ASan/UBSan/LSan.
Les deux builds produisent exactement le même compte rendu :

| couverture cumulée | résultat |
| --- | ---: |
| lots / consommations | 175 / 1 050 |
| appels ponctuels / survivantes comparées | 355 632 / 195 948 |
| rejets Pool union / par voie | 1 878 / 3 622 |
| incidences de fallback contenant une survivante de référence | 12 778 |
| découpes de bandes entre vagues / changements de rectangle | 78 426 / 68 616 |
| inversions effectivement corrigées avant sortie | 4 104 |
| consommations E=0 / E>0 mais S=0 | 216 / 6 |
| reprises pré-commit vérifiées / refus contrôlés | 139 / 20 |

Ces nombres sont des **compteurs de couverture**, pas des objets distincts
d'une trame : capacités et permutations rejouent les mêmes fixtures.
Les incidences fallback ne sont pas un nouveau compteur moteur d'émissions.

Recettes uniforme/terrain/amas/rangées, permutations des entrées,
s8/10/12 et K2/3/5/10 ; capacités1/2/7/17/64/>E. K1 est testé comme
**consommateur** sur des rectangles géométriques valides : le front q34 à
K1 ne peut pas démarrer avec mask6, faute de voie disponible. Le filtre
natif du lot, lui, accepte ces rectangles et rend toutes leurs voies mortes.
Les masques2/4/6, les trous d'ordinal dus aux fermetures, le partage de
facteurs et des requêtes appelantes modifiées après construction sont testés.
Les fixtures ciblées ne prétendent pas être toutes une décomposition WSPD.

Une petite découverte exhaustive dans les nœuds de trois fixtures64 trouve
un rectangle resté ouvert au filtre de boîtes mais dont toutes les paires
sont ensuite fermées ; six capacités exercent vraiment `finish()` avec E>0
et S=0. Les fonctions de décodage utilisées par le fallback sont aussi
testées sur un produit de 2³² et un ordinal 2³³+16 sans allouer ce produit.
C'est une porte des **formules u64**, pas le parcours de milliards de paires.
Produits et additions qui débordent sont refusés, sans écrêtage.

Trois mutations natives par build sont tuées causalement :

- masque candidat élargi au masque du rectangle : `waves.physical_queries` ;
- fallback rendu inactif : `waves.physical_queries` ;
- ordinal de bande substitué à l'ordinal original : `waves.duplicate_ordinal`.

Les deux premières qualifient le contrat des masques/compteurs physiques,
pas une détection démontrée d'une mauvaise géométrie finale. En particulier,
réouvrir une voie rejetée par Pool peut laisser S identique puisque le
filtre global la rejette à nouveau. Le troisième mutant casse l'identité
d'ordre. Aucun crash n'est utilisé comme verdict géométrique.

Lectures LIVE normale et `-O` identiques ; huit corruptions du reçu rejetées
causalement. Les sources et archives générateur réutilisées, les commandes,
sorties, configurations et binaires sont hachés avant/après. Aucun transfert
automatique des qualifications précédentes à ce consommateur. Pas de TSan,
multiworker du consommateur, GPU, mesure LiDAR ou nouvelle borne globale.

## Défauts trouvés avant gel et limites restantes

La contre-lecture a trouvé deux boucles `i=1; i!=S.size()` dangereuses lorsque
S est vide ; elles ont été remplacées par `i<S.size()` avant qualification.
Elle a aussi fait ajouter l'ancien buffer à la comptabilité de réallocation.
Le premier préflight a refusé l'appel `run_wspd_front(K1,mask6)` : le gate
a été corrigé pour tester le bon contrat consommateur, sans changer le
front ni masquer cet essai. Le build mutable de préflight n'est aucune
autorité de résultat ; voir le reçu pour cette chronologie.

Cette tranche ferme la porte fonctionnelle du **consommateur CPU S2** sans
tableaux globaux P/E. Elle ne réduit pas E, ne prouve pas E sous-quadratique
et ne mesure ni le tri, ni le transfert, ni FULL sur G4. La prochaine étape
est le port à vagues GPU avec préfixes/compaction par blocs, puis raccord
aux compteurs et à S3/FULL. Comparer encore les bandes avec les listes B
déjà ordonnées : cette porte ne choisit pas leur coût relatif à l'aveugle.
