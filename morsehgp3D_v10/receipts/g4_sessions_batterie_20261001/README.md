# Sessions G4 du 1er au 2 octobre 2026 : batterie « tour puis hiérarchies » et mesure du vote

2 octobre 2026, 00 h 40 UTC. Reçu du développeur. `backend=reference_cpu`, `public_status=not_claimed`.
CPU sur l'hôte G4 (48 fils), ordre K seul : ni mesure GPU, ni contrat de temps, ni qualification de la tour FULL 1..K.
Suite de [`g4_sessions_tvp_suite_20261001`](../g4_sessions_tvp_suite_20261001/README.md), qui reste inchangé.

## Sessions

Cible unique : `ehgp-v7-4fa0e0789a7d5bb06b787d35`, projet `devpod-gpu-exploration`, zone `us-central1-b`.
Commit de session : `aa2aa47abc4715d8a4a2f03fb0e85f5d4ef17e77`. Heures en UTC.

| Session | Objet | Début | Fin | Fermeture | Arrêt certifié | Commandes | Rapatrié |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `vc2` | vote sur tour condensée : 300 points avec marge, 2 000 points | 19:28:16 | 19:49:09 | `stopped` | oui | 1/1 | 7 058 061 octets, 2 372 fichiers |
| `vc3` | — | 19:51:01 | 19:51:46 | `generation_unknown` | **non** | démarrage refusé | rien |
| `ab1` | — | 19:53:27 | 19:54:12 | `generation_unknown` | **non** | démarrage refusé | rien |
| `ab1_e1` | LiDAR : 64 trames, niveaux tour et hiérarchies | 19:56:05 | 20:18:44 | `stopped` | oui | 1/1 | 3 079 734 octets, 1 200 fichiers |
| `vc3b` | vote : 2 000 points, second réplicat ; blocs par groupe | 20:19:23 | 20:40:52 | `stopped` | oui | 1/1 | 10 111 430 octets, 3 523 fichiers |
| `vc4` | vote : trames à K = 10, 8 000 points | 20:42:26 | 21:11:13 | `stopped` | oui | 1/1 | 1 070 923 octets, 567 fichiers |
| `ab2r_e2` | noyau du plan étendu, réplicat 0 ; sondes à 16 000 et 32 000 | 21:11:41 | 21:20:17 | `stopped` | oui | 1/1 | 4 783 848 octets, 792 fichiers |
| `abg_e1` | grandes tailles | 21:23:17 | 21:31:15 | `already_terminated` | oui | VM préemptée, aucun résultat | rien |
| `abgr_e1` | grandes tailles, relance : 16 000 et 32 000 points | 21:32:03 | 22:00:30 | `stopped` | oui | 2/2 | 5 357 893 octets, 1 207 fichiers |
| `abp_e1` | tout le plan étendu jusqu'à 8 000 points | 22:02:26 | 22:21:18 | `stopped` | oui | 2/2 | 41 739 616 octets, 6 103 fichiers |
| `abm_e1` | classes MAP : bloc iid, plan jusqu'à 8 000 points, bruit ignoré | 00:03:41 | 00:25:57 | `stopped` | oui | 2/2 | 73 521 679 octets, 6 535 fichiers |

Les reçus complets disent `failed_remote` quand la commande réussit : le contrôle Python par défaut du worker échoue
(la VM n'a pas `pip`), la commande utilise le Python embarqué du paquet et rend 0.

## Deux démarrages refusés : `vc3` et `ab1`

| Heure | Fait |
| --- | --- |
| 19:49:09 | fermeture certifiée de `vc2`, génération `2026-10-01T12:28:51.125-07:00` |
| 19:51:03 | `vc3` : démarrage gardé demandé |
| 19:51:41 | GCE horodate un arrêt de la cible (`lastStopTimestamp`) ; `lastStartTimestamp` reste celui de `vc2` |
| 19:51:46 | `start_and_verify.sh` rend 1 : la zone n'a pas la ressource (g4-standard-48 SPOT, état `STOCKOUT`). Le contrôleur rend 74, `SHUTDOWN_UNCERTIFIED : generation inconnue` |
| 19:52:14 | `--recover` : `TERMINATED`, même `lastStartTimestamp` ; statut `shutdown_uncertified`, faute de génération à certifier |
| 19:53:29 à 19:54:28 | `ab1` : même refus, même lecture (`lastStopTimestamp` 19:54:07) |
| 19:56:07 | `ab1_e1` : démarrage accepté, génération `2026-10-01T12:56:46.151-07:00` |

Lecture : aucune VM n'a démarré dans ces deux sessions. Le `lastStartTimestamp` est resté celui de `vc2`, puis la
session suivante a créé sa propre génération et l'a fermée avec certification. Le contrôleur, lui, ne certifie pas :
il n'a aucune génération à comparer. **Les deux reçus restent `shutdown_uncertified`**, et c'est à l'utilisateur de
l'acter. Les agents ont relancé après le code 74 ; la consigne écrite était de s'arrêter avec un rapport. La consigne
est maintenant explicite pour ce cas : reprise, relecture de l'état, nouveau nom de session, quatre essais au plus.

## Une préemption : `abg_e1`

La VM a été préemptée cinq minutes après le lancement du worker (`worker_outcome = target_lost`). La fermeture la
trouve `TERMINATED` avec sa génération (`already_terminated`, arrêt certifié). Rien n'a été rapatrié. Le même contenu
a été relancé sous le nom `abgr_e1` et a abouti.

## État final

Dernière session : `abm_e1`, fermée à 00:25:57 le 2 octobre, arrêt certifié, cible relue `TERMINATED`. Aucune session
en vol à l'heure de ce reçu.

## Pièces

Par session : `<session>_resume.json` (champs utiles du reçu ; commandes, chemins distants et chemin du compte de la
VM omis ; `full_receipt_sha256` est l'empreinte du reçu complet, gardé hors dépôt), `<session>_session.stderr`
(copie ; la ligne `SHUTDOWN_UNCERTIFIED` est coupée avant la commande de contrôle qu'elle cite), et pour `vc3` et
`ab1` `<session>_reprise.json`. `SHA256SUMS`.
