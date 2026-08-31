# MorseHGP3D v6 — chantier actif

Cadre à annoncer au début de toute tâche v6 :

```text
phase=exploration_v6_hors_registre
backend=cpu_reference
profile=quantized_u16_input_only
mode=audit_independant_math_and_architecture
public_status=not_claimed
```

La v6 remplace `morsehgp3D_v5/` comme chantier actif. Elle calcule **le même
objet** que la v5 (les dix forêts horizontales HGP K=1..10 du manuscrit,
Défs 20–31, Th. 2–7, niveaux ρ² exacts sur le profil u16, sémantique publiée
`forest_semantics=verified_events_only`, `proof_basis=gabriel_positive_connectivity`,
aucune sortie ne porte `require_exact=true`) sur un **socle v5 porté et
requalifié** (docs/PROVENANCE.md) avec une **génération q3/q4 neuve** au
contrat de coût **sortie-sensible** (voir
`audits/NOTE_CLAUDE_CONCEPTION_V6_20260831.md`, document fondateur, et
`audits/ETAT_COURANT.md` qui prime).

La v5 est le sujet différentiel : pin de référence `3bad233d`, digests gravés
dans `receipts/conformite_v5/` (cinq familles × {8000, 16000, 32000}, graine 3,
s=8, smax=11). La conformité v5↔v6 se prouve sur `digest_all` et les dix
`digest_forest_K*` (l'objet). Deux monnaies de candidats distinctes sont
gelées : `digest_candidates_v5_compat` (tag v4, post-RLE — diagnostic
différentiel ; diverge légitimement de la v5 depuis le cover q4 au
coefficient 4) et `digest_postprefilter` (tag neuf, survivants du préfiltre
exact — non-régression interne v6). Un digest de candidats mesure un filtre,
jamais l'objet.

## Ordre de lecture

1. `audits/NOTE_CLAUDE_CONCEPTION_V6_20260831.md` — l'architecture et ses
   raisons (panel de conception, corrections imposées, jalons).
2. `docs/MATHEMATIQUES.md` — l'objet, la réduction q2/q3/q4, les certificats
   v6 et leurs statuts.
3. `docs/ARCHITECTURE.md` — le pipeline **réel** (mis à jour à chaque jalon).
4. `docs/REGIMES.md` — familles de mesure, familles stationnaires,
   contre-familles, doctrine de pente.
5. `docs/GRAND_LIVRE.md` — les termes de coût publiés et les portes go/no-go.
6. `docs/PLAN_DE_TESTS.md`, `docs/PROVENANCE.md`, `docs/PISTES_FERMEES.md`.
7. `audits/ETAT_COURANT.md` (dès qu'un auditeur l'ouvre) puis `audits/` en
   ordre chronologique inverse.

## Commandes

```bash
cmake -S morsehgp3D_v6 -B build/v6 -DCMAKE_BUILD_TYPE=Release
cmake --build build/v6 --parallel
ctest --test-dir build/v6 --output-on-failure
ctest --test-dir build/v6 --output-on-failure -L scale8000     # tailles d'intérêt
./build/v6/mhgp6 --family=uniform --n=8000 --seed=3 --digest   # pilote produit
```

Préfixes : cibles `mhgp6_*`, macros `MHGP6_*`, namespace `mhgp6`.
C++20 sans extensions, `-Wall -Wextra -Wpedantic -Werror`, portes à code
exact via `cmake/run_expect.cmake` (0 conforme, 1 juge, 2 refus, 3
plancher/invariant, 4 mutant tué), labels `gate` / `oracle` /
`scale8000/16000/32000`. La CI GitHub ne construit pas la v6 : rapporter
explicitement les résultats CTest locaux.

## Règles reprises telles quelles (doctrine v5 conservée)

- Une seule structure spatiale (tri Morton 48 bits + arbre radix de Karras sur
  positions uniques) ; aucune mosaïque d'ordre supérieur, aucun catalogue
  ∝ C(n,k).
- Toute décision entière (i64/i128/U192/U320, largeurs déclarées en tête de
  fichier) ; flottant = filtre certifié à repli exact seulement ; jamais de
  jitter ; dégénérescences → refus explicite.
- Tueurs fail-open seulement (jamais de fausse mort) ; une optimisation change
  le coût, jamais l'objet.
- Profil : u16 quantifié, `PointId` u32 arbitraires, positions dupliquées
  refusées (`unsupported_degeneracy`), s ≥ 8, smax ≤ 11.
- Statuts transactionnels `complete_regular | unsupported_degeneracy |
  resource_exhausted | invalid_input | invariant_violated` ; jamais un préfixe
  publié.
- Sortie bit-identique quel que soit le nombre de fils ; parallélisme mesuré,
  jamais déclaré ; registre unique de mutants injectés dans le code de
  production (compilés sous `MHGP6_TESTING` seulement).
- Streaming par ordre K (`fold_inflight` borné), quatre rôles mémoire.
- GPU subordonné aux reçus G4 ; aucun étage device sans reçu de gain.
