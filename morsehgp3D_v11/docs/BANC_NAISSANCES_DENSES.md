# Ablation du lookup dense des naissances

Le bit FULL 512 active `FullParams::dense_birth_lookup`. Le banc accepte les
masques 0..1023 ; le bit 128 exige toujours le bit 8, soit 768 combinaisons
légales. Le défaut reste inactif. Cette option change la recherche d'un
`BirthSeed` dans la forêt ; elle ne change ni les descentes, ni les dates,
ni les unions, ni les verticales.

Chaque ordre publie `dense_birth_lookup` et `lookup_reserved_bytes`, lus sur
la forêt propriétaire. En mode sparse, la réservation vaut exactement huit
octets par naissance. En mode dense, elle vaut quatre octets par site pour
K1, quatre octets par boule du catalogue pour les autres ordres. L'événement
FULL publie également la route demandée et la somme de ces réservations.
Le collecteur vérifie les types, chaque valeur, la somme et son inclusion
dans la mémoire retenue totale avant tout décodage ou réemploi sémantique.
Il ne suppose pas que la table dense soit toujours plus grande ou plus petite.

`full_dense_diagnostics.py` porte le schéma
`ehgp.v11.full_dense_birth_lookup.v1`. Les rapports courants deviennent
`full_campaign.v10` et `full_parallel_campaign.v6`. Les anciens lecteurs
figés et captures ne changent pas. Les masques de comparaison du travail
399 et 143 ignorent le bit 512 : tous les champs `work`, y compris les
compteurs du mémo et du census, doivent rester identiques entre 511 et 1023.
Les règles de comparaison des deux voies du census restent inchangées.

Le calendrier exclusif `--dense-births` comporte vingt processus : six
paires LiDAR 511/1023, couvrant trois trames entières en u21/u24 avec W48 ;
ng00 u21 mode 1023 avec W1 et W8 ; trois paires uniformes u21 à 8k/16k/32k.
Une tentative en échec ne supprime pas sa paire. Les seules omissions
possibles viennent du budget et sont explicites. Les anciens calendriers
19/27/20/29 restent disponibles avec leurs propres options exclusives.

Le plan `bench/plans/full_dense_g4.json` demande la matrice complète (850 s),
le complément ASan18 (180 s), puis le banc (570 s, budget propre 500 s).
Le total de 1600 s plus 120 s de préparation déclarée vaut 1720 s. Les
gardes de session, la qualification préalable, les intentions persistées,
le plafond de résultats de 16 MiB et les contrôles du réemploi restent actifs.

Les tests factices conservent les 384 modes précédemment couverts et ajoutent
512/519/527/639/767/1023. Ils se nomment `TESTED_MODES` : cette liste ne
prétend pas exécuter nativement les 768 combinaisons. Le nouveau collecteur
teste notamment les réservations par ordre, la somme, la route sparse,
les refus avant décodage ou hit, des paires avec/sans mémo et census emprunté,
les divergences de travail malgré des sorties égales, les interruptions et
les omissions budgétaires. La petite porte native reste réservée à G4.

Aucune mesure de gain ni qualification native n'est acquise par ces modèles.
Le réemploi structurel des graines entre deux ordres est une tranche séparée.
