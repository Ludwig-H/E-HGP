# Session G4 v12 : mode d'emploi

`backend=reference_cpu`, `public_status=not_claimed` : une mesure prise ici ne certifie rien.

`v12_session.py` et `v12_worker.sh` sont un **port explicite** du protocole gardé de la v11 ([`README_V11.md`](README_V11.md), main `f31845d16`), lui-même port de celui de la v10 ([`README_V10.md`](README_V10.md) : déroulé, garanties, échéances, limites). Le cycle de vie est **inchangé** : clé OS Login à durée bornée, démarrage par `start_and_verify.sh` jamais signalé, recertification de la cible, téléversement vérifié, worker détaché, échéances avant `/run/nologin`, rapatriement borné, fermeture par la génération prouvée, reprise `--recover`, codes 0 / 2 / 3 / 74 / 75 / 76 / 70. Les deux scripts gardés sont les mêmes fichiers, aux mêmes épingles sha256. Les deux modes de source (`--commit`, `--snapshot`), le plan (`python_packages`, `default_build`, cibles contrôlées par leur forme), les faits de la VM et la cible explicite `--zone` / `--instance` sont ceux de la v11.

Ce qui change, et rien d'autre :

- la lignée : sources `morsehgp3D_v12/`, exécutables `./mhgp12` et `./mhgp12_*`, commandes `python3 {src}/morsehgp3D_v12/<script>.py`, dossier distant `$HOME/ehgp-v12/`, schémas `ehgp.v12.*`, variables `V12_*` et `MHGP12_DATA_DIR`, sessions nommées `v12.<date>.<objet>` ;
- le verrou commun exclut désormais **trois** lignées : une session v10, v11 ou v12 refuse les deux autres (même fichier `.ehgp-v10.lock`) ;
- **reçus sans chemin personnel** : `receipt.json` note le `$HOME` de la VM par le texte littéral `$HOME`, et le worker fait de même dans les relevés qu'il écrit lui-même ; un examen d'identité liste les fichiers rapatriés qui portent encore une identité (voir plus bas) ;
- **cache de données persistant** sur la VM : le worker exporte `MHGP12_CACHE_DIR` (`$HOME/ehgp-v12/cache`), géré par le script du paquet [`morsehgp3D_v12/bench/data_cache.py`](../morsehgp3D_v12/bench/data_cache.py).

## Lancer : une seule façon

```bash
# 1. valider sans rien lancer (lectures GCP seulement)
python3 gcp-migration/v12_session.py --snapshot "$PWD" --plan plan.json --data /chemin/des/donnees \
  --session-dir /workspaces/.ehgp-sessions/v12.<date>.<objet> --max-run-seconds 3600 \
  --zone us-central1-c --instance ehgp-v7-3b1d496aed430749ea7e049f
# 2. lancer : la même commande, plus --execute (rend la main aussitôt) ; plus --wait pour attendre la fin
python3 gcp-migration/v12_session.py ... --execute --wait
```

Sans `--zone` / `--instance`, la cible reste celle des lignées précédentes (`ehgp-v7-4fa0e0789a7d5bb06b787d35` en `us-central1-b`) ; la VM G4 actuelle de la v12 ([`PROVENANCE.md`](../morsehgp3D_v12/docs/PROVENANCE.md), § 5) est la paire de la zone c ci-dessus, à donner explicitement. La fin est signalée par la sentinelle `DONE` du dossier de session, qui contient le code de sortie ; le reçu est `receipt.json`. Arrêter le lanceur n'arrête pas la session. **Arrêter un workflow ou redémarrer le conteneur tue le processus de session sans arrêter la VM** : lancer `--recover` aussitôt.

## Plan

Identique à la v11, au schéma près :

```json
{
  "schema": "ehgp.v12.session_plan.v1",
  "default_build": true,
  "python_packages": "none",
  "results_cap_bytes": 67108864,
  "commands": [
    {"name": "jeux", "timeout_seconds": 1800,
     "argv": ["python3", "{src}/morsehgp3D_v12/bench/data_cache.py", "--manifest",
              "{src}/morsehgp3D_v12/bench/jeux_publics.json", "--get", "scene_a.bin", "scene_b.bin",
              "--link", "{build}/datasets", "--report", "{out}/data_cache.json"]},
    {"name": "tour", "timeout_seconds": 900, "argv": ["./mhgp12", "datasets/scene_a.bin", "{out}/tour"]}
  ]
}
```

