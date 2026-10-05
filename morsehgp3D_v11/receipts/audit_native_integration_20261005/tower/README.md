# S3 natif — contrelecture du raccord, 5 octobre 2026

**Lecture favorable, aucun nouveau défaut matériel trouvé.** Sources WIP de l'acteur
`build/v11-impl-s3`, HEAD `f98aeed67d4030dd78e11d5faf7d8556c4d17aaf` : capture du
5 octobre à 07:28 UTC, puis dépendances capturées séparément avant leur lecture.
`BEFORE.json`, `DEPENDENCIES_BEFORE.json`, `CORE_BEFORE.json` et `AFTER.json` donnent
les empreintes. Tous les fichiers lus et les deux rapports restent identiques à
la fin de la lecture. Ni acteur, ni dépôt, ni capsule précédente modifiés.

La source `attachment.cpp` (SHA 77f84f00506729b253c22216860ec8c0c9bcd579dcac1c9ed3ac8c54dadb01cd)
et `seed_log.hpp` sont **identiques** à `audit_supports_contract_20261005/tower/wip_0604`.
Les preuves D2/E5 et la garde des traces ne sont donc pas rejouées ni présentées
comme de nouvelles corrections. Le contrôle porte sur leur raccord au constructeur.

- [forest_plateau.cpp](source_before/morsehgp3D_v11/src/tower/forest_plateau.cpp) : lignes 50–60 et 110–115,
  journal de la naissance avant `find/touch`, dans les cellules étendues et régulières.
  Le journal n'affecte aucun choix du DSU. `journal_raccord.patch` conserve le diff exact.
- [forest_parallel.cpp](source_before/morsehgp3D_v11/src/tower/forest_parallel.cpp) : lignes 251–282,
  résolution parallèle puis application par ordinal ; `run` vide les lots avant une
  cellule étendue et ferme le plateau après toutes ses cellules. Aucun worker n'écrit
  le journal. Cette conclusion statique n'est pas une qualification TSan.
- [order_tree.cpp](source_before/morsehgp3D_v11/src/tower/order_tree.cpp) : lignes 12–32,
  capacité régulière exacte ou majorant C(m,K−p) étendu, admission avant allocation.
  Les naissances régulières et t=m n'ouvrent pas le journal. Une naissance étendue
  à t<m peut réserver inutilement son majorant : comportement annoncé, sans
  sous-estimation démontrée. Les réservations peuvent donc excéder le journal rempli.
- [attachment.cpp](source_before/morsehgp3D_v11/src/tower/attachment.cpp) : lignes 83–139 et 155–208,
  graines relevées à r−1, parent au rang r, rôle fermé, branches ouvertes, contrôle
  des six registres et compactage en place. Le sweep meurt avant `publish_prior` ;
  les marques sont rendues avant l'allocation de `prior`. La copie finale coexiste
  avec le journal, rendu ensuite par `build_order` avant le déplacement du domaine.
- [order_tree.cpp](source_before/morsehgp3D_v11/src/tower/order_tree.cpp) : lignes 83–107,
  diagnostics publiés seulement après réussite complète ; domaine non déplacé sur
  refus. [order_tree_fault.cpp](source_before/morsehgp3D_v11/tests/tower/order_tree_fault.cpp)
  examine chaque allocation observée, y compris avec lots W1/W4. Existence et
  pertinence des gardes vérifiées en lecture, jamais exécutées ici.
- [attach_judge.cpp](source_before/morsehgp3D_v11/tests/tower/attach_judge.cpp) : lignes 197–204 et 359–371,
  le comparateur garde le FULL 16379 et n'efface pour `build_order` que les trois
  options inapplicables (ordres concurrents et deux options de verticales).
  Les arbres sont comparés champ par champ ; les registres de travail liés aux
  descentes et aux verticales sont explicitement exclus de l'identité logique.

## Contrôle indépendant borné

[check_journal.py](check_journal.py) est autonome, bibliothèque standard seulement.
Il vérifie le majorant scalaire sur **3 896 configurations**, puis un journal abstrait
à deux plateaux, 8 cellules, 17 traces, 14 branches publiées, 120 ordres de terminaison
de jobs. Les coupes viennent d'une fermeture d'ensembles. La continuation laisse un
trou dans la source, ce qui éprouve le compactage en place des cellules suivantes.
La preuve générale `prior_total ≤ begin` et `prior_total + branches ≤ end` figure
dans le programme. Les coexistences mémoire additionnelles sont calculées exactement.

```text
python3 -B -S check_journal.py
python3 -B -O -S check_journal.py
```

**15 283 gardes**, sorties `normal.json` et `optimized.json` identiques octet pour octet.
Ce nombre décrit le petit modèle scalaire ; ce n'est ni un test C++, ni un nouvel
oracle géométrique FULL, ni un test de mémoire réelle, ni une mesure de performance.
Aucune importation de référence produit, aucun natif/build/GCP/fit exécuté.

## Portée des résultats annoncés

[impl_s3.md](reports_before/impl_s3.md) est un compte rendu WIP et annonce explicitement
ses essais locaux. Il distingue le prototype différentiel Fraction, les neuf mutants
locaux, une passe ASan interrompue par délai, et les portes G4 restant à jouer.
Ses chiffres, dumps et temps ne sont pas rejugés ici. La comparaison G4 doit conserver
le FULL qualifié comme référence et les mêmes options **applicables** à l'ordre seul,
non un masque littéralement identique. Le journal augmente les réservations quand il
est activé ; sans journal, l'invariance mathématique découle de l'absence d'effet du
pointeur nul sur le DSU, mais le coût CPU se mesure. Aucun gain ni absence générale
de régression temporelle n'est déduit de la lecture.
