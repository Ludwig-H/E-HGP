# q4 : partager le parcours des graines et des cellules

21 septembre 2026. **Étude mathématique et architecture prospective** après
la question du [journal de coordination](../../audits/COORDINATION_MORSEHGP3D_V8.md).
Le produit 33 est en qualification ; aucun port de cette piste, changement
de défaut, expérience de performance ou résultat GPU n'est revendiqué ici.

## 1. Ce qui peut être éliminé avant de construire les familles

Le [raccord Local28](../src/lanes/q4_local.cpp) prépare déjà un atlas par
arête, puis énumère les graines aiguës propriétaires et construit leur
famille avant de parcourir l'atlas. Beaucoup de ces droites ne rencontrent
finalement aucune feuille potentiellement admissible.

Pour un nœud spatial X et une cellule fermée C, si les formes de **tous**
les sites de X sont strictement positives partout dans C, ou toutes
strictement négatives, aucune graine de X ne peut appartenir à la coquille
d'une boule centrée dans C. Ce produit X×C peut être écarté ensemble.
Si tous les domaines de centres possibles sont ainsi écartés, toute la
population X est rejetée comme source de graines, avant les familles.

Ce n'est pas un retrait des témoins : les sites de X restent dans l'index,
le cover, les comptes et les coquilles des autres graines. Ce n'est pas non
plus un rejet q3. Aucun accès q4 ne dépend d'une acceptation q2 ou q3.

## 2. La borne existe déjà dans la géométrie locale

Poser D=|b−a|², w=2x−a−b ; p,q sont les deux vecteurs entiers de la base
du plan médiateur préparée par `Q4LocalGeometry`. Pour les coordonnées
de centre t=(ξ,η), la boule passant par a,b a :

$$c(t)=\frac{a+b+p\xi+q\eta}{2},\qquad L_x(t)=|w|^2-D-2w\cdot(p\xi+q\eta)=4\bigl(|x-c(t)|^2-R(t)^2\bigr).$$

La famille de la graine x est donc exactement la droite Lx=0. Pour Q=2⁴⁴,
écrire les coordonnées dyadiques de C comme α=Qξ, β=Qη et poser :

$$F(w,\alpha,\beta)=Q L_x(t)=-QD+\sum_{i=1}^{3}\bigl(Qw_i^2-2T_iw_i\bigr),\qquad T_i=p_i\alpha+q_i\beta.$$

Pour tout w, F est affine en (α,β). Les extrema sur X×C se ramènent donc
aux **quatre coins de C**, puis aux extrema en w à chacun de ces coins.
Cet échange est valide pour le minimum comme pour le maximum : pour chaque
site, l'extremum affine est atteint à un coin, puis on prend l'extremum sur
tous les sites. Il ne suppose pas que le coin soit le même pour tous les x.

À coin fixé, chaque coordonnée w appartient à un intervalle entier [l,u]
et la fonction est une quadratique convexe séparable. Le maximum exact
sur l'intervalle continu est atteint à une extrémité :

$$M_i=\max\bigl(Ql^2-2T_i l,\;Qu^2-2T_i u\bigr).$$

Pour le minimum, prendre la valeur à l si T_i<Ql, à u si T_i>Qu ; sinon
le minimum continu est −T_i²/Q. Une borne inférieure entière sûre est :

$$m_i=-\left\lceil\frac{T_i^2}{Q}\right\rceil=-\left(\left\lfloor\frac{T_i^2}{Q}\right\rfloor+\mathbf{1}_{T_i^2\bmod Q\ne0}\right).$$

Additionner les trois bornes et −QD, puis prendre le minimum et le maximum
sur les quatre coins de C. L'arrondi vers le bas peut seulement conserver
un produit inutile ; il ne peut pas supprimer un contact. La relaxation
continue reste sûre pour les sites u16, même si w a une parité imposée.
Une éventuelle spécialisation au réseau entier serait distincte et à payer.

