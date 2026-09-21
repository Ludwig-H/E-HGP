# Reprise locale r2 après correction de l'oracle Boost

La reprise ne modifie aucun moteur. Parmi les196 sources de la qualification, seule `tests/wspd_q34_gate.cpp` change : trois expressions `boost::rational<cpp_int> == 0` deviennent `numerator() == 0`. Les deux formulations testent exactement la même annulation ; la seconde évite l'appel d'égalité scalaire de Boost. L'ancien fichier complet est [archivé](preflight/wspd_q34_gate_before_boost_fix.cpp.txt), SHA256 `bf21ab35c718b1b82db4d7b880ffd751b809f0f6f3000f692392cf23a09a3e80`. Le SHA256 corrigé est `fc262936c39a185d2ec597f5ecf5fdb6117f3dc6adf35e572ccc1138dc574b59`.

Les autres195 empreintes sont identiques entre les manifestes de [l'ancienne porte SAN](gate_tb_fv9nw/MANIFEST.json) et de [la porte Release r2](gate_1a7ujimj/MANIFEST.json). Les anciens builds et reçus restent historiques et n'ont pas été reconstruits ni promus au nouvel instantané.

## Captures closes

| Capture | Résultat | Portée |
| --- | --- | --- |
| [gate_1a7ujimj](gate_1a7ujimj/COMPLETION.json) | PASS, dix portes | GCC13.3, Release, nouveau build `build/v8_lidar_global_r2_20260921` |
| [gate_1q8uuu7m](gate_1q8uuu7m/COMPLETION.json) | PASS, dix portes | Clang18.1, ASan/UBSan/LeakSanitizer, nouveau build `build/v8_lidar_global_sanitize_r2_20260921` |
| [regression_0cd1l_3e](regression_0cd1l_3e/COMPLETION.json) | PASS,92CTest | Régression complète Release r2 |
| [compiled_wqf_kywc](mutations/compiled_wqf_kywc/COMPLETION.json) | PASS, baseline et trois mutations compilées | Même porte rationnelle corrigée, copies temporaires du seul moteur muté |

Chaque capture confirme ses sources et artefacts inchangés avant/après, sans erreur de fermeture. Les portes locales utilisent les en-têtes Boost1.83 déjà disponibles ; cela ne constitue pas à soi seul un test sur le Boost système de la machine GCP.

Les dix portes totalisent119 746 contrôles par capture ; la porte globale en compte11 440, avec166 appels mono/globaux et46 appels au pipeline parallèle. Les JSON des portes SAN initiale, Release r2 et SAN r2 concordent sur tous les champs sauf `multiworker_calls`, qui dépend du planning effectif :31,24,31 respectivement. Ce compteur doit être positivement exercé, pas identique entre exécutions ; les validations géométriques et les identités de travail restent identiques.

Les trois mutants sont rejetés par la comparaison rationnelle des supports, profondeurs et coquilles, pas par un compteur ou une interruption. Leur [note r2](mutations/README_R2.md) et leurs [quatre relectures persistées](mutations/MUTANTS_R2_READBACK.json) sont distinctes des reçus initiaux. Les deux captures de portes et la régression ont également leurs [douze relectures persistées](R2_READBACK.json) : normal/−O, historiques et live, toutes PASS avec entrées inchangées et aucune erreur de fermeture.

La capture ASan/UBSan/LeakSanitizer a été exécutée hors du bac à sable, avec détection des fuites active, pour éviter l'incompatibilité ptrace déjà conservée dans [l'essai initial en échec](gate_kny8nlmd/COMPLETION.json). Cet ancien échec n'est pas supprimé.

Cette requalification ne répète ni les144 mesures d'arêtes LiDAR ni une campagne de croissance globale. Elle ne qualifie ni FULL, ni le contrat50k, ni une exécution GPU. Les sessions GCP et leur fermeture ont leurs propres reçus.
