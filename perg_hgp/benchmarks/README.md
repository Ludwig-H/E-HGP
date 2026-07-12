# Benchmarks 3D reproductibles de PowerCover3D

Ce dossier est volontairement indépendant de l'API installable. Il fournit :

- des nuages 3D synthétiques déterministes écrits en `.npy` `float32` par
  blocs, donc utilisables en lecture `mmap` jusqu'à 30 millions de points ;
- un orchestrateur qui lance **chaque méthode dans un nouveau processus** ;
- PowerCover3D CPU/CUDA, HDBSCAN et HGP-old lorsque leurs dépendances sont
  importables ;
- les temps PowerCover par étage et le temps mur, le pic RSS échantillonné et
  le pic VRAM du processus lorsque NVML (`nvidia-ml-py`) est disponible ;
- les rapports de précision, enveloppes, audits de voisins et certifications
  publiés par PowerCover ;
- des JSON stricts publiés atomiquement, un résumé JSON et un CSV.

Les commandes ci-dessous partent de la racine du dépôt. L'équivalent
`cd perg_hgp && python -m benchmarks ...` est aussi disponible.

## Aide et génération seule

```bash
python perg_hgp/benchmarks/cli.py --help
python perg_hgp/benchmarks/cli.py run --help

python perg_hgp/benchmarks/cli.py generate \
  --datasets anisotropic_blobs,two_density_corridor,lidar_surfaces,uniform,mixture \
  --n 10000 --seed 20260712 \
  --output /tmp/perg-hgp-data
```

Les générateurs sont :

- `anisotropic_blobs` : quatre Gaussiennes tournées/an-isotropes et du bruit ;
- `two_density_corridor` : deux amas de densités différentes reliés par un
  corridor fin déclaré ambigu/bruit ;
- `lidar_surfaces` : sol incliné, surfaces de poteaux, objets de type véhicule
  et retours aberrants ;
- `uniform` : cube homogène sans vérité terrain, utile pour le débit ;
- `mixture` : huit composantes de tailles/densités différentes et du bruit,
  conçue pour monter à 30 M.

Les composantes occupent des blocs de lignes contigus. Ce choix évite une
permutation `int64` supplémentaire de 240 Mo à 30 M. Pour un même nom, `n` et
seed, les octets produits sont indépendants de `--dataset-chunk-size`.

## Smoke test comparatif

```bash
python perg_hgp/benchmarks/cli.py run \
  --methods powercover-cpu,hdbscan,hgp-old \
  --datasets anisotropic_blobs,two_density_corridor,lidar_surfaces \
  --n 5000 --seed 20260712 --k 1-3 \
  --entropy fixed-hard,local-distortion --grid 24 \
  --min-cluster-size 30 --cut-count 9 --max-n-cut-scan 5000 \
  --output perg_hgp/benchmarks/results/smoke-3d \
  --allow-errors
```

`--allow-errors` est pratique lorsque HGP-old/CGAL n'est pas compilé : son
échec est enregistré sans masquer les autres résultats. Sans cette option, la
CLI finit avec un code non nul dès qu'un worker échoue, tout en conservant les
JSON déjà produits. `--resume` est actif par défaut et permet de reprendre une
session interrompue.

## Matrice CUDA G4 et 30 millions de points

Commencer par des paliers (100 k, 1 M, 10 M), puis seulement 30 M :

```bash
python perg_hgp/benchmarks/cli.py run \
  --methods powercover-cuda \
  --datasets mixture,uniform \
  --n 100000,1000000,10000000,30000000 \
  --seed 20260712 --k 1-10 \
  --entropy fixed-hard,local-distortion \
  --distortion-budget-relative 0.10 \
  --grid 96,128 \
  --m-reg 64 --chunk-size 262144 --power-chunk-size 65536 \
  --max-n-cut-scan 0 --no-save-labels \
  --output perg_hgp/benchmarks/results/g4-scaling-3d \
  --keep-going
```

Cette commande est une matrice complète : chaque triplet `(K, entropie,
grille)` refait un ajustement dans un processus propre. Pour un premier passage
30 M moins coûteux, limiter à `--k 10 --grid 128`, puis élargir la matrice une
fois le cas nominal validé.

