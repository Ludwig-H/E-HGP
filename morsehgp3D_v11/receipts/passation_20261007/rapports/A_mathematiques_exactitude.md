# Audit final v11 — objet mathématique, preuves, exactitude numérique

Cadre de cette lecture : instantané `ac081a06f` (lecture seule), aucune compilation, **GCP non utilisé**. Chemins relatifs à `morsehgp3D_v11/` (instantané `ac081a06f`). Convention : ce qui n'est pas marqué **[inférence]** a été lu dans la source citée.

## 1. Périmètre et état

**L'objet** (docs/MATHEMATIQUES.md § 1–4 et § 10) :
- **Entrée.** n sites distincts de poids 1 sur la grille entière [0,2^B)³. B vaut 21 par défaut ; 18 et 24 sont compilables. Sur LiDAR, la grille est de 1 mm.
- **Niveaux.** Pour k = 1..K (K ≤ 12 et K ≤ n), le niveau a est un **rayon carré** rationnel exact. Il est stocké non réduit, comparé par produits croisés et indexé par des rangs denses (`LevelRank`).
- **Forêt T_k.** C'est l'arbre de fusion de π0(Γ_k(a)), identifié à π0(L_k(a)) par T1. Les naissances sont les sites à k = 1, puis les boules de naissance de W_k. Les fusions sont N-aires, à plateau atomique ; un parent est strictement plus haut que ses enfants ; la racine est unique. La numérotation est canonique : naissances par (niveau, centre exact), fusions par (niveau, plus petite naissance).
- **Coupes.** Fermées et ouvertes sont distinctes ; une coupe ouverte se prend sur des boules **ouvertes**.
- **Verticales.** Chaque nœud d'ordre k est envoyé sur le nœud d'ordre k−1 vivant à la coupe fermée, ce qui traduit L_k ⊆ L_{k−1}.
- **Catalogue.** Cat_K = {boules critiques : p+q ≤ K+1}, avec S*, p, m, q, I et U.

**Sorties** (docs/SORTIES.md) :
- `full` : format MHGP11FUL1.
- `supports` : MHGP11SP v2. Contenu : T_K et les boules de W_K de rôle naissance ou fusion, sélectionnées par Kruskal au plateau, avec S* seul.
- `points` : H^r_{K+1} = P₁∘Π_{K+1}, dates √t+√m−√q exactes.
- `plat` : condensation au critère A, EOM N-aire avec φ = r^{−z}.

**Refus explicites** : poids > 1 (`unsupported_degeneracy`), `wide_leaf`, `support_shell_capacity`, `radical_sign_budget`, `tower_capacity`, et K ≥ n ≥ 2 pour `points` et `plat`.

**Volume.** Le code compte 21 348 lignes C++ en 12 modules sous `src/`. Il y a environ 47 documents, 125 reçus, 530 mutants dans les manifestes `tests/mutants/*.json`, et 360 commits v11 entre le 2 et le 7 octobre.

**Qualification et vitesse** (`public_status=not_claimed`) :
- Reprise R1–R4 au pin `98a009550` : 3 695 portes, 485 mutants, 80 portes ASan u24.
- Qualification V3 au pin `38faaf272` : Release u18 890/890, u21 et u24 800/800 chacun, ASan u24 800/800. Clang n'a jamais été disponible.
- K5 à chaud : 240–290 ms (audits/REPONSE_CLAUDE_SUPPORTS_20261004.md § Y). K10 : 1,8–3,3 s. La v10 faisait 204–254 ms à K5. **Ni 200 ms ni 100 ms ne sont atteints.**

## 2. Ce qui a marché

### Mathématiques (rédigées, relues par l'auditeur, inscrites au registre)

1. **Un contrat complet sans position générale.**
   - M1/M2, G1–G4 (complétude conditionnelle du catalogue), T1–T6, P1–P5 et J1–J3 sont démontrés dans MATHEMATIQUES.md § 1–8.
   - T2 (une trace est stricte si et seulement si elle est séparable) remplace partout la position générale.
   - Les lemmes A, P, W, B, C, D, E, F, G, H du § 10 et la suffisance de Kruskal ont statut `proved_here`. Voir le registre `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md`, sections « V11 », lu par `git show ac081a06f:`.
   - Le théorème 4 de la thèse est étendu sans position générale (lemme W.4).
