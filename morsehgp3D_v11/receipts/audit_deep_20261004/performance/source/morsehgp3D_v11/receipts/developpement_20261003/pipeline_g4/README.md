# Pipeline des forêts et voie liée : profil, essais et qualification G4, 3 octobre 2026

Reçu de la [note d'audit du 3 octobre](../../../audits/NOTE_CLAUDE_AUDIT_V11_20261003.md) et de la
[tranche 3](../../../docs/PERFORMANCE_FULL.md). Cadre :
`exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`.
**GCP utilisé** : sessions gardées `gcp-migration/v11_session.py`, `dev_snapshot` de l'arbre de travail, VM
`g4-standard-48` (AMD EPYC 9B45, 24 cœurs × 2 SMT), arrêt ciblé certifié et état `TERMINATED` relu après chacune.

| Session | Objet | Résultat |
|---|---|---|
| `claudeprof1` | Profil `perf` W1/W48 de la base `a45daff3a`, variantes `-march=x86-64-v3/v4` | Profil du § 2 de la note ; ISA sans gain, sorties identiques |
| `claudeab1` | Voie liée + filtre F6 de `power` (essai) contre la base | Sorties identiques ; régulière W1 −28 % ; filtre ≈ 1 %, **retiré** |
| `claudeab4` | Voie liée, naissances par blocs, pipeline, attentes futex | 666/666 portes, TSan 7/7, 11/11 mutants, identité ; mesure du tableau de la note |
| `claudeab5`, `claudeab6` | Qualification : VM Spot préemptée au démarrage, arrêt ciblé certifié, aucune commande | `guarded_start.stderr` |
| `claudeab7` | Qualification de la source livrée (tri des blocs en place, banc à verdict) | voir ci-dessous |

Les sessions `claudeab2` (erreur de compilation : initialiseur manquant) et `claudeab3` (un mutant invalide : paramètre
inutilisé) ont été corrigées par les suivantes ; leurs journaux restent dans `/workspaces/.ehgp-sessions/` sans
être repris ici.

Protocole : [`protocol/ab_g4.py`](protocol/ab_g4.py) (banc apparié à verdict : constructions, portes `fast`,
TSan ciblé, mutants choisis, cinq prises W48 alternées base/nouvelle par trame et une prise W1, empreinte du dump de
**chaque** prise comparée à la première prise de base, groupes de processus fermés et quiescents) et
[`protocol/profile_g4.py`](protocol/profile_g4.py). La base est l'archive `git archive a45daff3a morsehgp3D_v11`
(reçus exclus) transmise dans les données ; les entrées sont les trames `reuse1` (hachages du manifeste).
Chaque dossier `sessions/<nom>/` garde `receipt.json`, `results.tar.gz`, `plan.json` et `plan.sh`.

```sh
python3 -B morsehgp3D_v11/receipts/developpement_20261003/pipeline_g4/check.py
```

## Qualification `claudeab5`

Verdict du banc : **conforme** (aucun refus). Constructions base et nouvelle, suite `fast` **666/666**, TSan
`mhgp11_tower_(pipeline|population_concurrent)` **7/7**, **11/11** mutants tués (`pipeline_*` ×5,
`births_blocs_*` ×3, `population_lien_*` ×2, `vertical_parallel_graine_non_elevee`), statut FULL `ok` et dump
identique à la première prise de base pour **chacune des 36 prises**. Médianes de cinq prises W48 alternées et une
prise W1, mode 16379, K = 1..5, u21, en ms :

| Trame | Base W48 | Tranche 3 W48 | Domaine base → tranche 3 | Forêts base → tranche 3 | W1 base → tranche 3 |
|---|---:|---:|---:|---:|---:|
| 08/000000 | 446,5 | **412,4** | 223,5 → 253,0 | 209,2 → **171,0** | 10 370 → 9 133 |
| 08/000100 | 439,7 | **351,7** | 230,8 → 219,3 | 208,6 → **133,0** | 8 166 → 7 067 |
| 08/000200 | 440,4 | **380,7** | 251,9 → 222,2 | 204,5 → **157,7** | 9 756 → 8 467 |

Le domaine, inchangé, varie de ±30 ms d'une prise à l'autre (passe unique au plafond SMT) ; le gain est porté par les
forêts, régulier sur les trois trames. À W1, la résolution régulière passe de 4,61 / 3,53 / 4,17 s à 3,42 / 2,50 /
2,92 s. **200 ms n'est pas atteint** : voir le § 6 de la note.