C'est précisément le raisonnement déjà utilisé par la méthode privée
[`Q4LocalGeometry::node_bounds`](../src/lanes/q4_local_partition.cpp), où
le nœud représente actuellement des témoins Z. Un nœud de graines X donne
la **même** boîte et la **même** formule. Le port devrait partager ce noyau
de bornes, pas en dupliquer une deuxième implémentation.

### Arithmétique

Sous u16, M=65535, |w_i|≤2M, |p_i|,|q_i|≤M et |α|,|β|≤2Q. Les limites
déjà établies pour les partitions sont donc inchangées :

$$|T_i|\le4MQ,\qquad T_i^2\le16M^2Q^2<2^{124},\qquad |F|\le63M^2Q<2^{82}.$$

Promouvoir en i128 avant les produits par α/β, le carré T_i² et Qw_i².
Ne pas arrondir une division négative vers zéro. La cellule doit rester
dans le domaine dyadique validé de Local28, de profondeur au plus 44.
Pour une graine singleton, la borne affine existante `bounds(form(x),C)`
reste préférable ; il n'est pas nécessaire de refaire les minima quadratiques.

## 3. États de l'atlas, contacts et domaine Positive

Le parcours doit distinguer les quatre états immuables actuels :

- `Outside` : aucun centre d'un q4 positif propriétaire n'est possible.
- `Deep` : le compte uniformément strictement intérieur atteint K−2
  sur la cellule **fermée**. Toute graine peut être écartée dans cette cellule.
- `Leaf` : le compte de base est inférieur au seuil, mais les témoins
  actifs peuvent encore rendre le centre profond ; ce n'est pas une admission.
- `Branch` : consulter ses enfants ; le fragment parental a été libéré.
  Ne pas essayer de reconstruire un compte en dereférençant ce fragment.

Un bit immuable « possède une feuille vivante descendante », préparé en
postordre, permettrait de court-circuiter une branche dont tous les enfants
sont `Deep` ou `Outside`. Un atlas sans feuille vivante rejette immédiatement
toute l'arête q4. Payer ce calcul et ne pas appeler une feuille vivante
« non profonde partout » : seuls certains centres peuvent y être admissibles.

Pour le nouveau rejet de produit, exiger **minimum>0 ou maximum<0**.
Ne jamais remplacer ces signes par ≥/≤ : une droite touchant seulement
un coin, un côté ou une frontière commune peut porter un vrai support.
Les bornes se calculent sur les cellules fermées ; les conventions
`owns_right`/`owns_top` servent uniquement à attribuer l'émission finale
à une feuille. Elles ne doivent pas couper prématurément une tangence.

La sûreté du mode `Positive` reste celle de son parent original : domaine
construit avec **toutes** les complétions de la lentille fermée, y compris
les complétions obtuses et les égalités de longueur. Le second sommet y
d'un tétraèdre peut être hors du bloc X courant et ne pas être une graine
aiguë. Ne pas recalculer le domaine avec X seul ou les graines survivantes.
Le test « moins de deux complétions » concerne la population globale de
ce parent, jamais la taille de X. Une projection dégénérée conserve le
domaine sûr existant ; elle ne justifie pas un domaine vide.

Le cercle disque et le hull Positive sont des surcouvertures des centres
de supports **q4 positifs propriétaires**. Le nouveau filtre peut en tirer
une condition nécessaire avant de certifier chaque support, mais les tests
finaux propriétaire, positivité et canonisation restent obligatoires. Ce
raisonnement ne s'étend pas automatiquement à q3 ou à toutes les racines
du balayage d'une face non positive.

## 4. Deux parcours à distinguer pour ne pas déplacer le coût

### A. Premier filtre négatif, simple mais pas nécessairement rentable

Conserver le DFS de graines et ses rejets de lentille/angle par boîtes.
Avant de descendre un bloc X, demander à l'atlas si **au moins une** feuille
vivante peut rencontrer une de ses droites. La recherche dans l'atlas saute
`Deep`/`Outside`, puis les produits de signe strict ; elle peut s'arrêter
au premier contact potentiel. Si elle répond non, sauter tout le sous-arbre X.
Sinon continuer le DFS et les appels individuels existants sur les survivants.

