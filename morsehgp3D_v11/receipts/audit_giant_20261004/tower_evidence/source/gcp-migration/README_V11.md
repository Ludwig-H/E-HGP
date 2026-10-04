# Session G4 v11 : mode d'emploi

`backend=reference_cpu`, `public_status=not_claimed` : une mesure prise ici ne certifie rien.

`v11_session.py` et `v11_worker.sh` sont un **port explicite** du protocole gardé de la v10 ([`README_V10.md`](README_V10.md) : déroulé, garanties, échéances, limites). Le cycle de vie est **inchangé** : clé OS Login à durée bornée, démarrage par `start_and_verify.sh` jamais signalé, recertification de la cible, téléversement vérifié, worker détaché, échéances avant `/run/nologin`, rapatriement borné, fermeture par la génération prouvée, reprise `--recover`, codes 0 / 2 / 3 / 74 / 75 / 76 / 70. Les deux scripts gardés sont les mêmes fichiers, aux mêmes épingles sha256.

Ce qui change, et rien d'autre :

- la lignée : sources `morsehgp3D_v11/`, exécutables `./mhgp11` et `./mhgp11_*`, dossier distant `$HOME/ehgp-v11/`, schémas `ehgp.v11.*`, sessions nommées `v11.<date>.<objet>` ;
- un second mode de source, `--snapshot`, pour les essais de développement ;
- le plan : `python_packages`, `default_build`, cibles contrôlées par leur forme ;
- les faits de la VM relevés par le worker, et `MHGP11_DATA_DIR` exporté vers les commandes.
- la cible peut être donnée par la paire explicite `--zone` / `--instance`, dans le projet fixe.

## Lancer : une seule façon

```bash
# 1. valider sans rien lancer (lectures GCP seulement)
python3 gcp-migration/v11_session.py --snapshot "$PWD" --plan plan.json --data /chemin/des/donnees \
  --session-dir /workspaces/.ehgp-sessions/v11.<date>.<objet> --max-run-seconds 3600
# 2. lancer : la même commande, plus --execute (rend la main aussitôt) ; plus --wait pour attendre la fin
python3 gcp-migration/v11_session.py ... --execute --wait
```

La fin est signalée par la sentinelle `DONE` du dossier de session, qui contient le code de sortie ; le reçu est `receipt.json`. Arrêter le lanceur n'arrête pas la session. **Arrêter un workflow ou redémarrer le conteneur tue le processus de session sans arrêter la VM** : lancer `--recover` aussitôt.

## Nouvelle cible gardée

Sans option, la cible historique reste `ehgp-v7-4fa0e0789a7d5bb06b787d35`
en `us-central1-b`. Pour une VM créée séparément par le
[créateur gardé](README_v7_CREATION.md), donner **les deux** options,
par exemple `--zone us-central1-c --instance ehgp-v7-3b1d496aed430749ea7e049f`.
Le projet reste `devpod-gpu-exploration`. Les variables d'environnement ne
remplacent pas cette paire : le contrôleur fige ensuite la cible exacte dans
les transports, l'enfant détaché, le préflight, le lancement et le reçu.
Nom et zone sont validés avant tout appel cloud. Les exigences G4/SPOT,
label `project=e-hgp`, OS Login, STOP, durée et génération sont inchangées.

La création rend une VM arrêtée ; elle ne qualifie pas son outillage. Sur une
image neuve, le worker relève les outils puis les vraies configurations
échouent explicitement si un prérequis manque. Il n'installe aucun paquet
système. Le lot CPU v11 exige C++20/g++, Make, CMake/CTest ≥3.20, Python ≥3.10,
GNU time, les outils shell usuels et les runtimes ASan/UBSan/TSan. Clang reste
optionnel dans la matrice. Avec `python_packages=none`, pip, HDBSCAN, Boost et
CUDA ne sont pas requis. Tout outillage manquant se prépare ensuite dans une
session gardée distincte ; aucune installation implicite n'est ajoutée ici.
Les versions effectives viennent des captures, jamais de celles de la VM précédente.

Le contrôleur exécuté doit toujours être identique à celui du commit demandé.
Un raccord de cible doit donc être commis et poussé avant qualification. On
peut conserver les octets produit d'une version antérieure dans ce nouveau
commit ; son SHA nouveau doit alors être déclaré, sans exception de provenance.

## Deux modes de source

