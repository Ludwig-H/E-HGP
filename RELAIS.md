# RELAIS — session pilote GCP « e-hgp-gcp » → session principale

Campagne d'échelle MorseHGP3D v4 sur g4-standard-48 SPOT. Base : `main@772a8d9`.
Dernière mise à jour : 2026-08-18 ~01h55 UTC.

**⚠ URL OAUTH RÉGÉNÉRÉE** : la première URL (2026-08-17) a expiré au bout de 6 h sans code (le processus `gcloud` est mort sur EOF). Un nouveau flux a été relancé — utiliser **uniquement** l'URL ci-dessous (section « url-oauth »), valable ~6 h à partir de 01h55 UTC. Si elle expire encore, je relancerai et republierai.

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

Flux `gcloud auth login --no-launch-browser` lancé (FIFO + porteur d'écriture en tâche de fond du harnais). **URL à transmettre à l'utilisateur** — il doit s'y connecter avec son compte Google, puis renvoyer le code de vérification via la session principale sous la forme « CODE AUTH: <code> » :

```
https://accounts.google.com/o/oauth2/auth?response_type=code&client_id=32555940559.apps.googleusercontent.com&redirect_uri=https%3A%2F%2Fsdk.cloud.google.com%2Fauthcode.html&scope=openid+https%3A%2F%2Fwww.googleapis.com%2Fauth%2Fuserinfo.email+https%3A%2F%2Fwww.googleapis.com%2Fauth%2Fcloud-platform+https%3A%2F%2Fwww.googleapis.com%2Fauth%2Fappengine.admin+https%3A%2F%2Fwww.googleapis.com%2Fauth%2Fsqlservice.login+https%3A%2F%2Fwww.googleapis.com%2Fauth%2Fcompute+https%3A%2F%2Fwww.googleapis.com%2Fauth%2Faccounts.reauth&state=u9T4AyOllRMPH7hDfsX7rCIapVSOTq&prompt=consent&token_usage=remote&access_type=offline&code_challenge=YL_Tm0n7JDofNcQ9mkqwwkgpH10SfcB0g85ZPwlWs0A&code_challenge_method=S256
```

(Cette URL est publique par construction — PKCE, elle n'ouvre rien sans la connexion Google de l'utilisateur. Le code de vérification, lui, ne sera jamais écrit ici.)

## état

- [x] Étape 1 — sonde réseau : **OK**
- [~] Étape 2 — flux OAuth : URL publiée ci-dessus, **en attente du message « CODE AUTH: … »**
- [ ] Étape 3 — test de caviardage (lecture seule) : GO / NO-GO
- [ ] Étape 4 — campagne `session_campagne_v4_scale_g4.sh` (seulement si GO)

Rappel : aucune mutation GCP avant le GO de l'étape 3 ; aucun jeton ni code ne sera écrit dans ce fichier.
