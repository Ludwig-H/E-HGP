# RELAIS — session pilote GCP « e-hgp-gcp » ↔ session principale

Campagne d'échelle MorseHGP3D v4 sur g4-standard-48 SPOT. Base : `main@772a8d9`.
Dernière mise à jour : 2026-08-18 (session pilote Codespace — HOLD reçu, rien lancé).

## ⇒ HOLD REÇU — **rien lancé** (18 août, pilote Codespace)

`session_scale_threads_g4.sh` n'a **jamais** été exécuté : le refus du bac à
sable (section suivante) l'avait déjà empêché avant toute dépense. Aucune
interruption n'était donc nécessaire, aucun trap de cleanup à déclencher,
aucune certification à produire.

Inventaire **lecture seule** au moment du HOLD :

```
ehgp-blackwell-spot                europe-west4-a     g4-standard-48  TERMINATED
frangi-blackwell-spot              europe-west4-a     g4-standard-48  TERMINATED
ehgp-blackwell-spot-ai1a           europe-west4-ai1a  g4-standard-48  TERMINATED
cracksam-frangigraph-g4-spot-ew8c  europe-west8-c     g4-standard-48  TERMINATED
```

Aucun serveur tmux (`/tmp/tmux-1000/default` absent). `main` local =
`origin/main` = `c9c3a480`, worktree propre.

Brief A (~5 h 45) / B enregistré comme **ANNULÉ**. En attente du protocole
court `≤ 1 h` sur ce canal.

**Deux points à intégrer au nouveau brief, sinon il rebutera pareil :**

1. **Le refus du bac à sable n'est pas levé.** Il porte sur le démarrage
   d'une ressource facturable depuis une session non surveillée, pas sur la
   durée : un protocole d'une heure sera refusé exactement comme celui de
   5 h 45. Il faut une règle de permission Bash côté Louis, ou qu'il lance
   la ligne lui-même.
2. **Le constat « 100 % CPU » ci-dessous reste entier.** Il conditionne le
   *type de machine*, pas la durée : une heure de `g4-standard-48` reste une
   heure de GPU facturé au repos. À trancher avant de redescendre un brief.


## ⇒ SESSION A **NON LANCÉE** — refus du bac à sable (18 août, pilote Codespace)

**État GCP : inchangé. Aucune commande GCP mutante émise, aucune VM créée
ni démarrée, donc rien à certifier.** Les quatre instances du projet sont
`TERMINATED` avant comme après.

Tout ce qui précède la dépense a été exécuté et est VERT :

| Porte | Résultat |
| --- | --- |
| Étape 1 — `gcloud auth list` | `louis.hauseux@gmail.com` ACTIF |
| Étape 1 — projet | `devpod-gpu-exploration` |
| Étape 2 — pin | `HEAD=c9c3a480` ⊇ `ed28a89` (fast-forward, worktree propre) |
| Inventaire lecture seule | 4 instances, **toutes `TERMINATED`** |
| Préflight de budget (local) | `somme_timeouts=18000s` → `requis=20700s` (5 h 45) |
| — `MAX_RUN_SECONDS` | `25200 ≥ 20700` ✓ et `≤ 28800` ✓ |
| — arrêt invité | `24000s > 20700s` ✓ |
| — TTL clé SSH | `25200s > 24600s` ✓ |
| Porte transactionnelle `selftest_scale_threads.sh` | `violations=0`, **PROTOCOLE CONFORME** |

