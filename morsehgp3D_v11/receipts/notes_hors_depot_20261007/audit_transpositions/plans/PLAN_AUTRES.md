# Plan des transpositions hors vitesse vers la v11

4 octobre 2026, rédigé de 13 h 25 à 13 h 40 UTC (heures lues par `date -u`). Planificateur « autres » de l'audit des
transpositions ([`../CONTEXTE.md`](../CONTEXTE.md)) : correction, mathématiques, points et clustering, architecture,
outillage et tests. Entrées : les douze fouilles (`../fouille/*.md`), les douze contre-vérifications
(`../verif/*.md`), la liste compacte de 29 idées gardées ou à mesurer, la carte
[`../cartes/CARTE_V11.md`](../cartes/CARTE_V11.md).
Rappel de l'utilisateur : **« l'objectif du contrat est toujours 100 ms »** (FULL K = 1..5, trames sans sol de
30 000 à 60 000 sites, G4 W48 ; K = 10 si possible).

```text
phase=exploration_v11_hors_registre (plan, lecture seule)
backend=cpu_reference
profile=quantized_u21_input_only
public_status=not_claimed
GCP non utilisé ; aucune construction ni exécution native ; aucune commande git qui écrit
```

Étiquettes : **M** mesuré (reçu nommé), **E** estimé (arithmétique sur mesures ; les efforts sont tous E), **C**
conjecturé, **L** établi par lecture du code. Chemins v11 relatifs à `morsehgp3D_v11/` sur `origin/main` =
`56216392e` (13 h 13 UTC), lus par `git show` depuis `build/v11-claude-20261003`. Abréviations de reçus : [AB7], [Q],
[PROF1], [PTS4], [SWEEP2], [MEMO1] comme dans la carte v11 (§ 0). « j » = jour-agent, « h » = heure-agent.

---

## 0. En bref

1. **Aucune de ces 29 idées ne gagne une milliseconde.** Trois liens les rattachent pourtant aux 100 ms :
   - **la cible est sous-dimensionnée** (auditeurs-01) : les trames de 50 000 à 60 000 sites portent ×1,30
     (médiane) à ×1,87 (maximum) le travail de 08/000000 (**M** pour les comptes, [PTS4]) ; la cible effective sur
     08/000000 tombe vers **54–77 ms**, soit ÷5,4 à ÷7,7 depuis 412 ms, et non ÷4,1 (**E**) ;
   - **les leviers restants (3–15 ms chacun) ne sont pas décidables** avec le protocole actuel : à code identique, le
     domaine diffère de +7 à +50 ms sur cinq paires sur cinq à W48 ([AB7], recalculé par `../verif/v6.md`) ;
   - **chaque levier de vitesse réécrit la feuille, le census, les descentes ou la publication**, et la v11 n'a
     **aucune porte réelle** aux labels `scale8000/16000/32000` et `lidar` : les seules qui les portent testent le
     harnais (`mhgp11_fixture_scale` et `mhgp11_fixture_lidar` exécutent `cmake -E true`,
     `tests/support/gate_fixture/CMakeLists.txt` l. 20–21, **L**). Une omission commune à tous les modes et à tous
     les W passerait aujourd'hui tous les contrôles.
2. **Fait nouveau, lu à 13 h 25 UTC** : le développeur porte en ce moment le **census par masques de la feuille J3**
   (diff non commis de `src/catalogue/leaf.cpp`, `catalogue.cpp`, `internal.hpp`, `docs/CATALOGUE.md` : « 43 % des
   tests de puissance du census du catalogue décidés par masque » sur ng00). C'est la tranche (a) de v10_moteur_01.
   Les portes d'échelle du catalogue (lot A) et les fixtures de non-hérédité (lot C) deviennent **urgentes** : elles
   doivent précéder les tranches (b)–(e) de J3 et la feuille 24 de K = 10.
3. **29 idées → 15 lots** (doublons fusionnés, § 2). Ordre d'intérêt (§ 3) : cible du contrat ; portes d'échelle du
   catalogue ; protocole apparié ; fixtures gravées ; protocole E1 ; différentiel v10/v11 sur trames entières ; juge
   exact de l'ordre 1 ; instrumentation réduite ; mesure c(L) préalable au GPU ; CI ; empreintes des points ; préfixe
   Kmax ; juge bilatéral ; préalables de la publication ; prototype GPU ; conditionnels.
4. **Port natif de la hiérarchie de points et de la sortie plate** (lot L et § 5) : hors du chrono FULL, piste
   parallèle. On reprend du produit et des auditeurs la **forme du contrat** (numérotation canonique par plateau, reçus,
   rejeu O(n), budgets refusés avant calcul, contrat Q8), jamais le code. Préalables : empreintes chaînées et test de
   permutation à l'échelle en Python (maintenant), bras z = 2 et règle d'IC de H_L2 (en cours), bras MR_k-bord
   déclaré avant toute lecture d'E1.
5. **GPU** : trois idées, un seul dossier (lot M). C'est la seule architecture du corpus dont le plafond touche la
   passe unique à l'échelle des 100 ms (−100 à −150 ms, **C**). Commencer par mesurer c(L) sur l'arbre T0 de la v11,
   sans GPU.
6. **À ne pas reprendre** (§ 7) : sept idées de la liste ou des contre-vérifications (v5-I3, v5-I4, v4-02, v6-I5,
   auditeurs-06, auditeurs-09 comme transposition, v10_tour_06) et une vingtaine de fausses bonnes idées des fouilles
   (routage par vote, arbre multi-ordres, code du réducteur, cascade flottante du produit, masses § 9.1, harnais
   fail-closed v6, Euler pris pour un certificat…).

---

## 1. Ce qui a bougé depuis les contre-vérifications

