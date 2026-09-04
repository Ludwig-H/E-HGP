# Portes arithmétiques intégrées — reçu du 4 septembre 2026

`phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

**316/316 portes CPU exécutées à nouveau, code 0, sans censure ni test
ignoré.** Ce résultat suit un build Release explicitement incrémental ;
ce n'est ni un build complet neuf/hermétique, ni une addition de verdicts
historiques. Les [24 portes ciblées](targeted.summary.json) passent aussi
dans deux builds neufs du vrai CMake, Release et ASAN/UBSAN.

## Changement et provenance

Deux sources de test ont été portées octet pour octet depuis l'overlay
local scellé `build/v7_arithmetic_gates` :
[entiers](../../tests/arithmetic_integer_gate.cpp) et
[lanes](../../tests/arithmetic_lanes_gate.cpp). Le [CMake réel](../../CMakeLists.txt)
inscrit leurs deux cibles et 24 portes. Son chemin d'inclusion est directement
`CMAKE_CURRENT_SOURCE_DIR` ; le hook d'override de provenance de l'overlay
n'a pas été porté. Aucun src/oracle/CLI ni registre de mutants n'a changé.

Les [empreintes et flags réels](targeted.build_binding.json) attestent
l'identité des deux sources portées et la conservation des 22 fichiers
scellés de l'overlay. [overlay.historical.sha256](overlay.historical.sha256)
est une **copie historique** de son inventaire : ses chemins sous build/
ne sont pas des dépendances distribuées. Les résultats propres à cet
overlay (0,31 s et 0,59 s) ne sont pas ceux de l'intégration ci-dessous.

Les [obligations statiques](../../docs/ARITHMETIQUE_PRIMITIVES.md), le
[plan des portes](../../docs/PLAN_PORTES_ARITHMETIQUES.md), les contrelectures
externes et la fixture littérale de l'auditeur sont attribués et épinglés
dans [overlay.context.sha256](overlay.context.sha256). Les deux cas externes
repris sont Cassini q3 avec G=1 et centre lointain, puis le premier bit 256
d'un croisement générique U320. Le PGCD binaire et la division longue du
juge sont un port explicitement attribué à `linked_arcs_gate.cpp`.

## Exécutions intégrées

| Exécution | Build | Résultat | Temps réel CTest |
| --- | --- | --- | --- |
| Deux cibles, Release strict | Neuf, vrai CMake, cibles seules | 24/24, code 0 | 0,36 s |
| Deux cibles, ASAN/UBSAN/leaks | Neuf, vrai CMake, cibles seules | 24/24, code 0, aucun diagnostic | 0,61 s |
| Ensemble des portes CPU | Release incrémental, tests tous réexécutés | 316/316, code 0 | 558,50 s |

Les durées sont celles rapportées par CTest, pas des performances HGP.
Le runner complet observe 558,557656493 s pour sa commande CTest. Le build
incrémental prend 3,826323410 s ; configuration et inventaire ont également
le code 0. La suite est limitée à 7200 s et deux tests concurrents.

Les [commandes exactes](commands.txt), logs `targeted.*.log`,
[JUnit Release](targeted.release.junit.xml),
[JUnit sanitizers](targeted.sanitized.junit.xml) et
[inventaire ciblé](targeted.inventory.json) sont conservés. Les résultats
`targeted.*.result.json` sont les observations brutes de l'outil :
`wall_time_seconds` est une durée d'appel, pas nécessairement la durée
totale du processus. GCC 13.3.0, CMake/CTest 3.28.3, C++20,
-Wall -Wextra -Wpedantic -Werror ; Release -O3, sanitizers -O1.

La [suite complète](full_release/summary.json) possède son propre
[JUnit](full_release/ctest.junit.xml), ses logs stdout/stderr,
ses tentatives, codes, délais, pins et [liaison de build](full_release/build_binding.json).
Elle réutilise explicitement `build/v7_c_qualification`, pas ses anciens
résultats. Les 316 noms uniques correspondent exactement à
[expected_test_names.json](expected_test_names.json) ; aucun
failure/error/skipped ni statut autre que run n'est accepté.

## Causes exercées, pas simple présence de tests

Les 24 inscriptions comprennent six nominales, sept mutants, un refus
d'overflow du juge et dix refus d'usage. Codes et préfixes sont contrôlés
dans la même exécution par le wrapper CMake réel ; leur liste complète
est [targeted.registration_cases.json](targeted.registration_cases.json).

La troncature U192 est réfutée sur le numérateur de 136 bits d'un vrai q4,
avant tout produit U320. La troncature U320 est isolée sur des niveaux
littéraux génériques : aucun calcul U192 ne les construit en amont ;
les mots hauts aux bits 256 et 316 sont effectivement non nuls.
Ces niveaux génériques ne sont pas présentés comme des boules u16.
Les 50 croisements géométriques observés ont au contraire leur mot 4 nul.

Le nominal géométrique couvre 2/81/51 formes q2/q3/q4, les deux orientations,
rangs nuls, frontières strictes, trois signes de puissance, C q3 positif
et négatif, C q4 non nul, et Cassini. Le nominal entier couvre 192 PGCD,
dont 185 couples de Fibonacci, les zéros, réductions, floors définis et
retenues collectives. Les rejets hors domaine appartiennent aux validateurs
de fixtures, pas à une API de refus inventée pour les helpers produit.

L'autorité locale est OBig plus identités littérales ; la porte géométrique
n'utilise pas Boost. La branche Boost facultative de la porte entière n'a
pas été compilée ici, où Boost est absent. Les mutants du juge doivent être
tués par l'identité littérale avant tout appel produit. Un overflow OBig
empoisonne le verdict : le cas autonome impose le code 3, jamais un succès.

## Conservation et limites

Les 47 sources src/oracle ont les mêmes
[empreintes avant](sources.before.sha256) et [après](sources.after.sha256).
L'inventaire plus large de la suite contient 139 fichiers inchangés et
l'inventaire de 36 binaires conservé autour des tests reste identique.
Les deux CLI protégés restent
`25c9bf8e4ef3cded5647a22f16d81af7a1e778196ad3bff73884a7f58da985f2`,
[avant](cli.before.sha256) et [après](cli.after.sha256).
Le reçu C historique conserve les 46 fichiers de son propre sceau.

[run_full_release.py](run_full_release.py) charge des octets vérifiés du
helper historique, redirige explicitement son build et son nouveau dossier
de sorties, et refuse de réécrire un reçu existant. Seul l'ancien inventaire
de noms contribue aux attentes, aucun ancien verdict. Le juge JUnit est
testé en Python normal et -O : une fixture positive et sept rejets chacun.

`manifest.json` résume le périmètre ; `SHA256SUMS` scelle les fichiers de
ce reçu, sans aucun binaire. Le sous-dossier full_release a aussi son propre
manifeste. Une reproduction doit employer une nouvelle destination :
ne pas supprimer ce reçu pour relancer `--execute` en place.

Ces exécutions ne remplacent pas les preuves de bornes et leurs préconditions,
ne certifient pas à elles seules l'objet HGP complet, une publication exacte,
la performance 50k ou massive, ni CUDA/GPU. Aucun changement de statut public,
aucun commit, aucune opération GCP dans cette sous-tâche.
