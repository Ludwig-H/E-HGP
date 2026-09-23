# Reçu : session G4 R4 préemptée (aucune mesure)

23 septembre 2026, 02:42–02:46 UTC. **GCP utilisé** : session SPOT gardée sur
la cible fixe `devpod-gpu-exploration / us-central1-b /
ehgp-v7-4fa0e0789a7d5bb06b787d35`, génération
`2026-09-22T19:43:30.666-07:00`, paquet `a1d7a9bc` (plan : ablation du seul
cache des témoins, trois trames × K5/K10, plus une répétition).

La VM a été **préemptée par GCE** (`compute.instances.preempted`,
2026-09-22T19:45:36.621-07:00, utilisateur `system`) dix-huit secondes après
le lancement du worker, pendant sa préparation : connexion SSH perdue
(code 255, `host/worker.stderr`), description `STOPPING`
(`host/before_retrieve.stdout`), capture impossible (`capture_failed`,
`fixed G4 target/status`). L'arrêt ciblé a trouvé la cible déjà
`TERMINATED` et l'a certifiée (`targeted_shutdown_certified: true`) ; état
`TERMINATED` relu. Aucune sonde n'a tourné : ce reçu ne contient aucune
mesure. Journaux expurgés ; sorties `oslogin_add` et clé non versionnées.
