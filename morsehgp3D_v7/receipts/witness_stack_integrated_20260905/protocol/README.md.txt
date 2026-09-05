# Qualification F préparée — aucune porte moteur exécutée par cette préparation

Adaptation étroite du protocole E/q2 fermé `qualification.py` à SHA
`5f62b29d96895e9b46df591f0540dd5d4ecbd3fec4a8eb291e4e6d3c27fb7a60`.
Le diff complet est dans `changes_from_E.patch`. Les trois previews et les
neuf selftests sont des contrôles du protocole, pas des résultats moteur F.

Cadre : `phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`. GCP non utilisé.

## Inventaires et seules adaptations

Le full attendu contient exactement les 324 noms E scellés plus les
15 noms `NEW_NAMES`, soit 339. Le ciblé contient E33 plus les mêmes noms,
soit 48. Le Q2 historique reste présent une fois ; aucun nom E n'est retiré.
Le ciblé construit les neuf targets historiques et deux nouveaux :
`mhgp7_witness_stack_gate` et `mhgp7_witness_stack_semantic_gate`.

Les deux nouvelles cibles utilisent `MHGP7_TESTING=1`, sans include-dir
additionnel. Seule la seconde exige aussi
`MHGP7_WITNESS_STACK_NO_ALLOC_OBSERVER=1` : elle ne remplace pas les formes
globales de new/delete, et qualifie la sémantique sans claim d'allocation.
La compilation ciblée doit produire exactement les onze binaires.

Le juge historique complet D est désormais nommé `D_FULL`, pas `F` :
réutiliser son parseur JUnit ne réattribue pas une qualification D à F.
Le loader N charge exactement le runner E/F à SHA
`20f956612c598da256e24f8de893e7df5132f6dff0221dbc2edcdd7fe2ecce3d`.
Sa liaison historique garde les quatre CLI C/C/D/E, avec leurs propres
pins hétérogènes. Leurs builds, logs et reçus ne sont jamais des sorties
du nouveau protocole. La vérification de cette liaison a été appelée en
lecture seule, sans moteur, et a rendu 0.

| Mode | Build | Reçu neuf sous ce dossier |
|---|---|---|
| full | `build/v7_f_qualification`, incrémental après build CLI F | `full_receipts/` |
| release | `build/v7_f_tests_20260905/release`, neuf | `release_receipts/` |
| sanitized | `build/v7_f_tests_20260905/sanitized`, neuf | `sanitized_receipts/` |

Le reçu CLI F attendu est `build/v7_f_build_20260905/build_D.json` :
le nom est hérité, son contenu doit bien désigner F. L'exécution requiert
deux SHA complets explicitement fournis après revue, via
`--expected-f-sha256 SHA_F --build-receipt-sha256 SHA_RECU --execute`.
Le script n'infère pas un nouveau pin depuis le fichier qu'il trouve.
Sans `--execute`, `--mode full|release|sanitized` reste une preview.

## Gardes conservées sans réécriture

Create-only, refus des reprises, quatre commandes bornées par mode,
capture avant lancement, nettoyage des groupes possédés, inventaire exact,
JUnit rejugé au terminal et intégrité source/binaire/cache restent ceux d'E.
Les corps LastTest et leur fence sont inchangés : journal antérieur conservé,
fence neuf du même filesystem juste avant CTest, copie avant jugement,
blocs complets/noms/indices/footer, puis relecture source/archive/fence.
Une sortie CTest 0 sans ces preuves ne devient jamais `passed`.

Le full contrôle strictement la compilation du CLI, comme E ; les deux
modes ciblés contrôlent les onze commandes de compilation. Ne pas annoncer
que le seul contrôle full vérifie les flags de toutes ses cibles. Les
inventaires de binaires et tests complets sont néanmoins scellés et jugés.

ASan/UBSan exige exactement `ASAN_OPTIONS=detect_leaks=1:halt_on_error=1`,
`UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`, aucun `LSAN_OPTIONS`
et aucun `LD_*` non vide. Pas de désactivation LeakSanitizer : conserver
un refus ptrace/sandbox puis demander une tentative autorisée distincte.
Le CLI ciblé Release doit être byte-identique à F ; l'instrumenté garde
son SHA propre. Aucun plafond d'espace virtuel du banc mono n'est transféré
au sanitizer. Les durées sont celles de qualification, pas du pipeline.

## Revue et selftests

La revue du diff a détecté avant toute exécution une substitution textuelle
de `E_BUILD` qui avait aussi changé deux `CMAKE_BUILD_TYPE` en
`CMAKF_BUILD_TYPE`. Les deux clés ont été corrigées ; aucun CMake n'a reçu
ce texte. Le selftest impose désormais littéralement l'argument
`-DCMAKE_BUILD_TYPE=Release`, indépendant de la construction des fixtures.

Après correction seulement, les neuf méthodes passent en Python normal et
optimisé : inventaires339/48, 24 rejets JUnit, previews, ASAN/LSAN, conditions
terminales, pins explicites, flags/cache et 33 cycles synthétiques.
Les trois nouvelles mutations de macros sont refusées : NO_ALLOC manquant
sur semantic, TESTING manquant sur observer, NO_ALLOC ajouté indûment sur
observer. Les contrôles de logs historiques épinglés, neuf corruptions de
structure et huit scénarios d'archivage restent présents.

Les deux nouvelles commandes de compilation des selftests sont des
**fixtures synthétiques**, adaptées depuis des rows D scellées ; aucun
compilateur n'est exécuté. Les vrais graphes compilés F devront être jugés
par les campagnes après GO. Les résultats E ne sont pas réutilisés comme
résultats F. `preparation.json` conserve les commandes/outils observés,
leurs sorties et pins ; `SHA256SUMS` ferme cette préparation locale.

Root et l'agent `q2_qualification` ont relu indépendamment les pins finaux.
Aucun build/CTest/moteur F ne doit être lancé sans GO distinct, ni durant
une fenêtre de mesure mono. Cette préparation ne revendique aucun SLO,
gain, passage massif, GPU ou exactitude industrielle globale.
