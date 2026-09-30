# Géométrie, interfaces et juges — état du raccord

30 septembre 2026. Moteur publié inchangé depuis 4b7d70422 ; preuves jusqu'à 33fcb53a0 et workflow privé relu. Raccord R2 commencé par faits_math/juges ; primitives u32/bande isolées. public_status=not_claimed. Aucun GCP.

| Point | État actuel |
| --- | --- |
| G1 / arrondi | Rejeu de l'autre auditeur : 5 969 requêtes par mode, quatre modes passent ; filtre FE_TONEAREST. Différentiels/CTest clos. Workflow R2 : refus de la preuve du chemin réellement exécuté ; réparation non close. Aucun nouveau défaut numérique démontré. FTZ/DAZ et autres filtres FULL distincts. |
| Catalogue / FULL | Doublons I/U, ordre, attache morte et plateau ternaire binarisé refusés par les petits juges R2. Rejeu normal/−O ; campagne 33/33 observée. |
| Égalité core | Audit géant observé : mutant `<` contre `≤` survit aux portes et modifie des labels sur cinq points. Fixture d'égalité à ajouter au raccord, pas d'erreur HEAD démontrée. |
| Grandes sorties | Ordre K manquant, sites étrangers et tailles annoncées fausses admis par le lecteur structurel sur petits dumps. Passer K/site attendus et vérifier le schéma ; contrôle linéaire, sans qualification Γ massive. |
| CLI | Parseur/tête dans des copies. Collision étiquettes/arbre persistante ; défaut RAII du helper OutputSet R2 observé dans l'addendum 815d5fcb0. Diagnostic budgété et union à qualifier. |
| Domaine / indices | RankIndex corrigé ; B→u32 à protéger ; atlas/représentants bornent la forêt FULL complète actuelle. Arène ExtCell globale : garde recommandée, contre-modèle cardinal distinct d'une tour géométrique reproduite. Grid32/filtre relatif isolés ; M≥K+3/diagnostic restent à raccorder. [Contrat](../AUDIT_MASSIF_LIDAR_20260930.md). |

Sources : [R2](../audit_continu_20260929/CONTRE_AUDIT_R2_20260930.md), [SiteTree](../audit_continu_20260929/catalogue/CONTRE_AUDIT_SITETREE_CORRIGE_20260929.md). Ce sont ses rejeux ou ses observations, pas nos nouvelles exécutions.

La réduction de couverture reste utile : pour dist(x,C)≤r, témoin complet avec population≥K et p+q_min≤K. Résoudre son centre à cet ordre, suivre les ancêtres et dédupliquer les composantes ; garder chaque incidence I/U. **Conserver les fusions FULL à K+1.** La bande K2 η>0 réclame toutes ses paires, pas seulement les boules critiques. [Frontière](ANCRAGE_AMBIGUITES.md).

Binaire commun à vérifier : arrondis, listes/plateaux, paramètres/budgets, collisions et refus avant export. [Développement observé](../../receipts/audit_independant_20260930/developer_rebound/provenance/README.md) : première intégration docs/juges privée ; SiteTree/pool/tête/CLI restent des étapes distinctes. [Arène/protocole de vraies feuilles](../../receipts/audit_independant_20260930/developer_rebound/catalogue/README.md) : ancres hors boîte fermée, préparation/éligibilité/résidu aval payés ; groupes recouvrants ne s'ajoutent pas sans disjonction. [Ordre large/refus](../../receipts/audit_independant_20260930/wide_order_followup/README.md) : rang exact et propagation du refus à conserver.

[Base et preuves](../../receipts/audit_independant_20260929/historique/base_6206d1d11/GEOMETRIE_CATALOGUE.md) ; [ancienne lecture de nos copies](../../receipts/audit_independant_20260930/notes_avant_synthese/README.md). Ne pas rouvrir les causes fermées depuis.
