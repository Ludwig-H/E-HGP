# Errata de publication — 4 septembre 2026

`public_status=not_claimed`. Ces corrections ne changent ni le moteur,
ni les octets des reçus historiques, ni leurs domaines de qualification.

## Journaux exclus du premier push

Le commit `b8aef528eb3db1e4dcb4424b59e43be687049465` a omis seize
journaux locaux, exclus par la règle Git `*.log`. La
[CI 33930024823](https://github.com/Ludwig-H/E-HGP/actions/runs/33930024823)
a réussi la construction, mais échoué sur trois liens documentaires
manquants ; les tests de ce workflow ont été sautés, pas réussis.

Quinze journaux sont déjà épinglés par les manifestes des reçus
[arithmétiques](../receipts/arithmetic_gates_20260904/SHA256SUMS) et du
[harnais CI](../receipts/ci_sonde_environment_20260904/SHA256SUMS).
Leur publication conserve exactement ces octets et ces empreintes.
Le contrôle `tools/check_v7_receipt_publication.py` vérifie maintenant
chaque entrée des `SHA256SUMS` v7 contre les blobs de l'index Git,
et non contre des fichiers locaux potentiellement absents du commit.
Sa fixture positive et sept rejets restent actifs sous Python normal et `-O`.
Ce contrôle ne prétend pas valider les autres formats de reçus JSON.

Extension du 5 septembre : les chemins relatifs au dossier du reçu sont
également résolus contre l'index, avec les mêmes interdictions de sortie
du périmètre. Deux écritures désignant le même fichier sont rejetées
comme doublon. Les manifestes historiques ne sont pas réécrits pour
uniformiser leur convention de chemins.

## Journal incomplet du smoke mono historique

Le seizième fichier est le
[journal historique mono](../receipts/mono_inline_overlay_20260904/integrated_smoke/ctest.log).
Ses 121 octets ne contiennent que le début et la fin de session CTest,
sans sortie détaillée des quatre tests. Les mentions « log complet »
dans le README historique et « complete raw stdout » dans son reçu
sont donc incorrectes. Elles restent conservées comme antériorité,
avec le présent erratum explicite ; aucune sortie n'est reconstituée.

Le XML historique conserve son autorité limitée de résultat CTest,
mais ce journal ne prouve pas le stdout détaillé annoncé. La qualification
actuelle des portes mono repose séparément sur les replays ultérieurs,
notamment les [316 portes CPU fraîches](../receipts/arithmetic_gates_20260904/README.md),
et non sur une promotion de cette trace incomplète.
