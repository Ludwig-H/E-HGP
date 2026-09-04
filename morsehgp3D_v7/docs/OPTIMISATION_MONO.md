# Optimisation monothread : audit statique du 4 septembre 2026

Cadre : `phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
Ce document décrit le code lu avant les correctifs mono. Il ne rapporte aucune
nouvelle mesure de débit ni aucun objectif atteint. Les patches sont à qualifier
séparément ; le gel des sources produit reste respecté pendant cet audit.

Mise à jour du 4 septembre : le fold inline a depuis été intégré ; son
[contrat d'appel mono et ses portes](MODE_MONO.md) sont décrits séparément.
Les constats ci-dessous conservent leur portée historique pré-correctif.

Le [contrat de performance](CONTRAT_PERFORMANCE.md) vise **50 000 points et la
tour entière K1..K10 en moins d'une seconde**, pas K10 seul. La tour K1..K5
est uniquement le repli explicitement déclaré si K1..K10 échoue au délai.
L'ordre de travail est monothread, multi-CPU, puis GPU. Un refus rapide, une
censure ou `verified_events_only` ne satisfait pas le contrat exact.

## 1. Les options actuelles ne constituent pas un mono sans worker

La CLI expose `--threads`, `--fold-inflight` et `--fold-join` dans
[`cli/mhgp7.cpp`](../cli/mhgp7.cpp#L136). Pour `threads=1`,
[`parallel_items` et `parallel_ranges`](../src/parallel/pool.hpp#L112)
appellent directement le travail sur le fil appelant ; aucun fil n'est créé.
Le [tri local](../src/parallel/sort.hpp#L231) suit aussi son bras séquentiel.

Cependant, [`run.hpp`](../src/pipeline/run.hpp#L825) prépare A(K), puis crée
inconditionnellement un `std::thread` pour B(K). Le test de capacité des slots
intervient **après** la préparation A(K), ce qui explique pourquoi
`fold-inflight=1` seul ne supprime pas le chevauchement A/B.
[`fold-join=1`](../src/pipeline/run.hpp#L1022) joint B(K) avant A(K+1), mais
ne supprime pas sa création. Jusqu'à dix workers B sont ainsi créés
successivement pour une tour K1..K10, en plus du fil principal.

| Réglage | Conséquence statique |
| --- | --- |
| `threads=1`, join désactivé | Phases locales séquentielles, mais A/B et éventuellement B/B peuvent se chevaucher |
| `threads=1`, inflight=1, join=1 | Une seule phase de calcul à la fois ; B s'exécute encore sur un autre fil |
| Bras inline envisagé, mêmes options | A et B sur le fil appelant, sans worker B ; à qualifier, pas encore disponible |

Le compteur [`ouvriers fold`](../src/pipeline/run.hpp#L1309) vient de
`ForestResult::workers`, essentiellement des primitives de préparation ; il
ne compte pas les créations B. `pic_workers_b=1` désigne une concurrence
maximale, pas « aucun autre thread ». Épingler l'affinité sur un CPU contraint
les threads au même CPU, mais ne rend pas le programme mono sans worker.

## 2. Où le travail et les copies existent réellement

Les coûts ci-dessous sont **des sites de travail**, pas un classement mesuré.

| Site | Travail visible dans les sources | Conséquence à mesurer |
| --- | --- | --- |
| Génération et fusion | [`generate.hpp:1398`](../src/pipeline/generate.hpp#L1398) compte les shards, garde le budget, réserve puis copie vers `out`, même à un shard | Copie de tous les candidats et coexistence des capacités |
| Tri/RLE | [`candidates.hpp:28`](../src/pipeline/candidates.hpp#L28) trie les BallKey, l'arité puis la représentation du niveau ; les gros objets passent par permutation | Ce tri n'utilise pas l'ordre rationnel des rayons ; ne pas lui attribuer le coût U320 du fold |
| Préfiltre/census | [`census.hpp:61`](../src/pipeline/census.hpp#L61) calcule les extrema de trois paraboles par boîte visitée | Divisions i128 et évaluations de polynôme répétées ; compteurs `depth.nodes` et `census.nodes` disponibles |
| Matérialisation du census | [`expand.hpp:193`](../src/pipeline/expand.hpp#L193) écrit déjà dans une destination privée puis publie par swap | La v7 a déjà retiré les shards et leur copie globale de BallData ; ne pas proposer ce retrait comme neuf |
| Expansion | [`expand.hpp:326`](../src/pipeline/expand.hpp#L326) balaie les BallData à chaque K ; fusion du shard unique encore par copie | En régime régulier, un BallData n'appartient qu'à un ordre, mais la tour balaie jusqu'à dix fois le catalogue |
| Tri d'événements | [`fold.hpp:389`](../src/forest/fold.hpp#L389) trie une permutation u32 stable par `compare_exact_level` | Deux produits U192×U128→U320 par comparaison dans le code source |
| Internement | [`fold.hpp:519`](../src/forest/fold.hpp#L519) conserve 64 partitions, reconstruit chaque facette pour l'empreinte puis l'internement, fusionne et remappe | Six passes, tables surdimensionnées aux incidences, clés triées et trafic mémoire même en mono |
| Réduction/export des deltas | [`fold.hpp:1071`](../src/forest/fold.hpp#L1071) remplit un scratch, trie parents/naissances, copie vers vecteurs ou arènes CSR | CSR n'élimine pas la copie des clés ; ses allocations et ses copies ne sont pas nulles |
| Complétion candidate | [`silent_incidence.hpp:273`](../src/forest/silent_incidence.hpp#L273) reconstruit la table d'identités par K, trie le catalogue et les facettes, calcule des MEB locales puis cherche les intrus | Travail absent du témoin Gabriel v6 ; ne pas l'omettre des mesures du contrat |

Le [comparateur de niveaux](../src/lanes/level.hpp#L51) ne possède actuellement
aucun raccourci pour dénominateurs identiques. `ExactLevel::operator<` ordonne
la représentation et ne peut pas le remplacer. Deux niveaux rationnellement
égaux mais représentés différemment doivent rester dans le même lot.
Les comparateurs, le census scalaire et leurs primitives arithmétiques sont
repris de v6 hors renommages ; la v7 ajoute notamment le census direct,
la complétion et le fold normalisé. Aucun temps historique v6 n'est importé.

La MEB locale essaie au plus les supports de taille 2, 3 et 4 parmi onze points,
soit 550 essais avant filtrages/arrêt précoce, pas une énumération globale de
toutes les facettes. Elle s'arrête au premier support positif contenant la
coface, et la requête d'intrus doit encore vérifier la frontière pour refuser
les extra-shells. Couper cette vérification après deux intrus changerait le
domaine certifié ; ce n'est pas une optimisation admissible.

## 3. Priorité A : précalcul exact du minimum par axe

Dans [`AxisBounds::axis_min`](../src/pipeline/census.hpp#L64),
`floor_div128(-b_i, 2a)` ne dépend pas de la boîte, mais est réécrit à chaque
appel. Le code évalue ensuite quatre candidats clippés : plancher, plafond,
borne basse et borne haute. L'objet `AxisBounds` existe déjà une fois par
boule dans le préfiltre, le census et la requête d'intrus silencieux.

Pour une parabole entière $f(t)=at^{2}+bt$ avec $a>0$, poser
$q=\lfloor\frac{-b}{2a}\rfloor$ et $r=-b-2aq$, où $0\leq r<2a$.
Alors $f(q+1)-f(q)=a-r$. Un minimiseur entier global est donc
$m=q+\mathbf{1}_{r>a}$, avec choix de $q$ en cas d'égalité. La convexité
discrète implique qu'un minimiseur sur un intervalle entier $[l,h]$ est
$\mathrm{clip}(m,l,h)$. Les bornes basse et haute que le code actuel ajoute
aux deux voisins sont déjà couvertes par ce clipping.

Deux bras de qualification permettent d'isoler les transformations :

1. **Cache du plancher seulement** : calculer les trois q une fois, conserver
   les mêmes quatre évaluations. Les bornes et les parcours doivent être
   identiques, bit pour bit.
2. **Argmin précalculé** : calculer m une fois puis une seule évaluation par
   minimum d'axe et par boîte. Le maximum reste le maximum des deux bornes.

Les calculs de q, r et m doivent rester en i128 jusqu'au clipping au domaine
u16, avant tout cast i64. Ne pas évaluer $f(q)$ hors du domaine : même lorsque
les puissances des points u16 sont bornées, le carré d'un centre arbitrairement
lointain n'a pas la même borne. Les [bornes BallKey](../src/lanes/keys.hpp#L5)
et $a>0$ sont les préconditions ; les boîtes sont des boîtes de l'index u16.
Les maxima, les égalités à zéro, les seuils stricts du range-add et la collecte
complète des coquilles ne changent pas. Cette optimisation évite du calcul
interne, sans supprimer une boule, une coface, un certificat ou une incidence.

Avant d'attribuer un gain, inspecter le code machine : le compilateur peut
déjà éliminer un maximum inutilisé dans le census ou hisser certains calculs.
La fréquence réelle des divisions et le mur ne se déduisent pas du nombre de
lignes C++. Les nouveaux caches ne doivent pas être ajoutés à chaque BallData
global : les conserver dans la requête, avec un coût fixe par boule active.

Portes requises : oracle indépendant évaluant toutes les positions entières
d'intervalles bornés ; coefficients et boîtes extrêmes u16 ; centre entier,
demi-entier, extérieur à la boîte et clippings aux deux extrémités ; comparaison
des minima/maxima exacts, puis des parcours, coquilles, refus et digests ;
mutants d'arrondi, de clipping et de seuil ; plancher de non-vacuité pour les
trois cas q, q+1 et égalité. Un microbench mono A/B devra réutiliser exactement
les mêmes clés/boîtes et consommer ses résultats, sans mesure lourde concurrente.

## 4. Priorité B : exécuter B inline dans le vrai bras mono

Sous `threads=1` et `fold_join_before_next_k=true`, le scheduler impose déjà
la séquence A1, B1, A2, B2, etc. Extraire la fonction de travail B existante puis
l'appeler directement dans ce seul bras peut retirer la création/jonction du
thread. Garder les mêmes slots, captures d'exception, décisions de publication,
callbacks provisoires, statistiques et logique de `reap_front` évite de
réimplémenter la sémantique de réduction. Les autres réglages doivent suivre
le chemin multithread historique.

Preuve attendue : pour chaque ordre, même flot d'entrée, même préparation,
même réduction déterministe, même digest et même callback avant l'ordre
suivant ; en cas de faute, même première faute et même invalidation. Le thread
appelant du callback change dans ce mode déclaré : documenter ce point d'API
au lieu de promettre une identité de thread avec le scheduler historique.
Ce correctif est une condition de mesure mono stricte, pas une explication
supposée d'un écart de plusieurs ordres de grandeur.

Portes requises : identité de thread constatée dans tous les callbacks et
phases ; aucune création B dans le bras inline ; résultats identiques avec
classic/CSR, complétion activée/désactivée, K1..K10 et préfixe K1..K5 ; égalités
de niveaux et E5 ; faute A tardive, faute B et exception de callback ; refus
de ressources avec préfixe provisoire détruit ; maintien du chemin worker
pour `threads>1` ou `join=0`. Le compteur existant `ouvriers=1` ne suffit pas.

## 5. Profilage existant et limites de son interprétation

Le [CMake](../CMakeLists.txt#L533) expose `mhgp7_profile` et
`mhgp7_profile_liveness`. Le premier décompose la réduction et l'internement ;
le second ajoute des scans de vivacité susceptibles de modifier caches et
temps. `mhgp7_profile_sonde` expose trois ablations de copies/tris/lectures qui
**changent l'objet** ; leurs temps ne constituent jamais un débit produit.

[`print_run`](../src/pipeline/run.hpp#L1315) expose index, génération, RLE,
préfiltre, census, comptage, expansion, préparation/réduction du fold et digest.
`silent_incidence_ms` doit être lu séparément. Les fenêtres internes de
[`profil_reduce`](../src/pipeline/run.hpp#L1352) excluent notamment certaines
destructions, le digest, le callback et les sondes RSS ; elles ne couvrent pas
à elles seules tout B. `temps_fold_mur_ms` et le mur global restent nécessaires.
Les compteurs `v_census`, les essais MEB, les visites d'intrus, les cardinalités
et les capacités permettent d'attribuer le travail, mais ne sont pas une
preuve d'utilisation d'un seul CPU.

La durée de `run_pipeline` commence après la fabrication/lecture de l'entrée.
Le commit d'archive est après son retour ; l'écriture des forêts via callback
est dedans. Mesurer aussi le temps externe de bout en bout et préciser entrée,
génération, digest, export et synchronisation disque. Ne pas soustraire des
étapes obligatoires pour déclarer le seuil d'une seconde satisfait.

## 6. Protocole avant tout chiffre de gain

Conserver les sources/binaires avant/après, mêmes nuages u16 et PointId,
mêmes ordres et mêmes plafonds. Qualifier d'abord les objets et les refus,
puis mesurer le Release non instrumenté, sans autre campagne concurrente.
Les défauts du mode normalisé et les limites de son domaine ne sont jamais
masqués par une comparaison avec la seule route Gabriel.

La base de mesure nominale est la tour avec `--smax=11 --threads=1
--fold-inflight=1 --fold-join=1`, puis le vrai bras inline lorsqu'il sera
qualifié. Nommer explicitement le scheduler avant/après. Le repli utilise
`--smax=6`, jamais une extraction de K5 depuis une mesure incomparablement
tronquée. Conserver les deux échauffements, les dix nuages frais par famille,
le p95 et les budgets du contrat ; les tailles 8k/16k/32k précèdent le palier
50k et ne le remplacent pas.

Pour l'attribution, mesurer séparément les appels système de création de
threads, le temps CPU utilisateur/système, les changements de contexte,
cycles/instructions et défauts de cache lorsque les compteurs de l'hôte sont
disponibles. Leur indisponibilité doit être signalée, pas remplacée par une
supposition. N'ouvrir la montée multi-CPU, puis la qualification GPU, qu'après
une base mono correctement attribuée. Aucun résultat massif ni arrêt GCP
n'est produit par ce document. GCP non utilisé par cette sous-tâche.
