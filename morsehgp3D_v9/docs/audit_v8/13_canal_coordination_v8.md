# Lentille 13 — Canal de coordination v8 : questions, engagements et constats non clos

22 septembre 2026. Cadre : `phase=exploration_v9_hors_registre`,
`backend=none`, `mode=audit_v8_et_v7_avant_v9` (lentille complémentaire),
`public_status=not_claimed`. GCP non utilisé. Lecture seule : aucune commande
Git mutante, aucune compilation, aucun script du dépôt. Seuls `git log`,
`git show`, `git diff` (avec `GIT_OPTIONAL_LOCKS=0`), `grep`, `sed`, `ls` et
`wc` ont servi.

Source de vérité : worktree détaché `origin/main` **12294241**. Ouverture v9
confrontée : commit **3595725a**.

Conventions :

- `C:n` désigne `audits/COORDINATION_MORSEHGP3D_V8.md`, ligne n, à 12294241.
- **[hors HEAD]** désigne des lignes présentes seulement dans le worktree
  partagé `/workspaces/E-HGP` (diff non commis).
- Statuts : **prouvé** (preuve écrite, parfois avec fixture), **testé** (porte
  ou juge borné), **mesuré** (reçu nommé et présent à 12294241), **proposé**,
  **manquant**, **non vérifiable** (chiffre sans reçu).
- Un chiffre cité ici reprend une annonce du canal. Je nomme son reçu quand le
  canal le nomme et que `ls` le trouve à 12294241. Aucun reçu n'a été rejoué.

## 1. Périmètre, méthode, limites

Lu intégralement :

- `audits/COORDINATION_MORSEHGP3D_V8.md`, 3 850 lignes à 12294241. Le fichier
  est identique à celui de `a74e90f2`. Le dernier commit qui le touche est
  `204b0620` (21 sept., 20:08 UTC).
- **[hors HEAD]** le diff du worktree partagé sur ce fichier : +96 lignes (24
  indexées, 73 non indexées, une suppression). Il contient trois sections :
  - la section AUDITEUR_COMPLEMENTAIRE du 13 septembre, 53 lignes, insérée
    après C:1116 ;
  - la section « reprise u18 et saturation d'atlas » du 22 septembre ;
  - la section « raccord q3 global float32 » du 22 septembre.
- **[hors HEAD]** les passages cités des deux notes non suivies du
  complémentaire : `audits/morsehgp3D_v8_complementaire/P0_IDENTITES_NUAGE_ET_RECTANGLES.md`
  et `P0_CONSTANTES_CENSUS_Q2.md`.
- Côté v9 : `docs/AUDIT_V8_SYNTHESE.md`, `docs/HERITAGE_V7_V8.md`,
  `docs/PLAN_V9.md`, `docs/FAUSSES_PISTES.md`, `PASSATION.md`, `audits/*` et
  la lentille `docs/audit_v8/09_audits_independants.md`. Les autres lentilles
  ont été interrogées par `grep`.

Vérifications directes :

- Présence des fixtures annoncées par le canal dans `morsehgp3D_v8/tests/`
  (par `grep`, donc non exhaustive).
- Existence des reçus cités (`ls`).
- Relecture de `morsehgp3D_v8/docs/P0_TEMOINS_HERITES_Q2.md:160-201` et de
  `docs/Q34_PISTES_APRES_INDEXATION_20260921.md:195-219`.

Limites :

- Les auditeurs A et B répondaient surtout dans leurs propres fichiers
  (`morsehgp3D_v8/audits/DIALOGUE_COURANT.md`, `DIALOGUE_AUDITEUR_B.md`,
  dossiers datés). Le canal ne consigne leurs réponses que par un accusé de
  réception avec hash.
- Ici, « répondue » signifie : le canal consigne cet accusé. Je n'ai pas
  relu ces fichiers ; la lentille 9 l'a fait.

## 2. Plan du canal

| lignes | date | auteur | objet |
| --- | --- | --- | --- |
| 1–44 | 13/09 | ROOT | changement de cap, cinq questions aux auditeurs, fenêtre d'index |
| 46–127 | 13/09 | ROOT | P0 au premier rang, verrous B1–B5, démarrage de P0 |
| 129–273 | 13/09 | complémentaire | quantificateurs, rails, résidu transverse, ordre de DualBlocks, runner P1/P2, tubes |
| 275–438 | 13/09 | ROOT | tubes, correctifs (propriétaire, alias, runner), publication de P0, reprise |
| 440–571 | 13/09 | complémentaire | nappes 2D, R3, partage et axes, rotations, reçus appariés |
| 573–707 | 13/09 | constructeur puis complémentaire | tranche 3 (addition, intersection) ; coût du census |
| 709–983 | 13/09 | constructeur puis complémentaire | tranche 4 (census q2 partagé), raccord, coquilles, exceptions |
| 985–1116 | 13–14/09 | constructeur | lecteur renforcé, bornes préparées, cible massive |
| après 1116, **[hors HEAD]** | 13/09 | complémentaire | constantes préparées, identités du nuage et du rectangle (53 lignes) |
| 1118–1355 | 14/09 | constructeur | nuage et index partagés, premier front réel, census q2 du front |
| 1357–1660 | 14/09 | constructeur | tranches 9 à 12 : frère, ordre complément, census conjoint A×B, Pool terminal |
| 1662–2111 | 14–15/09 | constructeur | tranches 13 à 18 : workers, Donate, continuations, détachement, équipe, plages ; ouverture q3/q4 (C:1823) |
| 2113–2391 | 15–17/09 | constructeur | tranche 19 ; changement de constructeur (C:2155) ; tranches 20 et 21 |
| 2393–2961 | 20/09 | nouveau constructeur | tranches 22 à 30 : famille q4, candidats, covers, pool, collectif, carte des centres, atlas, couches, fenêtre |
| 2963–3409 | 21/09 | constructeur | tranches 31 à 34 : raccord global LiDAR, pilote G4, témoins indexés, Ξ affine, graines×cellules |
| 3411–3542 | 21/09 | constructeur | protocole spatial, contrat de trame entière, pilote G4 spatial |
| 3544–3798 | 21/09 | constructeur | float32 : précision, index, supports, identité, census q3 |
| 3800–3850 | 21/09 | constructeur | pilote sans sol |
| fin, **[hors HEAD]** | 22/09 | constructeur | reprise u18 et atlas saturant ; raccord q3 global float32 |

Constat de structure : le canal commis s'arrête à `204b0620`. Aucune section
n'annonce les six commits `src` du développeur (`748ec082` à `a74e90f2`), ni
les audits externes du 22 septembre. La lentille 9 (H4) le dit aussi.

## 3. Questions posées et leur état

### 3.1 ROOT, ouverture (13 septembre)

