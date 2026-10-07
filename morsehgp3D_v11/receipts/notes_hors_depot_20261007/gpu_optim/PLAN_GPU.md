# Plan d'optimisation GPU (et CPU) de la tour FULL v11

Rédigé le 6 octobre 2026, entre 01 h 33 et 01 h 50 UTC (`date -u`). Lecture seule, au commit `df904711a`
(origin/main). Aucun commit, aucun build, **GCP non utilisé**.

```text
phase=exploration_v11_hors_registre
backend=cpu_reference (référence) ; cuda_g4 (voie de banc mesurée)
profile=quantized_u21_input_only
public_status=not_claimed
```

Sources : les six cartes de ce dossier (`carte_gpu_existant.md`, `carte_profil_etages.md`, `carte_forets.md`,
`carte_catalogue.md`, `carte_aval.md`, `carte_infra.md`) et les reçus qu'elles citent. Je ne reprends pas leur
détail. Je les arbitre et je les ordonne.

Chaque chiffre porte une étiquette :

- **M-G4** : mesure lue dans un reçu G4 ;
- **M-loc** : mesure sur le codespace ;
- **E** : estimation, jamais une mesure.

## 0. Verdict

1. **Le contrat ne peut porter que sur `domain + tree`, en régime résident à chaud.** L'écriture de `full` (300 Mo,
   2,16 s, M-G4) est physiquement hors de 100 ms sur un disque provisionné à 290 Mo/s. Le contexte CUDA coûte 78 ms
   (M-G4) : toute voie GPU exige donc un contexte déjà ouvert. **La définition du contrat est à trancher par
   l'utilisateur.** Le plan suppose : FULL K5 en mémoire (`domain + tree`), processus résident, médiane des
   passes 2..P.
2. **Aujourd'hui, `domain + tree` à W48 vaut 386 / 319 / 382 ms** (ng00/ng01/ng02, M-G4 FIN). Il faut donc un
   facteur de 3,2 à 3,9. Aucun levier isolé n'y suffit : `tree` dépasse 100 ms à lui seul sur les 18 prises
   chaudes, et le parcours du catalogue (84–106 ms) aussi.
3. **La voie GPU actuelle (feuilles seules, un fil par feuille) est perdante à K5** : +30 à +50 ms de mur
   (M-G4, sessions 3 à 6). Elle gagne 2 à 6 % à K10. C'est un plafond d'Amdahl : les feuilles ne valent que 45 à
   54 ms du domaine, et l'exécuteur est en série derrière le parcours.
4. **Les 100 ms sont atteignables seulement si quatre chantiers réussissent ensemble** (§ 2) :
   - coût par pas des descentes divisé par environ 3, sur CPU ;
   - publication des forêts sortie du chemin critique ;
   - domaine résident sur GPU (parcours G1 et feuilles coopératives J3), recouvert ;
   - petits étages parallélisés.

   Le budget estimé arrive à 85–120 ms. **Ce n'est pas une promesse** : ng02 est la trame la plus dure, et K10 à
   100 ms est hors de portée (domain 0,8 s et tree 1,6 s, M-G4).
5. **Le premier levier en gain sûr n'est pas sur GPU.** Il est dans l'ordonnancement du `tree` (queue de 2 à
   59 ms) et dans la mesure de ce qui n'est pas attribué. **Le premier levier GPU** est le recouvrement
   parcours/feuilles, accompagné des portes GPU manquantes : aucune porte CTest GPU, aucun mutant sur le code
   device, voie absente de l'API.

## 1. Référence mesurée (K5, W48, médianes chaudes, ms)

| Poste | ng00 | ng01 | ng02 | Source |
| --- | ---: | ---: | ---: | --- |
| cloud + index | 3,6 | 3,2 | 4,2 | M-G4 FIN |
| domain | 222 | 180 | 231 | M-G4 FIN |
| — passe unique (parcours + feuilles CPU) | 163 | 144 | 175 | M-G4 AB |
| —— dont parcours seul (mode GPU) | 84–106 | | | M-G4 GPU6 |
| —— dont feuilles CPU (différence) | 45–54 | | | dérivé M-G4 |
| — prefix / sort / level_scan / assembly / compact | 20 / 12 / 4,5 / 4,4 / 4,6 | 17 / 10 / 3,7 / 4,1 / 4,2 | 22 / 13 / 5,0 / 4,6 / 5,3 | M-G4 AB |
| — résidu non attribué | 23–28 | | | M-G4 GPU6 |
| tree | 164 | 139 | 151 | M-G4 FIN |
| — contextes + classification + naissances | 17–24 | | | M-G4 diag1 |
| — R (fin de la dernière résolution) | 116–120 | 84–87 | 95–98 | M-G4 diag1 |
| — queue publication + verticales | 2–59 (médianes de 5 à 52) | | | M-G4 diag1 |
| **domain + tree** | **386** | **319** | **382** | M-G4 FIN |
| write `full` (hors contrat) | 2 163 | 1 820 | 2 363 | M-G4 FIN |