| Fait | Source | Effet sur ce plan |
| --- | --- | --- |
| `origin/main` = `56216392e` (13 h 13 UTC) : niveau q3 différé porté, dumps FULL identiques sur ng00 et ng02 | `git log origin/main` | auditeurs-06 sans objet |
| Census par masques J3 en cours, non commis (transposée `dominated[i]` remplie dans la boucle de dominance ; site présent dans les deux unions ⇒ refus `catalogue_invariant` ; ledger et dumps annoncés identiques) | `git diff` du worktree `build/v11-claude-20261003` (`docs/CATALOGUE.md` + 11 lignes, `leaf.cpp` + 31) | lot A et lot C1 avant les tranches suivantes de J3 |
| Correction auditeurs-08 en cours, non commise : `LINES` avec `('eom', 2)`, fixtures `F4_z2`, `F4b_z1/z2`, `bench/points_flat_claims.py` (Holm **et** borne IC > −0,02), porte `mhgp11_tower_points_flat_claims` (`fast`) | même worktree (`bench/points_flat_gate.py`, `tests/tower/tests.cmake`) | lot K : reste à commettre et à rejouer sur G4 |
| Nouveau `bench/ab_g4.py` non commis : N variantes, positions tournantes et inversées une prise sur deux, empreintes de dumps vérifiées, « médianes … **sans test statistique** » | en-tête du fichier | lot B se greffe dessus en quelques heures |
| Labels d'échelle déclarés (`cmake/gates.cmake` l. 40–52, RUN_SERIAL, saut propre si `MHGP11_DATA_DIR` manque) mais **vides** de toute porte qui juge le moteur : seules des portes du harnais les portent | `tests/support/gate_fixture/CMakeLists.txt` l. 20–24 | les lots A, D, E, F en sont les premiers locataires |
| La conception privée de la v11 avait déjà nommé ces portes : `mhgp11_catalogue_restriction`, `mhgp11_catalogue_euler`, `mhgp11_catalogue_boxes` (juge d'échantillon), `mhgp11_catalogue_diff_v10`, et côté tour Euler, restriction, juge de recensement, ordre 1 contre l'EMST | `build/v11-persist/conception/CONCEPTION_GENERATEUR.md` l. 507–511 ; `CONCEPTION_TOUR.md` § 1.4 | on reprend ces noms et leurs planchers |
| `docs/ARCHITECTURE.md` § 6 exige (b) des campagnes appariées contre la v10 figée **sur les trames du contrat** et (c) des invariants globaux à l'échelle | l. 148–153 | ni (b) ni (c) n'existent à l'échelle : lots A, D, E |
| Ancres v10, juges EMST, oracle des sauts, scripts de fixtures des fouilles : tous sous `build/`, ignoré par git (`.gitignore` l. 21) | `git check-ignore` | tout ce qui devient porte se **recopie dans le dépôt** avec sa provenance (commit v10, sha256) |

---

## 2. Fusion des doublons : 29 idées, 15 lots

| Lot | Idées de la liste | Ce qui reste après fusion |
| --- | --- | --- |
| **A** portes d'échelle du catalogue | v10_moteur_02, v9-01, part « retraits isolés » de v9-02, part Euler de v10_tour_03 | un juge Euler stdlib sur Cat_{K+2}, la restriction clé par clé, l'épingle du mode 0, leurs mutants de données |
| **B** protocole de mesure apparié | v6-I3, v4-01, v5-I1 (conditionnel) | statistiques par paire et bras A/A greffés sur `bench/ab_g4.py` ; intra-processus à mesurer |
| **C** fixtures gravées | v2_v3-01, v8-4, v10_tour_08 | cinq fixtures de non-hérédité, portails silencieux, α3, longues descentes, limites d'Euler |
| **D** juge exact de l'ordre 1 | ehgp_zoltan_03, part EMST de v10_tour_03 | EMST entier, plateaux N-aires comparés à la forêt K1 |
| **E** différentiel v10/v11 sur trames entières | v10_tour_03 | `full_v10_diff.py` étendu aux trames, ancres Merkle recopiées |
| **F** préfixe Kmax | v6-I4, v5-I2 | une porte d'octets K = 10 contre K = 5 |
| **G** décomposition et occupation | v4-04, v9-03, v8-3 (réduits) | une tranche d'instrumentation, W24 contre W48 |
| **H** cible du contrat | auditeurs-01 | une session de mesure sur les trames lourdes et d'autres séquences |
| **I** juge bilatéral du catalogue | v8-2 (+ juges q2/q3 de v9-02, conditionnels) | `mhgp11_catalogue_boxes` sous forme bilatérale |
| **J** CI | produit-ci-v11 | Clang et Python 3.10 nu |
| **K** sortie plate E1 | auditeurs-08, v10_tour_09 | z = 2 contre l'oracle, IC de H_L2, bras MR_k-bord |
| **L** points natifs | produit-empreintes-points (+ contrat Q8 ; auditeurs-09 n'est pas une transposition) | empreintes chaînées, permutation, puis port natif |
| **M** GPU | v10_moteur_07, auditeurs-07, produit-gpu-lecons (+ quatre lignes de v5-I3, leçon de couture de v6-I5) | c(L) sur T0, contrat, prototype de sous-arbre |
| **N** publication (architecture) | v6-I2 | relève du plan de vitesse ; ici seulement ses préalables hors vitesse |
| **O** conditionnels faibles | v4-03, ehgp_zoltan_01, ehgp_zoltan_02, reste de v9-02, v9-03/v8-3 complets, v5-I1 | à décider après A, D, E, G |

---

## 3. Classement par intérêt réel pour la v11

Intérêt réel = ce que la v11 perd sans l'idée : une décision fausse sur le contrat, une perte silencieuse non vue à
l'échelle, un levier non décidable, une revendication E1 fausse, ou un port natif non qualifiable.

| Rang | Lot | Verdict | Intérêt réel (une phrase) | Effort (E) | Où |
| ---: | --- | --- | --- | --- | --- |
| 1 | H cible | garder | change la décision : ÷5,4–7,7 au lieu de ÷4,1 ; aucune ligne de moteur | 2 h de préparation + une partie de session G4 | G4 |
| 2 | A portes d'échelle du catalogue | garder, **urgent** | seul juge global d'une perte silencieuse de boules à l'échelle, au moment où la feuille est réécrite | 1,5–2 j + G4 (Cat7 sur trois trames) | local (8k–32k) puis G4 |
| 3 | B protocole apparié | garder (statistiques) ; intra-processus à mesurer | rend décidables les leviers de 3–15 ms ; corrige un biais systématique à code identique | 2–4 h | local, puis G4 |
| 4 | C fixtures gravées | garder | tuent des mutants que J3, la feuille 24, le census et les descentes rendront possibles | 1,5 j | local ; longues descentes en `long` |
| 5 | K sortie plate E1 | garder | protège P08 (bras primaire z = 2 jamais testé contre l'oracle ; IC de H_L2) et l'attribution face à HDBSCAN | 1 h + rejeu G4 ; bras MR_k-bord 0,5 j | local + G4 |
| 6 | E différentiel v10/v11 | garder | exigé par ARCHITECTURE § 6 (b) ; premier juge de toute la tour K ≥ 2 et de K = 10 sur une trame | 0,5–1 j | G4 |
| 7 | D ordre 1 exact | garder | égalité exacte d'un ordre entier, théorème au registre ; voit la voie q2 et les plateaux à W48 | 0,5–1 j | local (8k–32k) puis G4 |
| 8 | G instrumentation réduite | garder | attribue 38–44 ms non décomposés, la cause de la queue de publication et le plafond SMT | 0,5–1 j | G4 |
| 9 | M0 c(L) sur T0 | à mesurer | décide si la seule route compatible avec 100 ms sur la passe unique existe pour l'arbre v11 | 0,5 j, sans GPU | local W1 |
| 10 | J CI v11 | garder | évite une session G4 perdue (`claudequal1`) ; fait respecter GCC et Clang | 80 lignes YAML | GitHub |
| 11 | L points : empreintes puis port | garder (empreintes) ; port = chantier v11 | rend qualifiable le port natif (Python ↔ natif, W1/W48, permutation) | 0,5–1 j, puis plusieurs jours | local, G4 |
| 12 | F préfixe Kmax | garder, priorité basse | seul invariant global bon marché reliant K = 10 au K = 5 qualifié | 0,5 j | local 8k, G4 |
| 13 | I juge bilatéral | garder (validité, puis complétude à plancher) | vérifie niveaux, I et U de chaque boule (Euler ne les voit pas) et la complétude sur échantillon | 2–3 j | G4 |
| 14 | N publication | à mesurer (plan de vitesse) | registre, porte d'équivalence et mutants avant toute réécriture de la publication | 0,5 j (registre) | local |
| 15 | M GPU, prototype | à mesurer, dernier recours | −100 à −150 ms de passe unique (**C**) si c(64) se transpose | 14–16 j | G4 |
| 16 | O conditionnels | à décider | juges de tranche et de segments, différentiel v4, juges q2/q3, intra-processus, instruments complets | 0,5–1 j chacun | G4 |

Deux pistes se lisent dans ce tableau : le **moteur FULL** (H, A, B, C, E, D, G, M0, J, F, I, N, M) et les **points**
(K, L). Elles ne se disputent pas les mêmes fichiers : la piste points ne touche ni `catalogue` ni `tower`.

---

## 4. Fiches

### Lot H — Cible du contrat (auditeurs-01) : rang 1

- **Fait** (**M**, [PTS4] `case_metadata.json.gz`, rejoué par `../verif/auditeurs.md`) : 127 trames c08 sans sol,
  32 462–98 560 sites ; nœuds d'ordre 5 ∝ n^1,386 ; plage [50 000, 60 000] : 5 trames, ×1,30 (médiane) et ×1,87
  (c08_003412, 58 418 sites, 1 076 856 nœuds d'ordre 5) rapportés à 08/000000 (576 371).
- **Réserves** (verif) : c08_003412 est atypique (18,4 nœuds d'ordre 5 par site contre 13,3–14,4 pour les quatre
  autres) ; 08/000000 est lourde pour sa taille (14,45 par site) ; le masque de PTS4 vient d'un instantané
  `e26b48055` antérieur au script épinglé ; « temps ∝ nœuds d'ordre 5 » est **E**.
- **À faire** : une partie de session G4, même binaire que la prochaine A/B : FULL K5 W48, cinq prises, sur
  c08_000502, c08_001518, c08_001881, c08_002992, c08_003412 et deux ou trois trames d'autres séquences, préparées par
  `bench/points_lidar_prepare.py` (masque Patchwork++ v8 épinglé, l. 12–15 ; recompter n) ; publier **le maximum**
  et la médiane par plage, avec les compteurs de travail (lot G). Ajouter ces trames au dossier `MHGP11_DATA_DIR` des
  portes `lidar`, pour que les lots A, D, E et F les jugent aussi.
- **Question à remonter à l'utilisateur, pas à trancher ici** : 85 des 127 trames c08 dépassent 60 000 sites sans
  sol ; la plage « 30 000–60 000 » du contrat exclut la majorité de cet échantillon (non aléatoire, choisi pour ses
  objets).
- **Porte** : aucune ; un reçu de mesure, processus neufs, froid.

### Lot A — Portes d'échelle du catalogue : rang 2, urgent

- **Pourquoi** (**M**) : en v10, le mutant `m_frontier` perdait 2 134 à 9 523 boules avec `status ok`, invisible à
  la porte du dépôt, tué seulement par Euler et la restriction (`build/v11-persist/audit_v10/L05_CODE_CATALOGUE.md`
  § 5.5) ; sur 3 062 retraits d'une boule admissible, la tour v10 publiait 295 forêts fausses sans refus, toutes vues
  par Euler (`build/v11-persist/audit_v10/L02_MATH_TOUR.md` § 7.5, cité par L01). En v9, sur 35 mutants du
  générateur, 9 étaient tués par l'invariant seul et vus par aucun autre contrôle, 3 de plus par K+2
  (`morsehgp3D_v9/audits/NOTE_C_INVARIANT_EULER_20260923.md`, M-loc). En v11, J1 n'est jugé que sur ≤ 14 sites (`tests/catalogue/fraction_oracle.py` l. 74–86), J3 n'est qu'écrit
  (`docs/MATHEMATIQUES.md` § 8), et l'égalité 2047/16379 ne voit pas un défaut latent des bits que les deux modes
  partagent (J2 cache, tri, frontière, arènes, verticales, census emprunté, lookup, réemploi). Honnêteté sur le
  précédent : `m_frontier` perdait un nombre de boules qui dépendait des fils (208 290 à 1 fil, 200 901 à 4, au lieu
  de 210 424, L05 l. 245), donc l'égalité W1/W48 de la v11 l'aurait vu ; ce que seuls Euler et la restriction voient,
  ce sont les pertes **déterministes** (élagages G1, G3, J2, masques du census, q3 différé) et les défauts latents.
- **Mécanisme** :
  1. **Juge d'Euler** `bench/catalogue_euler.py`, bibliothèque standard seule (`fractions`, `math.comb`), port du
     `euler_juge.py` de l'audit v10 (158 lignes, `build/v11-persist/audit_v10/preuves_l01_math_catalogue/`, à
     recopier) ; il lit le dump `MHGP11CAT1` (décodage par `bench/catalogue_semantic.py`, qui n'est qu'un décodeur de
     format) ; boule régulière (m = q_min) : contribution signée binomiale de MATHEMATIQUES § 8 (J3) ; coquille
     étendue : énumération des parties A ⊆ U (au plus 2^12 par boule) avec « centre dans conv(A) » décidé en
     `Fraction` par le juge ; égalité « n[k = 1] + Σ e_k = 1 » pour k = 1..K.
  2. **K+2** : catalogue construit à K+2 (K7 pour le contrat K5, K12 pour K10, admis puisque `kMaxMebSites` = 12) ;
     Euler à K seul est aveugle aux deux derniers ordres (zone aveugle 26,2–29,4 % du catalogue K5 en v9, M-loc).
  3. **Restriction clé par clé** : Cat_{K+2} filtré par p + q_min ≤ K + 1, rangs recalculés, égal octet pour octet
     (forme canonique de `catalogue_semantic`) à Cat_K.
  4. **Épingle du mode 0** : dump FULL de la voie de référence (`FullParams{}`, `CatalogueParams{}`) sur les trames,
     même source, égalité octet pour octet du mode 16379. Couvre niveaux et identités, qu'Euler ne voit pas. Coût
     **C** : quelques secondes à quelques dizaines de secondes par trame à W48 (modes 3 et 7 du vieux code : 14,5–19 s,
     [SWEEP2], [MEMO1]).
  5. **Non-vacuité** : (i) auto-test du juge : une boule retirée par strate du flux décodé (régulière p ≤ K − 3 ; q2 à
     p = K − 1 et q3 à p = K − 2 de la zone aveugle ; une coquille étendue) ⇒ code 1 d'Euler ou de la restriction ;
     (ii) mutant de code à code 4 (une émission sautée sous une condition rare) ; (iii) limites gravées (lot C4).
- **À vérifier avant d'écrire** : le banc `bench/catalogue_probe.cpp` doit construire le catalogue par la **même
  voie** que le mode 16379 (passe unique, frontière adaptative, graphe de paires, census par masques), sinon la porte
  juge une autre voie que celle qu'on chronomètre.
- **Portes** (noms de `CONCEPTION_GENERATEUR.md`) : `mhgp11_catalogue_euler`, `mhgp11_catalogue_restriction`
  (`scale8000`, `scale16000`, `scale32000` sur coupes emboîtées des trois trames régénérées par un script épinglé
  comme `morsehgp3D_v9/audits/c_omission_20260923/regen_inputs.py` ; `lidar` et `long` sur trames entières) ;
  `mhgp11_full_mode0_pin` (`lidar`, `long`). Codes 0 / 1 écart / 2 entrée refusée / 3 plancher / 4 mutant tué.
  Planchers : boules jugées = cardinal publié (ng00 K5 : 1 306 696, [Q]) ; au moins une coquille étendue ; chaque
  ordre 1..K vérifié. Python sous `python3 -S` et `-O` (porte G4 = Python 3.10 sans numpy).
- **Doctrine** : nécessaire, **jamais** un certificat de complétude (deux omissions peuvent se compenser) ; passe
  O(M), aucun catalogue en C(n, k) ; hors chrono. Inscrire l'identité J3 au registre racine
  (`docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md` ne la contient pas, `grep -i euler` vide).
- **Simplification rendue possible** (décision v11, pas une transposition) : une fois l'épingle du mode 0 en place,
  les bits toujours actifs de 16379 peuvent devenir deux préréglages nommés (référence, rapide) au lieu de
  16 384 combinaisons opt-in (déjà relevé par NV A1–A2).

### Lot B — Protocole de mesure apparié : rang 3

- **Pourquoi** (**M**, recalculé par les verifs v4, v5, v6) : à code identique (domaine), différences new − base à
  W48 sur ng00 : +16,3 / +7,0 / +33,4 / +49,7 / +39,9 ms, cinq sur cinq positives ; σ_log entre processus 0,111 /
  0,125 / 0,137 ⇒ ≈ 30–45 paires pour trancher 5 %, 8–12 pour 10 % ; étendue de la passe unique ≤ 0,2 % à W8,
  3,8–25,9 % à W48 ([Q]). Les leviers restants valent 3–15 ms.
- **Mécanisme** (sur le nouveau `bench/ab_g4.py`, aucune ligne de moteur) : bras **A/A** (la même archive sous deux
  noms) ; estimateur = médiane des rapports par paire adjacente ; **test des signes** exact ; nombre de paires fixé
  d'avance depuis σ mesuré ; règle de décision écrite : levier de travail CPU décidé à W1/W8 sur compteurs et temps,
  confirmé à W48 ; levier de passage à l'échelle (contention, atomiques, SMT) à W48 **et** W24 ; ablation destructive
  bornante avant toute réécriture (mutant sous `MHGP11_TESTING`) ; chiffre contractuel = processus neufs, froid, le
  chaud publié à côté seulement.
- **Porte** : auto-test stdlib du rapporteur (`fast`) : séries synthétiques à verdict connu (10 victoires sur 10 ⇒
  P = 1/1024 ; série nulle ⇒ non significatif) ; refus code 2 d'un nombre de paires impair ou sous le seuil.
- **Conditionnel (v5-I1)** : banc intra-processus seulement si un A/A G4 (2 × 10 prises alternées contre 10 processus
  neufs, ng00, W48) donne une étendue ≤ 5 % et ≥ 3 fois plus petite ; jamais pour un levier d'allocation (les grosses
  réservations repassent par `::operator new`, `src/core/buffer.cpp` l. 34) ; reconstruire l'index à chaque prise
  (consommé par `std::move`, `bench/full_probe.cpp` l. 210).
- **Ne pas importer** le harnais fail-closed de la v6 (directive « minimum de garde-fous »).

### Lot C — Fixtures gravées : rang 4

Toutes à coordonnées entières positives, dans u18/u21/u24 ; à recopier dans le dépôt avec leur provenance.

- **C1 — non-hérédité du rang et seuils K (v2_v3-01)**. Coordonnées exactes : `../fouille/v2_v3_fixtures_check.py`
  (sha256 `c81dd4dda3e51272…`, fonctions `r3v2`, `f64`, `q2x`, `f16`, `crux`, recalcul `Fraction` rejoué code 0) :
  - R3v2, 40 sites : q4 de centre (100, 100, 100), R² = 10 800, p = 0, m = 4, poids 1/4 ; quatre faces de rang
    fermé ≥ 12 ;
  - F64, 64 sites : q4 de centre (40, 40, 40), R² = 1 200, p = 0 ; six arêtes et quatre faces de rang fermé 12 ;
  - Q2X, 14 sites : a = (100,100,100), b = (200,100,100), x = (150,30,120), y = (150,30,80), témoins
    (150 + i, 140, 100) pour i = −4..5 ; centre (150, 80, 100), R² = 2 900, poids (5/14, 5/14, 1/7, 1/7) ;
  - F16, 19 sites : triangle (125,100,100), (93,124,100), (93,76,100) et apex (100, 100, 100 + h),
    h ∈ {−33..−26, 26..33} : Cat_K contient exactement 2(K − 2) de ces q4 pour K = 3..10 ;
  - CRUX, 4 sites : (5,8,9), (5,8,11), (9,12,5), (15,11,12), centre (10,10,10), R² = 30, poids (5,6,5,12)/28,
    seules deux faces aiguës.
  Q2X et CRUX dans le juge exhaustif (≤ 14 sites) ; R3v2, F64, F16 en faits analytiques gravés (centre, niveau, p,
  m, q_min, présence dans Cat_K selon K) dans `tests/catalogue/model_test.py` ; ajouter une variante K = 3 de F64 à
  ≤ 14 sites. **Mutants** (vraies mutations de `live_rows`, causales) : poids d'une paire remplacé par le rang de sa
  boule q2 ; préfixe q4 élagué par le rang de sa face ; face canonique aiguë exigée ; seuil θ décalé d'une unité.
  Une ligne « le rang n'est pas héréditaire (F64) » dans MATHEMATIQUES § 3.
- **C2 — portails silencieux et α3 (v8-4)**. A = (0,1,0), B = (2,5,0), C = (4,1,0), D = (1,0,0), E = (3,0,0),
  Kmax = 2 ; vérité recalculée par l'étage A de la référence v11 (`../verif/v8.md` § 4) : à l'ordre 2, naissances AD
  et CE à 1/2, DE à 1, AB et BC à 5 ; fusion {AD, CE, DE} à 5/2 ; fusion finale à 25/4 (ABC) ; AC (niveau 4) n'est
  pas une naissance. Dans `reference/hgp11_ref/families.py` (`FIXTURES`) et les listes nommées de
  `tests/tower/descent_oracle.py` et `forest_oracle.py`. Mutant : substituer MEB(AB) à la cible de AC (rang et date
  justes, parent faux). Contre-exemple α3, pour le **juge** du lot I et non pour le moteur : tétraèdre translaté de
  +(0, 12, 12), (0,12,12), (60,12,12), (20,54,12), (28,22,61), et z = (28,0,0) : 3H² > Ξ et pourtant z est extérieur
  (puissance +1468/343) ; 2H² < Ξ. Les fixtures d'égalité du census par boîtes de la v8 se réexpriment en
  présentation v11 (ancre, N, D, feuilles ≤ 8) **avec** le levier de census du plan de vitesse.
- **C3 — longues descentes (v10_tour_08)**. Deux ou trois nuages « noyau serré et halo » (n = 14, noyau de 7, 9
  ou 11 sites ; sauts aux ordres 6 à 10) et un ou deux des six nuages à 3–4 sauts de TV2 § 17.4, gravés aux
  coordonnées exactes depuis `build/v11-persist/audit_v10/preuves_l06_code_tour/oracles/oracle_sauts.py` et
  `build/v10-persist/design/tower_v2_spike/long_descent_any.py` ; oracle Γ_k borné (exclu de la règle de
  non-exhaustivité) ; suite `long` (208 à 1 844 s par nuage avec l'oracle de l'audit, **M**, à remesurer avec
  l'oracle v11). Mutants : `knn_from_support_site` (survivait à 15 des 16 fixtures v10, mourait sur les longues
  descentes) et sauts décalés jusqu'à K = 10.
- **C4 — limites d'Euler, fixtures négatives**. Cinq sites de MATHEMATIQUES § 8 ((0,5,0), (8,9,0), (8,1,0),
  (35,5,0), (45,5,0), k = 1 : triangle +1 et paire −1 au niveau 25) et nuage D/T de 13 sites
  (`morsehgp3D_v9/audits/CONTRE_EXEMPLE_EULER_KPLUS2_20260923.md`) : Euler et K+2 passent malgré l'omission ; une autre
  porte (lot D ou E, ou le refus de la tour) doit la voir. Elles interdisent de lire le lot A comme un certificat.
- **Quand** : C1 avant les tranches (b)–(e) de J3 et la feuille 24 de K = 10 ; C2 et C3 avant toute retouche du
  census des descentes, de la politique de saut ou d'un mémo de cellule.

### Lot K — Sortie plate E1 : rang 5 (piste points)

- **auditeurs-08** : sur `origin/main`, `bench/points_flat_gate.py` l. 38 n'a pas z = 2 alors que le synthétique le
  prend en primaire, et `bench/points_flat_summary.py` l. 237 revendique H_L2 sur Holm seul (**L**). La correction du
  développeur (§ 1) doit être commise telle quelle, puis la porte plate rejouée sur G4 contre l'oracle **avant**
  toute mesure primaire. Fixtures : `F4_z2`, `F4b_z1`, `F4b_z2` (seule où z = 2 se distingue de z = 1) ; règles de
  revendication à −0,021 et −0,02 (borne stricte).
- **v10_tour_09** : ajouter MR_k-bord en **lignes descriptives déclarées** avant toute lecture d'E1, sans toucher
  les familles primaires de `plans/e1_prereg_lidar_20261004.json` : (i) entrée « bord » de la v10
  (`morsehgp3D_v10/tests/head/mreach.hpp` l. 1–25) ; (ii) si elle se définit sur l'arbre MR, l'entrée de la v11
  (ancrage en rayon). Preuve du besoin (**M**, G4, préenregistré) : à même entrée et même tête, Δ tour − MR₂-bord =
  −0,001…+0,010 de K = 2 à 10 (`morsehgp3D_v10/receipts/test_cover_C_20260929/README.md` l. 66–84). Sans ce bras,
  « T − A = hiérarchie » confond l'entrée et la hiérarchie. `docs/HIERARCHIE_POINTS.md` § 8 (l. 332–337) le listait
  déjà pour E1 et E5.

### Lot E — Différentiel v10/v11 sur trames entières : rang 6

- **Pourquoi** : ARCHITECTURE § 6 (b) ; aujourd'hui `tests/tower/full_v10_diff.py` ne compare que 14 petites
  fixtures (`NAMES`) ; les cardinaux v11 de ng00 égalent déjà les ancres v10, mais seul un différentiel établit
  l'égalité des objets ; aucune K = 10 v11 n'a jamais été comparée.
- **Mécanisme** : étendre `full_v10_diff.py` (G4 seulement, archive v10 épinglée par `v10_frozen_manifest.json`,
  entrées u18 : les `sha256_entree` des ancres sont celles de `REUSE1_INPUTS` de `bench/verify_full_captures.py`)
  aux trois trames à K = 5 et à 08/000000 à K = 10 ; comparer les octets canoniques communs des forêts et des
  verticales (pas les attaches, qui suivent l'entrée `core` de la v10). Recopier dans le dépôt `anchors.jsonl`
  (v10 `afb081774`) et `tower_merkle.py` (hashlib seul) comme témoin sans exécution de la v10.
- **Porte** : `mhgp11_full_v10_lidar` (`diff_v10`, `lidar`, `long`) ; planchers : ordres 1..K comparés, nœuds par
  ordre égaux aux ancres (ng00 : 79 681 / 178 127 / 285 910 / 421 661 / 576 371 aux ordres 1 à 5).
- **Coût** : v10 ≈ 1 s par trame à K = 5 (verif v10_tour) ; v11 K = 10 non mesurée (**C** : 1,5–1,8 s à W48).
- **Doctrine** : la v10 est une source différentielle, pas une autorité ; un écart ouvre une fixture minimisée vers
  les étages A et B de `reference/`, il ne tranche pas. Elle partage la lignée de la v11 (R2) : elle ne remplace pas
  le lot D, indépendant.

### Lot D — Juge exact de l'ordre 1 (J2) : rang 7

- **Pourquoi** : théorèmes `proved_here` du registre racine (l. 48–49 : un EMST exact détermine les mêmes partitions
  strictes et fermées que le graphe complet, donc la même forêt compacte canonique) et J2 de MATHEMATIQUES § 8
  (« comparer la structure N-aire aux plateaux, pas seulement le multiensemble des longueurs »), jamais implanté. Il
  voit les boules q2 perdues par l'ordonnancement, les courses à W48 et les plateaux mal groupés sur de vrais ex
  æquo : à l'ordre 1 de ng00, 86 fusions ternaires et 1 quaternaire (ancres v10, **M**).
- **Mécanisme** : EMST exact en entiers (Borůvka sur grille uniforme ; d² entiers < 2^44 en u21), puis Kruskal à
  **unions simultanées** à longueur égale ; forme canonique (niveau d²/4 réduit, enfants vus comme ensembles de
  sites) comparée à la forêt K1 du dump FULL décodée par `bench/full_semantic.py`. Plus fort que le juge de l'audit
  v10 (`emst_judge.py` : multiensemble des niveaux, Delaunay Qhull flottant, numpy/scipy, hors porte G4).
- **Porte** : `mhgp11_tower_k1_emst` (`scale8000/16000/32000`, `lidar`), stdlib, `python3 -S` et `-O` ; planchers :
  somme sur les fusions de (enfants − 1) = n − 1, un seul arbre (ng00 : 39 796 fusions pour 39 885 sites) ;
  plateaux N-aires ≥ 1 (ng00 : 87) ; mutants de dump à code 4 : plateau binarisé, fusion
  q2 retirée, niveau décalé d'un rang. Si la durée Python dépasse ≈ 5 minutes par trame (estimée 1–3 min, non
  mesurée), port en C++ autonome hors produit, sans lien à `mhgp11`.
- **Outillage commun** (lecture du dump, sites, forme canonique) réutilisé par les juges conditionnels du lot O.

### Lot G — Décomposition et occupation, périmètre réduit : rang 8

- **Pourquoi** : 38–44 ms de FULL W48 non décomposés (préambule 18–21 ms mesuré en bloc ; résidu du domaine 12–14 ms
  et contextes 8–10 ms par différence) ; surcoût CPU W48/W1 ×1,50–1,53 non attribué ; queue de publication de 0,9 à
  45 ms à travail identique, dont la cause (famine des publieurs ou débit) n'est pas mesurable faute d'instant de
  **début** des tâches (`../verif/v7.md` § 1.3) ; plus longue tâche de la passe unique estimée alors que
  `single_task_max_ns` est calculé (`src/catalogue/single_pass.cpp` l. 160–162) et non publié
  (`bench/full_probe.cpp` l. 159).
- **Mécanisme** (une tranche, hors sorties canoniques, jamais comparée entre exécutions) : publier
  `single_task_sum/max_ns` ; sous-chronos du préambule (racine, rondes, sélection, `capture`, attente du Pool) ;
  chronos de la table des supports et des contextes ; instants début / prêt / fin de chaque tâche du pipeline ; delta
  `getrusage` (CPU utilisateur et système, fautes mineures) par étage ; ouvriers ayant exécuté au moins une tranche,
  compté dans la tâche ; ledger du census publié par ordre **et par arité** (`nodes`, `bounds`, `inside_blocks`,
  `outside_blocks`, `point_tests`) ; prises **W24** épinglées (un fil par cœur) à côté de W48. Garde : Σ tâches ≤
  min(W, J) × mur.
- **Second stade seulement** si W24 contre W48 laisse plus de 10 ms inexpliquées : CPU de fil par ouvrier
  (`CLOCK_THREAD_CPUTIME_ID` avec `cpu_clock_valid`), mode THP, attente par ouvrier. Le CPU de fil ne sépare **pas**
  SMT et contention (`../verif/v8.md` § 3) : il ne sert qu'au déséquilibre.

### Lot M — GPU, dossier unique : rang 9 (étape 0) et rang 15 (prototype)

- **Pourquoi l'ouvrir en mesure** : même cumulées, les transpositions CPU retirent au mieux −35 à −120 ms côté
  catalogue (`../verif/v10_moteur.md` § 3) et −40 à −100 ms côté tour (`../verif/v10_tour.md` § 3), non additifs
  (**E/C**) ; FULL resterait vers 200–330 ms, loin d'une cible effective de 54–77 ms. Le seul plafond compatible est
  le déchargement des **sous-arbres** du catalogue : c(L), part de l'étage des boîtes qui reste sur CPU, vaut 52,3 %
  à L = 16, 23,8 % à L = 32, 15,1 % à L = 64 et 7,9 % à L = 256 (**M local v10**, trame 02, K = 5).
- **Étape 0, sans GPU** : mesurer c(L) sur l'arbre T0 adaptatif de la **v11** (rdtsc par sous-arbre de liste parente
  ≤ L, W1, trois trames, L = 16, 32, 64, 256) ; l'arbre v10 n'est pas celui de la v11. Poursuivre si c(64) ≤ 20 %.
