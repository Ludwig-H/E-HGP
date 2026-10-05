# Vérification légère de la tranche S9 : hiérarchie de points native et `--sortie=points`

Relecture du 5 octobre 2026, de 17 h 00 à 17 h 06 UTC (`date -u`). Commit relu : `451301787`, local, HEAD détaché sur
`53c027fe8`, dans `/workspaces/E-HGP/build/v11-impl-l3`. Rien n'a été modifié dans le worktree. GCP non utilisé.

Cadre :

```text
phase=exploration_v11_hors_registre
backend=cpu_reference
profile=quantized_u21_input_only
public_status=not_claimed
```

## Verdict

Le port est fidèle et l'exactitude tient sur le chemin réussi. Une correction reste **bloquante** : le chemin de refus
du tri des dates strictes, que l'auditeur avait signalé (`d448b3d03`, reçu `audit_s9_wip_20261005`,
`APPORTS_AUDIT_d448b3d03.md`) avant le commit. Le commit le reprend **à l'identique**. Ce défaut ne touche pas les
sorties produites aujourd'hui. Il rend toutefois indéfini, avec risque de lecture hors tableau, le refus
`radical_sign_budget` que l'API promet.

## Bloquant

**B1. Tri des dates strictes : `src/points/point_tree.cpp:80-91` (`entry_order`).**
- Après un refus de `compare_dates`, le comparateur renvoie `a < b` sur les `SiteIdx`, et cela pour toutes les
  comparaisons suivantes.
- `failure` n'est testé qu'au retour de `std::sort`. La relation d'ordre change donc pendant le tri, ce qui est un
  comportement indéfini.
- Dans libstdc++, le partitionnement et l'insertion non gardés reposent sur une sentinelle établie par l'ordre
  précédent. Le modèle de l'auditeur sort du tableau avec 17 entrées.
- Ce défaut était connu avant le commit (apport `d448b3d03`, lu par l'orchestrateur à 16 h 56) et n'a été ni corrigé
  ni mentionné dans les écarts de `impl_s9.md`.

**Correction attendue.**
- Interrompre le tri au premier `Outcome` refusé, sans comparaison de substitution. Deux voies conviennent :
  - un tri propre qui propage l'`Outcome` (par exemple un tri fusion stable, puisque l'ordre voulu est déjà
    « date, puis SiteIdx ») ;
  - une exception locale levée par le comparateur et interceptée **dans** `entry_order`, sans jamais traverser
    `noexcept`.
- Garder le départage par `SiteIdx` pour les seules égalités exactes.
- Ajouter une porte par injection (voir A1).

## À corriger

