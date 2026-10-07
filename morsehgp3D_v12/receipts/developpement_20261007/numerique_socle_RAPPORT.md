# Rapport — contrat numérique en repère local dans le socle de la v12

Heures lues avec `date -u` : début 2026-10-07 11:15:24 UTC ; fin 2026-10-07 14:24:56 UTC (rapport clos).

```text
phase=exploration_v12_hors_registre
backend=cpu_reference
objet=full_pi0
quantification=quantized_u21_input_only (profils 24 et 32 admis à la construction, non qualifiés selon D6)
public_status=not_claimed
GCP non utilisé
```

Base : `morsehgp3D_v12/` à `26b53648c`. Le code du socle (`src/`, `tests/`, `cmake/`, `tools/`, `CMakeLists.txt`,
`bench/index_io*`) est identique au HEAD `3d6c92c1f` (191 fichiers comparés par empreinte de blob à 13 h 51 min 05 s
UTC, aucun écart) ; `patch_numerique.diff` (82 fichiers) passe `git apply --check` sur ce HEAD (13 h 49 min 29 s UTC,
lecture seule). `README.md` et `docs/ARCHITECTURE.md` ont changé au HEAD depuis la base : intégrer par le correctif,
pas en recopiant ces deux fichiers. Copie modifiée : `scratchpad/v12_numerique/morsehgp3D_v12/` ; constructions dans
`scratchpad/v12_numerique/build21`, `build24`, `build32`, `build_san21`, `build_san32` (références d'origine
`build_base21`, `build_base24` ; expérience de clé `build_exp21`). Aucune écriture sous `/workspaces/E-HGP`, aucune
commande git d'écriture. Détail fichier par fichier : `CHANGEMENTS.md`.

La contre-lecture Codex du socle (`receipts/audit_socle_microbancs_20261007/session/REPORT.md`, pin `95247cf4b`,
postérieure à la base) constate que le socle T0 « ne réalise pas encore le repère local, le census gardé ni le
traitement des doublons prévu par D8 » : ce travail livre les deux premiers, pas D8 (§ 7).

## 1. Résultat en une phrase

Le socle calcule désormais chaque prédicat en repère local, choisit sa voie par palier d'étendue (étroit ≤ 16, moyen
≤ 24, large ≤ 33), lie ses certificats à leur domaine, recense les boules certifiées par une garde entière sans centre
absolu, et se construit et passe toutes ses portes rapides aux profils 21, 24 **et 32** — sans qu'aucune valeur, aucun
oracle ni aucune empreinte ne change aux profils 21 et 24. Seule la clé de Morton normalisée (étape 7) n'est pas
appliquée : elle change l'ordre canonique des sites que plusieurs juges encodent (§ 4).

## 2. Constructions

Release, GCC du codespace, C++20 sans extension, `-Wall -Wextra -Wpedantic -Werror`, `-j4` au plus.

| Profil | Configuration | Construction | Avertissements |
| --- | --- | --- | --- |
| u21 (`-DMHGP12_COORD_BITS=21`, défaut) | admise | complète, 0 erreur | 0 |
| u24 | admise | complète, 0 erreur | 0 |
| u32 | **admise** (refusée avant ce travail) | complète, 0 erreur | 0 |
| 18 | refusée, jeton `mhgp12_coord_bits_18_abandonne` | — | — |
| 33 (ou toute autre valeur) | refusée, jeton `mhgp12_coord_bits_invalide` | refus aussi à la compilation | — |

