# Microbancs de la v12

Les variantes de recherche vivent ici, **hors du produit** ([`../docs/ARCHITECTURE.md`](../docs/ARCHITECTURE.md),
règle 1) : chacune a son nom de mesure (`MES-*` de [`../docs/PLAN.md`](../docs/PLAN.md)), sa construction propre, sa
règle d'adoption écrite avant la mesure et son juge à trois verdicts (adopté, rejeté, refusé ; seul « adopté » permet
l'adoption). Un microbanc peut lier la v11 gelée (`ac081a06f`) pour vider ses entrées ou servir de témoin : c'est
pourquoi ce dossier n'est pas couvert par `tools/check_style.py`, dont la règle `[mhgp11]` refuse tout identifiant de la
v11 dans le produit. Aucune donnée SemanticKITTI ni vidage dérivé n'entre dans le dépôt ; les résultats vont dans
`../receipts/`, en comptes et empreintes seulement.

| Dossier | Mesure | État |
| --- | --- | --- |
| [`mes_m2_feuille/`](mes_m2_feuille/README.md) | `MES-M2` : feuille du catalogue data-parallèle sur un warp, deux formes (J3 par phases, cohérente) en source unique `__host__ __device__`, contre le témoin un-fil de la v11 ; `MES-S` (étendues locales) | identité exacte sur l'hôte (9 cas, 2 748 544 feuilles, warp simulé) ; banc CUDA compilé pour `sm_120`, **à jouer sur G4** ([reçu local](../receipts/mes_m2_local_20261007/RAPPORT.md)) |
| [`mes_m6_session/`](mes_m6_session/mes_m6_session_cost.cu) | `MES-M6` : coût de la Session résidente sur l'appareil (contexte, flux, réservations, lancements, graphes, transferts) | compilé ; corrigé après `CST-0209` et `CST-0210` ; **à jouer sur G4** |
