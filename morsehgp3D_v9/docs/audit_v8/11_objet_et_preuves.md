# Lentille 11/12 — Objet mathématique, spécification et preuves (v8 → v9), version contre-vérifiée

Contre-vérification adversariale du rapport `11_objet_et_preuves.md`, 22 septembre 2026, en lecture seule. Cadre : `phase=exploration_v8_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only` (moteur porté à `quantized_u18_input_only` par a74e90f2), `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`. GCP non utilisé. Aucune compilation, aucun CTest, aucun benchmark, aucune commande git mutante.

Référence : worktree détaché à `origin/main` **12294241**. Tout ce qui vient de l'état non commis du worktree partagé est marqué **[NC]**.

Statuts : **prouvé** = preuve écrite relue et fixture gravée ; **testé** = porte bornée ou oracle à petite taille ; **mesuré** = reçu épinglé (commit, sha256, sorties) ; **proposé** = note sans implémentation ou sans qualification ; **manquant** = absent du dépôt ; **non vérifiable** = chiffre sans reçu. Chaque correction apportée au rapport d'origine est signalée par **(corrigé)** ou **(ajouté)** et récapitulée à la dernière section.

## 1. Périmètre lu

### Lu intégralement (état publié 12294241)

| Fichier | Rôle pour cette lentille |
| --- | --- |
| `morsehgp3D_v8/audits/FONDEMENTS_ET_OBJET.md` | objet visé, réduction FULL, seuils, borne de sortie |
| `morsehgp3D_v8/audits/VERROUS_MATHEMATIQUES_20260914.md` (199 l.) | verrous de l'auditeur B |
| `morsehgp3D_v8/audits/q34_global_contract_20260921/SUPPORT_ET_CITRON.md` (189 l., sha256 `6bcaf398…`) | citron et complétude du front q3/q4, preuve refaite |
| `morsehgp3D_v8/audits/FACETTES_SILENCIEUSES_REPRISE_V7_20260919.md` | contrat de portage FULL v7 → v8 |
| `morsehgp3D_v8/audits/CERTIFICATS_COLLECTIFS_AVANT_ATLAS_20260922.md` | certificat collectif d'arête |
| `morsehgp3D_v8/audits/lidar_rectangles_20260922/README.md` (107 l.) **(ajouté)** | preuve native 1 mm de l'auditeur à HEAD |
| `morsehgp3D_v8/docs/Q3_CERTIFICAT_ATLAS_20260921.md` (98 l.) | graines q3 certifiées par l'atlas |
| `morsehgp3D_v8/docs/ELARGISSEMENT_18_BITS_20260922.md` (125 l.) | bornes 18 bits |
| `morsehgp3D_v8/docs/Q3_Q4_CERTIFICAT_COLLECTIF_20260920.md` (183 l.) | corde resserrée, minimum collectif |
| `morsehgp3D_v8/src/spindle/predicates.hpp`, `src/lanes/exact_ball.{hpp,cpp}`, `src/core/types.hpp`, `src/pipeline/wspd_q34.hpp`, `src/pipeline/prepared_cloud.hpp` | prédicats, clés, domaine, contrat du flux |
| `morsehgp3D_v8/tests/wspd_q34_mutations.py`, `tests/q34_indexed_mutations.py` **(ajouté)**, `CMakeLists.txt` l. 550–600 **(ajouté)** | mutants compilés et leur statut CTest |
| `[NC] morsehgp3D_v8/docs/REPRISE_U18_ET_ATLAS_SATURANT_20260922.md` (126 l.), `[NC] receipts/u18_resume_20260922/preflight_center_oracle/README.md`, `[NC] receipts/u18_resume_20260922/release{,_r2}/CTEST.xml` **(ajouté)** | tranche en suspens |

### Lu partiellement

`docs/Q3_Q4_REJET_FAMILIAL_20260920.md` (§§1–2), `docs/Q3_Q4_OBJETS_ET_STRATEGIE_20260914.md` (§§3–5), `docs/Q3_Q4_CANDIDATS_PAR_SEED_20260920.md` (l. 20–100), `docs/FAUSSES_PISTES.md` (l. 30–40, 386–396), `docs/P0_TEMOINS_HERITES_Q2.md` (théorème H), `audits/P0_TUBES_ET_RANGS.md` (§1), `audits/q34_global_contract_20260921/PREFIXES_TEMOINS.md` (exemple), `audits/q34_global_contract_20260921/oracle.py` (l. 340–352), `audits/DIALOGUE_AUDITEUR_B.md` (l. 128–155, 585–612), `audits/DIALOGUE_COURANT.md` (grep), `docs/AUDIT_REPRISE_DEVELOPPEUR_20260921.md` (l. 50–100), `docs/AUDIT_V7_SYNTHESE.md` (l. 38–62), `docs/CONTRAT_TRAMES_SEMANTICKITTI_20260921.md` (l. 38–50), `docs/PRECISION_FLOAT32_ET_GRILLE_20260921.md` (grep), `docs/SPECIFICATION_MORSEHGP3D.md` (grep « multiplicit »), `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md` (l. 36–42, 120–134), `docs/math/INCIDENCES_SILENCIEUSES_GAMMA.md` (§5.3.1 complet), `morsehgp3D_v7/audits/NIVEAUX_ET_CERTIFICAT_HGP_COURANT.md` (l. 25–32), `AGENTS.md` (l. 10–18 et grep), `tests/fixtures/regressions/` (liste et quatre fixtures).

### Manuscrit

`docs/references/MANUSCRIT_THESE_HAUSEUX.pdf`, pages PDF 110–116 extraites par `pdftotext` : Déf. 25–29, Fait 12, Prop. 5–6, Th. 4. Déf. 28 confirmée (intérieur de la plus petite boule englobante vide de points extérieurs au simplexe). Les autres pages n'ont pas été relues.

### Reçus ouverts et hachés

| Reçu | sha256 (préfixe) | Contenu vérifié |
| --- | --- | --- |
| `audits/q34_stream_crosscheck_spatial_20260921/Q4_BILATERAL_SPATIAL.json` | `e55d88d0` | 7 morceaux, moteur d6e1bd9e, K5 |
| `…/Q4_BILATERAL_SPATIAL_K10.json` | `b7423d9c` | idem, K10 |
| `audits/q34_stream_crosscheck_t32_8k_20260921/Q34_STREAM_CROSSCHECK_T32_8K.json` | `367ed209` | préfixe 8 000, moteur d1b4dbc6 |
| `audits/q34_stream_crosscheck_t3{2,3,4}_20260921/README.md`, `q3_stream_crosscheck_20260921`, `q4_stream_crosscheck_20260921` **(ajouté)** | — | exhaustifs 1k–4k à 4dbe3024, d1b4dbc6, 2629a536, d6e1bd9e |
| `audits/collective_edge_20260922/RESULTS.json` | `5d98ac93` | corpus exact, base a74e90f2 |
| `receipts/audit_v7_20260913/MATH_CHECKS.json` | `c63fbb63` | borne de sortie |
| `receipts/q34_indexed_20260921/mutations_final/compiled_foznl6jv/COMPLETION.json` **(ajouté)** | — | mutants `q4_wrong_alpha`, `lemon_contact_counted_inside`, `admitted_lane_recounted_in_children` tués |
| `receipts/lidar_global_20260921/mutations/compiled_*/COMPLETION.json` **(ajouté)** | — | mutants d'indépendance des voies tués |
| `receipts/ground_phase1_20260921/README.md` | — | identité appariée, sonde 0e2c18ca |

### Non lu

`audits/COORDINATION_MORSEHGP3D_V8.md` (canal constructeur/auditeurs), l'essentiel de `DIALOGUE_AUDITEUR_B.md`, les notes `docs/P0_*` hors théorème H, les notes float32, `morsehgp3D_v7/audits/S1_COURANT.md`, `FRONT_ET_TEMOINS_COURANT.md`, `PLATEAUX_FULL_ET_ANCRES.md`, la plupart des sources `tests/*.cpp`, le diff [NC] complet hors `types.hpp`, `exact_ball.{hpp,cpp}`, `ELARGISSEMENT…md`, `AGENTS.md` et `CMakeLists.txt`.

