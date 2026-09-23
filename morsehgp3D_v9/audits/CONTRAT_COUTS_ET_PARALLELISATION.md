# V9 — contrat mesurable, coût complet et parallélisme intérieur

22 septembre 2026. Moteur v8 lu à `a74e90f2`, ouverture v9 `3595725a`. Audit de transition, sans qualification v9 ni exécution GCP. Cette note porte sur le système et l'ordonnancement ; les propositions géométriques q3/q4 exigent leurs propres preuves. Le [premier jalon de temps v9](https://github.com/Ludwig-H/E-HGP/blob/3595725a/morsehgp3D_v9/docs/AUDIT_V8_SYNTHESE.md) est la trame LiDAR entière sans sol, moteur entier u18/grille 1 mm. Le float32 original reste le défaut d'entrée fixé en v8, avec développement temporel v9 suspendu ; le contrat principal antérieur sur trame brute entière demeure, sa portée temporelle v9 étant à confirmer. Les trois trames disponibles proviennent toutes de la séquence SemanticKITTI 08.

## Écart réel avec la cible

La cible G4 est la **tour FULL entière K=1..10 <1 s**, repli **K=1..5 <1 s**, puis **100 ms** sur le même périmètre sans sol u18/1 mm retenu à l'ouverture. Elle comprend préparation, événements, catalogue, parents, hiérarchies et transferts nécessaires. Lecture disque et démarrage froid/chaud doivent être publiés séparément et leur frontière fixée avant qualification. Une coupe spatiale, un préfixe ou un flux q3/q4 ne constitue pas cette tour.

Le [reçu G4 du flux q3/q4](../../morsehgp3D_v8/receipts/q34_spatial_20260921/README.md) donne, sur **trames brutes entières u16/2 cm**, K5/s8, 48 workers CPU, une observation chacune :

| Trame 08 | Sites | Mur du flux | CPU cumulé | CPU logiques occupés en moyenne | Mur / 1 s | CPU / 48 s, facteur CPU seul si débit inchangé |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 000000 | 119 142 | 165,214 s | 691,65 s | 4,19 | 165,2 | 14,41 |
| 000100 | 119 942 | 34,319 s | 382,00 s | 11,13 | 34,3 | 7,96 |
| 000200 | 120 725 | 505,479 s | 973,37 s | 1,93 | 505,5 | 20,28 |

La dernière colonne ne suppose qu'une division arithmétique du travail CPU mesuré entre **48 CPU logiques occupés à 100 %**, hypothèse favorable pour une route CPU seule à débit par logique inchangé, sans coût FULL ni transfert. Les cœurs physiques sont 24 avec SMT ; ce n'est pas une prévision de débit ni une borne pour une future route GPU. Pour 100 ms, les mêmes facteurs CPU seuls deviennent 144,1 / 79,6 / 202,8. Le facteur de temps mur requis depuis ce flux mesuré est 1 652 / 343 / 5 055 pour 100 ms. Les 768–769 jobs Coarse s'achèvent, mais l'arête et ses travaux intérieurs sont indivisibles ; augmenter encore le nombre de jobs de front seul ne rend pas ces arêtes parallèles. K10, s10/s12 et d'autres séquences n'ont pas été exécutés dans ce reçu. Le GPU G4 n'a pas participé à ces calculs.

