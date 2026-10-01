# Sessions G4 de la campagne tour → points du 1er octobre 2026 : clôtures et reprise gardée

1er octobre 2026. Reçu du développeur. `backend=reference_cpu`, `public_status=not_claimed`.
CPU sur l'hôte G4 (48 fils), ordre K seul : ni mesure GPU, ni contrat de temps, ni qualification de la tour FULL 1..K.

## Sessions

Cible unique : `ehgp-v7-4fa0e0789a7d5bb06b787d35`, projet `devpod-gpu-exploration`, zone `us-central1-b`.
Commit de session : `aa2aa47abc4715d8a4a2f03fb0e85f5d4ef17e77`. Durée maximale : 3600 s.

| Session | Génération (UTC) | Fin du worker | Statut | Arrêt certifié | Résultats rapatriés |
| --- | --- | --- | --- | --- | --- |
| `tvpab1` | 13:29:29 | code 1, commande 1/1 | `failed_remote`, `stopped` | oui | 522 873 457 octets, 2332 fichiers au manifeste |
| `tvppy1` | 13:53:39 | code 1, commande 1/1 | `failed_remote`, `stopped` | oui | 41 034 005 octets, 4809 fichiers |
| `tvppy2` | 14:20:32 | code 1, commande 0/1 (délai externe) | `failed_remote`, `stopped` | oui | 4 700 874 octets, 3647 fichiers |
| `tvpab2` | 14:52:58 | code 1 à 15:12:57 | session orpheline, reprise gardée | oui (reprise) | aucun |
| `tvpc1` | 15:32:57 | code 1 à 15:58:21, commande 1/1 | `failed_remote`, `stopped` | oui | 134 399 803 octets, 1656 fichiers |

Le statut `failed_remote` des sessions dont la commande réussit vient du worker : son contrôle Python par défaut
échoue (`pip: failed`, la VM n'a pas `pip`), alors que la commande utilise le Python embarqué du paquet et rend 0.
Ce n'est pas un échec de la batterie. Ce n'est pas non plus une batterie complète : les inventaires sont partiels
(741 unités sur 1054 pour `tvpab1`, 1587 sur 2336 pour `tvppy1`, 1200 sur 3072 pour `tvppy2`).

## Session orpheline `tvpab2`

| Heure UTC | Fait |
| --- | --- |
| 14:52:05 | lancement par l'agent de diagnostic de la campagne (`--execute --wait`) |
| 14:52:58 | démarrage gardé de la VM, génération `2026-10-01T07:52:58.128-07:00` |
| 14:55:12 | lancement du worker |
| 15:12:57 | fin du worker, code 1 |
| vers 15:13 | l'agent est coupé par une erreur du service (HTTP 529) pendant le rapatriement ; le processus de session disparaît avec lui : ni `DONE`, ni reçu |
| 15:16:58 | `python3 gcp-migration/v10_session.py --recover --session-dir …/v10.20261001.tvpab2`, lancé par l'agent suivant de la campagne : la VM est observée `RUNNING` |
| 15:18:11 | arrêt de la VM (`lastStopTimestamp`) |
| 15:18:23 | reçu de reprise : `status = stopped`, `targeted_shutdown_certified = true`, clé OS Login retirée, aucune erreur |

La VM est donc restée environ cinq minutes sans processus de session côté hôte. Sa garde invitée restait armée.
Aucun résultat n'a été rapatrié et aucune mesure n'est tirée de cette session.

## Règles ajoutées

- Après toute coupure d'agent, chercher d'abord une session sans `DONE` ni processus vivant, et lancer `--recover`.
- Les scripts d'orchestration réessaient un agent coupé avec une note de reprise, puis s'arrêtent ; ils ne
  poursuivent plus sur un résultat absent.
- Une étape scellée (jeu de test) n'est jamais dans le même enchaînement qu'une chaîne longue.

## Pièces

Par session : `<session>_resume.json` (champs utiles du reçu ; les commandes, les chemins distants et le chemin du
compte de la VM sont omis ; `full_receipt_sha256` est l'empreinte du reçu complet, gardé hors dépôt) et
`<session>_session.stderr` (copie). `SHA256SUMS`.
