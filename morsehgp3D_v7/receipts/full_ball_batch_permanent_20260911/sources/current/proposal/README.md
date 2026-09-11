# Candidat privé : régression permanente du callback de lot

V7 hors registre, CPU de référence, u16, statut public non revendiqué. Cinq CTests proposés : callback CPU1, CPU4, 65 rejets ciblés, argument inconnu et argument absent. **Aucun header produit n’est instrumenté ou réécrit** : la copie de qualification consomme exactement la couture `83f1c78e0656f08cd42522e4cd36d153ce283a6082246a36fe5225b3790c6366`. Le moteur actif, `audits/`, Git et GCP restent inchangés.

`tests/full_ball_batch_gate.cpp` inclut le produit avant toute macro. Seuls les appels du juge FULL existant sont interposés : construction sans callback, puis avec un owner CPU1/4, comparaison complète des populations, nœuds, parents, successeurs, contributions, ordres, verticales et niveaux physiques. Le juge indépendant Gram/Gamma continue à vérifier coupes, recouvrement, multifusions, descente à rayon égal et semis après échange. Les compteurs de géométrie et Q/H/T sont égaux aux compteurs du même moteur sans callback ; allocations et capacités d’exécution ne sont pas prétendues identiques.

Deux helpers restent **dans tests/** : la recette géométrique scalaire extraite historiquement de `6763` et son owner de référence, relocalisés depuis la couture qualifiée. Le callback reçoit naturellement requêtes/semis/consommateurs/ordinaux. Il calcule d’abord les attentes BallId depuis cette recette, puis exécute le lot ; les attentes sont construites avant toute faute et confrontées aux résultats. Ce double calcul est du travail de juge, déclaré séparément par `diagnostic_reference_MEB_calls` et exclu de `result.work`. Il ne constitue ni une optimisation ni un benchmark.

La recette scalaire n’est pas un oracle géométrique indépendant du moteur dont elle provient : l’indépendance géométrique de la forêt vient de Gram/Gamma. La comparaison directe des BallId certifie ici le transport/dispatch et le choix de la recette de référence sur les requêtes reçues ; elle ne remplace pas l’ancienne capture interne `batch_trace` du Builder et ne prouve pas que tous les événements internes sont identiques. Aucun générateur de source ni hook produit n’est exécuté par CTest.

Les 62 cas injectés conservent les causes/status exacts de la couture : résultat court, ordinal, domaine, niveau non strict, Q/H/T, ordre parasite, publication partielle en erreur, travail inconnu, exception, refus après travail, débordement de l’agrégat local, capacité débordante, agrégat du cœur débordant, mauvais rang et mauvais terminal pourtant admissible. Deux nombres de workers et K2/K3 exercent le refus après préfixe. Chaque erreur garde la tour vide, zéro worker encore admis et le bon `work_known` ; une erreur de capacité doit conserver tous les appels déjà payés. S’ajoutent owner étranger de même forme, callback sans opt-in statique et lot sans requête/K=n sans appel au callback. Plancher attendu : 65 cas, 32 après préfixe, 44 avec travail connu non nul.

Les corpus nominaux restent ceux du juge FULL borné n≤8, avec variantes d’identités et fixtures de semis. La présente gate ne couvre pas le futur callback GPU, ni le raccord census K1..10 de cette voie ; la gate census permanente sans callback est distincte. Les bornes du juge et TIMEOUT CTest n’ajoutent aucun quota produit. Aucune mesure de vitesse, aucun contrat 50k ni multi-millions ne sont acquis.

`origin.json` épingle les helpers originaux ; `overlay.json` épingle l’unique header produit copié. `candidate/` contient trois nouveaux fichiers tests, `CMake.diff` cinq enregistrements CTest. Les captures compilent un snapshot composé avec CMake, un seul job et aucune macro `MHGP7_TESTING`.

```bash
python3 -B build/v7_batch_permanent_20260911/record.py --out o2_r1
python3 -B build/v7_batch_permanent_20260911/record.py --out san_root_r1 --san
```

Statut à la rédaction : préparé, pas encore qualifié. Le créneau O2 doit être accordé par ROOT ; SAN est exécuté par ROOT séparément, sans désactiver la détection des fuites.