C'est un filtre exact, sans nouvelle liste de faces. Mais il peut reparcourir
les cellules pour chaque descendant de X, puis encore pour la vraie graine.
Une boucle plate « toutes les cellules × tous les blocs X » est précisément
le terme à éviter. Il faut publier ce surcoût, même si moins de familles
sont ensuite construites ; ce premier raccord ne partage pas tout l'aval.

### B. Vraie descente des produits X×C

Traiter des descripteurs de deux nœuds du même parent immuable. Après les
rejets d'état, les conditions nécessaires de graine et la borne ci-dessus :

1. Si le produit est exclu, terminer ce descripteur.
2. Sinon scinder **un seul facteur** : les deux enfants spatiaux de X,
   ou les quatre enfants de C. Leur union couvre exactement le produit.
3. Lorsque X est singleton et C est une feuille, vérifier l'angle et
   l'arête propriétaire, construire la famille si nécessaire et appeler
   directement le balayage de **cette feuille**, avec son fragment exact.

Le choix du facteur à scinder est une décision de coût à mesurer. Il ne
doit jamais transformer une ambiguïté en rejet ni imposer un quota de
graines/cellules. Les rejets géométriques propres à X sont réutilisables
quand seul C change, en conservant explicitement leur contexte.

Ne pas appeler `engine.seed(x)` depuis chaque paire terminale : cette entrée
repart à la racine de l'atlas et pourrait reproduire les mêmes émissions
plusieurs fois. Le raccord terminal doit réutiliser le `sweep(fragment,...)`
local, son compte non saturé, son clipping, tous ses témoins actifs et
l'attribution finale par cellule. Une même graine peut rencontrer plusieurs
feuilles : chaque couple graine/feuille doit être traité une seule fois.
Tous les IDs de coquille de la feuille restent recherchés, pas seulement
les sites du bloc de graines X.

Cette version peut construire une famille plusieurs fois pour une même
graine atteinte par plusieurs branches C. Mesurer les constructions réelles,
ou organiser un regroupement/caching possédé et en payer le stockage ; ne
promettre ni « une préparation par graine » ni ce partage gratuitement.
Matérialiser tous les couples de conflit, ou recopier une liste complète
de cellules à chaque fils X, pourrait recréer une mémoire et un travail
proportionnels au produit initial. Une DFS de descripteurs évite cette
matérialisation, sans garantir un petit nombre de visites.

## 5. Interface minimale et coût total

Le parent peut rester `Q4LocalAtlasPtr` : il possède déjà la géométrie,
le cover et le même index global. Ajouter une requête interne de borne
sur nœud spatial/cellule, partageant `node_bounds`, et un accès en lecture
aux états/enfants/fragments des feuilles suffit au raisonnement. Un éventuel
accès public doit valider IDs et cellules ; une vue empruntée ne dépasse
pas la durée de vie de l'atlas possédé par l'appel ou la tâche.

Pseudo-interface prospective : `run_q4_local_seed_block_candidates(atlas,
seed_node_id, consumer)`, avec buffers et compteurs privés. Le plan partagé
reste immuable ; aucun nuage, index, atlas ou cover n'est copié par bloc.
On peut commencer par la racine globale X. Réutiliser les `cover_nodes()`
disjoints est aussi sûr après la preuve de couverture des graines propriétaires,
mais leur parcours/préparation doit rester explicite et payé.

Pour une DFS qui scinde un seul facteur à la fois, la pile pendante est
bornée par les hauteurs, pas par le nombre de graines :

$$\#\mathrm{cadres}\le1+h_X+3h_C\le1+48+3\cdot44=181.$$

Ce n'est pas une borne sur le travail. Elle suppose exactement l'index u16
et l'atlas de profondeur au plus 44 existants ; une autre structure impose
une nouvelle preuve. Ne pas allouer cette pile par graine ou matérialiser
tous les produits pour la parallélisation massive. Des tâches plus fines
peuvent posséder le parent et des IDs de nœuds, avec des buffers par worker ;
la jointure, l'annulation et la mémoire simultanée resteraient à qualifier.

