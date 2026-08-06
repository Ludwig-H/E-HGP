# Design — Verrou ④ : produire la tour exacte depuis le nuage

Statut : design normatif, implémentation par incréments. Aucun claim.
Quatrième verrou de l'ordre normatif de la roadmap ; débloque le gate 1 M
(requalifié depuis 15L) puis les campagnes ⑤ (50 k → dizaines de millions,
garde-fou Delaunay minimal, cible produit < 1 s à 50 k).

## Inventaire vérifié : la tour existe, son étage supports est le goulot

La chaîne complète depuis un nuage brut existe déjà en composants réels —
c'est exactement ce que construit la fixture du reducer
(`test_hierarchy_direct_morse_forest_reducer.cpp:536`) :

1. `CanonicalPointCloud::rejecting_duplicates` ;
2. `MortonLbvhIndex::build` ;
3. `build_exact_pair_support_stream(index, cloud, K, budget.pair)` ;
4. `build_exact_higher_support_stream(index, cloud, K, budget.higher)` ;
5. `build_exact_direct_support_terminal_facade(index, cloud, K, budget,
   pair, higher)` ;
6. `build_exact_direct_morse_event_journal(cloud, facade)` ;
7. `build_exact_direct_saddle_arm_seed_journal(cloud, facade,
   event_journal, seed_budget)`.

Le verrou ④ n'est donc PAS de créer la tour : c'est de rendre son étage 4
scalable. `build_exact_higher_support_stream` est le flux exhaustif hôte
dont le no-go n=32/5000 a été historiquement constaté — et dont le
remplacement scalable est précisément le verrou ① fermé : la frontière
device-tiled qualifiée M5b, pilotée transactionnellement par le pont M2
(`HigherSupportDeviceTiledSessionBridge` →
`ExactHigherSupportAnchoredSession`), dont le différentiel scellé a déjà
établi l'égalité d'ensembles à payload complet des événements et
diagnostics avec l'oracle exhaustif.

## Incréments

**④-a — producteur composé host-first.** Un composant
`build_exact_direct_morse_tower_from_cloud` expose la chaîne 1→7 derrière
une surface bornée unique : budgets par étage, sélecteur de backend scellé
pour l'étage supports (`exhaustive_host_v1` seul dans cet incrément),
reçu de tour (digests d'identité par étage : nuage canonique, compteurs
LBVH, audits des deux flux, certificat de façade, comptes des journaux),
fail-closed à chaque étage. Test : la tour composée reproduit exactement
la fixture du reducer (mêmes journaux par `operator==`) sur les scénarios
existants — pas de nouveau petit test, un différentiel de composition.

**④-b — backend `device_tiled_session_v1`.** L'assemblage d'un
`ExactHigherSupportStreamResult` compatible façade depuis les chunks
committés de la session ancrée pilotée par le pont M2 : événements,
diagnostics extra-shell et certificats de prune drainés des chunks
(l'oracle trie canoniquement — l'assemblage reprend
`canonical_sort_records_`), audit BigInt recomposé depuis l'identité
d'induction $R_j + C(F_j) = \binom{n}{3} + \binom{n}{4}$ déjà
recalculée à chaque commit. Différentiel : backend device ≡ backend
exhaustif sur les nuages sanctionnés (n=32 : les six cas historiques),
via le moteur hôte certifié bit-identique au natif (aucun GPU requis
localement). L'étage paires reste `build_exact_pair_support_stream`
hôte tant que sa tenue à 50 k n'est pas mesurée ; le chemin paires
device-tiled 50 k déjà qualifié est le remplaçant désigné si besoin.

**④-c — producteur CLI et gate 1 M.** Outil de campagne
(`--points/--seed/--family/--maximum-order/--directory`) : tour → reducer
segmenté (15I) → archive durable 15L → relecture bornée ; c'est le
« recertifier massif réel » requalifié. Gate complet 1 M avant
10 000 001 points sur G4, puis 50 k < 1 s et le comparateur Delaunay
minimal comme garde-fou externe (jamais dans l'algorithme), selon les
directives scellées du 6/8.

## Points de vigilance

- La façade consomme les RÉSULTATS des flux : l'assemblage ④-b doit
  produire chaque champ consommé par
  `build_exact_direct_support_terminal_facade`, pas seulement les
  événements ; l'inventaire exact des champs lus par la façade précède
  l'implémentation.
- Les budgets de l'étage supports scalable ne sont plus des caps de
  travail artificiels (directive du 6/8 : plus de caps dans les
  validations) mais les capacités physiques scellées du moteur tuilé.
- Le sélecteur de backend est scellé dans le reçu de tour ; aucun chemin
  ne peut mélanger silencieusement les deux backends dans une même tour.

## Décisions d'intégration ④-b (vérifiées sur les surfaces réelles)

- La façade consomme exactement : `requirements`, `events`,
  `relevant_extra_shell_diagnostics`, `audit`, `stream_complete()` et le
  fait d'absence certifiée — jamais les `prune_certificates` eux-mêmes.
- Le pont M2 expose par transaction `committed_events` et
  `committed_extra_shell_diagnostics` déplacés du chunk vérifié par la
  session, plus `session_terminal()`, `trusted_checkpoint()` et son audit
  de masses BigInt ; l'assemblage boucle `advance_one_tile_transaction()`
  jusqu'au terminal, échoue fermé sur poison/censure (l'orchestration de
  rebind reste au-dessus en v1).
