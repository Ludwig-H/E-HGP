# Contre-vérification adverse : source « v10 : générateur, catalogue, raccord R2, vitesse »

4 octobre 2026, vers 13 h 10 UTC (`date -u` lu à 13 h 08). Contre-vérificateur du rapport
[`fouille/v10_moteur.md`](../fouille/v10_moteur.md). Contexte : [`CONTEXTE.md`](../CONTEXTE.md) ; carte :
[`CARTE_V11.md`](../cartes/CARTE_V11.md). Contrat rappelé par l'utilisateur : **100 ms à K = 5** sur G4 (trames
sans sol de 30 000 à 60 000 sites, moteur entier exact), K = 10 si possible.

```text
phase=exploration_v11_hors_registre (contre-vérification, lecture seule)
backend=cpu_reference
profile=quantized_u21_input_only (v11) ; quantized_u18_input_only (v10 mesurée)
public_status=not_claimed
GCP non utilisé ; aucune construction ni exécution native ; aucune commande git qui écrit
```

Légende : **M** mesuré (reçu nommé), **E** estimé (calcul sur mesures), **C** conjecturé, **[lu]** lecture du code.

## 0. Méthode et deux corrections de contexte

Relu moi-même : la source J3 (`morsehgp3D_v10/receipts/audit_continu_20260929/performance_corrected/observed/feuille/src/morsehgp3D_v10/src/catalogue/leaf.hpp`),
ses mesures (`feuille-verif/mesures/ab_abba_t{1,4}.txt` et `.tsv`), `feuille/profil/callgrind/resume.txt`,
`feuille/verif/grand_livre_v3_vs_j2c.txt`, les quatre `feuille-verif/fuzz/*.tsv`, `feuille-verif/mut/m404.tsv`,
le contre-audit `morsehgp3D_v10/audits/audit_continu_20260929/performance/CONTRE_AUDIT_PROTO_CPU_20260929.md` § 1 ;
la frontière v3b (`frontiere/mesures/complet_c_resume.txt`, `modele_g4.txt`) ; `preuves_l01_math_catalogue/`
(Euler, restriction, mutants) ; `preuves_l05_code_catalogue/ablation_M.txt` ; la conception GPU v10 § 3 et le reçu S7 ;
`GEN_v2.md` § 3.4, § 9 ; côté v11, `origin/main` (`17514012b`) : `src/catalogue/leaf.cpp`, `boxes.cpp`,
`adaptive_prepare.cpp`, `catalogue.hpp`, `bench/points_export.cpp`, docs `CATALOGUE.md`, `CATALOGUE_OPTIMISATIONS.md`,
`MATHEMATIQUES.md` § 8 ; les archives G4 `claudeab7` et `claudeprof1` (extraites dans mon scratchpad, relues :
compteurs de `t_new_lidar_ng00_w1_r0.stdout`, `perf_new_self.stdout`, `report_w48_children.stdout`,
`report_w48_self.stdout`) ; les réponses de l'auditeur aux sept verrous
(`audits/AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md`, section « Réponses R1–R7 », poussée à 13 h 06 dans
`17514012b`) ; les pistes fermées v5/v6 et `docs/archive/abandoned/README.md`.

1. **Le worktree `build/v11-claude-20261003` n'est pas `origin/main`.** Son HEAD est `2b1abb6a5` et il porte des
   modifications **non commitées** du développeur (`leaf.cpp`, `sphere.cpp`, `predicates.cpp`, `meb.cpp`, nouveau
   `tests/num/q3_candidate_test.cpp`) : c'est le verrou 2 (q3 différé, `Q3Candidate`) en cours. Sur `origin/main`,
   `leaf.cpp` construit toujours la sphère q3 (`sphere_of`, l. 113–120) avant `center_in_box` (l. 227–233) [lu,
   `git show origin/main:…/leaf.cpp`]. Les constats « déjà dans la v11 » ci-dessous valent pour `origin/main` ; le q3
   différé est « en cours, non livré ».
