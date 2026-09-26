# S2 : arithmétique étroite sous garde exacte

26 septembre 2026. Prototype d'audit B isolé ; `exploration_v9_hors_registre`,
`cpu_reference`, `quantized_u18_input_only`, `not_claimed`. Aucun fichier
produit modifié. GCP non utilisé. La cible finale demeure la tour FULL K5
explicite en 100 ms ; ce dossier ne calcule pas une tour et ne revendique
aucun gain GPU.

## Ce que change la proposition

Le filtre des **paires fixes** prépare trois intervalles de composantes du
produit vectoriel. Le produit actuel élève systématiquement leurs six
extrémités au carré en entier 128 bits. Le prototype calcule ces extrémités
exactement comme le produit, puis prend la voie entière 64 bits seulement si
la garde suivante est vérifiée : $0 < H_{max,4} \leq 2^{30}$ et chacune des
six extrémités croisées appartient à $[-2^{28},2^{28}]$. Sinon il reprend le
calcul 128 bits, en réutilisant les extrémités déjà obtenues.

Il ne s'agit **pas** d'une approximation, ni d'une réduction de coordonnées,
ni d'une modification des boîtes générales des rectangles WSPD. Les boîtes
et points doivent déjà être ceux d'un index validé u18 ; le helper numérique
n'est pas une fabrique acceptant des coordonnées arbitraires.

## Preuve et limites

Chaque carré croisé est au plus $2^{56}$. La somme des trois majorants est
au plus $3\,2^{56}$, et son produit par 16 au plus $3\,2^{60} < 2^{62}$.
Pour les deux voies, $\alpha\in\{2,3\}$, donc
$\alpha H_{max,4}^{2} \leq 3\,2^{60} < 2^{62}$. Lorsque le minorant est
positif, il est au plus le majorant, donc la même borne vaut pour son carré.
Toutes ces valeurs sont non négatives et tiennent en `int64` signé, y
compris les sommes intermédiaires. Les produits nécessaires au calcul des
extrémités restent ceux du domaine u18, déjà bornés en `int64` dans le
produit. Aucune valeur n'est élevée au carré avant la garde étroite.

La garde est fermée : égalité aux seuils autorisée ; un dépassement d'une
unité entraîne le repli. Elle n'utilise pas `abs`, donc `INT64_MIN` est
refusé sans négation débordante. Les intervalles inversés sont refusés par
la garde. Les comparaisons géométriques gardent exactement `<=` pour
l'exclusion et `>` pour l'admission stricte ; la tangence n'est pas promue
en intérieur. Les entrées arbitraires refusées par cette garde ne deviennent
pas pour autant des entrées admissibles au repli : la provenance u18 reste
obligatoire.

Le parcours DFS, ses comptes, seuils K, ordre des enfants et sorties restent
inchangés. Le nombre de visites, la complexité en fonction du nuage et la
mémoire des objets globaux restent identiques : cette piste réduit seulement
le coût arithmétique local. Elle ne fournit aucune preuve sous-quadratique.

## Coût ajouté et décision d'intégration

La proposition ajoute un test sur H et les gardes des six extrémités, puis
une branche. Les extrémités sont réutilisées au repli ; il ne faut pas appeler
`pair_xi` après avoir déjà payé leur préparation. Sur CUDA, la divergence
de la branche, les registres supplémentaires et le code des deux voies
peuvent absorber le gain des produits plus courts. Les chronos CPU ne
permettent pas de décider de cela. Il faut une ablation G4 appariée de S2,
masques et visites égaux, puis chaîne FULL avec catalogue et tour égaux.
Ce port GPU reste à faire, sans changement de défaut ici.

## Fichiers et reproduction

- `narrow_pair.hpp` : helpers numériques et twin du DFS des paires.
- `gate.cpp` : gardes de domaine, égalités, extrêmes numériques et 200 000
  géométries déterministes, moitié locales translatées près de la limite
  u18, moitié couvrant tout le domaine. Comparaison à `cpp_int` Boost et au
  calcul `pair_xi` existant ; jamais un oracle flottant.
