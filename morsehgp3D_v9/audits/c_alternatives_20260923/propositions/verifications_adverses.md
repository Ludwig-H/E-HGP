# Vérifications adverses des finalistes (brut)

===== refute-cout:D3_echelles tient_avec_reserves
La conclusion tient et sort renforcée : D3 seul reste loin de 1 s et l'effort doit aller vers la réfutation des ancres longues. En revanche, les chiffres de la projection sont faux pour deux trames sur trois, et toujours dans le sens favorable à D3.

**Faille principale : les parts 8k ont été transposées aux trames entières.** Pour 08/000000 et 08/000200, la projection applique les parts mesurées sur le disque emboîté de 8k sites (53,6 % et 36,6 % pour s02 à h = 0,5 / 1 m). Or le reçu du développeur lidar_scaling_local_20260923 montre que, de 8k à la trame entière, les paires développées sont multipliées par 6,5 (s00 K5) et 12,3 (s02 K5), et les core_sites par 10,6 et 23. Les émissions q3/q4 ne sont multipliées que par 3,6 et 4,9. Cette croissance porte presque entièrement sur les ancres longues.

**Mesure directe sur la trame entière 08/000200 K5** (45 845 sites, chaîne 93066733 instrumentée, un fil, `nice 19`, 137 s CPU). Les compteurs égalent le reçu : 22 722 345 paires, 348 530 318 core_sites et 1 407 885 boules, ce qui égale aussi R7b.
- **Part globale** pour h = 0,5 / 1 / 2 m : 79,0 / 70,1 / 56,9 %, au lieu de 53,6 / 36,6 / 25,3 %.
- **Écart relatif** : +47 % et +92 %. L'incertitude annoncée de ±25 % est donc réfutée.
- **Chaîne D3 idéale sur 000200 K5** : 5,07 s (h = 1 m) et 5,50 s (h = 0,5 m), au lieu de 3,46 et 4,27 s. Le gain est ×1,18–1,28, au lieu de ×1,52–1,88.
- **Grandes boules** : celles de rayon > 0,5 m font 11,0 % du catalogue, et 4,4 % au-delà de 1 m.

**Biais secondaires :**
1. **Horloge par arête (pessimiste pour D3).** `clock_gettime(CLOCK_THREAD_CPUTIME_ID)` est un appel système sur cet hôte : une fenêtre vide lit 0,99–1,03 µs, contre 28 ns avec MONOTONIC. Chaque arête et chaque rectangle chronométrés portent donc environ 1 µs de sonde. Le coût d'une arête longue passe de 2,8–5,3 à 1,8–4,3 µs, et les parts baissent de 2 à 7 points. Ce biais est en partie compensé par le travail réel non chronométré (front, boucle, environ 1–3,5 s sur un fil), qui est surtout global.
2. **Extrapolation K10 de s01 (optimiste).** La baisse observée entre K5 et K10 à h = 0,5 m est de 2,7–3,0 points, pas de 5. La part est donc d'environ 76 % et non 72 %, soit environ 8,3 s au lieu de 8,1 s.
3. **Petites boules q2 omises (pessimiste).** La projection garde toute la phase q2 (0,23–0,82 s) dans les « autres postes », alors que D3 localise aussi ses petites boules. L'effet est au plus de quelques dixièmes de seconde.
4. **Temps G4 supposé proportionnel au CPU d'un fil.** La q34 G4 n'occupe qu'environ 43 % des 48 fils sur la chaîne, et la répartition du temps réel entre local et global n'est pas mesurée.

Aucun de ces biais ne renverse la conclusion.
PREUVES: ["Mesure directe, trame entière 08/000200 K5 (fichier /tmp/claude-1000/-workspaces-E-HGP/71988aca-a49a-4d33-96db-f05468331005/scratchpad/design/d3_refut/attrib_scene_02_full_k5.json, binaire d3_attrib de l'auditeur, 181 s réelles / 137 s utilisateur). Arêtes > 1,6 m : 93,9 % des paires, rejet par les témoins 94,4 %, 5,9 % des émissions, 70,1 % du CPU q3/q4 (66,3 % après correction de l'horloge). Parts globales pour h = 0,25 / 0,5 / 1 / 2 m : 84,9 / 79,0 / 70,1 / 56,9 %, soit 83,0 / 76,3 / 66,3 / 51,3 % avec δ = 1 µs. Gains plafonds q3/q4 : ×1,27 / ×1,43 / ×1,76 (brut).", "Recoupement des compteurs de cette mesure : 22 722 345 paires développées, 348,5 M core_sites, 307,9 M cover_sites et 338,0 M tests d'atlas, égaux à piece_full de s02_k5 dans le reçu lidar_scaling_local_20260923 ; 1 407 885 boules, égal à R7b probe_16–19.", 'Reçu lidar_scaling_local_20260923/out, de 8k à la trame entière. Paires développées : 3,65 → 23,69 M (s00 K5), 1,84 → 22,72 M (s02 K5), 4,95 → 30,78 M (s00 K10), 3,24 → 32,79 M (s02 K10). Émissions q3/q4 multipliées par 3,4–5,0 seulement. Temps q34 à 8 fils multiplié par 4,6 (s00 K5), 8,2 (s02 K5), 4,9 (s00 K10) et 7,9 (s02 K10).', "Microbanc /tmp/claude-1000/-workspaces-E-HGP/71988aca-a49a-4d33-96db-f05468331005/scratchpad/design/d3_refut/clk.cpp (EPYC 7763). Une fenêtre CLOCK_THREAD_CPUTIME_ID vide lit 987–1030 ns, contre 28 ns pour MONOTONIC. La copie wspd_q34_probe.cpp chronomètre chaque arête (edge) et chaque filtre de rectangle avec cette horloge. Dans les sept exécutions de l'auditeur, le temps q34 non chronométré vaut 1,58–2,24 µs par arête.", "Recalcul avec δ = 1 µs par fenêtre (recompute.py). Parts à h = 0,5 m : 57,3 → 52,2 (s00 K5 8k), 73,5 → 69,2 (s01 8k), 78,9 → 76,6 (s01 32k), 53,6 → 49,9 (s02 K5), 54,6 → 51,9 (s00 K10), 50,6 → 48,4 (s02 K10). Coût d'une arête longue : 1,8–4,3 µs au lieu de 2,8–5,3.", "Modèle minorant pour les trames non mesurées (fullframe_model.py), avec un CPU local qui croît comme les émissions. Calibration s01 de 8k à 32k : le CPU des arêtes ≤ 0,8 m croît ×2,29 et les émissions ×3,83, donc le modèle minore la part globale. Parts minimales sur trame entière à h = 0,5 / 1 m : s00 K5 66,7 / 52,7 % ; s00 K10 65,0 / 48,6 % ; s02 K10 68,6 / 56,3 %. Sur s02 K5, le modèle donne 72,6 / 62,6 % et la mesure 79,0 / 70,1 % : c'est bien un minorant.", "Chiffres R7b vérifiés dans SUMMARY.json (moyennes MEB ON), q34 en secondes : 4,065 / 2,595 / 4,795 à K5 et 8,20 / 5,27 / 9,825 à K10. Chaînes : 5,63 / 3,74 / 6,505 à K5 et 13,92 / 9,58 / 15,29 à K10. L'arithmétique de la table de la proposition est juste ; seules les parts injectées sont fausses.", 'Extrapolation K10 de s01 : baisse observée entre K5 et K10 à h = 0,5 m de −2,7 point (s00 : 57,3 → 54,6) et −3,0 (s02 : 53,6 → 50,6) ; à h = 1 m, −6,1 et −5,5. La proposition applique environ −5 points aux deux seuils.']
CORRECTION: **Tableau de projection corrigé** (idéal : local gratuit et recouvert ; global = part × q3/q4 du reçu R7b). Pour chaque trame, les deux valeurs sont h = 1 m / h = 0,5 m.