| | `--commit SHA` | `--snapshot RACINE` |
|---|---|---|
| Paquet | `git archive` du commit : `morsehgp3D_v11/` et le worker | fichiers réguliers de `RACINE/morsehgp3D_v11`, suivis ou non, et le worker de l'arbre de travail |
| Exigences | commit poussé sur `origin/main` ; contrôleur exécuté identique à celui du commit | `RACINE` est l'arbre de travail du contrôleur exécuté ; aucun `git fetch` |
| Scripts gardés | égaux à leurs épingles | égaux à leurs épingles |
| Reçu | `source_kind = commit`, `evidence_grade = pushed_commit` | `source_kind = worktree_snapshot`, `evidence_grade = dev_snapshot`, `commit = null` |

L'instantané exclut les dossiers `__pycache__` et `build*` (jamais parcourus), les fichiers `*.pyc` et les fichiers de plus de 32 Mio. Il **refuse** les liens symboliques, les fichiers spéciaux, les noms non imprimables et tout fichier contenant une clé privée. Il est borné à 20 000 fichiers, 1 Gio non compressé et 256 Mio compressé. Ne nommez donc aucun dossier de sources `build*`, et gardez les données hors de `morsehgp3D_v11/` : tout ce qui s'y trouve part sur la VM.

Les archives de commit respectent aussi `morsehgp3D_v11/.gitattributes`.
Les copies des anciens paquets source sous
`receipts/audit_independant_20261002/**/raw/package/package.tar.gz` sont
marquées `export-ignore` : elles restent dans Git et dans les captures
historiques, mais ne sont plus réembarquées dans chaque nouveau paquet
natif. Aucun source, oracle, fixture ni résultat de qualification n'est
exclu par cette règle. Le SHA du nouveau paquet demeure celui réellement
téléversé. Le mode instantané suit sa liste d'exclusions propre ci-dessus,
sans transfert implicite de cette règle Git.

La préflight et le reçu d'un instantané disent : le sha256 du paquet, le manifeste (chemin, taille, sha256 et mode de chaque fichier), la liste des exclusions, le commit `HEAD`, l'empreinte de `git status --porcelain` et les sha256 du contrôleur, du worker et des scripts gardés. Le paquet est conservé dans `package/package.tar.gz`, avec `package/SNAPSHOT_MANIFEST.sha256`. Chaque fichier est lu une seule fois, et le manifeste est calculé sur les octets envoyés ; le même arbre donne le même paquet.

**Ce que vaut un reçu d'instantané.** Il prouve que les commandes ont tourné sur la VM avec exactement le paquet décrit par le manifeste, et que l'arrêt est certifié. **Ce qu'il ne vaut pas** : ce n'est pas une preuve publiable. L'arbre n'est rattaché à aucun commit, il n'est pas figé pendant la lecture (d'autres écritures peuvent s'y mêler), et personne ne peut le reconstruire depuis le dépôt. Tout résultat destiné à un audit ou à un reçu se refait en mode `--commit`.

## Plan

```json
{
  "schema": "ehgp.v11.session_plan.v1",
  "default_build": false,
  "python_packages": "none",
  "results_cap_bytes": 67108864,
  "commands": [
    {"name": "matrice", "timeout_seconds": 1500,
     "argv": ["python3", "{src}/morsehgp3D_v11/tools/g4_matrix.py", "--src", "{src}", "--out", "{out}",
              "--data", "{data}", "--work", "{build}/matrix"]}
  ]
}
```

Par rapport à la v10 :

- `python_packages` : `"none"` par défaut. Aucun contrôle pip ; `python3` et `ctest` tournent avec le Python 3.10 nu de la VM. `"pinned"` rétablit le comportement de la v10 (numpy 2.2.6, scipy 1.15.3, scikit-learn 1.7.2, hdbscan 0.8.44 exigés, installés par pip à défaut). La clé `needs_python` n'existe plus, et rien n'est déduit des commandes : la v10 rendait `failed_remote` dès que pip manquait, même quand toutes les commandes avaient réussi.
- `default_build` : `true` par défaut, construction Release du CMake de `morsehgp3D_v11` avant les commandes. Avec `false`, rien n'est construit, `{build}` est un dossier vide, et seules des commandes `python3` sont admises : la matrice construit elle-même.
- `build_targets` reste facultatif. Les cibles et les binaires ne sont plus cherchés dans le CMake du commit ; seule leur forme est contrôlée (`mhgp11` ou `mhgp11_*`). Un binaire inexistant échoue donc sur la VM, non à la validation.
- `argv[0]` : `./mhgp11`, `./mhgp11_*`, `python3 {src}/morsehgp3D_v11/<script du paquet>.py`, ou `ctest --no-tests=error ...`.

