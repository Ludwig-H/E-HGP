# Dialogue courant de l’auditeur indépendant A v8

14 septembre 2026, sur main. Écritures limitées à ce dossier.
`phase=exploration_v8_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`. GCP non utilisé.

## Petits rectangles après Pool : conserver le chemin actuel

Le [nouvel essai sur les racines singleton](q2_small_roots_20260914/README.md)
compare Complement, Global itératif et Global récursif, après le même
Pool/paires64. Changement limité aux rectangles initiaux A=B=singleton,
avant tout crédit ; aucun descendant ni préfixe Pool modifié.

Les 360 flux du gate passent l’oracle indépendant en Release et Clang
ASan/UBSan. Les 30 mesures conservent front, Pool et supports complets.
À LiDAR50k, Global élimine 26,18 millions d’opérations structurelles mais
ajoute 24,11 millions de visites géométriques. Le total passe de 11,027 s
à 11,304/11,275 s ; la répétition inversée ne montre pas de gain non plus.
Les tailles 8k/16k/32k et s10/12 ne justifient pas de changement général.
**Ne pas ajouter cette variante au raccord Pool sur cette seule intuition.**

Les préparations singleton n’utilisent déjà pas les constantes de 48 octets
et le certificat frère n’y intervient déjà pas. La simplification déplace
surtout le coût entre parcours structurel et tests de boîtes. Après Pool,
continuer à viser une réduction du travail géométrique et du front ; les
chronomètres actuels ne séparent pas le temps propre du front du comptage.
Les résultats portent sur e3af11a7 plus l’adaptateur A ; le port Pool en
cours chez le constructeur devra conserver ses propres preuves.

Une suite constructive est détaillée dans la même note : pour une paire
fixée, une boîte de maximum H nul contient au plus un site de coquille,
à rechercher après admission. Un tampon borné de descripteurs pourrait
éviter une partie de la seconde collecte, avec repli global au débordement.
Les fixtures distinguent tangence réelle et tangence sans site ; aucun
gain n'est encore mesuré et ce cache n'est pas implémenté.

## Entretien et preuves utiles

Les détails du raccord Pool déjà repris dans les documents du constructeur
quittent ce dialogue. Le [prototype et ses reçus](q2_pool_bridge_20260914/README.md)
restent la référence du gain précédent. Les audits d’[ordre LiDAR](q2_order_lidar_20260914/README.md),
de [relais conjoint](q2_product_20260914/README.md) et de
[plans partagés](P0_SOUS_RECTANGLES_ET_GROUPES.md#95-partager-le-plan-parent-puis-découper-ses-tâches)
restent consultables à leur chemin ; les reçus Rectangle/Tubes utilisés
par leurs reproductions ne sont pas déplacés. Fichiers B, constructeur
et auditeur complémentaire préservés. P0, q3/q4, FULL, multi-CPU/GPU,
contrats de tour 50k et régime multi-millions restent ouverts.

Réservation courte d’index A : ce dialogue et `q2_small_roots_20260914/`
seulement. Elle est close dès publication du commit correspondant sur main.
