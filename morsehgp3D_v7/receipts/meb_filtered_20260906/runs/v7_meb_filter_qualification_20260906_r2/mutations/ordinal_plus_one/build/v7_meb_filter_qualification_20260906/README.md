# Qualification locale du filtre MEB — 6 septembre 2026

`phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`. GCP non utilisé.

Cette campagne reprend explicitement le helper préparé `484a89bc`, inchangé,
le différentiel historique `0645aa00`, ses formes exactes `d6dbba19` et
le repli F `f75a136a`. Elle ne modifie ni le produit FULL, ni son dispatch,
ni ses plafonds. Les fichiers de préparation du 5 septembre restent intacts.

`geometry_gate.cpp` dérive de la porte `c9971f8c` : mêmes données,
384 ordres, matrice de 9 216 appels, 128 frontières et 1 507 ordinaux.
Les 123 frontières historiques sont conservées, avec les attendus P7
du nouveau calendrier ; quatre étapes P3 exercent à nouveau l'épuisement
au milieu de la deuxième MEB.
Une frontière q4 supplémentaire exerce le rang 550 à c=MAX−550 et L=MAX.
Le calendrier attendu change ; chaque appel confronte maintenant F,
le filtre avec Trace et le filtre natif NoObserver. Les primitives de F
restent communes : ce différentiel n'est pas un oracle géométrique indépendant.
`budget_gate.cpp` et `trajectory_gate.cpp` ajoutent des portes ciblées.
Le juge rationnel de l'auditeur est réutilisé explicitement, sans exécuter
ses anciens runners ni écrire dans son dossier.

Qualification en préparation : les nombres ci-dessus sont des planchers
attendus, pas des résultats. Les captures devront fixer sources, dépendances,
commandes, statuts et mutants. Aucune latence ni intégration n'est promise.

## Complément R2, après clôture du premier lot

La preuve corrigée du plan radical impose une base positive unique dans le
pivot admissible. La sentinelle à deux bases reste explicitement hors Q
positif. Une sentinelle admissible supplémentaire du tétraèdre régulier
vérifie que l'ordre q4-first conserve le support mais change P : huit
appels locaux, six appels natifs et un rejeu global côté test, rapportés
séparément des 180 appels natifs initiaux. Le mutant est rejeté pour le
calendrier après contrôle de l'égalité des supports, jamais pour une
ambiguïté native fictive. R1 et ses sources sont conservés sans réécriture ;
la seconde campagne part d'un répertoire neuf. Ces ajouts ne modifient pas
le helper préparé `484a89bc`, ni aucun fichier produit.
