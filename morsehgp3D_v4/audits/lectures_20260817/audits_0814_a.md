# Lecture des audits v3 du 14/08/2026 — tranche A (9 fichiers)

Cadre commun à tous les fichiers : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`. Tous ces documents sont écrits par **l'auditeur**
(contre-audits indépendants) et jugent le travail live de **Claude**
(prototypes, deltas de worktree, commits). GCP non utilisé par l'auditeur.
Convention de rang partout : régime régulier `smax=11` / `K_max=10` ; seuils de
fermeture par témoins intérieurs stricts : **8 pour q4, 9 pour q3, 10 pour q2**.

---

## 1. Fichier par fichier

### 1.1 `AUDIT_CONTRE_RECEPTION_M4_V2_DEPTHBLOCK_5BFC5C8_20260814.md` (1148 lignes)

- **Qui/quoi** : l'auditeur contre-reçoit le sampler `CarrierApexEstimator-v2` et
  `q4_brute_oracle` de Claude (pin `5bfc5c8`, deltas jusqu'à `cec4a4f`). Verdict :
  le verrou conceptuel M4 est levé, mais le résultat utile est « **prouver la
  profondeur avant le fill et ne jamais créer ce nombre** (quartique) ».
- **Sampler réfuté** : à `K=1` tirage, l'intervalle `±2σ` a largeur nulle autour
  d'une valeur fausse (`C4=1560±0` vs exact `4652` ; `M4=4680±0` vs `60280`).
  Le claim « exhaustif quand K>N » est faux (`n=120`, `N=6917`, `K=20000` :
  quadrature `119520,2±1189` vs exact `119669`). Le compteur `doublons=3` ne
  détecte que des rangs égaux consécutifs alors qu'au moins 13 083 tirages
  répètent. Le delta live remplace `2σ` par Hoeffding (demi-largeurs `48240` /
  `892500` à K=1) — réparé comme formule, pas comme loi (multiply-high sans
  rejet, δ non réparti, W4 sans intervalle).
- **`q4_brute_oracle`** : le claim « `M4=Θ(n^4)` pour tout nuage » est réfuté par
  `two_lines` (`M4=0`). Mesures exhaustives : `M4/C(n,4)` = 0,6429 (uniform n=60),
  0,6610/0,6598/0,6577 (eight_clusters n=60/90/120) ; `W4/M4` = 0,1206–0,1508 ;
  tests `I<=7` par point = 26,8–39,8. L'extrapolation `H4≈30n` est un modèle
  exploratoire, **pas** une borne (pas de borne linéaire universelle pour la
  Delaunay d'ordre fixé en 3D).
- **`BlockBallDepth8`/Corner8 (réponse Q7)** : pour `O(a,b,x,y)=det3(u,v,w)` et
  `J=det4((u,||u||²),(v,||v||²),(w,||w||²),(r,||r||²))`, `z` strictement intérieur
  ssi `O*J<0` (produit interdit, signes jugés séparément). À signe d'orientation
  fixe, `sigma*J` est strictement convexe en `z` → **les 8 coins d'une AABB témoin
  sont complets pour `ALL_INTERIOR`**, jamais pour `NONE`. Bornes u16 :
  `|O|<6·65535³<2^51`, `|J|<=72·65535⁵<2^87`, BallForm `E<720·65535⁵<2^90` — i128
  suffit. Fixture u16 gravée (5 boîtes `{0,1}³` autour de 20000/30000…, 4096
  supports) : marge owner min `11892000`, barycentrique min `13217143/721310286`,
  pire `J_U=-79011820908103787995` aux 8 coins.
- **`BlockJungDualTile` sans fractions** : `d=b-a`, `D=||d||²`,
  `A=W·D-Σ w_z·||a+b-2z||²`, `P=W(a+b)-2Σ w_z z`, `R=D||P||²-(P·d)²` ;
  **q3 : `A>0` et `3A²>4R` ; q4 : `A>0` et `A²>2R`**. Fixture collective q4 :
  `a=(0,0,0), b=(100,0,0), z1=(42,26,0), z2=(42,0,26)`, poids `(1,1)` :
  `A=14080, R=54080000, A²=198246400>2R=108160000` (aucun singleton ne passe).
  Fixture poids : `(3,1)` ferme q4 (`A=64,R=1872`) là où `(1,1)` (`A=32,R=720`)
  ne dépasse que q3 ; égalité `A²=2R` = shell.
- **Profondeur = transversal** : `Depth(P,h)=AND_{z∈G} Depth(P∖{z},h-1)` ; pire
  arbre `(3^h-1)/2` = **3280 nœuds (h=8), 9841 (h=9)**. Fixture cruciale : 6
  témoins universels + 3 gadgets → **profondeur 8 mais packing disjoint maximal 7**
  (le packing de 8 groupes disjoints n'est pas une décision complète). Identité
  exacte pour paire fixe : **`d=tau(E)`** (transversal minimal de l'hypergraphe
  des bases de Helly couvrantes), avec `nu(E)<=tau(E)<=d`. Noyau tolérant
  (Montejano–Oliveros th. 3.1 + Tuza) : `eta(3,h)<(h+1)²` → **80 IDs suffisent
  toujours pour q4, 99 pour q3** (existentiel ; ne rend pas la recherche gratuite).
- **Solution fixe-face (§5.7)** : après face aiguë fixée, les centres q4 forment
  un segment `J_f` ; `T2=D·(G-2(E-F)²)`, `J_f={tau : 2tau²<=T2}` (q3 : `3`).
  Un noyau top-k exact de **16 IDs (q4) / 18 (q3)** (`p+2k=2r-p`) décide
  `Depth>=r` — remplace le DAG à 3280 appels par un payload constant. Ordres de
  seuils ~155 bits, bouts ~207 bits → i256.
- **`BlockJungDual64` (§5.9)** : `A0=-L(a·b)+(a+b)·Z-Q`,
  `C0=L(a×b)-a×Z-Z×b` ; q4 : `A0>0 && 2A0²>||C0||²` ; q3 : `3A0²`. `(A0,C0)`
  bi-affine en `(a,b)` + cône de Lorentz convexe → **64 couples de coins
  nécessaires et suffisants** sur l'enveloppe AABB continue, à base et poids
  fixes (reproposer des poids par coin est interdit). u16 : `|A0|<=3LU²<2^50`,
  `|C0_i|<=2LU²<2^49`, i128 suffit pour `L<=65535`.
- **M4 par intervalles (§6)** : identité exacte par arête
  `M4_e=E_e(V_e)-E_e(N_e)-Σ_pi(E_e(V_{e,pi})-E_e(N_{e,pi}))` ; checksum global
  `Q_aff` = nombre de 4-sous-ensembles affine-indépendants. Masse injective d'un
  atome par **Möbius sur les 15 partitions des 4 rôles** ; comptabilité
  `M4_pending/M4_L/M4_U` conservée aux splits. **Interdit** : RLE projectif
  global (à 50 000 : 1 249 975 000 arêtes ≈ 10 Go ; 62 496 250 050 000
  incidences ≈ 1 Po). Jamais saturer les termes d'une soustraction séparément.
  `ApexWellCenteredBlock` : 3 inégalités fermées (~174 bits) pour la positivité.
- **§5.8** : le claim de Claude « gain primal rigoureusement nul » est réfuté par
  son propre binaire (`eight_clusters n=600` : dual 169 vs primal 170 fermées ;
  `n=1500` : 189 vs 190). La porte `dual_fixture` imprime le littéral `d=8` au
  lieu de le calculer. Fixture Helly `k=3` nécessaire donnée
  (`z1=(5,90,100), z2=(5,100,90), z3=(0,110,110)` : seul le triple couvre).
- **Fallback shallow (§7)** : niveaux `0..7` des lignes `F_z(c)=2c·(z-a)-||z-a||²`
  dans le plan médiateur ; borne historique `< e·(k+1)·m` centres shallow et
  `< 2e(k+1)m` incidences shell pour `m` formes, profondeur `k`.

### 1.2 `AUDIT_CONTRE_RECEPTION_SUPPORT_COMPLET_CORNER8_WST34_22D1CB0_20260814.md` (840 lignes)

- **Rétractation majeure (CAUTION)** : la révision `3703097` de Claude est
  « mathématiquement rouge » — le filtre d'acuité affirme `E+X-D=2H` pour
  `H=(x-a)·(b-x)` alors que l'identité exacte est **`E+X-D=-2H`** (aigu ssi
  `H<0`). Il éliminait le côté aigu ; toutes ses mesures (dont le gain `1,62x`)
  sont invalides. Réparé par `a73161c` + fixture tétraèdre régulier
  `(0,0,0),(0,1,1),(1,0,1),(1,1,0)` (`H=-1` pour ses deux carriers).
- **Théorème central (intuition de Louis reçue après correction de
  vocabulaire)** : ce n'est pas le nombre de sites sur le shell mais le **support
  minimal positif complet** qui fixe l'arité : taille 2 → unique boule diamétrale
  q2 ; taille 3 (triangle strictement aigu) → unique miniboule **ambiante** q3
  centrée au circumcentre intrinsèque ; taille 4 (tétraèdre bien centré) →
  unique circumsphère q4. **Une seule boule par support complet**, jamais toutes
  les sphères incidentes. Fixtures : carré cosphérique
  `(0,0,0),(4,0,0),(2,2,0),(2,0,2)` (reste q2) ; témoin hors plan
  `z=(2,1,1)` de puissance `-11/3` (le census q3 est ambiant 3D).
- **Corner8 reçu borné** : verdict = « tout support de `A×B×C×D` × tout `z` de
  `Z` est intérieur », rien de plus (ni IDs distincts, ni positivité, ni owner,
  ni shell) → compteur `domain_mass_closed`, jamais `M4_closed`. Coût réel : ~
  **1508 multiplications d'extrémités i128 par bloc `ALL`** (pas « huit
  opérations »). Contre-support : `(10,10,10),(14,10,10),(11,11,10),(11,10,11)`
  a barycentriques `(2,1,-1,-1)` — Corner8 peut classer sans que le support soit
  positif.
- **WST3/WST4** : lemme de broad phase exact (si `ab` arête maximale, tout autre
  sommet est à distance `<=||b-a||` des deux bouts) → couverture reçue, mais
  **pas** « source physique exact-once » : représentations sous ancres non-owner
  (fixture triangle équilatéral en distances carrées 2), diagonales `x=a/x=b`,
  tie-break sur rang Morton et non `PointId`, positions dupliquées refusées
  contre le profil. Statut : « candidate-cover exact-once après projection owner
  sémantique » — projection non factorisée dans le producteur.
- **Chiffres** : uniform n=120 s=2 : 1368 rectangles, 19 390 blocs WST3, masse
  589 653, 88 602 visites, 280 840 triangles, 0 manquant/0 doublon ; WST4
  uniform n=60 : 487 635 quadruplets. La masse WST4 `Σ k_t(k_t+1)/2` atteint des
  **centaines de millions de records à n=8000** (~`n^4`). Rampe
  12500/25000/50000 (Corner8 OFF) : pentes **2,050 (visites)** et **2,193 (F4)** ;
  masse candidate ≈ `3·C(n,3)` en q3 et `6·C(n,4)` en q4 ; **le filtre d'acuité
  coarse conserve >99,998 % des blocs**.
- **Bugs stables depuis `a73161c`** : `--echelle=num/den` implémente **l'inverse**
  du ratio documenté (WST4 n=1000 : 6 159 060 à `1/1`, 35 494 à `1/64`,
  65 167 274 à `64/1` — sens inverse du contrat) ; sortie des comptes en `double`
  6 chiffres (pas un reçu exact). La rampe `1000..32000` réfute l'extrapolation
  de pente locale `1,73` ; les nœuds LBVH ne sont pas les cubes d'un niveau
  Morton fixé (l'argument de packing `O(eta^-3)` ne s'applique pas).
- **Commit `069d903` (--corner8)** : diagnostic placé **après** matérialisation ;
  sur la diagonale `C=D` le produit contient `x=y` donc l'orientation contient
  zéro → à échelle grossière `100 % orient_indecise` (uniform n=60 : 10 000
  couples soumis, 9704 indécis, 1 fermé, 35 117 visites). `sep_ok` ne teste que
  `C–D`, pas `A–C/A–D/B–C/B–D`. Commit `52585c5` : `Sym2` **compté, pas routé**
  (25,4M/103,8M/459,5M itérations à n=1000/2000/4000, pentes 2,029/2,146 —
  `3248x` les 141 468 couples plats) ; le caller bisigne perd le mauvais bit via
  un `continue` global (fixtures `O=-2/J=-4` aux coins mais centre `J=+2`).
- **Réponses Q10–Q13** : aucune séparation pairwise ne borne l'aplatissement
  (fixture ±1000 avec `R²=1000001`) ; forme de Gram **sign-free**
  `Phi(S,z)=Delta·||s||²-s^T·M·r=O·J`, convexe quel que soit le signe de `O` car
  coefficient `Delta=O²>0`, ~138 bits u16 ; positivité par numérateurs
  `r_1,r_2,r_3` et `2Delta-r_1-r_2-r_3` sur `2Delta>0` ; une seule `CKPairTape`
  coarse + raffinement local par lane (parent remplacé atomiquement par
  partition complète) ; diagonale traitée par
  `Sym2(C)=Sym2(L) ⊔ (L×R) ⊔ Sym2(R)`. Fixture 12.1 : facteurs lointains à
  orientations `-3481000000` et `+3481000000` — les distances ne fixent jamais
  l'orientation.
- **Analogue q3 de Corner8** : `N=V(U-F)u+U(V-F)v` ; intérieur ambiant ssi
  `Delta·||z-a||²-N·(z-a)<0`, convexe → 8 coins. Factorisation symbolique
  `Q4CandidateCellPairs=Sym2(A) ⊔ Cross(A,N)` (à ne jamais développer) +
  `LensDescriptor` paresseux + `FaceAxisJungDepth8Block` avant tout produit
  résiduel.

### 1.3 `AUDIT_CONTRE_SESSION_AXIS_TOP8_G4_840A2E2_20260814.md` (213 lignes)

- **Qui/quoi** : audit statique de la recette GCP `session_axis_top8_g4.sh`
  (puis du successeur `session_q4seed_axis_topr4_g4.sh`). Verdict : non
  recevable, ne pas relancer.
- **P0 sécurité** : le trap armé avant `start_and_verify.sh` peut appeler
  `stop_and_verify.sh --yes` **sans** `--expected-last-start-timestamp` → peut
  arrêter une génération qu'il n'a pas démarrée. L'addendum montre que ce chemin
  a été **réellement exécuté** (transcript du 14/08, 13:00:56 UTC-7) — la cible
  était heureusement déjà arrêtée par le garde interne.
- **P0 preuve négative** : le verdict distant sous `set -e` s'exécute **avant**
  les `scp` : une réfutation détruit précisément les fichiers qui l'expliquent
  (`rm -rf` suivant).
- **P0 faisabilité** : 76 runs séquentiels (30×n=120, 18×n=200, 12×n=300,
  16×n=200), timeout 3300 s chacun = 250 800 s ≈ 70 h, contre coupe-circuits
  55/75/90 min ; le probe est en puissance **cinq** (facteurs 32/412/3125 vs
  n=60). Pas de `--kill-after`, pas de deadline global monotone.
- **Successeur** : parser structurellement cassé (`len(juges)==len(codes)` faux
  par construction → verdict jamais `ACCORD`), champs périmés lus comme zéro,
  `RUN_TIMEOUT` non parsé injecté dans le shell distant, 23 tests annoncés vs 36
  au pin. La suite `39/39` reçoit le probe borné, **pas** une route 50k.
- Le verdict `manquants=0/census_faux=0` ne juge ni les listes de vrais
  `PointId`, ni le shell, ni le primary, ni la multiplicité des `SupportKey`.

### 1.4 `AUDIT_DEBLOCAGE_Q4_PORTEUR_AIGU_SOC64_LIVE_35FCEA8_20260814.md` (444 lignes)

- **Verdict** : le verrou q4 n'est plus le prédicat ponctuel mais **la génération
  de la relation sans développer les paires**. SOC64/CORNER512 sûrs comme
  certificats `ALL`.
- **Lanes SOC** : pour `e=z-a`, `t=b-z`, `H=e·t` : q2 `H>0` ; q3 `H>0 et
  4H²>EX` ; q4 `H>0 et 3H²>EX`. Spindle :
  `x²+r²+(L/√(c-1))·r < L²/4` (c=4 ou 3), convexe → **CORNER512 = 8×8×8 coins
  exacts** sur l'enveloppe continue. u16 : `H,E,X` sur 34 bits, `EX`,`4H²` <70
  bits, i128 suffit.
- **Double comptage du shadow SOC64 trouvé puis réparé** : un ancêtre
  `SOC64-ALL` puis un descendant `central-ALL` créditaient les mêmes IDs.
  Ledger corrigé (eight_clusters/500/r4) : 15 775 fermetures / 21 945 masse,
  contre 20 788 / 28 081 pour la somme brute (gardée comme mutant témoin).
- **Contre-famille u16 décisive (fixture permanente)** :
  `A_i=(i,0,0)`, `B_j=(0,j,H)`, `H=65535`, `m<=25000`. Chaque paire croisée a une
  sphère vide `c_ij=(i,j,(H²+i²-j²)/(2H))` dans le disque de Jung → **aucun
  certificat universel sound (LP, pelages, cages) ne peut fermer les
  `m²=n²/4` paires**. Pourtant `(A_k-A_i)·(B_j-A_i)=-(k-i)·i<0` : aucun triangle
  aigu, donc **source q3/q4 réellement vide**. Les certificats universels sont
  des fast paths, jamais la preuve de complétude.
- **Lemme du porteur aigu (avec preuve)** : tout tétraèdre non dégénéré à
  circumcentre strictement intérieur a, pour toute arête maximale `ab`, **au
  moins une face incidente `abx`/`aby` strictement aiguë** (via `n=a+b`,
  aigu en `z` ssi `n·z<||n||²/2`, et `O` hors du demi-espace). Ouvre la source
  `RectKey → AcuteCarrierBlock → sweep 1D` — lève le verrou de **complétude**,
  pas encore celui de **coût**.
- **Fenêtre `2B_R` sharp** : si `B_R` contient les endpoints owner, tout
  troisième sommet est dans `2B_R` (preuve par `||m||+√3·||h||<=2`, égalité
  atteinte). vs `3B_R` : plafond de cellules ÷3,375, couples ÷≈11,39.
- **Porte aiguë + sweep 1D** : admissible ssi `D>=E`, `D>=X`, `E+X>D` (+
  tie-break `EdgeKey`). Après `(a,b,x)` : `G=DE-F²`, `n=d×u`,
  `W=E(D-F)d+D(E-F)u` ; centres `c(tau)=a+(W+tau·n)/(2G)` ; puissance
  `P_z(tau)=G||s||²-W·s-tau·(n·s)` affine en tau ; événement
  `tau_y=(G||y-a||²-W·(y-a))/(n·(y-a))`. Comparaisons ~155 bits → limbs
  prouvés/BigInt au juge.
- **`OriginOnionDepth-h`** : inversion `p_z=(z-a)/||z-a||²`, pelage de coques
  convexes ; `p_b` intérieur relatif de `K_0..K_{h-1}` → toute sphère par `a,b`
  contient `h` IDs ; test entier de facette `v||d||²-u·d>=1` (~87 bits, i128) ;
  `h=8/9/10` ferme q4/q3/q2. Prune OR, jamais l'architecture source.
- **Chiffres de coût** : « chaîne complète » CPU 78,841 s sur uniform/50000
  (21 413 140 `SupportKey`) ; eight_clusters/r4/50000 : 525 902 961 arêtes q4
  sémantiques, 31 852 043 terminaux, 63 654 087 tests de front ; n=500 :
  191 538 784 couples de lentilles et 334 430 649 tests intérieurs pour
  32 280 sorties q4. Shadow n=2000 : 3 809 028 tâches, 18 871 452 prédicats,
  vague 2,83→4,94 s. Gate de pente exigée : `<=1,35` sur deux pas consécutifs à
  `1500/3000/6000`, sinon arrêt avant 50k. Correction de méthode : une fraction
  de fermeture plate ne change pas la pente
  (`p'=p+log2((1-f_2n)/(1-f_n))`) — le chiffre « 32,22 % » n'est pas une gate.

