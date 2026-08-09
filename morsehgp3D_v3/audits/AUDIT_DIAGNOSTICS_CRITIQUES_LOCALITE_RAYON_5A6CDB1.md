# Audit des diagnostics de criticité, de localité et de rayon — commit `5a6cdb1`

> **Verdict : GO comme exploration, NO-GO comme certificat de localité, de taille de catalogue ou de coupe complète.** Les mesures isolent un signal utile : sur le nuage surfacique synthétique observé, le parcours produit beaucoup plus de sommets d'arrangement qu'il ne trouve de coquilles bien centrées. En revanche, `3 661` n'est pas la taille du catalogue, le ratio `7,32/point` n'est pas une loi d'échelle, le rang k-NN est conditionné par la sortie incomplète et par les égalités de distance, et le driver de rayon ne mesure pas le rayon des sommets d'arrangement non critiques. Les phrases « catalogue 50 k de l'ordre de 400 000 », « ballon critique forcément petit » et « une grille uniforme suffit » ne sont donc pas établies.

## 1. Snapshot et portée

Le diagnostic a été exécuté contre le header du commit `5a6cdb1af030a264ce07adddd312be2c458459b4`, de SHA-256 `c1548b3ce5336a423ceb7f069ba3311749efdca057025bbde1c63333be193457`. Il ne qualifie pas les révisions live postérieures de `prototype/order_k_bfs.hpp`.

| artefact scratch | SHA-256 |
|---|---|
| `local.cpp` | `f0eb829ea9ebb750159c793e3714f22032c6bdd57f79666e3333705d8b0e347d` |
| `loc2.tgz` | `ce74769e37b35ed1e25122a1f9b04521da5ecad2832b9179d1de17c33a9cfc74` |
| `radius.cpp` | `83761728005d85cffc48608e9d126aba22262980673813a42fcba2068398bec3` |
| `rad.tgz` | `d04de3435cc4ac7088900928b5ce9fbb438700a59da19c1b65bf540feed5b93c` |

Le préfixe jusqu'à la ligne 2 084 du journal local de la session Claude avait le SHA-256 `c7e5243f5f0ca69a1d9f0679049fd699f8abbd7a63ce8a6da34bf2a91eb5c69e`. Les lignes probantes 2 063, 2 064, 2 069, 2 071, 2 073, 2 076, 2 077, 2 082 et 2 084 avaient ensemble le SHA-256 `05b9a2391b50c2911f0a3c10321ea4c9d99c9c9e28832496d29f521d59c393ab`. Ces empreintes figent les claims, les commandes et les sorties même si le journal continue de croître.

Les drivers ont été compilés en CPU avec `g++ -std=c++20 -O3 -march=native` sur la cible G4 déjà utilisée par Claude. Ils ne font aucun calcul CUDA. Les deux profils de `local.cpp` ont été lancés en `nohup` concurrents; la campagne principale comportait elle aussi plusieurs jobs concurrents. Cette concurrence importe pour les temps, mais pas pour les objections logiques ci-dessous. Le profil dit LiDAR est le générateur sinusoïdal quantifié décrit dans [`AUDIT_PERFORMANCE_ORDER_K_5A6CDB1.md`](AUDIT_PERFORMANCE_ORDER_K_5A6CDB1.md), pas un scan réel.

Audit strictement en lecture : aucune VM n'a été créée, démarrée, interrogée, arrêtée ou modifiée.

## 2. Résultats observés et conclusion permise

Les sorties gardées dans le journal de session donnent :

| profil | $n$ | sommets visités | coquilles déclarées critiques | critiques par point | rang NN médian / p90 / p99 / max |
|---|---:|---:|---:|---:|---:|
| LiDAR synthétique | 500 | 337 429 | 3 661 | 7,32 | 35 / 54 / 70 / 88 |
| uniforme | 500 | 338 046 | 59 152 | 118,30 | 50 / 77 / 106 / 183 |

Une seconde passe donne, pour le même profil LiDAR synthétique mais à 1 000 points, 718 782 sommets visités et 10 626 coquilles déclarées critiques, soit 10,626 par point. Le ratio augmente donc déjà d'environ 45 % entre les deux tailles observées; il n'est pas constant dans ces données.

