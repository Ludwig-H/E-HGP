# Tranche S7 : sortie `--sortie=supports`

Rapport du 5 octobre 2026, 13 h 05 UTC (`date -u`). GCP non utilisé.

```text
phase=exploration_v11_hors_registre
backend=cpu_reference
profile=quantized_u21_input_only
public_status=not_claimed
```

## Commit local

- `bd7c8e130` dans le worktree `build/v11-impl-l2` (HEAD détaché, parent `9e7428995` = S6b sur S5), **non poussé**.
- 22 fichiers, +2 053 / −84.

## Ce qui est livré

**API.**
- `OutputKind::supports`, `SupportsRequest` et `request_kind`. `Product` porte soit la tour, soit `OrderTree` avec
  `SupportHierarchy` (`domain()`, `order_tree()`, `hierarchy()`).
- `compute(supports)` suit cet enchaînement : nuage, index, domaine, puis `build_order` au masque 7 035, puis
  `build_support_hierarchy` sur le Pool de la Session.
- `api_detail::order_params()` donne `full_params()` sans les options 128, 1 024 et 8 192. `kOrderMask = 7035` est
  vérifié par un `static_assert` (7 035 = 16 379 − 9 344). La différence est publiée dans SORTIES § 1, l'en-tête
  `api.hpp`, PROVENANCE et le README.
- `src/api/write_supports.cpp` est écrit à neuf. Il produit `MHGP11SP` v1 exactement selon SORTIES § 6 : en-tête de
  136 octets, colonnes alignées sur 8 octets, bourrage nul, ordres canoniques, aucun compte. Les décalages sont calculés
  avant l'écriture et contrôlés à chaque section.
- `supports_manifest` (§ 8) publie les comptes et les agrégats dans l'ordre fixe. Son `tree_k_sha256` est la
  signature V2 sur `OrderTree::forest`.
- `publish` choisit l'écrivain et le manifeste. Pour supports, il ne remplace pas l'étage `output` mesuré par
  `compute`.

**Ligne d'état.** Les étages sont séparés :
- `tree` : `build_order` moins `attach_ns` ;
- `attach` : `attach_ns` ;
- `output` : l'assemblage ;
- `write` : écriture et publication.

Les `counts` de supports sont `nodes`, `balls`, `supports`, `prior`.

**CLI.** `--sortie=supports` est admis ; `points` et `plat` restent refusés `parameter_out_of_range`. La clé `output`
de la ligne de refus vaut désormais la sortie demandée.

**Lecteur.** `bench/mhgp11_formats.py` contient `read_supports`, `SupportsFile`, `closure`, `ball_counts`,
`sphere_of` et `is_positive_support`.
- Il dérive tout ce que SORTIES § 6 déclare dérivé : enfants, postordre, tailles de sous-arbre, rattachement, q_min,
  niveaux et centres exacts, feuilles de K1, cinq comptes par boule et deux par support, `components`.
- Il contrôle :
  - l'en-tête, les décalages, la taille et le bourrage ;
  - l'ordre de Morton strict des sites ;
  - un arbre bien formé (racine en dernier, parent de numéro et de rang plus grands, naissances puis fusions d'au
    moins deux enfants, genres) et la numérotation canonique ;
  - l'appartenance à W_K et la règle m ≤ 24 ;
  - les boules triées par (rang, S* complété par kNone), la première boule propre au rang du nœud ;
  - les rangs cohérents avec les niveaux exacts ;
  - les supports : arité, ordre, S* positif, chaque support positif sur la sphère de S* ;
  - les rôles (lemmes B et C, dont la réunion des branches égale aux enfants), et `strict_traces` = 0 ⇔ naissance.
- `check_directory` recompte les agrégats du manifeste et recalcule `tree_k_sha256` depuis le seul fichier.
- La lecture du manifeste admet la sortie `supports`.

**Portes** (`tests/cli/tests.cmake`) :
- `mhgp11_cli_supports_oracle` ;
- `mhgp11_cli_supports_scale8000`, `_scale16000`, `_scale32000` ;
- `mhgp11_cli_supports_lidar_ng00_k5`, `_ng01_k5`, `_ng02_k5` ;
- chacune avec sa jumelle `_opt`.

**Mesure.** `bench/sorties_g4.py`.

**Documents.** SORTIES (État, § 1, § 3, § 6 « Écrivain et lecteur », § 8, § 11 « Outil »), PROVENANCE (nouvelle
section S7), README, ARCHITECTURE et `cmake/modules.cmake` (`api` dépend maintenant de `supports`).

## Choix

- **Prédicats du lecteur.** Ils sont écrits en entiers homogènes : centre C/D avec D > 0, rayon num/D². C'est
  équivalent aux prédicats en `Fraction`, et 5 à 10 fois plus rapide à l'échelle. `Fraction` ne sert qu'à la
  conversion vers l'oracle.
