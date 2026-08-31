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
  → E3 par ancre : AnchorCredit (compose unique), ResidualTape à rôles,
       tueurs W_q exact / secteurs corrigés / grille 10.5
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

## E3 — crédits et tueurs d'ancre `[LIVRÉ]`

`src/credit/` : `AnchorCredit` (unique opération `compose`, certificat C5),
`CoreCredit` (recertification `collect_universal_ids`, V1 nominale
`r_core == h_core`, repli sans exclusion), `ResidualTape` (rôles
témoin/support par record, masques de lane, exclusions par lane —
certificat C6). Tueurs en coût croissant, tous fail-open, tous composant par
`compose` : comptage W_q exact saturé sur le cover ; secteurs octogone K=8
(forme corrigée `min_k max(cnt[k], cnt_res[k] + credits) ≥ h_q`, le max
par secteur AVANT le min) ; grille de cellules G=8 (Th. 10.5,
`rho2_den` 12/8).

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
  - passe 2, O(m_e log m_e), seeds survivants seulement : racines
    `μ_z = P/B` triées (produits croisés exacts), clip non strict à ±μ*,
    balayage → minimum exact sur corde, fragments shallow ; chaque site à
    rôle support est lu à sa racine `μ_d` : profondeur au point (règle de
    bloc) + crédits < h₄ ⟹ cascade O(1) (lentille, owner, exact-once,
    préfiltres i64, puissance de face, Cramer, centre strict) ⟹ émission.
  La boucle C×D et le filtre de profondeur par candidat n'existent pas en v6.

## E5 — aval `[LIVRÉ]`

`src/pipeline/candidates.hpp` (tri stable parallèle + RLE par clé : arité
minimale puis plus petite représentation du niveau), `census.hpp`
(`ball_depth_at_least` : bornes de parabole convexe par axe, range-add O(1),
arrêt au seuil, STRICT ; `ball_census` : I_B/U_B complets, plafonds),
`expand.hpp` (événements, plateaux par quotient exact, Carathéodory),
`src/forest/` (fold streamé par K, `fold_inflight` borné, macro-lots de
niveaux sémantiquement égaux, deltas, partition dense ; rendu § 9.1),
`digest.hpp` (format `mhgp4-digest-v1`, monnaie de conformité v5↔v6).

Frontières de digest : à J2, le multiensemble émis est identique à la v5
(le sweep est une pure transformation de coût), donc `digest_balls` v6 ==
`digest_balls` v5 et la conformité le vérifie aussi. Le jour où un tueur v6
plus fort rompt légitimement cette égalité, `digest_balls` bascule sur la
frontière post-préfiltre exact (voir `docs/PROVENANCE.md`) ; la conformité
v5↔v6 reste sur l'objet (`digest_all`, `digest_forest_K*`).

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
par `fold_inflight`, publication dans l'ordre des K. GPU : subordonné aux
reçus G4 (voir la note de conception § 7) ; aucun code device en v6 tant
qu'aucun reçu de gain n'existe.

## Statuts transactionnels

`complete_regular | unsupported_degeneracy | resource_exhausted |
invalid_input | invariant_violated` ; gardes de bibliothèque avant tout
calcul ; jamais un préfixe publié ; callbacks provisoires jusqu'au statut
terminal. Payload `mhgp6-forests-horizontal-v1`,
`forest_semantics=verified_events_only`, `vertical_maps=none`.
