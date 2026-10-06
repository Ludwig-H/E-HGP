# L4 : conserver le tableau emprunté jusqu'à la jonction sur refus

Constat source au pin `cf28afb04040eacf1eb92080d75e8a0ebb58a4eb` (première partie L4/T3), comparé à `c1675e4c9`. Aucun build, test C++ ou lancement cloud ; les captures sont extraites par `git show` du commit, indépendamment du chantier vivant.

Dans `single_pass.cpp:226–227`, `lane` est déclaré avant le tableau `claimed`. `start` reçoit une vue de ce tableau (:233), puis `State::queues` conserve ce span (`leaf_overlap.cpp:59,122`). Le fil charge ses éléments dans `gather_leaves` (:75–77 ; `single_pass_batch.cpp:184–186,195–198`). Sur refus de `parallel_for` (:237), ou de `prefix` (:241), `MHGP11_TRY` retourne immédiatement (`core/status.hpp:182–185`). Le tableau se détruit avant `~OverlapLane`, qui appelle ensuite `abort` puis `join` (:106–110). Si le fil est déjà sorti de son attente, `abort` n'interrompt pas le `run_chunk` en cours (:95–99) : il peut encore charger un pointeur dans le tableau dont la durée de vie a fini. Les queues et leurs pages restent possédées par `outputs` ; le problème concerne le tableau de pointeurs emprunté.

Le transfert par retour détruit les variables automatiques dans l'ordre inverse de leur construction ; cette règle s'applique même si la destruction de l'objet est triviale. Lire la valeur d'un objet après la fin de sa durée de vie n'est pas autorisé par les règles générales de durée de vie. Sources primaires consultées : [ordre de destruction du draft C++](https://eel.is/c++draft/stmt.dcl#2), [fin de vie](https://eel.is/c++draft/basic.life#2), [accès hors durée de vie](https://eel.is/c++draft/basic.life#8).

Correction minimale : déclarer `std::array<const TaskLeafQueue*, Capacity> claimed{}` avant `OverlapLane lane`. Le destructeur de lane rejoint alors le fil avant la fin de vie du tableau, sur succès comme sur refus. Une copie possédée des pointeurs par State serait une autre solution, mais n'est pas nécessaire pour réparer ce chemin. Aucun changement géométrique requis.

`replay.py` énumère les entrelacements d'un sous-lot déjà prêt et d'un refus du Pool. Témoin : fil prêt → retour refusé → fin de vie de claimed → abort → lecture claimed → fin du fil → join. Le même modèle impose join avant la fin de vie de claimed avec les déclarations corrigées et n'admet plus de lecture hors-vie. Ce modèle explique la causalité du défaut source ; il ne prétend pas reproduire un crash ou un diagnostic sanitizer natif.

Porte G4 ciblée suggérée : injecter un refus dans une tâche pendant qu'un sous-lot précédent a quitté son attente ; bloquer temporairement le consommateur avant la lecture des pointeurs par une barrière de test. Reprendre ce consommateur pendant abort/join et vérifier, sous ASan/UBSan avec contrôle de durée de vie de pile, refus conservé, absence de sortie partielle, budget rendu et absence d'accès hors-vie. Une barrière rend le chemin causal et évite de compter sur la vitesse relative des fils. Le mutant inversant les deux déclarations doit être détecté par cette porte. Aucun résultat de cette porte n'est disponible ici.

Le reste du chemin examiné conserve les synchronisations de publication des files sous mutex, le Pool privé du fil et la jonction avant la concaténation normale. La concaténation et les relancements par sous-lot restent des coûts à mesurer du prototype ; ils ne sont pas assimilés ici à un défaut d'une tranche encore en construction.

Rejeu stdlib en lecture seule du JSON figé :

```sh
python3 -B replay.py --check proof.json
python3 -O -B replay.py --check proof.json
```
