# Premier build d'intégration, avant gel

21 septembre2026. Deux commandes ont retourné2 pendant que la porte globale
était encore en cours d'adaptation au nouveau registre :

```text
cmake --build build/v8_q4_seed_cells_20260921 -j 4
cmake --build build/v8_q4_seed_cells_sanitize_20260921 --target mhgp8_wspd_q34_probe mhgp8_q4_seed_cells_gate mhgp8_wspd_q34_gate mhgp8_q4_local_gate mhgp8_q34_pair_bounds_gate -j 2
```

GCC13 et Clang18 ont refusé la même assertion dans
`tests/wspd_q34_gate.cpp:80`, qui conservait387 mots pour `WspdQ34Work`
alors que le nouveau registre comporte424 mots. Diagnostic GCC :
`the comparison reduces to '(3392 == 3096)'` ; l'ancien `bit_cast`
vers387 mots échouait également. Moteur, sonde et porte seed_cells
instrumentée ont compilé. Aucune qualification close ni résultat de
performance n'est déduit de ces builds d'intégration.

L'adaptation de la porte, annoncée avant cet échec, doit conserver les
contrôles de taille et les étendre à424, pas les retirer. Reprise des
deux builds après sa livraison ; aucun build historique épinglé modifié.

Une première exécution directe Release de `mhgp8_q4_seed_cells_gate
--selftest` retourne1 avec `live summary did not visit each atlas cell
once`. Le moteur avait le court-circuit des atlas morts, mais la porte
avait été compilée pendant l'édition de son ancienne assertion : le texte
du diagnostic n'était plus celui de la source livrée. Les deux sources de
portes sont désormais stables ; leurs horodatages ont été actualisés pour
forcer une compilation cohérente. Aucun reçu qualifié n'était gelé.

## R1 : deux défauts du harnais, moteur inchangé

Les96 CTests Release et quatre portes/24sondes Clang ASan/UBSan passent.
Les24sondes Release, trois sondes disabled/scalar et six mesures8k sont
également closes. Mais le mutant supprimant les contacts positifs survit
à la porte : la fixture ne rend pas ce contact canonique dans la cellule
propriétaire. Le reçu `mutations/compiled_9pi54doq` doit rester un échec,
jamais un mutant tué.

L'autotest du lecteur laisse aussi survivre la corruption
`live_child_reads+1` lorsque des atlas entiers ont été écartés : la seule
borne supérieure ne suffit pas. Les deux échecs normal/−O sont conservés
dans `SELFTEST_R1_FAILURE.json`. L'invariant complémentaire requis est
la divisibilité par4, puisque toute branche visitée lit ses quatre enfants.

Avant toute correction, les216 sources R1 sont archivées dans
`SOURCES_R1.tar.gz`. Les builds `build/v8_q4_seed_cells_20260921` et
`build/v8_q4_seed_cells_sanitize_20260921` restent désormais épinglés.
La reprise utilise deux nouveaux builds suffixés `r2_20260921` ; seuls
la fixture et le lecteur doivent changer, pas le moteur ni les options.
