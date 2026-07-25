# Architecture GPU G4 de MorseHGP3D

> **Objet.** Ce document traduit la spécification mathématique en architecture de calcul. Il ne remplace ni les preuves, ni les benchmarks. Les cibles de latence sont des portes expérimentales.

## 1. Machine de référence

La cible initiale est `g4-standard-48` : un GPU NVIDIA RTX PRO 6000 Blackwell Server Edition avec 96 Go de GDDR7, 48 vCPU et 180 Go de mémoire hôte. La configuration et les zones doivent être revérifiées avant chaque campagne dans la [documentation G4 de Google Cloud](https://cloud.google.com/compute/docs/accelerator-optimized-machines). Les caractéristiques du GPU sont publiées par [NVIDIA](https://www.nvidia.com/en-us/data-center/rtx-pro-6000-blackwell-server-edition/).

Conséquences :

- architecture mono-GPU d'abord;
- propositions géométriques FP32 seulement lorsqu'elles sont explicitement `proposal_only`; les prunes de Phase 9 utilisent au minimum des intervalles FP64 dirigés avant rejeu exact;
- décisions filtrées, sans supposer un débit FP64 de datacenter;
- 76,8 Go au plus alloués par le cœur, soit 80 % de la VRAM;
- mémoire hôte utilisée pour les runs triés, checkpoints et fallbacks;
- aucun benchmark mesuré pendant une compilation JIT.

Les Tensor Cores ne sont pas au centre du problème. Les opérations dominantes sont parcours de produits BVH, bornes de boîtes, compactions, tris, prédicats et union–find.

## 2. Couches logicielles

```text
Python API
    validation légère, configuration, vues DLPack, sérialisation

C++20 orchestration
    états, budgets, checkpoints, files CPU de fallback

CUDA core
    LBVH, branch-and-bound supports 2/3/4, rangs, tris, lots DSU

Exact predicates
    filtres FP64, expansions GPU, multiprécision CPU

Reference backend
    supports/Gamma exhaustifs CPU bornés, indépendants du target produit
```

PyTorch ne doit pas être une dépendance obligatoire du cœur. Une intégration optionnelle peut accepter un tenseur CUDA via DLPack sans copie. La liaison Python utilisera nanobind ou pybind11 et ne créera jamais un objet Python par support ou événement.

## 3. Sémantique des précisions

| étage | usage | pouvoir de décision |
|---|---|---|
| FP32 | coordonnées de proposition, Morton, bornes approchées, ordre de priorité | aucun si marge non certifiée |
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

### 4.2 Frontières de supports et journal hiérarchique

Pour $n\geq1$, poser $K_{\mathrm{eff}}=\min(K_{\max},n)$ et $s_{\max}=\min(K_{\mathrm{eff}}+1,n)$. Le chemin produit utilise $s_{\max}$ uniquement comme seuil de rang : il ne crée aucun univers de labels de cardinal $k$, aucune couche top-$m$ et aucune coface Gamma. Une seule exploration des supports alimente tous les ordres demandés.

```text
SupportFrontierSoA
    node_id_0[capacity] ... node_id_3[capacity]
    support_size[capacity]
    traversal_state[capacity]
    guaranteed_interior_count[capacity]
    replay_parent[capacity]

LeafSupportSoA
    point_id_0[count] ... point_id_3[count]
    support_size[count]
    proposal_flags[count]

MorseJournalSoA
    event_kind[count]
    exact_level_handle[count]
    support_ids[count][4]
    closed_rank[count]
    attachment_offsets[count + 1]
```

Les files sont bornées par capacité et traitées en deux passes count/emit ou par scans segmentés. Une saturation conserve la frontière non traitée dans `PartialScope` et retourne `budget_exhausted`; elle ne tronque jamais silencieusement. Les tuples feuilles et événements sont triés par clé totale avant déduplication. Un hash accélère le tri, mais toute collision est résolue par les identifiants complets.

### 4.3 Témoins géométriques éphémères

Une boule, un polytope local ou une cellule de Voronoï peut être construit comme témoin temporaire d'un candidat ou dans un test différentiel, puis doit être évincé après classification. Il n'existe pas de `CellArena` produit, pas de couture globale et pas de reconstruction canonique de chaque label enfant.

Geogram, Paragram ou la primitive H-polytope interne peuvent fournir des propositions et des oracles de test. Si Voronoï n'est nécessaire qu'à un différentiel, l'adaptateur Geogram est préféré à une nouvelle implémentation. Aucun de ces moteurs ne décide seul un prune, une complétude ou un statut public.

### 4.4 Événements

Les événements sont en SoA, environ 96 à 160 octets par enregistrement selon le niveau d'audit. Les listes intérieures ont au plus $m_{\star}$ identifiants et les supports au plus quatre dans le domaine générique initial. Les valeurs exactes volumineuses vivent dans une arène séparée et sont référencées par handle.

ExactArena conserve numérateurs, dénominateurs positifs, représentation lazy éventuelle, hash canonique et approximation filtrée. Deux handles mathématiquement égaux doivent se sérialiser sous la même forme canonique avant déduplication, checkpoint ou sortie publique.

Chaque événement stocke aussi l'indice $\mu$, la multiplicité $\Delta=\binom{\lvert U\rvert-1}{\mu}$, les offsets de ses bras et, après réduction, le nombre de classes $H_0$ effectivement tuées. Pour $\mu=1$, les $\lvert U\rvert$ bras sont traités dans un lot unique; le DSU peut tuer de zéro à $\lvert U\rvert-1$ classes, sans binariser artificiellement la multifusion.

### 4.5 Forêts

Les forêts, DSU, ancres et lots sont des tableaux d'identifiants compacts. Les unions de points ne sont matérialisées qu'à la demande sous forme de CSR ou bitmap compressé; pour $k\geq2$, elles peuvent se recouvrir.

## 5. Allocateur, stockage et budgets typés

Un seul allocateur device est autorisé : `cudaMallocAsync` ou RMM après benchmark, jamais les deux en concurrence. Le planificateur réserve :

- 20 % de VRAM au runtime, aux bibliothèques et aux fallbacks;
- deux arènes alternées pour frontières et compaction;
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

L'implémentation actuelle construit le Morton/LBVH récursivement sur CPU, puis les contextes CUDA des Phases 4–5 chargent un snapshot immuable. Cette voie suffit au premier noyau de Phase 9, mais pas à la qualification `warm_e2e` ni au régime 10 M+. Le target doit construire sur device les codes Morton, le radix sort stable, la topologie et les AABB bottom-up, puis vérifier son manifeste contre la référence.

L'index construit une fois sert à :

- ordonner canoniquement les points et exposer des AABB SoA;
- parcourir les produits auto-duaux de deux, trois ou quatre nœuds supports;
- compter par sous-arbre des témoins garantis strictement intérieurs à toutes les boules d'un produit;
- arrêter un comptage dès que le rang utile est dépassé;
- proposer le rang et le shell global d'un support feuille, puis laisser `reference_cpu` ou la cascade exacte les décider;
- proposer les voisins utiles aux descentes et attaches sans leur donner de pouvoir d'exclusion.

Pour les paires, une requête warp-coopérative borne $\phi(x,u,v)=(x-u)\mathbin{\cdot}(x-v)$ sur trois AABB. Une borne supérieure strictement négative compte le sous-arbre témoin entier; zéro impose une descente. Les témoins comptés forment une antichaîne de plages Morton deux à deux disjointes et disjointes des supports, afin qu'aucun point ne soit compté deux fois. Pour les triplets et quadruplets, les filtres de dépendance, barycentriques et d'in-sphère obéissent au même contrat proposition--rejeu exact.

### 6.1 Pruneur P2--P5c de rang pair borné

14Q réalise le parcours de rang pair sans catalogue de paires. Le contexte partagé P1/P2 conserve pour chacun des $2n-1$ nœuds ses six bornes dyadiques, sa plage Morton et ses deux enfants, soit exactement $80(2n-1)$ octets. Un lot contient au plus $P$ produits déjà canoniques. Deux frontières de capacité $W$ alternent; chaque epoch compte les sorties, exécute deux scans exclusifs puis émet stablement descendants et terminaux.

Le premier diagnostic G4 P2 à 12 500 points `uniform_latin` mesure 1 395,5 ms pour le chunk CPU historique, 1 804,5 ms au premier passage assisté et 1 654,3 ms résident. Les 43,5 millions d'items visités et 2 352 epochs ne donnent que 45 propositions sur 1 741 produits, dont 26 consommées. L'assistance est donc plus lente; le couple parcours répétés--faible rendement est le goulot observé. Ce résultat arrête l'escalade de P2 et motive P3/P4.

P3 calcule sur device une borne inférieure binary64 dirigée de $\phi$ pour une paire-ancre authentifiée. Une boîte proposée non intérieure ne devient `anchor_noninterior` qu'après recalcul dyadique exact sur CPU; elle ne descend alors plus. P4 réunit ces exclusions, les terminaux stricts et les feuilles externes dans une coupe canonique de capacité $C$. Un terminal device contient deux mots `uint64_t`, soit 16 octets; les deux supports sont des intervalles implicites. Hors snapshot LBVH et P1, la réservation fixe, scan inclus, devient exactement $40P+80W+16C+8+T_{\mathrm{scan}}$ octets. L'audit distingue D2H physique et actif, les trois classes terminales, prune, keep, fallback, digest et cause d'arrêt.

Le device ne publie pas de décision. L'hôte authentifie et reclassifie exactement chaque terminal. Il produit un prune seulement après un préfixe strict minimal atteignant le seuil. Il produit un keep conservatif seulement si les supports implicites et les terminaux pavent exactement la racine et si le seuil strict n'est pas atteint. Les produits absents ou inachevés retombent sur le parcours CPU historique. L'enveloppe consommée est éphémère et son vérificateur frais rejoue prune et keep; checkpoint, wire et vérificateur durable v1 restent inchangés.

Le commit `d1e6d54` évite la construction et la réduction de rationnels pour les seules décisions de signe en gardant des dyadiques exacts non normalisés. Un passage local budgeté par famille et taille mesure `uniform_latin` à 626,180, 875,731 et 752,328 ms, puis `eight_clusters` à 299,284, 301,732 et 316,609 ms pour 12 500, 25 000 et 50 000 points. Tous les chunks finissent `budget_exhausted`; ces valeurs ne décrivent ni un pipeline complet, ni un p95.

P5 conserve la capacité device $16C$, mais dimensionne le vecteur hôte au nombre actif $T$ et ne copie que $16T$ octets. Le test CUDA réel ciblé passe au SHA `a012af982e1e75ec5f9ba9c5a17d16178b795f90`. Sur `uniform_latin` à 12 500 points, $K=10$, `work=20000`, $P=1$, $W=32768$, $C=16384$ et $E=64$, le composant et son vérificateur frais réussissent sans cap ni fallback GPU. Les 223 callbacks exécutent 3 404 epochs, visitent 46 145 items et font recertifier exactement 22 381 terminaux par le CPU. Les 23 prune et 200 keep proposés donnent 22 prune et 199 keep consommés, avec deux replis CPU historiques.

Le D2H terminal physique vaut désormais l'actif, 358 096 octets, au lieu de 58 458 112 octets avant P5, soit 99,39 % de moins. Ce gain de trafic ne franchit pas le gate de vitesse : le CPU historique prend 325,031 ms, l'assistance 808,867 ms au premier passage et 646,858 ms résidente. Le résident précédent à 665,206 ms n'autorise qu'un constat inter-run d'environ 2,76 %, pas une distribution de performance. La boucle synchronisée `count`--scans--`emit`, répétée par epoch, est donc le coût dominant observé.

P5a implémente localement le cas $P=1$. Un mot de corde de 4 octets par nœud donne le successeur stackless left-first : un nœud développé avance vers son fils gauche, un nœud terminal saute vers sa corde. Un unique kernel borné parcourt le produit et écrit sa coupe sans frontière, scan ou boucle hôte par niveau. Sur snapshots résidents, le contrôle de 40 octets impose une synchronisation; un transcript actif non vide en impose une seconde pour copier exactement ses $16T$ octets. Toute borne $Q=\min(N,WE)$ ou terminale dépassée invalide la proposition entière et déclenche le parcours CPU historique. La recertification dyadique exacte, l'authentification des intervalles, la couverture de racine et les décisions prune ou keep restent exclusivement sur CPU.

Cette structure ajoute $O(n+P+W+C)$ et aucun terme en nombre global de paires; P5a ajoute seulement 4 octets par nœud du LBVH déjà construit et son workspace propre vaut $32P+16C+40$ octets. Elle ne matérialise ni facette, coface, incidence globale, cellule, structure Gamma, arène globale de paires ou mosaïque de Delaunay d'ordre supérieur. Les tests hôte ciblés ferment le layout, les caps $Q/C$, le rejeu résident et le fallback entier.

Le petit gate G4 P5a au SHA `cd4ab8b0a5cddf24fdb060073232186b5ba716b4` échoue à son tour. Sur `uniform_latin` à 12 500 points, le CPU historique prend 320,620 ms, le premier passage assisté 1 094,620 ms et le résident 934,077 ms. Les 222 callbacks lancent 222 kernels stackless, sans kernel de comptage ni scan, visitent 41 243 nœuds, imposent 444 synchronisations résidentes et font recertifier exactement 20 176 terminaux par le CPU. Le composant et son vérificateur frais réussissent sans cap ni fallback GPU; 22 prune et 200 keep sont proposés, 22 et 199 consommés, puis un produit suit le fallback CPU historique. La suppression des epochs ne compense donc ni le worker device sériel, ni les synchronisations par produit, ni la recertification exacte.

P5b introduit une enveloppe CPU exacte bornée : après alignement d'au plus 124 bits par coordonnée, des entiers de 256 bits donnent les signes maximum et ancre; hors enveloppe, le chemin BigInt reste obligatoire avant décision. Au SHA exact `159edc0c39e4e56e02ba4bc3b1f5cdd5889320bf`, le même gate mesure 298,696 ms pour le CPU historique, 1 035,830 ms au premier passage assisté, 846,446 ms au résident et 277,460 ms au vérificateur frais. Les comptes scientifiques et opérationnels P5a restent inchangés, sans cap ni fallback GPU. Les gains inter-run de 6,84 % sur le CPU et 9,38 % sur le résident sont utiles mais insuffisants : le résident reste 2,83 fois plus lent que le CPU.

P5c applique directement l'identité exacte $\left\Vert x-\frac{u+v}{2}\right\Vert^2-\frac{\left\Vert u-v\right\Vert^2}{4}=(x-u)\cdot(x-v)$ dans la classification boule--AABB CPU. Le minimum exact de $\phi$ strictement positif ferme un sous-arbre extérieur; le maximum exact strictement négatif ferme un sous-arbre intérieur; l'égalité descend. Les extrema restent séparables sur les trois axes et utilisent les entiers exacts bornés P5b avec fallback BigInt. Le centre rationnel et `ExactLevel` ne sont plus matérialisés pour les paires rejetées, et aucune structure globale supplémentaire n'est retenue. Pour compatibilité du schéma d'audit, `exact_point_distance_evaluation_count` compte les classifications exactes de feuilles non résolues par les tests stricts, fournies par les extrema de $\phi$ et nécessairement à égalité sur une boîte dégénérée, malgré son nom historique.

Au SHA exact `a5780312906230e92065e008ee7ac3d362a09bdc`, le gate G4 P5c mesure 28,093 ms sur le CPU historique, 796,881 ms au premier passage assisté, 607,506 ms au résident et 31,925 ms au vérificateur frais. Les baisses inter-run contre P5b valent 90,59 % sur le CPU et 28,23 % sur le résident, sans revendication statistique. Les 222 callbacks et kernels, 41 243 visites, 444 synchronisations, 20 176 recertifications exactes, 22 prune et 200 keep proposés, 22 et 199 consommés et un fallback CPU restent inchangés, sans cap ni fallback GPU. La classification exacte n'est donc plus le terme dominant du CPU, mais le worker device sériel et ses deux synchronisations par produit laissent le résident 21,62 fois plus lent que le CPU.

Le chemin interactif peut désormais évaluer P5c CPU sous une porte distincte, tandis que le chemin massif exige toujours des lots GPU réellement parallèles et des synchronisations amorties. L'escalade à `eight_clusters`, 50 k et 10 M reste arrêtée pour ce gate assisté. Les artefacts [phase14q_p5b_g4_159edc0.json](validation/phase14q_p5b_g4_159edc0.json) et [phase14q_p5c_g4_a578031.json](validation/phase14q_p5c_g4_a578031.json) restent des diagnostics de composant, jamais le protocole `warm_e2e`, un SLO 50 k ou une preuve de capacité produit à 10 M.

Une recherche approchée ou une petite liste locale peut prioriser la frontière; le parcours global et la fermeture de sa frontière restent obligatoires en mode exact.

## 7. Voronoï et puissance : propositions et tests seulement

Le chemin produit n'a pas besoin de construire un diagramme de Voronoï ou de puissance global. Ces moteurs restent utiles pour proposer des supports, visualiser et bâtir des différentiels bornés.

Pour un oracle Voronoï CPU de test, Geogram est préféré à une réimplémentation locale. Le projet [Paragram](https://github.com/zenseact/paragram), code associé à Taveira et al., reste la comparaison GPU pertinente pour clipping, culling directionnel, BVH et chunking. Leurs adjacences et flottants ne suffisent pas au certificat E-HGP.

Plan d'intégration :

1. adapter Geogram pour les tests CPU qui demandent réellement Voronoï;
2. benchmarker Paragram sans modification pour les seules propositions GPU pertinentes;
3. vérifier les conventions de poids sur des cellules analytiques;
4. compiler une variante d'audit sans fast math;
5. comparer les supports proposés au flux direct et à l'oracle exhaustif borné;
6. conserver l'adaptateur optionnel seulement s'il réduit le temps de développement ou le travail GPU mesuré.

Le cœur E-HGP ne dépendra pas du statut `success` du moteur externe pour publier `exact`.

## 8. Noyaux CUDA

| noyau | granularité | sortie |
|---|---|---|
| validation et doublons | thread par point puis radix sort | sites canoniques |
| Morton et LBVH | primitives CCCL/CUB | index global |
| expansion auto-duale des supports | warp par unité de frontière | produits enfants canoniques |
| borne diamétrale support deux | warp par produit et nœuds témoins | prune proposé ou descente |
| bornes supports trois/quatre | warp ou bloc par produit | prune proposé ou descente |
| compaction de frontière | scan et sélection | prochaine frontière bornée |
| centre support 2/3/4 | warp par tuple feuille | centre et barycentriques proposés |
| rang et shell | warp par support feuille | événement, diagnostic ou rejet proposé |
| rejeu exact de tous les prunes certifiants | lots CPU bornés | décisions certifiées |
| déduplication des événements | radix sort et comparaison complète | flux canonique |
| miniball $k\leq K_{\mathrm{eff}}$ | warp par facette | centre, rayon et support |
| successeur top-$k$ | warp par centre | arc de descente |
| comparaison de niveaux | filtre puis file exacte | lots exacts |
| quotient de lot | bloc par niveau | nœuds de fusion et DSU |
| bras de Morse | warp par événement | racines antérieures, classes tuées |
| morphismes verticaux | tableaux compacts | ancres et images |
| coupe | kernels CSR | composantes et couverture |

CCCL/CUB fournit radix sort, scan, sélection, réduction et tri segmenté. Les primitives choisies et leur espace temporaire doivent être versionnés dans le manifeste de benchmark.

## 9. Ordonnancement faible latence jusqu'à 50 000 points

Après le transfert initial, le chemin de faible latence garde points, LBVH, deux frontières alternées, flux d'événements et DSU sur le device :

1. validation et index une seule fois;
2. expansion count/emit de la frontière auto-duale, d'abord pour les supports deux puis pour trois et quatre;
3. prunes proposés par intervalles GPU, compaction immédiate des produits ambigus et mise en lots de tous les prunes qui pourraient contribuer à une sortie certifiée;
4. classification des tuples feuilles, puis tri et déduplication du flux partagé;
5. construction du journal Morse et réduction des $K_{\mathrm{eff}}$ tranches sans copie hôte;
6. vérification des gateways et des ancres;
7. copie finale compacte seulement.

Les séquences de kernels stabilisées sont capturées dans des CUDA Graphs. Les compteurs et files restent sur device; une synchronisation hôte n'est autorisée qu'à une frontière de phase, une décision de budget ou un fallback réel.

Le protocole distingue :

- `cold` : chargement du processus et des bibliothèques;
- `warm` : contexte créé, données à copier;
- `resident_core` : données et index déjà sur GPU, métrique diagnostique secondaire.

Le SLO produit principal est `warm_e2e` : runtime et allocateur initialisés, mais validation, transfert H2D, construction du LBVH, calcul et matérialisation de la sortie inclus.

## 10. Streaming transactionnel à dix millions de points et davantage

Lorsque la mémoire le permet, les points et le LBVH restent résidents; à dix millions de points ou davantage, les frontières, événements et attaches sont diffusés transactionnellement. Le premier mode scalable traite ces objets par lots bornés :

1. sélectionner un segment canonique de frontière selon les budgets mémoire et temps;
2. produire ses propositions de prune, les reçus de leur recertification exacte, ses descendants ambigus et ses tuples feuilles contre le LBVH global;
3. classifier les feuilles et émettre événements, diagnostics et compteurs;
4. écrire des runs triés et checksummés de descendants, événements et replays exacts en mémoire hôte, sur Titanium SSD ou sur Hyperdisk selon leur durée de vie;
5. publier atomiquement le manifeste du segment, puis libérer ses buffers device;
6. fusionner et dédupliquer les runs de la génération;
7. reprendre jusqu'à frontière vide ou arrêt budgétaire;
8. réduire le flux complet en journal Morse, lui-même streamé par lots de niveaux égaux.

Une unité de frontière peut être évincée seulement après publication atomique de l'une de ses deux issues : prune exact rejouable, ou liste complète de descendants/feuilles. Le run d'événements n'affirme la complétude qu'après frontière vide pour chacune des tailles deux, trois et quatre. Aucune cellule top-$m$, coface Gamma ou couture géométrique n'apparaît dans ce protocole.

Le véritable out-of-core des **points** est différé. Il demandera une partition Morton, un répertoire global d'AABB et le chargement best-first de chunks jusqu'à certification. Un halo spatial fixe n'est pas exact.

## 11. Checkpoint et reprise Spot

Un checkpoint atomique contient :

- hash des points canoniques et de la configuration;
- taille de support, génération et segment courant de la frontière;
- unités de frontière committées, runs descendants et empreintes;
- flux d'événements committé avec statut par événement;
- `PartialScope`, tailles complètes, frontière restante et replays exacts encore attendus;
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

La combinaison `mode="budgeted", forest_semantics="partial_refinement"` est incompatible avec `require_exact=True`. Elle peut restituer les événements individuellement certifiés, les tailles de support closes, les préfixes de journal fermés et les loci dégénérés dans `PartialScope`; elle porte nécessairement `public_status="conditional"` ou `public_status="budget_exhausted"`. Une réponse d'événements seuls omet `forest_semantics` et fournit `PartialScope`.

## 14. Instrumentation obligatoire

Les plages NVTX séparent : validation, LBVH, expansion par taille de support, bornes, compaction, rejeu exact, classification feuille, tri, réduction, attaches, morphismes et sérialisation. Les métriques comprennent :

- temps GPU et mur par phase et par ordre;
- H2D et D2H séparés;
- pic VRAM et mémoire hôte;
- unités de frontière lues, prunées, descendues et réémises par taille de support;
- pics de frontière, tuples feuilles et octets de runs;
- visites BVH par type de requête;
- produits prunés par bon centrage, rang ou dépendance et produits ambigus;
- saturations de capacité et replays exacts;
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
| $50\,000$ | principal $<100$ ms; secondaire $<1$ s sur le même payload |
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
