# Contrelecture du certificat par nœuds pour le cœur q3/q4

23 septembre 2026. Lecture statique du moteur et du reçu local au commit
`1f73b40d` ; aucun moteur modifié, aucune nouvelle mesure HGP. Cadre :
`exploration_v9_hors_registre`, CPU de référence, entrée entière u18/grille
1 mm, statut `not_claimed`. La proposition examinée est « certificat sur les
nœuds de l'index (compter sans énumérer) » dans
[`lidar_scaling_local_20260923/README.md`](../receipts/lidar_scaling_local_20260923/README.md).

## Verdict

**La proposition peut préserver exactement le rejet des voies mortes**, à
condition que chaque population de nœud créditée soit composée de sites
*distincts* et *strictement* intérieurs à toutes les boules de la cellule de
centres. Elle économise potentiellement la construction d'une forme par site
du cœur et les balayages de ces formes. Elle n'élimine ni l'expansion des
paires, ni le parcours de l'index par arête, ni les travaux q3/q4 et FULL des
voies restées ouvertes. Aucun gain de temps ou de croissance n'est acquis par
la seule proposition.

Le chemin actuel traite le cœur **après** le filtre de paire : construction
de `Q34EdgeCover::make_diametral`, somme logique `core_sites`, chargement
d'une forme affine pour chacun de ses sites, puis preuve sur cellules
([`wspd_q34.cpp:549–573`](../src/gen/pipeline/wspd_q34.cpp),
[`q34_dead_lanes.cpp:52–79`](../src/gen/lanes/q34_dead_lanes.cpp)). Si une
voie reste ouverte, le code construit le cover complet et la traite à
nouveau. Le constructeur du cœur sait **déjà** admettre ou rejeter tout un
nœud par sa boîte, mais ses plages sont ensuite énumérées pour les formes
([`edge_cover.cpp:88–129`](../src/gen/lanes/edge_cover.cpp)).

## Invariants suffisants pour un port

1. Garder le même index immuable, propriétaire et arête `ab`. Pour reproduire
   **exactement les comptes du cœur actuel**, une construction suffisante est
   une antichaîne de nœuds aux plages spatiales disjointes dont l'union
   représente le disque diamétral fermé
   `S_core={z: |2z−a−b|² ≤ |b−a|²}`. Une boîte entièrement incluse est
   admise avec `range.size()` ; une boîte extérieure est omise ; une boîte
   sécante est subdivisée jusqu'à une feuille testée exactement. Le simple
   fait qu'une boîte *rencontre* le disque ne permet pas de créditer toute sa
   population. Les liens `escape` et les rangs de l'index ne sont pas des IDs
   originaux. **Cette couverture exacte n'est pas nécessaire au seul rejet** :
   une sous-famille disjointe de nœuds certifiés entièrement dans le disque
   fournit déjà des témoins valides. Les autres nœuds peuvent rester
   inconnus ; si la sous-famille ne prouve pas la voie, le code reprend le
   cover complet. Une telle version peut s'arrêter dès la saturation des
   seuils et éviter la construction exacte de `S_core`, mais elle peut aussi
   fermer moins de voies. Il faut mesurer les deux effets.
2. Pour une cellule fermée `C` de centres, borner exactement ou de manière
   conservatrice `L_z(u)=|2z−a−b|²−|b−a|²−2(2z−a−b)·(u₁A+u₂B)` sur
   `box(N)×C`, avec la même échelle entière que le prouveur. Si la borne
   supérieure est **strictement négative**, les `range.size()` sites du nœud
   sont crédités ; si la borne inférieure est `≥0`, aucun n'est intérieur
   strict et le nœud peut être omis de la seule preuve de voie morte. Sinon,
   raffiner le nœud ou conserver son incertitude. `max=0` ne donne jamais un
   crédit. La borne inférieure peut être affaiblie par un arrondi vers le bas,
   jamais vers le haut. Les bornes entières de `Q4LocalGeometry::node_bounds`
   ont déjà la structure boîte×cellule nécessaire
   ([`q4_local_partition.cpp:243–285`](../src/gen/lanes/q4_local_partition.cpp)) ;
   leur API actuelle exige toutefois un **cover complet** et en prépare la
   décomposition. L'appeler telle quelle avant le cœur annulerait une partie
   de l'économie visée. Son port dans un contexte de preuve autonome exige
   sa propre qualification numérique u18.
3. Pour chaque cellule, le compte hérité et la frontière de nœuds doivent
   partitionner **une seule fois** les sites choisis : un nœud crédité ne
   peut réapparaître comme descendant ni être additionné à un ancêtre. Un
   enfant hérite du compte de sa cellule parente et raffine seulement sa
   frontière ; les quatre enfants ne s'additionnent pas entre eux. On peut
   arrêter et publier un certificat terminal dès le seuil atteint, mais un
   fragment interrompu ne devient ni un compte exact ni une coquille.
4. Conserver les disques de centres et seuils indépendants : `T₃=K−1`,
   `T₄=K−2`, aucune preuve à seuil nul. Toute cellule rencontrant le disque
   d'une voie doit être couverte par `T` intérieurs stricts ou raffinée ; à
   profondeur maximale, l'incertitude laisse la voie ouverte. Le test actuel
   d'un centre au coin permet d'abandonner une preuve seulement après un
   **compte exact** inférieur au seuil, ou après une borne *supérieure*
   certifiée inférieure au seuil. Un simple minorant insuffisant ne réfute
   rien. Les frontières fermées des cellules ne peuvent créer de trou.
