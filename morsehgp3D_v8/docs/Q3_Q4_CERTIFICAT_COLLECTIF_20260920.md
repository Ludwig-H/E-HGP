# q3/q4 : assez de témoins partout, pas nécessairement les mêmes

20 septembre 2026, tranche26 après8d0a0f0f. Cadre :
`exploration_v8_hors_registre`, `cpu_reference`, `quantized_u16_input_only`,
`implementation_v8_p0`, `public_status=not_claimed`.

## Changement et limite recherchée

Le [filtre25](Q3_Q4_REJET_FAMILIAL_20260920.md) cherche des points qui
sont intérieurs à toutes les boules possibles d'une même face. C'est
suffisant pour rejeter cette famille, mais trop exigeant : deux témoins
peuvent se relayer, chacun couvrant une partie des boules possibles.

Cette tranche porte explicitement les deux propositions mathématiques de
l'[audit A4215dd16](../audits/q34_collectif_20260920/README.md), sans importer
son code Python ni ses résultats comme qualification du produit :

1. Réduire l'intervalle des centres possibles par une meilleure borne.
2. Calculer la plus petite population intérieure du petit pool sur tout
   cet intervalle. Si elle suffit, rejeter toute la famille q4.

Les quatre combinaisons sont explicites : `Jung` ou `Variance`, et
`Universal` ou `Collective`. Elles utilisent le **même pool spatial**.
Le pool de l'auditeur, choisi près du milieu de l'arête, est différent ;
ses chiffres ne peuvent pas servir de comparaison de ce port.

Il ne s'agit pas encore de partager le census de plusieurs faces. Un
petit préfixe plus efficace peut laisser un grand résidu ; la série dense
8k/16k/32k de la tranche25 doit rester discriminante.

## Corde resserrée, arithmétique sûre

La paramétrisation et les formes P, B sont celles de la note25. Ici D
désigne ab², E=ax², X=bx² et G le déterminant de Gram de la face aiguë.
Le rayon au carré de la face vaut r=DEX/(4G). L'arête ab est maximale.

Pour un tétraèdre positif, soit t la somme des poids barycentriques de
sa face. La variance des sommets de cette face à poids normalisés est
au plus r. Les distances au carré au quatrième sommet sont au plus D.
La variance totale donne donc :

$$R^2\leq t^2r+t(1-t)D\leq\frac{D^2}{4(D-r)}\leq\frac{3D}{8}.$$

Poser V=2G−EX et T=4G−EX. L'angle opposé à l'arête maximale est entre
π/3 inclus et π/2 exclu ; G est EX fois le carré de son sinus.
Donc G≤EX≤4G/3 et 0<V<T.
Avec le paramètre μ de la famille :

$$\mu^2\leq\frac{DV^2}{T}\leq\frac{J}{2},\qquad J=D(3G-2EX).$$

La différence des deux majorants est DG(4G−3EX)/(2T)≥0.

Le produit DV² ne tient pas nécessairement dans i128. Le code ne le
forme pas :

$$q=\left\lceil\frac{DV}{T}\right\rceil,\qquad U=\min\left(U_{\mathrm{Jung}},\left\lceil\sqrt{qV}\right\rceil\right).$$

Les deux plafonds se calculent par division quotient/reste et racine
entière, jamais en flottants. Comme q≤D et V≤2G, DV et qV sont au plus
54M⁶<2¹⁰² pour M=65535. Les deux candidats au minimum sont des majorants
certifiés ; leur minimum l'est également. Les inégalités P±UB restent
dans la borne i128 de la note25. Les coûts de l'ancien plafond et du
plafond resserré sont distingués, y compris leurs deux racines entières.
L'arrondi extérieur peut effacer le gain géométrique : U n'est pas
présenté comme le plafond exact de la borne rationnelle non arrondie.

## Minimum du pool sur l'intervalle fermé

Un site z est strictement intérieur exactement lorsque P(z)−μB(z)<0.
Si B=0, il contribue partout ou nulle part. Sinon, sa contribution
change à la racine P/B : entrée si B>0, sortie si B<0. À la racine,
le site appartient à la coquille et ne contribue pas au compte strict.

La première passe calcule P et B une seule fois par proposition. Elle
fait les tests universels q3/q4 et, dans le mode collectif, prépare la
population juste avant −U et les IDs des événements dans [−U,U]. Les
comparaisons aux bornes utilisent P+UB et P−UB, avec le signe de B.
Les points dont la racine est hors intervalle contribuent au bon état
initial ; les sommets de la face, P=B=0, ne sont jamais des témoins.

Le préfixe universel peut s'arrêter quand les deux voies disponibles sont
rejetées. S'il rejette q4 seul, q3 poursuit ses propositions, mais il ne
faut plus préparer un balayage collectif devenu inutile. Les préparations
déjà payées restent dans les compteurs, même sans balayage.

**Le tri n'a lieu que si q4 n'a pas été rejeté par le certificat universel.**
Dans ce cas le pool a été entièrement visité. Les racines sont triées
avec le comparateur réduit exact de la famille, sans former P₁B₂ en
i128, et départagées par ID. Les racines égales sont traitées ensemble :

1. Retirer toutes leurs sorties.
2. Lire la population stricte et mettre à jour son minimum.
3. Ajouter toutes leurs entrées.

