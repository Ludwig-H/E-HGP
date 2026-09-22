# Reçu de l'audit général de la v8 (ouverture de la v9)

22 septembre 2026. Cadre : `exploration_v9_hors_registre`, `backend=none`,
`mode=ouverture_audit_v8_et_v7`, `public_status=not_claimed`. GCP non utilisé.
Commit audité : `origin/main` **12294241**, lu dans un worktree détaché propre
(vérifié avant et après chaque contrôle). Ce reçu ne mesure aucune
performance ; il épingle ce qui a été audité et rejoue ce qui pouvait l'être
sans calcul lourd.

## Contenu

| fichier | contenu |
| --- | --- |
| `INVENTORY.json` | commit audité ; nombre de fichiers et de commits de la v7 et de la v8 ; tailles par dossier ; sha256 des 59 documents `morsehgp3D_v8/docs/*.md`, des entrées (`README`, `PASSATION`, `ETAT_COURANT`, `AGENTS.md`, coordination v8) et du `README.md` des 47 reçus v8 qui en ont un (sur 48) ; liste des 89 entrées indexées **non commises** du worktree partagé avec le sha256 de leur contenu, et des fichiers v8 non suivis |
| `CHECKS.json` | les deux constructions neuves et leurs résultats CTest, les tests non exécutés et leur raison, les trois lecteurs de reçus rejoués |
| `logs/` | journaux de configuration et de CTest des deux constructions (chemin du worktree audité anonymisé) |
| `tools/` | les deux scripts qui ont produit ce reçu |
| `SHA256SUMS` | empreintes de tous les fichiers ci-dessus |

## Résultats

Première construction, Release, Boost 1.83 (`build/v7_boost_gate/extracted/usr`,
non versionné), hors de l'arbre de build canonique : **125 tests exécutés, 125
verts**, 158 s ; 7 non exécutés (2 portes spatiales, 5 scripts de mutation
désactivés hors de `<worktree>/build/`). Seconde construction, placée sous
`<worktree>/build/`, étiquette `mutation` seule : **8 exécutés, 8 verts**,
1 désactivé. Union : **132 tests enregistrés, 129 verts, 0 échec**, 3 désactivés
par construction :

- `mhgp8_q34_indexed_witness_mutations` : site de mutation non unique depuis `2629a536` ;
- `mhgp8_q34_spatial_gate` et sa variante `-O` : actives seulement dans le build
  épinglé `build/v8_q4_seed_cells_r2_20260921`, où `ctest` est interdit.

Lecteurs rejoués au commit audité (`morsehgp3D_v8/bench/run_ground_baseline.py read`,
normal `-O`) : `ground_baseline_20260921` (9 lignes), `ground_phase1_20260921`
(6 lignes) et sa variante `only` (3 lignes), tous `passed`.

Ces résultats disent que la v8 publiée est cohérente avec ses propres portes.
Ils ne qualifient aucun contrat : la v8 ne produit pas la tour (voir la
[synthèse](../../docs/AUDIT_V8_SYNTHESE.md)).

## Rejouer

```bash
git worktree add --detach /tmp/v8_head 12294241
cmake -S /tmp/v8_head/morsehgp3D_v8 -B /tmp/v8_head/build/audit -DCMAKE_BUILD_TYPE=Release -DBOOST_ROOT=<boost>
cmake --build /tmp/v8_head/build/audit --parallel
ctest --test-dir /tmp/v8_head/build/audit -j 3 --output-on-failure
sha256sum -c SHA256SUMS
```

Les scripts de `tools/` contiennent les chemins de l'hôte d'origine ; ils
documentent la production du reçu, ils ne sont pas une porte.
