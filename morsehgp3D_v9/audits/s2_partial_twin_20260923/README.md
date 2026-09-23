# S2 : un reçu partiel peut rester sans jumeau moteur LiDAR

Audit ciblé du commit `1f5dede11d841e2d726f643ea2b8fbe55fbba9aa` (23 septembre 2026). Le nouveau préflight compare bien la tour GPU/batch à la tour moteur sur son **nuage synthétique**. Le plan exige aussi un jumeau moteur pour chaque configuration batch LiDAR. Mais cette dernière exigence porte sur les *cas inscrits au plan*, pas sur les cas effectivement achevés.

La [reproduction](reproduce.py) utilise le faux G4 déjà présent dans `gcp-migration/tower_selftest_v9.py`. Elle laisse terminer le cas 0 du plan par défaut, GPU 08/000000/K5/W48, puis endort uniquement le cas 1 moteur jusqu'à la fin du budget. Le contrôleur et le lecteur indépendant acceptent le reçu `partial` (`exit_code=0`) avec `completed_case_indices=[0]`, `cross_worker_comparisons=[]`, `backend=cuda_g4`, `GPU_executed=true` et `FULL_executed=true`. Le [résultat minimal](observed.json) est issu de cette exécution. Aucun vrai GPU, aucune vraie trame LiDAR et aucune qualification de performance ne sont impliqués par cette reproduction de protocole.

Cause : `validate_plan` lignes 290–294 exige seulement la présence du jumeau ; `compare_cases` lignes 764–775 ignore les cas incomplets ; le worker produit `partial` lignes 1009–1019. Le lecteur `validate_received` lignes 295–305 demande une tour achevée, mais accepte `all([])` lorsque les comparaisons sont vides ; le contrôleur publie ensuite `GPU_executed=true` aux lignes 483–487. Toutes les lignes renvoient aux fichiers du commit audité.

**Correction proposée :** pour chaque résultat GPU/batch LiDAR `complete_relative` présenté comme validé, exiger un résultat moteur `complete_relative` du même fichier/K/s et une comparaison d'objet égale. Si le jumeau manque dans un reçu partiel, conserver les mesures brutes comme `unpaired` et empêcher toute lecture de ce cas comme équivalence LiDAR vérifiée. Un test adversarial peut reprendre ce script en exigeant le refus ou le statut explicite `unpaired`.

Rejouer depuis la racine avec un checkout détaché du commit épinglé :

```sh
git worktree add --detach /tmp/mhgp9-s2-partial-1f5d 1f5dede11d841e2d726f643ea2b8fbe55fbba9aa
python3 -B morsehgp3D_v9/audits/s2_partial_twin_20260923/reproduce.py --source /tmp/mhgp9-s2-partial-1f5d
git worktree remove /tmp/mhgp9-s2-partial-1f5d
```

Le script refuse tout autre `HEAD` et ne modifie pas le dépôt. Ce
worktree détaché ne crée aucune branche.
