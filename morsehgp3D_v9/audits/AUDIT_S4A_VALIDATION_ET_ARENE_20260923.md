# S4a massif : certifier l'index une fois et budgéter l'arène complète

23 septembre 2026 — lecture du port local `aad7416a5` et de son WIP CUDA
`src/gpu/filter_runner.cu` SHA-256 `bc54dafe…`, sans mesure G4 nouvelle.
Cette note concerne le coût de la chaîne et la distinction entre report
d'arête et refus de l'appel, pas l'exactitude des boules acceptées.

## Trois validations intégrales avant les noyaux

Dans `src/gpu/filter_runner.hpp:58–93`, `validate_filter_input` balaie les
coordonnées de **chaque rang de chaque nœud** pour vérifier sa boîte.
`validate_certificate_input` réutilise ce contrôle (`:154–157`) et
`validate_lanes_input` le réutilise encore (`:235–239`). Sur le chemin
hybride S2+S3+S4a, les appels de batch GPU exécutent donc trois fois le
même contrôle de l'index, outre les autres gardes utiles. Le chemin hôte
S4a le refait aussi. Si `I(v)` est la plage du nœud `v`, le seul contrôle
des boîtes coûte `3 Σ_v |I(v)|` lectures/comparaisons de coordonnées par
appel. Pour un arbre médian complet à `n=2^h` feuilles singleton,
`Σ_v |I(v)|=n(h+1)` : à `n=2^25`, **872 415 232 appartenances
point–nœud par appel**, et environ **2,617 milliards** sur les trois
appels S2/S3/S4a. Ces nombres sont des comptes analytiques, **pas** des
durées mesurées ni une borne pour tous les arbres. Le coût reste
`O(n log n)` sur cet arbre, donc sous-quadratique, mais il pèse dans le
contrat de dizaines de millions de points et 100 ms.

La validation précède les événements CUDA de chaque batch ; les durées
internes `*_device_ms`/noyau ne la contiennent pas. Le mur de chaîne la
paie bien. Publier séparément `index_validate_ms` et le nombre de passages
dans une sonde G4 avant d'attribuer tout écart entre temps appareil et
chaîne aux transferts ou à q4.

**Voie constructive.** L'index et ses tableaux sont préparés une fois par
chaîne puis transmis immuablement à S2, S3 et S4a. Porter un propriétaire
interne certifié lors de cette préparation et vérifier à chaque appel
seulement les nouveaux rectangles/arêtes, capacités et la correspondance
du propriétaire ; garder la validation complète pour l'entrée publique
non certifiée. Une alternative qui conserve le contrôle géométrique sans
rescanner chaque ancêtre : sur un arbre effectivement partitionné, calculer
les extrema des feuilles une fois puis ceux des parents en ordre inverse,
et vérifier que chaque boîte contient les extrema calculés. Cette
induction coûte `O(n + nombre de nœuds)` ; elle requiert une porte sur la
partition et les liens de l'arbre, et ne doit pas simplement croire les
boîtes des enfants. Faire muter une boîte parent qui exclut un point et
une feuille mal liée pour garder le refus causal.

## Le plafond d'arène CUDA n'est pas un repli mémoire universel

Le WIP CUDA borne l'arène par défaut à
`min(4 × edges + 4096, free_bytes / (8 × 128))` en enregistrements de
128 octets (`filter_runner.cu:789–814`). C'est un progrès : si une
réservation dépasse **l'arène déjà allouée**, la seule arête est marquée
`deferred`, puis `Engine::certified_edge` reprend q3 sur CPU ; les autres
tranches décidées sont compactées sans trou. Le résultat reste exact sur
ce chemin à la lecture du source.

`free_bytes` est toutefois lu **avant** les allocations de l'index, des
arêtes, des sorties, des slabs et de l'arène (`:829–853`). Les slabs
peuvent employer jusqu'à un autre quart de cette mémoire libre. Un
`cudaMalloc` de ces buffers peut encore échouer : il devient
`BatchError::capacity`, puis `ChainStatus::kResourceExhausted`
(`tower_chain.cpp:316–325`), **sans** traîne CPU. Le commentaire WIP
« never refuses the call » ne vaut donc que pour le débordement des
enregistrements après allocation réussie. Pour des scènes massives,
budgéter les buffers fixes avant de choisir arène et nombre de warps,
puis prévoir un retry contrôlé ou des lots bornés si l'allocation échoue.

Quand l'arène déborde, `q3_lane` a déjà exécuté son census GPU ; le CPU
recalcule ensuite l'arête. Le ledger `lanes_*` n'ajoute que les arêtes
**décidées** (`filter_runner.cu:729–760`) et ne compte donc pas ce travail
GPU perdu. Le prochain reçu doit publier capacité d'arène, réservations,
reports par cause, temps de noyau et de traîne, allocations, validation et
mur de chaîne, sur plein, moitiés, quarts et densités 1/4–1/2–1. Un
changement de mémoire libre peut changer la fraction reportée sans
changer les octets d'entrée ; comparer des essais appariés avec cette
fraction visible. Aucun contrat G4/FULL n'est inféré de ce WIP.
