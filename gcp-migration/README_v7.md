# Session bornée G4 v7

Ce protocole capture le worktree v6/v7 réel, pas un commit historique présenté
comme source v7. Son résultat reste `public_status=not_claimed`. Il ne crée
ni ne reconfigure de VM et ne gère aucune clé OS Login.

Avant tout lancement, lire les règles GCP de `AGENTS.md`, revoir les nouveaux
fichiers, geler les sources et exécuter localement :

```bash
python3 gcp-migration/selftest_session_v7.py
python3 -O gcp-migration/selftest_session_v7.py
python3 -B gcp-migration/selftest_private_cmake_v7.py
python3 -B -O gcp-migration/selftest_private_cmake_v7.py
python3 -B gcp-migration/selftest_private_cmake_controller_v7.py
python3 -B -O gcp-migration/selftest_private_cmake_controller_v7.py
python3 -B gcp-migration/selftest_cpu_towers_v7.py
python3 -B -O gcp-migration/selftest_cpu_towers_v7.py
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
provisionnement CPU minimal si nécessaire, CPU v6/v7 apparié uniform/terrain 50k K1..10,
repli réellement exécuté K1..5 si nécessaire et possible, diagnostic candidat default50k,
diagnostic wide8k borné240s si possible, puis primitives GPU C2/C4 si outils
et au moins780s disponibles. Pas de pipeline GPU C6, pas de campagne50M,
pas de mise à jour pilote, pas de reboot, pas d'allongement automatique.

Le repli K5 utilise les mêmes nuages de 50000 points, seed3, séparation8,
CSR et48threads, avec `--smax=6` au lieu de11. Il est demandé si le K10 v7
achevé dépasse1000ms **de temps processus externe**, si son watchdog120s
le censure ou si le moteur émet un refus qualifié. Le temps externe inclut
génération et digest ; `pipeline_ms` est conservé séparément. Les résultats
K5 ne sont ni extrapolés ni obtenus en tronquant la sortie K10 : les deux
binaires v6/v7 sont réexécutés, puis le parseur exige la ligne K5 exacte,
cinq cardinalités et cinq digests avec leur agrégat chaîné.

`cpu_campaign.json` et les observations brutes distinguent `engine_completed`,
`engine_refused`, `censored`, `failed`, `invalid` et `not_attempted`.
Une paire incomplète est `not_comparable`, sans champ `equal`, objet ou digest
inventé. Une vraie divergence reste `diverged`. Un K10 censuré/refusé reste
un échec de campagne CPU même si K5 ou le GPU réussit ensuite : le worker
termine alors à1 avec `diagnostics_completed=true`, et la réception revalide
quand même tous les sous-résultats indépendants. Le refus mathématique
`unsupported_degeneracy` reste distinct de `resource_exhausted`.

Quand le GPU est demandé,920s sont réservées à travers **chaque** repli K5
et les deux candidats. Une paire K5 ne commence qu'avec1200s restantes
(2×120s +2×20s de drainage +920s), avec recontrôle avant chaque bras.
Chaque candidat ne commence que si son plafond120s/240s,20s de drainage
et cette réserve restent disponibles. Sinon, `not_attempted` avec
`budget_insufficient` est enregistré : ni report silencieux, ni allongement
de la VM ou du budget worker2100s. Le plan et ses limites sont revalidés au
retour. La réserve facilite une vraie qualification des primitives GPU,
sans promettre leur disponibilité ni leur réussite.

La porte `selftest_cpu_towers_v7.py` emploie des sorties **synthétiques** de
50000 points et les vrais parseurs5/10 : elle exerce censure/refus K10,
véritable branche K5, divergence/censure K5, budgets avant et pendant la
paire, candidats omis, refus des faux `equal` et revalidation GPU malgré
un terminal CPU non nul. Ces fixtures ne sont pas des mesures de performance.

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

Pour CUDA20, CMake système doit être au moins3.26 : le module NVIDIA de
CMake3.22 ne fournit pas le flag correspondant. Le GPU seul peut utiliser
une distribution privée CMake3.31.6 quand le système est plus ancien,
GPU demandé et NVCC fonctionnel. Le CPU conserve les outils apt mesurés.
Cette voie exige au moins920s restantes :120s pour téléchargement,
extraction et probes,780s pour les primitives GPU,20s de marge de drainage.
La garde invitée est relue avant l'appel ; le helper entier est lancé par
`run_logged` dans un groupe de processus possédé, avec watchdog120s. Les
contrôles coopératifs de délai et les timeouts socket ne remplacent pas
cette enveloppe externe, notamment pour les I/O bloquantes.

La [roue PyPI versionnée](https://pypi.org/pypi/cmake/3.31.6/json),
`cmake-3.31.6-py3-none-manylinux_2_17_x86_64.manylinux2014_x86_64.whl`,
est épinglée à27800904octets et au SHA-256
`1c8b05df0602365da91ee6a3336fe57525b137706c4ab5675498f662ae1dbcec`.
Le ZIP complet est vérifié avant extraction : chemins traversants,
absolus, doublons, liens et fichiers spéciaux sont rejetés ; taille
décompressée et nombre de membres sont plafonnés. Seuls les fichiers
CMake natifs et leurs ressources sont extraits dans
`root/tooling/cmake-3.31.6`, privé et create-only, **hors `out/`** : pas de
pip, dépôt OS étranger, installation CUDA/pilote, reboot ni copie des gros
binaires dans les reçus. Un échec peut laisser ce provisoire privé borné ;
il n'est jamais sélectionné ni réutilisé automatiquement.

`private_cmake_v7.py` est lui-même inclus et haché dans le snapshot. Les
reçus conservent commandes, statut, timeout, échéance dérivée, SHA du helper
et de la roue, tailles, versions/probes et SHA des exécutables. La réception
rejoue leur validation, contrôle les chemins réellement choisis pour
configuration/build/CTest et exige `--no-tests=error`. Si l'installation
est omise faute de budget, censurée ou en échec, CMake3.22 n'est pas utilisé
pour une prétendue réussite CUDA20 : le GPU est explicitement indisponible.

Un smoke réel local de cette roue a réussi sous watchdog120s, avec
extraction privée et probes natives cmake/ctest3.31.6 : voir
`morsehgp3D_v7/receipts/gcp_cmake_overlay_20260904/summary.json`.
Ce résultat qualifie cet outillage local, pas une exécution GPU du moteur.

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
