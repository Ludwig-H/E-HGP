# Géométrie, interfaces et juges — état du raccord

30 septembre 2026, preuves relues jusqu'à d679ae29d. Produit inchangé ; copies R2 distinctes. public_status=not_claimed. Aucun nouveau rejeu sur unités inchangées, aucun GCP.

| Point | État actuel |
| --- | --- |
| G1 / arrondi | Rejeu de l'autre auditeur : 5 969 requêtes par mode, quatre modes passent ; filtre FE_TONEAREST. Différentiels/CTest clos. Huit mutants survivants, dont contournement de la voie dirigée : contrôler le chemin. FTZ/DAZ et autres filtres FULL distincts. |
| Catalogue / FULL | Doublons I/U, ordre, attache morte et plateau ternaire binarisé refusés par les petits juges R2. Rejeu normal/−O ; campagne 33/33 observée. |
| Grandes sorties | Ordre K manquant, sites étrangers et tailles annoncées fausses admis par le lecteur structurel sur petits dumps. Passer K/site attendus et vérifier le schéma ; contrôle linéaire, sans qualification Γ massive. |
| CLI | Parseur strict et tête numérique dans deux copies. Diagnostic budgété annoncé, capture encore à recevoir. Collision étiquettes/arbre persistante. Leur union reste à qualifier. |
| Domaine / indices | M≥K+3 garde la feuille produit dans la copie ; diagnostic M≥K exige budget explicite. Massif : garde B→u32, milieu/produit RankIndex, atlas et mémoire. [Contrat](../AUDIT_MASSIF_LIDAR_20260930.md). |

Sources : [R2](../audit_continu_20260929/CONTRE_AUDIT_R2_20260930.md), [SiteTree](../audit_continu_20260929/catalogue/CONTRE_AUDIT_SITETREE_CORRIGE_20260929.md). Ce sont ses rejeux ou ses observations, pas nos nouvelles exécutions.

La réduction de couverture reste utile : pour dist(x,C)≤r, témoin complet avec population≥K et p+q_min≤K. Résoudre son centre à cet ordre, suivre les ancêtres et dédupliquer les composantes ; garder chaque incidence I/U. **Conserver les fusions FULL à K+1.** La bande K2 η>0 réclame toutes ses paires, pas seulement les boules critiques. [Frontière](ANCRAGE_AMBIGUITES.md).

Binaire commun à vérifier : quatre arrondis, contrôles positifs/mutants de listes et plateaux, paramètres/budgets, collisions et refus avant export. Les lacunes de lecteurs ne démontrent aucune nouvelle erreur de topologie native.

[Base et preuves](../../receipts/audit_independant_20260929/historique/base_6206d1d11/GEOMETRIE_CATALOGUE.md) ; [ancienne lecture de nos copies](../../receipts/audit_independant_20260930/notes_avant_synthese/README.md). Ne pas rouvrir les causes fermées depuis.
