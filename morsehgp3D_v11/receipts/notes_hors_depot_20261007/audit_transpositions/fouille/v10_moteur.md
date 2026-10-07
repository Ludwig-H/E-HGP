# Fouille de la v10 (générateur, catalogue, raccord R2, vitesse) : ce qui se transpose utilement dans la v11

4 octobre 2026, rédigé de 12 h 32 à 13 h UTC (heure lue par `date -u`). Auditeur de la source « v10 : générateur,
catalogue, raccord R2, vitesse » de l'audit des transpositions ([contexte](../CONTEXTE.md)). Les deux cartes
[CARTE_V11](../cartes/CARTE_V11.md) et [CARTE_V10_VITESSE](../cartes/CARTE_V10_VITESSE.md) ont été lues d'abord ;
ce rapport ne les répète pas, il va plus loin dans ma source et la recoupe avec le code v11.

```text
phase=exploration_v11_hors_registre (audit des transpositions, lecture seule)
backend=cpu_reference
profile=quantized_u21_input_only (v11) ; quantized_u18_input_only (v10 et R2 mesurés)
public_status=not_claimed
GCP non utilisé ; aucune construction ni exécution native ; aucune commande git qui écrit
```

Contrat (rappel de l'utilisateur) : tour HGP FULL des trames SemanticKITTI sans sol, grille 1 mm, moteur entier
exact, **100 ms sur G4 à K = 5**, et si possible K = 10. Le jalon de 200 ms n'est pas le but.

## Étiquettes et sources

**M** = mesuré (reçu G4, ou reçu local nommé, précisé), **E** = estimé (arithmétique sur des mesures, méthode
donnée), **C** = conjecture. **[lu]** = établi par lecture du code. Les mesures locales (codespace Zen 3 chargé)
ne valent que pour des rapports entre variantes.

| Abréviation | Source (lecture seule) |
| --- | --- |
| V10 | `morsehgp3D_v10/` ; `src/catalogue/generator.cpp` est identique à `777406b82` (v10 rapide) |
| R2 | `build/v10-integration-r2/src/morsehgp3D_v10/` (`865f5e6`). Le générateur R2 = générateur v10 + gardes (budget de nœuds, règle M ≥ K+3, refus u32, `bad_alloc` rendu en refus) ; aucune boucle chaude ne change [lu, `diff` des deux `generator.cpp`] |
| V11 | `build/v11-claude-20261003/morsehgp3D_v11/` à `origin/main` = `c22be4e41` ; sources du moteur = `b87285378` (CARTE_V11 § 0) |
| J3 | feuille v3 privée de la v10, **conservée** dans `morsehgp3D_v10/receipts/audit_continu_20260929/performance_corrected/observed/feuille/src/morsehgp3D_v10/src/catalogue/leaf.hpp` (412 lignes) et `.../observed/feuille/J3_feuille_v3.patch` ; mesures `.../observed/feuille-verif/mesures/ab_abba_t1.txt`, `ab_abba_t4.txt`, `.../observed/feuille/profil/callgrind/resume.txt`, `.../observed/feuille/verif/grand_livre_v3_vs_j2c.txt`, `.../observed/feuille-verif/fuzz/*.tsv`, `.../observed/feuille-verif/mut/m404.tsv` ; contre-audit `morsehgp3D_v10/audits/audit_continu_20260929/performance/CONTRE_AUDIT_PROTO_CPU_20260929.md` |
| V3B | frontière privée v3b : `.../performance_corrected/observed/frontiere/frontiere_v3b.patch`, `.../frontiere/mesures/complet_c_resume.txt`, `modele_g4.txt`, `.../frontiere/controles/differentiel_1_4_48_proto_v3b.txt`, `tsan_plein_cadre_v3b.txt` |
| RED | `build/v10-persist/gpu_design/reduction_algorithmique_cpu/CONCEPTION_REDUCTION_CPU_20260929.md` (lots A, B, C, mesures locales sur quarts) |
| GPU, S7 | `build/v10-persist/gpu_design/voie_partielle_feuilles/CONCEPTION_VOIE_GPU_PARTIELLE_20260929.md` ; `morsehgp3D_v10/receipts/g4_session7_cuda_probe_20260929/README.md` |
| GEN2 | `morsehgp3D_v10/docs/conception/GEN_v2.md` (conception du générateur v10, § 3 lemmes, § 9 `LeafOracle`) |
| L01, L05 | `build/v11-persist/audit_v10/L01_MATH_CATALOGUE.md`, `L05_CODE_CATALOGUE.md` et leurs dossiers `preuves_*` |
| S4 | reçu G4 v10 session 4 (`777406b82`), tel que relu dans CARTE_V10_VITESSE § 1 |
| AB7 | `receipts/developpement_20261003/pipeline_g4/sessions/claudeab7/results.tar.gz` (v11 `b87285378`) : **extrait et relu par moi** (prises `t_new_lidar_ng0*_w*_r*.stdout`, `perf_new_self.stdout`) |
| PROF1 | `.../pipeline_g4/sessions/claudeprof1/results.tar.gz` : profils `perf` G4 W1/W48 de la base `a45daff3a` (même catalogue que `b87285378`) ; extrait et relu par moi |
| NOTE3 | `receipts/audit_dialogues_20261004/NOTE_CLAUDE_AUDIT_V11_20261003.md.snapshot` |
| VERROUS | `audits/QUESTION_CLAUDE_VITESSE_100MS_20261004.md` (développeur v11, ce matin 12 h 15 UTC) |
| CG11 | `build/v11-persist/conception/CONCEPTION_GENERATEUR.md` (conception v11 du 2 octobre) |

---

## 0. Résumé

1. **Même objet, mêmes candidats.** Le catalogue v11 dérive du générateur R2, qui est le générateur v10 rapide
   plus des gardes. Sur 08/000000 à K = 5, la v11 teste **le même nombre de candidats par boule que la v10 :
   56,6** (dominance 24,85 + droites évaluées 23,94 + q4 7,86, AB7 ; v10 : 24,8 + 23,9 + 7,8, L05 § 6.1), juge
   3,27 M présentations dans les deux. L'écart de vitesse vient des formes de calcul, pas des lemmes.
2. **Le gisement principal de ma source n'est pas dans le produit v10 mais dans sa feuille J3**, restée privée et
   pourtant conservée (source complète et mesures) dans un reçu v10. Mêmes présentations jugées, mêmes 13
   compteurs, dumps et niveaux exacts identiques ; sur la **trame 02 entière du contrat**, le CPU de l'étage des
   boîtes baisse de **×1,37 à K = 5 et ×1,55 à K = 10** (M local, ABBA), les mauvaises prédictions de branchement
   de ×2,2 à ×2,4 (M callgrind). La v11 l'a écartée (NOTE3 § 4) au motif que sa passe unique « est du même ordre »
   que l'étage des boîtes v10 ; or J3 se mesure **contre** cet étage, et la passe unique v11 le dépasse de ×1,27 à
   un fil sur G4 (M). Estimation : **−30 à −75 ms** de passe unique à W48 (E), davantage à K = 10.
3. Six autres idées retenues : portes d'échelle Euler à K+2 et restriction (absentes de la v11, indispensables pour
   qualifier 1 et 3) ; filtre G1 et préambule (tête parallèle v3b, noyau sans branchement) ; **feuille 24 à
   K = 10** (la v11 code 16 en dur : ×1,6 à ×1,8 sur le catalogue K = 10 d'après la v10) ; arènes réutilisées sans
   atomique par nœud ; `LeafOracle` (census de la tour par les listes K-certifiées du catalogue, jamais codé) ; voie
   GPU par sous-arbres (plafonds d'Amdahl mesurés en v10).
4. **Aucune transposition CPU de la v10 ne donne 100 ms.** Cumulées, elles ramèneraient FULL K = 5 W48 de
   352–412 ms vers ~200–360 ms selon la trame et la fourchette (E/C, § 5). Le seul levier de ma source dont le plafond est compatible avec 100 ms
   est le déchargement des **sous-arbres** du catalogue sur le GPU de la G4 (c(64) = 15 % de l'étage des boîtes
   resté sur CPU, M v10), qui suppose une feuille à source unique : J3 l'est déjà.
5. Le § 6 répond, depuis ma source, à cinq des sept verrous posés ce matin par le développeur (VERROUS § B).

---

## 1. Ce qui a été lu

- V10 : `generator.cpp` (876 lignes, en entier), `site_tree.cpp/hpp`, `sched/pool.*`, `sched/sort.hpp`,
  `docs/conception/GEN_v2.md` (§ 3, 6, 8, 9, 10), `docs/DEVELOPPEMENT_FRONTIERE_ET_PRECISION_20260930.md`,
  reçus `g4_session2_perf`, `g4_session7_cuda_probe`, `audit_continu_20260929/performance_corrected` (source J3,
  patch v3b, mesures, fuzz, mutants), `group_moments_20260930`, audits `audit_continu_20260929/performance/*`.
- R2 : `generator.cpp` (diff complet contre la v10), `notes/A_FAIRE_APRES_RACCORD.md`, `notes/pool.md`.
- Privé v10 : RED (en entier), GPU (§ 1–3, 9–10), L05 (en entier), L01 (§ juges), `preuves_l05_code_catalogue/`
  (`ablation_M.txt`, `ablation_kT.txt`, microbancs, invariants), `preuves_l01_math_catalogue/euler_juge.py`.
- V11 : `src/catalogue/` (leaf, boxes, internal, small_pair_graph, center_line_cache, adaptive_*, single_pass*,
  support, catalogue.hpp), `src/num/` (sphere, predicates, q4_weights, center_region, level), `src/index/`,
  `src/tower/full_domain.cpp`, `src/core/buffer.*`, `src/sched/`, docs CATALOGUE*.md, FULL_DOMAIN.md,
  PERFORMANCE_FULL.md, CONCEPTION_MOTEUR.md, MATHEMATIQUES.md § 8, Q4_POIDS_PRESENTATION.md ; reçus AB7, PROF1,
  `audit_deep_20261004/performance`, `audit_heritage_20261004`, `pts4_review_20261003` ; NOTE3, VERROUS ; CG11.
- Pistes fermées : `docs/archive/abandoned/README.md`, `morsehgp3D_v3/audits/PISTES_FERMEES.md`,
  `morsehgp3D_v5/docs/PISTES_FERMEES.md`, `morsehgp3D_v6/docs/PISTES_FERMEES.md`.

---

## 2. Mécanisme par mécanisme : v10 / R2 contre v11 (catalogue, domaine, index)

| Mécanisme | v10 `777406b82` = R2 `865f5e6` | v11 `b87285378` | Effet constaté |
| --- | --- | --- | --- |
| Repère des boîtes | T = 6 bits sous-unitaires (`generator.cpp` l. 22) | T0, arrêt à largeur 1 (`boxes.cpp` l. 143–155) | neutre sur LiDAR : dumps identiques et travail à ±0,3 % pour T = 6/3/0 (M, `ablation_kT.txt`) |
| Filtre G1 d'un nœud | réservoir 3K, forme D-loc, comptage **sans branchement** en deux temps (s0 puis le reste, l. 496–559) | mêmes termes, témoins prétraités, mais boucle à **sortie anticipée** `found < kmax` (`boxes.cpp` l. 72) | la boucle v11 n'est pas vectorisable : d'où « `-march` sans gain » (PROF1) ; filtre = 7,3 % du CPU à W48 (PROF1), 8,0 % à W1 (AB7) |
| Liste par nœud | `std::vector` local, **aucun atomique partagé** (l. 570) | `Buffer::allocate` par nœud (`boxes.cpp` l. 60) : CAS `used` + CAS `peak` + `fetch_sub` (`buffer.cpp` l. 19, 25, 47) | `buffer_acquire/release` 0,86 % du CPU à W48, sous 0,2 % à W1 (M, PROF1) |
| Tête de l'arbre | largeur à barrières, 19 482 tâches à P = 48, 21,8–23,2 ms (×2,1) (S4) | plan « lourds d'abord » ≤ 1 024 tâches, 27 rondes, racine sérielle ; préambule 18,8–21,0 ms à W48, ×4,3 (AB7) | parité ; les deux sont un squelette quasi séquentiel |
| Énumération de feuille | **étages** : paires, triplets (droite une fois par triplet, table H des triplets vivants), quadruplets par ET de trois lignes de H (l. 319–465) | **DFS** de préfixes + graphe de paires (≤ 32 sites) + lignes vivantes + cache J2 (`leaf.cpp` l. 198–258) | mêmes 56,6 candidats par boule, mais la v11 ajoute 43,7 M consultations du cache (33,5 par boule) et 120,4 M préfixes logiques sur 08/000000 (M, AB7) |
| Recensement local | tous les sites de la feuille, `geom::side` → `int` | tous les sites, `num::side` → `Result<int>` + `checked_add` par site (`leaf.cpp` l. 156, 163) | 37,0 M tests sur 08/000000 (M, AB7) ; J3 (privée) ne teste que les sites que les masques ne décident pas |
| Niveau q3 | centre seul avant la boîte, niveau après admission (l. 423–426, 262–275) | `Sphere::through(a,b,c)` construit niveau (produit `Wide`) et deux certificats **avant** `center_in_box` (`leaf.cpp` l. 113–120, 227–233 ; `sphere.cpp` l. 35–52) | « q3 différé » déjà retenu (`receipts/audit_heritage_20261004/`) |
| Quadruplet | centre, boîte, quatre orientations (l. 456–459) | `Q4Candidate::through` : N, det, poids stricts, puis boîte ; niveau différé (`q4_weights.hpp`) | la v11 a déjà l'essentiel du réordonnancement exact (variante B de L05 § 6.7) |
| Droite des centres | i64 + i128 final, une évaluation par triplet | `Int<3B+5>` = i128 à u21 (`center_region.cpp` l. 12–13, 78–82), via cache | J3 : voie i64 sans branchement, termes de paire calculés une fois |
| Compteurs de la boucle chaude | `++` nus, ajout une fois par feuille | `checked_add` par test (≈ 0,5 milliard par trame, NOTE3 § 6) | verrou 1 du développeur |
| Émission | `Rec` de 104 o dans des vecteurs par fil (l. 41–48) | `Emission` de **104 o** en pages fixes par ordinal (déduit exactement de AB7 : `arena_blocks` 9 102 et `arena_capacity_bytes` 177 930 240 donnent 5 608 pages d'émissions et 3 494 de population) | parité de format |
| Ordre et assemblage | PSRS sur clé `double`, bandes 2^-40 ; 28 ms à W48 (S4) | tri indirect à clés F3/F4, assemblage par blocs ; 26 ms (tri 11,5 + rangs 9,7 + compactage 4,8, AB7) | parité |
| Taille de feuille M(K) | **table** 12 / 16 / 24 / 28 (l. 641), calibrée | **16 pour tout K** dans les bancs (`bench/points_export.cpp` l. 377), 32 par défaut (`catalogue.hpp` l. 26) | à K = 10, la feuille 16 coûte ×1,6–1,8 (idée 04) |
| Census de la tour | `SiteTree` k-d serré, marges flottantes figées u18, ≈ 1 µs (CARTE_V10 T5) | arbre de plages Morton depuis la racine, bornes i128 ; ≈ 30 bornes et 78,5 tests de points par appel à l'ordre 5 (M, AB7 ; NOTE3 A6) | voir idée 06 (route par les feuilles du catalogue) |
| Protocole de mesure | troisième passe chaude d'un processus (S4) | processus neuf par prise (AB7) | 3–7 % d'écart passe 1/passe 3 en v10 (S4) ; voir idée 05 |
| Feuille GPU | J3 à source unique hôte/GPU, compilée sm_120 (privé) | aucune voie GPU | voir idée 07 |

Ce que la v11 a **perdu en se refaisant**, côté catalogue : la table des triplets vivants, le niveau q3 après
admission, les compteurs nus, l'absence d'atomique par nœud, la table M(K), la mesure en passe chaude. Ce qu'elle
n'a **jamais repris** alors que c'était prêt : la feuille J3 entière, la tête parallèle v3b, les juges d'échelle
de l'audit v10. Ce qu'elle fait **mieux** : le graphe de paires, le niveau q4 différé, les poids q4 avant la
boîte, l'émission seulement si S* est la présentation (sans mémo), le budget mémoire honnête, le pipeline des
forêts.

---

## 3. Idées retenues

Classées par rendement attendu vers 100 ms à K = 5, puis par nécessité (outils). Aucune ne rouvre une piste
fermée : le catalogue critique reste l'objet commun, sans mosaïque de Delaunay d'ordre supérieur ni catalogue en
C(n, k).

### v10_moteur_01 — Porter la feuille J3 : étages, table H, census par masques, triangle médian, enveloppe du tétraèdre, voie i64

**Source.** J3 `leaf.hpp` : dominance dans les deux sens `dom`/`domby` sans branchement (l. 107–134) ;
**recensement limité aux sites que les masques ne décident pas** (lemme R, l. 136–170) ; côté q2 en i64
(l. 172–179) ; droite des centres sans branchement avec termes de paire `U`, `PP`, `NN` calculés une fois par
paire (l. 238–249, 263–277) ; **enveloppe du triangle médian** avant le centre q3 (lemme M3, l. 301–309) ; table H
des triplets vivants (l. 300, 311) ; quadruplets par ET de trois lignes de H, **enveloppe du tétraèdre** (lemme E4,
l. 372–374) puis intérieur strict par barycentriques de Gram sans le centre (lemme B4, l. 375–386) ; voie étroite
i64 choisie une fois par feuille (l. 400–410). Intégration : `J3_feuille_v3.patch` (`LeafSink` : centre, boîte,
census par masques, puis `judge_finish`, qui calcule le niveau **après** admission).

**Preuve (M sur la v10, E pour la v11).**

- Trame 02 entière (45 845 sites, celle du contrat), CPU du processus pendant l'étage des boîtes, A/B adverse
  ABBA local : K = 5 un fil 7,683 → 5,603 s (**×1,371**, n = 6, appariés ×1,359–1,381) ; K = 10 32,928 → 21,278 s
  (**×1,548**) ; quatre fils ×1,362 et ×1,559 ; quart 01 ×1,361 (`ab_abba_t1.txt`, `ab_abba_t4.txt`). Mesures
  locales sous charge 31–47 : rapports seulement.
- Callgrind, quart 01 : instructions de la feuille ×1,46 (K = 5) et ×1,67 (K = 10) ; branchements ×2,12 et ×2,36 ;
  mauvaises prédictions ×2,16 et ×2,37 (`callgrind/resume.txt`).
- Identité : 13 compteurs égaux à J2c sur 10 couples (deux trames entières, un quart, deux synthétiques,
  K = 5 et 10) ; empreintes des niveaux exacts égales à 1 et 4 fils ; fuzz : **1 060 cas conclusifs, 0 désaccord**
  (dix délais publiés, exclus du dénominateur) ; mutants : 9 tués, le 10e prouvé équivalent par parité
  (`m404.tsv` ; CONTRE_AUDIT_PROTO_CPU § 1). L05 l'a rejouée indépendamment : −19,3 % d'instructions, −40 % de
  mauvaises prédictions, ×1,33 sur l'étage, 30 contrôles conformes à son oracle brut (L05 § 6.7).
- Sélectivités (RED § 6.2, quart 01, M local) : M3 retire 60 % (K = 5) à 67 % (K = 10) des calculs de centres q3,
  E4 25 à 31 % des centres q4. Supprimer la droite des centres doublerait les quadruplets (1,34 → 3,07 M à K = 5) :
  J3 la garde.

**Déjà dans la v11 ?** Non, sauf l'esprit de B4 (`Q4Candidate` et ses poids, `q4_weights.hpp`). La v11 a gardé le
DFS (`leaf.cpp` l. 198–258), ne construit que `dom` (`prepare`, l. 39–53), teste tous les sites au census
(l. 154–173), construit le niveau q3 avant la boîte (l. 113–120), évalue la droite en i128 via le cache (l. 85).
La table H a été explicitement écartée (`docs/CATALOGUE_OPTIMISATIONS.md` l. 69) et J3 « examinée » sans port
(NOTE3 § 4). Les verrous 1 (compteurs) et 2 (q3 différé) en cours (VERROUS § B) sont **deux morceaux de J3**.

**Doctrine.** Tout entier, aucune décision flottante ; mêmes présentations jugées, donc même catalogue après le
tri canonique (l'ordre d'émission dans une feuille change, la sortie non : ordre total (niveau, S*), ex æquo
refusés) [lu]. La règle v11 « émettre seulement si S* est la présentation » (`leaf.cpp` l. 185) reste valide :
S* est énuméré dans la même feuille. Bornes à refaire : la voie large J3 était écrite pour |X| < 2^24 (u18 × 2^6),
ce qui couvre aussi les coordonnées T0 de la v11 jusqu'à B = 24 (E) ; la voie étroite doit être regardée par
profil (en T0, les mineurs de Gram tiennent en i64 si les différences sont < 2^13 : 98,5 % des listes de feuille de
la trame 02 à K = 5, 97,7 % à K = 10, L05 histogramme repris par CG11 l. 74 ; la droite tient en i64 pour 100 % des
feuilles). Le grand livre change de sens (`prefixes`, `region_line_*` remplacés par paires, triplets, droites,
quadruplets, tests de census évités) : contrat de compteurs à réécrire.

**Gain attendu (E).** Base G4 à un fil, trame 02 : étage des boîtes v10 3 508,5 ms (S4) ; passe unique v11
4 461,5 ms (AB7) ; rapport ×1,27, dont ×1,06 dû à u21 (DEEP). Fourchette : si seul le gain propre de J3 passe,
×1,37 ; si le port atteint le coût unitaire de J3 sur la v10, 3 508,5 / 1,371 × 1,06 ≈ 2 710 ms, soit ×1,65. À
W48, sur les médianes indépendantes de la passe unique (195,0 / 167,0 / 159,5 ms, AB7) : **−53 à −77 ms** (00),
−45 à −66 (01), −43 à −63 (02). Garde-fou : Zen 5 prédit mieux les branchements et le gain SMT peut baisser quand
les mauvaises prédictions disparaissent ; borne basse prudente **−30 ms**. K = 10 : ×1,55 de plus sur l'étage (M
local).

**Ordre de port conseillé** (chaque tranche A/B G4 à dumps identiques) : (a) census par masques (`domby` rempli
dans la même boucle de `prepare`, masque d'extérieurs tenu par profondeur dans `extend`, saut dans
`census_and_emit`) ; (b) triangle médian avant `sphere_of`, enveloppe du tétraèdre avant `q4_of` ; (c) compteurs
locaux et niveau q3 différé (verrous 1–2) ; (d) droite i64 à termes de paire ; (e) étages + table H à la place du
DFS et du cache pour m ≤ 32, le DFS restant la référence différentielle et le chemin des feuilles larges.

**Risques.** La v5 a fermé un « étage i64 du préfiltre q4 comme gain de temps » faute de gain mesuré
(`morsehgp3D_v5/docs/PISTES_FERMEES.md`) : mesurer chaque tranche, n'attribuer aucun gain à la voie i64 sans A/B.
Deux mutants de bord survivaient au différentiel LiDAR (M-C3 sur plans dyadiques, M-C5 débordement de la voie
étroite) et n'étaient tués que par des fixtures (RED § 12.4–12.5) : porter F-DYA, F-TRI, F-L64 et la liste de
mutants `m404`.

### v10_moteur_02 — Portes d'échelle du catalogue : Euler à K+2, restriction, égalité des listes

**Source.** L01 E4 : juge d'Euler indépendant (`preuves_l01_math_catalogue/euler_juge.py`, 158 lignes) qui traite
exactement les coquilles étendues ; validé sur petits nuages (267 contrôles, 5 133 boules dont 696 étendues), puis
appliqué aux trois trames du contrat à k_cat = 12 : somme égale à 1 aux ordres 1 à 10 ; deux mutants (une boule
de trop ou de moins) donnent ÉCART (`mutants_euler.txt`). Restriction `cat(K) = restrict(cat(K+2))` à K = 5 et 10,
sha256 égaux (`jkm2_lidar.txt`, `j1_juge.py`). L05 § 5.5 : le mutant `m_frontier` **perd 2 134 à 9 523 boules avec
`status ok`**, invisible à la porte du dépôt v10, tué par Euler et par la restriction ; L01 cite L02 § 7.5 : sur
3 062 retraits d'une boule admissible, la tour publie 295 forêts fausses sans refus, toutes vues par Euler. RED
§ 12.2 : porte `--list-hash`, empreinte commutative de (boîte, liste) sur tous les nœuds, plus forte que le dump
pour tout changement du filtre ou de la tête.

**Déjà dans la v11 ?** Documentés seulement : MATHEMATIQUES § 8 (J1 restriction, J3 Euler « diagnostic
nécessaire », l. 342–366) et CONCEPTION_MOTEUR l. 145. Aucun test, banc ni outil ne calcule Euler (`git grep -i
euler` sur `tests/`, `bench/`, `tools/` : rien côté catalogue) ; la restriction n'est jugée que dans le petit lot
de la sonde (84 restrictions, ≤ 14 sites, `tests/catalogue/README.md`).

**Pourquoi maintenant.** Les idées 01, 03 et 05 changent la feuille, la tête de l'arbre et l'allocation. Le juge
Gram/Fraction ne voit jamais une feuille de plus de 14 sites, une coupe de frontière ni un ordonnancement ; les
pertes de la v10 (`m_frontier`, `m_multiword`, `m_band`) étaient silencieuses. Ces juges sont globaux et non
exhaustifs, conformes à la règle du dépôt (invariants à l'échelle, jamais de juge O(n^3)).

**Coût et gain.** 1 à 2 jours (exporter l'histogramme N(q, p) et les seules coquilles étendues ; Cat_{K+2} sur
les trois trames et 8k/16k/32k sur G4). Aucun gain de temps ; c'est le filet qui permet de porter 01 et 03.
Rappel de MATHEMATIQUES § 8 : deux omissions peuvent se compenser ; Euler est nécessaire, pas suffisant.

### v10_moteur_03 — Filtre G1 : tête parallèle par tranches (v3b) et noyau sans branchement

**Source.** V3B : un nœud de tête dont la liste parente a au moins max(4 096, n/P) sites est filtré par tranches
de 1 024 sites sur le Pool ; le réservoir de chaque tranche est fusionné dans l'ordre des tranches, ce qui redonne
exactement le réservoir séquentiel (argument d'ordre (dd, rang) dans le commentaire du patch) ; vol de tâches avec
64 essais puis sommeil de 200 µs (`kHeadList`, `kHeadGrain`, `kIdleSpins`, `kIdleSleepUs` en tête du patch).
Contrôles : dumps identiques à 1, 4 et 48 fils sur 10 couples, arbre égal à J2c, TSan propre sur la trame 02
entière (K = 5 et 10, 8 fils). Noyau : RED lot A (forme D-loc sans branchement en SoA, 2,3–2,4 cycles par test en
AVX2) ; L05 § 6.4 : 5,2–5,5 cycles par test sans `-march`, −39 % d'instructions avec `x86-64-v3`, −48 % avec les
boucles interverties (sites dans la boucle interne), ×1,08 sur tout l'étage localement.

**Preuve.** Local (M) : `t_frontier` ×1,90–2,45 à K = 5, ×1,42–2,03 à K = 10 (4 et 8 fils), mais catalogue
entier ×0,88–1,05 en local (`complet_c_resume.txt`). Modèle G4 (E, calibré sur S4) : à P = 48, frontière mesurée
26,6 ms dont 17,8 ms de barrières, séries et déséquilibre ; estimée 2,6–4,8 ms après v3b (`modele_g4.txt`).

**Déjà dans la v11 ?** Non. Racine sérielle (`docs/CATALOGUE_FRONTIERE_ADAPTATIVE.md` l. 57) ; chaque ronde lance
une tâche par enfant (`adaptive_frontier.cpp` l. 82) ; le pilote fait seul le balayage de priorité de chaque liste
(`capture`, `adaptive_prepare.cpp` l. 11–31) et un tri par insertion des nœuds vivants (`select`, l. 46–64,
quadratique en au plus 1 024 nœuds), sur 27 rondes. Préambule 85,5 ms à W1, 20,1 ms à W48 sur 08/000000 (×4,3, M
AB7). Le développeur l'a nommé (NOTE3 § 6, pistes 1 et 2 : « filtre G1 vectorisable », « filtre d'un gros nœud
partagé entre workers ») ; il n'est pas dans les verrous du jour.

**Gain attendu (E/C).** Préambule 18,8–21,0 ms → 4–8 ms, si son chemin critique est bien le filtrage des grosses
listes (C : la part du pilote n'est pas mesurée ; la mesurer d'abord, temps par ronde séparé pilote/Pool) ; noyau
÷1,5–2 sur le filtre (7,3 % du CPU à W48) : −4 à −8 ms sur la passe unique. **Total −15 à −25 ms à W48.** Ce poste
compte double pour 100 ms : il appartient au squelette séquentiel (CARTE_V11 § 6, point 3).

**Doctrine.** Mêmes listes, mêmes nœuds ; porte d'égalité des listes (idée 02) ; TSan et stress pour tout
ordonnanceur plus fin (leçon de la course du Pool v10, CARTE_V10 N9). Le comptage `filter_tests` historique peut
rester logique.

### v10_moteur_04 — Table M(K) de la v10 : feuille 24 à K = 10

**Source.** `generator.cpp` l. 641 (12 / 16 / 24 / 28) ; L05 § 6.10 et `preuves_l05_code_catalogue/ablation_M.txt`
(trame 02, compteurs déterministes, catalogue identique pour tout M) : à K = 10, **M = 16 → 11 063 197 nœuds,
850 tests du filtre et 137,8 candidats par boule, coût modèle 17 455 cycles par boule, `t_boxes` local 32,77 s ;
M = 24 → 1 065 585 nœuds, 235, 57,9, 10 796 cycles, 18,55 s** ; M = 14 → 41,4 M nœuds. À K = 5, M = 16 est
l'optimum (9 783 cycles). Mécanisme (R2 `generator.cpp`, commentaire de `check_catalogue_params`) : près de
M = K + 3 la liste certifiée ne tombe plus sous M près des strates du diagramme de Voronoi d'ordre K et l'arbre
explose.

**Déjà dans la v11 ?** Non. `bench/points_export.cpp` l. 377 fixe `leaf_size = 16` pour tout K, y compris les
campagnes K1..10 `claudepts3/4` (FULL 16–109 s par trame ; médianes de 6,85 et 7,63 µs de mur par boule à 4 fils
sur LiDAR, 8,6 à 13 µs sur les synthétiques volumiques : E, mes calculs sur
`receipts/pts4_review_20261003/case_metadata.json.gz`, 22 processus simultanés, donc indicatif) ; `CatalogueParams{}` vaut 32 (`catalogue.hpp` l. 26) ; CARTE_V11 § 4 :
« feuille 16 admise » à K = 10. Le graphe de paires et le cache J2 (capacité 32) acceptent 24.

**Gain attendu (E).** Catalogue K = 10 ×1,6 (modèle) à ×1,77 (temps local bruité). Aucun effet à K = 5. Coût
nul : un paramètre et une ablation de compteurs (W1, déterministe) sur la v11 avant chronométrage, puisque son arbre
T0 diffère de celui de la v10.

### v10_moteur_05 — Zéro allocation ni atomique par nœud, arènes réutilisées et pré-touchées d'une trame à l'autre

**Source.** V10 : liste de nœud en `std::vector` local sans atomique partagé (`generator.cpp` l. 570), états par
fil `alignas(64)` (l. 50–63) ; RED lot A, A5 : « arène par profondeur, `lists[depth]` pré-dimensionnée, aucune
allocation par nœud » ; la v10 se mesurait en troisième passe chaude d'un processus (S4 : passes 1 à 259,8 /
218,6 / 265,5 ms contre 252,0 / 204,2 / 253,6 ms en passe 3, soit 3 à 7 %) ; CG11 G13 et l. 76 citent une mesure
privée de la v10 : **émission 993 → 115 cycles par boule quand les pages sont déjà touchées** (quart 01, K = 10 ;
fichier privé disparu avec `build/v10-perf`, donc E non revérifiable).

**Déjà dans la v11 ?** Partiellement en cours : le verrou 3 propose un bloc par tâche réservé sur un majorant
(VERROUS § B). Ce qui n'y est pas : la réutilisation entre trames et le pré-toucher. Aujourd'hui, un `Buffer` par
nœud (`boxes.cpp` l. 60) ; deux allocations par page d'arène (`single_pass_storage.hpp` l. 58–59), 9 102 pages
par trame (AB7) ; tout est touché pendant FULL dans un processus neuf. Profil W48 (M, PROF1) :
`asm_exc_page_fault` 3,12 % du CPU en cumul (dont `do_anonymous_page` 2,47 %), `native_queued_spin_lock_slowpath`
1,55 % et `__pte_offset_map_lock` 1,51 % (contention des tables de pages), `buffer_acquire` + `buffer_release`
0,86 % ; à W1 aucun de ces symboles n'atteint le seuil de 0,21 % du rapport publié. Le résidu non chronométré du
domaine (destruction des arènes, table des supports) vaut 11,8–13,9 ms à W48 (CARTE_V11 § 2.1). La borne par tâche
existe déjà (`AdaptiveFrontier::suffix_memory_bound`, `adaptive_frontier.cpp` l. 172–188 : 4·count·(3B − profondeur)).

**Gain attendu (C, symptômes M).** Fautes de page (verrous compris dans leur cumul) et atomiques du budget
≈ 4 % du CPU à W48 (3,12 + 0,86 %, ≈ 0,55 s sur 13,7 s) ; une partie du résidu de destruction ; **−5 à −20 ms à
W48**. Pour un flux LiDAR à 10 Hz, la session résidente est le cas d'usage
réel du contrat de 100 ms.

**Doctrine.** Budget honnête si l'arène est réservée une fois dans le `MemoryBudget` (pic déclaré ≥ pic réel,
admis avant calcul) ; publier les deux mesures, froide et chaude, dans chaque reçu, sans substituer l'une à l'autre.

### v10_moteur_06 — `LeafOracle` : census des descentes par les listes K-certifiées des feuilles du catalogue

**Source.** GEN2 § 3.4 (théorème C : si la boule a moins de K intérieurs, la liste K-certifiée de la boîte du
centre contient toute la boule fermée ; sinon elle en contient au moins K intérieurs), § 3.9 (lemme O, oracle
K-NN pour la tour), § 9 (`LeafOracle` : feuilles et listes en CSR, `locate(c)`, `knn_closed(c, k)`,
`lookup(c, r²)` ; Σm ≈ 12 M identifiants, ≈ 49 Mo à K = 5 sur la trame 02, estimation GEN2). **Jamais codé** dans
la v10 (L05 § 4 : « absent ; la tour reconstruit ses requêtes avec un arbre k-d séparé »).

**Déjà dans la v11 ?** Le théorème oui (G2, `docs/CATALOGUE.md` l. 42–45), la route non. Chaque census manqué par
la table des supports parcourt l'index global depuis la racine (`census_workspace.cpp` l. 47–65) : ≈ 30 bornes
par requête (NOTE3 A6), **78,5 tests de points par appel à l'ordre 5** (17 055 923 / 217 326, M AB7 08/000000 W1),
≈ 3,6–3,9 µs par appel ; le census pèse ≈ 10–12 % du CPU à W1 (`CensusWorkspace::query` 1,7–2,1 %,
`power_bound_signs` 4,5–5,0 %, `bound_terms` 2,0 %, plus une part de `side`) (M, AB7 et PROF1). Le verrou 4 ne
liste que (a) l'index sous F6 et (b) un arbre k-d sous F1–F6.

**Mécanisme.** Garder, à la fin du catalogue, les boîtes et listes des feuilles et les plans de coupe ; localiser
la feuille du centre (≈ 36 comparaisons rationnelles exactes) ; recenser ses ≈ 14 (K = 5) ou ≈ 22 (K = 10) sites.
Exact et **identique** au census global quand la liste compte moins de K intérieurs : alors I et toute U y sont,
et les k témoins d'une saturation (les k plus petits `SiteIdx` intérieurs, puisque le parcours v11 est préfixe
dans l'ordre de Morton [lu]) aussi. Sinon (au moins K intérieurs dans la liste, ou centre hors de toute boîte
ajustée, ce qui implique au moins K intérieurs), repli sur le census global, car les témoins canoniques dépendent
de l'ordre global.

