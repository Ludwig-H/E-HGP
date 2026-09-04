# Tris locaux des indices : contrat courant

Cadre : `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

La route par indices et ses allocations sont requalifiées dans [l'audit indépendant courant](AUDIT_RESIDENCE_20260904.md). Cette note décrit le contrat de la primitive et la porte du constructeur. Les résultats de l'auditeur ont leur propre reçu épinglé.

## Correction et portée

Dans [sort.hpp](../src/parallel/sort.hpp), seule la route de permutation appelle `sort_direct<true>`. Ses tranches utilisent maintenant `std::sort`. `IndexLess` départage chaque égalité de clé par l'indice original : deux indices distincts ne sont donc jamais équivalents. Le tri local, stable ou non, doit produire l'unique même permutation. L'application par cycles et les fusions sont inchangées. La route générique conserve `std::stable_sort` et sa stabilité observable sur les charges utiles distinctes.

Le modèle corrigé distingue le payload des indices du processus : au plus `8n` octets pour les deux tableaux d'indices parallèles, `4n` au séquentiel, puis un enregistrement temporaire pour les cycles. Les plans, équipes, barrières et piles s'ajoutent ; ce n'est pas une borne RSS. L'absence de tampon proportionnel à `n` dans le `std::sort` retenu est testée avec la bibliothèque effectivement compilée, pas postulée comme une propriété de toute bibliothèque C++.

La correction ne crée aucune cellule, coface ou incidence supplémentaire, ne change aucun seuil géométrique et ne reconstruit pas la mosaïque de Delaunay. Elle réduit seulement un coût intermédiaire de tri.

## Porte permanente et mesure

[perm_residence_gate.cpp](../tests/perm_residence_gate.cpp) intercepte les allocations C++ ordinaires avec leur durée de vie. Ses enregistrements de 72 octets portent une clé pauvre, un rang original distinct et huit mots de charge utile ; la référence est `std::stable_sort` élément par élément, pas seulement une vérification de l'ordre des clés. Les tableaux d'entrée et de référence sont alloués hors fenêtre instrumentée. Les piles natives, les en-têtes de malloc et les allocations suralignées sont hors mesure ; les types mesurés ne sont pas suralignés.

La porte exerce 1 000, 32 768 et 65 536 éléments à 1, 2, 4 et 8 fils. Elle vérifie les ouvriers réellement créés, le nombre de tableaux d'indices, l'absence de gros tampon additionnel, les allocations résiduelles nulles et la stabilité des deux routes. Son témoin négatif réexécute explicitement la route historique sur les indices ; un rendez-vous impose la coexistence des deux temporaires locaux pour vérifier que la sonde les voit effectivement. Ce témoin doit rester stable malgré sa résidence supérieure.

Compilation locale avec GCC 13.3.0 / libstdc++ :

```bash
c++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -DMHGP7_TESTING -pthread morsehgp3D_v7/tests/perm_residence_gate.cpp -o build/v7_scale_audit/mhgp7_perm_residence_gate
build/v7_scale_audit/mhgp7_perm_residence_gate
```

Codes de compilation et du nominal : 0. Résultat brut :

```text
perm_residence cases=12 ties=99211 sequential_peak=131072 parallel_peak=262736 legacy_peak=328272 legacy_simultaneous=2 allocation_failures=15 comparator_failures=5 launch_failures=1 failures=0
```

Pour les 32 768 éléments, le pic mesuré passe de `328272 = 10n + 592` à `262736 = 8n + 592` octets en parallèle ; les 592 octets restants sont des allocations instrumentées de métadonnées. Le séquentiel corrigé mesure `131072 = 4n`. Ce sont des mesures d'allocations auxiliaires sur cette sonde, pas un gain RSS ou une accélération du pipeline.

Les quinze allocations du tri parallèle corrigé sont refusées une à une. Les cinq pannes de comparaison couvrent le séquentiel, les tranches parallèles et les fusions inter-tranches. Un lancement partiel à deux fils créés sur quatre demandés est également refusé. Chaque panne doit laisser l'entrée inchangée avant l'application de permutation, drainer les fils et restituer toutes les allocations suivies. Un tri réussi après les pannes vérifie la réutilisation de la primitive.

Les trois invocations `--inject=perm-apply-scatter`, `--inject=perm-apply-partial` et `--inject=perm-tie-desc` rendent exactement le code 4. Les deux premières divergent sur les douze scènes et cassent l'ordre ; la dernière diverge sur les douze charges utiles en conservant l'ordre des clés (`unsorted=0`). Un mauvais argument est refusé en 2. Une sonde non exercée ou un rendez-vous non établi est refusé en 3, jamais présenté comme une réussite.

## Épinglage et limites

| Fichier | SHA-256 |
| --- | --- |
| `src/parallel/sort.hpp` | `ceb89f8ba42fbc3f44351f4dcbf5e347e7d66a4c85bce08f346e66a9b11f20bd` |
| `tests/perm_residence_gate.cpp` | `5660d9b0f9b5e30d50107ed75778c680f50561ff7058bbbcd508c56591d9603b` |

Les raccords CMake et la suite globale sont confiés au constructeur et doivent être rapportés séparément après reconstruction. Cette note ne qualifie ni les contrats 50k, ni les dizaines de millions de points, ni la complétude HGP. GCP non utilisé.
