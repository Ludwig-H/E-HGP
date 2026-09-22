# Audit v8 → v9 : q3 par cellules locales de miniballes et frontières certifiées

22 septembre 2026. Moteur v8 lu à `a74e90f2`, ouverture v9 `3595725a` ; **architecture proposée, non implémentée ni qualifiée en v9**. La [décision v9 documentée](https://github.com/Ludwig-H/E-HGP/blob/3595725a/morsehgp3D_v9/docs/AUDIT_V8_SYNTHESE.md) poursuit d'abord le jalon de temps sur trame LiDAR entière **sans sol en u18/1 mm**, K5 puis K10 sur G4, puis 100 ms. Le float32 original reste le défaut d'entrée contractuel antérieur, dont le développement temporel v9 est suspendu. La trame brute entière reste une obligation distincte ; sa portée temporelle v9 doit être fixée. Aucun alignement des points, des anneaux ou des passages LiDAR n'est supposé.

## Diagnostic qui commande le choix

La [reprise u18 inventoriée dans l’ouverture v9](https://github.com/Ludwig-H/E-HGP/blob/3595725a/morsehgp3D_v9/docs/AUDIT_V8_SYNTHESE.md) est la première lecture. Sa trame **08/000000 sans sol entière, 39 885 sites à 1 mm**, K5/s8/W8, dure 104,63 s mur et 812,82 CPU·s pour le seul flux q3/q4 ; atlas saturant désactivé. Le [reçu v8 publié après cette première lecture](../../morsehgp3D_v8/receipts/u18_resume_20260922/ground_1mm_first/only_probe_01_s00_k5_w8.json) (SHA-256 `89be317e0459f179…`) publie 23 686 751 paires développées, 1 716 642 arêtes q3, **184 461 509 graines q3**, 153 036 427 rejets par l'atlas et encore **31 425 082 boules construites/census q3**. Le census prépare 1 126 261 494 bornes de nœuds ; il n'émet que 691 284 supports q3. La même ligne émet 158 496 supports q4 et 2 707 842 IDs de coquille q3+q4. Les quotients descriptifs sont 267 graines q3 et 45,5 boules recensées par émission q3 ; ils ne sont pas des probabilités ni des coûts unitaires.


Le registre sépare `1 716 642−1 545 198=171 444` arêtes **q3 seules**,
sans atlas partagé. Elles portent
`184 461 509−168 343 794=16 117 715` constructions/census. Sur les
arêtes q3+q4, les `168 343 794` localisations se répartissent en
`153 036 427` rejets atlas, `20 845` centres hors domaine et
`15 286 522` census après consultation dans le domaine. Réutiliser
seulement les feuilles q4 peut donc toucher **au plus 48,6 %** des
`31 425 082` census, même avant ses replis `Deep` et `Outside`. Les
arêtes q3 seules demandent une voie propre.

L'atlas par arête a donc déjà écarté 83,0 % des graines q3 de cette ligne ; il reste 1,545 million d'arêtes avec atlas, 38,8 millions de cellules créées et des milliards de classifications de blocs/points. [La voie actuelle](../../morsehgp3D_v8/src/pipeline/wspd_q34.cpp) prépare un cover par arête, consulte l'atlas q4 *graine par graine*, puis lance `census_q3_ball` pour les survivantes ; [ce census](../../morsehgp3D_v8/src/lanes/q3_ball_census.cpp) repart de la racine pour chaque boule et collecte la coquille dans une autre traversée globale si elle passe. Le travail d'atlas et celui des 31,4 millions de census ne disparaissent donc pas en ajoutant seulement des workers. Ces données sont une répétition d'une seule trame, un flux incomplet, sans segmentation, catalogue, parents, FULL ou GPU. À débit CPU logique identique entre hôtes, sans GPU ni aval et avec 48 CPU logiques utilisés parfaitement, les 812,82 CPU·s représenteraient encore **×16,9** le budget d'une seconde et **×169** celui de 100 ms. Ce sont des scénarios arithmétiques conditionnels, pas des bornes ni des prévisions de performance G4.

Le [census float32 partagé](../../morsehgp3D_v8/docs/CENSUS_Q3_FLOAT32_PARTAGE_20260921.md) et [l'audit du relais](../../morsehgp3D_v8/audits/q3_prefix_relay_20260921/README.md) apportent une preuve utile : un bloc de graines peut transmettre `(compte, curseur)` sans recommencer le préfixe, avec coquille globale. Ils portent sur **une arête** et des familles synthétiques, sans borne sur le nombre d'arêtes. Le brouillon global float32 non suivi `morsehgp3D_v8/src/pipeline/float32_q3_global.cpp`, examiné en lecture seule, développe encore les paires de chaque rectangle puis appelle le filtre et la voie q3 possédée par arête ; cette source non suivie n'est **pas** qualifiée par cet audit. La [mesure spatiale](../../morsehgp3D_v8/docs/Q34_MESURES_SPATIALES_20260921.md) montre déjà un exposant observé de 2,502 pour les bornes q3 de la trame brute vers sa moitié positive ; un gain local sur 8k/16k/32k ne suffit pas.

### Premier levier après la base FULL : feuille q4 exacte vers census q3


Sur le profil mesuré `GlobalBoxes`, `wspd_q34.cpp` construit encore un
`Q34EdgeCover` pour les 171 444 arêtes q3 seules ; leur voie n'en lit
que `edge_ids()`, car le census prend l'index global. Une entrée typée
`(index, arête propriétaire)` pourrait éviter ces covers, en les gardant
pour q4, les arêtes communes et `ScalarCover`. Le contrôle causal est
l'identité des supports/coquilles avec `cover_builds` réduit d'autant ;
ce retrait ne supprime à lui seul aucun census.

Le port entier u18 du `SharedPrefix` est le candidat suivant pour ces
16,1 millions de census. Pour l'arête fixe `ab`, poser `D=|b−a|²`,
`u=x−a`, `E=(b−a)·u`, `G=D|u|²−E²` et `H=D u−E(b−a)`.
Le centre d'une graine aiguë propriétaire est
`c=(a+b)/2+ξ H/D`, où `ξ=D(|u|²−E)/(2G)` satisfait `0<ξ≤1/3`.
En effet, `t=E/D∈(0,1)` et `r=|u|²/D≤min(1,2t)≤3t−2t²`, ce qui
donne `3(r−t)≤2(r−t²)`. Les extrêmes de H sur un
paquet de graines X sont donc une boîte rationnelle conservatrice de
centres, sans construire toutes leurs boules. Un ticket possédé
`(compte strict, curseur Z)` transmet les témoins à ses enfants ; une
feuille Z ambiguë divise X **avant** consommation, puis chaque singleton
reprend le suffixe Z et collecte la coquille globalement. Un minorant
de puissance `≥0` exclut un nœud du **compte**, pas de la coquille :
`a=(1,2,0), b=(5,10,0), x=(9,2,0), z=(5,0,0)` donne un triangle
aigu propriétaire, centre `(5,5,0)` et z exactement sur la sphère.
Dans le domaine `[0,M]^3`, `M=262143`, une représentation commune des
centres a un numérateur `<30M³<2^59`, et les bornes de puissance
proposées `<234M⁴<2^80`. Pour construire un centre individuel, annuler
`D` **avant** les produits :
`c=[G(a+b)+(|u|²−E)H]/(2G)` ; son numérateur est
`<54M⁵<2^96`. L'évaluation littérale du produit
`D(|u|²−E)H` peut dépasser i128, même pour une graine valide :
`a=(0,0,0), b=(M,M,0), x=(M,0,M)` donne `4M⁷>2^127−1` sur l'axe z.
Ces bornes certifient les formes **réduites**, pas une réécriture
arbitraire de `ξH/D`. Un paquet
borné et un budget d'effort commun avec repli individuel limitent le
surcoût par graine, sans plafonner les candidats. Comparer les visites
X×Z, le travail individuel restant, les coquilles, le temps total et
les sorties FULL exactes sur trames entières avant d'attribuer un gain.
L'[oracle autonome](check_q3_shared_u18_20260922.py) passe en Python
normal et `-O` : 213 paquets, 3 492 graines aiguës propriétaires,
13 968 comparaisons de puissance, le contact ci-dessus et le témoin de
débordement. Les IDs implicites `a=0,b=1,x≥2` règlent les égalités de
plus longue arête dans l'oracle. Il vérifie les enveloppes rationnelles
sur les graines effectivement tirées, pas la construction d'extrêmes
depuis une boîte X de l'index ; il ne teste pas non plus le relais
du moteur v9 ni sa croissance sur LiDAR.

Avant un nouvel atlas par ancre, une optimisation à risque limité peut réutiliser **les feuilles exactes** de l'atlas q4 déjà payé pour une arête `ab`. [L'objet `Q4LocalFragment`](../../morsehgp3D_v8/src/lanes/q4_local_partition.hpp) garantit, sur sa cellule fermée, le **compte exact des nœuds déjà certifiés intérieurs** pour le même cover et une frontière disjointe complète de nœuds encore ambigus ; les autres nœuds sont strictement dehors. Sa forme locale a le même signe que `4Q` fois la puissance de la sphère de centre `c` passant par `a,b`, avec `Q>0` ([identité](../../morsehgp3D_v8/docs/Q3_CERTIFICAT_ATLAS_20260921.md)). Si le circumcentre q3 de `abx` est dans cette feuille, la positivité et la propriété de `ab` assurent que sa boule fermée entière est dans le cover de `ab` : en posant `D=|ab|²`, on a `R²≤D/3` et `|c−(a+b)/2|²=R²−D/4≤D/12`, donc `R+|c−(a+b)/2|≤√(3D)/2<√D`, le rayon du cover. Les sites strictement intérieurs **et tous les contacts de coquille** y sont. Le census q3 peut donc démarrer avec ce compte exact, tester seulement les sites de la frontière, saturer à K−1 ou conserver sa profondeur exacte, et construire la coquille complète dans cette même frontière. Les endpoints `a,b,x` ont signe zéro et doivent rester disponibles. Le fragment ne livre toutefois que le **compte** des intérieurs uniformes, pas leurs IDs : le catalogue FULL exige des handles vers ces nœuds ou une recollecte d'intérieurs une fois par boule canonique distincte, coût inclus. Ce port exige le **même nuage/index, la même arête, le même cover et la même cellule** ; une simple valeur numérique de compte détachée de son propriétaire n'est pas une preuve.

La distinction des états de [l'atlas](../../morsehgp3D_v8/src/lanes/q4_local.cpp) est décisive. `Leaf` a un fragment exact utilisable. `Deep` ne conserve plus son fragment : s'il certifie au moins K−1 sites, il rejette q3 immédiatement ; s'il ne garantit que K−2, seuil suffisant pour q4, il **ne décide pas q3**. Le nouveau certificat `saturate_deep` atteint K−1 mais n'a volontairement aucune frontière complète : il rejette q3, sans pouvoir amorcer un census accepté. Dans les cas `Deep` insuffisants, il faut raffiner un état complet encore possédé ou reprendre un census global à zéro. `Outside`, notamment sous le domaine `Positive` q4, ne dit rien sur q3 : un circumcentre q3 peut être hors du domaine q4 même si sa graine est valide ; repli global obligatoire. Cette discipline conserve aussi la coquille des préfixes et des frontières, que le seul `certified_inside_count` public ne peut pas fournir. Mesurer séparément feuilles q3 accessibles, profondes à K−2 seulement, profondes à K−1, dehors, masse de frontière et rescans de repli. Le [certificat d'atlas q3 v8](../../morsehgp3D_v8/docs/Q3_CERTIFICAT_ATLAS_20260921.md) ne faisait que rejeter : il n'est pas déjà ce raccord de census.
Une API typée de consultation de feuille, liée à l'atlas possédé, est donc à porter ; l'actuelle API publique ne livre que le compte certifié et ne permet pas de réutiliser la frontière. La cellule/propriété de la réponse doit rester figée pendant tout le census et la collecte.

## Lemme exact : une profondeur de miniballe est un rang de site

Soit un triangle strictement aigu de sites distincts `a,b,x`. Sa boule q3 est la **miniballe des trois sommets** : son centre `c` est le circumcentre dans leur plan, situé dans l'intérieur du triangle. Sa profondeur stricte est le nombre de sites `z` tels que

`Delta(a,z;c) = |z-c|² - |a-c|² = |z|² - |a|² - 2 c·(z-a) < 0`.

La dernière expression est **affine en c** pour `a,z` fixés. Les sommets `a,b,x` ont Delta=0 et ne sont jamais crédits ; les autres sites, y compris les graines invalides et celles d'autres arêtes, restent témoins. Pour K donné, q3 est rejeté dès que la profondeur atteint `h3=K−1`. Cette identité n'exige ni q2 accepté ni Delaunay. Elle est valable aussi pour q4 avec son seuil indépendant `h4=K−2`, sous la positivité et le contrat q4 propres à cette voie. Elle ne transforme aucun rejet q3 en rejet q4 sans vérifier le seuil q4.

Prendre une **petite cellule fermée C de centres** et l'index spatial immuable `Z`. Un nœud `N` du même index, de population `|N|`, est :

- `Inside` si un majorant certifié de Delta sur **C × boîte(N)** est strictement négatif ; ses `|N|` sites sont intérieurs à toutes les boules passant par `a` de centre dans C ;
- `Outside` si un minorant certifié est strictement positif ; ses sites ne sont ni intérieurs ni sur la coquille ;
- `Active` sinon, notamment en cas de contact possible ou de borne insuffisante.

Les nœuds retenus doivent former une partition **disjointe et complète** des sites originaux. La somme `I(C)` des populations `Inside` est un compte commun exact de ces nœuds, pas la profondeur exacte de chaque boule. Si `I(C)≥h3`, toutes les graines q3 de la cellule sont rejetées et l'on peut arrêter sa classification ; il s'agit alors d'un **certificat terminal**, sans fragment de sortie. Si `I(C)<h3`, terminer toute la partition et garder le fragment `(a,C,I(C),Active)` : les nœuds `Outside` sont supprimés du travail ultérieur. Aucune borne égale à zéro n'est un crédit ou une exclusion de coquille. Le nouvel [objet saturant v8](../../morsehgp3D_v8/docs/REPRISE_U18_ET_ATLAS_SATURANT_20260922.md), publié après cette lecture, applique la même distinction essentielle « fragment complet ou certificat profond » sur son atlas par arête ; ses compteurs de préfixe sont déjà inclus dans le travail de partition.

**Conséquence de génération, plus forte qu'un filtre.** Pour tout support q3 `a,b,x` dont le centre exact `c` est dans C, `Delta(a,b;c)=Delta(a,x;c)=0`. Ses deux autres sommets ne peuvent donc appartenir ni à `Inside` ni à `Outside` ; ils sont dans `Active`. Une cellule de frontière petite peut proposer uniquement les paires `b,x` de ses sites actifs, calculer chaque centre exact, puis vérifier acuité/miniballe, profondeur et propriétaire. Une cellule de frontière encore grande se subdivise ; le grain terminal est un choix de coût, **jamais une limite de recherche**. Les sites `Inside` peuvent être ajoutés au compte de chaque candidat une seule fois ; les sites `Active` sont testés exactement. Pour une boule admise, toute sa coquille est dans `Active` : `Inside` et `Outside` ont des signes stricts, si bien qu'une seconde traversée **globale** n'est pas nécessaire avec ce fragment complet. Un fragment seulement saturé ne permet ni ce census ni cette collecte.

Ce résultat décrit un générateur local de supports **k-Gabriel** positifs autour de l'ancre, fondé sur leurs miniballes et des cellules certifiées. Il ne demande pas de construire une mosaïque globale de Voronoi d'ordre K ni de supposer que les faces q3 proviennent de q2 déjà acceptées. La génération directe et l'optimisation des fragments sont proposées ; leur coût sur LiDAR n'a pas été mesuré.

## Exemple calculable et point de rupture d'une optimisation naïve

Dans le plan `z=0`, prendre `a=(0,0)`, `b1=(10,0)`, `x1=(5,8)`, `b2=(0,10)`, `x2=(8,5)`, et deux autres sites `z1=(1,0)`, `z2=(0,1)`. Les deux triangles sont aigus ; `ab_i²=100` est strictement leur plus longue arête, les deux autres carrés valent 89. Leurs centres exacts sont `(5,39/16)` et `(39/16,5)`. Dans la cellule fermée `C=[39/16,5]²`, les majorants de Delta pour `z1` et `z2` valent chacun `−31/8`. Deux témoins **communs à deux arêtes distinctes** rejettent donc les deux graines à K3, sans construire leurs boules. Le calcul direct donne, selon le centre, les paires de puissances `(−9,−31/8)` et `(−31/8,−9)`. Ce petit cas est une vérification algébrique de sûreté/amortissement, pas un benchmark.

Une palette de quelques voisins de `a` peut tester ces demi-espaces très vite, mais **ne certifie pas à elle seule toutes les cellules** : une acquisition de surface sans sol peut laisser un côté normal presque vide. Une palette insuffisante déclenche la classification exacte par blocs ou un repli individuel ; elle n'autorise jamais l'acceptation. Le voisinage euclidien k-NN de `a` n'est pas non plus un filtre de complétude des arêtes : des points arbitrairement proches de `a` peuvent être situés hors de la miniballe d'un triangle ayant un long côté. Ne générer que les faces du graphe k-NN ou que les faces q2 acceptées perdrait des q3 valides.

## Deux niveaux de tâches, sans arrangement massif

1. **Propositions économiques.** Réutiliser le front et l'index propriétaires v8 pour conserver les produits q3 résiduels, sans développer aussitôt toutes leurs paires. Générer des tâches de petits produits de supports `(A,B,X)` avec une enveloppe conservatrice des centres de triangles *positifs et propriétaires*. Tant que les deux extrémités ne sont pas singleton, l'enveloppe convexe des trois boîtes reste sûre mais large ; la formule resserrée `c=m+xi h`, `0<xi≤1/3`, ne vaut qu'après avoir fixé l'arête propriétaire. Voir [la preuve A](../../morsehgp3D_v8/audits/q3_seed_block_power_20260921/README.md) et le port float32 conditionnel non suivi `morsehgp3D_v8/src/core/float32_q3_owned_block.cpp`. L'ancienne propriété de plus longue arête doit encore être tranchée exactement, égalités d'IDs comprises.
2. **Certificats locaux réutilisables.** Pour une ancre `a` devenue fixe, tester d'abord une palette bornée de sites distincts et la cellule de centres. L'identité affine donne un maximum exact aux coins d'une boîte C pour chaque site ponctuel. En cas d'échec, interroger les nœuds Z avec des bornes extérieures puis un repli exact ; ne préparer un fragment complet que si la cellule survit et si sa réutilisation entre arêtes/graines peut payer sa construction. Mettre en cache seulement `(identité du nuage, ancre, cellule, preuve, fragment éventuel)` ; le cache peut être évincé sans effet sémantique. Une cellule 3D ancrée partage les preuves entre arêtes ; pour les cas difficiles, raffiner dans le plan médiateur 2D de l'arête, en réutilisant les fragments parentaux. Il s'agit de cellules locales visitées à la demande, pas de toutes les cellules du rang d'ordre K.
3. **Relais et émission.** Un fragment complet transmet le compte `I(C)` et ses blocs actifs, avec un seul propriétaire de cette partition. À défaut de fragment complet, la voie sûre est le [relais q3 v8](../../morsehgp3D_v8/audits/q3_prefix_relay_20260921/README.md) `(compte, curseur)` lié au même index/ancre/arête, ou un census intégral repartant de zéro ; ne jamais redémarrer de la racine avec un crédit. Chaque triangle choisi doit avoir une **tuile propriétaire demi-ouverte unique** pour son centre rationnel, tandis que les tests de preuve portent sur les cellules fermées : contacts aux frontières conservés, doublons de génération éliminés. Les faces externes maximales du domaine racine appartiennent à la dernière tuile, sinon un centre exactement sur cette borne serait perdu. Pour le raccord au front actuel, choisir `a` comme l'endpoint déterministe de **l'arête maximale propriétaire** ; choisir le plus petit ID des trois sommets sans repenser les produits d'arêtes pourrait perdre une présentation. La preuve de couverture doit porter sur cette même arête.

Puisque le centre positif est dans son triangle, toutes ces cellules peuvent être bornées initialement par la boîte globale finie du nuage. Il serait toutefois **quadratique dès la préparation** de classifier tous les sites à la racine de chaque ancre ; les tâches doivent être créées à la demande par les produits/sous-domaines rencontrés. **L'incidence q3** d'un support de profondeur exacte `p` apparaît pour la première fois au niveau **K=p+2** ; la même boule peut avoir un `q_min` plus petit et entrer plus tôt dans le catalogue. Un calcul au seuil maximal 9 peut donc alimenter K=1..10 sans dix censuses ; q4 a son propre premier niveau d'incidence `p+3`. Catalogue, incidences et parents FULL restent à construire et à chronométrer.
La condition de complétude des tâches est explicite : chaque triangle positif accepté possède une arête maximale propriétaire ; sa paire survit au masque q3 exact du front, son troisième sommet appartient à un bloc X couvrant cette paire, et l'enveloppe de ce bloc contient son vrai centre. Si une de ces trois assertions n'est pas prouvée par le port, le générateur doit conserver un chemin exhaustif. Un budget de tâches ou de mémoire n'autorise jamais l'omission du produit restant.

## Borne de coût honnête et contre-régimes

Soient `T` les produits et cellules réellement visités, `B` tous les tests de borne cellule×nœud Z **y compris préparations interrompues**, `F` les IDs de frontière copiés ou initialisés entre fragments, `m_t` la population active réelle de la cellule terminale t, `R=Σ_t binom(m_t,2)` les paires q3 proposées localement, `V` les tests exacts de profondeur/coquille dans les fragments, `P` les opérations de palettes, `S` les octets d'émissions et de catalogue. À structures à coût constant et index construit en `O(n log n)`, le compte à viser est de la forme `O(n log n + T + B + F + P + R + V + S)` ; les filtrages exacts, allocations, tris, transferts et replis doivent être détaillés plutôt que cachés dans cette notation. Une limite `m_t≤g` donne seulement `R≤(g−1)Σm_t/2` : elle **ne borne pas** la somme des populations actives répétées entre cellules ou ancres. La condition concrète de sous-quadratique en régime est que **chacun** de `T,B,F,P,R,V,S` croisse sous `n²` sur les trames et coupes appariées, sorties comprises. Cette condition n'est pas prouvée par la v8 ; le reçu 1 mm montre précisément des milliards d'IDs d'atlas copiés malgré des rejets utiles.

Il n'existe d'ailleurs aucune promesse universelle sous-quadratique **pour le flux exhaustif de présentations** : sur n points cocycliques en position symétrique, une fraction constante des `binom(n,3)` triangles est aiguë, tous ont profondeur stricte zéro et la même coquille de n sites. Leur émission explicite est déjà cubique. Le FULL par boules peut regrouper cette famille sous une clé et n'a pas à imposer ce flux comme sortie ; il doit alors prouver qu'aucune **boule canonique utile** n'est perdue et calculer `q_min`, intérieurs et plateaux. Les dégénérescences de grille et les contacts doivent être testés, jamais effacés par une hypothèse de position générale. Sur des surfaces LiDAR, beaucoup de sites peuvent aussi rester ambigus près des mêmes cellules ; le grain fixe et la palette ne sauveraient alors ni `R` ni `B`. Publier ces cas sans les confondre avec l'objectif empirique sans-sol/avec-sol.

## Parallélisme et premier port vérifiable

Une tâche v9 peut posséder `(nuage/index immuable, ancre, cellule ou produit, seuil maximal, fragment parent certifié)` ; moteurs de bornes, buffers de frontière, sorties et registres sont privés au worker. La division des cellules fournit du travail aux CPU même si une arête domine ; une file bornée ne peut ni perdre un suffixe ni attendre indéfiniment sa place. Les fragments complets sont immuables et à durée de vie possédée ; les maxima de mémoire simultanés, octets de cache et IDs actifs copiés sont publiés. Les bonnes disciplines déjà disponibles sont l'index partagé et les tickets figés v8, ainsi que le principe de baux de lots de [v6](../../morsehgp3D_v6/src/gpu/lot_ring.hpp) pour les transferts ; aucune performance GPU ne s'en hérite.

Sur G4, les tests nombreux et indépendants `(cellule, nœud Z)` et les paires de petites frontières se prêtent à une compaction en lots GPU avec index résident. Pour u18, un filtre numérique à erreur dirigée doit précéder un repli entier exact ; pour float32 original, le repli exact des [prédicats natifs](../../morsehgp3D_v8/docs/BOULES_FLOAT32_Q3_Q4_20260921.md) reste indispensable. Une indécision GPU devient une tâche exacte CPU ou un chemin exact device qualifié, jamais un signe deviné. Saturation de file, débordement de buffer ou manque de mémoire conserve toutes les tâches restantes ; les transferts et la compaction entrent dans le chrono. Il serait prématuré de porter les 1,1 milliard de bornes q3 v8 telles quelles sur GPU : la priorité est d'en **supprimer** une grande part.

Après la base bout-à-bout V9-1 du [plan ouvert](https://github.com/Ludwig-H/E-HGP/blob/3595725a/morsehgp3D_v9/docs/PLAN_V9.md), premier port d'optimisation conseillé, par ordre de preuve : (i) le raccord d'une feuille q4 exacte au q3, avec les replis ci-dessus ; (ii) un fragment q3 local complet/terminal sur cellule fermée, oracle rationnel et contacts ; (iii) génération des paires depuis la frontière, centre exact et propriétaire unique ; (iv) comparaison au flux v8 complet sur petits nuages, sorties et profondeurs, puis mêmes entrées sans-sol 1 mm ; (v) partage de cellules entre arêtes et équipe CPU ; (vi) lots GPU. Ces expériences peuvent démarrer indépendamment sans remplacer la porte FULL. Le profil float32 ne revient qu'après réouverture explicite de cette voie. Chaque étape enregistre les masses de produits résiduels, cellules, `B,F,P,R,V`, supports/coquilles, cache utile, RSS/pics, appels exacts et temps CPU/mur par worker. Tester d'abord 08/000000, 000100, 000200 entières puis plusieurs **séquences** ; pour la croissance, utiliser scène entière, moitiés et quarts du protocole, avec masque sans-sol fixé avant les coupes, puis le brut. Ce sont des diagnostics : seul un lanceur FULL sur trames entières peut revendiquer 1 s ou 100 ms.

### Références de source gelées pendant cette lecture

HEAD lu : `a74e90f2`. Le travail u18 de reprise était alors non commis, puis publié à `3f0d188f` ; le brouillon float32 lu reste non qualifié par cette note. SHA-256 des fichiers utilisés : `wspd_q34.cpp` `79ae04fe505671ab…`, `q3_ball_census.cpp` `1f72e612d7d19a66…`, `float32_q3_global.cpp` `444386739b40de36…`, `float32_q3_owned.cpp` `1de600cdd7471f68…`, reçu 1 mm cité `89be317e0459f179…`. Les chiffres du reçu sont ceux de sa ligne JSON, non d'une relance effectuée par cet audit. GCP non utilisé.