**Gain attendu (C).** Si une fraction f des appels est servie à ≈ 0,4 µs : f = 0,5 → −0,5 s de CPU à W1 sur
08/000000, ≈ −15 ms de résolution régulière à W48 ; f = 0,9 → −25 à −30 ms. **f est inconnu** : à l'ordre 5,
k = K et toute saturation retombe sur le repli. Premier pas sans risque : compter les résultats de census par
genre (complet ; saturé avec p < K ; p ≥ K). Mémoire : +40–50 Mo (E). À coordonner avec l'auditeur de la tour.

### v10_moteur_07 — Voie GPU du catalogue par sous-arbres, à partir de J3

**Source.** GPU § 3 (M, rdtsc local, trame 02, K = 5) : part c(L) de l'étage des boîtes qui reste sur CPU quand
le GPU prend les sous-arbres de liste parente ≤ L : **L = 16 (feuilles seules) 52,3 %, plafond ×1,91 ; L = 32
23,8 % ; L = 64 15,1 %, ×6,6 ; L = 256 7,9 %, ×12,7** ; quart 01 à K = 10 : 11,8 % à L = 64. GPU § 9.2 : charge GPU
à L = 64 ≈ 3–10 ms par trame (E). S7 (M G4) : `__int128` exact sur le device (16 777 216 produits, 0 écart), débits
2 579 G op/s i64 et 1 111 G op/s i128, 56,8 Go/s hôte→GPU, 1,86 µs par lancement de noyau vide. J3 compilée pour
sm_120 : 126 registres, pile 5 632 o, aucun débordement (CONTRE_AUDIT_PROTO_CPU § 1) ; prototype `leaf_fast.cuh` :
dumps identiques sur 10 entrées (GPU § 1).

