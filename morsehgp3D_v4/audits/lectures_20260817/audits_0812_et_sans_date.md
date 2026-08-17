# Rapport de lecture — audits v3 du 12/08/2026 et preuves épinglées (tranche « cellules de centres / ancre maximale / spindle / gate D »)

Sources : 18 fichiers de `/home/user/E-HGP/morsehgp3D_v3/audits/`, lus intégralement. Convention : « l'auditeur » = l'auteur des `AUDIT_*` (contre-auditeur indépendant, humain/externe) ; « Claude » = l'auteur des `NOTE_CLAUDE_*` et de `NOTE_SOLUTION_SOURCE_ANCRE_MAXIMALE_*`. Les `NOTE_SOLUTION_*` et `NOTE_ARCHITECTURE_*` datées du 12/08 restant dans le cadre `mode=audit_independant...` sont de l'auditeur sauf mention. Contexte fixe : `smax=11`, `K_max=10`, profil u16, `public_status=not_claimed`.

---

## 1. Fichier par fichier

### 1.1 `AUDIT_REPONSES_CELLULES_CENTRES_20260812.md` (auditeur → Claude ; 1142 lignes, le plus dense)

- **Qui/quoi** : l'auditeur répond aux `QUESTIONS_CLAUDE_CELLULES_CENTRES` et corrige `NOTE_ARCHITECTURE_GPU_LISTES_CELLULES_CENTRES`. Verdicts : le théorème de listes imbriquées est **exact** mais « ni le nombre de cellules, ni la taille des listes, ni le nombre de cliques ne possède aujourd'hui de borne compatible avec 50 000 points sous une seconde ».
- **Lemme budget–cellule** (reçu, central) : pour une cellule `C`, `R_h(C)` = `(h+1)`-ième plus petite valeur de `u_C`, `D_h(C)={x: l_C(x)<=R_h(C)}` ; si une boule positive owner de `C` a exactement `p` intérieurs stricts, alors `beta<=R_p(C)` et `I_B ∪ U_B ⊆ D_p(C)` (preuve en une ligne : sinon `p+1` témoins stricts). `A_q = D_(smax-q)`. Promotion par curseur : `e0(U)=max(tau_C(x): x∈U)` immuable, invariant `h<=r_h<=p`, terminaison en `<= p-e0` promotions, sortie anticipée au `(smax-q+1)`-ième intérieur seulement. Partition terminale **commune à tous les budgets d'une arité** obligatoire (fixture ABC/W : `e0=0` dans la racine, `e0=1` dans la cellule singleton du centre → double émission sinon).
- **Indépendance générative des arités** (réfutation de la cascade q2→q3→q4) avec deux fixtures rationnellement rejouées : (i) q3 pertinent sans aucune arête q2 pertinente : `A=(10,10,10)`, `B=(20,10,10)`, `C=(15,18,10)` + 30 témoins listés ; chaque boule diamétrale a `p=10` (`p+q=12>11`), le triangle a centre `(15,199/16,10)`, rayon carré `7921/256`, barycentriques `(89/256,89/256,39/128)`, `p=0`. (ii) q4 pertinent sans aucune facette q3 pertinente : tétraèdre `(20,20,20),(60,60,20),(60,20,60),(20,60,60)` + groupes `G01`/`G23` de 9 points ; centre `(40,40,40)`, rayon carré `1200`, chaque face a `p=9`.
- **Bornes terminales conditionnelles** (sous « aucun cinq sites cosphériques », qui n'est PAS garanti par le profil) : listes `13/12/11` pour q2/q3/q4, soit 78 paires, 220 triplets, 330 quadruplets — conditions de terminalisation, jamais caps d'exactitude. Contre-fixture cap-shell : centre `(10,10,10)` + 30 points à rayon 5 (permutations/signes de `(5,0,0)` et `(4,3,0)`) : paire antipodale `p=0, q=2` mais `|U_B|=30` — tue tout tampon shell fixe (`int in_ids[24]` + `exit(3)` du snapshot condamnés).
- **Baseline Poisson** (Edelsbrunner–Nikitenko, éq. (7)) : `E[N_(v,u,p)] = C^3_(v,u)·binom(p+u-1,u-1)·rho|Omega|`, constantes critiques 3D `C^3_(1,1)=4`, `C^3_(2,2)=3+3π²/16`, `C^3_(3,3)=3π²/16`. Source S positive attendue à `smax=11` : `(175+495π²/16)·rho|Omega| ≈ 480,340886` par point, soit **~24,017 millions de supports à 50 000 points** (2,000 M q2 / 10,914 M q3 / 11,103 M q4). Réfute « certificat sparse = catalogue exhaustif à petite constante » ; impose fusion en flux vers le consommateur H0. Loi conditionnelle `rho·nu_3·R^3 ~ Gamma(p+u,1)` pour dimensionner les queues (jamais un prune).
- **Théorème d'acuité q4** : tout tétraèdre propre positif possède **au moins deux faces aiguës** (Crux Mathematicorum 38(8), pb 3653) ; le lieu des centres d'une face est la droite rationnelle normale au plan par son circumcentre → test droite–cellule exact avant apex ; en lane q3 seule, support propre positif ⇔ triangle **strictement aigu** (3 produits scalaires i64 avant tout lift) — jamais utilisé pour conditionner q4 (fixture face canonique obtuse `P0=(5,10,10)…P3=(14,10,13)` sur sphère centre `(10,10,10)` r²=25, barycentriques `(12,45,45,60)/162`).
- **Mesures NO-GO du prototype cellules** : à n=40, compteurs = `C(40,2)/C(40,3)/C(40,4)` exactement (aucun prune) ; à n=50, 454 960 vues q4 > `C(50,4)=230 300` (plus cher que l'exhaustif). Rampe `n=100/200/400` (`leaf=4, pair_cap=256`) : pentes > 1,35 partout ; à n=400, **85,7 %** des lifts meurent seulement à l'owner ; extrapolation linéaire 50 k ≈ 3,0 M cellules, 905 M lifts, 448 M tests census. Snapshot `terrain n=500` : `lifts_built=2 980 691` mais constructions physiques ≥ `4 555 956` (+52,85 % non comptés), 93,48 % morts à l'owner. Décision : **conserver comme source CPU de référence et banc de réduction ; ne pas porter sur G4**.
- **Ordonnance corrective** : RLE `SupportKey` **avant** toute géométrie (radix, géométrie une fois par clé, owner par cellule half-open du centre), puis RLE `GeometricBallKey` avant census (un census par boule ; rejeter un support q4 ne tombstone pas la boule — fixture centre `(20,20,20)` r²=25, q3 pertinent/q4 non, `H_run=8`). Clé chaude = **5-uplet homogène primitif** `(D, C-2Da, D||a||²-C·a)` divisé par pgcd (jamais `U_B`, qui est post-census et potentiellement `Theta(n)`).
- **Scores affines à jauge fixe** : `F_x(z)=S||x||²-2⟨x,z⟩` avec `S=2^d` ; sous u16 et `d<=26`, `|F_x|<9·2^58<2^62` → top-9, égalités, bissecteurs **en i64**. Bornes dyadiques distance : `l,u<3(65535·2^d)²<2^(34+2d)` (i128 si `d<=26`, PAS un u64 device). Split : compte exact de cliques d'intervalles `N_q=Σ_i C(a_i,q-1)` (sweep `O(m log m)`, Helly 1D), majorant Kruskal–Katona `Q<=Q_KK(T)=C(a3,4)+C(a2,3)+C(a1,2)`, majorant `4Q_4<=(m_4-3)T_4`, bornes de degrés forward `T_3<=Σ C(d_3^+,2)`, `Q_4<=Σ C(d_3^+,3)` ; fixture `K_24` : `E+9T=18 492` passe le cap 20 000 alors que `Q=10 626` et `E+3T+6Q=70 104` (réfute le coefficient 9).
- **Divers reçus** : join sparse H0 par token de plateau Johnson `(GeometricBallKey,beta,S_B)` — postings `(k,F,g)`, travail `P_k=Σ_g C(|S_g|,k)`, max `C(11,5)=462` par générateur, 2 046 sur `k=1..10` ; lookup gateway sans requête négative (table complète ⇒ absence ⇒ `j>=2` intrus) ; multiplicité EGS (SoCG 2025) : point générique dans `<= C(p+3,3)` tétraèdres p-hefty (invariant d'oracle, pas une borne de coût) ; Poisson halo : optimum `delta=r_10/√3=0,772·s0` (pas 0,83), `r_10/s0=(30/(4π))^{1/3}=1,337`, et même `1,4e4·n` = 700 M contrôles de paires à 50 k.

### 1.2 `AUDIT_REPONSES_CLAUDE_CHAMBRES_NIVEAUX_CUTTING_20260812.md` (auditeur)

- **Résultat central** : le filtre `theta` est **strictement redondant** sur toute ancre vivante. Avec `d_env=smax-2`, `c=#{z:L_z>0}` : si `c>=d_env` la lane est morte ; sinon `theta<=0` donc `U_z<theta ⇒ U_z<0` (déjà le filtre `always_outside`). Compteur exigé : `theta_only_prunes_on_live_patch ≡ 0`.
- **Seuils angulaires exacts** : `gamma_3<arccos(√(2/3))=35,2643896828°`, `gamma_4<arccos(√(11/15))=31,0909303577°`. Le décimal `31,134°` de Claude est **faux** (fixture `a=0, b=(58,35,0), w=(29,0,0)` : angle ≈31,1088° mais `15||2w-a-b||²=18375>18356=4D²`). Lemme C : `theta_3=π/6`, `tan(theta_4)=√(2-√3)`, `theta_4=27,3678051586°` ; le cône fermé tronqué **axialement** à `u<=1/2` n'est pas dans le spindle ouvert — la forme euclidienne `||w-a||<=D/2` l'est (fixture `b=(2778,0,0), w=(1389,719,0)`).
- **Banque de 432 sous-cônes entiers** (48 chambres × 9 triangles sur rayons `v_ij=(3,i,j)`, `0<=j<=i<=3`) : `min cos²(gamma)=9/11>11/15` vérifié sur 27 arêtes. **Certificat annulaire q4 sans racine** : témoin `w` du même sous-cône avec `D²<=9r²` et `4r²<=D²` est strictement dans la boule de milieu q4 (`f(t)=t²-t·cos γ+11/60<0` aux extrémités : `2809·11<9·3600`, `169·11<9·225`). 8 IDs distincts ferment q4, 9 ferment q3, 10 ferment q2. Fixture positive `a=0, b=(9,0,0), w=(3,1,1)`.
- **Lemme 2 boule commune** (seuils de séparation dirigés, pertinents pour une WSPD) : rayon commun positif exige `d>2s` (q2), `d>s(1+√3)` (q3), `d>s(1+√15/2)` (q4), avec `s=r_A+r_B`, `d=||c_B-c_A||` — décisions par bornes entières, jamais ces décimaux.
- **Largeur arithmétique de la cutting** : formes `A=2U·e_1, B=2U·e_2, C=g` avec `|A|,|B|<2^35, |C|<2^36` ; après cisaille unimodulaire (`t<=m<=50000`) `|B'|<2^52` (i64) ; déterminants d'intersection `|Δ|<2^88`, signe d'une 3e ligne `<2^126` (i128) ; mais **ordonner deux abscisses rationnelles exige jusqu'à ~2^178** (≈160 bits même pour m~100) → fast path i128 détecté + cold path **192 bits signé minimum** ; `double` ou overflow-comme-ex-æquo interdits.
- **Cutting signée exacte du disque médiateur** : chart entier `|alpha|,|beta|<5/4` ; par patch : `L_z>0` → `always_inside` (identités conservées, compte `c_K`), `U_z<0` → supprimé, `L_z<=0<=U_z` → conflit ; q3 meurt à `c_K>smax-3`, q4 à `c_K>smax-4` ; `I_B=A_K ∪ {z∈C_K: F_z(w*)>0}`, `U_B=S ∪ {z∈C_K∖S: F_z(w*)=0}`. Cap de profondeur **interdit** comme résultat ; voie GPU = cutting Las Vegas validée par prédicats entiers.
- **Terminal des concurrences par dominance 2D** (§4.4) : coordonnées `t_z=d·(z-c_v)`, `r_z=u·(z-c_v)`, `s_z=n·(z-c_v)` (`n=d×u`) ; les sites `s_z=0` ne portent aucun q4 propre positif ; paire de côtés opposés positive ⇔ `s_x·s_y<0` et `D²R±2||u||²T>0` (`R=|s_y|r_x+|s_x|r_y`, `T=|s_y|t_x+|s_x|t_y`) ; scores `S_z^±` → reporting de dominance en `O(h_v log h_v + J_pos)` ; sixième distance collective : `||x-y||²<=D² ⇔ (x-c_v)·(y-c_v)>=rho²-D²/2` avec extrema par blocs. Bornes : centres distincts `<e(k+1)m`, incidences `I_<=k<2e(k+1)m` ; seuls `J_pos` et `H_out` restent potentiellement quadratiques.
- **Slabs de bissecteurs** (§5) : `phi(c,z)=2c·z-||z||²`, slab `delta_min<=r_0(c)<=delta_max` par convexité/concavité aux endpoints — la couture entre `ALL` et la cutting, réduit les patches inter-amas sans émettre `A×B`.
- **Rejeu au pin `9bcd137`** : 550 CTests, `32/33` mhgp3v_anchor en 503,57 s — `mhgp3v_anchor_mutant_census` tué par signal (~62 s), passe isolément en 39,15 s (instrumenter RSS/durée ; pas un verdict sémantique).

### 1.3 `AUDIT_REPONSES_MUR_AMAS_CENSUS_SPINDLE_20260812.md` (auditeur, contre-audit du « mur »)

- **Provenance réfutée** : les colonnes `candidate_pairs=C(n,2)` / `front_witness_prunes=0` de la note de Claude ne se reproduisent pas sur le même ELF pincé (`n=150` : `11 174/1` vs `11 175/0` ; `n=300` : `44 831/40` vs `44 850/0`). Les colonnes q4 se reproduisent. Leçon : versionner reçus bruts + SHA + commande ; ne jamais reconstruire une colonne depuis `C(n,2)`.
- Pentes empiriques `~2,71` (paires q4) et `~3,05` (temps) entre n=100 et 500 : « rouges, compatibles avec un régime cubique », **pas** « cubique démontré ».
- **Q1 : le juge indépendant vient d'abord** — oracle borné minimal comparant les enregistrements complets `(BallKey,SupportKey,I_B,U_B,ownerPair)`, arithmétique propre.
- **Lemme du nœud spindle** : `W_3(a,b)={z: g>0 ∧ 3g²>4Q}`, `W_4={z: g>0 ∧ g²>2Q}` (`g=D²-U·U`, `Q=D²(U·U)-(U·d)²`, `U=2z-a-b`) — convexes ; AABB fermée `⊆ W_q` ⇔ ses **8 coins** strictement dedans. Contre-exemple à « tout témoin universel est dans le vide » : `a=(10,10,10), b=(30,10,10), z=(11,10,10)` : `g=76, Q=0` → z, collé à l'amas de `a`, est témoin universel q3 **et** q4. À `smax=11` : 8 crédits tuent q4, 9 tuent q3 ; antichaîne par lane avec bit `q3_already_credited`.
- **Lift entier sur `A×B×C`** (première autorité exacte pour blocs) : `H(a,b,z)=(b-z)·(z-a)`, `R(a,b,z)=||(b-a)×(z-a)||²` ; identités `g=4H`, `Q=4R` ; q3 : `H>0 ∧ 3H²>R` ; q4 : `H>0 ∧ 2H²>R`. `Hmin` sur 3 AABB par 24 évaluations scalaires (séparable) ; `Rmax` sur les `8³=512` triples de coins. `Hmin>0 ∧ 2Hmin²>Rmax` certifie `ALL-W4` (sûr, incomplet : exemple `UNKNOWN` correct fourni). Largeurs : `|H|<2^34` (i64), `R`, `2H²`, `3H²` jusqu'à 69 bits → promotion **u128 avant** multiplication. Fixtures d'égalité q4 `a=(10,10,10), z=(11,10,10), b=(12,11,11)` et q3 `a=(10,10,10), z=(11,9,10), b=(11,8,11)` restent `UNKNOWN`.
- **Q3 `kept`** : aucune saturation déterministe ; cardinal attendu `~rho·D³` ; ancres survivantes `D=Θ((n/rho)^{1/3})` ⇒ `kept=Θ(n)` possible ; `kKeptCap=2048` = overflow/refus, pas une saturation ; publier `mean/p50/p95/p99/max kept` bucketés par `rho·D³`.
- **Q5 garde de densité** : hors chemin produit (fail-open mais aucun prune gagné, coût net positif) ; remplacée par le certificat spindle exact.
- **Q6 cône cible depuis z** : les domaines `C_3(a,z)`/`C_4(a,z)` sont des cônes de Lorentz **d'apex z, d'axe z-a**, demi-angles 60° et `arctan(√2)` — le `54,74°` de Claude devient exact **mesuré depuis z vers b**, pas depuis a ; formes économiques `H>0 ∧ 4H²>E2·X2` (q3), `H>0 ∧ 3H²>E2·X2` (q4) (`E2=||e||²`, `X2=||t||²`, `R=E2·X2-H²`). Banque `Z_a` par endpoint + traversée de nœuds partenaires `B` (iff par 8 coins) ; porte `NONE` sans racine via `Hmax` et minorant `Rlb` par intervalles de composantes. « Jamais balayer M voisins pour chaque PairId » ; `scanner M·C(n,2)` n'est jamais une option.
- Rejeux : premier pin `43/43` en 141,39 s (560 CTests) ; successeur `2a205f3` : 573 CTests, `56/56` en 75,50 s. §10 : le producteur spindle/cône du 13/08 est **refusé** (conversion `smax` créant un faux prune, cardinalité silencieusement réduite, juge incomplet par lane, résiduel non rejouable, ABI CUDA cassée, trois pentes rouges) même si le lemme des cônes est admis.

### 1.4 `AUDIT_REPONSES_VOLUME_PINCEAU_PROJECTIONS_20260812.md` (auditeur)

- **Théorème de séparation** (l'argument qui tue le front intégral) : famille u16 `A_i=(1+i,0,0)`, `B_j=(0,1+j,1)`, `n=2m`. Sommets shallow relevés R4 : `V_{<=9}(m)=55m²-440m+715` = **34 364 000 715** à `m=25 000` (niveau 0 seul : `(m-1)²=624 950 001`), alors que Source S = `20m-55` supports q2 = **499 945** ; **aucun** q3 (tout triangle a un angle obtus) ni q4 (contradiction barycentrique `1-z>1/2` et `z>1/2`) positif. Aussi `V_8=45m²-330m+495`, `V_7=36m²-240m+330`. Conclusion : le plein arrangement n'est ni la sortie ni un minorant ; **front intégral refusé comme architecture 50 k**.
- **Audit du pinceau** : le successeur choisit le premier croisement par `dir·sign(power_b(y))·sign(lambda(y))>0` sans produit croisé large (correct), mais reste un DFS ordonné heuristiquement, pire cas `Theta(n)` par requête + second parcours `collect_shell` ; ni best-first ni logarithmique ; le probe accepte `--coord` jusqu'à `1e8` alors que la borne i128 suppose u16.
- **Théorème de propriétaire + plafonds par arité** : q2 → `ell<=9`, q3 → `ell<=8`, q4 → `ell<=7` (naviguer q4 à `k_nav=9` paie deux niveaux pour rien). Fixture : `A=(15,10,20), B=(7,14,20), C=(7,6,20), D=(10,10,21)` — sphère `ABCD` niveau 0 (centre `(10,10,8)`, r²=169) porte un support q2 `AB` (centre `(11,12,20)`, r²=20, D intérieur à d²=6) et un q3 `ABC` (centre `(10,10,20)`, r²=25, D intérieur à d²=1), tous deux de niveau 1 : **le niveau du support n'est pas celui de son owner**.
- **`--harvest` défectueux** : rejette `inside+shell>smax` (rang fermé) et perd une Source S admissible extra-shell (`p+q<=11` mais `p+|U_B|>11`) ; clé sans `(BallKey,I_B,U_B,owner)` ; récolte naïve d'un shell de taille m tente `C(m,2)+C(m,3)+C(m,4)`.
- **Blueprint support-first** : lanes séparées (k=1 Yao-1 ≤ `48n` arêtes ; q2 profond cascade non reçue ; q3 candidats directs ; q4 center-cover), flux `candidat → décision → BallKey → RLE → strict-count/census 2 passes → U_B → activation → fold`, jamais `tous les sommets → filtrage` ; census 2 passes avec abandon au `(12-q_min)`-ième intérieur ; générateurs saturés (Johnson) pour plateaux. Corrections de compteurs (touches cumulées `5,18n`, pas `n/2` ; exposants locaux 1,078/1,044/1,025 descriptifs).
- Décision : front de boules = oracle q4 borné et banc de prédicats, jamais optimisé/porté G4.

### 1.5 `NOTE_ARCHITECTURE_GPU_LISTES_CELLULES_CENTRES_20260812.md` (auditeur, proposition constructive)

- **Théorème de localité hiérarchique** : `l_C(x)=min_{c∈K_C}||x-c||²`, `u_C(x)=max…`, `t_q=smax-q+1`, `R_q(C)` = `t_q`-ième statistique des `u_C`, `A_q(C)={x: l_C(x)<=R_q(C)}` ; si `t_q>n`, `A_q=X` fail-open. Monotonie sous subdivision (`R_q(D)<=R_q(C)`, `A_q(D)⊆A_q(C)`), calcul enfant local sans rescan global ; complétude : support owner pertinent ⇒ `beta<=R_q(C)` et `I_B∪U_B⊆A_q(C)` → **la liste est à la fois domaine de génération et certificat de census** (la propriété GPU décisive).
- **Invariant pool-relative** : avec domaine resserré `tight=C∩bbox(P)`, les seuils ne sont plus les globaux, mais la conservation `beta_B<=R_(p,P)(K)` ⇒ `I_B∪U_B⊆D_(p,P)(K)` tient (positivité ⇒ `c∈conv(U_B)⊆bbox(P)`).
- **Contre-pièges gravés** : `beta=R_q(C)` admissible (cellule singleton, support `(9,10,10),(11,10,10)`, `smax=3` : `R=beta=1` — `>=` ou `l<R` perd le support) ; garder **tous** les ex æquo à `R` ; `R+1` ≠ `beta<=R` (beta rationnel) ; `smax` ne borne ni `|A_q|` ni le shell.
- **Objet critique vs face shallow** : `Phi(c,lambda)=||c||²-lambda` ; positivité ⇔ minimum de `Phi` dans `relint conv(U)` ; fixture `U={(-1,0,0),(1,0,0)}, y=(0,2,0)` : sphère shallow centrée en y avec y intérieur ≠ miniboule critique centrée en 0 sans intérieur.
- **Machine `count/scan/fill`** : SoA, cellules Morton, top-9 warp, CSR `D_8` avec rangs `tau_C`, `D_7` en préfixe ; deux layouts exclusifs (distances `l/u` vs scores affines `L/U` + niveau translaté `theta_B=beta_B-||c_B-c_0||²`, jauge `c_0` fixe pour tout l'arbre).
- **Deux RLE** avec variantes mémoire (RLE spatiale par lot vs shardée par bits de `SupportKey` ; reshuffle `(GeometricBallKey,OwnerCellId)` obligatoire avant census dans la variante shardée) ; sentinelle terminale top-`(12-q)` dans `X∖U` avec certificat `delta` vs `beta`.
- **Fixture acuité** : `C=(10,10,10)` r²=30, `P0=(5,8,9), P1=(5,8,11), P2=(9,12,5), P3=(15,11,12)`, barycentriques `(5,6,5,12)/28` : exactement deux faces aiguës (borne 2 optimale). Centre q4 depuis carrier : `[2(n·v)N+n(G·Delta-2⟨N,v⟩)]/[2G(n·v)]` ; test précoce `t_d∈T` (intervalle droite∩cellule) avant lift.
- **Certificat local de liste bornée** : si `diam(K)<=alpha·rho` et `|P∩B(c0,(1+2alpha)rho)|<=Lambda(H+1)`, la liste terminale a `<=Lambda(H+1)` sites → gate exacte pour warp/bitset `<=64`.
- **Préflight mémoire** : bitset dense `m·ceil(m/64)` à m=50 000 = 39 100 000 u64 ≈ **298,3 MiB** — non sparse ; CSR forward + Kruskal–Katona à la place.

### 1.6 `NOTE_CLAUDE_MUR_CUBIQUE_AMAS_ET_COUT_CENSUS_20260812.md` (Claude)

- **theta démontré redondant puis désarmé** : `theta_only_prunes_on_live=0` sur 5 familles/2 moteurs, avec témoin de causalité (mutant `theta-no-fail-open` sur `uniform n=500` : 2 557 917 prunes en plus) ; économies : un `nth_element` par ancre hôte, un balayage `O(site_count·(smax-2))` device.
- **`eight_clusters` : front inerte et producteur cubique** : `candidate_pairs=C(n,2)` exactement et `front_witness_prunes=0` pour n=100..500 ; paires q4 2 446 467 → 191 538 784 ; pentes log-log **2,59–2,71** (paires), **2,78** (`interior_tests`), **3,05** (temps) ; « 50 000 hors d'atteinte par plusieurs ordres de grandeur ». Cause géométrique : boule témoin centrée en `z_0=(a+c_B)/2`, **dans le vide inter-amas**.
- **`uniform` : le mur est le census** : degré candidat/point croît 226→359→479 (n=500/1000/2000) à densité constante (déjà 2,06× la baseline pointwise 232,4) ; à n=2000, `236 221 639/(337,94×2000)=349,5 ≈ mean|kept|` : le census rebalaie `kept` entier pour chaque support ; `kept` h.w. 394/515/748 (pente 0,46). Extrapolation 50 k : ~1,7e7 supports → **1,3e10 prédicats i128** — hors budget 1 s à lui seul.
- **Deux défauts trouvés par `--compare-engines` (35 compteurs)** : le moteur `pipeline` (celui que nvcc compile) **ne comptait aucun rejet** (référence : positivité 1 169 095, non-aigu 352 312, owner 272 207, rang 297 896, dégénéré 221 ; pipeline : cinq zéros — « un compteur absent n'est pas un compteur nul ») ; la garde de densité réfutée par ablation (prunes identiques armée/désarmée sur les 3 familles, temps dégradé sur 2/3).
- **Mort par budget au plus tôt** : arrêt dès que le compte d'intérieurs certifiés dépasse le budget de la dernière lane vivante — sorties inchangées au chiffre près ; `eight_clusters n=500` : `site_evaluations` 33 870 356→13 983 250, wall 92,458→33,531 s ; facteur constant 1,4–2,8, **ne change pas la pente**.
- **Sonde cône M-NN** (question 6) : taux de mort par les `M` plus proches voisins de `a` : n=200 : 21,58/29,03/39,81 % (M=48/96/192) ; n=500 : 37,44/51,85/57,07 % ; n=1000 : 44,82/58,17/71,22 % — croît avec n.
- Questions : juge d'abord ? test de budget avant liste de sites ? `kept` structurel ? forme de la porte de mur ? retrait de la garde ? faute dans l'ordonnance cône ?

### 1.7 `NOTE_CLAUDE_PRODUCTEUR_ANCRE_EXACT_UNE_FOIS_20260812.md` (Claude)

- **Producteur par ancre maximale implémenté** (`anchor_envelope.hpp`/`anchor_source.cpp`) : émission **exacte-une-fois** depuis l'arête maximale canonique (la plus longue, tie-break plus petit `(min,max) PointId`) — plus de `SupportKey` RLE, lift, owner de cellule ni point location (vs 39,242 occurrences/support de la lane cellules).
- **Trois lanes = trois seuils sur une seule marge** : `u=2z-a-b`, `g=D²-u·u` ; q2 : `g>0`, q3 : `3g>2D²`, q4 : `15g>11D²` ; census q2 lu sur `g` (`g=0` = extra-shell). Marge de Jung encadrée par racine entière : `Q=(u·u)D²-(u·d)²`, `s=isqrt(2Q)+1`, `Llow=g-s`, `Uhigh=g+s`.
- **Certificat de front entier** : `R_q=Dmin/c_q-ext/4`, minoré par `(⌊k_q·isqrt(Dmin²)/10^4⌋-isqrt(ext²)-1)/4` avec `k_2=20000`, `k_3=11547`, `k_4=10327` ; produit fermé seulement si les **trois** lanes atteignent 10/9/8. Corrige une perte d'un facteur 2 (`Dmin/(2√15)` donnait une coupure ~`10,4·rho^{-1/3}` au lieu de `4,8`, soit 8× trop de candidats).
- **Exactitude** : `--verify` (extension rejouée sur toutes les paires) : accord exact sur `uniform/terrain/scanline_single_pass/scanline_overlap_multiecho`, n=60–120.
- **Claim retiré** : `candidate_pairs/n = 227/351/465` (n=500/1000/2000) comparés à tort à `(4π/3)(4,8)³≈463` — c'est un degré **dirigé** ; baseline pointwise = ~231,6 (dérivation exacte : 232,379n, coalescence 3 lanes : 233,807n) → observé ≈ **2× la baseline** (granularité des feuilles, à mesurer en fonction de `--leaf`).
- **Sortie** : n=4000 → 1 459 968 supports = 365/point (baseline totale q2+q3+q4 : `480,340886n`).
- **8 mutants** raccordés ; enseignements durables : `front-no-ext` **survit sur uniform** (le terme `ext/4` ne se voit que sur boîtes allongées → familles anisotropes obligatoires) ; `front-q4-only` exige la fixture `q4only` (8 points au milieu d'une longue paire : q4 meurt, q2 reste pertinent avec p=8) ; fixture `ties` = tétraèdre régulier `(0,0,0),(2,2,0),(2,0,2),(0,2,2)` (6 arêtes maximales, 25 doublons sans règle canonique).
- **Portage device** : `anchor_pipeline.hpp` sans STL/allocation/récursion en `MHGP_HD` ; le kernel `.cu` appelle `run_anchor_point` — **même fonction** hôte/device ; porte de parité sur clés triées + census `(p,extra)` + 25 compteurs ; capacités fixes préflightées, dépassement = refus code 3, jamais troncature.
- **Objectif d'échelle post-50k** (fixé par l'utilisateur) : dizaines de millions de points sur la même G4 → `DensePointIndex:u32` + clé 128 bits dès `n>65 535` ; à `10^7` points, ~`4,80340886e9` supports : **le fold H0 doit consommer en flux, sans catalogue global**. Le producteur par ancre est compatible (travail local, sortie exacte-une-fois streamable).

### 1.8 `NOTE_SOLUTION_LOCALITE_CERTIFIEE_INVERSION_20260812.md` (auditeur, spécification)

- **Lemme de l'antipode** : `z∈int(B) ⇔ ||s||²<D(u·s) ⇔ d<D·cos∠(u,v)` ; par inversion `zeta(z)=s/||s||²`, condition affine `u·zeta(z)>1/D` : les boules par `x` à `<=K-1` intérieurs = niveau `<K` du nuage inversé local (motive une recherche output-sensitive, ne borne rien).
- **Certificat par calottes** : `C_z(D)={u∈S²: u·v>d/D}` ; si toute direction est couverte par `>=K` calottes strictes à rayon `r`, toute boule par `x` à `<=K-1` intérieurs a `diam<r`. Seuils 10/9/8 par arité. Impossible aux ancres extrêmes de l'enveloppe convexe / nuage coplanaire → succès partiel avec fallback, jamais condition globale.
- **Discrétisation exacte** : subdivision géodésique de l'octaèdre, sommets entiers `|g_x|+|g_y|+|g_z|=m` ; `rho_C(z)=max_g ||g||·||s||²/(g·s)` ; test sommet : `g·s>0 ∧ (g·s)²r²>||g||²(||s||²)²` (i128 sur u16) ; convexité géodésique des calottes <90°.
- **Jung ferme l'intériorité, pas les partenaires** : `R<=diam(S)·√(3/8)`, `D²<=(3/2)diam²`. Contre-fixture **q2 ne ferme jamais q3/q4** : `A=(100,100,100), B=(200,100,100), C=(150,180,100)`, `W_j=(80,140,96+j)` j=0..9 : les dix `W_j` sont strictement intérieurs à la boule diamétrale de `AC` (excès `(W_j-A)·(W_j-C)=-200+(j-4)²<0`) donc q2 mort, mais tous strictement extérieurs au circumcircle de `ABC` (excès `2050+(j-4)²`) : `ABC` reste une activation q3 sans intérieur.
- **Exigences d'une source directe** : publier support + provenance, `I` complet, shell global + politique extra-shell, `BallKey` + niveau, owner exact-once indépendant du scheduling, statut de fenêtre ; un juge de cardinalité ne suffit pas ; `beta(F)<beta(Q)` n'implique aucune inclusion entre boules de centres différents.

### 1.9 `NOTE_SOLUTION_SOURCE_ANCRE_MAXIMALE_ENVELOPPE_20260812.md` (Claude, spécification pré-implémentation)

- **Décision d'orientation** : abandon de la lane cellules comme chemin produit — au point gelé `uniform n=50 000` : **839 582 666 géométries pour 21 395 212 supports = 39,242 occurrences/support**. But : `occurrences = SupportKey_unique` **par construction**.
- **Lemme A (disque de Jung de l'arête maximale)** : `(a,b)` arête maximale de longueur `D`, `w=2(c-m)` : `w·(b-a)=0` et `||w||²<=D²/3` (q3), `<=D²/2` (q4) (Jung : `R<=D/√3` triangle, `R<=D√(3/8)` tétraèdre ; bornes atteintes).
- **Lemme B (boule de milieu universelle)** : `B(m,rho_q)⊂B(c,|c-a|)` pour tout centre du disque, `rho_3=D/√12`, `rho_4=D/√15` ; d'où les trois tests entiers i64 et les seuils de mort **10/9/8**. Aucune boule témoin ne contient `a` ni `b` (`D/2>D/√12`).
- **Lemme C (cône de préfixe)** : profil `r_q(u)=(√(a_q²+4u-4u²)-a_q)/2`, `a_3=1/√3`, `a_4=1/√2`, `r_q(u)/u` décroissant ; cônes tronqués `theta_3=30°`, `theta_4=27,368°` [corrigé ensuite : forme axiale non sûre, forme distance-euclidienne sûre]. **Rayon de coupure certifié par (point, chambre)** : `h_q` témoins dans le sous-cône à distance `<=d` ⇒ toute paire de la chambre avec `D>=2d` est morte ; `+∞` tant que la chambre n'a pas ses témoins (chambre ouverte = énumérée exactement, réponse assumée à `eight_clusters`).
- **Théorème D (filtre theta)** : `theta` = 9e plus grande valeur de `L_z` ; `U*_z<theta` ⇒ écarté sans perte de census — prouvé ensuite **redondant** par l'auditeur (§1.2). **Théorème E** : les carriers ne sont jamais écartés ; centre q3 = point de la droite `ell_x` le plus proche de `m`.
- **Ordonnance** : LBVH + banque de chambres → liste de voisins triée par distance bucketée par chambre → masque de lane (Lemme B) → enveloppe mobile sur le préfixe `<=1,2248·D` → q2 census direct / q3 minimum auto-centré par droite / q4 intersections de droites → tests exacts → événements H0 → dix forêts → payload.
- **Attentes mesurables** : candidats de paires/point `~(4π/3)λ³ ≈ 1100–1700` ; sites évalués baseline `13 831,22·n` ; supports `≈440n` (22 M à 50 k). Gate : deux pentes `<=1,35` sur `uniform` **et** `eight_clusters` à `12 500/25 000/50 000`.

### 1.10 `AUDIT_CONNECTIVITE_ORDER_K_A8111F0.md` (auditeur, preuve épinglée, 09/08)

- Théorème conditionnel : le 1-squelette des sommets de niveau `<=k` d'un arrangement affine fini est **connexe** ; en arrangement simple de dim 4, arêtes finies = sommets consécutifs partageant trois hyperplans ; un germe de niveau zéro suffit à parcourir sans excursion au-dessus de `k`.
- Concerne le vrai arrangement, pas une représentation par quatre identifiants ; préconditions non transférables (coquille/germes/sommets multiples exacts).
- Campagne historique : 10 800 arrangements rationnels génériques (5–8 points), aucun contre-exemple — corrobore, ne qualifie rien.
- **Aucune conclusion de coût : un parcours peut rester en `Theta(n·V)`.**

### 1.11 `AUDIT_ORDER_K_FLATS_9C587E6.md` (auditeur, snapshot épinglé, 09/08)

- **Descente de rayon réfutée** : fixture `A=(0,0,0), B=(0,3,0), C=(2,1,0), P=(1,1,0), Q=(1,1,2)` : `P` strictement intérieur (`in_circle=-72`) mais `R²(ABC)=R²(ABP)=R²(BCP)=R²(CAP)=5/2` — quatre rayons égaux ; sur 120 permutations, 90 construisaient un germe, 30 rendaient `germe_non_certifie` ; l'égalité sans ordre bien fondé peut **cycler**.
- Le plafond historique `q*q+8` n'avait pas de preuve de complétude et débordait un `int` signé dès `q>=46341`.
- **Coordonnées dupliquées** : fixture `(0,0,0)×2, (2,0,0), (0,2,0), (0,0,2)` — l'échange des doublons conservait 11 records mais changeait 4 supports → décision durable : **refus explicite des coordonnées dupliquées** tant qu'aucune sémantique quotientée n'est reçue.

### 1.12 `AUDIT_SOURCE_DIRECTE_24AD3D37.md` (auditeur, reçu immuable, 09–10/08)

- Quatre défauts historiques devenus invariants : (1) `--judge 0` annonçait une égalité sans exécuter le juge ; (2) une map indexée par coquille seule écrasait des émissions doubles (mutant bidirectionnel : 126 émissions au lieu de 56) ; (3) le payload comparait la **taille** de `members`, pas les identifiants ordonnés ; (4) absence mathématique et refus de ressource partageaient un statut.
- Invariants permanents : modes mesure/juge/cover exclusifs et explicites ; ledger = une émission canonique par objet, pas une somme ; payload complet (membres, offsets, sources de forêt, statuts de refus) ; un chrono n'inclut jamais un juge d'un seul côté ; count-only ne qualifie rien.
- « Cette famille reste un oracle CPU borné, jamais une architecture 50 k. »

### 1.13 `AUDIT_VOIE_MULTIPLICITES_ORDER_K.md` (auditeur, preuve épinglée, 09/08)

- Objets **distincts** à un sommet : intérieur strict `B(v)`, coquille `S(v)`, niveau de navigation `|B(v)|`, rang fermé `|B(v)|+|S(v)|`, support canonique — le rang fermé filtre une publication, il **ne coupe pas la navigation**.
- **Théorème de propriétaire** : `U` support affinement indépendant, `1<=q<=4`, nuage de dimension affine 3 : le polyèdre de signes dans le flat de `U` est pointé, possède un sommet `o(U)` avec `B(o(U))⊆B_U` ; d'où `ell<=k_nav=smax-2` pour le catalogue de rang fermé `<=smax`.
- Propriétaire canonique par optimisation rationnelle exacte + tie-break lexicographique ; retire la table globale de propriétaires ; **ne borne ni le nombre de flats, ni le census, ni le temps à 50 k**.

### 1.14 `NOTE_GATE_D_PREMIERES_INCIDENCES_DU_COEUR.md` (auditeur, preuve statique, 09/08)

- Dichotomie exacte pour une facette cœur `F` (miniboule `B_F`, niveau `b_F`, `E_F=(B_F∩X)∖F`) : si `E_F≠∅`, premier niveau = `b_F`, co-minimiseurs = `F∪{x}` pour `x∈E_F` ; si `E_F=∅` **et** source directe ouverte complète et terminale, premier niveau = min des cofaces directes incidentes, tous les ex æquo conservés.
- Implémentation : streamer les suppressions de cofaces, grouper par clé de facette + niveau exact, census fermé complet ; plafond atteint ⇒ facette `unresolved` ; **un préfixe n'est jamais un minimum**.
- Évite l'étoile globale ; ne prouve ni la source directe terminale ni sa parcimonie.

### 1.15 `NOTE_GATE_D_UNE_ATTACHE_PAR_FACETTE_COEUR.md` (auditeur, preuve statique, 09/08)

- Sous la porte régulière forte, pour `F` cœur avec `>=2` intrus stricts : prendre les deux plus petits intrus `z_F,w_F` et un essentiel `u_F` ; le carrier `T_F=(F∖{u_F})∪{z_F}` a un niveau **strictement inférieur** ; une unique attache canonique vers sa composante induit la même partition que tous les co-minimiseurs silencieux, après contraction atomique du lot.
- Hypothèses non négociables : support minimal unique, census terminal, aucune égalité extérieure pertinente, resolver complet.
- Ne restitue ni les identités Gamma omises, ni les facettes non-cœur, ni les verticales — quotient horizontal normalisé H0 **seulement**.

### 1.16 `NOTE_PARENT_LOCAL_REVERSE_SEARCH_GATE_D.md` (auditeur, preuve statique, 09/08)

- Parent local de reverse search : au sommet `v` (coquille `S(v)`, intérieur `B(v)`, germe canonique, oracle `next(v,d)` exacts), un **programme linéaire rationnel en dimension 4** sur le cône tangent choisit un rayon extrême canonique ; le premier événement dans cette direction est un parent adjacent unique.
- Potentiel anti-cycle : le niveau baisse, ou la forme d'un intérieur choisi croît strictement (niveau >0), ou la fonction du germe décroît (niveau 0) → tout sommet shallow atteint le germe **sans table globale `seen`**.
- Couvre les sommets multiples seulement si coquille/intérieur/rangs/premier lot sont exacts ; ne ferme ni l'énumération des enfants, ni le census, ni le contrat 50 k.

### 1.17 `NOTE_POSITIVE_INDEX_KD_EXACT_ET_CERTIFICAT_PINCEAU.md` (auditeur, preuve statique, 09/08)

- Boîte vs boule rationnelle (centre `C/d`, rayon carré `N`) : distances min/max exactes axe par axe ; strictement extérieure si somme des carrés `>N`, strictement intérieure si `<N` ; **toute égalité descend**. Sur u16, les carrés intermédiaires atteignent ~`2^182` → multiprécision reçue obligatoire après promotion.
- Fixture permanente du filtre flottant réfuté : `a=(32767,32767,0), b=(57863,57862,0), c=(7672,7673,0), d=(60104,30135,1)` exactement cosphériques ; une enveloppe flottante historique élaguait la racine.
- Pinceau : la puissance d'un point est **affine** dans le paramètre du pinceau ; tout événement intérieur à un segment change de signe entre les deux sphères terminales (sauf points constants du flat) ; requête de signes + tri rationnel suffisent — ce n'est pas la différence symétrique des boules fermées.

---

## 2. Synthèse transverse

### 2.a Résultats mathématiques établis (définitions, théorèmes, formules exactes)

**Réduction par arête maximale (le cœur de la V4)** :
- Tout support propre positif possède au moins une arête de longueur maximale ; l'**arête maximale canonique** (la plus longue, tie-break plus petit `(min PointId, max PointId)`) est unique ⇒ émission exacte-une-fois par construction, sans RLE/lift/owner de cellule.
- **Lemme A** : `w=2(c-m)` ⊥ `(b-a)` et `||w||²<=D²/3` (q3), `||w||²<=D²/2` (q4).
- **Lemme B** : boules de milieu universelles `rho_3=D/√12`, `rho_4=D/√15` ; tests témoins entiers avec `u=2z-a-b`, en i64 sous u16 : q2 `u·u<D²` (seuil de mort **10**), q3 `3(u·u)<D²` (**9**), q4 `15(u·u)<4D²` (**8**) ; équivalents en marge `g=D²-u·u` : `g>0` / `3g>2D²` / `15g>11D²` ; `g=0` = extra-shell q2.
- **Spindles complets** : `W_3={z: g>0 ∧ 3g²>4Q}`, `W_4={z: g>0 ∧ g²>2Q}` avec `Q=D²(u·u)-(u·d)²` ; convexes ; AABB fermée incluse ⇔ 8 coins stricts.
- **Lift `A×B×C`** : `H(a,b,z)=(b-z)·(z-a)`, `R=||(b-a)×(z-a)||²` (`g=4H`, `Q=4R`) ; q3 : `H>0 ∧ 3H²>R` ; q4 : `H>0 ∧ 2H²>R` ; `Hmin` en 24 évaluations, `Rmax` sur 512 triples de coins ; certificat `ALL` sûr, `UNKNOWN` possible. Variante économe : `R=E2·X2-H²` ⇒ q3 : `4H²>E2·X2`, q4 : `3H²>E2·X2`.
- **Cônes cibles depuis le témoin** : `C_3(a,z)`/`C_4(a,z)` = cônes de Lorentz d'apex `z`, axe `z-a`, demi-angles **60°** (q3) et **arctan(√2)=54,7356°** (q4) — angle mesuré de `z` vers la cible `b`, jamais de `a` vers `z`.
- **Angles exacts** : `gamma_3<arccos(√(2/3))=35,2643896828°`, `gamma_4<arccos(√(11/15))=31,0909303577°` ; `theta_3=30°`, `tan(theta_4)=√(2-√3)`, `theta_4=27,3678051586°`. **Aucun décimal arrondi ne décide** ; seules les formes entières `H>0 ∧ c·H²>R` sont l'autorité.
- **Banque 432 sous-cônes** (48 chambres Yao × 9) : `min cos²(gamma)=9/11>11/15` ; fenêtre annulaire q4 : `D²<=9r² ∧ 4r²<=D²` (soit `1/3<=r/D<=1/2`), preuves sans racine `2809·11<9·3600` et `169·11<9·225`.
- **Théorème D/theta** : correct mais **strictement redondant** après `U_z<0` sur toute ancre vivante (preuve : ancre vivante ⇒ `theta<=0`).
- **Séparation de blocs (utile WSPD)** : rayon commun positif ⇔ `d>2s` (q2), `d>s(1+√3)` (q3), `d>s(1+√15/2)` (q4), `s=r_A+r_B`.

**Acuité et carriers** :
- q3 : support propre positif ⇔ triangle **strictement aigu** (test i64 avant lift).
- q4 : tout tétraèdre propre positif a **≥2 faces aiguës** (borne 2 optimale, fixture) ; un q4 positif d'arête maximale `(a,b)` a au moins une face positive parmi `abx`,`aby` (Théorème 5, employé par le producteur ancre) ; la face canonique (3 plus petits IDs) peut être obtuse — ne jamais conditionner q4 à q3.
- Centre q4 depuis carrier q3 : `[2(n·v)N + n(G·Delta - 2⟨N,v⟩)]/[2G(n·v)]` ; centre q3 = point de la droite-axe le plus proche du milieu `m`.

**Localité de cellules (héritage v3 réutilisable comme oracle)** : lemme budget–cellule `beta<=R_p(C)`, `I_B∪U_B⊆D_p(C)` ; `A_q=D_(smax-q)` ; invariant pool-relative ; scores affines à jauge fixe ; certificat d'expansion `Lambda(H+1)`.

**Indépendances et pièges prouvés** : arités générativement indépendantes (fixtures q3-sans-q2 et q4-sans-q3) ; la lane q2 ne ferme jamais q3/q4 (fixture W_j) ; le niveau du support ≠ niveau de son owner (fixture ABCD) ; `beta=R` admissible ; ties toujours conservés ; miniboule critique ≠ face shallow (`Phi(c,lambda)=||c||²-lambda`).

**Comptage / bornes** : baseline Poisson Source S = `(175+495π²/16)·rho|Omega| ≈ 480,341/point` (q2 : 40 ; q3 : 218,275 ; q4 : 222,066) ; théorème de séparation deux-droites : `V_{<=9}=55m²-440m+715` (quadratique) vs Source S `20m-55` (linéaire) — le plein arrangement n'est ni la sortie ni un minorant ; plafonds owner `ell<=9/8/7` par arité (`k_nav=smax-2` global) ; bornes terminales conditionnelles 13/12/11 (78/220/330) ; Kruskal–Katona et `4Q_4<=(m_4-3)T_4` ; cliques d'intervalles `N_q=Σ C(a_i,q-1)` ; token de plateau Johnson `P_k=Σ_g C(|S_g|,k)` (≤2046 par générateur sur k=1..10) ; EGS : `<=C(p+3,3)` tétraèdres p-hefty contenant un point générique.

**Largeurs arithmétiques prouvées (u16, smax=11)** : tests de lane i64 ; `|H|<2^34`, `R/2H²/3H²` ≤69 bits → u128 ; scores affines `<2^62` (i64) ; bornes dyadiques `<2^(34+2d)` (i128 si `d<=26`, pas u64) ; cisaille `|B'|<2^52` pour `m<=50 000` ; signe 3e ligne `<2^126` (i128) ; **ordre de deux événements rationnels jusqu'à ~2^178** → 192 bits signés minimum en cold path ; boîte/boule u16 jusqu'à ~`2^182` → multiprécision.

**Gate D / navigation (preuves épinglées)** : connectivité shallow du 1-squelette `<=k` ; parent local par LP rationnel dim 4 sans table `seen` ; dichotomie premières incidences (`E_F≠∅` vs cofaces directes) ; une attache par facette cœur (quotient H0 seulement) ; puissance affine le long d'un pinceau.

### 2.b Mesures chiffrées et décisions d'architecture

- **Contrats** : `smax=11`, `K_max=10` ; SLO principal `p95 warm_e2e<100 ms`, secondaire `<1 s`, sur `BenchmarkOutputContract-v1` **complet** (dix forêts, verticales, lots, certificat minimal — `hgp_reduced_normalized_h0_v3` n'est qu'une sous-porte diagnostique) ; familles bloquantes : **Poisson uniforme ET mélange équilibré de huit amas** (plan de tests §14.5) ; terrain/scanline = diagnostics ; rampe contractuelle `12 500/25 000/50 000`, gate = deux pentes successives `<=1,35` par compteur dominant + caps absolus. Objectif post-50 k : **dizaines de millions de points** ⇒ index u32, clés 128 bits, fold H0 en flux (à `10^7` : ~4,8e9 supports, aucun catalogue global possible).
- **Lane cellules de centres** : gelée à `uniform n=50 000` : 839 582 666 géométries / 21 395 212 supports = **39,242 occ/support** → abandonnée comme produit, conservée comme comparateur d'identités/falsificateur borné. Rampe 100/200/400 : pentes >1,35 ; 85,7–93,48 % des lifts meurent à l'owner ; `lifts_built` sous-compté de 52,85 %.
- **Producteur ancre maximale** (successeur) : vérifié exhaustivement à n=60–120 sur 4 familles ; `candidate_pairs/n` = 227/351/465 (n=500/1000/2000) ≈ **2× la baseline pointwise 231,6–233,8** (granularité de feuille) ; n=4000 : 365 supports/point (baseline 480,34).
- **`eight_clusters`** : front midball **inerte** (0 prunes, `C(n,2)` paires), pentes 2,59–2,71 (paires q4), 2,78 (interior_tests), 3,05 (temps) → régime « compatible cubique » ; cause : témoins cherchés dans le vide inter-amas ; le contre-exemple spindle montre que les témoins près des amas existent.
- **`uniform`** : le mur est le **census** : `interior_tests ≈ supports × mean|kept|` (349,5 à n=2000) ; `kept` h.w. 394/515/748 (pente 0,46, non bornée démontrée) ; extrapolation 50 k : ~1,3e10 prédicats i128.
- **Mort par budget au plus tôt** : sorties identiques, wall 92,458→33,531 s (`eight_clusters n=500`), facteur 1,4–2,8 constant, pente inchangée.
- **Sonde cônes M-NN** : mort 21,6 %→71,2 % selon (n, M) ; croît avec n.
- **theta** : redondant, désarmé ; garde de densité : réfutée par ablation, hors chemin produit.
- **G4/CUDA** : **aucun kernel jamais exécuté** dans toute la tranche ; NO-GO systématique avant : juge indépendant, payload officiel, rampe contractuelle, fermeture des compteurs ; parité device = même fonction `run_anchor_point` compilée deux fois + comparaison de 25 compteurs déterministes.
- **Mémoire** : bitset dense 50 k = 298,3 MiB (refusé) ; préflight d'octets obligatoire ; `resource_exhausted` jamais troncature ; CTests : 468→482→550→560→573 selon les pins.
- Machine de dev : 2 cœurs, temps « contaminés », jamais qualifiables — seuls les compteurs font foi.

### 2.c Bugs, erreurs, rétractations documentés

1. **Seuil q4 `31,134°` faux** (note Claude) — l'arrondi décimal n'était pas sûr ; fixture `(58,35,0)/(29,0,0)`.
2. **Lemme C : troncature axiale ≠ distance euclidienne** — le cône fermé tronqué axialement sort du spindle ouvert ; fixture `(2778,0,0)/(1389,719,0)` sous le faux `27,368°`.
3. **`54,74°` mal ancré** : exact depuis `z` vers `b`, faux depuis `a` vers `z` à distance finie (il faut la borne radiale).
4. **Baseline candidate 463 retirée** (Claude) : degré dirigé confondu avec pointwise ; vraie baseline ~231,6 ; l'observé est 2×.
5. **Certificat de front : perte d'un facteur 2 sur le rayon** (première version : coupure `10,4·rho^{-1/3}` au lieu de `4,8`, 8× trop de candidats) — corrigée.
6. **Moteur `pipeline` (chemin nvcc) sans compteurs de rejet** : cinq compteurs absents publiés comme zéro (« un compteur absent n'est pas un compteur nul »).
7. **Garde de densité** : aucune économie réelle, coût net — réfutée par ablation, retirée du chemin.
8. **theta** : crédité comme réduction ; démontré redondant (`theta_only_prunes_on_live=0` causal).
9. **Colonnes de front non reproductibles** sur ELF pincé (provenance) — reconstruites depuis `C(n,2)` au lieu d'être mesurées.
10. **`lifts_built` sous-compté de 52,85 %** (chaque test d'axe construisait un second `TriangleLift` non compté) ; `tri_in` jamais alimenté (lift recalculé).
11. **`--fixtures` CLI cassé** (CMake passait `--fixtures`, parseur exigeait `--fixtures=…`) : porte verte-par-accident 7/8 ; mutant `drop-ties` survivant ; `strata-stop` enregistré sans CTest.
12. **Tampon shell fixe `int in_ids[24]` + `exit(3)`** : incompatible avec `|U_B|` arbitraire (fixture 30 points cosphériques).
13. **Cascade inter-arités réfutée** (q3 depuis bras q2 « certifiés », q4 depuis q3 « reçus ») — deux fixtures permanentes.
14. **Score de split `E+9T` sans K4** : fixture `K_24` (`E+9T=18 492` passe cap 20 000, vrai coût 70 104) ; `Q/T=(m-3)/4` non borné.
15. **Descente de rayon** (order_k_flats) : cycle possible sur rayons égaux (fixture R²=5/2 ×4) ; cap `q*q+8` sans preuve + overflow int à `q>=46341`.
16. **Source directe historique** : `--judge 0` mentait ; map par coquille écrasait des émissions (126 vs 56) ; payload comparé par taille.
17. **Enveloppe flottante élaguait une racine cosphérique exacte** (fixture 4 points u16 énorme).
18. **`--harvest` : filtre de rang fermé** `inside+shell>smax` perdait la Source S extra-shell (`p+q<=11 < p+|U_B|`).
19. **Successeur spindle/cône du 13/08 refusé** : conversion `smax` → faux prune, cardinalité silencieusement réduite, juge incomplet par lane, résiduel non rejouable, ABI CUDA cassée, trois pentes rouges (lemme admis, producteur refusé).
20. **Note de solution cellules v1** : confusion `e0`/curseur `h`, partition terminale commune non exigée, « potentiel exact » surclamé, 4 fixtures annoncées / 5 listées — corrigés.

### 2.d Pistes fermées dans cette tranche, et pourquoi

- **Front intégral / matérialisation des sommets shallow de l'arrangement** : théorème de séparation (34,4 milliards de sommets vs 500 k supports à n=50 000) — le volume de navigation n'est ni la sortie ni un minorant. Fermé définitivement.
- **Lane « cellules de centres » comme chemin produit** : 39,242 occurrences/support au point gelé ; plus cher que l'exhaustif dès n=50 dans certains régimes. Conservée uniquement comme oracle/comparateur CPU.
- **Filtre `theta` (top-k d'enveloppe)** : mathématiquement redondant sur toute ancre vivante ; désarmé.
- **Garde de densité heuristique** : réfutée par ablation ; hors chemin produit (au mieux harness diagnostique désarmé).
- **Certificat de front « boule de milieu seule »** : inerte sur `eight_clusters` (témoins cherchés dans le vide) ; remplacé par les spindles complets `W_q` et les cônes depuis témoins.
- **Cascade générative q2→q3→q4** : réfutée par fixtures ; les lanes doivent être générativement indépendantes.
- **`U_B` comme clé chaude de déduplication** : n'existe qu'après census, taille `Theta(n)` ; remplacée par le 5-uplet homogène primitif.
- **Cliques d'intervalles comme réduction sparse** : filtre exact mais pire cas `Theta(m^4)` ; bitsets denses non-sparse (298 MiB à 50 k).
- **Voronoï/Delaunay local d'ordre ≤k comme fallback industriel** : recrée la mosaïque d'ordre supérieur interdite ; oracle borné seulement, non admis sans révision normative.
- **Proposer Delaunay par permutations aléatoires** : couverture `1/C(p+q,q)` (jusqu'à 1/330) ; hors chemin chaud.
- **Pinceau/DFS comme sweep produit** : `Theta(n)` pire cas par requête ; utile en oracle owner ponctuel seulement.
- **DFS spindle par `PairId`** : rescan quadratique après front quadratique ; seul le relèvement factorisé `A×B×C` est admis avant émission de `PairId`.
- **Balayage `M·C(n,2)` des banques de cônes** : « jamais une option ».
- **Yao48 pour q2 profond** : comparateur suspendu, non reçu comme source complète.

### 2.e Ce que la V4 doit conserver / éviter absolument

**Conserver (directement aligné avec la feuille de route V4)** :
1. **La réduction par arête maximale** est validée mathématiquement et implémentée avec accord exhaustif : Lemmes A/B/C, émission exacte-une-fois par arête maximale canonique, mort de lane par seuils **10/9/8** — c'est exactement le « support d'arité q trouvé par son arête maximale » de la V4. Les mutants et fixtures (`ties` tétraèdre régulier, `q4only`, `front-no-ext` sur familles anisotropes) sont à reprendre tels quels.
2. **L'élimination par h témoins en zone cœur** : les tests i64 `u·u<D²` / `3(u·u)<D²` / `15(u·u)<4D²` (zone cœur = boule de milieu) et, plus forts, les **spindles complets** `W_3/W_4` et la fenêtre annulaire `1/3<=r/D<=1/2` de la banque 432. Le seuil dépend de l'arité : h=10 (q2), 9 (q3), 8 (q4).
3. **L'élimination dual-tree h_a/h_b sur rectangles `A×B`** : le lift entier `H/R` sur `A×B×C` (24 évals pour `Hmin`, 512 coins pour `Rmax`, promotion u128), les cônes de Lorentz depuis les témoins (`iff` par 8 coins quand l'endpoint est ponctuel), la porte `NONE` sans racine, l'héritage des crédits sous split (antichaîne par lane, bit `q3_already_credited`, split de `C` ne recrédite jamais la masse) — jamais de quadratique `A×B`, jamais de DFS par paire.
4. **Les seuils de séparation** `d>2s` / `d>s(1+√3)` / `d>s(1+√15/2)` pour calibrer les paires bien séparées (WSPD s=6/8/10 : à confronter à ces constantes exactes).
5. **La mort par budget au plus tôt** (compte monotone d'intérieurs certifiés) — gain 1,4–2,8× démontré sans changer les sorties.
6. **q3 ⇔ triangle strictement aigu** (pré-filtre i64) et **≥2 faces aiguës par q4** (carriers) — mais q4 jamais conditionné au verdict q3.
7. **La discipline d'exactitude** : formes entières, jamais un décimal ; toute égalité `UNKNOWN`/descend ; ties conservés ; refus des doublons de coordonnées (`unsupported_degeneracy`, jamais de jitter) ; largeurs prouvées par prédicat (i64 → u128 → 192 bits → multiprécision) ; `resource_exhausted` jamais troncature ; capacités préflightées.
8. **La discipline de mesure** : juge indépendant `(BallKey,SupportKey,I_B,U_B,owner)` à arithmétique propre **avant** toute réception ; provenance pincée (SHA sources/ELF/commande/sorties brutes) ; compteurs nommés par certificat, jamais reconstruits ; identités de masse disjointes ; deux pentes `<=1,35` sur `uniform` ET `eight_clusters` ; planchers anti-vert-par-vacuité ; mutants raccordés en CTest à code exact.
9. **Le dimensionnement de charge** : ~480,34 supports/point attendus (24 M à 50 k, 4,8 milliards à 10^7) ⇒ fold H0 **en flux**, tokens de plateau Johnson pour les cosphères, index u32 + clés 128 bits, clé chaude = 5-uplet primitif.
10. **La parité device par fonction unique** (`run_anchor_point` compilée hôte et device) + comparaison de compteurs déterministes.
11. Les preuves gate D (dichotomie premières incidences, une attache par facette, parent local LP, pinceau affine) pour la **reconstruction de la forêt** en aval de la source.

**Éviter absolument** :
1. Toute matérialisation des transits/sommets de navigation (le plein arrangement est quadratique là où la sortie est linéaire).
2. Le census par rescan de liste par support (`interior_tests ≈ supports×|kept|` est le mur mesuré) — transporter les identités `always_inside`/conflits (cutting/enveloppe) ou census une fois par **boule** (RLE `BallKey`), jamais par support.
3. Les certificats de témoins **centrés dans le vide** (midball seule) sur régimes en amas ; toujours chercher les témoins près des endpoints (spindles/cônes).
4. Les décimaux arrondis, `i128` supposé suffisant pour l'**ordre** d'événements (2^178), les caps de profondeur comme résultat, les caps silencieux, les moyennes prises pour des maxima (`kept` ne sature pas).
5. Conditionner une arité à une autre (génération ou tombstone) ; supposer `|U_B|` borné par `smax` ; tampon shell fixe.
6. Publier des chiffres sans provenance rejouable ; créditer un filtre sans compteur causal de non-redondance ; un « compteur absent » publié comme zéro.
7. Ouvrir G4 avant : juge indépendant vert, payload officiel complet, rampe contractuelle CPU fermée.

---

## Questions ouvertes / ambiguïtés

1. **Facteur 2 de la granularité de feuille** : le producteur ancre laisse passer ~2× la baseline de paires candidates ; l'auditeur exige une mesure en fonction de `--leaf`, jamais faite dans cette tranche. Impact direct sur le volume de rectangles `A×B` que la WSPD V4 devra fermer.
2. **Croissance de `kept`** (pente empirique 0,46) : structurelle ou artefact du front incomplet ? L'auditeur répond « queue exponentielle en `rho·D³` sous PPP + vrai filtre spindle, mais le high-water ne converge pas nécessairement » — non tranché ; la V4 doit publier la distribution bucketée par `rho·D³`.
3. **Niveaux streamés vs cutting signée certifiée** : le choix est explicitement laissé « selon le vrai high-water » (ordre 6 de l'auditeur) ; aucun des deux n'a été reçu contre le juge dans cette tranche.
4. **Le producteur spindle/cône du 13/08** est refusé pour des défauts réparables (conversion smax, cardinalité, juge par lane, ABI CUDA) — son état final est hors de ma tranche (`AUDIT_CONTRE_AUDIT_SPINDLE_CONE_WORKTREE_20260813.md` non lu ici).
5. **Correspondance WSPD** : les audits ne parlent jamais de Callahan–Kosaraju nommément ; les objets les plus proches sont la banque de chambres Yao-48/432, le self-join canonique `A×B` à frontière persistante de nœuds `C`, et les seuils de séparation du Lemme 2. La traduction exacte « s=6/8/10 » ↔ `d>2s / d>s(1+√3) / d>s(1+√15/2)` reste à établir par la V4 (les constantes exactes des audits sont sur `s=r_A+r_B` et non sur le ratio WSPD standard).
6. **Extra-shell / plateaux** : aucun quotient de plateau reçu ne préserve à la fois H0, les verticales, les lots et le payload officiel ; le token Johnson n'est admis que pour le H0 normalisé. La V4 « forêt HGP complète K=10 » devra trancher la politique extra-shell (side queue, quotient, `unsupported_degeneracy`) — non résolue en v3.
7. **Périmètre du chrono SLO** : l'auditeur maintient que la cible d'une seconde est `BenchmarkOutputContract-v1` complet (dix forêts, verticales, lots, certificat) ; le contrat V4 « <100 ms K=10 » devra dire explicitement ce qui est dans le chronomètre.
8. **`mhgp3v_anchor_mutant_census` tué par signal** en campagne complète (~62 s) mais vert isolé : cause (RSS ? durée ?) jamais élucidée dans cette tranche.
9. **Baseline Poisson et régimes réels** : la formule `480,34·rho|Omega|` est bulk/continu ; bords, quantification u16, doublons, surfaces et amas sont hors hypothèses — les 24 M à 50 k sont un ordre de grandeur, pas une identité. Pour les dizaines de millions de points de la V4, seule l'extrapolation linéaire existe.
10. **Attribution auteur** : les `NOTE_SOLUTION_LOCALITE_*` et `NOTE_ARCHITECTURE_GPU_*` portent `mode=audit_independant_math_and_architecture` (auditeur), tandis que `NOTE_SOLUTION_SOURCE_ANCRE_MAXIMALE_*` porte `mode=proposition_math_non_recue` (Claude) — je l'ai lu ainsi ; les fichiers ne signent pas nominalement.
