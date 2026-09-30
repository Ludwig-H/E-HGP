# Audit indépendant v10 — décisions courantes

30 septembre 2026. Produit 4b7d70422 ; preuves relues jusqu'à d192637a7. RankIndex intégré, bande externe et primitives u32 isolées. Les corrections R2 du pool, de la tête et des CLI restent séparées. public_status=not_claimed. Aucun GCP.

## À traiter avec le développeur

| Sujet | État et prochaine décision |
| --- | --- |
| Frontière → points | Cible actuelle : un seul K fixé. Antichaîne avant LCA contre-vérifiée : couverture, laminarité et monotonie avec η conservées, certaines entrées avancées. Incidences internes conservées. Croisement K2/K3 : limite d'une combinaison future, pas blocage actuel. Rappel, masses et condensation à comparer. [Note mathématique](audit_independant_20260929/ANCRAGE_AMBIGUITES.md). |
| Précision | Décision utilisateur : grille u32 par paliers u24 puis u32 complet. Primitives, filtre et ordre large isolés. Deux vrais niveaux q2 K3 coïncident en double : rang exact commun requis ; refus du comparateur à propager. Repli et raccord ouverts. h/profil/IDs distincts, Morton non persistant sous translation. [Contrat](AUDIT_MASSIF_LIDAR_20260930.md). |
| Dizaines de millions | Catalogue/atlas et états simultanés dominent. Boîtes certifiées, segments et plateaux globaux ; RAM, disque, sortie et reprise à définir. Aucun SLO massif ou capacité qualifiée. |
| Indices | RankSearch intégré dans 4b7d70422 ; [notre porte indépendante](../receipts/audit_independant_20260930/rank_search_preintegration/README.md) passe. Garde catalogue proposée en copie R2 ; atlas/représentants protègent la forêt FULL complète actuelle, réserves à promouvoir. Ces invariants doivent rester globaux dans le port segmenté. Refus avant conversion ou IDs globaux plus larges. |
| Condensation | Nouveau défaut confirmé de tête : les départs de points peuvent laisser une branche sous min_cluster_size sans la clore. Stabilité K3 réelle excessive, inversions EOM sur API valide. Corriger les cohortes de rang exact avant comparaison statistique. [Lecture indépendante](../receipts/audit_independant_20260930/point_condensation_followup/README.md). |
| Raccord CLI | Parseur strict et tête numérique dans des copies différentes. Collision labels/arbre encore reproduite ; Outcome avant sortie confirmé dans la copie tête. Vérifier l'union sur un binaire stable. |
| Pool / tête | Sept groupes R2 sauvegardés, 726 payloads intacts. Les 13 portes observées servent u18/helpers, pas leur union. Pool accepté avec réserves ; vérification tête non terminée. Intégration commune ouverte. |
| SiteTree | Numérique G1/arrondis contre-vérifiés en copie R2 ; preuve du chemin réellement exécuté insuffisante, refus dans le workflow. Réparation non close, aucune nouvelle erreur numérique démontrée. |
| Juges FULL | Petits juges R2 renforcés. Lecteur structurel massif : K effectif, sites attendus et tailles annoncées à vérifier ; il ne certifie pas seul Γ géométrique. |
| Bancs / CUDA | ARI hors domaine fermé dans R2 ; schéma incomplet. Lecteur CUDA : OverflowError et tentative sans journal restent ouverts. [État tête/bancs](audit_independant_20260929/CONTRE_AUDIT_TETE_BANCS.md). |

Tranche courante : [antichaîne à K fixé](../receipts/audit_independant_20260930/fixed_k_antichain/README.md), [ordre large et coupe KNN](../receipts/audit_independant_20260930/wide_order_followup/README.md), [cardinalités et protection amont](../receipts/audit_independant_20260930/forest_cardinality/README.md), [sauvegarde/portes R2](../receipts/audit_independant_20260930/evidence_followup_20260930/provenance_408d1ffe4/README.md). Compilations, lectures et exécutions distinguées ; première interprétation des cardinalités retirée et conservée. [Géométrie/interfaces](audit_independant_20260929/CONTRE_AUDIT_GEOMETRIE.md).

## Dossier court, preuves conservées

Cinq notes actives : cette vue, massif/précision, frontière et deux contre-audits. Les [cinq rapports historiques](../receipts/audit_independant_20260929/historique/base_6206d1d11/README.md) ont quitté audits/ ; originaux et navigation hachés. Les [versions longues remplacées](../receipts/audit_independant_20260930/notes_avant_synthese/README.md) sont conservées exactement. Aucun ancien reçu ou rapport d'un autre auteur réécrit.

Preuves antérieures : [initiales](../receipts/audit_independant_20260929/), [majorité cinq sites](../receipts/audit_independant_20260930/tower_math/receipt.json), [massif](../receipts/audit_independant_20260930/massif/README.md), [précision](../receipts/audit_independant_20260930/precision_grille/). Suite développeur : correction de condensation, puis rappel frontière à K fixé, raccord commun R2 et port u24 ; préparer ensuite u32 complet. Le choix grille est confirmé dans la [passation Claude](NOTE_CLAUDE_REPRISE_ET_PRECISION_20260930.md).