**Déjà dans la v11 ?** Non (`cpu_reference`) ; verrou 7 ouvert ce matin. Règle de la v5 : une piste GPU ne se
rouvre qu'avec un noyau mesuré sur G4 et une porte d'égalité (`morsehgp3D_v5/docs/PISTES_FERMEES.md`) ; la présente
proposition la respecte.

**Gain attendu (C).** La passe unique (160–195 ms à W48) garderait sur CPU ≈ 15 % de son travail à L = 64, soit
≈ 25–30 ms, le GPU (3–10 ms) se recouvrant : **−120 à −160 ms** (C). C'est le seul levier de ma source dont le
plafond est compatible avec 100 ms. Effort : 8 jours-agent pour la voie « feuilles », 6 à 8 de plus pour les
sous-arbres (GPU § 15). Ordre : J3 d'abord (source unique), puis noyau de sous-arbre (filtre + feuille) ; les
feuilles seules ne suffisent pas (plafond ×1,9).

---

## 4. Fausses bonnes idées écartées

1. **Filtres flottants par lots (« lot D », L05 § 6.7)** : quadruplets ×8–11 contre la séquence v10, mais
   seulement ×3–4 contre l'ordre exact réordonné que la v11 a déjà ; gain propre ×1,06–1,11 sur l'étage (L05 Q2) ;
   l'essai F6 de la v11 sur les signes de puissance n'a gagné que ≈ 1 % (`PERFORMANCE_FULL.md` l. 186–188). Pas
   avant J3.
