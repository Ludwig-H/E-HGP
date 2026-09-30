# Reçu `faits_math` — faits mathématiques de la projection des points (29 septembre 2026)

```text
phase=exploration_v10_hors_registre
backend=cpu_reference
profile=quantized_u18_input_only
mode=audit_independant_math_and_architecture (correctifs d'audit, groupe faits_math)
public_status=not_claimed
GCP non utilisé
```

- Base : `0bce6cc00` (= origin/main), lue dans `build/v9-open-worktree`, jamais modifié.
- Copie de travail : `build/v10-fixes/faits_math/src`, dépôt privé de la copie, commit `base`. Elle contient `morsehgp3D_v10/`,
  `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md` et `tools/check_docs.py`.
- Patch : `build/v10-fixes/faits_math/faits_math.patch` (sha256 `af87eef1bcbf79f81ebc99caff15fa214041e3addcb19d84acec470673781dc7`),
  chemins racine, applicable par `git apply` à la racine du dépôt. Il est vérifié sur un export complet de `0bce6cc00`
  (`/tmp/faits_math_full`, `check_docs` vert). `git apply --check` passe aussi sur `12aa92110`, le HEAD que d'autres
  sessions ont entre-temps donné au worktree. Aucun des quatre fichiers touchés, ni le moteur, ni les CLI, ni les tests
  n'ont changé entre ces deux commits.

## 1. Constats reproduits avant correction (binaires non modifiés)

Binaires de référence `build/v10-wt` (code identique à `0bce6cc00`) :

| binaire | sha256 |
| --- | --- |
| `mhgp10_catalogue` | `a3bbad50b81b901c57cea7193bc48d58cce68126e44ae0a225ad678baa3af0cb` |
| `mhgp10_tower` | `a20776280ef4960d68f1b65ca7007b9e8a862809c57115b58486942856c22764` |
| `mhgp10_cluster` | `e00e2431f8c9daf7e90a429cb905f9c559d5a4699a376497aaf2e766f70def70` |

Le hash de `mhgp10_tower` est celui que citent les audits (`cover_discontinuity.json`, `crossing_report.json`) : c'est le
binaire audité.

La commande est `avant/reproduire.sh build/v10-wt avant/sorties`. Elle écrit 37 commandes dans `avant/sorties/commandes.log`,
toutes au **code 0**, et conserve tous les dumps, arbres et sorties standard. Le résumé lu directement dans les dumps
est `avant/constats_avant.txt`. Niveaux en rayon carré, rayons entre parenthèses.

- **F1, recouvrement des couvertures** (K = 2, sites 0, 2, 4).
  - Catalogue : au rang 0 (le plus bas niveau), deux boules de supports {0, 2} et {2, 4}, de populations {0, 2} et {2, 4}.
    Au rang 1, la boule {0, 4}, d'intérieur {2}.
  - Tour, ordre 2 : deux naissances au niveau 1 et leur fusion au niveau 4.
  - Entrée `cover` : les trois points entrent au rang `r1`, niveau 1 ; 0 et 2 dans le nœud 0, 4 dans le nœud 1. Coupe à
    a = 1 : `[[0, 2], [4]]`.
  - Entrée `core` : les trois points entrent au niveau 4 ; coupe à a = 1 vide.
- **F2, discontinuité de `cover`** (K = 2), fusion gauche–milieu :
  - `cover`, L = 1 000 : 998001/4 (499,5), puis 1 000 000 (1 000) ;
  - `core`, L = 1 000 : 998 001 (999), puis 1 002 001 (1 001) ;
  - `cover`, L = 100 000 : 9999800001/4 (49 999,5), puis 10¹⁰ (100 000) ;
  - `core`, L = 100 000 : 99 999², puis 100 001².
