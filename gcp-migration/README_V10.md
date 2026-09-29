# Session G4 v10 : protocole minimal gardé

`backend=reference_cpu`, `public_status=not_claimed` : une mesure prise ici ne certifie rien.

Quatre fichiers :

- `v10_session.py` : le contrôleur (dry-run, lanceur détaché, processus de session, reprise) ;
- `v10_worker.sh` : le worker invité (construction, commandes, emballage) ;
- `v10_selftest.py` : l'autotest hors ligne ;
- ce `README_V10.md`.

Le contrôleur et le worker utilisés sont **ceux du commit** de la session. Le contrôleur exécuté doit être identique à celui du commit, sinon la session est refusée.

Le cycle de vie reprend celui de `tower_session_v9.py`, en plus simple, sans exécuter aucun module v7/v9. Seuls les deux scripts gardés existants démarrent et arrêtent la VM : `start_and_verify.sh` et `stop_and_verify.sh`, lus au commit et épinglés par sha256 (mêmes épingles que v7/v9). La session fait par ailleurs, elle-même :

- des `gcloud compute ssh/scp`, qui réinscrivent la clé à chaque appel (UpdateSshPublicKey) ;
- l'ajout, puis le retrait, de sa clé OS Login.

## Lancer : une seule façon

```bash
# 1. committer et pousser le protocole et le code v10 sur origin/main
# 2. valider sans rien lancer (lectures GCP seulement ; git fetch met à jour refs/remotes/origin)
python3 gcp-migration/v10_session.py --commit <sha> --plan plan.json --data /chemin/des/donnees \
  --session-dir /workspaces/.ehgp-sessions/v10.<date> --max-run-seconds 3600
# 3. lancer : même commande + --execute (rend la main aussitôt) ; + --wait pour attendre la fin
python3 gcp-migration/v10_session.py ... --execute
```

`--execute` refait toute la validation, puis :

- crée le dossier de session en 0700, avec `session.lock` ;
- crée un dossier d'exécution jetable sous `$TMPDIR` ;
- lance le processus de session dans une **nouvelle session Unix**, avec ses sorties dans `session.stdout` et `session.stderr`, stdin sur `/dev/null`, sans terminal ni tube ;
- imprime son PID, gravé aussi dans `launch.json`, et rend la main.

Le processus détaché ne reçoit pas de Ctrl-C ; seuls SIGTERM et SIGHUP lui parviennent. La fin est signalée par la **sentinelle `DONE`**, qui contient le code de sortie. Elle est écrite dans la session ou, à défaut, dans le dossier d'exécution.

Avec `--wait`, le lanceur attend `DONE` et rend le même code. Si `DONE` manque, il rend le vrai code du processus de session, avec `done_missing`. Il ne rend 70 que si ce processus est mort sans statut. Tuer le lanceur n'arrête pas la session. L'option interne `--child` sert au lanceur et à l'autotest.

| Code | Sens |
|---|---|
| 0 | `completed` (tout conforme, arrêt certifié, reçu sans erreur) ; aussi « lancé » sans `--wait`, et dry-run réussi |
| 2 | `failed_before_start` : refus avant toute mutation, ou démarrage jamais certifié. Le reçu dit `gcp_mutation_phase_entered`, `start_may_have_been_requested` et `closure` : une VM démarrée puis arrêtée par le trap du démarrage y apparaît comme `already_terminated`. `--recover` rend aussi 2, sans rien faire, si le dossier de session n'existe pas. |
| 3 | `failed_remote` : démarrage certifié, puis un échec (transfert, construction, commande, débordement de résultats, rapatriement ou vérification) ; l'arrêt est certifié |
| 74 | `shutdown_uncertified` : suivre la reprise ci-dessous. Pour `--recover`, aussi toute erreur interne (épingle différente, reçu illisible…) |
| 75 | `foreign_generation_active` : notre génération est terminée, ou un démarrage étranger est en vol (y compris un RUNNING qui porte encore notre génération après un arrêt postérieur) ; **ne rien arrêter** |
| 76 | `--recover` seulement : le processus de session ou un script gardé de la session vit encore, rien n'a été arrêté |
| 70 | lanceur `--wait` : processus de session mort sans statut ; reprise ci-dessous |

## Plan

