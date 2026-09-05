# Qualification indépendante de la normalisation FULL v2

5 septembre 2026. Header `85c27ab91d7f159520a8db3098629447b0a213a134c5c042a86c585416847fad`, capturé depuis le worktree sur `04fd4c89`. CPU u16 hors registre, `public_status=not_claimed`. Le [verdict courant](../CACHE_FULL_COURANT.md) borne cette qualification aux catalogues fournis complets, exacts et réguliers.

## Exécutions indépendantes

Deux builds neufs O2 et ASan/UBSan, avec LeakSanitizer actif, utilisent les [19 headers produit capturés](source_pins.json), C++20 et `-Wall -Wextra -Wpedantic -Werror`. Les deux bridges appellent le produit sans `MHGP7_TESTING`. Les dépendances utilisateur sont liées aux captures ; la construction n’est pas hermétique.

| Corpus par build Gamma | Ordres | Sorties | Coupes |
| --- | --- | --- | --- |
| Fixtures lazy historiques réexécutées | 109 | 872 | 67 920 |
| Supplément rationnel de naissance simultanée | 5 | 40 | 1 200 |
| Total | 114 | 912 | 69 120 |

Les [entrées et résultats antérieurs](input_binding.json) servent de références épinglées. Les nouvelles forêts sont confrontées à Gamma rationnel ; niveaux, parents, minima, couverture et coupes concordent. Les 32 compteurs autres que `successor_steps` et les métadonnées hors calendrier restent identiques à `21b77d29`. Sur 744 sorties de profondeur positive, le changement retire exactement deux fois `normalized_anchors`, soit 17 808 opérations sur ce corpus répété entre représentations et politiques. Ce total n’est pas un gain de temps.

Les plafonds exacts sont calculés depuis les nouvelles charges : 16 réussites exactes, 180 refus cap−1 dont 16 de successeurs, douze conflits d’API. Toutes les sorties, y compris refusées, portent `full_successor_reads_writes_no_last_pair_v2`. Les arènes refusées sont vides. Aucune identité de succès n’est appliquée aux préfixes refusés. Les [jugements normaux](judgment_normal.json) et [optimisés](judgment_optimized.json) sont identiques ; les bruts O2 et SAN aussi.

La [primitive](primitive/README.md) est jugée séparément par une liste d’événements dérivée du chemin initial immutable : 3 486 cas, 3 851 appels par build. Tous les préfixes sont exercés sur les petites forêts monotones et plusieurs chemins jusqu’à profondeur 16 ; rappels, racines inconnues, compteurs cumulés et proches de MAX sont inclus. Le juge compare tableau entier, racine partielle, statut et compteurs. Les états initiaux proches de MAX restent synthétiques ; leur condition nécessaire `2*normalized <= steps` ne reconstitue pas un historique complet du Builder.

Deux mutations réelles du helper sont compilées en copies privées O2 : ancienne dernière paire (813 sorties différentes) et écriture avant sa charge (569). Les juges normal et `-O` exigent les témoins causaux attendus. Le premier build du mutant ancien, refusé sous `-Werror`, et sa révision compilable restent distincts ; un échec de build n’est pas une réfutation scientifique. Les mutants historiques lazy/singleton ne sont pas réexécutés ni transférés.

## Captures constructeur inspectées séparément

Le [contrôle reproductible](constructor_review.py) lie les 280 fichiers du paquet constructeur à son sceau `0e6c84ba…`, au reçu réussi `49be3d72…` et à 585 pins stables : 63 fichiers v7, 521 headers Boost et un ancien depfile d’inventaire. Il vérifie commandes, environnements, dépendances, huit binaires, arguments et codes exacts des vingt CTests, JUnit et sorties non tronquées après clôture. Seuls les différentiels singleton et successeurs portent la macro testing.

Les [contrôles normal](constructor_review_normal.json) et [optimisé](constructor_review_optimized.json) concordent : 20/20 Release et 20/20 ASan/UBSan, détection des fuites active ; 49 pannes eager et 209 lazy, toutes refusées sans échappement. Les 560 cas primitifs, 1 242 appels et 180 paires FULL du constructeur constituent son propre corpus. La première tentative `70714475…`, échec de compilation avant tout CTest, reste conservée dans son paquet. Ces résultats sont des captures contre-vérifiées, sans relance des CTests par l’auditeur.

## Reproduction, clôture et limites

Le [runner Gamma](../full_successor_run.py) expose `prepare`, `build --name O2|sanitized`, `run` et `judge`. Le [runner primitif](../full_successor_primitive.py) expose ses modes O2/SAN et mutants. Les commandes exactes, environnements, dépendances, codes et hashes des binaires sont dans les reçus de build et d’exécution. Les préparations sont create-only : tout nouveau rejeu doit choisir de nouveaux dossiers sous `audits/`, avec les mêmes sources capturées ; ne pas écraser ce paquet. Les patches et révisions des mutants sont conservés sous `primitive/`.

Les juges seuls peuvent relire les captures sans moteur, depuis `audits/` : `python3 -B full_successor_run.py judge`, `python3 -B full_successor_primitive.py judge --mode O2`, puis SAN et `judge-mutant --mode legacy_stop|write_before_charge`, chacun également sous `-O`. Le contrôle constructeur se relance par `python3 -B receipts_full_successor_20260905/constructor_review.py`, également sous `-O`, sur les sources et le paquet identifiés.

La [clôture effective](execution_closure.json) est postérieure à l’annonce initiale de libération CPU : quatre brefs rejeux primitifs ont eu lieu entre 21:54:33.912911 et 21:54:34.770106 UTC. Cette rupture de coordination a été signalée ; le constructeur exclut le temps 8k/s8 recouvrant cet intervalle et conserve le reçu pour le fonctionnel, avec rejeu séparé prévu. Les premières captures et leur script exact sont conservés sous `primitive/initial_loose_normalized/`. Les [deux dossiers temporaires propres](workspace_cleanup.json) sont supprimés après contrôle des dépendances ; aucune autre construction n’est touchée.

La sonde v3, son admission, les campagnes massives, la suite F complète, les temps, les ordres géométriques K9/K10, les verticales, masses et exports FULL restent hors de cette qualification. Aucun résultat GPU ou SLO produit. GCP non utilisé.