| Trame | K5 | K10 |
| --- | ---: | ---: |
| 08/000000 | ≥ 3,7 s / ≥ 4,3 s (au lieu de 3,17 / 3,89) | ≥ 9,7 s / ≥ 11,1 s (au lieu de 8,5 / 10,2) |
| 08/000100 | environ 2,8–2,9 s / 3,1–3,2 s (inchangé) | environ 7,5 s / environ 8,3 s (au lieu de 8,1) |
| 08/000200 | 4,9–5,1 s / 5,4–5,5 s (mesuré ; au lieu de 3,46 / 4,27) | ≥ 11,0 s / ≥ 12,2 s (au lieu de 8,5 / 10,4) |

**Fourchettes corrigées.**
- **Temps de chaîne** : K5 2,8–5,5 s et K10 7,5–12,2 s, au lieu de 2,9–4,3 et 7,5–10,4 s.
- **Gain idéal de chaîne** : ×1,2–1,5, au lieu de ×1,2–1,9 ; il vaut ×1,18–1,33 sur 000200 K5.
- **Gain plafond q3/q4 sur trame entière** : ×1,27–1,31 à h = 0,5 m et ×1,43–1,51 à h = 1 m.
- **Barre d'erreur** : il faut retirer « ±25 % sur les parts » et écrire que les parts 8k sous-estiment de 25 à 34 points la part d'une trame entière pour s02.
- **Titre** : remplacer « 51 à 79 % » par « 66 à 79 % du CPU q3/q4 sur trame entière à h = 0,5 m (8k : 48–79 %) ».
- **Coût d'une arête longue** : 1,8–4,3 µs, et non 2,8–5,3 µs.

**Méthode.** Mesurer les parts sur les trois trames entières, ou au minimum sur la plus grande coupe disponible, jamais à 8k. Chronométrer par lot (rdtsc, ou horloge prise par job ou par rectangle) plutôt qu'avec CLOCK_THREAD_CPUTIME_ID par arête. Compter à part le front WSPD global, et créditer aussi la phase q2 locale.

**Conclusion.** Celle de la proposition est renforcée. Sur 000200 K5, les ancres > 1,6 m pèsent 66–70 % du CPU q3/q4 pour 5,9 % des émissions. Le seuil de réouverture (« ancres longues sous environ 25 % ») est donc encore plus loin qu'annoncé, et l'effort doit aller vers une réfutation des ancres longues sans expansion (famille B).

===== refute-math:D3_echelles tient_avec_reserves
Je n'ai trouvé aucun contre-exemple à l'égalité ensembliste. Le lemme A est juste, et la v9 l'applique bien à l'arête que le code retient comme propriétaire : c'est la plus longue de toutes les arêtes du support (q4_local.cpp:27-35 compare les 6 arêtes, wspd_q34.cpp:722-726 fait de même en q3). Les présentations sont strictement centrées (triangle strictement aigu, q4 positif), et l'inégalité est donc même stricte. Le lemme B est juste aussi : l'appartenance au catalogue (p, coquille, q_min, positivité) ne lit que P∩B̄. Enfin, le générateur ne refuse rien de lui-même : le seul refus de dégénérescence, coquille > 12, est levé par le recensement global (tower_chain.cpp:531), et un halo ne peut donc pas en créer de faux.

Trois étapes de la preuve sont en revanche fausses telles qu'écrites, et elles portent sur la réalisation L1/L2.

(1) L'étape (ii) dit que « la propriété de l'ancre ne lit que P∩B̄ ». C'est faux dès que la plus longue arête est ex aequo. La v9 départage alors par edge_key sur les identifiants (wspd_q34.cpp:722-726, q4_local.cpp:34), et la règle canonique q4 `y<x` (q4_local.cpp:454) lit elle aussi l'ordre des identifiants. Ces égalités ne sont pas théoriques sur LiDAR 1 mm. Sur 08/000000 8k K5 avec h = 0,5 m, mon oracle trouve 183 287 présentations q3 du catalogue de rayon ≤ h (la v9 compte 185 251 supports q3 tous rayons, donc cohérent). Parmi elles, 16 ont une plus longue arête ex aequo, et 12 ont des extrémités a_min distinctes, donc potentiellement dans deux tuiles différentes (exemples : ids 596/633/636, 1679/1923/2144, 1910/2069/2180). Deux cas se présentent si E_T est renuméroté sans préserver l'ordre global (propres puis halo, ordre kd, Morton local en coordonnées relatives comme en L2) et que les arêtes sont élaguées par propriété avant génération :
- **numérotation « propres puis halo »** : les deux tuiles gardent la même présentation, ce qui produit un doublon ;
- **ordre local arbitraire** : chaque tuile peut choisir l'autre arête ex aequo, et la boule est perdue.

(2) L'étape (iv) et le § 3 disent que « la fusion par clé absorberait un doublon ». C'est faux. La fusion ne réunit que des présentations **distinctes** d'une même clé. Une présentation identique émise deux fois déclenche `chain_duplicate_presentation` (tower_chain.cpp:150), donc invariant_violated.

(3) La proposition dit qu'« une erreur de halo ferait échouer la chaîne ». C'est faux pour une clé manquante. Le recensement ne contrôle que les clés émises (tower_chain.cpp:520-538). Un halo qui perd un sommet du support supprime la boule sans rien déclencher ; seuls l'invariant d'Euler ou une comparaison catalogue contre catalogue la verraient.