| id | lignes | question | état à 12294241 | trace |
| --- | --- | --- | --- | --- |
| Q1 | C:16-18 | La construction Morton a-t-elle la borne de décomposition nécessaire, ou faut-il un fair-split qualifié ? | **ouverte** : aucune preuve | Le front v8 (bissection au milieu) n'a « aucune borne O(s³n) » (C:1197-1199). Lentille 2, § 5 : « lacune de preuve » |
| Q2 | C:19-21 | Propager des IDs de témoins universels parent→enfants sans double compte h_a/h_b | q2 : **close** (prouvé, testé, mesuré ; option, défaut inchangé). Multivoie : **ouverte** | Théorème H, tranche 21 (C:2318-2323), reçu `q2_front_inheritance_20260917`. Multivoie différé (C:2425) ; « juge des bornes Ξ qui n'existe pas » (C:2381) |
| Q3 | C:22-24 | Quelles classes de crédits et bornes négatives évitent le travail quadratique sur les gros facteurs ? | **non close** : constantes divisées, exposants inchangés | Pool terminal (C:1630-1643) ; fenêtre 2K (C:2275-2284) |
| Q4 | C:25-27 | Obligations minimales pour transporter la contraction parallèle du graphe daté vers plateaux, contributions et verticales | **ouverte, jamais traitée** | aucune section ultérieure ; la v8 n'a pas d'aval |
| Q5 | C:28-30 | Représentation implicite des sorties quadratiques : quelles requêtes, quels coûts d'expansion ? | **ouverte** | synthèse v9 § 9, question 2 |

C:32 : « Aucune réponse indépendante nouvelle n'est encore enregistrée ici. »
Aucune section ultérieure ne revient explicitement sur Q1, Q4 ou Q5.

### 3.2 Phase P0 : complémentaire et ROOT (13 septembre)

| id | lignes | question ou demande | état | trace |
| --- | --- | --- | --- | --- |
| Q6 | C:53-58 | ROOT : une structure meilleure que les petits ensembles de témoins ? Preuve de rejet et de complétude, coûts comptés | **répondue**, P0 non clos | Réponses du complémentaire : rails (C:151-172), résidu transverse (C:211-220), nappes 2D (C:442-450), groupes recouvrants (C:475-484) |
| Q7 | C:110-127 | ROOT : contrelire `local_credits` (disjonction, bornes strictes, résidu, familles dégénérées) | **répondue, avis favorable** ; limite : résidu transverse | C:139-177 (quatre mutants, C:174-177) ; C:211-230 |
| QC1 | C:143-144 | Complémentaire : arrêter un raffinement facultatif en gardant tous les indécis dans le résidu | **sans réponse consignée** | proposé |
| QC2 | C:194-208 | Complémentaire P1/P2 : runner sans contrôle du tuple ; JSON tronqué ; binaire haché seulement au départ | **close** | C:296-303 ; rejeu R3 favorable, C:452-454 |
| QC3 | C:222-230 | Complémentaire : ordre de visite de DualBlocks par projection vers B | **différée** (C:306-307), puis sans objet | DualBlocks remplacé (FAUSSES_PISTES v9, l. 55) |
| QC4 | C:217-219 | Complémentaire : certificats par sous-rectangles ou par profondeur de blocs pour le résidu transverse | **différée** (C:306-307, C:348-350) ; reprise en partie par le front WSPD | P0 non clos |
| Q8 | C:369-377 | ROOT : raffinement de nappes 2D sans rescan de queues ni quota | **répondue** par un prototype d'audit (C:442-450) ; ROOT retient les colonnes exactes (C:379-385) | filtre axial ensuite fermé (FAUSSES_PISTES v9, l. 53) |
| QC5 | C:455-458 | Complémentaire P2 : invariant `sheet` du lecteur appliqué à une voie inactive | **close** | C:394-396, C:548-549 |
| QC6 | C:460-466 | Complémentaire : affectation de `CreditPlan` après `bad_alloc` | **close** | C:387-390, C:500-506 |
| QC7 | C:508-517 | Complémentaire : rotation isométrique (6 483 670 → 256 M candidates) ; certificat de colonnes de direction déclarée | filtre axial fermé ; certificat directionnel **jamais poursuivi** | proposé |
| QC8 | C:538-546 | Complémentaire : lecteur qui mélange Release et Debug ; `checksum_kind` non contrôlé | **close** | C:552-559 |

### 3.3 Chaîne q2, tranches 3 à 18 (13–15 septembre)

