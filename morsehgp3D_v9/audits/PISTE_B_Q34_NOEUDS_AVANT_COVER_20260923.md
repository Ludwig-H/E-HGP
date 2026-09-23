# Piste B — prouver les voies q3/q4 mortes avant le cover

23 septembre 2026. Proposition mathématique et architecturale, **non
implémentée**, issue du [contre-audit du reçu](CONTRE_AUDIT_B_RECU_VOIES_MORTES_20260923.md).
Elle vise le vrai terme coûteux : `Q34EdgeCover::make` est appelé, puis
`Q34DeadLaneProver::load` fabrique une forme par site du cover **avant**
de savoir si les voies sont mortes. Sur 08/000100/K10, la version publiée
paie 4,15 milliards de ces formes pour 35 551 sites. Le certificat actuel
semble sûr, mais ne supprime pas ce travail préparatoire.
La [piste A de gardes par cellule](DOMINATION_Q4_PARESSEUSE_PAR_BLOCS_20260923.md#certifier-une-voie-morte-sans-balayer-chaque-cover)
vise le même verrou ; le présent document isole la variante qui crédite
directement des populations de nœuds par un test de puissance.

## Condition exacte pour un nœud entier

Fixer l'arête propriétaire `ab`, `v=b−a`, `L²=|v|²`, `m=(a+b)/2` et la base
entière `A,B` du plan bissecteur utilisée par le moteur. Un centre a
`2(c−m)=u₁A+u₂B`. Pour un site `z`, poser `w=2z−a−b` :

`F(z,u)=|w|²−L²−2w·(u₁A+u₂B)`.

Le site est strictement intérieur à la boule de centre `c` passant par
`a,b` si et seulement si `F(z,u)<0`. Pour une boîte spatiale fermée `Z`
d'un nœud de l'index et une cellule dyadique fermée `C` de paramètres
de centres, `F` est **convexe en z à u fixé** et **affine en u à z fixé**.
Ainsi le maximum de `F` sur `Z×C` est atteint parmi les **8 sommets de Z
× les 4 coins de C**. Tester exactement ces 32 couples donne un certificat
uniforme : si leur maximum est strictement négatif, tous les sites du
nœud sont intérieurs pour tous les centres de la cellule. On crédite sa
population une seule fois, sans lister les sites ni bâtir le cover.

Le disque q3 des triangles strictement aigus dont `ab` est plus longue
arête est `3|u₁A+u₂B|²≤L²` ; celui des tétraèdres q4 positifs est
`2|u₁A+u₂B|²≤L²`. Les deux tiennent dans la racine de paramètres
`[-2,2]²` déjà utilisée. En effet, les poids barycentriques positifs donnent
`R²=Σλᵢλⱼ|pᵢ−pⱼ|²≤L²/3` pour q3, `R²≤3L²/8` pour q4, puis
`|c−m|²=R²−L²/4` ; la base entière a une valeur singulière minimale
au moins `|v|/√3`.
Couvrir chaque disque par des cellules fermées
et obtenir dans chacune `K−1` sites distincts strictement intérieurs
prouve la voie q3 vide ; `K−2` prouve la voie q4 vide. Les nœuds crédités
d'une cellule doivent former une partition disjointe ; les crédits
d'autres cellules ou ceux du filtre universel existant ne s'additionnent
pas à ce compte. Les supports `a,b` ont `F=0` et ne sont jamais crédités.
Pour u18, les coefficients et les évaluations aux coins restent sous la
borne i64 du code courant ; promouvoir les produits avant opération et
rejouer les cas extrêmes sous UBSan.

Attention : les 32 coins certifient le **maximum**, pas le minimum de
`F`. Un nœud non crédité ne peut pas être déclaré uniformément extérieur
sur cette seule base : le minimum d'une fonction convexe peut se trouver
à l'intérieur de `Z`. Première réalisation sûre : garder ces nœuds dans
la frontière héritée des cellules enfants ; ajouter un minorant exact
seulement avec preuve séparée. Les frontières et comptes sont privés par
tâche, l'index est immuable et partagé. La preuve réussie ne nécessite
plus le cover ; en cas d'échec, construire le cover et lancer la voie
exacte inchangée. Si les deux voies sont prouvées, l'arête n'a aucun
cover à construire. Cette disposition se prête ensuite à des lots
multi-CPU/GPU, mais son coût de traversée doit d'abord être mesuré.

## Ne pas dupliquer le filtre déjà présent

`filter_q34_witnesses` précède **déjà** le cover. Son test exact
`3H²>Xi` / `2H²>Xi`, avec boîtes spatiales et crédit de populations,
certifie les sites intérieurs à **tous** les centres du disque q3/q4 ;
il subsume notamment l'idée élémentaire de compter les sites proches du
milieu de l'arête. Réintroduire cette petite boule serait un doublon.
La nouveauté proposée ici est différente : des **groupes de témoins
variables d'une cellule de centres à l'autre**, crédités par nœuds
avant de matérialiser les covers. Réutiliser si possible préparation,
index et informations du filtre préexistant, sans addition non prouvée
de ses crédits à ceux des cellules.

## Porte expérimentale avant promotion

Comparer sur les mêmes entrées trois voies : certificat désactivé,
certificat actuel, puis version nœuds-avant-cover. Préserver le flux
exact de clés, profondeurs, **IDs de coquille** et ordres FULL, pas
seulement le condensé du harnais qui ne garde que la taille de coquille.
Publier par arête et en sommes : voies prouvées, `cover_builds`,
`cover_sites`, `dead_form_sites`, visites de nœuds, tests de coins,
cellules/frontières, fallbacks, bytes retenus, CPU/mur et RSS. La baisse
du seul nombre de covers peut être annulée par les 32 tests de coins.
Faire d'abord W1 puis W8 sur les coupes capteur 8k/16k/32k appariées,
ensuite des trames entières sans sol et brutes de plusieurs séquences,
K5/K10 et s8/10/12. Seul le coût total générateur+catalogue+tour et sa
croissance peuvent juger l'objectif sous-quadratique et le contrat G4.