Le [premier reçu sans sol entier à 1 mm, inventorié à l'ouverture](https://github.com/Ludwig-H/E-HGP/blob/3595725a/morsehgp3D_v9/docs/AUDIT_V8_SYNTHESE.md), trame 08/000000, compte 39 885 sites, K5/s8/W8, **104,63 s mur** et **812,82 CPU·s** pour le seul flux q3/q4, avec 691 284 émissions q3 et 158 496 q4. Ce reçu v8 était **non commis au pin d'ouverture** ; il est depuis publié à `3f0d188f` dans `morsehgp3D_v8/receipts/u18_resume_20260922/ground_1mm_first/`, sans rejeu par cet audit. L'option `saturate_deep` y est désactivée ; une seule répétition et aucune identité W1/W8 de cette entrée ne sont disponibles. Ce reçu est un autre profil et un autre hôte : le comparer numériquement à la ligne brute u16/G4, ou à la base sans sol u16, ne donne aucun speedup. Son index partagé prend 15,653 ms, la préparation du nuage 2,192 ms et la lecture/hash 2,586 ms **sur l'hôte local** ; le front et ses consommateurs prennent 104 610,5 ms. Le pilote de segmentation indique 30,358 ms médians pour lecture brute→masque sur cette trame, processus exclu, [reçu du pilote](../../morsehgp3D_v8/docs/PILOTE_LIDAR_SANS_SOL_20260921.md). La préparation Python hors ligne, le catalogue et FULL ne sont dans aucune de ces durées. La segmentation et HGP doivent figurer séparément puis ensemble dans le scénario issu du brut.

Le flux 1 mm émet 849 780 callbacks et totalise 2 707 836 IDs de supports ainsi que 2 707 842 IDs de coquille. La sonde n'en conserve qu'un digest : une sortie complète, ses clés et son catalogue restent à mesurer. Pour situer l'échelle seulement, matérialiser ces deux listes en IDs de 64 bits exige déjà au moins 43,3 Mo d'écriture, avant clés, offsets et parents. Atteindre 100 ms exige également au moins 8,5 millions d'émissions/s sur cette scène si la même quantité de supports subsiste ; cela ne prouve pas que la bande passante mémoire serait le verrou principal. Les sorties FULL peuvent différer fortement de ce flux.

La [tour v7 G4](../../morsehgp3D_v7/docs/RESULTATS_TOUR_CACHE_G4_20260910.md) donne une autre alerte : sur un uniforme u16 à 50k, le census CUDA a coûté 189,346 ms de kernels à K10, tandis que FULL CPU prenait 390,481 s et la tour 418,921 s ; K5 restait à 33,569 s, dont 26,983 s FULL. Cette voie et ses données ne qualifient pas le LiDAR v9. Elles montrent pourquoi accélérer uniquement un noyau géométrique ne suffit pas au contrat de tour.

## Grand-livre de travail à fermer sur chaque trame

L'entrée v8 `run_wspd_q34_parallel` partage l'index et les jobs du front. Une arête résiduelle construit son cover, puis éventuellement un atlas Local28 commun aux voies q3/q4. La file actuelle divise les rectangles en plages de rangs A **avant** l'expansion des arêtes ; `Engine::edge`, le census q3 de chaque graine, la construction et le balayage de l'atlas q4 restent synchrones dans un worker. Les callbacks sont privés par slot, mais leurs vues sont empruntées pendant l'appel. Les tâches publiées refusées sont traitées localement, ce qui préserve la complétude. L'annulation joint tous les workers. Ces invariants, présents dans [wspd_q34.cpp](../../morsehgp3D_v8/src/pipeline/wspd_q34.cpp), sont à conserver lors du découpage intérieur.

Sur le reçu 1 mm, avant 849 780 émissions, le front laisse 23,687 millions de paires développées et 2,044 millions de covers ; q3 examine 184,462 millions de graines propriétaires et aiguës, dont 153,036 millions sont rejetées par l'atlas, puis construit 31,425 millions de boules. L'atlas prépare 38,795 millions de cellules, paie **3,252 milliards** de bornes de blocs, **7,316 milliards** de tests de points et **5,547 milliards** d'IDs copiés dans les frontières ; le tri q4 compte 163,678 millions de comparaisons. Le census q3 ajoute 1,126 milliard de bornes préparées. Ces colonnes n'ont pas le même coût unitaire et certaines sont des sous-comptes : ne jamais les sommer en un « nombre d'opérations ». Le modèle de croissance doit publier au minimum : produits de front, masse résiduelle par voie, paires réellement développées, covers, populations logiques et nœuds visités, graines, cellules/fragments/IDs copiés, bornes et tests q3/q4, événements/tri, sorties, clés, catalogue, parents, temps CPU/mur et RSS par phase.

Le [protocole spatial brut](../../morsehgp3D_v8/receipts/q34_spatial_20260921/README.md) a déjà une relation défavorable sur 08/000000 : trame→moitié x+, exposant observé 2,502 pour les bornes q3 et 2,332 pour les bornes de blocs q4 ; les sorties q3/q4 ont des exposants nettement inférieurs sur ses six relations. Les coupes modifient densité et frontières : ce sont des diagnostics de régime, pas une preuve asymptotique. Il faut répéter le même grand-livre sur plusieurs **séquences** entières, brutes et sans sol à masque figé, K5/K10 et s8/s10/s12, sans choisir après coup le meilleur morceau. Ajouter un diagnostic séparé de captations LiDAR superposées et de densités accrues, sans supposer leur alignement : des passages proches peuvent grossir les frontières actives et les coquilles sans contact exactement cosphérique. Le coût global visé est sensible aux sorties : une garantie universelle strictement sous-quadratique est impossible si le résultat explicite lui-même est quadratique.

Le fil directeur v9 est le **certificat local k-Gabriel d'un support canonique et de sa boule minimale**, avec compte de témoins stricts et coquille complète. L'index global donne des blocs de témoins et le front filtre des familles ; il n'est pas un diagramme de Voronoï/Delaunay d'ordre supérieur à construire. Un certificat commun ne devient une tâche qu'en présence de candidats, puis se partage entre supports d'une même arête ou d'un même bloc ; un signe indécis se raffine exactement. C'est cette paresse et le nombre total de certificats réellement visités, plutôt que la seule parallélisation des anciennes boucles, qu'il faut confronter au seuil sous-quadratique dans les deux régimes LiDAR. Les liens d'incidence entre supports cosphériques restent conservés après regroupement des boules.

### Filtrer une ligne `a × B` avant l'expansion q3/q4

Le [reçu G4 R4b](../receipts/g4_tower_r4b_20260923/README.md) établit
qu'un cache des témoins évite la recherche complète sur 53,36–69,76 %
des **paires résiduelles développées**, mais ne réduit ni leur
énumération, ni les covers. Un cran de granularité exact se trouve entre
le filtre du rectangle `A_box × B_box` et le filtre de chaque paire : pour
chaque ancre `a` dans A, appeler la primitive existante
`filter_q34_witnesses(index, singleton_box(a), box(B_node), K, mask, …)`.
Tout nœud de témoins admis par ses bornes `H`/`Xi` l'est pour **chaque**
`b∈B_node` ; un seuil de `K−1` témoins stricts éteint q3, et celui de
`K−2` éteint q4. Le masque rendu ne peut donc que retirer des voies
pour toute la ligne. Si les deux voies s'éteignent, créditer `|B|`
paires et passer à l'ancre suivante sans les matérialiser ; sinon
transmettre le sous-masque à la voie par paire inchangée. La preuve ne
transfère aucun compte entre lignes et ne suppose ni alignement ni
modèle de densité LiDAR.

La différence avec le filtre du rectangle est stricte : sur les
points collinéaires `A={0,3,5,10}`, `B={1000,1001}`, `K=3`, une sonde
compilée temporaire de la primitive renvoie le masque `6` pour tout
`A_box × B_box`, puis `0` pour la ligne `a=0` ; chacun des deux filtres
ponctuels de cette ligne renvoie aussi `0`. Les témoins `3,5` sont
stricts pour `a=0`, mais pas pour `a=10`. C'est une fixture de sûreté et
de gain structurel potentiel, **pas** une mesure LiDAR ni un test du
front WSPD complet. Le splitter actuel de `wspd_q34.cpp` répartit des
plages disjointes de rangs A et garde B entier ; `expand` parcourt A à
l'extérieur puis B. Chaque ligne est donc possédée par un seul worker,
y compris quand la file refuse une tâche, sans nouvel état partagé.

Le [sidecar reproductible](shadow_q34_rows_u18_20260923.cpp) refait le
front K5/s8 sur les **trois trames sans sol entières à 1 mm** du reçu
v8, avec le filtre `Affine` du produit et une seule ligne déterministe
pour environ 1/128 des rectangles survivants ayant `|A|≥2,|B|≥8`.
Les masses totales du front sont exactement celles de R4b ; la
bibliothèque locale utilisée portait le SHA-256 `11858d793bc1…` et les
trois entrées les SHA du reçu R4b. Un second build cohérent de
`aae9da0e` (bibliothèque SHA-256 `2f3d113b…`) retrouve exactement les
trois masses de front et toutes les lignes du tableau ci-dessous ; ce
rejeu vérifie que la centralisation des coordonnées dans l'index ne
change pas la sonde. Le script n'exécute ni le reste du générateur, ni
catalogue, ni FULL : c'est une observation géométrique locale, non un
reçu de chaîne.

| Trame 08 | Masse éligible après filtre rectangle / paires développées R4b | Lignes échantillonnées | Paires entièrement évitables dans l'échantillon | Visites DFS de ligne / paire évitable |
| --- | ---: | ---: | ---: | ---: |
| 000000 | 20 747 697 / 23 686 751 (87,6 %) | 191 | 3 207 | 15,7 |
| 000100 | 9 423 007 / 11 960 420 (78,8 %) | 143 | 1 604 | 21,2 |
| 000200 | 18 987 124 / 22 722 345 (83,6 %) | 275 | 3 926 | 29,8 |

La masse éligible est grande, mais l'échantillon d'**une ligne par
rectangle** n'est pas un estimateur du gain global. Le DFS neuf coûte
ici 15,7–29,8 visites par paire qu'il écarterait entièrement, avant
comparaison au filtre/cache de paire qu'il remplacerait ; aucune
économie nette n'est prouvée. Ne pas activer ce port sur ces chiffres.
Un transfert plus léger peut conserver les **seuls nœuds admis** par
le filtre rectangle : au plus `(K−1)+(K−2)=2K−3≤17` entrées avec voies
et comptes, disjointes par voie. Leur admission pour `A×B` reste vraie
pour `{a}×B`; le DFS de ligne doit sauter exactement leurs sous-arbres
par voie pour éviter tout double crédit. Ce ticket possédé est borné,
contrairement à une frontière complète de sous-arbres indécis qui peut
atteindre `Θ(n)` par rectangle. Mesurer son coût avant d'élargir l'API.

Avant activation, compter en mode shadow `row_queries`, visites de
nœuds, masse de lignes entièrement rejetées et masses q3/q4 retirées ;
facturer le DFS de ligne même si aucune paire n'est évitée. La porte
existante `q34_witness_search_gate` vérifie les boîtes, et
`wspd_q34_gate` doit comparer W1/W4, petit grain et refus de file.
Après activation, le grand-livre des paires doit satisfaire
`input_pair_mass = rectangle_pair_mass + row_full_pair_mass + expanded_pairs`
sur une exécution complète, sans compter deux fois les rejets par voie.
Le gain doit être jugé sur mur, CPU, covers, sorties et RSS pour les
trames LiDAR entières et les scènes denses, pas sur la seule masse
évitée.

### Covers communs sur un bloc d'arêtes survivantes : certificat entier secondaire

Le premier moteur v9 `d2700314` construit encore un `Q34EdgeCover` **par
arête** après son filtre ponctuel dans la configuration mesurée ; ce
filtre est facultatif dans l'API. Sur le reçu v8 sans sol 1 mm, cela
représente 2 043 612 covers et 440 194 038 visites d'index : un poste
mesurable, mais inférieur aux milliards de tests de l'atlas q4. Une
famille de covers peut partager un certificat exact au niveau d'un
bloc d'arêtes survivantes `E⊂A×B` et d'un nœud spatial `Z`, sans
transférer aucun crédit de profondeur. Le prédicat individuel est

`F(a,b,z)=|2z−a−b|²−4|a−b|²≤0`.

Pour chaque axe, borner les intervalles `R_i=2Z_i−A_i−B_i` et
`D_i=A_i−B_i`, puis définir `minsq` et `maxsq` exacts sur un intervalle
entier fermé. Poser

`L=Σ minsq(R_i)−4Σ maxsq(D_i)`,
`U=Σ maxsq(R_i)−4Σ minsq(D_i)`.

Alors `L≤F≤U` pour **toutes** les paires du produit, donc pour le
sous-ensemble `E` : `U≤0` admet le nœud Z pour toutes les arêtes,
`L>0` le rejette pour toutes, les autres cas divisent `E` ou `Z` puis
reviennent au test exact singleton. L'égalité `F=0` est admise. Avec
`M=262143`, `|L|,|U|≤12M²<2^40` ; i64 suffit si chaque soustraction
et carré est promu avant calcul. Un certificat partagé peut rester un
handle **intermédiaire** `(bloc E, nœud Z, décision)`. L'API actuelle
attend ensuite pour **chaque arête** des ranges de rangs spatiaux
triés, disjoints et fusionnés, `site_count`, `contains_id` et la
propriété de l'index : `Q4LocalGeometry::decompose_cover` consomme ces
ranges. Une partition du produit en tuiles disjointes ou un tri final
par `(arête, rang Z)` peut fournir l'ordre, puis il faut fusionner les
ranges adjacents et facturer matérialisation, allocations et copies.

**Après le filtre de paires, réduire les vraies arêtes survivantes.**
L'enveloppe A×B peut être très lâche quand `E` est un résidu clairsemé.
Pour une tuile bornée `E`, réduire en `O(|E|)` ses extrema exacts
`S_i^- = min_{(a,b)∈E}(a_i+b_i)`, `S_i^+`,
`R^- = min_{(a,b)∈E}4|a-b|²` et `R^+`.
Pour le nœud Z, poser `T_i=[2Z_i^-−S_i^+,2Z_i^+−S_i^-]` ; alors

`L_E = Σ_i minsq(T_i)−R^+ ≤ F(e,z) ≤ U_E = Σ_i maxsq(T_i)−R^-`

pour tous `e∈E,z∈Z`. Les intervalles des `T_i` sont contenus dans ceux
de A×B et `R^-,R^+` resserrent les bornes du rayon, donc
`L≤L_E≤U_E≤U` : aucune décision ancienne n'est perdue. Ces réductions
associatives conviennent à un lot CPU/GPU, **sans supposer un LiDAR
aligné**. L'[oracle entier](check_cover_batch_u18_20260922.py) vérifie
43 386 triplets A×B×Z et 1 000 familles survivantes en modes normal/`-O` ;
ses bornes se resserrent strictement 916/943 fois, et deux familles
clairsemées distinguent rejet/admission de l'ambiguïté A×B. Cela n'est
**pas** une mesure de rejet sur vrais résidus LiDAR. Payer la réduction,
les décisions `E×Z`, les replis exacts, les handles, la production finale
de ranges **par arête** et le coût aval d'atlas dans le même chrono ;
garder un grain explicite pour les tuiles même si le front laisse un grand
`|B|` dans un seul job. Réduire les arêtes en flux par tuile évite de
matérialiser d'un coup les 23,7 millions de paires du reçu, mais les
identités de chaque arête doivent survivre jusqu'à la matérialisation
certifiée des ranges. Aucun compte de profondeur n'est transféré par
ce certificat de cover.
La même tuile pourrait borner la lentille du domaine positif q4, mais
son `completion_box` exclut **les deux endpoints propres à chaque arête** :
une admission `E×Z` ne fournit pas cette boîte tant que Z peut contenir
un endpoint de E. Certifier cette disjonction ou raffiner avant agrégation.

Le même oracle subdivise désormais les **1 000 familles de survivantes**
avec les bornes `E×Z` ci-dessus : les tuiles terminales sont disjointes,
couvrent chaque couple arête–site et rendent le cover ponctuel exact
(16 928 visites de tuiles dans ces petits cas synthétiques). Trois
contacts exacts sont testés. Ce résultat ne vérifie **pas** la fusion des
ranges du produit v9, les rectangles WSPD réels ou le coût d'une file GPU.
Pour une
ablation pertinente, former les blocs après le masque de rectangle et
le filtre de paire **s'il est actif**, en conservant le masque q3/q4
de chaque arête ; ne mutualiser que lorsque plusieurs arêtes survivantes
partagent un produit. Le buffer `E` doit avoir sa propre fenêtre
bornée : `parallel_task_pairs` ne borne pas la taille d'une tâche si
`|B|` dépasse son grain. Sinon garder le cover individuel. Comparer sur les trois
trames les tests conjoints, décisions `in/out/ambiguous`, handles,
replis individuels, octets simultanés, durée complète et sorties
q3/q4 puis FULL identiques. Un arbre de boîtes d'audit ou quelques
arêtes choisies hors WSPD ne prouvent aucun gain de pipeline.

Une **fixture entière u18**, compatible avec le profil de la v9, rend la
barrière de sortie q2 directement testable. Pour `1≤i,j≤m`, poser
`A_i=(4Li,8L²−i²,0)` et `B_j=(0,j²,4Lj)`, avec `2L²>m³` et toutes les
coordonnées dans `[0,262143]`. Pour un autre `A_h`, la puissance dans la
boule diamétrale de `A_iB_j` vaut
`(h−i)[8L²(h−i)+(h+i)(h²+j²)]>0`; la formule symétrique vaut pour les
autres `B_h`. Les `m²` paires croisées sont donc toutes des feuilles q2
Gabriel strictes distinctes. À `m=40,L=179`, les 80 sites tiennent dans
u18 (coordonnée maximale `256327`) et produisent **1600 feuilles q2** ;
une vérification entière des `1600×78` puissances étrangères donne une
marge minimale `9769`. C'est une porte de sortie sensible aux octets à
ajouter au FULL, pas un modèle LiDAR ni une preuve d'asymptotique dans
l'univers u18 fini. Elle complète la [famille rationnelle à précision
croissante de la v7](../../morsehgp3D_v7/docs/CROISSANCE_ET_BORNE_DE_SORTIE.md),
sans en transférer les hypothèses de régularité.

