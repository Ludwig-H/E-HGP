# Contrat numérique en repère local dans le socle de la v12 — changements fichier par fichier

Base : `morsehgp3D_v12/` au commit `26b53648c`. Le code du socle (`src/`, `tests/`, `cmake/`, `tools/`,
`CMakeLists.txt`, `bench/index_io*`) est identique au HEAD `3d6c92c1f` : 191 fichiers comparés par empreinte de blob
le 7 octobre à 13 h 51 min 05 s UTC, aucun écart ; depuis la base, seuls docs, audits, reçus, `bench/data` et `microbancs/`
(qui compilent contre les sources de la v11, pas contre le socle) ont bougé. Copie de travail :
`scratchpad/v12_numerique/morsehgp3D_v12/`. Correctif complet : `patch_numerique.diff` (6 319 lignes, 82 fichiers,
3 640 insertions, 732 suppressions, chemins `a/morsehgp3D_v12/…` et `b/morsehgp3D_v12/…`) ; `git apply --check`
rend 0 sur le HEAD `3d6c92c1f` (contrôle en lecture seule, 13 h 49 min 29 s UTC). `README.md` et
`docs/ARCHITECTURE.md` ont changé au HEAD depuis la base : intégrer par le correctif, pas en recopiant ces deux
fichiers. Aucune écriture sous `/workspaces/E-HGP`, aucune commande git d'écriture, GCP non utilisé.

Cadre : `phase=exploration_v12_hors_registre`, `backend=cpu_reference`, `objet=full_pi0`,
`quantification=quantized_u21_input_only` (profils 24 et 32 admis à la construction, à qualifier selon D6),
`public_status=not_claimed`.

Principe tenu partout : **aucune valeur ne change**. Toutes les voies (native, certifiée, contrôlée, large) rendent la
valeur exacte ; seuls changent la largeur des intermédiaires, le choix de voie, les types de stockage au profil 32 et
les refus de profil. Les tailles de `Sphere` (144/160 octets) et de `Q4Candidate` (80) sont inchangées aux profils 21
et 24.

## 1. `src/num` — repère local, paliers, certificats, voies