On ne sature **jamais** cette population : elle peut diminuer ensuite.
Le minimum inclut les groupes aux deux bornes fermées. Entre deux
racines, la population est constante et ne peut être inférieure au
minimum lu aux groupes adjacents ; l'état initial couvre l'extrémité
gauche s'il n'y a pas d'événement à cette borne. Un groupe sortie/entrée
peut faire apparaître un creux ponctuel : tester seulement les bornes
ou seulement les intervalles ouverts serait faux.

Ce minimum exact du **pool** n'est qu'un minorant de la profondeur du
nuage. S'il atteint K−2, tous les q4 positifs propriétaires de cette
famille sont inutiles. Le q3 de la face reste une décision indépendante
au seuil K−1. Aucune de ces populations n'est ajoutée au census de repli.
Une voie non rejetée reprend le calcul exact à zéro, coquille complète
en cas d'acceptation, comme dans la tranche25.

## Objets, durée de vie et parallélisation

`Q34PoolOptions` est passé explicitement aux nouvelles entrées
`run_q34_collective_seed_candidates` et `run_q34_collective_edge_candidates`.
Les anciennes entrées et leurs défauts restent inchangés. Un pool vide
ne déclenche aucun filtre dans ces raccords ; les options invalides sont
néanmoins refusées avant toute émission, y compris à K1.

`assess_q34_family_pool` expose séparément l'évaluation d'une face. Il
valide pool, paramètres et propriété ; ses tests de propriété sont payés
dans ses propres compteurs. Ses bornes et son minimum ne sont utilisables
que lorsqu'un certificat, respectivement un balayage, a réellement été
construit. Un zéro par défaut n'est pas un census calculé.

`Q34PoolWorkspace` ne contient que des IDs d'événements, aucune liste de
formes P/B ou copie des coordonnées. Il est privé à l'appel, non copiable,
non déplaçable et réutilisé entre toutes les faces d'une arête. Il peut
servir à un autre pool après retour, mais jamais à deux appels concurrents.
Le pool immuable partagé possède toujours son cover, son index et son nuage.
Les vues de sortie restent synchrones ; une exception conserve les
émissions déjà reçues par le client, sans résultat partiel réussi.

Le workspace reste alloué pendant le repli. `peak_live_buffer_bytes`
additionne sa capacité au pic des buffers du repli pour **la même face**,
puis prend le maximum sur l'arête. Ce n'est pas la somme de deux maxima
indépendants, ni le RSS, ni la mémoire des callbacks/du nuage/de l'index.
Les trente compteurs de filtre séparent les tests, les événements préparés,
les tris, les groupes, les rejets et la capacité. Seuls `max_group` et
`peak_event_bytes` sont des maxima ; les autres sont des sommes.

Ces objets sont compatibles avec des appels indépendants sur des workers,
mais cette tranche n'ajoute pas de dispatcher q3/q4 ni de moteur GPU.
La distribution future doit partager le parent et posséder son scratch,
sans tableau de tous les événements de toutes les faces à la fois.

## Coût et qualification

Pour F faces et C propositions, le filtre universel coûte O(FC), le
collectif au plus O(FC log(1+C)), avec O(C) d'IDs réutilisables par appel.
Ce travail s'ajoute à la préparation du nuage/index/cover/pool, à la
génération des faces et au coût des familles qui survivent. Chacune de
ces dernières peut encore demander O(m log(1+m)) pour m sites couverts.
Ni une diminution du nombre de familles ni un petit temps de préfixe
ne prouvent donc une borne globale sous-quadratique.

Qualification :87 CTests Release, cinq gates et24 sondes sous Clang
ASan/UBSan,8549 contrôles de la nouvelle gate, trois mutants compilés
tués et20 paires différentielles contre le binaire25.208 mesures
principales, dont96 à8k/16k/32k, comparent les quatre options.
Les lectures normal/−O et empreintes de fermeture passent ; les163
sources et les builds q34_collective Release/Sanitizer sont épinglés.
Les [reçus](../receipts/q34_collective_20260920/README.md) conservent
les coûts détaillés, contre-régimes et erreurs de préflight.

À256 points adverses etC64, Variance+Collective réduit les lectures
du repli de15 616 à10 667 K5 et de33 536 à21 266 K10, sorties identiques.
Ses14 815/37 134 comparaisons de tri de filtre sont payées en plus.
Mais le filtre ne rejette rien de plus sur les grands fonds à deux faces.
La projection dense distincte exécute48 filtres de toutes les faces,
pas leurs replis : il reste au moins185,344M/358,240M lectures futures
à32k/C64/K5/10. Le dernier doublement de ce minimum vaut×9,694/×6,223.
Ni ce résidu ni le coût total ne sont qualifiés sous-quadratiques.

La priorité devient donc le partage de décisions **entre** faces dans
le plan des centres d'une arête, sans arrangement exhaustif. Cette
carte reste à porter et qualifier ; un budget de certificat ne pourra
jamais supprimer le repli exact d'une zone indécise. L'AABB de domaine
des centres devra inclure les complétions obtuses et ne permettra pas
de retirer les témoins extérieurs à la lentille du census.

WSPD q3/q4 global, catalogue canonique, intérieurs après regroupement,
reconstruction FULL, GPU/G4 et contrats de tour restent ouverts. Le
s8/10/12 de la WSPD n'intervient pas dans cette entrée à arête fournie.
GCP non utilisé.
