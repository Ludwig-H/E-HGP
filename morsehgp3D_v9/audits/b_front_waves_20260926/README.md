# Front q3/q4 par vagues : preuve exécutable CPU avant port GPU

26 septembre 2026. `exploration_v9_hors_registre`, CPU seulement,
`quantized_u18_input_only`, `not_claimed`. Aucun appel GCP, aucune modification
de `src/` ou CMake. Cette sonde prépare l'ordonnancement du front GPU ; elle
ne contient ni S2, ni certificats/voies q3/q4, ni catalogue/census, ni FULL.

## Résultat acquis

Le parcours par vagues produit exactement les mêmes rectangles avec leurs
masques, et **tous les champs** de `WspdFrontWork`, que `run_wspd_front`.
Les 504 cas combinent sept nuages (singleton, paire aux extrêmes u18,
contacts, extrêmes, uniforme, terrain, rangées), K2/5/10, s8/10/12,
Pure/MidpointSamples, masque q3 et masque q3/q4, puis ordre normal et inversé
des tâches dans chaque vague. Ils passent en Release Clang et ASan/UBSan.
Quatre mutations sont tuées causalement dans chaque build : enfant perdu,
profondeur changée, état de pile DFS changé et masque perdu. Un crash ne
vaut jamais succès. Les 9 cas synthétiques 8k/16k/32k passent aussi.

Le premier prototype conserve le `Task` produit de 72 octets. Le second
conserve seulement les champs utiles q3/q4 dans **16 octets**. Ses 504 cas
et quatre mutations passent en Release et ASan/UBSan ; uniforme32k et la
trame entière sans sol 08/000000 à s8/10/12 passent aussi. La porte d'encodage
teste aussi une restitution complète aux bornes u32/u16, six dépassements
ou domaines interdits à l'encodage et trois mots invalides au décodage.

## Comment la géométrie est réutilisée

L'API publique expose des sous-arbres complets mais pas le pas élémentaire
`Front::expand`. Celui-ci est public dans une classe d'un namespace anonyme
de `front.cpp`. La sonde inclut donc **une fois** ce fichier dans sa propre
unité de traduction et appelle son corps existant. Elle ne le recopie pas,
ne retire pas `private`, ne transforme pas ses signes et ne le recompile
pas une seconde fois via `libmhgp9_gen`. C'est une porte blanche de
l'ordonnanceur, pas un juge indépendant de la géométrie du produit.

Les tâches possèdent leurs scalaires ; les deux tableaux de tâches restent
liés au même `shared_ptr<const Q2CensusIndex>`. Chaque tâche est développée
**une seule fois** en zéro, deux ou trois enfants, ou un terminal. Des
paquets temporaires d'au plus 1 024 développements permettent de compter,
préfixer puis disperser les enfants et terminaux sans refaire la géométrie.
La taille 1 024 ne limite ni les vagues, ni les tâches, ni les sorties :
tous les paquets sont consommés avant de passer à la vague suivante.

`expand` émet ses enfants en ordre inverse du DFS canonique : la sonde
inverse chaque petit paquet d'enfants pour former l'ordre BFS canonique.
L'autre bras inverse la vague entière. Le juge normalise les rectangles
par `(a_node,b_node,mask)` et vérifie leur unicité. Le sort du juge est
hors chronomètres du front. L'identité de sortie n'est pas inférée d'un
simple condensé. `depth` et `dfs_pending` sont conservés : la profondeur
et le maximum logique de pile DFS restent identiques même si la mémoire
de la file BFS est beaucoup plus grande.

## Preuve de coût et encodage pour GPU

Soit T le nombre de tâches développées et R le nombre de rectangles émis.
La racine est unique ; tout autre produit apparaît exactement comme enfant
d'une seule tâche. Les branches diagonales LL/LR/RR partitionnent les
paires non ordonnées ; les branches non diagonales partitionnent un seul
facteur. Un masque fils est inclus dans celui du parent. Le pas existant
détermine seul ces branches et rejets. L'invariant mesuré est
`tasks = children + 1 = product_visits`, et `emitted = emitted_rectangles`.
Un terminal n'est jamais remis dans la file. Les échecs d'allocation ou de
taille provoquent une exception, jamais une sortie partielle déclarée
réussie.

Le travail **ajouté par l'ordonnancement** est O(T+R), chaque paquet faisant
un nombre constant de copies par enfant/sortie. L'espace auxiliaire est
O(T+R), plus le propriétaire/index partagé ; plus finement O(Fmax+R), où
Fmax est la largeur maximale des vagues, avec une réserve géométrique des
vecteurs et un scratch constant. Le juge ajoute séparément O(R log R) et
deux tableaux de rectangles. La géométrie demeure
O(T(D+K)+R) ; **T n'est pas prouvé sous-quadratique globalement**.

