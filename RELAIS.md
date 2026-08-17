# RELAIS — session pilote GCP « e-hgp-gcp » → session principale

Campagne d'échelle MorseHGP3D v4 sur g4-standard-48 SPOT. Base : `main@772a8d9`.
Dernière mise à jour : 2026-08-17 ~19h50 UTC.

## sonde

Réseau complet confirmé (proxy agent non sélectif, `selective:false`) :

| Endpoint | Code HTTP | Verdict |
|---|---|---|
| https://dl.google.com/ | 302 | ouvert |
| https://accounts.google.com/ | 302 | ouvert |
| https://oauth2.googleapis.com/ | 404 | ouvert (404 attendu sur la racine) |
| https://compute.googleapis.com/ | 404 | ouvert (404 attendu sur la racine) |
| https://storage.googleapis.com/ | 400 | ouvert (400 attendu sur la racine) |

SDK Google Cloud 532.0.0 installé depuis le tarball officiel (`storage.googleapis.com`), `gcloud --version` OK.

## url-oauth

(en attente — flux `gcloud auth login --no-launch-browser` en cours de lancement, URL publiée dans la prochaine mise à jour)

## état

- [x] Étape 1 — sonde réseau : **OK**
- [ ] Étape 2 — flux OAuth : URL à publier, puis attente du message « CODE AUTH: … »
- [ ] Étape 3 — test de caviardage (lecture seule) : GO / NO-GO
- [ ] Étape 4 — campagne `session_campagne_v4_scale_g4.sh` (seulement si GO)

Rappel : aucune mutation GCP avant le GO de l'étape 3 ; aucun jeton ni code ne sera écrit dans ce fichier.
