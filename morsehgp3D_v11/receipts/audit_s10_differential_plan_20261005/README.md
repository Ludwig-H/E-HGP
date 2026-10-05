# Plan differentiel dedie S10, apres la session S9

Proposition non executee, 5 octobre 2026. Le plan S9 de quatre portes points deja adopte dans `qual_sorties/plan_points_vs_python_auditeur.json` reste intact. `head_differential_plan.json` propose une seconde session de quatre portes head, au meme commit final S10 integre et pousse, choisi avec `--commit`. Sessions successives, une seule VM gardee. La cible `mhgp11_cli` suffit aux quatre portes head ; elle produit l'executable `mhgp11`.

Les quatre CTests sont selectionnes directement, chacun par une expression ancree, sans exclusions de la matrice. Python est epingle par le controleur. Synthetique : 24 nuages en amas a IDs non denses, K1..5, mcs3/5/10/20, EOM z1/2/3 et feuilles (120 cas, 1920 appels). Trames : ng00/ng01/ng02 entieres a K5, mcs10/20, huit appels chacune. Les partitions, le bruit et le plus petit PointId sont compares sur tous les points avec la tete Python qualifiee. Les attendus manuels F14 et les temoins CLIplat restent les portes courtes de la qualification ordinaire ; aucun oracle supplementaire n'est demande.

Fournir hors depot les six fichiers LiDAR du protocole. Ce plan ne remplace pas les sanitizers, mutants, portes courtes ni L2b. Chaque plan reserve quatre delais de commande de 4200 secondes, soit 16800 secondes, auxquels s'ajoutent installation Python, construction et fermeture. Dimensionner chaque session apres sa preflight selon les gardes du controleur. Aucune taille de VM ni duree d'execution n'est recommandee ou garantie.

`head_plan_check.json` et sa jumelle `_opt` conservent les empreintes du plan S9 original, du plan head, du controleur et des declarations CMake observees. Seule la forme est validee par `validate_plan` au controleur 311ef5e3c ; aucun preflight cloud, build ou test natif n'est execute. La source finale S10 reste a fixer apres integration.

`head_tests.cmake` conserve les déclarations observées, vérifiées par le rejeu.