## Découpage intérieur proposé pour v9

L'unité de travail persistante est un **contexte d'arête possédé** : identifiant de l'index immuable, paire d'IDs propriétaire, masque q3/q4, cover certifié et, si construit, atlas immuable. Les objets lourds ont une durée de vie liée au dernier descendant ; une tâche en file porte un handle de propriété, jamais un span vers la pile du créateur. Un mécanisme de fenêtres borne les arêtes parentes simultanément résidentes ; la taille de la fenêtre n'est ni un quota géométrique ni un plafond de sorties.

1. **Front et témoins.** Traiter les rectangles en vagues ; garder les masques séparés, les certificats de rejet et l'identité des sous-produits. Publier directement des plages de paires résiduelles dans des buffers bornés à offsets disjoints. Ne pas matérialiser toutes les 23,7 millions de paires du reçu 1 mm : deux IDs de 64 bits par paire feraient déjà environ 379 Mo, avant tout contexte. Si la file est pleine, le propriétaire poursuit localement ; aucun travail n'est abandonné ni bloqué derrière une file qui ne peut se vider.
2. **q3 : bloc de graines × sous-arbre de témoins.** Avant de construire toutes les boules, certifier sur un bloc X de graines propriétaires des bornes communes de puissance contre Z. Le ticket transmissible contient **ensemble** le compte strict et le curseur du préordre Z ; à une feuille ambiguë, diviser X avant de consommer Z. Chaque graine restante copie le ticket figé et traite les racines du suffixe, puis recherche la coquille sur l'index global. Les points de X restent témoins possibles des autres graines. Les contextes X/curseur sont les tâches partageables ; préparer la boîte de centres une fois par X, réutiliser un petit moteur privé, ne pas créer une pile complète par graine. Le [relais d'audit](../../morsehgp3D_v8/audits/q3_prefix_relay_20260921/README.md) et le [census float32 partagé](../../morsehgp3D_v8/docs/CENSUS_Q3_FLOAT32_PARTAGE_20260921.md) établissent des coutures utiles, pas un gain global sur LiDAR.
3. **q4 : cellules utiles et produits graine × cellule.** Sur l'arête, publier après construction des tâches indépendantes portant un bloc disjoint de graines et une racine de cellule vivante, avec le fragment exact parent conservé. Le parcours `LiveOnly` prouve l'intérêt du rejet des atlas sans feuille ; `Joined` montre comment certifier des produits, mais ses caches et bornes supplémentaires n'ont pas encore donné de gain stable. Une cellule au seuil `K−1` peut fournir un **certificat terminal de rejet** pour q3 et q4 ; elle ne fournit pas une frontière incomplète utilisable pour un balayage positif. Pour les cellules restantes, parcourir les nœuds Z disjoints puis agréger comptes et fragments sans double compter l'héritage. Étudier une représentation persistante des frontières par handles de sous-arbres immuables et deltas : elle viserait les 5,547 milliards d'IDs copiés du reçu, mais ses octets, visites et accès aléatoires doivent être mesurés. Le balayage d'une incidence `(graine, feuille)` reste atomique jusqu'à une preuve de partition des événements. Les contacts de puissance zéro, notamment aux frontières fermées de cellules, restent actifs ; garder exactement la règle actuelle d'affectation et de regroupement. À court terme, distribuer les blocs de graines d'une même arête avec atlas partagé est plus simple à prouver que distribuer un événement racine isolé.
4. **Sorties et FULL.** Chaque tâche émet vers un run privé avec clé de boule exacte, support, profondeur et coquille complète, puis fusion déterministe des runs. Garder toutes les **présentations de supports** dans le flux différentiel v8 ; le chemin industriel peut n'émettre qu'une boule canonique avec `q_min`, IDs intérieurs, coquille et incidences de facettes/parents utiles au FULL, **si** la couverture de toutes les clés est prouvée. Un compte intérieur de fragment n'est pas sa liste d'IDs : les recollecter une fois par clé distincte ou en conserver les nœuds possédés. Un digest n'est qu'un contrôle. Construire le catalogue commun et son atlas compact de blocs `(K, boule)` une fois, puis traiter les résolutions et graphes datés par fenêtres **sans fermer séparément des fractions d'un même niveau exact**. Les pivots, forêts minimales par date, multifusions et consultations historiques disposent de [prototypes v7 vérifiés](../../morsehgp3D_v7/docs/OBJETS_PARALLELES_TOUR_20260911.md) ; leurs preuves reposent sur de vrais census de petits nuages et doivent être requalifiées sous les flux et profils v9. Les événements q4 ne doivent pas être déduits des seuls triangles q3 acceptés.