2. **Énumération plate sans droite des centres** (CG11 G07) : sans la droite, ×2,3 de quadruplets (1,34 → 3,07 M à
   K = 5, 10,6 → 24,5 M à K = 10, RED § 7) ; ne se défend qu'avec des noyaux vectoriels certifiés jamais mesurés.
3. **Sous-maille T6 pour accélérer le LiDAR** (verrou 6) : dumps identiques et travail à ±0,3 % pour T = 6/3/0
   (`ablation_kT.txt`) ; T n'aide que les grilles entières (grille 16³ à K = 5 : quadruplets ×10 à T = 0).
4. **Grain fin à la v10** (19 482 tâches, frontière en largeur) : frontière 22–23 ms et ×2,1 (S4) ; la simulation
   LPT v11 est déjà proche de l'idéal (`CATALOGUE_FRONTIERE_ADAPTATIVE.md` l. 41–57) ; l'écart de passage à
   l'échelle vient surtout du gonflement du CPU sous SMT (×1,5 de W1 à W48, CARTE_V11 § 2.2), pas du déséquilibre.
5. **`-march` global** : sans gain mesuré sur la v11 (PROF1) ; seul un noyau écrit sans branchement en profite
   (idée 03).
6. **Émission de 16 octets, niveau recalculé depuis S*** (GEN2 § 8, CG11 G13) : 0,78 niveau distinct par boule
   (L05 § 6.5), le recalcul ne s'éviterait presque jamais ; la v10 était à parité de format (104 o). Le tri par
   base d'une clé calculée à l'émission reste une piste de la conception v11, sans mécanisme v10 plus rapide à
   porter.
