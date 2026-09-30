# Sorties CLI R2 : contre-épreuve de collision

30 septembre 2026. Deux appels natifs très courts, sur cinq sites, K2,
min_cluster_size=2 et un worker. Le contrôle écrit 20 octets d'étiquettes.
Avec la même destination pour les étiquettes et `--tree`, code 0/status ok
mais le texte d'arbre de 126 octets écrase ces étiquettes.

Le rapport, script, coordonnées et reçus sont conservés octet pour octet,
avec leur manifeste fermé. Aucun moteur modifié, pas de compilation/GCP.

**Correction de portée du rapport historique.** Sa conclusion d'absence
de build tête R2 ne tient pas : l'énumération initiale était trop étroite.
Le complément `../head/` observe le build et rejoue sa porte native avec
code 0. Les deux événements ne sont pas une qualification d'un binaire
commun : l'interface et la tête restent des copies distinctes.

Les refus de la nouvelle tête doivent être propagés avant d'accéder aux
tableaux d'étiquettes, vides en cas de refus. La copie tête conserve cette
propagation ; sa CLI doit être raccordée avec les écritures robustes de la
copie entrées, et refuser les destinations concurrentes avant réservation.
