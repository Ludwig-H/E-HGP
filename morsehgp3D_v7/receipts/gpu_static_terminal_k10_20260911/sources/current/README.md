# Extension privée du terminal CUDA à K9/K10

Cadre `phase=exploration_v7_hors_registre`, `backend=HOST_STUB`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`. Source publiée à `dc5a36ba64d5f41027d6acb23a949a38e75d0d56`, référence géométrique **c03**, aucun raccord au nouveau header actif `6763a877`. Aucun code actif, audit, Git ou GCP modifié.

Le terminal `6846376ab7fa44d6880cfa48b9b8bdde4e3d53decd5f74b1e08565b9e8a550bb` et le propriétaire `be3c422c8ff7a09650c7a91d175eebfef21585ca160150fb40bae83dcdd13a60` sont repris byte-identiques. La référence CPU et l’ABI POD sont inchangées. Les modifications portent seulement sur le corpus, son exporteur et les contrôles du juge/recorder. `import.json` lie explicitement le paquet publié `5b631b10…`, ses sources et le modèle T2. Les six qualifications hôte du propriétaire sont vérifiées via leurs manifests et snapshots communs reconstruits, pas réattribuées à un helper différent.

## Plan déclaré avant calcul

Les 949 facettes historiques K2..8 sont toutes rejouées. S’ajoutent les géométries T2 ligne12, coquille12+centre+extérieur14 et spatial12. Les deux nuages à 12 points fournissent chacun toutes les 286 facettes K9/K10. Sur 14 points, pour chacun des deux K, les rotations d’une fenêtre contiguë et d’une fenêtre avec un saut terminal sont fixées à l’avance : 28 masques par K après déduplication, sans observation géométrique. Total déclaré : **1 577 requêtes, dont 468 K9 et 160 K10**.

Les masques portent sur les indices géométriques Morton déterministes. `export_gate --plan` les écrit avant l’export ; le recorder épingle cet artefact. `export.cpp` construit aussi toutes les listes avant d’instancier les modèles/MEB. Tout refus inattendu est conservé avec nuage/masque et fait échouer le run entier : aucune facette n’est retirée après son résultat. La sélection finie de fixtures n’est pas un plafond du moteur.

Le modèle T2 préassigne les MEB depuis les supports positifs Gram q≤4 et les sous-ensembles contenus. Cela évite de refaire pour chaque facette l’ancien parcours exponentiel de supports ; il reste un oracle explicitement limité à n≤14. Sur les 14 petits nuages, ses 1 022 MEB sont reconfrontées au modèle historique. Le catalogue des grands cas est le catalogue Gram indépendant de la fenêtre `p+qmin≤11`, construite avant écriture des tableaux de census. Chaque trace du CPU c03 est reconfrontée à Gram pour sa clé primitive et son niveau.

Les planchers exigent, **à K9 et à K10 séparément**, des requêtes valides, des intrus, des descentes strictes et des supports q3/q4. La descente à rayon égal et les coquilles supplémentaires restent exercées par le corpus global. Tous les ordinals sont u64 et supérieurs à 2^32. Six refus supplémentaires contrôlent le dernier slot de K9/K10 : négatif, doublon, hors domaine, avant toute MEB. Les douze anciens refus et huit corruptions causales sont conservés.

## Qualification à exécuter

```bash
python3 -B build/v7_gpu_terminal_k10_20260911/cuda_trial/record.py --out export_r1 --mode export
python3 -B build/v7_gpu_terminal_k10_20260911/cuda_trial/record.py --out stub_r1 --mode stub --fixtures build/v7_gpu_terminal_k10_20260911/cuda_trial/export_r1
python3 -B build/v7_gpu_terminal_k10_20260911/cuda_trial/record.py --out nvcc_r1 --mode nvcc --fixtures build/v7_gpu_terminal_k10_20260911/cuda_trial/export_r1
```

SAN sera lancé séparément depuis ROOT. Les captures sont create-only et compilent leurs snapshots ; les prédicats de provenance sont testés normalement et sous `-O`. Chaque essai, même refusé, reste conservé. Aucun résultat de cette extension n’est encore annoncé dans ce document préparatoire.

Les 48 descripteurs ABI ne certifient que le layout. Le passage à trace pleine compare sites/supports/coquilles/clés/intrus/terminal et travail, avec niveaux égaux en valeur rationnelle. Capacité zéro compare résultat/travail/longueur/overflow, aucune ligne. Les vues device `const` ne sont pas relues pour attester l’immuabilité ; les rejets de requête ne certifient pas une transaction globale. Ce wrapper synchrone de preuve n’est ni la route industrielle ni un nouveau moteur FULL. Aucun device ne sera exécuté par ce recorder, aucune mesure G4/50k/multi-millions ni speedup n’est inférée.
