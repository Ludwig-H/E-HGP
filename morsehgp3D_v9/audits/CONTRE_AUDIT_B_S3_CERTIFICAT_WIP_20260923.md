# Préflight B du certificat S3 GPU — snapshot mutable

23 septembre 2026. Lecture **sans modification du moteur** du worktree développeur `build/v9-open-worktree`, dont le port S3 est désormais figé en commit local détaché `50dabc0fa` (mêmes sources moteur que son ancien hash `308110bc3`), **non publié sur `main`** à cette lecture. Ce document est un **signal de préflight**, pas une qualification du port ni un défaut attribué à R12. Revalider chaque ligne sur le commit finalement publié. Le contrat reste la tour entière sur plusieurs trames SemanticKITTI, d'abord K1..10 puis K1..5, en moins d'une seconde G4 ; aucune mesure S3 G4 n'existe ici.

Le port hôte `gpu/certificate.hpp` suit à la lecture l'ordre des plages, les bornes fermées, les formes u18, les cellules de profondeur 2..6 et les seuils `K−1`/`K−2` du producteur. Le gate provisoire compare masques **et compteurs** sur des survivants synthétiques 2k/8k K3/5/10, ainsi que le report sous petite capacité. Ce n'est ni une comparaison CUDA exécutée ni un gate aux bornes u18, aux trames LiDAR ou à K1/2. Le `WarpGroup` et le noyau persistent sont dans le commit local détaché ; ils doivent encore compiler, subir un différentiel par arête, un test des voies vides/différées et une campagne G4 avant toute déclaration d'exactitude device.

La campagne locale observée après le commit détaché passe **148/148
tests actifs**, un mutant étant désactivé, dans `build/v9-exp` (log
`Testing/Temporary/LastTest.log`, 18:18 UTC). Son cache indique
`MHGP9_ENABLE_CUDA=OFF` : cette réussite juge la référence CPU et
l'émulation hôte, **pas** la compilation ni les décisions du noyau
CUDA. Ce log de build non versionné est une observation de préflight,
pas un reçu archivé. La première dépense G4 devrait donc être un smoke
test court compilation + quelques arêtes et lot vide sur device ;
n'exécuter les 18 cas R13 qu'après égalité par arête et correction des
portes d'entrée.

## Trois portes de domaine à fermer

1. `run_certificate_batch` CUDA exécute `out.masks.assign(input.edge_mask, input.edge_mask + edges)` **avant** `if (edges == 0) return`. Or `validate_certificate_input` accepte `edge_count=0` avec `edge_mask=nullptr`. L'addition `nullptr+0`, puis l'itérateur de plage nul, n'ont pas de contrat C++ valide. Retourner avant `assign` ou le conditionner, puis ajouter un gate CUDA/stub `0 arête, pointeurs nuls`. Une liste vide est normale quand le filtre S2 élimine tout.
2. La garde héritée `validate_filter_input` accepte une feuille dont la plage contient **plus d'un rang**. `build_cover` S3 suppose au contraire que toute feuille est singleton : si la boîte d'une telle feuille est ambiguë, il descend vers `node.left=absent32` et **omet** la plage. Cela peut produire un `fault` (extrémités perdues), une couverture incomplète et des comptes faux. Pas de lecture hors limites directe dans cette boucle : le curseur sentinelle termine le parcours. Imposer `leaf ⇔ last−first=1` dans la validation de cette API, avec mutant de feuille multi-site ambiguë. L'index produit satisfait déjà cette propriété ; le défaut est à la frontière publique brute.

3. `validate_certificate_input` admet toute arête de masque non nul
dans `2|4` même quand `kmax=1` ou quand la voie q4 est indisponible à
`kmax=2`. `prove_lanes` ne tente alors pas cette voie et renvoie son
masque ouvert ; le port brut peut donc publier une décision q3 à K1
ou q4 à K2. Les survivants issus du filtre S2 de la chaîne respectent
le domaine, donc ce n'est **pas** une divergence démontrée du contrat
LiDAR K5/K10 ; c'est une garde d'API publique à fermer. Refuser un
   masque hors des voies disponibles (`0` à K1, `2` à K2, `2|4` à partir
   de K3) et tester les deux refus, ainsi que `dead_core=false`.

## Porte mémoire CUDA supplémentaire : retour anticipé sans barrière

