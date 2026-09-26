# Annexe de la contre-lecture C de R21

26 septembre 2026, auditeur C. Pièces de
[`CONTRE_AUDIT_C_R21_TOUR_INTEGREE_20260926.md`](../CONTRE_AUDIT_C_R21_TOUR_INTEGREE_20260926.md).
Base `273c33f7c`. GCP non utilisé. Aucun fichier du dépôt n'a été
modifié par les pistes.

## Contenu

- `lanes_results.json` : sorties structurées des sept pistes.
  - Reçu : contrôles passés et écarts.
  - Attribution : tableaux et constats.
  - Quatre revues de code : chaque constat avec les votes de ses
    sceptiques et son verdict `CONFIRMED`, `PLAUSIBLE` ou `REFUTED`.
  - ThreadSanitizer : exécutions et rapports.
- `receipt_checks/` : scripts de la contre-lecture du reçu.
  - `table.py` : table par cas, bras et cohérence avec les sorties
    brutes.
  - `pins.py` : les douze épingles.
  - `cmp.py`, `cmp2.py` : les 22 comparaisons recalculées.
  - `nums.py`, `raw.py`, `misc.py` : chiffres du README et du canal.
- `attribution/extract.py` : extraction des phases, bras GPU et
  jumeaux moteur, avec et sans sol.
- `tsan_logs/` : commande, sortie et code de chaque exécution sous
  ThreadSanitizer, avec le témoin positif `control_race`. La
  construction était CPU seule, `-DMHGP9_TSAN=ON`, RelWithDebInfo,
  lancée sous `setarch -R`.

Les chemins absolus des scripts pointent vers le worktree d'audit
`build/v9-audit-c-publish`. Pour les rejouer, remplacer la racine.