2. **L'auditeur v11 a répondu aux sept verrous à 13 h 06** (R1–R7) : compteurs locaux acceptés avec borne m ≤ 1024
   et ledger identique ; q3 différé avec **tous les champs actuels de `CatalogueLedger` identiques** (`prefixes`,
   `census_tests`, `region_*` compris) ; arène par tâche acceptée sous réservation budgétée ; census : « d'abord q2
   couplé, puis ablation de partition » ; GPU autorisé sous route entière exacte. Ces contrats pèsent sur 01, 05, 06 et 07.

## 1. Verdicts

| id | Verdict | Gain révisé à W48, K = 5 (sauf mention) | Preuve vérifiée |
|---|---|---|---|
| v10_moteur_01 feuille J3 | **garder** | −25 à −55 ms de passe unique (E), dont une part déjà visée par les verrous 1–2 ; K = 10 : ×1,55 sur l'étage (M local v10) | oui |
| v10_moteur_02 Euler K+2 et restriction | **garder** (outil) | aucun gain de temps ; filet nécessaire avant 01, 03, 05 | oui |
| v10_moteur_03 filtre G1 et préambule | **a_mesurer** | −5 à −20 ms (C) ; le chemin critique du préambule v11 n'est pas mesuré | partielle (mesures v10 ; transfert non établi) |
| v10_moteur_04 table M(K) | **garder** (K = 10 seulement) | aucun à K = 5 ; catalogue K = 10 ×1,6–1,8 (E d'après v10) | oui |
| v10_moteur_05 arènes résidentes | **a_mesurer** | −5 à −15 ms dans la trame (C), surtout par le verrou 3 déjà accepté ; le reste exige une mesure chaude distincte du contrat froid | partielle (symptômes M, mécanisme C) |
| v10_moteur_06 LeafOracle | **a_mesurer** | 0 à −30 ms de résolution régulière (C) ; fraction servie inconnue | partielle (théorème oui, route jamais codée) |
| v10_moteur_07 GPU par sous-arbres | **a_mesurer** | −100 à −150 ms de passe unique (C), sans suffire seul à 100 ms | partielle (c(L) et S7 M en v10 ; aucun noyau catalogue exécuté) |

## 2. Idée par idée

### v10_moteur_01 — feuille J3 : **garder**

**(2) Preuve.** Tout ce qui est cité existe et dit ce qu'on lui fait dire, à trois nuances près.

- `ab_abba_t1.txt` : trame 02 entière, K = 5, un fil, 7,683 → 5,603 s, ×1,371, n = 6, appariés ×1,359–1,381 ; K = 10
  32,928 → 21,278 s, ×1,548 ; `ab_abba_t4.txt` : ×1,362 et ×1,559 (n = 4 et 2). **M local**, charge 31–47. Le
  dénominateur est le **CPU du processus pendant `t_boxes`** (contre-audit § 1), pas le catalogue ni FULL.
- La base de l'A/B est la feuille du **produit v10 `777406b82`** (« catalogue J2c », ancêtre de `a10605a06`, base du
  callgrind) [`git merge-base` exécuté]. Or la v11 descend de R2 = `777406b82` + gardes : la base est la bonne.
  **Nuance 1** : la table H des triplets vivants est **déjà dans la base** (`generator.cpp` v10 l. 319–465) ; le ×1,37
  ne lui doit rien. Pour la v11 (DFS + graphe de paires + lignes vivantes + cache J2), remplacer le DFS par les étages
  est un changement de structure dont le gain n'est mesuré nulle part.