```json
{
  "schema": "ehgp.v10.session_plan.v1",
  "build_targets": ["mhgp10_cluster", "mhgp10_unit"],
  "build_timeout_seconds": 600,
  "results_cap_bytes": 536870912,
  "commands": [
    {"name": "unit", "timeout_seconds": 300, "argv": ["ctest", "--no-tests=error", "-L", "^fast$"]},
    {"name": "cluster_s00", "timeout_seconds": 600,
     "argv": ["./mhgp10_cluster", "{data}/scene_00.u32le", "{out}/s00.bin"]}
  ]
}
```

Les commandes tournent dans l'ordre, avec le dossier de build comme dossier courant. Chacune a son délai, et aucun shell n'intervient : les arguments sont passés littéralement.

Seuls ces jetons sont remplacés :

- `{src}` : la racine du paquet ;
- `{build}` : le dossier de build ;
- `{data}` : le dossier des données téléversées ;
- `{out}` : un dossier propre à la commande, rapatrié avec elle.

`argv[0]` ne peut prendre que trois formes :

- `./mhgp10_*` : une cible `add_executable` du commit, qui doit figurer dans `build_targets` si cette liste est donnée ;
- `python3 {src}/morsehgp3D_v10/<script suivi>.py` ;
- `ctest`, avec **`--no-tests=error` en premier argument**.

Pour `ctest`, les options `-S`, `-D`, `--build-*`, `-T`, `-M`, `--test-dir`, `-Q`, `--repeat` et `-N` sont refusées. Côté worker, un ctest n'est « ok » que si sa sortie contient `100% tests passed, 0 tests failed out of N`, avec N ≥ 1, **et** ne contient pas `The following tests did not run` (tests désactivés ou sautés) ; sinon son statut est `vacuous`.

Deux précautions sur ctest :

- une expression de label est une regex : `-L gate` sélectionne aussi `offgate` ;
- `-L gate` sur la vraie v10 inclut les oracles Python, qui exigent des binaires exclus par `build_targets` et durent plus de 600 s.

Python : `needs_python` est **déduit** dès qu'une commande est `python3` ou `ctest`. Le worker exige alors ces versions exactes :

- `numpy==2.2.6` ;
- `scipy==1.15.3` ;
- `scikit-learn==1.7.2` ;
- `hdbscan==0.8.44`.

Il exige aussi que `sklearn.cluster.HDBSCAN` et `hdbscan.validity` soient importables. À défaut, si `python3 -m pip` existe, il lance `pip install --user --only-binary=:all: --no-cache-dir` avec ces épingles ; des roues cp310 existent pour les quatre. L'installation va dans un dossier **propre à la session**, `PYTHONUSERBASE=$WORK/pyuser`, relevé dans `worker.txt` (`python_user_base`) : `~/.local` n'est jamais lu, et une installation interrompue lors d'une session précédente ne peut plus casser les suivantes. Les versions réellement importées sont écrites dans `worker.txt`, et `pip freeze` dans `env/`. Les dépendances transitives sont relevées, non épinglées. En cas d'échec, `remote_summary` donne aussi `worker_pip`, `worker_build` et `worker_data`.

`results_cap_bytes` (1 Mio à 1 Gio, 1 Gio par défaut) plafonne les résultats rapatriés :

- **stdout et stderr** de chaque étape sont tronqués au-delà de `min(64 Mio, plafond/4)` (tête et queue, avec un marqueur) ;
- au-delà du plafond, seuls les **gros** fichiers de `{out}` restent sur la VM, listés dans `overflow.txt` ; jamais les petits ;
- tout débordement ou toute troncature rend le worker `overflow`, et la session au mieux `failed_remote`, jamais `completed`. Le détail est donné dans `remote_summary`.

`--data` est un dossier plat de fichiers réguliers : `.u32le` de taille multiple de 4, jamais versionnés dans Git, sans « : » dans le chemin. Leurs sha256 sont vérifiés en local, sur la VM par la session, puis par le worker.

## Déroulé et garanties