- **N_j.** Les parties sans support sont comptées sur la réunion des supports (branchement mémoïsé), puis complétées
  par les sites de coquille interchangeables. Le cas régulier a une formule fermée.
- **Ordre des boules.** Le lecteur exige l'ordre (rang, S* complété par kNone), c'est-à-dire l'ordre des `BallIdx`.
  Il ferme ainsi le point du relecteur de S6b : sur 963 ordres et à l'échelle, l'ordre en mémoire égale l'ordre du
  fichier.
- **Contrat de l'exécutable.** Le cas `sortie supports` de `cli_contract.py` devient `mcs avec supports`. Le total
  reste de 64 refus, et la ligne gravée ne change pas.
- **Mutant existant.** `ligne_refus_sortie_toujours_full` (cli.json) a un nouveau motif, `const bool named = ...`, et
  garde le même sens. Le motif de `provenance_non_controlee` (api.json) suit `product.domain()`. Il a été rejoué et il
  est tué.
- **Plancher du manifeste cli** : 20 → 28.
- **Taille de `write_supports`.** Elle est découpée en méthodes de section (`SupportsWriter`) pour la règle des
  100 lignes.

## Portes, lignes exactes et durées locales

Construction Release u21 dans `build/v11-persist/b21-s7`, sur le codespace à 8 cœurs, au plus 4 fils par porte.

| Porte | Ligne | Durée locale |
| --- | --- | ---: |
| `mhgp11_cli_supports_oracle` | `cli_supports_oracle_couverture bits=21 nuages=211 exclus=0 ordres=963 boules=15270 etendues=2645 multiples=576 tetraedres=1145 ordres_6plus=7 signatures=810 refus=4` puis `cli_supports_oracle_ok controles=4736` | 29,9 s |
| `_oracle_opt` | même ligne | 28,4 s |
| `mhgp11_cli_supports_scale8000` | `cli_supports_scale_verdict conforme sites=8000 k=5 noeuds=273655 boules=395667 supports=395667 etendues=0 multiples=0 branches=273654 kparties=1549792 cofaces=230825 appels=12` | 25,8 s (`_opt` : 26,0 s) |
| `_scale16000` | `... sites=16000 k=5 noeuds=565098 boules=819004 supports=819004 etendues=0 multiples=0 branches=565097 kparties=3213739 cofaces=478947 appels=12` | 53,3 s |
| `_scale32000` | `... sites=32000 k=5 noeuds=1163756 boules=1690045 supports=1690045 etendues=0 multiples=0 branches=1163760 kparties=6639350 cofaces=989861 appels=12` | 111,3 s |
| `_lidar_ng00_k5` | `... sites=39885 k=5 noeuds=576371 boules=789886 supports=789889 etendues=141 multiples=3 branches=576483 kparties=3034691 cofaces=449011 appels=12` | 56,5 s |
| `_lidar_ng01_k5` | `... sites=35551 k=5 noeuds=478265 boules=652958 supports=652959 etendues=81 multiples=1 branches=478385 kparties=2502103 cofaces=369857 appels=12` | 45,3 s |
| `_lidar_ng02_k5` | `... sites=45845 k=5 noeuds=609376 boules=832386 supports=832394 etendues=354 multiples=8 branches=610022 kparties=3189636 cofaces=471577 appels=12` | 55,4 s |

Notes sur ces portes :
- Les `_opt` de 16 000, 32 000 et LiDAR n'ont pas été jouées localement. Elles devraient durer autant que leur
  jumelle.
- Les comptes d'échelle et LiDAR égalent ceux de `mhgp11_supports_hierarchy_*` (S6b).
- La ligne u18 de la porte oracle est gravée depuis l'oracle seul : 209 nuages, 2 exclus, 957 ordres,
  15 246 boules, signatures=804. Elle n'a pas été jouée nativement.

Contenu de chaque porte d'échelle et LiDAR, pour le nuage et pour un petit nuage de boite cosphérique :
- lecteur complet sur le premier dossier ;
- fichier et manifeste identiques à l'octet à W1, à W4 et à la répétition ;
- permutation : fichier identique, manifeste identique hors des entrées ;
- réétiquetage non dense, avec 0 et 0xFFFFFFFF : seule la colonne `SITES.point_id` change, et elle est l'image des
  anciens identifiants ;
- `tree_k_sha256` égal à celui de `--sortie=full`.

Contenu de la porte oracle :
- 963 appels `--sortie=supports`, alternés entre W1 dans l'ordre de l'oracle et W4 dans l'ordre inverse : le fichier
  lu égale `canonical(k, ids)` sur tous ses champs ;
- 810 appels `--sortie=full` (K ≤ 4) : même `tree_k_sha256`, lui-même jugé contre la sérialisation V2 du lecteur ;
- sphere9 (25 sites) à K1 et K2, W1 et W4 : code 2, `unsupported_degeneracy`/`support_shell_capacity` à l'étape
  `compute`, ni D ni D.pending, alors que FULL est conforme sur la même entrée ;
