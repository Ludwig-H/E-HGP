# Trois tentatives G4 terminal par lots — aucun calcul exécuté

Ce paquet conserve trois tentatives réelles, leurs erreurs et leurs arrêts
ciblés. Aucune n'a compilé ou exécuté la gate CUDA, ni lancé de benchmark.
Il ne qualifie donc aucun contrat 50k, aucun gain GPU, ni aucune tour GPU FULL.

| Tentative | Cible / génération | Résultat conservé |
| --- | --- | --- |
| US r1 | `us-central1-b / ehgp-v7-4fa0e0789a7d5bb06b787d35`, `2026-09-11T06:51:01.668-07:00` | Préemption signalée par le garde pendant le démarrage ; pas de worker. |
| US r2 | Même cible, `2026-09-11T06:53:44.713-07:00` | Préemption signalée par le garde pendant le démarrage ; pas de worker. |
| EU r1 | `europe-west4-a / ehgp-blackwell-spot`, `2026-09-11T06:59:41.712-07:00` | Deux gardes vérifiés, puis refus immédiat du worker : `existing tools required, no installation`. |

Le projet est `devpod-gpu-exploration`. Chaque reçu du contrôleur rapporte
`targeted_shutdown_certified=true`, lié à sa génération, au script d'arrêt
épinglé et à la sortie `TERMINATED`. Le cycle de vie EU reste historiquement
`targeted_running` : ce fichier décrit le démarrage et n'a pas été réécrit.
L'arrêt final est prouvé séparément par le processus stop et son reçu.

Le reçu invité EU conserve `commands=[]`, `runs=[]`, `binaries={}`. L'erreur
globale n'identifie pas lequel des outils NVCC, g++, nvidia-smi ou time manquait
ou n'était pas visible. Aucun diagnostic ultérieur ne doit être réattribué à
cette capture. Les 48 CPU ont été observés ; aucun GPU n'a été inventorié ou
utilisé par le worker. Le backend annoncé est son intention, pas une exécution.

La fenêtre du diagnostic cloud indépendant porte sur la première tentative US.
Elle contient un événement de préemption ; la deuxième préemption est attestée
ici par le diagnostic du script de démarrage, pas par un second log indépendant.
La note `PRE_START_FAILURE.md` conserve aussi un refus local antérieur pour le
mode de la clé ; elle indique elle-même l'absence de capture séparée du processus.

## Sources et confidentialité

Le worker reste `043197e4`, le contrôleur `177b25a0`, start `73d76c67` et stop
`ddcad77a`. Le snapshot source `15f0abbd` et son manifeste `803f1fcf` sont liés
au paquet séparé [terminal_batch_worker_20260911](../terminal_batch_worker_20260911/README.md),
manifeste `8758776c…bf16b2`, sans recopier trois fois son tar.

Le wrapper EU `3ec16f9b` ne change que la cible globale du contrôleur original.
Sa qualification pure normal/-O (13 contrôles, 13 refus), le mode inerte et les
sources exactes sont conservés. Le binding runtime a les mêmes valeurs JSON que
le binding qualifié, mais une présentation différente : les deux originaux sont
préservés, sans prétendre qu'ils sont identiques octet par octet.

Aucune clé privée/publique n'est incluse. Les trois profils OSLogin, les trois
descriptions riches d'instance avec métadonnées SSH et les trois sorties start
contenant le compte personnel sont omis avec SHA et taille. Pour ces dernières,
une copie dérivée masque explicitement la seule ligne `compte`. Les descriptions
sont remplacées par des projections déclarées des seuls champs de sûreté,
liées au SHA de leur original conservé localement. Ces projections ne sont pas
des flux bruts et leur transformation n'est pas revalidable sans les originaux
confidentiels. Les erreurs, reçus, générations et preuves invitées sont inchangés.

La clôture complémentaire lie les trois reçus, atteste la révocation de la même
clé OSLogin puis la suppression ciblée des seuls fichiers privés/publics de
session et du dossier devenu vide. Son inventaire final montre les trois VM
labellisées `TERMINATED`, sans mutation d'autres VM. Le lecteur vérifie ce reçu
historique ; il ne contacte pas le cloud et ne relit aucune clé.

## Lecture portable

```bash
python3 -B verify.py
python3 -B -O verify.py
python3 -B verify.py --extract /tmp/mhgp7-terminal-g4-attempts-fresh
```

L'extraction refuse un répertoire existant et ne restaure que les fichiers
publiables. Aucun fichier omis n'est reconstruit. Les noms logiques et hashes
sont dans `logical_manifest.json`, la déduplication physique dans
`storage_map.json`, les omissions et dérivés dans `provenance/`. Les Markdown
historiques sont stockés comme sources opaques. Aucun ELF, vendor ou secret
n'est distribué ; aucun script cloud du paquet n'est exécuté par le lecteur.