### 1.5 `AUDIT_LIVE_BLOCK_JUNG_CREDITS_TAU_783A789_20260814.md` (558 lignes)

- **Verdict** : `BlockJungDual64` mathématiquement sûr pour une base pondérée
  fixe (64 coins nécessaires et suffisants) ; le **premier raccord live des
  crédits était faux** (addition scalaire de crédits de groupes sans disjonction
  des `PointId` déjà crédités singletons). Réparé comme packing disjoint
  (3 CTests), non reçu (juge sous cap publie `OK`/code 0 avec
  `fermetures juges=7, sautes=10` ; options vacuaires acceptées).
- **Fixtures permanentes exigées** : `seven_collinear_plus_reused_pair`
  (`a=(0,0,0)`, `b=(10,0,0)`, `z_i=(i,0,0)` : marge `i(10-i)>0` → 7 universels,
  profondeur exactement 7 ; toute paire réutilisée ne crédite pas un 8e) ; et la
  fixture source u16 (`a,b,p,q` cosphériques `R²=372500`, owner `ab`, 6 témoins
  sur la corde : `I=6, U=4` — deux groupes disjoints couvrants n'ajoutent
  **aucune** identité, `tau=6`).
- **Réparation recommandée** : hypergraphe `F` de bases (1–3 IDs) prouvées
  uniformément ; **`tau(F)>=h` ferme sûrement** (`nu(F)<=tau(F)<=Depth` ;
  égalité `Depth=tau` seulement pour paire fixe avec toutes les bases de
  Helly). Solveur borné : branchement `<=3^7=2187` feuilles (q4), `3^8=6561`
  (q3), bitsets, aucune géométrie rejouée. Génération **par coupes** (HPI sur
  `P∖(U∪R)`), pas balayage des `C(24,2)=276..1172` paires.