Noter V le nombre de produits visités et I le nombre de couples
graine/feuille atteints. Le coût doit inclure construction de l'atlas,
V bornes, contrôles/constructions réellement répétés, puis les lectures,
tris et coquilles de I balayages locaux. V et I peuvent rester grands :
aucune borne sous-quadratique globale ne découle d'un test de bloc constant.

Compteurs proposés : produits visités, rejets `Deep`/`Outside`, rejets de
signe positif/négatif, zéros conservés, splits X/C, populations de graines
écartées **sans double compte**, couples terminaux, graines distinctes si
réellement suivies, répétitions de famille, lectures/tris/payload, pile et
capacité simultanée. La somme des populations sur plusieurs cellules n'est
pas un nombre de graines distinctes ; l'étiqueter comme travail de produit.

## 6. Conditions avant un port et avant une conclusion de gain

- Oracle rationnel indépendant de la puissance aux coins/intersections ;
  boîtes contenant leur sommet quadratique, bornes fractionnaires et u16 extrêmes.
- Tangences en coin/côté, grandes coquilles, frontières communes, cellules
  dégénérées ; un mutant supprimant l'égalité doit perdre une vraie sortie.
- Atlas entier `Deep`/`Outside`, branche sans feuille vivante, budgets de
  construction laissant une feuille ; une feuille n'est jamais abandonnée
  parce que sa partition était coûteuse.
- Supports q4 avec q3 refusé, complétion obtuse et second sommet hors de X ;
  sorties égales à la référence Local28, pas seulement nombres de familles.
- Mesures n8k/16k/32k et régimes LiDAR réels, coût atlas inclus, comparaison
  du filtre A et du parcours B, cas sans gain conservés. Les graines retirées
  ne suffisent pas à conclure si V, copies ou reconstructions augmentent.

La preuve permet cette exploration après qualification de 33. La priorité
relative à l'héritage des témoins entre produits dépendra du coût réellement
dominant, notamment en K10. Aucun gain, backend GPU, tour FULL ou contrat
50k/plusieurs dizaines de millions de points n'est acquis par cette note.

## 7. Ce que les coûts discrets de 32 permettent de prioriser

Source unique de cette comparaison : les douze reçus clos de
[lidar_86twby55](../receipts/q34_indexed_20260921/lidar/lidar_86twby55/COMPLETION.json),
scan0, n8k/16k/32k, K5/10, s8, quatre workers, RectanglePair+Boxes,
Local28/Window30. Relecture historique du collecteur32 en Python normal
et avec `-O` : PASS, douze commandes, 206 sources épinglées. Aucun benchmark
réexécuté, aucune preuve de 33 utilisée dans les chiffres ci-dessous.
Le [manifeste](../receipts/q34_indexed_20260921/lidar/lidar_86twby55/MANIFEST.json)
a pour SHA256 `2af1415b588f0466174ed5bdd3facc9a630afb70bba0fbca6dec1aab6483b293`.
Les compteurs sont ceux de `row.work.local28` ou `row.work.window30`.

### Local28 : construction et parcours grossissent davantage que le tri

Les volumes suivants sont en **millions d'opérations du compteur nommé**,
arrondis ; les rapports sont calculés sur les entiers des reçus. Une visite,
une borne de boîte i128 et une comparaison de racines ne coûtent pas pareil :
ne pas sommer ces colonnes pour en déduire des secondes ou un pourcentage CPU.
`partition.node_visits` inclut ses tests et passages non testés ; ce n'est
pas un poste indépendant à ajouter à ses `block_bound_tests`/`point_tests`.

