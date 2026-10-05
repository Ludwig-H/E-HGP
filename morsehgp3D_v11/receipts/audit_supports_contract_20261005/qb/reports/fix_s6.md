# Correction de la tranche S6a après contre-lecture : constats F1, F2 et F3

4 octobre 2026, rapport écrit à partir de 23 h 22 UTC ; toute heure de ce rapport vient de `date -u`. Rôle :
implémenteur de la tranche S6a (module `supports` : $\mathcal{Q}_b$, fermeture et comptes du lemme G), correction
après la contre-lecture indépendante `verif_s6.md`. Le compte rendu d'implémentation d'origine est `impl_s6.md`.

Conditions :

- **GCP non utilisé.**
- Aucun `git add`, commit, stash, branche ni push.
- Écritures limitées à trois endroits :
  - le worktree `build/v11-impl-s6`, détaché à `f98aeed67` ;
  - `/tmp/v11-s6-fix/`, pour les constructions, les essais et la campagne de mutants ;
  - ce rapport, avec ses pièces dans `fix_s6/`.
- Les trames LiDAR sont lues en place (`MHGP11_DATA_DIR=/workspaces/E-HGP/build/v11-full-data-20261002`), jamais
  copiées. Ni ce rapport ni ses pièces ne contiennent de coordonnée.
- `docs/SORTIES.md`, `MATHEMATIQUES.md` et `README.md` ne sont pas touchés, puisque le contrat L0 les écrit. Ce qu'il
  faut y ajouter est au § 6.
- État de départ vérifié : les 17 fichiers livrés avaient les sha256 de l'annexe d'`impl_s6.md`. Le worktree était
  donc bien celui qu'avait contre-lu `verif_s6.md`.

```text
phase=exploration_v11_hors_registre
backend=cpu_reference
profile=quantized_u21_input_only   (ASan+UBSan Debug rejoué sur core;supports)
public_status=not_claimed
```

## 0. En bref

- **Les trois corrections demandées sont appliquées ; aucune n'est refusée.** La contre-lecture n'avait ouvert aucun
  point bloquant.
- **F1.** Le contrat de concurrence du registre est écrit, dans le commentaire de `SupportLedger` et dans celui de
  `ball_supports` : un registre par fil, sommé après la jointure. La somme devient un membre public,
  `SupportLedger::add`, qu'emploie le produit. La porte `concurrency` exerce désormais ce contrat.
- **F2.** La documentation de `ball_shape` cite ses trois refus. La porte `refusals` joue désormais de bout en bout le
  refus `supports_invariant` d'une boule du catalogue hors de $\mathrm{Cat}_K$.
- **F3.** J'ai fait l'inventaire de tous les points de refus du module. Les gardes sans porte possible sont déclarées
  à trois endroits : dans le commentaire de tête d'`enumerate.cpp` et de `counts.cpp`, par une marque « sans porte » à
  chaque garde, et dans la section S6a de `docs/PROVENANCE.md`, avec une table.
  - L'inventaire a trouvé plus de gardes sans porte que le constat n'en citait : la propagation des refus de `num`
    dans `enumerate.cpp`, et trois gardes de `ball_counts`.
  - Il a aussi trouvé deux branches de `ball_counts` **atteignables** par l'API mais jugées par aucune porte. Elles
    le sont maintenant, par la porte `refusals`, et chacune a un mutant tué : le manifeste passe de 6 à 8 mutants.
- **Notes.** N5 et N7 sont corrigées au passage, dans les mêmes paragraphes. Les autres notes vont à S6b, S7, L0 ou
  G4 (§ 4).
- **Rejeu local, sur les sources finales.**
  - Construction Release u21 de tous les modules, sans avertissement.
  - Portes de la tranche : **41 sur 41**, dont les 20 portes d'échelle et `lidar` hors `long`. Toutes les lignes
    gravées sont identiques.
  - Suite `fast` complète : **744 portes conformes sur 747**. Les trois échecs sont étrangers à `supports` : les deux
    échecs connus, `full_paired_protocol` et `_opt`, plus un délai dépassé sous charge par
    `mhgp11_tower_full_campaign`, conforme rejouée seule avec sa jumelle.
  - ASan+UBSan Debug sur `core;supports` : **121 portes `fast` sur 121**, aucun rapport de sanitizer.
  - Campagne de mutants du module : **8 tués sur 8, par code**.
  - `run_mutants.py --check` et `check_style` sont conformes.
- **Aucun octet de sortie existant ne change.** Le produit ne change que par des commentaires et par le déplacement
  de la somme du registre, au calcul identique. Aucun format, aucune raison et aucune sonde de banc n'est touché.

## 1. Constat par constat

### F1 : registre et concurrence (`supports.hpp`) — appliqué

**Constat.** Le contrat de `ball_supports` disait seulement « appels concurrents permis sur des tampons distincts ».
Un `SupportLedger*` partagé entre fils serait une course, car `add()` n'est pas atomique.

**Correction.**