| id | lignes | question | état | trace |
| --- | --- | --- | --- | --- |
| Q9 | C:588-596 | Règle de rejet, d'acceptation ou de partage d'un nœud B | **répondue** : correcte | C:636-643 |
| Q10 | C:727-733 | Compteur partagé, frontière persistante, classification Z : manque-t-il une obligation ? | **répondue** avec obligations | C:790-818 : compteur exact sur les blocs consommés ; frontière qui partitionne les IDs ; jamais de redémarrage d'un enfant B à la racine Z ; saturation non reprenable à un seuil supérieur ; cache lié au nuage et à l'espace d'IDs ; recherche d'antipodes |
| Q11 | C:1057-1064 | Factoriser C=a+e et D=(e−a)², puis transporter ces constantes avec le curseur Z | dans HEAD, seul l'accusé « contrelecture 1bf806f0 » (C:1120). Réponse complète du complémentaire **[hors HEAD]** | Recalculer les constantes par enfant ; promotion signée avant différences et carrés ; piège de migration nuage/rectangle (voir N16) |
| Q12 | C:1141-1146 | À B : rejet certifié par K témoins sur des produits ancêtres, avant la fin de la séparation | **répondue**, puis réalisée par le front `MidpointSamples` | 77bcd0b8, 7a56d852 (C:1148-1155) ; C:1183-1192 |
| Q13 | C:1207-1209 | Bloc Z certifié pendant la descente ; transmission compacte des certificats partiels | transmission : théorème H (q2). Blocs Z : mesurés par B, **non tranchés** | lentille 9, l. 414 : la comparaison front + census n'a jamais été faite |
| Q14 | C:1237-1244 | Raccord direct du census sur les nœuds B du front ; réduction collective inter-amas et q3/q4 | raccord fait (C:1260-1355). Test de lentille : 0 à 1 % (C:1291-1292). Inter-amas : Pool (C:1584-1650). q3/q4 : Q25 | — |
| Q15 | C:1294-1298 | À A : rejouer le LiDAR u16 avec le même masque q2. À B : contrelire l'état conjoint (A, B, compte, curseur Z) | **répondue** | A 24a717d9 et 5c32ab95 (C:1359, C:1428-1429) ; B 1ca8f62d (C:1344-1345) |
| Q16 | C:1309-1312 | Fixture de raffinement prématuré ; choix de blocs qui change le travail | **répondue** par le constructeur lui-même | C:1314-1333 ; gravée dans `tests/q2_sibling_gate.cpp:262-275` |
| Q17 | C:1370-1374 | Stricteté du certificat frère, chevauchement avec le préfixe consommé, coût sur amas et LiDAR | **répondue** | A 5c32ab95 : preuve K−c (C:1386-1393), résultats LiDAR négatifs (C:1428-1429) |
| Q18 | C:1409-1420, C:1452-1453 | À A : ordre « complément puis B original » et sa continuation compacte | **répondue**, portée en tranche 10 | A a1ee8cb0, `q2_complement` (C:1466-1471, C:1490) ; fixture gravée `tests/q2_witness_order_gate.cpp:281-282` |
| Q19 | C:1473-1480, C:1511-1514 | À A : partager le census avant l'éclatement en ancres (tâche A×B) | **répondue** ; port de la tranche 11 **négatif** | A 7e315009 (C:1543-1545) ; C:1566-1573 ; fixture `tests/q2_joint_gate.cpp:241` |
| Q20 | C:1529-1532 | À A et B : plan local sur le propriétaire global, restriction sans minorant ajouté au compte | **répondue**, portée en tranche 12 | A fbbecc01, B e931d8f6 (C:1558-1563) |
| Q21 | C:1601-1606 | Contrelecture du port Pool (IDs et rangs, préfixes, compte nul, coût cumulé) | **répondue** | B 375c5288 (C:1680-1683) |
| Q22 | C:1672-1677 | Jobs du front : couverture, masques, identités, exceptions, déséquilibre | **répondue** | A 329e5b86, B 324c6398 (C:1691-1699) ; B 53d5640a, 0bc1f5fd (C:1719-1723) |
| Q23 | C:1753-1756 | Terminaison sans réveil perdu, exception avec workers dormants, échec de lancement | **répondue** | B db9cd8de (C:1805-1808) |
| Q24 | C:1819-1821 | Transfert de continuations entre workers ; cas positifs de suspension | **répondue** | B 61b86a54, 7b86e36b (C:1834-1843) ; impossibilité de l'admission multiple (C:1917-1929) |
| Q25 | C:1823-1831 | Ouverture q3/q4 : (1) arête × groupes de complétions ; (2) événements groupés q4 ; (3) éviter les m² arêtes entre deux rangées tout en gardant les propriétaires canoniques | (1) et (2) traités par l'oracle B 5124095b (C:1914-1915) puis les tranches 22–30. **(3) ouverte** | C:1831 : « Un meilleur s seul ne ferme pas ce dernier verrou » |
| Q26 | C:1845-1856 | À B : préciser les conditions de l'argument des rangées (u ≤ D/√3) | **close** | B c16459ea (C:1900-1904) |
| Q27 | C:1895-1898 | Obligations après détachements récursifs ; fermeture avec workers dormants | **répondue** | B bc9b2dc5, ddd915f4 (C:1917-1929) |
| Q28 | C:1966-1967, C:1986-1992 | Fermeture, annulation et bilan de l'équipe coopérative ; continuation singleton compacte | **répondue**, mais un test manque (N5) | B, contrelecture de 07h08 (C:1999-2001) ; conclusion C:2017-2019 |
| Q29 | C:2043-2048 | Durée de vie des plans parentaux, `b_order`, bilans globaux | **répondue** | reçu `ranges` de B (C:2096-2098) ; correction de lecture (C:2081-2083) |
| Q30 | C:2058-2064 | À B : le « si et seulement si » à la tangence j = 2i | **close** | C:2080-2081 ; gravé dans `tests/q3_q4_owner_independence_gate.py:100-129` |
| Q31 | C:2101-2103, C:2137-2143 | Une obligation interdit-elle de compacter les petits census ? | **répondue** : aucune obligation perdue | B a980976c, 6790abf3 (C:2145-2177) |

### 3.4 Tranches 19 à 21 (B constructeur, 17 septembre)

| id | lignes | question | état | trace |
| --- | --- | --- | --- | --- |
| Q32 | C:2211-2218 | À A et au complémentaire : compter les rejets de l'extension par voie sous masque 7 ; obligations des juges de bord | tranchée par des **relecteurs internes**, sans A ni le complémentaire : extension restreinte à q2 | C:2245-2257 |
| Q33 | C:2286-2295 | La restriction q2 est-elle trop prudente ? L'argument « n ≥ Kmax+2, donc l'extension n'est jamais vide » tient-il ? La projection du lecteur est-elle juste ? | réponse partielle de A. **L'argument n ≥ Kmax+2 reste sans réponse** ; le complémentaire ne répond pas | A (C:2421-2425) : la projection n'est pas un contrefactuel ; les promotions de voie seront à reproposer |
| Q34 | C:2372-2382 | Angles morts de l'identité du registre de crédits ; reprise exacte de la descente à l'échelle G4 ; majorant `inherited_rejections` ; juge Ξ | **ouvertes**, sauf les trois obligations multivoie de A | A (C:2421-2425, C:2442-2444) |

Pour Q32 à Q34, l'auteur de la question (ancien auditeur B, devenu
constructeur) note : « ses relecteurs sont des instances sans mémoire du
chantier, pas un troisième regard humain » (C:2372-2375).

### 3.5 Voies q3/q4, tranches 22 à 34 (20–21 septembre)