| K | Poste Local28 |8k|16k|32k|Rapports 8→16 /16→32|
|---|---|---:|---:|---:|---:|
|5| Graines préparées, `edge.seeds` |2,312|7,931|28,257|3,431 /3,563|
|5| Bornes Z de construction, `atlas.partition.block_bound_tests` |13,860|46,589|191,586|3,362 /4,112|
|5| Tests ponctuels de construction, `atlas.partition.point_tests` |67,036|200,676|675,184|2,994 /3,365|
|5| IDs de frontières copiés, `atlas.partition.frontier_ids_copied` |47,286|144,648|506,628|3,059 /3,502|
|5| Parcours de l'atlas, `sweep.query_visits` |24,982|100,372|562,408|4,018 /5,603|
|5| Sites lus dans les feuilles, `sweep.active_sites` |12,960|27,234|57,555|2,101 /2,113|
|5| Comparaisons de tri des événements, `sweep.sort_comparisons` |13,700|27,586|55,911|2,014 /2,027|
|10| Graines préparées, `edge.seeds` |10,712|27,913|83,663|2,606 /2,997|
|10| Bornes Z de construction, `atlas.partition.block_bound_tests` |71,934|197,794|644,666|2,750 /3,259|
|10| Tests ponctuels de construction, `atlas.partition.point_tests` |338,199|864,401|2437,430|2,556 /2,820|
|10| IDs de frontières copiés, `atlas.partition.frontier_ids_copied` |235,916|614,160|1782,590|2,603 /2,902|
|10| Parcours de l'atlas, `sweep.query_visits` |173,603|482,887|1741,815|2,782 /3,607|
|10| Sites lus dans les feuilles, `sweep.active_sites` |64,118|138,488|291,593|2,160 /2,106|
|10| Comparaisons de tri des événements, `sweep.sort_comparisons` |47,079|99,798|206,238|2,120 /2,067|

Les seules visites de construction des fragments totalisent
82,768→256,830→922,708 M en K5 et 424,025→1110,958→3274,716 M en K10.
Le nombre de cellules créées fait respectivement 0,940→2,333→5,944 M
et 4,435→10,161→23,756 M : les cellules seules ne décrivent pas le coût
de classifier/copier leurs frontières de témoins. Le futur produit X×C
ne supprime pas ces préparations s'il utilise exactement le même atlas
construit à l'avance ; ce poste reste un chantier distinct.

Le parcours par graine contient un autre signal directement exploitable.
Chaque graine atteignant une feuille contribue au moins une unité à
`sweep.leaf_queries`. Leur nombre distinct est donc **au plus** ce compteur,
même si certaines graines atteignent plusieurs feuilles. Par conséquent,
`1 − leaf_queries / edge.seeds` est une borne inférieure de la fraction
des graines qui construisent une famille mais n'atteignent aucune feuille.
Le générateur propriétaire assure ici `seed_owner_rejections=0`.

| K | Sans feuille, au moins à 8k |au moins à 16k|au moins à 32k|
|---|---:|---:|---:|
|5|76,06 %|85,28 %|91,33 %|
|10|75,37 %|79,54 %|85,70 %|

À 32k, la différence exacte `query_visits − line_tests` vaut 384 458 840
en K5 et 1 160 654 876 en K10 : ce sont les retours immédiats sur
`Deep`/`Outside` dans `visit`, avant tout test de droite. Un bit de branche
sans feuille vivante et la descente partagée peuvent viser ce travail,
sans préjuger du nombre de produits X×C nécessaire pour le remplacer.
**Rejeter tout l'atlas q4 ou une graine q4 ne permet jamais de sauter q3.**

Inversement, les balayages atteints restent petits dans cette série :
à 32k, 23,50 sites actifs lus par feuille en K5 et 24,37 en K10 ; après
clipping, 6,78 et 5,65 événements conservés par feuille. Ce sont des moyennes,
pas des plafonds ni une description des queues de distribution. Les groupes
profonds représentent encore 93,67 %/82,79 % des groupes visités, mais les
tris locaux font environ ×2 par doublement. Le nombre élevé de groupes
refusés ne justifie donc pas, seul, de remplacer chaque petit tri par une
sélection de fenêtre supplémentaire.

### Window30 : petit tri final, préparation et scans non gratuits