- `src/supports/supports.hpp:49-51`. Le commentaire de `SupportLedger` dit :
  - aucune protection contre la concurrence ;
  - `add` n'est pas atomique, un registre partagé entre fils est une course ;
  - un registre par fil, sommés par `add` après la jointure (porte `concurrency`).
- `supports.hpp:55-64`. La somme de deux registres était une fonction interne d'`enumerate.cpp`, `add`, dans un
  espace de noms anonyme. Elle devient le membre public `SupportLedger::add`, au même corps, `constexpr noexcept`.
  - Raison : « sommé après la jointure » exige une somme. Sans elle, l'appelant (l'assemblage S6b, une sonde) devrait
    recopier huit champs.
  - Ce n'est ni une option ni un crochet : le produit l'emploie (`enumerate.cpp:172` et `:204`).
- `supports.hpp:83-85`. Le contrat de `ball_supports` dit :
  - appels concurrents permis sur des tampons distincts **et des registres distincts** ;
  - le registre facultatif n'est pas protégé ;
  - un registre par fil, sommés par `SupportLedger::add` après la jointure, jamais un registre partagé entre fils.

**Porte.** `mhgp11_supports_unit_concurrency` (`tests/supports/supports_test.cpp:435-477`) passe maintenant un
registre à chaque appel.

- Le registre de la référence séquentielle est gravé : une boule étendue, 36 milieux $\binom{9}{2}$, 84 angles
  $\binom{9}{3}$, 126 quadruplets $\binom{9}{4}$, 18 supports.
- Chacun des 4 fils fait 50 appels avec son propre registre. Après la jointure, chaque registre égale 50 fois la
  référence. Les quatre sont ensuite sommés par `add` : 200 boules et 3 600 supports.
- Plancher : 83 → 90, égal aux contrôles mesurés (`test concurrency controles=90 echecs=0 plancher=90`, en u21 comme
  sous ASan+UBSan).
- TSan sur cette porte reste à jouer sur G4.

### F2 : refus de `ball_shape` — appliqué

**Constat.** La documentation de `ball_shape` ne citait que `parameter_out_of_range`, alors que la fonction rend aussi
les refus de `make_shape`.

**Correction.** `supports.hpp:89-92` liste les trois refus :

- `parameter_out_of_range` : `BallIdx` hors du catalogue ;
- `support_shell_capacity` : $m>24$, par `check_shell` ;
- `supports_invariant` : forme hors du domaine de `Shape`, c'est-à-dire $K$ hors de 1..12, ou boule hors de
  $\mathrm{Cat}_K$ à cet ordre ($p+q_{\min}>K+1$). Les autres clauses de ce domaine sont des invariants du catalogue.

**Porte.** Le refus `support_shell_capacity` de `ball_shape` était déjà joué par `unit_shell_bound`, sur 25 sites
cocycliques. Le refus `supports_invariant` ne l'était que par `make_shape` appelée directement. `unit_refusals` le
joue désormais de bout en bout (`supports_test.cpp:399-407`) :

- sur la ligne 0, 2, 4, de catalogue d'ordre maximal 2, la boule de diamètre $[0,4]$ a $p=1$ ;
- `ball_shape(…, 1)` rend `supports_invariant`, puisque $p+q_{\min}=3>2$ ;
- `ball_shape(…, 2)` réussit.

### F3 : gardes sans porte possible — appliqué, avec un inventaire complet

**Constat.** Les gardes d'invariant du catalogue d'`enumerate.cpp` (anciennes lignes 125, 142-143, 175, 190 et 212)
ne peuvent être atteintes par aucune porte. Les mutants `niveau_non_controle` et `premier_support_non_controle` de la
contre-lecture survivent à toutes les portes et au contre-juge. Le trou devait être écrit, pas découvert.

**Vérification.** Chacune de ces gardes ne se déclenche que si le domaine est corrompu.

- `FullDomain` ne se construit que par les deux `prepare_full_domain` : son constructeur de données est privé et
  leur est réservé, seul le déplacement est public (`src/tower/full_domain.hpp:16-36`).
- La règle 6 d'`ARCHITECTURE.md` interdit tout crochet dans le produit.
- Un mutant qui retire l'une de ces gardes est donc **équivalent** sur tout domaine préparé.
- Ce que ces gardes protègent est jugé en amont, par le juge d'Euler du catalogue : `well_formed` (qmin, m, rang,
  sites, tailles du CSR, $S^*\subseteq U_b$), puis les fautes `degenerate`, `level` et `canonical`
  (`bench/catalogue_euler.hpp:173-198`, `:236-241`, `:273-276` ; portes `mhgp11_catalogue_euler_*`).
- Le sens inverse est jugé, lui : aucune de ces gardes ne se déclenche sur toute $W_5$ des trois trames, sur la
  grille cosphérique ni sur les petits nuages. Les portes `registers` et `judge_small` échoueraient au premier refus.

**Inventaire.** Pour que le trou soit écrit en entier, j'ai classé **tous** les points de refus du module, pas
seulement ceux que cite le constat (lignes finales).

