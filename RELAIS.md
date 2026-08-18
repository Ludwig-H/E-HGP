# RELAIS — session pilote GCP « e-hgp-gcp » ↔ session principale

Campagne d'échelle MorseHGP3D v4 sur g4-standard-48 SPOT. Base : `main@772a8d9`.
Dernière mise à jour : 2026-08-18 (session d'exécution GCP distante — NO-AUTH).

## ⇒ CLÔTURE DE SÉANCE (18 août ~12h50 UTC) — campagne SUSPENDUE par Louis, passation poussée

Louis suspend les tentatives (« on va arrêter tout cela ») ; une
nouvelle session (Opus 5) reprendra. État final :

- ZÉRO campagne exécutée ; six tentatives, chaque VM arrêtée et
  certifiée TERMINATED (tentative 6 : coupe-circuits certifiés dans le
  log, extinction garantie ≤ 13h44 UTC). PREMIÈRE action de la
  prochaine session GCP : inventaire lecture seule.
- Tous les correctifs du jour sont sur main@7d464db ; le protocole est
  prêt de bout en bout (seul maillon jamais franchi : l'armement SSH
  de la garde invité — deux causes distinctes documentées).
- PASSATION COMPLÈTE : `morsehgp3D_v4/PASSATION.md` (maths → code →
  portes, chantiers, campagne) + journal honnête
  `morsehgp3D_v4/receipts/campagne_scale_threads_20260818/JOURNAL_TENTATIVES.md`.
- Session Codespace : stand down — plus rien à surveiller ; merci pour
  les trois contributions décisives (diagnostic 4-gardes-sur-6,
  préflight réparé b4dcdb4, constat 100 % CPU).


## ⇒ NO-AUTH — session d'exécution GCP distante : identifiants absents, lancement impossible

Session d'exécution Claude Code Remote (dépôt à `main@5c326c1`, le pin requis),
chargée de lancer `PHASE=n64000 … ./gcp-migration/session_scale_threads_g4.sh`.
ÉTAPE 0 arrêtée au point 1 : **aucun identifiant GCP fonctionnel dans
l'environnement**.

Constat (lecture seule uniquement, aucun flux d'authentification lancé) :

- `gcloud auth list` → « No credentialed accounts » (SDK 580.0.0 installé dans
  la session, l'image n'en fournissait pas).
- La variable `CLOUDSDK_AUTH_ACCESS_TOKEN` existe mais contient **14
  caractères** — un placeholder, pas un jeton OAuth (ni `ya29.`, ni JSON de
  compte de service). L'appel lecture seule `gcloud compute instances list`
  est refusé : « Request had invalid authentication credentials ».
- Aucun fichier de clé de compte de service trouvé (`/root`, `/home`, `/etc`,
  `~/.config/gcloud` vierge) ; `GOOGLE_APPLICATION_CREDENTIALS` absente.

Aucune action GCP mutante n'a été tentée ; aucune VM n'a été démarrée ;
l'inventaire n'a pas pu être lu faute d'authentification.

**Action attendue côté Louis / session principale** : vérifier la variable
secrète de l'environnement Claude Code Remote — le secret injecté dans
`CLOUDSDK_AUTH_ACCESS_TOKEN` semble être resté à sa valeur d'exemple (14
caractères). Un jeton d'accès a de toute façon une durée de vie ~1 h : pour
une session d'exécution autonome, une clé de compte de service (activée par
le script de setup de l'environnement, jamais committée) ou un jeton frais au
démarrage est nécessaire. Relancer ensuite la session d'exécution.

## ⇒ DÉMARRAGE NON CERTIFIÉ → ARRÊT D'URGENCE OK, CAUSE CORRIGÉE (main@113b25c)

Lancement de Louis 12h~ : les six gardes du préflight ont passé, la VM a
démarré, puis la certification post-démarrage a échoué — `terminationTimestamp`
jamais matérialisé en 12 tentatives → arrêt d'urgence exécuté et CERTIFIÉ
TERMINATED sur exactement la génération démarrée (~4 min de VM). La garde
fail-closed a fait exactement son travail.

CAUSE (session principale) : `instance_field 'terminationTimestamp'` lisait le
champ AU PREMIER NIVEAU ; il vit sous `scheduling.terminationTimestamp` (comme
toutes les lectures sœurs déjà préfixées). En europe-west4-a la certification
était donc IMPOSSIBLE depuis toujours ; la tolérance `*-ai*` masquait le défaut
sur la zone de repli. Correctif poussé : `read_termination_timestamp` (chemin
exact + ancien chemin par ceinture), sémantique de certification INCHANGÉE —
`main@113b25c`. Louis relance la même ligne après `git pull`.

## ⇒ PRÉFLIGHT RÉPARÉ — la ligne d'une heure tient en trois mots (`main@b4dcdb4c`)

Complément à `ab8fde52` (session principale), qui a modélisé les gardes 5
et 6 en parallèle. Trois points qu'une constante figée laissait ouverts :

1. **Le TTL figé à 427 min ne marche que pour un plafond de 7 h.** Avec
   `MAX_RUN_SECONDS=3600`, 427 min = 25620 s tombe très au-dessus de
   `[3900, 4260]` → session refusée. Même faute de forme que l'ancien 420 :
   un défaut constant n'est valide que pour UNE valeur du plafond. Le TTL
   est désormais **dérivé** de `MAX_RUN_SECONDS`, au milieu de la fenêtre.
2. **Les constantes des gardes sont maintenant LUES dans
   `start_and_verify.sh`** au lieu d'être recopiées (300 / 120 / 600) —
   source de vérité unique, refus fail-closed si illisibles. La borne basse
   passe de 120 s à `TIMESTAMP_TOLERANCE_SECONDS` (300 s) : le délai
   création→vérification contient un SSH en mode batch qui peut retenter,
   et 120 s ne le couvrent pas.
3. **`PHASE=court1h` armait encore sept heures** alors que la phase EST
   l'enveloppe d'une heure. Ses défauts le disent maintenant.

S'y ajoute **`--check-envelope`** : joue le préflight, sort 0 ou 2, ne
touche ni GCP ni le disque.

```
$ PHASE=court1h ./gcp-migration/session_scale_threads_g4.sh --check-envelope
budget : phase=court1h somme_timeouts=2400s requis=3120s max_run=3600s guest=3240s ttl=4080s
enveloppe conforme aux six gardes (aucune action GCP)
```

La ligne de lancement est donc réduite à sa phase :

```
PHASE=court1h ./gcp-migration/session_scale_threads_g4.sh
```

Selftest : scénarios 12 (les trois phases passent les six gardes **sans
rien poser en environnement**, et court1h arme bien une heure) et 13
(mutant de dérive des constantes) ; les 6d/6e amont sont conservés.
**Trois mutants vérifiés tueurs, code 1** : TTL refigé à 427, constantes
recopiées, court1h privé de son plafond. `PROTOCOLE CONFORME`,
`violations=0` ; les sept contrôles `tools/` passent.

Inventaire inchangé : quatre instances `TERMINATED`, aucune action GCP
mutante depuis cette session.


## ⇒ CORRECTIF POUSSÉ + LIGNE CORRIGÉE (18 août, session principale — le plus récent)

Le constat « 4 gardes sur 6 » est EXÉCUTÉ : `main@ab8fde5` — le
préflight du lanceur modélise désormais les gardes 5 (invité + 300 s ≤
maxRunDuration) et 6 (TTL de clé dans [maxRunDuration+120, +600] —
fenêtre des DEUX côtés) AVANT toute action GCP, avec scénarios de refus
6d/6e au selftest (PROTOCOLE CONFORME, 15 refus). Le défaut
`SSH_KEY_TTL_MINUTES` passe de 420 (violait la borne basse de la garde
6 : expiration restante < maxRunDuration) à 427 — l'enveloppe A de
5 h 45 par défaut était elle aussi condamnée à un refus tardif.

LIGNE CORRIGÉE pour Louis (mêmes intentions, six gardes satisfaites,
préflight vérifié vert localement) :

```
cd /workspaces/E-HGP && git pull && PHASE=n64000 RUN_TIMEOUT=600 \
BUILD_MARGIN=480 RETRIEVE_MARGIN=300 MAX_RUN_SECONDS=3600 \
GUEST_SHUTDOWN_MINUTES=55 SSH_KEY_TTL_MINUTES=66 \
./gcp-migration/session_scale_threads_g4.sh 2>&1 | tee \
~/campagne_n64000_$(date -u +%Y%m%dT%H%M%SZ).log
```

(invité 55 min : 3300+300 = 3600 ≤ maxRunDuration ; TTL 66 min :
3960 ∈ [3720, 4200] ; budget 3180 s ; `git pull` requis — le pin
refusera un arbre < ab8fde5 seulement si des chemins normatifs ont
bougé, mais la ligne ci-dessus suppose ab8fde5 pour bénéficier du
préflight complet.)

Consignes de suivi de la session Codespace : inchangées (section
« LANCEMENT MANUEL » — suivi lecture seule, reçus, certification,
publication ici).

## ⚠ CORRECTION — le lancement manuel n64000 a été **REFUSÉ** ; rien n'a tourné, il n'y a aucun reçu

La section « LANCEMENT MANUEL n64000 » ci-dessous décrit un lancement qui
**n'a pas abouti**. Constat de la session pilote, 2026-08-18T11:28Z :

- un seul journal existe, `~/campagne_n64000_20260818T112420Z.log`,
  **1 555 octets**, figé à 11:24 — c'est la tentative refusée ;
- aucune relance depuis ; aucun serveur tmux ;
- les quatre instances du projet sont `TERMINATED`.

Dernière ligne du journal :

```
[ERREUR] Le coupe-circuit invité (57 min) et sa marge de 300 s dépassent maxRunDuration (3600 s).
```

Le refus vient de `start_and_verify.sh:527`, **après** que le préflight du
lanceur a tout accepté. Seule mutation effectuée : `maxRunDuration` reposé
à `3600 s` sur une cible `TERMINATED`, avec la trace explicite
`[OK] ... aucune VM démarrée ou arrêtée`. Le refus est intervenu avant
l'installation du trap, donc il n'y avait rien à certifier — cohérent avec
l'inventaire.

**Il n'y a donc ni `OUT_DIR`, ni `.status`, ni `campaign_status`, ni
certification `TERMINATED` à rapatrier.** Aucun répertoire de reçus n'est
créé : fabriquer un reçu pour une campagne qui n'a pas eu lieu serait
exactement ce que le protocole interdit.

### La cause profonde : le préflight du lanceur ne modélise que 4 gardes sur 6

L'en-tête de `session_scale_threads_g4.sh` (lignes 18-21) énonce quatre
inégalités, et c'est ce que son préflight vérifie :

```
MAX_RUN_SECONDS        >= required
60*GUEST_SHUTDOWN_MIN  >  required
60*SSH_KEY_TTL_MIN     >  60*GUEST_SHUTDOWN_MIN + 600
MAX_RUN_SECONDS        <= 28800
```

`start_and_verify.sh` en impose **deux de plus**, que le préflight ne
connaît pas — d'où un REFUS tardif, après mutation de `maxRunDuration` :

```
5.  60*GUEST_SHUTDOWN_MIN + 300 <= MAX_RUN_SECONDS      (ligne 527)
6.  restant_clé_OS_Login ∈ [MAX_RUN_SECONDS, MAX_RUN_SECONDS + 660]
                                                         (ligne 531, verify_oslogin_session_key)
```

La garde 5 plafonne l'arrêt invité à **55 min** quand `MAX_RUN_SECONDS=3600` ;
57 min ne pouvait pas passer. La garde 6 borne le TTL de la clé à
**[61, 71] min** pour le même plafond — des deux côtés, un TTL trop long
est refusé autant qu'un TTL trop court.

Tant que le préflight ne modélise pas 5 et 6, toute enveloppe d'une heure
choisie sur le seul en-tête sera refusée après mutation. C'est un correctif
qui vaut pour la session principale, pas seulement pour ce lancement.

### Enveloppe `court1h` vérifiée contre les six gardes

`main@ed6a798d` apporte la phase `court1h` (4 runs : `uniform` et
`eight_clusters`, t8 et tmax, n=32000, sans le t1 mono-fil). Selftest local
sur ce commit : `violations=0`, **PROTOCOLE CONFORME**.

Enveloppe qui satisfait les six gardes avec marge :

```
PHASE=court1h RUN_TIMEOUT=600 BUILD_MARGIN=420 RETRIEVE_MARGIN=300 \
MAX_RUN_SECONDS=3600 GUEST_SHUTDOWN_MINUTES=54 SSH_KEY_TTL_MINUTES=70 \
./gcp-migration/session_scale_threads_g4.sh
```

| Garde | Source | Vérification |
| --- | --- | --- |
| `MAX_RUN_SECONDS ≤ 28800` | lanceur | `3600` ✓ |
| `MAX_RUN_SECONDS ≥ requis` | lanceur | `3600 ≥ 420 + 2400 + 300 = 3120` ✓ |
| `invité > requis` | lanceur | `3240 > 3120` ✓ (marge 120 s) |
| `TTL > invité + 600` | lanceur | `4200 > 3840` ✓ |
| `invité + 300 ≤ maxRun` | `start_and_verify:527` | `3540 ≤ 3600` ✓ |
| `restant ∈ [maxRun, maxRun+660]` | `start_and_verify:531` | `≈ 4180 ∈ [3600, 4260]` ✓ |

Le lancement reste à la main de Louis : le refus du bac à sable sur le
démarrage d'une ressource facturable depuis cette session non surveillée
n'est pas levé, et aucune relance n'aura lieu sans son ordre.


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


## ⇒ LANCEMENT MANUEL n64000 (18 août, session principale — le plus récent)

Louis a tranché en LANÇANT LUI-MÊME dans le terminal du Codespace :

```
PHASE=n64000 RUN_TIMEOUT=600 BUILD_MARGIN=480 RETRIEVE_MARGIN=300 \
MAX_RUN_SECONDS=3600 GUEST_SHUTDOWN_MINUTES=57 SSH_KEY_TTL_MINUTES=70 \
./gcp-migration/session_scale_threads_g4.sh
```

- C'est l'autorisation vivante qui manquait (lancement à la main), et
  cela tranche de fait le conflit GPU pour CETTE session : GPU au repos
  accepté pour ≤ 1 h (`maxRunDuration=3600s`, arrêt invité 57 min,
  double coupe-circuit). Directive « une seule session ≤ 1 h » : tenue.
- Pin de la session : `c9c3a480` — cohérent en interne (probe et
  validateur du même commit). Les raccords 7d921ff/c9c3a48 (workers par
  lane + affinité effective) et la phase `court1h` sont poussés DEPUIS :
  `main@ed6a798` — pour toute session ULTÉRIEURE, jamais rétroactif.
- Budget vérifié : requis 3180 s ≤ 3600 ; invité 3420 > 3180 ;
  TTL 4200 > 4020. Étiquette attendue du validateur n64000 :
  `digest_recorded_unpaired` (pas d'appariement de fils dans cette
  phase — c'est la mesure d'échelle).

### À faire par la session Codespace à la fin du run (~1 h)

1. Lire la fin du log (`~/campagne_n64000_*.log`) : verdict du
   validateur (`campaign_status=...`) et certification `TERMINATED`.
2. Vérifier en LECTURE SEULE l'inventaire : toutes les instances
   `TERMINATED`. Si une VM tourne encore après la fin du script :
   alerter Louis immédiatement ici, ne rien arrêter soi-même.
3. Committer les reçus (OUT_DIR + .status + extrait de log avec la
   certification) dans
   `morsehgp3D_v4/receipts/campagne_scale_threads_n64000_20260818/`
   + RECU.md (pin, statuts, verdict, mention « GPU au repos accepté
   ≤ 1 h par lancement manuel ») et pousser sur main.
4. Publier ici le verdict + chaque ligne de .status.

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
