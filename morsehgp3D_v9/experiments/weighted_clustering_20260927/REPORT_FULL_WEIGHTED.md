# Publication du pilote pondéré FULL

Ce document décrit une recette, pas des résultats. Le pilote doit avoir
terminé avec `status=completed`, 26 commandes et ses 364 lignes prévues.
Une capture échouée reste échouée ; aucune ligne favorable n'est extraite
pour la remplacer.

Depuis la racine du worktree, remplacer les chemins privés ci-dessous par
ceux de la capture close et choisir des sorties qui n'existent pas :

```sh
python3 -B morsehgp3D_v9/experiments/weighted_clustering_20260927/post_audit.py \
  /chemin/prive/capture-close /chemin/prive/post-audit-normal.json
python3 -B -O morsehgp3D_v9/experiments/weighted_clustering_20260927/post_audit.py \
  /chemin/prive/capture-close /chemin/prive/post-audit-optimized.json
cmp /chemin/prive/post-audit-normal.json /chemin/prive/post-audit-optimized.json
python3 -B morsehgp3D_v9/experiments/weighted_clustering_20260927/report_full_weighted.py \
  --capture /chemin/prive/capture-close \
  --post-audit /chemin/prive/post-audit-normal.json \
  --output morsehgp3D_v9/receipts/weighted_full_gaussian_20260927/nouvelle-publication
```

Le lecteur de publication vérifie les hashes LIVE avant/après : sources,
entrées, exécutable, payloads privés, intentions/commandes et qualification.
Il lie le contre-audit à cette capture précise. Il ne relance ni ajustement,
ni géométrie, ni EOM, et ne transforme pas sa vérification de hashes en
nouvelle qualification géométrique. Une défaillance tardive peut laisser
des fichiers de sortie partiels, mais aucun `receipt.json` completed.
Conserver cet essai et choisir une autre sortie après diagnostic.

Allowlist publique : `rows.csv` (364 lignes), `aggregates.csv` (140 groupes),
`TABLES.md` et `receipt.json`. Les nuages, grands exports natifs et tableaux
de facettes restent privés ; leurs captures sont liées par chemins/hashes.
Les tableaux principaux sont fixés à K5/seuil20, z1 et z2, pour les cinq
régimes annoncés. K10, seuil50 et toutes les graines restent dans les CSV.
HDBSCAN standard n'est présent qu'en z1 et est distinct de l'EOM commun.

Le seuil HGP pondéré est une masse de facettes avant vote, pas un minimum
garanti de points après vote. Les avertissements proches des seuils et les
égalités de vote sont publiés. Moyennes/écarts-types sont descriptifs,
pas des intervalles de confiance ni une preuve de dominance. Les régimes
étaient connus lors du choix du pilote ; il ne s'agit pas d'un test tenu
à l'écart. Le contre-audit a un ARI indépendant exact, mais partage le
solveur Hungarian SciPy et ne recalcule pas le NMI.
