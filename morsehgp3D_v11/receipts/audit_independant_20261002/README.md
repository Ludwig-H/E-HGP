# Reçus indépendants v11 — 2 octobre 2026

Ouverture `52687f8e5`, socle `5c5457a53`, réponse `2f9eb838a`, ports WIP figés avant
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
| [Portes corrigées historiques](gates_followup/README.md) | Programme absent refusé ; défaut interpréteur absent du snapshot désormais fermé par le wrapper publié. |
| [Numérique corrigé](numerical_followup/README.md) | F3/F4 justifiés sous domaine positif ; ce domaine a ensuite été écrit. |
| [Verrous exacts](math_locks_review/README.md) | 159 contrôles : choix de descente, local/global, F2, compensation Euler et symétrie cover. |
| [Session et lanceur publiés](runner_session_review_2/README.md) | Erreur de lancement fermée ; deux suivis de matrice : interruption globale et descendance entre mesures. Wrappers/fixtures seuls. |
| [Oracle séparé](reference_separation_review_2/README.md) | Réserve structurelle levée ; troisième juge collinéaire, catalogue distinct du seul FULL. Lecture et AST/Fraction autonomes. |
| [Cloud et capacité](cloud_contract_review_2/README.md) | Tri/Morton/IDs relus, formules exactes avec entrées vivantes ; pas benchmark massif. |
| [IO en cours](io_contract_review_2/README.md) | Morceau vide SHA-256 à protéger ; lecture normative, fixture G4 proposée non exécutée. |
| [Robustesse frontière](boundary_stability_review_2/README.md) | Preuve de stabilité en rayon du continu FULL ; premier cover/LCA discontinu, trois couples u18 proposés. |

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

Exceptions supplémentaires de la deuxième tranche, copies closes inchangées :

- `cloud_contract_review_2/sources/morsehgp3D_v11/docs/PROVENANCE.md` ;
- `runner_session_review_2/source/morsehgp3D_v11/docs/PROVENANCE.md`.