- `extremes.cpp` : 10 368 géométries supplémentaires aux coins des cubes
  de côté 1, 32 768 et 262 143, à l'origine puis translatés jusqu'au plafond
  u18. L'oracle `cpp_int` recalcule les extrema croisés en visitant les huit
  sommets de chaque boîte, indépendamment de leur formule optimisée.
- `sample.cpp` : vrai index et vrai front WSPD s8, vrai filtre rectangle,
  puis une paire chaque 1 024 positions du résidu. Les deux DFS comparent
  masque **et** nombre de visites pour chaque requête, trois répétitions.
  C'est un échantillonnage diagnostique des requêtes S2, pas une trame
  réduite présentée comme contrat ; l'index/front portent le nuage entier.
- `run.py` : capture des commandes, sorties, empreintes avant/après et
  lecteur LIVE. Le lecteur dépend des sources, builds et entrées locales
  épinglées, non d'une archive autonome. Aucune donnée KITTI n'est copiée.

Depuis la racine du dépôt, builds neufs (Boost 1.83 extrait disponible dans
l'environnement de cette capture) :

```sh
cmake -S morsehgp3D_v9 -B /tmp/mhgp9-b-100ms-20260926-build -DCMAKE_BUILD_TYPE=Release -DBOOST_ROOT=/workspaces/E-HGP/build/v7_boost_gate/extracted/usr
cmake --build /tmp/mhgp9-b-100ms-20260926-build --target mhgp9_gen --parallel 2
g++ -std=c++20 -O3 -Wall -Wextra -Wpedantic -Werror -I/workspaces/E-HGP/build/v7_boost_gate/extracted/usr/include morsehgp3D_v9/audits/b_s2_narrow_20260926/gate.cpp -o /tmp/mhgp9-b-s2-narrow-gate
clang++ -std=c++20 -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined -Wall -Wextra -Wpedantic -Werror -I/workspaces/E-HGP/build/v7_boost_gate/extracted/usr/include morsehgp3D_v9/audits/b_s2_narrow_20260926/gate.cpp -o /tmp/mhgp9-b-s2-narrow-gate-san
g++ -std=c++20 -O3 -Wall -Wextra -Wpedantic -Werror -Imorsehgp3D_v9/src/gen -I/workspaces/E-HGP/build/v7_boost_gate/extracted/usr/include morsehgp3D_v9/audits/b_s2_narrow_20260926/sample.cpp /tmp/mhgp9-b-100ms-20260926-build/libmhgp9_gen.a -pthread -o /tmp/mhgp9-b-s2-narrow-sample-r2
```

Les commandes exactes de la capture et leurs sorties brutes sont conservées
par le runner. Les premiers échecs de configuration (Boost absent du système)
et compilation stricte (conversion signée, `nodiscard`) sont signalés ; ils
ont empêché l'exécution, pas mis en évidence une erreur géométrique.

## Résultats bornés

`capture_r2.json` est clos : 10 commandes, 80 empreintes LIVE rejugées en
lecture normale et sous `python3 -O`. Les deux portes arithmétiques, GCC
Release et Clang ASan/UBSan, donnent exactement les mêmes 19 contrôles de
garde et 116 049 comparaisons numériques : 58 098 cas étroits, 57 930 replis
issus des géométries aléatoires, 21 cas numériques de seuil, plus 83 972
géométries rejetées avant Xi parce que Hmax n'est pas positif.

La porte supplémentaire `extremes.cpp` passe aussi en GCC Release et Clang
ASan/UBSan : 10 368 géométries chacune, dont 7 440 sorties H, 1 024 passages
étroits et 1 904 replis. Les cas contiennent explicitement les coordonnées
0, 1, 262 142 et 262 143. Sa capture distincte `extremes_capture.json`
transcrit les deux appels natifs et leurs empreintes ; elle ne fait pas
partie des dix commandes du runner r2. Reproduire les commandes de compilation
de `gate.cpp` ci-dessus en remplaçant `gate.cpp` par `extremes.cpp` et les
sorties par `/tmp/mhgp9-b-s2-narrow-extremes{,-san}`.

Les huit sondes comprennent 631 113 requêtes S2, trois fois comparées
individuellement. Leurs 10 549 457 visites (par répétition) sont identiques
au produit ; aucune divergence de masque. La proportion admissible concerne
uniquement les nœuds ayant Hmax positif, donc réellement soumis à Xi :

| régime | sites | K | paires après filtre rectangle | requêtes sondées | part i64 parmi tests Xi |
| --- | ---: | ---: | ---: | ---: | ---: |
| terrain | 8 000 | 5 | 140 093 | 137 | 100 % |
| terrain | 16 000 | 5 | 285 984 | 280 | 100 % |
| terrain | 32 000 | 5 | 591 301 | 578 | 100 % |
| amas | 8 000 | 5 | 28 352 433 | 27 688 | 76,51 % |
| amas | 16 000 | 5 | 112 770 168 | 110 128 | 75,67 % |
| amas | 32 000 | 5 | 449 652 733 | 439 114 | 74,60 % |
| 08/000000 sans sol, 1 mm | 39 885 | 5 | 23 686 751 | 23 132 | 87,56 % |
| même trame et même masque | 39 885 | 10 | 30 777 213 | 30 056 | 90,67 % |

L'entrée LiDAR est l'archive du pilote Patchwork++ citée dans la capture,
SHA-256 `0baa4de14c95838ef7bd18d5a98551ca513ed830ec1eeee84f649fa97c95abaf`.
Ce sont deux valeurs de K sur **une seule trame**, pas plusieurs scènes.

La masse résiduelle des amas croît de ×3,977 puis ×3,987 au doublement :
ce contre-régime reste quasi quadratique à cet étage S2. Le terrain fait
×2,041 puis ×2,068. Les visites sondées ne démontrent pas la croissance
exacte de toutes les visites non exécutées, et ces observations ne disent
rien de la tour globale ni d'une asymptotique. La variante arithmétique ne
change aucune de ces masses.

Les temps bruts CPU restent publiés mais **aucun gain de temps n'est acquis** :
quelques sondes ne durent que des fractions de milliseconde ; en fin de lot,
un autre audit a lancé des expériences concurrentes de travail discret.
Ils ne constituent pas une ablation G4. Pour cette raison, aucune synthèse
de speedup ni extrapolation vers les 100 ms n'est donnée.

`capture.json` conserve l'essai initial **FAILED** : trois tailles uniformes
ont fini, puis terrain8k a rencontré un plancher de test incorrect exigeant
un repli i128 dans chaque famille. Or le terrain sondé est entièrement
admissible en i64. Le correctif ne touche pas l'arithmétique ; il place la
non-vacuité de chaque branche au niveau de la campagne et des portes
numériques. Les lignes uniformes (parts i64 89,89 / 93,96 / 96,60 %) sont
historiques, pas membres du reçu r2 clos ; les sources/sondes de test initiales
ont changé et le lecteur LIVE r2 refuse donc de promouvoir ce premier lot.

Relecture du reçu clos, puis douze corruptions de contrôle du lecteur :

```sh
python3 morsehgp3D_v9/audits/b_s2_narrow_20260926/run.py --read morsehgp3D_v9/audits/b_s2_narrow_20260926/capture_r2.json
python3 -O morsehgp3D_v9/audits/b_s2_narrow_20260926/run.py --read morsehgp3D_v9/audits/b_s2_narrow_20260926/capture_r2.json
python3 morsehgp3D_v9/audits/b_s2_narrow_20260926/readback_gate.py morsehgp3D_v9/audits/b_s2_narrow_20260926/capture_r2.json
python3 -O morsehgp3D_v9/audits/b_s2_narrow_20260926/readback_gate.py morsehgp3D_v9/audits/b_s2_narrow_20260926/capture_r2.json
```
