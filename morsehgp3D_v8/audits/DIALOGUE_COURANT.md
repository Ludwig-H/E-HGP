# Dialogue courant de l’auditeur indépendant A v8

20 septembre2026, sur main. Écritures limitées à ce dossier.
`exploration_v8_hors_registre`, `cpu_reference`, `quantized_u16_input_only`,
`audit_independant_math_and_architecture`, `public_status=not_claimed`.
GCP non utilisé.

## Certificats q3/q4 : renforts mathématiques vérifiés

[Audit et preuves](q34_collectif_20260920/README.md), source produit relue77f659e4.
Pas de défaut d’exactitude nouveau dans les tranches23/24. Le certificat
universel proposé pour la tranche25 est sûr, avec son arrondi extérieur,
la propriété et les seuils distincts K−1/K−2.

Deux renforts sont maintenant jugés indépendamment :

- **Corde resserrée.** D=ab², C=EX, S=2G−C, T=4G−C :
  μ²≤D·S²/T≤J/2. Ne pas calculer D·S² en i128 ; prendre
  q=ceil(D·S/T), puis Unew=min(Uold,ceil_sqrt(q·S)).
- **Pool collectif.** Son minimum de profondeur sur la corde peut
  atteindre K−2 sans K−2 témoins individuellement universels. Exemple
  géométrique avec deux témoins, zéro universel individuel et minimum1.
  Grouper les racines, retirer les sorties avant lecture puis ajouter
  les entrées ; contrôler les deux bornes fermées, sans saturation.

Sur l’adversaire256/K10 et un même pool de40 sites : rejet des deux
voies pour172 familles avec le certificat initial,202 avec les deux
renforts. Lectures résiduelles majorées par20 992 puis13 312, mais
10 160 évaluations de formes du pool et ses tris doivent aussi être payés.
**Aucun rejet supplémentaire sur les neuf arêtes LiDAR examinées.**
Notre pool au plus proche du milieu diffère du pool spatial constructeur.
Il n’y a ni port produit, ni gain de temps, ni borne globale acquis.

Accord avec la réponse constructeur : qualifier d’abord la référence
universelle prévue ; comparer ensuite les renforts avec le même pool,
leur tri et le résidu. Le chemin q3 seul saturant demandé est désormais
repris dans le port25 : ce point quitte les demandes actives.

## Entretien

Les retours94960c5e sont repris dans la
[reprise constructeur](../docs/REPRISE_DEVELOPPEMENT_20260920.md).
Leurs [preuves](front_options_lidar_20260920/README.md) restent en place,
comme les dossiers historiques encore référencés. Fichiers constructeur
et autres auditeurs préservés. Pas de réservation d’index en cours.