Le contraste LiDAR/uniforme est un diagnostic intéressant : la géométrie surfacique synthétique sélectionnée contient beaucoup moins de coquilles bien centrées trouvées que le profil uniforme. Il motive l'étude d'un générateur local. Il ne prouve ni une borne universelle, ni le comportement d'autres scènes, densités, quantifications ou graines, ni la taille à 50 000 points.

## 3. `3 661` n'est pas la taille du catalogue critique

`local.cpp` appelle `order_k_vertices`, puis examine seulement les sommets que ce parcours a déjà produits. Il incrémente son compteur lorsque la miniboule de `x.shell` passe par tous les éléments de cette coquille et lorsque `shell.size()+level <= s_max`.

Ce compteur omet au moins :

- les sphères critiques absentes du parcours à cause des défauts de navigation déjà reproduits sur le cube et le pont cosphérique;
- les arités un, deux et trois récoltées séparément par `order_k_catalogue`;
- toute coquille perdue ou partielle avant le test de miniboule;
- un census global indépendant qui confirmerait la coquille fermée et son niveau exact.

Il s'agit donc du nombre de **coquilles déclarées critiques parmi les sommets trouvés**, pas d'un oracle du catalogue. Le filtre terminal peut écarter un faux rang; il ne répare ni la navigation ni la récolte.

L'extrapolation `7,32 × 50 000 ≈ 366 000`, arrondie en « ordre de 400 000 », ajoute une hypothèse de ratio constant à un compteur post-sélectionné. Une seule graine synthétique à $n=500$ ne peut pas porter cette hypothèse, et la mesure à $n=1000$ la contredit déjà quantitativement. Même une taille de sortie de 400 000 ne prouverait d'ailleurs pas le contrat d'une seconde : coût de génération, déduplication, censuses, forêts, mémoire et parallélisme restent à mesurer sur une voie complète et exacte.

## 4. Le rang k-NN observé ne certifie pas une fenêtre locale

Pour chaque point, `local.cpp` calcule et trie les distances vers les $n$ points, puis stocke une matrice entière `rank[n][n]`. Il mesure ensuite les rangs mutuels des seuls supports de miniboule retenus par le parcours.

Trois limites sont bloquantes :

1. **Biais de sélection.** Un support critique manquant n'entre jamais dans l'histogramme. Le maximum 88 ne borne donc pas le rang d'un support inconnu et ne certifie pas qu'une énumération locale retrouvera tout le catalogue.
2. **Mauvais objet de preuve.** Des rangs support--support modestes sur la sortie observée ne bornent ni les points témoins qu'il faut examiner pour certifier l'intérieur d'une sphère, ni un rayon de recherche complet, ni une source locale obligatoire pour chaque support critique.
3. **Égalités arbitraires.** Le tri porte sur des paires `(distance², PointId)`. Sur une grille u16, des distances égales sont fréquentes; le rang ordinal varie donc avec les identifiants et les permutations. Une fenêtre k-NN complète doit inclure toute la bande d'égalité au seuil, ou être définie par une distance exacte, et non par ce départage d'ID.

Le driver lui-même n'est pas une architecture 50 k : la matrice de rangs emploie $\Theta(n^2)$ entiers, environ 10 Go rien que pour ses éléments à 50 000 points, hors surcoûts des vecteurs, et les $n$ tris coûtent $\Theta(n^2\log n)$. Il convient comme instrument exploratoire à petite taille, pas comme chemin produit.

Une certification locale demanderait une propriété universelle du type : tout support critique admissible possède une source génératrice dans une fenêtre construite indépendamment de la sortie, avec fermeture exacte de la bande d'égalité et contrôle de tous les témoins nécessaires. À défaut, la voie doit rester fail-open vers un oracle ou un parcours exhaustif borné.

## 5. P0 — `radius.cpp` ne mesure pas le rayon des sommets non critiques

`radius2_of(x.shell)` appelle `miniball_of` et retourne le rayon de cette **miniboule**. Pour une coquille non bien centrée, la miniboule de ses points ne coïncide généralement pas avec la sphère d'arrangement qui passe par toute la coquille. Or le driver applique cette fonction à tous les sommets visités avant de publier la fraction « sommets sous le rayon critique maximal ».

Contre-exemple u16 exact, affinement indépendant :

