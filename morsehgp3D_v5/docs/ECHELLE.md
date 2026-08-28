# MorseHGP3D v5 — Passage à l'échelle : dizaines de millions de points sur une G4

Cadre : `phase=exploration_v5_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
Ce document est une **hypothèse d'architecture falsifiable**, pas un plan de
capacité : chaque chiffre cite un reçu ou une mesure locale (ratios), les
théorèmes à prouver sont listés avant le code (§ 6), et aucun run à 1 M, 10 M
ou 30 M n'existe. Rédigé le 28 août 2026 à partir des reçus G4 n° 9–12, des
mesures locales du 28 août, des audits v3/v4 cités au § 2, de trois
conceptions contradictoires (`docs/analyses/echelle_20260828/`) et de l'audit
ciblé `audits/AUDIT_PASSAGE_ECHELLE_20260828.md`, dont les objections sont
intégrées au § 4 (marquées **[audit]**). Le scénario chiffré est **CPU sur une
VM munie d'un GPU** : aucune résidence VRAM ni transfert n'est budgété tant
que la lane résidente sur device (`GPU.md`) n'est pas mesurée.

## 1. Les deux objectifs

1. **Principal — objet complet, `smax = 11`, K = 1..10** (dix forêts, profil
   d'entrée u16 inchangé), sur des nuages de 10 M à 30 M points.
2. **Secondaire — K = 1..5** (`smax = 6`, $h_q = 7 - q$), même code, profil
   **nommé** dans les reçus (jamais confondu avec l'objet complet ; un préfixe
   exact de la tour, § 3.3).

Machine cible (lecture seule du 28 août 2026) : `g4-standard-48` = 48 vCPU,
**180 Gio** de RAM, 1 × RTX PRO 6000 (96 Go), un disque persistant NVMe de
**100 Go** ; aucun SSD local attaché. Sessions gardées de **8 h au plus**
(`maxRunDuration`), donc tout calcul plus long doit être **reprenable**
(statut `incomplete_continuation`, flux sur disque persistant survivant à
l'arrêt).

## 2. Ce que la v3 et la v4 avaient déjà établi

- **v4, audit bloquant `C829` (17 août 2026)** — « la prochaine limite est la
  taille de l'objet, pas le prédicat » : 391 événements et 247 nœuds de fusion
  par point à 8000, ~11,7 G événements à 30 M ; il faut **nommer le produit**
  (`full_symbolic_stream` / `connectivity_hierarchy` / `query_or_labels`),
  préflight de cardinalité avant allocation, index 64 bits, plus de `ev_k`
  résident, séparer connectivité et rendu, garder `SpherePlateau` comme unité.
- **v4, contre-audit `772A8D`** — bornes de Poisson fermées : q2 seule impose
  en espérance $2K_{\max}(K_{\max}-1)$ facettes nées par point, soit **180 par
  point pour K = 10** (5,4 G à 30 M) et **40 pour K = 5** (1,2 G à 30 M) ;
  produit recommandé `facet_hierarchy_stream` (K-MST exact au niveau des
  facettes, streamé, output-sensitive), et un produit orienté requêtes (Th. 2
  du manuscrit : composantes de $L_K(r)$) comme seul candidat à un SLO court.
- **v3, réponses « route 50 k puis 10 M » (13 août 2026)** — un halo k-NN ne
  rend pas les tuiles indépendantes ; une coquille peut avoir $\Theta(n)$
  labels ; les événements inter-tuiles et les lots de même niveau exigent une
  **fusion globale** ; peuvent rester résidents : points, index, état du fold
  par ordre, fronts bornés ; les supports se streament.
- **doctrine produit** (`docs/GPU_G4_ARCHITECTURE.md`) : à 10–30 M, nuage et
  arbre résidents seulement si le préflight le démontre ; ancres, frontières,
  candidats et sorties streamés par plages Morton.

Doctrine v5 : ces conclusions sont des **hypothèses différentielles à
requalifier**, pas des acquis ; leurs contre-fixtures et ordres de grandeur
sont conservés, chaque preuve ou mesure v5 est épinglée séparément (§ 3, § 6).

## 3. Lois d'échelle mesurées

### 3.1 Par ordre et par point (reçu G4 n° 11, `uniform` 200 000, 48 fils)

| grandeur | total | par point | dont K = 10 |
|---|---:|---:|---:|
| boules uniques survivantes | 90 M | 450 | — |
| événements (K = 1..10) | 89,8 M | 449 | 105 |
| facettes (K = 1..10) | 562 M | 2811 | 912 |
| deltas | 62,8 M | 314 | 83 |
| attachements (naissances) | 468 M | 2340 | 787 |

Mémoire (paliers `rss_mb`) : 14 Go après génération, 34 Go après census
(boules 224 o), **66 Go au maximum du fold** (trois ordres résidents, ~11 Go
par ordre). Temps : 254 s (génération 69, préfiltre 31, census 19, mur du
fold 115 dont reduce séquentiel 1,4 µs par événement et digest 66 s cumul).

### 3.2 En fonction de l'ordre maximal (`uniform` 32 000, 8 fils, ratios)

| `smax` | K max | boules/pt | mur | pic du fold |
|---:|---:|---:|---:|---:|
| 5 | 4 | 49 | 25 s | 1,0 Go |
| 7 | 6 | 122 | 50 s | 2,4 Go |
| 9 | 8 | 243 | 96 s | 5,1 Go |
| 11 | 10 | 425 | 211 s | 8,8 Go |

Les cardinalités d'un ordre K sont **identiques** quel que soit `smax` ≥ K + 1
(vérifié à 32 000 sur `uniform` et `scanline` : K = 4 → 703 116 événements et
1 943 993 facettes dans les quatre runs) : la restriction est un préfixe exact
de la tour (§ 3.3). Le coût croît en ≈ K².

### 3.3 Le préfixe exact

La mort par profondeur $\left\vert I_B \right\vert \ge h_q = s_{\max} - q + 1$
ne tue que des boules dont tous les événements ont $K = q + d - 1 \ge s_{\max}$ ;
les ordres $K \le s_{\max} - 1$ sont donc inchangés. Un profil K = 5 est
l'objet complet **restreint** aux cinq premiers ordres, avec les mêmes
digests par ordre (`digest_forest[K]`) — la conformité du profil se prouve par
égalité de ces digests avec ceux d'un run `smax = 11` à 8000–32 000.

### 3.4 Facettes vivantes dans le fold (instrument `profil_vivantes`, 28 août)

Dans le fold d'un ordre trié par niveau, une facette n'est vivante (première
incidence passée, dernière à venir) que brièvement. Pic de facettes vivantes
par point, K = 10 : `uniform` 8000 16,8, 16 000 18,1 (+8 % par doublement) ;
`eight_clusters` 8000 13,3 ; `scanline` 8000 2,5. Fraction du total : K = 10
2,2 %, K = 4 10 %, K = 1 93 % (les singletons). **[audit]** Cette sonde n'est pas un majorant de l'état du fold : elle
échantillonne après chaque lot (une facette née et terminée dans le même lot
n'y entre jamais ; un plateau mono-lot annoncerait zéro), elle ignore les
racines et ancêtres union-find encore référencés, la clé canonique, le payload
final et les tampons du lot. Les 18 facettes/point sont une **borne basse
descriptive** ; aucune marge n'en fait un majorant. La mesure de réception (L0)
doit suivre, pendant chaque lot : facettes distinctes touchées, fermeture des
parents/racines/canoniques, état réutilisable après le lot, octets alloués et
pic externe (`ru_maxrss` reste l'autorité : 72,3 Gio à 200 k `uniform` contre
66,3 Gio au palier `rss_mb[4]`, échantillonné après réduction et publication —
à renommer `rss_after_publish_sample`). Deux fixtures précèdent tout
compactage : un plateau massif mono-lot et une chaîne où une facette sans
incidence future reste ancêtre d'un membre futur.

### 3.5 Rayons des boules survivantes (32 000, `smax = 11`)

Rayon maximal : `uniform` 22 unités (2,2 pas moyens), `scanline` 84 (2,4 pas),
`eight_clusters` **149 (7,4 pas, 23 % du domaine — les vides inter-amas)**. Un
halo par copie est donc impraticable sur les familles à vides ; la conception
laisse l'index **résident** et ne réplique aucun rectangle (§ 4.3, **[audit]**
: un halo exigerait sa propre preuve de complétude et d'exact-once).

## 4. Architecture retenue (décision du 28 août, après trois conceptions et trois critiques adverses — `docs/analyses/echelle_20260828/`)

**Cœur : fold streamé par ordre à état borné**, alimenté par un **amont
streamé par seaux Morton du centre des boules** ; le profil nommé
`prefixe_k5` est le premier échelon livrable ; une **clé d'ex aequo
explicite** des événements est le préalable commun. Le tuilage spatial *du
fold* (halo $R_{\max}$, journal de mort par cellule) est **rejeté** : halo
dépendant des données (23 % du domaine sur `eight_clusters`), amplification
de lecture ×3 à ×27, ~5 To d'E/S à 10 M — alors qu'une passe de comptage
PREMIÈRE/DERNIÈRE par ordre, par tri externe, obtient l'oubli des facettes
sans géométrie ni halo. La table des comptes en RAM envisagée au § 4.2
initial (64 Go à 10 M) est remplacée par ce comptage externe.

Ce que l'objet exige et que le plan ne touche pas : mêmes lanes, même RLE
(clé, arité minimale, plus petit niveau), même census complet sous
`shell_cap`, même tri stable par niveau exact, même union-find séquentiel
(« la racine de `first` absorbe »), mêmes fid par tri global des clés, mêmes
plateaux par quotient. Seule la **résidence** change.

### 4.1 Produit nommé

- `payload = mhgp5-forests-stream-v1` (flux des deltas par ordre, dans l'ordre
  des lots de niveau, avec certificat de reconstruction de la partition) et
  digest `mhgp5-digest-stream-v1` ; le digest v4 reste l'autorité de
  conformité tant que le résident existe (≤ 1 M) et un **convertisseur** flux
  → tableaux v4 (hors produit) prouve l'égalité aux tailles où les deux
  existent — dans le domaine u32 seulement : `final_canon_fid` et le digest v4
  sérialisent des u32, et K = 10 à 10 M dépasse `UINT32_MAX` ; le
  convertisseur v4 est une porte différentielle bornée, le flux massif a son
  wire et son digest u64 propres, sans cast silencieux. **[audit]** Le flux
  doit d'abord être **spécifié** : le payload
  courant porte, par K, `batch_levels`, deltas, `facet_keys` et
  `final_canon_fid` ; `facet_hierarchy_stream-v1` doit dire comment ces quatre
  objets sont reconstruits (lots sans delta, partition finale), et la porte aux
  tailles bornées est : décoder le flux en `ForestResult` complet, comparer les
  quatre objets élément par élément au résident, recalculer le digest v4 sur
  l'objet reconstruit et le comparer au pin v4, calculer à part le digest
  d'intégrité du flux (qui ne peut pas être « égal » au digest v4 : autre
  sérialisation). Borne basse de sortie au reçu 200 k : 467,9 M naissances ×
  41 o + ≥ 2 parents × 50,4 M nœuds + 62,8 M deltas × ≥ 105 o = **29,9 Go, soit
  ≥ 1,5 To à 10 M** pour K = 10 avant parents des croissances, cadrage,
  sommes de contrôle et reprise ; K = 5 : ≈ 0,15 To. Un wire plus compact doit
  être spécifié, versionné et mesuré.
- Statuts : `complete_regular | unsupported_degeneracy | resource_exhausted/{requires_tiling, live_state, disk, index, seau} | invalid_input | invariant_violated | incomplete_continuation` (reprise à une frontière de phase, jamais sur une prédiction de temps) ; `public_status=not_claimed`.

### 4.2 Résident, streamé, externe (`uniform`, 48 fils ; base reçu 200 k, cardinalités +10–15 % à 10 M)

| poste | 1 M | 10 M | 30 M |
|---|---:|---:|---:|
| index radix résident | 0,17 Go | 1,7 Go | 5,1 Go |
| rectangles WSPD vivants (3 lanes, 242/pt × 16 o) | 3,9 Go | **39–50 Go** → par vague | 116–150 Go → jamais résidents |
| boules censusées (aujourd'hui résidentes) | 100 Go | 1,0 To → SSD par seau | 3,0 To |
| événements tous K (runs SSD, ×2 pour le tri externe) | 72 Go | 720 Go | 2,2 To |
| comptage PREMIÈRE/DERNIÈRE (SSD, transitoire, par ordre) | 62 Go | 620 Go | 1,9 To |
| fold K = 10 : facettes vivantes (22/pt attendu, ×2,5 majoré) | 2,5 Go | 17–32 Go | 55–106 Go |
| fold K = 10 : enregistrements de composantes | **à instrumenter (L0)** | ? | ? |
| RAM de pointe, objet complet (`inflight` = 1) | ~15 Go | ~110–140 Go | ~165–200 Go (non garanti) |
| RAM de pointe, `prefixe_k5` | ~6 Go | ~60 Go | ~90 Go |
| SSD requis, complet (K par K, runs effacés) | 0,4 To | **≥ 4 To** | ≥ 12 To |
| SSD requis, `prefixe_k5` | 0,05 To | ~0,5 To | ~1,5 To |
| temps, complet | ~25 min | 6–7,5 h | 17–20 h (3 sessions) |
| temps, `prefixe_k5` | ~5 min | 1,5–2 h | 5–6 h |

**[audit]** Rectangles : la seule lane q4 compte 21,8 M `AliveRect` à
200 k, soit ≥ 17,4 Go à 10 M par extrapolation linéaire — la vague qui les
borne doit être mesurée avant d'entrer dans ce tableau. Sortie : ≥ 1,5 To à
10 M (§ 4.1). `eight_clusters` complet à 10 M : 8–9 h (hors session, exposant
~1,55 sur la génération) ; `scanline` : la lane q4 croît en exposant ~3
entre 32 k et 200 k — **[audit, V29]** ce n'est pas une contradiction du
modèle par seed : au reçu 11, 277,9 M seeds tués par cellules + 491,9 M par
le cœur + 171,5 M par la corde = 941,3 M seeds, à ~10 µs de temps-fil chacun
sur 48 fils ≈ 196 s, proches des 214,5 s mesurées ; la projection à 10 M
passe par le nombre de seeds (imprimé depuis le pin `c95cfa95` :
`seeds=q3/q4`, `completions_q4=`) et c'est la lane GPU résidente qui doit
lever ce verrou de la famille LiDAR. Toute ligne 10 M / 30 M est une extrapolation ;
les seules mesures opposables seront les reçus 1 M puis 10 M.

### 4.3 Les trois étages

1. **Amont streamé par seaux Morton du centre** : le seau d'une boule est une
   fonction pure de la `BallKey` ($c = -B/(2A)$ exact en i128, écrêté à la
   boîte u16 ; T7), donc le RLE par seau est globalement exact et la fusion
   k-aire des seaux triés restitue l'ordre global (`digest_balls` v4) ; les
   seaux sont des plages de l'arbre radix à effectif de positions égal (T8),
   jamais fonction de la charge ; le census lit l'index résident (pas de
   halo) ; **[audit]** le théorème de centre co-localise les émissions d'une
   même clé mais ne borne pas un seau (une famille cosphérique ou un centre
   chaud peut en concentrer un nombre arbitraire) : la variante retenue est un
   passage **append-only** de tous les rectangles (chacun traité exactement une
   fois, aucune réplication), routage déterministe, **barrière globale**, puis
   tri externe et RLE par clé exacte ; chaque seau déborde sur disque et se
   sous-partitionne récursivement par la clé, sa mémoire vient d'un budget,
   jamais d'une hypothèse d'occupation ; le Morton du centre est une clé de
   localité, pas l'autorité d'une borne ; magasin de boules SSD partitionné
   par $(q, d)$ ; expansion en une passe vers dix runs par ordre ; tri externe
   par ordre. **Amont retenu pour le premier jalon (audit de résolution,
   solution 4)** : traiter les rectangles par vagues bornées et écrire des runs
   de `BallCandidate` triés par le comparateur produit actuel ; fusion externe
   globale et RLE exact sur la clé complète (`digest_balls` calculé au
   passage) ; préfiltrer, censer et expanser chaque boule unique **une seule
   fois**, en envoyant chaque événement dans le run de son K avec `BallKey`
   source et `emit_rank` ; tri externe de chaque flux K par sa clé totale —
   sans matérialiser ~1 To de `BallData` ni ré-expanser par ordre ; le Morton
   du centre ne partitionnera le census (avec débordement obligatoire) qu'en
   second temps. Première porte : les mêmes candidats découpés en runs de
   tailles 1, 2, 3 et 7, fusionnés et RLE, égaux à `rle_candidates` résident.
2. **Fold streamé à état borné** : clé d'ex aequo explicite (niveau exact,
   `BallKey`, rang d'émission intra-boule ; T2) ; **[audit]** passe
   PREMIÈRE/DERNIÈRE **exacte** : tri externe des enregistrements
   (`FacetKey`, rang d'événement), identifiants stables attribués après
   comparaison complète de la clé, dernière position en u64 — aucune empreinte
   probabiliste, aucun compte u8 ; l'éviction n'est tentée qu'après une
   opération explicite de compression/re-racinement des membres conservés et
   une preuve de préservation du canonique (mutant
   `drop_at_last_direct_incidence` tué par la chaîne adversariale) ; si cette
   fermeture n'est pas bornée, le résultat négatif invalide le fold compact et
   n'est pas masqué ; table des
   facettes vivantes libérée **au lot** de la dernière incidence (T3, avec
   `root_key` figée à la création et `canon_key` = min mise à jour aux unions,
   contre-fixtures « absorbée ⇒ oubliable » réfutée) ; arène des
   enregistrements de composantes bornée par compteur instrumenté (T6) ;
   indices et positions d'incidence en u64 ; deltas émis en flux.
3. **Reprise `resume=replay_current_K`** **[audit]** : chaque ordre K terminé
   est écrit dans un fichier temporaire, `fsync`, validé, renommé atomiquement
   et inscrit au manifeste ; un K interrompu n'est jamais publié et est rejoué
   depuis le début à partir des runs immuables ; le préflight vérifie qu'un K
   isolé tient dans la durée de session ; falsification par `SIGKILL` à chaque
   frontière. Une reprise intra-K (curseurs de fusion, tas, état du fold,
   digest, offsets) ne viendra qu'avec un checkpoint complet, versionné et
   mesuré. Comptabilité à chaque frontière (aucune écriture sans espace pour
   ancien état + temporaire + fusion + manifeste + marge).

## 5. Modèle de temps

Constantes du reçu 200 k (48 fils) avec dérive de latence du reduce
(1,4 → 2,2 µs par événement à 10 M, +8 % par doublement) ; voir la table du
§ 4.2. **[audit]** Le disque de la VM est un boot **Hyperdisk Balanced** de
100 Go provisionné à 3 600 IOPS et **290 Mio/s** (`gcp-migration/deploy.sh`) ;
une G4 n'accepte pas les Persistent Disk zonaux, son plafond partagé est de
1 600 Mio/s et le débit Hyperdisk est half-duplex. Le modèle prend donc en
paramètres type, capacité, IOPS, débit provisionné et débit `fio` mesuré :
avec la borne basse de sortie (1,5 To) plus 230 Go de runs écrits et relus,
le minimum séquentiel est ≈ 1,96 To, soit **107 min idéales à 290 Mio/s**
(≈ 20 min à 1 600 Mio/s), sans lecture d'entrée, comptes, hachages ni marge.
Le fold `uniform` mesuré (115 s) extrapole linéairement à 1,6 h et le reduce
cumulé (128 s) à 1,8 h ; la génération vaut 68 s (`uniform`), 178 s
(`eight_clusters`), 244 s (`scanline`) à 200 k : « 6–7 h » est un scénario
`uniform`, pas une loi de famille — publier formule, facteur de streaming,
fourchette et sensibilité au débit, et ne promettre aucune session de 8 h
avant un pilote à 1 M. Toute modification de disque GCP est une mutation
gardée à ajouter aux scripts du dépôt.

## 6. Théorèmes à prouver avant le code (énoncés, `docs/analyses/echelle_20260828/synthese.txt` § 3)

- **T1 préfixe exact** : pour $s \le s'$ et tout $K \le s - 1$, événements,
  ordre et `forest[K]` identiques entre `smax = s` et `smax = s'` (livré :
  porte de préfixe ; corollaire : `digest_balls` et `digest_all` ne sont pas
  comparables entre profils).
