# Addendum — fold compact : canonique min-fid, tables à époque, partition dense (−32 % sur t_fold)

Date : 18 août 2026. Base : `e573888` ; code livré dans le commit
portant ce reçu. Exécute la réponse d'audit
`REPONSE_A_CLAUDE_57523A_FOLD_COMPACT_PREFLIGHT_DEVICE_20260818.md`
dans son ordre recommandé (§ 1.1, 1.2, 1.4, § 2, § 3, § 4).

## § 1.1 — canonique par min-fid, partition dense

`canon_of` (vecteur de `FacetKey`, comparaison complète à chaque
union) est remplacé par `canon_fid` (u32) : les fid étant attribués en
ordre de `FacetKey` croissante, $fid_1 < fid_2 \iff keys[fid_1] <
keys[fid_2]$ — l'union prend le min de deux u32. La partition finale
devient DENSE : `facet_keys[fid]` (strictement croissante) +
`final_canon_fid[fid]` (plus petit fid de la composante) ;
`final_partition` (map) n'est plus qu'une VUE de compatibilité remplie
sur demande (`fill_legacy_partition`) — le chemin d'échelle (juge
éteint) ne la paie plus. Invariants structurels PERMANENTS
(`partition_violations`, traité comme un invariant de rôles par
l'appelant) : clés strictement croissantes, $canon \leq fid$,
idempotence.

## § 1.2 — plus aucune map par lot

`prebatch_roots`, `pre_canon`, `touched` (trois `std::map` par
macro-lot) sont remplacés par des tableaux à époque sur les racines
(`pre_epoch`/`pre_canon_fid`, `post_epoch`/`post_slot`) et un brouillon
de deltas RÉUTILISÉ entre les lots (capacités conservées). Les listes
de racines sont TRIÉES avant matérialisation : l'ordre observable
historique (racines croissantes) est conservé au bit près — la porte à
deux backends l'atteste. Les seules allocations restantes sont celles
des deltas effectivement ÉMIS (le CSR de § 1.3 reste en réserve, voir
mesure).

## § 1.4 — chronos grossiers

Quatre chronos par appel (jamais par lot) : `t_batching`, `t_intern`,
`t_reduce`, `t_partition`, publiés par le probe (`fold_phases`, temps
CPU cumulé à N fils). La séparation rôles/unions/deltas n'est
volontairement pas mesurée (elle exigerait un chrono par lot).

## § 2 — portes

- Backend FIGÉ `build_forest_legacy` (copie stricte, marqué « ne pas
  optimiser : il est le témoin ») ; porte `--fold-compact-gate` sur le
  pipeline réel de deux familles × 10 K : égalité des compteurs, des
  niveaux de lot, des nœuds, des DELTAS paire à paire, de la map ET de
  la vue dense contre la map. À supprimer avec le backend après
  requalification.
- Mutant `canonical-is-uf-root` (le canonique suit la racine UF au lieu
  du minimum) : tué à code 4, comme prescrit.
- 113 CTest verts (toutes les portes historiques — relabeling,
  plateaux, deltas, juge — passent sur le fold compact avec la vue de
  compatibilité par défaut).

## § 3 — préflight requalifié et porte q2 durcie

- Le préflight s'imprime désormais `bytes_forest_events` (plus jamais
  « octets_resident » : il ne couvrait que le flux d'événements) et
  publie les bornes par buffer (`bytes_facet_incidence_records`,
  `bytes_event_to_fid`, `bytes_unique_facets_upper` — borne
  $\leq$ incidences —, `bytes_union_find`, `bytes_deltas_upper`,
  `bytes_partition_upper`) avec sa portée honnête :
  `event_expansion_after_census`. Le préflight par tuile de clés (aucun
  vecteur global de `BallData`) viendra avec le contrat
  `product`/`max_output_bytes`.
- Porte `q2_birth_gate` durcie : l'égalité d'un second diamètre est
  IMPOSSIBLE (théorème du § 3 : tous les autres points vivent dans la
  boule ouverte de rayon $D/2$) — toute occurrence est une VIOLATION,
  l'échappatoire « dégénérescence » est supprimée.

## § 4 — fermeture device

`detail_ev::uabs` est annoté `MHGP4_HD` (la frontière transitive de
`q4_level_raw` et `cmp_mu_same_side` est fermée) et le témoin de
compilation `src/gpu/device_compile_witness.cu` appelle chaque
primitive depuis un `__global__` — à compiler par la PREMIÈRE session
G4 avec nvcc avant tout kernel. Statut : `device_annotation_started` →
fermeture écrite, la preuve de compilation attend nvcc.

## Mesure (uniform n=8000, smax=11, `--threads=4`, juge éteint)

| | avant (aval parallèle) | **fold compact** |
|---|---|---|
| t_fold | 56,1 s | **38,3 s** (−32 %) |
| … t_batching | — | 2,3 s |
| … t_intern | — | **26,2 s** (nouveau dominant) |
| … t_reduce | ~40 s (est.) | **18,5 s** |
| … t_partition | (dans les 40 s) | **0,7 s** |

Sorties IDENTIQUES (3 126 158 événements, 19 465 140 fusions,
1 974 086 nœuds). Total du run : ~115 s (contre 343 s au moteur v1).
Verdict pour § 1.3 (CSR des deltas) : la réduction restante (18,5 s)
contient unions + rôles + deltas émis — le nouveau dominant est
l'INTERNEMENT (construction des `FRec` + tri + boucle d'internement,
26,2 s) : c'est lui, pas le CSR, le prochain candidat, et il est
partitionnable (votre § 4.2 : records produits par partitions, tris
locaux, fusion) — ce qui rejoint directement la route streaming.