**Raccord exact à résidence maîtrisée.** Dans le premier moteur v9,
`slots[W]` et `all` détiennent simultanément au moins `224P` octets de
capacités de présentations sur l'ABI lu par [B](CONTRE_AUDIT_B_RESIDENCE_CHAINE_20260922.md).
Des runs privés bornés, triés par `(BallKey, arité, support)` et fusionnés
avec pression de retour, gardent le même représentant minimal ; ils
doivent vérifier profondeur, coquille et doublons de **toutes** les
présentations d'une clé, puis recenser chaque clé une fois et attribuer
des BallIds stables. Le catalogue certifié reste consultable par BallKey
et par niveau rationnel exact. À l'ordre K, seules les boules dont
`p+q_min−1≤K≤p+u` peuvent être des terminaux du résolveur ; un index
immuable des clés actives à K peut réduire sa résidence, à comparer au
cache direct actuel `48·nextpow2(16n)` et à son reset par ordre. Ce
filtre ne supprime pas le catalogue global.

Les facettes d'un lot de même niveau peuvent être résolues par fenêtres
indépendantes contre l'état **avant** ce niveau. La fermeture exige
ensuite la connectivité de **tous** ses blocs par parents communs et la
publication des ancres seulement après le lot entier ; si le lot dépasse
la RAM, il faut spooler les blocs et réunir exactement leurs composantes
avant fermeture. Le seul tuilage des runs ne borne ni le disque ni la
sortie : `ChainResult.tower` exige aujourd'hui une tour complète en mémoire,
dont K1 représente au moins `216n` octets, soit 6,48 Go décimaux à 30 M
sites sur cette ABI. Une représentation adressable CSR/arène ou externe
avec budget de cache doit donc faire partie du contrat massif, y compris
son coût de lecture et d'export. Ces transformations préservent la
possibilité d'un calcul exact et parallèle ; ni une borne temporelle ni
la complétude des BallKeys n'en résultent sans oracle et mesures.