Tailles natives : `Sphere` 144 / 160 / 232 octets, `Q4Candidate` 80 / 80 / 144 aux profils 21 / 24 / 32 (21 et 24
inchangés). Style : `python3 tools/check_style.py --root morsehgp3D_v12` → `style_ok fichiers=189` (aussi sous `python3 -S -O`). Grammaire Python 3.10 des
quinze scripts touchés vérifiée par `ast.parse(..., feature_version=(3, 10))` (le codespace n'a que Python 3.12).

## 3. Portes

Commande : `ctest --test-dir <build> -LE long --no-tests=error -j4` (série finale du 7 octobre, 13 h 35 à 13 h 38
UTC ; journaux `ctest21_final.log`, `ctest24_final.log`, `ctest32_final.log`).

| Profil | Enregistrées | Sélectionnées (`-LE long`) | Passées | Sautées | Échouées |
| --- | ---: | ---: | ---: | ---: | ---: |
| u21 | 438 | 415 | 414 | 1 (`mhgp12_support_lidar_sentinel`, sans `MHGP12_DATA_DIR`) | 0 |
| u24 | 438 | 415 | 414 | 1 (idem) | 0 |
| u32 | 438 | 415 | 414 | 1 (idem) | 0 |
| socle d'origine, u21 et u24 (référence) | 412 | 389 | 388 | 1 (idem) | 0 |

Inventaire : 28 portes ajoutées (toutes `fast`), 2 retirées (`mhgp12_core_coord_bits_32_refusal` et
`mhgp12_support_configure_coord_bits_32`, remplacées par leurs jumelles `_33` puisque 32 est admis). Ajoutées :
16 groupes `mhgp12_num_local_*` et leur inventaire, `mhgp12_index_guarded_{fixtures,uncertified,witnesses,inventaire}`,
`mhgp12_index_guarded_fraction` (+ `_opt`), `mhgp12_index_translation` (+ `_opt`), `mhgp12_cloud_unit_identity`,
`mhgp12_core_coord_bits_33_refusal`, `mhgp12_support_configure_coord_bits_33`.

Aucune porte existante n'a changé d'attendu aux profils 21 et 24 : oracles Fraction, lignes gravées
(`q4_presentation`, `triangle_kind`, `num_big`, `num_radical`, `num_roots` à 21, `index_io`), empreintes des
références (`reference/test_supports.py`, `radical_port.py`) et comptes de travail des census (`mhgp12_index_unit_lattice`,
`mhgp12_index_borrowed_lattice`) sont identiques. Les renommages d'API (`Budgets<B>` → `SpanBudgets<S>`,
`Budget` → `DomainBudget`) gardent les mêmes valeurs ; au profil 32, les attendus propres au profil sont complétés
sans toucher ceux de 21 et 24 (`CHANGEMENTS.md` § 5).

Comptes observés des nouvelles portes Python, rejouées sous `python3 -S -O` (u21 et u24 / u32) : census gardé sur
la matrice de l'index, 842 / 881 requêtes certifiables aux populations identiques au census générique et jugées par
Fraction, 168 / 132 non certifiables tenues hors de la garde, 31 776 / 33 208 contrôles, et la garde exercée (208 / 224
boîtes disjointes, 290 / 306 partielles, 212 / 250 sites hors pavé ; 392 / 3 014 évaluations en voie large) ;
translation, 852 / 856 comparaisons à translation près, 458 / 460 nuages déplacés, 64 au bord bas, 394 / 396 au bord
haut. Portes C++ : translation des prédicats, 1 197 sphères, 7 182 requêtes et boîtes, 1 192 paires de centres ; census
gardé, 120 requêtes possédées et empruntées, garde exercée (184 boîtes disjointes, 208 partielles, 168 sites hors
pavé).

ASan + UBSan (`MHGP12_SANITIZE=ON`, `-fno-sanitize-recover=all`, unités `num`, `index`, `cloud`, profils 21 et 32,
journaux `ctest_san21.log`, `ctest_san32.log`) : 172 portes sur 172 à chaque profil, aucun « runtime error ». Hors de la
règle « sanitizers sur G4 » : contrôle local d'appoint des voies natives nouvelles, pas une qualification.

## 4. Clé de Morton normalisée : arrêtée, non regravée (étape 7)