2. **Contre-épreuve de l'auditeur** (audit_geant_20261005/math) : 65 ordres, 17 276 unions arbitraires, 1 453 coupes et 15 925 faces verticales comparées au nerf des régions témoins. Aucun défaut FULL.
3. **Euler J3, juge global à l'échelle.**
   - L'identité est démontrée, et Cat_{K+2} suffit pour juger tous les ordres jusqu'à K.
   - Les portes `mhgp11_catalogue_euler_scale8000/16000/32000` et `_lidar_ng00..ng02_k5/k10` passent (tests/catalogue/tests.cmake).
   - Les nombres de boules égalent **exactement** ceux du catalogue v10 : Cat₅ 1,10–1,41 M, Cat₁₂ 6,5–8,3 M. La coquille maximale vaut 5 sur LiDAR.
4. **Points** (docs/HIERARCHIE_POINTS.md, H1–H7).
   - Laminarité ; aucune réunion avant la fusion FULL.
   - Stabilité **3ε** des dates et des hauteurs.
   - Retard borné : α_{k+1} ≤ e ≤ α_{k+1} + d_k/2.
   - Date B égale à la rencontre de la pendaison et du cœur, stable en 3ε (audit_giant_20261004/mathematics/PROOF.md).
   - Raffinement EOM monotone en z.
   - Formule exacte de 1/(√t+√M−√q) (eom_exact_audit_20261004, 888 gardes).
5. **Lemmes courts devenus des gains moteur.**
   - Table de populations : une égalité exacte de population signale un pas terminal. Sur ng00 à K5, 1 350 322 descentes régulières sur 1 350 322 se terminent ainsi (docs/PERFORMANCE_FULL.md). Les présentations MEB passent de 15,7/12,6/15,6 M à 3,8/2,9/3,3 M.
   - MEB par diamètre (docs/MEB_DIAMETRE.md).
   - Zonogone de la droite des centres (docs/CENTER_REGION.md).
   - Borne entière exacte sur les sites et arbre radix de Morton : les tests de points du census tombent à ×0,258–0,265 (docs/INDEX.md).

### Oracle (reference/)

- **Deux étages et un juge.** L'étage A est la définition Γ_k exhaustive en `Fraction` (n ≤ 14, K ≤ 10). L'étage B est constructif. Le juge exige B = A champ par champ. S'y ajoute un oracle d'intervalles qui n'importe rien du paquet.
- **Résultats** : 1 362 ordres, 48 234 coupes, 13 029 nœuds, 22 mutants tués. Les dumps v10 sont identiques sur 190 nuages (26 530 lignes).
- **Oracle des supports S1** : 210 nuages, 951 ordres, 15 062 boules, 13 mutants, dont 6 qui prouvent la vivacité d'un contrôle.
- **Différentiels natifs** : supports sur 963 ordres ; FULL contre la v10 figée, 42 égalités (14 fixtures × 3 profils, receipts/full_20261002).

### Numérique

- **Budgets de bits typés** (src/num/budgets.hpp) : `Int<bits>` est calculé en `constexpr` (puissance 6B+8, niveau 8B+12 / 6B+8, comparaison 14B+20). Un débordement est une erreur de compilation.
- **Trois voies : native, contrôlée, Wide**, avec certificats globaux calculés par les fabriques (src/num/predicates.cpp, power_certificate.hpp, orientation_certificate.hpp).
  - q1, q2 et q4 sont natifs aux trois profils (72M⁵ < 2^127).
  - Seul q3 est dur (216M⁶).
- **Coût des profils faible.** Le catalogue mono gagne +5–6 % de u18 à u21. FULL u21 contre u24 : 1,463/1,155/1,515 s contre 1,483/1,154/1,531 s (reuse1).
- **Niveaux différés.** q4 : environ 99,7 % des niveaux candidats évités. q3 : 9,3/8,4 M niveaux évités par trame, pour un gain de 1 à 1,3 % du mur.
- **« Le flottant propose, l'entier décide ».**
  - `RootTable` certifie ses racines par isqrt.
  - Le tri F3/F4 rejoue en exact.
  - `RadicalSum` certifie une égalité par les classes de carrés et refuse au-delà de 2^−8192.
  - La tête binary64 est remplacée par des encadrements entiers (S10).