Réserve mineure sur les fixtures : le cas d'égalité du lemme A exige un simplexe régulier. Dans Z³, un triangle équilatéral ou un tétraèdre régulier a une arête² = 2t², donc r² = 2t²/3 ou 3t²/4, jamais un carré entier h². La fixture « ancre de carré exactement 8h²/3 » ne peut donc réaliser que r < h, et 8h²/3 n'est entier que si 3 divise h.
PREUVES: ['Lemme A contre le code : q4_local.cpp:27-35 (owned() compare les 6 arêtes à |ab|², ex aequo départagés par minmax des ids) ; wspd_q34.cpp:722-726 (même règle en q3) ; exact_ball.hpp:25 (triangle strictement aigu) et q4_local.cpp:450-452 (q4 positif) donnent un centre strictement dans relint conv S, donc |ab|² > 2qr²/(q-1).', "Lemme B et appartenance au catalogue : le catalogue est défini par p + q_min ≤ Kmax+1, que le recensement exige (tower_chain.cpp:540 chain_ball_outside_rank_window, 538 q_min = arité minimale présentée). Tous ces prédicats ne lisent que P∩B̄, et tout site de B̄ est à distance ≤ 2r ≤ 2h de chaque extrémité de l'ancre.", 'Pas de refus parasite créé par un halo : le seul refus de dégénérescence, coquille > 12, est levé par le recensement global sur les clés émises (tower_chain.cpp:531). Les throw du générateur (gen/lanes) sont des invariants ou des bornes arithmétiques, pas des refus de géométrie.', "La propriété dépend de l'ordre des identifiants : départage par edge_key (wspd_q34.cpp:405, 722-726 ; q4_local.cpp:34) et règle canonique q4 `y<x && acute(a,b,y)` (q4_local.cpp:454), puis un seul emit par groupe cosphérique (q4_local.cpp:460). prepare_cloud conserve l'ordre de l'appelant (prepared_cloud.cpp:39) et la chaîne prend PointId = rang d'entrée (tower_chain.cpp:437-440).", 'Oracle exact indépendant /tmp/claude-1000/-workspaces-E-HGP/71988aca-a49a-4d33-96db-f05468331005/scratchpad/design/d3_refut_juge/tie_oracle.cpp (i128, centre circonscrit rationnel, recensement strict/coquille exact), sur s00_k5_s8_w8_r0_nested_8000, h = 500 mm, Kmax = 5, 87 s CPU, nice 19, un seul fil : 56 597 869 triangles strictement aigus de rayon ≤ h, 183 287 présentations q3 du catalogue (≤ 185 251 supports q3 de la v9, cohérent), 16 avec plus longue arête ex aequo, dont 12 avec a_min différent entre les deux arêtes.', "Doublon fatal : tower_chain.cpp:148-150 exige `support différent ou arité différente` au sein d'une clé, sinon chain_duplicate_presentation, donc kInvariantViolated. Avec une numérotation « propres puis halo », l'analyse du cas isocèle (apex a, avec b de T_b et x de T_x au-dessous de a dans l'ordre global) montre que chaque tuile se choisit elle-même propriétaire et garde la présentation {a,b,x}, d'où ce doublon.", 'Perte silencieuse possible : le recensement ne vérifie que les clés émises (tower_chain.cpp:520 depth, 521 shell, 538 q_min). Aucune porte de la chaîne ne détecte une clé absente.', "Fixture non réalisable : l'égalité du lemme A exige σ = 1/q avec produits scalaires égaux, donc un simplexe régulier. Les simplexes réguliers de Z³ ont une arête² = 2t², donc r² = 2t²/3 (q3) ou 3t²/4 (q4), et ni 2t² = 3h² ni 3t² = 4h² n'ont de solution entière non nulle. r = h exact reste réalisable en q2 (a = 0, b = (2h,0,0)) et en q3 non extrémal, par exemple (5,0,0), (-3,4,0), (-3,-4,0) mis à l'échelle."]
CORRECTION: 1. **Ordre des identifiants.** Ajouter comme prémisse explicite du théorème et de L1/L2 que tout départage de la v9 et a_min lisent le même ordre GLOBAL. Deux façons de le garantir :
   - **(a) E_T transmis à la v9 dans l'ordre croissant des identifiants globaux.** prepare_cloud conserve l'ordre de l'appelant, et une renumérotation monotone préserve la comparaison lexicographique de (min,max) ainsi que la règle `y<x`.
   - **(b) Le noyau de tuile (L2 compris) porte le PointId global pour les départages**, même en coordonnées relatives.

   Remplacer « rang spatial minimal » par cet ordre, ou par tout ordre global fixé une fois pour toutes.

2. **Élagage des arêtes.** N'élaguer une arête par propriété de tuile qu'après avoir calculé son propriétaire avec les identifiants globaux. À défaut, générer toutes les arêtes courtes de E_T et filtrer les émissions en recalculant le propriétaire global à partir du support.

3. **Doublons.** Retirer l'argument « la fusion absorbe un doublon ». La disjonction doit être exacte, car un doublon de présentation est fatal (chain_duplicate_presentation). Ajouter une porte à code dédiée.

4. **Porte de complétude obligatoire.** Une clé absente est silencieuse. Imposer, comme porte de D3 sur les coupes 8k/16k/32k :
   - l'invariant d'Euler de l'auditeur C (n·[K=1] + Σ e_K = 1 pour K ≤ Kmax−2) ;
   - la comparaison catalogue contre catalogue D3 / v9 globale (même ensemble de clés, mêmes p et coquilles).