- **F3, croisement multi-K** (entrée `core`) :
  - sur 0, 20, 22, 50, 52 : (K = 1, a = 100) donne `[[0, 20, 22], [50, 52]]` et (K = 2, a = 225) donne
    `[[20, 22, 50, 52]]` ;
  - sur 0, 1, 4, 7 (témoin de l'audit indépendant) : (K = 1, a = 1/4) donne `[[0, 1], [4], [7]]` et (K = 4, a = 36)
    donne `[[1, 4]]`.
- **F4, égalité de la borne core** (K = 2) :
  - 1, 2 puis 0, 3 : fusion 1 puis 9 (rayons 1 puis 3) ;
  - 1, 1001 puis 0, 1002 : 1 000² puis 1 002² ;
  - 7, 1007 puis 0, 1014 : 1 000² puis 1 014².

## 2. Relecture de la preuve de stabilité (AUDIT_LAMINARITE § 2.2)

Énoncé : deux nuages étiquetés de même effectif, avec $\lvert x_i-y_i\rvert\leq\varepsilon$ pour tout i. Alors, pour tout K,
$\lvert u_K^{X}(i,j)-u_K^{Y}(i,j)\rvert\leq2\varepsilon$ pour i ≠ j, en rayon.

1. Inclusion. Si y a K témoins $x_l$ dans $\bar{B}(y,r)$, les $y_l$ sont dans $\bar{B}(y,r+\varepsilon)$. Donc
   $L_K^{X}(r)\subseteq L_K^{Y}(r+\varepsilon)$. Les témoins sont comptés par identifiant, ce qui reste juste avec des
   multiplicités.
2. Chemin. Si $x_i$ et $x_j$ sont dans une même composante de $L_K^{X}(r)$, ils appartiennent à $L_K^{X}(r)$. La
   composante étant une réunion finie de convexes fermés, ils sont reliés par un chemin, qui reste dans
   $L_K^{Y}(r+\varepsilon)$.
3. Segments. $x_i$ a K témoins $x_l$ au rayon r. Pour z dans $[x_i,y_i]$,
   $\lvert z-y_l\rvert\leq\lvert z-x_i\rvert+\lvert x_i-x_l\rvert+\lvert x_l-y_l\rvert\leq\varepsilon+r+\varepsilon$. Le
   témoin $x_i$ lui-même est inclus. Donc $[x_i,y_i]\subseteq L_K^{Y}(r+2\varepsilon)$, et de même pour j.
4. Conclusion. Par monotonie en r, $y_i$ et $y_j$ sont reliés dans $L_K^{Y}(r+2\varepsilon)$. L'infimum sur r donne
   $u^{Y}\leq u^{X}+2\varepsilon$ ; la symétrie donne la valeur absolue.
5. Entrées. Chaque $\lvert y_i-y_l\rvert$ diffère de $\lvert x_i-x_l\rvert$ d'au plus 2ε, donc la K-ième statistique
   d'ordre aussi.
6. Optimalité. Avec 0, 1 puis −ε, 1 + ε et K = 2, fusion et entrées passent de 1 à 1 + 2ε.

Verdict : la preuve est **correcte**. Elle est inscrite `proved_here`, avec sa portée : rayon seulement, toute dimension,
coupes fermées ; ni ajout ou retrait de points, ni changement de K, ni EOM. Le cas d'égalité est gravé en F4.

## 3. Correctif

Aucun changement du moteur. Quatre fichiers :

| fichier | changement |
| --- | --- |
| `morsehgp3D_v10/tests/regression/test_projection_facts.py` | nouvelle porte (Python nu, sans assert) |
| `morsehgp3D_v10/CMakeLists.txt` | enregistrement `mhgp10_regression_projection_facts`, bloc `if(Python3_FOUND)`, après `mhgp10_points_cover` |
| `morsehgp3D_v10/docs/conception/CLUSTER_v2.md` | second amendement du § 3.2 : `cover` est une laminarisation particulière des amas discrets |
| `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md` | section « V10 — projection des points depuis la tour FULL », après V9-S4 : sept lignes |

Les sept lignes du registre :

| énoncé | statut |
| --- | --- |
| hiérarchie core laminaire à K fixé, $u_K$ ultramétrique | `proved_here` |
| tranche monotone (r croissant, K décroissant) | `proved_here` |
| FULL multi-K = arbre unique sur les points (F3) | `false_in_general` |
| stabilité 2ε de core, constante atteinte (F4) | `proved_here` |
| `cover` laminaire | `proved_here` |
| `cover` = amas discrets (F1) | `false_in_general` |
| `cover` hérite de la stabilité de core (F2) | `false_in_general` |

## 4. Porte ajoutée : `mhgp10_regression_projection_facts`

- Enregistrement : `cmake -DCMD=python3 -DARGS="-O test_projection_facts.py BUILD" -DEXPECTED=0`, plus
  `EXPECT_LINE=projection_facts_ok faits=4/4 mutants_tues=5/5 equivalents_acceptes=1/1`. Le code exact et la ligne sont
  lus sur la même exécution, sous `python3 -O`.
- Labels `gate;regression;fast`. `fast` s'ajoute à la consigne `gate;regression` parce que la porte est en Python nu,
  prend moins d'une seconde et doit aussi tourner sur la VM G4.
- Codes : 0 conforme ; 1 fait contredit, sortie illisible, binaire en échec, mutant survivant ou équivalent rejeté ;
  2 binaire absent ; 3 plancher (133 contrôles, compte exact) non atteint. Le code 2 est exercé sans argument et sur un
  build absent. Le code 3 l'a été une fois : avec un plancher mal réglé à 150, le premier essai a rendu
  « PLANCHER 135 contrôles < 150 ».
- Sources lues : 37 exécutions natives par passage, `--threads=1`. `mhgp10_tower --dump` donne les niveaux rationnels
  exacts, `mhgp10_catalogue --dump` les supports et populations. `mhgp10_cluster --tree` donne la hiérarchie de points de
  la tête : ses doubles sont comparés exactement aux rationnels de la tour.
- Valeurs attendues : elles ne sont pas recopiées de sorties antérieures. Un oracle rationnel indépendant pour sites
  alignés (classe `Line`) les recalcule depuis les coordonnées. La trace de $L_K(r)$ sur l'axe est la réunion des
  intervalles des fenêtres de K sites consécutifs, ce qui donne entrées core et cover, hauteurs de fusion, partitions,
  couvertures discrètes et ensemble des partitions cover admissibles sous ex æquo. Les énoncés des fixtures (centres 1 et
  3, blocs croisés, saut (L + δ)/2, écart 2e) sont gravés comme affirmations.
- Mutants des sorties lues, tous tués, chacun par les contrôles propres à son fait (jamais par une exception) :

  | mutant | fait qui le tue |
  | --- | --- |
  | `cover_lu_comme_core` | F1, F2 |
  | `dates_d_entree_oubliees` | F4 |
  | `niveaux_lus_comme_rayons` | F4 |
  | `ordres_confondus` | F3 |
  | `point_conteste_differe` | F1 (ancrage différé du point contesté) |

- Doctrine « invoquer, ne pas re-parcourir » : une version intermédiaire contrôlait aussi la borne 2ε sur les trois
  paires core de F2, qui sont des cas intérieurs du théorème. Ce contrôle, redondant avec l'accord exact à l'oracle, a
  été retiré. Le théorème n'est gravé que par son cas d'égalité (F4) et par le contraste de F2.
- Mutant équivalent accepté : `autre_proprietaire`, où le point 2 est donné à l'autre composante. La porte ne recopie pas
  le départage du moteur.

## 5. Preuves

- **Porte sur les binaires de référence** (`build/v10-wt`) : code 0, en mode normal et sous `-O`. Les faits tiennent sur
  le moteur audité.
- **Porte sur le nouveau build** (`build/v10-fixes/faits_math/build`) : code 0. Les trois binaires sont **identiques octet
  pour octet** à ceux de référence (mêmes sha256, `apres/binaires.sha256`). Sorties : `apres/porte_*.txt` et
  `apres/porte_codes.txt`.
- **Compatibilité d'intégration** : la porte rend aussi le code 0 sur les binaires déjà construits par les groupes
  `oracles`, `entrees_cli`, `tete` et `pool` (`apres/compat_autres_groupes.txt`). Leurs changements de CLI n'ajoutent que
  des options opt-in (`--dump-levels`, `--dump-births`) ou des refus d'entrée ; les formats lus par défaut sont inchangés.
- **« Échoue sur l'ancien build »** : sans objet au sens littéral. Aucun défaut n'est corrigé, et les faits sont vrais du
  moteur audité, donc la porte passe sur l'ancien build comme sur le nouveau. La non-vacuité est établie par :
  - les cinq mutants des sorties lues, permanents et intégrés à la porte ;
  - trois mutants **compilés** du moteur, construits dans `/tmp/faits_math_mut` et tous tués (code 1). Diffs et sorties
    sont dans `apres/mutants_compiles/`.

  | mutant compilé | changement dans `src/tower/tower.cpp` | résultat |
  | --- | --- | --- |
  | `cover_as_core` | `const bool cover = false && …` | code 1, 23 échecs : F1 (6), F2 (16), et l'équivalent rejeté sur ces sorties faussées |
  | `core_as_cover` | `const bool cover = true \|\| …` | code 1, 42 échecs : F1 (1), F2 (14), F3 (5), F4 (21), et l'équivalent rejeté |
  | `core_entry_dkm1` | entrée core à $D_{K-1}$ au lieu de $D_K$ | code 1 : `mhgp10_cluster` refuse (code 3, `rank_order`), échec natif nommé |

- **`ctest -L gate` complet** sur le nouveau build (`apres/ctest_gate.log`) : **10/10, code 0**, 1 088 s. La nouvelle porte
  prend 0,35 s ; l'oracle catalogue 501 s, l'oracle tour 436 s. Un premier passage, lancé sous `nice 5` avec une
  charge machine de 28 à 44 sur 8 cœurs (autres agents), avait vu l'oracle catalogue dépasser son délai de 1 800 s.
  Il a été arrêté par groupe de processus (code 143) et son journal est conservé
  (`apres/ctest_gate_passage1_nice5_interrompu.log`). Cause environnementale : les binaires sont identiques à la
  référence, et le second passage en priorité normale est vert.
- **ASan/UBSan** (`-DMHGP10_SANITIZE=ON`, build jetable `/tmp/faits_math_asan`) : build code 0. La porte, jouée sur ces
  binaires sous `ASAN_OPTIONS=detect_leaks=1` et `UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1`, rend le **code 0**
  (37 exécutions natives, aucun rapport). Tout rapport aurait fait échouer un binaire, donc la porte. Traces :
  `apres/asan_build_code.txt` et `apres/asan_porte.txt`.
- **TSan** : non pertinent. Aucun code concurrent n'est touché et la porte exécute tous ses binaires à `--threads=1`.
- **Documentation** : `python3 tools/check_docs.py` sur l'export complet avec le patch appliqué valide 876 fichiers
  Markdown actifs. `check_scope`, `check_implementation_status`, `check_contracts` et `check_references` sont verts.
  `pycodestyle --max-line-length=120` et `pyflakes` ne signalent rien sur la porte.
- **Différentiels**. Tête et tour : `head_diff.py build/v10-wt/mhgp10_cluster build/v10-fixes/faits_math/build/mhgp10_cluster`
  rend **0 écart sur 18 cas** (étiquettes et arbre exporté, `core` et `cover`, K = 1 à 10), code 0
  (`apres/differentiel_tete.txt`). Le différentiel catalogue `differentiel_j2c.sh` n'a pas été rejoué : 30 exécutions
  lourdes (trames entières, K = 10, jusqu'à 4 fils) sur une machine partagée saturée, pour un `mhgp10_catalogue`
  identique octet pour octet à sa référence (même sha256), ce qui établit l'égalité des sorties plus fortement
  qu'un différentiel.

## 6. Sorties sur entrées valides

Aucune. Aucun source du moteur ni d'une CLI n'est modifié. Les trois binaires reconstruits sont identiques octet pour
octet aux binaires de référence (sha256 ci-dessus).

## 7. Révision en cours de l'auditeur continu

Pendant ce travail, l'auditeur révisait `AUDIT_LAMINARITE_POINTS_20260929.md` dans le worktree (modifications non
commitées, lues sans y toucher). Sa nouvelle § 7, relecture des parties I–II de la thèse, retire la recommandation
« `core`, référence fidèle au modèle ». La Déf. 8 définit l'amas discret par $X\cap\delta_r(C)$, pas par $C\cap X$ ; core
est une restriction exacte et stable, mais il retarde les points frontière. Mes textes s'alignent sur le seul fait
mathématique en jeu, $C\cap X\subseteq D_r(C)$ : deux sites à distance d sont couverts dès d/2 à K = 2 et n'entrent en
core qu'à d. F1 grave ce fait (α = 1 pour tous, aucun point core au rayon 1). L'amendement de CLUSTER_v2 ne reprend donc
plus la formule de la note d'accusé de réception (« core reste la référence fidèle au modèle »). La section V10 du registre
ne prend aucune position de politique. Les numéros de section cités (§ 1, 2, 2.2 et 4) restent valables dans la
version commitée à `12aa92110`, et les deux fichiers d'audit liés depuis le registre y existent toujours.

## 8. Limites et points ouverts

- La porte fixe la sémantique **actuelle** de `cover` (première couverture, un seul propriétaire). Une future
  projection, comme l'ancrage différé du point contesté, la fera échouer par construction : le mutant
  `point_conteste_differe` le montre déjà. Il faudra alors réviser ensemble la porte, la ligne F1 du registre et
  l'amendement de CLUSTER_v2.
- Fixtures alignées seulement : l'oracle `Line` est exact pour des sites colinéaires, pas en 3D général. Les faits sont
  des contre-exemples et un cas d'égalité ; ils ne qualifient ni une tête, ni un gain de score, ni la stabilité des
  sorties LiDAR.
- Niveaux double de `--tree` : exacts sur ces fixtures, et vérifiés comme tels. Le quotient numérique général de
  `point_dendrogram` (TOUR_ET_POINTS § 7) n'est pas traité ici.
- Non gravés, faute de faits propres au moteur : le consensus par maximum d'ultramétriques (vrai) et les contre-exemples
  moyenne, minimum et médiane (AUDIT_LAMINARITE § 4.3). Ce sont des faits de mathématique pure, à inscrire si une tête de
  consensus est proposée.
- Mentions « `cover` = amas discrets du théorème 2 » non amendées ici, hors du périmètre fixé (CLUSTER_v2 et le
  registre). Elles figurent dans `PASSATION.md` l. 28, `src/tower/tower.hpp` l. 129, `cli/mhgp10_cluster.cpp` l. 12 et
  `bench/synthetic/prereg/README.md` l. 86. Les reçus restent immuables. À aligner par le responsable de la passation et
  des errata.
- `fast` exige sur la VM un Python ≥ 3.8 (`math.isqrt`).
- Intégration : le bloc CMake est inséré après `mhgp10_points_cover`, pas en fin de bloc, pour limiter les conflits de
  contexte avec les patchs des autres groupes.
