# Prochain lot borné : rendre A rejouable avant de la paralléliser

27 septembre 2026, complément en lecture seule de `a7e80d7f9`.
Voir [le diagnostic](README.md). Aucun nouveau prototype, benchmark ou GCP.

## Ticket à déléguer après la couture S2

Livrer un manifeste possédé des vrais blocs actifs, puis un rejeu
chronologique indépendant retrouvant exactement A native. Première tranche
limitée aux petits catalogues réels et fixtures, Release/sanitizers.
Pas de MSF natif, nouvelle résolution MEB, modification B/C, encodeur ou
moteur. Ce ticket prouve la complétude de l'entrée du futur constructeur,
pas un gain de temps.

## Réutilisation, source par source

| source relue | réutilisable explicitement | ce qui manque / empêche un port direct |
| --- | --- | --- |
| [`event_model.py`](../b_full_phase_a_work_20260926/event_model.py), `validate`, `reference`, `candidate` | juge chronologique, schéma MSF/ancêtres/prédécesseurs, mutants | entrée abstraite `(rang,cibles,bool contribution)` ; ni BallId/K, masques, populations, niveaux représentés, CSR natifs, verticales ou refus C++ ; Kruskal/enracinement sériels |
| [`countercheck.py`](../b_full_phase_a_work_20260926/countercheck.py) | petits programmes exhaustifs, deux enracinements | aucun manifeste géométrique |
| [`capture_helpers.hpp`](../b_full_continuation_origin_20260927/capture_helpers.hpp), `real_drafts::hook`, `Slot`, `equal` | capture par K du draft/banque au raccord d'encodage ; comparaison champ à champ | trop tard pour A : références déjà remappées, blocs muets/cibles/ancres/racines pré-lot/`birth_ball` absents |
| [`static_grouping_gate.cpp`](../../tests/tower/static_grouping_gate.cpp), `Run`, `build`, `compare` | appel avec `FullBallStaticTrace`, cibles/firsts, témoin hash/tri | `build` efface les forêts après digest ; aucune tranche par bloc, aucun K1 ; digest insuffisant comme seul juge |
| [`gate.cpp` des continuations](../b_full_continuation_origin_20260927/gate.cpp), `fixtures`, `continuations`, `without_continuations` | vrais catalogues réguliers/étendus, ABCZ, carré, permutations, mutant de perte de contribution | pas d'export A ; `grouped_lots>0` ne localise pas la continuation |
| [`p1_sim.py`](../c_phase_a_20260926/toys/p1_sim.py), `Order`, `commit_steps` | second schéma avec `next`, `birth_ball`, `runs`, `anchors`, draft et compteurs | simulation de P1, non massive ; contribution abstraite ; ne pas reprendre sa spéculation pour ce ticket |
| [`phaseA_profile.diff`](../c_phase_a_20260926/phaseA_profile.diff) et [profil C](../c_phase_a_profil_20260926/README.md) | frontières d'instrumentation et ventilation des travaux | aucun export ; `rdtsc` non sérialisant et parts locales ne sont pas un chrono G4 |
| [`d5_probe.cpp`](../c_alternatives_20260923/experiences/d5/d5_probe.cpp), lignes 174–249, 578–589, 667–679 | exemples de programmes, `foff`, `tpos`, plateaux/positions | réplique géométrie et coquilles ; départage égal par BallId non portable à un catalogue public non trié par clé ; `foff` u32 non vérifié, K1 suppose ID=position, tampon 13 racines insuffisant, C ne vérifie que deux parents |
| [`check_phase_a_temporal_max_id_20260923.py`](../check_phase_a_temporal_max_id_20260923.py) | contre-fixtures ouvert/fermé et marques temporelles | scans par niveau, `assert` désactivables sous `-O` ; préférer le modèle du 26 comme porte |

Aucun de ces fichiers n'exporte déjà nativement tous les blocs/cibles/ancres.
Leurs qualifications historiques ne qualifient pas le nouveau manifeste.

## Trois observations natives minimales

Copie audit-only du header par script vérifiant SHA256 et nombre exact de
substitutions ; conserver source copiée, patch et hashes. Ne pas modifier
le moteur ni employer `#define private public` sur son graphe d'includes.
Une seule unité de traduction possède ce Builder : ne pas lier une autre
définition via `libmhgp9_chain.a`. Les helpers de capture donnent le patron
de propriété, pas les nouveaux hooks.