| Site | Refus | Classement |
| --- | --- | --- |
| `enumerate.cpp:76` (`mark`) | plus de `out.size()` supports | porte `unit_refusals` ; mutant `arret_premier_support` |
| `enumerate.cpp:111`, `:120` (`enumerate`) | refus de `orientation` et de `strictly_inside` | **sans porte** : budgets de `num` prouvés à la compilation |
| `enumerate.cpp:141-142` (`site_point`) | `SiteIdx` hors du nuage ; refus de `Point::make` | **sans porte** |
| `enumerate.cpp:159-161` (`canonical_sphere`) | refus de `Sphere::through` ; $S^*$ dégénéré ; rang hors de la table ; niveau différent du catalogue | **sans porte** ; mutant survivant `niveau_non_controle` |
| `enumerate.cpp:167` (`regular_supports`) | `out` vide | porte `unit_refusals` |
| `enumerate.cpp:183` (`extended_supports`) | brouillon trop court | porte `unit_refusals` |
| idem | brouillon nul après `check_shell` ; coquille de taille différente de m | **sans porte** ; le brouillon nul est une défense en profondeur du plafond, qui rend `coquille_sans_plafond` causal |
| `enumerate.cpp:198` (`extended_supports`) | premier support absent ou différent de $S^*$ | **sans porte** ; mutant survivant `premier_support_non_controle` |
| `enumerate.cpp:211` (`check_shell`) | $m>24$ | portes `unit_shell_bound`, `shell_capacity`, `unit_refusals` ; mutant `coquille_sans_plafond` |
| `enumerate.cpp:218` (`ball_supports`) | `BallIdx` hors du catalogue | porte `unit_refusals` |
| `enumerate.cpp:221` (`ball_supports`) | qmin hors de 2..4 ; $m<q_{\min}$ | **sans porte** |
| `counts.cpp:15-17` (`make_shape`) | plafond ; domaine de `Shape` | porte `unit_refusals`, clause par clause |
| `counts.cpp:28` (`ball_shape`) | `BallIdx` hors du catalogue | porte `unit_refusals` |
| `counts.cpp:38` (`ball_counts`) | taille de la fermeture différente de m | porte `unit_refusals` |
| idem | $N_{q_{\min}}=0$ | porte `unit_refusals` (**nouveau**) ; mutant `fermeture_sans_qmin` (**nouveau**) |
| idem | $N_m\neq 1$ | **sans porte** (**ajouté à la liste**) |
| `counts.cpp:42` (`ball_counts`) | $N_j>\binom{m}{j}$ | **sans porte** (**ajouté à la liste**) |
| idem | $N_j\neq 0$ pour $j<q_{\min}$ | porte `unit_refusals` (**nouveau**) ; mutant `fermeture_sous_qmin` (**nouveau**) |
| `counts.cpp:51` (`ball_counts`) | `cofaces` au-delà de $2^{32}-1$ | **sans porte** (**ajouté à la liste**) : Vandermonde |

Les lignes `enumerate.cpp:153`, `:187`, `:191` et `:196` ne font que propager les refus ci-dessus.

**Où c'est écrit.**

- `enumerate.cpp:13-25` : paragraphe « Gardes SANS PORTE POSSIBLE ». Il donne la liste des gardes et leur raison
  (domaine corrompu seulement, constructeur privé, règle 6). Il dit qu'un mutant qui en retire une est équivalent et
  survit par construction, en citant les deux mutants de la contre-lecture, et qu'il n'entre pas au manifeste. Il
  renvoie au juge d'Euler pour ce qu'elles protègent.
- Marques « Sans porte » sur place dans `enumerate.cpp` :
  - `:94` (refus de `num` dans `enumerate`) ;
  - `:138` (`site_point`) ;
  - `:145-147` (`canonical_sphere`) ;
  - `:182` (brouillon nul et taille de coquille ; le brouillon trop court y est noté « porte refusals ») ;
  - `:197` (premier support) ;
  - `:220` (champs qmin et m).
- `counts.cpp:3-9`, avec des marques en `:37` et `:48` : les trois gardes de `ball_counts` qui n'ont pas de porte
  possible, et pourquoi aucune fermeture ne les atteint.
  - La fermeture par défaut ($m=0$) est refusée avant, par sa taille : toute `Shape` a $m\geq 2$.
  - Une fermeture rendue par `ball_supports` a $N_m=1$ et $N_j\leq\binom{m}{j}$, quelle que soit la forme à
    laquelle on l'associe.
  - Il en découle $\mathrm{cofaces}\leq\binom{p+m}{K+1}<2^{32}$, par Vandermonde.
- `supports.hpp:78-82` : le contrat public de `ball_supports` sépare les refus atteignables (brouillon, capacité)
  des gardes sans porte possible.
- `docs/PROVENANCE.md`, section S6a, après la table des ports : un paragraphe « Gardes sans porte possible », une
  table par fichier et fonction, puis la liste des refus qui ont une porte.

**Les deux branches nouvelles.** L'inventaire a trouvé deux refus de `ball_counts` atteignables par l'API, mais
qu'aucune porte ne jugeait isolément. Il suffit d'associer à une forme la fermeture d'une autre boule de même
coquille. `unit_refusals` les joue maintenant (`supports_test.cpp:382-398`).

