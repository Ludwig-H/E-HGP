# Consommateur q34 par vagues bornées

27 septembre 2026, base `fd1a2c7ee`, audit CPU isolé sur grille entière
1 mm, `not_claimed`. Arène collective précédente figée, aucun moteur ni
GCP modifié. Interface acceptée avant implémentation.

## Propriété et masses

Un propriétaire `Prepared` copie les rectangles d'entrée, conserve le même
index natif immuable et applique le filtre rectangle Affine une seule fois.
Il stocke les plages originales, le masque restant et les préfixes bruts.
`raw_base[r]` additionne **tous les produits avant filtre rectangle**,
y compris ceux qui seront fermés. En revanche P/P3/P4 ne comptent que les
produits/voies encore ouverts après ce filtre. Ces deux quantités ne sont
pas interchangeables.

Tous les rectangles encore ouverts sont transportés : soit dans l'arène,
soit comme un segment fallback représentant leur produit intégral. La
politique `min_factor=2` et le seuil local impossible évitent une préparation
inutile, jamais une recherche. Un rectangle fermé ne devient pas fallback.
Les niveaux inactifs sont retirés par la primitive native avant toute
soustraction : K1 ne construit aucune arène q34, K2 ne porte que q3.

Les plans sont construits directement par l'arène collective figée : aucun
`Plan` historique, aucune cellule ancienne et aucune liste globale de P ou E
paires dans ce chemin. Seul le juge peut développer les petits produits.
Les segments portent un rectangle original, une bande d'arène ou le tag
fallback, puis un préfixe u64 de masse. Leur nombre est D+R_fallback,
pas E/taille_tuile.

## Vagues et sortie

Une vague contient au plus Q slots temporaires. Le décodeur trouve le
segment couvrant le curseur, puis décode ses lignes localement jusqu'à la
frontière du segment ou de la vague. Le masque est calculé avec les deux
crédits ; une bande union ne porte pas uniformément le masque6. Le fallback
utilise directement ses rangs originaux et son masque rectangle.

Le filtre ponctuel Affine natif s'exécute une fois sur chaque candidate E,
avec zéro crédit initial. Son masque résultat reste dans le slot jusqu'à
la compaction. Toute réserve de sortie est faite avant la dispersion :
en cas d'échec, aucune sortie complète n'est publiée. Le curseur implémenté
conserve les masques d'une vague pending et ne rejoue pas sa géométrie.
La porte dédiée injecte une panne pré-commit, pas dans l'allocateur standard,
puis vérifie deux refus et une validation sans nouvel appel géométrique.
Une panne pendant le filtrage ou la finalisation empoisonne le curseur ;
aucune reprise générale au-delà de cette réservation n'est promise.

Seules les S survivantes sont retenues, avec la clé originale
`raw_base[r] + (a_rank-A.first)*|B| + (b_rank-B.first)`.
Le tri des S clés, avec endpoints et masque sous la même permutation,
rétablit l'ordre natif **avant** construction de la sortie S2 destinée à
S3. Ce tri et la conversion sont payés. La coexistence temporaire du
tableau keyedS et de la sortie nativeS est comptabilisée.

`expanded_pairs` natif reste P. Le nouveau bilan publie séparément les E
appels physiques, P−E rejets union Pool et Pq−Eq rejets Pool par voie.
Les rejets ponctuels natifs se reconstruisent comme
`(Pq−Eq) + rejets_S2_sur_Eq`, jamais en ajoutant les masses q3 et q4 pour
obtenir l'union. Les visites géométriques changent ; les endpoints, masques
et l'ordre final S doivent être exactement identiques au natif.

## Borne de stockage et validité

Mémoire O(R+F+C+D+Q+S), index et nuage partagés à part. Tous les temporaires
par paire appartiennent à la vague Q ; les préfixes sont par segment.
Q est une capacité mémoire : le curseur avance jusqu'à EOF, même si Q=1.
Offsets/produits/additions sont contrôlés en u64 ; les rangs restent dans
le domaine u32 du consommateur natif, explicitement validé.
Le pic de capacités publié par le curseur inclut ancien+nouveau keyedS
pendant réallocation et keyedS+nativeS pendant conversion. Il ne mesure
pas les buffers ni le pic de construction de `Prepared`, le runtime,
les piles ou le RSS ; ces scopes restent distincts.

La disjonction des bandes provient de l'arène ; les fallbacks occupent des
rectangles non préparés et représentent exactement leurs produits.
L'ordre des rectangles d'entrée vient du producteur, pas d'une nouvelle
preuve de WSPD. Sur l'index natif à feuilles singleton exactes, un rejet
Pool aurait aussi été un rejet du filtre Affine ponctuel ; cela justifie
l'identité S, pas une égalité du travail géométrique ancien/nouveau.

## Portes, avant toute campagne

Comparer directement à `run_q34_filter_batch_cpu` les masques rectangles,
P, les deux nombres de rejets, et chaque survivante dans son ordre exact.
Capacités1/2/7/17/64/>E, lots vides, voies inactives, K1/2/3/5/10,
permutations d'entrée et s8/10/12. Exiger positivement : rejet Pool réel,
fallback survivant, bande scindée, franchissement rectangle et réordonnancement
S réellement nécessaire. Tester les préfixes au-delà de u32 sans allouer P.
Masques q3/q4/mixte et ordinal brut avec trous après fermeture sont distincts.

La branche « Pool local entièrement vide » reste inaccessible sur un
rectangle non vide sans cœur externe : sa paire minimum croisée survit.
E=0 se teste sur lots vides et rectangles/voies fermés, sans fausse fixture.
La future extension avec un cœur devra tester réellement son plan vide.

Release, ASan/UBSan/LSan, mutants causaux et lectures normales/`-O`
sont requis. Aucun raccord FULL, mesure LiDAR lourde ou GCP avant ces gates.

## Clôture

Ces portes passent dans la capture distincte du 27 septembre :175 lots,
1 050 consommations, trois mutations natives par build et huit corruptions
du lecteur. K1 porte sur la fermeture du consommateur, pas sur un producteur
q34 inexistant à ce niveau. Les grands offsets sont testés dans les mêmes
formules de décodage/ordinal que le fallback, sans prétendre avoir consommé
des milliards de positions. Lire le README pour les scopes et résultats.