- L'assemblage vit dans la couche GPU (`src/gpu/`) : la construction du
  lease de traversée (`MortonLbvhBuildContext`) y est naturelle — fake
  launchers en local (le fake scientifique certifié), natif sur G4 — et le
  producteur ④-a est refactoré pour exposer un helper partagé des étages
  aval (façade → journaux) consommé par les deux tours.
- Le tri canonique de l'oracle exhaustif (`canonical_sort_records_`) doit
  être exposé publiquement (même précédent que les digests de segments
  15I) : l'égalité de RÉSULTAT exige l'ordre canonique, pas seulement
  l'égalité d'ensembles du différentiel M2.
- Différentiel local : tours device ≡ exhaustive sur les nuages fixtures
  (n≤14, secondes) ; les six nuages n=32 en différentiel complet vont à la
  session G4 de ④-b (l'exhaustif hôte n=32 non capé est trop lourd pour le
  codespace, directive du 6/8).

## Design ④-b2 : le genre de source « session ancrée » de la façade

Constat scellé par ④-b1 : `verify_exact_higher_support_stream` re-exécute
le flux exhaustif et exige `observed == expected` au complet — base de
certification correcte pour de petits nuages, structurellement
anti-scalable et inaccessible au backend device (les payloads de prune ne
sont pas retenus).

**Principe.** La seule entité qui a VU et VÉRIFIÉ chaque transition est la
session ancrée (`commit_prepared` rejoue fraîchement chaque chunk sur CPU).
L'autorité du nouveau genre de source est donc la session elle-même, jamais
le pont ni l'appelant : un assembleur de flux possédé par la couche
hiérarchie accumule les records committés et minte, au terminal, un
certificat scellé que la façade vérifie de façon bornée.

**Composants.**
- `ExactHigherSupportAnchoredStreamAssembler` (hierarchy) : possède la
  session ancrée construite sur l'autorité ; expose à l'orchestrateur
  (le pont M2) le même protocole prepare/commit par délégation ; sur chaque
  commit vérifié, s'approprie les événements et diagnostics du chunk ; au
  terminal (checkpoint `locally_complete`), trie sous l'ordre canonique
  public, assemble le `ExactHigherSupportStreamResult` et minte le
  certificat — records, tri et digests restent internes à l'autorité.
- `ExactHigherSupportAnchoredStreamCertificate` (hierarchy, mint privé) :
  lie le digest du manifeste d'autorité, le digest du checkpoint terminal,
  le compte de transactions committées, les comptes et digests canoniques
  des événements et diagnostics du résultat assemblé, et l'audit terminal ;
  expose `certifies(result)` (recalcul des digests de contenu du résultat
  présenté, égalité stricte) et `certified_for_authority(manifest)`.
- Façade : `build_exact_direct_support_terminal_facade` gagne une surcharge
  acceptant `(higher_result, certificate)` ; sa base de certification
  devient : manifeste recalculé depuis (index, cloud, K) égal au manifeste
  lié, `certificate.certifies(higher_result)`, clôture BigInt de l'univers
  contre le binôme recalculé, et les invariants locaux du résultat — sans
  aucune re-exécution du flux. Le certificat renseigne
  `higher_source_kind = anchored_session_chain` dans le certificat de
  façade ; le chemin historique `fresh_resident_replay` reste intact.
- Pont M2 : surcharge de construction empruntant l'assembleur externe au
  lieu de posséder sa session ; l'assemblage gpu ④-b1 se réécrit sur
  l'assembleur et cesse de copier les records lui-même.

**Différentiels.** (1) façade(anchored) ≡ façade(exhaustif) membre à membre
sur les nuages fixtures — la tour device devient certifiable de bout en
bout et son différentiel complet vs la tour exhaustive se rejoue ;
(2) anti-forge : résultat mutés (événement altéré, audit gonflé, ordre
non canonique), certificat d'une autre session/autorité, et certificat
minté avant terminal — tous rejetés fermés.