La file CPU doit accepter des tâches de granularité adaptative avec coût estimé par compteurs locaux (taille des produits, nœuds Z/C encore actifs), vol de suffixes **non visités** seulement et repli local si saturée. La fin exige zéro seed non attribuée, zéro tâche en file et zéro worker actif. La pression mémoire doit limiter **à la fois** parents vivants, frontiers, runs de sortie, tâches et copies d'IDs ; `Q+W` objets de contrôle ne borne ni leurs octets ni les payloads. Publier RSS et pics couplés réellement simultanés, pas une somme de maxima par worker. Faire d'abord une implémentation CPU qui permet de rejouer exactement les mêmes tâches en W1/W8/W48, puis introduire des lots GPU.

## Port GPU G4 à coût compté

Le GPU peut traiter des vagues de produits de boîtes et de témoins, puis compacter les produits indécis et les émissions via comptage, préfixe et écriture à offsets disjoints. Les tableaux de points, nœuds, permutations, formes et atlas utiles restent résidents pendant une fenêtre ; les événements et runs sont transférés **par lots**, jamais un aller-retour par graine ou support. La capacité du lot limite la résidence, puis une boucle reprend jusqu'à épuisement exact du travail. Les décisions de signe exigent des bornes conservatrices et un chemin entier exact ; une évaluation flottante incertaine va dans une file de repli exact, sans epsilon ni rejet. Pour le profil float32 original, les entiers exacts jusqu'à 1728 bits des prédicats isolés imposent d'estimer la fraction de replis et leur trafic avant de choisir CPU ou noyau GPU spécialisé. Pour u18, porter les bornes i128/limbes avec preuves de capacité et les cas d'égalité. Ne pas déduire l'occupation réelle des seuls registres de compilation d'un prototype v7.

