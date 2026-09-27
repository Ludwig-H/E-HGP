# Couture résidente : rectangles filtrés avant l'arène

Prototype isolé du 27 septembre 2026, base `a7e80d7f9`, sans modification
du moteur. Profil entier u18 / grille 1 mm, K1..10 pour l'interface,
`not_claimed`. Qualification portable fraîche r1 close : Release et
Clang ASan/UBSan/LSan passent, lecteurs normal/−O identiques.
**La nouvelle unité CUDA n'a pas encore été compilée ni
exécutée.** Aucun nouveau résultat G4 ni FULL n'est revendiqué ici.

## Ce qui change

L'ancien prototype préparait chaque rectangle sur CPU avant d'envoyer les
paires au GPU. Cette préparation coûtait 9,898 s sur la première trame
sans sol G4 ; ce chrono mélangeait le filtre et l'arène. Le nouveau chemin
exécute d'abord le filtre exact des rectangles sur GPU, puis ne construit
l'arène CPU que pour les rectangles survivants. Les nœuds et points device
restent résidents entre les deux passes.

```text
index + requêtes → copie/validation → filtre rectangles par vagues Qr
  → masques R + descriptions R_live → arène CPU → paires par vagues Q
  → tri des seules survivantes → batch natif ordonné
```

Le filtre reste `gpu::filter_boxes`, y compris sa spécialisation correcte
des deux boîtes singleton. Les comptes ponctuels repartent de zéro ; aucun
crédit Pool n'est injecté dans le census. L'arène, ses copies, les transferts,
le tri et les allocations restent payés. Ni le précédent temps CPU ni son
rapport au GPU ne servent de baseline de performance produit : une future
promotion nécessitera une comparaison GPU/GPU appariée au filtre natif.

Attributions explicites : propriétaires/parcours issus de
[`waves.hpp`](../b_q34_arena_waves_20260927/waves.hpp), arène collective
[`arena.hpp`](../b_q34_collective_arena_20260927/arena.hpp), POD/décodeur
**inchangés** [`wire.hpp`](../b_q34_cuda_waves_20260927/wire.hpp), buffers et
consommateur CUDA portés de `runner.cu` au commit `33c1d28d7`, noyau rectangle
adapté de `src/gpu/filter_runner.cu` au commit `70168cc3b`. Le lecteur local
réutilise explicitement l'orchestration figée de `direct_bands/run.py`,
vérifiée avant son chargement ; les anciennes preuves ne sont pas héritées.

## Propriétaires et invariants

`Session::open(index, requests, K, backend, Qr)` copie les requêtes avant
validation. La construction privée de `FilteredRectangles` ne permet pas
d'injecter un masque fourni par l'appelant. La session est liée à une seule
population ; aucune méthode de re-filtrage ne remplace ses buffers par une
autre population. `Prepared::build(decision, workers)` dérive index/K de
cette décision et `consume` impose la même identité privée d'origine.

Les requêtes complètes et le préfixe brut R+1 sont libérés après toutes les
vagues, leur jointure et la compaction scellée. Restent les R octets de
masques natifs, y compris les zéros, et les seuls compacts vivants : ordinal
source, base brute, deux nœuds, masque. La base brute inclut les produits
fermés. Indices compact, source et arène sont différents.

L'arène temporaire est détruite après copie des POD et construction des
segments, avant publication de `Prepared`. La décision/index reste partagée
et immuable. L'utilisation de `Session` est **synchrone et non concurrente** ;
aucune garantie de session multi-appels simultanés n'est annoncée. Les API
brutes de `device.hpp` sont internes à cette couture, pas des fabriques
publiques de géométrie certifiée.

Praw compte tous les produits fournis, P les produits des rectangles encore
ouverts, E les requêtes après Pool, S les survivantes. `expanded_pairs=P`
reste le contrat natif ; E est publié séparément. Les rejets par voie sont
`(Pq−Eq)+rejets ponctuels`. Les masques rectangle restent ceux d'avant Pool.
Le batch final compare tous les masques et les survivantes/endpoints dans
l'ordre natif, pas seulement un digest. La décision certifie le filtrage
des rectangles fournis, **pas** leur disjonction/couverture WSPD globale.

Qr/Q bornent seulement les buffers. Le découpage est exhaustif, sans quota
de recherche ; chaque vague est au plus `INT_MAX`, les masses/ordinals sont
u64 contrôlés. Avec moins de 2^32 nœuds, les visites de chaque vague restent
sous 2^63 ; les additions inter-vagues sont contrôlées. `stack_failure` et
échecs CUDA interdisent tout succès partiel. Le moteur/validateur public
CUDA brut n'est pas modifié ni contourné pour des entrées extérieures : la
copie privée de l'index dérive ici de son propriétaire déjà certifié.

## Mesure et mémoire : périmètres

Le mode frame paie l'entrée, l'index et le **front CPU mono-thread** séparément
(`workers_front=1`). Ce n'est pas le front moteur à 48 threads. Le mur
`adapter` commence avant `Session::open`, inclut l'initialisation CUDA,
les copies/validations, rectangles, arène, consommation, et finit après
destruction de `Prepared`, fermeture puis destruction de `Session` et de
sa décision. Seule la sortie batch S+R est alors conservée ; l'index et les
requêtes originaux appartiennent à l'amont. La référence CPU4, comparaison,
et destruction finale de l'amont/sortie sont publiées à part et payées dans
`total`. La référence est un juge de correction, pas une baseline GPU.

`cuda_init` est inclus dans `open` et `adapter`. `close` libère nos buffers,
pas le contexte CUDA global ; aucun `cudaDeviceReset` n'est appelé. Dans le
consommateur, `waves_including_count_download` inclut la descente des petits
compteurs/tails ; `survivor_download_allocate` ne mesure que la descente S
et ses redimensionnements, tandis que `pair_download_bytes` compte les deux.