- **$N_j\neq 0$ sous $q_{\min}$.** On prend la fermeture de la diagonale du carré ($m=4$, $N=(0,0,2,4,1)$) avec
  `make_shape(0, 4, 3, 2)`, de $q_{\min}=3$.
- **$N_{q_{\min}}=0$.** On prend la fermeture du triangle aigu $(0,0,0)$, $(4,0,0)$, $(2,3,0)$, coquille régulière
  avec $N=(0,0,0,1)$, avec `make_shape(0, 3, 2, 2)`.
- Sans ces deux contrôles, chaque fermeture passerait et rendrait des comptes faux. Les mutants `fermeture_sous_qmin`
  et `fermeture_sans_qmin` sont tués par code par `mhgp11_supports_unit_refusals` (§ 3.4).
- Plancher de `refusals` : 24 → 37, égal aux contrôles mesurés.

## 2. Notes de la contre-lecture traitées au passage

- **N5.** Le constructeur par défaut public de `Closure` construit une fermeture vide. Le commentaire de `Closure`
  (`counts.hpp:67-69`) et la ligne `counts` de `PROVENANCE.md` disent maintenant « vide (constructeur par défaut,
  refusée par `ball_counts`) ou construite par `ball_supports` ». `Shape` reste « contrôlée ».
- **N7.** `mark_supports` va des lignes 135 à 165 de `bench/catalogue_euler.hpp`, et non 135 à 164. C'est vérifié par
  `git show f98aeed67:…`, dont le sha256 est inchangé (`f293df6e…`). La correction est faite dans `PROVENANCE.md` et
  dans `src/supports/source_pins.json`. Les autres plages citées sont exactes : l. 108–128, 230–241, 273–276 et 34–42.

## 3. Rejeu

### 3.1 Constructions

| Construction | Dossier | Issue |
| --- | --- | --- |
| Release u21, tous modules (`-DCMAKE_BUILD_TYPE=Release`), `cmake --build -j 2`, de 22 h 51 à 22 h 56 UTC | `/tmp/v11-s6-fix/b21` | `build=0`, aucun avertissement sous `-Werror` ; 805 portes enregistrées |
| idem, passe incrémentale finale | idem | `build=0` ; seuls `enumerate.cpp` et `counts.cpp` recompilés, après une retouche de commentaire de `supports.hpp` faite pendant la construction |
| ASan+UBSan Debug (`-DMHGP11_SANITIZE=ON`, `-fno-sanitize-recover=all`), `-DMHGP11_MODULES="core;supports"`, `-j 2` | `/tmp/v11-s6-fix/basan` | `build=0` ; 144 portes enregistrées |

Toutes les sources étaient figées avant 22 h 52 min 30 s UTC, départ de la campagne de mutants : dernières
modifications de 22 h 46 à 22 h 51 min 33 s UTC. Chaque porte ci-dessous a donc jugé l'état final.

### 3.2 Portes de la tranche, u21

Commande : `MHGP11_DATA_DIR=… ctest -R '^mhgp11_supports|^mhgp11_mutants_supports|^mhgp11_style|^mhgp11_core_unit_reasons' -LE long -j 2`,
de 22 h 57 à 23 h 09 UTC. Résultat : **41 sur 41**, en 727 s. Les portes d'échelle sont `RUN_SERIAL`.