| id | lignes | question | état | trace |
| --- | --- | --- | --- | --- |
| Q35 | C:2408-2419 | Obligations du noyau de famille q4 porté par un triangle aigu | **répondue** : accord de A, retour sur le contrat mais pas qualification | C:2421-2422, C:2448-2449 |
| Q36 | C:2469-2479 | Raccord par graine : première présentation positive, candidat invalide sans suppression de son groupe | **répondue** : tranches 23 et 24 relues sans défaut nouveau | C:2577-2579 |
| Q37 | C:2490-2501, C:2526-2532 | Cover fermé partagé par arête ; transfert conditionnel ; accès des faces par la lentille avec départage par IDs | **répondue** (idem). Le carré résiduel adverse demeure (N11) | C:2538-2539 |
| Q38 | C:2568-2575 | Plafond entier de la corde de Jung ; rejet universel de famille | **répondue** | A 4215dd16 (C:2596-2597) |
| Q39 | C:2607-2612, C:2638-2642 | Subdivision du plan des centres pour des minorants collectifs réutilisés | **répondue** ; port 27 **sans gain stable** | A `q4_center_blocks` (C:2644-2651) ; C:2738-2744 |
| Q40 | C:2700-2704, C:2717-2724 | Composition pool26 + carte ; blocs Z × cellules et leurs invariants de continuation | **répondue**, portée en tranche 28 | A `q4_local_sweeps` (C:2726-2733), MATH/CLIPPING (C:2755-2759) |
| Q41 | C:2792-2799 | W quadratique sur le dense permuté : regrouper les faces dans les cellules ou traiter collectivement les événements peu profonds ? Quels certificats ne déplacent pas le carré ? | réponse partielle (couches duales 29, C:2825-2848 ; composition de A, C:2907-2916). **Ouverte** | L'adversaire K10 garde n(n−2) au premier parcours (C:2937) |
| Q42 | C:2847-2848, C:2867-2873, C:2898-2905 | Dégénérescences (droites coïncidentes, signes opposés, racines multiples) ; énumération des sommets peu profonds ; fenêtre et index de couches | **répondue** par A ; index de couches **jamais construit** | `q4_kernel_composition`, `WINDOW_INDEX` (C:2907-2916, C:2940-2943) |
| Q43 | C:2946-2950 | Chaînes monotones exactes par frontière : compter c et trouver les H extrêmes sans scan ; tangences au bord ; représentation des c=0 | **ouverte, sans réponse** | — |
| Q44 | C:2981-2988 | Citron sur l'arête maximale d'un support positif, quelle que soit la coquille ; complétude sur les paires du front | **close** : prouvé | C:2990-2991 ; HERITAGE v9 § 1 |
| Q45 | C:3002-3008 | Sur le LiDAR, la faiblesse vient-elle du choix de K témoins autour du pivot, ou de l'absence de crédits h_a/h_b par blocs ? | **répondue**, conduit à la tranche 32 | B `front_lanes_lidar_20260921` (C:3056-3058) |
| Q46 | C:3020-3026 | Census q3 par boîtes de la puissance exacte ; certificats de famille ou blocs de graines avant les clés | boîtes : **portées** en tranche 32 (C:3083-3088) ; blocs de graines : voir Q52 et Q53 | — |
| Q47 | C:3071-3076 | Une descente partagée à deux masques et deux seuils | **répondue**, confirmée | B 74196a31 (C:3175-3177) |
| Q48 | C:3108-3120 | Le noyau 29 à T=K−1 peut-il filtrer les graines q3 ? Contre-fixture du mauvais T=K−2 | **différée** pour son coût (C:3175-3184) ; contre-fixture seulement documentée (N17) | — |
| Q49 | C:3128-3140 | (1) Ξ affine et exclusion locale d'une voie ; (2) transmettre les comptes de témoins communs et l'état Z entre produits subdivisés | (1) **portée** en tranche 33 (C:3195-3202). (2) invariants fournis par A, **non portée** | A `PREFIXES_TEMOINS` (C:3210-3216), `q34_prefix_order` (C:3246-3251), 93ce6fc5 (C:3269) |
| Q50 | C:3218-3224 | Ordre DFS figé par rectangle original, ou ordre circulaire en deux phases | **répondue**, non portée | A (C:3246-3251) : pivot du rectangle **original** ; raffiner la frontière avant toute borne ; `escape(S_d)` d'abord |
| Q51 | C:3231-3244 | Parcours conjoint blocs de graines X × cellules d'atlas | **répondue**, portée en tranche 34 : LiveOnly utile, Joined sans gain stable | A `q4_seed_cell_join` (C:3342-3346) ; C:3392-3398 |
| Q52 | C:3275-3292 | Témoins communs d'un bloc de graines q3 via F = 4J·puissance | **ouverte** (C:3347) ; alternative A f45e27c0 (centres conditionnels, C:3361-3366). **Aucun port dans le moteur entier** : la « priorité 35 » (C:3399-3400) n'a jamais été exécutée | — |
| Q53 | C:3367-3369 | Relais qui limite les reprises de racine sans grossir l'état | **répondue** (relais A 32297105, C:3456-3460), **non portée** dans le moteur entier | C:3458-3459 : « Il reste la priorité du prochain port moteur » |

### 3.6 LiDAR, G4 et float32 (21 septembre)

| id | lignes | question | état | trace |
| --- | --- | --- | --- | --- |
| Q54 | C:3462-3465 | Signaler une fenêtre libre pour des chronos isolés | **sans réponse consignée** | règle « hôte calme » de PLAN_V9 |
| Q55 | C:3566-3569, C:3619-3621, C:3657-3658 | Clés et bornes q3/q4 compatibles float32 ; normalisation des clés entre supports ; certificats de blocs | réponses de B (74fb0a6a, f7b220c4) ; voie **dormante** | — |
| Q56 | C:3701-3702 | Bornes de blocs, **ordre exact des rayons**, transport groupé des sorties | **ouverte**. L'ordre exact des niveaux et le transport des sorties concernent tout catalogue, pas seulement le float32 | — |
| Q57 | C:3704-3713 | Enveloppe de centres conditionnelle (SharedPrefix float32) | répondue par B 74fb0a6a (C:3787-3793) ; voie dormante | — |
| Q58 | C:3825-3828 | Port global avec propriété dans X ; enveloppe m+[0,1/3]h ; contre-exemples qui changeraient la population des témoins | **répondue** | B ad9bbc9d (C:3846-3850) : deux témoins (aigu non propriétaire, obtus) et un cas à deux arêtes maximales ; règle « X seulement, jamais Z » |

### 3.7 Questions hors HEAD

| id | lieu | question | état |
| --- | --- | --- | --- |
| Q59 | section du 22/09 « reprise u18 » **[hors HEAD]** | Le census q3 peut-il réutiliser seulement les feuilles exactes de l'atlas saturant, avec repli sur le certificat profond K−2 ? | sans réponse ; levier inscrit au PLAN V9-2 |
| Q60 | section du 22/09 « raccord q3 global float32 » **[hors HEAD]** | Autres fixtures courtes pour des sorties croissantes et pour le split-tree médian | sans réponse ; voie dormante |

### 3.8 Bilan : questions ouvertes qui comptent pour la v9

- Théorie du front :
  - Q1 : aucune borne de packing pour la WSPD à bissection au milieu.
  - Q25(3) : les m² arêtes entre deux rangées.
- Aval de la tour :
  - Q4 : obligations du graphe daté.
  - Q5 : sortie implicite.
  - Q56 : ordre exact des rayons et transport groupé des sorties.
- Héritage multivoie :
  - Q2 et Q34 : juge Ξ inexistant, angles morts du registre.
  - Q49(2) et Q50 : préfixes de témoins par voie entre produits.
- Travail q3 partagé par blocs de graines : Q52 et Q53. C'était la dernière
  priorité déclarée pour le moteur entier, et elle n'a jamais été exécutée.
- Atlas q4 :
  - Q41 et Q43 : carré résiduel.
  - Q48 : filtre T=K−1, différé.
- Points restés sans réponse :
  - Q33 : argument « n ≥ Kmax+2 ».
  - QC1 : résidu des indécis.
  - Q54 : fenêtre de mesure.

## 4. Engagements et décisions du constructeur encore pertinents

### 4.1 Invariants et refus déclarés