Les métriques mémoire sont des sous-périmètres, **pas un pic RSS/VRAM global** :

- `decision_retained_bytes` inclut capacités des masques/compacts et objet,
  pas l'index ni métadonnées d'allocateur ; la capacité compacte peut dépasser
  sa taille.
- `prepared_retained_bytes` inclut l'objet/POD, pas la décision partagée.
  `before_arena_release_bytes` ajoute l'arène encore présente, mais pas les
  requêtes planifiées transitoires ni la décision/index.
- `arena_owned_peak_bound` décrit le périmètre arène hérité, sans les copies
  Prepared simultanées, piles/allocations de bibliothèque de threads.
- `temporary_input_bytes` couvre copie des requêtes, préfixe et batch hôte,
  pas toutes les coexistences transitoires.
- `rectangle_device_bytes` et `pair_device_bytes` incluent **chacun déjà**
  `resident_device_bytes`. Les additionner compterait l'index deux fois ;
  ces phases sont successives. Les allocations internes runtime/CUB hors
  scratch déclaré et la croissance simultanée des vecteurs hôte S ne sont
  pas un pic global mesuré.

La mémoire n'a pas de tableau global P/E ; elle dépend de l'index, R,
R_live, facteurs/classes/bandes et S, plus Qr/Q. Le temps dépend encore des
visites rectangles/points, du coût des facteurs et du tri S. Aucune borne
sous-quadratique globale ou nouveau gain de chaîne n'en découle.

## Portes et usage

La gate réutilise 85 petits cas : familles uniforme/terrain/amas/rangées,
K1/2/5/10, s8/10/12, permutations, masques2/4/6, plans/fallbacks, E=0 et
E>0/S=0. Chaque cas compare le chemin portable pour trois Qr/Q/W, puis
le device avec Qr/Q7 et257 (et1 pour les toutes petites masses) si demandé.
Les compteurs physiques queries/q3/q4/rejets/visites doivent être identiques
entre ces passages. Une copie historique de Prepared/Snapshot sert seulement
d'oracle de couverture au petit gate ; elle n'existe pas dans le mode frame.

Les portes nouvelles vérifient source/compact/arène distincts, trous bruts,
masques denses, alias appelant modifié, index appelant libéré, mauvaises
identités/K/index, close idempotent et consume après close refusé. Trois
mutants compilés ferment indûment les masques, changent la base brute ou
suppriment la vérification d'origine. Deux injections portables simulent
`stack_failure` rectangle/paire, sans annoncer une panne CUDA réellement
injectée. Il n'y a pas encore de nouvelle injection `bad_alloc` ni de gate
TSan propre à ce raccord ; les workers appartiennent à l'arène inchangée.

`portable_runs` et `cuda_runs` comptent les appels comparés du corpus ; la
porte d'identité ajoute un appel valide séparé, hors ces compteurs. Les
tests de formules >2^32 n'allouent pas des milliards de paires et ne sont
pas un test de passage à l'échelle. Le petit fichier frame local est une
fixture artificielle de 12 points, jamais un scan LiDAR.

```sh
cmake -S morsehgp3D_v9/audits/b_q34_filtered_resident_20260927 -B NOUVEAU_BUILD -DMHGP9_RESIDENT_ENABLE_CUDA=ON -DCMAKE_BUILD_TYPE=Release
cmake --build NOUVEAU_BUILD -j 2
NOUVEAU_BUILD/mhgp9_q34_filtered_resident --gate --cuda
NOUVEAU_BUILD/mhgp9_q34_filtered_resident --frame ENTREE_U32 --cuda --qr 262144 --q 262144 --workers 4 --k 5 --s 8
```

Le CMake autonome compile les sources gen nécessaires depuis le snapshot ;
il n'exige pas une archive locale. En local sans CUDA, l'option est OFF et
`--cuda` est refusé. Un build neuf et des reçus distincts sont requis ; ne
pas reconstruire dans les répertoires de preuves déjà fermées.

## Qualification locale close

[Reçus r1](../../receipts/q34_filtered_resident_20260927/r1/README.md) :
50 commandes, premier essai frais sans échec. Chaque build rend les mêmes
85 cas / 255 passages portables, 75 990 requêtes et 29 901 survivantes
cumulées. Les 537 rejets Pool, 312 réductions strictes de voie, 1 833 trous
d'ordinals/bases et 36 observations où les trois indices diffèrent empêchent une
validation vide du nouveau raccord. Il y a 276 refus internes par build,
22 refus CLI au total, trois mutants tués par leur divergence attendue et
quatre injections de `stack_failure` portables (deux par build).

La fixture frame artificielle de 12 points donne P=E=S=63, hash d'entrée
`103698110648986415`, digest de sortie `4611423470994764243`, identiques en
Release et sous sanitizers. Ses temps ne sont pas une mesure LiDAR ni une
prédiction G4. Les sources, binaires, options de compilation, commandes et
sorties sont épinglés et rejugés par le lecteur LIVE.

Builds désormais immuables :
`/workspaces/E-HGP/build/v9-audit-q34-resident-20260927-r1_release` et
`/workspaces/E-HGP/build/v9-audit-q34-resident-20260927-r1_sanitize`.
Le préflight séparé reste mutable et n'est pas l'autorité de cette capture.

```sh
python3 morsehgp3D_v9/audits/b_q34_filtered_resident_20260927/run.py --readback morsehgp3D_v9/receipts/q34_filtered_resident_20260927/r1
python3 -O morsehgp3D_v9/audits/b_q34_filtered_resident_20260927/run.py --readback morsehgp3D_v9/receipts/q34_filtered_resident_20260927/r1
```
