# Contre-vérification adverse de la fouille « v10 : tour, tête, points, clustering, portes » (v10_tour_01 à 09)

4 octobre 2026, 13 h 14 UTC (`date -u`). Contre-vérificateur adverse de [`../fouille/v10_tour.md`](../fouille/v10_tour.md),
selon [`../CONTEXTE.md`](../CONTEXTE.md) et la carte [`../cartes/CARTE_V11.md`](../cartes/CARTE_V11.md). Rappel de
l'utilisateur : **le contrat est toujours 100 ms** (FULL K = 1..5, trames sans sol, G4 W48 ; si possible K = 10).

```text
phase=exploration_v11_hors_registre (contre-vérification, lecture seule)
backend=cpu_reference
profile=quantized_u21_input_only (v11 mesurée) ; quantized_u18_input_only (v10 mesurée)
public_status=not_claimed
GCP non utilisé ; aucune construction ni exécution native ; aucune commande git qui écrit
```

Légende : **M** mesuré (reçu nommé), **E** estimé (arithmétique sur mesures), **C** conjecturé.

## 0. Ce que j'ai relu moi-même

- Code v11 sur `origin/main` = `17514012b` (13 h 06 UTC ; dernier commit touchant `src/` : `3bd4d734e`), lu par
  `git show origin/main:` (le worktree a des modifications locales d'une autre session, non lues comme référence) :
  `src/tower/forest_pipeline.cpp`, `forest_plateau.cpp`, `descent.{hpp,cpp}`, `locate.cpp`, `population_lookup.cpp`,
  `full_domain.cpp`, `src/index/build.cpp`, `census_workspace.cpp`, `docs/CONCEPTION_MOTEUR.md`, `docs/FULL_FORESTS.md`,
  `reference/README.md`, `reference/ref_mutants.py`, `reference/hgp11_ref/families.py`, `plans/e1_prereg_*.json`,
  `docs/HIERARCHIE_POINTS.md`, `tests/tower/full_v10_diff.py`, `bench/verify_full_captures.py`.
- **Nouveau depuis la fouille** : réponse de l'auditeur aux sept verrous de Q100
  (`audits/AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md` § « Réponses R1–R7 », reçu
  `receipts/audit_selfreview_20261004/README.md`, commit `17514012b`). Elle tranche des points touchés par 02, 05, 06, 07.
- Reçus recalculés par petits scripts Python dans le scratchpad : AB7 (`claudeab7/results.tar.gz`, quinze prises W48
  et trois W1), PROF1 (`claudeprof1`, `report_w1_children/callers/self.stdout`, `report_w48_self.stdout`), v10 S1
  (`morsehgp3D_v10/receipts/g4_session1_20260929/results/cmd/008_c0_lidar02_k5_w1/stdout`), v10 S4
  (`g4_session4_j2c_20260929/results/cmd/008..013/stdout`), `conception/preuves_tour/noyau_v11.log`,
  `audit_v10/L06_CODE_TOUR.md` constat 04, `preuves_l06_code_tour/{isometrie,oracles}/`, v10 `receipts/test_cover_C_20260929/README.md`.

## 1. Chiffres de la fouille : ce qui tient, ce qui ne tient pas

| Affirmation de la fouille | Recalcul | Verdict |
| --- | --- | --- |
| Publication v11 ng02 W1, ordres 1–5 : 13,7/23,5/29,0/36,6/46,0 ms ; Kruskal v10 S1 5,3/9,6/14,3/20,0/28,4 | AB7 `orders[k].timings.plateaus_ns` et S1 `t_kruskal` : identiques ; aucune cellule étendue (`extended_cells` = 0), donc pur coût de plateau | **M, exact** |
| Queue W48 : 13 prises sur 15 entre 14,9 et 45,0 ms ; médianes 33,5/20,8/34,1 ; deux prises ng00 à 0,9 et 1,1 ms, forêts 137 ms | recalculé à l'identique | **M, exact** |
| Queue des verticales à W48 | **0,0 à 7,1 ms, nulle dans 11 prises sur 15** (`phases.verticals_ns`) | la partie « verticales » de 01 ne gagne rien à K = 5, W48 |
| Prototype `noyau_v11.log` : forêt et numérotation identiques, noyau ×2,49–2,90 | exact, **mais** même journal : `t_materialize_seq_ms` = 33,98 ms à l'ordre 5 (103,4 à l'ordre 10), donc noyau + matérialisation séquentielle = 52,2 ms **> 47,6 ms** du Kruskal par lots ; mesuré sous charge 19 sur 8 cœurs | gain du noyau **conditionné** à une matérialisation parallèle jamais prototypée |
| krbench L06 : noyau nu ÷2,7, noyau avec multifusions à la volée ÷1,6 (31,7 → 19,7 ms ordre 5), forêt identique | L06 l. 357–364 : exact | **M local**, seul gain séquentiel complet établi |
| Census 3,87 µs par appel (PROF1 10,32 % inclusif / 291 515) | 10,32 % de 10,93 s est exact, **mais inclusif du callback** (`LocatedQuery::consume` : `global_support`, `find_support`, pas de descente, `locate.cpp` l. 36–52). Parcours seul : `query` propre 1,72 % + `power_bound_signs` appelé par `query` 4,41 % + `side` appelé par `query` 2,10 % = **8,2 % = 0,90 s = 3,1 µs** ; ≈ 55 ns par borne de boîte (0,48 s / ≈ 8,7 M bornes, NV A6 « ~30 bornes ») | surévalué d'environ 20 % |
| v10 ≈ 1,06 µs par boule fermée | `site_tree.cpp` l. 183–227 : parcours **en binary64 avec `kMargin` = 0,02 non certifié**, clé exacte seulement dans la bande | comparaison flottant non certifié contre entier exact : l'écart ×3–4 n'est pas un écart de structure |
| Traces, pas, census ng00 W1 : 3 419 932 / 1 175 096 non-succès / 291 515 / 21,15 M ; 78,5 tests par census à l'ordre 5 | recalculé à l'identique | **M** |
| Machinerie du pas 0,44 s (`descend_each_step` propre 2,05 + `visit_located_part` 0,94 + …) | `descend_each_step` propre et `visit_located_part` font aussi hachage, sondes et MEB : la part attribuable aux ledgers est `add_descent` 0,67 + `add_all` 0,28 + `StepQuery::consume` 0,36 + une part de `cell_add` 0,47 ≈ **≤ 1,8 % = 0,17 s** (AB7 `perf_new_self.stdout`) | surévalué ×2,5 |
| Passes chaudes v10 S4, K = 5 : 259,8→252,0 ; 218,6→204,2 ; 265,5→253,6 ms | recalculé (`passes_catalogue_s` + `passes_tower_s`) : exact ; une prise par passe, décroissance monotone 3/3 | **M** (faible échantillon) |
| Ancres Merkle v10 : K = 5 sur 00/01/02, K = 10 sur 00 ; cardinaux v11 ng00 égaux | exact ; **les trois `sha256_entree` des ancres sont celles de `REUSE1_INPUTS` de `bench/verify_full_captures.py`** : mêmes entrées que les mesures v11 | **M** |
| Oracle de sauts : 7 nuages, 13 746 coupes, 0 écart, 546 sauts dont 252 aux ordres 6–10 | recalculé depuis les trois journaux : exact ; 208 à 1 844 s par nuage en Python | **M** |
| Lot C v10 : Δ tour − MR₂-bord = −0,001…+0,010 | `test_cover_C_20260929/README.md` l. 66–84 : exact | **M** |
| « Aucune occurrence de contraction des plateaux dans docs/ » (01) | faux : `docs/CONCEPTION_MOTEUR.md` l. 167 (« par événements binaires suivis d'une contraction exacte ») et l. 222 (levier 5 : « noyau de plateau contracté, historique d'attache ») | prévu par la conception v11, non implanté |