`argv[0]` : `./mhgp12`, `./mhgp12_*`, `python3 {src}/morsehgp3D_v12/<script du paquet>.py`, ou `ctest --no-tests=error ...` — exactement les formes de la v11. Les commandes reçoivent `MHGP12_DATA_DIR` (égal à `{data}`) et `MHGP12_CACHE_DIR`. La matrice de configurations de la v12 n'existe pas encore (`morsehgp3D_v12/tools/g4_matrix.py`, à porter en T0) ; le scénario de l'autotest qui la lance se déclare sauté tant qu'elle manque.

## Données volumineuses

**Par `--data` (inchangé).** Un dossier plat de 512 fichiers et 8 Gio au plus, haché localement à chaque validation (lanceur puis processus de session), téléversé par un seul `gcloud compute scp` à chaque session, revérifié deux fois sur la VM, puis **élagué au début de la session suivante** (`rm -rf` de `data/`, `build/`, `src/`… des anciens `ehgp-v12.*`). Le budget exige un téléversement prudent (2 Mio/s) sous la moitié de la fenêtre utile : avec la construction par défaut, `--max-run-seconds` vaut au moins 46 min sans données, 51 min pour 1 Gio, 1 h 48 pour 4 Gio et 3 h 03 pour 8 Gio, et tout ce téléversement se paie en temps de G4 à chaque session. Les données ne sont pas copiées dans le dossier de session, mais elles doivent exister sur le codespace : `/workspaces` n'a qu'environ 2 Go libres et `/tmp` est vidé à chaque redémarrage. Ce chemin reste celui des trames dérivées et des petits jeux (moins de quelques centaines de Mo).

**Par le cache de la VM (nouveau).** Pour les jeux publics de plusieurs centaines de Mo à quelques Go, une commande du plan les télécharge **directement sur la VM**, une fois pour toutes :