- **Contrat, à écrire avant tout code** : réponse R7 de l'auditeur (exact sur le device ou `unresolved` repris sur
  CPU avant admission ; un débordement n'est jamais un rejet ; q3 garde checked/Wide, puissance 134/152 bits, niveaux
  jusqu'à 180/134 ou 204/152 bits en u21/u24 ; budget host/pinned/device commun ; vrai nvcc ; portes CPU/device puis
  FULL identique ; chronos de préparation, transferts, retour, canonicalisation) ; compléments du produit (taux de
  replis compté et borné par une porte ; capacités comptées count–scan–emit, feuilles larges `width ≤ 1` jusqu'à
  `max_leaf` = 256 traitées à part ; contexte chaud déclaré hors chrono ; lanceurs factices hostiles) ; quatre lignes
  de la v5 (témoin device en phase 0 qui refuse la campagne ; `-fmad=false`, sans FTZ ; mutants device par masque
  borné ; élaguer avant l'envoi, un seul exécuteur propriétaire du device) ; leçon de la v6 (reconstruction hôte
  parallèle à offsets fixes : un noyau ×90 s'y était dissous en ×1,03–1,12 de bout en bout, **M** G4).
- **Prototype** : noyau de sous-arbre (filtre G1 + feuille) sur G4, octets identiques au CPU, temps de bout en bout
  transferts compris, contre la passe unique W48 ; abandon si le total device ne passe pas sous ≈ 40 ms. Préalables
  dans ce plan : lots A et E (identités à l'échelle), feuille J3 à source unique (portée par tranches). Effort de la
  conception v10 : 8 jours-agent pour les feuilles, 6–8 de plus pour les sous-arbres ; `backend=cuda_g4` déclaré.

### Lot J — CI GitHub de la v11 : rang 10

- Workflow sur `morsehgp3D_v11/**` : (b) Clang Release u21 et portes `unit` ; (c) portes Python rejouées sous
  Python 3.10 nu sans numpy (`-S`, `-O`) ; (a) GCC `ctest -L fast` si la durée tient sur un exécuteur à deux cœurs ;
  (d) ASan facultatif. Jamais de GCP (`tools/check_gcp_workflows.py`), jamais de données LiDAR, aucune qualification.
- Valeur : `claudequal1` perdue sur un `ModuleNotFoundError` (message de `eb036dbe2`, **M**) ; « Clang absent »
  de la qualification alors que la règle 11 de l'architecture exige GCC et Clang. N'aurait pas attrapé
  `claudepts5` (données requises).

### Lot L — Points : empreintes, puis port natif : rang 11 (piste points)

- **Étape 1, Python, maintenant** : empreintes SHA-256 chaînées FULL → H^r_{k+1} → sortie plate, sur des
  **enregistrements binaires** (le scellement décimal du produit coûtait 144,3 ms à 50 000 points, local, cause
  plausible non isolée) ; niveaux (t, m, q) en fractions réduites ; blocs numérotés canoniquement à l'intérieur de
  chaque plateau (tri par enfants puis sources, comme `morsehgp3d/src/cpu/api/point_hierarchy.cpp` l. 910–943 ;
  `tower_point_tree` numérote aujourd'hui dans l'ordre du balayage) ; labels canoniques déjà fournis
  (`bench/points_flat.py` l. 868–888) ; rejeu O(n) des invariants (un terminal par site, masse = Σ enfants +
  entrées, sélection = antichaîne, intervalles DFS contigus ; 2,8 ms à 50 000 points dans le produit, local) ; **test
  de permutation de l'entrée sur une trame entière** (labels canoniques et nombre de blocs par plateau égaux), seul
  élément gratuit qui manque à l'échelle (côté HGP, seule la fixture de cinq points sous 120 permutations existe,
  `bench/points_flat_gate.py` l. 252–265).
- **Étape 2, port natif** (modules `points` et `head` de `docs/ARCHITECTURE.md` § 2, absents de `src/`) : voir § 5.

### Lot F — Préfixe Kmax : rang 12

- Comparer octet pour octet les sections d'ordres 1..5 d'un dump FULL K = 10 à celles du dump K = 5, en sautant le
  seul mot `kmax` de l'en-tête (décodeur `bench/full_semantic.py` l. 119–186 : niveaux et centres rationnels exacts,
  aucun rang de catalogue) ; la référence porte strictement plus d'ordres ; plancher de nœuds à l'ordre 5. Mutant
  « fuite de Kmax » : lire `catalogue().kmax()` au lieu de l'ordre dans la garde de
  `src/tower/regular_vertical_seeds.hpp` l. 32. Porte `mhgp11_full_prefix_kmax` (`scale8000` en local, `lidar` sur
  G4).
