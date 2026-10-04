# R7 — contrelecture du correctif publié au commit 77db5738

Source exacte : `77db5738eb2dd5bc84ecdc4d85ade833124c58f8`. Les blobs Git
ont été copiés et comparés au worktree développeur propre avant dérivation.
Le reçu initial `../batch_transport/` reste intact ; son SHA racine est
conservé dans `SOURCE_BEFORE.json`. Les cinq différences de source pertinentes
sont sous `diff/`. Cette capsule n'exécute ni C++, ni nvcc, ni CUDA, ni GCP.

**Réservation device : corrigée en source.**
`sources/src/catalogue/leaf_batch_cuda.cu:98–108` garde la taille par
`add_bytes<T>`, réserve un `BudgetReservation` dans le compte commun avant
`cudaMalloc`, puis rend la réservation si l'allocation échoue. Les tableaux
d'entrée, de préfixes, le temporaire CUB et les sorties utilisent tous cette
voie. `BudgetReservation` partage le compte des `Buffer`
(`sources/src/core/buffer.hpp:107–135`) ; son acquisition utilise la même
addition atomique bornée par `limit` (`buffer.cpp:15–27,50`).

Les réservations restent vivantes pendant `download` et ses allocations
hôtes (`leaf_batch_cuda.cu:239–252`). Le destructeur de `DeviceArray`
appelle `cudaFree` avant la destruction du membre `reservation` (:93,97) :
la libération du compte vient après cet appel. Le compteur `bytes += size`
(:108) est borné : toutes les réservations device déjà ajoutées restent
simultanément vivantes, donc leur somme est au plus `budget.used()`, lui-même
au plus `limit <= u64max`. La limite de pile device explicite a été retirée.
Le contexte/runtime et la mémoire locale opaque restent déclarés hors des
payloads ; ce budget n'est pas une mesure de RSS ni du pic total GPU.
Les erreurs CUDA et le résultat de `cudaFree` demandent toujours une
qualification native ; aucune libération effective en panne n'est attestée
par cette seule lecture.

**Nouvelle restriction de lot à remplacer avant qualification générale.**
Les exécuteurs hôte et CUDA refusent désormais `view.count >
view.cloud_sites` (`leaf_batch.cpp:72`, `leaf_batch_cuda.cu:185`). La
documentation R7 annonce ce refus. Les boîtes T0 sont disjointes, mais leurs
listes K-certifiées se recouvrent : ce plafond ne découle pas de la géométrie.

Témoin entier u21 : les huit sommets de `{0,16}³` et le centre `(8,8,8)`,
IDs distincts, poids tous égaux à 1 ; `K=5`, `leaf_size=8`, `max_leaf=32`,
`max_nodes=0`, `single_pass=true`, `pair_graph=true`,
`batch_leaves=true`, `adaptive_frontier=false`, les autres options par
défaut. Le modèle exact de `boxes.cpp`, du préfixe fixe de profondeur 8 et
de `Frontier::execute_task/run_ready` donne **159 nœuds, 48 tâches, 80
feuilles réellement mises en file, 648 entrées de sites** ; aucune feuille
ne dépasse neuf sites. Le centre (SiteIdx 1 après tri Morton) appartient aux
80 listes. Les gardes du transport rendent donc `catalogue_invariant` sur
ce petit lot issu de paramètres publics licites. Il s'agit d'un refus
restrictif nouveau, pas d'une sortie géométrique erronée ni d'un crash
reproduit en natif.

La borne arithmétique annoncée est **conditionnellement correcte** :
`P=41448`, `kCountBound=32*3*P=3979008 < 2^22`, donc un lot admis avec au
plus `2^32−1` feuilles a chaque somme sous `2^54`. Cela ne démontre pas que
les feuilles d'un nuage sont au plus aussi nombreuses que ses sites.
Le témoin de 80 feuilles ne nécessite qu'une majoration de 318320640.

Correction sûre : contrôler `count * kCountBound` et les sommes réellement
réduites, ou choisir un plafond de feuilles explicitement justifié par les
accumulateurs et les domaines CUB/grille. Garder les contrôles des tailles
et le budget commun ; retirer l'assimilation `nombre de feuilles <= nombre
de sites`. Sur ce témoin, un test natif G4 ciblé devrait comparer CPU
ordinaire, lot hôte et lot CUDA, avec un refus mémoire séparé sous petit
plafond. Cette capsule n'a exécuté aucune de ces portes.

`check_frontier_count.py` est autonome en Python standard : neuf sites
synthétiques, calculs entiers, sources figées, comptage du chemin public et
borne d'accumulateur. Les sorties normal/optimisé sont identiques. Les
contrôles de hashes et assertions du modèle ne qualifient pas l'exécutable,
les kernels, leur mémoire en panne, leurs dumps ni leur temps.

```sh
python3 -B -S check_frontier_count.py
python3 -B -O -S check_frontier_count.py
sha256sum -c SHA256SUMS
```

Le ledger exclut seulement lui-même et `SHA256SUMS` racine de ses payloads.
`SHA256SUMS` inclut le ledger et exclut uniquement son propre chemin racine.
