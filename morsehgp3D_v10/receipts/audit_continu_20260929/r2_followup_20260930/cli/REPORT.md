# Contre-audit R2 : tête et interface, 30 septembre 2026

Observation jusqu'à 03:38:44 UTC. Lecture seule des sources et builds du développeur ; sondes natives de quelques millisecondes seulement. Aucun moteur modifié, aucune compilation, GCP0.

## Défaut concret de sortie

`mhgp10_cluster` (binaire SHA256 `a776427321ae27d3ccde41dffbf03fa8601144926a99ac727bb78a205bb04781`) accepte `OUT.i32le` identique à `--tree=OUT.i32le`. Sur cinq sites, K2, mcs2, W1, il rend code 0 et `status=ok`, mais le fichier final est le texte de l'arbre : 126 octets au lieu des 20 octets d'étiquettes. Le contrôle sans `--tree` écrit les 20 octets attendus. Les deux appels, stdout/stderr/codes, tailles, contenu initial et hashes avant/après sont dans `receipt.json` ; tous les hashes ciblés sont stables. `OutputSet::open` déduplique un nom sans distinguer les sorties logiques, puis `write` retronque le même fichier.

Action : refuser les destinations concurrentes avant toute réservation ; au minimum le même nom, puis les alias de fichier dans la politique documentée. Les sorties et l'entrée doivent aussi avoir une politique d'alias explicite. Ne pas extrapoler ce cas à des erreurs de géométrie.

## Raccord tête / interface encore nécessaire

La copie `entrees_cli` garde l'ancienne tête : elle accepte z=0, z=17 et mcs=0 (constaté en petites sondes, non présenté comme défaut de la nouvelle tête). Sa boucle appelle `cluster(d,list[i])` puis indexe `cl.label` et `cl.tree.node_cluster` sans propagation d'un `cl.outcome`. Avec la nouvelle tête, un refus laisse ces tableaux vides. La copie `tete` a déjà cette propagation ainsi que `validate(d,q)` avant l'écriture ; la fusion doit conserver ces contrôles et la validation des paramètres avant le catalogue. Les nouveaux tests de frontière CLI `entrees_cli` attendent encore l'acceptation z=0 et K1/n1, incompatible avec le contrat numérique de la tête R2.

## Nouvelle tête : progrès vérifié par lecture, pas encore qualifié

`head.cpp` SHA256 `a4c217e5710504610845aacbd37e90fc608fb93c913ee72595e1e9616db9122b`, `head.hpp` SHA256 `6ee4a52f44991a0c4841e783d807e7a7a36c201ec834800059edf08bf6a3e941` à 03:38:44. `prepare` protège la racine zéro indépendamment de mcs, refuse les fusions à zéro, ne calcule les lambdas que sur les rangs consommés et garde M*lambda_max < 2^1000. Le singleton zéro/mcs5 a une fixture explicite. Les refus de `condense`/`cluster` rendent un Outcome et des sorties vides. La propagation linéaire de sélection/étiquetage est conservée.

La porte causale de coût `head_complexity.cpp` est désormais temporelle, six chemins de chenille de q=2^20, watchdog 15 s par appel, partitions attendues, sans utiliser le compteur interne. Aucun build tête R2 ni reçu terminal R2 n'était visible ; les seuls exécutables tête étaient `build-r1`. Ne pas annoncer cette nouvelle tête qualifiée sur la base des campagnes R1.

## Reçus et processus

CLI R2 : 4/4 fast, 11,80 s, footer explicite rc=0 ; 6/6 autres, 169,13 s, footer CTest terminal mais pas de champ rc dans ce fichier. Ce ne sont pas des tests du futur binaire commun. Processus réels vus lors d'une lecture ps hors sandbox : différentiel CLI parent PID143118, catalogue LiDAR02/K10/W1 enfant PID147224 ; TSan CLI PID149457 dans `build-tsan/mhgp10_unit`, alors encore vivant. Aucun test tête R2 vivant observé à cet instant. Ces PIDs sont une photographie, pas un statut durable.
