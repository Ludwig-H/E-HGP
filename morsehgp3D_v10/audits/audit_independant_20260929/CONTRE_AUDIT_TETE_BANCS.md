# Tête, pool, bancs et CUDA — état du raccord

30 septembre 2026, preuves relues jusqu'à d679ae29d. Produit inchangé ; copies R2 non qualifiées ensemble. public_status=not_claimed. Aucun GCP.

| Sujet | Progrès et réserve |
| --- | --- |
| H1–H4 | Domaine conjoint niveaux/poids/z, racine zéro et M·λ maximal contrôlés en R2 ; porte native de l'autre auditeur : 26 fixtures valides, 25 refus. Trois retours quadratiques tués dans les journaux observés. H3 ancien corrigé dans cette copie. |
| CLI | Quatre sondes : refus numérique, y compris configuration invalide tardive dans le même dendrogramme, avant sortie. Collision labels/arbre reste code 0 avec écrasement. Recevoir ensemble Outcome, validation et écritures vérifiées. |
| Pool | CAS saturant, série et nettoyage contre-vérifiés. Deux différentiels 24/24 et oracles terminés ; préfixes SHA96/64 bits. Timeout d'une sonde à barrière distinct d'un deadlock produit démontré. |
| Bancs | ARI1,25/NaN non refusé rejetés. Alpha2/NaN, schémas absents/inconnus et colonne ari_s dupliquée encore admis. A/C historiques valides non réfutés. |
| CUDA | Schéma renforcé en bd8a9286f ; notre Python conserve entier JSON énorme → OverflowError et lancement impossible → pas d'attempt.json. Aucun faux succès ni GPU exécuté. |
| Dates / mémoire | FULL exact ; PointDendrogram coalesce certaines dates en double. Budget de pipeline ouvert. Nouvelle unité physique à soumettre à la garde conjointe. |

[R2 de l'autre auditeur](../audit_continu_20260929/CONTRE_AUDIT_R2_20260930.md) distingue ses rejeux des clôtures observées. Les anciens résultats ne deviennent pas une qualification commune.

Même arbre changé d'unité : β_phys=h²β_grille, λ_phys=h^(−z)λ_grille ; facteur commun positif sur les stabilités, décisions EOM idéales identiques. Requantifier, modifier les masses ou fusionner des dates change l'objet. Garder unité interne déclarée, refus finis et dates exactes séparées de l'affichage.

Prochaine porte commune : paramètres/CSR forgés, petit niveau positif, racine zéro, config invalide tardive sans sortie, collisions, faute d'allocation/worker puis réutilisation, mutations de plan/schéma. Pas de long lot sur des composants encore séparés.

Preuves : [base](../../receipts/audit_independant_20260929/historique/base_6206d1d11/TETE_BANCS_PREUVES.md), [copies exactes antérieures](../../receipts/audit_independant_20260930/notes_avant_synthese/README.md), [pool initial](../../receipts/audit_independant_20260929/contre_pool_preintegration/receipt.json), [CAS R2](../../receipts/audit_independant_20260930/pool/source_status_20260930.json), [lecteur CUDA](../../receipts/audit_independant_20260930/cuda_reader/bd8a9286f_469e3210/).
