# Audit indépendant v10 — décisions courantes

30 septembre 2026. Moteur u18 publié inchangé depuis 4b7d70422 ; preuves jusqu'à 33fcb53a0, addendum 815d5fcb0 et workflows privés relus. Raccord R2 commencé, recherches FULL→points soumises aux juges. Bande/u32 isolés. public_status=not_claimed. Aucun GCP.

## À traiter avec le développeur

| Sujet | État et prochaine décision |
| --- | --- |
| Frontière → points | Un K fixé. Antichaîne : couverture, laminarité et monotonie avec η conservées ; nouveau vrai cas à quatre points avance une partition. Deux extrêmes Euler suffisent sans tri ; helper C++ isolé Release/UBSan contre-vérifié. Index LCA et rappel restent distincts. Croisement inter-K : limite future. [Note](audit_independant_20260929/ANCRAGE_AMBIGUITES.md). |
| Précision | Décision utilisateur : grille u32 par paliers u24 puis u32 complet. Primitives, filtre et ordre large isolés. Deux vrais niveaux q2 K3 coïncident en double : rang exact commun requis ; refus du comparateur à propager. Repli et raccord ouverts. h/profil/IDs distincts, Morton non persistant sous translation. [Contrat](AUDIT_MASSIF_LIDAR_20260930.md). |
| Dizaines de millions | Catalogue/atlas et états simultanés dominent. Boîtes certifiées, segments et plateaux globaux ; RAM, disque, sortie et reprise à définir. Aucun SLO massif ou capacité qualifiée. |
| Indices | RankSearch intégré ; atlas/représentants protègent la forêt FULL complète actuelle. Arène ExtCell commune à tous K : garde globale explicite recommandée, bornes par K insuffisantes comme justification scalaire ; aucune corruption géométrique reproduite. Réserves/types du port segmenté à élargir. [Arène](../receipts/audit_independant_20260930/developer_rebound/catalogue/README.md). |
| Condensation | Départs sous mcs confirmés. [Référence par cohortes](../receipts/audit_independant_20260930/developer_rebound/condensation_reference/README.md) : partitions conservées, 536 condensations exactes, Release/UBSan ; rangs égaux contractés avant masses. FULL inchangé ; transport du vote node_cluster ouvert. |
| Masses fractionnaires | Proposition actuelle cohérente au split. Préciser mcs aux splits seuls ou à toute densité : la masse progressive peut franchir le seuil entre deux événements. [Cas exact six points](../receipts/audit_independant_20260930/developer_rebound/fractional_review/README.md). Les candidats Pκ/A7/majorité attendent leur jugement, aucun gain statistique accepté. |
| Raccord CLI | Parseur/tête encore à unir ; collision labels/arbre persistante. Nouveau helper OutputSet R2 : fuite de descripteur et troncature sous exception observées par l'autre auditeur. RAII avant toute allocation après fopen, puis qualifier l'union. |
| Pool / tête | [Faits_math/juges clos dans la copie privée](../receipts/audit_independant_20260930/developer_rebound/integration_delta/README.md), 15/15 portes ; moteur compilé inchangé. Autres groupes en cours, conflits CLI/oracles encore à traiter. Vérifier le même binaire final après SiteTree/pool/CLI/oracles/tête. |
| Attache core | Nouveau trou de couverture observé dans l'audit géant : mutant `<` au lieu de `≤` survit aux portes et change des labels sur cinq points. Ajouter la fixture d'égalité, conserver propriétaire vivant après plateau. Pas de nouveau défaut HEAD allégué. |
| SiteTree | Numérique G1/arrondis contre-vérifiés en copie R2 ; preuve du chemin réellement exécuté insuffisante, refus dans le workflow. Réparation non close, aucune nouvelle erreur numérique démontrée. |
| Juges FULL | Petits juges R2 renforcés. Lecteur structurel massif : K effectif, sites attendus et tailles annoncées à vérifier ; il ne certifie pas seul Γ géométrique. |
| Bancs / CUDA | ARI hors domaine fermé dans R2 ; schéma incomplet. Lecteur CUDA : OverflowError et tentative sans journal restent ouverts. [État tête/bancs](audit_independant_20260929/CONTRE_AUDIT_TETE_BANCS.md). |

Tranche courante : [rebond sur le développement actif](../receipts/audit_independant_20260930/developer_rebound/README.md). Références isolées de condensation/Euler, précision du contrat fractionnaire, arène et vraies feuilles. [Ordre large/KNN](../receipts/audit_independant_20260930/wide_order_followup/README.md), [protection amont](../receipts/audit_independant_20260930/forest_cardinality/README.md) conservés ; première interprétation retirée reste archivée. [Géométrie/interfaces](audit_independant_20260929/CONTRE_AUDIT_GEOMETRIE.md).

## Dossier court, preuves conservées

Cinq notes actives : cette vue, massif/précision, frontière et deux contre-audits. Les [cinq rapports historiques](../receipts/audit_independant_20260929/historique/base_6206d1d11/README.md) ont quitté audits/ ; originaux et navigation hachés. Les [versions longues remplacées](../receipts/audit_independant_20260930/notes_avant_synthese/README.md) sont conservées exactement. Aucun ancien reçu ou rapport d'un autre auteur réécrit.

Preuves antérieures : [initiales](../receipts/audit_independant_20260929/), [majorité cinq sites](../receipts/audit_independant_20260930/tower_math/receipt.json), [massif](../receipts/audit_independant_20260930/massif/README.md), [précision](../receipts/audit_independant_20260930/precision_grille/). Aide immédiate : transmettre cohortes/plateaux et sens de mcs aux juges actuels, puis contrôler le raccord R2 final et l'égalité core. Port u24 puis u32 complet conformément à la [passation Claude](NOTE_CLAUDE_REPRISE_ET_PRECISION_20260930.md).
