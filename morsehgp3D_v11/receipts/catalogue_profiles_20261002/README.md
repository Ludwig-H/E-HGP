# Comparaison CPU des profils 18, 21 et 24 bits

Le lecteur LIVE exige le recu brut local indique dans chaque capture, son hash et une archive originale
`results.tar.gz`. Aucune donnee KITTI ni sortie canonique volumineuse n'est copiee ici. Le code 0 signifie
coherence des pieces : les campagnes echouees ou interrompues restent explicitement non conformes.

```sh
python3 -B check.py
python3 -B -O check.py
python3 -B check_selftest.py
python3 -B -O check_selftest.py
```

La capture close `profiles1/` contient `receipt.json`, `matrix.json`, `inputs.json`, `profiles.json` si produit,
et une seule archive originale. La lecture utilise `extractfile`, jamais une extraction sur disque.
Les controleurs et le lecteur des captures precedentes restent distincts et inchanges.
Le self-test conserve cinq temoins JSON et 24 corruptions refusees en normal et `-O` ; les empreintes
des scripts et ces resultats sont dans `check_selftest.json`. Une commande ayant produit un rapport
exige aussi une fermeture explicite de son groupe de processus.

Le calendrier declare 36 unites : six entrees entieres, K5/K10, trois profils compiles, une repetition.
Les memes coordonnees u18 et IDs sont utilises par B18/B21/B24 ; aucune requantification. Apres echec K5,
seul K10 du meme profil peut etre omis. Un checkpoint `pending_semantic` conserve le resultat processus
si le decodage est interrompu. Les unites restantes ne deviennent pas des mesures.

Chaque profil porte son hash d'executable, sa provenance de compilation et son CMakeCache. Les temps de
catalogue incluent les deux passes, le tri et les sorties en memoire. Le processus inclut lecture/serialisation ;
la normalisation semantique Python est mesuree separement. Les reservations natives ne mesurent ni RSS
ni memoire du decodeur Python. Une seule repetition ne qualifie pas la repetabilite chronometrique.

Les empreintes semantiques normalisent le profil, le remplissage des limbes et les fractions exactes ;
coordonnees, IDs, supports, niveaux, rangs et populations restent compares. Les compteurs geometriques
sont compares aussi. Deux reussites divergentes restent signalees meme si le troisieme profil echoue.
Les fichiers canoniques sont supprimes apres mesure : leurs hashes bruts et semantiques sont des resultats
declares du pilote qualifie, sans rehachage possible de ces octets depuis la capture compacte.
Les petites portes tetraedriques emploient effectivement les hauts bits 21/24 et des attendus analytiques.

Capture `profiles1`, source `9df77494732b03ddf11dbcf1dcb11d96bef54a3b` : matrice conforme,
1 002/1 002 portes cumulees. Release18 227/227 ; bits21/24, ASan/UBSan24 et TSan21 152/152 chacun ;
poison21 153/153 ; mutants12/12, style2/2 ; Clang absent. Les 116 mutants sont detectes (dont deux refus
de compilation attendus dans core ; aucun signal/delai), 13 concernent num et 9 le catalogue.

Le calendrier compte 33 tentatives : 15 succes K5, 18 delais et 3 K10/32k omis apres echec K5.
Cinq entrees ont les memes sorties semantiques et travail aux trois profils, aucune divergence.
Chaque profil compte 11 tentatives, dont 5 succes. La campagne demeure **ECHEC**, meme si le lecteur
rend 0 pour la coherence de ses pieces. La generation a ete certifiee arretee ; cles privee/OS Login
retirees, reserve liberee. Les lecteurs normal/−O et leurs cinq temoins/24 corruptions passent.

Les temps sont publies dans [DEVELOPPEMENT.md](../../docs/DEVELOPPEMENT.md). Les cinq sorties u18
et compteurs geometriques sont aussi identiques aux sept mesures terminees de `catalogue3` (8k repete
trois fois). Les comparaisons entre sessions restent non appariees et la nouvelle campagne n'a qu'une
repetition par entree/profil. Aucune qualification de repetabilite ni de plusieurs sequences LiDAR.

`preflight_4800.json` retranscrit le premier refus avant lancement, sans constituer un recu natif :
maxRunDuration de la cible fixe a3600s, aucune mutation cloud. Le plan reel respecte cette garde.
Le catalogue CPU ne qualifie ni FULL, ni une execution GPU, ni le contrat de100ms.

## Complement ASan/UBSan en B18

La capture close `asan18/`, source `d77e4b77c0bb38be82b83908b2724bec1663645b`, qualifie le module
`num` seul : **13/13 portes**, configuration unique `gcc_asan_ubsan18`, une commande worker de matrice.
Elle exerce notamment la voie q3 native en B18, distincte de la voie large testee sous ASan en B24.
Le test `mhgp11_num_unit_power_paths` effectue 207 controles ; Fraction, en normal et `-O`, effectue
7 526 controles dont 504 geometries et 160 cas entiers. Aucun nouveau chrono catalogue n'en est deduit.

```sh
python3 -B check_asan18.py
python3 -B -O check_asan18.py
python3 -B check_asan18_selftest.py
python3 -B -O check_asan18_selftest.py
```

Ce lecteur distinct exige le cache B18, SANITIZE=ON, TSAN=OFF, POISON=OFF et MODULES=num, les empreintes
de `libmhgp11.a`, `mhgp11_num_probe`, `mhgp11_num_unit`, du cache, des flags et commandes de lien.
Il verifie l'instrumentation ASan/UBSan declaree par les flags et les trois portes numeriques nommees.
Le recu brut local, l'archive originale et la fermeture du groupe de processus restent obligatoires.
L'arret cible est certifie, les cles privee/OS Login retirees et la reservation liberee.
Deux temoins JSON et 18 corruptions du lecteur passent en normal et `-O` ; leurs hashes et resultats
sont conserves dans `check_asan18_selftest.json`. Une campagne echouee reste affichee comme telle.
