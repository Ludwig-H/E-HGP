# Lecteur strict séparé : alias CSV / JSONL

Archive brute inchangée :
`/tmp/mhgp10-banc-alias-audit-20260930.6wo4SVV0`.
Son manifeste complet SHA256 est
`3f23f6d3980296230a225e1425b5f44ad25bd3e66c6b8a917a4afa7bdb225cc0`.
Le lecteur exige exactement les12fichiers et le manifeste, vérifie tous les SHA
avant de lire tout reçu, source ou capture ; il refuse une variante de l'archive.

Le snapshot source a SHA
`14f3915daef73088360cf5d90be1a76aab81666ddb22764dc2ea003822714dea`.
Seuls cmd_run, COLS et CALL_KEYS sont extraits par AST ; aucun module LIVE,
probe.py, subprocess natif ou moteur n'est importé/exécuté.
Les builtins autorisés sont restreints. measure est simulé ; os/json/csv/signal/time
sont des interfaces limitées aux besoins de cette fonction. Les signaux réels,
processus, fallback infer_manifest et tout accès IO hors du mktemp sont refusés.

Contrôle positif : CSV et journal distincts, code0, deux formats corrects.
Contre-exemple : même nom de sortie, code0 et message ok, CSV et journal invalides.
Le rejeu produit exactement les mêmes OCTETS des trois exports (CRLF compris),
les mêmes SHA et le même stdout du runner. Chaque lecture fait deux appels AST,
deux measure simulés, zéro appel moteur et zéro subprocess dans le rejeu.

Les deux lecteurs normal/−O passent ; les stdout/stderr séparés et commandes
exactes sont dans receipt.json. Les dates y horodatent UNIQUEMENT les nouvelles
lectures. L'archive brute n'enregistre pas son acquisition UTC : elle n'est pas
inventée ni reconstruite à partir des mtimes.

Usage portable :
`python3 -B verify.py --archive /chemin/copie-brute`
puis
`python3 -B -O verify.py --archive /chemin/copie-brute`.
Sans --archive, le chemin historique original est utilisé. Le chemin LIVE
mentionné dans les métadonnées n'est jamais consulté.

Portée : collision du NOM EXACT des destinations du banc, contrôle des sorties
et archivage ; pas de généralisation aux liens/inodes, pas de mesure HGP, ni
qualification FULL/statistique/GCP/G4/100ms. Aucun fichier partagé ou original modifié.