Implantée dans une copie à part (`experience_cle/`, correctif `patch_cle_normalisee.diff`, 314 lignes : clé sur les
coordonnées moins le minimum global, `u64` si B_eff ≤ 21, `u128` sinon, choisie une fois par entrée ; `Cloud::origin()`
et `effective_bits()` ; coupe de l'index sur les coordonnées normalisées). Résultat au profil 21 : **9 portes rouges
sur 412** (`echecs_cle_normalisee_21.txt`) :

- `mhgp12_cloud_unit_judge` (juge indépendant `expected_of` et `canonical_order` : clé absolue) ;
- `mhgp12_index_fraction`, `mhgp12_index_borrowed_fraction`, `mhgp12_index_guarded_fraction` et leurs jumelles `-O`
  (`fraction_model.sites_of` range les sites par clé absolue : « identité canonique sites ») ;
- `mhgp12_mutants_cloud_manifest` et `-O` (motifs de `cloud.cpp` déplacés).

**Pourquoi l'empreinte change.** L'ordre de Morton n'est pas invariant par translation. Témoin exact :
P = (2,0,0), Q = (1,1,0), minimum global (1,0,0). Clés absolues : P → 8 (bit 1 de x en position 3), Q → 1 + 2 = 3,
donc Q avant P. Clés normalisées : P' = (1,0,0) → 1, Q' = (0,1,0) → 2, donc P avant Q. Tout nuage dont le minimum
n'est pas nul par axe peut donc changer d'ordre de sites (`SiteIdx`), donc les tableaux du `Cloud`, les listes du
census rendues en `SiteIdx`, la forme de l'arbre radix (nœuds, profondeur) et le travail du parcours. Les juges du socle
(et `bench/index_semantic.py`, le lecteur strict de la v11 via `bench/full_semantic.py`) définissent l'ordre canonique
par la clé **absolue** : les adopter à la clé normalisée est une regravure de la règle d'ordre, que le contrat (§ 4)
prévoit mais qui revient au développeur. Prévu sans être joué : au profil 24, la formule mémoire du nuage (enregistrement
de 16 octets si B_eff ≤ 21, au lieu de 32) changerait aussi l'attendu de `mhgp12_cloud_unit_budget`.

Ce qui est livré pour l'étape 7 sans changer l'ordre : la clé exacte du socle (63, 72 et **96 bits en `u128` au
profil 32**, aucune troncature), l'identité des sites par égalité de clé, et la porte `mhgp12_cloud_unit_identity`
(trois positions dont deux auraient la même clé tronquée → 3 sites ; vrais doublons intercalés → 2 sites ;
permutations à PointId stables) avec son mutant `identite_par_cle_tronquee`, tué. La porte d'invariance par
translation (étape 8) juge à translation près et n'a pas besoin de la clé normalisée.

## 5. Points du contrat discutés

Aucune règle du contrat ne s'est révélée fausse. Cinq points méritent l'attention de l'auditeur.

1. **« Garde d'un bit trop étroite » (§ 7, CST-0108).** Si le mutant s'entend du pavé construit avec s − 1, soit
   (m − M, m + 1,5 M), il est probablement **équivalent** : sur 180 000 supports certifiés tirés aux étendues 2, 3
   et 4 (q2, q3 aigus, q4 à centre intérieur ; `outils/portee_boules.py`), la boule fermée dépasse le coin minimal de
   son support d'au plus 1,30 M vers le haut et 0,64 M vers le bas, donc reste dans ce pave rétréci (indice, pas une
   preuve : la borne du contrat, R < 2M et c dans [m, m + M − 1], laisse 2,73 M). Le pavé (m − 2M, m + 3M) a donc
   du jeu, ce qui est sûr. Le mutant livré sous ce nom rétrécit d'un bit le **domaine de la garde** (certificat au
   domaine s + 1 au lieu de s + 2) ; il est tué par un témoin exact trouvé par recherche : support aigu
   (0,0,0), (821959,633415,78800), (178081,356898,963792), étendue 20, D = 1954560631510995730427746, certifié au
   domaine 21 et pas 22. Deux mutants de pavé tuables complètent (`pave_haut_trop_court` : m + M ;
   `pave_bas_trop_court` : m − 1), tués par la boule q2 (100,100,100)–(107,107,107), qui atteint x = 109,56 et 97,44.
