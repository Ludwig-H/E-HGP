# Oracle idéal des deux triangles et cible des points

30 septembre 2026. [Source géométrique et figures de thèse](../thesis_geometry/README.md).
Six observations nommées A..F, r₀=1. Arithmétique exacte dans Q(√3), distincte
du moteur entier et de sa factory de coordonnées. Aucun appel natif dans ce
paquet ; aucune conversion `int` ou décision flottante sur les coordonnées.

[check.py](check.py) énumère les 63 parties non vides et leurs MEB : tous les
supports de taille ≤3 dans le plan, cercle minimal contenant la partie.
Γ₂ est ensuite le nerf complet des 15 paires : deux fibres s'intersectent
si la MEB de leur union tient dans le rayon. Les unions de quatre points
sont incluses, et aucune règle candidate ne fournit les composantes attendues.

Les sept premières fibres AB, AC, BC, CD, DE, DF, EF naissent à β=1.
Les deux triangles se forment à β=4/3 ; la fusion globale des trois
composantes arrive à β=2+√3. Entre ces dates, FULL couvre **ABC, CD, DEF**,
avec C et D partagés. La cible exclusive est **ABC | DEF**.

Sur les témoins incidents de bande, une majorité strictement supérieure à
1/2 atteint cette cible à la naissance des triangles : C reçoit 2/3 dans
ABC, D reçoit 2/3 dans DEF, marge de vote 1/6. Uniforme et inverse-β sont
identiques dans l'idéal. η=0 et η=1/8 sélectionnent ici les sept mêmes paires.
L'univers et le dénominateur sont fixés avant activation ; les témoins
futurs ne sont pas renormalisés et l'absence de gagnant donne un singleton.

Six niveaux du nerf, 14 coupes, 16 contrôles de cible et 52 emboîtements
adjacents passent. Toutes les dates d'événement et intervalles intermédiaires
sont couverts, notamment les rayons 1,3 et 1,7. Ce résultat porte sur ce
cas idéal, pas sur la stabilité générale, les grands K ou la qualité LiDAR.
L'admission des sept paires dans la bande est particulière à cette géométrie.

**Deux unités de masse à garder distinctes :** après majorité dure, les deux
branches portent 3 points chacune. Avant durcissement, avec activation par
marches et poids uniformes de bande normalisés par observation, les masses
FULL sont **8/3, 2/3, 8/3** pour ABC, CD, DEF, somme 6. Un seuil de masse 3
éliminerait donc les deux branches fractionnaires ; un seuil 2 les admettrait
et éliminerait le pont. Ce calcul ne concerne ni la normalisation complète
par cofaces du manuscrit ni la variante progressive. Il fixe un contrôle
de protocole ; il ne rejoue pas leur sélection.

[normal.json](normal.json) et [optimized.json](optimized.json) sont identiques.
Reproduction : `python3 -B check.py` et `python3 -B -O check.py`. Le ledger
ferme le code et les résultats ; les dates sont encodées `[a,b]` pour a+b√3.
La [porte de condensation](../condensation_gate/README.md) vérifie séparément
la tête sur l'arbre de points prescrit, sans qualifier sa production.