1. **Préflight**, commune au dry-run, au lanceur et au processus de session. Elle vérifie :
   - que le commit est poussé, que le contrôleur exécuté est identique à celui du commit et que les scripts gardés correspondent à leurs épingles ;
   - que le plan, les données, le paquet et le budget sont valides ;
   - l'espace local, soit au moins deux fois le plafond des résultats plus 1 Gio, en plus de la réserve ;
   - en **lecture seule**, que la cible est TERMINATED, SPOT, action STOP, `g4-standard-48`, avec le label `project=e-hgp` et la métadonnée `enable-oslogin=TRUE`, et que son `maxRunDuration` égale `--max-run-seconds`.

   Le projet et le compte gcloud lus ici sont **figés** (`CLOUDSDK_CORE_PROJECT` et `CLOUDSDK_CORE_ACCOUNT`) pour toutes les commandes suivantes, scripts gardés compris.
2. **Préparation locale** :
   - une **réserve de 8 Mio** est préallouée dans le dossier de session ;
   - la clé ED25519 de session est créée en mode 600 ;
   - la clé est inscrite dans OS Login. C'est la première mutation, marquée avant l'appel pour qu'un ajout appliqué puis coupé par le délai soit quand même retiré. Son TTL vaut `ceil(M/60)+5` min, où M est `maxRunDuration`.
3. **Démarrage** : `start_and_verify.sh --guest-shutdown-minutes G`, avec `G = floor((M-900)/60)`, soit 45 min pour 3600 s. Il n'est **jamais signalé** pendant qu'il tourne : un signal reçu est différé jusqu'à sa fin, avec un délai externe de 2400 s. La session recertifie ensuite RUNNING sur notre génération, relit l'arrêt invité D, puis écrit `host/RECOVERY.txt`.
4. **Téléversement.** Le paquet, puis les données, partent dans `$HOME/ehgp-v10/ehgp-v10.XXXXXXXXXX` sur la VM, hors de `/tmp`. Des sessions précédentes, on élague les builds, sources, données, paquets Python, `overflow/`, et `results/` quand `results.tar.gz` existe ; un lien symbolique n'est jamais suivi. `results.tar.gz` et `worker.log` restent.
5. **Worker détaché.** Son identité est vérifiée par `/proc/PID/cmdline`. Le sondage passe par un SSH d'attente long. À chaque cycle :
   - la cible est **recertifiée**. Une relecture illisible (hoquet de l'API) est retentée avec un repli progressif (5, 10, 20, puis 30 s) pendant environ 3 min, jamais au-delà de D − 420 s, avant de conclure à une cible perdue ;
   - l'espace libre du disque de session et du dossier d'exécution est contrôlé. Sous 64 Mio (session) ou 32 Mio (dossier d'exécution) libres, la session ferme par anticipation, sans rapatrier.
6. **Échéances** :
   - le worker s'arrête au plus tard à D − 660 s ;
   - le sondage se termine à D − 600 s ;
   - le rapatriement est borné à 240 s, **recertifications comprises**, et en tout cas avant **D − 360 s**. `shutdown -P` crée en effet `/run/nologin` à D − 300 s, et pam_nologin refuse alors tout nouveau SSH.

   Aucun SSH ni SCP ne part au-delà de ces bornes.
7. **Chaque étape du worker** tourne dans son propre groupe de processus, sous `time -v` englobant `timeout --foreground`.
   - `time.txt` et la RSS sont renseignés même pour une commande coupée par son délai.
   - Le temps mural est pris à la sortie de la commande.
   - Le résidu du groupe reçoit SIGTERM puis SIGKILL. Si le groupe ne peut être certifié fermé, l'étape n'est pas « ok ».
   - Le statut du worker est calculé après le plafond des résultats.
8. **Rapatriement**, jamais après une interruption de l'hôte, une cible perdue ou un disque local presque plein : **l'arrêt passe d'abord**. La session relit la garde invitée et rapatrie `worker.log`. Si le worker n'a pas fini, elle lui envoie SIGTERM (identité vérifiée) avec une grâce bornée, sinon elle prend l'archive de secours. Elle contrôle la taille compressée et l'espace local, puis télécharge.
9. **Fermeture**, dans un `finally` dès la première mutation GCP. La génération vient **uniquement** du fichier de passage et du cycle de vie :
   - sans l'un ni l'autre, aucun démarrage n'a été demandé : **aucun arrêt** ;
   - un cycle de vie sans génération donne `shutdown_uncertified` : **aucun arrêt deviné**, reprise humaine.

   Sinon, les réserves sont libérées, puis un `describe` **en lecture seule** décide. Tant qu'il est illisible, il est retenté avec un repli progressif (5, 10, 20, puis 30 s) pendant environ 4 min :
   - TERMINATED avec notre génération G : arrêt déjà fait, certifié **sans stop** ;
   - RUNNING ou SUSPENDING avec G, **sans arrêt postérieur à G** (`lastStopTimestamp` absent ou antérieur) : `stop_and_verify.sh --yes --expected-last-start-timestamp G`, puis une relecture qui exige TERMINATED. Si elle ne le montre pas, un second cycle relecture → décision → stop est tenté, avec les mêmes règles ;
   - STOPPING avec G : même appel ; le script ne fait alors qu'attendre TERMINATED, sans nouvelle mutation ;
   - RUNNING ou SUSPENDING avec G, mais `lastStopTimestamp` postérieur à G : la VM a été arrêtée depuis notre démarrage, puis relancée par une autre session dont le `lastStartTimestamp` n'est pas encore matérialisé. **Rien n'est arrêté**, code 75 ;
   - STAGING, PROVISIONING ou tout autre état avec G : un démarrage étranger est en vol. **Rien n'est arrêté**, code 75 ;
   - une autre génération : notre génération est terminée. **Rien n'est arrêté**, code 75 (ou certifié si l'état est TERMINATED).

   Une fenêtre résiduelle de quelques secondes subsiste entre ce `describe` et les lectures internes de `stop_and_verify.sh`.
10. **Journaux.** Les sorties bavardes de toutes les commandes hôte, **scripts gardés compris**, vont dans le dossier d'exécution **jetable** sous `$TMPDIR`. Sur ce codespace, c'est un autre disque que `/workspaces`. Une copie miroir est faite au mieux dans `host/logs`.

    Un disque de session plein ne peut donc plus tuer `stop_and_verify.sh` sur son premier `printf` (autotest sur un vrai tmpfs plein). Si c'est le dossier d'exécution qui manque ou qui a moins de 16 Mio libres, les scripts gardés et les commandes critiques (relectures, arrêt, retrait de la clé) écrivent dans des **tubes** au lieu de fichiers. L'autotest le vérifie sur un vrai tmpfs plein, rempli de nouveau juste après la libération des réserves. Une réserve de 4 Mio y est aussi préallouée, puis libérée juste avant l'arrêt.

    Le reçu, `RECOVERY.txt`, `handoff.json`, `lifecycle.txt`, les marques et `DONE` restent dans le dossier de session, et la réserve de 8 Mio libérée juste avant l'arrêt leur laisse de la place. Si la session est quand même pleine, le reçu et `DONE` basculent dans le dossier d'exécution.

    **Un redémarrage du conteneur tue le processus de session** : ni reçu, ni `DONE`, ni arrêt. Lancer `--recover` dès le retour. D'ici là, seules la garde invitée (au plus 45 min après l'armement) et GCE (`maxRunDuration`) arrêtent la VM.