Travail logique de ng00 (M-G4) :

- catalogue : 379 M tests G1, 353 456 feuilles, 1,31 M boules ;
- tree : 4,80 M pas de descente, 291 k census, 3,79 M présentations MEB, 596 à 704 ns par pas (188 ns en v10).

## 2. Chemin vers 100 ms, étage par étage

Cible : environ 45 ms de domain et environ 50 ms de tree, avec cloud et index à 4 ms. Colonne « aujourd'hui » =
M-G4. Toutes les colonnes « cible » sont des **E**.

| Poste | Aujourd'hui (M-G4) | Cible (E) | Levier (§ 3) | Ce qui fonde la cible |
| --- | ---: | ---: | --- | --- |
| parcours G1 | 84–106 | 8–20 | L2 (GPU) | environ 50 M couples nœud-site par niveaux, en i64 pur ; condition c(64) ≤ 20 % non mesurée sur l'arbre v11 (v10 M-loc : 15 %) |
| feuilles | 45–54 (CPU) | 0–10 visibles | L2+L5 (GPU J3, recouvert) | un fil par feuille = 3,2 fils actifs sur 32 (M-G4) ; J3 de ×3 à ×6 = E non étayée |
| préambule + prefix | 20 + 20 | 5–10 | L6 (CPU) ou L2 | scan parallèle ; prefix n'accélère que ×4,2 de W1 à W48 (M-G4) |
| sort + level_scan + assembly + compact | 25–27 | 10–15 | L6 (CPU) | tri LSD des clés F3 ; compact ×7,9 (M-G4) |
| résidu domain | 23–28 | 5–10 | L6 (mesurer d'abord) | non attribué : rien n'est sûr |
| **domain** | **180–231** | **30–60** | | |
| contextes / classification / naissances | 17–24 | 8–12 | L8 (CPU) | parallélisme entre cohortes |
| R | 84–120 | 30–40 | L1 (CPU) | coût par pas ramené vers celui de la v10 (188 ns, M-loc v10) |
| queue publication | 2–59 | 0–10 | L3 (CPU), puis L7 | P5 vaut 36–52 ms seul à W48 (M-G4 c40) : il doit passer sous R |
| **tree** | **139–171** | **40–60** | | |
| **domain + tree** | **319–386** | **≈ 75–125** | | |

Lecture :

- **100 ms est à la frontière de l'estimation**, pas en deçà. Les trois postes qui décident sont les suivants.
  - **R** (L1) : sans un coût par pas divisé par 2,5 à 3, `tree` reste à 105–140 ms même avec une queue nulle.
  - **Le parcours G1 sur GPU** (L2) : aucun levier CPU connu ne divise par 5 un poste déjà accéléré ×23 à ×24.
  - **La publication de l'ordre 5** (L3 puis L7) : une fois R sous 50 ms, la publication série (35 à 51 ms, M-G4)
    devient le plancher. Il faudra alors une forêt exacte parallèle (Kruskal sur rangs entiers, GPU ou CPU).
- **À froid** (processus neuf), ajouter le contexte CUDA : 78 ms sous nsys, 136 à 237 ms dans le fil (M-G4).
  Le contrat devient alors impossible avec le GPU, et le CPU seul ne ferme pas le parcours.
- **K10** : domain 1,74 s (CLI, feuilles de 16) ou 0,83 à 0,90 s (feuilles de 24) et tree 1,62 à 1,66 s (M-G4).
  Même avec un facteur de 4 partout, on reste vers 0,6 s (E). **K10 n'a pas de chemin vers 100 ms** ; on vise
  seulement des gains relatifs.

## 3. Leviers classés par gain attendu sur `domain + tree` K5 (319–386 ms M-G4)

Le classement suit le gain estimé rapporté au total mesuré. La colonne « sûreté » indique la confiance dans le
gain. Les leviers hors contrat (write, K10, sorties autres que FULL) sont classés à part au § 3.2.

### 3.1 Contrat FULL K5

| Rang | Levier | Type | Gain sur domain+tree (E) | % du total | Sûreté |
| ---: | --- | --- | --- | ---: | --- |
| 1 | L2 Domaine résident sur GPU : parcours G1 par niveaux, feuilles sur l'appareil | gpu | −100 à −150 ms | 30–40 % | faible (c(L) non mesuré, port neuf) |
| 2 | L1 Coût par pas des descentes (R) : attribution par route, V3 census par borne de réseau, V7 mémo de cellule certifié | cpu | −50 à −80 ms | 15–22 % | moyenne (v10 à 188 ns mesurée, cause de l'écart inconnue) |
| 3 | L3 Placement SMT des publieurs et suiveurs lourds (P5, P4, V5, V4), puis flux compact des cellules au publieur | cpu | −15 à −50 ms (médianes) | 5–13 % | bonne pour la queue |
| 4 | L6 Petits étages du domaine : résidu attribué, préambule et prefix parallèles, tri LSD | cpu | −25 à −50 ms | 8–13 % | moyenne |
| 5 | L4 Recouvrement parcours CPU / exécuteur GPU des feuilles, file partagée CPU/GPU, tampons épinglés | gpu | −10 à −20 ms par rapport au CPU (−45 à −55 par rapport au GPU actuel) | 3–5 % | bonne (identité déjà prouvée) |
| 6 | L5 Feuille coopérative par warp (J3), cache J2 réel, arène de débordement | gpu | −0 à −20 ms à K5 seule ; nécessaire à L2 | 0–5 % | faible |
| 7 | L7 Forêt exacte parallèle (Kruskal sur rangs, GPU ou CPU) et verticales par sauts binaires | gpu | 0 aujourd'hui ; −25 à −40 ms une fois R < 50 ms | 0–10 % | faible |
| 8 | L8 Contextes, classification, naissances du tree en parallèle | cpu | −5 à −10 ms | 2–3 % | moyenne |
| 9 | L9 Pool de 52 à 56 fils (seulement avec L3) | cpu | −3 à −9 ms | 1–2 % | faible |
| — | L0 Portes GPU et régime résident (contexte à l'ouverture de Session) | gpu | 0 à chaud ; −13 à −56 ms à froid | prérequis | — |

Fiches :

**L1 — R : coût par pas des descentes (CPU).**

- **Preuve.**
  - R = 84 à 120 ms, stable à ±3 ms d'une prise à l'autre (60 prises, M-G4 diag1).
  - À W1, la résolution fait 92 % du CPU de l'étage, à 596–704 ns par pas, contre 188 ns en v10 (audit des
    transpositions).
  - Les 39 résolveurs sont équilibrés (fins à 113,5 et 115,7 ms, M-G4).
- **Exactitude.** Mêmes listes I/U dans l'ordre de Morton, donc mêmes descentes. Le mémo est un certificat typé
  (R ⊆ P_b, λ_b < λ). Aucun flottant. Mêmes registres hors census.
- **Portes.** `pipeline_equivalence` à W1, W4 et W48, `tree_k_sha256`, dump MHGP11FUL1 gravé, TSan du pipeline,
  mutants « mémo non certifié » et « borne de réseau inversée ».
- **Mesure.**
  - Compteurs déterministes d'abord : part de chaque route par pas (table, MEB, find_support, census, trace
    stricte), nœuds et tests du census.
  - Puis W1 apparié (bruit A/A ±0,5 %), puis W48 sur R et le mur, en 7 prises par trame.
- **Effort.** Instrumentation 0,5 jour ; V3 1 à 2 jours ; V7 2 à 3 jours.
- **Dépendances.** Aucune. **Prérequis de L7.**

**L2 — Domaine résident sur GPU (parcours G1 et feuilles sur l'appareil).**

- **Preuve.**
  - Le parcours vaut 84–106 ms à K5 et 114–139 ms à K10 (M-G4 GPU6).
  - Le filtre G1, le réservoir et les enveloppes tiennent en i64 (`2B+5 ≤ 63`, lecture du code).
  - 379 M tests G1 à K5 (M-G4).
- **Exactitude.**
  - Ordre du réservoir reproduit par la clé (distance, position), compactage stable, listes en ordre SiteIdx.
  - Une feuille de plus de 32 sites, ou une limite `wide_leaf` ou `max_nodes`, rend `unresolved`, rejoué sur CPU
    ou refusé avant calcul.
  - Le registre est fait de sommes et de maxima.
  - **À trancher** : registre identique (compteurs logiques) ou seulement catalogue publié identique.
- **Portes.** P1–P10 de `carte_infra.md` § 4, portes Euler K+2 sur les trames en mode GPU, mutants GPU (feuille
  omise, témoin faux, égalité G1 retournée) tués par le jumeau hôte.
- **Mesure.**
  - **Étape 0 sur CPU, sans GPU** : compteur c(L) et chrono par taille de nœud sur l'arbre v11. Poursuivre
    seulement si c(64) ≤ 20 %.
  - Puis ncu du noyau, puis A/B selon le protocole de § 4.
- **Effort.** 6 à 8 jours-agent pour le parcours, plus L5.
- **Dépendances.** L0, L4 (transport par tranches), L5 (feuilles coopératives). Gain réel seulement en régime
  résident.

**L3 — Placement des publieurs (CPU).**

- **Preuve.**
  - La queue varie de 2,0 à 58,7 ms.
  - Le CPU de P5 vaut 81–151 ms en pipeline, contre 35–46 ms à W1 et 36–52 ms en étage séparé à W48 : l'inflation
    vient de la cohabitation avec les résolveurs (M-G4 diag1 et c40).
- **Exactitude.** Seul l'ordonnancement change ; aucune décision ne lit l'horloge.
- **Portes.** `pipeline_equivalence` existantes ; identité MHGP11FUL1 à chaque prise.
- **Mesure.** `lscpu -e` consigné, puis A/B/A en Williams avec bras A/A, au moins 7 prises par trame, W48 :
  base, affinité, résolveurs −2, `nice +5`. Publier départ, fin, CPU et attente de chaque tâche.
- **Effort.** 0,5 à 1 jour, plus 1 à 2 jours pour le flux compact (O2 forêts).
- **Dépendances.** Aucune.

**L4 — Recouvrement parcours/feuilles GPU.**

- **Preuve.**
  - L'exécuteur est en série après le `parallel_for` (`single_pass_batch.cpp:184`).
  - Il coûte 52–59 ms à chaud à K5 et 330–360 ms à K10, contre 84–106 et 114–139 ms de parcours (M-G4).
- **Exactitude.**
  - Disposition fixée par (tâche, tranche, ordinal), indépendante de l'ordre d'achèvement.
  - `unresolved` est rejoué par `leaf.cpp` avant admission.
  - Les exécuteurs CPU et GPU sont déjà prouvés identiques sur 372 prises à froid et 84 processus à chaud.
- **Portes.** P1–P3, P6–P8, et une porte « répartition forcée » (tout CPU, tout GPU, alternance) qui doit donner
  le même dump.
- **Mesure.** Mode `gpu_overlap` dans `gpu_ab.py`, chronologie nsys du chevauchement. Seuil : médiane des
  rapports de domain ≤ 0,85 sur les trois trames à K10.
- **Effort.** 3 à 5 jours.
- **Dépendances.** L0.

**L5 — Feuille J3, cache J2, arène.**

- **Preuve.**
  - 3,29 fils actifs sur 32, 168 registres, pile de 3,3 Kio, 40 % d'attente L1TEX.
  - Le device calcule 75,0 M droites contre 31,3 M sur CPU (×2,4 à K5, ×3,4 à K10).
  - `fill_kernel` tient dans un seul bloc de 32 fils (17,2 ms à K5, M-G4 ncu et nsys).
- **Exactitude.** Contrat D de l'auditeur (compteurs logiques). Vérifier qu'aucun aval ne dépend de l'ordre
  intra-feuille avant le tri canonique.
- **Portes.** P5 (q3 extrême, q4 aux seuils 2^20 et 2^20 + 1, préfixe obtus, coquille qmin = 2), P9 (ptxas).
- **Mesure.** Tranches dans cet ordre, chacune A/B contre la précédente : arène, puis cache J2, puis J3.
- **Effort.**
  - arène : environ 1 jour ;
  - cache J2 et feuille templatée : 1 à 2 jours ;
  - J3 : 1 à 2 semaines.
- **Dépendances.** Mesurer d'abord Q1 : distribution du coût par feuille sur le device (clock64).

**L6 — Petits étages du domaine (CPU).**

- **Preuve.** 94–141 ms hors passe unique et résolution régulière (M-G4 AB, dérivé) ; prefix ×4,2, births ×6,7,
  compact ×7,9 de W1 à W48 ; résidu non attribué de 23–28 ms.
- **Exactitude.** Sommes préfixes déterministes ; le tri LSD des clés F3 est complété par le tri exact des
  chaînes non certifiées (`level_key_order == 0`), d'où la même permutation.
- **Portes.** Dumps identiques ; porte de niveaux égaux ou quasi égaux aux bornes de la bande 1 − 2^-40.
- **Mesure.** `ab_g4.py` : W1 apparié, puis W48 en 7 prises sur les champs `prefix_ns`, `sort_ns` et les nouveaux
  chronos du résidu.
- **Effort.** Faible à moyen.

**L7 — Forêt exacte parallèle.**

- **Préalable doctrinal.** Inscrire au registre des preuves l'égalité « contraction des rangs égaux de l'arbre de
  Kruskal = plateau atomique », avec ses fixtures (0,2,4), diamant K2 et E5. Le traitement séquentiel des niveaux
  égaux est `false_in_general`.
- **Mesure.** Microbanc CUDA autonome sur les naissances et graines vidées sur G4. Entrée dans le moteur
  seulement si K5 ≤ 10 ms, K10 ≤ 50 ms et R < 50 ms.
- **Dépendances.** L1.

**L0 — Prérequis GPU** (aucun gain de temps à chaud, condition d'admission de L2, L4, L5 et L7).

- Aujourd'hui : 0 porte CTest GPU, 0 mutant sur environ 1 300 lignes device, repli jamais exercé en natif, voie
  absente de `src/api` (`kEngineMask` 16379 figé).
- À écrire : P1–P10.
- Contexte ouvert à l'ouverture de la Session, modules préchargés, mode persistance consigné.
- Mémoire physique du pool enfin publiée (`used_high` et `reserved_high` valent `None` aujourd'hui).

### 3.2 Hors contrat FULL K5 (gains réels du CLI, classés par ms gagnées)

| Rang | Levier | Type | Gain | Preuve | Effort |
| ---: | --- | --- | --- | --- | --- |
| A | SHA-256 par SHA-NI (fichier et `tree_k_sha256`), repli scalaire conservé | cpu | E : −1,4 s sur write `full` ng00 K5 (2,16 s), environ −7 s à K10 | M-loc : SHA = 78 % du write `full`, 1 527 contre 246–262 Mo/s (×6) ; M-G4 : EPYC 9B45 a `sha_ni` | ≈ 100 lignes + porte |
| B | Feuilles de 24 à K10 dans l'API (16 gardées à K5) | cpu | M-G4 sonde : domain K10 1,74 s → 0,83–0,90 s ; mur −24 à −27 % (A/B appariée diag1) | dumps identiques à 16/24/32 sur les trois trames (M-G4) ; registre changé : décision et regravure | une ligne + portes |
| C | L4 à K10 (même code qu'au § 3.1) | gpu | E : −250 à −350 ms de passe unique | M-G4 : exécuteur 330–360 ms en série | voir L4 |
| D | Vrai tampon d'écriture de 1 à 4 Mio, recouvrement hachage/sérialisation, mesure de fsync, `sync_file_range` | cpu | E : −0,1 à −1 s selon la part de fsync (non mesurée sur G4) | M-loc : 73 460 appels `write` de 4 Kio ; `setvbuf(NULL, 64 Kio)` n'est pas honoré | petit à moyen |
| E | S11 (pipeline à un ordre) pour `points` et `plat` | cpu | E : tree 138–184 → 65–90 ms | M-G4 QUAL : ×8,6 à ×10,9 seulement de W1 à W48 | 2 à 3 jours |
| F | `attach` par requêtes d'ancêtre en parallèle ; postordre parallèle de `output` | cpu | E : −10 à −20 ms et −10 à −20 ms | M-G4 : attach 58–80 ms série, output ×4,2 | moyen |
| G | Colonnes groupées dans l'écriture de `points` | cpu | E : −100 à −200 ms | M-loc : 58 % du write `points` en sérialisation | petit |

## 4. Protocole A/B G4 commun

Le protocole est celui de `carte_infra.md` § 3, que je retiens tel quel. Points essentiels :

- **Organisation.** Un seul binaire, les bras choisis par masque, plus un bras A/A dans la session. Ordre de
  Williams entre bras.
- **Prises.**
  - à froid : 7 prises (processus neufs) ;
  - à chaud : 8 passes, médiane des passes 2..P, avec empreinte par passe.
- **Couverture.**
  - trames ng00, ng01 et ng02, à K5 (feuilles de 16) et K10 (feuilles de 24), W48 ;
  - W1 sur une trame pour le travail en CPU·s ;
  - **ajouter uniform 8000, 16000 et 32000** pour la pente : aucune mesure du domaine n'existe à ces tailles.
- **Identité à chaque prise.** sha256 du dump égal à l'empreinte gravée (K5 ng00 `3a2bfb4f9f48`, ng01
  `5212a2ced81b`, ng02 `78feb765e21c` ; K10 ng00 `61a4245b91d9`), registre égal, `unresolved` publié.
- **Décision.** Gain annoncé seulement si les trois trames vont dans le même sens et que l'écart dépasse le bruit
  A/A de la session. Repère : 6 % mesurés entre S5 et S6.
- **Profilage.** Nsight et ncu dans des passes séparées, jamais chronométrées.
- **Session et reçu.**
  - commande 0 : construction CUDA et fumée sur 3 000 sites ;
  - sessions de 4200 s ;
  - reçu avec `check.py` ;
  - `TERMINATED` certifié sur la cible exacte.

## 5. Premières tranches (chacune livrable et mesurable seule)

**T1 — Attribution et placement du `tree` (CPU, aucune décision changée).**

- **Contenu.**
  - Compteurs et chronos sans effet sur les sorties : part par route et par pas des descentes, résidu du domaine
    (compact, level_scan, assembly, allocation, table S*), chrono interne du parcours, c(L) sur l'arbre v11.
  - `lscpu -e` consigné.
  - Placement SMT des publieurs et suiveurs lourds (L3).
- **Mesure G4.**
  - A/B/A Williams, base contre affinité contre `nice`, 7 prises par trame, W48, K5.
  - Critères : queue et médiane de l'étage tree ; identité MHGP11FUL1 et `tree_k_sha256`.
  - Les compteurs de la même session décident L1 (quelle route coûte) et L2 (seuil c(64) ≤ 20 %).
- **Gain attendu (E).** −15 à −50 ms sur les médianes de tree, surtout sur ng02. Surtout, **la décision chiffrée
  de L1 et de L2**.

**T2 — Portes GPU, régime résident et recouvrement parcours/feuilles (GPU).**

- **Contenu.**
  - L0 : P1–P10, mode GPU dans `full_leaf_lanes` (label `gpu`), mutants device tués par le jumeau hôte, repli
    forcé, compute-sanitizer, ptxas consigné, contexte ouvert à la Session, pics mémoire du pool.
  - Puis L4 : tranches de feuilles soumises pendant le parcours, tampons épinglés budgétés, file partagée
    CPU/GPU.
- **Mesure G4.** Bras 16379, 81915 et `gpu_overlap`, plus A/A, à K5 et K10, à froid et à chaud, avec la
  chronologie nsys du chevauchement.
- **Seuils.**
  - K10 : domain ≤ 0,85 × CPU ;
  - K5 : au moins la parité avec le CPU à chaud.
- **Gain attendu (E).** −250 à −350 ms de passe unique à K10 ; −10 à −20 ms à K5 par rapport au CPU.

**T3 — Écriture : SHA-NI et tampon réel (CPU, hors contrat, plus gros gain de mur du CLI).**

- **Contenu.**
  - SHA-NI avec repli scalaire et différentiel scalaire contre SHA-NI ;
  - tampon de 1 à 4 Mio admis au MemoryBudget ;
  - chrono de fsync ;
  - correction de `SORTIES.md` § 9 et de l'en-tête de `writer.cpp`.
- **Mesure G4.** `sorties_g4.py` étendu à `points` et `plat`. `file_sha256` et `manifest_sha256` égaux aux
  reçus FIN. Critère : write `full` au moins 30 % plus bas.
- **Gain attendu (E).** Environ −1,4 s sur 2,56 s de total CLI pour `full` ng00 K5.
- **Doctrine à trancher.** Détection par cpuid, ou option `MHGP11_MARCH`.

Ensuite, selon les chiffres de T1 : L1 (V3 puis V7), puis la tranche L5 arène et cache J2, puis L2 si c(64) ≤ 20 %,
puis L7 quand R < 50 ms.

## 6. À ne pas faire

Ce qui suit est mesuré perdant, ou borné par Amdahl :

- **Feuilles seules sur GPU comme route vers 100 ms.** Plafond de ×1,3 sur le domaine à K5 ; perte mesurée de
  +30 à +50 ms de mur.
- **Exécuteur de lot sur l'hôte (49147).** 92 à 116 ms contre environ 39 à 54 ms pour `leaf.cpp`.
- **Feuilles de 24 ou de 32 avec un fil par feuille.** 542 contre 477 ms (S4).
- **Tri des feuilles par taille.** Aucun effet : 3,29 à 3,43 fils actifs.
- **Blocs plus gros et retrait de la barrière.** 3 % de gain.
- **Format compact en rangs locaux.** Neutre, à ne pas redemander.
- **Agrandir les cases fixes.** Elles occupent déjà 724 Mo à 1,09 Go ; l'arène est la bonne forme.
- **Proposition flottante sur l'appareil suivie d'une recertification CPU.** Contraire à F1 ; la recertification
  a mesuré 4,8 à 5,4 s.
- **Census sur GPU, catalogue entier sur l'appareil.** Jamais un gain de bout en bout en v5, v6 et v7.
- **Mémoire unifiée.** Fautes de page, retour à 4 Go/s.
- **Graphes CUDA et fusion des lancements.** Quelques µs par lancement.
- **MPS.** Un seul flux de trames.
- **SHA-256 sur GPU ou hachage par morceaux.** Le flux est séquentiel ; un hachage par morceaux change le
  manifeste.
- **Compression, mmap, O_DIRECT, suppression de fsync.** Elles changent les formats ou le contrat de durabilité.
- **GPU pour cloud et index (3,6 ms), tri des niveaux (11,6 ms, ×22), classification (2–3 ms), attach,
  assemblage, pendaison et tête plate.** Les transferts et le contexte coûtent autant que le travail.
- **Union-find synchrone par plateau sur GPU.** 438 k plateaux à l'ordre 5, donc autant de barrières.
- **Borůvka CPU** (1,26 s en v9) **et forêt par diviser pour régner** (couture de 31 à 45 %). À ne rouvrir que
  par un microbanc qui les bat (L7).
- **Résolution des descentes sur GPU (O8 des forêts).** Avant L1, et sans seuil de 150 ns par pas transferts
  compris : même profil divergent que les feuilles.
- **ISA `-march=x86-64-v3/v4`.** Aucun gain mesuré.
- **Plus de fils que de fils matériels sans affinité.** Ralentit P5, qui est sur le chemin critique.

## 7. Décisions demandées à l'utilisateur

1. **Régime du contrat de 100 ms.**
   - Contenu : `domain + tree` seul, ou CLI complet ?
   - Mode : à chaud en processus résident, ou à froid ?

   Hors du premier choix (« `domain + tree`, résident »), aucune voie GPU n'est admissible.
2. **K10.** Cible relative seulement (pas de chemin vers 100 ms). Faut-il passer aux feuilles de 24 dans l'API
   (registre regravé) ?
3. **Voie GPU dans `src/api` et la CLI.** Seulement une fois qu'elle gagne et qu'elle est couverte par P1–P10.
4. **Contrat de registre des voies GPU parallèles** : compteurs logiques (contrat D) ou physiques.
5. **Registre des preuves.** Inscrire l'égalité de contraction des plateaux avant L7.
6. **Détection de SHA-NI** : cpuid ou option de construction.

## 8. Corrections de documentation relevées (sans effet sur le calcul)

- `docs/CATALOGUE.md` annonce des cases de 32 enregistrements et 256 incidences ; le code a
  `kScratchRecords = 128` et `kScratchPopulation = 1024` (2 Kio par feuille).
- `docs/SORTIES.md` § 9 et `src/io/writer.cpp` annoncent un tampon de 64 Kio. glibc écrit en fait par blocs de
  4 Kio (M-loc, strace).