- **A1. Aucune porte n'exerce le refus en cours de tri : `tests/points/points_test.cpp`, groupe `refusals`.**
  Ajouter un cas, ou un crochet de test sous macro, qui injecte un refus `radical_sign_budget` dans
  `compare_dates` après plusieurs comparaisons réussies, dans un groupe strict d'au moins 17 entrées. La porte doit
  vérifier :
  - le code rendu ;
  - le budget rendu (`budget.reserved()` revenu à sa valeur d'entrée) ;
  - l'absence de publication par la CLI.

  Aujourd'hui, le groupe `refusals` couvre K = n, m hors domaine et le budget, mais jamais un refus pris dans le tri.
  C'est la porte que demande l'auditeur pour G4.
- **A2. Écart non déclaré : `/workspaces/E-HGP/build/v11-persist/sortie_supports/impl_s9.md` (section des écarts).**
  Ajouter que l'alerte `d448b3d03` sur `entry_order` n'est pas traitée dans `451301787`, ou mieux, la traiter (B1).
  La réponse à l'auditeur se fait par une `REPONSE_CLAUDE_*` dans `morsehgp3D_v11/audits/`, au moment du push.
- **A3. Compteur jamais publié : `src/points/settle.cpp:150-158` (`hang_sites`).** `RootTally::on_demand`, le nombre
  de racines calculées hors table (rangs sondés par la recherche du plancher), n'est ajouté à aucun `PointsStats`.
  - Ajouter `stats.on_demand` (somme des fils, plus l'arbre de points) ;
  - ou retirer ce compteur.

  Mesure seulement, aucune décision n'en dépend. Mais il sert à justifier les 16 octets par niveau de la table.

## Notes (vérifié, sans défaut)

**Incidences** (`incidences.cpp`).
- Les boules fortes ont `p + q_min <= K`. Les populations `I_b ∪ U_b` sont prises au nœud de rattachement de
  `WindowAttachment`. À K = 1, la feuille du site est prise au rang 0.
- Comptage puis remplissage, sans tableau par paire. Le tri par ligne est parallèle, à positions fixes.
- Chaque tampon est admis avant son allocation.

**Qualification** (`qualify.cpp`).
- `m <= K` : le rang du nœud.
- `m = K + 1` : les fusions sont qualifiées à leur naissance. Une naissance l'est au m-ième rang de ses paires
  (nœud, site), dédoublonnées au rang minimal. C'est conforme à `points_hierarchy.py:215-246`.
- Le contrôle `qual >= rang de naissance` est présent.
- Les incidences d'un nœud ont un rang dans [naissance, naissance du parent), donc une qualification atteinte l'est
  du vivant du nœud.

**Départs et rival** (`hang.cpp`).
- `t` et `p1` sont pris dans l'ordre des incidences.
- L'élagage par dominance est correct : les rangs sont distincts, donc les niveaux sont strictement croissants. Une
  égalité sur les deux rangs donne la même valeur.
- `two_vs_two(meet, q, M, r)` décide bien `sqrt(l_meet) - sqrt(l_r) > sqrt(l_M) - sqrt(l_Q)`.
- La recherche des `meets` par `lower_bound` est juste : le tri précède l'OU du LCA dans les bits bas.

**Encadrement** (`exact.cpp`).
- `two_vs_two` encadre `2^64 Σ` dans `(P - M - 2, P - M + 2)`. Les racines sont bornées par 2^89 (centre dans
  l'enveloppe du support, `num/roots.hpp:3`), donc les sommes `i128` ne débordent pas.
- Le court-circuit « mêmes rangs, même niveau » est exact, car le niveau est fonction du rang.

**Propriétaire et plancher** (`settle.cpp`).
- `P(low)` est vrai, puisque `t` et la naissance du propriétaire sont tous deux au plus la date.
- La proposition binary64 sert seulement à placer la recherche. Le galop et la dichotomie sont exacts, puis le
  certificat vérifie `P(r)` et `non P(r+1)` sur tout le catalogue.
- Les invariants « propriétaire vivant » et `t <= Q < M` sont contrôlés et refusés en `points_invariant`.

**`AncestorIndex`**.
- Pointeurs de saut de Myers : la longueur d'un saut ne dépend que de la profondeur, donc le LCA est correct.
- `highest` suppose `keep` monotone, ce qui vaut ici (naissances croissantes le long de la remontée).
- 8 octets par nœud, admis avant allocation.

**Arbre de points** (`point_tree.cpp`).
- Au rang r : les fusions, puis les entrées non strictes, puis un plateau par date stricte. Les plateaux sans
  événement sont retirés.
- Le contrôle `block_parent > b` et l'ordre strict des plateaux sont en place.
- Bornes d'admission : `P <= fusions + 2n`, `Bk <= 2n`.
- Construction séquentielle, donc indépendante de W.

**Format et CLI.**
- L'écrivain MHGP11PT suit `SPECIFICATION_FINALE.md` § 6.4 et `SORTIES.md` § 7 : en-tête de 144 octets, LEVELS non
  réduits à W mots, rang 0 référencé, contrôles `reached` à chaque section.
- K = n à K >= 2 est refusé dans `prepare_domain` (`parameter_out_of_range`, étape compute, avant l'index). La porte
  vérifie aussi l'absence de D et de D.pending.
- `plat` reste refusé.

**Oracle.** `points_oracle_stdlib.py` reste indépendant : coupes fermées de `reference/hgp11_ref` (Definition) et
`two_roots_sign` en `Fraction`. Il compare les dates, les propriétaires et les blocs à chaque plateau.

**Couverture minimale.**
- Le plancher `--min-exact 10` passe avec 12 replis : la marge est mince, mais la ligne est gravée.
- Aucun repli exact n'a lieu sur les trames. C'est déjà déclaré par l'implémenteur.

**Coût mémoire.** La table des racines réserve 16 octets par niveau de `Cat_K`, soit environ 81 Mo attendus à K10 sur
ng00, alors que la moitié seulement est remplie. C'est acceptable sous budget, mais à mesurer sur G4.

**Build.** `make -n` dans `b21-s9` ne reconstruit rien : le binaire correspond au commit. Seuls
`points_oracle_stdlib.py` et `tests.cmake` sont plus récents, et ils sont lus à l'exécution.

## Portes rejouées (build de l'implémenteur, `/workspaces/E-HGP/build/v11-persist/b21-s9`, `ctest -j3`)

Toutes sont vertes : 18 sur 18 en 146,9 s réelles. Durées sous charge `-j3` :

| Porte | Résultat | Durée |
| --- | --- | --- |
| `mhgp11_points_unit_{ancestors,determinism,refusals,budget,inventaire}` | vert | 0,08 à 0,19 s |
| `mhgp11_points_fixtures` (+ `_opt`) | vert | — |
| `mhgp11_points_oracle` | vert | 32,2 s (11,5 s déclarées, à vide) |
| `mhgp11_points_oracle_opt` | vert | 28,7 s |
| `mhgp11_cli_points` | vert | — |
| `mhgp11_cli_points_opt` | vert | 5,7 s |
| `mhgp11_cli_contract` (+ `_opt`) | vert | — |
| `mhgp11_core_unit_reasons` | vert | — |
| `mhgp11_style` | vert | — |
| `mhgp11_mutants_points_manifest` | vert | — |
| `mhgp11_points_scale8000` | vert | 29,7 s |
| `mhgp11_points_lidar_ng01_k5` | vert | 56,2 s |

Non rejoués, par méthode légère :
- les portes `long` (`points_vs_python*`, qui demandent numpy) ;
- `scale16000` et `scale32000`, ng00 et ng02 ;
- la campagne de mutants, les sanitizers et `check_docs`.

Les lignes gravées sont vérifiées par la regex `LINE` des portes.
