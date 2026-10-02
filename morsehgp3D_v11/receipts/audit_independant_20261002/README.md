# Reçus indépendants v11 — 2 octobre 2026

Ouverture `52687f8e5`, publication courante `6a22a9118`, fondations exécutées sur
G4 à `a97180667`, copies figées avant lecture. Aucun nouveau GCP par cet audit ;
la recoupe G4 qualifie les fondations CPU, pas FULL/GPU/performance. Les anciennes captures restent
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
| [Session et lanceur publiés](runner_session_review_2/README.md) | Erreur de lancement fermée ; interruption globale désormais corrigée ; isolation de descendance encore ouverte. Wrappers/fixtures seuls. |
| [Oracle séparé](reference_separation_review_2/README.md) | Réserve structurelle levée ; troisième juge collinéaire, catalogue distinct du seul FULL. Lecture et AST/Fraction autonomes. |
| [Cloud et capacité historique](cloud_contract_review_2/README.md) | Copie privée précédente ; comptage et formules exactes avec entrées vivantes, pas benchmark massif. |
| [IO privé non livré](io_contract_review_2/README.md) | Morceau vide SHA-256 à protéger dans cet ancien WIP ; aucune attribution au produit qualifié actuel. |
| [Robustesse frontière](boundary_stability_review_2/README.md) | Preuve de stabilité en rayon du continu FULL ; premier cover/LCA discontinu, trois couples u18 proposés. |
| [G4 recoupé](g4_qualification_review_3/README.md) | Trois paquets/archives Git et 91 entrées manifestes vérifiés ; deux échecs conservés, troisième campagne conforme. Isolation encore déclarée ouverte. |
| [Cloud immuable](cloud_immutable_review_3/README.md) | Sources identiques à G4, vingt portes dans six configurations ; anciennes réserves fermées, durée de vie du futur index et pic partagé à tester. |
| [Géométrie exacte](numeric_geometry_review_3/README.md) | Bornes B18/21/24 relues ; 196 contrôles autonomes normal/−O, seize requêtes G4 préparées non exécutées. |
| [Hiérarchie commune aux K](cross_order_contract_review_3/README.md) | Témoin statique sept points core K1/K2 incompatibles, modèle Gamma indépendant ; entiers/Level relus favorablement. |

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

Exceptions de la troisième tranche, pièces closes conservées octet pour octet
(journaux bruts et programme du modèle figé, ligne vide finale comprise) :

- `cloud_immutable_review_3/g4_logs/bits21/junit.xml` ;
- `cloud_immutable_review_3/g4_logs/bits24/junit.xml` ;
- `cloud_immutable_review_3/g4_logs/gcc_asan_ubsan/junit.xml` ;
- `cloud_immutable_review_3/g4_logs/gcc_release/junit.xml` ;
- `cloud_immutable_review_3/g4_logs/gcc_tsan/junit.xml` ;
- `cloud_immutable_review_3/g4_logs/poison/junit.xml` ;
- `cross_order_contract_review_3/model.py` ;
- `g4_qualification_review_3/excerpts/reprise1/results/cmd/000_matrice/files/matrix/gcc_release/LastTest.log` ;
- `g4_qualification_review_3/excerpts/reprise1/results/cmd/000_matrice/files/matrix/mutants/LastTest.log` ;
- `g4_qualification_review_3/excerpts/reprise2/results/cmd/000_matrice/files/matrix/gcc_release/LastTest.log` ;
- `g4_qualification_review_3/excerpts/reprise2/results/cmd/000_matrice/files/matrix/mutants/LastTest.log` ;
- `g4_qualification_review_3/excerpts/reprise3/results/cmd/000_matrice/files/matrix/gcc_release/LastTest.log` ;
- `g4_qualification_review_3/excerpts/reprise3/results/cmd/000_matrice/files/matrix/mutants/LastTest.log` ;
