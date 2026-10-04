# Écart v10/v11, tranche 2 : chaîne sérielle de la forêt et préchargement, 3 octobre 2026

Reçu de la section 7 de la [note d'audit](../../../audits/NOTE_CLAUDE_AUDIT_PERFORMANCE_V10_V11_20261003.md).
Même machine, mêmes entrées et même protocole que la [tranche 1](../ecart_v10_v11/README.md), dont ce reçu
relit les empreintes sans les modifier. **GCP non utilisé.**

Changements : séries de naissances composées à la classification (`BirthRuns`), préchargement en trois étages
de la table de populations dans `resolve_lane`, pas de `find` du balayage comptés localement. Un
préchargement des états DSU pendant la publication a été essayé puis retiré, faute de gain mesurable.

| Fichier | Contenu |
|---|---|
| `records.json` | Treize exécutions de la nouvelle voie, mode 16379 : trois trames ×3 à W4, trois nuages uniformes, 08/000000 à W1 |
| `measure2.py` | Plan de mesure ; réutilise `measure.py` de la tranche 1 |
| `mutants_tower_3.json` | Mutants des séries de naissances |
| `ctest_fast.txt` | Résumé de la suite `fast` complète |
| `check.py` | Lecteur : empreintes des deux reçus, identité des sorties avec la base, médianes |

```sh
python3 -B morsehgp3D_v11/receipts/developpement_20261003/ecart_v10_v11_tranche2/check.py
```

## Résultats

Identité : les treize sorties ont exactement les empreintes de la base `895680ff8` de la tranche 1.

Phase des naissances à W4 (médianes) : 08/000000 61 → 30 ms, 08/000100 52 → 27 ms, 08/000200 70 → 39 ms.
Phase régulière à W4 : −2 à −9 % ; à W1 sur 08/000000, 7,63 → 7,50 s. Les murs complets à W4 restent dans le
bruit de la machine (le domaine, inchangé, varie lui-même de +10 % entre les deux campagnes).

Portes : suite `fast` complète 667/667, une sentinelle LiDAR sautée faute d'entrées (`ctest_fast.txt`) ;
mutants `forest_cohort_centres_key`, `forest_cohort_capacity_short`, `forest_cohort_nonbirth_reset` et
`birth_runs_jonction_perdue` tous tués par code (`mutants_tower_3.json`). Le préchargement n'a pas de mutant :
il ne décide rien, et l'identité des sorties le couvre.
