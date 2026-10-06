# Contrat MST : suffisance de la famille et sélection de Kruskal

Le nouveau §10.10 du WIP prouve que toutes les naissances et fusions suffisent à reconstruire T_K, puis assimile cette famille à la sélection de Kruskal. Le rôle de fusion est figé au plateau : les unions précédentes peuvent déjà avoir relié toutes les branches d'une boule. Le triangle équilatéral déjà [prouvé](../../audit_supports_mst_20261006/mathematics/REPORT.md) garde ainsi trois supports au lieu de deux. Aucun nouveau calcul géométrique n'est nécessaire.

`proposed.patch` conserve la proposition de suffisance et sa preuve. Il corrige le choix publié dans MATHEMATIQUES §10.10 et les deux premières règles de SORTIES §6 : naissances conservées ; boules de fusion retenues si au moins une union réussit, dans l'ordre BallIdx, avec connexion finale des enfants. Une boule utile peut porter plusieurs unions et des liaisons redondantes ; son S* est publié une fois, ses prior originaux conservés. Aucune optimalité nouvelle du nombre ou du coût des hyperarêtes n'est affirmée.

Les deux documents WIP ont été lus deux fois avec octets identiques. `wip.patch` les reconstruit depuis le commit publié d4228f5e5 ; `sources.json` épingle base, capture et proposition. Le HEAD local du développeur est informatif, non requis au rejeu. Aucun document produit n'a été modifié. La correction rédactionnelle ne corrige pas le sélecteur natif : elle accompagne le [patch déjà proposé](../../audit_supports_mst_followup_20261006/native/README.md).

Relecture : `python3 -B replay.py --repo /workspaces/E-HGP` et `python3 -O -B replay.py --repo /workspaces/E-HGP`, puis `sha256sum -c SHA256SUMS`. Les diffs s'appliquent seulement dans une copie temporaire ; aucun build, test natif ou GCP.