- manifeste versionné dans le paquet (`ehgp.v12.public_datasets.v1`) : `name`, `url` (http ou https, sans identifiants), `sha256` et `size` épinglés ensemble, `license` obligatoire, `note` ;
- `--get` exige l'épingle avant toute requête ; un objet présent est revérifié à chaque usage ; rien n'entre sans sha256 et taille exacts ; le cache est adressé par le contenu (`objects/<sha256>`, lecture seule) ;
- `--probe` lit l'épingle d'une entrée nouvelle (sha256 et taille rapportés) ; l'objet sert sous son nom seulement une fois l'épingle commise au manifeste ;
- reprise à l'octet près (`Range` + `If-Range`) d'un téléchargement coupé par le délai de la commande ou une préemption ; serveur sans `Range` : reprise depuis zéro ; seul le sha256 final fait foi ;
- place : plancher libre (`--min-free-bytes`, 16 Gio par défaut) et plafond du cache (`--max-cache-bytes`, 40 Gio par défaut, sur un disque d'environ 97 Gio dont 80 libres) ; éviction des moins récemment utilisés non demandés, et refus sans rien évincer quand évincer tout ne suffirait pas ;
- `--link {build}/datasets` crée des liens durs : les commandes suivantes lisent `datasets/<nom>` depuis leur dossier courant (`{build}`). Un dossier de liens dans les résultats rapatriés (`{out}`) est refusé ;
- rapport JSON dans `{out}` sans chemin absolu ni identité ; `--status`, `--trim` et `--purge` entretiennent le cache par une commande du plan ;
- codes : 0, 1 (une entrée en échec ou interruption), 2 (usage ou manifeste), 3 (cache inutilisable).

Le cache garde les octets publiés tels quels. La conversion vers l'entrée du moteur reste à écrire, comme un script
du paquet qui lit `datasets/<nom>` et écrit dans `{build}` : LAS ou PLY non compressés se décodent en Python nu, LAZ
exige un décodeur (laszip) absent de la VM.

Le cache vit dans `$HOME/ehgp-v12/cache`, frère des dossiers de session : l'élagage du contrôleur ne vise que `$HOME/ehgp-v12/ehgp-v12.*` et ne le touche jamais. Le protocole n'admet aucune commande nouvelle : `data_cache.py` est un script du paquet, comme les autres. Licences : le cache est sur la VM, jamais dans le dépôt ; le manifeste ne contient que des URL, des épingles et des licences.

## Reçus sans identité

Sur la VM, `$HOME` vaut `/home/<nom POSIX dérivé de l'adresse du compte OS Login>`. La v11 l'écrivait dans `receipt.json` (`remote_directory`, `argv` des commandes hôte, `python_user_base`) et dans les relevés du worker. La v12 :

- écrit `receipt.json` avec `$HOME` à la place de ce chemin (rédaction au moment de l'écriture ; l'état en mémoire, les journaux d'hôte et les commandes exécutées restent exacts) ;
- fait écrire au worker `$HOME` dans `argv.txt`, `env/*.txt` et `worker.txt`, et `$USER` dans la sortie de `id` ;
- ajoute `results_identity_scan` au reçu : les fichiers rapatriés qui contiennent encore l'adresse du compte ou le chemin personnel (sorties de `cmake`, `ctest`, de vos commandes…), en chemins relatifs, **jamais les valeurs**. Ces fichiers n'entrent pas tels quels dans un reçu publié.

**Ce qui reste, volontairement.** L'adresse du compte gcloud figure encore dans `preflight.json` (`gcloud_account`), `host/RECOVERY.txt`, `recovery_command` et `recovery` de `receipt.json` : elle fige le compte de l'arrêt et de la reprise (revue adverse 2, dérive de la configuration gcloud partagée). La retirer touche la garde de reprise ; c'est une proposition à relire (rapport de port, `v12_session/RAPPORT.md`), pas un changement fait ici.

**Ce qu'un reçu publié contient.** `receipt.json` (après avoir masqué `recovery_command` et `recovery`, qui portent l'adresse du compte), les fichiers rapatriés que `results_identity_scan` ne liste pas, et les empreintes du paquet (`package_sha256`, manifeste d'instantané). **Jamais** : `preflight.json` brut (`gcloud_account`), `host/` (journaux des scripts gardés, dont `start_and_verify.sh` qui imprime le compte, et la réponse d'OS Login), `RECOVERY.txt`, `session.stdout` / `session.stderr`, `package/package.tar.gz` (copie de sources), ni `results.tar.gz` brut.

## Verrou partagé avec la v10 et la v11

Une seule VM peut être active à la fois. Le verrou des sessions v12 est donc **le même fichier** que celui de la v10 et de la v11, `<racine des sessions>/.ehgp-v10.lock`, quelle que soit la zone choisie. Une seconde session est refusée avec le code 2 avant tout appel GCP, de quelque lignée qu'elle soit. La reprise prend également ce verrou et rend 76 s'il est tenu. Le créateur v7 ne le prend pas lui-même : son opérateur doit le tenir pendant toute création et clôture. Ce verrou local n'exclut ni les protocoles antérieurs à la v10, ni une autre machine ; l'inventaire et les gardes cloud restent indispensables.

## Reprise

```bash
python3 gcp-migration/v12_session.py --recover --session-dir /workspaces/.ehgp-sessions/v12.<date>.<objet>
```

Mêmes règles que la v11 (cible retrouvée dans les traces concordantes, jamais devinée ; 76 tant que le processus de session ou un script gardé vit ; 0, 74 ou 75 sinon). Une session se reprend avec le contrôleur de sa lignée. La reprise ne rapatrie toujours rien : les résultats restent sur la VM dans `$HOME/ehgp-v12/ehgp-v12.*` (`results.tar.gz`, jamais élagué, ou `results/` à défaut d'archive), mais `overflow/` disparaît à la session suivante (une reprise qui rapatrie est prévue au plan, T0).

## Autotest

```bash
python3 gcp-migration/v12_selftest.py           # hors ligne, 60 scénarios, environ 20 min, code 0 attendu
python3 -B gcp-migration/v12_target_selftest.py
python3 -B -O gcp-migration/v12_target_selftest.py
```

Il reprend tous les scénarios de l'autotest v11 et ajoute : l'exclusion mutuelle des trois lignées (les **vrais** `v10_session.py` et `v11_session.py` contre le même faux cloud), le reçu sans chemin personnel et l'examen d'identité, et le cache de données (script seul sur un serveur HTTP de boucle locale `127.0.0.1` : épingle, reprise, serveur sans `Range`, contenu refusé, sondage, liens, éviction, plancher, interruption, refus ; puis deux sessions complètes qui le partagent sans retéléchargement). L'autotest de cible ajoute les fonctions pures de la rédaction et de l'examen. Comme pour la v11, il ne prouve ni GCE réel ni OS Login réel, ni le réseau de la VM.
