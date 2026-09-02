# NOTE_CLAUDE — pin sémantique KeyCSR et changement de priorité (2 septembre 2026)

```text
phase=exploration_v6_hors_registre
backend=cpu_reference
profile=quantized_u16_input_only
mode=audit_independant_math_and_architecture
public_status=not_claimed
```

Réponse au « Retour constructif sur le prototype KeyCSR non épinglé » de
`REPONSE_AUDITEUR_COMPACTDELTA_CSR_20260902.md` et à l'ordre recommandé (point 3
de « Dette d'échelle et ordre utile », `ETAT_COURANT.md`). Le commit qui porte
cette note est le **pin sémantique** du palier KeyCSR ; GCP non utilisé.

## 1. Ce que le pin contient

- Prototype reçu par le workflow (implémenteur + trois revues + passe de
  correction) : `src/forest/fold.hpp` (`ForestLayout {kClassic,kCsr}`,
  `ForestStorageKind`, `DeltaMeta`, `FacetKeyRange`, `ComponentDeltaView`,
  arènes possédées, offsets u32 demi-ouverts validés avant toute vue,
  `csr_emit` transactionnel), `src/core/mutants.hpp` (quinze `csr-*`),
  `src/pipeline/{run,digest}.hpp` (digest par l'accesseur commun, signature de
  stockage par K), `cli/mhgp6.cpp` (`--layout=classic|csr`, option produit),
  `tests/fold_csr_gate.cpp` + `tests/forest_witness.hpp` (`first_divergence`
  lisant les deux stockages à cru), `tests/conformity_v5.cpp`,
  `tests/selftest.cpp`, portes CMake (fixtures, offsets, débordement, copie,
  matrice pipeline, refus, mutants, conformité `--layout=csr`).
- Corrections des revues déjà intégrées avant cette note : garde de lisibilité
  sur `FacetKeyRange::size()` (pas un UB, conformément à votre rétractation),
  `storage_kind` signé avant le refus amont (fixture `--min-refus`),
  `forest_storage_kind=` imprimé depuis le kind **construit** (`mixte` si les
  K diffèrent), `bad_alloc` d'arène capturé dans `csr_emit` avec le mutant
  `csr-inject-bad-alloc`, `m.level`/`m.batch` à source unique,
  `delta(i) const&` avec surcharge `const&&` supprimée.

## 2. Les quatre points du retour, fermés dans ce commit

1. **Durée de vie de `for_each_delta`** : ref-qualifié `const&`, surcharge
   `const&&` supprimée ; dent de compilation dans `tests/fold_csr_gate.cpp`
   par deux concepts dépendants (`DeltaOnRef`, `ForEachDeltaOnRef`) avec
   `static_assert` positif sur lvalue et négatif sur temporaire, pour
   `delta(i)` comme pour `for_each_delta`.
2. **Portée du `try` de `csr-inject-bad-alloc`** : choix « resserrer » — le
   commentaire au site dit que seule la transaction d'append (gardes, appends
   d'arène, méta/offsets) est couverte ; les réserves initiales et le reste du
   fold propagent comme la route classique ; aucune interception générale de
   l'OOM n'est promise.
3. **Télémétrie** : `storage_allocations` renommé `csr_capacity_growths`
   (champ, ligne `stockage_foret`, docs) et documenté « csr seulement, classic
   = 0 = non instrumenté, jamais comparable » ; `offset_dernier_*` lus depuis
   `parents_off.back()` / `born_off.back()` (champs `*_off_back` de
   `ForestStorageStats`), plus jamais depuis la taille d'arène ;
   `octets_possedes exact=0` du classique reste une borne inférieure
   documentée (les capacités internes ne sont pas parcourues — à instrumenter
   au moment de la mesure, pas avant).
4. **Porte de profil sur le kind construit** : `tests/profil_gate.py` exige une
   ligne de tête unique `forest_layout=<demandé> forest_storage_kind=<construit>
   csr_fallback=0` et `kind=<construit>` sur chaque `stockage_foret K=` ;
   `tests/profil_contre_fixture.py` grave la scène que vous décriviez (demande
   `csr`, kinds construits classiques, ligne de tête puis `stockage_foret`) et
   vérifie qu'elle est tuée par la dent nommée, la sortie csr cohérente restant
   acceptée.

Aussi dans ce commit : allowlist `--inject` de `mhgp6_profile_sonde`
(exactement les trois ablations, aucun item vide, jamais un mutant de
production même présent au registre) avec trois portes de rejet
(`--inject=`, `--inject=,`, `--inject=render-active-only` ⇒ 2) ; câblage CTest
de `mhgp6_plan_keycsr_gate` (générateur `d6888093`).

## 3. Rejeux

- Workflow, avant les correctifs de cette note : `ctest -L gate -j3`
  181/181 (882 s), `scale8000` 3/3 (`mhgp6_fold_csr_pipeline_uniform_8000`
  1705 s, 0 désaccord sur 2 791 148 deltas), ASan+UBSan sur les portes csr
  rapides et les mutants de capacité : codes attendus, aucun diagnostic.
- Après les correctifs : portes ciblées (csr rapides, mutants, CLI layout,
  profil, contre-fixture, sonde, plan, conformités 200/400 des deux layouts)
  58/58 ; rejeu complet `ctest -L gate -j3` sur l'arbre exact de ce commit :
  **185/185** (638 s, dont `mhgp6_fold_csr_pipeline`, les huit conformités
  `--layout=csr`, dix-sept `mhgp6_mutant_csr_*`, `mhgp6_profil_identite`,
  `mhgp6_sonde_ablation_gate` rejouée seule ensuite sur le harnais final) ;
  `python tools/check_docs.py` : 253 fichiers.

## 4. Ce que je NE fais pas maintenant, et pourquoi

La directive exploitant du 2 septembre est explicite : « ne t'égare pas avec
trop de garde-fous de sécurité ; dès que ce sera bon, concentre-toi sur
l'implémentation multi-CPU et GPU pour nuages de dizaines de milliers de
points jusqu'à des dizaines de millions ; feu vert pour la GCP G4 ». En
conséquence :

- la **campagne de mesure KeyCSR** pré-inscrite (`reduce_v3`, manifeste
  scellé, rôles des cellules, hachage des binaires avant/après, adjacence,
  affinité attestée) est **différée**, pas abandonnée : le générateur de plan
  et sa porte existent, la pré-inscription reste verrouillée telle quelle ;
  elle sera armée avec vos conditions quand une session G4 aura un créneau
  après les mesures d'échelle. Aucun chiffre KeyCSR n'existe et aucun n'est
  revendiqué ;
- la priorité passe au **palier d'échelle** : (a) session G4 « échelle »
  (profil `gcp-migration/profils/g4_echelle_v1.env`, demande de GO à part) :
  frontière K=5 jusqu'à 2 M et K=10 jusqu'à 400k sous RLIMIT_AS, pilote
  série C à 100k/200k ; (b) palier GPU C6 = suppression du wire hôte qui
  plafonne le gain série C (conception par workflow en cours, avec juges
  adverses, avant tout code).

Vos rectifications de la sonde (35/36 puis 47/48 départs au-dessus du seuil
de charge, « huit CPU logiques exposés, pas une certification de cœurs ») sont
acceptées telles quelles et n'appellent pas de correctif de code.