- À brancher avec la **première prise K = 10 contractuelle** (prévue par `bench/full_campaign.py` l. 310, omise par
  budget) et avec le lot E à K = 10 : un seul dump K = 10 de 08/000000 sert les deux. En v10, la propriété tient sur
  cette trame : les ancres de 08/000000 portent, aux ordres 1 à 5, des empreintes de forêt, de verticales et
  d'attaches identiques à K = 5 et à K = 10 (recalculé ici sur `anchors.jsonl`, **M** v10).

### Lot I — Juge bilatéral d'échantillon du catalogue : rang 13

- **Registre d'abord** : lemme du citron (pour un support positif d'arité q, d'arête maximale ab de longueur D,
  ‖c − m‖² ≤ (q − 2)D²/(4q) ; couverture ‖2z − a − b‖² ≤ 4‖b − a‖², 3‖b − a‖² en q3), vérifié par `../verif/v8.md`,
  absent du registre racine.
- **Mécanisme** : implanter `mhgp11_catalogue_boxes` de la conception sous la forme bilatérale de la v8
  (`morsehgp3D_v8/audits/q34_stream_crosscheck_spatial_20260921/README_Q4_BILATERAL.md`) : validité **exhaustive** de
  chaque boule (positivité de S*, niveau exact, recensement exact dans la couverture de l'arête maximale, (p, I, U),
  admissibilité, règle canonique de S*, unicité) avec des prédicats propres au juge, jamais `num::side` ; complétude
  sur des arêtes maximales tirées **parmi les voisins proches** (b parmi les 4K plus proches de a, plus une fraction
  de paires lointaines) ; **plancher de non-vacuité** (par exemple 10^4 boules admissibles énumérées par trame) : la
  v8 n'en énumérait qu'une sur la trame entière à K5, sa direction complétude est à reconcevoir, pas à copier.
