# Reçu : J2, filtre des nœuds du catalogue en forme D-loc, catalogue identique (29 septembre 2026)

`backend=reference_cpu`, `public_status=not_claimed`. GCP non utilisé pour ce reçu : les mesures sont locales et
donnent des ordres de grandeur ; la mesure de référence se fera sur G4.

## Objet

Étape J2 du plan de performance du catalogue : le coût par test du filtre des nœuds de l'arbre de boîtes
(`src/catalogue/generator.cpp`, `filter_node` et `process`). Un agent l'a réalisée sur une copie de `568d45297`. Son
reçu complet, avec l'énoncé et la preuve de chaque lemme, est [`RECU_AGENT_J2.md`](RECU_AGENT_J2.md). Les scripts et
sorties qu'il cite sont copiés ici ; binaires et arbres de construction restent hors dépôt (`build/v10-j2/`).

Changements :

- forme D-loc exacte en i64 (lemme 1), sans flottant ;
- garde retirée comme test séparé (lemme 2 : elle est incluse dans D) ;
- comptage sans branchement, par tranches S0 puis Y \ S0 (lemme 3) ;
- réservoir par insertion avec le même départage (lemme 4) ;
- pré-ignorance par l'enveloppe de la liste parente (lemme 5) ;
- option CMake `MHGP10_MARCH`, vide par défaut ;
- grand livre : `guard_tests` et `dominance_tests` retirés, `filter_tests` et `preskipped_bbox` ajoutés,
  `skipped_bbox` imprimé.

## Relecture et intégration

- **Relecture des lemmes.**
  - D-loc est le maximum exact de |Y − C|² − |X − C|² sur la boîte fermée, donc le même entier que la forme par
    coins.
  - La garde est incluse dans D parce que S0 ⊆ Y.
  - Le départage (distance, rang) du réservoir est celui de (distance, indice), les listes restant croissantes.
  - L'enveloppe des candidats est incluse dans celle du parent.
  - D-loc suppose des boîtes cubiques : la racine l'est (même côté, une puissance de 2, sur les trois axes) et la
    découpe en huit le reste.
- **Différentiel à l'intégration** ([`differentiel_integration.txt`](differentiel_integration.txt)). Le binaire du
  worktree porte J2 et les correctifs d'audit du même jour. On le compare au binaire `568d45297` de l'agent
  (`build-base`) sur les 10 entrées :
  - dumps canoniques identiques à 1 et à 4 fils ;
  - grand livre identique hors compteurs retirés ou ajoutés ;
  - `skipped_bbox` égal à l'identité de l'arbre complet.
- **Portes** ([`ctest_gate_integration.txt`](ctest_gate_integration.txt)) : 9 sur 9 vertes avec J2 et les correctifs
  d'audit. L'oracle catalogue et l'oracle tour sont compris.
- **Clés retirées.** Aucun lecteur du dépôt ne lit `guard_tests` ni `dominance_tests`. Les seules occurrences sont
  des prototypes d'audit à implémentation propre (`audits/audit_v9_20260928/preuves/`).

## Mesures de l'agent (codespace Zen 3 chargé, 1 fil)

| Mesure | K = 5 | K = 10 |
| --- | --- | --- |
| Filtre seul (tics TSC) | ×3,4 | ×3,8 |
| Coût par test du filtre (tics) | 24 → 8,7 | 22 → 6,7 |
| Étage des boîtes, trame 02 entière (s) | 10,34 → 6,31 (×1,64) | 37,89 → 25,47 (×1,49) |

Avec `MHGP10_MARCH=x86-64-v3`, le filtre gagne ×4,5 à ×6,6.

Après J2, la feuille domine l'étage des boîtes : le filtre n'en pèse plus que 27 % à K = 5 et 17 % à K = 10.

## Suite

- Mesurer J2 sur G4, en build par défaut.
- L'option `MHGP10_MARCH=x86-64-v4` n'est permise sur G4 qu'après la même porte ISA (différentiel des dumps et du
  grand livre), qui n'a pas pu être jouée ici faute d'AVX-512.
- J3 : la feuille, en en-tête commun CPU/GPU.