- **Déterminisme** : sorties identiques à W1/W8/W48 (81/81 prises), sous permutation et sous réétiquetage.

## 3. Ce qui n'a pas marché ou a été abandonné

| Élément | Cause, preuve |
| --- | --- |
| F3 « par nombre d'instructions » | Témoin N = 2^53+1 : 7 opérations, erreur ≈ 8u > 7u (l'exposant réel vaut 63) ; remplacé par une propagation par expression (receipts/audit_independant_20261002/floating_bounds/PUBLICATION.md) |
| F2 « par degré » | x = 32767, A = 512x³ : (A+1)−A−1 donne −1 en binary64 (math_locks_review) |
| Filtre F6 sur la puissance | Gain d'environ 1 %, retiré (note de mémoire « ouverture-v11 », 3 oct.) |
| Enveloppes M3/E4 sur CPU | Coûtent environ 1 % de plus qu'elles n'évitent ; retirées de la voie CPU (audits/QUESTION_CLAUDE_VITESSE_100MS § E) |
| MEB « support + extérieur » (idée v10) | 341 → 392 présentations et 828 → 1 294 tests sur 108 fixtures ; écartée |
| Euler comme certificat | Compensation sur 5 sites ; défauts invisibles à 13 sites (K5) et 23 sites (K10) |
| Supports v1 (tous les Q_b) | Carrier instable (cercle, saut de Hausdorff ≥ 1/4) ; remplacé sur décision utilisateur par Kruskal avec S* seul ; le premier filtre par rôle laissait un cycle (triangle K1), corrigé en `07428324e` |
| Marge en niveau carré (Q₁ v10), ER0h | Aucune constante uniforme (rapport 31,8 à L = 10⁴ ; proposition S) |
| Polyèdre d'ordre k | Trois affirmations réfutées (voir § 4) ; prototypes Python seulement, environ 1 000 faces par site à K5 |
| Contrats 200/100 ms | Non tenus ; feuille coopérative GPU par paires et pages de 2 Mio rejetées par la mesure |

## 4. Pièges et leçons

**Bornes intermédiaires.** Une borne sur le résultat final ne borne pas les intermédiaires.
- Triangle aigu avec s = 2^21−1 : le premier produit vaut 6s⁶ ≥ 2^127, alors que la puissance finale tient dans i128 (catalogue_native_side_review_5).
- La largeur locale d'un support ne borne pas un témoin global.
- La borne |cross| < M² vaut pour trois points d'un même cube, pas pour deux `Vec` arbitraires : (m,m) et (m,−m) donnent −2m².
- La garde i64 du test cubique de région était dépassée dès u21 (porte `region_cubic_width`).
- L'export u24 sur trois mots était insuffisant (196/148 bits).
- `LevelSource` : borne en u128, calcul en i128, et le +1 déborde.

**Domaines de contrat.** Le minimum sur les points entiers d'une boîte n'est pas le minimum continu (segment q2 : 0 contre −D/2). Le contrat V3 est donc réservé aux sites.

**Coupes et dates.**
- Témoin D2 : 41 < 64 < 1681/25. Seuls les **ensembles de nœuds** coïncident entre coupe ouverte λ_b et coupe fermée de rang r_b−1, pas les composantes.
- Un mémo n'est valide qu'à partir de β_initial, jamais β_terminal (X = {0,2,4,6}).

**Contradictions gravées** (fixtures permanentes) :
- E5 (`tests/fixtures/regressions/gabriel_point_set_counterexample.json`) : Prop. 6 de la thèse fausse ; retirer les liaisons hors W_K change T_K.
- Sept sites {0,10,11,26,27,45,46} : les ordres se croisent. {0,2,4} : LCA discontinu. {0,2,5} : cover ≠ MR₂-bord. Ces cinq faits sont dans reference/test_projection_contracts.py.
- Cercle : supports instables.
- `polyhedron_order_k_counterexamples.json`, trois entrées `false_in_general` (`1fbeea5b8`) :
  - moins de k aberrants peuvent créer ou fusionner des composantes ;
  - l'identité des chaînes de l'arbre δ-contracté n'est pas stable ;
  - les réalisations ne s'emboîtent pas d'un ordre à l'autre.

