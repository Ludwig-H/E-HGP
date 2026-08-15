# Carte du code `prototype/`

Cadre : `phase=exploration_v3_hors_registre`,
`profile=quantized_u16_input_only`,
`public_status=not_claimed`.

Ce fichier existe pour une raison précise : **tout ce qui compile n'est pas sur
la route.** Les quatre-vingt-quinze sources de ce dossier sont toutes
atteignables depuis le build et toutes couvertes par des CTests, mais une
dizaine d'entre elles mesurent une question déjà tranchée ou implémentent une
idée abandonnée. Elles sont conservées — leurs fixtures et leurs réfutations ont
de la valeur — mais il ne faut pas les lire comme l'état de l'art du chantier.

L'autorité reste [`../audits/AUDIT_ETAT_COURANT.md`](../audits/AUDIT_ETAT_COURANT.md).
Le registre des pistes fermées est [`../audits/archive/README.md`](../audits/archive/README.md).

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

## Idées périmées encore compilées

Ces onze fichiers passent leurs portes et restent au build. Ils ne sont **pas**
une proposition courante. Ne pas les étendre sans rouvrir la piste selon la
règle de réouverture du registre.

| fichier | idée qu'il porte | pourquoi elle est fermée |
| --- | --- | --- |
| `order_k_bfs.hpp` | parcours de l'arrangement relevé par BFS | **Ses trois énoncés fondateurs sont faux hors position simple** — « niveau(voisin) = niveau(courant) ± 1 », germe de niveau zéro par décret sur une face de l'enveloppe, arêtes indexées par les `C(m,3)` triplets de la coquille. `order_k_flats.hpp` le remplace ; il survit uniquement comme **sujet** de `../oracle/oracle_main.cpp`, jamais comme autorité |
| `flats_scale_probe.cpp` | mesurer le volume de l'arrangement relevé pour décider si son parcours est une route | Le volume est quadratique là où la sortie normative est linéaire, théorème de séparation à l'appui. Question tranchée |
| `scale_profile.cpp` | profil à densité fixe du même arrangement | `1 270` sommets d'arrangement par point à `n=800` pour `300` sphères. Le chiffre est acquis ; **seule cible sans aucun CTest** |
| `device_wavefront_job.hpp` | portage GPU du parcours order-k | Le parcours n'étant plus la route, il ne reste que le débit `sm_120` et la parité bit à bit |
| `device_wavefront_kernel.cu` | transport CUDA du même parcours | idem |
| `device_wavefront_qualification.cpp` | différentiel hôte/device du même parcours | idem — les fixtures de refus restent utiles |
| `cell_source_mass_probe.cpp` | préflight de masse à 50 k de la source par cellules de centres | Pentes `> 1,35` sur quatre compteurs ; la source de production est passée à `CKPairTape` |
| `center_cover_mass_probe.cpp` | prune de masse par « huit témoins universels par patch » | Pentes `2,104` puis `1,896`, NO-GO avant G4. Son juge déterminantal q4 borné reste réutilisable |
| `conic_groups.hpp` | énumération de groupes par triples `C(m,3)` et par paire | Remplacée par les crédits cellulaires **sans triples**, l'inclusion conique se décidant par enveloppe convexe projective |
| `conic_groups_probe.cpp` | empaquetage glouton de triples coniques | Mesure faite, voie reprise sans triples |
| `common_core_probe.cpp` | cœur commun de Jung par blocs comme fermeture de masse | **`98,74 %` de cœurs vides** sur `eight_clusters`, et le probe rescanne les `n` points par bloc. Seul le théorème survit |

Un motif les relie presque toutes : **le parcours de l'arrangement relevé**. Six
des onze en sont des mesures, des portages ou des préflights. C'est la piste que
la mesure de volume a fermée, et c'est elle qui a fait basculer la route vers la
décomposition CK/WST.

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
