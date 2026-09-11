# T2 fraîche du CPU post-échange — reçu privé

Ce paquet qualifie les sources capturées full_ball_tower.hpp 6763a877… et full_ball_tower_gate.cpp c4f39462… sur les cas nommés ci-dessous. Le probe e96f8d36… est conservé comme provenance, mais n'est pas consommé par le TU T2. Aucun résultat c03, aucun résultat GPU ni contrat de temps n'est hérité.

O2 et ASan/UBSan/LSan ROOT passent chacun onze commandes : compilation stricte, oracle historique, trois géométries, rejets, quatre mutants T2 et arguments invalides. Les sources et binaires sont épinglés avant/après. L'ELF, Boost et les dépendances système ne sont pas distribués ; les sources, commandes, logs, dépendances MMD et scripts de capture le sont. Le script prepare.py décrit la provenance d'origine, sans promettre que la source active future aura les mêmes pins. PLAN.md reste le document préparatoire original, non réécrit après résultat.

Chaque build mesure 54 tours : line12, shell14, spatial12 × deux permutations/PointId × s=8/10/12 × cache/statique1/statique4. Cela représente 540 ordres, 18 vrais census, six références cache directement jugées contre Gamma et 48 comparaisons de payload. Les 13 000 coupes et 8 103 948 contrôles verticaux passent. Le juge renforcé contrôle aussi l'ordre annoncé et la taille du tableau vertical, une fois par ordre avant les coupes des références directement jugées. Ces deux gardes sont héritées comme code, pas comme résultat ; les deux mutants de métadonnées ont leur supplément distinct, non réexécuté ici.

| Géométrie | Q post-échange | H exacts | T terminaux | Paires de travail statique1/4 |
| --- | ---: | ---: | ---: | ---: |
| line12 | 0 | 0 | 0 | 6 |
| shell14 | 12 | 0 | 0 | 6 |
| spatial12 | 216 | 120 | 120 | 6 |
| Total par build | 228 | 120 | 120 | 18 |

Q/H/T sont des compteurs de travail sur les 36 tours statiques, non des performances. Les six tableaux par K (R/U/seeds initiales/Q/H/T), le travail MEB et les descentes concordent exactement entre statique1 et statique4 à géométrie, variante et s fixes. Les branches cache ont zéro travail statique. Toutes les égalités de partition sont vérifiées par le C++ et les journaux par ordre/tour sont recoupés par le lecteur. La non-vacuité des hits vient de spatial12, pas de line12 ou shell14. Cette capture n'isole pas un facteur de vitesse ni une économie globale de MEB par rapport au moteur antérieur.

Le juge historique confronte 1 022 MEB et 14 724 requêtes de composantes à l'oracle antérieur ; neuf rejets passent. Les quatre mutants exercés concernent affectation de sous-ensemble, coupe ouverte, adjacence add/remove et omission census. Ils meurent par les diagnostics causaux attendus, pas seulement par un code non nul. Ils ne doivent pas être présentés comme des mutants spécifiques du raccourci post-échange ; les mutants de ce raccourci sont qualifiés dans le paquet CPU dédié.

Lecture sans compilation :

    python3 -B reader.py --selftest
    python3 -B -O reader.py --selftest

Les six corruptions du lecteur sont des tests du reçu, pas de nouveaux mutants C++. Le manifeste référence des objets dédupliqués : un chemin logique est matérialisable en copiant l'objet objects/SHA256 vers ce chemin. Pour rejouer, extraire current/ sous un nouveau répertoire build/ du dépôt, fournir Boost à l'emplacement indiqué ou adapter explicitement la commande, puis lancer record.py avec --out NOUVEAU et --mode o2 ou --mode san. Les anciens répertoires de capture ne doivent pas être écrasés.

Portée : u16, positions distinctes et trois petites géométries nommées, public_status=not_claimed. Ce paquet ne certifie ni tous les nuages, ni le pondéré, ni K=n à n12/14, ni les contrats 50k/multi-millions, ni une exécution device. GCP non utilisé.
