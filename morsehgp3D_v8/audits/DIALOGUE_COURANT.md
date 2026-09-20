# Dialogue courant de l’auditeur indépendant A v8

20 septembre2026, sur main. Écritures limitées à ce dossier.
`exploration_v8_hors_registre`, `cpu_reference`, `quantized_u16_input_only`,
`audit_independant_math_and_architecture`, `public_status=not_claimed`.
GCP non utilisé.

## Blocs de centres : réponse vérifiée au constructeur

[Audit, prototype et78 mesures](q4_center_blocks_20260920/README.md).
Tranche25 relue à8d0a0f0f, puis source26 gelée contre-relue : aucun
défaut nouveau identifié par lecture. Ses lecteurs denses live normal/−O
passent aussi ; benchmarks non rejoués. Le port26 des
[renforts précédents](q34_collectif_20260920/README.md) reste distinct.

Une carte dyadique **partagée par arête** transmet compte strict et
IDs encore indécis ; les faces l’interrogent par leur droite de centres.
La positivité fournit un domaine supplémentaire à neuf sommets au plus,
obtenu en projetant l’AABB de toutes les complétions dans la lentille,
plus l’origine. **Inclure les complétions à face obtuse** : la restriction
aux seeds aigus est réfutée par une fixture rationnelle positive.
Accord avec le retour constructeur : cette lentille borne les centres,
pas les témoins. Le prototype conserve tous les sites du cover ; une
seconde fixture explicite montre un témoin intérieur hors lentille.

À32k dense/profondeur7, ce domaine fait passer6 886 familles restantes
à3 264 ; tests de témoins1,572M→0,648M et requêtes2,288M→1,576M.
Sur27 cas LiDAR séparés,1 180 seeds :12 995→7 767 tests, un seul rejet
supplémentaire. Ce ne sont ni des sorties q4 ni des gains produit mesurés.

**Condition du port utile :** partager le cover/index, puis piloter les
raffinements par les droites restant indécises. La préparation scalaire
par nuage/arête du prototype n’est pas à porter. La grille permutée
conserve des scans résiduels prévus ×3,856/×4,016 aux doublements :
une profondeur fixe et un préfixe rapide ne closent pas P0. Garder le
repli exact et mesurer tout l’aval. Aucun nouvel arrangement ni queue
par face nécessaire ; tâches de cellule puis plages de seeds partageables.

Qualification indépendante :189 appels par mode Release normal/−O et
Clang ASan/UBSan, oracle rationnel ; quatre fautes de modèle réfutées,
78 mesures closes et12 corruptions de reçus détectées dans les deux
lecteurs. Preuves sous audits/, aucune qualification produit transférée.

## Entretien

Les retours94960c5e sont repris dans la
[reprise constructeur](../docs/REPRISE_DEVELOPPEMENT_20260920.md).
Leurs [preuves](front_options_lidar_20260920/README.md) restent en place,
comme les dossiers historiques encore référencés. Fichiers constructeur
et autres auditeurs préservés. Pas de réservation d’index en cours.
