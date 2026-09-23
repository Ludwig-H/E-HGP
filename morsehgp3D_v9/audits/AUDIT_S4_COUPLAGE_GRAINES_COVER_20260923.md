# S4a : mesurer ensemble graines et cover, puis les vrais pas de warp

23 septembre 2026. Lecture seule du plan S4 et du reçu G4 R14 au commit
`3605cef99` (moteur `b68b6761`), des reçus CPU LiDAR brut, et du port S4a
**WIP non commis** dans `build/v9-open-worktree` à cette date (`lanes.hpp`
SHA-256 `31ff1ba41a263f0c7cd2d2631f529ef3c83f02f8716ce8ab86be1033854dd774`).
Aucun essai G4 ni nouveau chrono dans cette note. Les nombres ci-dessous sont des comptes
de travail, pas des prévisions de durée ou une qualification FULL.

## Ce que les reçus permettent réellement

Le plan S4 §4 projette environ 0,2 G et 0,65 G pas de warp q3 à K5/K10
avec 32 **graines** traitées ensemble pendant la lecture d'un site du cover.
Son nominal est

\[
P=\sum_{e\in E_3}\lceil g_e/32\rceil c_e,
\]

où `E3` est l'ensemble des arêtes décidées et encore ouvertes en q3 **après S3**,
`g_e` leurs graines aiguës propriétaires, et `c_e` la population de leur
cover complet. La projection du plan n'est pas une mesure de cette somme.
Les sorties R14 gardent `q3_seeds=Σg_e` et `cover_sites`, mais ce dernier
additionne **tous** les covers construits, dont ceux des voies q4 seules ou
finalement closes. Ni le reçu R14 ni les stdout des campagnes brutes ne
gardent les couples `(g_e,c_e)`. Dans les six cas R14 S2+S3 inspectés,
`q3_edge_queries=q3_edges` indique un appel de `q3_edge` par arête q3
comptée ; ce compteur est incrémenté **avant** le parcours des graines et
ne contrôle pas leur identité face au port WIP. `q34_batch.deferred=0`
dans ces mêmes cas. Aucun de ces constats ne fournit la jointure avec le
cover.

| Régime, 08/000000 | K | Sites | `q3_edges` | `q3_seeds` | `cover_builds` | `cover_sites`, toutes voies |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Sans sol R14 G4 | 5 | 39 885 | 701 678 | 9 318 486 | 900 377 | 251 972 209 |
| Sans sol R14 G4 | 10 | 39 885 | 1 437 421 | 33 354 383 | 1 934 399 | 874 278 487 |
| Brut CPU, densité 1/4 | 5 | 30 847 | 416 891 | 3 280 251 | 506 526 | 39 988 989 |
| Brut CPU, densité 1/2 | 5 | 61 694 | 840 839 | 8 242 180 | 1 045 864 | 115 366 032 |
| Brut CPU, entier | 5 | 123 389 | 1 679 637 | 21 384 178 | 2 179 536 | 364 354 011 |
| Brut CPU, densité 1/4 | 10 | 30 847 | 871 750 | 11 257 815 | 1 099 854 | 192 011 153 |
| Brut CPU, densité 1/2 | 10 | 61 694 | 1 750 794 | 26 574 970 | 2 216 801 | 362 455 230 |
| Brut CPU, entier | 10 | 123 389 | 3 530 449 | 67 539 736 | 4 560 470 | 1 074 719 197 |

R14 : `receipts/g4_tower_r14_20260923/vm/probe_{0,2}.stdout` à
`3605cef99`. Brut :
[`lidar_raw_physical_scaling_20260923/{quarter,half,full}_full.stdout`](lidar_raw_physical_scaling_20260923/README.md)
à K5 et [`lidar_raw_k10_density_20260923/{quarter,half,full}.stdout`](lidar_raw_k10_density_20260923/README.md)
à K10. Ces densités brutes sont des sous-ensembles emboîtés d'une scène ;
les autres coupes et les trames sans sol ont le même défaut de jointure.
Diviser les graines totales par les arêtes et multiplier par la taille
**moyenne de tous les covers** suppose une covariance nulle et confond les
ensembles d'arêtes. Par exemple, les mêmes `Σg=32`, `Σc=1026` et deux
arêtes sont compatibles avec `(g,c)=(32,1024),(0,2)` donnant `P=1024`,
ou `(32,34),(0,992)` donnant `P=34`. Chaque graine aiguë propriétaire
appartient au cover complet, donc `g_e≤c_e` et
`ceil(g_e/32)c_e≥g_e`. Même la borne sûre tirée seulement des agrégats
R14 000000/K5, `G≤P≤nG/32+C_all`, va de 9,318 M à
11,867 G pas : elle ne juge pas la projection de 0,2 G.