- **T2 clé totale** : (niveau exact, `BallKey`, rang d'émission intra-boule)
  est un ordre total qui coïncide avec le tri stable actuel sur l'ordre
  d'entrée (RLE puis émission) ; mutants `tiebreak-cell-local`,
  `tie-by-sorted-T`.
- **T3 dernière incidence** : après le lot de sa dernière incidence, l'état
  d'une facette n'est lu que par `find`, `canon` et les clés ; avec
  `root_key`/`canon_key` portées par les enregistrements, la suite des deltas
  est inchangée quand la facette est libérée ; contre-fixtures (chemin de 3
  points à K = 1, deux tétraèdres partageant une face à K = 2) ; mutant
  `free-on-absorb`.
- **T4 PREMIÈRE/DERNIÈRE exacts** (remplace la version par empreinte) :
  prépasse par ordre sur le flux trié par (niveau exact, `BallKey`,
  `emit_rank`) avec `event_rank_u64` ; incidences émises comme
  (`FacetKey` complète, `event_rank_u64`, slot), partition éventuelle par
  hachage pour les E/S seulement, tri et comparaison par clé complète, fusion
  lexicographique attribuant les `fid_u64` et marquant exactement une PREMIÈRE
  et une DERNIÈRE par facette, join retrié dans l'ordre du fold ; pic inclusif
  par lot `live += first[b] ; peak = max ; live -= last[b]` sans heuristique ;
  porte à hachage constant injecté (résultat identique) et mutant
  `lifetime-by-hash-only` divergent. Coût de wire (`FacetOccurrenceWire`,
  octets écrits/lus, facteur temporaire du tri K par K) à graver en L2 avant
  de recalculer le poste SSD — la ligne « 620 Go » du § 4.2 est un chiffre
  d'empreinte, pas de clés complètes.
- **T5 ordre intra-lot sans catalogue** : `fid(x) < fid(y)` ⟺
  `key(x) < key(y)` ; `post_list` triée par racine ≡ triée par `root_key` ;
  `(facet_keys, final_canon_fid)` est une fonction du flux de deltas ;
  invariant sans état `output = min(parents ∪ born)`.
- **T6 → invariant `components <= live_aliases`** (fold vivant
  *small-to-large*, audit de résolution) : un `Alias` par facette encore
  réutilisable (`fid_u64`, clé, `seen`, rôles du lot, composante, liens
  intrusifs) ; un `Component` par composante ayant au moins un alias
  (`logical_root_fid` = règle actuelle « la racine du composant de `first`
  absorbe », `canon_fid` = minimum historique, `historical_mass`, liste
  d'alias) ; union ordonnée : racine logique = celle de `first`, conteneur
  physique = le record de plus grande masse (small-to-large, relocalisations
  logarithmiques), canonique = min, masses sommées, record vide détruit ; lot
  en deux passes (rôles et gels pré-lot, puis unions dans l'ordre total) ;
  alias à `last_batch == b` supprimés après l'émission du lot `b` ; une
  composante sans alias est définitivement libérable (toute connexion future
  réutiliserait une facette à sa dernière incidence passée) ; les deltas triés
  par `logical_root_fid` reproduisent `post_list`. Aucune chaîne de parents
  morte, aucun reroot, aucun refcount : à toute frontière de lot
  `components <= aliases <= peak_live_exact`. Mutants
  `physical-root-is-logical-root`, `free-on-absorb`, `root-key-mutable`,
  `canon-not-min-on-union`.
- **T7 exact-once par seau** (localité seulement : la complétude vient du
  RLE par clé complète après fusion externe), **T8 découpage déterministe des
  seaux** (fonction de l'entrée, jamais de la charge — ce n'est pas un
  théorème d'équilibre), **T9 identité de masse par ordre** (comptés = expansés = longueur du run ;
  $\sum (q + d) = K + 1$ par événement ; fusions = facettes − composantes
  finales ; attachements = $\sum \left\vert \text{born} \right\vert$).

## 7. Portes

Égalité résident / streamé aux deux digests (v4 par convertisseur, flux) à
8 k / 16 k / 32 k (quatre familles + `terrain`, $s$ = 6/8/10,
contre-familles), seaux ∈ {1, 2, 3, 7, 31}, fils ∈ {1, 8, 48}, tailles de
run, `inflight` ∈ {1, 2, 3}, frontière de run au milieu d'un niveau égal,
coupure + reprise après chaque phase ; 200 k puis 400 k et **1 M** sur G4 ;
invariants globaux à 10 M / 30 M (T9 par K, violations nulles, K = 1 :
facettes = $n$, fusions = $n - 1$, idempotence de la partition par rejeu des
deltas sur un union-find frais, planchers par K, porte de préfixe,
invariance du digest de flux au découpage et aux fils, équivariance par
permutation) ; juge d'échantillon ($10^4$ par K, graine gravée : census par
balayage des seaux, niveau par `obigint`, ré-expansion, incidences recomptées
par un parcours différent, K = 1 par Borůvka à arithmétique distincte) ;
mutants à code 4 : `prefix-hq-off`, `tiebreak-cell-local`, `tie-by-sorted-T`,
`free-on-absorb`, `free-record-early`, `canon-not-min-on-union`,
`root-key-mutable`, `bucket-by-anchor`, `no-clamp`, `merge-order`,
`last-mark-shifted`, `pic-per-incidence`.

## 8. Ordre des livraisons

| # | livraison | objet | digest v4 |
|---|---|---|---|
| L0 | instrument `vivantes` promu en compteur de réception (pic par lot, enregistrements, singletons) + reçu 32 000 puis 200 k | inchangé | inchangé |
| L1 | **livré en partie (28 août)** : portée nommée de la tour dans la sortie et porte de préfixe par digests (K = 4, 5 ; trois familles ; mutant tué) — **[audit]** à corriger : le champ `profil=` collisionne avec le profil normatif `quantized_u16_input_only` → `tower_scope=profile_complete_k10` / `tower_scope=prefix_k5` (« complet » = complet dans le profil K ≤ 10), distinguer `smax_requested` / `smax_effective`, étendre la porte aux événements canoniques et aux `batch_levels` de tous les lots (omis par le digest v4) avec un plateau non trivial — restent : clé d'ex aequo explicite, indices u64, mutants T2 | inchangé | bit-identique |
| L2 | **livré en partie (28 août)** : réducteur vivant `reduce_fold_live` (`src/forest/fold_live.hpp`) — PREMIÈRE/DERNIÈRE en RAM, alias et composantes en arène, table `fid -> alias` à décalage arrière, union small-to-large ; porte `mhgp5_fold_live` : 58 ordres, 5 194 737 facettes, 733 029 deltas, **égalité champ à champ avec le fold résident**, rejeu T5 des deltas vivants vers la même partition, 0 violation d'invariant, de vie par lot ni de structure, cinq mutants tués ; restent le convertisseur flux → tableaux v4 et les durées de vie externes (L3), et **aucun gain CPU ni RSS n'est mesuré** — `reduce_fold_live` est hors du chemin produit | inchangé | bit-identique (deltas et `batch_levels` égaux ; catalogue et partition par le rejeu T5) |
| L3 | payload et digest de flux, manifeste atomique, statuts SSD | inchangé | second digest déclaré |
| L4 | amont streamé (solution 4, § 4.3) : runs de candidats triés par vague, fusion externe globale et RLE exact sur la clé complète, préfiltre/census/expansion une seule fois par boule unique, runs par ordre triés par clé totale ; le seau Morton du centre reste une localité optionnelle (V28 : la réconciliation globale des occurrences d'une clé est la seule nécessité) | inchangé | `balls` v4 par fusion k-aire |
| L5 | préflight par rôle (`MemAvailable`, cgroup, `statvfs`, débit mesuré) ; jalon **1 M** G4 : résident ≡ streamé | — | — |
| L6 | 10 M : `prefixe_k5` d'abord, puis objet complet | — | flux + invariants + juges |
| L7 | 30 M : `prefixe_k5` (une session) puis complet en sessions gardées avec reprise | — | idem |

Le GPU (lane résidente sur device, `GPU.md`) est un chantier **parallèle**
visant la lane q4 de `scanline`/`eight_clusters` et la génération à 10 M ; il
ne change ni l'objet ni ce plan.

## 8 bis. Ordre de travail retenu (audit du passage à l'échelle, 28 août)

1. **Porte de rejeu** — **livrée le 28 août** (`tests/delta_replay_gate.cpp`,
   portes `mhgp5_delta_replay*`, mutants `drop-nonmerge` et `attach-prebatch`
   tués), **avec les six fixtures gravées** (`tests/fold_fixtures_gate.cpp`,
   porte `mhgp5_fold_fixtures`) : sortie littérale, égalité résident/vivant,
   et invariance par empreinte d'adressage constante — celle-ci est un
   *crochet de stress* compilé sous `MHGP5_TESTING`
   (`fold_detail::fold_hash_constant`) et non un mutant, son verdict attendu
   étant l'absence de changement. Le plateau et l'absorption sont construits
   pour que l'ordre des **racines logiques** contredise l'ordre des clés de
   sortie ; une première version, qui ne les distinguait pas, aurait été verte
   sous une implémentation triant par clé. Énoncé d'origine :
   extraire du juge (`tests/forest_judge.cpp`) la porte
   indépendante « catalogue de facettes + deltas → `final_canon_fid`
   reconstruit », comparée champ à champ au `ForestResult` résident avec le
   vrai digest v4 ; graver les six fixtures (étoile K = 1 de 300 arêtes à
   niveaux croissants ; chaîne K = 1 `{0,1}` puis `{0,2}` ; deux simplexes
   K = 2 partageant une facette ; plateau mono-lot q3 à pic transitoire 3 ;
   grand composant absorbé logiquement par un singleton ; frontières externes
   avec hachage constant).
2. **Réducteur vivant en RAM** — **livré le 28 août** (`src/forest/fold_live.hpp`,
   `tests/fold_live_gate.cpp`, portes `mhgp5_fold_live*`). Cinq des six
   mutants demandés sont tués : `free-on-absorb`, `root-key-mutable`,
   `canon-not-min-on-union`, `last-mark-shifted` par désaccord de sortie,
   `physical-root-is-logical-root` par les **plafonds de relocalisation** —
   c'est le seul des cinq qui ne change pas la sortie, seulement le coût.
   Deux témoins, agrégé et par alias : `relocalisations <= facettes *
   (ceil(log2 facettes) + 1)` et `déplacements d'un alias <= ceil(log2
   facettes) + 1`. Le second est le causal : sur une **chaîne d'absorptions
   adverse** (un singleton frais est `first` à chaque niveau et absorbe
   logiquement la composante qui grossit), small-to-large ne déplace que le
   singleton — **1 déplacement** par alias — tandis que le mutant déplace
   toute la composante à chaque pas : **200** pour un plafond de 9. Les cinq
   mutants meurent donc sur deux cas synthétiques de quelques millisecondes,
   joués avant tout nuage : les portes passent de 36–50 s à **0,06–0,09 s**,
   et toutes portent un `TIMEOUT` explicite.
   `lifetime-by-hash-only` reste au chantier suivant : il porte sur le calcul
   **externe** des durées de vie, que L2 remplace encore par deux tableaux
   `u32` par facette (assumé et hors de la revendication de résidence).
   Résidence mesurée le 28 août (n = 1 000–1 500, quatre familles plus les
   deux contre-familles), publiée en **deux témoins séparés** — les mélanger
   fabriquerait un chiffre qui n'existe nulle part :
   *pic absolu* — l'ordre le plus gros (733 687 facettes) culmine à **16 929
   alias pour 471 composantes**, soit **2,31 %** ;
   *pire ratio* — sur les 15 ordres d'au moins 100 000 facettes, la pire
   fraction est **7,29 %** (7 994 alias, 206 composantes, 109 721 facettes).
   Les octets vont eux aussi par deux : **1,83 Mo** d'état logiquement vivant
   et **3,19 Mo** réellement alloués (arènes, listes libres, table) au témoin
   du pic absolu, contre **26,4 Mo** pour les deux champs par facette du
   résident (`FidState` 32 o + `final_canon_fid` 4 o). Le rapport — 8,3 fois
   sur les octets alloués — est une **estimation de structures choisies**, ni
   un gain d'allocation de bout en bout, ni un gain de RSS : `firstb`/`lastb`,
   `keys` et `ev_fid` restent en RAM à L2 et ne partent qu'à L3, et
   `reduce_fold_live` n'est pas encore sur le chemin produit.
   Ce qui est *vérifié*, en revanche, l'est à **chaque** frontière de lot :
   `composantes <= alias <= pic exact` (T6), et l'égalité forte
   `alias == compte exact du lot` avant **et** après les morts, plus la
   bijection `index <-> alias`, la longueur de chaque liste égale à son
   compte, l'absence de cycle et la **vacuité finale** de l'état — 0 violation
   sur 2 785 balayages structurels. Les deltas du vivant sont en outre
   **rejoués** (T5) sur le catalogue du résident et reconstruisent la même
   partition, fid par fid.
3. **Coutures externes** : RLE multi-runs (tailles 1, 2, 3, 7), lifetime avec
   hachage constant, join retour vers les événements.
4. **Payload et reprise** : wire u64, digest logique indépendant du découpage
   physique, publication atomique par K (`resume=replay_current_K` :
   temporaire unique, `fsync`, relecture et hachage, renommage, `fsync` du
   répertoire, manifeste renommé atomiquement ; un K précédent n'est repris
   que si son manifeste est `committed`).
5. **Pilote 1 M** seulement après mesure des octets, du pic inclusif et du
   débit du disque réellement attaché ; puis refaire le tableau et décider si
   10 M K = 5 ou K = 10 est la porte suivante.

## 9. Questions à trancher par l'utilisateur

1. **Profil des premiers reçus 10 M** : `prefixe_k5` d'abord (~1,5–2 h,
   ~60 Go, ~0,5 To de disque) avant l'objet complet ? `prefixe_k4` utile ?
2. **Disque** : accord pour attacher un SSD à la VM via les scripts gardés
   (≥ 1,5 To pour 10 M `prefixe_k5` / complet K par K, ≥ 4 To avec rétention
   complète ; 30 M complet ≥ 12 To) — débit **mesuré** au préflight.
3. **Sessions** : une mini-session (200 k + 1 M, ≤ 2 h) pour L5, une session
   ≤ 8 h pour 10 M, 30 M complet en 2–3 sessions avec reprise.
4. **Contrat de payload** : le digest de flux comme autorité au-delà de 1 M ;
   les `batch_levels` (240 Go à 10 M) restent-ils dans le contrat ?
5. **Familles** : `uniform` puis `eight_clusters` (10 M complet hors session
   → `prefixe_k5`) ; `terrain` à profiler à 200 k avant engagement.
