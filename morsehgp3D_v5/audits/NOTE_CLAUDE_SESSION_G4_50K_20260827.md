# Note — session G4 v5 du 27 août 2026 : conformité v4 sur la VM et contrat 50 000 points MESURÉ

- **Pin du moteur et du protocole :** `f37669ae` (bundle depuis le commit, manifeste du protocole dans `RECU.txt`)
- **Reçu :** `../receipts/campagne_g4_v5_20260827/` (16 × `.txt` + `.status`, `session.log`, `RECU.txt`)
- **Machine :** `g4-standard-48` SPOT, `europe-west4-ai1a`, 48 fils, GNU time ; VM démarrée 12:54Z, **arrêt certifié `TERMINATED` 13:19Z** (25 min), revérifié en lecture seule après la session. Aucune autre VM touchée.
- **Validation :** `validate_v5_campaign.py` épinglé → `campaign_status=complete` (16 runs), `remote_campaign_rc=0`, `scp_rc=0`, `session_rc=0`.
- `public_status=not_claimed` ; **aucune cible CUDA** : la session est 100 % CPU sur une machine à GPU.

## Phase 1 — conformité v4 ≡ v5 sur la VM (48 fils)

Les 12 runs (`uniform`, `terrain`, `eight_clusters`, `scanline_single_pass` × 8000/16000/32000) impriment `balls=egal all=egal`, code 0. Temps (s) et RSS max : `uniform` 32000 129 s / 11,3 Go ; `eight_clusters` 32000 196 s / 9,5 Go (contre 1 672 s à 8 fils sur machine partagée) ; `terrain` 32000 22 s ; `scanline` 32000 25 s.

## Phase 2 — contrat 50 000 points (K = 1..10 exact, s = 8, smax = 11, graine 3)

| famille | temps mur | RSS max | boules uniques | événements | facettes K=10 |
|---|---:|---:|---:|---:|---:|
| uniform | **219 s** | 17,2 Go | 21 622 480 | 21 558 051 | 43 563 526 |
| eight_clusters | **375 s** | 16,1 Go | 20 393 168 | 19 722 827 | 39 433 314 |
| terrain | 42 s | 3,1 Go | 3 978 613 | 3 964 427 | 6 442 612 |
| scanline_single_pass | 54 s | 2,8 Go | 3 707 819 | 3 978 593 | 6 050 252 |

C'est la **première forêt complète à 50 000 points** obtenue dans ce dépôt (la v4 la déclarait « jamais terminée ») ; ses `digest_all` sont gravés dans le reçu comme référence v5 à 50 k. Le contrat hérité (« K = 1..10 en < 100 ms sur G4 ») est à un facteur ~2 000 : il reste une cible, pas un résultat.

## Décomposition par étape (ms, 48 fils) — ce qui coûte vraiment

| étape | uniform 50k | eight_clusters 50k | terrain 50k |
|---|---:|---:|---:|
| génération (trois lanes) | 16 773 | **190 896** (rects q3 93 986, q4 87 630) | 13 686 |
| RLE | 4 401 | 4 099 | 737 |
| préfiltre de profondeur | 6 948 | 6 873 | 992 |
| census | 4 046 | 3 800 | 595 |
| expansion par K | 1 992 | 1 929 | 423 |
| **fold (dix ordres, séquentiel)** | **114 791** | **103 458** | 13 217 |

Deux verdicts :

1. **Le fold est le poste dominant sur les familles régulières (52 % du mur à `uniform`, un seul cœur actif)** : la v5 streame les dix folds séquentiellement pour tenir la résidence (17 Go ici contre les dix forêts résidentes de la v4). Le prochain gain est de paralléliser *à l'intérieur* d'un fold (internement par tranches, tri parallèle des clés uniques, roles par lot) sans remonter la résidence — mesuré par banc apparié, jamais déclaré.
2. **La génération domine sur la famille adversariale** (`eight_clusters` : 94 s de lane q3 et 88 s de lane q4 à 48 fils, contre 6,5 s à `uniform`) : ce sont les covers denses (amas serrés, milieux vides) — le poste que le port GPU devra viser en premier (kernel affine par ancre, warp-par-seed), et où l'ouvrier mesuré vaut bien 48.

Ces chiffres sont des mesures de coût sur machine dédiée, avec ouvriers mesurés (`ouvriers wspd=48/48/48 rects=48/48/48 prefiltre=48 census=48 expansion=48`) ; ils ne qualifient aucun SLO.
