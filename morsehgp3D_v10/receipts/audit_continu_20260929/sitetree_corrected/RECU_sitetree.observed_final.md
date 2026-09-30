# Reçu — contrat de `SiteTree` (clé `sitetree`, constat G1)

29 septembre 2026. Correction du constat G1 de l'audit indépendant
(`audits/audit_independant_20260929/GEOMETRIE_CATALOGUE.md`, § G1, et `CONTRE_AUDIT_GEOMETRIE.md`,
« Contrôles nécessaires sur les correctifs »).

```text
phase=exploration_v10_hors_registre
backend=cpu_reference
profile=quantized_u18_input_only
mode=correction_audit_independant (groupe sitetree)
public_status=not_claimed
GCP non utilisé
```

- **Base** : `git archive 0bce6cc00 morsehgp3D_v10` (le HEAD du worktree a avancé à `12aa92110` pendant la tâche ;
  `git diff 0bce6cc00 12aa92110` hors `audits/` ne touche que `PASSATION.md`, `README.md`,
  `bench/g4/cuda_probe.cu` et `receipts/ERRATA.md` : aucun fichier de ce correctif, le patch s'applique aux deux).
  Empreintes de base de `site_tree.cpp` et `site_tree.hpp` : `32296818…` et `3cb0a70d…`, celles que cite le
  contre-audit.
- **Copie de travail** : `R = /workspaces/E-HGP/build/v10-fixes/sitetree` (dépôt privé `R/src`, commit `base`).
- **Patch** : `R/sitetree.patch` (chemins `morsehgp3D_v10/…`, `git apply` à la racine du dépôt principal).
- **Worktree `build/v9-open-worktree`** : non modifié (aucun fichier, build, commit, branche ni stash).

## 1. Constat reproduit avant correction

Build intact : archive de `0bce6cc00` construite dans `/tmp/sitetree-avant` (Release, GCC 13.3.0, CMake 3.28.3).

| Reproduction | Commande | Code | Sortie |
| --- | --- | --- | --- |
| Sonde de l'auditeur (`probes/geometry_site_tree.cpp`, sha256 `987019a2…`, copiée dans `R/avant/`) contre le build intact | `R/avant/g1_compile.cmd`, puis `/tmp/sitetree-avant/geometry_site_tree` | **1** | `expected_shell=4 got_shell=3` ; site 1 `(152369,7,1)` : `exact_side=0 approx_delta=4194304 in_shell=0` ; `nearest_count=3 exact=0` |
| Même sonde contre `build/v10-wt/libmhgp10_core.a` (binaires de référence) | idem, autre bibliothèque | **1** | identique octet pour octet |
| Comparaison au reçu de l'auditeur (`geometry_site_tree_result.json`) | Python | — | `stdout` identique, code 1 identique |
| Vérification indépendante en `Fraction` (`R/avant/verif_cospherique.py`, Python nu, tient sous `-O`) | `python3 -O verif_cospherique.py` | **0** | centre exact `(450145/2, -50657630235/2, 309231580351/2)` ; quatre distances égales ; centre hors du cube u18 ; écart des distances approchées 4 194 304 contre une marge fixe de 0,02 |

Fichiers : `R/avant/g1_sonde_auditeur.{stdout,code}`, `R/avant/verif_cospherique.{py,stdout,code}`,
`R/avant/SHA256_SONDE.txt`.

**Cause.** `SiteTree::nearest` et `SiteTree::closed_ball` élaguaient et classaient les sites par une distance carrée
calculée en double au centre approché `a + fl(N)/fl(D)`, avec une marge fixe `kMargin = 0,02`. La justification
« erreur < 1e-3 en u18 » suppose le centre dans le nuage : l'erreur absolue croît comme `2^-53 |c|^2`. Pour un centre
circonscrit lointain (simplexe obtus ou presque plat), elle dépasse la marge : un site de coquille est élagué
(`closed_ball` le perd) ou déclaré intérieur, et le départage exact de `nearest` est faussé.

## 2. Portée produit : chaque appelant

Toutes les requêtes à centre rationnel du dépôt (`grep` de `closed_ball`, `nearest(`, `SiteTree` sur `src/`, `cli/`,
`tests/`, `bench/`, `receipts/` ; numéros de ligne de la base `0bce6cc00`) :

| Appelant | Requête | Centre | Dans le domaine du filtre ? |
| --- | --- | --- | --- |
| `src/tower/tower.cpp:886` | `closed_ball(a, S.c, …)`, juge d'échantillon (1 boule sur 32) d'une MEB stricte trouvée au catalogue | `S = meb(g, F)` (`:833`), ancre `a = g.P[S.anchor]` (`:846`), un site | oui, voir ci-dessous |
| `src/tower/tower.cpp:897` | `closed_ball(a, S.c, …)`, recensement hors catalogue | même `S` | oui |
| `src/tower/tower.cpp:1621` | `nearest(px, Center{{0,0,0},1}, kq, …)`, entrée core | centre = le site `px` | oui (N = 0, D = 1) |
| `tests/unit/unit_main.cpp:345–379` | les deux, centres circonscrits de 1 à 4 sites quelconques | peut sortir du nuage | non garanti : couvert désormais par le repli exact |
| `cli/mhgp10_tower.cpp:75`, `cli/mhgp10_cluster.cpp:112`, `receipts/head_point_dendrogram_20260929/headbench{,2}.cpp:28` | aucune : construisent l'arbre et le passent à `build_tower` | — | — |
| `tests/head/mreach.cpp:12, :98` | `kth_distance`, `within` : requêtes **entières**, exactes, hors G1 | — | — |

**Pourquoi le centre d'une MEB de la tour est dans le cube.** `meb()` (`tower.cpp:527`) rend soit la sphère
certifiée par `verify_meb` (`:467`), soit le Welzl exact de repli (`:542`).

- `verify_meb` contrôle l'appartenance du centre à l'enveloppe convexe **fermée** du support **avant** son filtre de
  distance (`set_approx`, `:501`) : `nr = 2` milieu (`:477`) ; `nr = 3` triangle non obtus, produits scalaires
  exacts `da, db, dd >= 0` (`:485`), donc centre dans le triangle fermé ; `nr = 4` orientation exacte (filtre
  semi-statique `orient_center_filtered`, repli `orient_center_wide`) du centre du même côté que le sommet opposé
  ou sur la face, pour les quatre faces (`:494`), donc centre dans le tétraèdre fermé.
- Le repli Welzl exact rend la MEB exacte de `F` ; le centre d'une MEB est dans l'enveloppe convexe des points de
  sa sphère, donc dans `conv(F)`.
- `F` est un ensemble de sites, `build_tower` refuse un nuage de plus de 18 bits (`:1167`) : `conv(F)` est dans
  le cube `[0, 2^18 - 1]^3`, l'ancre est un site. Une facette d'un seul site donne `N = 0, D = 1`.

**Mesure.** Un build instrumenté (`std::abort()` inséré dans les deux replis, `/tmp/sitetree-instr`, hors patch ;
son activité est prouvée : la sonde G1 liée à ce build meurt par SIGABRT, code 134) n'a jamais avorté sur :
les 7 cas du différentiel tour (2 090 188 `closed_ball` et 123 536 `nearest`, n = 8000 à 32 000, K = 5 et 10) ;
les 18 cas du différentiel tête (trames LiDAR entières jusqu'à 45 845 sites, K = 1 à 10, entrées core et cover) ;
les portes Python qui exécutent la tour : oracle tour (73 contrôles, 23 444 coupes, grilles cosphériques et
coplanaires comprises), couverture, collision de niveaux, équivalence par lots, condensation contre sklearn (codes 0,
`R/apres/instr_*.txt`). Aucun appel produit n'emprunte le repli.

## 3. Correctif

Fichiers : `src/cloud/site_tree.hpp`, `src/cloud/site_tree.cpp` (produit) ; `tests/regression/site_tree_far_center.cpp`,
`CMakeLists.txt`, commentaire de `tests/unit/unit_main.cpp` (portes).

1. **Garde rationnelle exacte** `bool SiteTree::filtered(anchor, c) const` : vraie si et seulement si tous les sites
   sont dans le cube u18 (drapeau `sites_u18_` calculé une fois sur la boîte racine exacte), l'ancre est dans le cube
   et le centre aussi : `0 <= D a_i + N_i <= L D` pour chaque axe, `L = 2^18 - 1`, en i128. Les bornes de
   représentation (`0 < D <= 2^82`, `|N_i| <= 2^100`, qui contiennent les formes q2/q3/q4 u18) sont testées
   d'abord : la garde ne déborde pour aucun `N`, aucun `D`.
2. **Domaine du filtre = domaine de la preuve.** Dans ce domaine, la marge fixe est prouvée (commentaire de
   `kMargin`) : `|cq_i - c_i| <= 4,02 u L = delta < 1,2e-10`, puis
   `|d - |z - c|^2| <= gamma_5 * 3 (L + delta)^2 + 3 delta (2L + delta) < 1,2e-4 + 1,9e-4 < 3,1e-4`, pour tout site
   comme pour `r2a` ; `0,02 > 2 * 3,1e-4`. Constantes vérifiées en rationnels exacts (total `2,985e-4`).
   Hypothèses : binaire64 au plus proche, pas de contraction FMA (`-ffast-math` refusé par `CMakeLists.txt`,
   `-ffp-contract=off` avec `MHGP10_MARCH`), conversion `__int128 -> double` correctement arrondie (vérifiée sur
   200 000 valeurs de 54 à 126 bits contre `float(int)` de Python : 0 écart).
3. **Repli exact hors du domaine**, pour les **deux** requêtes : `closed_ball` classe chaque site par sa clé exacte
   (sorties déjà triées par indice) ; `nearest` garde les `count` plus petits couples `(clé, site)` par insertion dans
   un tableau trié (même sémantique, même plafond de 64). Coût O(n) par requête, jamais atteint par la tour.
4. **Contrat écrit** dans l'en-tête : précondition de représentation (formes `center2/3/4` sur des points u18, ou
   centre sur un site ; `|s| < 2^122`), exactitude pour tout centre sous cette précondition, domaine du filtre, repli,
   plafond de `count` (64, déjà en vigueur, désormais écrit).

La tour n'est pas modifiée : `tower.cpp.o` est identique octet pour octet avant et après (§ 5).

## 4. Porte ajoutée

`mhgp10_regression_site_tree_far_center` (C++, `mhgp10_gate`, code exact 0, ligne `site_tree_far_center_ok`,
labels `gate;regression;fast`, 0,8 s). Codes : 0 conforme, 1 désaccord d'un juge, 3 plancher non atteint.

- **A, fixture gravée (sonde de l'auditeur)** : les quatre sites, `N = (4051305, -455918672115, 2783084223159)`,
  `D = 18` gravés et recalculés, cosphéricité jugée par une autre formule (`|2z - 2c|^2`, centre doublé entier).
  Attendu : coquille `{0,1,2,3}`, intérieur vide, `nearest(1..4)` = clés nulles départagées par indice ; centre hors
  domaine (`filtered` faux).
- **B, balayage** : 64 nuages u18, triangles très obtus, tétraèdres presque plats (voisin entier quelconque ou de
  plus petit déterminant non nul), variantes de la fixture, simplexes quelconques : 2 531 requêtes, 1 359 centres hors
  du cube, **505 requêtes adverses** (la marge fixe classait mal un site de coquille), 15 186 contrôles `nearest`.
  Juge d'arithmétique autre : `D s(z) = |D (z - a) - N|^2 - |N|^2` en entiers larges ; les clés rendues par `nearest`
  sont vérifiées (`D * clé = D s(z)`).
- **C, domaine** : `filtered` égal au jugement indépendant « centre dans le cube » (entiers larges) sur toutes les
  requêtes ; sept bords gravés (centres sur les faces `x = 0`, `x = L`, `y = L` : servis ; à `-3/2`, `L + 3/2`,
  `L + 15/2`, sous `z = 0` : repli) ; centres de type MEB toujours servis (960 sites, 960 milieux, 444 triangles
  aigus, 419 tétraèdres contenant leur centre) ; nuage de 21 bits toujours en repli (240 requêtes exactes).
  Planchers : hors cube >= 400, adverses >= 300, requêtes >= 1000, sites et milieux >= 500, aigus et tétraèdres
  >= 100, 21 bits >= 200, filtrées >= 2000, repli >= 600.

| Build | Commande | Code | Sortie |
| --- | --- | --- | --- |
| Ancien (bibliothèque de `0bce6cc00`), parties A et B (`-DMHGP10_GATE_OLD_API`, la partie C appelle l'API nouvelle) | `R/avant/porte_ancien_build.cmd` | **1** | 782 désaccords ; fixture : coquille `{0,2,3}`, `nearest(2) = {0,2}`, `nearest(3) = {0,2,3}` |
| Nouveau | `ctest -R site_tree_far_center` | **0** | `site_tree_far_center_ok` |
| Sonde de l'auditeur inchangée, liée au nouveau build | `R/apres/g1_sonde_auditeur.cmd` | **0** (1 avant) | `got_shell=4`, `nearest_count=3 exact=1` |

## 5. Preuves

Machine partagée (8 cœurs, charge 13 à 40 pendant les mesures). Fichiers sous `R/apres/` sauf mention.

| Preuve | Commande | Résultat |
| --- | --- | --- |
| Portes complètes, nouveau build | `ctest --test-dir R/build -L gate --output-on-failure` | **10/10**, code 0, 1 706 s (`ctest_gate_nouveau.txt`) |
| Porte G1 et unité sur le build final (après mise en forme des commentaires) | `ctest -R 'site_tree_far_center\|mhgp10_unit'` | 2/2, code 0 (`ctest_porte_finale.txt`) |
| Identité du build testé et du build final | `sha256sum` | `libmhgp10_core.a`, `mhgp10_tower`, `mhgp10_cluster`, `mhgp10_catalogue`, `mhgp10_unit` identiques : la mise en forme finale ne touche que des commentaires et la porte |
| Localisation du changement | `sha256sum` des objets de `mhgp10_core`, avant contre après | seul `site_tree.cpp.o` diffère ; `tower.cpp.o` et les huit autres objets sont identiques octet pour octet |
| Différentiel tête et tour (18 cas) | `head_diff.py build/v10-wt/mhgp10_cluster R/build/mhgp10_cluster /tmp/sitetree-headdiff` | **ECARTS 0 sur 18**, code 0 ; les 18 empreintes égalent celles du reçu gravé `receipts/head_point_dendrogram_20260929/differentiel_tete.txt` (`differentiel_tete.txt`) |
| Même différentiel, binaire instrumenté | idem avec `/tmp/sitetree-instr/build/mhgp10_cluster` | **ECARTS 0 sur 18**, code 0 (`differentiel_tete_instrumente.txt`) |
| Différentiel tour : dump exact et compteurs à 1 fil, référence / nouveau / instrumenté | `python3 R/outils/tower_diff.py build/v10-wt/mhgp10_tower R/build/mhgp10_tower /tmp/sitetree-instr/build/mhgp10_tower /tmp/sitetree-towerdiff` | **ECARTS 0 sur 7** (uniforme 8000/16000/32000 K5 core, coquilles 8000 K10 cover, filaments 16000 K10 core, terrain 32000 K5 cover, LiDAR 11 536 K10 cover), code 0 (`differentiel_tour.txt`). À 2 fils, deux passes du binaire de référence diffèrent déjà sur les compteurs de mémo : d'où 1 fil |
| Différentiel catalogue | `differentiel_j2c.sh R/build/mhgp10_catalogue build/v10-j2/build-base/mhgp10_catalogue` | **10/10 IDENTIQUES**, `FIN echec=0`, code 0 (`differentiel_catalogue.txt`) ; le catalogue n'appelle pas `SiteTree` |
| ASan + UBSan (`-DMHGP10_SANITIZE=ON`, `/tmp/sitetree-asan`) | porte G1, unité, tour sur LiDAR 5 286 sites K5 cover 4 fils | codes 0, aucun rapport (`asan_*.txt`) |
| ThreadSanitizer (`-DMHGP10_TSAN=ON`, `setarch -R`) | porte G1 ; tour LiDAR 5 286 sites K5 cover et uniforme 8000 K10 core, 4 fils | codes 0, aucun avertissement (`tsan_*.txt`) ; le correctif n'ajoute aucun état partagé (`sites_u18_` écrit au constructeur) |
| Coût de la garde | `valgrind --tool=callgrind` sur `R/outils/bench_guard.cpp`, n = 8000, 26 976 requêtes | +3 447 375 instructions, **128 par requête**, +2,8 % des instructions du micro-banc (préparation comprise) ; sommes de contrôle des sorties identiques (`callgrind_garde.txt`) |
| Temps mur | `bench_guard`, ancien/nouveau alternés, 3 × 5 passes, n = 8000/16000/32000 | indiscernable sous la charge (écarts de ±50 % entre passes d'un même binaire) ; sorties identiques (`bench_garde.txt`) |
| Conversion `__int128 -> double` | 200 000 valeurs de 54 à 126 bits contre `float(int)` de Python | 0 écart : arrondi correct, hypothèse de la preuve |

Portes ajoutées : `mhgp10_regression_site_tree_far_center` (§ 4) ; ancien build **1**, nouveau **0**.

## 6. Sorties sur les entrées valides

**Aucun changement.** Pour tout appel produit, `filtered` est vrai (§ 2, preuve et build instrumenté) et le chemin
exécuté est celui d'avant, instruction pour instruction hors de la garde. Les différentiels tête (18 cas), tour
(7 cas, dumps exacts et compteurs) et catalogue (10 cas) sont identiques. Seules changent les réponses à des centres
hors du cube (hors produit) : fausses avant, exactes après.

## 7. Limites et points ouverts

- **Précondition de représentation non vérifiée à l'exécution** (comme partout pour `geom::side_key`) : un `Center`
  construit à la main hors des formes q2/q3/q4 u18 peut déborder l'i128 dans la clé exacte, sur les deux chemins.
  La garde elle-même ne déborde jamais. Un refus explicite demanderait une signature à statut et une modification de
  `tower.cpp` : non fait, par minimalité. Les requêtes rationnelles sur un nuage de plus de 18 bits ne valent que
  pour des centres représentables (site, milieu) ; la tour refuse ces nuages.
- **Repli O(n) par requête** : exact mais lent. Aucun appel produit ne l'atteint ; un futur appelant à centres
  lointains paierait O(n) par requête. La conception (`ARCH_v2.md` § 3, `CONCEPTION_V10.md` § 12.0) prévoit de
  retirer les requêtes rationnelles de `SiteTree` au profit de `LeafOracle` ; ce correctif ne préjuge pas de cette
  migration.
- La fixture vit dans une porte dédiée plutôt que dans `test_site_tree_rational` (recommandation de l'audit) : le
  commentaire de l'unité rationnelle renvoie désormais à cette porte et annonce le repli.
- Les filtres flottants propres à la tour (`kApproxMargin` de `verify_meb` et de la décroissance I3) reposent sur
  le même domaine (centres de MEB dans `conv(F)`) et ne sont pas touchés : `verify_meb` contrôle l'enveloppe avant
  son filtre.
- Temps : les mesures murales locales ont été prises sous une charge de 13 à 40 sur 8 cœurs ; elles ne valent pas
  mesure de performance. Les compteurs déterministes (1 fil) et le comptage d'instructions sont les preuves.
- La partie C de la porte appelle `filtered`, absente de l'ancienne API : la démonstration « échoue avant » compile
  la porte avec `-DMHGP10_GATE_OLD_API` (parties A et B seules, mêmes contrôles de sortie).
- Non rejoué : G4, GPU (GCP non utilisé).

## 8. Fichiers

- `R/sitetree.patch` : sha256 `ed24d7da4c32fedfec899e586b3bae4bdbc19135bb8f88b8956321bd842a261b`, 5 fichiers,
  +540 −6 ; `git apply --check` vérifié sur des archives de `0bce6cc00` et de `12aa92110`.
- Sources finales (sha256) : `site_tree.hpp` `362bf31d…`, `site_tree.cpp` `d83e999b…`,
  `tests/regression/site_tree_far_center.cpp` `38157638…`, `CMakeLists.txt` `7e9fed3b…`,
  `tests/unit/unit_main.cpp` `6cb136b7…`.
- `R/avant/` : sonde de l'auditeur et ses sorties, vérification `Fraction`, porte contre l'ancien build.
- `R/apres/` : portes, différentiels, sanitizers, build instrumenté, coût de la garde.
- `R/outils/` : `tower_diff.py` (différentiel tour), `bench_guard.cpp` (micro-banc de la garde) ; hors patch.
- Builds jetables : `/tmp/sitetree-avant` (base), `/tmp/sitetree-final` (sources finales), `/tmp/sitetree-instr`
  (abort au repli), `/tmp/sitetree-asan`, `/tmp/sitetree-tsan`. Build demandé : `R/build`.
