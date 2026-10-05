# S6b — contrelecture native ciblée

Pin local : `9e7428995e3b359301d58d882610d9d4ee720fad`, worktree `build/v11-impl-l0`. Cadre : exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed.

**Verdict : aucun nouveau défaut important établi par la lecture.** Pré-passe globale avant allocation ; admission couvrant les coexistences ; buffers privés par worker ; positions identiques dans count et fill ; jointure avant libération et publication ; refus sans produit partiel ni modification des diagnostics.

Le produit emprunte explicitement l’identité géométrique de l’OrderTree, qui doit survivre à ses lectures, et possède ses buffers. La surestimation des tampons par tous les workers est déjà signalée par le développeur et n’est pas une nouvelle alerte.

`bounds.json` ferme, par arithmétique Python standard indépendante, 3 896 formes de W_K : comptes u32, préfixes et formules d’admission u64 sous les invariants du domaine préparé. Cela ne qualifie aucune exécution native.

`source_manifest.json` épingle la portée ; deux nouvelles sources sont conservées dans `sources/`. `review.json` distingue lecture intégrale, passages sélectionnés et limites. SHA inchangés à la fermeture. Aucune compilation, aucun test natif, aucune campagne GCP ; aucune note active modifiée.

Rejeu reproductible, depuis ce dossier :

```sh
python3 -S -B bounds.py > bounds.json
python3 -O -S -B bounds.py > bounds_opt.json
```

Les deux appels donnent le code 0, sans stderr, et des sorties identiques à l’octet (empreinte dans `replay.json`). Le script emploie uniquement la bibliothèque standard, sans `assert`, et n’exécute aucun `sizeof` natif : les tailles ABI 20/40/64 octets sont des hypothèses déclarées.
