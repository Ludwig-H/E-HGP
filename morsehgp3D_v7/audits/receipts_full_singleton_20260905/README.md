# Qualification indépendante du lot FULL unitaire

5 septembre 2026 ; source `21b77d29a4ba2bca453b602a8faa4564a978f4ba71af5167c164faae4ef0e1a5`, capturée depuis le worktree sur `764c80b9`. CPU u16 hors registre, `public_status=not_claimed`. Le [verdict courant](../CACHE_FULL_COURANT.md) donne l’argument mathématique et le domaine relatif aux catalogues fournis complets, exacts et réguliers.

## Exécutions propres

Deux builds neufs O2 et ASan/UBSan, avec LeakSanitizer actif, sont compilés depuis les [19 dépendances capturées](source_pins.json). Le bridge historique est compilé sans `MHGP7_TESTING`. Les sources système sont identifiées, sans prétendre à une construction hermétique.

| Corpus par build | Ordres | Sorties | Coupes |
| --- | --- | --- | --- |
| Fixtures lazy historiques, réexécutées | 109 | 872 | 67 920 |
| Supplément rationnel de naissance simultanée | 5 | 40 | 1 200 |
| Total | 114 | 912 | 69 120 |

Les [entrées historiques réutilisées](input_binding.json) restent sous leurs hashes ; aucun ancien résultat n’est présenté comme nouvelle exécution. Les sorties historiques sont identiques octet pour octet au témoin `13c6`, forêts, compteurs, 16 caps exacts, 180 refus cap−1 et douze conflits d’API compris. Les [jugements normaux](judgments_normal.json) et [optimisés](judgments_optimized.json) passent ; les deux sorties C++ nominales sont identiques.

Le [calendrier indépendant](calendar.json) dérive de Gamma les lots q2/q3/q4 et les consommations ultérieures de no-op. Il constate zéro naissance simultanée dans l’ancien corpus. Le [supplément](target_fixtures.json), coordonnées proposées par le constructeur mais calculs rationnels indépendants, vérifie 26 sous-ensembles et 13 enregistrements Gabriel. À K=2, niveau 25, une naissance précède une fusion à trois parents. Les cinq ordres, deux représentations et quatre politiques passent dans les [deux builds](target_judgments_normal.json), également sous [Python optimisé](target_judgments_optimized.json).

La [mutation privée](drop_fourth_parent.patch.txt) conserve toutes les résolutions et perd seulement le quatrième token dans la préparation locale. Build et transport code 0, stderr vide ; 136 sorties sur 872 diffèrent, 736 restent identiques. Le [juge rejette](drop_fourth_parent_witness.json) d’abord `lazy.identity` : une exécution qui devrait réussir refuse avec `full_gabriel_inconsistent_alias`. Le [contrôle optimisé](drop_fourth_parent_optimized.json) retrouve aussi 102 forêts erronées avec statut succès : E5/K4 publie `[0,1,2]` au lieu des parents `[0,1,2,3]`. Ce témoin n’est ni un crash ni une erreur de transport. Les trois anciens mutants lazy n’ont pas été relancés ici.

## Captures constructeur, autorité séparée

Le [contrôle reproductible](constructor_review.py) vérifie l’inventaire fermé du paquet constructeur, les hashes, la liaison au reçu `e3b64a03…`, les 584 pins stables (62 fichiers v7, 521 headers Boost et un ancien depfile d’inventaire), les commandes, dépendances, sept binaires, arguments et codes des 17 CTests. Seul le binaire singleton porte la macro testing ; LeakSanitizer est actif. Les comptes non tronqués sont lus dans `LastTest.stdout`, indépendamment des résumés JUnit.

Les [résultats normaux](constructor_review_normal.json) et [optimisés](constructor_review_optimized.json) concordent : 17/17 par build ; 49 fautes eager et 209 lazy, toutes refusées sans échappement ; 181 paires nominales et 357 paires refusées du différentiel singleton. Ces captures sont contre-vérifiées, sans relance des CTests par l’auditeur. Le paquet constructeur est identifié par son contenu ; son commit Git est indépendant de cette publication d’audit.

## Reproduction et limites

Les runners [historique](../full_singleton_run.py) et [supplémentaire](../full_singleton_target.py) donnent les actions `prepare`, `build`, `run`, `judge` et leurs arguments. Les commandes exactes, dépendances, environnements, sorties et codes sont conservés dans les fichiers `*_build.json` et `*_run.json`. Les préparations sont volontairement create-only : pour un nouveau rejeu, choisir deux nouveaux répertoires dans `audits/` dans une copie du runner ; ne pas écraser ce paquet clos. Pour reconstruire le mutant, copier `source/`, appliquer l’unique patch, puis reprendre sa commande de compilation avec ces nouveaux chemins.

Les jugements sur captures seuls se relancent sans moteur : `python3 -B full_singleton_run.py judge`, `python3 -B full_singleton_target.py judge`, puis leurs variantes `-O`, depuis `audits/`. Le contrôle constructeur s’exécute par `python3 -B receipts_full_singleton_20260905/constructor_review.py`, également sous `-O` ; il exige le paquet et les sources identifiés.

La [clôture CPU](execution_closure.json) est à 20:19:12 UTC. Aucune mesure de performance, campagne massive, suite F complète, verticale, masse ou archive FULL n’est qualifiée ici. La génération q4 et la normalisation restent inchangées. GCP non utilisé.
