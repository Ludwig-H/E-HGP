# 174 fronts clos, CPU 0

Le 6 septembre 2026, deux compilations O2 et neuf commandes closes, codes attendus : chaque bras `--selftest` 0 / `--unknown` 2, comparaison normale et `-O` 0 avec sortie byte-identique. Aucun produit édité, pas de SAN hérité, GCP non utilisé.

Reçu `receipt.json` : `82d3655eb44b78c382a36c69a2081deeab5a20fdcd6108965cf68494f640a1cc`. Inventaires compilés avant/après : `feb4036ced14d6bc4972d910b35ae95fed54fc14d25167e123c8a3cf0365837c`. Générateur candidat : `4217c7c51a8e3be7ce6b6644e7a6332ea61a8f1c75382848cceb5ab68507e762`. Les notes et sources de mesure ajoutées après clôture ne font pas partie de cet inventaire historique ; aucun octet capturé n'est réécrit.

Les 174 fronts, dont six refus, sont littéralement identiques hors les deux compteurs de travail autorisés. Visites de nœuds : 22 566→14 424, soit 8 142 de moins ; coins : 390→390. Aucun cas du corpus ne montre de surcoût, sans théorème de monotonie du travail. Non-vacuité : 36 paires terminales 6→3 visites, six paires q2 seul conservées 3→3. Les données sont dans `logs/compare_normal.stdout`.

Ces résultats ne chronomètrent pas la tour FULL. La mesure de composant 8k est séparée, une seule exécution par bras sur CPU 6 après autorisation. `front_measure.cpp` sérialise tous les rectangles ordonnés et leurs six champs, masses/pics/compteurs sémantiques après arrêt du chrono ; le JSON exact est comparé hors temps et visites/coins. Le temps exclut génération des points, construction de l'index et sérialisation. Source et dépendances de ces deux binaires sont liées séparément dans `measure_logs/` ; aucune mesure ne précède le GO CPU 6.