2. **Voie des paliers et seuils de la table.** Avec trois paliers, la voie native garantie par le palier s'arrête à
   16 pour le côté (la table dit 19) et l'orientation (16). Entre 17 et 19, le côté reste natif par le **certificat de
   domaine**, qui tient toujours pour s ≤ 19 (pire support : D < 24·2^(4s) < 2^(123−2s) ⟺ s ≤ 19), et de même le côté
   gardé au domaine s + 2 ; l'orientation reste native à 17 pour le pire support (certificat) et tombe en voie large à
   18. Les témoins « côté à 19 et 20 », « côté gardé à 19 et 20 » et « orientation à 16 et 17 » du § 7 sont donc joués
   avec leur compteur, plus la coupe de palier 16/17.
3. **Comparaison de centres en deux temps (§ 4).** Le contrat donne des parties entières sur 64 bits pour les
   naissances certifiées. Le comparateur public de num accepte aussi des sphères non certifiées (les portes existantes
   comparent des centres lointains, par exemple y = (1 − m(m − 1))/2, de 63 bits au profil 32) : les parties entières
   y sont en i128 (`Big` si les coefficients sortent de i128), les fractions par produits croisés natifs ou à la
   largeur du palier. Même ordre que la v11, axe par axe ; aucun produit de la taille de B.
