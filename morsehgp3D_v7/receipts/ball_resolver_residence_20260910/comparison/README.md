# Mémo exact des représentants FULL — variante privée

2026-09-10. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`. GCP non utilisé. Aucun fichier actif, index Git ou
fichier de l'auditeur modifié. Base annoncée par ROOT : `d188e3de`.

## Delta unique

Une table directe facultative mémorise le résultat du resolver pour une facette
triée d'indices géométriques Morton. Les dix cases de la clé sont comparées,
jamais seulement son hash. Une collision évince une entrée : elle peut ralentir,
pas changer une réponse. Aucun sondage linéaire ni liste de collisions.

Le token mémorisé est normalisé par `root(token, prior_count)` à chaque hit.
La table est remise à zéro entre les ordres K. Elle ne contient pas de catalogue
Gamma, seulement une fenêtre de réponses déjà certifiées et éventuellement de
naissances fermées. Les fichiers MEB et leurs décisions/comptages sont inchangés.

Deux modalités isolent le mécanisme :

- `plain` mémorise seulement les résolutions antérieures réussies ;
- `seed` ajoute, après fermeture complète du lot, la facette unique `I ∪ U`
  quand `K = |I| + |U|`. Aucun sous-ensemble d'une coquille n'est énuméré.

## Justification temporelle

Une résolution réussie établit l'appartenance de la facette à une composante
avant le lot courant. Les niveaux des lots de l'ordre courant croissent ; une
composante ne fait que continuer ou fusionner. Son token historique se normalise
donc vers la bonne racine pré-lot lors des consultations suivantes, y compris
pour une seconde résolution du même lot. Les tokens sont privés à l'ordre K.

Pour le semis, le support positif de B est contenu dans `I ∪ U` et tous ces
points sont dans B : B est sa MEB exacte. Lorsque ce groupe a cardinal K, c'est
l'unique facette K de sa population fermée. Son ancre est publiée après toutes
les résolutions du lot. La prochaine consultation intervient à un niveau
strictement supérieur ; sa naissance n'est donc jamais anticipée.

## Coût explicite

La capacité est la plus petite puissance de deux au moins égale à `16 n`.
Pas de plafond fixe. Une entrée occupe 48 octets dans cette compilation : coût
entre `768 n` et moins de `1536 n` octets, partagé/réutilisé entre les ordres.
Chaque remise à zéro visite exactement la capacité (compteur dédié). Lookup et
insertion sont O(K), K ≤ 10, sans parcours dépendant du nombre de collisions.
Échec d'allocation ou capacité non représentable désactive seulement le cache.
Cette allocation supplémentaire reste significative pour plusieurs dizaines de
millions de points ; elle n'est pas une qualification de mémoire G4.

## Sources, exécution et anti-dérive

`baseline/` et `cache/` conservent la première gate 24 nuages. Les répertoires
séparés `baseline_final/` et `cache_final/` conservent la gate renforcée 28 nuages
de ROOT : tower `bf1a28242dd2d6897f3dc02edff8c6077dbec1caa3f9b9a67813dcc748e3d75a`,
work `25d5cf9a193db2eefe432f21bf823941402d82975c53f92660a9f984fb57498c`.
Ce sont des copies physiques complètes, sans lien vers les sources actives.

Chaque `run_*` contient la copie du recorder, les commandes fermées, sorties
brutes et SHA avant/après de tous les en-têtes et unités privées. Les options
SAN finales emploient des exécutables non-PIE et ASan/UBSan avec arrêt sur erreur
et détection des fuites. Les options et sources effectives sont dans chaque reçu.

La porte `resolver_cache_gate` vérifie hits/misses exacts, collisions physiques,
éviction non vacue, remise à zéro, comptages, et cache facultatif indisponible.
`cache_mutants.py` conserve trois sources mutées privées : identité ignorée,
normalisation omise, remise à zéro par ordre omise. Aucun mutant produit actif.

Les diagnostics viennent seulement après les portes : n400 puis n1000,
uniform/graine3/s8/K1..10/mono-thread, un échauffement puis trois répétitions
appariées à ordre alterné pour baseline/plain/seed. Digests d'entrée et payload
doivent être identiques. Temps tour et total restent séparés. Aucun contrat
50k ou GPU n'est déduit de ces diagnostics.

## Résultats qualifiés et diagnostics

O2 et ASan/UBSan : `plain` et `seed` passent les portes tour, travail et cache
(selftest 0, argument inconnu 2), avec sources inchangées et mêmes sorties
nominales O2/SAN. La porte tour a 170 320 contrôles, 28 nuages, 112 ordres,
2 508 coupes, 45 948 contrôles verticaux, 504 naissances, 224 fusions et huit
instantanés de croissance. Les compteurs d'objet sont identiques à la baseline.
Seul le nombre de résolutions passant réellement par la recherche d'ancre change.

Les mutants identité ignorée et token non normalisé sont tués ; ce dernier
échoue sur E5 avec `full_ball_final_component_count`. Le mutant sans reset entre
ordres survit : les clés complètes de facettes triées, distinctes, non négatives,
avec padding nul encodent déjà K implicitement lorsque K ≥ 2. Le reset n'est
donc pas présenté comme condition géométrique indispensable de ce layout ; il
fixe la résidence privée par ordre. La sortie brute de cette fausse attente
initiale et son échec de recorder sont conservés, puis l'anti-dérive est vérifiée
séparément dans `summary.json`.

Diagnostics locaux partagés, trois répétitions avec ordre alterné ; tous les
digests d'entrée et de payload sont identiques :

| n | bras | appels MEB réels | supports exacts testés | tour médiane, s | total médian, s |
| ---: | --- | ---: | ---: | ---: | ---: |
| 400 | baseline | 392 135 | 31 719 733 | 2,7205 | 4,7015 |
| 400 | cache seul | 251 687 | 21 519 754 | 2,1252 | 4,1356 |
| 400 | cache + semis | 180 260 | 15 708 447 | 1,6217 | 3,6235 |
| 1 000 | baseline | 1 174 515 | 97 376 638 | 8,3710 | 15,2912 |
| 1 000 | cache seul | 783 299 | 68 700 123 | 6,4347 | 13,3494 |
| 1 000 | cache + semis | 583 337 | 52 168 577 | 5,2963 | 12,2319 |

L'attribution physique est stable : cache + semis retire 54,03 % puis 50,33 %
des appels MEB, et 50,48 % puis 46,43 % des supports évalués. Les temps sont
diagnostiques, car ROOT compilait des mutants et un autre agent compilait NVCC
pendant une partie de la campagne ; le triplet supplémentaire `run_quiet_diagnostics`
conserve les processus observés avant/après, mais n'est pas un contrat.

Mémoire du cache : 393 216 octets à n400 et 786 432 octets à n1000 ; les pics
RSS respectifs baseline/cache/cache+semis sont 80 896/81 588/81 540 Kio et
230 272/226 880/226 752 Kio. La baisse RSS à n1000 ne prouve pas un gain mémoire
du cache, qui ajoute bien son allocation ; le layout de l'allocateur change.
Le même dimensionnement prend 24 Mio à 32k, 48 Mio à 50k, 24 Gio à 30 millions :
ce dernier coût impose de requalifier le compromis de capacité pour G4.

`summary.json` vérifie aussi, ligne par ligne, les identités physiques :
consultations = hits cache + hits d'ancre, appels MEB = hits d'ancre + requêtes
d'intrus, insertions = hits d'ancre + semis, resets = capacité × (Kmax−1), et
mémoire = capacité × 48. Les relations et la non-vacuité des deux gains passent.

Le delta candidat est `full_ball_tower.patch`, SHA-256
`fd020331a4c5d1ef98a9f1cf9922cc858726217c33cfa05acb1fc3355758894d` ;
header baseline `0b72b4e9cb3858f7026d6b5d2b55f8a7b191903fc37aa24c140dfb8b557657e8`,
header candidat `70f53814edd6ee5c44715e9f5d5f97908241aa50e9f7e39b2dc42dd6a0d5c8f2`.
Le bras retenu emploie `-DMHGP7_PRIVATE_RESOLVER_SEED=1`. Toute promotion
normalisant ce commutateur privé doit requalifier les octets promus.

Triplet supplémentaire n1000, un passage par bras : tour
8,3463/6,8526/5,7408 s et total 15,1316/13,9032/13,0882 s
(baseline/cache/cache+semis), mêmes digests et compteurs physiques.
Aucun compilateur n'était observé avant ; un apparaît au relevé final, donc le
nom technique `run_quiet_diagnostics` ne vaut pas certification d'isolation.
Le gain temporel reste visible mais son amplitude doit être requalifiée sur G4.