Le mode `fixed-hard` impose par défaut `kappa=1`. Le mode recommandé
`local-distortion` utilise un budget relatif local et `local_scale_k=K` sauf
option contraire. La forme de grille peut être isotrope (`128`) ou explicite
(`160x128x96`). Quand `--min-resolved-radius` est fourni, la politique de grille
locale du backend prévaut sur la forme nominale ; ce choix figure dans le
rapport.

Les limites par défaut évitent de lancer accidentellement les références
coûteuses à 30 M : CPU PowerCover 100 k, HDBSCAN 2 M et HGP-old 100 k. Elles
sont ajustables avec `--max-n-*`; `0` signifie illimité. PowerCover CUDA est
illimité par défaut mais applique encore son estimation de VRAM et
`--max-vram-fraction`.

Sur la VM, installer `nvidia-ml-py` pour obtenir le pic VRAM par PID. Le rapport
conserve séparément la mémoire GPU totale du périphérique, qui peut inclure
d'autres processus. Les instructions de `gcp-migration` restent la source de
vérité pour démarrer la G4. Toujours arrêter la VM à la fin des calculs.

## Coupes, labels et ARI

Pour les petits jeux seulement (`n <= --max-n-cut-scan`), le worker extrait des
rayons candidats parmi les quantiles des naissances de sommets **et** des poids
d'arêtes des forêts base/inner/outer, puis appelle `hierarchy.labels_at_cut`.
Il enregistre chaque temps d'affectation, chaque ARI et éventuellement les
meilleurs labels.

Le champ canonique `sampled_oracle_best_ARI` est explicitement un **oracle
utilisant la vérité terrain sur un échantillon fini de coupes**. Il sert à
diagnostiquer si la hiérarchie contient une bonne coupe ; ce n'est ni une
sélection non supervisée, ni un score de production. Le champ historique
`oracle_best_ARI` reste présent comme alias de compatibilité et les JSON publient
la table `compatibility_aliases`. Ils portent aussi `production_selection:
false` et cet avertissement. Le chemin `labels_at_cut`, actuellement CPU et
borné, est désactivé pour les essais 30 M.

HDBSCAN et HGP-old sauvegardent leurs labels `int32`, leur SHA-256, l'ARI
standard (bruit `-1` inclus comme classe) et un ARI restreint aux points de
vérité terrain non bruités. `--no-save-labels` conserve les métriques sans les
artefacts volumineux.

## Résultats et reprise

Une session contient :

```text
session_manifest.json       matrice exacte, commit Git et commande
datasets/*.json             seed, SHA-256, bornes et description
datasets/*.npy              points/labels (ignorés par Git)
jobs/*.json                 entrée exacte de chaque worker
results/*.json              résultat atomique par processus
labels/*.npy                labels optionnels (ignorés par Git)
logs/*.log                  stdout/stderr par worker (ignorés par Git)
summary.json / summary.csv  table compacte régénérable
```

Chaque job contient aussi une empreinte SHA-256 des octets des sources
PowerCover, du harnais de benchmark et de HGP-old. Cette empreinte entre dans
son identifiant : avec `--resume`, une modification commitée, non commitée ou
non suivie produit donc un nouveau chemin de résultat au lieu de réutiliser une
mesure issue d'un ancien algorithme.

Pour reconstruire les résumés après copie ou interruption :

```bash
python perg_hgp/benchmarks/cli.py summarize \
  perg_hgp/benchmarks/results/g4-scaling-3d
```

Les petits JSON/CSV peuvent être versionnés. Les `.npy`, `.npz` et logs sont
ignorés sous `benchmarks/results` afin d'éviter d'ajouter des centaines de Mo
au dépôt.

La campagne Blackwell exécutée le 12 juillet 2026, y compris le cas
`N=30_000_000, K=10`, les régressions numériques découvertes et les références
HDBSCAN/HGP-CGAL, est synthétisée dans
[`results/G4_20260712_REPORT.md`](results/G4_20260712_REPORT.md).
