# Rejet de blocs et saturation : qualification locale

6 septembre 2026, `public_status=not_claimed`, CPU/u16. Prototype privé,
non intégré au producteur et non chronométré sur les grands nuages.

Le [helper](nominal/histogram_negative.hpp) combine crédits de sous-arbres,
rejets q3/q4 à ancre et point opposé fixés, puis saturation exacte à need.
Il rend min(histogramme exhaustif,need), sans arrêter après need tests.
Les positions encore non parcourues après saturation ont leur propre
compteur ; le grand-livre logique reste fermé. Voir la [preuve du rejet](../../audits/receipts_terminal_count_20260906/README.md).

O2 et ASan/UBSan passent chacun 432 comparaisons contre les histogrammes
scalaires et un prédicat entier indépendant. Les deux stdout sont identiques :
2 400 paires rejetées négativement, 6 964 positions non parcourues après
saturation, 260 blocs crédités avec écrêtage. Quatre cas q3/q4 vérifient
un point opposé strictement intérieur à sa boîte et absent des sites.
Les égalités strictes, la diagonale, les grands carrés u16/i128 et
need=0/1/2/8/10/MAX sont exercés.

Les arguments inconnus rendent 2. Le mutant physique qui remplace Xi_min
par Xi_max compile puis échoue avec code 1 et cause exacte
`negative.true_witness_preserved`. Ce mutant vise la conservation d'un
vrai témoin ; aucun mutant de saturation compilé n'est revendiqué.

Le [reçu](receipt.json) conserve les huit commandes closes, leurs codes,
les hashes et la stabilité des sources. Dernière fermeture :
`2026-09-06T12:49:59.786301+00:00`. Attentes directes sans nouveau quota
temps/CPU/fichier. Aucun processus ni session GCP laissé actif.

Publication par copie des captures, sans relance : logs, sources nominales
et mutantes, helper positif, recordeur et hashes. Les trois ELF sont exclus ;
leurs identités restent dans les captures. Les autres headers produit
sont épinglés dans les depfiles/hashes et déjà conservés dans le
[snapshot précédent](../wspd_histogram_blocks_20260906/source_snapshot/).
Les chemins de compilation du recordeur sont historiques privés, pas un
nouveau lanceur portable. PREPARATION.md.txt conserve la note de travail
octet pour octet ; ses liens historiques ne sont pas des liens actifs.

À la demande de minimiser le budget, aucun benchmark lourd supplémentaire
n'est lancé pour ce delta. Le gain de temps et sa croissance restent à mesurer.
