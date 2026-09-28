# Conception de la v10 (28 septembre 2026)

Documents de conception écrits par le workflow de conception de la session du 28 septembre 2026, en quatre temps :
cinq conceptions de sous-systèmes (v1), une critique adverse de chacune, une révision (v2), puis une intégration.

- [CONCEPTION_V10](CONCEPTION_V10.md) : la conception intégrée. Elle est **normative pour la suite** (§ 0) : un
  composant de l'implémentation existante (appelée V10-α) est gardé s'il passe la porte de sa phase, et migré sinon
  (§ 12.0).
- Détails par sous-système :
  - [GEN_v2](GEN_v2.md) : générateur par boîtes de centres ;
  - [TOWER_v2](TOWER_v2.md) : tour FULL, noyau sans lots, règle cartésienne ;
  - [CLUSTER_v2](CLUSTER_v2.md) : hiérarchies de points et têtes ;
  - [EVAL_v2](EVAL_v2.md) : banc et contrats de performance ;
  - [ARCH_v2](ARCH_v2.md) : infrastructure, portes et G4.

Les versions v1, les critiques et les sondes (prototypes C++ et Python, journaux, sous-dossiers `*_probe` et
`tower_v2_spike`) sont conservées hors dépôt, dans `build/v10-persist/design/`, avec leurs empreintes.

Deux mesures faites depuis confirment les prévisions de ces documents :

- **Clustering.** La prévision de CLUSTER_v2 § 0 est confirmée sur les graines `dev`
  ([reçu](../../receipts/bench_dev_selection_20260928/README.md)) : à K égal, la tour clusterise comme
  l'atteignabilité mutuelle, et le gain sur HDBSCAN vient de la tête.
- **Tour.** L'écart de coût entre la tour de V10-α et TOWER_v2 (§ 9.4) est celui mesuré dans la
  [passation](../../PASSATION.md).
