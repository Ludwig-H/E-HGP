# Origine des continuations — preuve et gate natif

27 septembre 2026. [Explication et attribution](../../audits/b_full_continuation_origin_20260927/README.md).
Moteur inchangé, GCP non utilisé, aucune mesure de performance.

`r1/` conserve l'échec de compilation du harnais et ses sources : le
`main` renommé ne bénéficiait plus du retour zéro implicite. Aucun gate
de cette première capture n'est promu. Builds de r1 conservés.

`r2/` ferme huit commandes : configuration, compilation, gate natif et
mutant sémantique dans chacun des builds Release et Clang ASan/UBSan/LSan.
32 chaînes, 116 ordres, 16 chaînes régulières sans continuation ; 16
chaînes étendues dont quatre sans continuation. Les drafts portent
16 actions de continuation : 208 comparaisons first-parent rapides et
24 par le repli, en sens direct/inverse, contre la forêt native.

Le mutant retire les continuations d'ABCZ : ses nœuds, parents et
successeurs restent identiques, son draft reste structurellement admis,
mais il perd une contribution datée. Réfutation attendue code 1, sans
crash et sans stderr, dans les deux builds.

Lecteur LIVE : sources/harness/bibliothèques hachés avant/après,
recette complète, intents/commandes/flux liés, inventaire exact des
deux binaires et configurations. Les sources propres sont copiées
avant chaque capture pour conserver aussi le premier échec.

```bash
python3 -B morsehgp3D_v9/audits/b_full_continuation_origin_20260927/run.py check
python3 -B -O morsehgp3D_v9/audits/b_full_continuation_origin_20260927/run.py check
```

Build r2 épinglé : `/workspaces/E-HGP/build/v9-continuation-origin-20260927-r2`.
Ce test ne fournit ni borne de croissance de la tour ni preuve de
complétude de son générateur. Il éprouve un critère de sélection de
l'encodeur et la nécessité du repli sur une sortie géométrique native.