7. **Crédit quantitatif de groupe (moments)** (`morsehgp3D_v10/receipts/audit_continu_20260929/group_moments_20260930/README.md`) :
   seule piste v10 qui réduirait le nombre de candidats, mais prouvée sur deux fixtures de 14 sites, sans aucun
   compte LiDAR ; coût Σ|ancres|·|groupes| non mesuré. À rouvrir seulement avec un compteur sur 08/000000.
8. **SiteTree v10 tel quel** : marges flottantes figées pour u18 (`site_tree.cpp`, `kMargin = 0.02`), non
   transportables sous F1–F6 (CARTE_V10 N1) ; voir idée 06 pour une autre route.
9. **Mémo linéaire des coquilles étendues, `emitted_level`, bandes repérées en série, Kruskal séquentiel** : la v11
   fait déjà mieux ou ne doit pas les reprendre (CARTE_V10 § 4).

---

## 5. Ce que ces transpositions font au budget de 100 ms

Base : médianes AB7 à W48, K = 5 (FULL 412 / 352 / 381 ms ; passe unique 195 / 167 / 160 ; préambule 20 / 19 / 21 ;
résolution régulière 116 / 86 / 96).

| Idée | Poste touché | Gain à W48 par trame | Statut |
| --- | --- | ---: | --- |
| 01 feuille J3 | passe unique | −30 à −77 ms | E (J3 M sur v10) |
| 03 filtre G1, tête v3b | préambule + filtre | −15 à −25 ms | E/C |
| 05 arènes résidentes | domaine et forêts | −5 à −20 ms | C |
| 06 `LeafOracle` | résolution régulière | 0 à −30 ms | C |
| **Cumul CPU** | | **−50 à −150 ms** | |
| FULL résultant (00 / 01 / 02) | | ≈ 262–362 / 202–302 / 231–331 ms | E/C |
| 07 GPU sous-arbres | passe unique | −120 à −160 ms | C |

