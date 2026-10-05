# Porte supports_route de fina2 : attentes gravées dans le mauvais profil

Source figée : `38b76701b9b0198fc1c37afe16e1480e638e513c`. Lecture seule ; aucune compilation, exécution native ou action G4 par cet auditeur. La session fina2 était encore en clôture lors de cette contrelecture ; son autorité globale appartient à la capsule de reçus du root.

Les six attentes de `tests/api/tests.cmake:66–71` contiennent les mêmes préfixes SHA-256 de fichier et de manifeste, sans condition sur `MHGP11_COORD_BITS` dans cette boucle. Ces valeurs viennent de la référence u21 de L2b. Le juge `cmake/run_expect.cmake` exige la ligne entière exactement, sur la même exécution que le code de retour.

Ces deux empreintes ne sont pas des invariants entre profils :

- `src/api/write_supports.cpp:140` écrit `kCoordBits` dans le deuxième mot u64 après la magie, à l'offset 16. `static_proof.json` rejoue uniquement les 24 premiers octets du contrat : les octets de cette colonne valent 18, 21 ou 24. Il ne prétend pas calculer le SHA d'un fichier natif complet.
- `src/api/manifest.cpp` écrit `coord_bits`, le SHA du fichier brut et `tree_k_sha256`. Les signatures de géométrie et d'arbre incorporent elles-mêmes `kCoordBits`. Les lignes exactes figurent dans `source_anchors.json`.
- `tests/api/supports_route.cpp:255–256` lit les fichiers bruts, et `:397–398` imprime les préfixes de leurs SHA. Aucune normalisation du profil n'intervient.

Constat démontré : le contrat de la porte applique des empreintes u21 à u18/u24. Ce défaut de qualification ne démontre aucune erreur du moteur. Il rend les attentes incorrectes même si les voies produisent des résultats conformes dans leur propre profil.

L'archive conserve bien six `Failed` dans `gcc_release` (u18) et six dans `bits24`, aucun échec de ces portes dans `bits21` et `poison` (u21). Les quatre `LastTest.log` ont seulement 121 octets de début/fin ; `ctest.log` ne contient aucune sortie runtime de la sonde (`ECART`, verdict, JSON de pics), et les `failures.excerpt` sont vides. On ne peut donc affirmer que les douze échecs viennent *uniquement* de la ligne attendue, ni attribuer un défaut produit. L'autre condition `peak_a != peak_b` reste une hypothèse de diagnostic sans observation causale dans ces reçus.

## Correction minimale proposée

Conserver intégralement les comparaisons d'identité : fichiers et manifestes bruts entre `order_tree` et `full_tower` (:264), entre W1/W4 (:363) et avec l'appel public `compute` (:376), ainsi que les planchers, comptes et registres de journal gravés. Préserver aussi les empreintes gravées u21.

Pour u18/u24 seulement, demander une ligne de verdict contenant les mêmes comptes et le même journal, sans les deux préfixes SHA u21 ; imprimer les SHA réels dans une ligne JSON descriptive séparée. La branche u21 garde sa ligne actuelle. Cela demande seulement d'accorder la sonde et `tests.cmake` au profil, sans changer le moteur, les sorties publiques ou le format, sans normaliser les fichiers et sans fabriquer des nouvelles références à partir du programme jugé. Le contrôle relationnel des voies reste octet pour octet dans tous les profils. La réparation doit ensuite être rejugée sur ces mêmes portes épinglées ; aucune réussite nouvelle n'est déduite de cette proposition.

## Rejeu borné

Depuis cette capsule :

```sh
python3 replay.py
python3 -O replay.py
```

Les deux sorties sont identiques octet pour octet à `static_proof.json` (SHA-256 : b739ccae579a18a278ccc83b45bd7b30edca8fe1c2421ee269b3da5f0e64491e). Le script est stdlib uniquement : il vérifie les sources figées, les six lignes inconditionnelles, les champs dépendant des bits et les comparaisons d'identité conservées. Il ne lance ni CLI ni sonde. `log_facts.json` garde le SHA de l'archive et des logs d'origine ; les extraits de statuts et les quatre `LastTest.log` sont présents sous `logs/`. Les sources produit ne sont jamais modifiées.
