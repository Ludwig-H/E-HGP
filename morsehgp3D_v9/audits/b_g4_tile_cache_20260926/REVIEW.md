# Revue indépendante du cache S2 par tuiles — 26 septembre 2026

Base lue : `6d573b58b`. Revue de conception pendant le port, pas encore
validation du CUDA ni nouveau résultat G4. Cadre `not_claimed`.

Cette note est la revue **avant exécution**, conservée comme telle.
Le [rapport après G4](../AUDIT_B_CACHE_S2_G4_20260926.md) et le
[reçu](../../receipts/g4_tile_cache_20260926/README.md) donnent désormais
les résultats du port mesuré ; ils ne qualifient toujours pas FULL.

## Invariants à conserver

Pour un rectangle survivant de facteurs A et B, une tuile couvre une
seule ligne et au plus 32 colonnes :
`tiles=|A|*ceil(|B|/32)`, `p=offset_pairs+row*|B|+32*tile_col+lane`.
La dernière tuile de chaque ligne ignore ses lanes hors B. Chaque indice
de paire est écrit exactement une fois ; les trous ne passent jamais au
compactage. Le représentant, colonne `32*tile_col`, fournit son propre
masque : ne pas refaire son parcours dans la deuxième passe.

La trace est une antichaîne **par voie**. Un parent crédité pour q3 et un
descendant crédité pour q4 peuvent coexister ; leurs masques séparent
leurs crédits. Chaque nœud tracé a apporté au moins un crédit positif, et
la somme des crédits effectifs est au plus `(K-1)+(K-2)` : 17 entrées à
K10 suffisent. Pour une autre paire, retester les bornes strictes de
chaque nœud ; jamais reprendre le masque de rejet du représentant.
`Hmin>0` interdit que l'un des nouveaux endpoints soit compté comme
témoin. Chaque voie restant ouverte repart de zéro dans le DFS complet.

Un échec de pile ou un débordement de trace interdit toute réussite
partielle. Tous les threads d'un warp doivent rejoindre les réductions
utilisant un masque plein ; une fin anticipée par lane ne convient pas.

## Entiers et mémoire

Le front véritable partitionne les paires, mais `FilterInput` public
autorise des rectangles répétés ou chevauchants. Le produit d'un rectangle
tient en u64, pas nécessairement leur somme. Vérifier les additions avant
le scan ; vérifier aussi le total des tuiles, les tailles d'allocations et
chaque conversion au domaine int de CUB. Le budget mémoire doit inclure
les traces, préfixes de tuiles et scratch, pas seulement les masques.

Les rectangles B=1 n'offrent aucune réutilisation ; ils risquent de payer
une trace par paire pour aucun gain. Publier le nombre de représentants,
la distribution des tailles de tuiles et le stockage. Un contournement
des petits rectangles est une variante supplémentaire à mesurer, pas un
gain à supposer. La baisse des visites reste distincte du temps CUDA.

## Portes demandées

- B de tailles 1, 31, 32, 33, 63, 64, 65, A de plusieurs lignes ; rectangles
  à masque zéro au début, au milieu et à la fin ; K1/2/3/5/10, masques
  2/4/6 ; trace vide, rejet partiel et complet, endpoints devenant témoins.
- Égalité de tous les masques avec le moteur CPU sans cache, plus ordre
  exact des survivants compactés. Les visites de la variante ne doivent
  **pas** être forcées égales au témoin ; séparer représentant, cache et
  repli. Le mode cache désactivé conserve son ancien juge de visites.
- Mutant causal sans retest : la fixture collinéaire déjà publiée doit
  échouer géométriquement, pas par crash. Une corruption d'un masque
  transféré doit rester détectée par le juge GPU.
- G4 : répétitions off/on contrebalancées, valeurs par invocation, pas
  seulement le meilleur de répétitions internes ; au minimum la trame
  08/000000 K5 complète et son jumeau exhaustif, puis K10 et les autres
  trames si le premier résultat justifie la dépense. s8/10/12 restent à
  distinguer. Ce lot qualifie S2, jamais FULL.

## Coordination

Le worktree développeur lu reste à `3cf62b8ca` : v29 multi-trames/THP,
avec worker et selftest non publiés. Aucun chevauchement produit S2 relevé.
Les chantiers lots compacts FULL, glu hôte et continuations du front lui
restent réservés. L'audit C R22 ne révèle pas de défaut d'objet sur ces
chemins ; ses réserves concernent surtout l'attribution des temps et la
frontière chaîne/processus. Le protocole de ce lot doit donc publier ses
frontières et conserver la relecture indépendante `TERMINATED` après
l'arrêt de la génération G4 exacte. Aucun appel GCP par cette revue.
