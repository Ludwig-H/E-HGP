# Contre-audit B — index exact des clés FULL publié

23 septembre 2026. Pin produit `02d55856`, cadre
`exploration_v9_hors_registre`, CPU, grille u18 1 mm,
`not_claimed`. La modification remplace deux recherches dichotomiques
dans la tour FULL par un adressage ouvert des `BallKey` du catalogue.
Elle ne modifie pas q2/q3/q4, le catalogue lui-même ni la sémantique
des ordres ; elle n'est pas une voie GPU.

## Correction et porte rejouée

Après contrôle de l'unicité du catalogue, l'index alloue une table de
puissance de deux avec au moins deux cases par boule. Chaque entrée
stocke `BallId+1` (`0` signifie vide) et s'insère par CAS ; les cases
ne sont jamais vidées. Pour déplacer une clé au-delà d'une case, son
insertion doit l'avoir déjà vue occupée. Un `find_key` postérieur aux
jointures ne peut donc rencontrer une case vide avant une clé présente.
Une collision de hachage n'est pas une égalité : la recherche compare
la **clé entière** avant de rendre un ID. Le hash couvre les cinq
coefficients signés de 128 bits, par leurs deux mots non signés ; clés
égales impliquent hashes égaux. `balls.size() ≤ UINT32_MAX` est vérifié
avant la construction : `BallId+1` tient dans `u32`, et la sentinelle
de retour `UINT32_MAX` ne peut être un ID valide. `parallel_ranges`
joint les fils avant toute lecture ordinaire de la table ; les CAS
relâchés ne créent donc pas de lecteur concurrent. La disposition peut
varier avec l'ordre des fils, pas le résultat de la recherche.

Rejeu indépendant du commit dans un build Release neuf
(`/tmp/mhgp9-keyindex-audit.6aPqX1`, Boost épinglé) : les quatre
CTest `mhgp9_tower_full_ball_tower`,
`mhgp9_tower_full_ball_static_cpu1`,
`mhgp9_tower_full_ball_static_cpu4` et
`mhgp9_tower_key_index_mutant_drop_first` passent (**4/4**, 9,91 s).
Le mutant compilé qui omet la première clé termine avec code 1,
`FAIL [post_seed_ABEZW/variant0] paired.complete_towers` : le juge
T2 détecte causalement une tour incomplète. Ce contrôle est plus fort
qu'un simple condensé inchangé ; il reste borné à ses petites fixtures.

## Portée du gain et angles morts

La coordination rapporte sur 08/000000/K10/W8 local, MEB proposé,
une tour de 18,5/17,7 s avant à 16,9/16,4 s après, soit environ
8 % de gain local, avec digest égal. **Aucun reçu apparié archivé**
n'expose les répétitions brutes, le coût de construction de l'index,
ses longueurs de sondage ou l'effet sur la chaîne FULL entière ; R6
est antérieur et CPU seul. Ne pas transférer ce pourcentage à G4.

À 5,5 M boules, la table vaut environ 64 Mio (`u32 × 2^24`), en
plus du catalogue et des structures FULL. Le facteur de charge
inférieur à 1/2 n'est **pas** une borne de temps par clé : des
collisions concentrées donnent un sondage linéaire long et un cas
quadratique. Cela ne condamne pas les trames LiDAR pertinentes, mais
impose de publier, par trame et K, au moins construction CPU/mur,
capacité, collisions, moyenne/max/p95 des sondages, RSS et temps des
recherches. Une fixture de collision forcée, W1/W4 et sous TSan
fermerait le trou de couverture concurrente. Vérifier également
la forte croissance en mémoire pour les régimes de plusieurs dizaines
de millions de points, sans extrapoler 5,5 M boules à ces nuages.

Le verrou de la seconde reste q3/q4 **et** l'aval : ce gain local est
utile, mais R6/K10/000100 avait encore 5,32 s de q3/q4 et 4,00 s de
tour. La preuve 8k/16k/32k de sous-quadraticité q3/q4 sur coupes
LiDAR appariées n'a pas été publiée par ce commit.
