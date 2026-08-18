# RELAIS — session pilote GCP « e-hgp-gcp » ↔ session principale

Campagne d'échelle MorseHGP3D v4 sur g4-standard-48 SPOT. Base : `main@772a8d9`.
Dernière mise à jour : 2026-08-18 (session principale).

## DÉCISION — voie OAuth FERMÉE, voie compte de service ADOPTÉE

La session principale entérine le refus du pilote : **plus jamais de flux
`gcloud auth login` interactif dans le bac à sable** (URL + relais de code =
chaîne de captation de justificatif, jeton `cloud-platform` du compte
personnel entier — refusé même sur ordre). L'URL OAuth précédemment publiée
ici est caduque et ne sera pas régénérée.

La voie retenue : **compte de service GCP attaché à l'environnement remote**
(paramètres de l'environnement Claude Code, cf.
`code.claude.com/docs/en/claude-code-on-the-web`). Le justificatif ne
transite ni par le chat, ni par ce dépôt, ni par ce fichier.

## Rôles minimaux (dérivés des scripts gardés, session principale)

Verbes réellement utilisés par `session_campagne_v4_scale_g4.sh` →
`start_and_verify.sh` / `stop_and_verify.sh` / `deploy.sh` :
`instances start|stop|create|describe|list|set-scheduling`,
`disks list`, `images describe-from-family`, `regions describe`,
`project-info describe`, `os-login ssh-keys add|remove`,
`os-login describe-profile`, `compute ssh|scp` (OS Login, `sudo -n` côté
invité pour le garde d'extinction), `beta quotas info describe` (préflight).
`deploy.sh` crée la VM avec `--no-service-account --no-scopes` (aucun
`actAs` requis par défaut).

Rôles à accorder au compte de service, **au niveau du projet** :

1. `roles/compute.instanceAdmin.v1` — cycle de vie de la VM (start/stop/
   create/describe/set-scheduling, disques, images, régions).
2. `roles/compute.osAdminLogin` — SSH par OS Login **avec sudo** (le garde
   invité exécute `sudo -n`), et gestion des clés de son propre profil
   OS Login.
3. (Optionnel, préflight quotas : `roles/cloudquotas.viewer` — sinon
   `check_quotas.sh` échouera proprement et peut être sauté.)
4. (Seulement si `RUNTIME_SERVICE_ACCOUNT` est un jour utilisé dans
   `deploy.sh` : `roles/iam.serviceAccountUser` sur ce compte-là — pas
   nécessaire aujourd'hui.)

## Marche à suivre pour l'utilisateur (aucun terminal requis)

1. Console GCP → IAM & Admin → Comptes de service → créer
   `e-hgp-g4@<PROJET>.iam.gserviceaccount.com` ; accorder les rôles 1–2
   (± 3) ci-dessus au niveau du projet.
2. Créer une **clé JSON** pour ce compte (à révoquer après la campagne).
3. claude.ai/code → paramètres de l'**environnement** de la session pilote →
   variables d'environnement (secret) : `GCP_SA_KEY_JSON` = contenu du JSON ;
   et script de setup de l'environnement :

   ```bash
   umask 077
   printf '%s' "$GCP_SA_KEY_JSON" > "$HOME/.gcp-sa.json"
   gcloud auth activate-service-account --key-file="$HOME/.gcp-sa.json"
   gcloud config set project <PROJET>
   export GOOGLE_APPLICATION_CREDENTIALS="$HOME/.gcp-sa.json"
   ```

4. Redémarrer la session pilote (nouveau conteneur) et dire « c'est en
   place ».

## HOLD LEVÉ (18 août) — protocole corrigé poussé : main@db2f4f2

Les deux audits bloquants (9223888 / b3a6eb4) sont exécutés. **Tirer
origin/main >= db2f4f2**, puis lancer dans CET ordre (celui des
audits — la décision d'architecture d'abord) :

1. **Session A (prioritaire)** :
   `PHASE=n32000 ./gcp-migration/session_scale_threads_g4.sh`
   — uniform t1/t8/tmax + eight_clusters t8/tmax, appariés par digest
   canonique ; préflight de budget intégré (refus code 2 si le budget
   ne tient pas — ne pas le contourner) ; ~5,75 h de budget, session
   25 200 s.
2. **Session B** : `PHASE=n64000 ./gcp-migration/session_scale_threads_g4.sh`
   — 4 familles à nproc fils (~4,75 h).
3. **Session C (si encore utile après A/B)** :
   `./gcp-migration/session_campagne_v4_scale_g4.sh` — la couverture
   historique 28 runs, restaurée à l'identique.

Chaque session : son pin, son validateur épinglé, son reçu, sa
certification TERMINATED — publier ici après CHAQUE session (pin +
campaign_status + la ligne de chaque .status). Une session A complète
vaut reçu même si B est préemptée. Préalable inchangé : auth de
l'étape A vérifiée (`gcloud auth list`).

## FEU VERT UTILISATEUR (18 août, plus récent)

« Les vrais tests qui décident de l'architecture doivent être sur des
nuages massifs n=8000,16000,32000,64000. Feu vert pour utiliser GCP !
Et privilégie des structures qui se paralléliseront bien ! »

Conséquences opérationnelles :

1. La campagne est ÉTENDUE (main@a694496) : phase 3 « échelle de
   fils » — n=32000 à 8 fils et nproc fils, n=64000 à nproc fils
   (uniform/terrain/eight_clusters/scanline), 36 statuts au total,
   selftest PROTOCOLE CONFORME. **Tirer origin/main >= a694496 avant de
   lancer** — le pin de protocole hache le script et le validateur
   depuis le commit.
2. Séquence inchangée : étape A (auth), B (préflight lecture seule),
   C (`session_campagne_v4_scale_g4.sh` avec ses garde-fous), D
   (certification `TERMINATED` + reçus publiés ici).
3. `RUN_TIMEOUT` par run = 3 h ; la phase 3 est séquentielle et vient
   APRÈS les vagues mono-fil — la session complète peut approcher la
   `maxRunDuration` : surveiller et, si la fenêtre se ferme, publier
   les statuts partiels (le validateur les qualifiera partial — reçus
   quand même, jamais de troncature silencieuse).

## MESSAGE DE L'UTILISATEUR (18 août, via la session principale)

« Pour le GCP, j'ai déjà créé tout ce qu'il fallait ; les informations
nécessaires sont dans gcp-migration. » — Ne pas attendre d'autre
configuration : vérifier l'auth MAINTENANT (étape A) et enchaîner si
elle est là. Paramètres de la cible (gcp-migration/README.md) :

- Projet : `devpod-gpu-exploration` (les scripts refusent tout autre
  projet actif — `gcloud config set project devpod-gpu-exploration`).
- VM : `ehgp-blackwell-spot`, zone `europe-west4-a` (g4-standard-48
  SPOT, RTX PRO 6000 Blackwell 96 Go, Hyperdisk Balanced) ; seul repli
  autorisé : `europe-west4-ai1a/ehgp-blackwell-spot-ai1a`.
- Quotas dimensionnés pour UNE G4 Spot concurrente ; jamais de
  Standard/on-demand.
- Si `gcloud auth list` ne montre AUCUN identifiant : publier ici
  « NO-AUTH » — la seule action restante est côté utilisateur
  (paramètres de l'environnement, section précédente) ; ne relancer
  aucun flux OAuth.

## Étapes côté pilote (après « c'est en place »)

- [ ] Étape A — `gcloud auth list` + `gcloud config get-value project`
      (lecture seule) : le compte de service actif est confirmé ici.
- [ ] Étape B — préflight lecture seule (`check_quotas.sh` si le rôle 3 est
      accordé, sinon `instances describe`).
- [ ] Étape C — campagne `gcp-migration/session_campagne_v4_scale_g4.sh`
      (garde-fous inchangés : SPOT, label `project=e-hgp`, double
      coupe-circuit, `maxRunDuration`).
- [ ] Étape D — **certification `TERMINATED`** sur exactement la cible,
      publiée ici avec les reçus de campagne.

Rappels inchangés : aucune mutation GCP hors scripts gardés ; aucun jeton,
code ou clé ne sera jamais écrit dans ce fichier ni dans le dépôt ; après
la campagne, l'utilisateur supprime la clé JSON du compte de service.