- **Préfiltre `BJD-BilinearBounds`** : par axe `g_i(x,y)=-L·x·y+Z_i(x+y)` ;
  `Amin=Σ min(g_i)-Q` est le **vrai** minimum de `A0` ; bornes coordonnées de
  `C_i` par 4 coins ; `Amin>0 et 2Amin²>Nmax → ALL q4` (3 pour q3), sinon
  fallback 64 coins (reconstruits par différences premières/mixtes, parité
  bit-à-bit exigée) ou `MIXED`. Coût : 36 valeurs bilinéaires scalaires.
  Proposant de poids par intersection de 64 intervalles + **Stern–Brocot**
  (dénominateur `<=65535`).
- **NO-GO chiffré du BJD post-descente** : n=600 uniform : mêmes 6 892 939
  lectures, +263 349 essais, ~4,32M couples de coins, `E4` 119 521→90 033 mais
  **aucune descente évitée**. n=1500 apparié : uniform masse q4 ouverte
  269 817→235 959 (−12,55 %) avec exactement 32 387 961 lectures et médiane CPU
  3,344→3,527 s (+5,47 %) ; eight_clusters −0,87 % et +8,15 %. **« NO-GO comme
  hot path sous cette ordonnance »**.
- **Coûts de référence 50k (reçus)** : `rampe_raffinement_g4_20260813`
  (s=8, r0) : 9 182 111 terminaux q4 ouverts (uniform), 7 961 883
  (eight_clusters) — un seul appel BJD/terminal ≈ 596,8/517,5 M prédicats.
  `chaine_complete_g4_20260813` (s=3, n=50000) : arbre 3,5 ms mais vague
  **624 377 753 recertifications / 18,437 s** (uniform) et 430 666 842 /
  12,799 s (amas) ; ~43 % descentes pures, ~50 % `NONE`, ~7 % `ALL`. Fronts
  s=8 = **24,47× / 17,05×** les fronts s=2 → la baseline de coût reste proche
  de `s=2` avec raffinement local.