- **Coût** (**E**) : validité ≈ 10^8–10^9 prédicats entiers, de l'ordre de la minute en C++ par trame ; sonde C++
  hors produit ou Python selon la mesure.
- **Conditionnel** : juges q2 et q3, clés absentes, fenêtre autonome de la v9 (v9-02) seulement si la campagne de
  retraits isolés (lot A, non-vacuité, puis crochet de test qui retire une boule avant la tour) laisse passer des
  retraits qui changent le condensé sans refus de `src/tower/locate.cpp` (l. 30, 50).

### Lot N — Publication : préalables hors vitesse (v6-I2) : rang 14

- Le levier (noyau DSU minimal, matérialisation hors chaîne) appartient au plan de vitesse, avec v10_tour_01, v7-1 et
  auditeurs-02. Ce plan n'en garde que les préalables : (i) le registre racine porte encore « contraction des
  plateaux par composantes fortement connexes » en `proof_obligation` (l. 240) : y inscrire T4–T6 (CT) ou PO-T18
  (TV2) avant tout port ; (ii) porte d'équivalence contre la publication actuelle, gardée comme référence ; (iii)
  mutants `m_bin`, `m_seq`, `lien_plateau_perdu`, `mat_no_plateau_grouping`, `kernel_junction_on_head`,
  `mat_successor_max`, plus les trois fautes que la porte v10 laissait passer (multifusion binarisée, image verticale
  ouverte, attache ouverte, L06-03) ; (iv) mesurer d'abord la cause de la queue (lot G), puis l'ablation destructive de
  `close` sous `MHGP11_TESTING`, et réécrire seulement si la borne dépasse ≈ 15 ms.