Le candidat G4 doit publier pour chaque phase : préparation hôte, H2D/D2H, allocations et réutilisation des buffers, kernels, repli exact, CPU concomitant, temps mur froid/chaud et mémoire de pointe hôte/device. La somme des temps de service peut dépasser le mur en présence de recouvrement ; le mur contractuel est mesuré directement. Les tâches GPU ne reçoivent que des handles/offsets possédés. Le CPU conserve les cas rares difficiles et l'ordonnancement global ; une erreur de noyau annule le lot avant publication. La [route CUDA v7](../../morsehgp3D_v7/docs/PARALLELISATION_PAR_LOTS_20260911.md) contient des idées réutilisables — buffers résidents, numéro de lot, identité d'ordinal, absence d'aller-retour par requête — mais son prototype de terminale FULL n'est pas une qualification q3/q4 v9.

## Portes de décision avant optimisation de constantes

- **Exactitude et possession :** oracle rationnel exhaustif sur petits nuages, contacts/cosphéricités/doublons, propriétaire d'arête, seuil strict K−1/K−2, signatures complètes des supports, clés, profondeurs, coquilles et tour ; comparaison W1/W8/W48 et CPU/GPU sur les mêmes entrées. Mutants : ticket q3 sans curseur, cellule saturée utilisée comme fragment, exclusion d'un témoin d'un autre x, contact zéro écarté, parent libéré avant callback, événement q4 coupé à la mauvaise frontière.
- **Grand-livre :** conservation des masses par lane du front jusqu'aux sorties ; un descendant n'est ni perdu ni rejoué ; crédits de nœuds Z disjoints ; atlas interrompu compté dans `work.partition`, son `saturation_work.prefixes` n'étant qu'un sous-ensemble ; les IDs copiés et les tests de relais sont payés. Mesurer aussi longueur maximale de tâche et distribution des temps utiles par arête, pas seulement les attentes des workers.
- **Croissance et échéance :** d'abord deux tailles appariées puis trames entières de plusieurs séquences, avec et sans sol, K5/K10, s8/s10/s12, mêmes masques et mêmes coordonnées. Publier nombre de sites, sorties, chaque poste dominant, rapports parent/enfant y compris défavorables, temps complet et pic mémoire. Un exposant inférieur à 2 sur quelques coupes oriente l'architecture ; le jalon 1 s/100 ms exige la tour FULL et le profil d'entrée contractuel effectivement mesurés sur G4.

Ordre de réalisation recommandé : établir d'abord la base FULL bout-à-bout du [plan v9](https://github.com/Ludwig-H/E-HGP/blob/3595725a/morsehgp3D_v9/docs/PLAN_V9.md), avec catalogue et parents vérifiés, puis réduire les milliards de classifications et de copies de fragments q4 ainsi que les millions de préparations q3 sur **ce même appel complet**, en rendant les descendants d'une arête distribuables. Les preuves géométriques et les micro-captures peuvent avancer en parallèle de la base FULL. Ajuster ensuite le choix CPU/GPU et les constantes. L'index u18 du premier reçu sans sol prend des millisecondes, tandis que les consommateurs prennent plus de cent secondes : le prochain effort doit modifier le **travail total** et sa granularité, pas seulement accélérer la lecture de l'index.
