# Session L1 — diagnostic local de récupération, sans nouvelle mesure

Au relevé horodaté dans [capture.json](capture.json), le worker a terminé avec le code **0**, mais le contrôleur a terminé **3 / `failed_remote`** : son unique erreur est le refus de rapatriement par réserve de disque locale. Aucune exécution ni action cloud n’a été faite par l’audit.

Le contrôleur épinglé exige `libre ≥ 2 × taille_archive + 1 Gio` avant `scp` (source `gcp-migration/v12_session.py`, lignes 143 et 1935–1937). Pour 23 176 octets annoncés, il exige **1 073 788 176 octets** ; le relevé ne disposait que de **878 231 552**, soit un déficit de **195 556 624**. Cette condition explique le refus même pour une petite archive. Elle ne prouve aucun échec du moteur ni dépassement de temps. Un nettoyage ultérieur du disque ne change pas ce diagnostic historique.

Les métadonnées locales issues de la commande distante indiquent deux empreintes identiques pour `results.tar.gz` (`3497c745…573b`), 23 176 octets, code 0. `results_archive=worker` désigne la catégorie du résultat, pas un chemin local. Le téléchargement intervient après la garde : à ce relevé, `results/` ne contient que `worker.log` vide, et `results_verified=false`. **Aucun temps FULL nouveau, journal de construction ni hash binaire n’est donc vérifiable ici.** Le code de sortie du worker ne remplace pas ces preuves.

L’arrêt dispose d’un certificat local : observation `RUNNING`, arrêt ciblé code 0, observation `TERMINATED` code 0, `targeted_shutdown_certified=true`. Il s’agit de la relecture de ces traces existantes, sans interrogation cloud supplémentaire. L’arrêt est distinct de la récupération, qui a échoué.

Le paquet source `e547240a…f134`, le plan `cdc81d10…cf11` et son script `11bf866d…1d` sont réhachés. Les cinq fichiers de sonde/pilote consignés dans la capture sont identiques aux objets Git du commit **403736300900a709b82316721eeb589fbc4a5350**. Le plan annonce W48, budgets hôte/appareil 160/88 Gio, délai global 1930 s et compilation 44 jobs ; cela décrit la commande prévue, sans certifier un binaire construit ou son exécution. Le contrôleur est également identique à son objet Git épinglé.

Les reçus/commandes bruts contiennent des identifiants et restent hors dépôt. Seuls champs autorisés, tailles, empreintes et états sont conservés ici ; aucun compte, projet, cible ni commande de récupération. Une éventuelle récupération ultérieure devra être lue séparément, sans convertir cette capture en preuve de résultats qu’elle ne contient pas.
