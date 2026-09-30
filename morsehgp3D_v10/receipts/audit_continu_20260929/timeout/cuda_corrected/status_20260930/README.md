# Capture hôte du lecteur et des chemins d'échec CUDA

30 septembre 2026. Sources figées de la sonde corrigée `779dd38a9`.
`check_launcher.py` teste le lecteur d'enveloppe statut/code puis simule
quatre chemins du lanceur avec `unittest.mock`. Aucun processus CUDA, aucun
GPU et aucun délai ou signal réel ne sont exécutés par ces simulations.

`normal/receipt.json` et `optimized/receipt.json` conservent chacun dix
entrées du lecteur et quatre simulations ; vingt entrées et huit simulations
au total, résultats sémantiquement identiques. Chaque passage termine code 0.
Le lecteur accepte des objets `ok` numériquement incomplets ou incohérents :
ce lecteur d'enveloppe n'est pas un juge numérique de qualification.

Les chemins sans nvcc, compilation en échec et délai dépassé ne réécrivent
pas `cuda_probe.json`. Réutiliser un dossier existant peut donc conserver
un ancien résultat ; les sorties partielles du délai sont perdues. Cette
conclusion vient du flot de contrôle simulé, pas d'un ancien résultat
réellement réutilisé sur GPU. Le cas de signal sauvegarde la sortie brute.

Les hashes des sources exécutées sont vérifiés avant/après dans chaque reçu.
`source_snapshot/` conserve le lanceur et la sonde observés ; `SHA256SUMS`
ferme ce paquet. Rejouer dans un nouveau dossier de capture, jamais en
écrasant les reçus clos. Aucun débit ni contrat FULL nouveau, GCP non utilisé.
