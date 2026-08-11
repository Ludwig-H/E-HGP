# Note de Claude — livraison des portes CPU : possession, préflight, ledger, familles scanline, sonde cellules, race réparée

Date : 11 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=complete_bounded_et_partial_refinement`, `mode=hybrid_prefix`,
`public_status=not_claimed`.

Cette note documente le lot committé au-dessus de `abb4c0e`, en réponse à
[`AUDIT_LIVE_PREFIX_INDEX_8DF7AC8_20260810.md`](AUDIT_LIVE_PREFIX_INDEX_8DF7AC8_20260810.md)
et à
[`REPONSE_QUESTIONS_CLAUDE_REPRISE_CPU_ET_SOURCE_50K_20260811.md`](REPONSE_QUESTIONS_CLAUDE_REPRISE_CPU_ET_SOURCE_50K_20260811.md)
(réception live intermédiaire comprise). Les cinq verrous préfixe, le ledger
pré-DSU, les familles scanline, le mode `prefix-all` gravé au différentiel,
la porte EMST par partitions, la data race `CertifiedIndex` et la première
sonde de masse de la source par cellules sont livrés. Aucun claim : les
audits motivent les corrections, ils ne certifient rien.

## 1. Terminaison hostile et porte multiensembliste (`prefix_index_gate`)

Le rang est tiré jusqu'à `min(11, universe)` : la campagne termine pour tout
`universe >= 8` — CTest dédié `mhgp3v_prefix_index_universe8`. La porte est
réécrite en multiensemble possédé par LOTS CONTIGUS (trois lots tirés par
deux coupes, masque de requêtes tiré) : la vérité est celle de la réponse Q2
— même lot avec au moins une extrémité requête, lots distincts seulement si
l'extrémité postérieure est requête — chaque paire à multiplicité exactement
un. La possession lit `staged_lot` (ce que le système SAIT du staging) pendant
que la vérité lit les lots logiques : c'est ce décalage qui fait mordre les
mutants de calendrier. Sept fixtures gravées, dont les communs
premier-de-M / dernier-de-N (tue `order-per-query`), la requête précoce avec
non-requête tardive du même lot (tue `stage-query-sequentially` par
l'identité de préflight), l'ancien-Q--nouveau-R entre deux lots (tue
`future-visible`) et la paire R--R laissée au sidecar. La permutation globale
commune (bijection de l'univers, une famille sur quatre) rend le même
multiensemble.

Campagne `400x24x8` : 84 367 paires possédées — 16 804 `Q--Q` de même lot,
50 750 candidat antérieur--requête postérieure, 16 813 `Q--R` de même lot —
48 539 obligations laissées au sidecar, 84 961 faux candidats refusés, masse
d'index 242 459 égale à la somme `L(r,K)`. Les huit mutants meurent, chacun
par le mécanisme prescrit :

| mutant | tué par |
| --- | --- |
| `prefix-length-minus-one` | fixture des communs en dernières positions |
| `drop-last-posting` | même fixture |
| `skip-recertification` | fixture du faux candidat |
| `duplicate-posting` | identité de masse `L(r,K)` (le préflight lui survit, comme prévu par Q3) |
| `double-query-pair` | multiplicité 2 dans le multiensemble |
| `order-per-query` | fixture premier-de-M / dernier-de-N |
| `stage-query-sequentially` | identité de préflight (degrés figés à la clôture du staging atomique) |
| `future-visible` | fixture ancien-Q--nouveau-R |

## 2. Possession canonique et identités dans le fold réel

`build_saturated_fold_hybrid` grave : `ActivationId` v1 = position dans
l'ordre global `(niveau exact, indice catalogue)` — admissible pour la gate
relative, la clé sémantique `(SphereKey, saturé)` reste à construire pour la
`GeneratorTable` device (réponse Q1) ; masque de requêtes figé au staging
atomique du lot (aides partagées `hybrid_level_batches`,
`hybrid_principal_certificates`, `hybrid_is_fallback_query`) ; possession
appliquée avant recertification ; préflight `predicted_hits` aux degrés figés
et DEUX identités à refus — `hits lus == prévus` et `entrées == somme L(r,K)`
(réponse Q3). Reçus séparés : `candidate_ids_including_self` /
`candidate_pairs_after_filter`.

Le LEDGER PRÉ-DSU (réponse Q2) est une capability de test :
`(ordre, lot, min(GeneratorId), max(GeneratorId))` après possession et
recertification, comparé dans `postings_join_gate` à une vérité indépendante
par énumération directe des couples. Il voit ce que l'idempotence DSU cache :
`double-query-pair` y meurt (7 702 paires possédées contre 7 260 attendues
sur la cosphère). Les cinq nouveaux mutants du fold meurent : ordre par
requête (différentiel des formes), futur visible (garde d'événement),
staging séquentiel (identité de préflight), double possession (ledger),
posting dupliqué (identité de masse).

## 3. Le mode `prefix-all`, gravé au différentiel

Septième forme dans `postings_join_gate` et mode `--join prefix-all` du
pipeline : le join préfixe RELATIF à toute `GeneratorTable` reçue — toutes
les requêtes, aucun certificat principal, aucun théorème des q attaches, donc
aucune prétention de famille complète. Son ledger possède sous
`is_query = rang > k`. La porte CMake
`mhgp3v_saturated_pipeline_prefix_all_partial` grave désormais
`--compare-joins 1` (dernier verrou de la réception live) : différentiel
contre la vérité G² sur la même famille tronquée, digests identiques,
préflight exact.

## 4. Sonde de masse au vrai masque et familles scanline

`prefix_mass_probe --mask hybrid` rejoue le staging par lot et le masque réel
du fold (aides partagées), avec préflight à refus et percentiles. Mesures à
`--points 48 --smax 11 --max-order 3 --seed 20260810`, hits à k=1 :

| famille | générateurs | lots | fallback | hits masque hybride | tout-requête | réduction |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| uniform | 4 777 | 4 420 | 497 (10 %) | 994 243 | 42 944 187 | ~43x |
| terrain | 1 778 | 1 369 | 530 (30 %) | 405 720 | 4 609 706 | ~11x |
| scanline_single_pass | 1 669 | 428 | 1 426 (85 %) | 1 357 751 | — | — |
| scanline_overlap_multiecho | 977 | 281 | 773 (79 %) | 343 916 | — | — |

Les familles `scanline_single_pass` et `scanline_overlap_multiecho`
(réponse Q5 : stries anisotropes 2x8, bandes de densité, trous markoviens,
bords francs par plateaux, multi-échos verticaux quantifiés, deux passages
décalés — construction entière) révèlent le régime annoncé : les ex æquo u16
du balayage envoient 80--85 % des générateurs au fallback via la prudence des
lots multiples. C'est la motivation mesurée de la « factorisation stricte des
ex æquo » de la réponse Q3 — qui exige d'abord la capability de fermeture des
carriers, jamais une hypothèse silencieuse ; la fixture des deux triangles de
rayon carré 25 est notée pour la factory `ValidatedHybridSidecar`.

## 5. Porte EMST k=1 par partitions et race `CertifiedIndex`

`structural_scale_check` compare désormais la PARTITION CANONIQUE des
`PointId` (étiquette = min de composante), stricte puis fermée, à chaque
niveau exact fusionné des deux côtés — couverture k=1 rejouée des lots contre
single-linkage rejoué des arêtes EMST — en O(masse·alpha(n)), praticable à
50 k. Sur `--points 200` : 35 704 lots de niveau exact, égalité stricte ET
fermée. Le mutant `--force-merge-early` (la fusion la plus tardive unie avant
le premier lot) rougit en partition STRICTE ; `--force-shift-fusion` rougit
toujours au multiensemble des niveaux.

La data race du `CertifiedIndex` partagé est réparée (chantier parallèle) :
compteurs `mutable` supprimés, `IndexMetrics` par appelant dans
`FlatStatistics`, scratch `NeighbourScratch` à époque par worker (l'ancien
`seen_candidate(n)` alloué par requête disparaît), ThreadSanitizer passe de
8 rapports (sortie 66, `sign_disagreement`) à 0 sur la même campagne, option
`MHGP3V_ENABLE_TSAN` câblée. La porte parallèle certifie le MULTIENSEMBLE
émis `(shell, intérieur, niveau)` à multiplicité un contre la vérité
séquentielle, à quatre threads et plus ; mutants `drop-odd-roots` et
`duplicate-root` tués, planchers de racines et de sommets de sous-arbres
calibrés à la moitié des campagnes mesurées.

## 6. Première sonde de masse de la source par cellules

`cell_source_mass_probe` implémente le préflight du théorème
cellule--candidat (réponse 20260811) sans former un seul tuple : grille
half-open sur la boîte racine (l'espace des CENTRES, cellules vides
comprises), témoins top-`t_q` ATTEINTS par anneaux de cellules avec borne
d'arrêt exacte (score de coin séparable), `Q_{q,C}` entier, dilations
exactes, accumulateurs `__int128`. L'identité de dilation (marche par
anneaux == balayage complet des n points) est une porte CTest sur `terrain`
et `scanline_overlap_multiecho`.

Mesures `terrain`, pas de cellule 4, seed 20260810 :

| n | cellules | m p50/max | R_2 | R_3 | R_4 |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 400 | 4 375 | 28 / 68 | 2,16e6 | 2,16e7 | 1,59e8 |
| 1 600 | 32 500 | 36 / 182 | 3,29e7 | 5,60e8 | 7,82e9 |
| 2 400 | 66 978 | 42 / 224 | 9,44e7 | 2,03e9 | 3,68e10 |

Constat honnête, conforme à l'avertissement de la réponse (« le nombre de
cellules est lui-même un poste d'admission ») : la lane q=2 est admissible,
q=3 tendue, et la lane q=4 est ROUGE sur cette partition uniforme — la
croissance est portée par les cellules du vide au-dessus de la nappe, dont le
`Q` couple la hauteur à l'étalement des témoins (`Q - h^2 ~ 2h*delta`) : `m`
y croît avec la hauteur de la boîte, et aucun raffinement du pas ne passe
sous ce plancher. C'est exactement le cas « condition nécessaire
supplémentaire ou autre partition, jamais un cap ». Les chiffres 50 k sont
au §7.

## 7. Sonde 50 k (pas 6, deux familles)

La course codespace (deux cœurs) a été abandonnée — trop lente pour être un
reçu — et remplacée par la session G4 mass-only du même jour : voir
[`NOTE_CLAUDE_SESSION_G4_MASSONLY_50K_20260811.md`](NOTE_CLAUDE_SESSION_G4_MASSONLY_50K_20260811.md)
(48 threads, 1,7--29 s par lane, trois familles, deux pas, prune publié).
Le verdict qualitatif du §6 est confirmé à l'échelle : q=2 admissible après
prune, q=3 et q=4 rouges sur grille uniforme.

## 8. Question à l'auditeur

La lane q=4 de la route cellule--centre déborde sur `terrain` par les
cellules hautes du vide. Trois voies étaient esquissées dans ta réponse :
le dispatcher exact par lane (comparer `somme_p C(d_p^+, q-1)` à `R_q` et
prendre la moindre masse admise), une condition nécessaire supplémentaire
pour les cellules dont la fermeture est loin du nuage (le `Q` par témoins
majore mal `beta` quand la cellule est haute), ou une partition anisotrope
(cellules aplaties près de la nappe, épaisses dans le vide). Laquelle
mérite la priorité de réception ? Ma lecture : le dispatcher est
immédiatement disponible (la route globale existe et son préflight est
exact), la condition nécessaire est le vrai théorème manquant — par exemple
une borne inférieure sur `beta` des miniboules possédées par `C` (tout
support est hors de `C` quand la cellule est vide : `beta >= dist^2(centre,
nuage)` impose `Q` utile seulement si la banque le reflète).

## 9. Tests

Suite complète : 191/191 CTests verts sur l'état combiné des deux chantiers
(TSan et préfixe), puis 7/7 sur le delta (`prefix_all` au différentiel,
sonde cellules, EMST partitions). Les commandes et sorties brutes sont
reproductibles depuis la provenance imprimée par chaque binaire.

GCP non utilisé.