5. **Fixtures à graver.**
   - Un triangle q3 strictement aigu, isocèle, à plus longue arête ex aequo, dont l'apex n'est pas l'identifiant minimal, avec les deux a_min dans deux tuiles différentes. Reprendre par exemple les ids 596/633/636 de s00 8k.
   - Un mutant « renumérotation spatiale locale de E_T », qui doit être tué (doublon ou violation d'Euler).
   - Remplacer la fixture « |ab|² = 8h²/3 avec r = h », irréalisable dans Z³, par trois fixtures : r = h exact en q2, r = h exact en q3 non régulier (cercle de réseau), et |ab|² = 8h²/3 exact avec 3 | h, où r < h.

Avec ces prémisses, la réduction tient sous la condition déjà déclarée : la complétude arête par arête de la v9, non inscrite au registre.

===== refute-cout:D1_continuite refute
**Poste oublié principal : le reste de FULL, qui ne profite pas des fils.**
- Ce reste regroupe la validation (passe 2 en série), les populations (en série), les images verticales et l'encodage (au plus Kmax tâches) et la banque. Aucun levier de D1 ne le vise : L5 ne traite que la phase A, L6 les cibles statiques, L8 la plomberie hors tour.
- Local W8, 000000/K10, sur 22,0 s : cibles statiques 15,3 s, lots 2,8, validation 1,8, populations 0,7, images 0,6, finition 0,8.
- Différentiel MEB : il retire 5,8 s en local (tour de 24,7 à 18,9 s), et 0,31 à 0,43 s sur G4 (R7b 000000/K10, tour de 4,37–4,40 s à 3,97–4,06 s). Les cibles statiques vont donc environ 15× plus vite sur G4 qu'en local. Le reste de FULL ne va qu'environ 2× plus vite : 3,4 s sur G4 pour 6,7 s en local.
- Répartition sur G4 qui en découle :
  - 000000/K10 : statiques ≈ 0,6 s, lots ≈ 1,4 s, reste ≈ 2,0 s ;
  - 000100/K10 (× 0,8) : statiques ≈ 0,48 s, lots ≈ 1,13 s (cohérent avec le « 1,2 s » de D1), **reste ≈ 1,6 s** ;
  - K5/000100 : reste ≈ 0,37 s.
- Recoupement : FULL ne gagne que ×1,02 à ×1,12 entre W24 et W48.
- Conséquences :
  - D1 met ce reste à 0,03–0,5 s dans son FULL GPU. Son plafond CPU de FULL (1,5–1,9 s à K10/000100) suppose le reste ≤ 1,1–1,5 s.
  - Le FULL GPU de D1 vaut en réalité au moins 1,05–2,2 s à K10/000100, et non 0,25–0,88 s.
  - Le total GPU à K10 devient 1,4–3,8 s (000100) et 1,9–4,8 s (000200). La probabilité 0,2 pour K10 < 1 s ne tient pas : elle vaut à peu près 0 dans D1.

**Fautes secondaires.**
1. **L3 et l'hyperthreading.** L3 compte 44 fils logiques à CPU·s constant, alors que la G4 n'a que 24 cœurs physiques. R5 (000000/K10) : W24 donne 21,59 s pour 299,7 CPU·s, W48 donne 17,37 s pour 424,8 CPU·s. C'est +42 % de CPU·s pour un gain de ×1,24 seulement ; q3/q4 passe de 12,15 à 8,35–8,57 s. Un fil SMT ajouté rapporte environ 0,26 cœur. Selon le profil d'inactivité (traîne ou occupation uniforme) :
   - L3 gagne ×1,33 à ×2,0 à K5/000200, et non ×1,95 ;
   - ×1,17 à ×1,55 à K10/000200, et non ×1,43.
2. **W_plat incomplet.** Il omet environ 2,68 G tests à K10/000100 (+41 %) et 4,0 G à K10/000200 : tests ponctuels d'atlas (1,31 G), bornes de cover du cœur, bornes de blocs d'atlas, tests des graines q3, événements de balayage q4. Effet modeste : +0,01 à 0,07 s.
3. **Phase A.** 2,18 / 1,43 = 1,52 s, et non 1,2 s. Les 2,18 s ont été mesurées sur 000000, pas sur 000100. Le rapport 1,43 compare des CPU·s W8 locales à des CPU·s W48 G4 ; CONTRE_AUDIT_A interdit d'en tirer une vitesse par fil.
4. **Plomberie hôte–GPU.** D1 la chiffre à 0,02–0,15 s, sans mécanisme ni mesure. Or les deux seules mesures device du dépôt donnent des noyaux à 2–4 % de leur étage :
   - v6 : 154 ms de noyaux pour 7 717 ms d'étage ;
   - v7 : 189 ms de noyaux pour 4 540 ms, dont 2 932 ms de reconstruction hôte, soit 137 ns par boule sur un seul fil.
5. **Calibration de D_arbre.** Elle repose sur le noyau le plus favorable au GPU : un fil par boule, index en lecture seule, sortie de taille fixe, deux passes, uniforme u16 50k. Le ratio de 89 nœuds par boule n'a jamais été mesuré sur ce noyau.

Les conclusions qualitatives tiennent et se renforcent : pas de 1 s en CPU seul, 100 ms hors de portée. Ce sont les chiffres de D1 qui sont trop optimistes, de façon systématique.
PREUVES: ["Code FULL /workspaces/E-HGP/build/v9-audit-c-publish/morsehgp3D_v9/src/tower/forest/full_ball_tower.hpp : run() l. 343–353 (validate_catalogue en tête) ; run_orders_parallel l. 488–549 (cibles statiques séquentielles entre ordres K ; lots et images en parallel_items(kmax) ; assign_populations en série) ; finish l. 412–450 (banque, encodage parallèle par K) ; validate_catalogue l. 922 et suivantes (passe 1 parallèle, passe 2 en série dans l'ordre des index).", 'Ventilation locale W8 sur 000000/K10, /workspaces/E-HGP/build/v9-audit-c-publish/audits/COORDINATION_MORSEHGP3D_V9.md l. 606–610 : 22,0 s = statiques 15,3 + validation 1,8 + lots 2,8 + populations 0,7 + images 0,6 + finition 0,8. Même fichier l. 622 : le MEB proposé fait passer la tour locale de 24,7 à 18,9 s.', 'Reçu R7b, /workspaces/E-HGP/build/v9-audit-c-publish/morsehgp3D_v9/receipts/g4_tower_r7b_20260923/vm/probe_4..7.stdout : tour 000000/K10 à 4,372 / 4,397 s MEB OFF contre 4,062 / 3,969 s MEB ON, soit −0,31 à −0,43 s sur G4 contre −5,8 s en local. probe_12 : K10/000100 FULL 3,199 s, q3/q4 5,226 s. probe_8 : K5/000100 FULL 0,734 s. probe_16 : K5/000200 FULL 0,969 s.', 'Reçu R5, morsehgp3D_v9/receipts/g4_tower_r5_20260923/vm/probe_12 (W24) contre probe_1 et probe_7 (W48), sur 000000/K10 : chaîne 21,586 s / 299,7 CPU·s contre 17,369 s / 424,8 CPU·s ; q3/q4 12,148 s contre 8,572 / 8,354 s ; tour 5,890 s contre 5,368 / 5,264 s. Voir aussi CONTRE_AUDIT_B_G4_R5_20260923.md l. 63 et AUDIT_C_OBJET_ET_RECONSTRUCTION_TOUR_20260923.md l. 482–484 (FULL ×1,02 à ×1,12 de W24 à W48). lscpu R7b : AMD EPYC 9B45, 24 cœurs × 2 fils.', 'Ledger v12, /workspaces/E-HGP/build/v9-audit-c-publish/morsehgp3D_v9/receipts/lidar_scaling_local_20260923/out/s0{1,2}_k{5,10}_w8_r0/*_piece_full.json. La liste plate de D1 redonne exactement 1,91 / 2,97 / 6,52 / 10,25 G. Restent hors de W_plat à K10/000100 : atlas_point_tests 1,313 G, core_cover_bound_tests 0,637 G, atlas_block_bounds 0,251 G, core_cover_point_tests 0,144 G, q3_seed_bound_tests + q3_seed_point_tests 0,176 G, q4_sweep_events 0,114 G, dead_point_tests + dead_core_point_tests 0,042 G. Total ≈ 2,68 G (+41 %) ; ≈ 4,0 G à K10/000200.', "Phase A : 2 179,7 ms CPU pour l'ordre K10, mesurés sur le catalogue 000000 et non 000100 (/workspaces/E-HGP/build/v9-audit-c-publish/morsehgp3D_v9/audits/PHASE_A_GRAPHE_TEMPOREL_20260923.md). 2,18 / 1,43 = 1,52 s. CONTRE_AUDIT_A_MESURES_PLAN_20260922.md l. 20 interdit de convertir un rapport de CPU·s entre W48 G4 et une W locale en vitesse par fil.", "Plomberie GPU : /workspaces/E-HGP/build/v9-audit-c-publish/morsehgp3D_v7/docs/RESULTATS_TOUR_CACHE_G4_20260910.md l. 32–36 (189,346 ms de noyaux pour 4,540 s de phase : préparation 904,9 ms, reconstruction hôte 2 931,9 ms, 21 468 368 boules). morsehgp3D_v6/docs/GPU.md l. 219–222 (154 ms de noyaux pour 7 717 ms, 88 % d'hôte sur un seul fil). morsehgp3D_v9/docs/audit_v8/12_parallelisme_gpu_perf.md l. 257 (critère d'arrêt : code hôte ≤ moitié de l'étage GPU).", "Calibration GPU : /workspaces/E-HGP/build/v9-audit-c-publish/morsehgp3D_v7/src/gpu/census_kernels.cuh l. 119–245. k_prefilter et k_census font un parcours en profondeur par fil et par boule, index en lecture, sortie fixe de 21 cases, deux passes, sur un nuage uniforme u16 50k. Aucun compteur de nœuds n'est publié : les 89 nœuds par boule viennent du recensement CPU v9 LiDAR (391,7 M nœuds pour 4,38 M boules)."]
CORRECTION: **1. Remplacer les chiffres de D1.**

CPU seul, 48 fils, tous leviers D1 :

| cas | D1 | corrigé |
| --- | ---: | ---: |
| K5/000100 | 1,9–2,4 s | 1,9–2,9 s |
| K5/000200 | 2,8–3,5 s | 2,8–4,7 s |
| K10/000100 | 6,0–7,5 s | 6,2–8,2 s |
| K10/000200 | 8,2–10,1 s | 7,7–11,9 s |

GPU complet, plomberie hôte non chiffrée :

| cas | D1 | corrigé |
| --- | ---: | ---: |
| K5/000100 | 0,22–0,84 s | 0,42–1,10 s |
| K5/000200 | 0,31–1,13 s | 0,59–1,48 s |
| K10/000100 | 0,63–2,47 s | 1,4–3,8 s |
| K10/000200 | 0,86–3,32 s | 1,9–4,8 s |

La valeur centrale à K5 passe d'environ 0,43–0,59 s à environ 0,7–0,9 s.

**2. Probabilités subjectives.**
- K10 sous 1 s dans D1 : environ 0, et non 0,2. Il faudrait refondre la validation, les populations, les images et l'encodage, ce qui sort de D1.
- K5 sous 1 s : environ 0,3–0,5 sur une trame du type 000100, moins sur 000200.
- 100 ms : inchangé, hors de portée, et la conclusion se renforce.

**3. Ajouter explicitement un levier L9, « reste de FULL », avec sa propre cible.**
- Il couvre la passe 2 de validation, les populations en série, les images et l'encodage (au plus Kmax tâches) et la banque.
- Il vaut environ 1,6 s sur G4 à K10/000100 et environ 0,37 s à K5/000100.
- Sans ce levier, tout FULL projeté sous 1 s à K10 est faux.

**4. Remplacer les inférences par des mesures, avant tout portage.**
- Utiliser la sonde v13 (c768e06a), qui publie tower_phases_ms et q34_occupancy (cpu_sum_s, wait_sum_s).
- Mesurer la ventilation de FULL sur G4, ou au moins en W8 local, sur les trames entières.
- Mesurer l'occupation réelle de q3/q4 : ne plus déduire la CPU G4 par étape de la part W1 à 8k.
- Borner L3 par 24 cœurs physiques plus un gain SMT mesuré (environ 0,26 cœur par fil ajouté d'après R5), et non par 44 fils logiques.

**5. Chiffrer la plomberie GPU au pire.**
- Prendre au pire le critère d'arrêt v8 (hôte ≤ 50 % de l'étage GPU), ce qui double la part q3/q4 GPU.
- Exiger une reconstruction hôte parallèle. En v7 elle coûtait 137 ns par boule sur un seul fil, soit environ 0,6 s pour 4,38 M boules.

**6. Petites corrections.**
- Phase A : 1,52 s sur 000000, et non 1,2 s.
- Compléter W_plat avec les 2,7 à 4,0 G tests omis.

**7. Ce que D1 garde.** Sa recommandation d'ordre qualitatif reste valable : D1 comme référence exacte, pas de 1 s en CPU seul, falsification bornée avant le portage GPU de q3/q4. Mais son rôle de « porteur de la tour FULL commune » doit inclure L9. Sinon, ce reste de FULL plafonne aussi toute famille alternative de générateurs à environ 1,5 s à K10.

===== refute-math:D1_continuite tient_avec_reserves
Aucun contre-exemple au catalogue. Sur 1 562 nuages adverses (775 566 boules attendues), comparés clé par clé à des oracles exhaustifs exacts, la chaîne v9 publiée ne présente aucun écart : ni boule manquante, ni boule en trop, ni p, q_min ou u faux. Les refus explicites (coquille > 12) tombent exactement là où l'oracle voit une coquille > 12 dans le catalogue.

Le critère p + q_min ≤ Kmax+1 est juste, dégénérescences comprises. Nerf du lien local : pour m ≤ q_min−2, A_m = ∪_{|T|=m} ∩_{v∈T}{x·v>0} est une union d'ouverts convexes non vides dont le graphe de Johnson des T est connexe, car |T∪T'| = m+1 ≤ q_min−1 donne 0 ∉ conv (Gordan). A_m est donc connexe et π0 ne change pas avant K = p + q_min − 1.

Les failles sont de trois ordres.

(1) Ce n'est pas un théorème. Deux maillons restent conditionnels ou en revue (Q34_PROPRIETAIRE_PASSAGE_AMONT : « preuve conditionnelle de programme » ; Q4_INDUCTION_ATLAS_EVENEMENTS). D1 le dit lui-même.

(2) « D1 ne change rien / le GPU ne change que l'ordonnancement » est faux pour ses propres leviers :
- L1 (certificat de bloc avant A×B) et L2 (cœur compté par nœuds) sont de nouveaux certificats d'élagage, chacun avec ses obligations d'exactitude : sites strictement intérieurs, distincts, crédits non additionnables entre cellules. Il y a aussi le piège des 32 coins, qui majorent F et ne le minorent pas : a=(0,0,0), b=(4,0,0), Z=[0,4]²×{0}, F ≥ 0 aux coins mais F((2,0,0)) = −16.
- L5 (lemme du maximum d'ID) ne couvre que la voie statique, pas la voie séquentielle à jetons.
Aucune de ces obligations ne figure dans la section complétude de D1.

(3) Le juge global d'échelle a un angle mort structurel, que D1 sous-estime quand il écrit « changer de générateur sous juge global ».
- Euler pour K ≤ Kmax−2 ne voit aucune boule de p ≥ Kmax−2. Sur 08/000000 8k, c'est 112 258 / 342 181 boules (32,8 %) à K5 et 191 398 / 1 567 942 (12,2 %) à K10, et 12,6–12,7 % à K10 sur 000100/000200.
- Le mutant publié q3_atlas_rejects_at_k_minus_2 retire 2 502 boules à K5 avec E_1..E_3 = 1. Seule la restriction du catalogue K7 le tue.
- Ce protocole Kmax+2 est impossible à K10. La chaîne refuse kmax > 10 (tower_chain.cpp:290, kBallInteriorMax = 9). À K10, les ordres 9 et 10 n'ont donc aucun juge d'échelle autre que l'égalité avec la référence CPU.
- Même dans la fenêtre vérifiée, des omissions peuvent se compenser. La contribution générique d'une boule s'écrit x^{p+1}(x−1)^{q−1}. Omettre une q2 de profondeur p et une q3 à chaque profondeur p, p+1, …, p+T laisse E_K intact pour tout K ≤ p+T+1.
- En revanche, aucune omission générique ne se compense sur tous les K : C2 + (x−1)C3 + (x−1)²C4 = 0 à coefficients ≥ 0 force C2 = C3 = C4 = 0.
PREUVES: ['Harnais et oracles (hors dépôt), dans /tmp/claude-1000/-workspaces-E-HGP/71988aca-a49a-4d33-96db-f05468331005/scratchpad/design/jure_D1_completude/ :\n- cat_dump.cpp et oracle_cmp.py : oracle rationnel Python, toutes les parties à 2, 3 ou 4 sites, barycentriques > 0, dédoublonnage (centre, r²), q_min, filtre p+q_min ≤ Kmax+1 ;\n- med_judge.cpp : oracle exhaustif en entiers i128, formules de circumcentre q3/q4, positivité par Cramer sur la Gram ;\n- campaign.py, med_campaign.py, lidar_windows.py : lanceurs des campagnes.\nTout est lié aux bibliothèques publiées build_93066733 (libmhgp9_chain.a, libmhgp9_gen.a), leviers par défaut, s = 8, run_tower = 1, keep_catalogue = 1.', 'Petits nuages, oracle rationnel. 1 240 nuages de n ≤ 16 sites, en huit familles :\n- grille 0..4 et son image en similitude 18 bits ;\n- lignes de balayage colinéaires ;\n- plans (z constant) et deux plans ;\n- nuage entièrement colinéaire ;\n- amas plus sites aux coins 0 et M = 262143 ;\n- sites de sphères entières (R² = 9…29).\nKmax ∈ {2, 3, 5, 10}, W1 ou W2, 66 907 boules attendues. Résultat : 0 écart, 14 refus corrects (coquille > 12).', "Nuages moyens, oracle entier. 263 nuages jusqu'à n = 153 : murs à rangées de balayage, uniformes, amas épars (grandes boules à peu d'intérieurs), blob plus lointains, sous-réseaux, droites 3D. La chaîne tourne sur l'image off + f·z, jusqu'à 18 bits pleins ; la similitude préserve coquilles, p et q_min. 421 039 boules : 0 manquante, 0 en trop, 0 fausse, 0 débordement i128, 0 écart entre le recensement de la chaîne et le mien ; 2 refus corrects.", 'Fenêtres LiDAR réelles. 59 fenêtres de 52 à 140 sites prises dans scene_00/01/02_full.u32le (grille 1 mm, sans sol), Kmax 5 ou 10, identité ou image 18 bits. 287 620 boules, 0 écart.', "Contrôle négatif du comparateur : ORACLE_KMAX_SHIFT=1 sur 60 sites uniformes fait voir 791 boules manquantes, donc le juge n'est pas vert par vacuité.", 'Dépense : environ 220 CPU·s en nice 19, un fil par processus, aucune commande Git ni GCP. GCP non utilisé.', "Contrôle mathématique du critère q_min. À l'ordre K = p+m, le lien local d'un centre de coquille de directions V est A_m = ∪_{|T|=m} H(T). Pour m ≤ q_min−2 :\n- chaque H(T) est non vide, puisque 0 ∉ conv T (Gordan) ;\n- les T successifs du graphe de Johnson vérifient |T∪T'| ≤ q_min−1, donc H(T∪T') ≠ ∅ ;\n- une union d'ouverts convexes à nerf connexe est connexe.\nDonc pas de naissance ni de fusion avant K = p+q_min−1, et p + q_min ≤ Kmax+1 est exactement la condition nécessaire et suffisante pour π0 jusqu'à Kmax.", 'Bornes du lemme amont revérifiées :\n- R² = Σλiλj|pi−pj|² ≤ D(1−Σλi²)/2, soit ≤ D/3 en q3 et ≤ 3D/8 en q4 ;\n- |2(z−m)·s| ≤ √(Ξ/3) en q3 et √(Ξ/2) en q4 ;\n- cover de rayon √D : (√3+1)√(D/8) < √D ;\n- F = |2z−a−b|² − 4|a−b|² ∈ [−12M², 12M²], moins de 2^40, donc tient en i64.']
CORRECTION: 1) Titre et statut. Parler d'« énoncé de complétude conditionnel », pas de théorème, tant que Q34_PROPRIETAIRE_PASSAGE_AMONT et Q4_INDUCTION_ATLAS_EVENEMENTS ne sont pas clos au registre.

2) Remplacer « D1 ne change rien / le GPU ne change que l'ordonnancement ». Proposition : « l'objet est inchangé ; chaque levier qui élague ou réorganise une décision porte sa propre obligation d'exactitude ». Cette obligation couvre :
- L1 et L2 : crédit seulement de sites distincts et strictement intérieurs, en antichaîne disjointe ; pas d'addition entre cellules ; pas d'exclusion par les coins, car ils majorent F sans le minorer ;
- L5 : voie statique seulement, ou preuve pour la voie à jetons ;
- L6 : la proposition GPU reste une proposition, et proposals = verified + fallbacks est publié.
Chaque levier doit en outre fournir une fixture gravée, un mutant causal tué et l'égalité clé par clé du catalogue et du condensé de tour sur les coupes 8k, 16k et 32k avant d'entrer dans le plafond chiffré.