```text
a=(0,0,0), b=(4,0,0), c=(1,1,0), d=(1,0,1)
```

La sphère commune aux quatre points a pour centre $(2,-1,-1)$ et rayon carré $6$. La miniboule de l'ensemble est le diamètre `ab`, de centre $(2,0,0)$ et rayon carré $4$; elle contient `c` et `d` strictement. `radius2_of` publie donc 4 pour un sommet d'arrangement de rayon carré 6.

Par conséquent, les nombres suivants ne mesurent pas l'effet d'une coupe en rayon sur le parcours :

| profil | $n$ | max critique publié | sommets publiés sous ce max |
|---|---:|---:|---:|
| LiDAR synthétique | 500 | 90,0 | 63,6 % |
| LiDAR synthétique | 1 000 | 90,6 | 66,0 % |
| uniforme | 500 | 16 033,5 | 88,3 % |

Le problème n'est pas une simple erreur d'arrondi : la quantité géométrique est différente. Il faut reconstruire exactement la sphère d'arrangement de chaque sommet, ou transporter son centre et son rayon exacts, avant de prétendre simuler une coupe.

## 6. Le rayon maximal observé est circulaire et « bien centré » ne signifie pas « petit »

Le seuil 90,6 est le maximum des seules coquilles critiques trouvées par le parcours courant. Une sphère critique omise, notamment derrière une strate cosphérique coupée, est absente à la fois du maximum et du test. Couper au maximum de la sortie ne peut donc pas certifier la complétude de cette sortie.

La phrase « son centre est dans l'enveloppe de son support, donc le ballon est forcément petit » est mathématiquement fausse sans hypothèse métrique supplémentaire. La condition de bon centrage contrôle la position barycentrique du centre; elle n'impose aucune petite échelle absolue.

Fixture u16 entièrement exacte :

```text
(40000,40000,40000), (40000,0,0), (0,40000,0), (0,0,40000)
```

Ce tétraèdre régulier a pour centre $(20000,20000,20000)$, coefficients barycentriques $1/4$ et rayon carré $1\,200\,000\,000$. Avec ces quatre points seuls, sa boule est vide et critique de rang quatre. On peut ajouter de nombreux points u16 distincts près de `(65535,65535,65535)` : ils restent strictement extérieurs. La même construction à l'échelle $M$ a un rayon $\sqrt{3}M$; le bon centrage ne fournit donc aucune constante comparable aux 90 unités observées, jusqu'à la limite de la grille.

Enfin, un seuil calculé en `double` ne peut pas décider de façon certifiée une égalité au bord. Une optimisation complète doit comparer des rayons carrés rationnels ou des puissances exactes, avec une convention explicite pour les ex aequo. Un filtre flottant peut servir de préfiltre s'il est enveloppant et suivi d'un rattrapage exact; il ne peut pas être la porte de complétude.

## 7. Voie constructive à conserver

Ces expériences ont une vraie utilité : elles montrent que, sur le profil surfacique choisi, l'essentiel du coût courant est dans la navigation d'arrangement et non dans les coquilles bien centrées trouvées. Elles fournissent aussi des distributions à reproduire sur plusieurs tailles, graines et données réelles.

La prochaine expérience qualifiante devrait :

1. construire une source locale indépendante de `order_k_vertices` et sans matrice globale $n^2$;
2. fermer exactement les égalités de distance et être invariante par permutation des `PointId`;
3. comparer son catalogue complet, toutes arités incluses, à un oracle exhaustif indépendant sur de petits nuages;
4. injecter en fixtures permanentes le cube, le pont cosphérique, le sommet rayon 6/miniboule 4 et le tétraèdre bien centré de grande échelle;
5. tester plusieurs graines, tailles, densités et scans LiDAR réels, en publiant distributions et pires cas plutôt qu'un ratio unique;
6. garder tout filtre de rayon ou de grille fail-open tant que l'enveloppe de recherche et le traitement exact du bord ne sont pas prouvés.

Décision : créditer les drivers comme **diagnostics exploratoires**. Ne pas promouvoir leurs maxima observés en constantes de recherche, leur compte critique en taille de catalogue, ni leur extrapolation en preuve du contrat 50 k.

GCP non utilisé par cet audit.