| Porte (et jumelle `_opt` s'il y en a une) | Ligne exacte rendue |
| --- | --- |
| `mhgp11_supports_unit_{square,right_triangle,growth,cube,octahedron,circle,lines,mixed,shell_bound,refusals,constants,concurrency}` | `mhgp11_test_ok tests=1 controles=` 79, 51, 42, 110, 29, 72, 149, 196, 16, **37**, 19, **90** ; plancher égal aux contrôles |
| `mhgp11_supports_unit_inventaire` | `inventaire_ok tests=12` |
| `mhgp11_supports_shell_capacity` (code 2) | `supports_probe_verdict refus support_shell_capacity` |
| `mhgp11_supports_judge_small` | `supports_sample_judge_couverture boules=9067 etendues=4520 multiples=1545 tetraedres=1554 brutes=9066` ; `supports_sample_judge_ok controles=154294` |
| `mhgp11_supports_sample_judge_scale8000` | `supports_sample_judge_couverture boules=200 etendues=0 multiples=0 tetraedres=43 brutes=200` ; `controles=3404` |
| `mhgp11_supports_sample_judge_grid8000` | `supports_sample_judge_couverture boules=300 etendues=149 multiples=49 tetraedres=178 brutes=300` ; `controles=5104` |
| `mhgp11_supports_sample_judge_lidar_ng00_k5` | `supports_sample_judge_couverture boules=400 etendues=200 multiples=3 tetraedres=28 brutes=400` ; `controles=6804` |
| `mhgp11_supports_registers_uniform8000_k5` | `supports_probe_verdict conforme k=5 n=8000 boules=597998 choisies=395667 etendues=0 supports=395667 multiples=0 coquille_max=4 entree=3be1324202d28360` |
| `mhgp11_supports_registers_grid8000_k5` | `supports_probe_verdict conforme k=5 n=8000 boules=468514 choisies=337517 etendues=125064 supports=502847 multiples=55729 coquille_max=14 entree=5a5697d925f68c1e` |
| `mhgp11_supports_registers_uniform16000_k5` | `supports_probe_verdict conforme k=5 n=16000 boules=1233046 choisies=819004 etendues=0 supports=819004 multiples=0 coquille_max=4 entree=3469c29b4c34b7e3` |
| `mhgp11_supports_registers_uniform32000_k5` | `supports_probe_verdict conforme k=5 n=32000 boules=2536732 choisies=1690045 etendues=0 supports=1690045 multiples=0 coquille_max=4 entree=aea3dec129cef911` |
| `mhgp11_supports_registers_lidar_ng00_k5` | `supports_probe_verdict conforme k=5 n=39885 boules=1306696 choisies=789886 etendues=141 supports=789889 multiples=3 coquille_max=5 entree=975c390e5912fabe` |
| `mhgp11_supports_registers_lidar_ng01_k5` | `supports_probe_verdict conforme k=5 n=35551 boules=1095926 choisies=652958 etendues=81 supports=652959 multiples=1 coquille_max=4 entree=6b918ef47e9ae56e` |
| `mhgp11_supports_registers_lidar_ng02_k5` | `supports_probe_verdict conforme k=5 n=45845 boules=1407885 choisies=832386 etendues=354 supports=832394 multiples=8 coquille_max=5 entree=6e11fa8bc5ee6432` |
| `mhgp11_mutants_supports_manifest` | `manifeste_ok module=supports mutants=8 plancher=8` |
| `mhgp11_style` | `style_ok fichiers=430` |
| `mhgp11_core_unit_reasons` | `mhgp11_test_ok tests=1 controles=92` (table des raisons inchangée par cette correction) |

Remarques sur ce tableau :

- Les jumelles `_opt` rendent les mêmes lignes.
- Les sept portes `registers` affichent aussi `supports_sample_judge_ok controles=11` : code 0, six paires de
  registres égales et empreinte de l'entrée.
- Les lignes gravées sont identiques à celles d'`impl_s6.md` et de `verif_s6.md`.
- Les trois juges d'échantillon ont été rejoués en `-V` entre 23 h 22 et 23 h 23 UTC, pour attribuer à chaque porte
  son nombre de contrôles ; ils rendent les mêmes lignes.
- Pièces : `fix_s6/portes_tranche_u21_lignes.txt` (lignes et nombre d'occurrences) et
  `fix_s6/portes_tranche_u21_ctest.log` (résumé de CTest).

### 3.3 Suite `fast` complète, u21

Commande : `MHGP11_DATA_DIR=… ctest -L fast -j 2`, de 23 h 09 à 23 h 18 UTC, en 536 s. Résultat : **747 portes, 744
conformes, 3 en échec**, aucune dans `supports`. La sentinelle `mhgp11_support_lidar_sentinel` est jouée, puisque
`MHGP11_DATA_DIR` est posé, et elle passe. Les 22 portes `fast` du module et du socle touchés passent toutes :
supports, manifeste, style, `core_unit_reasons` et sentinelle.

Les trois échecs :

- `mhgp11_tower_full_paired_protocol` et `_opt` sont les **deux échecs connus**. Ils lèvent `FileNotFoundError` sur
  `morsehgp3D_v11/receipts/qualification_performance_20261003/baseline_source_manifest.json`, car le checkout partiel
  du worktree n'a pas `receipts/`. Ils sont signalés, pas corrigés.
- `mhgp11_tower_full_campaign` a dépassé son délai (`Timeout 120.02 sec`), sous une charge moyenne de 9 à 17 sur 8
  cœurs. C'est le même phénomène que celui qu'avait vu S5 sur la jumelle.
  - Rejouée seule à partir de 23 h 18 min 56 s UTC, sous une charge de 5,7, avec sa jumelle : `Passed 79.45 sec`
    et `Passed 89.80 sec`.
  - Cette porte, du module `tower`, ne lit rien de ce que touche cette correction.

### 3.4 ASan+UBSan Debug, `core;supports`

Commande : `ctest -L fast -j 2`, de 23 h 12 à 23 h 14 UTC. Résultat : **121 sur 121**.

- Le journal complet ne contient ni `runtime error`, ni `AddressSanitizer`, ni `LeakSanitizer`, ni
  `UndefinedBehaviorSanitizer`.
- Les 16 portes `fast` du module passent, ainsi que `mhgp11_core_unit_reasons`.
- Lignes rendues :
  - `test concurrency controles=90 echecs=0 plancher=90` ;
  - `test refusals controles=37 echecs=0 plancher=37` ;
  - `inventaire_ok tests=12` ;
  - `supports_probe_verdict refus support_shell_capacity` ;
  - `supports_sample_judge_couverture boules=9067 etendues=4520 multiples=1545 tetraedres=1554 brutes=9066` ;
  - `supports_sample_judge_ok controles=154294` ;
  - `manifeste_ok module=supports mutants=8 plancher=8` ;
  - `manifeste_ok module=core mutants=78 plancher=78` ;
  - `style_ok fichiers=42` (unités `core` et `supports`).

### 3.5 Campagne de mutants du module

Commande :
`run_mutants.py --manifest tests/mutants/supports.json --source <worktree>/morsehgp3D_v11 --work /tmp/v11-s6-fix/mutants-work --report /tmp/v11-s6-fix/mutants_supports.json --jobs 2 --build-jobs 1 --cmake-arg=-DCMAKE_BUILD_TYPE=Release`.
Elle a tourné de 22 h 52 min 30 s à 23 h 06 min 43 s UTC. Le témoin est vert.

| Mutant | Porte qui le tue | Issue |
| --- | --- | --- |
| `triangle_droit_accepte` | `mhgp11_supports_unit_right_triangle` | tué, par code |
| `drapeau_q4_presentation` | `mhgp11_supports_unit_cube` | tué, par code |
| `arret_premier_support` | `mhgp11_supports_unit_square` | tué, par code |
| `fermeture_omise` | `mhgp11_supports_unit_square` | tué, par code |
| `cofaces_ordre_k` | `mhgp11_supports_unit_square` | tué, par code |
| `coquille_sans_plafond` | `mhgp11_supports_shell_capacity` | tué, par code |
| `fermeture_sans_qmin` (**nouveau**) | `mhgp11_supports_unit_refusals` | tué, par code |
| `fermeture_sous_qmin` (**nouveau**) | `mhgp11_supports_unit_refusals` | tué, par code |

Ligne finale :
`mutants_ok module=supports mutants=8 tues=8 dont_signal=0 dont_delai=0 dont_construction=0 plancher=8`.

Empreintes du compte rendu :

- manifeste : `461c031de0c5f1cbb67e6acb0377915a5484360329b801f589e48cd7292e6ebe` ;
- sources jugées : `ae02dba4bbfd379272e345a209a4d8b50925b599adc52d7314f3fb95dfc425d0`.

Le compte rendu est recopié dans `fix_s6/mutants_supports_report.json`.

Les deux nouveaux mutants retirent chacun une clause de la validation de la fermeture dans `ball_counts`
(`counts.cpp:38` et `:42`). La campagne ne contient pas les deux survivants de la contre-lecture, qui sont équivalents
par construction (§ 1, F3). La table des raisons n'a pas changé : le rejeu de `mhgp11_mutants_core` n'est donc pas dû
à cette correction.

### 3.6 Manifeste et style, à la fin (23 h 21 UTC)

- `python3 -S -B tests/mutants/run_mutants.py --manifest tests/mutants/supports.json --source . --check` rend
  `manifeste_ok module=supports mutants=8 plancher=8`. La même commande sous `-O` rend la même ligne.
- `python3 -S -B tools/check_style.py --root .` rend `style_ok fichiers=430`, sous `-O` aussi. Avec
  `--units supports`, elle rend `style_ok fichiers=8`.
- Aucun octet non ASCII dans `src/supports/`. J'avais d'abord écrit des guillemets français dans un commentaire ; ils
  ont été remplacés par des guillemets droits avant toute construction.
- Le docs checker racine (`tools/check_docs.py`) ne couvre pas `morsehgp3D_v11/`. Sa fonction `validate` a été
  appliquée à `docs/PROVENANCE.md` : elle ne relève que quatre liens morts antérieurs vers `receipts/`, absent du
  checkout partiel. Elle ne relève rien dans la section S6a.

## 4. Notes renvoyées, et pourquoi

| Note | Renvoi | Raison |
| --- | --- | --- |
| N1 : I6 et $S_{\mathrm{journal}}=\binom{m}{t}-N_t$ boule par boule | S6b | demande le journal de S3 et l'assemblage ; à ajouter à `mhgp11_supports_scale*` |
| N2 : portes K10 sans ligne gravée | G4 | les trois portes `registers_lidar_ng0{0,1,2}_k10` (label `long`) se gravent après la session G4 ; d'ici là, le juge exige le code 0 et les six paires de registres égales |
| N3 : `shell_capacity` n'exige pas l'absence de ligne de boule | S7 | une porte à code n'exige qu'une ligne présente ; la porte de transaction du CLI exigera le dossier absent |
| N4 : coquille régulière sans contrôle des `SiteIdx` de $S^*$ | L0 | choix voulu, le catalogue fait foi (G2) ; à écrire dans `SORTIES.md` |
| N6 : ordre final des raisons face à `environment_selftest` | intégration | se fixe dans l'ordre des livraisons ; `status_test.cpp` passera à 28 raisons ; cette correction ne touche pas la table |
| N8 : fichiers différés | S6b | `hierarchy.cpp`, `Ball`, `supports_oracle.py`, `scale.py`, mutants `boules_propres_decroissantes` et `supports_tri_pointid` |

## 5. Ce que je n'affirme pas

- Aucune qualification G4. Les durées citées sont celles des portes sur un Codespace chargé, à une charge moyenne de 5
  à 17 sur 8 cœurs : ce ne sont pas des mesures.
- Ni u24 ni u18 n'ont été rejoués après cette correction.
  - La contre-lecture les avait joués sur la version précédente : 19 portes sur 19 dans chacun.
  - Les changements de cette correction sont des commentaires, une somme déplacée et des contrôles sur de petites
    coordonnées, valides à tout profil.
  - La matrice G4 les rejoue.
- Ni TSan ni Clang n'ont été joués.
- Je n'ai pas rejoué le contre-juge `myjudge.py`, ni le différentiel contre l'oracle L0. Aucune sortie de la sonde n'a
  changé : les lignes gravées sont identiques.
- Les gardes sans porte possible restent sans porte : elles sont déclarées, pas jugées. Leur jugement est celui du
  catalogue, en amont.

## 6. Pour les documents de L0

- **`docs/SORTIES.md`, raisons.** `supports_invariant` recouvre deux familles.
  - Des refus **atteignables** par l'API : brouillon ou `out` trop court ; fermeture étrangère à la forme (taille,
    $N_{q_{\min}}=0$, $N_j\neq 0$ sous $q_{\min}$) ; forme hors du domaine de `Shape`, dont une boule hors de
    $\mathrm{Cat}_K$ par `ball_shape`.
  - Des **gardes d'invariant du catalogue sans porte possible**, dont la liste est dans `PROVENANCE.md`, section
    supports. Un tel refus signale un domaine corrompu, jamais une entrée.
- **`docs/SORTIES.md`, API.**
  - `ball_shape` refuse `parameter_out_of_range` (`BallIdx` hors du catalogue), `support_shell_capacity` ($m>24$) et
    `supports_invariant` ($K$ hors de 1..12, ou boule hors de $\mathrm{Cat}_K$).
  - `ball_supports` rend aussi `parameter_out_of_range` pour un `BallIdx` hors du catalogue.
- **`docs/SORTIES.md`, concurrence.** `SupportLedger` est un registre de mesure, jamais une décision, et il n'est pas
  protégé. On tient un registre par fil, sommé par `SupportLedger::add` après la jointure (porte
  `mhgp11_supports_unit_concurrency`).
- **Mutants du module `supports`.**
  - Le manifeste en a 8 : `fermeture_sans_qmin` et `fermeture_sous_qmin` sont ajoutés, tués par
    `mhgp11_supports_unit_refusals`.
  - Il en aura 10 avec les deux de S6b. La liste du § 8.5 de la spécification est à compléter.
  - Les mutants qui retireraient une garde sans porte possible n'y entrent pas, puisqu'ils sont équivalents.
- **Reprendre les ajouts de `verif_s6.md` § 9.**
  - Les noms C++ : `ball_counts` (qui rend `BallCounts`), `support_cofaces`, `support_gabriel_cofaces`.
  - $N_j$ est rendu comme `Closure` et n'est jamais stocké.
  - `supports_invariant` couvre aussi « plus de `out.size()` supports ».
  - Le différentiel `mhgp11_supports_fraction` doit réordonner les supports : (arité, coordonnées) côté oracle,
    (arité, `SiteIdx`) côté natif.
  - Note N4 : la coquille régulière ne revalide pas les `SiteIdx` de $S^*$, puisque le catalogue fait foi (G2).
- **`MATHEMATIQUES.md` § 10.6–10.7.** Rien à changer.

## 7. Pour la session G4

- **TSan** sur `mhgp11_supports_unit_concurrency`, qui exerce maintenant un registre par fil (4 fils de 50 appels)
  sommé par `add` après la jointure, puis sur la sonde.
- **Campagne `mhgp11_mutants_supports`** (label `long`). Elle compte maintenant 8 mutants, au plancher 8 ; le résultat
  local est de 8 tués sur 8, par code.
- **Matrice.** u21 et u24 Release, u18, ASan+UBSan, poison, et Clang s'il est disponible.
- **Graver les lignes K10** de `mhgp11_supports_registers_lidar_ng0{0,1,2}_k10` (note N2).
- **`sample_judge.py` sous Python 3.10 nu.** Les lignes doivent être identiques à celles du § 3.2.
- **Survivants déclarés.** Ne pas compter `niveau_non_controle` ni `premier_support_non_controle` comme des manques :
  ce sont des mutants équivalents, déclarés comme tels dans le code et dans `PROVENANCE.md`.
- **`mhgp11_mutants_core`** reste à rejouer pour la tranche S6a, qui a ajouté deux raisons. Cette correction n'y
  change rien.

## 8. `git status --short` final

Relevé à 23 h 23 UTC dans `build/v11-impl-s6`. Ce sont les mêmes entrées qu'au départ. Aucun fichier ignoré n'a été
créé et aucun `__pycache__` n'a été laissé.

```text
 M morsehgp3D_v11/cmake/modules.cmake
 M morsehgp3D_v11/docs/ARCHITECTURE.md
 M morsehgp3D_v11/docs/PROVENANCE.md
 M morsehgp3D_v11/src/core/reasons.def
 M morsehgp3D_v11/tests/core/status_test.cpp
?? morsehgp3D_v11/src/supports/
?? morsehgp3D_v11/tests/mutants/supports.json
?? morsehgp3D_v11/tests/supports/
```

## Annexe : empreintes

Chemins relatifs à `morsehgp3D_v11/` dans `build/v11-impl-s6`, relevés à 23 h 23 UTC (rien n'est commité ni indexé).
La colonne « État » compare chaque fichier à l'annexe d'`impl_s6.md`.

| Fichier | État | sha256 |
| --- | --- | --- |
| `cmake/modules.cmake` | inchangé | `c60dd9c3737995f2eced7d2769ad98f6ac6d68a05699c24752ba75bc21554602` |
| `docs/ARCHITECTURE.md` | inchangé | `6212482760bb27495574699930d161b881db21109f50b93de648fbff40d3b108` |
| `docs/PROVENANCE.md` | **modifié** (F1, F3, N5, N7, nouveaux mutants) | `1816019e2ad8e82f2b6d868b3e4062b1c6078203ec4bbd10c8d5c709f9ccbc25` |
| `src/core/reasons.def` | inchangé | `a0d38d86466f797616395500f9071a332b0e8e9291ce2b045c2cbccdd9632c75` |
| `tests/core/status_test.cpp` | inchangé | `ecc22534892f6f7023c4d9e4697559551a2920650cfc4691175a7a95f30ef6be` |
| `src/supports/counts.cpp` | **modifié** (F3) | `23e01a8784e54d0dbe8d60ce3a2ec7ad9f4cae53f8668f39a5f2d4e728e8989a` |
| `src/supports/counts.hpp` | **modifié** (N5) | `c3e91e1de8bc7f72f5dc7b7067e1d72d6e9e82f924dd713cfc5287231092883e` |
| `src/supports/enumerate.cpp` | **modifié** (F1, F3) | `3a8fcd0d7af148e4d7748234c8c02bec5f75158bfef41c5e7c5299177dbe7c86` |
| `src/supports/module.cmake` | inchangé | `95342e4228ddb1f92d2ccdf487f2b4fc2e08c7c7804ffecb6db2e91fdac8535e` |
| `src/supports/source_pins.json` | **modifié** (N7) | `08ac193093184ff7e99b9ef8d8d91c0c41963a39a304b13d705dc871f52bd1b1` |
| `src/supports/supports.hpp` | **modifié** (F1, F2, F3) | `cd0c4dd4aee1868b2eb266e9bcba7340247dc3e2fc36d8706c44c14de358993b` |
| `tests/supports/sample_judge.py` | inchangé | `1c8285c81213411c3f9e0ed28b45c912e447232be5e246043ec5a4141ca5caad` |
| `tests/supports/supports_probe.cpp` | inchangé | `2e1dd632478c568b7383d5a8ced8afb1447a85318b893c93bcfd98ff88fcbbab` |
| `tests/supports/supports_support.hpp` | inchangé | `4863cdf0456418c18be60d3937bd643857652d19134ff3c68b22f4570673cd11` |
| `tests/supports/supports_test.cpp` | **modifié** (F1, F2, F3) | `4f2c84098c25811f3a3457f772f3d1d756d46d8fabe723e22f8c2873d743960f` |
| `tests/supports/tests.cmake` | inchangé | `324cd2d8cb21d376eb72d337f4a582ec2408c4e56ba3da982ab366f33e3e5781` |
| `tests/mutants/supports.json` | **modifié** (2 mutants, plancher 8) | `461c031de0c5f1cbb67e6acb0377915a5484360329b801f589e48cd7292e6ebe` |

Pièces du dossier `fix_s6/` :

| Fichier | Contenu | sha256 |
| --- | --- | --- |
| `mutants_supports_report.json` | compte rendu de la campagne (§ 3.5) | `4a78bc4fcd262e14d48ce4c1514787b922794d9cd4c0a18056c0feff9132bf37` |
| `portes_tranche_u21_lignes.txt` | lignes rendues par les 41 portes de la tranche, avec leur nombre d'occurrences | `3023f98a52646c938f4c26a4407aee60ed864c8c1b4ba9a5e7e7994fca863d31` |
| `portes_tranche_u21_ctest.log` | résumé de CTest des 41 portes de la tranche | `78624c73efffc8d23364c9e849ffa7a1bf8b53e106c02d2e310c43c148255804` |

Les constructions et les journaux restent dans `/tmp/v11-s6-fix/` et peuvent être effacés au redémarrage. On y trouve
`b21`, `basan`, `mutants-work`, `orig/` (copie de l'état contre-lu), ainsi que les journaux `build-*.log`,
`slice-u21.log`, `fast-u21.log`, `fast-asan.log`, `mutants-campaign.log`, `full_campaign_rerun.log` et
`sample_judges_rerun.log`.