- **SLO** : `BenchmarkOutputContract-v1` = 16 étages critiques : 0 complet, 6
  incomplets, 10 absents ; `warm_e2e_mesurable=non` — statut exact :
  « chronométrage officiel encore inéligible ». Session G4 du 14/08 : échec en
  CTest, cible certifiée `TERMINATED`, aucun chiffre 50k.
- **Seuils d'ingénierie posés** : ≤16 bases/tuile (tier rapide), ≤64
  (exceptionnel) ; solveur ≤3280/9841 nœuds ; GO diagnostic si
  `T_saved/T_proof>=2` à 3000 et 6000 sur les deux familles.

### 1.6 `AUDIT_LIVE_BORNE_SUP_CREDITS_A58D020_20260814.md` (207 lignes)

- **Lemme reçu** : `upper[L,q]=cred[L,q]+Σ(pop(t))` sur l'antichaîne de tâches
  vivantes majore les crédits singleton atteignables ; `upper<need` prouve
  `OPEN_FOR_THIS_LEDGER` (jamais « aucun événement Morse »). Invariant :
  `pop(parent)=pop(gauche)+pop(droit)` ; `MIXED` remplace, ne retire pas.
- **Faute exacte du snapshot `90640885`** : au dépilement d'un parent `MIXED`,
  la population est soustraite **sans** réajouter celle des enfants → `reste=0`
  dès le premier `MIXED`, les 3 lanes déclarées mortes, sortie entièrement
  ouverte publiée avec `pending=0` et `fenetre_finale=OUI`. Chiffres
  (eight_clusters n=1500) : baseline 9 570 325 lectures, fermetures
  90 735/18 609/11 853 ; borne-sup 129 764 lectures, **0/0/0**, masses ouvertes
  toutes à 1 124 250, `lanes_mortes=389 292`. Le « gain » de temps était un
  abandon incorrect.
- **Réparation `ec5ec3d4`** : mêmes fates/masses que la baseline, lectures
  9 570 325→9 567 705, `visites_evitees=385` seulement. Restent bloquants :
  la mortalité calculée sur `cred/mask` éteint aussi `cmask` (vue combinée SOC
  peut avoir `ccred>cred`) ; **BJD crédite depuis une banque de feuilles déjà
  retirées de `reste`** → `cred+reste` ne majore pas le crédit collectif futur.
  Divergences exécutables : terrain n=16 fermetures combinées 1→0 ; terrain
  n=64 BJD8 q4 84→82 ; uniform n=200 BJD8 2510→2509.
- Majoration composée proposée pour BJD :
  `cred+reste+min(cap_groupes, floor(nfeuilles_libres/2))`.
- **Contre-audit croisé** : `central NONE => Midball ALL impossible` est **faux**
  (fixture `A=[0,8], B=[10,100], C={9}`) ; le bloc Release Midball contient 24
  multiplications (inlining), pas 72.

### 1.7 `AUDIT_LIVE_HC_BLOCK_DEPTH_A58D020_20260814.md` (157 lignes)

- **Théorème reçu (`HCBlockDepth`)** : `H=t·e`, `C=t×e`, identité de Lagrange
  `||t||²||e||²=H²+||C||²` ; lanes universelles : q2 `H>0` ; q3 `H>0 et
  3H²>||C||²` ; q4 `H>0 et 2H²>||C||²`. Bloc : `hmin` exact (somme de minima
  bilinéaires/concaves aux extrémités) ; `Σ M_i²` majore `||C||²` ;
  `hmin>0 et 2hmin²>ΣM_i² ⇒ ALL q4` (3 pour q3). Perte de corrélation → faux
  `MIXED` seulement, jamais faux `ALL`. u16 : `|C_i|<=2·65535²<2^33`, i128.