| Fichier | Quoi | Pourquoi (contrat, constat) | Portes |
| --- | --- | --- | --- |
| `budgets.hpp` (réécrit) | `Budgets<B>` remplacé par `SpanBudgets<S>` (toutes les expressions du § 3, budgets mixtes 6s+11, 7s+14, 3s+10, 2s+7, milieu 5s+8/5s+6, réservoir 2s+4, fractions de centres 8s+10) ; paliers `Tier` étroit ≤ 16, moyen ≤ 24, large ≤ 33 (coupes proposées par le contrat, gardées : 16 = seuil de l'orientation 7s+9 et du côté gardé 6s+11 ; 24 = seuil du numérateur q3 5s+5 et du milieu 5s+6 ; 33 = fermetures à 2^32) ; `static_assert` des coupes ; `Lane`, `LaneCount` (compteurs logiques de voies) ; stockage `DomainBudget = SpanBudgets<kCoordBits>` réservé aux types (`CenterInt`, `CenterDen`, `SideInt`, `DotInt`, …), jamais à un choix de voie | § 3 « budgets en fonction de s », « trois voies », « paliers proposés » ; CST-0111, CST-0208 | `mhgp12_num_local_frame_tiers`, `mhgp12_num_unit_budgets` |
| `frame.hpp` (nouveau) | `Frame` : NUM-REPERE certifié (coin minimal, étendue `span_bits` = largeur de bit exacte, `s = 0` pour une étendue nulle), coordonnées jusqu'à 2^32 en `u64`, fermeture de boîte (`add_box`), refus au-delà de 2^32 ; `tier()` | § 2 NUM-REPERE, NUM-COUVERTURE ; CST-0204 (s = 33) | `mhgp12_num_local_frame_extents`, `_frame_refusals` |
| `power_certificate.hpp` | certificat de puissance **paramétré par son domaine t** (`power_certificate_i128(D, N, t)`), domaine maximal `power_domain_i128` (proposition par longueurs, décision par le certificat exact) ; l'ancien certificat global = cas t = B | § 3 « certificats liés à leur domaine » ; CST-0201 | `mhgp12_num_unit_certificate_*`, `mhgp12_num_local_guard_certificate` |
| `orientation_certificate.hpp` | même chose pour l'orientation (`orientation_certificate_i128(D, N, t)`, `orientation_domain_i128`) | CST-0201 | `mhgp12_num_unit_orientation_*`, `mhgp12_num_local_lane_orientation` |
| `geometry.hpp` | `Sphere`, `Q3Candidate`, `Q4Candidate` portent l'étendue de leur support de présentation et deux domaines de certificat (`CenterDomains`, exposants t sur un octet) à la place des booléens (rangés dans l'alignement : tailles inchangées à 21/24) ; accès `support_span()`, `tier()`, `power_domain()`, `orientation_domain()` ; `q3_power_i128_certified()` et `orientation_i128_certified()` gardent leur sens (domaine ≥ B) ; compteurs `LaneCount*` facultatifs sur les prédicats ; `compare_centers` documenté en deux temps ; `squared_distance` documentée NUM-REQUETE | § 2, § 3, § 4 | toutes les portes num |
| `geometry_internal.hpp` | `span_of`, `presentation_span`, `dot`/`cross` en i64 sous précondition d'étendue ≤ 30/31, `dot128`/`cross128` à toute étendue ; `product<Words>` générique (i64, i128, Wide) par `multiply_into` ; `as_i128` (conversion contrôlée), `store_numerator/denominator` (refus d'invariant si la valeur sort du stockage) ; `Exact` (entier exact de 320 bits à drapeau de dépassement collant) pour la voie large des constructions | « types reconstruits », « aucune conversion rétrécissante avant un builtin » | — |
| `center_view.hpp` (nouveau) | vue interne des coefficients (déplacée de `predicates.cpp`) : copies i128 quand elles tiennent, aucune copie aux profils 21/24 ; `power_lane` (voie par palier de u = max(s, t), puis certificat, puis contrôlée, puis large), `power_words` (largeur du repli par palier) | § 3 « trois voies par expression », « largeur fixée par le palier » | `mhgp12_num_local_lane_side` |
| `sphere.cpp` | fabriques : voie i64/i128 de la v11 jusqu'au palier moyen (s ≤ 24, chaque budget au plafond du palier), voie exacte `Exact` au palier large (25 ≤ s ≤ 32, profil 32) ; domaines de certificat calculés une fois ; niveau q4 à la largeur du palier ; compteur de voie sur `Q3Candidate::through` et `Q4Candidate::through` | § 3 (centres 5s+5, 4s+5, niveaux) | `mhgp12_num_local_lane_center`, portes num existantes |
| `q4_weights.hpp` | poids de présentation : largeur du repli au palier moyen (6·24+7) au lieu de 6B+7 ; version exacte au palier large | § 3 | `mhgp12_num_q4_presentation_*` |
| `predicates.cpp` (réécrit autour des voies) | puissance, côté, bornes de boîte, orientation avec centre, intérieur strict, milieu : voie par palier du repère support+arguments, certificat au domaine t réel de la requête, essai contrôlé à opérandes i128 (norme et facteurs déjà élargis), repli large à la largeur du palier ; **test du milieu en forme locale** 2N_j = D((a_j−o_j)+(b_j−o_j)) ; **distance carrée NUM-REQUETE** (u64 natif jusqu'à l'étendue 31, puis u64 contrôlé, puis u128) ; orientation 4 points et classification en i128 à toute étendue | CST-0114, CST-0110, § 3 | `mhgp12_num_local_lane_*`, `mhgp12_num_unit_*`, oracles Fraction num |
| `power_checked.hpp` | `checked_power_sum` à opérandes i128 (norme jusqu'à 3·2^66) ; surcharge i64 qui élargit | « aucune conversion rétrécissante avant un `__builtin_*_overflow` » | `mhgp12_num_unit_checked_*` |
| `centers.cpp` (réécrit) | **comparaison de centres en deux temps** : parties entières `floor(N_j/D) + a_j` (plancher mathématique), puis fractions par produits croisés r·D' contre r'·D, axe par axe, natifs si les longueurs le permettent, sinon largeur du palier (8s+10) ; coefficients hors i128 (profil 32) par division de `Big` | § 4 (contre-lu `f6f65a0d8`) | `mhgp12_num_centers_*`, `mhgp12_num_local_lane_centers_order` |
| `lattice_bounds.hpp`, `lattice.cpp` (nouveau), `lattice_internal.hpp` (nouveau) | `LatticeSphere` (voie générique, toute sphère) **sans centre absolu** : plancher de N_j/D en local, saturation, puis + a_j ; norme en i128 au profil 32 ; compteurs de voies | CST-0109 | `mhgp12_num_lattice_*`, `mhgp12_index_unit_lattice` |
| `guard.hpp`, `guard.cpp` (nouveaux) | **`CertifiedBall`** : seule fabrique, le certificat exact c ∈ conv(S) (q1, q2 ; q3 strictement aigu ; q4 poids de présentation > 0) ; **`GuardedSphere`** : pavé ouvert (m−2M, m+3M), site hors pavé extérieur sans arithmétique, boîte disjointe rejetée, point entier le plus proche **en local** saturé à la boîte, boîte non contenue → majorant +1 sans arithmétique (raffinée, jamais rejetée), coin lointain au budget mixte ; voie uniforme par boule : étroit natif, sinon **certificat au domaine s+2**, sinon contrôlée, sinon large (3 mots au palier moyen, 4 au large) ; `side_offset` pour les requêtes du catalogue en repère local ; `GuardLedger` | NUM-CERTIFIEE, NUM-GARDE ; CST-0108, CST-0109, CST-0201, CST-0111 | `mhgp12_num_local_certify`, `_guard_sites`, `_guard_boxes`, `_guard_certificate` |
| `local.hpp`, `local.cpp` (nouveaux) | `reservoir_distance` : distance du réservoir Σ(2x−lo−hi)² en repère de feuille (couverture contrôlée), i64 aux paliers étroit et moyen, i128 au large | CST-0208, NUM-COUVERTURE | `mhgp12_num_local_lane_reservoir` |
| `level.hpp` | `compare(Level, Level)` : i128 natif si les deux produits croisés tiennent en 127 bits, sinon 256, 384 ou 512 bits (largeurs des paliers pour 14s+20) | § 3 « comparaison de niveaux jusqu'à 512 bits à s = 33 » | `mhgp12_num_local_lane_levels`, `mhgp12_num_unit_levels` |
| `wide.hpp` | `multiply_into<W>` (produit à largeur fixée, ne parcourt que les mots utiles, refuse ce qui ne tient pas) ; `operator==` de valeur | largeur fixée par le palier | toutes les voies larges |
| `center_region.cpp` | lemme Z en repère local (coordonnées moins le coin minimal de {région, sites}) : formes affines en i64 aux paliers étroit et moyen, i128 au large ; plus aucun carré absolu de 2B+4 bits | § 3 (prédicats de feuille) ; nécessaire au profil 32 | `mhgp12_num_center_region_*` |
| `roots.hpp`, `roots.cpp` | borne des racines `R < 2^(B+65) ≤ 2^97` (au lieu de 89) : 16 termes et leurs unités restent sous 2^102 en i128 | profil 32 | `mhgp12_num_roots` |
| `num.hpp`, `module.cmake` | exposent `frame.hpp`, `guard.hpp`, `local.hpp` ; sources `lattice.cpp guard.cpp local.cpp` | — | — |

## 2. `src/index` — census gardé

| Fichier | Quoi | Pourquoi | Portes |
| --- | --- | --- | --- |
| `index.hpp` | `census(index, CertifiedBall, …)` et `CensusWorkspace::query(…, CertifiedBall, …)` ; `CensusLedger` gagne `lanes` et `guard_disjoint/partial/outside` | NUM-GARDE, CST-0108 (le type interdit de garder une boule non certifiée) | `mhgp12_index_guarded_*`, `mhgp12_index_guarded_fraction` |
| `bounds.hpp` (nouveau) | adaptateurs `GenericBounds` (LatticeSphere) et `GuardedBounds` (GuardedSphere), une préparation par parcours, compteurs reportés | — | — |
| `census.cpp`, `census_workspace.cpp` | parcours gabarisés sur les bornes : le même code sert les deux census ; les motifs des mutants existants restent uniques | — | mutants `index` |

## 3. `src/core`, `src/cloud`, construction

| Fichier | Quoi | Pourquoi | Portes |
| --- | --- | --- | --- |
| `src/core/types.hpp` | profils 21, 24 **et 32** ; `kCoordMax` calculé en u64 ; `i8` | étape 8 | `mhgp12_core_coord_bits_*`, `mhgp12_core_unit_types` |
| `CMakeLists.txt` | 32 admis ; jeton `mhgp12_coord_bits_32_differe` retiré ; 18 garde son jeton ; tout autre valeur `mhgp12_coord_bits_invalide` | étape 8 | `mhgp12_support_configure_coord_bits_18`, `_33` |
| `src/cloud/cloud.hpp` | `CoordWidth::max()` en u64 (2^32 − 1 au profil 32) | profil 32 | `mhgp12_cloud_unit_width` |
| `README.md`, `docs/ARCHITECTURE.md` (§ 7) | profils 21, 24, 32 ; 18 refusé | — | `mhgp12_style` |

La **clé de Morton normalisée** (coordonnées moins le minimum global, u64 si B_eff ≤ 21, u128 sinon) **n'est pas
appliquée** : voir `RAPPORT.md` § 4 et `patch_cle_normalisee.diff`. La clé livrée reste celle du socle, exacte et sans
troncature (63 bits à 21, 72 à 24, **96 bits en u128 à 32**), identité des sites par égalité de clé.

## 4. Portes ajoutées

| Porte | Contenu |
| --- | --- |
| `mhgp12_num_local_*` (16 groupes, `tests/num/{frame,lanes,translation,guard}_test.cpp`) | NUM-REPERE (s = 0, s = 22/25/33 aux fermetures 2^b, site extérieur, garde par requête s+2 contre union s+3) ; portes de palier avec compteurs (côté 16/17 et 19/20, bornes, orientation 16/17/18, milieu 24/25, distance 31/32, réservoir 24/25 et témoin s = 30, centres q3/q4 24/25, niveaux 127/128 bits, fractions de centres 15/17) ; invariance par translation des prédicats ; certification (témoin (419,0,0),(435,15,0),(434,14,0), triangle presque aligné, droit, q4 à poids nul) ; garde (contact à la sphère contre contact au pavé, bords ouverts, boule débordante, boîte disjointe, contact de coin, boîte partielle [0,200]^3, segment 0–1, petit triangle équilatéral où seul l'arrondi donne le minimum entier, comparaison contre la voie entière générique, 81 boîtes pour chacune de trois boules, 243 comparaisons) ; certificat CST-0201 (témoin s = 20 de Codex : D = 6h^4, N = (4,2,2)h^5, premier produit de 128 bits, valeur finale 151531357695222388613514543567625781250 exacte, voie large comptée ; support d'étendue 20 certifié à 21 mais pas à 22) |
| `mhgp12_index_guarded_{fixtures,uncertified,witnesses}` | census gardé = census générique (possédé et emprunté), compteurs de garde ; candidates non certifiées dans la voie générique ; boîte partielle ; témoin CST-0201 dans un census dès B ≥ 22 |
| `mhgp12_index_guarded_fraction` (+ `_opt`) | toute la matrice de l'index (1010/1013 requêtes) par la sonde `--guarded` : supports certifiables → mêmes populations que le census générique et juge Fraction ; non certifiables → `uncertified` |
| `mhgp12_index_translation` (+ `_opt`) | invariance par translation jugée à translation près : nuages de test translatés aux deux bords du domaine du profil (à 32 : [0, 2^32)), census générique et gardé, sites et populations rangés par PointId |
| `mhgp12_cloud_unit_identity` | CST-0202 : trois positions dont deux auraient la même clé tronquée → 3 sites ; vrais doublons intercalés (A,1),(B,2),(A,3) → 2 sites ; permutations à PointId stables |
| `mhgp12_support_configure_coord_bits_33`, `mhgp12_core_coord_bits_33_refusal` | remplacent les refus de 32 : 33 refusé à la configuration et à la compilation |

## 5. Portes existantes adaptées (sans changement d'attendu aux profils 21 et 24)

- Renommages d'API : `Budgets<21>` → `SpanBudgets<21>` (mêmes valeurs), `Budget::` → `DomainBudget::`.
- Profil 32 ajouté aux en-têtes acceptés des oracles Python (`fraction`, `bounds`, `center_region`, `centers`, `distance`,
  `checked_power`, `power_certificate`, `orientation_certificate`, `q4_presentation`, `triangle_kind`, index `fraction`
  et `borrowed`) ; auto-tests des modèles inchangés (lignes gravées intactes).
- Faits propres au profil, complétés pour 32 sans toucher 21/24 : tailles (`profile_values.hpp`), longueur 2m^6 (193),
  H = 4L^6 (194), strates de `bounds_oracle.py` (3417/356/32), inventaire de `borrowed_oracle.py` (1013), voie
  contrôlée impossible pour des coefficients hors i128 (`checked_power_oracle.py`, `power_certificate_oracle.py`,
  `power_certificate_test.cpp`, `checked_power_test.cpp`), centres lointains (w = 100000 à 32), refus hors domaine
  joués à la largeur déclarée 31 (tout u32 est dans le domaine du profil 32), largeurs décimale et hexadécimale de
  `distance_oracle.py`, banc `index_io_test.py` (pas de refus hors domaine possible à 32 : sept appels, quatre refus).
- Références de test rendues indépendantes du stockage (`power_reference.hpp`, `checked_power_support.hpp`, références
  d'orientation et de niveau q3, juges du test des bornes entières) : elles débordaient i128 au profil 32.
- Mutants : motifs mis à jour (même mutation sémantique) pour 25 mutants num, 2 index et 3 core ; deux mutants num
  changent seulement de fichier, motif identique (`lattice_coin_proche`, `lattice_puissance_sans_ancre_z` :
  `predicates.cpp` → `lattice.cpp`) ; `profil_32_admis` devient `profil_33_admis`.

## 6. Mutants ajoutés

num (15) : `certificat_du_support_seul`, `garde_un_bit_trop_etroite`, `recensement_avant_certificat`,
`garde_boite_partielle_rejetee`, `pave_haut_trop_court`, `pave_bas_trop_court`, `garde_plancher_au_lieu_du_plus_proche`,
`garde_coin_proche`, `reservoir_i64_partout`, `distance_somme_u64_debordante` (profil 32), `milieu_facteur_deux_perdu`,
`centres_partie_entiere_ignoree`, `domaine_puissance_sous_estime`, `repere_minimum_inverse`, `palier_etroit_dix_sept`
(attendu à la construction). index (3) : `census_garde_sans_garde`, `census_garde_emprunte_sans_garde`,
`garde_compteur_partiel_perdu`. cloud (1) : `identite_par_cle_tronquee`. Planchers : num 76, index 18, cloud 17.