- sphere5 (24 sites) admise, avec `max_supports_per_ball` = 828.

**Non-régression.** 39 portes vertes en 68,8 s réelles (`-j3`) :
- `mhgp11_api_*`, dont `session`, `publish_reader`, `fault` et `selftest` ;
- `mhgp11_cli_contract` (`cli_contract_verdict conforme refus64 temoins3`) et `mhgp11_cli_tree_signature`, avec leurs
  jumelles `_opt` ;
- `mhgp11_cli_full_identity` (11 s), `_full_determinism` (30,6 s) et `_full_relabel` (10,3 s).

**Mutants.** Les deux manifestes passent `run_mutants.py --check` :
- `manifeste_ok module=cli mutants=28 plancher=28` ;
- `manifeste_ok module=api mutants=22 plancher=22`.

Neuf mutants ont été joués avec `--only` (7 min 46 s, 2 × 2 cœurs) :
- `mutants_ok module=cli mutants=9 tues=9`, tous tués par code ;
- les mutants : `sp_colonnes_echangees` (BALLS.rank et prior_count), `sp_ordre_supports`, `sp_branches_omises`,
  `sp_signature_v1`, `sp_manifeste_constant`, `sp_signature_fichier`, `sp_sortie_supports_comme_full`,
  `sp_masque_16379` et `ligne_refus_sortie_toujours_full` (motif mis à jour) ;
- tous sont tués par `mhgp11_cli_supports_oracle`, sauf le dernier, tué par `mhgp11_cli_contract`.

Le mutant api `provenance_non_controlee` (motif mis à jour) est tué aussi (2 min).

**Contrôles statiques.**
- `check_style` : `style_ok fichiers=482`.
- `tools/check_docs.py` : 156 lignes, comme la base, et aucune ne porte sur les documents v11 modifiés.

**Aucun octet existant ne change.** `MHGP11FUL1` est inchangé (`full_identity` vert), comme `MHGP11PH`, les sondes et
le module `tower`.

## Essai local de `bench/sorties_g4.py` (aucune conclusion de temps)

- Configuration : W1 et W4, deux prises, trois trames, K5. Durée 5 min 10 s, 36 appels, aucun défaut.
- Sorties identiques entre prises et entre W, `tree_k_sha256` commun.
- Décision `non_evaluee` (W48 absent).
- Document : `impl_s7_sorties_g4_essai_local.json`, à côté de ce rapport.
- À titre indicatif seulement, sur le codespace : l'étage `tree` de supports vaut 56 à 64 % de celui de full à W1 et
  à W4 ; `attach` 82 à 108 ms ; `output` 91 à 299 ms ; `write` 244 à 309 ms contre 1,6 à 2,1 s pour full.

## Écarts

- **Porte d'API absente.** La spécification prévoyait `tests/api/supports_format.py` (`mhgp11_api_supports_format`),
  qui n'est pas écrit. Ce qu'elle devait juger (signature égale à celle de full, arbre de l'ordre K) est jugé au
  niveau du CLI : 810 ordres et toutes les portes d'échelle.
- **Déterminisme et réétiquetage.** Ils sont dans les portes d'échelle (`cli_supports_scale.py`), pas dans des portes
  `_determinism`/`_relabel` séparées. Localement W1/W4 ; W48 à ajouter sur G4 (`--fils=1,4,48`).
- **Pas de porte K10** pour `--sortie=supports`.
- **Entiers plutôt que `Fraction` dans le lecteur**, ce qui est équivalent : voir Choix.
- **Mutant `sp_branches_omises`.** Il remplace les branches par le nœud 0 ; il ne supprime pas la section.

## Ce qui doit tourner sur G4

- **Constructions.** Matrice u18 et u24 : la ligne oracle u18 n'a jamais été jouée nativement. Ajouter ASan/UBSan et
  TSan sur `mhgp11_cli_supports_*`.
- **Échelle et LiDAR.** Les portes, avec leurs `_opt`. Durées locales cumulées : environ 350 s, et le double avec
  `_opt`. Prévoir une configuration à `--fils=1,4,48` pour le déterminisme à W48.
- **Mutants.** Campagne complète `mhgp11_mutants_cli` (28) et `api` (22).
- **Mesure appariée de la règle de L2.**
  - Commande :
    `python3 bench/sorties_g4.py --cli <mhgp11> --fils=1,48 --prises=3 --k10=ng00 --commit=<sha> --out=<reçu>/sorties_g4.json`.
  - Elle compare FULL au masque 16 379 et supports au masque 7 035 ; la décision n'est évaluée qu'à W48 sur ng00,
    ng01 et ng02.
  - Durée estimée sur G4, à partir des 5 min locales à W1/W4 avec deux prises : environ 5 à 8 min, plus K10.
- **Budget de temps** à prévoir dans `tools/g4_matrix.json` pour ces nouvelles portes : oracle environ 1 min avec
  `_opt` ; échelle et LiDAR environ 12 min avec `_opt`, sur 4 fils locaux.
