# Sessions G4 du 1er octobre 2026, suite : `tvpc2`, `tvpd1`, `vc1`

1er octobre 2026. Reçu du développeur. `backend=reference_cpu`, `public_status=not_claimed`.
CPU sur l'hôte G4 (48 fils), ordre K seul : ni mesure GPU, ni contrat de temps, ni qualification de la tour FULL 1..K.
Suite de [`g4_sessions_tvp_20261001`](../g4_sessions_tvp_20261001/README.md), qui reste inchangé.

## Sessions

Cible unique : `ehgp-v7-4fa0e0789a7d5bb06b787d35`, projet `devpod-gpu-exploration`, zone `us-central1-b`.
Commit de session : `aa2aa47abc4715d8a4a2f03fb0e85f5d4ef17e77`. Durée maximale : 3600 s.

| Session | Objet | Génération (UTC) | Fin du worker (UTC) | Fermeture (UTC) | Arrêt certifié | État observé | Rapatrié |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `tvpc2` | niveaux A, B et coupes : 262 unités restantes à n = 8 000 | 16:02:06 | 16:30:13, commande 1/1 | 16:32:16 | oui | `TERMINATED` | 82 143 921 octets, 834 fichiers au manifeste |
| `tvpd1` | quatre variantes de sélection : 500 unités | 16:33:45 | 16:54:32, commande 1/1 | 16:56:34 | oui | `TERMINATED` | 122 989 532 octets, 1 548 fichiers |
| `vc1` | vote sur la tour condensée, scènes de 300 points, variantes **sans** date à marge | 18:13:22 | 18:39:37, commande 1/1 | 18:41:38 | oui | `TERMINATED` | 8 221 402 octets, 1 630 fichiers |

Les trois reçus disent `failed_remote` pour la seule raison déjà décrite : le contrôle Python par défaut du worker
échoue (la VM n'a pas `pip`), alors que la commande, qui utilise le Python embarqué du paquet, rend 0.

## État à la coupure du codespace

Le codespace a été arrêté vers 19 h 02 UTC. À ce moment, aucune session n'était en vol : `vc1` était fermée depuis
18:41:38. Les deux enchaînements capables de lancer une session avaient été arrêtés à 18 h 44, sans session
ouverte. Au redémarrage (19 h 14 UTC), chaque dossier de session du jour porte `DONE` ou un reçu de reprise.

## Portée

- `tvpd1` mesure des variantes de **sélection**. L'utilisateur a depuis fixé l'ordre : la tour, puis les
  hiérarchies de points, la sélection et z en dernier. Ces résultats sont mis de côté.
- `vc1` n'est pas encore analysée. Son code a été figé à 18 h 03, avant l'ajout de la date à marge à la règle.

## Pièces

Par session : `<session>_resume.json` (champs utiles du reçu ; les commandes, les chemins distants et le chemin du
compte de la VM sont omis ; `full_receipt_sha256` est l'empreinte du reçu complet, gardé hors dépôt) et
`<session>_session.stderr` (copie). `SHA256SUMS`.