11. **Après la fermeture**, quel que soit son résultat : retrait de la clé OS Login, puis effacement de la clé privée. Viennent ensuite :
    - la **taille décompressée** de l'archive, lue dans l'index du tar, qui doit rester sous le plafond, avec deux fois cette taille libre plus 1 Gio en local ;
    - l'extraction sûre ;
    - la vérification de `MANIFEST.sha256` ;
    - le recoupement de la provenance : commit, plan, paquet, génération, noms des commandes, sha256 des binaires. Une archive de secours est rangée sous `unverified_*`.

`completed` exige un reçu sans erreur et aucun débordement. Les temps et versions de la VM (g++ 11.4, Python 3.10) ne sont **pas comparables** à ceux du codespace (g++ 13.3, Python 3.12) ; le reçu le dit.

## Reprise

- **Code 74, génération connue.** Suivre `host/RECOVERY.txt`, également gravé dans le reçu :
  1. `describe` en lecture seule, avec `lastStopTimestamp` ;
  2. **seulement** si l'état est RUNNING ou SUSPENDING avec cette génération **et** sans arrêt postérieur à elle (`lastStopTimestamp` absent ou antérieur), lancer la commande ci-dessous. Si l'état est STOPPING, attendre et relire.

     ```
     CLOUDSDK_CORE_PROJECT=devpod-gpu-exploration CLOUDSDK_CORE_ACCOUNT=<compte> GCP_PROJECT_ID=devpod-gpu-exploration GCP_ZONE=us-central1-b GCP_INSTANCE_NAME=ehgp-v7-4fa0e0789a7d5bb06b787d35 <session>/host/stop_and_verify.sh --yes --expected-last-start-timestamp <génération>
     ```

  Les variables `GCP_*` sont indispensables : sans elles, `stop_and_verify.sh` vise `ehgp-blackwell-spot`. `CLOUDSDK_CORE_*` figent le projet et le compte de la session, même si la configuration gcloud partagée a dérivé. L'étape 1 empêche d'arrêter un démarrage étranger qui porte encore notre génération : STAGING, ou RUNNING après un arrêt postérieur. Plus simple et vérifié : `--recover`.
