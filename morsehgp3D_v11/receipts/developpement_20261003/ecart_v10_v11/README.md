# Écart v10/v11 : diagnostic et premiers correctifs, mesures locales du 3 octobre 2026

Reçu de la [note d'audit](../../../audits/NOTE_CLAUDE_AUDIT_PERFORMANCE_V10_V11_20261003.md).
Base : `895680ff8` (HEAD de `main`) ; correctifs : commit qui ajoute ce reçu. Cadre
`exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`.
**GCP non utilisé** : aucun identifiant dans le conteneur, aucune commande GCP lancée.

## Machine et protocole

Conteneur partagé à 4 cœurs, GCC 13, Release (`-O3`), profil u21, aucune option `-march`. Le bruit
entre deux exécutions identiques atteint ±10 % : chaque case LiDAR est jouée trois fois, en
alternant base et nouvelle voie, et la médiane est retenue. Ces temps ne sont pas des temps G4.

Entrées : les six cas du manifeste `reuse1` (trois trames SemanticKITTI 08 sans sol entières, grille
1 mm, et trois nuages uniformes de 8 000, 16 000 et 32 000 sites), reconstruits par
`restore_inputs.py` depuis les fichiers déjà présents dans le dépôt, empreintes vérifiées. Aucun
octet de données n'est copié ici.

Commande d'un cas (sonde FULL existante, K = 5, feuille 16) :

```sh
mhgp11_full_bench <xyz> <ids> <sortie.bin> 5 16 256 0 4294967295 8589934592 <W> <mode>
```

Mode 2047 : configuration G4 de la reprise. Mode 16379 : 2047 + graphe de paires (2048), table de
populations (4096) et ordres concurrents (8192), sans mémo (4).

## Fichiers

| Fichier | Contenu |
|---|---|
| `records.json` | Toutes les exécutions : durées, compteurs du catalogue et des descentes, empreinte de la sortie |
| `tasks_base.txt`, `tasks_new.txt` | Durées par tâche de la passe unique à W1 (diagnostic `task_times.cpp`) |
| `sim.txt` | Murs simulés à W8/W24/W48 par `sim.py` (ordre du plan et LPT) |
| `mutants_*.json` | Rapports des campagnes ciblées (mutants nouveaux ou réancrés) |
| `ctest_fast.txt` | Résumé de la suite `fast` complète |
| `measure.py`, `restore_inputs.py`, `task_times.cpp`, `sim.py` | Outils, rejouables hors dépôt |
| `check.py` | Lecteur : empreintes, identité des sorties, médianes |

```sh
python3 -B morsehgp3D_v11/receipts/developpement_20261003/ecart_v10_v11/check.py
```

## Résultats

**Identité.** 35 exécutions (six entrées ; base et nouvelle voie ; modes 2047 et 16379 ; W1 et W4) : une seule
empreinte de sortie par entrée. Les gros fichiers de sortie ont été supprimés après hachage ; seuls leurs
SHA256 sont conservés dans `records.json`.

**Murs locaux** (médianes de trois exécutions à W4 pour les trames, une sinon) :

| Entrée | Base 2047 | Nouveau 2047 | Nouveau 16379 |
|---|---:|---:|---:|
| 08/000000 | 7 550 ms | 6 286 ms | 4 471 ms |
| 08/000100 | 5 731 ms | 4 777 ms | 3 624 ms |
| 08/000200 | 6 977 ms | 5 836 ms | 4 336 ms |
| uniforme 8 000 / 16 000 / 32 000 | 2 682 / 5 900 / 13 580 ms | — | 1 696 / 3 759 / 8 223 ms |

W1 sur 08/000000 : 25,2 s → 17,1 s. Pic `MemoryBudget` : 328 → 339 Mo.

**Durées par tâche de la passe unique** (W1, 08/000000, `sim.txt`) : base 951 tâches, la plus longue 0,885 s
sur 12,25 s, mur simulé W48 0,923 s (idéal 0,255 s) ; nouveau sans graphe 1023 tâches, 0,045 s sur 9,30 s,
0,202 s ; avec graphe 0,035 s sur 7,99 s, 0,177 s.

**Portes.** Suite `fast` complète au code final : 666/666, une sentinelle LiDAR sautée faute d'entrées
(`ctest_fast.txt`). Mutants ciblés, tous tués par code au final : `num` 1/1, `index` 2/2, `catalogue` 9/9,
`tower` 9/9. Deux étapes intermédiaires sont conservées, non effacées : la première campagne catalogue
(`mutants_catalogue_1.json`) a laissé survivre les trois mutants des lignes vivantes et de la coupe du préfixe,
faute de fixtures assez riches, d'où la porte `mhgp11_catalogue_pair_graph_live_rows` (douze nuages
aléatoires) qui les tue (`mutants_catalogue_2.json`) ; `racine_courante_non_suivie` n'était d'abord tué que par
délai (chaîne cyclique sans fin dans `close`), d'où une garde de cardinal qui le refuse par code
(`mutants_tower_2.json`). Les `sources_sha256` des rapports désignent les copies figées de chaque campagne.

## Limites

Aucune mesure G4 : l'estimation de la note (≈ 0,32 s à W48) dérive du rapport W1 G4/local de l'ancien code et
d'un modèle de chemin critique, sans contention mémoire ni NUMA. Pas de K = 10, pas d'ASan/TSan dans cette
session, pas de comparaison canonique à la v10 figée.

