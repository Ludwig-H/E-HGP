# Session G4 `v10.20261001.iou1` : reprise gardée et arrêt certifié

1er octobre 2026. Reçu du développeur. `backend=reference_cpu`, `public_status=not_claimed`.
Aucune mesure n'est tirée de cette session.

## Faits

| Heure UTC | Fait |
| --- | --- |
| 09:02:55 | lancement de la session par un agent de préparation d'une campagne de batteries, qui ne devait faire qu'un essai à blanc |
| 09:03:45 | démarrage gardé de la VM (`start_and_verify.sh` du commit), génération `2026-10-01T02:03:45.325-07:00` |
| 09:06:08 | lancement du worker, dernière ligne de `session.stderr` |
| vers 09:07 | arrêt du workflow par le développeur : le processus de session, bien que détaché, disparaît ; ni `DONE` ni reçu |
| 09:50:54 | arrêt de la VM par sa garde invitée (`lastStopTimestamp`) |
| 10:07:27 | `python3 gcp-migration/v10_session.py --recover --session-dir …/v10.20261001.iou1` : code 0 |

## Clôture

`reprise_resume.json` : `closure = already_terminated`, `targeted_shutdown_certified = true`, état observé
`TERMINATED` sur `ehgp-v7-4fa0e0789a7d5bb06b787d35` (projet `devpod-gpu-exploration`, zone `us-central1-b`), clé
OS Login retirée, aucune erreur. Le résumé omet `recovery_command` et `host_commands` ; l'empreinte du reçu complet,
gardé hors dépôt, y figure.

Aucun résultat n'a été rapatrié : le worker a tourné jusqu'à la garde, et son travail est perdu.

## Règle retenue

- Un agent qui lance une session reste jusqu'à son `DONE`, lit le reçu et confirme l'arrêt certifié.
- Avant d'arrêter un workflow, le développeur cherche une session vivante ; s'il y en a une, il attend son `DONE` ou
  lance `--recover` aussitôt après.

## Pièces

`reprise_resume.json`, `session.stderr` (copie), `SHA256SUMS`.