Les commandes reçoivent la variable `MHGP11_DATA_DIR`, égale à `{data}`. Le reçu recopie `vm_facts` : versions de g++, clang++ (ou `absent`), cmake, ctest et python3, nombre de fils, mémoire, drapeaux `avx2`, `avx512f`, `avx512dq`, `avx512vl` et `bmi2`, et l'état des bibliothèques de sanitizers (`ok`, `ok_setarch`, `run_failed`, `compile_failed`). `ok_setarch` signifie qu'un binaire ne s'exécute de façon fiable que sous `setarch -R`.

La matrice des configurations s'écrit dans [`g4_matrix.json`](../morsehgp3D_v11/tools/g4_matrix.json) et s'exécute par [`g4_matrix.py`](../morsehgp3D_v11/tools/g4_matrix.py), qui écrit `{out}/matrix/summary.json` ; après la session, ce résumé se trouve dans `results/extracted/results/cmd/000_matrice/files/matrix/`. Une matrice non conforme rend un code non nul : la session finit alors en `failed_remote` (code 3), résultats rapatriés et arrêt certifié. C'est le signal attendu d'une porte en échec, pas une panne du protocole ; `remote_summary.failed_commands` et `summary.json` disent laquelle.

## Verrou partagé avec la v10

Une seule VM peut être active à la fois. Le verrou des sessions v11 est donc
**le même fichier** que celui de la v10, `<racine des sessions>/.ehgp-v10.lock`,
quelle que soit la zone choisie. Une seconde session est refusée avec le code 2
avant tout appel GCP. La reprise prend également ce verrou et rend 76 s'il
est tenu. Le créateur v7 ne le prend pas lui-même : son opérateur doit le
tenir pendant toute création et clôture. Ce verrou local n'exclut ni les
protocoles antérieurs, ni une autre machine ; l'inventaire et les gardes
cloud restent indispensables.

## Reprise

```bash
python3 gcp-migration/v11_session.py --recover --session-dir /workspaces/.ehgp-sessions/v11.<date>.<objet>
```

Mêmes règles que la v10, quel que soit le mode de source. La reprise rend 76 tant que le processus de session ou un script gardé de la session vit encore. Sinon elle ferme par la génération gravée, après un `describe` en lecture seule, et n'arrête jamais une VM qui n'est pas prouvée la nôtre. Elle rend 0 si l'arrêt est certifié ou si aucun démarrage n'a pu avoir lieu, 74 ou 75 sinon. Une session se reprend avec le contrôleur de sa lignée.

La reprise retrouve la cible dans les traces concordantes de cette session
(préflight, lancement, reçu, handoff et cycle de vie). Elle ne remplace pas
une cible manquante par la cible actuelle pour autoriser un arrêt. Un journal
tronqué peut être suppléé par les preuves gardées ; une contradiction lisible
refuse la fermeture avant tout appel cloud. Une paire CLI explicite doit
être identique à la cible retrouvée. L'ancien lancement sans champ cible
reste lisible, et l'absence de tout script de démarrage conserve le cas
`no_start_requested` sans action cloud.

## Autotest

```bash
python3 gcp-migration/v11_selftest.py      # hors ligne, 58 scénarios, 20 à 25 min, code 0 attendu
python3 -B gcp-migration/v11_target_selftest.py
python3 -B -O gcp-migration/v11_target_selftest.py
```

Il reprend tous les scénarios de l'autotest v10 et ajoute : l'instantané (manifeste exact, exclusions, refus des liens, des fichiers spéciaux et des clés, paquet reproductible, session complète qui conserve le paquet), `python_packages`, `default_build`, les cibles contrôlées par leur forme, les faits de la VM, l'exclusion mutuelle avec le vrai `v10_session.py`, et la matrice lancée par une commande du plan (conforme, non conforme, délai atteint pendant une porte). Comme pour la v10, il ne prouve ni GCE réel ni OS Login réel.

Le petit autotest de cible remplace entièrement cloud et worker par des
doubles Python : aucune compilation ni commande cloud. Il couvre paire CLI,
transport à l'enfant, gardes de cible, refus des traces contradictoires,
reprise de la zone c depuis le défaut b, exclusion par le verrou commun et
conservation du contrôle d'identité contrôleur/commit et des épingles.