3) Juge d'échelle.
- À K5 : rendre obligatoire le protocole Kmax+2, c'est-à-dire Euler jusqu'à K5 sur un catalogue K7 et restriction égale clé par clé au catalogue K5, pour tout générateur ou port nouveau.
- À K10 (K12 hors représentation) : l'égalité octet par octet avec la référence CPU sur les trames du contrat est la seule garantie des ordres 9–10 et des 12–13 % de boules de p ≥ 8. Elle devient une porte permanente, pas une précaution transitoire. On peut aussi ajouter un mode d'audit « comptage seul » : seuils des voies relevés de +2 et recensement réduit à (clé, p, q_min, u) avec p ≤ 11, sans tour. Il rendrait E_9 et E_10 vérifiables.
- Nommer l'angle mort des compensations tronquées (exemple x^{p+1}(x−1)x^{T+1}). Le doubler d'un juge d'échantillon unilatéral : sites tirés au hasard, énumération exacte des supports aigus ou positifs dans un voisinage borné, recensement exact par l'index, présence exigée dans le catalogue si p + q_min ≤ Kmax+1.

4) GPU. Exiger l'identité des décisions, du catalogue et de la tour, pas celle de tous les compteurs de parcours, qui changent légitimement avec des vagues compactées. Seuls les compteurs de bilan (tâches, replis, propositions) doivent être égaux entre W1, W48 et le GPU.