- Callgrind (`resume.txt`) : feuille Ir ×1,46 / ×1,67, Bc ×2,12 / ×2,36, Bcm ×2,16 / ×2,37 : exact.
- Identité : `grand_livre_v3_vs_j2c.txt` = 10/10 couples identiques (13 compteurs). Fuzz : 1 070 lignes, **1 060
  `ok`, 10 `timeout`**, toutes `IDENTIQUE` [compté]. **Nuance 2** : le contre-audit ne retient que **940** cas
  entièrement conclusifs (g101/g202/g303, sans g505_clang) et signale que le juge ASan/UBSan accepterait deux délais
  symétriques ; « 1 060 » n'est vrai qu'en comptant g505. `m404.tsv` : 9 tués, `M3_haut_strict` survit et est
  **prouvé équivalent** (contre-audit § 1) : correct.
- **Nuance 3** : les bornes de J3 sont écrites pour u18 avec T = 6 (`leaf.hpp` l. 104–106 : |lo|, |hi| < 2^25 ;
  `side2` « bornes u18 … < 2^39 ») ; elles sont à refaire pour u21/u24 en T0 (le rapport le dit).

**(1) Déjà dans la v11 ?** Non pour l'essentiel, sur `origin/main` [lu] : `prepare` ne remplit qu'une direction de
dominance (`leaf.cpp` l. 37–53 : une seule matrice « qui me domine », la ligne de i reçoit j si j domine i ;
aucune matrice `domby`, donc aucun masque d'extérieurs certifiés) ;
`census_and_emit` teste chaque site par `num::side` avec `checked_add` (l. 154–173) ; `census_tests` = **37 007 938**
sur 08/000000 W1 [M, AB7] ; pas de triangle médian, pas d'enveloppe du tétraèdre ; droites J2 via cache
(`region_line_tests` 75 015 824, évaluations 31 280 290, succès 43 735 534 [M, AB7]). Partiellement : `Q4Candidate`
(poids stricts avant la boîte, esprit de B4) ; q3 différé **en cours non commité** (§ 0) ; compteurs locaux
**acceptés** (R1). La table H n'a pas été « écartée » pour un motif de fond : `CATALOGUE_OPTIMISATIONS.md` l. 69 dit
seulement « Ses masques de triplets dépendent aussi de G3 ; ils ne sont pas portés ».

**(3) Gain pour la v11.** Le rapport applique ×1,37–×1,65 à **toute** la passe unique. Le profil W1 b872 (M,
`perf_new_self.stdout`) donne pour la feuille `extend` 16,06 + `enumerate_leaf` 6,83 + `center_line_meets` 5,01 +
`Q4Candidate::through` 3,03 + `Sphere::through` 2,96 + `center_in_box` 1,77 + `strictly_acute` 1,04 + `checked_level`
0,61 ≈ 37 %, plus l'essentiel de `num::side` (7,10 %), contre `filter` 8,00 % : la feuille fait ≈ 80–85 % de la
passe, le filtre ≈ 15–20 % (E). J3 ne touche pas le filtre. Feuille ×1,37 → passe ×0,77 ; feuille ×1,65 (si le port
efface aussi les surcoûts propres de la v11 : `checked_add`, `Result<int>`, consultations du cache) → passe ×0,67.
Sur les prises médianes 180 / 167 / 170 ms [AB7] : **−38 à −60 ms** (E), et **−25 ms** en borne prudente (W48 SMT :
moins de mauvaises prédictions peut réduire le gain SMT ; aucune mesure J3 au-delà de 4 fils). Une partie de ce gain
(compteurs locaux, q3 différé) est déjà en route par les verrous 1–2 : le gain **incrémental** du port J3 au-delà est
plutôt −20 à −40 ms (E). Amdahl : même −55 ms laisse FULL ≈ 300–360 ms.

**(4) Doctrine.** Conforme : tout entier, mêmes présentations jugées, catalogue identique après tri canonique ; la
règle « émettre si S* est la présentation » reste valide [lu]. Deux conflits à régler avant le port : (a) R2 de
l'auditeur exige des champs de `CatalogueLedger` identiques (`prefixes`, `region_line_*`) — le remplacement du DFS
par les étages exige soit de recalculer ces comptes logiques, soit un nouveau contrat de ledger négocié ; (b) R1 : la
voie étroite i64 et un `side` total ne sont admis qu'avec un certificat couvrant coefficients et replis (« m ≤ 32 ne
le certifie pas »).

