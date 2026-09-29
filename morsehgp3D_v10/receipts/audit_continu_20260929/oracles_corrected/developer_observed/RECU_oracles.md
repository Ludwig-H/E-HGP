# Reçu — clé `oracles` : juge des verticales de la tour et oracle du catalogue

29 septembre 2026. Chantier `morsehgp3D_v10`.

```text
phase=exploration_v10_hors_registre
backend=cpu_reference
profile=quantized_u18_input_only
mode=correction_de_juges
public_status=not_claimed
GCP non utilisé
```

- Base : worktree `build/v9-open-worktree`, HEAD `0bce6cc00` (= origin/main), lu seulement. Copie de travail
  `R = build/v10-fixes/oracles`, dépôt privé `R/src` (commit local `c37778f`, sans rapport avec le dépôt).
- Patch : `R/oracles.patch` (chemins `morsehgp3D_v10/...`). Vérifié : `git apply --check` puis `git apply` sur une
  extraction neuve de `0bce6cc00`, arbre résultant identique à `R/src` (hors `__pycache__`). Pendant le travail,
  d'autres sessions ont avancé le HEAD du worktree à `12aa92110`. Aucun des fichiers touchés ici n'a changé entre
  les deux commits, et `git apply --check` passe aussi sur une extraction de `12aa92110`.
- Fichiers touchés : `tests/oracle/test_tower_oracle.py`, `tests/oracle/test_catalogue_oracle.py`,
  `cli/mhgp10_tower.cpp` (option `--dump-births`), `cli/mhgp10_catalogue.cpp` (option `--dump-levels`),
  `CMakeLists.txt` (15 portes de mutants), `docs/SPEC_V10.md` (table des portes, § 6).
  `reference/hgp10_ref.py` n'est **pas** modifié. Bibliothèque `src/` inchangée.
- Audits lus : `audit_continu_20260929/` (AUDIT_ETAT, pool_head/AUDIT_POOL_TETE_VERTICAL + `probe_vertical.py`,
  catalogue/AUDIT_CATALOGUE_J2_J2C + `check_r1_interrupted.py`, `check.py`, `probe.cpp`) et
  `audit_independant_20260929/` (TOUR_ET_POINTS § 3, `tower_full_check.py`, `tower_probe.cpp`, GEOMETRIE_CATALOGUE
  G2). Les fichiers non suivis de l'auditeur indépendant n'ont pas été touchés.
- Machine partagée : 8 cœurs, charge 22 à 39 pendant tout le travail ; les durées murales sont données sous cette
  charge, les temps CPU à côté. Builds à `--parallel 2`.

## 1. Constats reproduits avant correction

