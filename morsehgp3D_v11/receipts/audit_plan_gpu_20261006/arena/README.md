# N1 : ownership favorable en lecture, budget et portes à adapter

Coupe WIP cinq fichiers du 6 octobre, base `cf5da0e91fb8740c1d748577fefb129c37b02d3d` dans `build/v11-impl-l3`. Les empreintes avant/après la capture sont identiques. Ce brouillon n'est ni publié ni qualifié par cette revue ; aucune compilation, exécution native ou action G4.

Le WIP conserve les `ReadyNode` possédés de la frontière, contrairement à la conversion globale en vues envisagée par le plan. Dans `boxes.cpp:128–147`, la liste du parent reste vivante pendant les deux enfants et le mark est restauré même sur `Outcome` en échec. Les appels de suffixe n'enregistrent aucune vue dans la frontière. Le choix de lane reste celui de `single_pass.cpp:54` et `parallel.cpp:37`, avec un Workspace privé ; aucune contradiction importante d'ownership trouvée dans ce périmètre. Ce constat de lecture n'est pas un test de concurrence. La préparation utilise déjà des Workspace par défaut à capacité 0 : `single_pass.cpp:189–193` et `parallel.cpp:97–103` pour le prélude, `adaptive_frontier.cpp:66–70` pour chaque enfant de ronde. Ces chemins appellent `prepare_node`, pas `process` ; ils gardent leurs Buffers possédés et n'ont pas besoin de cette arène de suffixe.

L'arène réelle est fixe : `internal.hpp:48`, 2^18 SiteIdx, soit 1 MiB par lane sous le contrat SiteIdx/u32. `catalogue.cpp` admet et alloue cette capacité à chaque Workspace, désormais en dernier pour préserver l'ordre des allocations historiques. Les admissions de `single_pass.cpp:196–205` et `parallel.cpp:109–119` ajoutent encore le majorant historique des buffers de suffixe. Cela reste conservateur et permet le repli exact ; 48 lanes actives coûtent déjà 48 MiB d'arènes, avec frontière, autres scratchs et sorties en sus. Aucun gain ni absence de repli ne sont démontrés.

Cas source utile pour une porte de budget : `tests/catalogue/adaptive_support.hpp:25–26` donne neuf points ; `adaptive_frontier.cpp:108–129` exige 37 tâches/75 nœuds à K1/leaf4. La porte actuelle est W4/budget illimité. Son extension à W16/16 MiB ne peut pas être admise dans ce WIP : 16 arènes occupent seules les 16 MiB, puis les autres scratchs positifs et la frontière s'ajoutent. La sélection du plan ne dépend pas de W (`adaptive_prepare.cpp:46–63`), et l'arrêt normal à 38 feuilles, sous le plafond 1024, implique des tâches terminales : ces arènes n'y servent pas au parcours de descendants. Il s'agit d'une prédiction source à tester, pas d'un échec natif observé. Pour l'ablation A/B du même binaire annoncée au plan:323, rendre la capacité explicite, 0 pour la référence, 2^18 pour le candidat ; un suffixe nul peut garder une arène nulle.

Deux mutations du plan ne sont pas causales telles qu'écrites. `make_root` garde son Buffer hors arène (`boxes.cpp:214–228`) : 3B+1 listes filtrées suffisent pour le walk serial, et un suffixe après frontière possédée a la borne plus fine `count*(3B-depth)` (`frontier.cpp:118–119`, `adaptive_frontier.cpp:183`). Réduire 3B+2 à 3B+1 conserve donc une borne valide. Dans le WIP fixe, supprimer le rewind peut augmenter les replis Buffer exacts sans changer dump/ledger : le modèle borné le montre. Le contrôler par retour au mark (et top=0 entre tâches), high-water et nombre de replis/allocations rend la porte causale. Le modèle n'est pas un nuage naturel ni une preuve d'alias C++.

Contrôles ciblés utiles avant adoption : capacité exactement suffisante puis un élément trop courte, mélange arène/repli avec sortie et ledger identiques, restauration du mark après refus injecté, voie référence/candidate dans le même binaire, W1/W4/W48 et vérification de lane privée. Les portes natives futures appartiennent au développeur ; aucun résultat n'est déduit de leur rédaction. Le déplacement `prepare_node` de l'allocation avant le contrôle local max_nodes est visible mais aucun enjeu matériel démontré n'est remonté ici.

Rejeu stdlib, sans écrire le JSON figé :

```sh
python3 -B replay.py --check proof.json
python3 -O -B replay.py --check proof.json
```

`source_manifest.json` ancre le plan et les cinq fichiers ; `wip.diff` compare uniquement ces fichiers à la base. Les deux fichiers de faute changent le nombre attendu d'allocations pour compter l'arène ; leur présence n'est pas un verdict de test.