**Compteurs et mutants.**
- « Feuilles ≤ sites » est faux : 353 456 feuilles pour 39 885 sites ; toutes les trames étaient refusées (session claudegpu2).
- Des mutants compilés en u18 étaient vides, leurs branches éliminées pour B ≤ 20.
- Les « gardes sans porte possible » produisent des mutants équivalents.

**Oracles.** Un oracle `Decimal` ne suffit pas aux plateaux exacts : (2,162,50) et (8,98,32) valent tous deux 5√2. Le filtre flottant de la première version donnait un signe faux sur un autre témoin.

**Quantification.** Sur les trois trames, 61 à 67 % des nœuds K5 vivent moins de 2δ (δ = √3/2 mm). Seuls les nœuds de vie > 2δ ont une identité stable. Les formes reconnaissables du vélo synthétique sont portées par des nœuds de 0,1 à 0,4 mm (REPONSE_CLAUDE_POLYEDRE_ORDRE_K_RESULTATS_20261007).

**Mesure.**
- Les instructions ne sont pas le temps : V3 retire 44 % des instructions et l'étage des forêts ne baisse que de 7,5 %.
- Le local n'est pas G4 : les pages de 2 Mio gagnent 17 % en local et perdent sur G4.

## 5. Dettes et problèmes ouverts

**Mathématiques**
- **FULL pondéré** : refusé ; le modèle par copies n'est ni relu ni implanté (§ 9).
- **Coquilles étendues.** Seule l'énumération exhaustive bornée existe. Une coquille cosphérique de 270 sites, entrée u18 valide, est refusée (max_leaf 256 ; catalogue_boundary_work_review_4). Aucun quotient polynomial n'est prouvé.
- **Complexité.** Aucune terminaison prouvée pour une politique de subdivision arbitraire, aucune borne sous-quadratique. **[inférence]** Sur LiDAR, le catalogue croît comme n·K² environ : 31–33 boules par site à Cat₅, 120–140 à Cat₁₀.
- **Points.**
  - La constante géométrique minimale est encadrée entre κ/2 et 1+2κ.
  - Aucune règle connue n'est à la fois stable par insertion, locale au profil, et réussit T0 et Q1–Q4.
  - Restent ouverts : le critère d'existence d'un cluster, la synthèse de plusieurs ordres, et le chapitre 7. L'obstruction de Palm Θ_H < Θ_poly n'est que conditionnelle.
- **Tête** : règle de sélection ouverte ; la partition n'est pas stable près d'une égalité EOM.
- **Polyèdre A_k(r)** : il faut un représentant petit, de topologie certifiée et robuste.

**Validation**
- **Différentiel canonique v10/v11 sur trames entières jamais fermé.** Seules les cardinalités sont égales. Or ARCHITECTURE § 6 en fait une condition de conformité.
- La suite complète de la référence (5 617 nuages) ne tourne que sur G4.
- L'oracle des supports s'arrête à K ≤ 5 et aux coquilles ≤ 12 sites. Les longues descentes ne sont qu'effleurées.
- Portes natives demandées le 4 octobre : q3 extrême, q4 au seuil 2^20±1, préfixe obtus, coquille qmin2. Une porte `mhgp11_catalogue_leaf_narrow` existe ; la clôture complète n'est pas établie par ma lecture.
- **[inférence]** Le contre-exemple L02 au théorème 5 (note de mémoire « ouverture-v11 ») n'a pas de fixture v11 propre que j'aie trouvée ; le registre le couvre par héritage de E5.

**Documentation** : DEVELOPPEMENT.md est figé au 3 octobre ; la formule de mémoire de CATALOGUE.md doit être 16C⌈C/64⌉.

## 6. Recommandations pour la v12

