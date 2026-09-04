# Reçu historique de qualification de l'overlay A1

Ce reçu conserve l'état **qualifié en overlay, non intégré** observé le
4 septembre 2026 avant la levée du gel comparatif. Il ne doit pas être réécrit
pour faire passer ces essais pour une validation du produit après intégration.
Les essais ont eu lieu dans `build/v7_archive_fix/`, pas dans ces copies.

`README.overlay.md` est une copie octet pour octet de la notice de l'overlay au
moment de la conservation. Les headers avant/après, le nouveau test, la probe
externe inchangée, le test API historique, les trois patches non appliqués et
les sorties Release/sanitizers sont conservés ici. Aucun binaire n'est copié.
`qualification.json` épingle les binaires réellement exécutés et leurs sources.
Les dépendances restent celles de la source v7 gelée et sont décrites dans la
notice ; les fichiers C++ copiés ici ne constituent pas un nouveau build produit.

Résultats observés :

- probe externe sur source gelée : code 97 et résidu provisoire ;
- même probe sur overlay, avec et sans panne persistante : code 0, sans résidu ;
- nouveau test Release : code 0, 23 refus d'allocation effectivement atteints ;
- nouveau test ASAN/UBSAN/fuites final : code 0, mêmes compteurs, aucun diagnostic ;
- test API historique sur overlay : code 0 ;
- wrapper CMake exigeant code 0 et préfixe de non-vacuité : code 0 ;
- vérification des trois patches par `git apply --check` : code 0, sans application.

Le premier échec sanitizer provenait de l'injecteur de test incomplet
(`new(nothrow)`), pas d'un diagnostic ignoré dans le produit. Il est documenté
dans la notice ; la reprise finale ne désactive aucun diagnostic de mismatch.
Les résidus synthétiques des contre-fixtures ont été inspectés et retirés par
opérations ciblées. Aucune donnée utilisateur n'a été supprimée.

Ce reçu ne promet ni un nettoyage après terminaison forcée/OOM killer, ni la
disparition d'un résidu que l'OS refuse de supprimer : ce dernier cas est testé
et explicitement diagnostiqué. Aucun statut mathématique ou contrat de
capacité n'est promu. GCP non utilisé par cette sous-tâche.
