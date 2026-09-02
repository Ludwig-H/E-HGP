# Architecture v6 — le pipeline réel

Règle héritée de la v5 : ce document décrit le pipeline **réellement codé**.
Chaque étage porte son état : `[LIVRÉ]` (code + portes vertes) ou `[PRÉVU]`
(conception seulement, voir `audits/NOTE_CLAUDE_CONCEPTION_V6_20260831.md`).
Un étage prévu n'est pas une promesse de coût.

## Vue d'ensemble

```text
entrée (PointId u32, positions u16³ distinctes)
  → E0 index : tri Morton 48 → buckets uniques → arbre radix (Karras),
       boîtes serrées, ledger global des paires
  → E1 UNE descente WSPD fusionnée à masques de lanes
       → MultiAliveRect{rect, mask, core[3]}
  → E2 fermeture des facteurs (route S directe / route M saturée)
  → E3 par ancre : EndpointCredit + tueurs W_q exact / secteurs corrigés /
       grille 10.5 (AnchorCredit/ResidualTape : contrat 2, prévu J3)
  → E4 lanes : q2 direct ; q3 seeds aigus + filtre de profondeur ;
       q4 sweep de corde unifié (passe 1 scan saturé, passe 2 racines triées)
  → E5 tri stable + RLE par BallKey → préfiltre exact count-only
       → census I_B/U_B → événements (plateaux) → fold streamé par K → rendu
  → E6 (conditionnel) Tier R par rectangle, moteur plan par ancre lourde
```

## E0 — index `[LIVRÉ]`

`src/core/` (types, morton, intmath, wide, dint, mutants, sha256, parse) et
`src/tree/cloud_index.hpp`. Une seule structure spatiale : tri Morton 48 bits
par (clé, PointId), bucketisation des positions dupliquées (capacité de
représentation ; le pipeline les refuse avant géométrie), arbre radix binaire
de Karras sur positions uniques, remontée des boîtes serrées. `PointId` ≠
index dense ≠ rang Morton (conversion uniquement par `point_id(u)`).
Coordonnée hors profil ou PointId dupliqué ⟹ index invalide avant toute
construction.

## E1 — descente WSPD fusionnée `[LIVRÉ]`

`src/wspd/wavefront.hpp` (primitive `separated` entière sans racine, interdits
gravés : terminal dès que séparé, scission du plus grand diamètre géométrique)
et `src/pipeline/generate.hpp::alive_rectangles_fused`. Une seule descente
pour les trois lanes : par paire de nœuds, `count_universal_witnesses`
(masques de lanes, compteurs écrêtés à h_q) ; une lane sort du masque dès que
son cœur atteint h_q ; le rectangle n'est scindé que si le masque reste non
vide ; à un terminal, un second comptage avec autorité de coins fournit
`core[3]`. Les décisions de scission sont indépendantes de la lane : l'arbre
de rectangles est identique à trois descentes séparées (porte d'égalité
`mhgp6_fused_descent_gate` contre la triple descente test-only, avec
`smax_effective`). Sortie parallèle par tranches ordonnées, bit-identique au
séquentiel. Ledger global des paires :
`Σ masses émises + Σ masses tuées = C(n,2) − Σ C(mult_u, 2)`.

## E2 — fermeture des facteurs `[LIVRÉ, route S]` `[PRÉVU, route M]`

Route S (défaut, facteurs sous seuil) : histogrammes h_a/h_b par produit
direct aux 8 coins (autorité exacte, ordre de parcours historique). Route M
(au-dessus du seuil, figé avant mesure) : requêtes one-sided ALL/NONE/MIXED
saturées au seuil + range-add (certificat C7) ; compteurs V_R/C_R/P_R.
Cascade : `need = h_q − core` ; raccourci `(|A|−1) + (|B|−1) < need` ⟹
histogrammes sautés ; ancres survivantes ssi `core + h_a + h_b < h_q`.

## E3 — tueurs d'ancre `[LIVRÉ : EndpointCredit + tueurs portés]`
`[PRÉVU : AnchorCredit/CoreCredit/ResidualTape]`