Le format compact est `{u32 a,b; u16 depth,dfs_pending; u8 mask,terminal;
u16 reserved}`. `static_assert` impose 16 octets et copie triviale. Les
conversions contrôlent chaque domaine avant réduction ; les mots réservés
doivent rester nuls. Les témoins hérités sont absents (`count==0`) et le
masque ne contient que les voies 2/4. Le domaine entier u18 borne le chemin
spatial à 54, le chemin produit à 108, et l'accumulation des frères à au
plus 216 : u16 ne plafonne donc aucune recherche du domaine présent.
Les nœuds qui dépasseraient u32 sont refusés explicitement par cet encodage,
comme par l'index GPU actuel ; aucune réduction silencieuse n'est admise.
La sonde native 72 octets reste le témoin disponible pour les domaines
d'indices plus larges. Le port GPU devra extraire le pas géométrique vers
un en-tête explicite ; inclure un `.cpp` n'est pas la proposition produit.

La sonde appelle `expand` séquentiellement : son objet `Front` accumule
actuellement les compteurs dans `result_` et n'est **pas thread-safe**.
La décision géométrique ne lit pas ces compteurs. Le port GPU doit donc
extraire le pas sans état partagé et réduire des compteurs locaux, par
sommes pour le travail et maxima pour les pics. L'indépendance des tâches
ne permet pas d'appeler l'objet actuel depuis plusieurs threads.

## Mesures synthétiques natives closes

Chaque temps est celui d'un passage CPU mono local, sans répétition ; les
temps CPU de fil sont publiés avec le mur. La variante par vagues ne
gagne pas sur ce CPU : environ 11 à 20 % de coût de plus. Son but ici est
de rendre le travail disponible en masse sur GPU, gain encore non mesuré.

| régime | n | tâches T | rectangles R | DFS CPU s | vagues CPU s |
|---|---:|---:|---:|---:|---:|
| uniforme anisotrope | 8 000 | 5 180 056 | 1 373 829 | 2,880 | 3,194 |
| uniforme anisotrope | 16 000 | 14 782 978 | 3 486 293 | 8,418 | 9,344 |
| uniforme anisotrope | 32 000 | 35 282 886 | 7 737 222 | 20,330 | 23,156 |
| terrain | 8 000 | 1 020 876 | 300 756 | 0,532 | 0,609 |
| terrain | 16 000 | 2 170 530 | 624 333 | 1,153 | 1,353 |
| terrain | 32 000 | 4 476 690 | 1 273 272 | 2,545 | 2,883 |
| rangées | 8 000 | 94 330 | 32 756 | 0,029 | 0,035 |
| rangées | 16 000 | 189 460 | 65 772 | 0,062 | 0,073 |
| rangées | 32 000 | 379 770 | 131 804 | 0,125 | 0,141 |

Les exposants observés de T sont 1,003–1,513 ; ceux des descentes témoin
1,104–1,619. Cela décrit **le front seulement** sur ces neuf entrées, pas
l'aval. Le générateur garde x=7i, donc augmente l'étendue x avec n ; y/z
sont pseudo-aléatoires uniformes (z dans une bande de 1 m pour terrain).
Ce n'est ni une expérience à domaine fixé ni le protocole de coupes
capteur LiDAR. Les rangées gardent leur espacement. Les détails exacts
sont exécutables dans `synthetic()`.

La réserve maximale des deux tableaux de tâches atteint 1 264 032 864
octets à uniforme32k, plus 263 976 192 octets de réserve de rectangles
et 270 352 octets de scratch. Ce sont des capacités de buffers identifiées,
**pas RSS**, sans le juge ni l'index. Ce coût motive l'encodage 16 octets,
la publication des pics par vague et, plus tard, le tuilage hors mémoire
si nécessaire ; déplacer le travail sur GPU ne rend pas cette mémoire
gratuite.

La reprise **compacte** de uniforme32k garde exactement T, R, largeur,
compteurs et statistiques de tuiles. La réserve des tâches tombe à
**280 896 192 octets**, soit exactement un facteur 4,5 ; scratch et
rectangles restent identiques. Sur ce passage, DFS vaut 19,536 CPU·s et
les vagues 21,101 CPU·s. Les deux passages natif/compact ne sont pas
entrelacés : leur écart de chronomètre ne mesure pas isolément le gain
du format. La réduction des octets, elle, est déterministe.

## Trame entière sans sol et grille 1 mm