### Lot O — Conditionnels faibles : rang 16

| Idée | Condition pour la reprendre | Coût |
| --- | --- | --- |
| ehgp_zoltan_01 juge de tranche (minorants exacts) | sur des dumps réels, part des nœuds contrôlés et détection des mutants injectés (fusion abaissée dans son intervalle, verticale échangée) non négligeables ; M-loc : 30,7 % et 37,3 % sur petits nuages | ≈ 1 j après le lot D |
| ehgp_zoltan_02 juge de segments (majorants exacts) | tue un mutant de perte de boules à l'échelle (il ne voit pas une naissance absente) | ≈ 1 j |
| v4-03 différentiel contre la v4 | D et E laissent un trou ; pilote n = 400 aligné d'abord ; u16, horizontal, coquilles < 32 | ≈ 1 j |
| juges q2/q3 et clés absentes de v9-02 | voir lot I | 1–2 j |
| v5-I1 intra-processus | voir lot B | 200 lignes + session courte |
| v9-03 / v8-3 complets | voir lot G | 0,5 j |

Reste **sans juge indépendant de lignée** pour k ≥ 2 à l'échelle, même après ce plan : les lots E (lignée v10) et O
(conditions nécessaires) ne remplacent pas la porte de certificats d'échantillon que la conception de la tour v11
prévoyait (`build/v11-persist/conception/CONCEPTION_TOUR.md` l. 552, `mhgp11_tower_certificates`), hors de la liste.

