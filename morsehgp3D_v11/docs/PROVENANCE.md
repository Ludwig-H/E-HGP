# Provenance

La v11 est une base de code neuve. Ce qui vient de la v10 est un **port explicite** : la source est épinglée, les
adaptations sont dites, et le composant porté a ses propres portes dans la v11. Rien n'est repris implicitement.

## Sources

- **v10 publiée** : `origin/main` au commit `afb081774`, dossier `morsehgp3D_v10/`.
- **Raccord R2 de la v10** : série de réparation issue des audits des 29 et 30 septembre 2026, vérifiée
  (GCC et Clang 82 portes sur 82, ASan et TSan 80 sur 80, 425 mutants relus) mais jamais importée dans `main`.
  Dépôt local `build/v10-integration-r2/src`, commit `865f5e6`. Les fichiers portés depuis ce dépôt sont épinglés par
  leur empreinte sha256 ; l'archive de la série est conservée hors dépôt (`build/v10-integration-r2/series`).

## Règle

Une ligne par fichier porté : fichier v11, source (dépôt, commit, chemin, sha256), adaptations, portes v11 qui le
requalifient. Un fichier écrit à neuf n'apparaît pas ici.

## Table

À remplir par tranche (fondations en cours).