4. **Voie générique au profil 32.** `LatticeSphere` (census générique, toute sphère) a pour repère le domaine entier
   (boîtes et sites n'importe où) : au profil 32, une q3 ou une q4 n'y est native que sous certificat au domaine 32
   (D < 2^59) ; sinon les bornes continues de la v11 servent, exactes mais moins sélectives. Le chemin chaud prévu est
   le census gardé, en repère local. Rien n'est mesuré ici.
5. **Stockage au profil 32.** Les coefficients et niveaux stockés suivent le plus grand support du profil (s ≤ B) :
   au profil 32, `Sphere` passe de 144 à 232 octets et les numérateurs de centre sont des entiers larges de 192 bits.
   Les voies de calcul lisent le palier (copie i128 contrôlée dès que les coefficients y tiennent), mais la mémoire par
   boule croît : à mesurer selon D6 (CST-0207) avant tout choix de profil.

## 6. Mutants

Lanceur `tests/mutants/run_mutants.py` (copie mutée, construction limitée au module, porte citée jouée et lue dans
le rapport structuré de CTest), `--jobs 4` (3 pour le complément core), une construction à un fil par mutant, profil
21 par défaut et profils propres au mutant (`options`). **Toutes les campagnes ci-dessous jugent l'arbre final** :
`sources_sha256` = `fbf8f004c8273398268146ab3fb61ce1e2d940b2995c5aed27997b596e848b88`, témoin non muté vert,
code de sortie 0. Comptes rendus : `mutants_<unité>.json` et `.log` (`mutants_core_complement.*` pour la seconde passe
core).

| Manifeste | Mutants jugés | Tués | Dont à la construction | Survivants | Plancher | Fin (UTC) |
| --- | ---: | ---: | --- | ---: | ---: | --- |
| `num.json`, campagne complète (début 13:46:06) | 76 | 76 | 1 (`palier_etroit_dix_sept`) | 0 | 76 | 14:17:37 |
| `index.json`, campagne complète | 18 | 18 | 0 | 0 | 18 | 14:24:21 |
| `cloud.json`, campagne complète | 17 | 17 | 0 | 0 | 17 | 14:19:40 |
| `core.json`, les sept mutants qui visent un fichier modifié (`types.hpp` ×6, `CMakeLists.txt` ×1), deux passes `--only` | 7 | 7 | 1 (`identifiants_confondus`) | 0 | 4 puis 3 (84 au manifeste) | 13:34:41 et 14:18:09 |

Première campagne num (arbre antérieur au dernier témoin) : 75 tués sur 76, survivant
`garde_plancher_au_lieu_du_plus_proche` (minorant pris au plancher du centre local au lieu de l'entier le plus proche) :
aucun témoin n'avait un rayon assez petit pour que le plancher sorte de la boule quand l'arrondi y reste. Témoin
ajouté (`guard_boxes`) : triangle équilatéral (100,100,100), (101,101,100), (101,100,101), centre
(100 + 2/3, 100 + 1/3, 100 + 1/3), R² = 2/3, boîte [100,101]^3 : le plancher (100,100,100) est sur la sphère,
l'entier le plus proche (101,100,100) est intérieur (distance² 1/3). Rejoué seul (3 sur 3, fin 13:42:39) puis dans la
campagne complète : tué par `mhgp12_num_local_guard_boxes`. Les campagnes index et cloud jouées plus tôt sur des
arbres antérieurs (18 sur 18, 17 sur 17) sont gardées sous `mutants_{index,cloud}_arbre_anterieur.*`.

Non rejoués : `io` (22 mutants), `sched` (8) et les 77 autres mutants de `core` ; aucun ne vise un fichier modifié et
leurs portes sont inchangées ; les six manifestes passent `--check`. Les campagnes complètes de tous les manifestes
restent à jouer sur G4 avec la matrice de qualification (CST-0002).

## 7. Non fait, et pourquoi

- **Clé de Morton normalisée** (étape 7) : implantée en expérience, arrêtée sur le changement d'ordre canonique (§ 4),
  livrée en correctif séparé, non appliquée. Décision de regravure au développeur.
- **Refus D8 et option « sites distincts »** : hors des neuf étapes ; le nuage regroupe toujours les doublons en sites à
  multiplicité (comportement du socle).
- **Voie de l'appareil (§ 6)** : aucun module GPU dans le socle ; paliers et budgets sont des `constexpr` réutilisables.
- **Dominance G1 (2s+3)**, **orientation avec centre sur sites de coquille (7s+14)** et **orientation d'un quatrième
  site gardé (3s+10)** : leurs appelants (catalogue, `supports`, `LEM-T7`) ne sont pas dans le socle. Le réservoir
  (2s+4) est écrit dans num (`local.hpp`) avec sa porte, comme demandé ; les orientations génériques de num couvrent
  des points quelconques par le repère de leurs arguments.
- **Compteurs de voies pour le produit scalaire et le produit vectoriel isolés, `strictly_acute`** : leur voie
  (i64 jusqu'à l'étendue 30/31, i128 au-delà) n'a pas de compteur propre ; elle est exercée par les portes de centres et
  de géométrie aux trois profils.
- **Export des centres absolus et des niveaux (§ 4)**, **lecteur qui retrie pour `supports` et `cover`**,
  **S\* par ordre lexicographique des coordonnées** : modules de vues et de supports absents du socle.
- **Mesures** : aucun temps, aucune mesure MES-S, aucun coût de profil au sens de D6 (CST-0207). Le socle compile et
  passe au profil 32 ; il n'est pas qualifié.
- **Matrice G4, suite `long`, campagnes complètes de tous les manifestes** : non jouées (règle « lourd sur G4 »).
- **Documents** : `docs/PORTS.md` et `docs/PROVENANCE.md` décrivent le port T0 et ne sont pas réécrits ;
  `audits/CONSTATS.md` n'est pas touché (lignes CST-0108 à CST-0114, CST-0201, CST-0202, CST-0204, CST-0208 à faire
  passer par le développeur avec les preuves de clôture : portes et mutants ci-dessus) ; le contrat n'est pas modifié.

## 8. Fichiers

Rapport clos le 2026-10-07 14:24:56 UTC (heure lue avec `date -u`).

Livrables dans `scratchpad/v12_numerique/` : `morsehgp3D_v12/` (copie modifiée), `CHANGEMENTS.md`, `RAPPORT.md`,
`patch_numerique.diff` (correctif complet, s'applique au dépôt courant), `patch_cle_normalisee.diff` et
`echecs_cle_normalisee_21.txt` (expérience de l'étape 7, non appliquée ; copie `experience_cle/`), journaux
`ctest{21,24,32}_final.log`, `ctest_san{21,32}.log`, comptes rendus de mutants de l'arbre final
`mutants_{num,index,cloud,core,core_complement}.{json,log}`, comptes rendus antérieurs
(`mutants_num_premiere_campagne.*`, `mutants_num_reprise.*`, `mutants_{index,cloud}_arbre_anterieur.*`), chronologie
des campagnes `campagnes.txt`, outils de travail `outils/` (recherche des témoins, mise à jour des manifestes,
séries de portes et de mutants).

Fichiers modifiés dans `morsehgp3D_v12/` (64) :

- `CMakeLists.txt`
- `README.md`
- `bench/index_io_test.py`
- `docs/ARCHITECTURE.md`
- `src/cloud/cloud.hpp`
- `src/core/types.hpp`
- `src/index/census.cpp`
- `src/index/census_workspace.cpp`
- `src/index/index.hpp`
- `src/num/budgets.hpp`
- `src/num/center_region.cpp`
- `src/num/centers.cpp`
- `src/num/geometry.hpp`
- `src/num/geometry_internal.hpp`
- `src/num/lattice_bounds.hpp`
- `src/num/level.hpp`
- `src/num/module.cmake`
- `src/num/num.hpp`
- `src/num/orientation_certificate.hpp`
- `src/num/power_certificate.hpp`
- `src/num/power_checked.hpp`
- `src/num/predicates.cpp`
- `src/num/q4_weights.hpp`
- `src/num/roots.cpp`
- `src/num/roots.hpp`
- `src/num/sphere.cpp`
- `src/num/wide.hpp`
- `tests/cloud/cloud_test.cpp`
- `tests/cloud/tests.cmake`
- `tests/core/status_test.cpp`
- `tests/core/tests.cmake`
- `tests/index/borrowed_oracle.py`
- `tests/index/fraction_oracle.py`
- `tests/index/probe.cpp`
- `tests/index/tests.cmake`
- `tests/mutants/cloud.json`
- `tests/mutants/core.json`
- `tests/mutants/index.json`
- `tests/mutants/num.json`
- `tests/num/bounds_oracle.py`
- `tests/num/candidate_test.cpp`
- `tests/num/center_region_oracle.py`
- `tests/num/centers_oracle.py`
- `tests/num/checked_power_oracle.py`
- `tests/num/checked_power_support.hpp`
- `tests/num/checked_power_test.cpp`
- `tests/num/distance_oracle.py`
- `tests/num/distance_probe.cpp`
- `tests/num/distance_test.cpp`
- `tests/num/fraction_oracle.py`
- `tests/num/integer_test.cpp`
- `tests/num/lattice_bounds_test.cpp`
- `tests/num/orientation_certificate_oracle.py`
- `tests/num/orientation_certificate_test.cpp`
- `tests/num/power_certificate_oracle.py`
- `tests/num/power_certificate_test.cpp`
- `tests/num/power_reference.hpp`
- `tests/num/power_test.cpp`
- `tests/num/q3_candidate_test.cpp`
- `tests/num/q4_presentation_oracle.py`
- `tests/num/q4_presentation_test.cpp`
- `tests/num/tests.cmake`
- `tests/num/triangle_kind_oracle.py`
- `tests/support/tests.cmake`

Fichiers ajoutés (18) :

- `src/index/bounds.hpp`
- `src/num/center_view.hpp`
- `src/num/frame.hpp`
- `src/num/guard.cpp`
- `src/num/guard.hpp`
- `src/num/lattice.cpp`
- `src/num/lattice_internal.hpp`
- `src/num/local.cpp`
- `src/num/local.hpp`
- `tests/index/guarded_oracle.py`
- `tests/index/guarded_test.cpp`
- `tests/index/translation_oracle.py`
- `tests/num/frame_test.cpp`
- `tests/num/guard_test.cpp`
- `tests/num/lanes_test.cpp`
- `tests/num/local_reference.hpp`
- `tests/num/profile_values.hpp`
- `tests/num/translation_test.cpp`
