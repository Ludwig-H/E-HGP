# Blocs nommés réels — NOT_EXECUTED_50K

10 septembre 2026. **Gate préparée et testée sur de petits catalogues, jamais exécutée à 50k ni sur GCP dans ce reçu.** `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

Le [rapport historique](capture/README.md.source), les [attentes épinglées](capture/expectations.json) et le [plan G4 gardé historique](capture/GCP_PLAN.md.source) répondent au contrôle nommé demandé par le second auditeur. L’observateur intervient seulement dans une copie de test, avant fermeture d’un lot et après installation de toutes ses ancres. Aucun header nominal actif n’a été modifié pour cette gate.

Sur l’entrée historique de digest `3f7c6dd4…feedab3f`, les contrôles futurs sont : 174406/K5 → un parent global, 254569/K2 → deux, 996863/K6 → deux, et 1251653/K10 → ancre conservée avec contribution vide. Les identités sont les couples **(K, BallKey)**, pas les indices de catalogue. Ces comptes ne sont ni les arités finales des nœuds ni une preuve de complétude S1.

Qualification réellement exécutée O2 et ASan/UBSan strictes : carré et ABCZ doublé, quatre observations nommées, 68 contrôles de racines strictes, 17 lots groupés, huit corruptions refusées, sorties physiques inchangées avec/sans observateur. Le mutant « ancre BallKey sans normalisation » est tué sur E5 par `named.pre_root_not_live` ; un refus inconclusif est distingué d’un mutant tué. Les six corruptions des attentes sont refusées par le lecteur, normal et `-O`. **Aucun de ces tests ne remplace l’exécution future des quatre blocs 50k.**

Le [manifeste logique d'origine](capture/manifest.json), sans ELF, est conservé octet pour octet et épinglé à `46a3f36cbebe795d1aebb9ce6a2381a1bf38106698fa357a6975079bb76f066f`. Les cinq documents Markdown capturés, dont les autorités de l'auditeur, sont des artefacts historiques `.md.source`, pas la navigation documentaire active. Le [mapping original → stockage](capture/storage_map.json) conserve leurs noms logiques et tous leurs octets. Le lecteur vérifie les fichiers stockés puis reconstitue temporairement l'arborescence logique pour exécuter le lecteur original inchangé ; il ne lance aucun moteur, ne régénère pas les 50k points et ne touche pas GCP :

```bash
python3 -B morsehgp3D_v7/receipts/full_ball_named_blocks_20260910/verify.py
python3 -B -O morsehgp3D_v7/receipts/full_ball_named_blocks_20260910/verify.py
```

La future commande explicite `--pinned-50000 --threads=48` appartient à une campagne de validation séparée, pas à la sonde de performance. Le chemin CUDA-census est préparé mais non compilé/exécuté par ce reçu ; FULL reste CPU. Contrats 50k/1s, 100ms et régime massif non qualifiés. GCP non utilisé.
