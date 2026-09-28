# Audit critique de la v9

```text
phase=exploration_v10_hors_registre
backend=cpu_reference
public_status=not_claimed
```

- **Base jugée** : worktree `build/v9-open-worktree`, commit `ce8a649dd` (origin/main au début de l'audit), plus le diff non commis connu (`experiments/tower_clustering_20260928/{cluster.py,run_tower.py}`, `experiments/synthetic_bench_20260928/README.md`). Aucun fichier de `morsehgp3D_v9/` n'a changé entre `ce8a649dd` et `95109dac2`. Pendant l'audit, `morsehgp3D_v10/` a été ouvert en parallèle (`d7a1459dd` à `95109dac2`) ; ce document ne juge pas la v10.
- **Méthode** : 15 lentilles indépendantes (L01 à L15), puis un vérificateur adverse par constat majeur. Tout a été fait en lecture seule sur le dépôt. Les exécutions locales ont tourné sous `nice -n 19` avec au plus 2 fils, sur des binaires déjà construits sous `build/v9-*`. Scripts, sorties et empreintes : `morsehgp3D_v10/audits/audit_v9_20260928/preuves/<lentille>/` et `morsehgp3D_v10/audits/audit_v9_20260928/preuves/verif_*/`. **GCP non utilisé.**
- **Sévérité** : haute, moyenne, basse ; « info » pour un acquis. Après vérification, aucun constat ne reste au niveau « critique » : les vérificateurs ont ramené chacun à « haute » ou moins, notamment parce que le clustering v9 est une expérience `not_claimed`. La gravité d'ensemble tient au **cumul** des défauts du § 8.
- **Statut de vérification** :
  - *confirmé* : le vérificateur a reproduit le constat ;
  - *corrigé* : le vérificateur a amendé portée, chiffres ou cause, et seule la forme corrigée figure ici ;
  - *non vérifié* : constat d'une seule lentille, avec preuve fournie mais sans contre-vérification ;
  - *réfuté* : affirmation fausse, citée seulement pour être écartée (§ 9.3).
- **Nature de la preuve** : PROUVÉ (théorème et fixture), MESURÉ (reçu ou sortie rejouable), AFFIRMÉ (prose seulement).
- **Chemins** : relatifs à `morsehgp3D_v9/` sauf mention contraire. Le « registre » désigne `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md` à la racine.

## Verdict d'ensemble

**Le moteur calcule le bon objet, exactement, relativement à son catalogue.** Pour chaque ordre K ≤ min(Kmax, n) ≤ 10, la v9 construit l'arbre de fusion de π0 de $L_{K}(a) = \lbrace y \in \mathbb{R}^{3} : \lvert X \cap \bar{B}(y, \sqrt{a}) \rvert \geq K \rbrace$ (Th. 2 du manuscrit). Cet arbre comporte les facettes isolées, les multifusions non binarisées, les contributions datées et les images verticales K → K−1. Les niveaux sont des rationnels exacts et toute l'arithmétique est entière (u18, i64/i128/U192/U320). Le calcul passe les contrôles suivants :
- le juge exhaustif Γ (T2, n ≤ 14) ;
- une mini-T2 aléatoire réécrite ici en Python/Fraction : 70 nuages dégénérés, K = 1..5, 13 696 coupes, **0 désaccord** ;
- un juge K = 1 contre l'EMST exact, sur 8k serré, 8k large et la trame LiDAR 08/000000 (39 885 sites) : 0 écart ;
- un énumérateur indépendant par boîtes de centres, qui retrouve les cardinalités du catalogue sur 14 entrées de 8k à 46k, plus la trame brute b02 (125 526 sites).

Aucun défaut d'exactitude du moteur n'a été trouvé.

**La complétude, elle, n'est que relative.** Aucune preuve de complétude du générateur n'est inscrite au registre. L'invariant d'Euler est une condition nécessaire, contrôlée seulement jusqu'à K ≤ Kmax−2. Les boules de fusion seule à l'ordre Kmax (26 à 29 % du catalogue à K5) ne sont ni vues par Euler ni refusées par la tour. Des retraits **artificiels** dans cette zone y produisent des tours fausses publiées `complete_relative` : 10 sur 68 en q2, 26 sur 67 en q3. Aucune omission naturelle n'a été observée ; des juges d'échantillon ciblés existent.

**Le générateur n'est pas sensible à la taille de sortie**, mais seulement sur certaines géométries :
- expansion en 0,44·n² des paires q3/q4 sur des amas de coins séparés : 306,7 s à 32k ;
- travail cubique (en compteurs) sur des sphères creuses ;
- en revanche, travail quasi linéaire sur les autres familles du banc à K = 2 et sur LiDAR.

Sur G4, la chaîne interne atteint 0,76 à 0,98 s à K5 sur trois trames LiDAR sans sol, mais le mur du processus est de 1,5 à 1,9 s, et les condensés de vérification expliquent l'essentiel de l'écart. Le GPU n'accélère que q3/q4 et reste inactif environ 77 % du temps. La tour CPU (246 à 315 ms à K5) suffit à exclure 100 ms dans cette organisation.

**La revendication « hierarchical clustering from the FULL tower, and it beats HDBSCAN's oracle » (`cda636b5e`) n'est pas établie**, pour cinq raisons :
1. La topologie de clustering est le graphe des seules cofaces de Gabriel, que le registre classe `false_in_general` (E5), et non la forêt FULL, pourtant présente dans le même export mais jamais lue. On obtient 18/88/416 racines à K = 2/3/5, contre 1.
2. La condensation n'est pas conforme à HDBSCAN.
3. L'« oracle » HDBSCAN ne parcourt que `min_cluster_size`, avec `min_samples` lié.
4. L'ARI compte le bruit comme une classe.
5. Aucun reçu n'est versionné, et les chiffres précèdent le correctif `ce8a649dd`.

Une fois la condensation corrigée et la racine exclue, le pipeline v9 (toujours sur topologie de Gabriel) obtient environ 0,734 d'ARI sur 250 exécutions. C'est à peu près le niveau d'un HDBSCAN aux mêmes réglages sans oracle (ms = 2, mcs = √n : 0,738). Il reste sous l'oracle 2D (0,782) et sous des têtes simples avec remplissage qui n'utilisent pas la tour (0,80 à 0,84). **Rien ne montre encore que la géométrie exacte de la tour améliore le clustering.**

**Méthode et organisation** :
- CI v9 rouge sur 40 poussées consécutives ;
- reçus sous /tmp devenus irrejouables ;
- dérivés KITTI versionnés contre `AGENTS.md:46` ;
- documentation tenue comme un journal ;
- 31 leviers booléens et fichiers monolithiques ;
- l'erreur E5 est réapparue pour la cinquième fois depuis la v3.

**Pour la v10** :
- **Garder** l'objet, l'arithmétique, le mécanisme d'ancres FULL, les fixtures et les juges.
- **Changer** le générateur. L'indexation par centres est une candidate forte, mais sans borne de pire cas.
- **Changer** la représentation de la tour : rangs u32, sortie CSR directe, parallélisme intra-ordre.
- **Changer** le clustering : il doit lire FULL, avec une condensation HDBSCAN exacte et une porte K = 1 ≡ HDBSCAN(min_samples = 1). L'ablation « tour contre bifiltration k-NN » est obligatoire.
- **Changer** le banc : oracle 2D, HDBSCAN apparié, métriques multiples, préenregistrement, familles à 8k/16k/32k, reçus versionnés.

## 1. Ce que la v9 calcule et ce qui est prouvé

#### A9-01 — L'objet calculé est bien défini et exact relativement au catalogue
- Sévérité : info. Statut : confirmé. Lentilles : L01, L04, L06.
- **Objet.** Catalogue `BallData` : clé primitive, niveau exact, q_min, I strict avec p ≤ 9, coquille U avec u ≤ 12. Règles :
  - admission : p + q_min ≤ min(Kmax+1, n) ;
  - calendrier : K ∈ [p+q_min−1, min(Kmax, p+u)] ;
  - une ancre par couple (K, BallKey) ;
  - lots atomiques par niveau exact, regroupés par racines antérieures au lot : 0 racine donne une naissance, 1 une continuation, 2 ou plus une multifusion ;
  - verticales prises à la coupe fermée.
- **Preuve.**
  - `src/chain/tower_chain.cpp:990,975,944-963` ; `src/tower/forest/full_ball_tower.hpp:1474-1475,2772-2819,2946-3030` ; `morsehgp3D_v7/audits/receipts_plateaux_full_20260906/BALL_ANCHORS.md` §§ 2-5.
  - T2 : `tests/tower/census_tower_oracle.hpp`.
  - Mini-T2 : `morsehgp3D_v10/audits/audit_v9_20260928/preuves/L06_tests_oracles/mini_t2.py`, 13 696 contrôles, 0 désaccord, rejouée par le vérificateur. Elle ne compare que le nombre de composantes à l'ordre K, ni les partitions ni les verticales.
  - K = 1 contre EMST : `k1_emst_judge.py`, rejoué (39 885 sites, 12 coupes, 0 écart).
- **Pour la v10** : cet objet est la référence. On le reproduit, on ne le redéfinit pas.

#### A9-02 — Complétude seulement relative : zone aveugle de détection documentée, aucune omission naturelle observée
- Sévérité : moyenne. Statut : corrigé. Lentilles : L01-F3, L10-04, L15-07, L04-F9, L03-F6.
- **Constat corrigé.**
  - Statut publié : `complete_relative_to_cross_checked_catalogue` (`tower_chain.cpp:2115-2118`).
  - Euler n'est vérifié que pour K ≤ min(Kmax−2, n) (`tower_chain.cpp:2024-2039`). C'est une condition nécessaire, pas suffisante.
  - La tour refuse toute omission isolée d'une boule d'ordre haut telle que p+u ≤ Kmax : 575 retraits sur 575 refusés à 8k (`AUDIT_C_OBJET_ET_RECONSTRUCTION_TOUR_20260923.md:414-428`, lemme de la première cofacette, conditionnel et non registré).
  - La zone aveugle est donc exactement celle des boules de fusion seule à Kmax : q2 à p = Kmax−1 et q3 à p = Kmax−2. Elle pèse 26,2 à 29,4 % du catalogue à K5, 16,8 à 17,8 % à K7 et 9,8 à 11,0 % à K10 (sur 8k).
  - Dans cette zone, 2 retraits sur 312 seulement sont refusés. Parmi les retraits acceptés, 10 sur 68 (q2) et 26 sur 67 (q3) modifient le condensé FULL : ce sont des tours fausses publiées.
  - Il s'agit de retraits **artificiels** de clés déjà émises. Les juges indépendants ne trouvent aucune omission naturelle : 204 683 incidences q2 et 286 706 incidences q3 toutes présentes ; juge brut vert sur 6 cas sur 6, 14 mutants tués.
  - Le vérificateur a rejoué la sonde au HEAD (sortie identique au 23 sept., condensé d94334eda24b3927).
- **Réfuté en partie.** « Euler + Kmax+2 est réfuté par la fixture de 13 points » n'est vrai que **sans** la tour. FULL refuse les omissions D, T et D+T de cette fixture (8 refus sur 8, rejoué au HEAD). Le protocole Kmax+2 reste toutefois hors domaine à K10 (`kBallInteriorMax = 9`) et laisse passer les omissions conjointes compensées.
- **Preuve.** `audits/c_omission_20260923/README.md` ; `audits/CONTRE_EXEMPLE_EULER_KPLUS2_20260923.md` ; `audits/AUDIT_REPRISE_20260926_MATH_EXACTITUDE.md` (le juge brut couvre 0,008 à 0,026 % des clés) ; juges q2/q3 sous le label `scale8000` (`CMakeLists.txt:336-462`).
- **Pour la v10** : publier séparément « exactitude arithmétique », « reconstruction relative » et « complétude des clés ». Porter et requalifier les juges ciblés (ils existent). Chercher un argument de complétude dédié aux fusions seules à Kmax.

#### A9-03 — Des théorèmes porteurs sont absents du registre ; « prouvé conditionnel + testé » est surévalué
- Sévérité : moyenne. Statut : corrigé. Lentilles : L01-F4, L10-09, L15-10, L02-L3.
- **Absents du registre** :
  - l'extension non régulière (quotient de coquille, ancres par (K, BallKey), descente lexicographique à rayon égal, contributions datées), active sur toutes les trames (135 à 572 coquilles étendues par trame à K5) ;
  - l'invariant d'Euler par le nerf, pourtant motif de refus (`tower_chain.cpp:2024-2039`) ;
  - la complétude propre au générateur : théorème H, frère, ComplementFirst, Pool, lemme du citron, certificat d'atlas, inductions q3/q4.
- **Présents** : section V9-S4 (lignes 1265-1288 : suffisance du cover, lemmes L1-L11 et L15, avec la réserve « ni preuve de complétude des clés jamais proposées »), lignes 151-153 (28 sept.), ligne 120 (théorème 4.2), lignes 180-181.
- **Nuances.** Le lemme de la première cofacette n'est pas invoqué par le code. La règle « inscrire avant d'invoquer » est propre au plan v9 (`docs/PLAN_V9.md:261-262`, `docs/HERITAGE_V7_V8.md:30-31`). `HERITAGE_V7_V8.md:20` qualifie l'extension de « prouvé conditionnel + testé », alors que l'audit C n'y voit qu'une esquisse en huit étapes, contrôlée sur 33 nuages.
- **Pour la v10** : un registre court (une vingtaine d'énoncés : Th. 2, Prop. 5, th. 4.2, S.1-S.6, BALL_ANCHORS, Euler par le nerf, première cofacette, complétude du générateur retenu, verticales). Chaque énoncé porte son statut, ses prémisses, sa fixture d'égalité et sa porte.

#### A9-04 — Le statut publié ne distingue pas le chemin régulier du quotient de plateau
- Sévérité : moyenne. Statut : non vérifié. Lentille : L01-F7.
- `PLAN_V9.md:109-111` prévoyait `exact_full_regular`, `exact_full_quotient_certified` et un refus de domaine. Le code ne publie que `complete_relative*` (`tower_chain.cpp:42,2115-2118`). Un grep de `exact_full_regular` dans `src/` ne renvoie rien.
- **Pour la v10** : un statut typé par exécution, qui indique le nombre de boules étendues et la base de preuve effectivement consommée.

#### A9-05 — Domaine plus étroit que l'objet : doublons refusés, coquille > 12 fatale, K ≤ 10, garde 2^117 sans marge
- Sévérité : moyenne. Statut : non vérifié (observé indirectement). Lentilles : L01-F6, L02-M5, L03-F7, L10-13, L15-13.
- **Constats.**
  - La spécification compte les points avec multiplicité, alors que la v9 refuse tout doublon (`src/gen/pipeline/prepared_cloud.cpp:71-76`) et que le banc lève une erreur (`experiments/synthetic_bench_20260928/bench_datasets.py:313-314`).
  - Une seule coquille de plus de 12 sites fait refuser toute la chaîne (`tower_chain.cpp:940-943`, `src/tower/forest/local_plateau.hpp:47-48`). Le vérificateur de L13-F2 l'a observé sur une entrée de 108 points cocirculaires plus 2 000 points de bruit : `unsupported_degeneracy chain_shell_above_12`. Ce refus est honnête. En revanche, la v5 présentait à tort ce plafond comme « borné par théorème » (`morsehgp3D_v5/README.md:34-36`), et la v8 a exercé des coquilles de 30 sites.
  - La garde `certified_cell` à 2^117 n'a aucune marge à 18 bits (audit C § 2.3).
- **Pour la v10** : décider explicitement de la sémantique des multiplicités, qui conditionne l'équité face à HDBSCAN. Remplacer la table en O(u·2^u) par le lien inférieur Λ_t sur S² (arrangement de grands cercles, à graver en fixture avant usage), ou compter et publier chaque refus.

#### A9-06 — Séparation WSPD : paramètre de performance, sans effet sur l'objet
- Sévérité : basse. Statut : non vérifié. Lentilles : L01-F10, L02-L1.
- La séparation n'intervient dans aucun certificat q2 ni q3/q4 (audit C § 2.2 ; `q2_node_pool.hpp:44-46`). La chaîne refuse pourtant s < 8 comme une entrée invalide (`tower_chain.cpp:1326`). À s = 8, 65 à 91 % des rectangles q2 émis sont des paires 1×1.
- La directive utilisateur « s ≥ 8 » reste en vigueur et n'est pas remise en cause ici.
- **Pour la v10** : documenter s comme réglage de performance sous cette directive, et ajouter une porte d'invariance de l'objet (condensé identique à s = 8, 10 et 12).

#### A9-07 — Verticales : suffisance conditionnelle, jugées seulement pour n ≤ 14 et par la naturalité aux fusions
- Sévérité : basse. Statut : non vérifié. Lentille : L01-F11.
- Registre l. 134 (`conditional_theorem`) ; `full_ball_tower.hpp:2936,2965,3019`.
- **Pour la v10** : un juge d'échantillon des images π0(L_{K+1}(a)) → π0(L_K(a)) à l'échelle.

#### A9-08 — Catalogue scellé R-29 : validation au 1/64 et résidu mémoire déclaré
- Sévérité : moyenne. Statut : corrigé. Lentilles : L01-F8, L12-07.
- La passe 1 de la tour ne porte que sur une boule sur 64 (`full_ball_tower.hpp:220,1839-1864`). Le résidu est déclaré : `n_interior > 9` provoque une lecture hors bornes, et la passe 2 lit `ix.upos[site]` sans vérifier la borne (`full_ball_tower.hpp:190-206`).
- Le résidu n'est atteignable que par une corruption en mémoire après le recensement : constructeur privé, vecteur const, API publique entièrement validée.
- Le mode est désactivé par défaut (`tower_chain.hpp:192`), mais actif dans 30 des 36 bras de R22, y compris les meilleurs chronos. Gain : environ 16 ms à K5 et 60 ms à K10.
- **Pour la v10** : la validité doit être portée par les types chez le producteur, sans échantillonnage qui laisse une lecture hors bornes possible.

#### A9-09 — L'objet v9 n'est défini nulle part de façon autonome
- Sévérité : moyenne. Statut : non vérifié. Lentilles : L01-F9, L15-09.
- `docs/SPECIFICATION_MORSEHGP3D.md` fait 1 589 lignes, surtout sur des jalons retirés. Le registre fait 1 320 lignes et mêle jalons et théorèmes. L'objet v9 n'est défini que par renvoi aux audits v7.
- **Pour la v10** : une spécification de quelques pages (objet, catalogue, ancres, lots, verticales, domaine, statuts).

## 2. Générateur q2

#### A9-10 — Chemin q2 de production exact
- Sévérité : info. Statut : non vérifié (lecture et juges existants). Lentille : L02.
- Le chemin est `run_wspd_q2_census_parallel`, appelé depuis `tower_chain.cpp:1416-1421`. Le prédicat est H = (z−a)·(b−z), en entiers < 2^44. La couverture des paires se fait au plus bas ancêtre commun, avec un registre de masse Σ rejetées + résiduelles = C(n,2) vérifié à l'exécution. Le théorème H transmet des rangs, jamais des comptes. Le Pool terminal filtre par bandes h_a + h_b.
- Juges : oracles exhaustifs jusqu'à 20 sites ; juge d'échantillon indépendant `tests/chain/q2_sample_judge.cpp` à 2k et 8k.
- Lemme mesuré : p ≤ min(ρ_a(b), ρ_b(a)), sans aucune violation sur 2,4 M paires LiDAR.

#### A9-11 — Recensement redondant : deux passes de trop, pas trois recensements
- Sévérité : moyenne. Statut : corrigé. Lentilles : L02-H1, L07-F5, L12-07.
- **Constat corrigé.** Les deux passes redondantes sont :
  - la collecte des identifiants par le générateur, faite par paire depuis la racine (`q2_census.cpp:364-388`). La chaîne n'en garde que les tailles (`tower_chain.cpp:1401-1414`) ;
  - le re-recensement de chaque clé distincte sur l'index de la tour (`tower_chain.cpp:881-1013`, 1119).
- Le comptage partagé par bloc n'est pas redondant. Le recoupement entre deux implémentations est une doctrine délibérée (`tower_chain.hpp:8-17`). Le re-recensement est fait par clé distincte, conformément à la lettre de `PLAN_V9.md:86-88`.
- Dans R22, le côté q2 est entièrement recouvert par l'appareil (q2_wait = 0 sur les 36 sondes). Le gain possible sur le chemin critique est faible ; le recensement précoce ne retire que 4 ms à K5.
- **Pour la v10** : une seule collecte par clé distincte, avec les identifiants transmis à la tour. Le recensement indépendant devient un juge d'échantillon.

#### A9-12 — Deux fronts WSPD et deux index spatiaux construits indépendamment
- Sévérité : moyenne. Statut : corrigé. Lentilles : L02-H2, L12-10.
- **Deux fronts** : q2 (masque 1, `q2_census.cpp:1258`) et q3/q4 (masque 6, `tower_chain.cpp:1478`). C'est l'option (b) prescrite par `PLAN_V9.md:83-86`. Le front unique n'était permis qu'après un juge des bornes Ξ, jamais livré (`PLAN_V9.md:174`).
- **Deux index construits** : l'arbre kd du générateur et l'arbre de Karras de la tour (8,6 à 34,6 ms). L'index plat du GPU n'est qu'une copie du premier.
- Le nœud de l'index gen pèse 64 o, en size_t, sans réservation (`q2_census.cpp:97-171`). `gen_index` prend 12,2 ms à 40k et 37,3 ms à 123k, sur le chemin critique : 1,3 % de la chaîne, mais 12 % d'un budget de 100 ms.
- **Pour la v10** : un index u32 unique, construit en parallèle et partagé par tous les étages.

#### A9-13 — Rapport candidates/acceptées élevé sur LiDAR ; q2 hors de l'enveloppe de 100 ms
- Sévérité : moyenne. Statut : corrigé. Lentille : L02-H3.
- **Chiffres corrigés.** Sur les trames entières sans sol, q2 examine 18,5 à 21,4 candidates par support accepté à K5 et 13,4 à 15,4 à K10, contre 1,7 à 2,8 sur les familles synthétiques. On compte 272 à 412 visites de nœuds par support, contre 101 à 165 en uniforme.
- **Réfuté** : « ce rapport croît avec n ». La croissance vient des coupes radiales emboîtées, qui agrandissent un disque (trame s00 entière : 20,0, alors que la coupe de 32k donne 32,9).
- Le temps q2 sur G4 est de 76 à 116 ms à K5 et de 148 à 223 ms à K10, mais recouvert par l'appareil. La v9 reconnaît déjà l'écart par rapport aux 35 ms prévus (`audits/PLAN_CRITIQUE_100MS_FULL_20260924.md:59,67-68`).

#### A9-14 — Leviers q2 figés sans requalification à 1 mm ; grand-livre non publié
- Sévérité : moyenne. Statut : corrigé. Lentille : L02-H4.
- **Constat.** La configuration q2 est codée en dur (`tower_chain.cpp:1416-1421`) et ne se règle depuis aucune option. La sonde G4 ne publie que 3 compteurs de travail q2.
- **Ablations locales** (compteurs déterministes, 102 exécutions, rejouées bit à bit) :
  - le Pool est indispensable (candidates ×1,53 à ×4,95 sans lui) ;
  - ComplementFirst retire 9 à 20 % des visites géométriques (le chiffre de « −2 % » venait d'une pondération qui comptait les scissions structurelles) ;
  - le frère retire au plus −7 % ;
  - {4, all, true} donne un travail ×0,75 à 0,78 à 32k seulement, et de 0,62 à 1,05 aux autres tailles.
- **Nuances.** La base v8 avait été mesurée sur KITTI réel à 2 cm. La v9 a mesuré trois leviers d'ordonnancement q2 (R11, R19, R22). Le constat recoupe `audits/c_audit_20260923/lectures/L2_rapport.md` § 6.
- **Pour la v10** : publier le grand-livre complet, garder le Pool, et ne rien retirer sans chronométrage apparié.

#### A9-15 — Code mort et défauts d'API contraires à la configuration de production
- Sévérité : moyenne. Statut : corrigé (par L12-05). Lentilles : L02-M1, L12-05, L03-F5.
- **Constat.**
  - Au moins 14 valeurs d'énumération du générateur ne sont jamais passées par la chaîne : `WspdFrontMode::Pure`, `Q2CensusMode::Pairwise`, `Q2SiblingMode::Disabled`, `Q2WitnessOrder::GlobalDfs`, `Q2AnchorMode::{SharedProduct,SharedAnchors}`, `WspdQ4Backend::Window30`, entre autres.
  - Quatre points d'entrée publics n'ont aucun appelant ni aucun test : `run_q34_{collective,mapped}_{seed,edge}_candidates`.
  - Les défauts de l'API gen sont justement les modes non utilisés en production, et la chaîne doit les surcharger à la main.
  - `core/fixed_signed.hpp` (323 lignes) n'est inclus nulle part.
- `src/gen` est à 84,5 % identique à la v8 (14 817 lignes sur 17 534). C'est une bifurcation figée, pas une double maintenance.

#### A9-16 — Une voie rapide kNN exacte n'est pas exploitée, mais les ponts longs l'empêchent de suffire
- Sévérité : moyenne. Statut : non vérifié (mesure de lentille). Lentille : L02-M4.
- Sur 12 cas LiDAR, 24,0 à 27,1 % des supports q2 ont un rang min(ρ_a, ρ_b) < K : ils sont acceptés sans recensement et lisibles dans la liste kNN. En revanche, 0,15 à 0,61 % ont un rang ≥ 64K, et cette part croît avec n. Ce sont les ponts inter-amas, justement utiles au clustering.
- Preuve : `morsehgp3D_v10/audits/audit_v9_20260928/preuves/L02_gen_q2/knn.jsonl`.

#### A9-17 — Aucune porte q2 à 16k ni 32k ; invariants bon marché absents
- Sévérité : moyenne. Statut : non vérifié. Lentille : L02-M2.
- Les juges q2 ne tournent qu'à 2k et 8k (`CMakeLists.txt:336-465`). Les invariants « kNN ⇒ q2 » et « EMST ⊂ présentations à p = 0 », proposés par l'auditeur C, ne sont pas implémentés. Le juge K = 1 contre EMST, mesuré par L06, n'existe pas en CTest.

#### A9-18 — Pentes lues sur des coupes LiDAR radiales emboîtées
- Sévérité : moyenne. Statut : corrigé (via le vérificateur de L02-H3). Lentilles : L02-M3, L07-F1.
- `receipts/lidar_scaling_local_20260923` lit ses exposants sur des disques emboîtés : chaque doublement ajoute des points plus clairsemés. La phrase « boules sous-linéaires » décrit donc la scène, pas l'algorithme.
- **Pour la v10** : tirer les pentes de familles homogènes à 8k, 16k et 32k.

## 3. Générateur q3/q4

#### A9-19 — Géométrie q3/q4 saine et bien prouvée
- Sévérité : info. Statut : non vérifié (recalculs de lentille ; lemmes au registre V9-S4). Lentille : L03.
- Lemme du citron : α3 = 3, α4 = 2, soit |c−m|² ≤ D/12 en q3 et ≤ D/8 en q4 (`src/gen/lanes/q34_dead_lanes.hpp:31-38`).
- Suffisance du cover. Formes affines du plan bissecteur. Graine aiguë canonique (Σλ_v·H(v) = −2|c−m|² < 0). Propriété par l'arête la plus longue du support. Clé `ExactBall` primitive (5 × i128). Balayage à seaux S4b.
- Corollaire vérifié par le vérificateur de L03-F2 : un témoin q3 vérifie l'angle azb > 120°, un témoin q4 vérifie azb > 125,26°. Le filtre de rectangle ne peut donc trouver aucun témoin universel dans le vide entre deux facteurs.

#### A9-20 — Expansion quadratique des paires q3/q4 sur amas de coins séparés
- Sévérité : haute. Statut : confirmé, avec une portée corrigée. Lentilles : L03-F2, L10-05, L13-F1, L05-03, L07-F1.
- **Constat.**
  - Sur la recette `eight_corner_clusters_splitmix64_v1` (huit cubes u16 de côté 1024, séparés d'environ 29 000) à K5, s = 8, on développe 28,35 / 112,77 / 449,65 M de paires à 8k / 16k / 32k, soit ×3,98 par doublement (0,443 à 0,439·n²). C'est le produit inter-amas entier.
  - Chaîne : 21,9 / 83,4 / 306,7 s (W4) ; le filtre q34 prend 221 s sur 306,7 à 32k.
  - Les paires développées sont réfutées une à une (98,2 à 99,45 %). Survivants : 0,50 / 1,10 / 2,49 M (×2,2). Catalogue : 510 758 / 1 099 180 / 2 305 835.
  - Cause : le front émet les 28 produits inter-amas comme des rectangles d'amas entiers. Le test de rectangle est en tout ou rien et n'est suivi d'aucun raffinement récursif (`src/gen/pipeline/wspd_q34.cpp:460-546`) : en cas d'échec, tout le produit est développé.
  - Le plan par facteurs (prototype non porté) réduit l'expansion à environ 6,8 %, mais la croissance reste de ×3,942.
- **Portée corrigée.** Cette recette **n'est pas** une famille du banc synthétique du 28 septembre.
  - Sur le banc à K = 2, de 8k à 32k : spherical, filaments, hierarchical, bridge et heteroscedastic sont quasi linéaires (pente de chaîne 0,87 à 1,22) ; spherical K5 aussi.
  - Hierarchical K5 est superlinéaire : ×3,15 puis ×3,20 de 4k à 16k.
  - Sur LiDAR, la chaîne a une pente de 0,73 à 1,49.
- **Preuve.** `receipts/q3_payload_local_20260926/README.md:56-116` ; `receipts/q34_factor_plan_20260926/README.md:70-83` ; mesures des vérificateurs `morsehgp3D_v10/audits/audit_v9_20260928/preuves/L10-05_verif/`, `verif_L07_F1/`.
- **Pour la v10** : réfuter par sous-blocs certifiés avant l'expansion, ou changer d'indexation (A9-95). Mettre en porte le nombre de paires testées sur des familles adverses (amas de coins, plans séparés, coquilles).

#### A9-21 — Travail cubique en compteurs sur des sphères creuses (famille shells)
- Sévérité : haute. Statut : corrigé. Lentille : L03-F1.
- **Constat corrigé.**
  - Sur `shells` (8 sphères creuses à densité surfacique fixe), à K5 comme à K2, les compteurs du générateur ont un exposant de 2,8 à 3,0 entre 8k et 32k : `core_sites`, `dead_core_uniform_tests` (1,6 G, puis 12,1 G, puis 97,0 G) et `cover_sites`. Les sorties restent linéaires (375 k boules à 32k).
  - Le mécanisme exact : 18,2 M arêtes survivent au filtre de témoins, et **chacune** construit le noyau diamétral (1 212 sites en moyenne, 22,1 G au total). Seules 16 % construisent ensuite le cover (4 276 sites en moyenne).
  - Les cordes quasi antipodales survivent à 92–96 %, avec 0 témoin médian.
- **Temps.** Le mur q34 passe de 57,5 à 179,9 puis 1 438,5 s à K5 ; ce dernier chiffre est mesuré avec `--no-tower`. Les temps ne se reproduisent pas à mieux qu'un facteur 1,5 (rejeu à 16k : 277 s). Seul le dernier doublement est ×8 en CPU.
- Le plan du banc ne fait tourner `shells` qu'à n = 2000 : les résultats publiés du banc ne sont pas bloqués.
- **Preuve.** `morsehgp3D_v10/audits/audit_v9_20260928/preuves/L03_gen_q34/data/shells_*_k5.json` ; `verif_L03_F1/` ; binaire `build/v9-q3-payload-integration-20260926/mhgp9_tower_probe` (sha256 3647d904…, dont les sources n'ont pas changé jusqu'à HEAD ; provenance plausible mais non prouvée formellement).

#### A9-22 — Entonnoir dominé par la réfutation ; un reçu décrit mal les arêtes vivantes
- Sévérité : moyenne. Statut : corrigé. Lentille : L03-F3.
- **Constat.** Sur 708 686 arêtes ouvertes après certificats, seules 312 064 émettent. Les 396 622 autres (56 % **en nombre**) sont ouvertes mais muettes. La phrase « ce sont les arêtes qui émettent » (`receipts/q34_survivor_phases_20260923/README.md:70-72`) est donc inexacte, et aucun addendum ne la corrige.
- **Portée corrigée.**
  - Les parts « 85,7 / 96,7 % des cycles » et « cover de 1 704 contre 43 sites » décrivent l'état **antérieur** au certificat de voie morte (`e54f727c`).
  - La part du travail actuel consacrée aux arêtes muettes n'est pas mesurée.
  - Graines q4 : 5,81 M dans le registre moteur, 7,13 M selon le compteur S4b ; `PLAN_S4.md:287` interdit de comparer ces deux registres.

#### A9-23 — Cascade de filtres : une seule redondance stricte (cover construit deux fois sur le chemin GPU)
- Sévérité : basse. Statut : corrigé. Lentille : L03-F4.
- **Constat corrigé.** Sur le chemin GPU, le cover d'une arête ouverte est construit deux fois : `src/gpu/certificate.hpp:896`, puis `src/gpu/lanes.hpp:405`. L'étape T3, qui devait transmettre les plages du cover aux voies, n'a pas été faite. Le noyau diamétral (`certificate.hpp:883`) est une autre boule, un étage de filtre. Le coût de la redondance est d'environ 4 à 5 % des visites, soit 15 ms à K5 et 40 ms à K10.
- **Réfuté** : remplacer toute la cascade par « un seul noyau par candidat » appliquerait une marche de 137 à 204 visites à 23,7 M paires. Ce serait probablement plus cher que la cascade actuelle.

#### A9-24 — Deux implémentations de l'énumération q3/q4 et trois des formules de boule
- Sévérité : moyenne. Statut : corrigé. Lentilles : L03-F5, L12-05.
- **Deux énumérateurs** :
  - le moteur à atlas v8 (`wspd_q34.cpp`, `q4_local*`), qui est le chemin par défaut et le repli des arêtes reportées ;
  - les voies sans atlas S4a/S4b, une seule source `MHGP9_HD` exécutée par HostGroup ou WarpGroup.
- **Trois formules de clé et de niveau** : `gen/lanes/exact_ball.cpp:85,107`, `gpu/lanes.hpp:201` et `gpu/q4_lanes.hpp:190`, puis `tower/lanes/q3.hpp` et `q4.hpp`. Cette dernière sert aussi à la tour ; le recoupement v8/v7 est délibéré (`tower_chain.cpp:890-891`, « chain_key_mismatch_v8_v7 »).
- Volume : environ 16,6 kLOC pour q3/q4, plus 25,8 k lignes de tests ; 18 champs `WspdQ34Options`.
- **Pour la v10** : une seule implémentation par voie, jugée par l'égalité du multiensemble (clé, support, profondeur, coquille), pas compteur par compteur.

#### A9-25 — Commentaires restés en u16 après le passage à u18
- Sévérité : basse. Statut : non vérifié. Lentille : L03-F8.
- `q34_witness_search.hpp:86-88`, `q34_witness_search.cpp:100`, `q3_ball_census.hpp:79`, `wspd_q34.cpp:721`.

## 4. Catalogue et tour FULL

#### A9-26 — Construction de la tour correcte ; un seul objet quel que soit le nombre de fils
- Sévérité : info. Statut : confirmé (lecture et portes). Lentilles : L04, L12-04.
- La résolution de chaque représentant se fait sur l'état antérieur au lot : MEB proposée par Welzl en double puis vérifiée exactement, recherche d'intrus strict, décroissance lexicographique (rayon, coquille retenue). Un terminal manquant donne un refus. Le regroupement haché est exact. Une seule racine finale par K (`full_ball_tower.hpp:1402-1404`).
- La sortie est bit-identique pour 0, 1, 4 et 8 fils (porte `mhgp9_chain_static_paths`). Le condensé LiDAR 67450c64611075b1 est épinglé de W = 1 à 8 et identique à la sonde G4 à W = 48.

#### A9-27 — Chemin critique de la tour : sériel intra-ordre à K5, phase 0 sérialisée entre ordres à K10
- Sévérité : moyenne. Statut : corrigé. Lentille : L04-F2.
- **À K5** (R22 probe_0, 289 ms) : validation 34, phase 0(K5) 38, phase A(K5) 148, phase C(K5) 28, encodage 39. Les phases A, C et l'encodage, séquentielles à l'intérieur de chaque ordre, pèsent environ 215 ms : c'est un majorant du temps purement sériel, et à lui seul plus de deux fois le budget de 100 ms.
- **À K10** (1 212 ms), la décomposition de L04 est fausse. La phase 0 est sérialisée d'un ordre à l'autre par K décroissant (`full_ball_tower.hpp:1026`) et occupe 60 à 75 % de la fenêtre. Le vrai chemin : validation 117,5, phase 0 de K10 à K8 489,7, phase A(K8) 399,7, phase C(K9) 101,4, encodage(K10) 92,8, soit environ 1 201 ms. Le sériel intra-ordre y pèse environ 594 ms (49 %).
- **Réfuté** : « ajouter des cœurs ne réduit pas le plancher » à K10.
- Le proxy « union-find maigre, 48 à 87 ms » de L04 ne prédit pas le moteur.
- **Pour la v10** : parallélisme intra-ordre pour A, C et l'encodage ; supprimer la sérialisation de la phase 0 entre ordres ; écrire la sortie directement.

#### A9-28 — Empreinte mémoire : environ 810 o par boule, sortie de 1 Go à K10
- Sévérité : moyenne. Statut : corrigé. Lentilles : L04-F3, L07-F6, L12-08.
- **Constat corrigé.** Le pic RSS du bras GPU comprend le bassin épinglé : 1,28 Go à K10. Le jumeau moteur calcule le même objet avec :
  - 1 206 408 Kio (K5) et 4 468 856 Kio (K10) sur 39 885 sites ;
  - 8 820 864 Kio sur le brut K10 (125 526 sites).
  - Le RSS suit donc le nombre de boules, environ 755 à 810 o par boule.
- **Tailles vérifiées à la compilation** : ExactLevel 48 o, FullNode 64 o, BallData 224 o, requête de phase 0 56 o ; FullDatedContribution fait **80 o** (et non 72).
- Dans la sortie publiée, les copies du niveau représentent 56 à 57 % des octets (1,007 Go à K10). Ce sont 13 à 20 % du pic RSS, pas la cause principale.
- **Réfuté** : « toute comparaison passe par U320 ». Les phases 0 et A comparent déjà des rangs u32 (`full_ball_tower.hpp:1934-2000`, E1/E3 depuis `96a053805`).
- Pas bloquant pour le contrat v9 ni pour un banc à 32k. Bloquant à K10 au-delà d'environ 2 M sites (extrapolation, non mesurée).
- **Pour la v10** : rangs u32 dans la sortie, une table unique des niveaux exacts, populations lues dans le catalogue en CSR, et une clé reconstruite depuis le support.

#### A9-29 — Phase 0 super-linéaire en K, pas en n
- Sévérité : moyenne. Statut : corrigé. Lentille : L04-F4.
- **Chiffres exacts** : de K5 à K10 sur la trame 00, les nœuds de sortie font ×4,82, les appels MEB ×8,77 et les nœuds visités lors des recherches d'intrus ×12,72. La phase 0 passe de 84 à 738 ms (dont résolution : 564 ms).
- **Cause corrigée.** Le coût par requête ne passe que de 48 à 57 nœuds. La croissance vient du nombre de requêtes et de pas de descente (×10,7) et du coût d'une MEB à k sites (distances par appel ×4,1). Relancer depuis un nœud local n'apporterait qu'un gain constant. Les leviers utiles : saut au centre (D5), MEB incrémentale.
- À K5 fixé, la phase 0 est quasi linéaire en n. Pour le banc synthétique, c'est un coût secondaire (la tour pèse 0,7 à 10 % de la chaîne).
- Le constat figurait déjà dans `audits/FULL_PARTAGE_INTER_ORDRES_20260926.md` § 1.

#### A9-30 — Complexité du code de la tour et couplage de l'export de clustering à un fichier d'audit
- Sévérité : moyenne. Statut : corrigé. Lentilles : L04-F5, L12-04, L12-06.
- **Constat corrigé.**
  - `full_ball_tower.hpp` fait 3 097 lignes et contient quatre voies de résolution :
    - temporelle, choisie automatiquement à W ≤ 1 ;
    - statique séquentielle ;
    - statique parallèle ;
    - recouverte.
  - Des options sœurs s'y ajoutent (overlap_static, pipelined_tail, hash_grouping, persistent_pool, sealed, meb_proposal). Les défauts de la tour (`static_threads = 0`, `overlap_static = false`) diffèrent de ceux de la chaîne (`tower_static_threads = -1`, `true`).
  - La sortie reste identique quel que soit le chemin, sur ce qui est testé.
  - Le tri n'est pas qu'un témoin : c'est le repli au-delà de 2^31 requêtes (l. 2602).
  - L'API `FullBallBatchResolver` / `prepare_external_batch` (101 lignes) n'a aucun appelant v9.
  - Les macros de mutants sont nombreuses mais compilées seulement dans les cibles de test.
- **Plus grave** : `audits/b_full_a_manifest_20260927/native_a.hpp` (2 484 lignes, copie générée et épinglée par empreinte de la tour) est inclus par `experiments/weighted_clustering_20260927/native_attachment_export.cpp:4`, l'export d'attaches du clustering. Toute modification de la tour casse donc l'export.
- **Pour la v10** : un seul algorithme de tour, identique à 1 et à W fils ; la voie temporelle devient un juge de test ; aucun code d'audit n'est inclus par le produit.

#### A9-31 — L'encodage final re-valide et recopie tout le brouillon
- Sévérité : basse. Statut : non vérifié (chronos confirmés par L07-F6). Lentille : L04-F6.
- `full_ball_tower.hpp:694-743` et `full_coverage_certificate.hpp:287-402`. L'encodage coûte 39 ms à K5 et 93 ms à K10, soit 3 à 4 % de la chaîne.

#### A9-32 — Sursouscription : 409 à 889 fils créés par construction
- Sévérité : moyenne. Statut : non vérifié (comptes confirmés par le vérificateur de L04-F2). Lentilles : L04-F7, L12-11.
- `helper_threads` vaut 409 à K5 et 889 à K10, en plus des 47 fils du pool. Au moins 12 sites de création de fils existent. Les temps de lots publiés par K sont gonflés par la contention : A(K5) vaut 190 ms dans le passage K10 contre 148 ms dans le passage K5.
- **Pour la v10** : un ordonnanceur unique et une occupation mesurée par étape.

#### A9-33 — Condensés dépendants de la représentation
- Sévérité : moyenne. Statut : non vérifié. Lentille : L04-F8.
- `tower_digest` hache la représentation brute non réduite du niveau (`tower_chain.cpp:1202-1207,1249-1282`). `catalogue_digest` hache des rangs Morton, pas des PointId.
- **Pour la v10** : un condensé canonique (niveaux réduits, PointId, numérotation canonique) et un exportateur de compatibilité v9 distinct pour les campagnes appariées.

#### A9-34 — Alias mutable de la banque de populations toujours ouvert
- Sévérité : moyenne. Statut : non vérifié (reçu existant). Lentille : L04-F10.
- `full_coverage_certificate.hpp:95` ; `audits/b_population_alias_20260927/README.md` ; `receipts/population_alias_20260927`.

#### A9-35 — Ancres (K, boule) non publiées ; image de naissance calculée par DSU alors qu'elle est directe
- Sévérité : basse. Statut : non vérifié. Lentilles : L04-F11, L04-F12.
- Les ancres sont détruites avec `OrderState` (`full_ball_tower.hpp:1128-1131`). La voie pondérée du 27 septembre a dû les récupérer par instrumentation. L'image d'une naissance à K est l'ancre du même bloc à K−1, déjà racine à la fin de son lot.
- **Pour la v10** : publier `anchor[K][bloc]`. Le rattachement exact des facettes au clustering devient alors trivial.

## 5. GPU et G4

#### A9-36 — Le GPU n'accélère que q3/q4 et reste inactif environ 77 % du temps
- Sévérité : haute. Statut : confirmé. Lentilles : L05-01, L07-F9, L10-12.
- Trois appels à l'appareil, tous consacrés à q3/q4 (`tower_chain.cpp:1504-1603`) : filtre témoin S2, certificats S3, voies S4a/S4b.
- Restent sur CPU : le front, q2 (recouvert, q2_wait = 0), la fusion, le recensement et la tour.
- **Nsight R2** (`docs/PROFIL_FULL_NSYS_20260927.md`) : noyaux 207,9 ms par passage, union noyaux + copies + remises à zéro 213,1 ms, pour une médiane chaude de 912 ms. Les activités GPU ne se recouvrent pas entre elles.
- Si les noyaux ne coûtaient rien, la chaîne K5 descendrait vers 0,70 à 0,72 s. La tour seule prend 246 à 315 ms à K5.
- Le document v9 reconnaît lui-même que 100 ms est hors de portée dans cette organisation.

#### A9-37 — Le gain d'environ ×3 mélange appareil, autre algorithme et recouvrements
- Sévérité : moyenne. Statut : corrigé. Lentille : L05-02.
- **Mesures R22** : chaîne 0,926 s (GPU) contre 2,770 s (moteur), soit ×2,33 à ×3,05. Le mur du processus ne gagne que ×1,60 à ×1,97. CPU·s : 17,3 contre 114,9.
- Les deux bras diffèrent de 12 leviers. S4a et S4b n'ont pas de chemin CPU natif : seul existe un jumeau hôte qui exécute les warps en série. Ce jumeau utilise à ±7 % près le même temps CPU que le moteur, donc son retard vient de l'ordonnancement, pas du travail.
- S2 par lots sur CPU a été mesuré dans R13 (3,84 s contre 3,27 s à K5). S3 et S4 n'ont jamais été chronométrés sur CPU sur G4.
- **Pour la v10** : tout gain GPU se mesure contre une version CPU native du même algorithme, en temps mural et en CPU·s.

#### A9-38 — Plafonds durs du chemin GPU et refus global
- Sévérité : haute. Statut : corrigé. Lentille : L05-03.
- Le filtre fait un appel unique pour toute la trame, limité à 2^31−1 paires et à la moitié de la mémoire libre (`src/gpu/filter_runner.cu:572-576`). Tout dépassement refuse la chaîne entière (`tower_chain.cpp:273-274,1604-1608`), sans repli CPU.
- Sur les amas de coins à K5, le seuil tombe vers n ≈ 70k. Le banc synthétique n'utilise pas ce chemin : il est compilé avec le stub.
- Le tampon d'événements q4 atteint 4 091 sur 4 096, mais son débordement reporte l'arête au CPU : ce n'est pas un refus.
- Arène de covers : 485,8 M sites utilisés sur environ 855 M au brut K10, soit une marge de ×1,76. Le commentaire « 379 M » du code est périmé.
- **Pour la v10** : des lots bornés avec repli par lot, et aucun travail quadratique confié à l'appareil.

#### A9-39 — Protocole G4 lourd et échecs évitables
- Sévérité : moyenne. Statut : corrigé. Lentilles : L05-04, L12-14.
- Environ 38 tentatives entre le 22 et le 27 septembre. Sept ont touché GCE sans produire aucune mesure : R4 (préemption), R7 (rupture de stock), S1 tentative 1, q3 original, q3 reprise R2, q34_survivors r1, Nsight r1. L'allocation perdue représente environ 15 à 17 minutes.
- Le schéma de la sonde est passé de v1 à v30 en environ quatre jours.
- La capture exclut `output/build` (`gcp-migration/tower_session_v9.py:526`) : le binaire n'est jamais conservé.
- Après l'échec du 23 septembre (CUDA_STANDARD 20 refusé par CMake 3.22), le préflight du 27 a de nouveau tourné sous CMake 3.28.3 et échoué : la leçon n'a pas été retenue.
- **Réfuté** : « le Python de protocole dépasse le C++ produit ». Le Python propre aux sessions représente environ 18 300 lignes, pour 33 489 lignes de C++.
- **Pour la v10** : un exécuteur générique piloté par un plan ; une chaîne d'outils épinglée à celle de la VM (CMake 3.22.1, nvcc 12.9.41, pilote 580.173.02) ; le binaire capturé avec son empreinte.

#### A9-40 — Plomberie CUDA naïve
- Sévérité : moyenne. Statut : non vérifié. Lentille : L05-06.
- Allocation et libération à chaque appel, flux par défaut synchrone, copies depuis de la mémoire paginable, ardoises de certificats d'environ 10,25 Go par appel (`filter_runner.cu:520-541,590-592,839-866`). La colle hôte coûte environ 70 ms à K5.

#### A9-41 — Variantes GPU et S2 du 26–27 septembre neutres ou négatives de bout en bout
- Sévérité : moyenne. Statut : non vérifié. Lentilles : L05-07, L07-F8.
- Cache par tuiles : −1 à −4 ms. Passe fusionnée L15 : +3,5 à +4 % sur l'étape T. Consommateur par vagues : 9,9 s de préparation CPU. Adaptateur résident : 504 à 628 ms. Aucune n'a changé le mur (`receipts/g4_tile_cache_20260926`, `q34_cuda_g4_20260927`, `q34_resident_g4_20260927`).

#### A9-42 — Chemin GPU calibré sur une seule carte de 96 Go en sm_120
- Sévérité : basse. Statut : non vérifié. Lentille : L05-08.
- `CMakeLists.txt:869` ; capacités fixées en fractions de la mémoire (`filter_runner.cu:839,1244-1246,1325`).

## 6. Portes et oracles

#### A9-43 — Oracle Γ excellent, mais le T2 de chaîne ne tourne qu'à Kmax = 10 sur trois nuages
- Sévérité : moyenne. Statut : corrigé. Lentilles : L06-F4, L10-14.
- **Constat corrigé.** `tests/chain/chain_census_tower_gate.cpp:143,154,180,184,187,215` fixe K = 10, sur line12 (colinéaire, sans q3/q4), shell14 et spatial12. À K = 10 sur ces nuages, les certificats de voie morte et les témoins ne s'exercent presque jamais : sur spatial12, `dead_q3` et `dead_q4` sont prouvés 0 fois sur 66, et aucune paire n'est rejetée par témoin.
- Le bloc « tour publiée » passe bien les points dans l'ordre de la fixture puis dans l'ordre inverse. Mais les PointId arbitraires n'atteignent jamais la chaîne.
- **Réfuté** : « le repli K1..5 n'est jugé que par cette audit ». Il existe `experiments/weighted_clustering_20260927/qualify_geometry.py` (Fraction, 28 cas à K ∈ {1, 2, 3, 5} plus un cas K10 à n = 11, reçu `status=passed`, hors CTest), les juges d'échantillon à K5/K10 et Euler à K5/K10 sur 36 nuages.
- **Pour la v10** : un T2 aléatoire multi-K (au moins 200 nuages dégénérés, K = 1..min(10, n), entrée permutée, IDs épars), avec des planchers sur les voies mortes aux petits K.

#### A9-44 — Juges bon marché mesurés ici mais absents de la v9
- Sévérité : moyenne. Statut : confirmé (rejoués par les vérificateurs). Lentilles : L06, L14, L08.
- **K = 1 contre EMST** exact : 0 écart sur 8k serré, 8k large et 39 885 sites LiDAR. Il ne voit que l'ordre 1 : c'est un garde-fou de plomberie, pas un juge de complétude.
- **Clustering à K = 1 contre sklearn HDBSCAN(min_samples = 1)** : identité des étiquettes après correction de la condensation (A9-59).
- **Pour la v10** : ces deux juges entrent dans la CI rapide dès le premier jour.

#### A9-45 — CI v9 rouge sur 40 poussées consécutives ; mutant de pool instable
- Sévérité : haute. Statut : confirmé, avec des chiffres corrigés. Lentilles : L06-F3, L12-02, L15-04. Les vérificateurs l'ont cotée entre moyenne et haute ; elle est retenue haute ici parce qu'elle a masqué les selftests G4 pendant toute la période des revendications de clustering.
- **Constat.**
  - Le workflow `morsehgp3d-v9.yml` a échoué 40 fois de suite, du run 36269945775 (sur `f9f273bb0`, créé le 26 sept. à 20:33Z) au run 36400572483 (sur `ce8a649dd`). Dernier succès : `fce85d823`.
  - Échecs : `mhgp9_lidar_scaling_reader_{normal,optimized}`, `cause=v13_baseline_refused`.
  - Cause exacte : `9df1b4cb2` (« probe v29 ») ajoute `frames` et `host` à `TOP_KEYS` du lecteur G4 (`gcp-migration/tower_worker_v9.py:209-217`), alors que `bench/run_lidar_scaling.py:48` reste épinglé en v28 et fait juger sa fixture par le validateur vivant.
  - L'étape « offline G4 protocol selftests » est sautée dans les 40 runs.
  - Le mutant `mhgp9_tower_task_pool_mutant_join_closed` repose sur une course (comportement indéfini). Il fait échouer environ 12 runs sur 59 depuis `7c4cda986`, et 6 exécutions sur 20 en local (erreur de segmentation).
  - Le même lecteur avait déjà rendu la CI rouge le 23 septembre pour une autre raison (`AUDIT_C_OBJET_ET_RECONSTRUCTION_TOUR_20260923.md:502-507`).
  - Les commits de clustering, `cda636b5e` compris, ont été poussés sur une CI rouge.
- **Pour la v10** : une CI verte comme condition d'intégration ; aucun lecteur de reçu archivé couplé au validateur vivant ; des mutants déterministes seulement.

#### A9-46 — Les juges de l'objet pèsent environ 3 % du temps CI, les différentiels internes 64 à 66 %
- Sévérité : moyenne. Statut : corrigé. Lentille : L06-F5.
- Sur 2 870 s de ctest (run 36398520124) : juges de l'objet au sens large (T2, juges d'échantillon, Euler, mutants tués par un juge, oracles gen) environ 92 s ; noyau différentiel (levier activé contre désactivé, jumeau hôte contre moteur) 1 844 s.
- Un différentiel ne voit pas une boule omise par les deux bras (`audits/b_full_raw_completeness_gap_20260924/README.md`).
- Le build prend 17 min 26 s et ctest 23 min 56 s, avec `--parallel 2`, un choix du workflow.

#### A9-47 — Juges d'échelle : q4 jugé seulement par un outil d'audit ; juges produits non rejoués après le 24 septembre
- Sévérité : moyenne. Statut : corrigé. Lentille : L06-F6.
- Le juge à supports indépendants de C (`audits/c_raw_support_judge_20260925`) couvre q2, q3 et q4 sur les trames brutes (mutant drop-q4 tué). Il n'est pas porté en CTest (R-35 ouvert).
- `src/` est identique de `f9f273bb0` à HEAD, et les épingles brutes sont reproduites sur G4 : le vert du juge brut se transfère donc.
- Les juges q2/q3 produits n'ont aucune trace archivée après `350f82e6` plus patch (24 sept.).
- `chain_absent_keys` s'appuie sur les primitives de la tour et ignore les coquilles de plus de 12 sites.

#### A9-48 — Trois mécanismes de mutants, dont un registre d'exécution presque orphelin ; aucun mutant q2
- Sévérité : moyenne. Statut : corrigé. Lentilles : L06-F7, L12-06, L15-04.
- Les trois mécanismes :
  - macros `#if` (63 macros distinctes dans `src/`, 76 lignes conditionnelles) ;
  - copies de source mutées (`tests/gen/mutants.json`, 35 mutants, dont `admitted_lane_recounted_in_children` désactivé depuis v8 `2629a536` parce que son motif n'est pas unique, alors qu'un motif élargi suffirait) ;
  - registre d'exécution `src/tower/core/mutants.hpp` : 132 noms, 18 sites, un seul activé par une porte.
- Aucun mutant ne vise la voie q2.
- `tower_chain.cpp` est compilé 35 fois en CI (38,5 s par compilation en local).

#### A9-49 — Labels trompeurs et vert par vacuité aux tailles d'intérêt
- Sévérité : moyenne. Statut : non vérifié. Lentille : L06-F9.
- Les labels `scale8000/16000/32000` sont posés sur un diagnostic q4 de 0,01 s (`CMakeLists.txt:1264-1267`). Le label `oracle` couvre 110 tests, dont des comparaisons produit contre produit. Les tests « gpu » de la CI n'exercent que le jumeau hôte.

#### A9-50 — Oracles proliférants ; `reference/` jamais branché
- Sévérité : moyenne. Statut : non vérifié. Lentille : L06-F10.
- Au moins huit oracles coexistent. `reference/morsehgp3d_oracle/oracle.py:320-328` refuse les catalogues non génériques. Γ n'a pas de second juge entre n = 12 et 14.

## 7. Performance mesurée et chemin critique

#### A9-51 — « K5 < 1 s » : chrono interne sur une seule séquence ; le mur du processus est 1,8 à 2 fois plus long
- Sévérité : moyenne. Statut : corrigé. Lentilles : L07-F3, L06-F8, L05-05, L15-11.
- **Mesures R22**, `chain_total`, sur les trames sans sol de la séquence 08 :
  - K5 : 0,926 (00, et 0,940 au second passage), 0,760 (000100) et 0,983 s (000200) ;
  - K10 : 2,27 à 2,99 s ;
  - brut : K5 1,81 à 2,03 s, K10 5,31 à 6,01 s.
- Le mur du processus vaut 1,52 à 1,87 s à K5 et 4,78 à 6,09 s à K10, soit 1,76 à 2,11 fois la chaîne.
- L'écart vient **surtout des condensés de vérification** : 52 à 60 % à K5, 73 à 76 % à K10. Le contexte CUDA et le bassin épinglé comptent pour 20 à 27 % à K5. Un processus froid sans condensés ferait environ 1,12 à 1,36 s à K5.
- Le mode résident a été mesuré, mais seulement en rejouant la **même** trame (premier passage 943 ms, médiane chaude 923 ms ; Nsight 912 ms). Aucune boucle sur des trames distinctes n'est possible avec la sonde (`bench/tower_probe.cpp:221-224,259`). La trame 000200 a un second passage à 0,965 s (`receipts/g4_q3_payload_20260926`).
- Les reçus mentionnent eux-mêmes le mur externe. Seul le titre de la PASSATION (`PASSATION.md:963`) le tait.
- **Pour la v10** : contrat défini sur une boucle résidente de trames distinctes (p50 et p95), plus le mur froid publié sans condensés et le coût de vérification publié à part.

#### A9-52 — Chaîne sérielle au niveau des étages ; le recouvrement parfait ne gagnerait qu'environ 23 %
- Sévérité : moyenne. Statut : corrigé. Lentille : L07-F4.
- **R22 probe_0** : front 98, S2 96, S3 119, S4 81, colle non chronométrée 84, fusion 28, recensement 102 et tour 289 ms. La somme des étages séquentiels donne 915 ms pour une chaîne de 926 ms.
- Déjà recouverts : q2, le recensement précoce, la préparation GPU et les ordres de la tour entre eux.
- Part des noyaux : 22,4 à 22,8 % à K5. Occupation CPU : 39 %.
- Un recouvrement CPU/GPU parfait ne gagnerait au plus que le temps actif de l'appareil, environ 213 ms. Le chemin critique est côté CPU.

#### A9-53 — Travail aval : recensement redondant, mais la géométrie de la tour n'est pas refaite
- Sévérité : moyenne. Statut : corrigé. Lentilles : L07-F5, L02-H1.
- Le recensement tardif (101,7 ms à K5, 499,6 ms à K10) redescend l'index pour chaque clé q3/q4. Le paquet d'intérieurs q3 fait tomber les visites de 100,7 M à 45,7 M, mais le recensement de 102,4 à 82,5 ms seulement, et le gain de bout en bout n'est pas stable (−5,1 ms en médiane, une paire régresse). Le levier reste désactivé.
- **Réfuté** : « la tour recalcule des MEB et des intrus déjà connus ». La phase 0 calcule la cible terminale de chaque facette, une relation globale que le générateur ne produit pas.

#### A9-54 — Aucune mesure G4 aux tailles d'intérêt 8k, 16k et 32k
- Sévérité : moyenne. Statut : corrigé. Lentille : L07-F7.
- Tous les reçus G4 portent sur trois trames de la séquence 08. Les familles synthétiques à 8k/16k/32k n'ont été mesurées qu'en local W4 sous charge (compteurs déterministes disponibles). Le banc du 28 septembre ne dépasse n = 2000 que pour la famille sphérique : 8000 (5 scènes) et 32000 (1 scène, références seulement), jamais 16000.

#### A9-55 — Moteur figé depuis R22 ; prototypes du 27 septembre non portés
- Sévérité : moyenne. Statut : non vérifié. Lentilles : L07-F8, L12-16.
- Aucune tranche n'a modifié le moteur depuis R22. Une quinzaine de prototypes vivent dans `audits/b_*` (26 CMakeLists séparés), mesurés neutres ou négatifs (`docs/FAUSSES_PISTES.md:155-175`). De R19 à R22, la chaîne n'a gagné que −0,25 s en quatre sessions.

#### A9-56 — La taille de la sortie borne tout contrat de temps
- Sévérité : moyenne. Statut : non vérifié. Lentille : L13-F3.
- Le catalogue compte environ 3,7K + 2,2K(K−1) + 1,6·C(K,3) boules par site en uniforme 3D (421,6 à K10 et 32k), et 120 à 138 en LiDAR sans sol à K10. Tenir 100 ms à K10 sur 46k sites demande environ 55 M boules/s (environ 18 ns par boule). La borne Ω(N²) à K fixé est prouvée en v7 (`morsehgp3D_v7/docs/CROISSANCE_ET_BORNE_DE_SORTIE.md`).

#### A9-57 — Reçus de performance partiellement non rejouables
- Sévérité : moyenne. Statut : corrigé. Lentilles : L07-F11, L15-05.
- 44 dossiers de reçus sur 78 citent /tmp, soit 80 chemins distincts, dont aucun n'existe plus. 38 citent `build/`, 80 chemins distincts dont 6 manquent déjà. 15 seulement sont autonomes.
- Les lecteurs Nsight R1/R2 refusent de rejouer au HEAD parce qu'un script qu'ils épinglent a changé (`gcp-migration/tower_worker_v9.py`, modifié par `74fbcc362`).
- Nuance : les entrées du 27 septembre se régénèrent octet pour octet, et les chiffres de tête se recalculent depuis `scores.csv`. Ce qui est perdu, c'est la ré-exécution du calcul.

## 8. Clustering et bancs face à HDBSCAN

### 8.1 Défauts du pipeline `tower_clustering_20260928`

#### A9-58 — La hiérarchie de clustering est le graphe des seules cofaces de Gabriel, pas la forêt FULL
- Sévérité : haute. Statut : confirmé. Lentilles : L01-F1, L04-F1, L06-F1, L08-F2, L10-03, L11-F1, L14-F2, L15-01.
- **Constat.**
  - `measure.read_export` (`measure.py:65-87`) ne lit que `cofaces` et `catalogue`, jamais `report['native']`.
  - `cluster.facet_levels` et `merge_tree` (`cluster.py:67-122`) unissent les facettes de chaque coface de Gabriel à son β.
  - La forêt FULL T_K est pourtant sérialisée dans le même JSON (`native_weighted_export.cpp:127`).
  - C'est exactement la réduction que le registre classe `false_in_general` : l. 40-41 (Prop. 6, Th. 5), l. 98 et l. 129.
  - La convention par défaut `gabriel` (`run_tower.py:101` au HEAD, l. 116 dans le diff non commis) élague en plus les facettes non-Gabriel. La convention `boundary` ne corrige rien : elle reproduit E5 et fragmente elle aussi.
- **Fixtures exactes.**
  - E5 à K = 2 : à 83886/3563, Γ₂ vaut {ABCDE}, alors que le repli sépare encore ABC jusqu'à 24, dans les deux conventions.
  - Témoin à 4 points (0,9,0), (24,9,0), (12,27,0), (12,0,0) à K = 2 : 2 racines définitives en `gabriel`, 1 en `boundary` et en FULL.
  - Témoin à 5 points [[4,11,5],[5,2,5],[8,2,2],[11,3,1],[11,5,1]] : 2 racines contre 1.
  - `fixture_tiny64_k2.json` : la convention `boundary` se trompe à 9 niveaux sur 352 contre un Γ indépendant.
- **Banc, graine 2026092800**, hierarchical, n = 2000, medium, 8 groupes :
  - 18 / 88 / 416 racines à K = 2 / 3 / 5 (`gabriel`), 10 / 18 / 89 (`boundary`), 1 dans FULL ;
  - spherical K = 2 : 6 racines contre 1 ;
  - à K = 3, 5 composantes lourdes (847 / 517 / 255 / 250 / 179 points) ne fusionnent jamais ; à K = 5, 12 composantes dépassent √n points.
- **Portée à K = 2.** Les racines parasites y sont des facettes isolées de masse ≤ 1,03. Ne garder que la racine principale change l'ARI de 0,0003 au plus. L'effet des retards de fusion de type E5 à l'intérieur de la racine principale n'est pas quantifié.
- **Conséquences.**
  - Le titre « from the FULL tower » est faux sur l'objet : la tour est exécutée, mais seul son catalogue est consommé.
  - La conclusion de `ce8a649dd`, « raising K … makes it worse », n'est pas étayée : c'est un artefact, et l'effet réel de K reste inconnu.
  - C'est une **régression** : le 27 septembre, le port pondéré avait diagnostiqué le même défaut (26 composantes contre 1) et l'avait corrigé par les attaches FULL (`experiments/weighted_clustering_20260927/ETAT_COURANT.md:43-55`, `AUDIT_SILENT_ATTACHMENTS.md`).
- **Pour la v10** : le clustering consomme la forêt FULL, avec attaches datées et ancres. Portes obligatoires : E5, témoin à 4 points, témoin à 5 points, nombre de racines égal à FULL, et égalité des partitions et des dates de fusion à des coupes d'échantillon. Le compte de racines seul est nécessaire mais pas suffisant : sur E5, le graphe finit avec une racine, mais à une date fausse.

#### A9-59 — La condensation s'écarte de HDBSCAN quand aucun enfant n'atteint le seuil
- Sévérité : haute. Statut : confirmé. Lentilles : L06-F2, L08-F1, L11-F2, L10-07.
- **Constat.** `cluster.py:210` (`elif child in big or not big: stack.append(child)`) poursuit la descente. Les facettes tombent alors à leur propre λ de naissance au lieu du λ de la scission. C'est contraire à la docstring (l. 134-138), à sklearn (`_tree.pyx:204-217`), à HGP-old (`_cython.pyx:538-557`) et à `weighted_eom.py:248-250` du 27 septembre.
- **Jouet à 8 facettes** : stabilité 7,0 par enfant contre 1,0. Sur un arbre de 16 points, HDBSCAN retient 2 clusters et la v9 en retient 4 : l'EOM devient identique à la sélection `leaf`.
- **Différentiel K = 1** (masses unitaires, 20 nuages × 2 valeurs de mcs, contre sklearn HDBSCAN(min_samples = 1)) : HEAD 31/40, `cda636b5e` 8/40, correctif d'une ligne 40/40. Seule l'EOM est affectée ; la sélection `leaf` est identique.
- **L'effet n'a pas un sens uniforme.** Avec le correctif, shells passe de 0,412 à 1,000 et filaments de 0,713 à 0,763, mais anisotropic passe de 0,948 à 0,854. Spherical hard (graine 0) s'effondre de 0,831 à 0,176 tant que la racine reste sélectionnable.
- Les conclusions tirées de cette EOM sont confondues : « z = 1 est le mauvais exposant sur shells » (`cda636b5e`), « l'échelle log répare shells » (`ce8a649dd`), « leaf domine eom ».

#### A9-60 — Chaque racine devient un cluster, quelle que soit sa masse
- Sévérité : moyenne. Statut : corrigé. Lentilles : L08-F4, L09-F11.
- `cluster.py:176` met chaque racine en file sans tester de seuil. Une racine sans enfant est retenue par l'EOM (l. 252 ; l. 242 au HEAD) comme par `leaf`. Le diff non commis (`allow_single_cluster=False`) laisse passer ces racines sans enfant.
- Effet mesuré : de 0 à +10 groupes par scène à n = 2000, +7 à 8000 sur spherical. L'effet sur l'ARI reste inférieur ou égal à 0,003, mais la colonne `clusters` est gonflée.

#### A9-61 — `merge_tree` : clé périmée sur les plateaux, facettes perdues (défaut latent)
- Sévérité : moyenne. Statut : confirmé. Lentilles : L08-F5, L10-08.
- `merged[union.find(anchor)]` (`cluster.py:99-120`). Contre-exemple : cofaces (0,1,3), (0,2,4) et (0,3,4) au même β = 4, qui fait perdre les facettes (0,4) et (2,4).
- Sur les 6 permutations de l'ordre des cofaces : 1 perte, 3 imbrications, 2 résultats corrects. Sur 1 505 catalogues à plateau unique : 105 échecs contre 0 pour `weighted_model`.
- Sur le banc, aucune perte n'est observée ; un seul nœud est imbriqué, sans effet sur les étiquettes. Aucune porte ne vérifie que les racines couvrent toutes les facettes.

#### A9-62 — Mémoire quadratique en Python ; 16k et 32k hors de portée
- Sévérité : haute. Statut : confirmé. Lentilles : L07-F2, L08-F6, L14-F12.
- Chaque nœud recopie l'ensemble des membres de son sous-arbre (`cluster.py:114-117`), soit Σ profondeur des feuilles. Σ|members| vaut 77 866 565 à 8k (RSS 3,8 Go) et 1 458 939 460 à 32k (environ 65 Go estimés), avec un exposant d'environ 2,07.
- Médiane de 16,5 s par scène à 8k, contre 0,51 s pour HDBSCAN par défaut.
- Le plan du banc ne contient même pas 16000 (`plan.py:17`), et la tour n'a jamais tourné à 16k ni à 32k.
- **Réfuté en partie** : la convention `gabriel` reste faisable à 8k (environ 4 Go). L'infaisabilité de `boundary` à 8k (environ 20 Go) est extrapolée, pas mesurée.
- L'export seul prend déjà 9,4 s en spherical 32k et 366 s en shells 32k.
- **Pour la v10** : clustering en C++, tailles calculées en post-ordre et intervalles DFS, pas de JSON rationnel intermédiaire. Mesurer séparément l'étage clustering et l'étage générateur.

#### A9-63 — Régression par rapport aux modules du 27 septembre
- Sévérité : haute. Statut : corrigé. Lentille : L08-F10.
- `cluster.py` a été réécrit sans porter `weighted_model.py` ni `weighted_eom.py`, qui gèrent correctement les plateaux, la scission, le refus des forêts et la conservation dyadique exacte.
- Correction : la topologie de `weighted_model` n'est pas un oracle valide, puisque c'est elle qui donnait 26 composantes. La chaîne correcte du 27 septembre est `weighted_model` (masses et vote), plus `full_weighted_tree.py` (topologie FULL, racine unique), plus `weighted_eom.py` (condensation).
- **Pour la v10** : tout port est explicite et différentiel contre ces trois modules.

### 8.2 Revendications et adversaire

#### A9-64 — « Beats HDBSCAN's oracle » : chiffres mesurés hors reçu, portée étroite, antérieurs au correctif, ne survivent pas tels quels
- Sévérité : haute. Statut : corrigé. Lentilles : L01-F2, L04-F1, L08-F3, L09-F1, L10-01, L12-01, L14-F1, L15-03.
- **Origine.** Les chiffres de `cda636b5e` (+0,727 / +0,365 / +0,179 contre liaison simple / défaut / oracle, 34-0-0) sont **mesurés**, mais hors reçu. Ils viennent de `tour_smoke.csv` et `cmp_smoke.csv`, écrits à 08:34 dans le /tmp de la session précédente, 3,5 minutes avant le commit.
- **Portée réelle** : 34 couples, soit 17 scènes sphériques × 2 graines, avec n ∈ {500, 2000}. « easy » et « extreme » ne reposent que sur 2 couples chacun. Le corps du commit limitait lui-même sa portée ; c'est le titre qui généralise.
- `receipts/synthetic_bench_20260928/r1/` ne contient que les références HDBSCAN : 0 ligne de la tour, et son README dit « Aucun moteur v9 n'intervient ».
- **Rejeu bit à bit** par trois vérificateurs, avec le binaire `build/v9-weighted-native-20260927-r1/native_weighted_export` (sha256 a53f1c4d…). Écart moyen à l'oracle 1D :

| Code | Périmètre | Écart à l'oracle 1D |
| --- | --- | --- |
| `cda636b5e` | 34 couples | +0,179 (34-0-0) |
| `cda636b5e` | 110 sphériques | +0,187 (106-4) |
| `cda636b5e` | 250 exécutions | +0,024 (183-67) : hierarchical −0,512 (0-20), shells −0,418 (0-20), filaments −0,109 |
| `ce8a649dd` (HEAD) | 34 couples | +0,070 (29-5-0), 5 effondrements à un seul cluster (ARI 0) |
| `ce8a649dd` (HEAD) | 110 sphériques | +0,077 (92-18) |
| `ce8a649dd` (HEAD) | 250 exécutions | −0,039 (154-96) |
| HEAD + condensation conforme + racine exclue | 34 couples | +0,108 (32-0-2) |
| HEAD + condensation conforme + racine exclue | 250 exécutions | +0,051 (195-55), soit environ 0,734 d'ARI moyen ; en moyenne par famille −0,029 ; hierarchical et shells 0-20 |

- La baisse sphérique après correctif vient entièrement de 18 effondrements en un seul groupe, parce que la racine reste sélectionnable. Hors effondrements, on a 0,858 contre 0,857.
- **« Everything else unchanged »** (`ce8a649dd`) est exact sur la tranche déclarée (une graine, n = 2000, g8, medium) et faux sur la campagne complète (moyenne 0,707 → 0,644 sur 250 exécutions). « That is what made hierarchical land on 0,463 » est réfuté : hierarchical reste à 0,463 après correctif.
- **Pour la v10** : aucune revendication sans reçu versionné (CSV, empreintes, plan figé, toutes familles). Publier un erratum pour le titre de `cda636b5e`.

#### A9-65 — L'« oracle » HDBSCAN du banc n'est pas une borne supérieure
- Sévérité : haute. Statut : confirmé. Lentilles : L09-F2, L10-02, L14-F3.
- **Construction.** `baselines.py:21,43-47,55-65` ne parcourt que `min_cluster_size` ∈ {5, …, 100}, avec `min_samples=None`, c'est-à-dire égal à mcs dans sklearn 1.9.1 (`hdbscan.py:797-798`), et l'EOM seule. L'oracle choisit la borne basse de sa grille (mcs = 5) dans 138 scènes sur 255.
- **Réglages fixes, sans étiquette**, sur 250 exécutions :
  - HDBSCAN(ms = 2, mcs = √n) : 0,738 contre 0,683 (194-26-30, +0,054) ;
  - (ms = 3, mcs = √n) : 0,700 (+0,017, non significatif) ;
  - HDBSCAN par défaut avec remplissage des points de bruit par le plus proche voisin : 0,683, égal à l'oracle.
- **Oracle 2D** (min_samples × mcs × eom/leaf) : 0,782 sur 250 exécutions (208-40-2) ; 0,801 contre 0,699 au point n = 2000, g8 (ms = 1 retenu dans 148 cas sur 160).
- **Réserves.** ms = 2 a été choisi a posteriori parmi 14 réglages. Son avance vient surtout de la famille sphérique, qui fait 44 % du plan (+0,142). En moyenne équilibrée par famille, l'écart tombe à +0,005 ; hors sphérique, il s'inverse (0,703 contre 0,717, à cause de hierarchical : 0,595 contre 0,958).
- **Réfuté** : « borne supérieure inatteignable », « la battre est la seule preuve qui vaille » (`baselines.py:3-7`, README du banc l. 90-100, README du reçu l. 41-43), ainsi que le message de `test_bench.py:158`.
- **Pour la v10** : publier comme références obligatoires l'oracle 2D, HDBSCAN aux mêmes réglages que la tour et les variantes avec remplissage, avec des moyennes par exécution et par famille.

#### A9-66 — Réglages non appariés entre la tour et HDBSCAN
- Sévérité : haute. Statut : corrigé. Lentilles : L09-F3, L14-F11, L11-F5.
- **Réglages.** La tour tourne à K = 2, z = 1, masse √n. HDBSCAN « par défaut » lisse avec ms = 20. Le protocole `audits/b_point_hierarchy_k_20260927/FAIRNESS.md` (ms = K, même EOM, racine exclue) a été abandonné le 28.
- **Tour contre (√n, ms = 2)** : −0,031 avant correctif, −0,094 après.
- **Tour contre (√n, ms = 3)** (convention HGP-old, K+1) : +0,007 avant, −0,056 après.
- **Contre `hdbscan_default`**, changer seulement min_samples explique 88 % de la marge d'avant correctif. Pour le titre sphérique, un oracle à ms = 3 ramène l'écart de +0,187 à +0,091 : environ la moitié survit avant correctif.
- **Convention K ↔ min_samples.** La v9 utilise sklearn ms = K (le point compte), cohérent avec d_K(x)/2 ≤ α_K(x) ≤ d_K(x). HGP-old et la thèse utilisent K+1. Les deux conventions diffèrent de 0,038 sur ce banc.
- **Pour la v10** : une table de correspondance déclarée (K ↔ min_samples, masse ↔ mcs, z ↔ λ, sélection), avec les deux bras publiés.

#### A9-67 — ARI avec le bruit prédit compté comme une classe : la couverture est confondue avec la qualité
- Sévérité : haute. Statut : confirmé. Lentilles : L09-F6, L14-F4.
- **Métrique.** `baselines.py:9-11,29-40` : la couverture est publiée mais n'entre pas dans la décision (`compare.py:98-99`), et l'oracle se choisit sur ce même ARI.
- **Effet du remplissage** : remplir le bruit de HDBSCAN par défaut donne +0,156 [+0,134 ; +0,179] (+0,161 sur les 240 exécutions sans bruit).
- **ARI avec les rejets comptés comme singletons** (225 couples, n ≤ 2000, code non commis), contre l'oracle :
  - lambda_eom : +0,039 devient +0,021 ;
  - log_eom : +0,024 devient −0,004 ;
  - leaf : +0,003 devient −0,019.
- **Portée** : à n = 2000 contre les têtes z = 1, la marge de la tour résiste à la correction de couverture. À n = 8000, elle disparaît : tour remplie contre HDBSCAN rempli, −0,021, 2-0-23.
- **Sous 30 % de bruit**, le remplissage détruit l'ARI (0,624 → 0,441). La tour (0,642) égale seulement HDBSCAN non rempli.
- **Réfuté** : « même biais dans le tableau 9.3 de la thèse ». La thèse ne dit pas comment l'ARI traite les non-classés, et le notebook de HGP-old calcule l'ARI hors bruit prédit. C'est affirmé, non démontré.
- **Pour la v10** : une politique de bruit préenregistrée ; ARI sur tous les points, ARI avec rejets en singletons, ARI sur points couverts, AMI, couverture et nombre de groupes.

#### A9-68 — Calibration des niveaux sur l'échec d'un seul réglage
- Sévérité : moyenne. Statut : corrigé. Lentilles : L09-F4, L10-15, L14-F9.
- **Constat.** Les niveaux sont définis par l'ARI de HDBSCAN(mcs = ms = 20) (`bench_datasets.py:26-63`), qui n'est pas le défaut de sklearn (mcs = 5 : 0,589 contre 0,527 ; shells 1,000). La calibration reproduit exactement les graines 11–15, mais ne généralise pas : cellule centrale 0,396 sur les graines du plan contre 0,73 publié, filaments medium 0,599.
- La porte est évaluée sur les graines de calibration elles-mêmes (`test_bench.py:109,125`, tolérance 0,18 sur 3 graines, alors que la doc annonce 0,15 sur 5). Elle est donc tautologique.
- **Réfuté** : « graines de calibration non disjointes » (elles le sont : 11–15 contre 2026092800–04) et « extreme bimodal » (c'est un effondrement systématique ; la bimodalité touche medium et hard).
- Le plafond GMM à modèle connu vaut environ 0,9 à « extreme » sur 5 familles sur 8 seulement.
- `SUMMARY.json` : `centre_by_groups["8"]` = 0,5266 mélange les trois niveaux de bruit ; la scène sans bruit vaut 0,396.

#### A9-69 — README du banc : tables reproductibles mais mal étiquetées
- Sévérité : moyenne. Statut : corrigé. Lentilles : L09-F5, L14-F8.
- **Réfuté** : « table des tailles non reproductible ». La table commitée se retrouve au centième près avec les graines 11 à 13. En revanche, le README non commis annonce « cinq graines » alors que seule la ligne shells en utilise cinq.
- Sa colonne « oracle par scène » juxtapose des moyennes de famille (spherical 0,64) et des valeurs par taille, d'où un oracle apparemment inférieur au défaut, impossible scène par scène.
- L'explication « fragmentation » de la chute de shells est fausse : il s'agit d'une fusion (3 groupes au lieu de 8, sans bruit).

#### A9-70 — Réglages choisis après les scores ; pas de séparation développement/test ; conclusions tirées d'une seule graine
- Sévérité : haute. Statut : corrigé. Lentilles : L09-F7, L09-F8.
- **Réglages.** `allow_single_cluster=False` et la sélection `leaf` (non commis) ont été ajoutés pendant la campagne. Le premier se justifie par la parité avec le défaut sklearn. La docstring « leaf domine l'EOM d'environ six centièmes » (`cluster.py:271`) a été écrite **avant** toute mesure de `leaf`, par inférence depuis le code défectueux ; `tour_v5` la contredit ensuite (eom 0,721 contre leaf 0,694 sur 13 exécutions anisotropes).
- **Mono-graine.** Les chiffres par famille de `ce8a649dd` sont ceux d'une seule graine : échelle log, masse 100 → 1,000 (cette dernière reproduite sur 5 graines).
- **Statistique.** Aucun intervalle de confiance, aucun test, aucune correction multiple. L'unité statistique est la graine (5 au plus) : 5-0 donne au mieux p = 0,0625.
- **Pour la v10** : préenregistrer la configuration primaire, régler sur des graines et des familles de développement, geler, puis exécuter une seule fois sur les graines de test.

#### A9-71 — Convention F des facettes attribuée à tort à la thèse
- Sévérité : moyenne. Statut : corrigé. Lentilles : L01-F5, L10-06, L11-F1, L15-15, L08-F7.
- **Où est l'erreur.** `measure.py:28` et le message de `268ad5a80` disent « F = facettes de Gabriel seules, convention de la thèse et du moteur v9 ». Le registre (l. 151) n'attribue cette convention qu'au moteur, ce qui est faux aussi : l'export déclare `catalog_universe: gabriel_complete_boundary` et « le consommateur construit F = ∂C » (`EXPORT.md:24,39-40`).
- **Ce que dit la thèse.** La Déf. 29 (PDF p. 115) et l'Algorithme 1, étape 3 (PDF p. 126) prennent F = ∂C, comme HGP-old (`hypergraph.py:245-266`). La formule ambiguë du § 9.1 (PDF p. 122, « F_K correspond aux simplexes de Gabriel ») explique probablement la mauvaise lecture.
- m_τ ≤ 1 n'est prouvé que sous ∂C (registre l. 151). En convention `gabriel`, on observe m_τ jusqu'à 1,21.
- **Réfuté** : « seuil √n incohérent ». La somme des masses vaut le nombre de points couverts dans les deux conventions.
- `read_export` omet les facettes faiblement Gabriel à coquille étendue (`measure.py:85`), par exemple les diagonales d'un carré. Aucun impact sur le banc (0 coquille dégénérée sur 8 scènes).
- Les chiffres 0,639 / 1,046 cités par `268ad5a80` ne se reproduisent pas (0,646 / 1,004).

#### A9-72 — Catalogue des cofaces : la prose dit Gabriel, HGP-old calcule Voronoï d'ordre K+1
- Sévérité : moyenne. Statut : corrigé. Lentilles : L11-F4, L11-F3.
- HGP-old et HGP-Clusterer3D pondèrent les (K+1)-parties dont la cellule de Voronoï d'ordre K+1 est non vide (`_geometry_binding.cpp:221-405`). Cet ensemble est strictement plus grand que Gabriel en position générale seulement.
- Écart mesuré : à n = 400, K = 2, 66 % des ensembles Voronoï ne sont pas de Gabriel, et S_vor/S_gab va de 1 à 14,6. Le registre l. 153 le classe déjà `false_in_general` pour les poids.
- Sonde n = 12, 8 tirages : aucun des deux catalogues n'est faux sur les ensembles de points des composantes non triviales. Tous les désaccords sont des **retards d'attache** de facettes non-Gabriel (Gabriel : 5 instances sur 8 ; Voronoï : 7 sur 8), sans scission.
- **Pour la v10** : nommer le catalogue et la sémantique d'attache (première coface, ou naissance ρ(τ) dans la composante FULL). La porte « reproduit HGP-old » et la porte d'exactitude sont deux portes distinctes.

#### A9-73 — HGP-old : `min_samples > K+1` corrompt silencieusement la voie d'ordre K
- Sévérité : moyenne. Statut : confirmé. Lentille : L11-F5.
- La voie reçoit des simplexes de largeur `min_samples` mais les lit avec un pas de K+1 : la vérification de forme est un simple `pass` (`_cython.pyx:774-777`), et le repli Python tronque aux K+1 premiers sommets (`hypergraph.py:255-256`).
- Simulation (K = 2, largeur 4) : 46 faces émises, dont 17 doublons, soit 29 faces distinctes au lieu de 34.
- L'exemple du README de HGP-old (`min_samples=5, K=2`) déclenche le défaut. Sur les voies gudhi, `min_samples` devient un rayon cœur.
- Portée : HGP-old seulement, qui n'est pas importé par la v9. Les portes « reproduit HGP-old » doivent tourner avec `min_samples = K+1`.

#### A9-74 — La famille hierarchical impose une convention de niveau fondée sur la masse
- Sévérité : moyenne. Statut : corrigé. Lentille : L14-F6.
- **Géométrie.** Les sous-amas sont séparés d'au moins 6,3 σ_interne, contre 5,2 σ pour les groupes sphériques. Aucune règle fondée seulement sur le contraste ne peut fusionner les premiers tout en séparant les seconds.
- **Réfuté** : « aucune règle de densité cohérente ne le peut ». La vérité repose sur la masse (sous-amas de 83 points, groupe de 250). Un seuil de masse entre les deux fait les deux : HDBSCAN(mcs = 75, ms = 5) donne spherical 0,80 et hierarchical 0,79.
- Le générateur n'émet pas les sous-labels (`bench_datasets.py:249-250`). La docstring affirme un écart-type de groupe égal à 1, alors qu'il vaut 1,24 à 1,26.
- **Pour la v10** : émettre les sous-labels et évaluer sur deux niveaux.

#### A9-75 — La meilleure tête simple mesurée n'utilise pas la tour ; son gain vient surtout du remplissage
- Sévérité : moyenne. Statut : corrigé. Lentille : L14-F5.
- **Tête mesurée** : EOM sur l'arbre mreach de sklearn, λ = r^(−ẑ), ms = 2 ou 3, mcs = √n, remplissage. On obtient 0,831 à 0,837, et 0,839 avec un remplissage borné ρ = 2, sur 250 exécutions.
- **Décomposition** :
  - le remplissage seul apporte environ +0,156 ;
  - z = 3 contre z = 1 ajoute +0,039 à +0,045 (192 égalités sur 250) ;
  - ẑ, estimé par maximum de vraisemblance, vaut environ 3 sur 7 familles sur 8, filaments comprises (2,1 sur shells), donc ẑ ne se distingue pas de z = 3 fixe.
- **Face à l'oracle 2D avec remplissage** (0,852, borne basse) : −0,015 [−0,037 ; +0,008].
- **Réserves.** Tous les réglages ont été choisis sur ces mêmes 250 exécutions. Aucune mesure au-delà de 8k. Sous 30 % de bruit, le remplissage fait tomber l'ARI à 0,44.
- **Entrelacement.** π0 de la tranche L_K au rayon r et le graphe mreach sont entrelacés à un facteur 2 **sur le rayon**, soit un facteur 4 sur a = r². C'est un argument, pas un théorème inscrit. Il borne l'écart entre arbres, pas l'ARI d'une sélection EOM.
- **Pour la v10** : l'ablation « même tête sur la tour contre sur la bifiltration k-NN » est la seule façon de justifier la tour par la qualité de clustering.

#### A9-76 — La campagne du 27 septembre tire son gain de la première couverture, pas du § 9.1
- Sévérité : haute. Statut : corrigé. Lentille : L10-11.
- Sur 34 scènes (K5, m20, z1) :
  - routage 0,5574, première couverture 0,5517, vote pondéré 0,4511, HDBSCAN commun 0,4198 ;
  - la première couverture capte 95,9 % de l'écart (94 à 100 % selon le profil) ;
  - médiane ΔARI +0,0074 ;
  - le F1 du vote pondéré passe sous HDBSCAN dans les quatre profils.
- Le seuil du vote est une masse de facettes, pas une cardinalité de points : la comparaison n'est pas au même paramètre.
- **Réfuté** : « s'inverse par graine » pour l'ARI (les deux graines sont positives). L'inversion ne touche que le F1, au profil m20/z1.
- La suspension des conclusions n'est écrite que dans `experiments/README.md:17-20`. `README.md:9-10`, `PASSATION.md:14` et `audits/ETAT_COURANT.md:8` affichent toujours 0,5574 contre 0,4198 sans réserve.

#### A9-77 — Pas de mesure de clustering aux tailles d'intérêt ; les refus ne sont pas pris en compte dans les appariements
- Sévérité : basse. Statut : corrigé. Lentilles : L14-F13, L15-12, L09-F8.
- La tour n'a été mesurée qu'à 500, 2000 et 8000 (sphérique). `run_tower.py:126-130,136` imprime `REFUS` et compte les refus dans le JSON final : ce n'est donc pas silencieux. En revanche, ils manquent au CSV, et `compare.py:96` saute les couples manquants. Aucun refus n'a eu lieu dans les rejeux.

#### A9-78 — Critique de la thèse comme source du clustering
- Sévérité : basse. Statut : non vérifié. Lentilles : L11, L14.
- Aucun théorème ne lie m_τ ni l'EOM pondérée à une masse de probabilité : la Prop. 7 garantit seulement une partition.
- ψ = 1/t^p avec p ambiant (PDF p. 122) est incohérent pour des données de dimension intrinsèque d < p. C'est la raison d'être de la directive utilisateur « z = dimension intrinsèque ».
- Le tableau 9.3 repose sur une exécution unique, sans min_samples de HDBSCAN déclaré ; birch2 y donne 0,441 pour HGP contre 0,996 pour HDBSCAN, réparé a posteriori par z = 2.
- Divergence à déclarer dans la spécification v10, conformément à la directive « la thèse est une source critiquable ».

## 9. Registre des audits ouverts

### 9.1 Entrées ouvertes du suivi v9 (`audits/README.md`, R-01 à R-39)

| Entrée | Objet | État au 28 septembre | Effet pour la v10 |
| --- | --- | --- | --- |
| R-03 | T2 de chaîne à K5 | Ouvert : T2 seulement à Kmax = 10 (A9-43) | T2 multi-K dès le départ |
| R-04 | Extension non régulière au registre | Ouvert : aucune ligne au registre (A9-03) | Inscrire avant usage |
| R-29 | Catalogue scellé | Livré, avec un résidu déclaré (A9-08) | Validité portée par les types |
| R-30 | 100 ms | Jugé infaisable avec les algorithmes connus (auditeur C) ; horizon de refonte 0,25–0,5 s à K5, 0,7–1,4 s à K10 | Cibles étagées, en boules/s |
| R-35 | Juge brut à supports indépendants | Outil d'audit, pas porté en CTest (A9-47) | Porter en porte nocturne |
| R-36 à R-39 | Entrées de l'auditeur C du 26 sept. | Dernières entrées indexées | — |

### 9.2 Traçabilité et contradictions non résolues

#### A9-79 — Audit du 28 septembre hors dépôt ; état, passation et index non tenus
- Sévérité : moyenne. Statut : non vérifié (partiellement confirmé par le vérificateur de L10-11). Lentille : L10-10.
- L'audit géant du 28 septembre (39 agents) est cité par cinq commits (`f4d74af7e`, `22cb1895b`, `74fbcc362`, `9622876e8`, `061d1ac9c`) mais n'existe que dans le scratchpad de la session précédente.
- `ETAT_COURANT.md`, `PASSATION.md`, le README v9 et `COORDINATION_MORSEHGP3D_V9.md` ne mentionnent jamais le 28 septembre. L'index s'arrête au 26.
- Ne sont indexés ni `RELECTURE_THESE_ET_HGP_OLD_20260927`, ni `LECTURE_HGP_OLD_CLUSTERER3D_20260927`, ni `CATALOGUE_ET_POIDS_NON_GABRIEL_20260927`, ni les `b_*_20260927`.
- Question laissée ouverte : le correctif `74fbcc362` de la règle d'épingles casse-t-il 22 des 31 selftests, comme l'avait annoncé l'audit du 28 ? Non revérifié.

#### A9-80 — Nuages dérivés de KITTI versionnés dans la v9, contre l'engagement d'ouverture
- Sévérité : moyenne. Statut : confirmé. Lentille : L15-06.
- **Constat.**
  - `audits/s4a_cpu_scene02_physical_panel_20260924/inputs/` (commit `cdc5dc4b3`) contient 84 fichiers, dont 21 nuages de 08/000200 sans sol à 1 mm (2 888 208 octets de coordonnées ; `full_full.u32le` = 45 845 × 12).
  - `audits/s4a_ground_hot_quarter_20260923/inputs/` (`95d35a392`) contient 3 nuages identiques à des nuages du panneau.
  - La règle est `AGENTS.md:46` : « Aucun octet de données KITTI dans la v9 ». `PASSATION.md:1049` affirme encore le contraire.
  - Le dépôt GitHub est public, sous licence MIT. KITTI est distribué sous CC BY-NC-SA (connaissance externe, version non revérifiée).
  - Le contrôleur CI promis par `PLAN_V9.md:59-61` n'a jamais été écrit.
- **Portée.** L'apport propre à la v9 est marginal face à la v8 : 114 Mo de dérivés et trois scans bruts `RAW.bin` dans `morsehgp3D_v8/receipts/.../snapshot.tar.gz`.
- **Pour la v10** : `data/` ignoré par Git, fetcher à sha256 attendus, et un contrôle CI qui inspecte aussi les membres d'archives. Décision de réécriture d'historique ou d'exception de licence : à l'utilisateur.

#### A9-81 — Lecteurs de reçus G4 trop permissifs
- Sévérité : basse. Statut : non vérifié. Lentille : L10-17.
- Le lecteur v28 accepte `declared_support_checks=0`, `reason="fabricated"` et `early_census_keys=0` (`audits/AUDIT_REPRISE_20260926_FULL.md`).

### 9.3 Affirmations réfutées (dans la v9 ou dans les lentilles)

| Affirmation | Où | Verdict |
| --- | --- | --- |
| « hierarchical clustering from the FULL tower » | titre de `cda636b5e` | Faux sur l'objet : graphe des cofaces de Gabriel (A9-58) |
| « it beats HDBSCAN's oracle » (+0,179, 34-0-0) | titre de `cda636b5e` | Portée de 34 couples ; ne survit pas tel quel au correctif ; oracle restreint (A9-64, A9-65) |
| « borne supérieure inatteignable » | `baselines.py:3-7`, READMEs du banc et du reçu | Faux : réglages fixes et oracle 2D la dépassent (A9-65) |
| « strict improvement, everything else unchanged » | `ce8a649dd` | Vrai sur une graine, faux sur la campagne (A9-64) |
| « raising K makes it worse » | `ce8a649dd` | Non étayé : artefact de la topologie de Gabriel (A9-58) |
| « F = Gabriel, convention de la thèse » | `measure.py:28`, `268ad5a80` | Faux : Déf. 29 et Alg. 1 prennent ∂C (A9-71) |
| « leaf domine l'EOM d'environ six centièmes » | `cluster.py:271` (non commis) | Écrit avant mesure, contredit ensuite (A9-70) |
| « la v9 ne versionne aucun octet KITTI » | `PASSATION.md:1049` | Faux (A9-80) |
| « intérieur ≤ 9, coquille ≤ 12, bornées par théorème » | `morsehgp3D_v5/README.md:34-36` | Faux ; la v9 en fait un refus de domaine (A9-05) |
| « Euler + Kmax+2 réfuté par 13 points » (lentilles) | L01-F3, L10-04 | Vrai sans la tour ; la tour refuse la fixture 8 fois sur 8 (A9-02) |
| « K5 < 1 s inatteignable par cette architecture » (lentille) | L13-F1 | Faux : R22 donne 0,76 à 0,98 s de chaîne interne (A9-51) |
| « le Python de protocole dépasse le C++ produit » (lentille) | L05-04 | Faux : environ 18 300 lignes contre 33 489 (A9-39) |
| « la tour recalcule une géométrie déjà connue » (lentille) | L07-F5 | Faux : la cible de facette est propre à la tour (A9-53) |
| « q3_interior_payload est un levier négatif » (lentille) | L12-03 | Faux : plus rapide dans 7 cas sur 8, gain non stable (A9-83) |
| « toute comparaison de niveaux passe par U320 » (lentille) | L07-F6 | Faux : rangs u32 déjà internes (A9-28) |
| « le ratio q2 candidates/acceptées croît avec n » (lentille) | L02-H3 | Faux : effet des coupes en disque (A9-13) |
| « 39 échecs CI consécutifs » (lentille) | L06-F3 | 40 (A9-45) |
| « FullDatedContribution fait 72 o » (lentille) | L04-F3 | 80 o (A9-28) |
| « seuil √n incohérent en convention gabriel » (lentille) | L01-F5 | Faux : la masse totale vaut le nombre de points couverts dans les deux conventions (A9-71) |

## 10. Architecture et dette

#### A9-82 — Trois portages recousus par une chaîne de 860 lignes
- Sévérité : moyenne. Statut : corrigé. Lentilles : L12 (synthèse), L12-05.
- `src/gen` est un port v8 identique à 84,5 %. `src/tower` est un port v7 identique à 53 %. `src/gpu` est une réimplémentation jumelle des voies q3/q4.
- `run_tower_chain` fait 860 lignes (`tower_chain.cpp:1316-2175`). Trois index spatiaux coexistent, dont deux construits (A9-12), ainsi que quatre types de clé de boule : `Q2BallKey`, `gen::ExactBall`, `tower::BallKey` et `Key5`.

#### A9-83 — 38 options de chaîne, dont 31 booléens ; aucun préréglage « configuration mesurée »
- Sévérité : moyenne. Statut : corrigé. Lentilles : L12-03, L07-F10, L15-08.
- **Constat.**
  - `ChainOptions` (`tower_chain.hpp:51-216`) compte 38 champs, dont 31 booléens. Les dépendances passent par 16 refus écrits à la main, plus d'autres dans `wspd_q34.cpp` (24 exceptions levées), et certaines sont inertes sans être refusées.
  - Le meilleur bras de R22 diffère des défauts par 12 leviers. La plupart ne sont qu'une seule décision (le backend GPU) découpée en drapeaux interdépendants.
  - Le vrai défaut de la bibliothèque (workers = 1, sans sceau) n'a jamais été chronométré.
  - Un seul levier est mesuré négatif : `q34_lanes_fused` (étape T +4 % à K5).
- Cela contrevient au principe 5 de `PLAN_V9.md:25-27` et répète un défaut v8 (`AUDIT_V8_SYNTHESE.md:223-224`).
- **Pour la v10** : paramètres limités à kmax, s (≥ 8), fils, backend et niveau de vérification. Un levier n'entre que par une ablation appariée et devient alors le défaut.

#### A9-84 — Chemin de la tour choisi implicitement selon le nombre de fils
- Sévérité : moyenne. Statut : corrigé. Lentille : L12-04.
- `tower_chain.cpp:2067-2068` donne la voie temporelle à W = 1 et la voie statique à W > 1. La sortie est appariée entre voies à n ≈ 1 000–1 500 et sur la trame 000000 de W = 1 à 8 ; aucune porte n'apparie la voie temporelle à 8k/16k/32k.
- Le travail publié (`tower_work`) diffère selon la voie : un reçu à W = 1 n'est pas comparable à un reçu à W > 1.

#### A9-85 — Monolithes et coût de build
- Sévérité : moyenne. Statut : corrigé. Lentilles : L12-06, L15-08.
- Fichiers : `full_ball_tower.hpp` 3 097 lignes (classe Builder environ 2 475, en en-tête seul, incluse via `tower_chain.hpp:38`), `q2_census.cpp` 2 858, `tower_chain.cpp` 2 177, `wspd_q34.cpp` 1 900, `filter_runner.cu` 1 740.
- CI : 231 unités de compilation, 169 exécutables, 18 min 28 s de build.

#### A9-86 — Modèle d'erreur : une faute interne peut sortir comme « entrée invalide »
- Sévérité : moyenne. Statut : non vérifié. Lentille : L12-12.
- Dix énumérations de statut coexistent. Le générateur lève 413 exceptions standard, et tout `std::invalid_argument` devient `kInvalidInput` (`tower_chain.cpp:2119-2139`).

#### A9-87 — Pas d'API publique ; instrumentation exposée
- Sévérité : moyenne. Statut : non vérifié. Lentille : L12-13.
- Les consommateurs incluent directement `src/chain/tower_chain.hpp`. `ChainResult` expose 129 compteurs, 57 chronos par lots et 72 statistiques de tour, soit 454 feuilles JSON par sonde.

#### A9-88 — Documentation-journal et surproduction d'audits
- Sévérité : moyenne. Statut : non vérifié. Lentilles : L12-15, L15-09, L01-F9.
- 72,6 k lignes de Markdown pour 33,5 k lignes de code. README de 277 lignes, PASSATION de 1 066, `audits/ETAT_COURANT.md` de 2 743 (censé être le « verdict mutable unique »). 349 entrées d'audit, 628 commits v9 en six jours.
- Les reçus et audits représentent 80 Mo sur 86,3 Mo suivis.

#### A9-89 — Le développement algorithmique récent vit dans `audits/`
- Sévérité : moyenne. Statut : non vérifié (couplage `native_a.hpp` confirmé par le vérificateur de L04-F5). Lentille : L12-16.
- 26 CMakeLists séparés sous `audits/b_*`, environ 81 600 lignes de code sous `audits/`, aucun prototype intégré. Un worktree /tmp a été perdu (`REPRISE_DEV_20260927.md:79-86`).

#### A9-90 — Build : Boost imposé au produit, CUDA figé sur sm_120 et C++17
- Sévérité : basse. Statut : non vérifié. Lentille : L12-18.
- `CMakeLists.txt:33-45,866-871`. Boost n'est utilisé que par les juges.

## 11. Leçons de l'héritage

#### A9-91 — L'erreur E5 revient pour la cinquième fois ; aucune porte transversale
- Sévérité : haute. Statut : corrigé. Lentille : L15-02.
- **Historique corrigé** :
  - v3 : origine et fermeture de la piste par E5 (`morsehgp3D_v3/audits/PISTES_FERMEES.md:53`) ;
  - v4/v5 : fold, porté sous une portée déclarée (`morsehgp3D_v5/README.md:7-20`) ;
  - audit de reprise v8 : recommandation interceptée par le registre (`AUDIT_V8_SYNTHESE.md:188-193`) ;
  - v9, 27 septembre : détectée (26 racines contre 1), corrigée, et E5 branché sur ce consommateur en Python hors CTest (`qualify_full_attachments.py:214`) ;
  - v9, 28 septembre : rechute non détectée, E5 absent des tests, en environ 15 heures.
- La seule porte `experiments/` enregistrée en CTest (`CMakeLists.txt:1164`) est le contre-exemple Gabriel/ordre-Voronoï, pas E5.
- **Pour la v10** : une bibliothèque de fixtures « objet » imposée par CMake à tout producteur ou lecteur de hiérarchie (E5, A-E, deux fixtures à 4 points, coquille à 7 points, carré K2, triangle rectangle, K = 1, K = n), avec un mutant « Gabriel seul » qui doit être tué.

#### A9-92 — Méthode : chronos de composant présentés comme contrats, chiffres sans reçu, portes vacantes
- Sévérité : moyenne. Statut : non vérifié (cas particuliers confirmés ailleurs). Lentilles : L15-11, L15-14, L15-03.
- Précédents : v7 (189 ms de noyaux pour une tour de 419 s) ; v9 S1 (63,8 ms de filtre sans le front) ; `chain_total` en v9 (A9-51) ; règle d'épingle de digest qui ne pouvait rien refuser (`74fbcc362`) ; portes vacantes de la v3 (`WILL_FAIL`, regex).

#### A9-93 — Acquis techniques v7/v8 solides à porter tels quels
- Sévérité : info. Statut : non vérifié (lecture ; en partie jugé par les portes v9). Lentille : L15.
- Prédicat témoin entier (`src/gen/spindle/predicates.hpp:130-160`). Domaine u18 et bornes vérifiées (`audits/check_u18_bounds_20260922.py`). Seuils h_q = Kmax+2−q (`proved_here`). Clé primitive commune. Objet FULL v7. Juge T2. Théorèmes v8 (H, Pool, frère, familial). `run_expect.cmake`. Recette Boost hors conteneur.

#### A9-94 — Pièges pratiques documentés à ne pas refaire
- Sévérité : info. Statut : non vérifié. Lentille : L15.
- Chaîne d'outils de la VM G4 : GCC 11.4, CMake 3.22.1, sm_120. /tmp est effacé au redémarrage. Index Git partagé : exiger `git diff --cached --quiet` avant `git add`. Ne pas lancer `ctest -N` dans des builds épinglés. `pgrep -f` se reconnaît lui-même. Environ 70 garde-fous v8 n'ont pas été recopiés (`docs/audit_v8/15_entrees_v8_non_lues.md` § 4).

## 12. Ce que la v10 garde, jette et change

| Élément v9 | Décision | Raison | Réf. |
| --- | --- | --- | --- |
| Objet π0(L_K(a)), K ≤ 10, niveaux rationnels exacts, verticales à la coupe fermée | Garder | Défini, exact relativement au catalogue | A9-01 |
| Arithmétique entière u18, clé primitive, filtres flottants certifiés avec repli exact | Garder | Aucun défaut trouvé ; bornes vérifiées | A9-19, A9-93 |
| Mécanisme d'ancres (K, BallKey), lots atomiques, descente lexicographique, quotient de coquille | Garder, inscrire au registre | Correct, mais extension non régulière non registrée | A9-03, A9-26 |
| Oracle Γ (T2), fixtures E5, A-E, 4/5 points, carré, 13 points, coquille à 7 points | Garder, étendre en T2 aléatoire multi-K | Seul vrai juge de l'objet | A9-43 |
| Invariant d'Euler par le nerf | Garder comme alarme nécessaire | Aveugle aux deux ordres supérieurs | A9-02 |
| Juges d'échantillon q2/q3, juge brut q2/q3/q4, juge des clés absentes | Garder et porter en porte nocturne | Seuls juges de la zone aveugle | A9-47 |
| Juge K = 1 contre EMST ; clustering K = 1 contre HDBSCAN(ms = 1) | Ajouter | Bon marché, attrape de vrais défauts | A9-44 |
| Front WSPD de paires et cascade témoin/noyau/cover comme énumérateur q3/q4 | Changer | Quadratique sur amas, cubique sur coquilles | A9-20, A9-21 |
| Générateur par boîtes de centres à gardes et dominateurs | Candidat, à qualifier | Comptes identiques à la v9 sur 14 entrées, 4 à 50 fois moins de CPU ; pas de borne de pire cas (108 points cocirculaires : plus de 300 s) | A9-95 |
| Double implémentation q3/q4 (moteur à atlas et voies jumelles) | Jeter l'une des deux | Coût de maintenance, compteurs contraints | A9-24 |
| Recensement redondant (collecte par paire, re-census par clé) | Changer : une collecte par clé distincte, identifiants transmis | Deux passes de trop | A9-11 |
| Deux index spatiaux construits, index gen séquentiel en size_t | Changer : un index u32 parallèle partagé | 12 à 37 ms sur le chemin critique | A9-12 |
| Niveau exact de 48 o recopié partout, BallData de 224 o | Changer : rangs u32, table unique, CSR | Environ 810 o par boule | A9-28 |
| Phase A, C et encodage séquentiels par ordre ; phase 0 sérialisée entre K | Changer : parallélisme intra-ordre, sortie directe | 215 ms sériels à K5 | A9-27 |
| Voie temporelle v7 choisie à W = 1, API de lot morte, options sœurs | Jeter du produit, garder en juge | Un seul algorithme | A9-30, A9-84 |
| Sceau R-29 échantillonné au 1/64 | Jeter | Résidu hors bornes | A9-08 |
| Ancres détruites | Changer : publier `anchor[K][bloc]` | Interface de clustering exacte | A9-35 |
| GPU pour q3/q4 seulement, appel unique avec refus global | Reporter après le CPU propre ; lots bornés avec repli | 77 % d'inactivité, gain non attribué | A9-36 à A9-38 |
| Prédicats `MHGP9_HD` à source unique, portes de port | Garder le principe | Exactitude hôte/appareil prouvée | A9-37 |
| Clustering sur le graphe des cofaces de Gabriel (`cluster.py`) | Jeter | E5, régression | A9-58 |
| Condensation `cluster.condense`, `merge_tree` avec ensembles de membres | Jeter ; porter la sémantique de `weighted_eom` en C++ | Non conforme HDBSCAN, mémoire quadratique | A9-59, A9-62 |
| Masses du § 9.1 (S_τ, T_x, w, m_τ), invariants exacts | Garder, sous F = ∂C déclaré | Définition de la thèse, m_τ ≤ 1 | A9-71 |
| Convention « gabriel » par défaut | Jeter (au plus en ablation nommée) | Mal attribuée, casse la connexité | A9-71 |
| Oracle HDBSCAN 1D, ARI bruit = classe comme critère unique | Jeter | Pas une borne ; confond couverture et qualité | A9-65, A9-67 |
| Banc à 8 familles, générateur à empreinte, `compare.py` apparié | Garder et corriger | Oracle 2D, HDBSCAN apparié, métriques multiples, sous-labels, 8k/16k/32k | A9-65 à A9-70 |
| Leviers booléens permanents, défauts d'API ≠ configuration mesurée | Jeter | Principe 5 non tenu | A9-83 |
| Mutants par recompilation et par substitution de texte | Changer : un registre compilé, déterministe | 35 compilations, mutants instables | A9-48 |
| Reçus sous /tmp ou dépendants de `build/` | Jeter | 44 dossiers sur 78 irrejouables | A9-57 |
| Documentation-journal | Changer : README stable, ÉTAT court réécrit, registre court | Décisions perdues, E5 récurrent | A9-88, A9-91 |
| Données KITTI dans Git | Jeter : fetcher à empreinte, contrôle CI | Licence et engagement | A9-80 |
| Protocole G4 par expérience, schéma de sonde v1 à v30 | Changer : exécuteur générique, schéma additif, binaire capturé | Sept tentatives sans mesure | A9-39 |
| Scripts `start_and_verify.sh` / `stop_and_verify.sh` | Garder | Toutes les sessions certifiées TERMINATED | A9-39 |

#### A9-95 — Générateur par boîtes de centres : candidat fort, non qualifié
- Sévérité : haute (conséquence de conception). Statut : corrigé. Lentilles : L13-F2, L13-F1.
- **Lemme.** Soit Q une boîte de centres et S0 un ensemble de K sites. Pour toute boule admissible de centre dans Q, au moins un garde de S0 n'est pas strictement plus proche que chaque site de la boule. Et si K sites dominent un site x sur toute la boîte, x ne peut appartenir à aucune boule admissible.
- **Mesures (prototype `cble.cpp`).**
  - Comptes par q_min et coquilles étendues identiques à la v9 sur 14 entrées, plus la trame brute b02 (3 071 514 boules).
  - Identité par (q, p) avec un oracle Fraction indépendant sur 45 cas, u18 compris.
  - Amas : 1,90 / 4,11 / 8,68 s, contre 21,9 / 83,4 / 306,7 s pour la v9 (4 fils côté v9, 2 côté prototype).
  - Plans : 1,10 / 2,03 s contre 54,7 / 239,8 s.
  - LiDAR K10 : 97 CPU·s, contre 366,8 pour le moteur CPU et 70,3 pour le bras GPU.
- **Corrections.**
  - Seuls les comptes sont établis : il n'existe ni enregistrement ni condensé boule par boule.
  - L'exactitude du compte p demande un second lemme par récurrence, démontré par le vérificateur mais non écrit.
  - Le coût n'est pas garanti : 108 points entiers cocirculaires plus 2 000 points de bruit dépassent 300 s sans résultat, alors que la v9 refuse explicitement.
  - Les préfiltres flottants ne sont pas certifiés.
  - Les doublons ne sont pas gérés.
  - Le gain n'est que de ×2 à ×3 en temps mural sur uniforme et terrain.
- **Pour la v10** : la v10 ouverte en parallèle contient un générateur par boîtes (`a0d92fd6e`). La contre-fixture cocirculaire et l'identité boule par boule contre la v9 doivent y être rejouées avant tout statut.

## 13. Questions ouvertes

1. **Objet du clustering** (décision de l'utilisateur) : la v10 condense-t-elle la forêt FULL T_K à K fixé (attaches silencieuses comprises), ou un arbre « Algorithme 1 » assumé comme une autre méthode et nommé comme tel ? Sur le banc, la topologie de Gabriel ne change presque rien à K = 2, mais tout à partir de K = 3.
2. **Univers de poids du § 9.1** : ∂C des cofaces de Gabriel (Alg. 1, m_τ ≤ 1) ou catalogue d'ordre-Voronoï K+1 (HGP-old, HGP-Clusterer3D) ? Et sémantique d'attache : première coface, ou naissance ρ(τ) dans FULL ?
3. **La géométrie exacte apporte-t-elle quelque chose au clustering ?** Même tête (EOM à λ = r^(−z), sélection, politique de bruit) sur la tour et sur la bifiltration k-NN, aux mêmes K : si l'intervalle de confiance de ΔARI contient 0, la tour ne se justifie pas par le clustering.
4. **Convention K ↔ min_samples** pour l'équité : sklearn ms = K (le point compte, cohérent avec l'encadrement de α_K) ou K+1 (HGP-old, thèse) ? Publier les deux bras ?
5. **Politique de bruit** : abstention, remplissage borné ou complet ? Elle déplace l'ARI de ±0,15 à 0,2 et doit être préenregistrée.
6. **z** : fixé à 1 pour l'équité, estimé (ẑ ≈ 3 à l'échelle k-NN sur ce banc, 2 sur les coquilles), ou choisi par branche ? La directive « z = dimension intrinsèque » reste à qualifier avec une condensation correcte.
7. **Sélection multi-K** par les applications verticales (tranches γ de Rolle–Scoccola, consensus vertical) : les verticales FULL sont-elles publiées sous une forme consommable ?
8. **Multiplicités** : sites pondérés dans I, U et les rangs, ou refus documenté dans le contrat du banc ?
9. **Complétude à l'échelle** des fusions seules à Kmax : un argument dédié existe-t-il, ou faut-il un juge stratifié nocturne ? Kmax+2 est hors domaine à K10.
10. **Borne de pire cas** du générateur par centres sur des entrées fortement cosphériques (grilles u18, famille Ω(N²) de la v7, trames brutes) : faut-il un refus explicite quand m > M au côté 1 ?
11. **Contrat de temps** : mur de processus froid ou boucle résidente de trames distinctes ? Session d'appareil comprise ou amortie ? Cibles étagées : 250 à 400 ms à K5 sur G4 résident ; au plus 1 s à 8k et 5 à 10 s à 32k sur CPU 8 fils pour la chaîne tour + clustering.
12. **Tranche K effectivement utile** au banc (K = 2 ou 3 d'après les mesures) : si K ≤ 3 suffit, q4 disparaît presque et l'effort LiDAR K5/K10 sort de l'objectif « battre HDBSCAN ».
13. **Données KITTI** : réécriture d'historique (v8 et v9) ou exception de licence documentée ? Décision de l'utilisateur.
14. **Hiérarchie à deux niveaux** : quelle vérité et quelle métrique pour `hierarchical` (niveau grossier imposé, ARI contre chaque niveau, meilleur niveau de l'arbre) ?
15. **Commits d'agents orphelins** (`38f420085`, `6d38ed95a`, `e181dec27`) : à relire avant toute GC.