Même protocole et mêmes nombres de sorties/digests appariés ; cette égalité de grands
digests reste distincte de l'oracle exhaustif des petites portes. Window30
paie d'abord les couches du noyau global de chaque arête, puis les tas et
les deux passages sur le résidu ; les coûts du noyau 29 font partie du coût 30.

| K | Poste Window30, en millions |8k|16k|32k|
|---|---|---:|---:|---:|
|5| Comparaisons lexicographiques du noyau, `selection.lex_comparisons` |144,938|546,749|2118,258|
|5| Orientations du noyau, `selection.orientation_tests` |145,881|520,159|1897,489|
|5| Premier passage, `sweep.family.sites` |27,716|69,870|170,320|
|5| Second passage, `window.second_pass_sites` |4,423|9,000|18,392|
|5| Comparaisons des tas, `window.heap_comparisons` |37,089|93,549|227,888|
|5| Tri intérieur final, `sweep.family.sort_comparisons` |0,464|0,932|1,862|
|10| Comparaisons lexicographiques du noyau |729,040|2042,797|6603,804|
|10| Orientations du noyau |1649,636|4565,614|14508,706|
|10| Premier passage |523,756|1322,901|3326,665|
|10| Second passage |85,910|182,646|371,395|
|10| Comparaisons des tas |1187,835|2963,450|7397,959|
|10| Tri intérieur final |28,832|59,109|117,843|

À 32k/K10, le noyau global par arête de Window30 paie déjà 6,604 Md
comparaisons lexicographiques et 14,509 Md orientations. L'ajouter avant
l'atlas28 ne fournit aucune garantie préalable d'amortir ce nouveau
poste. Composer les preuves reste possible ; leur composition n'est
pas une preuve de gain. Une fenêtre **locale**, construite à partir du
fragment exact et du compte permanent incluant les constantes et les
événements clippés intérieurs, est une autre hypothèse : elle doit garder
la fenêtre fermée, les contacts et l'ownership de la cellule. Elle ne
doit pas rescanner le cover global pour chaque graine/feuille. Le coût
des petites listes ci-dessus impose de comparer tout le travail ajouté.

### Ordre de recherche proposé, conditionnel aux mesures closes suivantes

1. **Dans q4, commencer par les graines × centres** : filtre négatif de
   blocs puis, si nécessaire, vraie descente jointe, avec le nombre de
   familles et les reprises de cellules explicitement payés. Le taux de
   graines sans feuille et la croissance des parcours justifient cette
   priorité structurelle ; ils ne prouvent pas qu'une borne de bloc plus
   chère sera rentable. Ne pas transformer cette priorité en boucle plate
   de tous les blocs par toutes les cellules.
2. **Conserver un chantier de préparation de l'atlas** : copies et bornes
   Z constituent déjà un gros travail avant toute requête. Une préparation
   conditionnée par les conflits avec X, ou une composition emboîtée avec
   noyau local, nécessiterait une preuve et une mesure propres. Ne pas
   annoncer ce coût résolu par le seul raccord de la section 4.
3. **Ne pas faire de Window30 une substitution générale à Local28** sur la
   seule réduction du tri. Une composition locale reste à expérimenter,
   mais n'est pas prioritaire sur les milliers de millions de préparations
   et de visites inutiles alors que les tris de feuilles sont petits.
4. **L'héritage des témoins traite un autre poste, global q3/q4**. Dans 32,
   les H des rectangles font ×2,416/×2,253 au dernier doublement K5/K10,
   mais ceux des paires ×5,364/×3,850 et leurs Xi ×7,440/×4,693.
   La tranche 33 vise précisément une partie de ce défaut. Attendre son
   bilan clos 32k pour comparer les priorités ; si ces croissances se
   corrigent, le chantier X×C peut passer avant un nouveau partage de
   témoins. Ces nombres de 32 ne prédisent aucun gain en 33 ni la répartition
   du temps entre q3, filtres et q4.

Cette hiérarchie repose sur les volumes et leurs causes, pas sur une somme
de compteurs hétérogènes convertie en performance. Elle ne clôt ni la
sous-quadraticité de tous les régimes, ni les contrats G4.
