# adaptive2 : compilation du test de faute refusée

Source exécutée `18d1ba695022457b82117df3cc2dfba821d908ae`, session
`v11.20261002.adaptive2`. Le worker termine code1, `DONE=3` ; campagne
non conforme, sans essai du banc adaptatif.

| Configuration | Portes passées/sélectionnées | Échecs CTest |
|---|---:|---:|
| Release u18 | 469/471 | 2 |
| ASan/UBSan u24, TSan u21, u21, u24 | 394/396 chacune | 2 chacune |
| Poison u21 | 395/397 | 2 |
| Mutants u18 | 20/21 | 1 |
| Style | 2/2 | 0 |
| Clang facultatif | absent | — |
| Supplément ASan/UBSan u18, num/index/tower | 176/178 | 2 |

La matrice totalise **2 462/2 475**, avec 13 échecs CTest ; le supplément
ajoute deux échecs. Tous ont un résultat clos, sans délai ni interruption.
Les deux portes `mhgp11_tower_memo_fault_starvation` et
`mhgp11_tower_memo_fault_inventaire` échouent au lancement dans sept
configurations : **14 lancements impossibles, aucun corps de ces tests
exécuté**. CTest les classe en échec, pas en `not_run`.

Le diagnostic de construction est identique : `memo_fault.cpp:31`,
`-Werror=misleading-indentation`. La source figée place `const auto
preserved=times;` sur la même ligne après la boucle `for`. Le collecteur
poursuit les autres portes malgré cette cible non construite. Ce défaut
de test ne démontre pas un défaut d'allocation ou de géométrie du produit.

La porte des mutants tower échoue également à construire son témoin sain :
son JUnit dit explicitement `TEMOIN ROUGE module=tower : aucun mutant juge`.
Il ne s'agit ni d'un mutant tué, ni d'un mutant survivant. Six autres portes
de campagnes de mutations et les quatorze portes de manifestes passent ;
aucun nouveau décompte individuel des mutants n'est revendiqué ici.

Le banc adaptatif refuse code2 en 0,036s, avant rapport et avant essai natif,
car la qualification est non conforme. Aucun chrono catalogue, mémo ou FULL,
ni aucune mesure de réutilisation sémantique, ne provient de cette capture.
Les succès fonctionnels partiels ne rendent pas la campagne conforme et
ne qualifient aucun clustering.

Le reçu original atteste l'arrêt ciblé de la génération exécutée, la
vérification des résultats, le retrait de la clé OS Login, la suppression
de la clé privée et la libération du verrou ; erreurs et avertissements
sont vides. Aucune récupération complémentaire n'est nécessaire ici.

```sh
python3 morsehgp3D_v11/receipts/catalogue_adaptive_20261002/adaptive2_failure/check.py
python3 -O morsehgp3D_v11/receipts/catalogue_adaptive_20261002/adaptive2_failure/check.py
python3 morsehgp3D_v11/receipts/catalogue_adaptive_20261002/adaptive2_failure/check_selftest.py
python3 -O morsehgp3D_v11/receipts/catalogue_adaptive_20261002/adaptive2_failure/check_selftest.py
```

Code0 signifie **cohérence d'une campagne échouée**. Le lecteur LIVE exige
le reçu brut local, son hash, le commit Git figé et l'archive originale
unique. Il rejugera inventaires/JUnit/comptes, diagnostics de compilation
et de lancement, cache et options de chaque configuration, copies compactes,
manifeste tar, commande exacte et fermeture. Le tar est lu sans extraction.
Les sources, dont `semantic_cache.py`, sont épinglées par commit et hashes ;
aucun pilote WIP n'est importé. Les aides du lecteur `adaptive1_failure`
servent uniquement aux contrats communs de preuve.

Contrôles purs normal/−O : **5 témoins, 36 corruptions refusées**, zéro appel
natif. La capsule ne copie ni paquet source massif ni données KITTI.
