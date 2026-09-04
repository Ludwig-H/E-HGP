# Sonde d'ablation — correction du harnais CI

**La correction est vérifiée localement, sans affaiblir le lanceur : 23 scènes passent en Python normal et sous `-O`, y compris avec `LD_LIBRARY_PATH` hérité.** [Reçu indépendant : sources, patch, commandes, environnements et sorties](receipts_iteration3/sonde_ci_current.json).

Cadre : `exploration_v7_hors_registre` / `cpu_reference` / `quantized_u16_input_only` / `audit_independant_math_and_architecture` / `public_status=not_claimed`. Audit du 4 septembre 2026 ; trois sources stables avant/après, copies inchangées sous `audits/.work_sonde_ci/`, `TMPDIR` dans ce dossier et `PYTHONDONTWRITEBYTECODE=1`.

La trace locale du [job CI d9e4ee01](../receipts/ci_sonde_environment_20260904/github_failed_excerpt.log) rapporte un seul échec parmi 292 tests : les trois contrôles d'inventaire de `sonde_ablation_gate` recevaient le refus attendu du lanceur, car `setup-python` avait défini `LD_LIBRARY_PATH`. Le harnais attendait un inventaire nominal sans nettoyer cet environnement. Cette lecture utilise les fichiers locaux épinglés dans le reçu ; aucun workflow n'a été déclenché.

Le delta de `tests/sonde_ablation_gate.py:1037–1052` conserve explicitement un appel brut sous cet environnement et exige **code 2, stdout vide et motif `REFUS : variable LD_LIBRARY_PATH definie`**. Les deux inventaires nominaux reçoivent ensuite un environnement sans les sept variables déjà refusées par `Porte.lancer`. Les contrôles de fichiers intrus et d'égalité avec le manifeste restent présents. Le bloc `patch.dict` restaure l'environnement en sortie, y compris sur exception.

Le lanceur et l'agrégateur sont bit-identiques à ceux du commit CI. En particulier, la garde de `bench/sonde_ablation_reduce.sh:211–213` reste placée avant la branche `--inventaire` et conserve le refus des variables de chargement. La correction ne modifie ni l'autorisation des ablations ni les vérifications avant publication.

| Rejeu indépendant | Résultat |
| --- | --- |
| Porte complète, Python normal, variable héritée simulée | **0**, 23 scènes vertes, stderr vide |
| Même porte sous `python3 -O` | **0**, 23 scènes vertes, stderr vide |
| Appel direct `--inventaire` avec `LD_LIBRARY_PATH` | **2**, stdout vide, motif de refus attendu |
| Même appel après nettoyage de l'environnement | **0**, inventaire exact : `META.txt`, `out/SHA256SUMS`, `out/intrus.txt` ; seul le manifeste racine est exclu |

Les quatre fichiers de cette dernière fixture sont inchangés après les deux appels. Aucune adaptation du chemin temporaire dans les sources n'a été nécessaire. Les portes utilisent exclusivement leurs faux binaires : aucune campagne moteur, compilation, mesure GPU ou action GCP. Le succès local qualifie ce delta de harnais ; il ne constitue pas un nouveau résultat CI global ni une qualification du moteur.

| Source | SHA-256 |
| --- | --- |
| `tests/sonde_ablation_gate.py` | `acca76c18306b6161651f55b292365232c20baa983ba278c37def75e3b22d3bc` |
| `bench/sonde_ablation_reduce.sh` | `6312dc32ae6dca057b184e1b29da39cdfbc8116fd47f501b5e341c56200e3927` |
| `bench/sonde_ablation_reduce.py` | `901ba9ee87939aed0ff9f113dff7ea4c024a75f15a8c4717e12511f8d8fbbecb` |

GCP non utilisé.