| id | décision | lignes | statut | présent en v9 ? |
| --- | --- | --- | --- | --- |
| D1 | Aucun quota ni troncature. Un budget ne coupe jamais une recherche ni une sortie ; un grain de relais ne tronque rien | C:308, C:1192, C:2132-2133, C:2566, C:3747 | appliqué en v8 (déclaré à chaque tranche) | **absent** de SYNTHESE et HERITAGE comme invariant écrit |
| D2 | Le census repart de zéro ; aucun crédit de filtre n'est ajouté au compte d'une boule | C:1591, C:2242, C:2563-2564, C:3068, C:3136 | appliqué | FAUSSES_PISTES, l. 24 (compte transmis) ; HERITAGE implicite |
| D3 | Voies indépendantes : q3 et q4 jamais conditionnés par l'acceptation q2, q4 jamais par q3 | C:2522, C:2978, C:3026 | testé (contre-fixtures) | oui : SYNTHESE § 5, HERITAGE § 4 |
| D4 | Un rejet de voie n'est jamais l'inertie globale d'une boule (réserve de A sur I1) | C:1224-1225 | déclaré | **absent** (seule la lentille 9 cite I1–I3) |
| D5 | Exclure un bloc Z retire une voie du masque **local** seulement ; ce n'est jamais un rejet de l'arête | C:3160-3165 | appliqué (tranche 33) | **absent** |
| D6 | La lentille borne les centres positifs mais n'autorise pas à retirer du census les témoins hors lentille ; unions de cellules par minimum, jamais par addition | C:2653-2656 | déclaré | **absent** |
| D7 | Le filtre à min ≥ 0 est sûr pour un rejet mais impropre à l'émission des coquilles ; ses comptes comprimés ne sont pas des comptes exacts | C:2727-2730 | déclaré | **absent** ; utile pour `saturate_deep` |
| D8 | Séparer le seuil de sélection d'un noyau (T=K−1) du seuil d'émission q4 (K−2) | C:3113-3115 | proposé (API) | **absent** |
| D9 | L'enveloppe propriétaire filtre X, jamais Z ; les graines invalides restent des témoins | C:3742-3743, C:3827-3828, C:3847-3850 | testé (F9) | FAUSSES_PISTES, l. 27 |
| D10 | Pour W3, exiger explicitement H > 0 en plus de 3H² > Ξ | C:1903-1904 | déclaré ; aucun mutant dédié trouvé par `grep` | **absent** |
| D11 | Tester les quatre poids q4 avant de normaliser l'orientation | C:3654-3655 | testé (float32) | FAUSSES_PISTES, l. 28 |
| D12 | Défauts = moteur historique (Individual, Coarse, GlobalDfs, fenêtre 1, sans héritage) ; Pairwise+joint refusé ; facteur de fenêtre ≠ 1 refusé hors q2 | C:1508, C:2249-2251, C:2284, C:2369-2370 | appliqué | SYNTHESE § 6 (gravité moyenne) ; PLAN, principe 5 |
| D13 | Ne jamais passer les formats f32/u32 à la sonde u16 ; ne jamais lancer G4 sous un nom de configuration trompeur | C:3564-3565, C:3479-3480 | déclaré | en partie : HERITAGE, ligne `.u32le` (« profil déclaré par la grille ») |
| D14 | Repli exact du Pool quand le plan ne retire aucune paire. Motif : un routage naïf aurait développé 16 M paires sur deux colonnes alignées (chiffre de préflight, **non vérifiable**) | C:1609-1616 | testé (portes Pool) | **absent** de la ligne « Pool terminal » de HERITAGE § 2 |
| D15 | Preuve du minimum transversal : la paire de distance minimale entre A et B n'a aucun témoin diamétral strict interne ; restriction vide ⇔ h = 0 | C:832-837, C:1609-1610 | prouvé (note du complémentaire) | **absent** |

### 4.2 Obligations retenues pour le catalogue et la tour (jamais exécutées en v8)

| id | obligation | lignes | statut | présent en v9 ? |
| --- | --- | --- | --- | --- |
| O1 | Partage futur par clé ou par nuage : garder les surplus déjà consommés si un seuil supérieur arrive, ou recalculer au nouveau seuil | C:739-741, C:800-808 | proposé ; l'API v8 est mono-seuil (C:862-864) | **absent** de SYNTHESE et HERITAGE (lentille 9 : « API multi-seuil ») |
| O2 | La répétition des payloads d'une même boule est une « cible déclarée du futur catalogue » ; fixture de 398 sites, 5 supports, 1 clé | C:952-961, C:999-1002 | testé par le juge du complémentaire ; catalogue manquant | **absent** de HERITAGE § 4 |
| O3 | Deux diamètres d'une même boule ne font pas deux boules. Une boule a au plus floor(taille de la coquille / 2) diamètres, de partenaire S−x ; recherche d'antipodes | C:668-673, C:722-725, C:812-815 | prouvé (note du complémentaire) ; non porté | **absent** |
| O4 | Un cache de canonisation dépend du nuage immuable et de l'espace d'IDs | C:810-812 | proposé | **absent** |
| O5 | La coquille émise inclut le support ; le consommateur forme « coquille moins support » ; support et coquille sont deux volumes, pas une partition | C:3093-3096 | contrat du flux v8 | **absent** |
| O6 | ABCDE et MEB hors catalogue sont des obligations du port FULL ; la coquille ≤ K du petit pivot n'est pas la coquille globale, non bornée | C:2444-2448 | déclaré | ABCDE : HERITAGE § 4 ; coquille non bornée : FAUSSES_PISTES, l. 31 |
| O7 | Rejeter un support à poids nul ne dispense pas de collecter son site comme contact d'une autre présentation positive de la même boule | C:3673-3676 | déclaré | **absent** |
| O8 | Test commun q2/q3/q4 sur une même boule (centre 0, rayon 5) ; ordre exact des rayons ; transport groupé des sorties | C:3699-3702 | testé en float32 (lentille 4) ; le reste est ouvert | **absent** de HERITAGE § 4. Existe aussi en entier : `tests/exact_ball_gate.cpp:207` |
| O9 | Invariants « de rétention par boule et de plus longue arête q3/q4 » ajoutés au plan | C:1154-1155 | gravés en partie (lentille 9) | **absent** |

### 4.3 Pistes différées, ni portées ni réfutées

| id | piste | lignes | statut | présent en v9 ? |
| --- | --- | --- | --- | --- |
| X1 | Reprise exacte de la descente : même pivot, 17 à 38 % des pas de descente, ×0,94 à ×0,98. Patch archivé, « à réévaluer à grande échelle » | C:2320-2322, C:2378-2379 ; `docs/P0_TEMOINS_HERITES_Q2.md:173,181-197` | mesuré (moteur réel) | **absent** de HERITAGE ; lentille 2, recommandation 12 |
| X2 | Héritage de témoins multivoie, avec un juge des bornes Ξ | C:2381-2382, C:2425, C:2442-2444 | manquant | HERITAGE dit « étendre l'héritage par voie », **sans exiger le juge Ξ** |
| X3 | Travail q3 partagé par blocs de graines (relais A 32297105, centres conditionnels A f45e27c0, question F = 4J·puissance) dans le moteur entier | C:3309-3311, C:3399-3400, C:3456-3460, C:3525-3526 | prouvé et testé en modèle (A) ; porté seulement en float32, sur **une** arête (C:3739-3770) | **absent** de SYNTHESE, HERITAGE et PLAN V9-2 |
| X4 | Comptes et curseurs de témoins par voie, transmis entre produits subdivisés (A `PREFIXES_TEMOINS`, `q34_prefix_order`) | C:3131-3140, C:3210-3216, C:3246-3251 | proposé, modèle de A | **absent** |
| X5 | Couches q3 à T=K−1 | C:3108-3120, C:3175-3184 | prouvé ; non rentable selon B | FAUSSES_PISTES, l. 72 |
| X6 | Index de couches en chaînes monotones (fenêtre 30) | C:2898-2905, C:2946-2950 | proposé | **absent** |
| X7 | Remontée des extrema de crédits de B | C:628-630, C:837-838 | proposé | absent (mineur) |
| X8 | Cache tangent (A d608cc28) | C:1624-1626 | proposé | absent (mineur) |
| X9 | Gros callback Pool indivisible ; un census commencé n'est pas divisé par la redistribution du front | C:1697, C:1806-1808 | mesuré, sans gain général | FAUSSES_PISTES, l. 63 ; PLAN V9-3 (« arêtes lourdes scindées ») |

