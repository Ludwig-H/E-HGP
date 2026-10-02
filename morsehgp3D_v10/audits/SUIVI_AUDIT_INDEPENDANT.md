# Audit indépendant v10 — décisions courantes

2 octobre 2026. Lecture des travaux jusqu'à `afb081774` et des prototypes
locaux actuels. Produit u18 inchangé depuis `4b7d70422` ; les copies R2,
vote et maturité restent distinctes. `public_status=not_claimed`.
Aucun moteur modifié, lancement GCP ou allocation massive par cet audit.

**Priorité : présence des cibles dans FULL, puis dans une hiérarchie de
points compatible. Sélection et z restent différés.** Le worktree v11
est préparé ; `morsehgp3D_v11/` n'existe pas encore à notre lecture.

| Sujet | Décision utile au développeur |
| --- | --- |
| Frontière | Garder ABC\|DEF avant fusion dans les deux triangles. La taille de cœur améliore les petites masses mais échoue cette cible ; la taille couverte passe les fixtures et conserve trop de petites branches. Aucune règle universelle acquise. [Réponse actuelle](REPONSE_CLAUDE_BATTERIE_TOUR_HIERARCHIES_20261002.md). |
| Maturité | Opérateur causal stable relativement à une projection stable ; il peut supprimer des blocs. La maturité géométrique compte parfois un site dans deux branches : cela ne garantit pas deux blocs exclusifs de mcs membres. [Contrat et témoin minimal](../receipts/audit_independant_20261002/maturity_review/README.md). |
| Dates exactes | Nouveau helper capturé conforme : 90 exécutions, 2 880 clés et 2 430 contrôles de raffinement normal/−O ; deux mutations causales rejetées. Conserver rangs et égalités exacts jusque dans les dates ajoutées. [Reçu](../receipts/audit_independant_20261002/date_order_review/README.md). |
| Comparaisons | Rapports récents exploratoires, pas victoire générale. Distinguer vérité latente, MAP, bruit ignoré et masque void ; publier taille et univers de chaque famille. [Revue indépendante](../receipts/audit_independant_20261002/battery_review/README.md). |
| LiDAR massif | Dimensionner les phases simultanées : catalogue/FULL, attaches, extraction de points et scratch nearest. Aucun contrat 10–50 M acquis. [Contrat actualisé](AUDIT_MASSIF_LIDAR_20260930.md). |
| Précision | Grille u32 par paliers u24 puis u32 complet ; pas physique, IDs et repère commun explicites. Primitives larges isolées, moteur FULL encore u18. Refus du comparateur à propager ; double réservé aux valeurs approchées. |
| Raccord R2 | Six portes CTest terminales dans la copie privée ; lot Pool encore code1 et mutant MR1 survivant. Ne plus présenter les lots comme RUNNING, ni les CTests comme qualification globale. [État recoupé](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md#sessions-g4-tvpc1tvpc2-et-intégration-r2--état-terminal-courant). |

Mes cinq notes actives sont cette vue, massif/précision, frontière et
les deux contre-audits. Elles remplacent leurs états anciens ; les reçus
figés et les travaux des autres auditeurs restent conservés. Les anciennes
preuves demeurent dans [les reçus](../receipts/audit_independant_20260930/),
sans réintroduire de rapports périmés dans `audits/`.