- **`python3 gcp-migration/v10_session.py --recover --session-dir <session>`** :
  - rend 2, sans rien faire, si le dossier de session n'existe pas ;
  - prend le verrou `session.lock`, en l'attendant jusqu'à 60 s ;
  - attend ensuite jusqu'à 60 s de plus qu'aucun processus **capable de muter la VM** ne vive encore : le processus de session lui-même (PID de `launch.json`, ou `v10_session.py --child` sur ce dossier) ou un script gardé de la session (`host/start_and_verify.sh`, `host/stop_and_verify.sh`). Les chemins sont comparés après résolution des liens. Sinon, il **refuse avec 76** en les listant ;
  - ignore les simples observateurs (`tail -f`, boucle d'attente de `DONE`, SSH de sondage orphelin), qu'il liste dans `ignored_processes` ;
  - ferme ensuite exactement comme la session : génération gravée, `describe` préalable avec les mêmes règles, retrait de la clé ;
  - écrit `recovery_<date>.json` ;
  - rend 0 si l'arrêt est certifié, ou si aucun démarrage n'a pu avoir lieu (aucun processus vivant, aucun cycle de vie). Sinon, il rend 74 ou 75.
- **Code 70 du lanceur** (processus de session tué) et **redémarrage du conteneur** : ne rien relancer, mais lancer `--recover` dès que possible. Il vérifie lui-même qu'aucun processus de la session ne vit encore, et rend 76 tant que c'est le cas.
- **Génération inconnue** (rupture de stock, `start` en vol). Le reçu prescrit dans l'ordre :
  1. vérifier qu'aucun processus ne référence la session ;
  2. `describe`, puis `gcloud compute operations list` (lecture seule) ;
  3. relire quelques minutes plus tard ;
  4. ne **jamais** lancer `stop_and_verify.sh` sans `--expected-last-start-timestamp`.
- **Code 75.** Ne rien arrêter : la VM, ou le démarrage en vol, appartient à une autre session.

## Autotest

```bash
python3 gcp-migration/v10_selftest.py      # 47 scénarios, environ 20 min, code 0 attendu
```

Le montage :

- les **vrais** scripts gardés tournent contre un faux `gcloud` à état ;
- le faux gcloud émule aussi `/run/nologin` : tout SSH ou SCP est refusé à partir de D − 300 s ;
- la garde invitée réelle tourne avec un faux `shutdown` ;
- `ssh` et `scp` directs sont interdits, et `CLOUDSDK_CONFIG` est vide ;
- le mini-projet CMake est réellement construit ;
- les fixtures de disque plein montent un **vrai tmpfs plein** sur le dossier de session ou sur le dossier d'exécution, sous `unshare -Urm`, sans aucun privilège. Elles sont sautées, et le signalent, si les espaces de noms utilisateur manquent.

Fixtures issues des revues adverses :

- **Revue 1** :
  - T1, STAGING concurrent : jamais arrêté ;
  - console morte, suivie de SIGINT ou de SIGHUP (qui rend 74 si le stop échoue) ;
  - T3 (SIGTERM différé pendant le start) et T4 (préemption pendant le start, certifiée sans stop) ;
  - T5, génération étrangère : 75, aucun stop, au plus un SSH vers elle ;
  - T6, contrôleur tué pendant le start, puis `--recover` ;
  - rupture de stock, et `start` en échec alors que la VM tourne : 74 sans stop.
- **Revue 2** :
  - disque de session plein avant l'arrêt, puis **rempli de nouveau** juste après la libération de la réserve ;
  - génération périmée face à un STAGING étranger, pour la fermeture ordinaire comme pour `--recover` ;
  - `--recover` pendant un `start_and_verify.sh` orphelin : 76, puis arrêt une fois l'orphelin terminé ;
  - branche `overdue` sans aucun SSH après D − 360 s ;
  - `completed` refusé en cas de débordement ou de troncature ;
  - archive de secours à forte expansion refusée avant extraction ;
  - dérive de la configuration gcloud partagée ;
  - OS Login désactivé : refus ;
  - clé ajoutée puis délai dépassé : clé retirée ;
  - `DONE` impossible à écrire : le lanceur rend le vrai code.
- **Revue 3** :
  - RUNNING qui porte encore notre génération après un arrêt postérieur (`lastStopTimestamp`) : jamais arrêté, pour la fermeture ordinaire comme pour `--recover` ;
  - dossier d'exécution (`$TMPDIR`) plein, puis rempli de nouveau après la libération des réserves : arrêt certifié par tubes ;
  - `--recover` ignore les observateurs (`tail -f`, attente de `DONE`, SSH orphelin), mais refuse avec 76 tant que le processus de session vit ;
  - hoquet de l'API (`describe` JSON illisible six fois au sondage, quatre fois à la fermeture) : retenté, session conforme ;
  - `~/.local` cassé par un pip interrompu : sans effet ;
  - `DONE` périmé dans un dossier d'exécution réutilisé : ignoré ; `--recover` sur un dossier absent : 2.

S'y ajoutent :

- les branches abandon, archive de secours et worker mort sans `worker.exit` ;
- ctest vacant ou avec test désactivé ;
- commande qui ignore SIGTERM (résidu de groupe tué, `time.txt` présent, temps mural non gonflé) ;
- pip absent, installable ou déjà présent ;
- coupure SSH avant et après le lancement du worker ;
- les refus avant tout appel GCP.

## Limites

- **`maxRunDuration` n'est jamais reconfiguré.** L'allowlist de `set_max_run_duration_and_verify.sh` ne couvre que `europe-west4`. `--max-run-seconds` doit donc égaler la valeur déjà configurée, 3600 s lors des sessions v9. La fenêtre du worker est alors d'environ 29 min, construction et installation Python comprises.
- **Marges réduites de moitié par rapport à v9.** Le TTL OS Login de 65 min ne laisse que 300 s entre l'inscription et le contrôle de `start_and_verify.sh` (600 s en v9 ; 14 à 69 s mesurées sur les sessions v9). La garde de 45 min laisse 480 s d'armement (780 s en v9 ; 40 à 179 s mesurées).
- **Détection des processus par `/proc`.** `--recover` ne bloque que sur le processus de session et sur les scripts gardés de la session, reconnus par leur ligne de commande. Un enfant orphelin d'un script gardé (un `gcloud compute instances start` en vol) n'y est pas reconnu. Le verrou ne couvre que le processus de session.
- **Horloges.** Les bornes W, D − 600 s et D − 360 s sont calculées avec l'horloge du codespace, contre un D lu sur la VM ; le décalage n'est pas mesuré.
- **`lastStopTimestamp`.** La règle d'arrêt postérieur suppose que GCE le met à jour à chaque arrêt, préemption comprise, comme dans les reçus v9 ; l'autotest ne l'émule qu'ainsi.
- **Troncature des flux après coup.** Une commande qui écrit des dizaines de Go peut remplir le disque de la VM avant d'être tronquée (marge distante : 2 Gio).
- **Mesures du worker.** `max_rss_kb` couvre la commande et ses descendants attendus, pas les orphelins tués par la fermeture du groupe. Un descendant qui s'évade par `setsid` échappe à cette fermeture.
- **Une seule session v10 à la fois.** Le verrou `.ehgp-v10.lock` de la racine des sessions l'impose depuis ce codespace. Il n'exclut ni les autres protocoles (v9…) ni les autres machines ; `start_and_verify.sh` refuse cependant de démarrer une cible qui n'est pas TERMINATED.
- **Hypothèses sur la VM non vérifiées** : présence de `pip` et accès à PyPI, `KillUserProcesses=no` et `RemoveIPC` (logind). Le worker place `JOBLIB_TEMP_FOLDER` et `PYTHONUSERBASE` sous son dossier de travail.
- **Les fixtures sont locales.** L'autotest ne prouve ni GCE réel (arrêt accepté en STAGING, `lastStartTimestamp` périmé, `lastStopTimestamp`, `/run/nologin`), ni OS Login réel. La première session réelle reste à observer.