- **NO-GO d'intégration** : aucune CTest HC ; CTest Midball existante cassée
  (12/13, regex sur ancien message) ; jusqu'à **3 recalculs identiques par
  tâche** ; `--hc --midball` échoue sur un faux plancher (HC consomme les gains
  q2, Midball publie 0, plancher inconditionnel → code 3) ; ABI sans
  `UNKNOWN/preflight` (réutilise `kLaneNone`).
- **Coût réel** : ≥48 multiplications i64 + 4 i128 par appel sain (le
  commentaire « une trentaine » est faux au niveau source).
- **Signal de performance (n=1500 eight_clusters, mêmes rectangles)** : lectures
  9 570 325→8 383 723 (−12,40 %) ; résiduels q2 −29,95 %, q3 −14,31 %,
  q4 −9,22 % ; fermetures 90 735/18 609/11 853→93 193/30 720/22 425 ; **mais
  médiane CPU 0,825→1,210 s (+46,7 %)** sur machine partagée. Meilleur signal
  logique que Midball seul ; coût non reçu.
- HC est un **prune de support partiel** (témoin universel de toutes les boules
  admissibles d'une ancre), jamais la boule unique d'un événement complet.

### 1.8 `AUDIT_MINIBOULE_UNIQUE_RESIDUEL_SHALLOW_5809BD2_20260814.md` (797 lignes)

- **Rétractation d'en-tête** : le commit `694920a` titrait « fenêtre exacte en
  `n^1,09` » ; le contre-audit resserre : **q2 est exact par paire** (sous
  domaine régulier) ; **q3/q4 ne produisent qu'un majorant `U<h`**, pas la
  fenêtre des événements.
- **Lemme de miniboule unique (identité de variance)** :
  `Σ_i lambda_i·||p_i-y||² = r² + ||c-y||²` → la boule `(c,r)` est l'unique
  miniboule d'un support affinement indépendant à coefficients positifs.
  Famille de toutes les sphères incidentes : `c=o+w`, `rho²=R²+||w||²`, `w∈N`
  (dimension `4-|S|` : plan pour paire, droite pour triangle, point pour
  tétraèdre) ; puissance
  `Pow_z(w)=||z-o||²-R²-2⟨proj_N(z-o),w⟩`.
- **Cœur affine exact** : `⋂_{S⊂𝒮} int(𝒮) = aff(S) ∩ int B(o,R)` → certificat
  `UnboundedAffineCoreCount` (q2 : IDs du segment ouvert `]a,b[` ; q3 : IDs du
  plan dans le circumdisque ; q4 : toute la boule). **Sandwich exact
  `U_K <= D_K <= C`** : `C<h` court-circuite (le certificateur universel ne peut
  pas fermer) ; `U_K>=h` ferme ; `U_K<h<=C` = résiduel collectif pour
  `tau(F)`/sweep/split.
- **Non-hérédité (fixtures u16 gravées)** : (i) q2 profond (10 intérieurs) mais
  q3 (`abc3`, circumcentre `(100,100,100)`, `R²=2500`, poids `(25/64,25/64,7/32)`,
  10 témoins extérieurs) et q4 (poids `(1/4,…)`) **subsistent** ; (ii)
  q3 fermé (rang 12) mais le tétraèdre `abxy` bien centré garde rang 4 —
  interdit de supprimer une face carrier après fermeture de son **événement** q3.
- **Forme finie exacte** dans le plan médiateur : `F_z(w)=D-||U_z||²+2U_z·w`
  avec `F_z(2t)=-4·Pow_z(t)` (fixture de signe obligatoire) ; q2 `w=0` ; q3 pied
  de norme minimale ; q4 intersection de deux lignes. À `K_max=10` : un q3
  retenu a ≤8 intérieurs, un q4 retenu ≤7 → **niveaux shallow `0..7` seulement**.
- **`MidballBlockDepth`** : le minimum de `H(a,b,z)=(z-a)·(b-z)` sur trois AABB
  est exact (8 triplets de bornes par axe, `|·|<3·65535²<2^34`, i64) ; le `NONE`
  n'est exact que sur le **réseau entier u16** (fixture `A={0},B={1},C=[0,1]` :
  max réseau 0, max continu 1/4) — contrat du header faux.
- **Réfutation de l'inclusion « Delaunay d'ordre 11 »** : fixture permanente à
  12 triples `p_x,q_x,r_x` (`x=420..431`, normales `(260,0),(-130,220),(-130,-220)`
  de somme nulle) : profondeur **toujours ≥12** (paire hors Delaunay-11) mais
  `u_q4=0` → le sampler classe l'ancre ouverte. Tue
  `universal-core-window-subset-delaunay11`. **La finitude de la source n'est
  jamais une preuve de sparsité.**
- **Réponse Q4 (contrat)** : « Ni la cible principale `100 ms`, ni la cible
  secondaire `1 s` à `50000` ne change » ; extrapolation « 100 ns/record » non
  recevable ; ~5,3 M records projetés exigent fermeture/fusion ou kernel
  massivement parallèle.
- **Interdits de coût** : un scan canonique par paire est interdit : front
  `s=2` ≈ 1 392 028 blocs (uniform) × 50 000 témoins > **69 milliards de
  tests** ; tout doit rester dual-tree/blockwise. La source shallow est
  **pair-level** : aucun arrangement shallow commun à un rectangle CK n'est
  prouvé — sans classifieur paramétré, la couture développe les `PairId`.
- Mesures : exhaustif n=200 : 19 900 paires, fenêtres `U<h` =
  3790/10059/10937, 3 184 359 tests ; sampler `--fenetre-exacte` n=200 :
  fractions ouvertes 0,198 (q2) / 0,520 (q3) / 0,559 (q4) ; Hoeffding non reçu
  comme IC (flux SplitMix déterministe sans modèle).

### 1.9 `AUDIT_RECU_GRAM_UNIFIE_1FD9CF1_20260814.md` (153 lignes)

- **Reçu borné positif** : vérifie exactement deux identités du microkernel Gram
  q4 : **`Delta=det(M^T·M)=det(M)²=O²`** et **`Phi=Delta·(||s||²-s^T·M^{-T}·ell)
  =O·J`** (noyau de preuve de `PROPOSITION.md` §4.3).
- Script Python autonome inclus (seed `20260814`, coordonnées `[0,20]`) :
  `PASS accepted=10000 draws=10029`,
  `fixture_sha256=e0a7f07c…d61905f4b1`.
