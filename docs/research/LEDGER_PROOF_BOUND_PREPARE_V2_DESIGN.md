# Design — prepare_transition proof-bound v2 du ledger sans rebuild d'index

Statut : design en cours d'implémentation (verrou ② de l'ordre normatif de la
roadmap). Aucun claim. Référence de scoping : session du 5/8/2026, vérifiée
contre `direct_sparse_root_ledger.cpp` au commit `8b3fbeb`.

## Constat v1 (vérifié ligne à ligne)

Le `prepare_transition` v1 (`direct_sparse_root_ledger.cpp:1578-2343`) est
correct mais non borné en trois familles d'opérations :

1. **Lookups actifs non bornés** : `impl_->handle_row(parent_handle)` par
   parent de groupe (l. 1883), par naissance standalone (l. 2018), par record
   de preview (l. 2076) et par entrée staged (l. 2101) — chaque appel sonde
   l'index actif sans budget. La variante bornée
   `active_root_index_search_bounded` (l. 322-376) n'est consommée que par
   `probe_active_root` (l. 1485-1576).
2. **Consultation historique du forest** : `forest.lookup(record.requested_handle)`
   par record de preview (l. 2074) — c'est la dépendance `const Forest&` à
   éliminer; et au commit, la réconciliation post-commit refait un
   `forest.lookup` par record (l. 2397-2405).
3. **Rebuild d'index** : `build_replacement` (l. 2161-2233) re-scanne la
   table de slots entière et la reconstruit quand `required_slot_count`
   dépasse la capacité courante.

Le digest v1 (`compute_preview_digest`, domaine `forest-preview/v1`)
n'absorbe pas `pre_ticket_origin` / `pre_root_handle` / `pre_component_size`
— d'où l'interdiction (commentaire du forest hpp:767-770) de passer le
preview pré-origine au chemin v1.

## Décisions v2

**Signature.** Nouvelle surcharge sans `const Forest&` :

```cpp
prepare_transition_from_active_root_proofs(
    ExactDirectSparseStableFacetForestPreparedBatch&& forest_ticket,
    ExactDirectSparseStableFacetForestProofBoundPreoriginPreparedPreview&&,
    std::span<const ExactDirectSparseRootLedgerActiveRootProbeResult>
        active_root_proofs,
    groups, parent_root_handles, group_point_deltas,
    standalone_births, standalone_birth_point_deltas) noexcept;
```

**Élimination des lookups (garantie `no_historical_handle_or_root_lookup`).**
- Chaque `handle_row(...)` v1 est remplacé par la consommation d'un proof
  `probe_active_root` déjà minté, fourni par l'appelant, validé pour le stamp
  courant (modèle de `materialize_active_coverages_from_active_root_proofs`,
  qui expose déjà `no_historical_active_root_lookup`). L'ensemble de proofs
  doit être canonique (trié par handle, sans doublon) et couvrir exactement
  les handles requis; tout manque est un rejet, jamais un scan de secours.
- Chaque `forest.lookup(...)` v1 est remplacé par les champs pré-origine du
  record de preview proof-bound (`pre_root_handle`, `pre_component_size`,
  `pre_ticket_origin`), qui certifient l'état pré-batch sans consulter
  l'index historique du forest.

**No-rebuild.** La capacité des deux tables de slots (`active_handle_slots`,
`active_root_id_slots`) est dimensionnée UNE FOIS à l'initialisation depuis
`maximum_active_root_count` (le budget est déjà scellé et cumulatif). Le
chemin v2 refuse (`no_budget_exhausted`) toute transition dont le
`post_active_count` exigerait plus de slots que cette capacité initiale :
`build_replacement` est structurellement inatteignable depuis v2, et le
commit n'insère que depuis `staged_entries` (insertions bornées par la
longueur de chaîne de sondage, elle-même bornée par le facteur de charge
scellé de la table pré-dimensionnée).

**Domaine digest v2.** Nouveau domaine
`…/forest-preview/v2/sha256/` pour le digest de reçu de preview, absorbant
par record les trois champs pré-origine en plus des champs v1; nouveau
domaine de transition v2 distinct pour `canonical_transition_digest`. Les
chemins v1 et v2 ne peuvent pas produire le même digest (séparation de
domaine testée).

**Commit v2.** Le ticket préparé v2 porte un marqueur proof-bound; à
`commit`, la réconciliation par `forest.lookup` est remplacée par la
vérification des compteurs du commit forest contre le reçu du preview
pré-origine (inserted/union/effective déjà comparés en v1) plus l'égalité
du post-stamp du forest avec le post-stamp attendu scellé du preview;
`erase_slot`/`insert_slot` restent, bornés par la table pré-dimensionnée.
Toute divergence poison le ledger (le forest n'a pas d'image de rollback),
inchangé.

**Mode.** Le champ `direct_sparse_root_ledger_mode` reste v1 (gelé); le
reçu v2 porte `proof_bound_preorigin_prepare_v2 = true` et le nouveau champ
de décision documente le chemin. Aucun changement de schéma de stockage.

**Tests** (`test_hierarchy_direct_sparse_root_ledger.cpp`, oracle à poser
près de la l. 1341) : différentiel v1/v2 sur transitions identiques (mêmes
entrées → mêmes entrées d'histoire, coverage, root ids, stamps), séparation
de domaine des digests, rejets fail-closed (proof manquant/périmé/forgé,
preview v1 refusé par v2 et réciproquement, capacité de slots insuffisante
→ refus sans rebuild), anti-forge sur les champs pré-origine.
