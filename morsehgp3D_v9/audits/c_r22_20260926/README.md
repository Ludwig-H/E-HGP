# Annexe de la contre-lecture C de R22

26 septembre 2026, auditeur C. Pièces de
[`CONTRE_AUDIT_C_R22_20260926.md`](../CONTRE_AUDIT_C_R22_20260926.md).
Base `f44a8db03`. GCP non utilisé. Aucun fichier du dépôt n'a été
modifié par les pistes.

## Contenu

- `lanes_results.json` : sorties structurées des six pistes.
  - Reçu : contrôles passés et écarts.
  - Attribution : tableaux A à F et constats.
  - Trois revues : sceau, recensement précoce, bassin épinglé.
  - Contrôle des corrections des constats R21.
  - Chaque constat porte les votes de ses sceptiques et son verdict
    `CONFIRMED`, `PLAUSIBLE` ou `REFUTED`.
- `receipt_checks/` : scripts de contre-lecture du reçu.
  - Table par cas et bras.
  - Épingles, comparaisons, paires entrelacées et échantillon scellé.
  - Dérive du recensement R21 → R22.
- `attribution/` : extraction, tableaux par phase et par levier, modèle
  de fenêtre de la tour et budget du brut K5.
  - Les scripts et leurs sorties `*_out.md`.
  - `mb/init_bench.cpp` : microbanc local de l'initialisation des
    `BallData`, sur hôte chargé ; il ne vaut rien comme mesure G4.

Les chemins absolus des scripts pointent vers le worktree d'audit
`build/v9-audit-c-publish`. Pour les rejouer, remplacer la racine.