---

## 5. Port natif de la hiérarchie de points et de la sortie plate

- **Statut** : $H^{r}_{k+1}$ et la tête plate n'existent qu'en Python (`bench/points_radius.py`, `bench/points_flat.py`) ;
  les modules `points` et `head` prévus par l'architecture sont absents de `src/` (**L**). Coûts Python sur
  c08_000054 (39 873 sites, sous contention, [PTS4], **M**) : 3,1 s à k = 5 et 7,4 s à k = 10, contre 4,2 s pour
  `sklearn.cluster.HDBSCAN`. Hors du chrono FULL et du contrat de 100 ms ; nécessaire aux campagnes P08
  (141 trames × ordres) et à une comparaison de temps honnête contre sklearn. Gain natif ≥ ×10 : **C**.
- **À reprendre** (forme, pas code) :
  - du **produit** : numérotation canonique d'un lot de même niveau par tri (enfants, puis sources), reçu de réduction
    (chaque nœud avec niveau, enfants, sources ; chaque point vers son terminal), reçu de sélection (méthode, mcs,
    paramètres, nœuds retenus), rejeu O(n) des invariants, identifiant de payload invariant par permutation, budgets
    refusés avant calcul (`point_hierarchy.cpp` l. 270–390, 910–943, 1516–1580, 1637–1667, 1990–2042) ;
  - du **contrat Q8 des auditeurs v11** (`docs/HIERARCHIE_POINTS.md` § 8 l. 325–327 ;
    `receipts/points_answers_20261003/root/Q8_CONTRAT.md`) : type de date distinct des niveaux (`PointRadiusDate`, trois
    rangs, ordre commun avec FULL) ; budgets exacts par profil, 8192 bits avec refus, jamais une égalité déclarée ;
    comparateur √n1 + √n2 contre √n3 + √n4 par W = 4(x − y) − U², puis W² contre 16U²y, gardes de signe avant les
    carrés ; arbre de points ≤ 2n − 1 nœuds, sans matrice n² ni listes de membres par ancêtre ; DP score/décision par
    cluster puis un seul passage d'émission des labels ;
  - de l'**auditeur v10** : LCA d'une antichaîne par deux extrêmes d'un tour d'Euler, sans tri ni stockage
    (`morsehgp3D_v10/audits/AUDIT_ETAT_COURANT.md` l. 1135–1138) ;
  - de la **doctrine F** : bornes flottantes à u = 2^-52 en C++ sous tout mode d'arrondi (le 2^-53 de
    `points_flat.py` l. 42 n'est juste qu'en Python, toujours au plus proche).
- **Portes du port** : différentiel exact Python ↔ natif par les empreintes du lot L sur trames entières (k = 2..5 et
  10 ; z ∈ {1, 2, 3} ; mcs ∈ {k, 10, 20, √n} ; feuilles) ; identité W1/W48 ; permutation de l'entrée à 30–60 k sites ;
  oracle borné existant de `points_flat_gate.py` rejoué sur le natif avec ses neuf mutants (`binarise`,
  `masse_finale`, `seuil_moins_un`, `flottant_seul`, `egalite_enfants`, `racine_admise`, `sorties_brutes`,
  `niveau_carre`, `coupe_ouverte`) ; fixtures de budget (radicande carré parfait ; q = (14 000 000 − 6/997)², qui
  donnait un signe faux au filtre, `HIERARCHIE_POINTS.md` l. 119–124) ; mémoire linéaire comptée dans le budget de FULL.
