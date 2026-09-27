# Encodeur structurel batché — capture close du 27 septembre

Sources : [prototype et preuve](../../audits/b_full_batch_encoder_20260927/README.md),
base `24308be81bc91a13f8533d6adb29c62f0fb5c4b9`. Le moteur n'a pas été
modifié. GCP non utilisé, un thread CPU, pas de chrono de tour.

`r1/capture.json` ferme les 14 commandes avec argv, cwd, environnement,
codes de sortie, mur, empreintes avant/après des sources, dépendances
compilées, stdout/stderr et exécutables. Premier essai entièrement réussi,
aucun essai préalable compilé non conservé. GCC 13.3.0 Release et Clang
18.1.3 ASan/UBSan/LSan explicites. Le build suivant est désormais épinglé :

`/workspaces/E-HGP/build/v9-audit-full-batch-20260927-r1/{release,sanitize}`.

Chaque exécutable : 6 838 entrées × deux calendriers = 13 676 comparaisons
au constructeur natif, 404 acceptées / 6 434 refusées. Les entrées
acceptées contiennent au total 13 780 nœuds, 13 131 parents et 16 500
contributions, comptés une fois par entrée, **pas doublés par calendrier**.
Tous les champs publics sont comparés avant de calculer le digest
`2613309417252955929`, identique entre les builds.

Six exécutions mutantes (trois défauts dans chaque build) sortent avec
code 1 et la cause attendue `object_or_reason_mismatch`, sans stderr ni
erreur sanitizer. Lecteurs `normal` et `-O` exécutés avec succès après
clôture. Les preuves sont LIVE et locales, non autonomes hors de leurs
sources et binaires épinglés.

Portée : identité structurelle du draft plat et premier motif de refus,
allocations réussies. Pas de qualification géométrique, de verticales,
de multithreading/GPU, de tour FULL complète ou de gain vers 100 ms.