===== refute-cout:D5_full refute
L'idée tient, mais les chiffres reposent sur un poste mal estimé et sur un facteur de conversion trop optimiste.

1) Poste sous-estimé : l'index (a)+(b) et la jointure de toutes les facettes. Le harnais D5 ne les chronomètre pas. Je les ai mesurés avec des tables plates, en vérifiant exactement les listes d'IDs, sur le même catalogue 8k/K10 :
- construction : 5 681 111 entrées, 420–511 ms ;
- jointure : 3 042 148 facettes, 719–863 ms, traduction en positions de programme comprise ;
- les 349 ms de recherches sur facettes distinctes déjà incluses dans les 1 748 ms de « saut » sont à déduire.
Le coût oublié net est donc de 0,79–1,02 s à 8k. D5 coûte 3,25–3,48 s au lieu de 2,456 s, soit un rapport produit/D5 de ×2,5–2,6, le bas de la fourchette ×2,5–3,5 annoncée. Ramené à la trame 000100, ce poste vaut 5,0–7,9 s-fil local, contre un budget annoncé de 2,4–6,1 Gcyc (≈ 0,8–2 s) : il est sous-estimé d'un facteur 3 à 8. Le coût par opération croît avec la table : 68 → 325 ns par facette de K2 à K10, et ×2,6 de 8k à 16k pour un volume doublé.

2) Borne de travail total. Sur la trame 000100/K10, le travail de D5 vaut ≈ 15–19 s-fil local :
- saut : 7,0 s ;
- (a)+(b) : 5,0–7,9 s ;
- programmes : 2,2 s ;
- phase A : 0,7–1,0 s ;
- images : 0,3–0,5 s.
Le facteur 58–70× (un fil local vers 48 fils G4) suppose une efficacité parfaite et un gain SMT ≥ 1,3 sur 24 cœurs. La calibration par la tour mono-fil donne G4 = 1,6–1,9× un fil local : 48 fils plafonnent donc à 42–59×. Même parfaitement réparti, le mur est ≥ 0,25–0,45 s, et ≥ 0,21–0,33 s même au facteur 58–70× du proposant. La moitié basse de « 0,15–0,30 s » est donc impossible. Le plafond de 0,4 s est dépassé pour 000000/000200, qui ont ×1,24–1,27 plus de travail.

3) Palier 2. Le GPU ne prend que l'étage (c). Il reste sur CPU (a)+(b), les programmes, la phase A et les images, soit 8,2–12,6 s-fil local : ≥ 0,14–0,30 s, contre 0,07–0,15 s annoncés.

4) Phase A « ÷6–8 ». On compare D5 à 8k avec le produit sur la trame :
- 87 ns/bloc côté D5 est le meilleur tirage : le même harnais sur le même catalogue donne 124 ns à l'ordre 10 ;
- 730 ns côté produit est une conversion de 2 190 cycles à 3 GHz ; le CPU mesuré est de 1 029 ns/bloc sur la trame 000000 ;
- sur la trame, chaque recherche de racine coûte ≈ 390 cycles de défauts de cache (mesure du développeur), et D5 fait les mêmes recherches (≈ 1,8 par bloc).
La phase A de K10 peut donc atteindre 0,2–0,37 s, et non 0,09–0,15 s.

5) Sortie « ÷15 ». Le format compact prend le catalogue comme banque et comme source des niveaux. Or le produit libère le catalogue en fin de chaîne. Le catalogue pèse 224 o/boule : 221 Mo à 8k/K10 et 982 Mo sur la trame.
- à 8k : 16,2 + 221 = 237 Mo, contre 245,6 Mo (÷1,04) ;
- sur la trame : ≈ 1,05 Go, contre ≈ 1,19 Go (÷1,13).

