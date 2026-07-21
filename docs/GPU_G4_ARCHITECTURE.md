# Architecture GPU G4 de MorseHGP3D

> **Objet.** Ce document traduit la spécification mathématique en architecture de calcul. Il ne remplace ni les preuves, ni les benchmarks. Les cibles de latence sont des portes expérimentales.

## 1. Machine de référence

La cible initiale est `g4-standard-48` : un GPU NVIDIA RTX PRO 6000 Blackwell Server Edition avec 96 Go de GDDR7, 48 vCPU et 180 Go de mémoire hôte. La configuration et les zones doivent être revérifiées avant chaque campagne dans la [documentation G4 de Google Cloud](https://cloud.google.com/compute/docs/accelerator-optimized-machines). Les caractéristiques du GPU sont publiées par [NVIDIA](https://www.nvidia.com/en-us/data-center/rtx-pro-6000-blackwell-server-edition/).

Conséquences :

- architecture mono-GPU d'abord;
- propositions massivement FP32 et sensibles à la bande passante;
- décisions filtrées, sans supposer un débit FP64 de datacenter;
- 76,8 Go au plus alloués par le cœur, soit 80 % de la VRAM;
- mémoire hôte utilisée pour les runs triés, checkpoints et fallbacks;
- aucun benchmark mesuré pendant une compilation JIT.

Les Tensor Cores ne sont pas au centre du problème. Les opérations dominantes sont clipping, parcours BVH, tris, réductions, prédicats et union–find.

## 2. Couches logicielles

```text
Python API
    validation légère, configuration, vues DLPack, sérialisation

C++20 orchestration
    états, budgets, checkpoints, files CPU de fallback

CUDA core
    LBVH, diagrammes restreints, rangs, miniballs, tris, hyper-Kruskal

Exact predicates
    filtres FP64, expansions GPU, multiprécision CPU

Reference backend
    oracle exhaustif CPU indépendant
```

PyTorch ne doit pas être une dépendance obligatoire du cœur. Une intégration optionnelle peut accepter un tenseur CUDA via DLPack sans copie. La liaison Python utilisera nanobind ou pybind11 et ne créera jamais un objet Python par cellule ou événement.

## 3. Sémantique des précisions

| étage | usage | pouvoir de décision |
|---|---|---|
| FP32 | coordonnées de proposition, Morton, clipping approché, ordre de priorité | aucun si marge non certifiée |
| FP64 filtré | évaluation avec borne d'erreur | décision si l'intervalle exclut zéro |
| expansions GPU | signes de déterminants, formes affines et produits croisés ambigus | décision exacte du signe |
| multiprécision CPU | cas rares, égalités et diagnostic | décision exacte finale |

`--use_fast_math` est interdit dans tout translation unit qui produit un signe certifiant. Les approximations peuvent vivre dans des kernels séparés, avec des sorties marquées `proposal_only`.

Un compteur par type de prédicat enregistre le nombre de décisions à chaque étage. Un taux de fallback élevé est un problème de performance, jamais une raison de diminuer la précision.

Dyadique en entrée ne signifie pas dyadique après résolution d'un support de taille trois ou quatre. Les centres et niveaux utilisent ExactRational3 et ExactLevel, sous forme homogène $(num,den)$ avec $den>0$, réduite canoniquement ou conservée lazy avec une clé canonique. Comparer deux niveaux revient à décider le signe exact d'un produit croisé; les expansions servent au signe, jamais à simuler la division, et un fallback bigint termine les cas non décidés. La valeur FP64 associée n'est qu'une clé de tri préalable.

## 4. Structures de données

### 4.1 Observations

```text
PointsSoA
    x[n], y[n], z[n]              # type d'entrée conservé
    site_id[n]                    # uint32 pour dix millions de points et davantage
    multiplicity[n]               # option future
    morton_code[n]                # uint64
```

Les AABB du LBVH utilisent des bornes sûres. Les coordonnées réordonnées gardent une table de permutation vers les identifiants canoniques.

### 4.2 Labels géométriques et hiérarchiques

Pour $n\geq1$, poser $K_{\mathrm{eff}}=\min(K_{\max},n)$ et $s_{\max}=\min(K_{\mathrm{eff}}+1,n)$. Si $s_{\max}\geq2$, la profondeur générique vaut $m_{\star}=s_{\max}-2$; si $s_{\max}=1$, les minima de rayon nul suffisent et aucune cellule n'est raffinée. Le lemme d'arrêt $H_0$ distingue donc trois tailles : intérieur $m\leq m_{\star}$, facette $k\leq K_{\mathrm{eff}}$ et ensemble fermé $s\leq s_{\max}$. Les maxima neuf, dix et onze ne valent que pour $K_{\max}=10$ et $n\geq11$.

```text
TopCellLabelArena[m <= m_star]
    ids[count][m]
    hash[count]
    parent_offsets[count + 1]
    status[count]

CanonicalChildArena[q <= m_star + 1]
    ids[count][q]
    terminal[count]
    certificate_handle[count]

FacetLabelArena[k <= K_eff]
    ids[count][k]

ClosedEventSetArena[s <= s_max]
    ids[count][s]
```

Chaque label est trié et stocké à stride fixe dans son arène. Un hash 64 ou 128 bits accélère le tri, mais toute collision est résolue par comparaison du label complet.

### 4.3 Cellules et morceaux

Les polyèdres sont stockés dans des arènes CSR : plans, sommets approchés, plans liants et incidences. Aucun `new` par cellule. Les fast paths ont des capacités explicites par classe de complexité; tout dépassement va dans une file d'overflow, jamais dans une troncature.

```text
CellArena
    cell_offsets
    plane_ids
    vertex_offsets
    approximate_xyz
    binding_plane_ids
    artificial_boundary_bits
    closure_status
```

La v1 ne recolle pas les fragments géométriques issus de parents différents. Après déduplication d'un label enfant $Q$, elle reconstruit une unique cellule canonique depuis la boîte strictement paddée $\Omega$. Les fragments émettent seulement un `FragmentHint` composé d'un label, de strates candidates et de contraintes $Q\times(X\setminus Q)$ globalement sûres. Ces amorces peuvent accélérer le clipping, mais ne participent jamais au certificat de complétude.

### 4.4 Événements

Les événements sont en SoA, environ 96 à 160 octets par enregistrement selon le niveau d'audit. Les listes intérieures ont au plus $m_{\star}$ identifiants et les supports au plus quatre dans le domaine générique initial. Les valeurs exactes volumineuses vivent dans une arène séparée et sont référencées par handle.

ExactArena conserve numérateurs, dénominateurs positifs, représentation lazy éventuelle, hash canonique et approximation filtrée. Deux handles mathématiquement égaux doivent se sérialiser sous la même forme canonique avant déduplication, checkpoint ou sortie publique.

Chaque événement stocke aussi l'indice $\mu$, la multiplicité $\Delta=\binom{\lvert U\rvert-1}{\mu}$, les offsets de ses bras et, après réduction, le nombre de classes $H_0$ effectivement tuées. Pour $\mu=1$, les $\lvert U\rvert$ bras sont traités dans un lot unique; le DSU peut tuer de zéro à $\lvert U\rvert-1$ classes, sans binariser artificiellement la multifusion.

### 4.5 Forêts

Les forêts, DSU, ancres et lots sont des tableaux d'identifiants compacts. Les unions de points ne sont matérialisées qu'à la demande sous forme de CSR ou bitmap compressé; pour $k\geq2$, elles peuvent se recouvrir.

## 5. Allocateur, stockage et budgets typés

Un seul allocateur device est autorisé : `cudaMallocAsync` ou RMM après benchmark, jamais les deux en concurrence. Le planificateur réserve :

- 20 % de VRAM au runtime, aux bibliothèques et aux fallbacks;
- deux arènes alternées pour clipping et compaction;
- un buffer de tri dimensionné par la primitive CCCL/CUB;
- une réserve d'urgence qui permet de sérialiser un checkpoint avant arrêt budgétaire.

Chaque type publie ses octets actifs, son pic et son facteur de fragmentation. Avant un lot, une estimation pessimiste décide s'il reste résident ou passe en streaming.

`BudgetPolicy` contient exclusivement des quantités typées :

| champ | unité | portée |
|---|---:|---|
| `device_budget_bytes` | octets | allocations CUDA du cœur et espaces temporaires |
| `host_budget_bytes` | octets | arènes CPU, mémoire épinglée, fallbacks et buffers de merge |
| `scratch_budget_bytes` | octets | runs, temporaires, journaux et checkpoints locaux |
| `output_budget_bytes` | octets | payload public matérialisé, hors scratch interne |
| `time_budget_s` | secondes monotones | calcul, avec réserve séparée pour checkpoint et arrêt |

En l'absence de valeur absolue, le device dérive au plus 80 % de la VRAM et l'hôte au plus 70 % de la RAM détectée; le certificat enregistre les valeurs absolues effectivement retenues. Un streaming certifié exige un `scratch_budget_bytes` explicite et une réserve de checkpoint qui n'est jamais prêtée aux runs.

Sur G4, le stockage persistant pris en charge est **Hyperdisk** et le disque de démarrage est Hyperdisk Balanced. `g4-standard-48` peut recevoir jusqu'à quatre Titanium SSD pour le scratch rapide. Le manifeste distingue Hyperdisk durable, Titanium SSD éphémère et mémoire hôte; un checkpoint destiné à survivre à l'instance est copié sur Hyperdisk avant de considérer l'étape durable.

La garde disque est transactionnelle. Avant toute écriture, elle démontre que le scratch libre couvre simultanément l'ancien état, le nouveau fichier temporaire, le pire espace de merge, le prochain checkpoint et une marge fixe. Elle écrit dans un nom temporaire, synchronise données et répertoire, vérifie taille et checksum, renomme atomiquement, publie le manifeste, puis seulement libère l'ancien état. Si cette preuve échoue, elle n'écrit rien de nouveau et retourne `budget_exhausted` depuis le dernier checkpoint cohérent.

## 6. Index spatial global

Un Morton sort suivi d'un LBVH est construit une fois. Il sert à :

- proposer les sites voisins d'une cellule;
- calculer le plus proche voisin exact hors d'un petit label $I$;
- calculer le top-$k$ et le shell complet;
- compter l'intérieur d'une sphère avec arrêt dès que le rang dépasse $s_{\max}$;
- certifier l'absence de point dans une miniball de Gabriel.

Une requête warp-coopérative maintient un petit heap de taille $K_{\mathrm{eff}}+2$. L'exclusion de $m_{\star}$ identifiants au plus est faite dans des registres ou une petite table partagée. Les AABB ne sont élaguées que lorsque leur borne inférieure certifiée dépasse strictement le seuil courant.

Une recherche approchée peut fournir le seuil initial; le parcours global reste obligatoire en mode exact.

## 7. Primitive de diagramme de puissance

Le moteur doit accepter de nombreux problèmes indépendants : un domaine convexe parent, une liste initiale de sites et une liste d'exclusions. Il retourne les cellules restreintes, leurs sommets, les plans liants et les incidences.

Le projet [Paragram](https://github.com/zenseact/paragram), code associé à Taveira et al., est une base de comparaison pertinente : clipping GPU, culling directionnel, BVH et chunking. Il devra être épinglé au commit audité, testé sans fast math et enveloppé par notre couche de fermeture. Son adjacency CSR et ses flottants ne suffisent pas au certificat E-HGP.

Plan d'intégration :

1. benchmarker la version amont sans modification;
2. vérifier les conventions de poids sur des cellules analytiques;
3. exposer sommets, plans liants et statuts d'overflow;
4. compiler une variante d'audit sans fast math;
5. ajouter l'oracle exact aux sommets;
6. comparer chaque cellule aux oracles CPU;
7. décider ensuite entre adaptateur, fork minimal ou réimplémentation.

Le cœur E-HGP ne dépendra pas du statut `success` du moteur externe pour publier `exact`.

Pour un label enfant dédupliqué $Q$, la fermeture de référence repart toujours de $\Omega$. Elle clippe par une amorce de contraintes cross $Q\times(X\setminus Q)$, puis, à chaque sommet provisoire, calcule exactement le ou les plus éloignés de $Q$ et le 1-NN avec co-ties dans $X\setminus Q$. Toute inégalité violée ou égale manquante est ajoutée et la cellule est reclippée. Seule une file vide produit `CanonicalCellCertificate`; aucune union flottante de fragments ne peut produire ce certificat.

## 8. Noyaux CUDA

| noyau | granularité | sortie |
|---|---|---|
| validation et doublons | thread par point puis radix sort | sites canoniques |
| Morton et LBVH | primitives CCCL/CUB | index global |
| proposition de sites | warp ou bloc par parent | file initiale |
| clipping restreint | bloc par cellule | polyèdre provisoire |
| fermeture aux sommets | warp par sommet | violateurs et co-minimiseurs |
| déduplication des labels | radix sort par ordre | enfants uniques |
| réconciliation des incidences | sort-reduce | strates globales |
| reconstruction canonique | bloc par label enfant | cellule unique, violateurs, co-ties |
| centre support 2/3/4 | warp par strate | témoin de centre et barycentriques |
| rang et shell | warp par candidat | événement accepté ou rejeté |
| miniball $k\leq K_{\mathrm{eff}}$ | warp par facette | centre, rayon et support |
| successeur top-$k$ | warp par centre | arc de descente |
| comparaison de niveaux | filtre puis file exacte | lots exacts |
| hyper-Kruskal | bloc par lot | nœuds de fusion et DSU |
| bras de Morse | warp par événement | racines antérieures, classes tuées |
| morphismes verticaux | tableaux compacts | ancres et images |
| coupe | kernels CSR | composantes et couverture |

CCCL/CUB fournit radix sort, scan, sélection, réduction et tri segmenté. Les primitives choisies et leur espace temporaire doivent être versionnés dans le manifeste de benchmark.

## 9. Ordonnancement faible latence jusqu'à 50 000 points

Après le transfert initial, le chemin de faible latence garde points, LBVH, cellules, catalogue et DSU sur le device :

1. validation et index une seule fois;
2. fermeture ordre par ordre, avec réutilisation des buffers;
3. extraction des événements dès qu'une cellule est fermée;
4. tri du catalogue sur GPU;
5. réduction des $K_{\mathrm{eff}}$ tranches sans copie hôte;
6. vérification des ancres;
7. copie finale compacte seulement.

Les séquences de kernels stabilisées sont capturées dans des CUDA Graphs. Les compteurs et files restent sur device; une synchronisation hôte n'est autorisée qu'à une frontière de phase, une décision de budget ou un fallback réel.

Le protocole distingue :

- `cold` : chargement du processus et des bibliothèques;
- `warm` : contexte créé, données à copier;
- `resident_core` : données et index déjà sur GPU, métrique diagnostique secondaire.

Le SLO produit principal est `warm_e2e` : runtime et allocateur initialisés, mais validation, transfert H2D, construction du LBVH, calcul et matérialisation de la sortie inclus.

## 10. Streaming transactionnel à dix millions de points et davantage

Lorsque la mémoire le permet, les points et le LBVH restent résidents; à dix millions de points ou davantage, les cellules et incidences sont le risque dominant et doivent être diffusées transactionnellement. Le premier mode scalable traite ces objets par lots bornés :

1. sélectionner un lot de parents selon le budget mémoire;
2. construire et fermer chaque morceau contre le LBVH global;
3. extraire événements et signatures d'incidence;
4. écrire des runs de labels, contraintes sûres et événements triés et checksummés en mémoire hôte, sur Titanium SSD ou sur Hyperdisk selon leur durée de vie;
5. libérer définitivement la géométrie fermée;
6. reprendre jusqu'à fermeture de l'ordre;
7. fusionner les runs, dédupliquer les labels et reconstruire chaque cellule canonique indépendamment du chunk d'origine;
8. checkpoint avant l'ordre suivant.

Une cellule peut être évincée seulement si son certificat est indépendant des futures propositions : à chaque sommet, le plus éloigné dans $Q$ a été comparé exactement au 1-NN dans $X\setminus Q$, tous les violateurs et co-ties ont été ajoutés, et la file est vide. Une profondeur ne devient fermée qu'après déduplication globale des labels et fermeture de toutes leurs cellules canoniques. Le complexe de fragments et ses coutures restent une optimisation future derrière preuve et différentiel; ils ne font pas partie du chemin exact v1.

Le véritable out-of-core des **points** est différé. Il demandera une partition Morton, un répertoire global d'AABB et le chargement best-first de chunks jusqu'à certification. Un halo spatial fixe n'est pas exact.

## 11. Checkpoint et reprise Spot

Un checkpoint atomique contient :

- hash des points canoniques et de la configuration;
- ordre et ronde de fermeture;
- labels parents fermés et file restante;
- runs triés et empreintes;
- catalogue partiel avec statut par événement;
- `PartialScope`, cellules canoniques fermées et labels encore attendus;
- état DSU uniquement si tous les lots antérieurs sont fermés;
- arène `ExactLevel` canonique et état des comparaisons différées;
- versions CUDA, pilote, binaire et format.

La reprise refuse tout changement de données, de sémantique numérique ou de version de schéma. Une préemption forcée est injectée dans les tests. Le résultat résident et le résultat repris doivent être identiques octet par octet dans le mode déterministe.

## 12. Déterminisme

Le déterminisme public repose sur :

- labels triés canoniquement;
- radix sorts stables ou clés totales explicites;
- lots définis par égalité exacte;
- pivot d'hyperarête lexicographique;
- hooking DSU déterministe;
- IDs de nœuds attribués après tri sémantique;
- fallbacks rejouables;
- absence de décision par ordre d'arrivée atomique.

Le temps peut varier; la forêt, les niveaux et le certificat sémantique ne le doivent pas.

## 13. API cible

```python
model = MorseHGP3D(
    k_max=10,
    profile="hgp_reduced",
    mode="certified",
    forest_semantics="exact",
    require_exact=True,
    device_budget_bytes=None,     # défaut borné à 80 % de la VRAM
    host_budget_bytes=None,       # défaut borné à 70 % de la RAM
    scratch_budget_bytes=None,    # obligatoire en streaming
    output_budget_bytes=None,
    time_budget_s=None,
    duplicate_policy="reject",
    deterministic=True,
    checkpoint_dir=None,
)

result = model.fit(points)
cut = result.cut(k=7, squared_level=a, membership="points")
```

`require_exact=True` lève une erreur structurée lorsque le profil ne peut être certifié. L'exception contient le checkpoint, la raison, les compteurs et les instructions de reprise; elle ne retourne pas silencieusement une forêt conditionnelle.

La combinaison `mode="budgeted", forest_semantics="partial_refinement"` est incompatible avec `require_exact=True`. Elle peut restituer les événements individuellement certifiés, les profondeurs et ordres fermés, les cellules canoniques closes et les loci dégénérés dans `PartialScope`; elle porte nécessairement `public_status="conditional"` ou `public_status="budget_exhausted"`. Une réponse d'événements seuls omet `forest_semantics` et fournit `PartialScope`.

## 14. Instrumentation obligatoire

Les plages NVTX séparent : validation, LBVH, chaque ordre de raffinement, fermeture, extraction, rang, tri, réduction, attaches, morphismes et sérialisation. Les métriques comprennent :

- temps GPU et mur par phase et par ordre;
- H2D et D2H séparés;
- pic VRAM et mémoire hôte;
- cellules, plans, sommets et colonnes par ordre;
- visites BVH par type de requête;
- overflows et reclippings;
- prédicats FP64, expansions et CPU;
- événements par rang et taille de support;
- taille des lots et unions redondantes;
- longueur des descentes;
- volume des runs et temps de merge;
- nombre de synchronisations hôte.

Un benchmark sans ces compteurs est exploratoire, pas publiable.

## 15. Cibles réfutables

Le payload chronométré est gelé par `BenchmarkOutputContract-v1` : profil `hgp_reduced`, dix forêts horizontales, applications verticales, lots égaux, statut et certificat minimal transférés en mémoire hôte épinglée, puis synchronisation GPU. Validation, H2D, LBVH, catalogue, réduction et transfert de ce payload sont inclus. Sont exclus, mais mesurés séparément : catalogue de replay complet, expansion des unions de points, sérialisation disque et profil `full_pi0` tant que sa phase de preuve n'est pas fermée.

Sur `g4-standard-48`, protocole `warm_e2e`, $K_{\max}=10$, nuages volumiques réguliers et profil exact demandé :

| taille | objectif p95 `warm_e2e` |
|---:|---:|
| $1\,000$ | $25$ ms |
| $10\,000$ | $200$ ms |
| $50\,000$ | $<1$ s |
| $100\,000$ | $3$ s |
| $1$ million | $60$ s si le certificat reste sparse |
| $10$ millions | $10$ min si le certificat reste sparse |

Ces valeurs sont des portes de recherche, pas des prédictions. Une exécution `conditional` ne valide pas la cible exacte. Un pire cas qui dépasse le budget doit être identifié comme tel, pas masqué par une troncature.

## 16. Dépendances pressenties

Obligatoires : CUDA 12.9, C++20, CMake, Ninja, CCCL/CUB, DLPack, une liaison Python légère, Boost.Multiprecision pour le fallback de référence, pytest, Hypothesis et NVTX.

Optionnelles et isolées :

- Paragram pour la primitive de diagramme;
- cuVS brute force comme différentiel top-$k$;
- ArborX ou l'algorithme EMST GPU comme référence $k=1$;
- CGAL comme oracle de test après audit de licence;
- Geogram via un adaptateur versionné comme baseline Voronoï CPU de test; ses sorties ne deviennent jamais une autorité exacte sans rejeu par les contrats du dépôt;
- RMM si ses mesures dépassent `cudaMallocAsync`.

Chaque dépendance optionnelle doit pouvoir être désactivée sans modifier la définition mathématique.

## 17. Sécurité opérationnelle

Aucun gain de performance ne justifie une VM oubliée. Les campagnes utilisent exclusivement les scripts de [`gcp-migration`](../gcp-migration/), avec durée GCE bornée, arrêt invité armé et fermeture finale de l'instance exactement ciblée vérifiée `TERMINATED`. Les autres instances E-HGP sont seulement inventoriées et signalées; elles ne sont jamais bloquantes ni mutées sans autorisation explicite. Les contraintes détaillées de durée et de remise à zéro figurent dans [`AGENTS.md`](../AGENTS.md).