**(5) Piste fermée ?** Non. Mais `morsehgp3D_v5/docs/PISTES_FERMEES.md` l. 68–70 ferme « l'étage i64 du préfiltre q4
comme gain de temps » (médiane 1,0021) : la tranche (d) « droite i64 » ne se garde que sur A/B.

**Mesure à faire.** Par tranches dans l'ordre (a) census par masques, (b) M3 + E4, (d) droite i64 à termes de paire,
(e) étages : A/B G4 W1 et W48, trois trames, cinq prises appariées, dumps identiques, nouveau compteur « tests de
census évités », plus les fixtures F-DYA/F-TRI/F-L64 et les mutants m404 portés.

### v10_moteur_02 — portes d'Euler à K+2 et de restriction : **garder** (outillage)

**(2) Preuve.** Vérifiée : `euler_lidar00/01/02.txt` donnent 1 aux ordres 1 à 10 sur 8 314 472 / 6 492 748 /
8 025 829 boules à k_cat = 12 (529 / 341 / 1 559 étendues) ; `mutants_euler.txt` : deux mutants (une boule en moins,
un p déplacé) en ÉCART ; `jkm2_lidar.txt` : restriction égale aux six couples trame × K ∈ {5, 10} ; L05 l. 245 :
`m_frontier` survit à la porte du dépôt v10 (0/161), perd des boules avec `status ok` (208 290 à 1 fil, 200 901 à
4 fils au lieu de 210 424), tué par Euler et la restriction ; L01 cite L02 § 7.5 (295 forêts fausses sur 3 062
retraits). `euler_juge.py` n'importe que la bibliothèque standard (`fractions`, `math.comb`) : compatible avec les
portes Python nues de la G4.

**(1) Déjà dans la v11 ?** Non : `git grep -i euler` sur `origin/main` ne trouve que des documents
(`MATHEMATIQUES.md` § 8, `CONCEPTION_MOTEUR.md`, audits) ; la restriction n'est jugée que par
`tests/catalogue/fraction_oracle.py` l. 82–83 (≤ 14 sites, 84 restrictions).

**(3) Gain.** Aucun en temps. Valeur : c'est le seul juge global qui voit une perte silencieuse de boules à l'échelle
(feuilles > 14 sites, coupes de frontière, ordonnancement), exactement là où 01, 03 et 05 vont changer le code. Le
déterminisme W1/W48 de la v11 ne tue pas une perte **déterministe**.

**(4) Doctrine.** Conforme (invariant global, non exhaustif, pas de juge O(n^3)). Nécessaire, pas suffisant
(`MATHEMATIQUES.md` § 8 : deux omissions peuvent se compenser) ; ne jamais le présenter en certificat.

**Mesure à faire.** Exporter N(q, p) et les seules coquilles étendues depuis le banc catalogue ; porte G4 Cat_{K+2}
(K = 5 → Cat7) sur les trois trames et 8k/16k/32k ; restriction Cat7 → Cat5 flux contre flux ; mutant de perte de
boule à code 4.

### v10_moteur_03 — filtre G1 sans branchement et tête par tranches : **a_mesurer**

**(2) Preuve.** Les fichiers existent et disent bien : `complet_c_resume.txt` (local, 4 et 8 fils) `t_frontier`
×1,90–2,45 (K = 5) et ×1,42–2,03 (K = 10), **mais catalogue entier ×0,885–1,027 à K = 5 et ×0,78–1,10 à K = 10**
(v2 *plus lent* à K = 10, W = 4 : ×0,781). `modele_g4.txt` : v10 frontière 26,6 ms à P = 48 dont 17,8 ms de reste
(barrières, séries, déséquilibre), estimée 2,6–4,8 ms après v3b, **gain net estimé ×1,13–1,15 sur le catalogue
v10**. C'est un **modèle** calibré, pas une mesure G4 de v3b.

