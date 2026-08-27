# MorseHGP3D v5 — Architecture

Ce document décrit le pipeline **réel** (celui de `src/pipeline/run.hpp`), pas
un plan. Autorité mathématique : [`MATHEMATIQUES.md`](MATHEMATIQUES.md). Plan
de mesure : [`PLAN_DE_TESTS.md`](PLAN_DE_TESTS.md). Provenance module par
module : [`PROVENANCE.md`](PROVENANCE.md).

## 0. Contrats

- **Objet** : les dix forêts horizontales HGP (K = 1..K_max, K_max = 10 au
  profil), niveaux $\rho^2$ exacts, événements exacts (MATHEMATIQUES § 1–2).
  Les applications verticales entre ordres (la tour) ne sont pas livrées.
  Une optimisation ne modifie ni l'objet, ni les niveaux, ni les inclusions ;
  la porte de conformité v4 ≡ v5 (digests canoniques au format v4) le grave.
- **Profil d'entrée** : u16 quantifié seulement (grille $[0, 65536)^3$),
  `PointId` u32 arbitraires (≠ index dense ≠ rang Morton), dégénérescences →
  refus explicite, jamais de jitter. Les positions dupliquées sont refusées
  (`unsupported_degeneracy`) — **arbitrage V1**
  (`../audits/REPONSE_A_CLAUDE_87E915BD_VERROUS_OUVERTURE_20260827.md`,
  27 août 2026) : le refus est la sémantique normative ; la bucketisation
  de l'index (§ 1) est une capacité de représentation, pas une autorisation
  sémantique, et un HGP pondéré (ni défini ni prouvé) ne pourrait entrer que
  comme phase distincte, jamais comme optimisation silencieuse du profil.
  Frontières à rendre cohérentes : `run_pipeline` refuse les doublons avant
  génération ; toute API basse acceptant un `CloudIndex` déclare et vérifie
  la précondition « positions distinctes » (ou devient explicitement
  pondérée) ; une fixture permanente vérifie le code de sortie exact, zéro
  callback et zéro payload partiel ; `range_weight()` ne laisse pas croire
  qu'un census pondéré est livré.
- **Interdits d'architecture** : aucune structure de Delaunay d'aucun ordre,
  aucun arrangement global, aucune matrice globale de cofaces, aucun catalogue
  résident de tous les supports, aucun tableau indexé par toutes les paires /
  triplets / quadruplets ($\propto \binom{n}{k}$ interdit).
- **Statuts transactionnels** : toute exécution termine dans
  `complete_regular | unsupported_degeneracy | resource_exhausted | invalid_input | invariant_violated`.
  Les refus (`invalid_input`, `unsupported_degeneracy`, `resource_exhausted`)
  sont décidés **avant le premier callback** : les gardes d'entrée avant tout
  calcul, les gardes de capacité de tous les ordres sur les comptes
  (`count_events_by_k`) avant la première publication. Les callbacks
  `on_forest` sont **provisoires** jusqu'au statut terminal : seule une
  violation d'invariant (un défaut du calcul) peut encore invalider une
  sortie déjà publiée, et le consommateur doit lire `RunResult::status`
  avant de tenir la sortie pour publiable. Arbitrage V3
  (`../audits/REPONSE_A_CLAUDE_87E915BD_VERROUS_OUVERTURE_20260827.md`,
  27 août 2026) : un flux physiquement émis avant le statut terminal n'est
  recevable que marqué `provisional` et invalidable **atomiquement** ; l'API
  actuelle ne porte pas encore ce protocole (le caractère provisoire des
  callbacks est documenté, pas marqué ni invalidable dans le payload), et
  la sortie n'est publiable qu'au statut terminal global (§ 7).
- **Cibles** : portes d'invariants et de mesure à n = 8000, 16000, 32000 sur
  la machine de développement (8 cœurs, 31 Go) ; puis 50 000 points sur G4 ;
  puis des dizaines de millions de points. Aucun claim de temps ni de
  capacité sans reçu. À ces tailles, la porte d'échelle est la **matrice
  d'autorités** de l'arbitrage V4 (MATHEMATIQUES § 8 ;
  `../audits/REPONSE_A_CLAUDE_87E915BD_VERROUS_OUVERTURE_20260827.md`,
  27 août 2026) : conformité différentielle v4/v5, K = 1 contre un
  single-linkage/MST indépendant, rejeu intégral des deltas et partition
  finale par K, échantillon déterministe rejugé par miniboule, census et
  niveau indépendants, invariants verticaux dès que la tour existe,
  planchers de non-vacuité, reçu complet — jamais un seul compteur, et la
  conformité v4 n'est qu'une porte de divergence.

