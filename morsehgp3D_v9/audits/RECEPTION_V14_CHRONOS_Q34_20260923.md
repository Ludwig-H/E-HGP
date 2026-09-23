# Sonde v14 : deux identités manquantes dans les chronos q3/q4

23 septembre 2026. Lecture du produit publié à `67fce4e9`, puis du
correctif `fe1142b5`, sans nouveau reçu G4. Les deux contre-exemples
restent acceptés par le lecteur de `fe1142b5`. Cadre `reference_cpu`,
`quantized_u18_input_only`, `not_claimed`. La v14 prépare les jobs du front q3/q4 par masse décroissante,
en vise 64 par fil et expose `job_sum_s` et `max_job_ms`. Les portes de
géométrie du front et de q3/q4 comparent les sorties et comptes logiques à
leurs oracles ; les chiffres de gain local cités dans `docs/PROVENANCE.md`
ne sont pas encore une ablation G4 épinglée. Le
[rectificatif R8](RECTIFICATIF_R8_Q34_ORDONNANCEMENT_20260923.md) précise
que `max_job_ms` exclut les plages de rectangles consommées depuis la file.

Le lecteur `gcp-migration/tower_worker_v9.py:434–451` vérifie séparément
`job_sum_s≤W·wall_max_ms/1000` et `max_job_ms≤wall_max_ms`, où W est le nombre de fils
démarrés. Il accepte pourtant les deux réponses synthétiques suivantes,
avec `status=complete_relative`, W=4, `wall_min_ms=wall_max_ms=q34=1000`,
quatre jobs, aucune tâche publiée et tous les autres champs d'occupation
valides :

| Mutation isolée | Valeurs (secondes sauf `max_job_ms`) | Verdict actuel | Contradiction |
| --- | --- | --- | --- |
| Maximum sans somme | `job_sum_s=0`, `max_job_ms=900` | accepté | Le maximum d'une liste de durées positives ne dépasse pas leur somme. |
| Jobs et attente disjoints | `job_sum_s=3`, `wait_sum_s=2`, `max_job_ms=900`, `cpu_sum_s=0,5` | accepté | Les cinq secondes cumulées ne tiennent pas dans les quatre secondes-fils disponibles. |

Les intervalles `run_job` et `condition_variable::wait` sont disjoints
pour chaque ouvrier (`wspd_q34.cpp:890–897,925–936`). Il faut donc aussi
`max_job_ms/1000≤job_sum_s+ε` et
`job_sum_s+wait_sum_s≤W·wall_max_ms/1000+ε`, avec ε couvrant les arrondis
de publication à trois décimales (1 ms suffit pour la première relation,
environ 2 ms pour la seconde). Le temps des jobs est un **mur**, pas
du CPU : aucune inégalité `job_sum_s≤cpu_sum_s` n'est proposée. La
fixture synthétique du selftest actuel a elle-même un job maximal de
0,08 ms pour une somme de 0,05 ms à W1 ; choisir une tolérance d'environ
1 ms pour la première relation, ou corriger cette fixture avant de durcir
le lecteur.

Ajouter les deux mutations à la porte du lecteur avant de traiter les
nouveaux chronos comme preuve d'une queue de travail. Pour attribuer le
temps restant, publier aussi la plus longue plage et les instants de fin
du dernier job et de la dernière plage ; `max_job_ms` seul ne voit pas la
file aval. Ce constat concerne la **réception des mesures**, sans écart
observé de candidat, catalogue ou tour FULL.