**(1) Déjà dans la v11 ?** Non [lu] : `boxes.cpp` l. 70–78 garde la boucle à sortie anticipée
`for (; i < selected && found < kmax; ++i)` ; `adaptive_prepare.cpp` : `capture` balaie seul chaque liste enfant
(l. 11–31), `select` trie par insertion jusqu'à 1 024 nœuds vivants à chaque ronde (l. 46–64), une ronde = une
barrière (`run_round`), racine sérielle (`prepare_root`).

**(3) Gain pour la v11.** La structure diffère : la v11 n'a pas la frontière en largeur v10 mais 27 rondes « lourds
d'abord » ≤ 1 024 tâches. Ordre de grandeur (E) : à 18,97 ns par site filtré (G4, v10, `modele_g4.txt`), la racine de
40 000 sites coûte ≈ 0,8 ms et la chaîne des listes lourdes quelques ms ; les 20,1 ms W48 du préambule [AB7]
viennent donc probablement autant du **pilote séquentiel** (`capture`, `select`, 27 barrières) que du filtrage des
grosses listes. Le découpage v3b n'attaque que le second. Noyau : L05 mesure ×1,08 sur tout l'étage v10 en local ;
la v11 a mesuré `-march` sans gain sur sa boucle actuelle [PROF1]. Gain révisé : **−5 à −20 ms** (C), dont la moitié
au plus pour le noyau.