Lecture : **les transpositions CPU de la v10 ne ferment pas 100 ms** (au mieux ≈ ×2 à ×2,6 au-dessus). Elles ne
touchent ni la queue de publication de l'ordre 5 ni le reste du squelette des forêts (≈ 50–60 ms), hors de ma
source. La seule route de ma source compatible avec 100 ms combine J3 et le GPU par sous-arbres, plus les leviers
des forêts que d'autres auditeurs apportent. À K = 10, l'idée 04 (×1,6–1,8 sur le catalogue) et J3 (×1,55) sont
les deux leviers qui comptent ; 100 ms y restent hors de portée.

---

## 6. Réponses aux verrous du développeur (VERROUS § B), depuis ma source

- **Verrous 1 et 2 (compteurs, q3 différé)** : ce sont deux morceaux de J3, qui les avait résolus ensemble
  (compteurs locaux ajoutés une fois par feuille, niveau dans `judge_finish` après admission) et y ajoutait le
  census par masques, le triangle médian, l'enveloppe du tétraèdre, la droite i64 et la table H. Porter la feuille
  par tranches dans l'ordre de l'idée 01 plutôt que patcher le DFS morceau par morceau.
- **Verrou 3 (arènes par tâche)** : oui, et aller jusqu'à la réutilisation entre trames et au pré-toucher (idée 05) ;
  la borne par tâche existe déjà (`suffix_memory_bound`).
- **Verrou 4 (census)** : une troisième route, exacte et sans flottant, par les listes K-certifiées (idée 06) ;
  mesurer d'abord la fraction servie. Le `SiteTree` v10 ne se porte pas tel quel (marges u18 figées).
- **Verrou 6 (T6 contre T0)** : la v10 a déjà la réponse pour le LiDAR : aucun effet (`ablation_kT.txt`) ; T ne
  compte que pour des synthétiques quantifiés sur grille.
- **Verrou 7 (GPU)** : sous-arbres, pas feuilles seules (c(16) = 52 % reste sur CPU, plafond ×1,9) ; `__int128`
  exact et à ×2,3 du coût i64 sur le device ; la feuille J3 compile déjà pour sm_120 sans débordement.

FIN