Livré (ports requalifiés par les portes) : `EndpointCredit` (base h_a+h_b,
domaine disjoint hors A∪B) transmis aux tueurs ; tueurs en coût croissant,
tous fail-open : comptage W_q exact saturé sur le cover ; secteurs octogone
K=8 (forme corrigée `min_k max(cnt[k], cnt_hors[k] + base) ≥ h_q`, le max
par secteur AVANT le min) ; grille de cellules G=8 (Th. 10.5, `rho2_den`
12/8). Prévu (contrat 2 de MATHEMATIQUES C3, chantier J3) : `src/credit/`
avec `AnchorCredit` (unique opération `compose`), `CoreCredit`
(recertification), `ResidualTape` (rôles et exclusions par lane).

## E4 — lanes `[LIVRÉ]`

`src/lanes/` + `src/pipeline/generate.hpp`.

- q2 : `BallKey` primitive (1, −(a+b), a·b), toujours émise, profondeur au
  census.
- q3 : cover coefficient 3, boucle seeds aigus (lentille, acuité stricte,
  owner EdgeKey), forme de Gram, filtre de profondeur à la génération (scan
  saturé, étage flottant certifié à repli exact), émission.
- q4 : sweep de corde unifié par seed aigu (certificats C1–C4) :
  - passe 1, O(m_e), rôle témoin : `P(z)`, `B(z)` ; cœur de Jung
    (`P < 0 ∧ 2P² > J·B²`) saturé — sortie anticipée identique v5 ;
    au passage `depth0` (profondeur au centre μ = 0) décide le q3 du carrier ;
  - passe 2, O(m_e log m_e + p_e), seeds survivants seulement : racines
    `μ_z = P/B` triées (produits croisés exacts, comparaisons comptées),
    clip non strict à ±μ* ; chaque site à rôle support est lu à sa racine
    `μ_d` : profondeur au point (règle de bloc, cover complet, AUCUN crédit)
    < h₄ ⟹ cascade O(1) (lentille, owner, exact-once, préfiltres i64,
    puissance de face, Cramer, centre strict) ⟹ émission.
  Le rescan de profondeur PAR CANDIDAT n'existe pas en v6 (mutualisé par
  seed) ; l'incidence seed–complétion reste payée (une racine par site
  éligible, `q4_completions`). Contrat de profondeur : contrat 1 de
  MATHEMATIQUES C3 — cover complet, verdict `depth_at(μ_d) ≥ h₄` sans aucun
  crédit ajouté. Cover PAR LANE : coefficient 3 pour q3 (sharp), 4 pour q4
  (les intérieurs q4 vivent dans le coefficient 4 — P0 du 31 août, porte
  `mhgp6_cover_coef4`, mutant `q4-cover-coef3`).

## E5 — aval `[LIVRÉ]`

