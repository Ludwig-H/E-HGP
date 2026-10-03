# graph2 — deux défauts de tests, aucun banc FULL lancé

Source poussée `245eee1aec7ee9e3e8fd50d227d20ab7554c75db`, session
`v11.20261003.graph2`, fermée `failed_remote` (worker 1, DONE 3).
Le reçu original certifie arrêt ciblé, génération, retrait de la clé OS Login,
destruction de la clé privée et libération de la réserve ; `errors` et
`warnings` sont vides. Le lecteur rend **coherent=true, conforming=false**.

| Configuration | Portes réussies / sélectionnées |
|---|---:|
| GCC Release u18 | 627 / 639 |
| Mutants | 20 / 21 |
| GCC ASan/UBSan | 552 / 564 |
| GCC TSan | 552 / 564 |
| Profil u21 | 552 / 564 |
| Profil u24 | 552 / 564 |
| Poison | 553 / 565 |
| Style | 2 / 2 |
| Total matrice | **3410 / 3483** |
| Supplément ASan18 | **297 / 309** |

Clang Release facultatif est absent. Aucun résultat de porte n'est manquant.
Les huit constructions concernées sont `build_failed` ; les tests disponibles
ont néanmoins été lancés. JUnit et les sorties complètes LastTest sont recoupés.

## Compilation de la nouvelle porte singleton

`tests/tower/descent_test.cpp:158` compare directement deux numérateurs de
`num::Level` avec `==`. Leur type est `Wide<3>` en u18/u21 et `Wide<4>` en u24,
sans cet opérateur. La ligne 159 fait de même pour les dénominateurs `Wide<3>`
en u21/u24 ; elle reste compilable en u18. Le lecteur exige exactement ces
diagnostics, leurs types et leurs lignes selon chaque configuration.

Le binaire `mhgp11_tower_descent` manque donc. Ses dix groupes et son inventaire
échouent au lancement dans les sept configurations fonctionnelles, supplément
compris : **77 portes**, code shell 127, sans exécution de leur contenu natif.
Le témoin de construction tower échoue aussi avant mutation :
`TEMOIN ROUGE module=tower : aucun mutant juge`. **Zéro mutant tower est jugé.**
Les six autres modules conservent **198 morts par code/ligne et deux refus de
construction attendus**, sans mort par signal ou délai ; l'inventaire exact est
vérifié contre chacun des manifestes de la source exécutée.

## Ancienne injection mémoire devenue inapplicable

`mhgp11_tower_vertical_parallel_fault_vertical_census_failure` s'exécute et
échoue dans les sept configurations fonctionnelles : 38 contrôles, huit échecs,
plancher 30. Le test attend un refus mémoire après les quatre allocations des
structures verticales, mais vise des descentes de l'ordre 2 vers des singletons
de l'ordre 1. Le raccourci singleton de cette source appelle seulement le MEB
borné puis construit sa graine, sans census ni allocation correspondante.

Dans les deux passages W1/W4, le refus attendu n'arrive pas, l'injection reste
non déclenchée, les diagnostics sont publiés et trois descentes sont comptées.
Le lecteur conserve les huit échecs exacts et leurs valeurs. C'est un défaut
de cible du test d'injection ; aucun défaut géométrique du produit n'est établi
par ces deux causes. Leur correction ultérieure doit être qualifiée séparément.

## Bancs et budget

Les commandes `002_pair_graph_leaf16` et `003_pair_graph_leaf8` terminent code 2
avec `full_parallel_refused: ValueError`, sans rapport ni tentative FULL.
Le calendrier épinglé déclare vingt unités par commande : six paires LiDAR
u21/u24 en modes 2047/4095 à W48, mode 4095 W1/W8 sur ng00 u21, puis trois
paires uniformes u21. Les **40 unités sont non démarrées**, pas des résultats
perdus (`unpersisted=0`). Aucun temps FULL, gain du graphe ou effet de leaf8
n'est mesuré ici. Le refus du collecteur ne conserve pas de traceback ; on
n'invente donc pas de diagnostic enfant plus précis.

Préflight : 850 + 180 + 570 + 570 = 2170 secondes de commandes, plus 120 de
préparation, soit **2290 ≤ 2336 secondes** utiles ; `oversubscribed=false`.
Les quatre groupes sont fermés, sans troncature, éviction ou groupe résiduel
tué ; le worker déclare `interrupted=0`. Les diagnostics préliminaires restent
dans `first_compile_failure.txt` et `first_vertical_failure.txt` : ils ne sont
pas l'autorité de clôture.

## Relecture LIVE, sans compilation ni cloud

```sh
python -B morsehgp3D_v11/receipts/full_pair_graph_20261003/graph2_failure/check.py
python -B -O morsehgp3D_v11/receipts/full_pair_graph_20261003/graph2_failure/check.py
python -B morsehgp3D_v11/receipts/full_pair_graph_20261003/graph2_failure/check_selftest.py
python -B -O morsehgp3D_v11/receipts/full_pair_graph_20261003/graph2_failure/check_selftest.py
```

Les lectures normal/−O donnent le même verdict. Les contrôles adversariaux
comptent **11 témoins et 75 corruptions rejetées**, notamment les deux logs
altérés ensemble, les raisons de compilation, les valeurs d'injection,
l'inventaire des mutants et les gardes de fermeture. Une dépendance altérée est
refusée avant import. Les premières lectures sont conservées dans
`reader_attempts.json` ; aucune n'a échoué.

Le lecteur exige le reçu original LIVE, l'archive originale, le paquet source
local et les objets Git. Le paquet a été déplacé vers un stockage local avec
symlink à son ancien chemin ; son hash reste vérifié. Les sept helpers sont
copiés depuis la source exécutée et épinglés avant import, sans dépendance au
WIP. Le calendrier seul est évalué depuis sa fonction AST épinglée.
Toutes les sources épinglées sont aussi comparées aux membres du paquet exécuté.
Une archive de résultats de 748550 octets est copiée ; aucun paquet source,
binaire natif ou payload LiDAR n'est ajouté à cette capsule. Les empreintes
d'entrée sont reliées au manifeste uploadé, sans prétendre relire les payloads.