## 1. Une seule structure spatiale

`src/tree/cloud_index.hpp` : tri par clé de Morton 48 bits, bucketisation des
positions dupliquées (géométrie sur positions UNIQUES, identités et
multiplicités dans un CSR — capacité de représentation de l'index seulement :
le pipeline refuse les doublons avant toute géométrie, § 0, arbitrage V1),
arbre radix binaire de Karras sur les clés
uniques, cellules alignées (borne d'empilement) et boîtes serrées
(certificats). Le même arbre sert la source WSPD, le comptage de témoins et
les requêtes de cover. Il n'existe ni octree séparé ni second arbre.

## 2. Le pipeline

```text
0  entrée (PointId, u16³)  →  index (§ 1)                         tree/
1  pour chaque lane q ∈ {2,3,4} :
   1a  descente WSPD ternaire : paire MORTE (h_coeur ≥ h_q, sans      wspd/, spindle/
       descente) | TERMINALE (séparée, instruite) | SCINDÉE
   1b  par rectangle vivant : h_a/h_b (8 coins, exact), ancres        generate.hpp
       survivantes (h_coeur + h_a + h_b < h_q)
   1c  par ancre : cover (coef 3, ou 4 pour les intérieurs q4) ;      lanes/, float_filter.hpp
       q2 : boule diamétrale ; q3 : seeds aigus + filtre de
       profondeur certifié ; q4 : W_4, seed, cœur de Jung,
       complétions (owner, exact-once, préfiltres, Cramer, centre)
2  RLE par BallKey : arité minimale puis plus petite représentation    candidates.hpp
3  préfiltre count-only : mort à |I_B| ≥ h_qmin                         census.hpp, expand.hpp
4  census I_B / U_B complets des survivantes : coquille COMPLÈTE sous
   plafond explicite, puis resource_exhausted sans troncature — aucune
   compression par supports minimaux (arbitrage V2, MATHEMATIQUES § 7.5)
5  comptage des événements par K (sans matérialisation) → gardes de       expand.hpp
   capacité de TOUS les ordres avant toute publication
6  pour K = 1..K_max, STREAMÉ en PIPELINE À DEUX ÉTAGES :                  forest/plateau.hpp,
   étage A (parallèle, `threads` ouvriers) : expansion de l'ordre K,       forest/fold.hpp
   tri stable parallèle des événements, internement PARTITIONNÉ par        parallel/sort.hpp
   empreinte (64 partitions fixes), fusion parallèle par rangs de valeurs,
   remap ; étage B (un fil d'arrière-plan PAR ORDRE, jusqu'à
   `fold_inflight` ordres en vol — les réductions d'ordres distincts sont
   indépendantes —, PUBLICATION sous verrou dans l'ordre des K, un ordre à
   la fois) : réduction (union-find à état packé, prefetch glissante,
   deltas, partition dense), signature SHA-256 (SHA-NI si disponible),
   callback provisoire, libération. Sortie bit-identique au pipeline
   séquentiel ; les étages B de K−F+1..K recouvrent l'étage A de K+1 ;
   résidence bornée à `fold_inflight` + 1 ordres (2 par défaut → 3).
```

Chaque étape parallèle découpe par tranches d'index et fusionne **en ordre de
tranche** : la sortie est bit-identique au séquentiel quel que soit le nombre
de fils, et le nombre d'ouvriers réellement créés est retourné, jamais
déclaré (`src/parallel/pool.hpp`, `src/parallel/sort.hpp`). Le fold
partitionné ne dépend pas non plus du nombre de fils : partitions fixes par
les six bits hauts de l'empreinte, fid finaux par tri global des clés uniques
(`mhgp5_par_gate`, `mhgp5_parallel_sort_gate`, conformité v4). Le gain d'une
parallélisation se prouve par banc apparié contrebalancé intra-processus
(`mhgp5_fold_bench` : médiane des rapports par paire), jamais par deux
chronos.

## 3. Doctrine d'exactitude

Toute décision est entière et dimensionnée (i64 sous $2^{34}$, i128 au-delà,
U192/U320 pour les niveaux) ; les largeurs sont déclarées en tête de chaque
fichier de `src/lanes/`. Le flottant n'existe que comme **filtre certifié à
repli exact** (`src/pipeline/float_filter.hpp`) : séquence FMA figée, borne
d'erreur prouvée par seed, garde d'arrondi (`__FAST_MATH__`, `fegetround`).
Une boule est une boule : la `BallKey` primitive $(A, B, C)$ identifie la
boule quelle que soit la lane génératrice ; le niveau est un rayon **au
carré** en fraction non réduite, comparé par produits croisés.

## 4. Frontières mémoire (la faute de fond de la v4)

La v4 gardait les dix forêts résidentes et annonçait un « pic projeté » qui
oubliait les résultats terminés. La v5 nomme quatre rôles et ne confond
jamais deux d'entre eux :

| rôle | contenu | durée de vie |
|---|---|---|
| amont | index, candidats post-RLE (libérés après la signature), **boules censusées** (résidentes jusqu'au dernier K : le seul amont des expansions) | libérés dès que l'étape suivante n'en a plus besoin (`run.hpp` : `swap` explicites) |
| en construction | événements du K courant (expansion par K), table d'internement, union-find, tables à époque | un seul K à la fois |
| sortie persistante | ce que le consommateur **choisit** de garder dans `on_forest` (digest, cardinalités, ou la forêt) | décision de l'appelant, jamais du pipeline |
| temporaires | brouillons par ouvrier (cover, sites affines, histogrammes) | par tâche |

Le pipeline ne promet aucun « pic » ; il publie ce qu'il libère et quand. Un
plafond, s'il est demandé, se calcule sur des majorants comptés **par rôle**
et refuse avant allocation — chantier ouvert, voir `../audits/ETAT_COURANT.md`.

## 5. Ce que la v5 ne construit pas

Aucune mosaïque de Delaunay d'ordre supérieur, aucune liste globale de
cellules ou de cofaces, aucun tableau indexé par paire. Les seuls objets
globaux sont l'index spatial ($O(n)$), les rectangles WSPD vivants d'une lane
($O(s^3 n)$, consommés par vague), les candidats post-RLE et, par K, les
facettes du K-graphe. Les oracles exhaustifs (`oracle/`, `tests/`) restent
bornés et hors du chemin produit.

## 6. Mutants

Registre unique `src/core/mutants.hpp` : chaque défaut connu a un nom dans
`kMutants`, exactement un point d'injection `MHGP5_MUTANT("nom")` dans `src/`,
et une porte CTest à code 4. Les mutants ne sont jamais des options ; le
registre est vide dans tout chemin nominal.

## 7. Contrat de payload (arbitrage V3)

Arbitrage V3 (`../audits/REPONSE_A_CLAUDE_87E915BD_VERROUS_OUVERTURE_20260827.md`,
27 août 2026) : aucun des objets partiels proposés (hiérarchie de
connectivité seule par K, partition finale à une coupe, requêtes ciblées) ne
peut être appelé à lui seul « hiérarchie HGP calculée » ; le terme « forêt
complète K = 1..10 » est retiré tant que la tour et la publication
transactionnelle ne sont pas livrées. Le contrat minimal recevable pour un
flux par K déclare la version de représentation, les niveaux de lots, les
deltas (parents, naissances, représentant de sortie), la partition finale ou
un certificat de reconstruction, la politique de rétention des facettes, les
applications verticales si l'objet revendiqué est la tour, et le statut
terminal global avant publication. Le payload livré à ce pin :

Version de représentation déclarée : `mhgp5-forests-horizontal-v1`
(`src/pipeline/run.hpp`, imprimée par le pilote : `payload=… authority=…
callbacks=… vertical_maps=…`).

| champ | valeur |
|---|---|
| `payload_kind` | forêts horizontales par ordre K = 1..K_max — **pas la tour** |
| par K | `batch_levels` (niveaux exacts des lots), `deltas` (lot, niveau, parents, nées, représentant de sortie), partition finale dense (`facet_keys` strictement croissantes, `final_canon_fid` = plus petit fid de la composante) |
| rétention des facettes | toutes (`F_K^render`), jamais un préfixe |
| applications verticales | **aucune** (non livrées ; un objet « tour » sera un autre `payload_kind` versionné — décision d'architecture ci-dessous) |
| autorité | `RunResult::status` terminal ; les callbacks `on_forest` sont provisoires jusqu'à ce statut |
| signature | `mhgp4-digest-v1` (balls, forest par K, all) — porte de conformité v4, pas une preuve d'exactitude |

Une coupe ciblée ou une partition à un rayon donné sera un **autre** payload
versionné (nom, objet reconstructible, autorité, politique de coupe) ; elle
ne prouve pas que les forêts ont été matérialisées. À très grande taille, un
flux émis avant le statut terminal n'est recevable que marqué `provisional`
et invalidable atomiquement — protocole non porté par l'API actuelle (§ 0).

### 7.1 Applications verticales : décision d'architecture (réponse de l'auditeur du 27 août 2026)

Décision détaillée dans la
[`REPONSE_A_CLAUDE_APPLICATIONS_VERTICALES_20260827.md`](../audits/REPONSE_A_CLAUDE_APPLICATIONS_VERTICALES_20260827.md),
retenue comme contrat pour la future livraison de la tour :

- **Bonne définition.** Pour $\sigma \in F_{K+1}(r)$, toutes ses facettes de
  cardinal $K$ sont deux à deux adjacentes dans $\Gamma_K(r)$ (leur union est
  $\sigma$) ; deux cofaces adjacents dans $\Gamma_{K+1}(r)$ ont toutes leurs
  facettes dans une même composante inférieure (clôture par faces). D'où
  $v_K^r : \theta_{K+1}(r) \to \theta_K(r)$, bien définie sur le graphe
  complet des intersections de témoins. Si le produit ne parcourt que les
  cofaces élémentaires, on invoque séparément l'égalité des composantes
  $H_0$ ; cela n'autorise pas à identifier les adjacences.
- **État fermé, jamais « juste avant ».** À $r = \rho(\sigma)$, les facettes
  de $\sigma$ peuvent encore être réparties entre plusieurs composantes de
  $\Gamma_K$ dans l'état strictement antérieur ; avec des niveaux ex æquo,
  aucun ordre interne au plateau n'est canonique. La valeur verticale est
  l'unique composante inférieure **après application de tout le macro-lot de
  niveau $r$** : `cut_side=closed`. Un mutant `vertical-prebatch` (composante
  pré-lot) doit mourir sur la fixture « naissance reliant plusieurs
  composantes inférieures ».
- **Dérivation sans complexe global.** Conserver par $K$ toutes les clés de
  $F_K^{render}$ avec leur niveau de naissance exact ; savoir rejouer la
  partition horizontale après chaque macro-lot fermé ; dériver de chaque clé
  de $F_{K+1}^{render}$ ses $K+1$ facettes par suppression d'un sommet (jamais
  par projection sur les points) ; associer la composante supérieure post-lot
  à l'unique composante inférieure post-lot qui contient ces facettes ;
  versionner `payload_kind`, `cut_level`, `cut_side=closed`. Indexation : un
  événement de la forêt d'ordre $K+1$ est un coface de cardinal $K+2$ ; le
  sommet de $\Gamma_{K+1}$ est la `FacetKey` de cardinal $K+1$, et c'est elle
  qui porte l'incidence verticale.
- **Oracle borné `vertical-oracle-v1`** ($n \leq 12$) : énumérer
  indépendamment $F_K(r)$, les composantes complètes de $\Gamma_K(r)$ après
  chaque niveau exact, toutes les valeurs $v_K^r$, puis les carrés de
  naturalité entre niveaux consécutifs. Fixtures minimales : naissance reliant
  plusieurs composantes pré-lot ; deux cofaces ex æquo ; réindexage des
  points ; deux objets de mêmes sommets projetés et d'incidences différentes.
- **Rendu § 9.1 indépendant de la tour**, mais pas du flux d'incidences :
  $S_\tau$ exige toutes les cofaces incidentes avec multiplicité et niveau
  exact ; les seules clés distinctes et la partition finale ne le
  reconstruisent pas. Un rendu **livré** demandera un payload de rendu
  versionné (`facette -> (lot, multiplicité)`, niveau de naissance exact par
  facette, autorité du statut terminal). Ordre de livraison retenu : rendu
  par ordre d'abord, puis `vertical-oracle-v1` et dérivé CPU des payloads
  horizontaux ; promotion en payload public seulement après les fixtures de
  naturalité et la vérification de rétention des données de naissance et
  d'incidence jusqu'au statut terminal.