`src/pipeline/candidates.hpp` (tri stable parallèle + RLE par clé : arité
minimale puis plus petite représentation du niveau), `census.hpp`
(`ball_depth_at_least` : bornes de parabole convexe par axe, range-add O(1),
arrêt au seuil, STRICT ; `ball_census` : I_B/U_B complets, plafonds),
`expand.hpp` (événements, plateaux par quotient exact, Carathéodory),
`src/forest/` (fold streamé par K, `fold_inflight` borné, macro-lots de
niveaux sémantiquement égaux, deltas, partition dense ; rendu § 9.1),
`digest.hpp` (format `mhgp4-digest-v1`, monnaie de conformité v5↔v6).
Deltas en deux stockages signés `forest_storage_kind ∈
{vector_component_delta_v1, csr_facet_keys_v1}` (route `--layout=`, défaut
classic, aucune route de repli, `csr_fallback` mesuré et nul par
construction) : le CSR porte `DeltaMeta` (96 octets), deux vecteurs
d'offsets demi-ouverts u32 et deux arènes possédées de `FacetKey` ; vue
`ComponentDeltaView` reconstruite à la demande par `ForestResult::delta(i)`,
jamais stockée ; digest champ par champ via l'accesseur — octets identiques
pour les deux stockages. Un échec csr (structure → `invariant_violated`,
capacité ou `bad_alloc` d'arène capturé dans le fold → `resource_exhausted`,
jamais une exception hors de `reduce_fold`) vide le payload avant tout
callback ; `csr_capacity_growths` compte, sous csr seulement, les changements
observés de `capacity()`, réserves initiales comprises (télémétrie causale ;
le classique n'est pas instrumenté et publie 0, jamais un chiffre comparable) ;
`offset_dernier_*` est lu depuis `parents_off.back()` / `born_off.back()`.

Frontières de digest (gelées, P0 du 31 août) : la conformité d'objet
v5↔v6 juge `digest_all` et `digest_forest_K*` seulement. Deux monnaies de
candidats distinctes : `digest_candidates_v5_compat` (tag v4, uniques
post-RLE — diagnostic différentiel de génération ; diverge légitimement de
la v5 depuis le cover q4 coefficient 4) et `digest_postprefilter`
(tag `mhgp6-digest-v1:postprefilter-candidates`, records survivants du
préfiltre exact — non-régression interne v6, golden gravé sur uniform 400).
Aucun renommage conditionnel futur. Le stockage des deltas n'entre jamais
dans le digest (ni kind, ni offsets, ni padding, ni capacité) : la
conformité v5 juge le même reçu sous `--layout=classic` et `--layout=csr`.

## E6 — étages conditionnels `[PRÉVU]`

Déclenchés seulement si les pentes stationnaires l'exigent (voir
`docs/GRAND_LIVRE.md`) : Tier R (grille 3D de centres par rectangle,
certificat unilatéral i64, sonde contrefactuelle appariée obligatoire avant
raccord) ; moteur plan par ancre lourde (niveaux ≤ h du plan bissecteur,
route adaptative m0).

## Parallélisme

Brouillons par ouvrier, fusion en ordre d'ouvrier, tri stable + RLE
canonisent ; sortie bit-identique quel que soit le nombre de fils ; ouvriers
mesurés (compteurs retournés). Fold : étage A parallèle, étage B en vol borné
par `fold_inflight`, publication dans l'ordre des K. `ForestResult` est
copiable et autonome : une copie prise dans `on_forest` reste lisible après
la destruction de l'original et des événements (`mhgp6_fold_csr_copie`). GPU : subordonné aux
reçus G4 (voir la note de conception § 7) ; aucun code device en v6 tant
qu'aucun reçu de gain n'existe.

## Statuts transactionnels

`complete_regular | unsupported_degeneracy | resource_exhausted |
invalid_input | invariant_violated` ; gardes de bibliothèque avant tout
calcul ; jamais un préfixe publié ; callbacks provisoires jusqu'au statut
terminal. Payload `mhgp6-forests-horizontal-v1`,
`forest_semantics=verified_events_only`, `vertical_maps=none`.

`run_pipeline` enrobe le corps du pipeline et convertit un `std::bad_alloc`
— la seule exception qu'il capture, y compris relancée depuis un worker du
fold — en refus transactionnel `resource_exhausted` qui **nomme l'étage
atteint** (curseur `RunResult::stage_reached`, avancé aux mêmes points que
les `rss_mb`), sans jamais publier de préfixe de payload ; ce n'est pas une
garantie anti-OOM (l'OOM killer reste hors de portée, `RLIMIT_AS` borne
l'espace virtuel et non le RSS). Le texte du refus ne porte jamais la sous-chaîne `bad_alloc` : le validateur de campagne tient les classes d'issue de la phase frontière pour mutuellement exclusives, un code 2 étant un refus typé et un code 134 une allocation non capturée. La provision du message est elle-même sous la garde (mutant `caps-throw-bad-alloc-provision`). « Aucun préfixe publié » se lit avec sa portée exacte : les callbacks déjà appelés restent provisoires jusqu'au statut terminal et l'invalidation interne ne reprend aucun effet externe, donc la porte ne prouve l'absence de publication que pour une panne antérieure au premier callback.
