# Tête et bancs — limites courantes des conclusions

2 octobre 2026. Produit u18 inchangé ; variantes vote/maturité dans des
prototypes séparés. `public_status=not_claimed`. Aucun GCP dans cet audit.

| Sujet | État utile au développeur |
| --- | --- |
| Condensation | Correction par cohortes requise : fermer dès que la masse active passe sous mcs, aplatir au plateau parent. [Oracle et sondes historiques](../../receipts/audit_independant_20260930/developer_rebound/condensation_reference/README.md). Une copie R2 testée n'est pas le produit publié. |
| Vote rapide v2 | Les deux défauts de cumul/rang ont été corrigés et contre-vérifiés par l'autre auditeur. Les campagnes vc1/vc2/vc3b/vc4 gardent leurs anciens pins ; le contrôle d'impact borné ne les requalifie pas toutes. [État et pins](../audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md#correctifs-courants-et-réponses-q13-q14-q15). |
| Nouvelle maturité | Tests d'existence et de projection à distinguer de l'EOM. R utilise une validation future et assume ses sauts ; M n'est stable que relativement à son entrée. [Mathématique](ANCRAGE_AMBIGUITES.md). |
| Dates | Le nouveau helper donne les bons rangs dans nos 90 exécutions, y compris collisions double. Ce contrôle scalaire ne vérifie pas les poids/stabilités de toute la tête. [Preuve](../../receipts/audit_independant_20261002/date_order_review/README.md). |
| MAP / métriques | Référence iid réparée : composante à prior positif conservée même sans tirage. Distinguer marginal exact, iid exact et modèle plug_in. Comparaisons directes : annoncer aussi le masque void et les cardinalités candidates. [Revue](../../receipts/audit_independant_20261002/battery_review/README.md). |
| HDBSCAN | Témoin officiel, même machine/entrée/version ; sensibilité aux ordres annoncée séparément. Les vrais plateaux d'un graphe pondéré fixé donnent une multifusion ; les nœuds binaires transitoires ne sont pas des composantes persistantes nouvelles. |
| Raccord / bancs | Six portes CTest closes en privé, Pool encore non conforme. Les anciens défauts CLI/schémas et leurs corrections doivent être jugés dans l'union finale. [Revue R2](../audit_continu_20260929/CONTRE_AUDIT_R2_20260930.md). Aucun FULL GPU/100 ms ou capacité 10–50 M déduit. |

Les résultats anciens A/C ne sont pas annulés par un défaut détecté sur
une autre API, mais leur périmètre historique reste explicite. Les nouveaux
rapports sont des diagnostics de développement, pas un test final scellé.
Bruit ignoré, meilleur bloc libre, antichaîne compatible et coupe commune
répondent à des questions différentes ; publier chaque univers.

Une simple conversion d'unité multiplie toutes les stabilités définies
par le même facteur h^(−z), à arbre/masses/z identiques. Requantification,
fusion de dates ou nouvelle politique de masse peuvent changer EOM.

[Preuves historiques](../../receipts/audit_independant_20260929/historique/base_6206d1d11/TETE_BANCS_PREUVES.md),
[anciennes copies](../../receipts/audit_independant_20260930/notes_avant_synthese/README.md).
Nos deux contre-audits sont actualisés ; aucun ancien reçu réécrit.