- **Quand** : étape 1 du lot L maintenant ; le port quand la règle E1 est figée (lot K commis, P08 lancé), en piste
  parallèle au moteur FULL (ordre de l'utilisateur : tour, hiérarchie, puis z). Effort : plusieurs jours-agent (**E**).
- **À ne pas reprendre pour le port** : le code du réducteur produit (`cpp_int`, `std::map`, mono-fil : 2,79 s à
  50 000 points d'ordre 1, local ; plafonds par défaut sous des sorties v9 réelles) ; son routage par vote et son arbre
  multi-ordres ; la tête v10 (condensation fausse aux cohortes, niveaux flottants) ; le scellement en texte décimal.

---

## 6. Ordre de transposition

**Étape 0 — sans G4, à partir de maintenant** (≈ 5–6 j-agent en tout, parallélisables)

1. Lot K : commettre la correction auditeurs-08 et sa porte `fast` (1 h) ; déclarer les lignes MR_k-bord avant toute
   lecture d'E1.
2. Lot B : statistiques par paire, test des signes et bras A/A dans `bench/ab_g4.py`, **avant** la prochaine A/B G4
   (celle du census par masques et du q3 différé).
3. Lot A : juge d'Euler et restriction stdlib, auto-tests de non-vacuité, essais sur coupes 8k/16k/32k ; vérifier que
   `catalogue_probe` suit la voie 16379.
4. Lot C : C1, C2, C4 en portes rapides ; C3 en suite `long`.
5. Lot D : juge J2 stdlib et ses mutants de dump.
6. Registre racine : identité J3 (lot A), lemme du citron (avant le lot I), ligne 240 (avant le lot N).
7. Lot L étape 1 ; lot J en parallèle ; recopier dans le dépôt ancres, scripts et coordonnées sortis de `build/`.

**Étape 1 — une session G4 gardée « cible et juges »** (`gcp-migration/v11_session.py`), greffée si possible sur la
prochaine A/B du développeur ; les juges tournent **après** les prises chronométrées, dans des processus séparés :

- H : trames lourdes et autres séquences, K5 W48, cinq prises, avec l'instrumentation G et des prises W24 ;
- A : Cat7, Euler, restriction sur les trois trames (et les lourdes) ; épingle du mode 0 ;
- D : juge J2 sur les dumps K5 ; E : différentiel v10/v11 K5 sur trois trames, K10 sur 08/000000 ;
- F : préfixe K10 → K5 sur le même dump K10 ;
- K : rejeu de la porte plate avec z = 2 contre l'oracle.

**Étape 2 — portes à verdir avant chaque famille de leviers du plan de vitesse**

| Avant… | Portes vertes exigées |
| --- | --- |
| les tranches (b)–(e) de J3, la feuille 24 de K = 10, les compteurs locaux et les arènes de la feuille | A, C1 |
| toute retouche du census des descentes (bornes de réseau, arbre radix, k-d), de la politique de saut, de `resolve1` ou d'un mémo daté | C2, C3, E |
| toute réécriture de la publication ou des verticales (noyau sans lots, MSF, maximum d'ID) | N (registre, mutants, équivalence), G (cause de la queue), D, E |
| la première revendication K = 10 | F, A à K12, E à K10 |
| tout code GPU | M0, contrat M, A et E comme témoins de bout en bout |
| toute lecture d'E1 | K |
| le port natif des points | L étape 1 |

**Étape 3** — lot M0 dès que la bibliothèque d'instrumentation G existe ; décision GPU sur c(64) et sur le bilan de
la session H. **Étape 4** — lot I (validité, puis complétude à plancher) ; lot O selon les trous. **Étape 5** — port
natif des points (§ 5) quand E1 est figé.

---

## 7. Idées à ne pas reprendre

### 7.1 De la liste et des contre-vérifications

| Idée | Raison | Ce qui en survit |
| --- | --- | --- |
| v5-I3 cahier des charges device de la v5 | absorbé par R7 (`17514012b`) ; mesures v5 sur une boucle régulière, jamais un gain de bout en bout | quatre lignes dans le contrat du lot M |
| v5-I4 pic réservé par étage, majorant unique des arènes | absorbé par R3 ; la forme D = 4n(3B + 2) serait un majorant faux (elle borne les listes simultanées du suffixe, pas les nœuds ni les émissions) | une dette d'une ligne : pics par étage dans `FullTimings`, `ru_maxrss` en fin de processus (ARCHITECTURE § 7.1) |
| v4-02 porte à deux autorités | R3 l'impose déjà ; le budget v11 compte des réservations réelles | jouer le mutant « majorant amputé d'un terme » sur trames entières (profondeur 36) dans l'implantation de R3 |
| v6-I5 modèle de coût GPU v6 | aucun étage v11 équivalent | leçon de couture hôte (lot M) |
| auditeurs-06 niveau q3 différé | déjà porté (`56216392e`) | l'A/B G4 annoncé par le développeur |
| auditeurs-09 comme transposition | rien d'une version antérieure qui ne soit dans le plan v11 | le port reste un chantier v11 (§ 5) |
| v10_tour_06 ledger de descente sans copie | ≤ 1,8 % du CPU W1 | replié dans le port des compteurs locaux |
| v8-3 / v9-03 tels que proposés | le CPU de fil ne tranche pas entre SMT et contention ; l'équivalent v11 du correctif R9 (LPT, lourds d'abord) existe déjà | version réduite (lot G) |

### 7.2 Fausses bonnes idées des fouilles, hors vitesse

| Idée (source) | Pourquoi |
| --- | --- |
| Routage descendant par vote (produit), routage médian (note dendrogramme v3), ER0h, vote § 9.1 | rejetés par la v11 (`docs/HIERARCHIE_POINTS.md` § 5–6) : discontinus, aucune constante de stabilité uniforme ; masses par k-facette ∝ C(n, k) interdites |
| Arbre multi-ordres λ = k/r^z (produit), têtes multi-K de la v10 | jamais mesuré, ou mesurées négatives en v10 (EOM intégrée 0,722–0,777 contre 0,795 ; tranche γ −0,016 à −0,037 ; multi-K +0,0046 hors échantillon) ; recherche seulement (piste P6) |
| Code du réducteur produit ou de la tête v10 comme base du port | forme du contrat seulement (§ 5) |
| Cascade flottante certifiée du produit | suppose l'arrondi au plus proche et un MXCSR sans FTZ/DAZ ; contraire à F1–F6 « sous tout mode » |
| Rendu § 9.1, masses fractionnaires (v4, v5, Zoltan), fixture des poids « non Gabriel » (v9) | la v11 a tranché pour des masses entières (les triangles de T0 pèsent 8/3 < 3) |
| Condensation à seuil relatif α (Zoltan) | non mesurée ; au plus un bras E1-bis |
| Point-MST du produit (seul chiffre sous 100 ms) | autre objet, piste fermée (`docs/archive/abandoned/README.md`) |
| Catalogue « sans énumération » par descentes MEB-Lloyd (E-HGP) | complétude non acquise, faux positif cosphérique B1 |
| Juge par grille (E-HGP), second juge de Γ_k, tour à témoins | unilatéral et faux possible (F-AUD-9) ; doublon de l'étage A ; majorant seulement |
| DTM ou Delaunay pondéré vers D_k | piste fermée par E-HGP (entrelacement multiplicatif, jamais égalité) |
| Euler seul, ou catalogue scellé sans juge | aveugle aux deux derniers ordres ; jamais un certificat (lot C4) |
| Harnais fail-closed v6 (23 scènes, manifestes de manifestes) | directive « minimum de garde-fous » |
| `VmHWM` par étage (v6), pic projeté (v4) | `MemoryBudget` mesure mieux ; le pic projeté v4 a menti deux fois |
| Schémas JSON v1/v2 du produit pour les reçus v11 | chaque reçu a déjà son `check.py` |
| Contre-fixtures `linked_arcs_u16` (v6), cosphère 24 et cocyclique 384 (v3) | hors régime LiDAR ; la v11 grave `coquille14` |
| Protocole spatial trame/moitiés/quarts (v8) | les pentes se mesurent à 8k/16k/32k |
| Budget d'ingénierie par étage du produit (25/45/20/10 ms) | la carte v11 § 6 le fait avec des mesures |
| Comparer des constantes entre deux processus | piste fermée de méthode (v4, v5) : raison d'être du lot B |
| Verdict « 100 ms impossible » sans borne inférieure ; projections en CPU·s/48 | leçons de méthode des auditeurs, pas des idées |
| Contrat sur un seul ordre (`only_order`) | change l'objet (FULL K1..5) |
| Sous-maille T6 (verrou 6) | neutre sur LiDAR (même dump, nœuds +0,2 %, `ablation_kT.txt`) |
| CI : GCC seul, ASan obligatoire | doublon de la qualification G4 ; ASan facultatif |

---

## 8. Limites

- Aucun chiffre nouveau : les gains sont **E** ou **C**, les efforts sont estimés ; les durées des juges Python à
  l'échelle (lots A, D) ne sont pas mesurées.
- Le diff du développeur lu à 13 h 25 UTC n'est pas commis : il peut changer.
- La cible corrigée du lot H suppose un temps proportionnel aux nœuds d'ordre 5 (**E**) ; seule la session H la
  mesure.
- Ce plan ne classe pas les leviers de vitesse ; il fixe les portes qui doivent les précéder.

FIN