Inventaire publié (directive « vérifie qu'aucune VM ne tourne ») :

```
ehgp-blackwell-spot                europe-west4-a     g4-standard-48  TERMINATED
frangi-blackwell-spot              europe-west4-a     g4-standard-48  TERMINATED
ehgp-blackwell-spot-ai1a           europe-west4-ai1a  g4-standard-48  TERMINATED
cracksam-frangigraph-g4-spot-ew8c  europe-west8-c     g4-standard-48  TERMINATED
```

**Ce qui bloque** : le classificateur du mode auto du bac à sable a refusé
la commande de lancement (`tmux` + `PHASE=n32000
./gcp-migration/session_scale_threads_g4.sh`) — démarrage d'une ressource
facturable dans une session que personne ne surveille en direct. Le refus
n'a pas été contourné : le brief de la session principale n'est pas une
autorisation vivante de l'utilisateur, et forcer le passage aurait été
exactement ce que la garde protège. La levée appartient à Louis (règle de
permission Bash, ou lancement à la main).

Rien d'autre ne manque : au moment du déblocage, la séquence repart telle
quelle, pin inclus.

## ⚠ CONSTAT À TRANCHER AVANT DE RELANCER — `scale_threads` est **100 % CPU**

Vérifié sur `main@c9c3a480` : **aucune cible CUDA dans `morsehgp3D_v4`.**
Aucun `CMakeLists.txt` de v4 ne mentionne `CUDA`, `enable_language` ni
`.cu` ; le seul fichier `.cu` du module (`src/gpu/device_compile_witness.cu`)
n'est pas câblé au build. Le lanceur configure `cmake -S morsehgp3D_v4 -B
build -DCMAKE_BUILD_TYPE=Release` (sans option CUDA) et le runner exécute
`./build/mhgp4_forest_probe` sous `taskset -c 0-47`, dont un run `t1`
**strictement mono-fil**.

Conséquence : sur `g4-standard-48`, le GPU RTX PRO 6000 serait facturé **au
repos pendant les ~5 h 45** de la session A, et autant en session B.

Cela heurte de front la contrainte impérative du 9 août 2026 — *« ne jamais
utiliser une instance GPU pour une charge CPU seule ; une VM à GPU ne se
justifie que par du CUDA réellement exécuté »*, dont la règle d'application
est précisément « avant de démarrer une VM gardée, vérifier que la charge
lance bien un noyau CUDA ; sinon choisir un type de machine sans GPU ».

Mais `AGENTS.md` § *Sécurité des VM GCP* verrouille l'autre bout : le point
d'entrée unique `start_and_verify.sh` **vérifie le type `g4-standard-48`**,
et `gcp-migration/` est le seul chemin autorisé. Il n'existe donc aujourd'hui
aucun chemin gardé vers une machine 48 cœurs **sans** GPU.

Les deux règles sont en conflit réel et le partage n'appartient pas à une
session autonome. Deux issues, au choix de Louis :

1. **Accepter le GPU au repos** pour A et B — la campagne a besoin de 48
   cœurs et 176 Go, que le Codespace (2 vCPU) ne peut pas fournir ; c'est
   le seul type de machine que les scripts gardés acceptent.
2. **Ouvrir un chemin gardé CPU-only** (ex. `c4-standard-48`) dans
   `start_and_verify.sh` avant de dépenser — plus conforme à la règle du
   9 août, mais c'est une modification du garde-fou, donc un accord explicite.

Aucune des deux ne peut être choisie sans lui. Tant que le point 1 n'est pas
tranché, relancer la campagne telle quelle dépenserait sciemment contre une
règle qu'il a qualifiée d'impérative.


## ⇒ GO FINAL (18 août, le plus récent) — pin `main@ed28a89`

L'utilisateur a dit « Go ! (Et vérifie qu'aucune VM ne tourne avant
d'en lancer) ». Un troisième audit ciblé (66886c0) est arrivé et a été
EXÉCUTÉ avant dépense : le validateur lie désormais chaque nom de run à
son argv (hash NUL recalculé) ET à l'identité imprimée par le probe,
exige les workers MESURÉS au point de création des threads et le schéma
complet `digest_balls + K1..10 + all` / `cardinalites K=1..10` ;
selftest PROTOCOLE CONFORME (12 refus causaux), CTest 128/128.

**Tirer `origin/main >= ed28a89` avant de lancer** (le pin de protocole
hache script + validateur depuis le commit — un arbre plus vieux refuse).

La directive « aucune VM ne tourne » est dans le protocole :
`session_scale_threads_g4.sh` fait un inventaire LECTURE SEULE de
toutes les instances du projet avant toute mutation et REFUSE (code 2)
si l'une n'est pas `TERMINATED` — cible, repli `-ai1a` ou autre. En cas
de refus : publier l'inventaire ici, ne rien arrêter soi-même.

Séquence (inchangée par ailleurs, sections ci-dessous) :

1. Étape A — `gcloud auth list` (si vide : publier « NO-AUTH », stop).
2. Session A : `PHASE=n32000 ./gcp-migration/session_scale_threads_g4.sh`
3. Publier ici : pin + `campaign_status` + chaque ligne de `.status` +
   certification `TERMINATED`.
4. Session B : `PHASE=n64000 ./gcp-migration/session_scale_threads_g4.sh`
   puis publication idem.
5. Session C (`session_campagne_v4_scale_g4.sh`) seulement si A et B
   sont complètes et le budget de la journée le permet.

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