Les 39 885 sites de 08/000000 sont lus dans leur intégralité ; hash et
taille 478 620 octets sont publiés dans `packed_frame/PROVENANCE.json`.
Le front seul est exécuté à K5, avec tous les compteurs et rectangles
comparés à DFS. Le masque est celui du pilote géométrique figé v8.

| s | tâches T | rectangles R | vagues | capacité tâches octets | DFS CPU s | vagues CPU s |
|---|---:|---:|---:|---:|---:|---:|
| 8 | 8 849 315 | 3 133 819 | 43 | 38 634 592 | 5,151 | 5,458 |
| 10 | 10 533 259 | 3 718 750 | 44 | 52 004 288 | 6,006 | 6,485 |
| 12 | 12 031 871 | 4 266 861 | 44 | 55 696 352 | 6,932 | 7,330 |

Ces temps sont mono CPU locaux, pas les 98,6 ms du front W48 sur G4.
À s8, R et la masse brute 103 861 099 coïncident avec R24-B. La vague
la plus large vaut 827 114 tâches à s8. Sur ce seul front, la version
compacte reste 5,7 à 8,0 % plus lente en CPU ; aucun gain GPU extrapolé.
Une seule trame/une séquence, aucun test de croissance LiDAR à partir
de ces trois choix de s.

## Tuiles de 32 paires : portée de la statistique

La sonde concatène virtuellement les produits des rectangles bruts dans
l'ordre DFS, puis dans l'ordre de sortie BFS. Pour un rectangle `[a,b)`,
le nombre de tuiles globales complètes de 32 paires entièrement dedans
est `max(0,floor(b/32)-ceil(a/32))`. Les autres tuiles complètes traversent
une frontière. Le reliquat final de moins de 32 paires est exclu.
L'histogramme de masses est `[1,2..7,8..31,32..255,256..4095,>=4096]`.
Ces statistiques sont **avant le filtre S2 des rectangles** : il change
les masses et les offsets. Elles ne quantifient donc pas encore le taux
de succès d'un fast path de warp dans `pair_kernel`.

Sur la trame entière, les tuiles DFS contenues dans un rectangle représentent
**87,0 % / 84,0 % / 80,8 %** à s8/10/12. Les décomptes exacts sont
2 823 484/3 245 659, 2 461 744/2 929 586 et 2 218 318/2 744 764.
L'ordre BFS change légèrement les offsets et donne des taux proches.
À l'inverse, presque toutes les tuiles de l'uniforme synthétique traversent
des petits rectangles. Cela justifie une mesure du fast path **après S2**
sur LiDAR avant tout port, pas une promesse de gain.

## Rejouer et lire

Depuis la racine du dépôt, compiler dans un fichier neuf :

```sh
clang++ -std=c++20 -O2 -DNDEBUG -Wall -Wextra -Wpedantic -Werror \
  -ffunction-sections -fdata-sections -pthread -Imorsehgp3D_v9/src/gen \
  morsehgp3D_v9/audits/b_front_waves_20260926/front_waves.cpp \
  morsehgp3D_v9/src/gen/pipeline/prepared_cloud.cpp \
  morsehgp3D_v9/src/gen/pipeline/q2_census.cpp \
  -Wl,--gc-sections -o /tmp/mhgp9-front-waves-new
python3 -B morsehgp3D_v9/audits/b_front_waves_20260926/run.py \
  --binary /tmp/mhgp9-front-waves-new --out /tmp/mhgp9-front-waves-new-fixtures \
  --mode fixtures --packed
```

Clang utilisé : 18.1.3. ASan/UBSan remplace `-O2 -DNDEBUG` par
`-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all
-fno-omit-frame-pointer`. L'essai interactif initial GCC a refusé
l'inclusion blanche pour `-Wsubobject-linkage`, plus deux erreurs de la
sonde corrigées (indentation et accès en écriture à `Point3::operator[]`).
Ce n'est pas un défaut du moteur. La compilation Clang stricte ne relâche
aucun avertissement.

`run.py` écrit les commandes, sorties, retours, hashes de binaire et
sources avant capture, vérifie la stabilité après capture et clôture
par SHA-256. `verify.py CAPTURE...` lit les archives sans accès aux anciens
binaires ; il recalcule les identités, les tableaux et les exposants.
Il passe aussi sous `python3 -O`. Les fichiers `front_waves_native.cpp`
et `run_native.py` sont exactement les sources antérieures épinglées
par les premières captures (hashes vérifiés), conservées comme témoins
et non comme une deuxième implémentation produit.

Captures closes : `release_fixtures/`, `sanitize_fixtures/`,
`release_scaling/`, `packed_fixtures/`, `packed_sanitize_fixtures/`,
`packed_scale32/`, `packed_frame/`. Les lecteurs normal et `-O` passent.
Aucun contrat FULL/G4 acquis.
