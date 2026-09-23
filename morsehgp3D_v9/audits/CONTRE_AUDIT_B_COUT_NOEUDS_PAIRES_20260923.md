# Contre-audit B — coût physique des paires de nœuds

23 septembre 2026. Lecture du
[shadow par nœuds](paired_guard_node_blocks_20260923/README.md) au
commit `874f32286`, comparé au
[shadow Top-B indexé](paired_guard_index_shadow_20260923/README.md).
Ces expériences portent sur les mêmes 120 arêtes q3/q4 lourdes tirées
**après** connaissance du cœur d'une seule trame brute K5/s8/u18.
Aucun changement du moteur produit, reçu G4 ou temps de chaîne dans
ce contre-audit.

## Certificat positif : pas de défaut trouvé

Pour les nœuds U,V, `Hmin` minore bien `H` et `Xmax` majore
`|d×(w_g+w_h)|²` sur leurs boîtes ; les opérations u18 en i64/i128
gardent une marge de largeur. Un nœud admis est entièrement dans le
cœur et un quadrant, sans extrémité ; les nœuds retenus sont disjoints
dans chaque voie, puis `min(|U|,|V|)` crédite des sites distincts.
Une recherche interrompue n'affirme qu'une preuve **positive**, son
échec conserve la voie exacte. La lecture de source n'a trouvé aucun
faux positif. Les SHA des deux dossiers et lecteurs normal/`-O`
passent ; le lecteur des nœuds recontrôle **758 certificats** et leur
association aux coordonnées LIVE u18 épinglées. Il n'est pas un
oracle de complétude pour les échecs et ne rejoue pas des preuves
individuelles pour tous les bras cap1/2 à budget64. La sortie
ASan/UBSan transitoire est déclarée mais non conservée.

## Le compteur `boxes` manque le second calcul

Dans [`node_blocks.cpp`](paired_guard_node_blocks_20260923/node_blocks.cpp),
`query::push` appelle `bounds(g,box)` et incrémente `work.boxes` à
l'insertion (**lignes 130–134**). Après extraction, la boucle rappelle
`bounds(g,box)` (**lignes 140–146**) sans incrémenter ce compteur.
Chaque nœud dépilé avait été inséré une fois : le nombre physique
de calculs de bornes est donc exactement `boxes+pops`. Le
[`index_shadow.cpp`](paired_guard_index_shadow_20260923/index_shadow.cpp)
stocke au contraire `Bounds` dans son `QueueItem` et ne le recalcule
pas à l'extraction. Les colonnes « Boîtes » ne comparent donc pas le
même travail physique.

| 120 arêtes | Fermées | `boxes` publié | `pops` | bornes effectivement calculées | tests de paires |
| --- | ---: | ---: | ---: | ---: | ---: |
| Top-B points B16, budget64 | 21 | 58 444 | 30 334 | **58 444** | 11 751 |
| Nœuds cap4, budget64 | 32 | 57 794 | 30 314 | **88 108** | 14 624 |
| Top-B points B16, budget256 | 67 | 83 768 | 48 553 | **83 768** | 195 514 |
| Nœuds cap4, budget256 | 67 | 82 160 | 47 593 | **129 753** | 184 736 |

À budget64, les 11 fermetures supplémentaires des blocs coûtent
donc **29 664** calculs de bornes et **2 873** tests de paires de plus
que les points, et non légèrement moins de boîtes. À budget256,
les blocs économisent 10 778 tests de paires, mais calculent
**45 985** bornes supplémentaires ; leurs 67 fermetures concernent
des arêtes partiellement différentes et `F` fermable net augmente
seulement de 5 190. Ces unités n'ont pas le même coût, et les temps
du shadow sont pollués par l'hôte partagé : aucun verdict de vitesse
ne découle de leur somme brute. La comparaison doit refaire le
ledger ou stocker `Bounds` dans la file des nœuds, puis mesurer la
chaîne sur le flux entier, succès **et** échecs inclus.

Le sidecar recalcule aussi `F` par un balayage des 123 389 sites
pour chacune de ses 1 920 configurations/arêtes, **hors** intervalle
chronométré de sélection. C'est légitime pour contrôler un reçu
d'audit, pas un coût de production à masquer si l'on rejoue le
sidecar comme prototype de chaîne. Le bloc par arête, même corrigé,
ne prouve toujours pas le [certificat uniforme sur un rectangle
WSPD](CERTIFICAT_B_PAIRES_GARDES_RECTANGLE_20260923.md) ; son
rendement K10, s10/s12 et sous-quadratique reste ouvert.
