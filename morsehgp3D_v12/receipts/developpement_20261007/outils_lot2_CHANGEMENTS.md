# Juges M5 et M6, diagnostic `stats` de M5, tables de découpes : changements fichier par fichier (lot 2)

7 octobre 2026. Base HEAD `98ca07556` ; correctif `../patch_lot2.diff` (chemins `a/morsehgp3D_v12/...`,
`b/morsehgp3D_v12/...`, applicable par `git apply` à la racine du dépôt). Règles d'adoption, seuils, statistiques et
géométrie inchangés ; seules changent l'admission des preuves, la relecture et l'ordre des gardes.

État : terminé (19 h 41 UTC).

## MES-M5 (`microbancs/mes_m5_parcours/`)

| Fichier | Changement | Constat | Test |
| --- | --- | --- | --- |
| `include/mhgp12/traversal/driver.hpp` | diagnostic par niveau (`stats`) réservé avant la boucle à une borne fixe (`3 B + 1` niveaux) : plus aucune allocation de l'hôte entre la lecture des totaux d'un niveau et ses gardes `wide_leaf` et capacité ; même contenu publié | observation du reçu `audit_juges_emst_20261007/m5` (`CST-0018` / budget) | `driver_selftest` 29/0 ; sur le `driver.hpp` du HEAD : 21 écarts ; identité hôte des six fixtures inchangée |
| `host/driver_selftest.cpp` (nouveau), `CMakeLists.txt` | porte de l'ordre des gardes : vrai `Driver::run` sur un exécuteur factice (totaux fabriqués niveau par niveau), opérateur `new` global compté ; refus parents, tâches et feuille large aux profondeurs 0, 1, 2, 4, 8, 16, 32 sans réservation, noyau ni allocation après les totaux ; témoins vivants (allocation vue, niveau admis qui atteint la réservation suivante) ; cible `mhgp12_traversal_driver_selftest` | idem | GCC 13 et Clang 18 `-Werror` ; mutant natif `m5_stats_alloue_avant_la_garde` tué |
| `scripts/g4_traversal_bench.py` | `comparison_problem` : champs de comparaison typés et recoupés (identité, statut, grand livre aux cinq comptes, feuilles et référence égale au vidage, empreintes hexadécimales, compteurs d'écart, première différence ; identité vraie ⇒ statut et grand livre du vidage, feuilles égales, aucun écart, empreintes égales), appliqué aux lignes d'identité hôte et à `bench_case` (tours, fixtures appareil, Compute Sanitizer) ; `bench_case(…, reps, reference_digest)` : empreinte de référence égale à celle de l'identité hôte du même vidage, séries `total_ms`, `resident_ms`, `wall_ms` d'exactement `reps` valeurs finies positives ; empreintes de référence collectées à l'identité hôte (session et relecture) ; relecture : grand livre et feuilles des vidages lus dans la sortie publiée de l'outil (`summary`) et dans `oracle_check` des fixtures ; raison du refus d'une prise sanitizer publiée par cas | `CST-0018` (reçu `audit_juges_emst_20261007/juges`) | porte 38/0 ; session C relue « adopté », identique à l'octet ; 6 mutants nouveaux tués |
| `tests/test_juge_m5.py` | fabrique complète (champs du vrai producteur : statut, grand livre, feuilles, empreintes, compteurs, `wall_ms`) ; prises périmées complètes écrites après les vidages ; 8 injections nouvelles (les quatre de l'auditeur, puis grand livre GPU différent, feuilles de référence hors vidage, empreinte de référence forgée, empreintes absentes) ; une exception du pilote devient un écart | idem | 38 cas, 0 écart ; pilote du HEAD : les 8 injections adoptées |
| `README.md` | carte (porte des gardes), preuves exigées (champs de comparaison, empreinte croisée, effectifs), commandes avant session, note de capacité | — | règles de `tools/check_docs.py` rejouées |

## MES-M6 (`microbancs/mes_m6_session/`)

| Fichier | Changement | Constat | Test |
| --- | --- | --- | --- |
| `run_m6.py` | relecture v2 stricte : `report_v2_problems` (champs obligatoires typés, SHA-256 du binaire et des deux sources attendues, compilation de code 0, isolation du début cohérente, aucun refus publié), `isolation_problem` (`quiet` vrai si et seulement si code 0 et aucune ligne de processus, et vrai), `run_v2_problems` (durée, sortie d'erreur, empreinte, deux relevés cohérents, prise déclarée conforme sans problème), médianes publiées égales à celles recalculées ; relecture v1 inchangée (limites déclarées) | `CST-0018` (reçu `audit_juges_emst_20261007/juges`) | porte 54/0 ; session A relue `mes_m6_ok` ; pilote du HEAD : les 10 rapports falsifiés nouveaux relus `mes_m6_ok`, code 0 |
| `tests/test_juge_m6.py` | `FALSIFICATIONS_V2` : les trois mutations de l'auditeur (provenance vide, isolation contredite, refus explicite laissé) et sept gardes seules (empreinte du binaire vide, sources vides ou incomplètes, isolation du début contredite, prise déclarée non conforme, médianes fausses, champ obligatoire absent) | idem | 54 cas, 0 écart ; 8 mutants nouveaux tués |

## Mutants des juges (`microbancs/outils/mutants_juges.py`)

| Changement | Test |
| --- | --- |
| 15 mutants nouveaux, un par garde nouvelle : 6 du juge de MES-M5 (`m5_identite_sans_recoupement`, `m5_grand_livre_facultatif`, `m5_empreintes_non_typees`, `m5_feuilles_de_reference_libres`, `m5_empreinte_croisee_ignoree`, `m5_mesures_non_comptees`), 1 natif (`m5_stats_alloue_avant_la_garde`, porte `m5_driver` nouvelle), 8 de la relecture de MES-M6 (`m6_rejuge_schema_ignore`, `m6_rejuge_binaire_non_type`, `m6_rejuge_sources_non_typees`, `m6_rejuge_isolation_contredite_admise`, `m6_rejuge_isolation_du_debut_ignoree`, `m6_rejuge_refus_publies_ignores`, `m6_rejuge_prise_non_conforme_admise`, `m6_rejuge_medianes_non_recoupees`) ; `m6_rejuge_isolation_ignoree` remis au nouveau code ; 81 au total | voir `RAPPORT.md` § 4 |

## Documents

| Fichier | Changement |
| --- | --- |
| `docs/DONNEES.md` | en-tête et § 3 : découpes régénérées le 7 octobre par la règle corrigée, plus de mention « à rejouer » ; quatre tables de découpes (69 lignes) refaites depuis les manifestes : taille visée, sites distincts réels, retours couverts, côté du carré, bits, empreinte à 16 caractères ; § 7 et § 9 : durée du rejeu complet et tailles des quatre paquets multi-millions ; aucune coordonnée |
| `microbancs/README.md` | état de MES-M5, MES-M6 et de l'outil de mutants (81) |