5. Une preuve du cœur retire seulement les bits de voie effectivement
   démontrés. L'arête ne se ferme que lorsque tous ses bits actifs sont
   retirés ; chaque bit ouvert repart sur le cover **complet**, sans transférer
   de compte partiel vers le census, l'atlas ou les graines. Si tous les bits
   sont prouvés, aucune incidence q3/q4 n'est possible, donc aucun ID de
   coquille n'a à être produit pour cette arête. Le cœur n'est jamais une
   source de coquille ou d'intérieurs pour une voie survivante.

Les mêmes tests de boîte×cellule peuvent, en principe, démarrer à la racine
de l'index et créditer des sites hors du disque diamétral : tout site du
nuage strictement intérieur est un témoin valable pour **rejeter** une voie.
Ce serait un autre algorithme et un autre registre de travail, pas le même
`core_sites`. La restriction au cœur suffit à comparer directement avec le
prouveur actuel et facilite un différentiel exact.

Il n'y a **aucune sortie par site obligatoire pour une arête dont les deux
voies sont prouvées mortes**. Pour une voie qui survit, la représentation en
nœuds du cœur ne remplace pas la suite : le q3 accepté doit publier sa
coquille complète d'IDs originaux, et le q4 courant forme et balaie ses
événements et contacts avant d'émettre la coquille complète
([`wspd_q34.cpp:633–690`](../src/gen/pipeline/wspd_q34.cpp),
[`q4_local.cpp:367–459`](../src/gen/lanes/q4_local.cpp)). Il existe donc un
coût de sortie proportionnel aux IDs effectivement émis ; le traitement de
tous les sites du cover pour chaque voie ouverte est, lui, une propriété du
code actuel, pas une borne inférieure mathématique de toute architecture.

## Coût et lecture des compteurs

`core_sites` est la **population logique** du disque, incrémentée une fois
par arête à partir de `site_count()` ; ce n'est pas un compte d'accès mémoire
([`wspd_q34.cpp:552–556`](../src/gen/pipeline/wspd_q34.cpp)). Un port par
nœuds qui représente **exactement** le même cœur doit conserver cette valeur
et donc sa pente, même s'il ne lit plus tous les sites. Une preuve sur
sous-famille peut éviter de calculer cette population exacte : publier alors
la population logique **baseline** de la voie actuelle en comparaison
appariée, et séparément les populations de nœuds visités, crédités et laissés
inconnus par la nouvelle voie. Ne pas renommer le nombre de sites visités
`core_sites` ni imposer son égalité à l'ancien `core_sites`. Le travail
réellement évité se voit dans `dead_core.form_sites` (actuellement
`core_sites−2·core_builds`),
les tests de formes et les octets de tampon. Cette identité historique
décrit le chargement actuel : si `form_sites` devient un compte de formes
réellement matérialisées, l'identité et le schéma du lecteur doivent être
versionnés, non conservés artificiellement. Il faut publier séparément
populations logiques, visites/bornes de nœuds du cœur et des cellules,
feuilles exactes, nœuds ambigus/frontières copiées, comptes crédités et
temps CPU/mur/RSS ; remplacer un compteur de sites par un compteur de nœuds
sans conserver les deux masquerait le coût.

Exemple du reçu `s02/K10`, 16k→32k : `core_sites` vaut 141,097 M→1 023,499 M,
mais `core_cover_node_visits` vaut déjà 231,995 M→1 161,756 M et
`dead_core_uniform_tests` 397,861 M→1 965,318 M. Sur le seul cas 32k,
`core_cover_bound_tests` vaut 937,147 M et les tests ponctuels du constructeur
224,609 M (JSON individuel archivé). Réduire les formes sans réduire ces
postes peut déplacer le coût. La boîte×cellule coûte elle-même plusieurs
calculs par visite ; des boîtes grandes ou tangentes peuvent forcer la
descente jusqu'aux feuilles et laisser un pire cas `O(n)` par arête, répété
pour de nombreuses cellules. Il faut comparer le travail **total** sur les
mêmes trames et K, y compris paires résiduelles, cover complet, graines,
atlas, émissions, catalogue et tour.

La phrase du README « seul signal superquadratique » est trop étroite si
elle désigne tous les compteurs : son propre tableau donne également
`dead_core_uniform_tests` à `p=2,41` pour s00/K5 8k→16k et `p=2,30` pour
s02/K10 16k→32k ; les paires développées atteignent `p=2,27` sur s02/K5.
Ce sont deux doublements locaux, sans borne asymptotique. La priorité sur le
cœur reste plausible, mais doit être mesurée avec la réduction des longues
paires **avant** le cœur et le coût aval explicite.

## Porte minimale

Comparer l'ancien et le nouveau prouveur sur les **mêmes arêtes et masques**.
Un nouveau prouveur conservateur peut laisser plus de voies ouvertes et un
autre découpage peut en fermer davantage ; l'égalité des bits n'est donc pas
une obligation générale. Chaque bit fermé doit être justifié par son
certificat de profondeur strict, et le flux final doit rester identique.
Inclure K1/2, arêtes avec une seule voie
close, contacts `L=0`, boîtes sécantes et tangentes, cellules aux frontières,
coordonnées u18 extrêmes, branche de repli et interruption de `load`.
Comparer ensuite le flux complet trié par clé exacte, arité, support,
profondeur **et tous les IDs de coquille**, puis les catalogues/tours et leurs
digests. Mesurer W1/W8 sur les trois trames sans sol et les tailles emboîtées,
sans promouvoir ces dernières en contrat de trame entière ou en borne
sous-quadratique. Le bénéfice éventuel du cœur ne qualifie pas le seuil G4.
