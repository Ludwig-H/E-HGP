# Documentation E-HGP

La documentation est organisée par **statut scientifique**, non par date de création.

## Références actives

1. Le [`README.md` racine](../README.md) fixe l'ambition, les deux backends et les limites annoncées.
2. Le [corpus mathématique A–E](math/README.md) contient les définitions, théorèmes, conjectures et certificats qui gouverneront les futurs backends.
3. Le [rapport ordre–échelle du 14 juillet 2026](../RAPPORT_HGP_ORDRE_ECHELLE_DESCENTE_GPU_2026-07-14.md) conserve la synthèse longue et la comparaison avec le code existant.

## Documents de recherche datés

Ces textes expliquent l'évolution des idées. Ils restent utiles pour les preuves intermédiaires, les contre-exemples et l'audit du code, mais ne prévalent pas sur le corpus actif.

- [Rapport Morse et SoftLens — 13 juillet 2026](../RAPPORT_HIERARCHIE_HGP_MORSE_SOFTLENS_GPU_2026-07-13%20%281%29.md)
- [Addendum DTM et hiérarchie sparse — 13 juillet 2026](../ADDENDUM_DTM_HIERARCHIE_SPARSE_GPU_2026-07-13.md)
- [Rapport grande dimension — 12 juillet 2026](../perg_hgp/RAPPORT_PERG_HGP_GRANDE_DIMENSION_2026-07-12.md)
- [Spécification certifiée antérieure](../algorithme_reg_knn_hgp_certifie_v2.md)
- [Audit PowerCover3D](../perg_hgp/Rapport_audit_PowerCover3D_30M_K10_SemanticKITTI_2026-07-11.md)

## Documentation du code existant

- [`perg_hgp/README.md`](../perg_hgp/README.md) — package Python actuel ;
- [`perg_hgp/POWER_COVER_3D.md`](../perg_hgp/POWER_COVER_3D.md) — contrat du backend cubique ;
- [`E-HGP/DESIGN.md`](../E-HGP/DESIGN.md) — architecture E-HGP antérieure ;
- [`perg_hgp/benchmarks/`](../perg_hgp/benchmarks/) — protocoles et résultats de benchmark.

## Règle de lecture

Une affirmation de performance dans un rapport daté décrit un prototype et son protocole. Une garantie du futur `MorseHGP3D` ou `HomogeneousLensTower` doit apparaître dans le corpus A–E, puis dans le contrat du backend correspondant lorsqu'il existera.