Ce qui tient :
- dédoublonnage facultatif : la multiplicité des facettes non hachées ne vaut que 1,073–1,097 ;
- K5 palier 1, 0,06–0,12 s, plausible ;
- un gain FULL ×6–13 reste probable.
PREUVES: ["Harnais de contre-expertise : /tmp/claude-1000/-workspaces-E-HGP/71988aca-a49a-4d33-96db-f05468331005/scratchpad/design/d5_refut/hash_probe.cpp. Il implémente l'index plat (graines I∪U et entrées S_C∪I_C∖{z}, empreintes XOR, sondage linéaire, entrées de 16 o, charge ≤ 0,5) et la jointure de chaque facette régulière avec vérification exacte des listes d'IDs relues dans le catalogue, puis la traduction en position de programme. Conditions : un fil, nice 19, charge de l'hôte 4–6. Sorties au même endroit : hash_8k_k10.json, hash_8k_k10_b.json, hash_8k_k5.json, hash_16k_k5.json. Catalogues : ../d5/cat_s01_8k_k10.bin (987 076 boules) et ../d5/cat_s01_16k_k5.bin (511 801 boules).", 'Mesures 8k/K10 (deux tirages). Construction : 5 681 111 entrées en 510,6 puis 420,3 ms (60–211 ns/entrée). Jointure : 3 042 148 facettes en 862,5 puis 719,0 ms ; 68–325 ns par facette, et 233–325 ns aux ordres K ≥ 7, où les tables font 34–67 Mo. Résolues par (a)/(b) : 547 170 sur 625 668 à K10 ; verify_fail = 0.', 'Recherches de style harnais (unordered_map) sur les 1 558 256 facettes distinctes : 349 ms, déjà comprises dans les 1 748 ms de saut de probe5. Coût net oublié : 0,79–1,02 s, soit un total D5 de 3,25–3,48 s contre 8 577 ms pour le produit (×2,5–2,6).', "Mise à l'échelle de (a)+(b) : 8k/K5 = 164 ms (653 858 entrées, 670 476 facettes) ; 16k/K5 = 429 ms (1 299 781 entrées, 1 328 435 facettes), soit ×2,6 pour un volume ×2. Le budget annoncé pour la trame, 1,4–4,1 + 1–2 Gcyc, correspond à 21–53 ns par opération ; mesuré : ≈ 130–160 ns par opération à 8k.", "Multiplicité des facettes non hachées, occurrences sur distinctes : 302 565 / 281 900 = 1,073 à 8k/K10, et 80 844 / 73 694 = 1,097 à 16k/K5. Le point (d) (pas de tri global) tient : +7–10 % seulement sur l'étage (c).", 'Calibration de la conversion. Tour mono-fil en voie temporelle : locale (receipts/first_tower_20260922) 94,3 / 130,0 / 102,8 s à K10, contre 55,7 / 75,9 / 64,3 s sur G4 (R1, probe_4/3/5, ts=0), soit 1,6–1,7× par fil ; à K5, 1,6–1,9×. Sur 24 cœurs, 48 fils ≤ 42–59× un fil local. CONTRE_AUDIT_B_G4_R1 : le ×11,6 « mêle machine et parallélisme ».', 'Travail D5 sur la trame 000100/K10 (35 551 sites), en s-fil local, avec les ratios du proposant (×4,43 représentants, ×4,82 MEB, ×5,28 nœuds) :\n- saut : (1,748 − 0,349) × 5,0 ≈ 7,0 ;\n- (a)+(b) : 1,14–1,37 × 4,43 × 1,0–1,3 = 5,0–7,9 ;\n- programmes : 0,444 × 4,9 ≈ 2,2 ;\n- phase A : 0,156–0,22 × 4,43 = 0,69–0,97 ;\n- images par pointeurs de saut : 0,3–0,5, non mesuré.\nTotal ≈ 15–19 s, soit un mur ≥ 0,25–0,45 s à 42–59×, et ≥ 0,21–0,33 s à 58–70×.', "Phase A. Le lean2 du proposant donne à l'ordre 10 du même catalogue 30,83 ms (probe5, 87 ns/bloc) et 44,04 ms (probe4, 124 ns/bloc). Pour le produit sur la trame 000000/K10, deux mesures du développeur : 2 179,7 ms pour 2 117 675 blocs (1 029 ns, PHASE_A_GRAPHE_TEMPOREL) et 3,8 M recherches de racine à ≈ 390 cycles de défaut de cache (COORDINATION 07 h 55). Côté D5 : facettes par bloc 625 714 / 354 053 = 1,77, et ensemble à accès aléatoire ≈ 2,5 Mo à 8k contre ≈ 11–15 Mo sur la trame."]
CORRECTION: Projection révisée à retenir :
- **K10 palier 1** : 000100 ≈ 0,25–0,5 s ; 000000/000200 ≈ 0,3–0,6 s. Plafond prudent 0,6 s. Le gain reste ×6–13 face à 3,2–4,0 s.
- **K5 palier 1** : 0,06–0,12 s, maintenu.
- **Palier 2** : ≥ 0,15–0,3 s tant que (a)+(b) reste sur CPU. Il se rapprocherait de 0,1 s seulement si deux choses sont faites :
  - (a)+(b) passe sous ≈ 40 ns par opération, soit ≈ 13,7 M jointures et 25 M insertions sur la trame (préchargement groupé ou GPU) ;
  - la phase A par tranches est mesurée avec les dix ordres concurrents.

Corrections du texte :
1. Chiffrer (a)+(b) à partir de la mesure plate (≈ 1,1–1,4 s-fil à 8k/K10, et non 0,2–0,5 s), et donner le rapport produit/D5 réel : ×2,5–2,6 à 8k, ×2,3 à 16k/K5.
2. Remplacer le facteur 58–70× par 42–59× au mieux (calibration tour mono R1/first_tower : G4 = 1,6–1,9× par fil). Publier la borne de travail total/capacité à côté du chemin critique.
3. Comparer la phase A au produit à la même échelle. Ne pas prendre le meilleur tirage de 87 ns (124 ns sur le même catalogue). Annoncer 0,2–0,37 s possibles pour K10 si les défauts de cache de la trame (≈ 390 cycles par racine) se transfèrent.
4. Pour la sortie, compter le catalogue retenu (224 o/boule, 0,98 Go sur la trame), ou le réencoder (≈ 56 o/boule, total ≈ 0,32 Go, ÷3,7). Le chiffre « ÷15 » est à retirer.
5. Corriger l'index : 7,1 M entrées à K10 sur la trame, et 0,43–0,9 Go pour dix ordres coexistants.
6. Fixer le périmètre chronométré. L'expansion exigée par la porte same_payload et la conversion vers CertifiedTowerInput doivent être chronométrées et publiées à part. La comparaison avec les 3,20 s du produit (encodage, banque et validation compris) doit le dire.