La [trace par arête du cœur](edge_matched_core_20260923/README.md) ne
répare pas cela : son enregistrement de 16 octets porte deux IDs bruts, la
taille du **cœur diamétral** et un masque avant/après cœur, pas `c_e` ni
`g_e`. Le [shadow de segments S2](s2_segment_mass_20260923/README.md)
joint ces tailles de cœur et les masques aux rectangles ; il ne mesure
pas le cover complet ni ses graines. Ses gros volumes ne sont donc pas
une mesure de `P`.

## Le port WIP a une autre unité et un arrêt précoce

Dans `src/gpu/lanes.hpp` du worktree WIP (lignes 23–30 et 366–409),
**un warp possède une arête, traite ses graines l'une après l'autre**, et
ses 32 voies classent 32 sites du cover pour une graine. Son nominal de
census complet est

\[
N=\sum_{e\in E_3}g_e\lceil c_e/32\rceil.
\]

Si la graine `s` s'arrête après le préfixe logique `t_{e,s}` de l'ordre
en huit anneaux du port, le nombre de tours de ballot réellement exécutés
est `A=Σ_{e,s}⌈t_{e,s}/32⌉≤N`. Le dernier ballot évalue aussi les voies
valides **après** le site logique d'arrêt. `census_point_tests` inscrit
seulement ce préfixe logique, donc ne mesure ni `A` ni les évaluations
physiques. Dans `tests/gpu/lanes_port_gate.cpp:271`, `--file` publie
`ceil(local.census_point_tests/32)` **une fois par arête**, honnêtement
libellé `warp_steps_lower_bound`. Ce n'est pas `Σ_s⌈t_{e,s}/32⌉` : deux
graines s'arrêtant chacune au quatrième site donnent deux ballots mais
`ceil((4+4)/32)=1`. Il ne faut pas employer ce minorant comme travail
effectif ou coût calibré de S4a.

Les deux ordinaux nominaux partagent le terme `L=Σg_ec_e/32` :
`L≤P<L+Σ_{g_e>0}c_e` et `L≤N<L+G`, donc **`N≤P+G`**. Si les 0,2 G du plan
étaient une vraie mesure appariée sur R14/K5, le nominal WIP ne pourrait
les dépasser que d'au plus 9,318 M ballots ; cette condition n'est pas
établie. Quand `g_e` et `c_e` dépassent 32, les deux sont proches au premier
ordre. Leur disposition, accès mémoire, occupation, divergence et chemin
critique par arête diffèrent malgré cette relation arithmétique.

Avant les census, le WIP reconstruit le cover, classe les sites en anneaux
par **deux passages**, et appelle deux ballots de classification par
passage : quatre évaluations de l'anneau par site, puis un balayage des
graines par site. Ces postes et les transferts/replis ne figurent pas dans
`P` ou `N`. La session de plages résidentes S4.0 du plan n'est pas encore
le chemin de ce port WIP. Un `A` plus petit ne prouve donc pas un noyau
plus rapide.

## Porte courte, sans trace géante

Sur les mêmes octets et mêmes masques S2/S3, sommer au moment où **le
cover et la liste des graines de chaque arête coexistent** : `g_e`, `c_e`,
`ceil(g_e/32)c_e`, `g_e ceil(c_e/32)` en entiers vérifiés, par ordinal
source et masque. Le code produit connaît déjà `cover->site_count()` dans
`filtered_edge`/`certified_edge` et incrémente `q3.seeds` dans `q3_edge` ;
le `--file` WIP possède aussi `local.seeds` et `local.cover_sites` au même
endroit. Publier sommes, maxima et quantiles **par arête**, pour brut et
sans sol, K5/K10, quarts/moitiés physiques et densités 1/4, 1/2, entière.
Vérifier `Σg_e=q3_seeds`, `Σc_e` contre le sous-total q3 des covers,
`q3_edge_queries=q3_edges` ou compter explicitement les graines des
arêtes sautées par atlas, et l'identité des masques/sorties avec le jumeau.
Ne jamais prendre les covers q4 seuls pour `Σc_e`.

Dans le ballot du WIP, incrémenter **par graine** `census_warp_batches`
à chaque groupe de sites visité et `physical_power_tests` du nombre de
voies valides de ce groupe ; garder séparément les `census_point_tests`
logiques et les arrêts. Ajouter `ring_evaluations=4c_e`, les passages de
graine, la reconstruction du cover, sorties et reports CPU. Vérifier
`A=Σ_s⌈t_{e,s}/32⌉` contre ces compteurs sur le port hôte et le noyau,
avec une fixture de deux préfixes de quatre sites qui tue l'emploi de
`ceil(Σ_s t/32)`. Pour les arêtes reportées par capacité de cover,
d'enregistrements ou d'arène, publier leur nombre, leurs coûts CPU et le
nominal qu'elles auraient représenté ; un report n'est pas un zéro de
travail. La porte de décision reste le **mur de chaîne** G4 avec copie,
recouvrement et traîne, à sorties identiques, une fois ces masses fermées.
