# Reprise après correction de compatibilité de l'oracle

Autorité r2 : [compiled_wqf_kywc](compiled_wqf_kywc/COMPLETION.json), dans le nouveau build `build/v8_lidar_global_r2_20260921`. La capture initiale [compiled_5slu1ek1](compiled_5slu1ek1/COMPLETION.json) et son readback restent des preuves historiques de leur propre instantané ; leurs sources ne sont plus celles de la reprise.

L'unique changement parmi les196 sources remplace trois comparaisons `boost::rational<cpp_int> == 0` par `numerator() == 0` dans la porte indépendante. Le moteur et les trois mutations sont inchangés. La [porte précédente complète](../preflight/wspd_q34_gate_before_boost_fix.cpp.txt) est conservée avec son SHA256 `bf21ab35c718b1b82db4d7b880ffd751b809f0f6f3000f692392cf23a09a3e80` ; la porte r2 porte le SHA256 `fc262936c39a185d2ec597f5ecf5fdb6117f3dc6adf35e572ccc1138dc574b59`.

Le baseline corrigé passe11 440 contrôles. Les trois mutations compilées — coquille q3 comptée comme intérieur, q4 conditionné à l'émission q3, masque q4 supprimé après rejet q3 — sont toutes tuées par la comparaison rationnelle complète des supports, profondeurs et coquilles. Chacune termine avec exactement `wspd q34 gate: global q34 stream differs from independent rational support/depth/shell oracle`, pas avec une erreur de compteur ou un crash.

Les dix commandes et leurs sorties brutes sont conservées. La fermeture confirme196 sources, quatre artefacts, helper et compilateur inchangés. Le [readback r2](MUTANTS_R2_READBACK.json) confirme les quatre lectures normal/−O, live/historique, et229 entrées inchangées avant/après. Aucun ancien build n'a été reconstruit.

Cette capture locale ne qualifie ni GCP, ni FULL, ni le contrat50k.
