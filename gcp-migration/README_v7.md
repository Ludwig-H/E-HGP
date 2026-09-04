# Session bornée G4 v7

Ce protocole capture le worktree v6/v7 réel, pas un commit historique présenté
comme source v7. Son résultat reste `public_status=not_claimed`. Il ne crée
ni ne reconfigure de VM et ne gère aucune clé OS Login.

Avant tout lancement, lire les règles GCP de `AGENTS.md`, revoir les nouveaux
fichiers, geler les sources et exécuter localement :

```bash
python3 gcp-migration/selftest_session_v7.py
python3 -O gcp-migration/selftest_session_v7.py
bash -n gcp-migration/session_campagne_v7_g4.sh
```

Ces tests n'utilisent pas GCP. Ils couvrent le processus borné, descendants
ignorant TERM et leader déjà sorti, les snapshots/modes, la réception,
les générations de garde, onze issues de cycle de vie dont disque plein,
et le vrai trap d'urgence du start appelant le vrai stop sous faux gcloud.
Les fixtures positives/adverses du candidat utilisent le parseur épinglé.
Le bootstrap CPU est exécuté en sandbox avec faux sudo/dpkg/apt : outils
présents, installation minimale, échec et timeout. Des versions falsifiées
et une garde absente sont rejetées, aussi sous Python optimisé.

Préparer une copie dans un répertoire privé persistant 0700, hors build :

```bash
python3 gcp-migration/v7_g4_session.py prepare --session-base /CHEMIN/PRIVE/sessions-v7
```

Conserver le chemin absolu imprimé. La préparation inclut les deux arbres
source, les fixtures de conformité historiques explicites, les parsers,
la provenance du port et le contrôleur/wrapper/gardes. Chaque fichier est
copié, haché et relu ; les fichiers sources sont 0444, les exécutables 0555.
Le manifeste et l'archive ont chacun leur SHA-256. Ne pas modifier cette
copie ni réutiliser une session déjà tentée.

Juste avant `run`, préparer une clé ED25519 de session privée 0600 avec
expiration OS Login unique à 70 minutes, selon les gardes existantes.
Ne jamais inscrire son contenu privé dans un log ou reçu. La VM doit être
TERMINATED, label `project=e-hgp`, `g4-standard-48`, SPOT, action STOP,
maintenance TERMINATE, redémarrage automatique désactivé, durée GCE 3600 s.
Si une reconfiguration est nécessaire, elle relève d'une étape autorisée
distincte via les scripts gardés du dépôt.

Lancer exclusivement avec une cible exacte vérifiée :

```bash
python3 gcp-migration/v7_g4_session.py run \
  --session /CHEMIN/PRIVE/sessions-v7/v7session.ID \
  --project devpod-gpu-exploration --zone europe-west4-a \
  --instance ehgp-blackwell-spot --ssh-key /CHEMIN/PRIVE/cle-session
```

Le wrapper `session_campagne_v7_g4.sh` accepte les mêmes arguments.
`--without-gpu` désactive explicitement les seules primitives device.

Le contrôleur appelle la garde start avec 45 minutes invité et vérifie son
marqueur `double_guard_verified` avant transfert/calcul. Le worker a 2100 s
maximum, toujours avant l'échéance invitée moins 180 s. Ordre des travaux :
provisionnement CPU minimal si nécessaire, CPU v6/v7 apparié uniform/terrain 50k, diagnostic candidat default50k,
diagnostic wide8k borné240s si possible, puis primitives GPU C2/C4 si outils
et au moins780s disponibles. Pas de pipeline GPU C6, pas de campagne50M,
pas de mise à jour pilote, pas de reboot, pas d'allongement automatique.

L'image Ubuntu peut ne fournir ni compilateur ni CMake. **Après** les deux
gardes, le worker vérifie les paquets `build-essential`, `cmake`,
`libboost-dev`, `time` et les exécutables. S'il en manque, il exécute seulement
`apt-get update` puis leur installation non interactive sans recommandations,
sans upgrade global ni redémarrage des services demandé par needrestart.
Le processus privilégié porte son propre `timeout` de 300 s maximum, avec
KILL après 10 s, à l'intérieur du budget worker et de son délai invité.
La fenêtre complète journalisée conserve une marge de 20 s ; une erreur apt
ou une expiration invalide le worker puis déclenche l'arrêt exact habituel.

Les versions littérales CMake, compilateur, Make, GNU time et paquets sont
conservées, ainsi qu'une compilation/exécution C++20 utilisant Boost.
`cpu_toolchain.json` est reconstruit depuis ces preuves lors de la réception :
CMake >= 3.20, C++20 et Boost >= 1.74 sont exigés. Cela qualifie l'outillage,
pas le moteur ; ses vrais builds et juges restent obligatoires. Le GCC 11
fourni par Ubuntu 22.04 n'est pas remplacé automatiquement.

`nvcc` est recherché dans PATH et les répertoires CUDA connus. S'il est
présent, sa version et son code de sortie sont conservés dans
`gpu_tools.json` et les fichiers bruts ; s'il est absent ou défaillant,
aucune installation CUDA/pilote/GPU n'est tentée et les primitives restent
indisponibles. Les reçus des premières sessions en échec sont conservés.

Le statut final du contrôleur décrit l'exécution du protocole, pas
l'exactitude du moteur. `candidate_status.json`, `candidate_wide_status.json`
et `gpu_status.json` séparent succès moteur, refus, censure et étape omise.
Les commandes, stdout/stderr, codes, temps/RSS, SHA binaires et manifests
restent dans la session. L'entrée générée est identifiée par paramètres et
source ; aucun SHA littéral du tableau de points n'est revendiqué.

La réception est re-hachée et reparsée avant publication. L'arrêt exact se
fait même si le journal est inaccessible, avec `lastStartTimestamp` connu,
puis l'état TERMINATED de la même génération est relu. Une erreur de
capacité avant nouvelle génération est close seulement si la cible est
encore TERMINATED avec sa génération précédente. Code74 signifie arrêt
non certifié : le JSON final donne la cible et la commande de contrôle.
Les autres VM ne sont jamais arrêtées par ce protocole.

Le reçu d'une session interrompue ne s'invente pas. En cas de perte du
contrôleur ou SIGKILL, vérifier la cible et reprendre l'arrêt via
`stop_and_verify.sh --yes --expected-last-start-timestamp HORODATAGE`
avec exactement les mêmes variables GCP_PROJECT_ID/GCP_ZONE/GCP_INSTANCE_NAME.
La durée GCE et l'arrêt invité sont des coupe-circuits, pas une preuve que
l'état final a été lu et certifié.
