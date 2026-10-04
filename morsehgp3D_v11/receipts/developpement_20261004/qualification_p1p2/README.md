# Qualification G4 des correctifs P1 et P2 — 4 octobre 2026

Cadre : `exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`. Matrice complète
de la v11 (`tools/g4_matrix.py`, plan [`plan.json`](sessions/claudequal2/plan.json)) sur la VM G4 gardée
`ehgp-v7-3b1d496aed430749ea7e049f` (us-central1-c), par `gcp-migration/v11_session.py`. Objet : les deux corrections
demandées par la note moteur des auditeurs (`audits/AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md`), livrées en
`3bd4d734e` et décrites dans [la réponse](../../../audits/QUESTION_CLAUDE_VITESSE_100MS_20261004.md) § A :

- **P1** : l'attente d'une verticale sur l'ordre inférieur (`await_lower`, `tower/forest_internal.hpp`) suit aussi
  l'abandon ; porte `mhgp11_tower_pipeline_abandon`, mutant `pipeline_abandon_apres_reveil` ;
- **P2** : l'export POINTS écrit quatre mots par niveau en u24 (format version 2), trois sinon (version 1) ; porte
  `mhgp11_tower_points_export_width` et sa jumelle `-O`, mutant `export_points_trois_mots` (u24).

## Sessions

| Session | Commit | Statut | Ce qui s'est passé |
| --- | --- | --- | --- |
| `claudequal1` | `d597ed9ba` | `failed_remote`, VM `TERMINATED` certifiée | seule la porte de largeur échoue partout (`ModuleNotFoundError: numpy` : la matrice joue les portes Python sur le Python 3.10 nu de la VM), et son échec dans le témoin u24 de la campagne de mutants de la tour laisse cette campagne sans juge ; toutes les autres portes passent |
| `claudequal2` | `eb036dbe2` | `completed`, VM `TERMINATED` certifiée | la porte relit l'export avec `struct` (bibliothèque standard seule, vérifiée sous `python3 -S`) ; matrice conforme et complète |

## Résultat (`claudequal2`)

| Configuration | Portes |
| --- | --- |
| GCC Release | 686/686 |
| GCC ASan/UBSan | 611/611 |
| GCC TSan | 611/611 |
| profils u21 / u24 | 611/611 / 611/611 |
| empoisonnement | 612/612 |
| style | 2/2 |
| Clang Release | non jouée : `clang++` absent de l'hôte (`summary.json`, champ `host`) |
| campagnes de mutants | 21/21 portes ; mutants tués par module : catalogue 49/49, cloud 16/16, core 78/78, index 11/11, num 51/51, sched 8/8, tower 125/125 |

Les deux mutants nouveaux, `pipeline_abandon_apres_reveil` et `export_points_trois_mots`, sont tués (code). Le
groupe `abandon` passe aussi sous TSan. Leçon gardée : une porte Python enregistrée dans CTest n'importe que la
bibliothèque standard.

Rejeu : `python3 check.py` (et `python3 -O check.py`) relit les reçus, vérifie l'arrêt certifié des deux sessions,
les empreintes du plan et de `results.tar.gz` de `claudequal2`, puis la matrice et les comptes de mutants dans
l'archive ; verdict attendu `qualification_p1p2_verdict conforme`. L'archive de `claudequal1` n'est pas versionnée :
son empreinte reste dans son `receipt.json`.

Ce reçu qualifie les deux correctifs et la base `eb036dbe2` ; il ne mesure aucune vitesse et ne change aucun statut
public.
