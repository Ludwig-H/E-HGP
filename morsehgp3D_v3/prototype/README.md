# Carte du code `prototype/`

Cadre : `phase=exploration_v3_hors_registre`,
`profile=quantized_u16_input_only`,
`public_status=not_claimed`.

Ce fichier existe pour une raison précise : **tout ce qui compile n'est pas sur
la route.** Les sources de ce dossier sont toutes atteignables depuis le build
et couvertes par des CTests ; cela ne dit rien de leur actualité.

Vingt-trois sources portant une idée fermée ont été supprimées le 15 août 2026,
avec leurs cibles et leurs cent quatre-vingt-douze CTests :

- **le parcours de l'arrangement relevé** — son portage GPU (`device_wavefront_*`)
  et les deux profils de volume (`scale_profile`, `flats_scale_probe`) qui l'ont
  précisément fermé ;
- **les cellules de centres** — `centre_cell_source.cpp`, `cell_credits`,
  `cell_prune`, `directional_dominance`, leur juge et leurs deux scripts,
  supersédés par `CKPairTape` ;
- **le préfixe fini de voisins** comme autorité exhaustive (`prefix_index_gate`,
  `prefix_mass_probe`), interdit par le registre de la racine ;
- **la génération locale certifiée par cône** (`certified_locality_probe`), faux
  vert de saturation ;
- **la source directe par arrangement** (`direct_source.cpp`), dont l'en-tête
  disait déjà « la route passe ailleurs » ;
- les deux probes de groupes coniques et le cœur commun de Jung.

Leur substance est dans [`../audits/PISTES_FERMEES.md`](../audits/PISTES_FERMEES.md) ;
le reste est dans l'historique Git.

L'autorité reste [`../audits/AUDIT_ETAT_COURANT.md`](../audits/AUDIT_ETAT_COURANT.md).
Le mémo des pistes fermées est [`../audits/PISTES_FERMEES.md`](../audits/PISTES_FERMEES.md).

## La route courante, en un paragraphe

`CKPairTape -> Lane2 / Lane3 / Lane4`, **trois producteurs autonomes**. Une
partition Callahan–Kosaraju écrit `binom(X,2)` en rectangles disjoints depuis
l'index Morton ; chaque lane possède ensuite son propre fuseau d'ancre
(`W2 : H>0` ; `W3 : H>0, 3H^2>Xi` ; `W4 : H>0, 2H^2>Xi`), son propre census et
sa propre preuve de complétude. Aucune lane ne lit la sortie d'une autre : le
rang n'est pas héréditaire, et une fixture de 64 points porte un q4 de rang `4`
dont les six arêtes et les quatre faces ont rang `12`.

Contrat et preuves : [`../audits/NOTE_SOLUTION_CONTRAT_SOURCE_AIGUE_20260814.md`](../audits/NOTE_SOLUTION_CONTRAT_SOURCE_AIGUE_20260814.md).

## Les chantiers

| chantier | fichiers pivots | rôle |
| --- | --- | --- |
| **source par ancre** | `anchor_source.cpp`, `anchor_pipeline.hpp`, `anchor_envelope.hpp` | producteur complet, quadratique par construction — **vérité terrain** à petite taille, jamais la route de production |
| **WSPD / WST** | `wspd_front.hpp`, `wspd_wavefront.hpp`, `wst3_probe.cpp`, `rect_front.hpp` | partition des paires, extension ternaire par la lentille, produit quaternaire |
| **noyau q4** | `q4seed_axis_topr4.hpp`, `q4seed_axis_topr4_probe.cpp`, `lane_grid.hpp` | `Q4SeedAxisTopR4` : sélection best-first axiale, le chantier le plus récent |
| **certificats de bloc** | `midball_block.hpp`, `corner8_ball.hpp`, `block_jung_dual.hpp`, `soc64_rect.hpp`, `spindle_cone.hpp` | prédicats `ALL` exacts ou suffisants ; deux sont exacts, aucun ne décide `NONE` |
| **device** | `axis_device_*`, `anchor_source_kernel.cu`, `faceowner_device_*` | mêmes fonctions compilées pour deux cibles ; la parité est une propriété de construction, pas un accord à espérer |
| **dimensionnement** | `lane_source_scale_probe.cpp`, `caps_admissible_probe.cpp` | mesure de l'objet (J0) |
| **oracles et juges** | `../oracle/*`, `q4_brute_oracle.cpp`, `anchored_catalogue.hpp` | arithmétique volontairement différente de la production |

## Ce qui reste, et qui porte encore une idée fermée

Deux fichiers, gardés pour une raison chacun.

| fichier | idée qu'il porte | pourquoi il reste |
| --- | --- | --- |
| `order_k_bfs.hpp` | parcours de l'arrangement relevé par BFS, dont **les trois énoncés fondateurs sont faux hors position simple** | il reste le **sujet** de `../oracle/oracle_main.cpp`, qui en tire `OrderKStatistics` et `order_k_catalogue`. Le supprimer casse le juge |
| `center_cover_mass_probe.cpp` | prune de masse par « huit témoins universels par patch » : pentes `2,104` puis `1,896`, NO-GO avant G4 | le binaire héberge toute la suite `p1a_*`, étrangère à la piste fermée |

Trois autres restent alors que leur piste figure au registre d'abandon de la
racine, parce que le registre autorise explicitement d'en conserver « un oracle
borné » : la tour de boules saturées (`saturated_fold*`, `hybrid_fold_validated`),
`order_k_flats.hpp` — qui alimente le juge `gamma_forest_judge` reconstruisant
`Gamma_k` depuis la définition du manuscrit — et `prefix_index.hpp`, dont
`saturated_fold_hybrid.hpp` a encore besoin. Aucun n'est un chemin de
production ; tous sont bornés et hors du produit.

## Réfutations gravées dans le code

Trois fichiers portent une réfutation qu'il ne faut surtout pas « nettoyer » :

- `anchored_catalogue.hpp` — le certificat « si `2 r_max` des supports déjà
  trouvés n'atteint pas le premier voisin exclu, aucun support n'a pu être
  manqué » est **faux**. Contre-exemple reproduit : 22 points, graine `4242`,
  `69` sphères au lieu de `70`. Il alimente `mhgp3v_false_certificate_regression`.
- `gabriel_degree_gate.cpp` — **aucune borne de degré Gabriel** ne découle de la
  dimension ni de `smax`. Fixture u16 permanente : étoile à **treize** feuilles
  de rang fermé exactement `2`. Tout cap de degré 12 est réfuté.
- `anchor_pipeline.hpp` — le filtre `theta` de l'enveloppe mobile est conservé
  **désarmé**, avec un compteur qui démontre qu'il est identiquement nul sur une
  ancre vivante, doublé d'un plancher de non-vacuité. C'est l'autorité de la
  réfutation, pas un reliquat.