Comptage de commits : `git log 2b658cbe^..12294241 -- morsehgp3D_v8` donne 169 commits ; `git rev-list --count 2b658cbe..12294241` donne 200 commits tous chemins confondus. Aucune méthode essayée ne retrouve les 158 du brief : écart **non résolu**. Aucun commit de la plage ne touche `morsehgp3D_v7/` : les pins v7 à 12294241 sont ceux de l'ouverture v8.

## 2. Ce qui a été fait (vu par l'objet et les preuves)

| Date | Commits | Apport mathématique |
| --- | --- | --- |
| 13 sept. | 2b658cbe, f375d2c6, e409aa46 | Audit d'ouverture : objet FULL reformulé (`FONDEMENTS_ET_OBJET.md`), seuils $h_q=K_{\max}+2-q$, E5 et graphe induit rappelés, recalcul rationnel de la borne de sortie quadratique (`MATH_CHECKS.json`). |
| 13 sept. | dc246d6b, e9e97e64, 7f4ba045, 90d22425 | Lemme des tubes (cône à marge stricte, $D\geq10R$), moments collectifs par blocs, crédits axiaux additifs (voie q2). |
| 14 sept. | 7009ec8b, 1ca8f62d | `VERROUS_MATHEMATIQUES_20260914.md` (B) : seuils hors position générale, élimination par voie distincte de la rétention par boule, fuseaux W3/W4, E5, décision de domaine non régulier demandée. |
| 14–17 sept. | f7edd646 … 3e94c868 | Chaîne q2 front + census contre force brute ; théorème H (témoins hérités transmis par **rangs**, pas par compte). |
| 19–20 sept. | ae98dbfa, 0d965fae, 19cee4ac, 4ccf8431, c92aad13 | Audit des facettes silencieuses : reprendre la sémantique de `full_ball_tower.hpp` v7 ; régression obligatoire A–E. |
| 20 sept. | 785d0589, 77f659e4, 8d0a0f0f, 4215dd16, ddaafdc0 | Clé `ExactBall` commune aux trois arités ; candidats q3/q4 par graine ; certificat familial ; corde resserrée et minimum collectif du pool. |
| 20 sept. | 66b1551f, c051bdb0, 31b0243a, f07fbd8c | Carte des centres q4, balayages locaux, couches convexes duales, fenêtre de faible profondeur $[L,U]$. |
| 21 sept. | 4dbe3024, 462c29a1, d9251d10, 413f70cf, 46fac512, d1b4dbc6, 2629a536, d6e1bd9e | Flux global q3/q4 ; preuve indépendante du citron et du front (`SUPPORT_ET_CITRON.md`) ; exhaustifs indépendants 1k–4k à quatre moteurs successifs et 8k à d1b4dbc6 ; mutants `q4_wrong_alpha` et `lemon_contact_counted_inside` tués (reçu `q34_indexed_20260921`). |
| 21 sept. | 6e279d6a, e2b09f94 | Protocole bilatéral de B sur les sept morceaux de la scène 0 (jusqu'à 119 142 sites) : validité de chaque record **q4**, complétude par échantillon. |
| 21 sept. | 36724438 … a005f8aa | Briques float32 sans perte (prédicats, boules, clés, ordre des racines q4 par déterminant de degré 5). |
| 21 sept. | 0948d2d0 | Rejet exact des graines q3 par l'atlas de centres q4. |
| 22 sept. | a74e90f2 | Élargissement du moteur à 18 bits : `Point3` passe de `uint16_t` à `int32_t`, bornes réécrites avec $M=262143$ ; les 42 portes extrêmes 18 bits restent **[NC]**. |
| 22 sept. | bf73e194 … 39c6b824 | Certificat collectif d'**arête** (paires et triangles), prototype d'audit, base a74e90f2. |
| 22 sept. | b1c91e44 … 12294241 | Cascade de filtres de rectangles ; preuve native sur trois trames entières 1 mm (`lidar_rectangles_20260922`) **(ajouté)**. |
| 22 sept. [NC] | non commis | Garde de domaine aux fabriques publiques et à `Box3`, juge du centre q3, arrêt saturant de l'atlas (`saturate_deep`, désactivé), 42 portes 18 bits, reçu d'identité u16 `ground_18bits_20260922/u16_identity`. |

## 3. État par composant

| Composant | Statut | Preuve |
| --- | --- | --- |
| Objet normatif : tour $\pi_{0}(L_k(a))$ pour $1\leq k\leq K_{\mathrm{eff}}$, isolés, naissances, multifusions, verticales | **manquant en v8** | `src/forest/`, `src/io/`, `src/cloud/` ne contiennent que `.gitkeep` ; `wspd_q34.hpp:100–101` ; `AUDIT_REPRISE_DEVELOPPEUR_20260921.md:56–57, 69` |
| Réduction FULL v7 : naissances = minima Gabriel de cardinal K, multifusions aux niveaux Gabriel K+1 avec parents | **théorème conditionnel** (prémisses régulières : supports essentiels uniques, intrus stricts, pas d'extra-shell pertinente), non porté **(corrigé)** | `STATUT_PREUVES_ET_HEURISTIQUES.md:131–134` (`conditional_theorem`) ; `AUDIT_NIVEAUX_GABRIEL_20260905.md` §1.1 ; `NIVEAUX_ET_CERTIFICAT_HGP_COURANT.md` |
| Réfutation de Prop. 6 / Th. 5 (E5) et du graphe induit sur les minima | **prouvé** (registre `false_in_general`, fixture racine) | `tests/fixtures/regressions/gabriel_point_set_counterexample.json` ; registre l. 40–41 ; `FONDEMENTS_ET_OBJET.md` §« Gabriel » ; Prop. 6 relue (PDF 115–116) |
| Régression silencieuse A=(0,1,0), B=(2,5,0), C=(4,1,0), D=(1,0,0), E=(3,0,0), Kmax=2 | **proposé** (obligatoire, non gravée) | `FACETTES_SILENCIEUSES_REPRISE_V7_20260919.md:34–37` ; absente de `morsehgp3D_v8/tests`, des fixtures racine et, par grep des coordonnées, des tests v7 |
| Seuils $h_q=K_{\max}+2-q$ (10/9/8 à K10, 5/4/3 à K5) hors position générale | **prouvé** (autorité racine `proved_here`) | Th. 4.2 et Cor. 4.3, `INCIDENCES_SILENCIEUSES_GAMMA.md` §5.3.1 ; registre l. 120 ; fixture `higher_rank_prune_does_not_certify_star_regularity.json`. Réserve **(ajouté)** : Cor. 4.3 ne compose qu'avec un refus explicite `unsupported_rank_relevant_extra_shell_degeneracy` des extra-shells pertinentes ; les « 250 nuages » de `VERROUS…md:32` sont **non vérifiables** |
| Élimination par voie distincte de l'inertie de la boule ; voies q2/q3/q4 indépendantes | **prouvé + testé + mutants tués** | `SUPPORT_ET_CITRON.md:78–85` ; `tests/q3_q4_owner_independence_gate.py` (6 cas, CTest) ; mutants `q4_gated_by_q3_acceptance`, `q4_gated_by_q3_front_mask` tués, reçu `receipts/lidar_global_20260921/mutations/` **(ajouté)** |
| Lemme du citron, $\alpha_3=3$, $\alpha_4=2$, sans hypothèse sur la coquille | **prouvé** | `SUPPORT_ET_CITRON.md` §2, refait ci-dessous ; `predicates.hpp:124–126, 157–158` ; contacts gravés `wspd_q34_gate.cpp:361–362`, `q34_witness_search_gate.cpp:272`, mutant `lemon_contact_counted_inside` tué (reçu `q34_indexed_20260921`) ; contre-fixtures de nécessité seulement dans `oracle.py:344–352` |
| Contre-exemple « $\alpha_3$ appliqué à q4 » | **prouvé + gravé + mutant tué** ; mutant **désactivé** dans CTest à HEAD **(corrigé)** | `DIALOGUE_AUDITEUR_B.md:141–150` ; fixture translatée de (+100,+100,+100) dans `q34_witness_search_gate.cpp:282–293`, rejugée par le solveur de Gram de la porte ; mutant compilé `q4_wrong_alpha` (`tests/q34_indexed_mutations.py:20`) tué au reçu `q34_indexed_20260921/mutations_final/compiled_foznl6jv` (source `q34_witness_search.cpp` `14f12022…`, soit d1b4dbc6) ; `mhgp8_q34_indexed_witness_mutations` enregistré `DISABLED` depuis 2629a536 (`CMakeLists.txt:592–600`), encore désactivé dans la capture [NC] `release_r2` |
| Régions diamétrales W2, prédicats de boîtes | **prouvé + testé** | `predicates.hpp:81–122, 161–249` ; « 5 006 triplets et 42 égalités » de `VERROUS…md:84` **non vérifiables** |
| Complétude du front q3/q4 | **prouvé** sous contrats locaux ; **testé** | `SUPPORT_ET_CITRON.md` §4 ; mutant `parallel_refused_range_dropped` enregistré (`wspd_q34_mutations.py:57–60`), **sans reçu de mort commis à HEAD** **(corrigé)** ; passe dans la capture [NC] `release_r2` |
| Propriété unique et graine q4 canonique | **prouvé** | `SUPPORT_ET_CITRON.md:19–22, 103–118` ; identité de face aiguë refaite ci-dessous |
| Identité des boules : clé primitive $(A,B_x,B_y,B_z,C)$, $A>0$, pgcd 1 | **prouvé + testé** (à u16 seulement à HEAD) **(corrigé)** | `exact_ball.cpp:72–150` ; `exact_ball_gate.cpp:166–189` (sphère de 30 sites, clé `{1,-40,-40,-40,1175}` recalculée) ; les fixtures extrêmes de la porte sont en `uint16_t`/65535, aucune à 262 143 à HEAD |
| Ordre des niveaux (rayons carrés exacts entre arités) | **manquant** | `Q3_Q4_CANDIDATS_PAR_SEED_20260920.md:32–34` ; `FAUSSES_PISTES.md:35–39` ; aucune comparaison de rayons dans `src/` (grep) |
| Catalogue canonique dédoublonné, $q_{\min}$, intérieurs q3/q4 | **manquant** | `wspd_q34.hpp:113–114` ; `Q3_Q4_CANDIDATS…md:89–90` ; q2 hors de l'appel (`AUDIT_REPRISE…md:64, 94`) |
| Certificat familial q3/q4 | **prouvé + testé** | `Q3_Q4_REJET_FAMILIAL_20260920.md` §§1–2 ; reçu `receipts/q34_pruning_20260920` |
| Corde resserrée et minimum collectif du pool | **prouvé + testé** | `Q3_Q4_CERTIFICAT_COLLECTIF_20260920.md:35–55` ; reçu `receipts/q34_collective_20260920` ; trois mutants compilés annoncés tués (l. 158) |
| Certificat collectif d'arête | **prouvé + testé en prototype**, **proposé** pour le moteur | `CERTIFICATS_COLLECTIFS_AVANT_ATLAS_20260922.md` §§1–5 ; `collective_edge_20260922/RESULTS.json` ; hors CMake ; contrôles natifs 1 mm dans `lidar_rectangles_20260922` §5 (archive de résultats hors dépôt) |
| Fenêtre $[L,U]$, au plus $2H-2$ IDs triés | **prouvé + testé** | `Q4_FENETRE_DE_FAIBLE_PROFONDEUR_20260920.md:22–56` |
| Couches convexes duales | **prouvé + testé** | `Q4_COUCHES_DUALES_20260920.md:21–53` |
| Certificat d'atlas pour q3 | **prouvé** (note) + **testé** (oracle rationnel) + identité appariée ; mutant $K-2$ **sans reçu commis** ; pas de juge unitaire du centre ni de fixture d'égalité à HEAD **(corrigé)** | `Q3_CERTIFICAT_ATLAS_20260921.md:28–90` ; `wspd_q34.cpp:546` ; aucun test ne référence `q3_center` ou `certified_inside_count` ; `ground_phase1_20260921/README.md:11–12` |
| Saturation de l'atlas à $K-1$ (`saturate_deep`) | **proposé [NC]** | `REPRISE_U18…md:37–59` ; préflight 78/79, capture `release` 134/136, capture `release_r2` 136/136 exécutés (3 désactivés) **(ajouté)** |
| Lemme des tubes ($D\geq10R$ ; marges $1/4$, $1/8$, $41/128$) | **prouvé** | `P0_TUBES_ET_RANGS.md:22–48`, borne de $\Xi$ refaite ; attaque de B à $D=10R$ **non vérifiable** |
| Théorème H | **prouvé + testé** | `P0_TEMOINS_HERITES_Q2.md:36–43` ; fixture « compte transmis » (`FAUSSES_PISTES.md:392`) |
| Invariant d'état des préfixes de témoins | **prouvé**, **proposé** pour le produit | `PREFIXES_TEMOINS.md:19–20` ($H=74$, $\Xi=400$ puis $H=-76$) |
| Borne de sortie quadratique à K fixé | **prouvé** (v7) + contrôle fini | `full_output_growth.md` ; `MATH_CHECKS.json` |
| Bornes arithmétiques 18 bits | **prouvé** en commentaires ; **non testé aux extrêmes 18 bits à HEAD** ; domaine non imposé hors `prepare_cloud` **(corrigé)** | `exact_ball.cpp:97–100, 134–135, 148` ; `q4_local_partition.cpp:143–145, 242–244` ; `require_valid_box` ne vérifie que l'inversion (`types.hpp:86–90`) ; correctif et 42 portes [NC] |
| Dégénérescences : pas de jitter ; doublons et hors-plage refusés ; coquilles complètes non bornées | **testé** | `prepared_cloud.hpp:42–46` ; `cloud_owner_gate.cpp:229–253` (collision (0,1,0)/(0,0,65536)) |
| Quotient des plateaux, statut `unsupported_degeneracy` | **manquant en v8** ; fixtures et statut nommé **existent à la racine** **(corrigé)** | grep `unsupported`/`degenera` vide dans `src/` hors commentaires ; `VERROUS…md` §5 ouvert ; `tests/fixtures/regressions/delaunay_gabriel_unsupported_degeneracy_right_triangle.json` (attendu `unsupported_degeneracy`), `gabriel_carrier_strict_interior_extra_shell_matrix.json` ; statut `unsupported_rank_relevant_extra_shell_degeneracy` nommé par Cor. 4.3 |

### Vérifications refaites dans cette contre-vérification (à la main, sans exécution)

- **Citron.** Identité de variance $R^{2}=\sum_{i<j}\lambda_i\lambda_j\lVert v_i-v_j\rVert^{2}\leq\frac{q-1}{2q}D^{2}$, puis $\lVert c-m\rVert^{2}\leq\frac{q-2}{4q}D^{2}$. Avec $u=z-m$, $v=c-m$ : $\Pi_B(z)=-H-2u\cdot v$ et $(z-a)\times(b-z)=2u\times w$ où $w=(b-a)/2$, d'où $\Xi=D^{2}\lVert u_{\perp}\rVert^{2}$. On obtient $\Pi_B(z)\leq-H+\sqrt{\Xi(q-2)/q}$, donc $\alpha_3=3$ et $\alpha_4=2$. Conforme à `SUPPORT_ET_CITRON.md:36–55`.
- **Fixtures u16 du citron**, recalculées : contact q3 $c=(34,32,32)$, $R^{2}=24$, $H=12$, $\Xi=432=3H^{2}$ ; contact q4 $c=(33,33,27)$, $R^{2}=27$, $H=12$, $\Xi=288=2H^{2}$ ; arête non maximale $c=(36,38,30)$, $R^{2}=100$, $H=27$, $\Xi=1296$, $\Pi=21$ ; triangle obtus $c=(38,15,30)$, $R^{2}=289$, $H=48$, $\Xi=4096$, $\Pi=72$. Tout conforme.
- **$\alpha_3$ en q4.** $z-a=(28,-12,-12)$, $b-z=(32,12,12)$ : $H=608$, produit vectoriel $(0,-720,720)$, $\Xi=1036800$, $2H^{2}=739328$, $3H^{2}=1108992$. Conforme ; la porte teste $4H=2432$ et $16\Xi=16588800$.
- **Face aiguë en q4.** $\sum_i\lambda_iH(v_i)=-2\lVert c-m\rVert^{2}<0$. Conforme à `SUPPORT_ET_CITRON.md:109`.
- **Covers.** q3 : $\lVert z-m\rVert\leq D/(2\sqrt{3})+D/\sqrt{3}$, donc $4\lVert z-m\rVert^{2}\leq3D^{2}$. q4 : $(1+\sqrt{3})^{2}/8=(2+\sqrt{3})/4$, donc $4\lVert z-m\rVert^{2}\leq(2+\sqrt{3})D^{2}<4D^{2}$. Conforme.
- **Corde.** $R_{\mu}^{2}=R_0^{2}+\mu^{2}/(4G)\leq3D/8$ donne $2\mu^{2}\leq D(3G-2EX)$ ($D$ carré). Borne resserrée $\mu^{2}\leq G(D-2r)^{2}/(D-r)=DV^{2}/T$. Écart $J/2-DV^{2}/T=DG(4G-3EX)/(2T)\geq0$. Conforme.
- **Collectif d'arête.** Les intérieurs forment une couverture par sommets du graphe des paires certifiées ; un triangle certifié impose au moins deux intérieurs. Conforme.
- **Tubes.** Identité de Lagrange avec $u_{\perp}\cdot v_{\perp}\geq-st$ et $xy-st>0$ : $\Xi\leq(xt+sy)^{2}\leq(\rho+1/4)^{2}x^{2}y^{2}$ ; marges q3 $27/16-25/16=1/8$ et q4 $338/256-1=41/128$. Conforme.
- **Atlas.** $4Q\,\Pi(z;c)=Q c_{0}+f_x\alpha+f_y\beta$ refait pour $c=m+(\alpha A+\beta B)/(2Q)$. Un centre q3 possédé vérifie $\lVert c-m\rVert^{2}\leq\lvert ab\rvert^{2}/12<\lvert ab\rvert^{2}/8$, et la boule q3 tient dans le cover q4 ($\sqrt{3}/2\leq1$). Les cellules `Outside` (`q4_local_partition.cpp:207–229`) ne certifient rien. Sûr.

## 4. Chiffres clés

| Grandeur | Valeur | Source épinglée | Réserve |
| --- | --- | --- | --- |
| Seuils de voie K10 / K5 | 10/9/8 ; 5/4/3 | `FONDEMENTS_ET_OBJET.md:138–141` ; Cor. 4.3 | valent pour $K_{\mathrm{eff}}$, avec $\max(\cdot,0)$ |
| Coefficients du citron | $\alpha_3=3$, $\alpha_4=2$ | `SUPPORT_ET_CITRON.md:54` ; `predicates.hpp:124–126` | support positif + arête maximale |
| Borne de rayon d'un support positif | $R^{2}\leq\frac{q-1}{2q}D^{2}$ | `SUPPORT_ET_CITRON.md:38` | D = diamètre du support |
| Cover q3 / q4 | $4\lVert z-m\rVert^{2}\leq3D^{2}$ ; $\leq(2+\sqrt{3})D^{2}$, surcouvert par $4D^{2}$ | `Q3_Q4_OBJETS_ET_STRATEGIE_20260914.md:69` | coefficient 3 faux en q4 |
| Borne de corde q4 | $2\mu^{2}\leq J=D(3G-2EX)$, $J\geq DG/3$ ; resserrée $\mu^{2}\leq DV^{2}/T$ | `Q3_Q4_REJET_FAMILIAL_20260920.md:104–108` ; `Q3_Q4_CERTIFICAT_COLLECTIF_20260920.md:43–51` | D est une distance au carré dans ces notes |
| Lemme des tubes | $D\geq10R$ ; faux crédits dès $D/R\approx7{,}6$ (q3), 6,3 (q2), 6,1 (q4) | `VERROUS_MATHEMATIQUES_20260914.md:188–199` | attaque sans reçu : non vérifiable |
| Domaine des coordonnées | $[0,262143]$ | `types.hpp:21–29` | imposé seulement par `prepare_cloud` à HEAD |
| Bornes 18 bits | puissance q3 $<2^{117}$ ; atlas i64 $<2^{62}$ avec $Q=2^{20}$ | `exact_ball.cpp:100, 135, 148` ; `q4_local_partition.cpp:242–244` | marge d'un bit ; non testées aux extrêmes 18 bits à HEAD |
| Borne de sortie FULL | famille rationnelle : $m^{2}$ feuilles sur $N=2m+K-2$ ; contrôles entiers u16 à K=2 : $(m+1)^{2}$ naissances croisées sur $2(m+1)$ sites, 9/25/81/289 aux tailles 6/10/18/34 ($m=2,4,8,16$) ; 1 360 labels, 34 720 et 3 740 puissances **(corrigé)** | `full_output_growth.md:29–37` ; `MATH_CHECKS.json` (`c63fbb63…`) | contrôle fini |
| Exhaustif indépendant, préfixe LiDAR 8 000 sites (scan 0, avec sol), K5 | 93 914 records q3 (multiensemble) et 10 756 boules q4 distinctes identiques ; 10 756 records q4 | `q34_stream_crosscheck_t32_8k_20260921/README.md:38`, JSON `367ed209…`, moteur d1b4dbc6 | une taille, un scan, coquilles non comparées |
| Idem K10 | 409 195 q3 et 116 985 boules q4 identiques ; **116 988 records q4** **(corrigé)** | idem l. 39 | 3 présentations en double |
| Exhaustifs 1k–4k | identités q3/q4 à 4dbe3024, d1b4dbc6, 2629a536, d6e1bd9e **(ajouté)** | `q3_stream_crosscheck_20260921`, `q4_stream_crosscheck_20260921`, `q34_stream_crosscheck_t3{2,3,4}_20260921` | oracles de correction, pas d'échelle |
| Validité bilatérale q4, scène 0 entière (119 142 sites), K5 | 190 405 records, tous valides ; 187 142 boules distinctes (clé, profondeur) | `Q4_BILATERAL_SPATIAL.json` (`e55d88d0…`), d6e1bd9e | complétude : 2 000 paires, 61 conservées, 1 boule énumérée |
| Idem K10 | 2 158 063 records valides ; 2 137 403 distinctes | `Q4_BILATERAL_SPATIAL_K10.json` (`b7423d9c…`) | 52 paires conservées, 22 boules énumérées |
| Doublons du flux q4, scène 0 | 3 263 (1,71 %) à K5 ; 20 660 (0,96 %) à K10 | calculé sur les deux reçus | B les lit comme plateaux cosphériques ; le reçu ne distingue pas un support répété d'un second support |
| Records q3, scène 0 entière | 1 252 577 (K5) ; 6 221 665 (K10) | champ `rows[].probe.output.q3` des reçus bilatéraux **(corrigé)** | présentations ; **aucun juge q3 indépendant à cette échelle** |
| Quart x+y+ (29 926 sites), K5 | 28 854 records q4 pour 28 481 boules distinctes (373 doublons) | `Q4_BILATERAL_SPATIAL.json`, ligne `quarter_xpos_ypos` **(corrigé)** | mesuré |
| Exhaustifs du quart et de la moitié x+ | 270 202 q3 et égalité des 28 481 q4 avec l'énumération indépendante (quart) ; 528 575 q3 (moitié x+) | `DIALOGUE_AUDITEUR_B.md:593–601` | « ligne observée » ; runner `run_spatial_q3_large.py` commis sans JSON : non vérifiable |
| Collectif d'arête, corpus exact | 24 nuages ; 4 891 boules ; 68 474 puissances ; 8 736 requêtes (3 956 non vides) ; 280 rejets individuels ; 521/522 avec paires/triangles ; 242 supplémentaires dont 123 non vides ; 0 faux minorant | `CERTIFICATS_COLLECTIFS…md:140–147` ; `RESULTS.json` (`5d98ac93…`), base a74e90f2 | prototype hors moteur |
| Fixture triangles collectifs | 0 crédit individuel, au plus 7 par paires, 10 par triangles ; 19 sites | `CERTIFICATS_COLLECTIFS…md:72–92` | construite |
| Collectif du pool, adversaire 256, C64 | lectures de repli 15 616 → 10 667 (K5), 33 536 → 21 266 (K10) | `Q3_Q4_CERTIFICAT_COLLECTIF_20260920.md:164–166` | taille adverse, pas une pente |
| Rejets q3 par l'atlas | 90,7 % (quart sans sol, 7 067 sites) ; 73,8 % (préfixe 8k) | `Q3_CERTIFICAT_ATLAS_20260921.md:80–83` **(corrigé : pas l. 261–266)** | « diagnostic, pas un reçu » |
| Identité appariée après atlas q3 | sorties et compteurs d'émission identiques sur 9 lignes (3 scènes sans sol, K5 W1, K5 W8, K10 W8) | `ground_phase1_20260921/README.md:11–12`, sonde 0e2c18ca | les 3 lignes d'origine de la scène 0 sont non vérifiables, leur rejeu `only_*` l'est ; empreintes xor/somme |
| Fusions de grille à 2 cm | 4 247 / 4 537 / 4 801 retours (retours moins sites) | `receipts/lidar_spatial_20260921/README.md:30–31` | à 1 mm, aucune fusion supplémentaire sur les trois trames (`PRECISION_FLOAT32_ET_GRILLE_20260921.md:110–111`) |
| MEB canonique des facettes silencieuses | 9 865 demandes ; 17 363 entrées ; 70 540 comparaisons ; rapport 0,262–0,271 (K10 : 0,174–0,184) | `FACETTES_SILENCIEUSES_REPRISE_V7_20260919.md:64–84` | microbenchmark hors tour |
| Tour v7 50k (rappel) | K10 418,873 s ; K5 33,853 s ; 21,47 M boules ; 27,27 M nœuds | `docs/AUDIT_V7_SYNTHESE.md:45–57` **(corrigé)** | uniforme u16, G4, FULL mono-thread, 10 sept. |
| Préflight et captures [NC] | 78/79 portes courtes ; `release` 134/136 exécutés ; `release_r2` 136/136 exécutés, 3 désactivés **(ajouté)** | `[NC] REPRISE_U18…md:63–78` ; `[NC] release{,_r2}/CTEST.xml` | non commis ; mutants témoins toujours désactivés |
| Preuve native 1 mm (auditeur) | trois trames entières sans sol : 39 885 / 35 551 / 45 845 sites ; juge de Lagrange 70 672 comparaisons ; 221 rejets collectifs q4 confrontés à l'appel natif, aucune émission perdue **(ajouté)** | `audits/lidar_rectangles_20260922/README.md` | archive `RESULTS_NATIVE_LIDAR.json` hors dépôt : non vérifiable dans le dépôt |

## 5. Défauts, risques et dettes

### Gravité haute

1. **La v8 ne calcule pas l'objet HGP.** Aucune forêt, aucune tour, aucune verticale, ni K1 ni K=n : `src/forest/.gitkeep` seul ; `wspd_q34.hpp:100–101` ; `AUDIT_REPRISE_DEVELOPPEUR_20260921.md:56–57, 69`. Les chronos v8 sont ceux d'un **flux de candidats**, pas du contrat de tour.
2. **Le flux n'est pas un catalogue de boules.**
   - Doublons : 1,71 % des records q4 à K5 sur la scène 0 (reçus bilatéraux).
   - q2 est un autre pipeline, alors que les boules de $q_{\min}=2$ ne sont garanties que par la voie q2 (`SUPPORT_ET_CITRON.md:81–82`).
   - Intérieurs q3/q4 non émis ; aucun ordre exact des niveaux entre arités.

   Sans union q2 ∪ q3 ∪ q4 dédoublonnée par clé, avec $q_{\min}$, intérieurs et niveaux, la complétude par voie ne donne aucun catalogue exploitable par `full_ball_tower`.
3. **Domaine non régulier non décidé en v8, et déjà mesurable.**
   - Rien n'est implémenté pour les plateaux cosphériques dans `src/v8`.
   - Deux supports positifs distincts d'une même boule q4 imposent au moins cinq sites cosphériques. Les doublons du flux q4 (3 263 à K5, 20 660 à K10 ; 373 sur le quart x+y+) sont, d'après B, de tels plateaux : ils donnent un **ordre de grandeur mesuré** du domaine non régulier sur une vraie trame 2 cm **(ajouté)**. Le reçu ne permet pas d'exclure qu'un même support soit émis deux fois.
   - La v7 plafonnait la coquille à 12 sites (`morsehgp3D_v7/src/pipeline/expand.hpp:47`). La v8 collecte des coquilles non bornées sans sémantique aval.
   - Ce qui existe déjà et que la v8 n'a pas repris **(corrigé)** : la fixture racine `delaunay_gabriel_unsupported_degeneracy_right_triangle.json` (triangle rectangle AB/ABC, attendu `unsupported_degeneracy`), la matrice `gabriel_carrier_strict_interior_extra_shell_matrix.json`, et le statut `unsupported_rank_relevant_extra_shell_degeneracy` exigé par Cor. 4.3.
4. **Registre des preuves non mis à jour.** Aucun commit de `2b658cbe^..12294241` ne touche `docs/math/` ni `docs/SPECIFICATION_MORSEHGP3D.md` (git log vide). Les résultats nouveaux restent dans des notes v8 sans statut `proved_here` :
   - le citron sous ses deux hypothèses ;
   - les certificats familial, collectif et d'arête ;
   - la fenêtre $[L,U]$ et les couches duales ;
   - l'atlas, le théorème H et les tubes.

### Gravité moyenne

5. **Validation à l'échelle partielle, et nulle pour q3.**
   - q4 : validité exhaustive des records sur les sept morceaux, complétude sur 1 (K5) et 22 (K10) boules à la scène entière.
   - q3 : **aucun reçu indépendant commis** à l'échelle de la scène, ni en validité ni en complétude. `run_spatial_q3_large.py` est commis sans JSON ; la moitié x+ est une « ligne observée » **(ajouté)**.
   - Seul le préfixe 8 000 d'un scan avec sol a un exhaustif reçu (d1b4dbc6), coquilles non comparées. Le dernier moteur exhaustivement contrôlé, d6e1bd9e, ne l'est qu'à 1k–4k.
6. **Atlas q3 et port 18 bits sans contrôle croisé indépendant à l'échelle** **(corrigé)**.
   - Atlas q3 (0948d2d0) : identité d'empreintes à 0e2c18ca (`ground_phase1_20260921`), différentielle et pas un oracle.
   - Port 18 bits (a74e90f2) : `ground_phase1` est **antérieur** au port et ne le couvre pas. L'identité des « 444 compteurs » annoncée par le message de commit n'a de reçu que [NC] (`ground_18bits_20260922/u16_identity`, indexé non commis). À HEAD, la non-régression u16 du port est donc **non vérifiable**.
   - Côté 1 mm, HEAD contient la preuve native de l'auditeur (`lidar_rectangles_20260922`) : front sur trois trames entières et filtres comparés. Ce n'est pas un contrôle du flux q3/q4, et l'archive des résultats est hors dépôt. `[NC] ground_1mm_first` reste une ligne unique sans paire W1/W8.
7. **Le domaine arithmétique n'est imposé qu'à `prepare_cloud` à HEAD.**
   - `ExactBall::make_q2/q3/q4` acceptent des `int32_t` quelconques (`exact_ball.cpp:72–150`).
   - `require_valid_box` ne vérifie que l'inversion des bornes (`types.hpp:86–90`) ; `point_witness` ne vérifie rien **(ajouté)**.
   - Le défaut est **introduit par a74e90f2** : avant, `Point3` en `uint16_t` garantissait le domaine par le type.
   - Correctif seulement [NC] (`require_valid_point`, `valid_box` étendu).
8. **Couverture de test des bornes 18 bits absente à HEAD** **(ajouté)**. Les portes de `ExactBall`, des prédicats et de l'atlas n'exercent que les extrêmes u16 (65 535). Seul `cloud_owner_gate.cpp:229–253` teste 262 143, pour le refus d'entrée. Les 42 portes extrêmes sont [NC].
9. **Écart d'objet sur l'entrée, et autorité contradictoire à HEAD** **(corrigé)**.
   - La spécification (§2 l. 67) permet à une première version certifiée d'exiger la position générale ; les multiplicités viennent « ensuite ». Refuser les doublons n'est donc pas en soi une non-conformité.
   - Mais la grille fusionne des retours avant le calcul (4 247 à 4 801 à 2 cm) ; l'objet calculé est HGP sur les sites distincts de la grille, déclaré (`CONTRAT_TRAMES_SEMANTICKITTI_20260921.md:43–45`).
   - À HEAD, `ELARGISSEMENT_18_BITS_20260922.md` s'ouvre sur une « décision utilisateur » plaçant le contrat temps sur le moteur entier 18 bits. Or `AGENTS.md:15` dit encore « float32 originales par défaut » et ne mentionne pas le 18 bits. La même note annonce un stockage `uint32_t`, alors que le code est en `int32_t`. Le [NC] réécrit ces passages.
10. **Mutants de preuve désactivés à HEAD** **(ajouté)**. `mhgp8_q34_indexed_witness_mutations` (`q4_wrong_alpha`, `lemon_contact_counted_inside`, `admitted_lane_recounted_in_children`) est `DISABLED` depuis 2629a536 : un site de mutation n'est plus unique (`CMakeLists.txt:592–600`). Il l'est encore dans la capture [NC] `release_r2`. Le dernier reçu de mort date de d1b4dbc6. Les morts des mutants `q3_atlas_rejects_at_k_minus_2` et `parallel_refused_range_dropped` n'ont pas de reçu commis.
11. **Incohérences documentaires sur les autorités de preuve.**
    - `VERROUS…md:8` renvoie la preuve du citron à `DIALOGUE_COURANT.md`, qui ne mentionne plus le citron (grep vide) ; la preuve est dans `SUPPORT_ET_CITRON.md`.
    - `ELARGISSEMENT…md` l. 45–47 dit « l'échelle passe à Q = 2^18 ». Or l. 94–97 et le code (`q4_local_partition.hpp:21–23`) gardent $Q=2^{20}$.
12. **Atlas sans fixture d'égalité ni juge unitaire du centre à HEAD.** Aucun test ne référence `q3_center` ou `certified_inside_count`. La vérification « 199 graines » (`Q3_CERTIFICAT_ATLAS_20260921.md:62–65`) n'a pas de reçu. Le premier juge [NC] inversait lui-même x et y.

### Gravité basse

13. **Symbole $h_q$ surchargé** : seuil dans `FONDEMENTS` et `WSPD_Q2_Q3_Q4.md`, compte de témoins avec cible `T_q` dans `SYNTHESE_PRIORITES_LIDAR_20260922.md:21`.
14. **Pas de glossaire** distinguant Gabriel (Déf. 28, boule ouverte, relue) et minimum Gabriel strict (boule fermée), demandé par `VERROUS…md:136–139`.
15. **$K_{\mathrm{eff}}=\min(K_{\max},n)$ non utilisé par le front.** Effet conservateur : des seuils plus hauts rejettent moins. C'est un écart de contrat aux petites tailles, pas un défaut d'exactitude du flux.
16. **Contre-fixtures de nécessité du citron** (arête maximale, positivité) seulement dans `oracle.py:342–352`, pas dans CTest ; pas de mutant compilé « propriété omise » ni « positivité omise ».
17. **Tension avec la règle anti-exhaustif** : exhaustifs de B à 8k et sur les quarts (heures de calcul). Ce sont des audits différentiels, pas des portes.
18. **Bornes WSPD non démontrées** pour l'arbre employé (`REGIME_WSPD_20260914.md:86–88`) ; aucune borne de travail prouvée.

## 6. Questions ouvertes

1. **Domaine exact v9.** Quel sous-domaine non régulier est annoncé exact, avec quel quotient de plateau, et quel statut fail-closed ailleurs ? Reprendre `unsupported_rank_relevant_extra_shell_degeneracy` (Cor. 4.3) et les fixtures racine du triangle rectangle et de la matrice d'extra-shell. Il manque encore ABCZ et le carré avec résultat attendu.
2. **Multiplicités.** La v9 garde-t-elle HGP sur les sites distincts comme décision utilisateur explicite, ou implémente-t-elle les multiplicités de la spécification ? À 1 mm, aucune fusion supplémentaire sur les trois trames ; à 2 cm, des milliers.
3. **Fenêtre d'inertie plus fine ?** Le Th. 4.2 vaut pour **tout** support minimal positif $U$. Une boule ayant des supports minimaux de tailles 2 et 4 serait inerte pour $k\leq p+2$, alors que la v8 la retient si $p+q_{\min}\leq K_{\max}+1$. Lecture proposée, **non vérifiée**. Précision **(ajouté)** : ce cas suppose une coquille plus grande que chaque support, donc hors du domaine régulier ; il ne se pose qu'après la décision de la question 1.
4. **Interface de sortie** : catalogue de boules (une présentation valide par boule) ou toutes les incidences de supports ? Le flux q2 émet les incidences, le flux q3/q4 une présentation par groupe (`Q3_Q4_OBJETS…md:35`).
5. **Représentation des niveaux** : format exact et comparateur sous 18 bits, sachant que les rayons carrés et leurs comparaisons peuvent dépasser i128 (`Q3_Q4_CANDIDATS…md:32–34`).
6. **Groupement collectif** : quelle classe de groupes est prouvée et rentable (`CERTIFICATS_COLLECTIFS…md:72–74`) ?
7. **Constante de packing** de la WSPD sur l'arbre v8.
8. **(ajouté)** Les doublons q4 sont-ils tous des plateaux (supports distincts), ou existe-t-il des supports émis deux fois ? Le reçu bilatéral ne le dit pas ; un juge v9 doit compter les supports distincts par boule.

## 7. À porter en v9 et à ne pas reprendre

### À porter (explicitement, épinglé, requalifié)

| Quoi | Où (source) | Pin | Pourquoi |
| --- | --- | --- | --- |
| Définition de l'objet et réduction FULL (minima Gabriel, multifusions, parents pré-lot, ancres après fermeture de plateau) | `morsehgp3D_v7/docs/AUDIT_NIVEAUX_GABRIEL_20260905.md`, `morsehgp3D_v7/audits/NIVEAUX_ET_CERTIFICAT_HGP_COURANT.md`, registre l. 131–134 | 12294241 | seule autorité écrite de l'objet réduit, `conditional_theorem` sous prémisses régulières |
| Sémantique de tour par boules | `morsehgp3D_v7/src/forest/full_ball_tower.hpp` (sha256 `83f1c78e…`), `morsehgp3D_v7/docs/TOUR_FULL_PAR_BOULES.md` | 12294241 | décision de `FACETTES_SILENCIEUSES…md` ; retirer le plafond `kBallShellMax = 12` |
| Lemme du citron et complétude du front | `morsehgp3D_v8/audits/q34_global_contract_20260921/SUPPORT_ET_CITRON.md` | `6bcaf398…` | preuve courte, sans hypothèse de coquille |
| Seuils, inertie hors régularité, statut fail-closed nommé | `docs/math/INCIDENCES_SILENCIEUSES_GAMMA.md` §5.3.1 ; registre l. 120 | 12294241 | autorité racine `proved_here` des $h_q$ |
| Fixtures racine du domaine non régulier **(ajouté)** | `tests/fixtures/regressions/delaunay_gabriel_unsupported_degeneracy_right_triangle.json`, `gabriel_carrier_strict_interior_extra_shell_matrix.json`, `higher_rank_prune_does_not_certify_star_regularity.json` | 12294241 | résultats attendus déjà écrits |
| Prédicats W2/W3/W4 de points et de boîtes | `morsehgp3D_v8/src/spindle/predicates.hpp` | `8ea1509a…` | exacts ; ajouter la garde de domaine |
| Clé de boule primitive | `morsehgp3D_v8/src/lanes/exact_ball.{hpp,cpp}` + garde [NC] | `c4f8acc1…` (cpp à HEAD) | identité commune aux arités |
| Fixtures d'indépendance des voies et mutants associés | `morsehgp3D_v8/tests/q3_q4_owner_independence_gate.py` ; `tests/wspd_q34_mutations.py` | `af014861…` ; reçu `lidar_global_20260921/mutations` | interdit l'accès par acceptation q2 ou q3 |
| Fixtures de contact et contre-exemple $\alpha_3$ en q4 **(corrigé)** | `wspd_q34_gate.cpp:361–362` ; `q34_witness_search_gate.cpp:272, 282–293, 484` ; mutants `tests/q34_indexed_mutations.py:19–24` | 12294241 ; reçu `q34_indexed_20260921/mutations_final` | tuent `≥` et $\alpha_3$ en q4 ; réaligner le script avant de réactiver |
| Certificat familial et corde resserrée | `docs/Q3_Q4_REJET_FAMILIAL_20260920.md`, `docs/Q3_Q4_CERTIFICAT_COLLECTIF_20260920.md` | 12294241 | rejets exacts de familles |
| Fenêtre $[L,U]$ et couches duales | `docs/Q4_FENETRE_DE_FAIBLE_PROFONDEUR_20260920.md`, `docs/Q4_COUCHES_DUALES_20260920.md` | 12294241 | réduisent tri et census sans perte |
| Certificat d'atlas pour q3 (puis saturation à $K-1$ après qualification) | `docs/Q3_CERTIFICAT_ATLAS_20260921.md` ; [NC] `REPRISE_U18…md` | 0948d2d0 | preuve courte ; il manque juge du centre et fixture d'égalité |
| Certificat collectif d'arête | `audits/CERTIFICATS_COLLECTIFS_AVANT_ATLAS_20260922.md`, `audits/collective_edge_20260922/` | `5d98ac93…` | rejette ce que les témoins individuels ne peuvent pas rejeter |
| Théorème H et invariant des préfixes | `docs/P0_TEMOINS_HERITES_Q2.md`, `audits/q34_global_contract_20260921/PREFIXES_TEMOINS.md` | 12294241 | héritage par rangs et curseurs |
| Lemme des tubes ($D\geq10R$) | `audits/P0_TUBES_ET_RANGS.md` §1 | 12294241 | crédits locaux |
| Contre-fixtures de l'objet | `tests/fixtures/regressions/gabriel_point_set_counterexample.json`, régression A–E, AB/ABC | 12294241 | à graver dès la première tranche de forêt |
| Borne de sortie quadratique | `morsehgp3D_v7/audits/receipts_probe_meb_review_20260906/full_output_growth.md`, `MATH_CHECKS.json` | `c63fbb63…` | contrats en surcoût au-delà de la sortie |
| Protocole bilatéral (validité exhaustive des records, complétude échantillonnée) | `audits/q34_stream_crosscheck_spatial_20260921/` | `e55d88d0…`, `b7423d9c…` | juge d'échelle compatible avec la règle ; à étendre à q3 et aux supports distincts |

### À ne pas reprendre (fausses pistes fermées, avec la preuve)

| Piste | Mesure ou contre-exemple qui la ferme |
| --- | --- |
| Graphe induit sur les seuls minima Gabriel | fusion retardée de 169/9 à 41/2, 4 points u16 (`NIVEAUX_ET_CERTIFICAT_HGP_COURANT.md:29`) |
| Prop. 6, Th. 5, K-MST, Algorithme 1 comme autorité de correction | E5, registre `false_in_general` (l. 40–41) ; « 3 violations sur 366 nuages » de `VERROUS…md:106` non vérifiable |
| Appliquer $\alpha_3=3$ à q4 | tétraèdre (0,0,0),(60,0,0),(20,42,0),(28,10,49), z=(28,−12,−12) : $2H^{2}=739328\leq\Xi=1036800<3H^{2}$ (`DIALOGUE_AUDITEUR_B.md:141–150`) ; gravé et mutant tué |
| Témoin non strict ($\geq$) | contacts (32,34,28) en q3 et (32,32,32) en q4 (`SUPPORT_ET_CITRON.md` §5) ; mutant tué |
| Citron sans arête maximale ou sans positivité | deux contre-fixtures u16 (`SUPPORT_ET_CITRON.md:170–173`) |
| Alimenter q3/q4 par les arêtes acceptées en q2, ou q4 par les graines acceptées en q3 | fixtures `Q3_Q4_OBJETS…md` §3 ; mutants tués (reçu `lidar_global_20260921`) |
| Cover q4 au coefficient 3 ; arrêt au premier groupe q4 profond | mutant `q4_uses_q3_cover` de `oracle.py:340` ; `WSPD_Q2_Q3_Q4.md:152–169` |
| Transmettre aux enfants le **compte** de témoins du parent | support perdu, fixture de 5 points K=2 (`FAUSSES_PISTES.md:392`) |
| Groupements collectifs différents par coin | contre-exemple segment A (`CERTIFICATS_COLLECTIFS…md` §4) |
| Rejet universel de boîte vu comme mort des ancres ; saturation locale de $h_b$ | `WSPD_Q2_Q3_Q4.md` §8 |
| Plafond de coquille v7 à 12 transposé en v8/v9 | `FACETTES_SILENCIEUSES…md:114–115` |
| Tri naïf des racines q4 par produits croisés (degré 9) | `FAUSSES_PISTES.md:35–39` |
| Cache des formes de l'atlas ; classification conjointe des quatre filles | 27,32 s contre 27,32 s ; +18 % (`JOURNAL_DEVELOPPEMENT_20260921.md`) — hôte partagé, une répétition |

## 8. Recommandations priorisées pour la v9

1. **Définir l'objet de sortie avant tout chrono.** Première tranche : catalogue canonique q2 ∪ q3 ∪ q4 dédoublonné par clé primitive, avec $q_{\min}$, IDs intérieurs, coquille complète, **supports distincts comptés** et niveau exact ordonné. Ensuite, port explicite et épinglé de `full_ball_tower` v7 (forêts K=1..10, parents pré-lot, verticales).
2. **Décider le domaine exact avec les fixtures qui existent déjà** : reprendre les fixtures racine du triangle rectangle et de la matrice d'extra-shell, et le statut `unsupported_rank_relevant_extra_shell_degeneracy`. Compléter ABCZ et le carré. Mesurer la part non régulière par le taux de doublons q4 (1,71 % à K5 sur la scène 0 à 2 cm) et la refaire à 1 mm.
3. **Graver dès le premier jour les contre-fixtures de l'objet** en portes à code exact : E5, régression A–E, AB/ABC, K1, K=n. Oracles bornés T2 ($n\leq12$–$14$).
4. **Enregistrer les preuves v8 réutilisées** dans `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md` (`proved_here`, fixtures d'égalité) : citron, familial, collectif, collectif d'arête, $[L,U]$, couches duales, atlas, théorème H, tubes. Corriger le pointeur mort de `VERROUS…md:8`.
5. **Remettre en service les mutants de preuve.**
   - Réaligner `tests/q34_indexed_mutations.py` et réactiver `q4_wrong_alpha` et `lemon_contact_counted_inside`.
   - Ajouter « propriété omise » et « positivité omise », avec les contre-fixtures de nécessité du citron.
   - Produire un reçu de mort commis pour l'atlas $K-2$ et la plage parallèle perdue.
6. **Porter la garde de domaine et tester le 18 bits à ses extrêmes** : fabriques, boîtes, formes et centres de l'atlas ; portes à 262 143 pour `ExactBall`, les prédicats et l'atlas ; fixture d'égalité de l'atlas (centre sur frontière de cellule fermée, site sur la coquille) ; juge unitaire du centre q3.
7. **Juges d'échelle conformes à la règle, q3 compris**, à 8 000 / 16 000 / 32 000 et sur trames entières 1 mm, pour chaque filtre exact nouveau :
   - validité de chaque record q3 **et** q4 ;
   - complétude par échantillon stratifié (plusieurs centaines de boules, pas 1 ou 22) ;
   - identités de registre.
8. **Ne porter les filtres coûteux qu'après le catalogue** (collectif d'arête, cascade de rectangles, saturation de l'atlas), sur critère de coût aval évité.
9. **Unifier notations et autorités d'entrée** : $h_q$ seuil, $c_q$ compte, $T_q$ cible ; glossaire Gabriel / minimum Gabriel strict / support minimal positif / présentation / boule ; aligner la note 18 bits sur `AGENTS.md` (float32 par défaut, grille optionnelle).
10. Faire vérifier par l'auditeur mathématique la lecture de la question 6.3 (fenêtre par le plus grand support minimal), après la décision de domaine.

## Contre-vérification

Chaque affirmation principale et chaque chiffre du rapport d'origine a été confronté à sa source. Verdicts : **confirmé**, **corrigé**, **réfuté**, **non vérifiable**.

### Affirmations principales

| # | Affirmation d'origine | Verdict | Note |
| --- | --- | --- | --- |
| 1 | Ni forêt, ni tour, ni verticales ; q2 séparé | confirmé | trois dossiers `.gitkeep` ; `wspd_q34.hpp:100–101` ; audit de reprise l. 56–57, 69 |
| 2 | Doublons 190 405 / 187 142 (K5), 2 158 063 / 2 137 403 (K10) | confirmé | JSON hachés ; boules distinctes par (clé, profondeur) ; le reçu ne distingue pas support répété et second support |
| 3 | Aucun catalogue canonique | confirmé | `Q3_Q4_CANDIDATS…md:32–34, 89–90` ; aucune comparaison de rayons dans `src/` |
| 4 | Citron $\alpha_3=3$, $\alpha_4=2$, hypothèses nécessaires | confirmé | preuve et quatre fixtures recalculées |
| 5 | $\alpha_3$ en q4 faux ; « contact (32,32,32) gravé l. 272 » ; pas de mutant compilé du produit | corrigé | le contre-exemple lui-même est gravé l. 282–293 ; mutant `q4_wrong_alpha` tué (reçu `q34_indexed_20260921`), mais désactivé dans CTest depuis 2629a536 |
| 6 | Seuils $h_q$ valides hors position générale | confirmé | registre l. 120 `proved_here` ; réserve : Cor. 4.3 exige le refus des extra-shells pertinentes |
| 7 | Rejet de présentation distinct de l'inertie ; mutants | confirmé | reçu de mort ajouté (`lidar_global_20260921/mutations`) |
| 8 | Clé primitive prouvée et testée | confirmé | précision : testée seulement aux extrêmes u16 à HEAD |
| 9 | Certificat familial et corde resserrée | confirmé | identités refaites |
| 10 | Atlas : certificat, mutant $K-2$ tué, identité appariée | corrigé | mutant sans reçu de mort commis ; note de 98 lignes, les chiffres sont l. 80–83 |
| 11 | Collectif d'arête, 123 rejets supplémentaires non vides | confirmé | 242 supplémentaires dont 123 non vides ; base a74e90f2 |
| 12 | Complétude à l'échelle échantillonnée (1 et 22 boules) | confirmé | ajout : aucun juge q3 reçu à l'échelle |
| 13 | Exhaustif 8k : 93 914 / 10 756 et 409 195 / 116 985 | confirmé | q4 comparé en boules distinctes ; 116 988 records à K10 |
| 14 | Aucun commit v8 sur `docs/math/` ni la spécification | confirmé | git log vide sur la plage |
| 15 | Domaine non régulier non décidé | corrigé | vrai pour `src/` v8 ; fixtures racine avec résultat attendu et statut nommé existent |
| 16 | Sites distincts « non conformes » à la spécification ; float32 par défaut | corrigé | la spécification §2 l. 67 autorise une première version en position générale ; le vrai conflit à HEAD oppose la note 18 bits et `AGENTS.md:15` |
| 17 | Fabriques publiques sans garde à HEAD | confirmé | plus large : `require_valid_box` et `point_witness` aussi ; défaut introduit par a74e90f2 |
| 18 | Réduction FULL v7 « prouvé (v7) » | corrigé | `conditional_theorem` sous prémisses régulières (registre l. 131–134) |

### Chiffres

| Chiffre d'origine | Verdict | Note |
| --- | --- | --- |
| Seuils 10/9/8 ; 5/4/3 | confirmé | Cor. 4.3 |
| $\alpha_3=3$, $\alpha_4=2$ | confirmé | `predicates.hpp:124–126` |
| $R^{2}\leq(q-1)D^{2}/(2q)$ | confirmé | preuve refaite |
| Covers q3/q4 | confirmé | calcul refait |
| Borne de corde | confirmé | identités refaites |
| Tubes $D\geq10R$ ; 7,6 / 6,3 / 6,1 | confirmé (texte) ; non vérifiable (attaque) | `VERROUS…md:188–199` sans reçu |
| Domaine $[0,262143]$ | confirmé | `types.hpp:21–29` |
| Bornes $2^{117}$, $2^{62}$, $Q=2^{20}$ | confirmé | commentaires de preuve |
| Borne de sortie « $m^{2}$ sur $2m+K-2$ ; 9/25/81/289 aux tailles 6/10/18/34 » | corrigé | les contrôles entiers donnent $(m+1)^{2}$ sur $2(m+1)$ sites à K=2 ; $m^{2}$ sur $2m+K-2$ est la famille rationnelle |
| Exhaustif 8k K5 | confirmé | JSON `367ed209…` |
| Exhaustif 8k K10 | confirmé | 116 988 records pour 116 985 boules |
| Records q4 valides K5 et K10 | confirmé | JSON hachés |
| Doublons 3 263 (1,7 %) et 20 660 (0,96 %) | confirmé | 1,71 % et 0,96 % |
| Records q3 « champ `records_q3` » | corrigé | valeur exacte, champ `rows[].probe.output.q3` |
| Boules énumérées 1 et 22 | confirmé | README l. 48 |
| Exhaustif des quarts « sans reçu » | corrigé en partie | 28 854 records et 28 481 boules du quart sont dans le reçu bilatéral ; l'égalité avec l'énumération exhaustive, 270 202 et 528 575 restent non vérifiables |
| Corpus collectif d'arête | confirmé | `RESULTS.json` |
| Collectif du pool 15 616 → 10 667, 33 536 → 21 266 | confirmé | l. 164–166 |
| Atlas 90,7 % et 73,8 % à « l. 261–266 » | corrigé | l. 80–83 ; valeurs exactes, non vérifiables (diagnostic) |
| Identité après atlas sur 9 lignes | confirmé | trois lignes d'origine non vérifiables, rejeu vérifiable |
| Fusions 4 247 / 4 537 / 4 801 | confirmé | retours moins sites à 2 cm |
| MEB 9 865 ; 17 363 ; 70 540 ; rapports | confirmé | l. 64–84 |
| Tour v7 50k à « l. 52–64 » | corrigé | l. 45–57 ; uniforme u16, G4 |
| Préflight [NC] 78/79 ; 134/136 | confirmé | ajout : `release_r2` 136/136 exécutés, 3 désactivés |

### Omissions ajoutées

- Le mutant compilé `q4_wrong_alpha` et le contre-exemple gravé existent. Le mutant est désactivé dans CTest à HEAD et dans la capture [NC] `release_r2`.
- Aucun juge indépendant q3 n'est reçu à l'échelle de la scène.
- La non-régression u16 du port 18 bits n'a de reçu que [NC]. `ground_phase1` précède a74e90f2.
- Les bornes 18 bits ne sont testées aux extrêmes 262 143 par aucune porte commise, hors refus d'entrée.
- La garde de domaine manque aussi à `Box3` et aux prédicats. Le défaut vient du passage `uint16_t` → `int32_t`.
- L'auditeur a déposé à HEAD une preuve native sur trames entières 1 mm (`lidar_rectangles_20260922`). Son archive de résultats est hors dépôt.
- Des fixtures racine du domaine non régulier et un statut fail-closed nommé existent déjà.
- Les doublons q4 mesurent, d'après B, la part non régulière du flux.
- La note 18 bits à HEAD contredit `AGENTS.md` (décision d'entrée, type de stockage).
- La spécification autorise une première version en position générale.
- Les exhaustifs 1k–4k couvrent quatre moteurs successifs, dont d6e1bd9e.
- Les morts des mutants atlas $K-2$ et plage parallèle perdue n'ont pas de reçu commis.
