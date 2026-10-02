# Reçus indépendants v11 — 2 octobre 2026

Ouverture `52687f8e5`, réponses jusqu'à `986f75799`, ports WIP figés avant
sondes. Aucun moteur FULL v11 ou GCP/performance. Les anciennes captures restent
inchangées ; les corrections ont leurs reçus distincts. Deux notes actives :
[fondations](../../audits/AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md) et
[verrous du moteur](../../audits/AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md).

| Reçu | Portée |
| --- | --- |
| [F3 initial](floating_bounds/PUBLICATION.md) | Contre-exemple de la doctrine initiale et correction constructive, 62 gardes. Réserve désormais résolue. |
| [Architecture initiale](architecture_review/README.md) | Quatre documents figés, contrôle documentaire ; décisions désormais fixées au §7. |
| [Provenance R2](provenance_review/README.md) | MR1 ancien fermé, source/series/gates distinctes, pas de transfert de qualification. |
| [Core intégré initial](integration_review/README.md) | GCC Release B18 : 70 portes passent, sentinelle LiDAR sautée, mutants longs exclus. Source a ensuite évolué. |
| [Core initial](core_review/README.md) | Défaut StageTimer exécuté, budget/Buffer contrôlés ; 77 contrôles du reçu. |
| [Stopwatch corrigé](core_stopwatch_followup/README.md) | Destruction sans allocation ; publication explicite, refus et reprise. 38 contrôles. |
| [Oracle initial](reference_review/README.md) | 1 732 gardes, coupes/verticales/triangles ; premières copies seulement. |
| [Portes initiales](gates_review/README.md) | Programme absent compté tué dans ancienne copie ; filtre G4 corrigé. |
| [Portes corrigées](gates_followup/README.md) | Programme absent désormais refusé ; interpréteur absent encore classé signal dans la copie suivante. |
| [Numérique corrigé](numerical_followup/README.md) | F3/F4 justifiés sous domaine positif ; ce domaine a ensuite été écrit. |
| [Verrous exacts](math_locks_review/README.md) | 159 contrôles : choix de descente, local/global, F2, compensation Euler et symétrie cover. |

Chaque fermeture est vérifiée sans changer les pièces initiales. Les pages
courantes ne remplacent pas leurs hashes. Le README figé F3 conserve une référence
contextuelle alors locale, expliquée dans PUBLICATION ; ce n'est pas une dépendance
de la preuve. Les espaces des extraits/diffs/logs bruts sont conservés pour leurs
empreintes ; seuls les fichiers explicitement identifiés sont exclus du contrôle
d'espacement Git, jamais les notes rédigées.

Exceptions exactes au contrôle d’espacement, sorties brutes closes :

- `core_review/compiler_version.txt` ;
- `core_stopwatch_followup/compiler_version.stdout.txt` ;
- `integration_review/LastTest.log` ;
- `integration_review/gates.json` ;
- `integration_review/run_00.stdout` ;
- `integration_review/run_02.stdout`.
