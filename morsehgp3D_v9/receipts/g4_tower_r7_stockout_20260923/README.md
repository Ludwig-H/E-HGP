# Reçu : tentative G4 R7, rupture de stock GCE (aucune mesure)

23 septembre 2026, 05:46 UTC. Cadre : `exploration_v9_hors_registre`,
`backend=reference_cpu`, `public_status=not_claimed`. **GCP utilisé** : une
demande de démarrage gardé sur la cible fixe `devpod-gpu-exploration /
us-central1-b / ehgp-v7-4fa0e0789a7d5bb06b787d35` (g4-standard-48 SPOT,
RTX PRO 6000) a été **refusée par GCE pour rupture de stock**
(`resource_availability`, `STOCKOUT` en us-central1-b ; zones disponibles
indiquées : us-central1-f, us-central1-c). La cible est épinglée : aucune
autre zone n'a été essayée.

Paquet `78e94b04` (sonde v10, plan v5 : ablation appariée du levier
`tower_meb_proposal`, 24 cas). Le script gardé n'a pu certifier aucune
génération et a refusé, par conception, tout arrêt non versionné : statut hôte
`shutdown_uncertified`, état de cycle de vie `start_may_have_been_requested`.
Relecture seule ensuite (`host/after_readonly_status.json`) : `TERMINATED`,
`lastStartTimestamp` **inchangé** depuis R6 (`2026-09-22T21:43:10.709-07:00`)
— la VM n'a pas démarré ; le `lastStopTimestamp` de 05:46:38 UTC est celui de
la demande refusée. Aucun arrêt supplémentaire n'était nécessaire ni lancé.
Aucun worker, aucune sonde, aucun chrono.

Contenu : `host/` (intentions et commandes du contrôleur, reçu hôte, état de
cycle de vie, sorties du démarrage gardé et de la session expurgées de
l'adresse du compte, statut relu), `PACKAGE.json`, `SHA256SUMS`. La session
sera reprise à l'identique quand la capacité reviendra.
