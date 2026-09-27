# Contrelecture du pilote ponctuel — 27 septembre 2026

Le lecteur [post_audit_point_dendrogram.py](post_audit_point_dendrogram.py)
passe en modes normal et `-O` sur la même capture close : **234 lignes**,
dont **182 lignes héritées strictement inchangées**, 52 nouvelles sélections,
26 arbres ponctuels, 52 condensations archivées et 364 coupes rejouées.
Les 104 artefacts et 1 292 pins LIVE sont vérifiés avant retour.
Aucune source ni capture scientifique figée n'a été modifiée.

## Portée exacte

Le lecteur raccorde la qualification ponctuelle, le reçu pondéré antérieur,
son post-audit, le manifeste et les poids archivés par leurs hashes. Il
contrôle la grille entière, les sources avant/après, les dates rationnelles,
les cardinalités et la provenance structurelle des arbres décodés.
Les coupes strictes/fermées sont comparées aux empreintes enregistrées par
le pilote : **le routage depuis les poids n'est pas réexécuté**.

La validation figée des condensations rejoue conservation des masses,
durées et stabilités locales ; les labels doivent correspondre aux ancêtres
sélectionnés, sans racine sélectionnée et avec chaque groupe final de
cardinalité au moins `min_cluster_size`. Il n'y a pas de reconstruction de
la condensation ni de nouvelle optimisation EOM dans cette contrelecture.

Les quatre variantes d'ARI, couverture, nombres de groupes/bruit et scores
appariés sont recalculés sur les 52 labels individuels. L'ARI réutilise
l'arithmétique indépendante entière/Fraction du
[post-audit figé](../weighted_clustering_20260927/post_audit.py), SHA
`95cece7c7b74d839495485d8bad018abffb246458f2d4a800e68c479387e6e1d`.
Les contingences et fractions du score apparié sont indépendantes, mais
**le solveur hongrois SciPy est partagé**. La NMI n'est pas recalculée.
Les 182 anciens scores sont repris à l'identique et liés à leur audit,
pas présentés comme 182 nouveaux replays statistiques.

Aucun fit HDBSCAN, calcul géométrique, accès GCP ni preuve de supériorité
statistique ne résulte de ce lecteur. Les limites du [plan](PLAN.md)
restent celles du pilote : scènes connues, K5 seulement, routage exact sur
les scores binary64 relevés en dyadiques, sélection EOM binary64.

## Commandes et portes locales

Depuis la racine du worktree `build/v9-resume-20260927` :

```sh
python3 -B -m unittest discover -s morsehgp3D_v9/experiments/point_dendrogram_20260927 -p test_benchmark_audit.py
python3 -O -B -m unittest discover -s morsehgp3D_v9/experiments/point_dendrogram_20260927 -p test_benchmark_audit.py
python3 -B -m unittest discover -s morsehgp3D_v9/experiments/point_dendrogram_20260927 -p test_post_audit_point_dendrogram.py
python3 -O -B -m unittest discover -s morsehgp3D_v9/experiments/point_dendrogram_20260927 -p test_post_audit_point_dendrogram.py
```

Résultats : **11 tests runner** puis **10 tests lecteur**, PASS dans chacun
des deux modes. Ces portes synthétiques ne sont pas des mesures GPU ni des
scènes de production. Elles couvrent notamment l'ordre projection/sélection
avant vérité terrain, le registre complet synthétique, la conservation des
erreurs et interruptions, les refus de mutations, les dates nulles/infinies,
le bruit séparé en singletons et les cardinalités finales.

Sources gelées de cette contrelecture :

| Source | SHA-256 |
|---|---|
| `test_benchmark_audit.py` | `61abf9d6d86bfe0c570d4fccc9df6746d8ffc476d9215e275225bb81fbb4bf95` |
| `post_audit_point_dendrogram.py` | `09d2fa0f3439ccc73109be0691783ac4c9d3cba607dcdf9d0c9f4410c9cc9204` |
| `test_post_audit_point_dendrogram.py` | `9e23a83dce4b286b3c483026ecd9c25c8ab5a0b13efa55a8cdf67d82f39ad892` |

## Captures de lecture

Capture scientifique privée, close sans nouveau fit ni géométrie :
`/tmp/mhgp9-point-dendrogram-pilot-20260927-F4NFQA/capture/receipt.json`,
SHA `89a206c4305d26a98b945952f155b5bf0661208a9b00fb744fe900f250472fb9`.

```sh
/home/codespace/.python/current/bin/python -B morsehgp3D_v9/experiments/point_dendrogram_20260927/post_audit_point_dendrogram.py --capture /tmp/mhgp9-point-dendrogram-pilot-20260927-F4NFQA/capture --output /tmp/mhgp9-point-dendrogram-post-audit-20260927-r1-normal
/home/codespace/.python/current/bin/python -O -B morsehgp3D_v9/experiments/point_dendrogram_20260927/post_audit_point_dendrogram.py --capture /tmp/mhgp9-point-dendrogram-pilot-20260927-F4NFQA/capture --output /tmp/mhgp9-point-dendrogram-post-audit-20260927-r1-optimized
```

Les deux reçus `audit.json`, dans les deux dossiers de sortie ci-dessus,
ont le même SHA
`54f33bed3b813703c9933c99a13a5ff6c379240bfc2a5a50a3d11ad7ea8bcfe1`.
Ils sont des reçus LIVE dépendant des entrées privées épinglées, pas des
archives autonomes. Un nouveau rejeu doit choisir un **nouveau** dossier
de sortie ; aucun écrasement n'est autorisé. L'interpréteur indiqué fait
partie du raccord aux captures antérieures.