**(4) Doctrine.** Mêmes listes ; exige 02 et TSan/stress pour tout ordonnancement plus fin. `filter_tests` doit rester
le compte logique actuel (nombre de tests jusqu'au K-ième dominateur), recalculable dans un noyau sans branchement.

**(5) Piste fermée ?** Non.

**Mesure à faire.** Banc G4 W48, trois trames : chronométrer le préambule **par ronde**, séparé pilote (capture,
select, budget) / Pool (filtre) / attente de barrière, avec la taille de la plus grosse liste par ronde ; puis A/B
séparés (noyau sans branchement W1 ; tranches v3b W48) à dumps identiques.

### v10_moteur_04 — table M(K), feuille 24 à K = 10 : **garder** (pour K = 10)

**(2) Preuve.** `ablation_M.txt` (trame 02, compteurs déterministes, catalogue identique pour tout M) : K = 10,
M = 16 → 11 063 197 nœuds, filtre 850 / boule, candidats 137,8, modèle 17 455 cycles, `t_boxes` 32,77 s ; M = 24 →
1 065 585, 235, 57,9, 10 796, 18,55 s. Exact. (Même fichier : M = 20 donne 17,12 s ; le minimum du modèle est
M = 24 ou 28 selon le jeu de coûts.) K = 5 : M = 16 optimal (9 783). `generator.cpp` v10 l. 641 : table 12/16/24/28 [lu].

**(1) Déjà dans la v11 ?** Non : `bench/points_export.cpp` l. 377 `params.leaf_size = 16` pour tout K ; tous les
pilotes Python du banc (`catalogue_*.py`, `full_campaign.py`, `full_memo.py`) fixent 16 ; `CatalogueParams{}` = 32
(`catalogue.hpp` l. 26). Indice concordant : les six K10 catalogue de [PAR5] et les cinq de [CAT3] ont expiré à 15 s
(CARTE_V11 § 4), ce qui est compatible avec l'explosion à M = 16.

**(3) Gain.** Nul pour le contrat K = 5. Catalogue K = 10 ×1,6 (modèle) à ×1,77 (temps local bruité) **en v10**
(E pour la v11 : arbre T0 et graphe de paires différents). Utile aussi aux campagnes de points K1..10 (16–109 s par
scène [PTS4]).

**(4) Doctrine.** Conforme : la taille de feuille ne change pas le catalogue ; à vérifier par dumps identiques.
Contrainte v11 : graphe de paires et cache J2 jusqu'à 32 sites, donc 24 et 28 restent sur la voie rapide.

**Mesure à faire.** Ablation de compteurs W1 (nœuds, `filter_tests`, préfixes, candidats par boule) pour feuille
16/20/24/28 à K = 10 sur la trame 02, puis une prise W48 ; fixer la table par K dans le banc.

### v10_moteur_05 — zéro allocation par nœud, arènes réutilisées : **a_mesurer**

**(2) Preuve.** Symptômes vérifiés [PROF1, W48, `report_w48_children` / `report_w48_self`] : `asm_exc_page_fault`
3,12 % en cumul, `do_anonymous_page` 2,47 %, `native_queued_spin_lock_slowpath` 1,55 %, `__pte_offset_map_lock`
1,51 %, `buffer_release` 0,51 % + `buffer_acquire` 0,35 %. Mécanisme v10 (vecteur local sans atomique) lu. Mais le
« 993 → 115 cycles par boule » est une mesure **privée disparue** (non revérifiable) et le spinlock de 1,55 % n'est
pas attribué aux tables de pages plutôt qu'au Pool (le Pool v11 attend par mutex/condition, CARTE_V11 § 2.2).

**(1) Déjà dans la v11 ?** La partie intra-trame est le **verrou 3**, accepté par l'auditeur (R3 : un Buffer privé par
tâche active, réservation majorante budgétée, portes plafond/−1, panne d'allocation, abandon, retour au budget) et en
prototype. La réutilisation entre trames et le pré-toucher n'y sont pas.

**(3) Gain.** Intra-trame (verrou 3) : la part fautes + atomiques ≈ 4 % du CPU W48 n'est pas entièrement évitable
(les pages d'arènes de sortie restent à toucher une fois dans un processus neuf) : **−5 à −15 ms** (C). Inter-trames :
ne gagne que dans une session résidente, donc une mesure **chaude** ; le contrat est aujourd'hui mesuré en processus
neuf (froid). La v10 elle-même ne montrait que 3–7 % entre passe 1 et passe 3 (S4).

**(4) Doctrine.** Conforme sous R3. Publier froid et chaud séparément, jamais l'un pour l'autre.

**Mesure à faire.** G4 W48, trois trames : (i) A/B du verrou 3 seul ; (ii) banc résident trois passes par processus,
`perf stat -e page-faults` et temps par passe, froid contre chaud.

### v10_moteur_06 — `LeafOracle` : **a_mesurer**

**(2) Preuve.** Théorème C : `GEN_v2.md` § 3.4 ; G2 dans la v11 (`docs/CATALOGUE.md` l. 42–45 : « Si le vrai nombre
d'intérieurs est inférieur à K, la liste contient toute la boule fermée »). `LeafOracle` (`GEN_v2.md` § 9 l. 401)
n'a **jamais été codé** (L05 l. 144 ; aucun symbole dans `morsehgp3D_v10/src`). Compteurs AB7 vérifiés : ordre 5,
217 326 appels, 17 055 923 tests de points (78,5 par appel) ; 291 515 appels en tout. Le coût de 3,6–3,9 µs par
appel est une estimation de NOTE3, pas une mesure.

**(1) Déjà dans la v11 ?** Le théorème oui, la route non : `census_workspace.cpp` parcourt l'index global.

**(3) Gain.** Calcul (E) : 291 515 appels × ≈ 3,35 µs gagnés ≈ 0,98 s de CPU W1 au plus ; au rapport W1/W48 de la
résolution (3 424 / 115,8 ms [AB7]), ≈ f × 33 ms. f est **inconnu** et défavorable à l'ordre 5 (k = K : toute
saturation retombe sur le repli). D'où **0 à −30 ms** (C). L'auditeur v11 (R4) préfère d'abord les extrema q2
couplés puis une ablation de partition ; il exige que tout certificat local couvre aussi les sites hors de la feuille
— ce que fait G2, mais l'affirmation « centre hors de toute boîte ajustée ⇒ au moins K intérieurs » n'est qu'esquissée
(elle tient pour un centre de MEB, dans l'enveloppe convexe de sa partie, si la domination porte sur la boîte avant
ajustement) : à écrire au registre avant tout port.

**(4) Doctrine.** Pas de catalogue global en C(n, k) : CSR ∝ Σm ≈ 12 M identifiants (≈ 49 Mo, estimation GEN2) ;
sorties identiques seulement avec repli dès que la liste compte au moins K intérieurs (témoins canoniques).

**Mesure à faire.** Compteurs W1 déterministes, par ordre, des genres de census (complet ; saturé avec p < K ;
p ≥ K) et, pour chaque appel, « la liste de la feuille du centre aurait-elle suffi ? » ; ne prototyper que si f ≥ 0,5.

### v10_moteur_07 — GPU par sous-arbres : **a_mesurer**

**(2) Preuve.** Conception GPU v10 l. 120–123 : c(L) trame 02 K = 5 = 52,3 % (L = 16), 23,8 % (32), 15,1 % (64),
7,9 % (256) ; quart 01 K = 10 : 11,8 % à L = 64 — **M local en v10**. S7 (README l. 17–23) : `__int128` exact sur
16 777 216 produits, 2 579 / 1 111 G op/s, 56,8 Go/s épinglé, 1,86 µs par lancement — **M G4**. Aucun noyau du
catalogue n'a tourné sur G4 ; la charge GPU de 3–10 ms est estimée.

**(1) Déjà dans la v11 ?** Non (`cpu_reference`) ; verrou 7, autorisé par R7 sous route entière exacte.

**(3) Gain.** c(L) a été mesuré sur l'arbre v10 (T6, frontière en largeur) ; l'arbre T0 adaptatif de la v11 n'a pas
été mesuré. Si c(64) ≈ 15 % se transpose : passe unique 167–195 → 25–35 ms CPU, plus transferts, retour et
canonicalisation (leçon C6 de la v6 : l'hôte avait mangé le gain) : **−100 à −150 ms** (C). Même alors, FULL garde
les forêts (133–172 ms) et les petits étages (98–116 ms, CARTE_V11 § 6) : nécessaire, pas suffisant pour 100 ms.

**(4) Doctrine.** R7 : centres i128 ≠ catalogue i128 (q3 garde checked/Wide, niveaux jusqu'à 180/134 bits en u21) ;
`unresolved` repris exactement sur CPU avant admission ; jamais un débordement converti en rejet ; budget host/pinned/
device commun ; nouveau backend `cuda_g4` subordonné.

**(5) Piste fermée ?** Non, sous la règle v5 (l. 45–47) : noyau mesuré sur G4 et porte d'égalité.

**Mesure à faire.** D'abord, sans GPU : c(L) sur l'arbre v11 (rdtsc par sous-arbre, W1, trois trames). Puis noyau
de sous-arbre (filtre + feuille) sur G4 contre le CPU, octets identiques, temps de bout en bout transferts compris.

## 3. Bilan vers 100 ms

Ce que cette source apporte à K = 5, au mieux et cumulé : 01 (−25 à −55) + 03 (−5 à −20) + 05 (−5 à −15) + 06
(0 à −30) ≈ **−35 à −120 ms** sur 352–412 ms de FULL W48 [AB7] (E/C), soit ≈ 230–375 ms. Aucune transposition CPU
de la v10 ne ferme 100 ms ; 07 est le seul levier dont le plafond touche la passe unique à la bonne échelle, et il
reste à mesurer. 02 n'apporte aucun temps mais conditionne la sûreté de 01, 03 et 05. 04 ne sert que K = 10.

FIN