- Portée strictement limitée : les deux calculs partagent `det3` → **contrôle
  algébrique corrélé**, pas un différentiel d'implémentation ; ne reçoit ni
  enclosure de boîtes, ni positivité, ni source WST, ni census, ni C++, ni
  performance ; n'exerce que q4 (le tirage ne garantit ni `Phi<0/=0` ni les
  deux signes de `O`).

---

## 2. Synthèse transverse

### 2.a Résultats mathématiques établis (à recopier tels quels en V4)

1. **Miniboule unique / support minimal positif complet** :
   `Σ lambda_i||p_i-y||² = r²+||c-y||²` ; support de taille 2/3/4 → une seule
   boule canonique (diamétrale / circumdisque ambiant / circumsphère). Sphères
   incidentes : `c=o+w, rho²=R²+||w||², w∈N`, dimension `4-|S|`. Cœur affine :
   `⋂ int(𝒮) = aff(S) ∩ int B(o,R)`. Sandwich `U_K <= D_K <= C`.
2. **Lemme du porteur aigu** (prouvé) : tétraèdre bien centré + arête maximale
   `ab` ⇒ au moins une face incidente strictement aiguë. Acuité du troisième
   angle : `K=(a-x)·(b-x)>0 ⇔ E+X-D>0` ; avec `H=(x-a)·(b-x)`, `E+X-D=-2H`
   (aigu ssi `H<0`). Porte carrier : `D>=E`, `D>=X`, `E+X>D`.
3. **Fenêtre `2B_R` sharp** pour les carriers (facteur 2 optimal, gain ÷3,375 en
   cellules, ÷11,39 en couples vs `3B_R`).
4. **Sweep 1D d'axe de face** : `c(tau)=a+(W+tau·n)/(2G)`,
   `P_z(tau)=G||s||²-W·s-tau(n·s)`, événement `tau_y` rationnel ; segment q4
   `J_f={tau : 2tau²<=T2}`, `T2=D(G-2(E-F)²)` (q3 : 3) ; **noyau exact de 16 IDs
   (q4) / 18 (q3)** décidant `Depth>=r` par scan top-k `O(n)`.
5. **Prédicat in-sphere sans signe** : `Phi(S,z)=Delta||s||²-s^T·M·r=O·J`,
   `Delta=O²` (vérifié sur 10 000 tirages) — convexe en `z` quel que soit le
   signe de `O` → **8 coins d'une AABB témoin complets pour `ALL_INTERIOR`**
   (jamais `NONE`). Positivité par numérateurs `r_1,r_2,r_3, 2Delta-Σr_i` sur
   `2Delta`. Variante bisigne : `O>0`/`J` convexe, `O<0`/`J` concave, deux
   ledgers jamais additionnés.
6. **Certificats de bloc universels exacts sur enveloppe AABB continue** :
   Midball q2 (min de `H` aux 8 triplets de bornes par axe) ; HC (Lagrange :
   q2 `H>0`, q3 `3H²>||C||²`, q4 `2H²>||C||²`) ; SOC64/CORNER512 (cône convexe,
   64/512 coins) ; `BlockJungDual64` (`A0,C0`, 64 coins nécessaires et
   suffisants à base+poids fixes). Jung ponctuel sans fractions :
   `A=WD-Σw_z||a+b-2z||²`, `R=D||P||²-(P·d)²`, q3 `3A²>4R`, q4 `A²>2R`.
7. **Profondeur = transversal** : `nu(F)<=tau(F)<=Depth` (toujours) ;
   `Depth=tau(E)` pour paire fixe avec toutes les bases de Helly ; solveur borné
   `3^{h-1}` ; noyau tolérant `eta(3,h)<(h+1)²` ⇒ **80 IDs (q4) / 99 (q3)** ;
   fixture profondeur 8 / packing 7 (le packing n'est pas complet) ;
   `OriginOnionDepth-h` par inversion (h=8/9/10).
8. **Comptage M4 exact sans matérialisation** : identité `M4_e` par arête +
   checksum `Q_aff`, Möbius sur 15 partitions, intervalle `[M4_L,M4_U]` conservé
   aux splits ; borne shallow `< e(k+1)m` centres.
9. **Contre-familles (théorèmes négatifs)** : deux droites u16 → `n²/4` paires
   universellement infermables mais source q3/q4 **vide** (les certificats
   universels ne sont pas une preuve de complétude) ; 12-triples → `u=0` mais
   `d>=12` (la fenêtre `U<h` n'est pas incluse dans Delaunay d'ordre 11) ;
   non-hérédité q2→q3/q4 et q3→q4 ; aucune séparation pairwise ne fixe
   l'orientation ni ne borne l'aplatissement.
10. **Budgets de bits u16 (i128 sauf mention)** : SOC `H,E,X` 34 bits ;
    Midball 2^34 ; HC `|C_i|<2^33` ; onion 87 bits ; BJD64 `2^50/2^49` ;
    Corner8 `|O|<2^51`, `|J|<2^87`, BallForm `<2^90` ; Phi ~138 bits ;
    positivité apex ~174 bits ; sweep tau ~155 bits, bouts ~207 bits (i256) ;
    juge dual réduit ~180 bits ; `6·C(n,4)<2^61` et `n^4<2^63` pour `n<=50000`
    (uint64 device après preflight).

### 2.b Mesures chiffrées et décisions d'architecture

- **Matériel** : diagnostics sur CPU partagé (48 cœurs) ; cible GPU = G4/
  Blackwell SPOT (`ehgp-blackwell-spot-ai1a`, europe-west4-ai1a). **Aucune
  mesure GPU valide n'existe** : sessions G4 du 14/08 échouées (CTest rouge,
  recette non recevable), `TERMINATED` certifié. SLO officiellement
  **inéligible** (16 étages : 0 complet/6 incomplets/10 absents).
- **Le mur est quartique et mesuré** : masse candidate WST4 ≈ `6·C(n,4)`
  (pentes 2,05–2,19 sur 12500→50000) ; filtre d'acuité coarse conserve
  >99,998 % des blocs ; à n=8000 déjà des centaines de millions de records.
  Chaîne CPU 50k : 78,8 s ; vague s=3 : 624 M recertifications / 18,4 s
  (43 % descentes pures, 50 % NONE, 7 % ALL) ; 9,18 M terminaux q4 ouverts à
  s=8.
- **Sélectivités exhaustives (petit n)** : `M4/C(n,4)` ≈ 0,64–0,66 ;
  `W4/M4` ≈ 0,12–0,15 ; tests `I<=7` par point ≈ 27–40 ; fenêtres d'ancres
  ouvertes à n=200 : 0,198/0,520/0,559 (q2/q3/q4).
- **Certificats bon marché : gains logiques réels, coût CPU négatif dans
  l'ordonnance actuelle** : HC : lectures −12,4 %, résiduels −30/−14/−9 %, temps
  +46,7 % ; BJD post-descente : masse −12,55 % mais temps +5,5/+8,2 %, zéro
  lecture évitée (NO-GO hot path) ; shadow SOC64 : +74,6 % de temps de vague.
  **Décision transverse : tout certificat doit agir AVANT la descente** (vague
  jointe héritant les preuves), sinon c'est une ablation de couverture.