1. Dans `order_lots`, après `order_prepare_lean(o)` réussi, avant la boucle :
   copier `programs[o.k]`, niveaux/`level_run`, `lean_count`,
   `lean_contribution`, `lean_interior`, et `static_targets` pour K≥2 ou
   `lean_k1` pour K1. Copier aussi le domaine K1 ordonné. Si `lean_failed`
   est présent, aucun manifeste complet publié : ce ticket capture les
   constructions réussies, pas une nouvelle sémantique de refus catalogue.
2. Dans `order_block_lean`, après la boucle des représentants et avant
   `sort/unique` : racines brutes par occurrence, avec `(K, position_bloc,
   ordinal_représentant)`. Une mauvaise tranche ne se cache pas dans un
   ensemble de racines dédoublonnées.
3. À la sortie réussie de `order_lots`, après le contrôle de composante
   finale et avant retour : `anchors`, `current.next`, `runs`, `birth_ball`,
   compteurs sémantiques A, `draft.flat`. Ses références sont encore
   `kBallTag|BallId` ; B ne commence qu'après le retour.

Un slot possédé par K, un écrivain, aucune vue conservée ; lectures après
tous les joins. Une exception de capture invalide toute la capture.
Publier allocations/copies d'observation séparément, jamais comme coût
candidat. Un bras pass-through vérifie que l'instrumentation ne change
aucun champ de la tour, pas seulement ses compteurs.

## Fonctions à écrire dans ce seul lot

1. `capture_a_input/output/roots(...)` remplit ces slots sans recopier MEB,
   `ShellTable` ou les décisions de `count_block_at`.
2. `make_manifest(capture)` construit préfixes u64, conversion des cibles
   vers les blocs actifs du même K, domaine K1 distinct, rang zéro réservé,
   `lot_first_ball`, masques/intérieurs explicites. Un tri/join de
   `(K,BallId)` suffit pour cette porte ; compter son coût sans inventer
   déjà la table GPU optimale.
3. `validate_manifest(...)` contrôle CSR complet, unicité des blocs, lots,
   ordinals, domaines et cibles strictement antérieures avant toute lecture
   indirecte ; comparer les tranches aux comptes/ordinals natifs et traces
   de phase 0. Pas de nouveau plafond de recherche.
4. `replay_a(...)`, DSU chronologique indépendante inspirée de `reference`,
   rend draft ball-tagged, ancres, successeurs, naissances, rangs et racines
   par occurrence. Comparer chaque champ au natif ; les vecteurs de
   compression internes ne sont pas une sortie sémantique à égaler.

Un juge Python facultatif projette les petits manifestes vers le modèle
du 26 : sites K1 comme blocs initiaux de rang zéro, cibles décalées.
Ses booléens de contribution ne remplacent jamais le juge C++ des masques,
références et représentations rationnelles exactes.

## Conditions de fin

Paire, triangle, tétraèdre u18, carré, ABCZ et ABCZ doublé, bloc inerte ;
K2/3/5/10 quand le catalogue le permet, deux permutations, static1/static4,
hash/tri. Ajouter les IDs non identitaires de `input(fixture,variant)` dans
[full_ball_tower_gate.cpp](../../tests/tower/full_ball_tower_gate.cpp),
et la coquille à 32 parents des contre-fixtures D5 : pas de tampon de treize.

Mutants : frontière CSR décalée à taille totale conservée ; bloc muet omis ;
site K1 confondu avec rang géométrique ; `lot_first_ball` remplacé par le
premier contributeur ; contribution ABCZ supprimée. Chaque mutant doit
partir d'un manifeste valide, agir effectivement et être tué par le bon
champ, pas un crash. L'ordre physique des slots K ne doit pas changer A.

Succès : mêmes tableaux A et racines pré-lot, cas non réguliers/contacts/
continuations réellement exercés, refus ciblés, Release/sanitizers et
lecteurs normal/−O, sources/binaires/dépendances épinglés avant/après.
Cela permet un **autre ticket** `event_a(manifest)` C++ puis parallèle.
Pas de grande campagne LiDAR ou GCP avant cette porte. La première capture
réelle publiera V/E/G/P/C et mémoire, pas un gain FULL supposé.