### 4.4 Conventions et protocoles

| id | point | lignes | statut | présent en v9 ? |
| --- | --- | --- | --- | --- |
| P1 | Convention de séparation `box_gap_diameter_v1` (gap ≥ s·max(diag)), non comparable au s de la v4 ; « le front futur doit fixer sa convention » ; le s des fixtures n'est pas réinterprété en s v4 | C:1152-1155, C:1194-1195 ; `morsehgp3D_v8/docs/P0_FRONT_REEL.md:35` | appliqué | **absent** de SYNTHESE et HERITAGE. PLAN_V9, l. 17 écrit « s = 8 (jamais moins) » sans nommer la convention |
| P2 | LiDAR : protocole distinct pour la densification et l'extension, les poses communes et les collisions de quantification | C:1253-1257 | proposé | **absent** |
| P3 | Sans sol : garder le brut, publier les erreurs de retrait et le coût segmentation+HGP, figer le masque avant les coupes, conserver IDs et retours. Retirer des sites peut créer davantage de boules peu profondes | C:3775-3780 | protocole écrit (`docs/LIDAR_SANS_SOL_PROTOCOLE_20260921.md`) | en partie : SYNTHESE § 9, question 1 (frontière du chronomètre) |
| P4 | Profil 20 mm et fusions publiés ; plusieurs scènes, diversité élargie avant qualification ; les 50k ne sont pas un sous-échantillonnage autorisé | C:3470-3473 | décision | SYNTHESE § 2 et § 6 |
| P5 | Dans les reçus, `git_commit` est le HEAD de lancement, pas le contenu testé ; les snapshots font autorité | C:3797-3798 | constat | lentilles 4 et 9 ; **absent** de SYNTHESE et HERITAGE |

### 4.5 Décisions de l'utilisateur relayées par le canal