- **Échelle de séparation** : fronts s=8 = 24,47×/17,05× les fronts s=2 →
  décision : **une seule `CKPairTape` coarse (proche s=2)** + raffinement local
  par lane avec remplacement atomique parent→partition d'enfants ; jamais deux
  WSPD indépendantes par lane.
- **Gates de rampe normalisées** : tailles `1500/3000/6000`, deux pentes
  physiques consécutives `<=1,35` (compteurs physiques : lectures,
  recertifications, wide ops, octets, HWM — pas la seule masse `E4`) ;
  `T_saved/T_proof>=2` ; caps 16/64 bases par tuile, solveur ≤3280/9841 nœuds.
  Contrat rappelé : **100 ms principal / 1 s secondaire à 50 000** (Q4 du §8.3).
- **Comptage avant remplissage** : `count → preflight → fill` avec
  `DecisionTape` (lane/vue, 4 `NodeKey`, masse, décision, offsets, digest) ; le
  fill ne recalcule aucun prédicat ; conservation de masse et `pending=0` comme
  portes.

### 2.c Bugs, erreurs, rétractations documentés

1. **Signe d'acuité inversé** (`3703097`) : `E+X-D=2H` au lieu de `-2H` — le
   filtre éliminait le côté aigu ; gain `1,62x` et toutes les mesures invalidés.
2. **Borne-sup fausse** (`90640885`) : population des enfants `MIXED` non
   réinsérée → 0 fermeture partout publié avec `pending=0` /
   `fenetre_finale=OUI` (fausse finalité). Réparée (`ec5ec3d4`) mais interactions
   vue combinée/BJD toujours fausses (fermetures 84→82, 1→0…).
3. **Double comptage de crédits** : (i) shadow SOC64 (ancêtre `SOC-ALL` +
   descendant `central-ALL` créditent les mêmes IDs) ; (ii) premier raccord BJD
   (groupes réutilisant des singletons déjà crédités → 8 annoncé pour
   profondeur 7). Réparés en packing disjoint.
4. **Caller bisigne** (`52585c5`) : `continue` global après crédit du mauvais
   signe → faux négatifs structurels (fixture `O=-2`, coins `J=-4`, centre
   `J=+2`).
5. **`--none-descend`/vue combinée éteinte** : le prune `central-NONE` cachait
   14 383 témoins exacts et 11 848 témoins SOC (n=200) ; le delta descend mais
   ne peut pas créditer SOC (masques intersectés avec le central).
6. **`--echelle=num/den` inversé** (contrat vs code), non mordu par les portes ;
   comptes en `double` 6 chiffres pour des reçus « exacts ».
7. **Sampler v2** : barre `±2σ` de largeur nulle autour d'une valeur fausse ;
   claim « exhaustif » faux ; compteur `doublons` faux (consécutifs seulement) ;
   multiply-high non uniforme exact.
8. **Claims retirés/resserrés** : « `M4=Θ(n^4)` pour tout nuage » (faux,
   `two_lines`) ; « fenêtre exacte `n^1,09` » (`694920a`) → majorant `U<h`
   d'ancres, inclusion Delaunay-11 invalide ; « gain primal rigoureusement
   nul » (réfuté 169→170) ; « SOC64 à 96 % de son plafond » (valable sur une
   cohorte conditionnée seulement) ; profondeur « deux ordres de grandeur »
   (mesuré 3,2×–6,6×) ; « une trentaine de multiplications » HC (≥48+4).
9. **Contrats d'API** : `kLaneNone` réutilisé pour « ne conclut pas » (confusion
   avec un `NONE` géométrique) ; `dual_lane` sans cap `Σw<=65535` ; mutants à
   overflow signé (UB) ; `Midball NONE` exact seulement sur le réseau (header
   faux) ; commentaire minimax inversé dans `jung_dual.hpp` ; littéral `d=8`
   imprimé au lieu d'être calculé ; portes `PASS_REGULAR_EXPRESSION` masquant
   des codes non nuls ; juges sous cap publiant `OK`/code 0.
10. **Recettes GCP** : trap pouvant arrêter une génération non démarrée (chemin
    **exécuté**) ; verdict avant rapatriement (une réfutation détruit sa
    preuve) ; budgets impossibles (250 800 s vs 55 min) ; parser ne pouvant
    jamais rendre `ACCORD` ; `RUN_TIMEOUT` injecté en shell distant.
11. `MHGP_HD` autour de `__int128` : **aucun chemin CUDA reçu**.

### 2.d Pistes fermées dans cette tranche, et pourquoi

- **Matérialiser la masse quartique** (produit brut WST4, PairId développés,
  tableau par paire, RLE projectif global — 10 Go/1 Po à 50k) : la masse
  pré-rang peut être réellement quartique (famille ouverte explicite) ; elle ne
  doit **jamais être remplie**.
- **Rampe/sampler M4 à 50 000** avant réception des compteurs physiques :
  inutile et interdite.
- **Certificats universels (LP, pelages, cages, singletons) comme preuve de
  complétude** : contre-famille deux-droites (`n²/4` paires infermables, source
  vide). Ce sont des OR de prune.
- **Packing de 8 groupes disjoints comme décision** : fixture profondeur 8 /
  packing 7. Route : `tau(F)`.
- **BJD/greedy égal-poids après la descente** : NO-GO chiffré (aucune lecture
  évitée, temps en hausse).
- **Cascade de rang q2→q3/q4 et hérédité q3→q4** : fixtures de non-hérédité.
- **Inclusion « fenêtre `U<h` ⊂ Delaunay d'ordre 11 »** : réfutée (fixture
  12-triples).
- **Séparations pairwise comme précondition d'orientation / d'aplatissement** :
  réfutées (fixtures ±3481000000 et tétraèdre plat `R²=1000001`).
- **Poids Jung différents par coin, transférés au rectangle** : détruit la
  quantification (poids communs à la tuile obligatoires).
- **Scan de tous les sites par face / scan canonique par paire** : >69 G tests
  à 50k ; tout reste dual-tree.
- **LBVH à arrêt grossier comme preuve de packing** : pas de taille minimale de
  maille ; seule une vraie grille Morton à niveau fixé reçoit `O(eta^-3)`.
- **Owner circulaire par `BallKey`** : l'owner arête-maximale + ordre physique
  est confirmé deux fois.
- **Recettes GCP en l'état** : trois P0 chacune.

### 2.e Ce que la V4 doit conserver / éviter absolument

**Conserver (directement transposable à la roadmap V4)** :

- **Ontologie des supports** : q2/q3/q4 = supports minimaux positifs complets,
  une seule boule canonique chacun ; le troisième témoin q3 doit former un
  **triangle strictement aigu**, le q4 est porté par une face aiguë incidente à
  l'arête maximale (lemme du porteur aigu) — c'est exactement la structure
  `q2/q3 (+x aigu)/q4 (+x,y)` de la roadmap V4.