## 2. Verdicts

### v10_tour_01 — forêt sans lots : **à mesurer** (priorité haute)

- **Absent de la v11 : vrai.** `forest_plateau.cpp` (origin/main) : DSU par plateau, union à la plus petite racine
  sans union par taille (`unite_roots` l. 29–37), nœuds créés à la fermeture de chaque plateau (`close` l. 69–98) ;
  une tâche de publication par ordre (`forest_pipeline.cpp` l. 73–83). Le levier est prévu (`CONCEPTION_MOTEUR.md`
  l. 167, l. 222, priorité 5), jamais codé.
- **Preuve d'équivalence : solide** (forêt et numérotation identiques sur les vidages réels, ordres 1–5 et 10 ;
  2 444 942 images concordantes ; registre l. 240 toujours `proof_obligation`, à mettre à jour avant le port).
- **Gain annoncé : pas établi à W48.** (i) La queue n'est pas un plancher : deux prises ng00 sans queue à résolution
  égale ; elle varie de 0,9 à 39,6 ms sur la même trame à travail identique, signature d'un effet d'ordonnancement
  (publieur sur un frère SMT d'un résolveur, contention mémoire) autant que de débit. Le publieur d'ordre 5 coûte
  41,8 ms à W1 pour une résolution de 115 ms à W48 : à débit nominal il suivrait ; la cause n'est pas mesurée.
  (ii) Le prototype le plus rapide ne compte pas la matérialisation (34 ms séquentiels à l'ordre 5 dans le même
  journal, au-dessus du Kruskal par lots) ; seule la variante « multifusions à la volée » (÷1,6) est un gain
  séquentiel complet mesuré. (iii) Les verticales n'ont aucune queue à K = 5, W48.
- **Pourquoi il reste prioritaire (E, Amdahl).** Pour 100 ms, la résolution doit tomber vers 50 ms (Q100 § B) ; le
  publieur séquentiel d'ordre 5 (41,8 / 34,3 / 46,0 ms à W1, plus lent sous SMT) devient alors le chemin critique quel
  que soit l'ordonnancement. Le ramener à ≈ 20–29 ms (÷1,6, krbench transposé) est nécessaire, pas suffisant.
- **Banc qui tranche (G4, W48, trois trames, cinq prises)** : d'abord la mesure bon marché que la fouille propose —
  `CLOCK_THREAD_CPUTIME_ID` et temps d'attente des portes par tâche du pipeline (publieurs, suiveurs) à côté de
  `finished[t]` — pour séparer débit et attente ; puis A/B de la publication actuelle contre un noyau à multifusions
  à la volée, sorties octet pour octet, queue de publication et FULL publiés.
- **Gain révisé** : K = 5, W48 : de 0 à −30 ms selon la cause de la queue (médianes 21–34 ms) ; publieur d'ordre 5
  seul : 41,8 → ≈ 20–29 ms à W1 (E), ≈ 11–18 ms si la matérialisation parallèle tient (C). K = 10 : non mesuré en v11.

### v10_tour_02 — census aux k plus proches sur un k-d serré : **à mesurer**

- **Absent : vrai.** `census_workspace.cpp` l. 47–64 : parcours préfixe d'un arbre coupé au milieu des rangs Morton
  (`build.cpp` l. 55–72), boîtes réunies donc **serrées mais chevauchantes** (NV A6) ; saturation aux k premiers
  intérieurs (`descent.cpp` l. 139–142 : `interior().first(k)`). Le choix « k intérieurs quelconques » est une
  décision écrite de la v11 (`CONCEPTION_MOTEUR.md` l. 152–154) et la référence classe `jump_any` comme mutant
  **équivalent** (`reference/ref_mutants.py` l. 134–137).
- **Preuve affaiblie** : le coût du parcours est ≈ 3,1 µs et non 3,87 µs (callback inclus) ; il est dominé par
  ≈ 30 bornes de boîte exactes à ≈ 55 ns ; le « ≈ 1,06 µs » de la v10 est celui d'un parcours flottant non certifié
  (`kMargin`), non transposable tel quel ; le banc M5 n'a jamais été joué. Le contrat « k plus proches » exige
  d'explorer jusqu'à certifier le k-ième voisin : plus de travail par requête que l'arrêt aux k premiers trouvés,
  pour un bénéfice mesuré de −3 % de MEB et −7 % de sauts (mutant `JUMP_ANY`, local, v10). Il faut **découpler** la
  partition (k-d à coupes spatiales, boîtes disjointes) du contrat (k plus proches).
- **Auditeur, 13 h 06 (R4)** : « d'abord q2 couplé, puis ablation de partition … Aucun nouveau défaut d'index ni
  gain k-d présumé ». Compatible avec un banc, pas avec un port direct.
- **Banc qui tranche** : vider les ≈ 291 515 sphères de census de ng00 (et les ≈ 0,29 M de l'ordre 5 à K = 5) ;
  rejouer sur G4, W1 et W48 : index actuel ; index actuel + extrema q2 couplés ; k-d spatial (coupe médiane sur
  l'axe le plus étendu) avec les mêmes bornes exactes et le même contrat saturant ; puis la variante k plus proches.
  Compteurs (nœuds, bornes, sites) et ns par requête ; sorties I/U identiques au contrat.
- **Gain révisé** : parcours 0,90 s de CPU W1 sur ng00 ; même ÷3 → −0,6 s → au plus ≈ −20 ms à W48 ; plage
  plausible −5 à −20 ms (C). K = 10 : non mesuré (la projection −165 à −190 ms repose sur le coût flottant v10).

### v10_tour_03 — ancres v10 et juges d'échelle : **garder**

- **Absent à l'échelle : vrai** (NV A5 « reste ouvert … seul témoin indépendant à l'échelle » ; `full_v10_diff.py`
  ne couvre que quatorze petites fixtures ; aucune porte Euler ni EMST sous `tests/`).
- **Preuve vérifiée** : ancres présentes pour K = 5 (00, 01, 02) et K = 10 (00), **sur les mêmes entrées** que les
  mesures v11 (empreintes d'entrée égales à celles de `verify_full_captures.py`) ; cardinaux v11 égaux.
- **Précisions** : l'empreinte de Merkle hache les naissances par leur seul niveau (forme et niveaux, pas l'identité
  des sites) ; elle est plus faible que les octets canoniques communs de `full_v10_diff.py`. Faire les deux :
  étendre `full_v10_diff.py` aux trames entières sur G4 (v10 figée, ≈ 1 s par trame), garder les ancres comme témoin
  sans exécution. Ne comparer que forêts et verticales (le champ « attaches » des ancres suit l'entrée `core` de la
  v10). L'EMST exige scipy (Qhull) : juge **hors porte** G4 (Python 3.10 sans numpy) ; la Merkle (hashlib) peut être
  une porte. Euler reste complémentaire, jamais certificat. La v10 est une source différentielle, pas une autorité :
  un écart ouvre une fixture, il ne tranche pas.
- **Gain** : aucun temps ; premier juge indépendant de toute la tour K ≥ 2 sur trame du contrat, et de K = 10 ;
  protège 01, 02, 04, 05, 07.

### v10_tour_04 — semis par ordre, empreinte additive, étiquettes : **à mesurer** (priorité basse)

- **Partiel dans la v11 : vrai.** Table I∪U unique tous ordres, lignes K + 3 mots, avec étiquette de 32 bits
  (`population_lookup.cpp` l. 47, 175–176) ; hachage séquentiel de la partie (l. 18–24) ; `find_support` sans
  étiquette, relit `balls_data()[ball].support` à chaque sonde (`full_domain.cpp` l. 91–101).
- **Preuve** : statut conjecture ; les « ≈ 1,3 s de consultations » sont réelles (AB7) mais pas toutes évitables.
  (b) L'empreinte additive économise le hachage de k ≤ 5 mots : marginal. (c) La résolution ordre par ordre
  **contredit le pipeline actuel**, qui trie les blocs par première boule pour que les publications avancent
  ensemble (`forest_pipeline.cpp` l. 160–164) : à écarter sans 01. Restent (a) tables par ordre et (d) étiquette
  dans la table des supports (`find_support` 2,73 % propre à W1).
- **Banc qui tranche** : M3 de CT sur G4 (représentants vidés de ng00, W48) : table unique contre tables par ordre ;
  table des supports avec et sans étiquette ; garder l'existant sous 15 % d'écart.
- **Gain révisé** : −3 à −10 ms à W48 (C), sous le seuil d'intérêt de la carte (< 15 ms) s'il est seul.

### v10_tour_05 — arrêt à la première cellule de fenêtre (resolve1) : **à mesurer**

- **Absent : vrai** (`descent.cpp` l. 133–148 : `strict_trace` continue jusqu'à une naissance).
- **Doctrine : désormais cadrée.** L'auditeur (R5, 13 h 06) accepte le partage de cellule « comme certificat typé » :
  R ⊆ P_b complet, validité a ≥ λ_b fermée, a > λ_b ouverte, λ_b < λ pour le prédécesseur strict d'un plateau ;
  c'est exactement la forme du lemme T3 de CT (A.3). `CONCEPTION_MOTEUR.md` l. 163–164 (« porte sa date de
  validité ») est respecté si la cible est la cellule et non un terminal emprunté.
- **Preuve du gain : estimée, fragile.** « v10 à 7–8 % du minimum » vient d'un quart de trame en binary64
  (PR § 3.1, statistique) ; « v11 +6,5 % de pas hors table » est mesuré (S1/AB7). Gain ≈ 13 % des 1,175 M pas hors
  table de ng00 à ≈ 2,3 µs → ≈ 0,35 s W1 → ≈ −12 ms à W48 au mieux. Le suivi des pointeurs se fait dans la
  publication, c'est-à-dire sur le chemin séquentiel que 01 cherche à raccourcir, et change les graines réemployées
  par les verticales.
- **Banc qui tranche** : d'abord un compteur seul (G4 ou local, sans chrono) : nombre de pas évités si la descente
  s'arrêtait à la première cellule de fenêtre, par ordre, sur ng00/01/02 ; puis A/B G4 W48 avec suivi dans la
  publication, après les fixtures de 08, graines finales et forêts identiques.
- **Gain révisé** : −5 à −12 ms à W48 (E, borne haute), moins après 02.

### v10_tour_06 — ledger de lane sans copie : **écarter** (comme idée autonome)

- Absent : vrai, mais le gain est surévalué ×2,5 : les fonctions de ledger pèsent ≤ 1,8 % du CPU W1 (0,17 s) ;
  la moitié donne ≈ −3 ms à W48, au plus ≈ −6 ms si tout disparaissait. Sous le seuil d'intérêt. Le principe
  (compteurs locaux bornés, une addition vérifiée) est déjà accepté par l'auditeur pour le catalogue (R1) ; à
  replier dans ce port, en notant que R1 ne couvre ni le census global ni les filtres de nœuds : la borne a priori
  par lane doit être prouvée à part.

### v10_tour_07 — passes chaudes et arènes réutilisées : **à mesurer**

- **Absent : vrai** (`full_probe.cpp` sans passes répétées ; `Buffer` par `operator new/delete`). Recouvre en partie
  Q100 point 3 (arène par tâche), accepté par l'auditeur (R3) sous conditions de budget.
- **Preuve** : v10 S4 mesurée (−3,0 / −6,6 / −4,5 % à K = 5 ; une prise par passe) ; profil v11 W48 : verrou noyau
  1,55 %, fautes de page ≈ 3 % en cumul (PROF1 ; part propre de `do_user_addr_fault` 0,25 %). Transposition E.
- **Banc qui tranche** : option `--repeat=R` (Pool, domaine et arènes gardés) dans `full_probe`, G4 W48, trois
  trames, cinq prises, froid et chaud publiés côte à côte, mêmes sorties et même budget compté. **Décision de
  l'utilisateur requise** : le chiffre chaud ne vaut pour le contrat que si 100 ms est une latence par trame d'un
  flux à processus résident.
- **Gain révisé** : −3 à −7 % ≈ −10 à −29 ms (E, transposé de la v10).

### v10_tour_08 — fixtures de longues descentes : **garder**

- **Partiel dans la v11 : vrai** (`reference/README.md` l. 93–95 : régime « effleuré », 145 sauts ; `families.py`
  l. 248–251 : nuages larges à K ≤ 3). Aucune occurrence « halo » ni « knn_from_support » dans la référence.
- **Preuve vérifiée** (§ 1). Oracle borné n = 14, hors règle de non-exhaustivité. Coût réel : 208 à 1 844 s par
  nuage avec l'oracle de l'audit ; en graver deux ou trois en suite complète, pas en suite rapide, et mesurer la
  durée avec l'oracle v11.
- **Gain** : exactitude ; porte nécessaire avant 02 et 05 (qui changent le chemin des descentes), mutants de saut
  tués jusqu'à K = 10.

### v10_tour_09 — bras témoin MR_k-bord dans E1 : **garder** (avec une correction)

- **Absent des préenregistrements : vrai.** `plans/e1_prereg_lidar_20261004.json` (10 h 53 UTC) : lignes T_eom1/2/3,
  T_leaf, A_*, R0, R0L ; `e1_prereg_synthetique` : A_eom2 attribue « T − A = hiérarchie ». `HIERARCHIE_POINTS.md`
  l. 333 et l. 337 listent pourtant MR₂-bord pour E1 et E5. Aucune campagne E1 n'est publiée sur `origin/main` :
  l'ajout reste possible avant toute lecture.
- **Preuve vérifiée** (lot C v10, G4, préenregistré) : à même entrée et même tête, la hiérarchie de sklearn fait
  aussi bien que la tour à K ≤ 8. Sans ce bras, T − A confond entrée et hiérarchie ; l'auditeur rappelle aussi qu'un
  T − A non significatif n'établit pas une nullité.
- **Correction** : l'entrée « bord » de la v10 (min_y max(α² core2(y), |x − y|²)) n'est pas l'entrée de la v11
  (H^r_{k+1}, ancrage en rayon) ; publier MR_k avec l'entrée bord de la v10 **et**, si elle se définit sur l'arbre
  MR, avec l'entrée de la v11, en lignes descriptives déclarées, sans toucher les familles primaires.
- **Gain** : attribution honnête de E1 face à HDBSCAN ; aucun temps.

## 3. Bilan pour les 100 ms

Les gains de vitesse retenus « à mesurer » (01, 02, 04, 05, 07) font ensemble, au mieux, de l'ordre de −40 à
−100 ms sur FULL K = 5 à W48 (non additifs, E/C), soit la tour v11 ramenée vers le niveau de la v10 : la passe unique
du catalogue (167–180 ms) reste hors de ce périmètre. La contribution structurelle propre à cette source est 01 :
sans noyau, le publieur séquentiel d'ordre 5 (34–46 ms à W1) borne la tour par le bas dès que la résolution passe
sous ≈ 60 ms. Ordre conseillé : 08 et 03 (portes), mesure par tâche du pipeline puis A/B de 01, banc census (02),
compteur de 05, passes chaudes (07) ; 04 en dernier ; 06 replié dans le port des compteurs du catalogue.

FIN