| date | lignes | énoncé | dans SYNTHESE § 2 ? |
| --- | --- | --- | --- |
| 13/09 | C:5-9 | Audit général v7, puis refonte v8 ; publication sur main uniquement | implicite |
| 13/09 | C:48-51 | P0 au premier rang, sans imposer la piste des petits ensembles de témoins | oui |
| 13/09 | C:78-82 | Consigner les verrous B1 à B5 (`morsehgp3D_v8/docs/VERROUS_ARCHITECTURE.md`) | **non** : document jamais cité par SYNTHESE ni HERITAGE ; son contenu B3–B5 est en partie repris par PLAN V9-2 à V9-4 |
| 13/09 | C:101 | Feu vert pour coder ; G4 SPOT gardée si nécessaire | non (sans effet aujourd'hui) |
| 13/09 | C:1047-1048 | Contrats 50k sur G4 ; « contrôle continu de la complexité » | seulement la ligne « hérité v7 » |
| 21/09 | C:2965-2970 | Critère : croissance et coût sur les régimes visés (SemanticKITTI) plutôt qu'une preuve sous-quadratique uniforme préalable. Les contre-régimes sont des tests de résistance, pas un veto ; exactitude non relâchée | **absent** (lentille 1, C2, qui cite `AGENTS.md:314`) |
| 21/09 | C:3122-3123 | Adapter aux voies q3/q4 les astuces q2 qui partagent le travail par groupes | **absent** |
| 21/09 | C:3421-3423 | Croissance sur scène entière, moitiés et quarts, coupés par des plans passant par le capteur | SYNTHESE § 5 (protocole) |
| 21/09 | C:3468-3473 | Tour d'une trame entière, plusieurs scènes, K=1..10 (repli 1..5), 1 s puis 100 ms | oui |
| 21/09 | C:3546-3550 | Précision nettement meilleure que 2 cm ; float32 de préférence ; grille 1 mm optionnelle | oui |
| 21/09 | C:3772 | LiDAR sans sol aussi prioritaire | oui (via la directive de 20:10) |

## 5. Constats d'audit non clos

| id | constat | source | statut | présent en v9 ? |
| --- | --- | --- | --- | --- |
| N1 | Aucune borne de packing pour la WSPD à bissection au milieu | C:16-18, C:1197-1199 | manquant (preuve) | **absent** de SYNTHESE et HERITAGE ; lentille 2, § 5 |
| N2 | Résidu transverse : crédits universels tous nuls, mais profondeur 2·max(0, écart des rangs − 1). À h10 et n512, 65 536 paires émises pour 2 786 survivantes du census | C:211-220 ; `audits/morsehgp3D_v8_complementaire/P0_RESIDU_TRANSVERSE.md` | testé (complémentaire) ; P0 non clos | fixture **absente** de HERITAGE § 4 |
| N3 | Deux rangées lointaines, A_i=(1000,i,0), B_j=(60000,j,0) avec 3(m−1)² < 59000² : aucun témoin W3/W4 pour une paire transversale, donc m² paires q3/q4 quels que soient K et s | C:1202-1206 ; verrou C:1828-1831 | prouvé (argument du constructeur). Rangées présentes dans des portes q2 (`tests/wspd_q2_ranges_gate.cpp:316`) ; **non trouvées par grep comme contre-régime q3/q4** | **absent** |
| N4 | Mutant de `classify_witness_block` (lacune 2.2) et autres lacunes de portes de B | C:1217-1220 | 2.2 ouverte ; 2.3, 2.4, 2.6 et 2.8 non closes (lentille 9, M4) | 2.2 : HERITAGE § 4. 2.3 à 2.8 : **absentes** (la 2.8 : 16 tests sur 34 échouent sous un autre nom de dossier) |
| N5 | Aucune preuve positive qu'un receveur dormait déjà au moment du `throw` | C:1976-1978 | manquant (test) | **absent** de toute la v9 (`grep` négatif) |
| N6 | `GlobalDfs` (défaut) n'est couvert que par la porte C++ ; sondes et campagnes fixent ComplementFirst | C:2173-2176 | limite déclarée | **absent** |
| N7 | Extension multivoie sans juge : trois mutants du moteur survivaient sous masque 7 ; le juge Ξ n'existe pas | C:2246-2248, C:2381 | manquant | HERITAGE : condition de port sans juge ; lentille 2, recommandation 5 |
| N8 | Angles morts de l'identité du registre de crédits ; `inherited_rejections` n'est qu'un majorant, faute de contrefactuel | C:2376-2380 | ouvert | **absent** |
| N9 | Énoncé de monotonie faux, remplacé par la domination préfixe. Rangs tronqués à 8 ou 16 bits indétectables tant qu'aucun nuage de porte ne dépasse 128 sites | C:2327-2336 | corrigé en v8 (nuage de 320 sites, campagne à 70 000 sites) | leçon **absente** : dimensionner les nuages de porte au-delà de la largeur de chaque champ (rangs, IDs, coordonnées 18 bits) |
| N10 | Mutants inertes ou survivants : `worker_digest` inerte (le worker 0 peut être vide) ; `minimum>=0` survivant à la fixture de contact ; autotest qui laissait passer `child_reads+1` | C:2919-2927, C:3371-3374 | corrigés | leçon en partie dans PLAN et FAUSSES_PISTES (mutants causaux) |
| N11 | Adversaire q4 K10 : n(n−2) au premier parcours ; W quadratique sur le dense permuté (×3,02 et ×3,15 après couches) | C:2792-2794, C:2858-2859, C:2937 ; reçus `q4_shallow_20260920`, `q4_window_20260920` | mesuré | FAUSSES_PISTES, l. 75 (covers) ; SYNTHESE § 6.3 en termes généraux |
| N12 | Chaîne d'outils G4 : GCC 11 en `-Werror` arrête R1 sur un avertissement absent en local ; R2 bloquée, cause « hypothèse sans trace de pile » (`rational/int == 0` sous un Boost ancien) | C:3028-3031, C:3089-3091 | ouvert | **absent** de SYNTHESE, HERITAGE et PLAN ; lentille 8 (R9), lentille 9 (L5) |
| N13 | Sanitizers dans cet environnement : GCC TSan inutilisable (SIGSEGV, « unexpected memory mapping »), d'où Clang TSan ; LSan exige de sortir du sandbox (ptrace) ; le lanceur résolvait `clang++` en `clang` ; GCC refuse `-O2` avec UBSan | C:1769-1772, C:1172-1173, C:1397-1400, C:3577-3578, C:685-686 | constats d'environnement | **absent** : PLAN V9-0 prévoit `MHGP9_SANITIZE` et `MHGP9_TSAN` sans compilateur ni exigence hors sandbox |
| N14 | Chronos pris sous la charge de trois processus lourds d'auditeurs ; demande de fenêtre libre restée sans suite | C:3462-3465 | ouvert | règles de mesure du PLAN |
| N15 | Exposants de croissance sur la scène 0, relation trame → moitié positive : 2,502 (bornes q3) et 2,332 (blocs q4). « Ne pas omettre » ces rapports défavorables | C:3511-3517 ; reçu `receipts/q34_spatial_20260921/GROWTH_SCAN0_K5_S8.json` | mesuré (u16, 2 cm, avec sol) ; méthode critiquée (lentille 9, M5) | **absent** de SYNTHESE § 4 |
| N16 | Piège de migration **[hors HEAD]** : sites [100, 101, 200, 0], A = {100, 101}, B1 = {200}, B2 = {0}, K1, s12. Transférer à B2 le crédit de 100 obtenu contre B1 supprime le diamètre Gabriel (100, 0). Le seuil vient de la requête, la permutation vers les IDs originaux doit être déclarée, un cache de plages B dépend de l'ordre | diff hors HEAD après C:1116 ; `P0_IDENTITES_NUAGE_ET_RECTANGLES.md` (non suivi) | prouvé (note non publiée) ; règles appliquées par le constructeur (C:1149-1153) ; fixture **non gravée** (`grep` négatif) | **absent** : SYNTHESE § 8 cite seulement les « notes de l'auditeur complémentaire du 13 septembre » |
| N17 | Contre-fixture du mauvais seuil T=K−2 : a(10,10,10), b(14,10,10), x(12,13,10), y(11,12,10), z±(12,12,8/12), K3 | C:3116-3118 ; `docs/Q34_PISTES_APRES_INDEXATION_20260921.md:205-219` (« pas une nouvelle gate exécutée ») | proposé (documenté) | **absent** |
| N18 | Exemple d'exclusion locale : a=(10,10,10), b=(20,10,10), Z=[14,16]×[13,14]×{10}. Ξ affine [900,1600] contre borne générale [576,2304] ; seule la borne affine exclut les deux citrons | C:3163-3165 ; `docs/Q34_PISTES_APRES_INDEXATION_20260921.md:113-114` | documenté ; non trouvé comme fixture par `grep` | **absent** |
| N19 | Aucune section du canal pour les six commits `src` et les audits du 22 septembre | voir § 2 | constat | lentille 9 (H4) ; SYNTHESE ne le formule pas |

## 6. Confrontation avec AUDIT_V8_SYNTHESE.md et HERITAGE_V7_V8.md

### 6.1 Ce que les deux documents couvrent déjà

- P0 non clos et constantes divisées (SYNTHESE § 2, § 6.3).
- Citron, contre-exemple α3 sur q4, théorème H, certificat frère, ordre
  ComplementFirst, Pool (HERITAGE § 1).
- Indépendance des voies (SYNTHESE § 5 ; HERITAGE § 4).
- Contrat de trame entière, float32, sans sol, 18 bits (SYNTHESE § 2).
- Défauts lents en opt-in (SYNTHESE § 6).
- Incident `4c3cdb0c` et état non commis (SYNTHESE § 6.7 et § 8).
- Sortie implicite (SYNTHESE § 9, question 2).
- Fixtures déjà listées : lacune 2.2, E5, ABCDE, contre-exemple α3,
  {0,5,10,11}, cube, rails.
- Graphes datés de la v7 comme plan de l'aval (HERITAGE § 3).

### 6.2 Ce qui manque

Gravité haute :

1. **Travail q3 partagé par blocs de graines** (X3, Q52, Q53). C'est la
   dernière priorité déclarée pour le moteur entier (C:3399-3400,
   C:3458-3459, C:3525-3526). Elle a des preuves et des modèles de A
   (32297105, f45e27c0), mais n'a été portée qu'en float32, sur une seule
   arête. Ni SYNTHESE, ni HERITAGE, ni PLAN V9-2 ne la citent. Le census q3
   pèse environ 28 % du seul profil épinglé (lentille 9, H1).
2. **Invariants et obligations de l'aval**, jamais exécutés en v8, que le
   catalogue v9 (PLAN V9-1) devra tenir :
   - O1 : surplus multi-seuils ;
   - O2 : fixture de coquille répétée de 398 sites ;
   - O3 : deux diamètres ne font pas deux boules ; recherche d'antipodes ;
   - O5 : coquille = support compris ;
   - O7 : contact d'un support à poids nul ;
   - O8 : ordre exact des rayons et transport groupé des sorties ;
   - D4 : un rejet de voie n'est pas une inertie de boule.
3. **Questions de fond jamais répondues** : Q1 (borne de packing de la
   bissection au milieu) et Q4 (obligations du graphe daté). La v9 construit
   l'aval sur des graphes datés (HERITAGE § 3) sans que ces obligations aient
   été écrites.

Gravité moyenne :

4. **Décision utilisateur du 21 septembre** sur le critère de croissance :
   les contre-régimes sont des tests de résistance, pas un veto (C:2965-2970,
   `AGENTS.md:314`). S'y ajoute la consigne « astuces q2 qui partagent le
   travail par groupes » (C:3122-3123). Toutes deux manquent au tableau
   SYNTHESE § 2.
5. **Convention de séparation** `box_gap_diameter_v1` (P1). PLAN_V9 impose
   s = 8 sans dire dans quelle convention. Or le s = 8 de la v8 est encadré
   par les s = 14 et s = 16 de la v4 (lentille 9, § 4).
6. **Héritage multivoie** : il faut un juge indépendant des bornes Ξ (N7) et
   les trois obligations de A (C:2442-2444). La ligne « Front WSPD » de
   HERITAGE § 2 dit seulement « étendre l'héritage par voie ».
7. **Piège de migration nuage/rectangle** (N16) : pas encore gravé comme
   fixture. Les règles « seuil de la requête » et « permutation déclarée »
   devront tenir dans les tickets et parents partagés du PLAN V9-3.
8. **Contre-régime des deux rangées lointaines** pour q3/q4 (N3) et **résidu
   transverse** (N2). Ce sont les seules fixtures où aucun témoin ponctuel ne
   peut rejeter. Elles motivent le certificat collectif et manquent à
   HERITAGE § 4.
9. **Fixtures déjà gravées en v8 mais absentes de HERITAGE § 4** :
   - raffinement prématuré (`tests/q2_sibling_gate.cpp:262-275`) ;
   - ComplementFirst (`tests/q2_witness_order_gate.cpp:281-282`) ;
   - census conjoint (`tests/q2_joint_gate.cpp:241`) ;
   - tangence des rangées (`tests/q3_q4_owner_independence_gate.py:100-129`) ;
   - point q4 isolé à huit contacts (`tests/q4_seed_cells_gate.cpp:127`,
     `tests/q4_window_gate.cpp:52`) ;
   - boule commune aux trois arités (`tests/exact_ball_gate.cpp:207`).
10. **Fixtures seulement documentées** à graver : N17 (seuil T=K−2) et N18
    (exclusion locale par Ξ affine).
11. **Repli exact du Pool** et preuve du minimum transversal (D14, D15) :
    absents des conditions de port de la ligne « Pool terminal ».
12. **Environnement de test et de G4** (N12, N13) : GCC 11 et un Boost ancien
    sur la VM, GCC TSan inutilisable, LSan hors sandbox. Sans ces faits,
    PLAN V9-0 (CI, `MHGP9_TSAN`) et le protocole G4 v9 répéteront les échecs
    R1 et R2.
13. **Lacunes de portes 2.3, 2.4, 2.6 et 2.8** de B (N4) ; test positif
    « receveur endormi au `throw` » (N5) ; couverture de `GlobalDfs` (N6).
14. **Invariants D5 à D8 et D10** :
    - D5 : masque local et non arête ;
    - D6 : la lentille borne les centres, pas les témoins ; unions par
      minimum ;
    - D7 : min ≥ 0 ne vaut pas pour l'émission ;
    - D8 : seuils de sélection et d'émission séparés ;
    - D10 : H > 0 explicite pour W3.

    Chacun est une règle qu'un port v9 peut violer sans que les portes
    actuelles le voient.

Gravité basse :

15. Exposants de croissance 2,502 et 2,332 (N15), à citer avec la réserve M5.
16. Reprise exacte de la descente (X1), différée « à grande échelle » ;
    lentille 2 seulement.
17. Sémantique de `git_commit` dans les reçus v8 (P5), à connaître avant de
    relire un reçu v8.
18. Invariant « aucun quota, aucune troncature » (D1), jamais écrit dans les
    documents v9.
19. Protocole LiDAR de densification, poses communes et collisions (P2).
20. `morsehgp3D_v8/docs/VERROUS_ARCHITECTURE.md` (B1–B5), consigné à la
    demande de l'utilisateur, jamais cité.

### 6.3 Défauts et incohérences dans les documents v9

1. **FAUSSES_PISTES contredit la lentille 9 sur les « blocs Z certifiés le
   long de la descente ».** `docs/FAUSSES_PISTES.md:59` classe la piste comme
   fermée (57,6 → 78,6 s). `docs/audit_v8/09_audits_independants.md:414` la
   classe « différée, non fermée » : B demandait que la comparaison front +
   census tranche, et elle n'a jamais été faite. Le canal la laissait aussi
   ouverte (C:1207-1209). Une piste mal classée comme fermée ne se rouvre, par
   la règle de FAUSSES_PISTES, qu'avec un fait nouveau.
2. **Même contradiction sur « rejeter q4 seul pour supprimer l'atlas ».**
   FAUSSES_PISTES, l. 78 la dit fermée ; la lentille 9, l. 417 y voit une
   réserve de méthode, pas une réfutation.
3. **SYNTHESE § 2 se présente comme la liste « dans l'ordre » des contrats et
   décisions, mais omet la décision C2 du 21 septembre** (croissance sur les
   régimes visés, contre-régimes sans veto). Elle figure pourtant dans
   `AGENTS.md:314` et dans la lentille 1.
4. **SYNTHESE § 8 présente les notes non commises du complémentaire comme un
   simple état en suspens.** Elles contiennent un contre-exemple de
   correction (N16) et deux notes non suivies ; `P0_IDENTITES_NUAGE_ET_RECTANGLES.md`
   cite en outre `CLOUD_RECTANGLE_IDENTITY_CHECKS.json`, absent (lentille 9,
   M1). Les « publier ou abandonner » sans en extraire la fixture perdrait le
   seul contre-exemple de migration entre rectangles.
5. **PLAN_V9, l. 17 impose « s = 8 (jamais moins) » sans convention de
   séparation.** Le s de la v8 (`box_gap_diameter_v1`) et celui de la v4 ne
   sont pas comparables (P1). Sans convention écrite, la règle est ambiguë
   pour un nouveau front.
6. **HERITAGE § 2, ligne « Front WSPD », fixe la condition « étendre
   l'héritage par voie » sans le juge Ξ.** C'est précisément l'absence de ce
   juge qui a fait restreindre l'extension à q2 : trois mutants survivaient
   sous masque 7 (C:2246-2248).

## 7. Recommandations pour la v9

1. Ajouter à PLAN V9-2 le levier « census q3 partagé par blocs de graines »
   (relais de A, centres conditionnels), en le comparant par ablation à
   `0948d2d0` (rejet par l'atlas) et à la saturation d'atlas. Porter la
   preuve de A depuis le modèle, pas depuis le code float32 dormant.
2. Écrire, avant le catalogue V9-1, un contrat d'aval qui reprend O1 à O9 et
   D4. Graver O2, O3, O8 et N16 comme fixtures d'égalité.
3. Compléter HERITAGE § 4 : fixtures gravées du § 6.2, point 9 ; fixtures
   seulement documentées N17 et N18 ; contre-régimes N2 et N3.
4. Écrire la convention de séparation v9, avec sa correspondance mesurée
   vers le s de la v4.
5. Consigner dans PLAN V9-0 la chaîne d'outils G4 (GCC 11.4, Boost
   d'Ubuntu 22.04) comme cible de CI, ainsi que Clang TSan et LSan hors
   sandbox.
6. Reclasser dans FAUSSES_PISTES les deux pistes du § 6.3 comme
   « différées ».
7. Ajouter à SYNTHESE § 2 la décision C2 et la consigne du 21 septembre.
8. Rouvrir, dans le canal v9, les questions Q1, Q4, Q25(3), Q33 et Q43, avec
   un destinataire nommé.
