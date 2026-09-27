# Nsight FULL R2 — reconstruction dans la session

Autorisation utilisateur du 27 septembre 2026 : profilage FULL sur G4.
Cadre : `phase=exploration_v9_hors_registre`, `backend=cuda_g4`,
`profile=quantized_u18_input_only`, `mode=full_nsys_diagnostic`,
`public_status=not_claimed`. Ni changement moteur ni contrat de temps
nouvellement acquis par la préparation de ce diagnostic.

R1 n'avait pas trouvé l'ancien binaire distant sous la forme attendue.
R2 reprend le même snapshot source `ddf4776d754a8db59a1333e11d56b39d8cb6f51a`,
pas cet ancien fichier `/tmp`. Une seule cible CMake est reconstruite :
`mhgp9_tower_probe`, Release, CUDA activé, avertissements stricts inchangés.
Le nouveau hash du binaire est enregistré ; il n'est pas supposé égal
à celui de l'ancien build. Compilation et préparation restent hors chrono
de la chaîne, mais font partie du coût de la session cloud.

## Mesure ciblée

Trame 08/000000 sans sol entière, 39 885 sites, grille 1 mm, K1..5,
s8/W48, options identiques à la [référence FULL](../../receipts/g4_core_warm_20260927/README.md).
Petits préflights GPU jugé, jumeau moteur et capacité réduite avant la
trame réelle. Puis deux processus : quatre passages sans profiler et
quatre sous Nsight Systems CLI 2025.3.1, avec les mêmes arguments natifs.
Le validateur historique contrôle chaque sortie, les quatre passages et
les trois digests ; ce n'est pas un nouvel oracle de complétude.

La capture utilise CUDA/OSRT, sans échantillonnage CPU, commutations de
contexte ni marqueurs NVTX ajoutés au moteur. Les rapports Nsight/SQLite
restent privés. Les temps sous profiler servent au diagnostic, pas au
contrat. Activité GPU n'est pas occupation des SM ; ne pas soustraire
cette activité du temps du processus pour estimer le CPU FULL : les
digests et l'initialisation CUDA sont notamment hors `chain_total`.

## Artefacts, durée et limites

Sources épinglées avant compilation ; outils, dépendances réellement
compilées, binaire et bibliothèques vérifiés jusqu'à la fermeture. Les
headers système sont inventoriés via les dépendances produites par le
compilateur après build : cette capture ne qualifie pas à nouveau toute
la chaîne de compilation. Elle ne dépend pas d'un fichier `.rsp` artificiel.

Le build et le paquet Nsight sont hors `output/`. Le binaire final est
conservé dans `output/qualified_probe`, avec son hash, pour être récupéré
avec les résultats. Aucune installation système, aucun changement de
pilote ou CUDA. Le paquet CLI NVIDIA est vérifié puis extrait en privé.

Une G4 SPOT, gardes historiques 30 min invité/3 600 s GCE inchangées ;
budget utile du worker 600 s. Le contrôleur est interrompu coopérativement
après 900 s puis joint, sans tuer son nettoyage. Arrêt de la même génération
obligatoire après succès comme échec. Aucun lancement sans `--execute`.

Préparation locale : `selftest_capture.py` et `selftest_worker.py`,
en normal puis sous `python3 -B -O` : respectivement 5/9 et 10/14
contrôles positifs/refus. Les sept tests de l'analyseur d'intervalles
passent également dans les deux modes ; aucune activité GPU n'en découle.
Publier le commit avant lancement :

```sh
python3 -B morsehgp3D_v9/audits/b_full_nsys_r2_20260927/capture.py --execute --commit COMMIT_COMPLET --session-dir DOSSIER_NEUF
```

La collecte réussie ne vaut pas analyse : relire les sorties natives,
avertissements du profiler et tables SQLite avant toute conclusion sur
le chemin critique ou un potentiel d'accélération.