**Garder tel quel**
1. MATHEMATIQUES.md § 1–8 et § 10, sous forme de contrat normatif, avec les sections V11 du registre et la galerie de témoins en fixtures : E5, D2, carré, cube, octaèdre, sphere5, deux triangles, {0,2,4}, {0,2,5}, sept sites, compensation d'Euler, cercle, fixtures du polyèdre.
2. `reference/` entière : étages A et B, juge B = A, oracle d'intervalles, S1 et ses mutants de vivacité, familles SplitMix64.
3. La doctrine numérique : budgets `constexpr` par expression, certificats globaux dans les fabriques, voies native/contrôlée/Wide, `Level` non réduit avec `LevelRank`, F1–F6 (F3 par expression), auto-test F5.
4. L'index radix de Morton avec borne entière, le contrat de census (K témoins stricts ou I/U complets), le lemme de la table de populations et le journal des graines (lemme D, sans descente supplémentaire).

**Simplifier**
- Un seul profil produit (u21) plus u24 en matrice ; u18 seulement en différentiel historique, ou abandonné.
- Une voie canonique et une voie de référence, au lieu des masques d'options : 2047, 16379, 278523, 802811, 868347:400…
- Trois documents vivants au lieu d'environ 47.

**Refaire autrement**
1. Concevoir le numérique autour des certificats **avant** le port. q3 est la seule arité dure. Graver d'abord les témoins de bord : s = 2^B−1, famille régulière m = 2^B−1, centres lointains.
2. Rouvrir la partition T > 0. La reformulation de l'auditeur (comparer QE à Dr, budget 4B+5+T ≤ 127) rend T6 compatible avec u24. **[inférence]** C'est une source probable de l'écart de vitesse avec la v10.
3. Trancher la sémantique des multiplicités avant le moteur.
4. Fermer le différentiel v10/v11 sur LiDAR **tôt**, ou retirer la revendication « même objet ».
5. Publier la vie de chaque nœud rapportée à δ, et ne promettre une identité stable qu'au-delà de 2δ.
6. Garder points, tête et polyèdre en aval, avec leurs contrats propres.

**Verrous à poser explicitement** : complétude et coût des coquilles étendues, FULL pondéré, borne de travail, règle de points stable par insertion, représentant robuste de A_k(r), et le contrat de 100 ms (K5 à environ 240–290 ms, K10 en secondes).

## 7. Références clés

**Mathématiques et contrat**
- docs/MATHEMATIQUES.md
- docs/HIERARCHIE_POINTS.md
- docs/SORTIES.md
- docs/SORTIE_PLATE.md
- registre racine `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md` (sections V11)

**Numérique et moteur**
- docs/ARCHITECTURE.md (§ 3–4, § 7)
- docs/CONCEPTION_MOTEUR.md
- docs/PREDICATS_{I128_CONTROLES,Q3_CERTIFICAT,ORIENTATION_CERTIFICAT}.md
- docs/Q4_POIDS_PRESENTATION.md
- docs/CENTER_REGION.md
- docs/MEB.md, docs/MEB_DIAMETRE.md, docs/MEB_CONSTRUCTIONS_DIFFEREES.md
- docs/INDEX.md, docs/CATALOGUE.md, docs/FULL_FORESTS.md
- src/num/budgets.hpp, src/num/predicates.cpp, src/num/lattice_bounds.hpp, src/num/radical.hpp, src/num/roots.hpp

**Provenance et audit v10** : docs/PROVENANCE.md, docs/AUDIT_V10_SYNTHESE.md

**Oracle** : reference/README.md, reference/hgp11_ref/, reference/test_projection_contracts.py, reference/test_supports.py

**Audits**
- audits/AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md
- audits/AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md
- audits/AUDIT_OUVERTURE_ET_REPRISE_V10_20261002.md
- audits/REPONSE_CLAUDE_SUPPORTS_20261004.md
- audits/REPONSE_CLAUDE_POLYEDRE_ORDRE_K_RESULTATS_20261007.md

**Reçus**
- receipts/meb_20261002, receipts/index_20261002, receipts/center_region_20261002
- receipts/palm_obstruction_20261003, receipts/eom_exact_audit_20261004
- receipts/audit_independant_20261002 (floating_bounds, math_locks_review, catalogue_native_side_review_5, boundary_stability_review_2)
- receipts/audit_giant_20261004/mathematics/PROOF.md
- receipts/full_20261002
- receipts/catalogue_profiles_20261002