Mesures à faire avant tout port :
- (a)+(b) plat avec préchargement, puis phase A maigre, sur un catalogue de trame entière (K5 d'abord), avec les dix ordres concurrents ;
- ventilation par phase de la tour produit sur G4, pour étalonner la conversion ;
- pointeurs de saut des images, sur 16k/32k.

===== refute-math:D5_full tient_avec_reserves
Le cœur de D5 tient. Sur 12 nuages fortement dégénérés, D5 donne les mêmes racines pré-lot, les mêmes nœuds et fusions que le produit, et aucune naissance n'a besoin de remonter. En revanche, deux énoncés écrits sont faux sur des fixtures exactes, et le code du palier 1 mesuré contient un tampon trop court.

(1) La règle de descente 1–4, telle qu'elle est écrite, refuse là où le produit réussit. Il lui manque le « coup d'ancre » sur D à chaque état.
- Fixture fx_cz (4 sites) : a=(500,1000,1000), b=(1500,1000,1000), c=(1000,1000,1500), x=(1000,1600,1000).
- B est la boule q3 de abx (triangle aigu) ; on regarde sa facette {a,b} à K=2.
- D = MEB({a,b}) a pour coquille {a,b,c} (angle droit en c), avec p=0, q_min=2 et une fenêtre [1,3] qui contient 2. Le produit renvoie donc D.
- La règle écrite prend G = {a,b} (les deux plus petits rangs), soit G = F. MEB(G) = D, le niveau ne baisse pas, on passe à l'échange de la règle 4. Il faut un intrus strictement intérieur, et il n'y en a aucun : échec.
- Le harnais d5_probe fait bien ce test (clé au catalogue, puis fenêtre) au début de chaque itération : les mesures publiées restent valides.
- Mais le texte ne le contient pas : ni § 2(c), ni l'item 2 de la complétude (« un repli est l'échange du produit »). Et ce test sert aussi APRÈS un saut (4 à 29 fois par nuage dégénéré), pas seulement à la profondeur 0.

(2) La Proposition F (bijection du format compact) est fausse avec des exceptions (nœud, boule, masque, drapeau).
- Dans un lot, une continuation contributive prend le rang du PREMIER bloc de son groupe. Ce bloc peut être muet (bloc régulier μ=1 sans contribution) ; il est alors absent du format compact.
- Sortie réelle du produit, nuage lat5_3 (35 sites d'une grille 5³, graine 3), K=8 :
  - contributions[28] est une continuation (segment 23, boule 267, p=4, u=5, masque 1, position 7 dans le lot) ;
  - elle vient AVANT contributions[29..], les naissances des boules 92, 105, … (positions 1 à 6) ;
  - la banque attribue la population 873 à la boule 267, puis 874 à la boule 105.
- Une expansion qui range chaque contribution à la position de sa boule produit un autre ordre des contributions et d'autres identifiants de population. La porte octet par octet échoue.
- Le cas apparaît dans 8 des 9 nuages grille/plan (1 ou 2 lots chacun). Même constat sur lat5_1 à K=9 : boule 459 à [316], avant la naissance de la boule 153 à [317].

(3) Le palier 1 « aucune allocation » écrit hors bornes.
- Le chemin des lots singletons du harnais range les racines distinctes dans r[kBallShellMax+1], soit 13 cases.
- Un bloc dont la coquille compte 12 sites peut avoir 32 facettes, chacune avec sa propre racine.
- Fixture fx_ico12 : les 12 sites c ± 100·v, avec v ∈ {(0,1,7),(5,5,0),(3,4,5),(7,0,1),(4,−3,5),(−5,0,5)} et |v|²=50.
- ShellTable donne 32 composantes strictes à K6 ; le produit crée une fusion à 32 parents. Le harnais écrit donc hors bornes (comportement indéfini), et ce passage n'a rien montré.
PREUVES: ['Conditions : chaîne v9 93066733 (bibliothèques du scratchpad), harnais compilés en -O2, nice 19, un fil, environ 2 min CPU au total ; expériences arrêtées quand la charge a dépassé 11. Aucune écriture dans le dépôt. GCP non utilisé.', "Renforcement du Lemme B (preuve de l'auditeur). On suppose que D = MEB(état) n'est pas terminal : soit D est hors catalogue (catalogue complet, donc p_D + q_min > Kmax+1 ≥ K+1), soit K < p_D + q_min − 1. Le cas K > p_D + u_D est impossible, puisque l'état a K sites dans D̄. Alors G prend d'abord I_D, puis au plus K − p_D ≤ q_min − 2 sites de coquille. G ne contient donc aucun support de D, et MEB(G) < r_D strictement. Conséquence : avec le test « D terminal » à chaque état, la règle 4 (repli) n'est jamais atteinte, et la terminaison est une descente stricte de niveau.", 'Douze nuages dégénérés : grilles 5³ (densité 0,35) et 6³ (densité 0,25), grille plane 7×7×2, 25 carrés cocirculaires avec intérieurs ; 27 à 143 sites ; K = 1 à 10. Les grilles 4³ sont refusées par la chaîne (chain_shell_above_12). Résultat avec le test D : racines pré-lot du saut égales à celles du produit sur 221 556 facettes, 0 écart ; 0 repli ; 0 naissance à remonter ; 0 échec de naturalité ; nœuds et fusions égaux au produit à chaque K.', "Coups d'ancre sur D dans la boucle du saut : 151 à 3 660 par nuage, tous sur des coquilles étendues (81 à 97 % au milieu de la fenêtre), dont 4 à 29 après un saut.", "Variante « texte » (règle 1–4 sans coup d'ancre sur D) : 165 à 2 000 états sans intrus par nuage, 210 à 2 787 occurrences de facettes non résolues ; aucune racine fausse quand la règle aboutit.", "fx_cz : saut avec test égal au produit (9 racines sur 9 à K2) ; règle écrite : 1 facette non résolue. Dans fx_c34, fx_cm34 et fx_czm, c a un rang plus petit et la règle écrite aboutit : l'échec dépend de l'ordre de Morton.", 'Ordre des contributions vérifié directement sur le FullCoverageCertificate du produit (order_check.cpp) : lat5_3 K8, contributions[28] (continuation, boule 267) avant [29..37] (naissances 92, 105, 126, …) ; lat5_1 K9, [316] (continuation, boule 459) avant [317] (naissance, boule 153), au même niveau exact.', "fx_ico12 : chaîne complète, tour produit complete_relative, K6 : 33 nœuds et une fusion à 32 parents (comp_check.cpp). Le d5_probe d'origine passe sans rien signaler."]
CORRECTION: 1. Écrire la règle 0 et l'appliquer à CHAQUE état, y compris après un saut : D = MEB(état) ; si la clé de D est au catalogue et que p_D + q_min − 1 ≤ K ≤ p_D + u_D, la cible est D. La règle 4 devient alors un refus d'invariant (signe d'un catalogue incomplet) au lieu d'un repli, et la preuve de terminaison se réduit à la descente stricte démontrée ci-dessus. Graver fx_cz comme fixture permanente, avec un mutant « sans règle 0 » qui doit être tué.

2. Format compact : ajouter à chaque exception la position de programme du premier bloc de son groupe (un u32, soit 4 octets par exception). Autre possibilité : stocker les exceptions dans l'ordre des actions, avec l'indice du prochain nœud créé. L'expansion trie ensuite les actions de chaque lot selon cette clé. Graver lat5_3 (K8) et lat5_1 (K9) comme fixtures d'égalité octet par octet (contributions et banque de populations).

3. Phase A maigre : ne pas borner le nombre de racines d'un bloc par kBallShellMax+1. La borne est le nombre de composantes strictes (32 mesurées pour u = 12 ; borne triviale C(12,6) = 924). Au-delà d'un petit tampon, basculer sur le chemin général à offsets CSR. Graver fx_ico12 (fusion à 32 parents).

4. Point (d) : la cible n'est une fonction pure de F que sans mémo. Avec un mémo partagé, elle dépend de l'ordonnancement. Les portes doivent donc comparer des racines, jamais des cibles ni des compteurs ; le mémo est soit propre à chaque fil, soit en écriture unique.

5. Catalogue scellé : garantir par construction, et vérifier par un juge d'échantillon, la positivité du support déclaré des boules régulières, dont dépend le Lemme A. La préparation ShellTable des coquilles étendues doit rester.

6. Les Propositions D et E restent des esquisses. Avant toute adoption : porte octet par octet de l'expansion contre le produit sur les 12 nuages ci-dessus et sur les coupes LiDAR 8k–32k. En plus, un contrôle ASan du chemin des lots singletons.

