# Correction du mutant de digest worker : reprise de qualification30

La première régression complète
[regression_9a8cwnny](../regression_9a8cwnny/COMPLETION.json) reste conservée
en échec :90/91 CTests, code CTest8. Le
[XML](../regression_9a8cwnny/result.xml) et la
[sortie brute](../regression_9a8cwnny/record_0000.json) montrent que
`mhgp8_wspd_q2_dynamic_receipts_gate_optimized` laisse survivre
`worker_digest`, alors que la variante normale passe.

Cause : le test remplaçait systématiquement `workers[0].digest.sum` par
`"0"`. La répartition du travail est concurrente ; aucun contrat ne garantit
une somme non nulle dans ce slot. Le lecteur contrôle exactement la somme
modulo2^64 des workers. Une substitution unique par zéro acceptée implique
donc que l'ancienne valeur était déjà zéro. L'archive de cet échec contient
le résumé du lecteur, pas le détail du worker : on ne prétend pas avoir
observé directement son nombre de jobs, ni prouvé qu'il était inactif.
Ce défaut n'est pas lié à la suppression des `assert` par Python `-O`.

La seule source modifiée est
`tests/wspd_q2_dynamic_receipts_gate.py`. Le mutant bascule maintenant le
bit faible (`valeur XOR1`) et réencode en **hexadécimal** : toute valeur
u64 valide change, y compris0 et2^64−1, sans sortir du domaine. Le digest
total reste inchangé, donc l'identité de réduction doit réellement casser.

Trois contrôles locaux déterministes exercent0,1 et2^64−1. À partir d'une
ligne complète réelle, ils fabriquent d'abord des sommes worker/total
cohérentes, acceptées par le vrai `validate_result`; ils appliquent ensuite
le même mutant et exigent précisément
`worker canonical digest reduction mismatch`. Ce sont des tests du
contrat de réduction, pas des nouveaux résultats géométriques. Ils ne
changent ni le schéma ni les compteurs des captures de mutants existantes.

Qualification ciblée : normal et `-O` PASS, sortie identique dans les deux
cas :108 lignes réelles,60 mutants,12 contrôles de domaine,13 options
invalides,3 captures d'échec conservées et1 lecture positive. Commandes,
sorties exactes/base64, codes, dates et hashes avant/après figurent dans
[WORKER_DIGEST_FIX_CHECKS.json](WORKER_DIGEST_FIX_CHECKS.json).
La source, la sonde et le cache sont inchangés pendant ces deux commandes.

Source initiale intégralement archivée avant édition par `apply_patch` :
[wspd_q2_dynamic_receipts_gate_before_worker_digest_fix.py](wspd_q2_dynamic_receipts_gate_before_worker_digest_fix.py),
SHA256 `a4b0428cbd66d03f70b316946498646f4e81fd40272526dd4b6fecc0ea9532fd`.
Correctif gelé :
`83b231df83432fed6bee6a11d7319ca5272bc6f7fa08b0094230501257116250`.

Aucun moteur, sonde, runner, CMake ou build épinglé n'est modifié. Cette
gate appartient néanmoins à l'inventaire189 : les captures30 précédentes
restent historiques avec leurs anciens pins ; la qualification complète
est explicitement rouverte et reprise séparément, sans réécrire leurs reçus.

Risque analogue signalé, sans nouvelle modification :
`tests/wspd_q2_parallel_receipts_gate.py` emploie aussi `sum="0"` pour son
mutant worker. Cette autre gate n'a pas échoué dans la régression capturée ;
aucune preuve positive n'est retirée lorsque ses mutants ont effectivement
été rejetés avec code1. Le risque de mutation sans effet demeure pour un
futur ordonnancement, mais n'est pas une défaillance observée de cette gate
dans la capture présente. Elle n'est pas corrigée dans cette reprise bornée.
