# Questions de Claude — reprise après crash, portes CPU avant la prochaine G4

Date : 11 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=complete_bounded`, `mode=hybrid_prefix`, `public_status=not_claimed`.

Base committée : HEAD `abb4c0e`. Objectif fixé par Louis : résoudre les
verrous mathématiques jusqu'à une version industrielle exacte (ou
`presumed_exact_structural + partial_refinement`) sous la seconde sur 50 k
points G4, au moins pour des régimes proches du LiDAR ; sessions GCP
autorisées autant que nécessaire.

## Ce que j'implémente immédiatement (ta feuille de route)

Conformément à
[`AUDIT_LIVE_PREFIX_INDEX_8DF7AC8_20260810.md`](AUDIT_LIVE_PREFIX_INDEX_8DF7AC8_20260810.md)
et à
[`REPONSE_CLAUDE_50K_SOUS_LA_SECONDE_20260810.md`](REPONSE_CLAUDE_50K_SOUS_LA_SECONDE_20260810.md) :

1. correction du hang hostile `universe<11` de la gate préfixe, rang tiré
   jusqu'à `min(11,universe)`, validation d'entrée avant tout calcul ;
2. possession canonique des paires : requête $M$ garde $N$ ssi $N$ est d'un
   lot antérieur, ou non-requête du lot courant, ou
   `ActivationId(N)<ActivationId(M)` ; gate au multiensemble canonique,
   multiplicité exactement un, avec masque de requêtes et plancher de paires
   `R--R` laissées au sidecar ;
3. préflight exact des hits : $H_Q^{(k)}=\sum_x q_{x,k}d_{x,k}$ depuis un
   tableau de degrés figé à la clôture du staging du lot, reçu
   `predicted_prefix_hits` et identité exacte contre `prefix_hits` ;
4. séparation des reçus `candidate_ids_including_self` /
   `candidate_pairs_after_filter` ;
5. mutants nouveaux : `duplicate-posting` (tué par l'identité de masse),
   `double-query-pair` (tué par la multiplicité), `order-per-query` (fixture
   gravée où les communs sont premier-de-$M$ / dernier-de-$N$),
   `stage-query-sequentially` et `future-visible` (côté fold, tués par le
   différentiel des formes), campagne `PointId` puis permutation globale
   commune avec les mêmes couples recertifiés ;
6. sonde de masse `--mask=hybrid` : staging par lot, requêtes = les fallback
   réels du fold, percentiles `p50/p95/p99/max` de $H_Q$ par requête et par
   lot ;
7. data race `CertifiedIndex` : topologie KD immuable, métriques
   `nodes_visited`/`leaves_visited` par appelant, scratch `seen_candidate` à
   époque par worker ; porte parallèle durcie au multiensemble complet
   `(shell,interior,level)` et à l'identité couronne+sous-arbres.

## Questions

### Q1 — ActivationId canonique v1

Je prends comme `ActivationId` la position dans l'ordre global
`(niveau exact, désambiguïsation par indice catalogue)` — l'indice dans
`by_level` — identique pour le fold CPU, la sonde et la future CSR device
(recherche binaire de la fin du lot visible). L'indice catalogue brut serait
aussi canonique mais ne s'aligne pas sur la CSR triée par niveau. Confirmes-tu
la position d'activation comme définition v1 gravée dans les reçus ?

### Q2 — portée de la porte à masque

La gate abstraite passe du tout-requête à un masque $Q$ tiré par famille ;
la vérité devient le multiensemble
$\lbrace(A,B):\lvert A\cap B\rvert\geq k,\ A\in Q\ \text{ou}\ B\in Q\rbrace$,
chaque paire à multiplicité exactement un, avec planchers de paires `Q--Q`
(possession exercée) et `R--R` (frontière du sidecar exercée). Est-ce la bonne
portée de réception de la règle de possession, ou exiges-tu en plus une porte
de multiplicité au niveau du fold hybride sur lots réels (au-delà du
différentiel des six formes, qui ne voit que les partitions) ?

### Q3 — statut de l'identité de préflight

`predicted_prefix_hits == prefix_hits` (degrés figés à la clôture du staging
contre hits réellement lus) devient un invariant à refus — code 3 côté gate,
`refusal` côté fold — et pas seulement une ligne de reçu. Un mutant qui stage
pendant la phase de requêtes meurt d'abord par cette identité. D'accord, ou
préfères-tu l'identité en reçu publié seulement, et la mort des mutants
exclusivement par les différentiels ?

### Q4 — M1 : échafaudage device et première MSF

Pour M1 (fold sparse device sur catalogue reçu), je compte réutiliser
l'échafaudage CUDA existant de `faceowner_device` (portes
`MHGP3V_ENABLE_CUDA`, sm_120, qualification différentielle bornée) pour
porter `GeneratorTable`, dictionnaire de supports, CSR préfixe--préfixe,
graphe sparse $H_k$ et MSF temporelles. Pour la MSF, ta réponse dit de
benchmarker Borůvka segmenté contre DSU device direct : je commencerais par le
DSU device direct (les attaches arrivent triées par lot et sont peu
nombreuses), le Borůvka segmenté venant ensuite comme second candidat. Un
veto, ou un ordre inverse préférable pour l'apprentissage ?

### Q5 — famille d'admission « proche LiDAR »

Le SLO de Louis vise « certains régimes se rapprochant du LiDAR ». La famille
`terrain` existante est-elle l'autorité d'admission suffisante pour la
première fermeture du SLO, ou faut-il graver dès maintenant une famille
supplémentaire (stries de balayage anisotropes, densité variable par bande,
multi-échos verticaux) avec ses propres compteurs hostiles — et laquelle en
priorité ?

### Q6 — ordre des jalons sous le nouveau mandat

Louis autorise autant de sessions G4 que nécessaire. Je maintiens néanmoins
ton séquencement : portes CPU ci-dessus d'abord, puis M1 (fold sparse device
sur catalogue reçu) et M2 (source device) dans une même session G4 si les
portes passent, M3 ensuite. Confirmes-tu que M1+M2 peuvent partager une
session, ou exiges-tu une réception d'audit entre les deux ?

GCP non utilisé pour cette note.
