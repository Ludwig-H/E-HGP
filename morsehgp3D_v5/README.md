# morsehgp3D_v5 — reprise à propre de la v4

Cadre : `phase=exploration_v5_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

La v5 remplace `morsehgp3D_v4/` comme chantier actif. Elle calcule **le même
objet que la v4** — les **dix forêts horizontales** HGP K = 1..10 du manuscrit
(Défs 20–31, Théorèmes 2–7), niveaux et événements exacts sur le profil u16 ;
les applications verticales entre ordres (la « tour ») ne sont **pas**
livrées — avec **le même contrat de test** (fixtures gravées, mutants tués, codes de sortie exacts, planchers
de couverture, équivariance) et **une base de code neuve** : la v4 est un
sujet différentiel et une source de contre-fixtures et de digests épinglés,
jamais une base de code ni une autorité implicite. Tout port contractuel
depuis la v4 est explicite, épinglé et requalifié (`docs/PROVENANCE.md`).

## Ce que la v5 corrige de la v4

Les errances de fond relevées par l'audit du 22 août 2026
(`morsehgp3D_v4/audits/ETAT_COURANT.md`) sont traitées **dès la conception** :

- **résidence** : la v4 gardait les dix forêts résidentes (7,7 Go à n=8000,
  21 Go à 32000) et son plafond mémoire mentait ; la v5 **streame par ordre K**
  (expansion, fold, signature, publication, libération — un seul K en
  construction, les boules censusées comme seul amont résident) et nomme
  ses rôles mémoire sans promettre de pic ;
- **monolithes** : `bench/forest_probe.cpp` (4 478 lignes, toutes les portes)
  et `ball_stream.hpp` (1 805 lignes) deviennent des modules nommés et une
  porte par thème dans `tests/` ;
- **mutants** : registre unique (`src/core/mutants.hpp`, `MHGP5_MUTANT`) au
  lieu de booléens enfilés dans les signatures ou de copies locales du code ;
- **chemins morts et opt-in négatifs** (sélection axiale, `build_forest_legacy`,
  modes de banc dans la production) : retirés ; ce qui survit est consigné
  dans `docs/PISTES_FERMEES.md` ;
- **doctrine dispersée dans les commentaires** : `docs/ARCHITECTURE.md` est le
  seul document d'architecture, et il décrit le pipeline réel.

## Contrats

Dix forêts horizontales K = 1..10, événements et niveaux exacts, rendu § 9.1 ;
**portes d'invariants et de mesure à n = 8000, 16000, 32000** sur cette
machine (8 cœurs, 31 Go), puis contrats à 50 000 points sur G4 via
`gcp-migration/`, puis des dizaines de millions de points. Aucun claim tant
que les portes ne le prouvent pas : `public_status` reste `not_claimed`.
L'état courant est le verdict de l'auditeur (`audits/ETAT_COURANT.md`) ;
les réponses de l'implémenteur sont les `audits/REPONSE_CLAUDE_*`.

**Conformité v4 ≡ v5** (digests canoniques au format v4, `digest_balls` et
`digest_all`, sur les mêmes entrées) : porte `mhgp5_conformity_*`, reçu
`receipts/conformite_v4/digests_v4.txt` calculé par la v4 ; les campagnes à
manifeste sont dans `receipts/conformite_v4/`. Un digest égal prouve « même
objet que la v4 », jamais l'exactitude HGP : celle-ci relève des oracles
bornés (label `oracle`) et des preuves de `docs/MATHEMATIQUES.md`.

## Parcours de lecture

1. `audits/ETAT_COURANT.md` — l'état courant unique, ancré au `HEAD`.
2. `docs/ARCHITECTURE.md` — les structures, le pipeline, les frontières
   mémoire, les statuts transactionnels.
3. `docs/MATHEMATIQUES.md` — l'objet, la réduction q2/q3/q4, les statuts de
   chaque énoncé (hérités de la v4 avec leur statut re-déclaré).
4. `docs/PLAN_DE_TESTS.md` — labels CTest, tailles, oracles, mutants.
5. `docs/PROVENANCE.md` — module par module, ce qui vient de la v4, comment
   c'est épinglé et par quelle porte c'est requalifié.
6. `docs/PISTES_FERMEES.md` — mémo append-only hérité v3 + v4.
7. `audits/README.md` — le contrat de l'auditeur.

## Construire et tester

```bash
cmake -S morsehgp3D_v5 -B build/v5 -DCMAKE_BUILD_TYPE=Release
cmake --build build/v5 --parallel
ctest --test-dir build/v5 --output-on-failure
ctest --test-dir build/v5 --output-on-failure -L gate        # fixtures, mutants, invariants (~5 min)
ctest --test-dir build/v5 --output-on-failure -L oracle      # juges independants a petit n
ctest --test-dir build/v5 --output-on-failure -L scale8000   # tailles d'interet (puis scale16000, scale32000)
cmake -S morsehgp3D_v5 -B build/v5-asan -DCMAKE_BUILD_TYPE=Debug -DMHGP5_ENABLE_SANITIZERS=ON   # ASan + UBSan
```

Préfixe des cibles et tests : `mhgp5_`. Portes à code de sortie exact
(0 conforme, 1 juge, 2 refus, 3 invariant, 4 mutant tué), crash par signal
refusé partout (`cmake/run_expect.cmake`). Les mutants ne sont compilés que
dans les cibles de test (`MHGP5_TESTING`) ; le pilote produit `mhgp5` n'en a
aucun.

## Arborescence

```text
src/core      types u16/i64/i128, entiers larges U192/U320, Morton, mutants, SHA-256
src/cloud     familles de nuages (port bit à bit v3/v4, digests gravés)
src/tree      index spatial unique : positions uniques, buckets, arbre radix
src/wspd      décomposition bien séparée par vagues
src/spindle   fuseaux W_q, boule-cœur, comptage de témoins (h_coeur/h_a/h_b)
src/lanes     formes exactes q2/q3/q4, clés, niveaux, cover d'arête
src/pipeline  génération → RLE → préfiltre → census → plateaux → flux
src/forest    fold par K (macro-lots, deltas), rendu § 9.1
oracle/       juge indépendant, arithmétique volontairement autre
tests/        une porte par thème
bench/        probes counter-only
cli/          point d'entrée mhgp5
docs/ audits/ receipts/
```