- **Owner = arête maximale + tie-break `EdgeKey` total**, fenêtre carrier
  **`2B_R`** (sharp), porte aiguë séparable par axe (`K_min>0 → ALL_ACUTE`,
  `K_max<=0 → NONE_ACUTE`), factorisation `Sym2(A) ⊔ Cross(A,N)` non développée.
- **Élimination par témoins dans la « zone cœur »** : la V4 doit distinguer
  trois strates reçues : (i) témoins **universels** (HC/SOC/Midball/cœur
  affine, sandwich `U_K<=D_K<=C`, `C<h` en early-exit négatif) — GPU-friendly,
  quelques dizaines de multiplications i64/i128, verdicts `ALL` exacts aux
  coins ; (ii) témoins **collectifs** (`tau(F)>=h`, bases de Helly ≤3 IDs,
  BJD64 + BilinearBounds + Stern–Brocot, solveur bitset ≤3280 nœuds, noyau
  ≤80/99 IDs) ; (iii) le **résiduel fini** (sweep 1D par face, noyau 16/18 IDs,
  niveaux shallow 0..7 edge-local). L'élimination `h_a/h_b` en dual-tree de la
  roadmap correspond à (i)–(ii) et doit exiger des **ledgers de vrais `PointId`
  disjoints** (jamais une addition scalaire de crédits).
- **Prédicats exacts u16 avec budgets de bits gravés** (table §2.a.10) ; jamais
  `O*J` formé ; `Phi` sign-free convexe comme prédicat par défaut ; égalité =
  shell partout.
- **Comptabilité** : `count→preflight→fill`, Möbius injectif, intervalle
  `[M4_L,M4_U]`, conservation de masse aux splits atomiques
  (`Sym2(C)=Sym2(L)⊔(L×R)⊔Sym2(R)` pour la diagonale), exact-once par
  partition, `pending/PARTIAL/UNKNOWN` fail-closed.
- **Discipline de test** : fixtures permanentes (two_lines/deux droites,
  12-triples, seven_collinear, profondeur-8/packing-7, non-hérédité, fixture
  Corner8 4096, tétraèdre régulier d'acuité, fixture de signe `F_z=-4Pow`) ;
  mutants à code de sortie exact (jamais regex seule, jamais UB) ; planchers de
  non-vacuité ; permutation des `PointId` à géométrie constante ; juges à
  arithmétique indépendante (pas de `det3` partagé) ; gates de pente `<=1,35`
  aux tailles contractuelles.
- **Ordonnance** : certificats **avant** la descente/le produit ; une seule
  décomposition coarse + raffinement local ; le fallback shallow seulement sur
  le vrai résiduel.

**Éviter absolument** :

- Créer, même transitoirement, un objet ∝ `C(n,4)` (ou `Σk_t²` développé) ; le
  compteur `Sym2` lui-même était un mur `O(n²)`.
- Placer un certificat après la descente et compter son « gain » sur la masse
  sémantique (`E4`) au lieu des compteurs physiques.
- Confondre : masse candidate ≠ `M4_apex` ≠ supports positifs ≠ événements ;
  `domain_mass_closed` ≠ `M4_closed` ; fermeture d'un événement ≠ suppression du
  carrier ; `U<h` ≠ profondeur ; échec de packing ≠ profondeur insuffisante ;
  base invalide ≠ `NONE`.
- Toute inférence statistique sans loi contractée (± σ, Hoeffding sur SplitMix
  déterministe) et toute extrapolation de pente depuis <3 tailles.
- Les benchmarks sur machine partagée comme critère ; les timings G4 sans
  `BenchmarkOutputContract` complet ; les recettes GCP sans fail-closed
  (rapatriement avant verdict, deadline monotone, arrêt versionné par
  génération).
- `double` pour un compte exact ; saturation avant soustraction ; `i128`
  implicite comme autorité au-delà de ~120 bits (sweep : i256).

---

## Questions ouvertes / ambiguïtés

1. **Écart de contrat** : ces audits fixent le contrat à « 100 ms principal /
   1 s secondaire à `n=50000` » (§8.3 Q4 de l'audit miniboule), tandis que la
   feuille de route V4 parle de « forêt complète K=10 en <100 ms (K=5/<1 s) …
   nuages jusqu'à des dizaines de millions de points ». Le lien entre `K=10`
   (V4) et `smax=11/K_max=10` (v3) semble être le même paramètre de rang, mais
   l'échelle de `n` visée (50 000 vs dizaines de millions) n'est pas conciliée
   dans cette tranche.
2. **`H4=Theta(n)` non prouvé** : trois tailles exhaustives suggèrent
   `H4≈30n`, mais aucune borne (même la Delaunay ordinaire peut être
   quadratique) ; la restriction well-centered pourrait aider — non démontré.
3. **Sparsité des faces aiguës / touches site-face** : le lemme du porteur aigu
   règle la complétude, pas le coût ; aucun théorème ne borne le nombre de
   faces aiguës ni la constante de blocs par rectangle (~32 observée, jamais
   prouvée).
4. **Uniformisation pair-level → rectangle** : le plan médiateur, les pieds q3
   et les intersections q4 varient avec `(a,b)` ; aucun arrangement shallow
   commun à un rectangle CK n'est prouvé. C'est le point le plus risqué de la
   couture « dual-tree, jamais quadratique A×B » de la V4.
5. **Exactitude de `eta(3,h)`** : la borne `<(h+1)²` (80/99 IDs) est reçue ;
   ses valeurs exactes ne sont pas revendiquées ; le coût de découverte du
   noyau reste non mesuré (`O(k²)` faces, `O(k³)` tests annoncés « encore à
   mesurer »).
6. **Chemin GPU** : `__int128` (`MHGP_HD`) n'a aucun chemin CUDA reçu ; le coût
   Corner8 (~1508 multiplications d'extrémités i128 par bloc `ALL`) n'a jamais
   été profilé côté device ; aucune mesure G4 valide n'existe dans cette
   tranche.
7. **Identités des auditeurs** : plusieurs audits notent explicitement qu'aucun
   artefact ne prouve une identité organisationnelle indépendante des
   « flux » ; toutes les réceptions sont par contenu/pin/fixture, jamais par
   autorité.
8. **Correspondance de vocabulaire V4** : la « zone cœur » de la roadmap V4
   n'est pas définie dans ces audits ; les candidats reçus sont le **cœur
   affine** (exact, souvent vide), le **cœur conique** SOC/HC (universel,
   incomplet) et le **disque/segment de Jung** (domaine des centres). Le choix
   et la composition exacts restent à spécifier en V4.
9. **`s` de la WSPD** : la roadmap V4 annonce `s=6/8/10` ; les audits montrent
   que les fronts `s=8` coûtent 17–24× ceux de `s=2` et recommandent une
   partition coarse + raffinement local par prédicat (Q12). La valeur de `s`
   optimale pour la V4 est donc ouverte, et l'usage de `s` doit être limité à
   la partition des paires, pas à la précision des certificats.