Sur le commit local `50dabc0fa`, `gpu/certificate.hpp:390–391` laisse
plusieurs lanes écrire les IDs partiels dans le tableau `next` d'un
frontier. Si un bloc suivant atteint le seuil, `enter_cell` retourne
à `:383–387` **sans** `group.sync()`. `prove_lanes` (`:470–486`)
peut aussitôt visiter une cellule sœur qui réutilise la même plage du
slab : une autre lane peut réécrire le même `next[j]`. Les deux
`__ballot_sync` de `WarpGroup::ballot2` (`filter_runner.cu:489–494`)
ne créent [aucun ordre mémoire selon NVIDIA](https://docs.nvidia.com/cuda/cuda-programming-guide/05-appendices/cpp-language-extensions.html#warp-vote-functions),
alors que [`__syncwarp` ordonne les accès des lanes participantes](https://docs.nvidia.com/cuda/cuda-programming-guide/05-appendices/cpp-language-extensions.html#synchronization-functions).
La barrière du chemin sans arrêt, à `certificate.hpp:393`, se situe
après la branche et ne protège pas ce réemploi. C'est un **risque
statique concret de WAW inter-lanes**, pas une divergence observée sur
G4 ; l'émulation hôte ne peut pas le réfuter.

Ajouter une barrière de warp avant ce retour anticipé, puis un gate
device causal où un bloc écrit des partiels, le bloc suivant atteint
le seuil, et la cellule sœur réutilise le frontier. Comparer masques
et travail **par arête** à la référence CPU, sous répétitions CUDA ;
ne pas lancer R13 comme qualification avant cette porte.

## Travail, mémoire et juges encore ouverts

Le slab S3 vaut exactement `52×capacity` octets **par warp** : `2u32` de plages, trois `i64` de formes et cinq `u32` de frontières par site. Le défaut `capacity=65536` donne **3,25 Mio par warp** ; le lanceur peut demander jusqu'à 16 warps par SM et un quart de la mémoire GPU libre en slabs. Il faut publier histogrammes `max(core_sites,cover_sites)`, fraction et coût des reports CPU, nombre réel de warps et pic HBM sur brut/sans-sol, K5/K10. Le contrat de dizaines de millions exige en plus des sorties par lots : le S3 provisoire téléverse toujours les tableaux de toutes les arêtes, et l'appel répète la garde `3Σ|plage(v)|` et la copie de l'index après S2. Réutiliser l'index certifié/chargé est une optimisation de chaîne, pas une modification de la preuve.

Le noyau donne une arête par warp persistant. La construction du cœur/cover et la descente des cellules restent des parcours **uniformes dans les 32 voies** ; seuls les chargements de formes et les scans des frontières répartissent effectivement les sites. Le scheduling masque des arêtes courtes, mais ne réduit pas la masse de la **classe** des arêtes lourdes : sur le brut K5, les 1 % plus gros cœurs des arêtes traversantes ne portent que 3,0 % de leur masse et les 10 % en portent 27,7 % ([reçu apparié](edge_matched_core_20260923/README.md)). Le [reçu brut K10](lidar_raw_k10_density_20260923/full.stdout) compte sur 08/000000 **1 242 755 011** visites de nœuds pour le cœur, **720 414 128** pour le cover, puis **2 383 439 336** et **4 376 234 522** tests uniformes respectifs. Dans le [reçu G4 R12](../receipts/g4_tower_r12_20260923/README.md), le sans-sol 08/000000 représente encore environ **540,6 M** visites cœur+cover et **1,458 Md** tests uniformes à K5 ; **1,394 Md** visites et **5,232 Md** tests à K10. Ce sont des **comptes logiques du moteur antérieur**, ni des mesures S3 GPU ni des durées. Profiler registres/spills `ptxas`, occupation, histogrammes p50/p95/p99/max des sites/visites/cellules par arête, et temps de la queue ; déplacer ce travail sur G4 ne le rend pas sous-quadratique. Le filtre S2 paie encore deux recherches binaires de rectangle par paire (filtrage et dispersion, `O(P log R)`) et refuse `P>2³¹−1` au lieu de tuiler : blocage architectural pour les dizaines de millions, pas plafond mathématique.

Le raccord `Engine::certified_edge` reconstruit sur CPU le cover des arêtes dont au moins une voie reste ouverte après S3, nécessaire à l'atlas et à la génération. Sur le sans-sol 08/000000 du reçu de phases, cela concernerait **708 686** arêtes K5 et **1 463 362** K10, et non tous les 900 377/1 934 399 covers initialement construits. `edges_ns` paie cette reconstruction, mais `work.cover.*` garde les **comptes logiques** du certificat GPU et ne la recompte pas ; les pics `peak_edge_buffer_bytes` excluent les slabs HBM et les cœurs GPU fermés ne passent plus par `observe`. Séparer travail physique CPU/GPU, pics RSS/HBM et ledger logique avant de comparer les coûts. Les identités agrégées de masques/compteurs du nouveau `check_certificate_batch` ne certifient pas chaque décision : exiger par arête le différentiel CPU/GPU et catalogue/tour clé par clé. Un `CertificateOutput.available=true` peut coexister avec une erreur d'allocation ou device capturée ; le bridge doit refuser **`error` non vide et `faults>0`**, pas seulement tester `available`.

Le gate de chaîne WIP conserve dans son flux comparé les coefficients de
boule, supports, profondeur et **taille** de coquille, mais pas les **IDs**
de coquille malgré son commentaire. Son `same_work` compare seulement
une partie de `WspdQ34Work`, laissant notamment les compteurs q3 et
local/q4 de côté. Les condensés FULL ensuite comparés sont utiles mais
ne transforment pas cette porte en égalité exhaustive du flux ou du
travail. Copier les deux suites d'IDs `shell_first` et `shell_second`
(leur séparation et leur ordre ont un sens dans le flux), puis trier
seulement les **candidats** pour les comparer ; étendre le comparateur
aux champs logiques q3, local/window, blocs, cellules et payload, puis
tuer un mutant qui remplace un ID
de coquille à taille inchangée. Couvrir aussi l'option `dead_core=false`
et K2 sur **device**, pas seulement dans la référence CPU.

Le cas vide a aussi une conséquence de **libellé** : la voie CUDA pose
`available=true` et le nom du GPU après son préflight, puis retourne
sans kernel lorsque `edges==0`. Le bridge publie ce nom comme
`certificate_backend` et le gate compte alors un `gpu_run` sur la seule
base d'un statut complet et d'un backend non CPU. Cela prouverait un
appareil disponible, **pas** un certificat calculé sur GPU. Reprendre
la séparation déjà introduite pour S2 entre préflight et nombre de
cas/arêtes S3 effectivement achevés ; imposer un mutant « zéro arête,
préflight GPU vrai, exécution S3 fausse » au lecteur G4 futur.

Le worker v18 **local, non publié** corrige à présent la version et les leviers du
protocole G4, mais son marqueur `GPU_executed` reste lié au levier, pas au
nombre de certificats réellement **décidés** sur GPU. Contre-exemple
local du lecteur : `survivors=deferred=3`, backend GPU et temps device
positif satisfont `validate_probe`, puis `gpu_completed_cases` marque
le cas GPU ; toutes les arêtes peuvent pourtant être reprises par le CPU.
Reproduction sans appareil ni donnée LiDAR : construire le premier cas
de `tower_snapshot_v9.default_plan()` avec
`tower_selftest_v9.FAKE_PROBE`, poser
`q34_batch.deferred=q34_batch.survivors`, puis appeler dans cet ordre
`tower_worker_v9.validate_probe(value,case,0)` et
`gpu_completed_cases([case],[{"outcome":"complete_relative"}])`.
Lecture B sur le diff courant : `3 0.02 complete_relative [0]` pour
`survivors`, `certificate_device_ms`, validation et liste GPU.
Publier `certificate_decided_edges`/lancements réels et distinguer
« filtre GPU », « certificat GPU utile » et « tour complète GPU ».

Enfin, la sonde `bench/tower_probe.cpp` du commit annonce `mhgp9_tower_probe_v18`. Le worker, son selftest et le lecteur LiDAR ont été adaptés pendant cette contrelecture ; ils ont ensuite été figés dans un commit local sans reçu G4. La porte chaîne reste insuffisante sur les IDs des coquilles et le travail complet. Avant une VM, fermer ces portes et les deux défauts d'entrée. Le nouveau certificat S3 ne règle de toute façon pas seul le budget : R12 K5 laisse encore 1,177/1,388/1,513 s de chaîne si l'on retire fictivement **tous** les survivants et que les autres phases restent inchangées.

## Libellés à corriger avant de présenter le port

Le développeur a depuis figé localement un paquet S3 (`50dabc0fa`,
encore hors `main` à cette lecture). Sa `PASSATION.md` reprend « 35/65 % »
et « 28/72 % » comme si c'étaient des parts de calcul, puis ferme la
piste des formes paresseuses sur un « au plus 1,3 % ». Le
[contre-calcul TSC](CONTRELECTURE_CYCLES_SURVIVANTS_Q34_20260923.md)
établit seulement des parts de **temps écoulé de fils désordonnancés**,
et 1,3 % est une projection de deux essais, pas une borne ni une
ablation. Conserver la priorité pratique S3/atlas est raisonnable ;
ne pas la justifier par un plafond inexistant.

La nouvelle `docs/PROVENANCE.md` appelle l'égalité du `catalogue_digest`
une comparaison « clé par clé » GPU/moteur. Le code calcule et compare
un **condensé FNV 64 bits** de toutes les clés/intérieurs/coquilles ;
c'est un contrôle renforcé, mais pas une égalité littérale du catalogue,
et le gate de flux omet encore les IDs des coquilles. La ligne locale
08/000000/K5 annoncée avec digests égaux n'est pas accompagnée ici
d'un reçu versionné liant binaire, entrée et commande. Enfin, même une
chaîne achevée avec S2/S3 CUDA laisse atlas, voies q3/q4 et FULL sur CPU :
la décrire comme une « tour achevée **sur** l'appareil » surinterprète
le backend. Distinguer appareil disponible, filtre GPU, arêtes S3
décidées sur GPU et tour mixte achevée ; seul un reçu G4 avec
différentiel device peut qualifier S3.

## Relecture du correctif `6596b13a2`, publié sous `545c71799` (19 h 21 UTC)

Ce commit, initialement **local détaché** puis rebasé sur `main`, ajoute
`group.sync()` avant chaque accès
utile à la frontière du niveau courant, au lieu de compter sur les
votes. Sous le flot uniforme du warp, cela ordonne aussi le réemploi
après le retour anticipé signalé plus haut ; la **cause statique WAW
semble fermée** par lecture. L'occupation mesurée remplace le nombre
fixe de warps par SM et le paramètre de capacité permet une sonde de
reports. Aucun noyau CUDA de ce commit n'a encore été compilé ou
comparé sur appareil dans un reçu : ne pas transformer cette correction
source en qualification G4.
Le [gate structurel indépendant de A](s3_frontier_barrier_gate_20260923/README.md)
épingle le header corrigé octet pour octet, exerce trois cellules
sœurs avec retour anticipé et réemploi, et tue le mutant sans la
nouvelle barrière : accès WAW **et WAR** non ordonnés disparaissent
dans son modèle hôte instrumenté. Il renforce la preuve de placement
de la barrière, sans simuler la mémoire ni exécuter CUDA.

Les gardes publiques ci-dessus **restent ouvertes dans ce commit** :
`filter_runner.cu` affecte toujours la plage
`[edge_mask,edge_mask+edge_count)` avant le retour `edge_count==0`,
alors que le validateur permet `edge_mask=nullptr` pour le lot vide.
Le validateur accepte aussi une feuille couvrant plusieurs rangs ;
`build_cover` peut alors devoir la diviser sans enfant. Enfin le
masque q4 à K2 n'est pas refusé à l'entrée. Les nouveaux cas `K2`,
sans cœur et capacité 64 renforcent les tests hôtes, mais ne ferment
pas ces trois chemins bruts. Le nouveau condensé catalogue **inclut**
les IDs triés des coquilles et une mutation en teste la sensibilité ;
le gate de flux compare toutefois seulement leurs tailles, et un
condensé n'est pas une comparaison littérale indépendante de leur
appartenance attendue. Le dernier journal
CTest ciblé observé (`build/v9-exp`, 19 h 08–19 h 12, **CUDA OFF**)
contient 23 tests réussis, dont la porte chaîne à 24 flux/84 mutants
et `gpu_runs=0` ; il n'est ni une suite complète ni un test device.

Avant R13 payant : traiter ces trois gardes, exercer une fixture CUDA
du retour anticipé/frère et une mise en attente réelle à capacité 64,
puis comparer par arête masque et travail à la référence. Le nouveau
protocole refuse désormais `deferred=survivors` dans son préflight
réduit et exige zéro report sur les trames normales plus petites que
l'ardoise ; cela corrige l'ancien faux « GPU utile » dans **ces cas**.
Ce préflight device est une **intention de protocole**, pas encore un
résultat. Un compteur explicite des arêtes décidées sur GPU reste
préférable dans chaque reçu de production.
