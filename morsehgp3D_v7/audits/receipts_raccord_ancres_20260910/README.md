# Raccord FULL par ancres de boule : contre-lecture, vacuité des lots groupés, verrous

10 septembre 2026. Second auditeur (session `e-hgp-c6`), périmètre déclaré dans
[COORDINATION_AUDITEURS.md](../COORDINATION_AUDITEURS.md). Cadre :
`phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
GCP non utilisé par cet audit ; aucune écriture hors `morsehgp3D_v7/audits/` ;
aucune branche.

Objet : le raccord FULL par ancres de boule, lu comme **WIP non commité** à
partir de 13:04 UTC puis **publié par `d188e3de`** (14:05 UTC) avec exactement
les octets de mes derniers snapshots : `src/forest/full_ball_tower.hpp`
`0b72b4e9…`, `src/forest/anchor_meb.hpp` `386072c8…`,
`tests/full_ball_tower_gate.cpp` `d2a08edb…`, `bench/full_ball_tower_probe.cpp`
`ba6c16c3…`. Autorités confrontées : [BALL_ANCHORS](../receipts_plateaux_full_20260906/BALL_ANCHORS.md),
[LOCAL_DIAGNOSTICS](../receipts_plateaux_full_20260906/LOCAL_DIAGNOSTICS.md),
[COVERAGE_THRESHOLDS](../receipts_plateaux_full_20260906/COVERAGE_THRESHOLDS.md),
[GLOBAL_PARENTS](../receipts_plateaux_full_20260906/GLOBAL_PARENTS.md),
[décision FULL](../NIVEAUX_ET_CERTIFICAT_HGP_COURANT.md). Sont aussi jugés le
delta `local_plateau.hpp` de `1fbe49d3` et la cohérence documentaire de cette
livraison. La qualification propre du journal `full_coverage_certificate.hpp`
appartient à l'[auditeur historique](../receipts_coverage_cpp_20260910/README.md) ;
ce qui le concerne est transmis en § 9. **Ce reçu ne reçoit pas le raccord** :
il en borne la qualification et en fixe les verrous.

## 1. Snapshots lus

| Snapshot (UTC) | Fichier | SHA-256 |
| --- | --- | --- |
| 13:04:02 | `src/forest/full_ball_tower.hpp` | `4929a542…af55718c` (= `full_gate_r3` du constructeur) |
| 13:04:02 | `src/forest/anchor_meb.hpp` | `386072c8…5536f786` (= `d188e3de`) |
| 13:04:02 | `tests/anchor_meb_gate.cpp` | `5e3ee812…029b753a` (= `d188e3de`) |
| 13:04:02 | `tests/full_ball_tower_gate.cpp` | `7ed1ecba…0e73bea` |
| 13:08:41 | `tests/full_ball_tower_gate.cpp` (+ `actual_equal_radius_descent`) | `d2a08edb…04b9ca` (= `d188e3de`) |
| 13:25:47 | `src/forest/full_ball_tower.hpp` (+ `MonotoneHistory`, `validate_declared_support`, lots unitaires) | `0b72b4e9…557657e8` (= `d188e3de`) |
| 13:26:18 | `bench/full_ball_tower_probe.cpp` | `ba6c16c3…f03143` (= `d188e3de`) |

`snapshots/SHA256SUMS` fixe ces octets. Les constats ci-dessous portent sur
les octets publiés, sauf mention « 13:04 ».

## 2. Rejeux indépendants (overlay hors dépôt, `g++ 13.3`, `-Werror`, Boost épinglé)

| Binaire | O2 | ASan/UBSan (`detect_leaks=1`) |
| --- | --- | --- |
| `anchor_meb_gate` | 0 ; 11 752 contrôles, 605 comparaisons à l'oracle Gram rationnel, 300 permutations, 197 coquilles sélectionnées > support, supports acceptés 82/393/110/20 | 0, mêmes octets |
| `full_ball_tower_gate` 7ed1ecba + header 13:04 | 0 ; 67 454 contrôles, 22 nuages, 92 ordres, 1 592 coupes, 13 938 contrôles verticaux ; **`same_radius_steps=0`** | 0, mêmes octets |
| `full_ball_tower_gate` d2a08edb + header 13:04 | 0 ; 130 734 contrôles, 24 nuages, 100 ordres, 2 136 coupes, 35 462 contrôles verticaux ; `same_radius_steps=2` | 0, mêmes octets |
| `full_ball_tower_gate` d2a08edb + header 0b72b4e9 (publié) | 0 ; mêmes compteurs | — |

Sorties : `gates_wip_130402/`, `gate_v2_130841/`, `normaliseur_temporel/`.

## 3. Lecture statique du raccord publié : conforme aux autorités

- **Fenêtre amont conservée** : `p + q_min > min(Kmax+1, n)` refusé
  (`full_ball_outside_rank_window`), jamais `p + u ≤ smax` ; `shell7_window`
  publie la naissance de couverture sept.
- **Intervalle d'ancres** `[p+q_min−1, min(Kmax, p+u)]`, programmé par boule en
  ordre de niveau exact.
- **Boules régulières sans table** : deux rangs, q représentants, aucune table
  2^u ; `validate_declared_support` (positivité, clé, niveau) équivaut à la
  MEB témoin pour l'admission.
- **Hit par BallKey avant tout intrus** ; clé trouvée sans ancre dans son
  intervalle ⇒ `full_ball_missing_closed_anchor`, jamais un repli.
- **Descente lexicographique** rayon puis coquille sélectionnée ; rayon égal
  admis seulement à clé égale et coquille diminuée de un.
- **Lot atomique** : représentants résolus dans l'état pré-lot avant tout nœud ;
  regroupement par racines partagées ; bloc à un parent sans contribution =
  inerte, sans action mais avec ancre ; ancres après fermeture.
- **Verticale** : image d'une naissance = ancre `(K−1, BallKey)` à la coupe
  fermée ; images des parents d'une multifusion exigées égales ; le lecteur
  normalise à la coupe demandée.
- **Terminal de complétude relative** : une racine vivante par ordre.

## 4. Constats exécutés

### 4.1 Lots groupés : deux mutants causaux du reçu survivent sur le header publié (P1)

Le reçu `receipts/ball_tower_20260910/captures/full_gate_r3` rejoue ses quatre
mutants (`drop_growth`, `drop_inert_anchor`, `strict_radius_only`,
`wrong_vertical_cut`) sur le header **`4929a542`** (`sources.json`), pas sur
`0b72b4e9` ; `monotone_summary` ne rejoue sur `0b72b4e9` que le mutant
d'activation anticipée. Or `0b72b4e9` introduit un chemin « lot unitaire »
(`close_lot`, `blocks.size() == 1`) distinct du chemin groupé. Les deux mutants
réappliqués **au seul chemin groupé** (`lots_groupes_vacuite/*.diff`) :

| Mutant (chemin groupé) | Porte publiée d2a08edb | Ma fixture à neuf points |
| --- | --- | --- |
| croissance omise (contribution d'une continuation non journalisée) | **verte**, 130 734 contrôles, rc 0 | verte |
| ancre inerte omise (bloc à une racine sans contribution sans ancre) | **verte**, rc 0 | verte |

Cause : les fixtures qui tuaient ces mutants (`growth_ABCZ`, `E5`) passent
désormais par le chemin unitaire ; la branche croissance et l'affectation
d'ancre du chemin groupé sont vertes par vacuité. Fixture proposée, deux
copies éloignées de ABCZ (`lots_groupes_vacuite/full_ball_tower_gate_doubled.cpp`,
`growth_ABCZ_doubled_lot`, Kmax 3) : le lot du niveau 25 porte deux blocs, le
nominal passe le juge Gamma (28 nuages, 170 316 contrôles, `growth_snapshots=8`),
le mutant croissance échoue `gamma.coverage_multiset_including_multiplicity`,
le mutant ancre échoue `full_ball_vertical_birth_anchor`. Une seconde fixture
`inert_ball_doubled_lot` (tétraèdre+origine et paire lointaine de même
niveau 3) passe nominalement. Confirmé par deux réfutateurs indépendants.

### 4.2 Branche « rayon égal » : vacue au snapshot 13:04, exercée depuis 13:08

Aucune fixture n'exerçait `cmp == 0` de `resolve` ; `require(cmp < 0)`
(`fixture_rayon_egal/mutant_strict_descent.diff`) laissait la porte verte.
Fixture indépendante (`fixture_rayon_egal/`, recherche entière, catalogue =
MEB de tous les sous-ensembles, arité recalculée sur la population fermée) :

```text
(54,53) (46,47) (53,54) (47,46)   rectangle inscrit au cercle centre (50,50) r=5, deux diamètres
(53,47) (54,48) (52,46) (51,46)   quatre intérieurs
(0,70)                            point lointain ; Kmax 8 ou 9
```

O2 et ASan/UBSan : `same_radius_steps=1, descending_steps=8`, tour complète ;
le mutant strict refuse. Neuf points dépassent l'oracle borné à huit de la
porte : témoin de non-vacuité et de causalité. La fixture du constructeur
`actual_equal_radius_descent` (boule hors fenêtre) tue le même mutant
(`strict_radius_only` dans son reçu) ; une fixture 3D « clé au catalogue, K
sous l'intervalle » exerce l'autre branche
(`contre_lectures/prospectif/equal_radius.cpp`). `same_radius_steps` dépend de
l'ordre Morton (3 symétries sur 8) : jamais dans un digest ni une porte
d'équivariance.

### 4.3 K1 de la tour = single-linkage exact (nuages générés, census WSPD réel)

`k1_single_linkage/` : ordre 1 de la tour sur `uniform` (graine 3, s=8) contre
une référence Python indépendante (toutes les paires, lots à distance égale,
niveau `d²/4`) ; sorties `-B` et `-O` identiques.

| n | paires | lots à niveau égal | événements référence = tour | divergence |
| ---: | ---: | ---: | ---: | ---: |
| 400 | 79 800 | 0 | 399 | 0 |
| 2 000 | 1 999 000 | 698 | 1 999 | 0 |
| 4 000 | 7 998 000 | 11 003 | 3 999 | 0 |

### 4.4 Corpus aléatoire entier, juge Gamma rationnel indépendant (`corpus_aleatoire/`)

Pont C++ sur le header 13:04 ; juge Python sans en-tête produit (Gram rationnel
copié de `meb_rational_oracle_20260905.py`, circumboules de 2 à 4 points, clé
primitive, fenêtre miroir, Gamma incrémental, images verticales) ; n=5..12 à
petites coordonnées, PointId arbitraires (dont 0 et 2³²−1).

| Campagne | Nuages | Ordres | Coupes | Racines | Images verticales | Divergences | Refus |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| principale (200 + 7 fixtures) | 207 | 795 | 69 694 | 119 751 | 93 038 | 0 | 0 |
| étendue | 2 000 | 7 773 | 633 958 | 1 059 397 | 811 601 | 0 | 0 |
| cocirculaire | 300 | 1 177 | 73 976 | 134 179 | 100 459 | 0 | 0 |

Coquilles supplémentaires exercées 2 120 / 22 050 / 1 053 (u jusqu'à 12) ;
lots à niveau égal 3 303 / 32 805 / 3 548 (jusqu'à 16 boules) ; aucune
ambiguïté de couverture ; sorties normal/`-O` identiques. Mutants du corpus :
`drop_extra_ball` refusé 148 fois, divergent 12, silencieux 39 (tous expliqués :
boules inertes ou à parent unique déjà couvert) ; `wrong_arity` refusé 199/199 ;
mutant du juge 8 458 divergences.

### 4.5 Normaliseur temporel `MonotoneHistory` (publié), contre-vérification demandée

`normaliseur_temporel/monotone_history_probe.cpp` : 40 histoires aléatoires
valides (46 163 nœuds, 5 092 fusions à niveau égal, peignes jusqu'à 1 283 de
profondeur), **11 891 006 requêtes** monotones identiques à `History::root_at`,
40 refus `full_ball_lower_cut_not_monotone` ; ASan/UBSan identique. La
contrainte « ouvert→fermé permis, fermé→ouvert refusé à coupe égale » est
correcte ; chaque arête est activée une fois. **Favorable.** La porte du peigne
`mhgp7_full_ball_work` est enregistrée dans `d188e3de`.

### 4.6 Sonde retenue : diagnostics, pas mesures

`sonde_130841/` : `uniform`, s=8, Kmax 10, 1 fil, n=8/400/2000 : 0,0008 / 5,4 /
56,6 s (tour 0,0004 / 3,36 / 35,5 s, RSS 4 / 79 / 478 Mio), **`extra_records=0`**
partout, comme à 8k/16k/32k dans les captures du constructeur.
`familles_structurees/` (n=2000, 2 fils) : `terrain` 0, `eight_clusters` 0,
**`scanline_single_pass` 76**, **`scanline_overlap_multiecho` 3 726** coquilles
supplémentaires avec dix pas à rayon égal, tour complète. Les 8k/16k/32k
publiés (215 / 418 / 965 s, 4,0 / 8,3 / 17,2 millions de nœuds) ne sont pas
contrôlés ici ; aucun temps n'est apparié ni contractuel.

## 5. Verrous avant toute levée de la garde des coquilles supplémentaires

Gravité issue de la vérification adverse (§ 8) ; P1 = obligatoire avant levée.

1. **[P1, § 4.1] Rejouer les quatre mutants du reçu sur le header publié** et
   graver une fixture à lot groupé pour la croissance et l'ancre inerte
   (`growth_ABCZ_doubled_lot` fournie) ; sans elle deux mutants causaux
   survivent sur `0b72b4e9`.
2. **[P1] Blocs réels 50k.** Aucun contrôle nommé ne confronte le raccord aux
   certificats 1/2/2 de GLOBAL_PARENTS : la sonde n'imprime que des agrégats et
   `block.roots` n'est conservé nulle part par `(K, BallKey)` ; la passation
   publiée reconnaît qu'ils « restent à retrouver ». Exiger l'exposition
   test-only des racines pré-lot pour les quatre clés extraites, un contrôle par
   bloc (174406/K5 → 1 ; 254569/K2 → 2 ; 996863/K6 → 2 ; 1251653 → ancre K10
   conservée, contribution vide) sur l'entrée épinglée (digest `3f7c6dd4…`),
   un mutant tué (retourner `anchors[*found]` sans `root()`), et la mention que
   cela vérifie trois blocs, ni la complétude S1 ni l'arité finale des nœuds.
   Confirmé par deux réfutateurs ; le troisième le classe comme rappel adressé
   au futur reçu, non comme défaut de `1fbe49d3`.
3. **[P2, non vérifié par réfutateurs]** Couplage Kmax census ↔ Kmax tour : un
   catalogue K10 fourni à une tour Kmax 5 est refusé
   (`full_ball_outside_rank_window`) alors que BALL_ANCHORS § 1 autorise
   l'omission ; trancher (refus vs filtrage compté) et corriger « repli K5
   déduit de la capture K10 ».
4. **[P2, non vérifié]** Digest/export : `mhgp7-full-ball-dense-payload-v1` non
   canonique ; aucune porte `threads=1` vs 8 ni permutation physique à PointId
   constants ; aucune archive ne déclare `contributions`.
5. **[P2, non vérifié]** Identités à asserter : `N_A = Σ_B (min(Kmax,p+u) −
   max(1,p+q_min−1) + 1)` (5 510 027 pour 3 113 381 boules à 8k) et
   `resolver_meb_calls = anchor_hits + intruder_queries` (11 957 768 =
   10 396 562 + 1 561 206).
6. **[P2, non vérifié]** Vocabulaire : `cuda_census_cpu_full` est hors
   vocabulaire ; une route census-GPU + tour CPU séquentielle ne peut pas
   revendiquer « tour FULL sur G4 sous une seconde » ; sur le diagnostic 8k,
   ≈ 30 s amont contre 122 s de tour et 11 957 768 appels MEB pour
   10 456 312 représentants là où F résolvait les facettes régulières par label.
7. **[P2, non vérifié]** Fixtures des seuils (cercle plan vs octaèdre u=6 ;
   coquille asymétrique h_S=3), qui passent aujourd'hui, à graver avant tout
   raccourci `h_x`.
8. **[P2, recommandation]** Non-vacuité des coquilles supplémentaires aux
   tailles d'intérêt : `uniform` n'en produit aucune à 2 000, 8 000, 16 000 ni
   32 000 points ; `scanline_*` en produit (§ 4.6). Un reçu par famille
   structurée avec planchers `--min-extra-records` avant toute phrase sur les
   plateaux à l'échelle. (Réfuté comme P1 : aucune telle phrase n'existe et la
   porte oracle porte déjà ses planchers.)
9. **[remarque]** Contre-fixture « parents locaux ≠ globaux » et fixture
   `arity_mix` : la propriété est déjà exercée 42 fois par les fixtures
   existantes et les mutants correspondants sont déjà tués par `E5`,
   `growth_redundant`, `shell7_window`, `growth_ABCZ` ; les ajouter au reçu par
   traçabilité seulement. (Réfuté comme P1 par trois réfutateurs.)
10. **[remarque]** Table des refus (`full_ball_missing_closed_anchor`,
    `full_ball_anchor_level`, `*_not_prior`) à documenter comme fautes
    d'autorité ; fixture-limite de l'autorité relative (boule de fenêtre absente
    ⇒ niveau de fusion faux sans refus si un intrus existe) et statut S1 dans
    chaque reçu ; n=1 non exercé.

## 6. Delta `local_plateau.hpp` de `1fbe49d3` (raccourci diamétral) : reçu

`contre_lectures/local_plateau/` : la preuve (aux t=1 les arêtes absentes sont
exactement les paires antipodales, un couplage ; K_u moins un couplage est
connexe pour u≥3) et son adéquation au code sont vérifiées ; le descripteur émis
égale celui du chemin général sur 313 coquilles entières et le juge rationnel ;
les quatre mutants du reçu sont tués par le contrôle nommé. Remarques : (a) à
t=1 et q_min ≥ 3 le chemin général (2·2^u slots) est encore parcouru pour un
résultat identique — coût seulement ; (b) `inert_sufficient` vaut faux sur le
rang « toujours localement inerte » : convention déclarée, gardée seulement par
les littéraux des quatre coquilles réelles ; (c) un mutant `q_min ≤ 2` est
sémantiquement équivalent, non tuable.

## 7. Cohérence documentaire de `1fbe49d3`

`contre_lectures/coherence` : `check_docs` 405 fichiers, registre 20 phases
inchangé, `morsehgp3D_v7/audits/` non touché, pins et manifestes des deux reçus
exacts, `verify.py` normal/`-O` code 0. **Aucune affirmation fausse** ; cinq
imprécisions : « six CTests comparent … » (trois portes indépendantes),
« détection de fuites active » non vérifiable depuis le reçu direct,
« remplacements uniques » (chaque mutant réécrit aussi deux chemins d'inclusion),
« revue interne d'un alias mutable » non vérifiable, ligne de FAUSSES_PISTES
qui décrit une garde. `git diff --check` sur le commit rend 2 (blancs finaux
dans des captures brutes, conservés à dessein).

## 8. Méthode et vérification adverse

Six contre-lectures indépendantes ont rendu 51 constats bruts
(`contre_lectures/constats_bruts_six_dimensions.json`) ; les 18 constats de
gravité ≤ P2 les mieux classés ont été soumis chacun à trois réfutateurs à
lentilles distinctes (reproduction, contrat mathématique, portée/antériorité) ;
un constat n'est conservé que si deux lentilles au moins ne le réfutent pas.
Bilan (`contre_lectures/verdicts_adverses.json`) : V1 et V5 conservés (V5
requalifié en § 4.1) ; V2, V3, V4 réfutés à l'unanimité ; G1, G2, G3 conservés
et abaissés à P2 ; SEM-1, OD-1, OD-2 conservés ; 21 réfutateurs et la critique
de complétude ont échoué sur la limite de session (G4, G5, G6, COH-01, V6, V7,
V8 non jugés, marqués « non vérifié ») ; V9–V11 hors plafond. Les mutants du
§ 4.1 ont été reproduits par moi-même après leur signalement par un réfutateur.

## 9. Transmis à l'auditeur du journal (hors de ma qualification)

Sur `full_coverage_certificate.hpp` et sa porte à `1fbe49d3` ; la porte
renforcée `arena.*` de `d188e3de` ferme le trou des parents :

- Produit [P2, 2 confirmations dont une à faible confiance, 1 réfutation
  « invariant du producteur »] : une continuation ou une multifusion d'ordre K
  accepte une contribution depuis une population de cardinal < K ; le plancher
  « ≥ K » n'est appliqué qu'aux naissances (`journal_semantique/probe_permissive.cpp`,
  P1/P2). À trancher dans le contrat : refus du format ou obligation du producteur.
- Porte [P2 confirmés] : naissances non discriminées (mutants « plusieurs
  contributions », « drapeau intérieur non contrôlé », « cardinal < K »
  survivants) ; tri des parents non discriminé (raison attendue produite par
  un lot ultérieur) ; lecteur `include_interior=false` sur population à
  intérieur jamais exercé et statut non contrôlé sur racine absente
  (`journal_gate_mutants/MUTANTS_RESULTATS.txt`, 41 mutants).
- Porte [non vérifiés] : gardes de domaine sans fixture (ordre > 10, domaine <
  ordre, K1 incomplet) ; aucune multifusion K ≥ 2 ni arité ≥ 3 sous Gamma ;
  quatre CTests sans `TIMEOUT` ; chemin `length_error` non déclaré non exercé.
- Oracle différentiel : 388 journaux valides, 13 042 coupes, 320 invalides sur
  41 classes, 12 raisons produit, **0 divergence**
  (`journal_oracle_differentiel/`).

## 10. Suite du 10 septembre, soirée

[`suite_cache_20260910/`](suite_cache_20260910/README.md) : contre-lecture du WIP
suivant du constructeur (cache exact évictif des résolutions de facettes,
réserve exacte des arènes du journal, libération des structures de
construction, front WSPD de témoins universels par lots). Lecture favorable du
cache ; empreintes de payload identiques avec et sans cache sur cinq nuages ;
corpus aléatoire rejoué (2 507 nuages, 0 divergence) ; front par lots identique
au front scalaire, ordre, grand-livre et travail compris, jusqu'à n=8 000 ;
un défaut de porte : `facet_resolver_cache_gate` avorte sous ASan faute des
surcharges `nothrow` de `new`/`delete` (correctif vérifié).
