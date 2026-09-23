# Lemme de la première cofacette : détection conditionnelle d'une omission

Auditeur A, 23 septembre 2026. Complément mathématique à la
[sonde d'omission de C](c_omission_20260923/README.md), sans nouvelle
mesure. Hypothèses : sites distincts, miniboules euclidiennes exactes,
catalogue complet **hors les clés supprimées**, et représentation
acceptée par FULL (notamment coquilles de taille au plus 12). Le
résolveur des facettes est la voie géométrique **interne** ; un batch
externe exige une preuve sémantique supplémentaire, car l'API vérifie
ses compteurs et le domaine des cibles sans recalculer chaque terminal.
Rien ici ne prouve que le générateur v9 produit le catalogue complet.

## Énoncé conditionnel

Soit une clé de boule $B$, avec $p_B$ sites strictement intérieurs et
$u_B$ sites sur la coquille. Posons $S=I_B∪U_B$ et
$K=|S|=p_B+u_B≤K_{max}$, avec $K≥2$. Si $B$ est la seule clé retirée
d'un catalogue autrement complet, la construction **jusqu'à** l'ordre
$K$ doit refuser ; elle peut déjà refuser à un ordre inférieur.
Plus généralement, elle doit refuser si l'ensemble **non vide** des
clés retirées est entièrement contenu dans la classe $p+u≤K_{max}$.
Ce second énoncé ne couvre pas des suppressions mêlées à des clés de
fin de fenêtre.

## Preuve géométrique et raccord au code

Pour $K<n$, tous les sites hors de $S$ sont strictement extérieurs à
$B=MEB(S)$. Choisissons $z∉S$ qui minimise
$β=r(MEB(S∪{z}))$ et notons cette nouvelle boule $C$. L'unicité de
la miniboule donne $β>α=r(B)$.

Aucun $w∉S$ ne peut être **strictement intérieur** à $C$. Sinon,
pour les centres $c_C,c_B$, posons $c_t=(1−t)c_C+t c_B$. Pour chaque
$s∈S$, la convexité donne

`‖s−c_t‖² ≤ (1−t)β² + tα² < β²`.

Pour $t>0$ assez petit, $w$ reste lui aussi strictement dans la boule
de rayon $β$ par continuité. Le maximum des distances de $S∪{w}$ à
$c_t$ serait donc inférieur à $β$, contredisant le choix de $z$.
Ainsi tous les intérieurs de $C$ sont dans $S$. Un support minimal de
$C$ peut être choisi dans $S∪{z}$, d'où $p_C+q_min(C)≤K+1$ ; sa
population fermée contient $S∪{z}$, donc $p_C+u_C≥K+1$. Sa fenêtre
contient l'ordre $K$ : $p_C+q_min(C)−1≤K≤p_C+u_C$. Sous l'hypothèse
de catalogue complet, le bloc $C$ y est programmé.

Le $K$-ensemble $S$ est une facette de ce bloc. Il est **isolé dans
la composante stricte juste avant le niveau de $C$** : une première
liaison stricte de $S$ à un autre $K$-ensemble impliquerait une
cofacette $S∪{w}$ de rayon inférieur à $β$. Le bloc $C$ doit donc
émettre $S$ lui-même comme représentant. Dans `full_ball_tower.hpp`,
`visit_block_at` émet les facettes régulières ou les représentants des
composantes strictes de `ShellTable` ; `prepare_static_order` les
transmet au résolveur.
Celui-ci calcule la miniboule exacte de $S$, soit $B$, puis cherche sa
clé et un intrus strict hors de $S$ (`static_terminal`).
La clé est absente, et un tel intrus n'existe pas puisque $S$ est toute
la population fermée de $B$ : `full_ball_static_missing_weak_terminal`.
La voie temporelle réalise le même test avec `resolve`. Si $K=n$,
le contrôle `full_ball_final_component_count` refuse l'absence de
l'unique naissance d'ordre $K$.

Pour plusieurs retraits tous de rang haut au plus $K_{max}$, prenons
$B$ de rayon maximal parmi eux. Sa cofacette $C$ a un rayon strictement
supérieur : elle ne peut être retirée, qu'elle soit ou non elle-même
dans cette classe. Le raisonnement précédent s'applique.

## Portée et porte proposée

La sonde C trouve **515/515** refus sur huit cas 8k à K5/K7 pour les
retraits isolés de cette classe ; ce sont des contrôles expérimentaux,
distincts de la preuve conditionnelle. Les **2/280** refus observés
*au-delà* du seuil montrent que $p+u≤K_{max}$ n'est pas un « si et
seulement si » de la détection. Des omissions mêlant des clés de fin
de fenêtre peuvent supprimer précisément la cofacette témoin.

Une exécution K5→K6 avec tour FULL et restriction clé par clé serait
un juge supplémentaire des omissions isolées q2/q3 de fin de fenêtre
à K5, pas un certificat universel ; à K10, elle demande un domaine
K11, encore absent. La [fixture de 23 sites de B](CONTRE_EXEMPLE_EULER_KPLUS2_20260923.md)
permet déjà de tester FULL à K10 sur la clé D sans porter K11.

Avant inscription au registre, qualifier la couture exacte « première
cofacette → représentant strict → résolveur » sur coquilles étendues,
égalités de niveau et voies statique/temporelle ; tester aussi le batch
externe si celui-ci devient actif. Un mutant de retrait doit vérifier
**statut et raison**, non seulement les afficher. La campagne C porte
sur des disques LiDAR 8k et des familles synthétiques, sans résultat
K10 publié ; elle ne mesure ni demi/quarts spatiaux ni densités
1/4–1/2–1.