Build non modifié `R/build-avant` (Release). Ses binaires sont identiques octet pour octet à ceux de
`build/v10-wt` : `mhgp10_tower` sha256 `a2077628…22764` (le même que dans `evidence.json` de l'audit),
`mhgp10_catalogue` sha256 `a3bbad50…af0cb`. Porte publiée intacte extraite dans `R/avant/base_src`
(`test_tower_oracle.py` sha256 `4dd097f9…7e48`, celui de l'audit ; `test_catalogue_oracle.py` `f581b1ce…e853`).

### (a) Une image verticale fausse avant l'entrée des points survit au juge publié

```text
python3 audits/audit_continu_20260929/pool_head/probe_vertical.py R/build-avant/mhgp10_tower R/src/morsehgp3D_v10 R/avant/probe_vertical_out
code 0 ; baseline_gate [null, 7] ; mutated_gate [null, 7] ; outcome wrong_vertical_survives_published_gate
```

Même dump que `pool_head/tower.txt` de l'audit (`diff` vide). Sorties : `R/avant/probe_vertical.stdout`,
`R/avant/probe_vertical_out/`.

Élargissement (`R/avant/old_judge_mutants.py` → `old_judge_mutants.json`) : la fonction `check()` de la porte
publiée, dont seul `parse()` est remplacé pour injecter la mutation, s'exécute telle quelle sur le dump du binaire
publié.

| Mutant (seule l'image `lower` change) | Juge publié |
| --- | --- |
| `vertical_audit` : naissance {0,2} (ordre 2, niveau 1) → singleton {5} | accepté |
| `vertical_descendant` : → feuille {0}, descendante de la bonne image vivante | accepté |
| `vertical_later` : → racine d'ordre 1, née à 9/4 > 1 | accepté |
| `vertical_fusion` : fusion d'ordre 2 du triangle équilatéral au niveau 2/3 (aucun point entré) → composante de la paire lointaine | accepté |
| `vertical_missing` : image −1 | rejeté |
| 218 mutants systématiques de 7 fixtures (toute autre image vivante, un descendant, le parent né après) | **200 acceptés**, dont 56 des 67 images vivantes fausses |

### (b) L'oracle du catalogue ne juge ni S* canonique, ni les niveaux, ni le rang dense, ni les poids

`R/avant/old_catalogue_mutants.py` → `old_catalogue_mutants.json`, même méthode (`parse_dump` remplacé) :

| Constat | Résultat sur la porte publiée |
| --- | --- |
| S* remplacé par l'autre diagonale du carré (support valide, non canonique), carré + (7,7,1), K = 3 | accepté |
| tous les rangs + 1 (ordre conservé, pas dense depuis 0) | accepté |
| trou de rang (rangs ≥ 1 augmentés de 1) | accepté |
| drapeau « coquille étendue » inversé | accepté |
| niveau exact | absent du dump : `0 2 0 2 0 \| 0,0,0 2,0,0 \| \| 0,0,0 2,0,0`, 5 champs avant S* |
| entrées pondérées | 0 nuage à doublon dans la campagne publiée (40 nuages, graine 20260928) |
| catalogues pondérés **corrects** (carré pondéré, octaèdre pondéré, K = 2, 3, 5) | faux écarts : « poids de coquille », « en trop 7 » |

Durées de la porte publiée sur `R/build-avant` : tour 9 min 35 s mur, 294 s CPU (73 contrôles, 23 444 coupes) ;
catalogue 13 min 56 s mur, 257 s CPU (161 contrôles, 14 132 boules).

## 2. Correctif

### Sorties nouvelles, seulement sous option (formats par défaut inchangés)

- `mhgp10_tower --dump-births` : chaque ligne `node` d'une naissance reçoit une K-partie témoin `x,y,z …`.
  À K = 1, le site ; sinon I puis les K − p premiers sites de U. Pour une naissance, aucune partie de U de taille
  K − p n'est séparable : chacune contient le centre dans son enveloppe, donc I ∪ A a pour miniboule la boule de
  naissance, quelle que soit A.
- `mhgp10_catalogue --dump-levels` : le niveau exact `num den` de `cat.level[rang]` suit `flags`. Un rang hors de
  la table donnerait `-1 0`, que le juge refuse.

### Juge de la tour (`tests/oracle/test_tower_oracle.py`)

1. **Témoins vérifiés, jamais crus** : pour chaque feuille (naissance), `beta(W) == niveau` en `Fraction` (force
   brute sur les supports, même sémantique que `R.meb`). Arbre : une racine, sans cycle, parent jamais plus bas,
   fusions d'au moins deux enfants, témoin sur chaque naissance et seulement sur elles.
2. **Bijection à chaque niveau d'événement**. Les niveaux balayés sont les niveaux critiques de Γ_k, ceux des nœuds
   d'ordre k et k + 1, et les entrées. À chaque coupe fermée a :
   - r ↦ composante de Γ_k(a) contenant W(feuille(r)) est injective sur les nœuds vivants ;
   - le nombre de nœuds vivants égale le nombre de composantes (surjectivité) ;
   - les enfants d'une fusion tombent dans une même composante (cohérence de toutes les feuilles, par récurrence) ;
   - les attaches C∩X sont jugées par identité (composante de kNN(x)), avec l'entrée égale à D_k(x).
   La coupe ouverte en a est la coupe fermée au niveau d'événement précédent.
3. **Verticales à chaque naissance et fusion u d'ordre k + 1**, niveau a, même si C∩X est vide. Il faut :
   - `lower[u]` existant, né au plus tard en a ;
   - `lower[u]` **vivant** à la coupe fermée a, contrat de `tower.hpp` que `ancestor(…, rank)` réalise dans
     `tower.cpp` ;
   - son identité égale à la composante de Γ_k(a) de W(feuille(u)) privée d'un point.

   Justification : W ⊂ B̄(y, √a) pour y dans la région de u, donc y ∈ L_{k+1}(a) ⊂ L_k(a). La composante de L_k(a)
   qui contient y correspond (théorème 2) à celle de toute k-partie de B̄(y, √a). Les nœuds vivants étant en
   bijection avec les composantes, toute autre image vivante a une autre identité, et toute image non vivante
   viole le contrat : le juge est complet pour une mutation d'une seule image.
4. Contrôles historiques conservés à l'identique : comptes et partitions aux coupes ouvertes et fermées,
   verticales par points. `vertical_check(orders, k, P, levels)` garde la signature utilisée par
   `probe_vertical.py`, mais exige un dump `--dump-births`.
5. Coût : miniboules en cache par nuage (candidats par support), partagé entre K = 1, 3, 5 et entre mutants.
   Autocontrôle : `Balls.beta == R.meb` sur **toutes** les parties d'E5, de l'octaèdre et du cube. La porte passe
   de 294 s à 15 s de CPU.
6. **Mutants systématiques rejoués dans la porte** : 218 sur 7 fixtures (AUDIT3, TRIANGLE, E5, LINE5, carré,
   octaèdre, cube K = 5). Tous doivent être tués, sinon code 3.

### Oracle du catalogue (`tests/oracle/test_catalogue_oracle.py`)

1. **S* canonique** : parmi les parties de U de cardinal q_min qui sont des supports valides (affinement
   indépendantes, centre circonscrit égal au centre, barycentriques > 0), la plus petite dans l'ordre
   lexicographique des clés de Morton (`morton3` de `cloud.cpp` : x au bit 0, y au bit 1, z au bit 2). Elle doit
   être imprimée dans cet ordre, qui est celui des indices de sites.
2. **Niveau exact** publié (`--dump-levels`) égal au rayon carré de la référence (`Fraction`).
3. **Rang** égal au rang dense, depuis 0, des niveaux publiés ; stdout `balls` et `levels` exacts.
4. **Poids et drapeaux** : u pondéré, bit0 ⇔ |U| > q_min, bit1 ⇔ un site de U de poids > 1.
5. **Entrées pondérées** : 13 nuages (graine propre 2026092901, multiplicités 1, 1, 2, 3, 5, doublons mélangés dans
   l'entrée) × K = 1, 2, 3, 5. Attendu : les sphères critiques des *sites* (`R.critical_balls`), p et u pondérés,
   admission de SPEC_V10 § 3 (coquille pondérée : p ≤ K − 1 ; sinon p + q_min ≤ K + 1). Avec des poids 1, c'est
   exactement `R.catalogue`. La suite des 40 nuages publiés et la translation restent inchangées.
6. **Fixtures gravées** de l'audit : extrêmes u18, paire pondérée, carré en z = 262143, octaèdre pondéré, cube aux
   extrêmes, triangle rectangle, plus carré et carré pondéré. Elles tournent à K = 1, 2, 3, 5, 10, 12 et sous
   (feuille, fils) = (défaut, 1), (max(8, K + 1), 4), (256, 1) : 144 appels.
7. **Mutants systématiques rejoués dans la porte** : 571 sur 6 fixtures. Chaque autre support valide, un autre
   niveau, chaque drapeau inversé, u + 1, chaque boule retirée, rangs + 1, un trou à chaque rang. Tous tués.

## 3. Portes ajoutées et renforcées

| Porte | Code attendu | Labels |
| --- | --- | --- |
| `mhgp10_tower_oracle` (renforcée ; planchers ci-dessous) | 0 | gate;oracle |
| `mhgp10_catalogue_oracle` (renforcée ; planchers ci-dessous) | 0 | gate;oracle |
| `mhgp10_tower_oracle_mutant_{vertical_audit, vertical_descendant, vertical_later, vertical_fusion, vertical_missing, witness_level, witness_swap}` | 4 + ligne `mutant_killed NOM` | gate;oracle;fast |
| `mhgp10_catalogue_oracle_mutant_{support_not_canonical, level_value, rank_offset, rank_gap, flag_extended, flag_weighted, weight_shell, weighted_admission}` | 4 + ligne `mutant_killed NOM` | gate;oracle;fast |

Chaque mutant gravé exige d'abord que la fixture non mutée passe (sinon code 1), puis que le mutant soit rejeté.
Un mutant survivant rend 0, et la porte échoue. Le label `fast` est tenu : Python nu, sans assert, vérifié sous
`python3 -O`. Durée : 0,2 à 0,9 s par mutant.

Planchers contre le vert par vacuité. Valeurs observées sur la campagne par défaut, suivies du plancher entre
parenthèses :

- tour : 23 444 coupes (≥ 700 par nuage) ; 12 574 bijections (≥ 400/nuage) ; 3 978 verticales jugées (≥ 120/nuage),
  dont 2 905 sans point entré (≥ 80/nuage) ; 3 271 témoins (≥ 100/nuage) ; 2 252 attaches (≥ 70/nuage) ;
  218 mutants tous tués (≥ 200), dont 200 invisibles pour l'ancien contrôle par points (≥ 150) ;
- catalogue : 18 140 boules jugées (≥ 1 000), niveaux et rangs denses compris ; 619 S* choisis parmi plusieurs
  supports (≥ 100) ; 2 937 coquilles pondérées (≥ 500) ; 4 212 étendues (≥ 500) ; 52 appels pondérés (≥ 16) ;
  571 mutants tous tués (≥ 500).

Les planchers de la porte catalogue tiennent aussi à 12 nuages, valeur que `controles_j2c.sh` lui passe
(code 0 : 6 159 boules, 304 S* à choix, 1 560 pondérées, 1 311 étendues, 16 appels pondérés). La porte tour tient
à 2 et 4 nuages (code 0).

**Échec sur l'ancien build, succès sur le nouveau** :

- les 15 portes de mutants, jouées avec `run_expect.cmake` contre `R/build-avant`, échouent. Les binaires publiés
  refusent l'option de témoin ou de niveau (code 2), et `run_expect` rapporte « code de sortie 1, attendu 4 »
  (`R/avant/gates_neuves_sur_ancien_build.txt`) ;
- la porte publiée **accepte** ces mêmes mutants (§ 1) ;
- sur `R/build`, les 24 portes passent.

Rejeu de la sonde de l'audit contre la nouvelle porte (`R/apres/replay_probe_vertical.py`, mêmes étapes que
`probe_vertical.py`) :

| Porte | Original | Mutant |
| --- | --- | --- |
| publiée (`replay_old.json`) | `[null, 7]` | `[null, 7]`, mutant survivant |
| nouvelle (`replay_new.json`) | `[null, 10]` | « verticale k=2 noeud 0 a=1 : image 2 fausse », mutant tué |

`probe_vertical.py` lui-même, lancé via l'enveloppe `R/apres/tower_with_births.sh` (qui ajoute
`--dump-births`), finit en `RuntimeError: mutation not proved`, code 1.

## 4. Preuves

- `ctest --test-dir R/build -L gate` : **24/24**, deux passes. Première passe 691 s, finale 786 s murales
  (`R/apres/ctest_gate.txt`, `ctest_gate_final.txt`). Tour 57 et 66 s, catalogue 226 et 277 s, sous la limite de
  20 min.
- **ASan + UBSan** (`-DMHGP10_SANITIZE=ON`, Debug, `/tmp/oracles_asan`) :
  - `ctest -R oracle` : 17/17 en 494 s (`R/apres/ctest_asan_oracles.txt`) ;
  - quart LiDAR `lidar01_quarter_x_neg_y_neg`, K = 5 avec `--dump-births` : 258 722 nœuds, 150 171 témoins, code 0 ;
  - même entrée, K = 10 avec `--dump-levels` : 767 555 boules, code 0 ;
  - aucun rapport de sanitizer (`R/apres/asan_quart_lidar.txt`).
- **ThreadSanitizer** : les deux juges passent sur binaires instrumentés (§ 7).
- **Différentiel catalogue** (`build/v10-j2/j2c/differentiel_j2c.sh`, nouveau binaire contre la base d'avant J2) :
  10/10 identiques (§ 7).
- **Dumps par défaut**, octet pour octet, contre les binaires de référence `build/v10-wt`
  (`R/apres/differentiel_dumps.sh`, `differentiel_dumps.txt`, `differentiel_tour_k10.sh`) :
  - catalogue K = 1, 5, 10 et tour K = 1, 5 sur un quart LiDAR (8 074 points), une scène à coquilles
    (16 000 points) et la trame `lidar02_full` (45 845 points) : 15/15 identiques ;
  - « dump avec option, champs ajoutés retirés » est identique au dump par défaut ;
  - tour K = 10 avec attaches : identique (§ 7).
- **Différentiel tête** (`head_diff.py`, 18 cas) : 0 écart (§ 7). `mhgp10_cluster` n'est pas touché ; le binaire
  reconstruit est identique octet pour octet à la référence (sha256 `e00e2431…ef70`).

## 5. Sorties sur entrées valides

**Aucune ne change.** La bibliothèque est inchangée ; les dumps par défaut du catalogue et de la tour, ainsi que
les sorties de `mhgp10_cluster`, sont identiques octet pour octet (§ 4). Les seules sorties nouvelles
(témoins, niveaux) n'existent que sous `--dump-births` et `--dump-levels`.

## 6. Limites

- Les témoins viennent de la tour. Le juge ne les croit pas : il vérifie `beta` exact, la cohérence et la
  bijection. Un échange de témoins entre naissances symétriques qui préserve toutes les relations reste
  indiscernable. Il est aussi sans conséquence : c'est un réétiquetage cohérent des nœuds, pas une erreur d'objet.
- Petits nuages seulement (tour : n ≤ 12, K ≤ 5 ; catalogue : au plus 22 sites, K ≤ 12). Ce sont des oracles de
  correction, pas des mesures d'échelle ; aucune conclusion de coût n'en est tirée. À l'échelle, seuls le
  différentiel octet pour octet et le juge d'échantillon des témoins (§ 7) ont été joués.
- La porte tour juge l'entrée `core` (comme avant). L'entrée `cover` reste couverte par `mhgp10_points_cover`.
- Le catalogue pondéré est jugé au contrat actuel de SPEC_V10 § 3 (p ≤ K − 1), pas à la règle plus serrée de
  GEN_v2 (constat G2 de l'audit indépendant, hors de ce correctif). La tour refuse toujours les multiplicités.
- `mhgp10_tower --no-points --dump=…` plante dans l'export des attaches (`cli/mhgp10_tower.cpp:213`, UBSan
  « reference binding to null pointer »), avec ou sans `--dump-births`, comme le binaire publié. C'est le constat
  P1 « Export » de l'audit continu, confié à un autre correctif et non modifié ici. Le premier passage du
  différentiel K = 10 l'a rencontré ; le complément K = 10 est donc fait avec attaches.
- `reference/test_ref.py` n'a pas été rejoué : `hgp10_ref.py` n'est pas modifié.

## 7. Différentiels, TSan et juge d'échantillon

- **ThreadSanitizer** (`-DMHGP10_TSAN=ON`, Release, `/tmp/oracles_tsan`, binaires instrumentés) :
  - `setarch $(uname -m) -R python3 test_tower_oracle.py /tmp/oracles_tsan` : code 0, mêmes compteurs que le build
    Release ;
  - `setarch … test_catalogue_oracle.py /tmp/oracles_tsan 12` : code 0 ;
  - sorties : `R/apres/tsan_*_oracle.txt`. Le code C++ modifié est une écriture séquentielle du dump après le calcul,
    hors du pool.
- **Différentiel catalogue J2c** (`differentiel_j2c.sh R/build/mhgp10_catalogue build/v10-j2/build-base/mhgp10_catalogue`) :
  - 5 entrées × K = 5 et 10, à 1 et 4 fils : **10/10 IDENTIQUES** ;
  - compteurs du catalogue égaux à la référence, arbre égal entre 1 et 4 fils, identité binaire ok ;
  - `FIN echec=0`, 2 152 s (`R/apres/differentiel_catalogue_j2c.txt`).
- **Dumps par défaut** contre `build/v10-wt` (`R/apres/differentiel_dumps.txt`) : **15/15 IDENTIQUES** (catalogue
  et tour), options retirées comprises. Le seizième cas, tour K = 10 en `--no-points --dump`, plante (SIGSEGV)
  **sur le binaire de référence comme sur le binaire modifié**, voir § 6. Complément avec attaches
  (`differentiel_tour_k10.txt`) : 1 144 078 lignes, 633 907 témoins, **IDENTIQUES** (`28c9b784c7f62d70`).
- **Tête** (`head_diff.py build/v10-wt/mhgp10_cluster R/build/mhgp10_cluster /tmp/oracles_headdiff`) :
  **ECARTS 0 sur 18**, 436 s (`R/apres/head_diff.txt`).
- **Juge d'échantillon à l'échelle** (`R/apres/echantillon_temoins.py`) sur `--dump-births` du quart LiDAR à
  K = 10 : 150 naissances tirées par ordre, graine 2026092902. Résultat : **1 500 témoins**, `beta(W)` égal au niveau
  publié en `Fraction`, **0 écart** (`R/apres/echantillon_temoins.txt`). Il n'y a eu aucun juge exhaustif à cette
  taille.

## 8. Empreintes

| Objet | sha256 |
| --- | --- |
| `R/oracles.patch` | `73f569ce30ae875df744214644c09fd61bbd6b17a754be937db9c36048141c4c` |
| `tests/oracle/test_tower_oracle.py` (nouveau) | `ad615bce4ea97dd94f22b2359ffafbf97ea6d844c899104f2b432798f53b1c38` |
| `tests/oracle/test_catalogue_oracle.py` (nouveau) | `b814428d4c63133441b8e21f921f23fd28bcd7b4d35421ea6050e9e37a9689a3` |
| `R/build/mhgp10_tower` (nouveau) | `ad7e119ffd6d41a08acc658d2f8d2397bf7dc8afe36d3249a635a7926dcdf876` |
| `R/build/mhgp10_catalogue` (nouveau) | `15cd0c22025c08327473195cd413e4d1f1746ef218095bced52700b630aca228` |
| `R/build/mhgp10_cluster` (= `build/v10-wt`) | `e00e2431f8c9daf7e90a429cb905f9c559d5a4699a376497aaf2e766f70def70` |
| `R/build-avant/mhgp10_tower` (= `build/v10-wt`, = audit) | `a20776280ef4960d68f1b65ca7007b9e8a862809c57115b58486942856c22764` |
| `R/build-avant/mhgp10_catalogue` (= `build/v10-wt`) | `a3bbad50b81b901c57cea7193bc48d58cce68126e44ae0a225ad678baa3af0cb` |
| porte tour publiée | `4dd097f9bdb861ade442af9f1f882093383c0d326fd17fd003bcb850787b7e48` |
| porte catalogue publiée | `f581b1ce6ee207589aaac9c275081d56b3d0cb22982e7fd894a0a1cf366fe853` |
| `R/avant/old_judge_mutants.py` | `64b26b75d731cf033b1547d5ba107ff45b4506caa283bbc47907fe1d53b22df1` |
| `R/avant/old_catalogue_mutants.py` | `eb9f3fdb466bd6ba5c0842bf2b4ab7fe35009321d8acb649b4a34ac3cb29ca2a` |
| `R/apres/replay_probe_vertical.py` | `fad1d2bd25f0d18bdd63f1e7f164a0c9a3e173bd592ee4c3ec495478a1859efc` |
| `R/apres/echantillon_temoins.py` | `d38a142ace2838973ed70f60f276c029f2bd77edf1dba7c75f3a618b2ecb9277` |
