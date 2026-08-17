# Lecture des audits v3 du 15 août 2026 (tranche A) — matière pour la conception V4

Périmètre : 17 fichiers de `morsehgp3D_v3/audits/` datés du 15/08/2026. Tous sont écrits par **l'auditeur indépendant** (parfois deux auditeurs qui se contre-auditent) et répondent à des commits/notes de Claude. Cadre commun : `phase=exploration_v3_hors_registre`, `profile=quantized_u16_input_only`, `public_status=not_claimed`. Vocabulaire : q2/q3/q4 = arités des bases minimales positives (paire diamétrale / triangle strictement aigu / tétraèdre bien centré) ; `h_q = s_max - q + 1` (à `s_max=11` : `h_2=10, h_3=9, h_4=8`) ; `s_max = K_max + 1` (cible `K=10` → `s_max=11`).

---

## 1. Fichier par fichier

### 1.1 AUDIT_CONSTRUCTIF_AXIS_DEVICE_2C14313_20260815.md
Auditeur ; juge le raccord q4 « axial » (`Q4SeedAxisTopR4`, commit `2d8aa5f`) et la qualification host/device (`2c14313`), plus le reçu CUDA récupéré (`11130cb`).

- **Pivot q4 reçu** : `select_axis_topr4` remplace la boucle `for carrier, for apex` (produit de lentille) par un top-`k` : un root est retenu s'il a strictement moins de `k` sites strictement meilleurs, ties complets ; coût `O(m*k)` au lieu de `O(m^2)` par seed. À `n=6000, smax=6` : mêmes `89 796` q4 des deux côtés, mais propositions `48 791 131` paires → `830 044` roots (**facteur ≈ 59**). `10 000` configurations rationnelles sans écart pour `r4>=2`.
- **Le mur se déplace, il ne disparaît pas** : le temps reste ~25–28 s à `n=6000` car le nouveau terme physique est `sum_seed |B(m_e,D_e) ∩ P| + rescans de census` (200–500 sites de `inner` × `2 956 531` seeds). La primitive suivante est une **descente hiérarchique**, pas une micro-optimisation des déterminants.
- **Reçu CUDA** (RTX PRO 6000 Blackwell, driver 580.173.02, CUDA 12.9) : 12 cas, parité `ecarts=0` sur `18 617 211` verdicts de seeds et `5 789 713 735` incidences site-seed ; débit kernel `4 999,97`–`13 979,84 Msites/s` (`11,99`–`87,80 ms`) ; accélération `179,4x`–`292,1x` **contre un seul thread host** (le « 4–6x vs 48 cœurs » est une division par 48 sous scaling idéal, pas une mesure). Seuls **3 lots complets** (`cap=0`) : `uniform,smax=6, n=1500/3000/6000` (`4 969 941` seeds, `489 483 354` incidences, kernels `11,99/26,87/55,33 ms`) ; régression diagnostique → ~`0,48 s` kernel-only à `n=50000`. Les 9 autres lots (amas, `smax=11`) sont des préfixes `cap=1`. Verdict exact : `AXIS_FLAT_PREFIX_PARITY`, jamais « parité exacte de la source ». Le `RESULTATS.md` de Claude recopie mal le cardinal (14 787 889 au lieu de 18 617 211) et son estimation `K=10 ≈ 750 ms` n'est pas une borne.
- **Le CSR plat doit disparaître** : projection `K=5` → `2,45 G` incidences > `INT_MAX` (offset `int` !), ~`9,8 Go` d'IDs, ~`5,42 Go` de `SeedOut`. Solution : produire les seeds **par tuile sur device**, descente `Q_theta`, compacter les survivants, libérer la tuile.
- **Primitive J2 proposée (`RationalBallRange`)** : pour un cutoff rationnel `theta=p/q`, poser `Q_theta(z)=q*A_z - p*B_z` (quadratique convexe séparable). Un même moteur donne First (`B_z>0`), Last (`B_z<0`) et Census (`Q_theta<0/=0/>0` = intérieur/shell/extérieur). Sur AABB : min par clamp axe-par-axe (`min>0` → `OUT`), max aux 8 coins (`max<0` → `ALL`), sinon descente. **Largeur : ~278 bits sous u16 → `BigInt<5>` (320 bits) requis ; `BigInt<4>` insuffisant** pour ce prune. `O(k log n)` n'est pas une borne reçue — mesurer `node_visits` et `sites_lus`.
- **Gains immédiats avant BVH** : (i) supprimer le rescan de census via `census_replay(sel,...)` avec fates routés (`EXACT`/`UNSUPPORTED_DEGENERACY`/`PENDING_CAP`/`HORS_DOMAINE`) ; (ii) factoriser le CSR par arête (`Lane4EdgeBatch` : `S_ab = {z != a,b : ||2z-a-b||^2 <= 4*D2}`, multiplicité seeds/arête ≈ **10,90/11,26/11,46** à `n=1500/3000/6000`) ; (iii) passer la baseline de 5 scans à 2 (÷2,5 les appels `site_power/classify`).
- **Grille CSR de sondes validée** : `937 739` seeds, `51 327 628` incidences identiques au scan naïf sur 64 configurations ; preuve : `offd2` minore la distance cellule-boîte et `floor(sqrt(D2))+2` absorbe l'écart au demi-milieu avant le filtre exact `nu<=4D2`. Builder de qualification : `200 000` seeds / `19,3 M` sites en `40 s` à `n=3000`.
- **Portes device manquantes** : `SeedOut` tronqué à 24 IDs/côté (pas `n_perm`, ni shell) ; H2D/D2H/allocations non publiés ; `DEBORDEMENT` agrégé aux morts (interdit — il doit déclencher continuation/fallback/refus) ; `r4=65` déborde `seuil[64]` sous ASan ; `code=\0` dans les lignes du reçu. Cible `-DCMAKE_CUDA_ARCHITECTURES=120-real` + `ptxas -v` demandés.

### 1.2 AUDIT_CONTRE_RECEPTION_PORTEUR_AIGU_207B542_20260815.md
Auditeur ; contre-réception des pins `2ce76e0` (porteur aigu + descente par ancre), `c8e3de7` (exact-once scission), `207b542` (frontières, `D=0`).

- **Reçu** : le lemme ponctuel « `x` porteur aigu ⇔ `x` dans la lentille et `H<0` » sous hypothèse `ab` arête maximale, avec `D=||a-b||^2, E=||a-x||^2, X=||b-x||^2, H=(x-a)·(b-x)` et l'identité `E+X-D = -2H`. **Trichotomie de la lentille** : `H>0` témoin q2 strict ; `H=0` shell (angle droit, ni témoin ni carrier) ; `H<0` carrier aigu. La phrase « témoin q2 = exactement non-porteur » est fausse sur `H=0`.
- **Reçu** : exact-once de la scission par `PairId` (9 portes, `oracle_couverture_ko=0`, `oracle_faux_morts=0`, `oracle_ids_doubles=0`) ; strictes de `pair_lane` sur 10 cas entiers.
- **P0 n°1 (sémantique)** : le champ imprimé `V4_pair_walive` est en réalité `S4_prefilter_survivor` (survivantes du préfiltre), pas le vrai `V4` décidé exactement — `two_lines` masque le défaut car son mou vaut 1 (`S4=V4=43128`).
- **P0 n°2 (owner)** : `est_seed` vérifie seulement `E<=D, X<=D, H<0` (arête maximale **au sens large**), sans tie-break `EdgeKey` — un triangle équilatéral entier `a=(0,0,0), b=(1,1,0), x=(1,0,1)` (trois longueurs² = 2) est compté trois fois. Correctif : `EdgeKey(u,v)=(min(id),max(id))`, ordre « longueur maximale puis EdgeKey minimale », avec vrais `PointId` (jamais les rangs Morton). Tétraèdre régulier : 12 incidences faibles vs `owner_edge carriers = 2` et `centre q4 exact-once = 1`.
- **La descente par ancre n'est pas logarithmique** : ~30 visites/ancre sur `two_lines,n=400` est une mesure, pas une complexité ; contre-régime : points de part et d'autre de la sphère diamétrale → `Theta(n)` par ancre. Facteur `13,29` (two_lines) robuste, `1,20` (eight_clusters) fragile.
- **Classifieur `NONE/ALL_STRICT_OWNER/MIXED` à paire fixée** proposé : `NONE` si `dmin(a,X)^2>D` ou `dmin(b,X)^2>D` ou `max||2x-M||^2<=D` ; `ALL_STRICT_OWNER` si `dmax(a,X)^2<D`, `dmax(b,X)^2<D`, `min||2x-M||^2>D` (aucun tie possible, crédit en `O(1)` ou émission `AcuteCarrierBlock`).
- L'ablation `A×B` fixe (gain 1,005) **ne réfute pas** le gateway triple adaptatif `A×B×C` (qui utilise aussi les clauses owner et subdivise le facteur le plus incertain).
- La fixture `D=0` de `207b542` est ponctuelle : elle ne traverse pas le pipeline (pas de nuage à `PointId` dupliqués, pas de `paires_D0`/`univers_ancres`).

### 1.3 AUDIT_CONTRE_SESSION_J0_LANE_SOURCE_G4_20260815.md
Auditeur ; contre-audit de la session J0 `lane_source` sur la VM G4 utilisée **uniquement comme CPU 48 cœurs**. Rampe scientifique **rouge**.

- Compteurs `uniform` (tronqués, non certifiés) — total candidats/point presque linéaire : `smax=6` : 76,27 / 78,27 / 80,10 à `n=12500/25000/50000` (temps 1/2/4 s) ; `smax=11` : 401,5 / 415,2 / 428,7 (8/19/38 s). Exposants ≈ `1,03–1,04`. La baisse `smax=11→6` vaut un facteur **5,35**, pas le facteur 12 supposé du plan.
- **Le verrou** : à `uniform,n=50000,smax=11` : `75 780 216` ancres testées, `171 956 174` seeds aigus, **`6 091 112 797` paires de lentille**, `1 100 846 370` q4 « bien centrés », ratio `paires_lentille/q4 = 623,5` (`595,8` à smax=6). Le produit de lentille paie des centaines de complétions par candidat — pas une route industrielle.
- **Familles difficiles : échec immédiat** (code 3, coupure) : `terrain 12500` : `861 M` (smax=6) et `2 522 M` (smax=11) paires de lentille, rapport cutoff `0,940` ; `eight_clusters 12500` : `6 267 M` / `24 136 M`, rapports `0,864/0,926`, 35 s/147 s. Aucun 25k/50k sur ces familles.
- L'écart `0,09 %` avec les `21 413 140` clés historiques est une **corroboration de cardinalité**, pas une identité de populations (pas de comparaison `SupportKey`/owners/`I_B/U_B`/`BallKey`).
- Sécurité GCP : deux générations arrêtées et certifiées `TERMINATED` ; mais reçu non atomique (transcript écrasé par une récupération refusée), pas de deadline globale, `timeout` sans `--kill-after`. Réparation : dossier par `generation+tar_sha`, rename atomique, manifest JSON.
- Porte suivante : séparer physiquement les ledgers q2/q3/q4, réparer owner q3 et census `I_B/U_B/BallKey`, remplacer le produit de lentille par la sélection axiale — **avant** toute nouvelle rampe.

### 1.4 AUDIT_POSITIF_DELTA_AC61A77_00CF78C_20260815.md
Second auditeur (revue statique, sans rejeu) ; delta après `eb1b52a`. Ton positif : les 5 rétractations de `32e11e7` montrent que le protocole distingue enfin observation/explication/théorème.

- **Reçus** : refus `--vrai-vivant` si `masse_non_decide != 0` (code 3) ; vrai `ceil(sqrt)` ; invariant `survivantes >= W-vivantes` correctement déclassé (circulaire) ; rétractations dual-tree et facteur 6,4 ; sampler réhabilité (résidus `-1,50` à `+1,52` σ binomiaux).
- **Théorème central (Proposition 1, Campbell–Mecke)** : Poisson homogène intensité `lambda` dans `R^d`, région témoin `|W(a,b)| = v r^d`, paire W-vivante si `< h` points dans `W`. Alors `E[V_h(Q_L)]/E[|X ∩ Q_L|] → s_{d-1} h / (2 d v)` ; **en dimension 3 : `E[V_h]/E[n] → 2 pi h / (3 v)`**. Preuve : `integral_0^inf P(Poisson(t)<h) dt = h`.
- **Proposition 2 (coût de lentille)** : pour une paire W-vivante (mesure de Palm) : `E[#(X∩W)|vivante] = (h-1)/2` (uniforme sur `{0..h-1}`), `E[lambda|W| | vivante] = (h+1)/2`, `E[#(X∩L)|vivante] = (R h + R - 2)/2` avec `R=|L|/|W|`. Avec `R=10,86, h_4=8` : **47,9 candidats**, pas 87. Mesure `uniform=38,9` proche du modèle ; `eight_clusters=87,8` révèle l'inhomogénéité.
- **Contre-exemple déterministe au `O(h)`** : `Theta(n)` points dans `L\W` sans point dans `W` → pire cas linéaire par ancre ; `two_lines` : `Theta(n^2)` ancres vivantes et zéro porteur aigu → « ne devient pas quadratique » est **faux sans hypothèse**.
- **Proposition 3 (condition déterministe)** : nuage `d`-Ahlfors régulier (`c r^d <= #X∩B(x,r) <= C r^d`) et fuseau contenant une boule de rayon `kappa_q |ab|` ⇒ `|V_q| = O((C/c) h_q n)`. Échoue exactement sur les mélanges séparés, droites croisées, vides macroscopiques.
- Corrections : formuler les pentes `[1,068 ; 1,163]` comme observations locales ; comparaison `s=6/s=8` sur univers commun (`statut PairId : D0|OFFCAP|CLOSED|SURVIVE`) ; exclure `D=0` (`T+ = C(n,2) - sum_x C(m_x,2)`) en gardant les multiplicités comme témoins ; mou : publier `mu-1` ET `1-1/mu` ; `V=0 → mu=+inf` ou `NA`, jamais `0`.
- P2 : requête exacte d'intersection de deux boules sur octree Morton (`OUT/IN/descente`) → CSR de candidats par arête sans rescanner les `n` points ; `two_lines` doit passer de `Theta(n^2)` ancres à zéro seed positif **sans allocation quadratique**.

### 1.5 AUDIT_PREFILTRE_COMBINE_HMAX_Q2_Q3_Q4_20260815.md
Auditeur ; audit fondateur du préfiltre combiné WSPD (pin `4cd1f82` ; P0 q2 réparé ensuite par `3bf1bf3`).

- **Fuseaux exacts** (avec `H(a,b,z)=(b-z)·(z-a)=D^2/4-||z-m||^2` et `Xi=||(b-a)×(z-a)||^2`) : `W_2={H>0}`, `W_3={H>0, 3H^2>Xi}`, `W_4={H>0, 2H^2>Xi}` ; `W4 ⊂ W3 ⊂ W2` ; l'égalité est le shell, jamais créditée.
- **Sens de l'implication corrigé** : `W_q(a,b) = intersection des intérieurs de toutes les miniboules admissibles` ⇒ `z ∈ W_q ⇒ z intérieur à toute complétion`. L'implication inverse (écrite dans `PROPOSITION.md` §6bis.1) est **fausse** pour q3/q4 (contre-exemple : deux complétions équilatérales, site `(0,1,0)`). Le fuseau donne un **minorant**, jamais un census.
- **Seuils sûrs** : `|sigma| >= q + |I_B|` (déf. 28 du manuscrit : pas d'intrus dans l'intérieur ouvert) ⇒ tout candidat `|sigma|<=s_max` impossible si `|I_B| >= h_q := s_max-q+1`. Sans hypothèse de position générale.
- **Ensembles maximaux sous factorisation cœur/A/B** : `C_q(A,B)={z ∉ A∪B : ∀a,∀b, z∈W_q(a,b)}`, `A_q(a;B)`, `B_q(A;b)` — deux à deux disjoints par `PointId`, somme = minorant licite. Maximalité **relative au schéma**.
- **P0 q2 (fausse fermeture réelle)** : le même nœud était crédité en bloc puis recrédité par feuilles ; à `n=160` : `uniform` 573 faux rejets (3083 annoncées vs 3656 vraies vivantes), `terrain` 713, `eight_clusters` 353. Réparation : pile `(node, active_lane_mask)`, lane effacée après crédit bulk. Les portes étaient vertes par vacuité (`recouvrements` jamais alimenté).
- **q3/q4 sûrs mais non maximaux** : `xi_max_over_box` maximise les composantes séparément (majorant) et les extrema `H_min/Xi_max` sont décorrélés — 4 contre-exemples entiers gravés. **Le correctif exact existe déjà** : `F_q = ||(b-a)×(z-a)|| - sqrt(c_q) H` est séparément convexe ⇒ maximum aux coins : **8 coins** (`h_a/h_b`), **64 couples** (cœur, z fixé), **512 triples** (`A×B×Z`) — `spindle_cone.hpp::all_lane_of_box`, `soc64_rect.hpp::corner512_all_lane`.
- AABB exact ≠ maximal discret (coins fictifs) ; hiérarchie : (1) max abstrait discret, (2) max exact AABB continu 8/64/512, (3) sous-certificat courant.
- **Coût** : histogramme `O(|A|+|B|)` une fois les `h` connus, mais formation de `h_a,h_b` en `O(3(|A|^2+|B|^2))` — éviter `A×B` en gardant les self-joins ne change pas l'ordre. Route : auto-jointure hiérarchique disjointe avec blocs ALL crédités paresseusement.
- Autres : oracle de couverture par `PairId` matérialisé exigé (la masse `C(n,2)` peut compenser omission+doublon) ; histogramme 16 cases vs `s_max<=32` ; paires `D=0` survivantes à tort ; mutant dangereux = `h_q-1` (pas `h_q+1`).

### 1.6 AUDIT_REAUDIT_DUAL_TREE_COEUR_BOULE_SEPARATION_EB1B52A_20260815.md
Auditeur ; démonte deux conclusions quantitatives de Claude.

- **Dual-tree : gain `2,2–3,0x` = artefact de baseline** : la baseline bouclait 3× sur les lanes alors que `corner8_lane` est déjà multi-lane. Contre une baseline équitable fusionnée, le dual-tree coûte **+30,0 % (`terrain`), +0,22 % (`uniform`), +7,52 % (`eight_clusters`)** d'évaluations à `n=4000, s=6`. La vraie optimisation reçue est la **fusion des trois lanes**. Le dual-tree reste sémantiquement exact (conservation prouvée : partition ordonnée des couples, masque de lanes, mutant sans masque → 212 puis 1525 fausses morts).
- **`s=6` vs `s=8` : le cap explique 99,052 % du gain** à `terrain,n=32000` : gain total `49 566 930` ancres dont `49 096 900` par baisse de masse hors `cap-cellule=512` ; `cellule_max=482` est calculé **après** le rejet (argument inversé). Sur masse jugée : gains `17,503 %` (n=8000, sans hors-cap) et `17,238 %` (n=32000) — **l'effet intrinsèque de la séparation est presque invariant**.
- **`--vrai-vivant`** : `V_q = {(a,b) : ||a-b||>0, |P ∩ W_q(a,b)| < h_q}` (« W-vivantes », indépendant de la WSPD) — exact seulement si `masse_non_decide=0` (contre-rejeu : cap=1 → publie 1201 au lieu de 1594 en se disant exact) ; l'invariant `survivantes>=vraiment vivantes` est **circulaire** (le mutant `bulk-sans-masque` ferme une vraie paire et le mode imprime quand même `mou=1.000`).
- **Coût cubique au pire cas** : `O(n^2 + n|union_q S_q|)` ; `n<=40000` autorise ~`9,6·10^13` appels — pas un budget.
- **Sampler** : erreur de Claude = comparaison d'erreurs relatives à des écarts-types absolus ; écarts-types relatifs corrects `2,414/1,207/0,604 %` pour `K=5000/20000/80000` (`uniform,n=600,q4`, `T=179700, V=45913, p=0,2555`) ; les 9 écarts entre `-1,50` et `+1,52` σ — binomial compatible.
- **Borne couplée sûre** : `R_coup = kappa_q d - sqrt((4 kappa_q^2+1)(r_A^2+r_B^2)/2)` (via `|p|^2+|w|^2=(|u|^2+|v|^2)/2` et Cauchy) ; prendre `max(R_dec,R_coup)`.
- Boule extérieure redondante par théorème (si `Z` disjointe de la boule extérieure alors `H<=0` partout, donc `h_any_upper` la subsume). Pertes gratuites : `floor_sqrt+1` au lieu du vrai `ceil` ; tangence `near2>=R2` (boule ouverte) ; commentaire « `R4+1` absorbé » faux (fixture q3 : `a=(0,0,0), b=(7,0,0), z=(3,2,0)`, rayon vrai `14/sqrt(3)=8,083`).

### 1.7 AUDIT_REAUDIT_PREFILTRE_COMBINE_COEUR_BOULE_41DFD2C_20260815.md
Auditeur ; ré-audit au `HEAD=66b4f0c` : réparation q2 reçue, cœur-boule cadré, apex-boule réfutée.

- **q2 réparé et reçu (domaine u16)** : `Frame{node,mask}`, antichaîne de sous-arbres disjoints par lane ; fixture `coeur5` (`bulk_credits=1`, 21/21 vivantes), mutant → 5 IDs doubles + 1 fausse morte (code 1) ; `4054/8666` sur `uniform,n=160` = exactement l'ablation sans bulk ; 756 campagnes + 192 parités sans désaccord.
- **Corner64 reçu** : lanes ponctuelles `q2: H>0 ; q3: H>0 et 4H^2>ET ; q4: H>0 et 3H^2>ET` (avec `e=z-a, t=b-z, H=e·t, E=|e|^2, T=|t|^2`) ; cône de Lorentz convexe ⇒ les 8×8 couples de sommets décident `ALL` sur l'enveloppe continue. Le `+17/+18 %` de surcoût n'est pas reçu (pas de brut chronométrique).
- **Boule midpoint (ouverte)** : plus grande boule centrée en `m` dans le fuseau ouvert : `kappa_2=1/2`, `kappa_3=1/(2 sqrt(3))`, `kappa_4=sin(15°)`. Le shell n'est pas crédité (fixtures entières : `4H^2=ET`).
- **Deux boules d'endpoints** : `R_dec,q = kappa_q(d-r) - r/2` (avec `d=|c_B-c_A|, r=r_A+r_B`) ; borne couplée `R_coup,q = kappa_q d - sqrt((4 kappa_q^2+1)(r_A^2+r_B^2)/2)` ; dans le cas équilibré `R*_q = kappa_q d - r sqrt(1+4 kappa_q^2)` est la **plus grande** boule centrée. **Seuils continus équilibrés** (convention `d >= (s+2)r`) : `s>2sqrt(2)-2` (q2), `s>2` (q3), `s>2,351` (q4).
- **Convention WSPD réelle du code** : `d - r_A - r_B >= s·max(r_A,r_B)` ⇒ `R_dec,q/r >= kappa_q s/2 - 1/2` ; tableau `R/r` : q2 : 1,0/1,5/2,0 ; q3 : 0,366/0,655/0,943 ; q4 : 0,276/0,535/0,794 pour `s=6/8/10` (le tableau `4,000/5,464/5,864` de Claude était décalé de 2 unités). En entier, petits nœuds = pire cas (`rmax2=1` demande `s>=3` q2, `s>=6` q3/q4 pour le chemin entier).
- **P0 apex-boule (`66b4f0c`)** : `apex_sin2` met au carré `sin(theta'_q - arcsin(N/D))` **sans tester le signe de `gamma_q`** — contre-fixture entière à `separation=1` (q4 : `U=4 000 000, N=1999, W=3999` ; accepte `1 198 524 000 000 < 1 760 229 561 475` alors que `3H^2-ET = -12 493 898 044`) → `oracle_faux_morts=1`. Gardes exactes : `q3: 3W>N^2 ; q4: 2W>N^2` avant le carré. De plus la conclusion de complexité est fausse : arrêt après `h_q` **succès**, pas essais → `Theta(|A|^2)` possible.
- **Cône robuste exact face à une boule partenaire** : `Ball(c,r)` entière dans le cône de lane q ssi `s·sin(alpha_q) - rho·cos(alpha_q) > r` (distance signée au complément du cône) ; sans normalisation : `q2 : J > r sqrt(E)` ; `q3 : sqrt(3) J - sqrt(Q) > 2r sqrt(E)` ; `q4 : sqrt(2) J - sqrt(Q) > sqrt(3) r sqrt(E)` avec `J=e·t0, E=|e|^2, Q=E|t0|^2-J^2`.
- **Test fixe Q30 sans plancher final** : `D=2^30`, `A_q=floor(2 kappa_q D)`, `T_q = A_q L2 - S2 D` ; crédit ssi `T_q>0 et dist2·D^2 < T_q^2` — tout en `i128` sous u16 ; constantes certifiées à la compilation (`3A_3^2<D^2` ; `X=2D^2-A_4^2, X>0, X^2>3D^4`).
- **Domination** : avec `sphere_of(box)` (circonscrite à l'AABB), le cœur-boule est un **sous-certificat de Corner64** — jamais un élagueur de Corner64. Vrais gains complémentaires : sphère des **points** (MEB/Ritter), incomparable à l'AABB, union par IDs.
- **Enveloppes convexes = maximalité discrète** : remplacer `A,B` par `conv(A),conv(B)` ne change pas les quantificateurs ; hulls/k-DOP = échelle de compromis.
- P0 domaine : `--coord` non borné → overflow UBSan et faux morts ; `--oracle=N` n'impose pas `n<=N` ; parsing CLI avant validation (`--points=4294967298` → `n=2`).

### 1.8 AUDIT_RECEPTION_COEUR_JUNG_SEED_A609AA_20260815.md
Auditeur ; réception mathématique du « cœur permanent de Jung » par seed (proposé dans l'audit 1.10).

- **Paramétrisation** (variables du noyau `q4seed_axis_topr4.hpp`) : `d=b-a, u=x-a, D=d·d, G=DE-F^2=||d×u||^2, n=d×u, W=E(D-F)d+D(E-F)u` ; centres `c(tau)=a+(W+tau n)/(2G)` ; `c0=a+W/(2G)` ; avec `s=|tau|/(2 sqrt(G))` : `||c(tau)-c0||=s`, `R(tau)^2=R0^2+s^2` ; Jung : `R(tau)<=RJ=sqrt(3D/8)` ⇒ `0<=s<=T=sqrt(RJ^2-R0^2)`.
- **Intersection commune exacte** : `K_J(T) = {z : ||z-c0||^2 + 2T|(z-c0)·e_n| < R0^2}` — décidée par les deux puissances d'extrémité `P_z(±tau_max)<0` (`P_z` affine en `tau`).
- **Boule centrée maximale** : `rho = RJ - sqrt(RJ^2-R0^2)` ; pour `ab` arête maximale d'un triangle strictement aigu, `C ∈ [60°,90°)`, `R0=|ab|/(2 sin C)` ⇒ `|ab|/2 < R0 <= |ab|/sqrt(3)` et **`sin(15°)|ab| < rho <= |ab|/sqrt(6)`** ⇒ **`closed_ball(c0,|ab|/4) ⊂ K_J`**.
- **Correction** : l'inégalité **large** `<=` au rayon `|ab|/4` est **sûre** (car `rho > sin(15°)|ab| > |ab|/4`) — ne pas la déclarer mutant létal. Mutant réellement faux : rayon `17|ab|/64` (réfuté par triangle isocèle presque rectangle `a=(0,0,0), b=(64,0,0), x=(32,33,0)`). Raffinement optionnel : `33|ab|/128` (`4096||v||^2 <= 1089 D G^2`, +10 % de volume).
- **Forme entière** : `v(z)=2G(z-a)-W`, `z-c0=v(z)/(2G)` ; test : `4||v(z)||^2 <= D G^2`. **Largeurs u16** : `D,E,|F|<2^34, G<2^68, |W_i|<2^86, |v_i|<2^87` ⇒ `4||v||^2 < 2^177`, `D G^2 < 2^170` → **`BigInt<4>` requis, `i128` insuffisant** ; carrés après promotion. Max sur AABB **séparable** : 6 évaluations de composante + 3 max (pas 8 coins).
- **Ledger obligatoire** : `SeedCoreQuarter ⊂ SeedJungPermanent16` — jamais `p = p_quarter + p_jung` naïf ; passes avec spans masqués, `p_core = |union|` saturé à `r4` ; interface vers le noyau axial : `initial_permanent_count + spans/digest + active ranges`, `k = r4 - p_total`.
- **Fates exclusifs** : `DEAD_CORE_QUARTER / DEAD_JUNG_PERMANENT / OPEN_AXIS / PENDING_RESOURCE / UNSUPPORTED_DEGENERACY / NUMERIC_FAILURE` ; **`PENDING_RESOURCE` ne devient jamais `DEAD`**.
- Fixture seed : `a=(1000,1000,1000), b=(1100,1000,1000), x=(1050,1060,1000)`, centre `c0=(1050, 1000+55/6, 1000)`, 8 IDs près du centre → `DEAD_CORE_QUARTER`, zéro root comparée.

### 1.9 AUDIT_RECEPTION_SCISSION_CAP_5CE2634_20260815.md
Auditeur ; réception positive du commit `5ce2634` (scission récursive du cap, familles scanline, rétractation two_lines, mou `inf/NA`).

- **Scission mathématiquement saine** : chaque paire a un unique LCA ⇒ couverture exacte ; scinder `U×V` en `U_g×V ⊔ U_d×V` conserve la partition ; la séparation est **retestée** sur chaque enfant (les sphères d'AABB ne sont pas emboîtées) ; terminaison par décroissance stricte des populations.
- **Formules exactes `two_lines`** (`A_i=(i,0,0), B_j=(0,j,H)`, `n=2m`, `H=65535` par défaut) : `V_2 = 10n - 55` ; `V_3 = n^2/4 + 9n - 90` ; `V_4 = n^2/4 + 8n - 72`. Table de vérification `n=300/600/1200/2400` (ex. q4 : `24 828 / 94 728 / 369 528 / 1 459 128`). Terme linéaire par droite : `h m - h(h+1)/2` ; q2 croisé : `i+j-2` témoins, survit si `i+j<=11` (55 paires). Golden analytique recommandé (voir 1.13 pour le domaine corrigé).
- **P0.1** : les tests de scission vérifient la **masse totale** (`masse=C(n,2)`) mais pas l'exact-once par `PairId` (compensation omission/doublon possible) — ajouter `--oracle=200` sur les six familles + mutants `cap-oublie-enfant`, `cap-duplique-enfant`.
- **P0.2** : doublons `D=0` incohérents (legacy saute sans compter ; lignes ordinaires sur `C(n,2)` ; garde finale et échantillonneur non corrigés) — fixture 5 IDs dont 2 colocalisés : `paires_D0=1, univers_ancres=9, V_2=V_3=V_4=9`, mutant `vivant-inclut-D0` → `10/10/10` doit mourir.
- **P0.3** : le scan « sans O(n^3) » est **cubique au pire cas** — `two_lines` : `|S_4|=Theta(n^2)`, chaque paire croisée scanne `Theta(n)` sites ⇒ `Theta(n^3)`. Porte de budget `--max-vivant-visites`, invariants `evals==travail`, `travail <= (n-2)*paires_uniques`.
- **P0.4** : matérialisation des `rects` elle-même quadratique à petit cap (`C(65535,2) > 2 G` rectangles) → `--max-rectangles`, traitement en flux.
- **Scission exacte pour la couverture mais pas neutre pour le filtre** : le minorant `h_coeur+h_a+h_b` n'est **pas monotone** sous raffinement (un témoin quittant `A` peut perdre son crédit) — sweep `cap ∈ {1,4,16,64,512}` avec ensembles de `PairId`.
- `coord=65536` (étendue) non rejouable via CLI ; constantes de dimension intrinsèque : q4 W-vivant `34,6244`/point (plan) vs `139,0696` (volume) ; lentille `14,2305` vs `47,8917`.

### 1.10 AUDIT_SUIVI_PORTEUR_AIGU_GATEWAY_JUNG_207B542_20260815.md
Auditeur ; suivi constructif — c'est ici que naissent le gateway complet et le cœur de Jung.

- Reçus : lemme porteur aigu (`x carrier aigu de l'owner ab ⇔ E<=D, X<=D, H<0`, tie-break `EdgeKey`), stricte `H<0` (fixture `H=0` tue `H<=0`), exact-once scission par `PairId`, réfutation du facteur deux (voir 1.14).
- Compteurs trop forts : `V4_pair_walive` = `S4_pair_prefilter_survivors` dans `--seeds` ; `est_seed` = owner **faible** (golden `regular_tetra : weak_owner_carriers=12, exact_owner_carriers=2`).
- **Le certificat rectangle mesuré ne couvrait que `H>=0`** — les deux coupures singleton qui marchent sont exactement les deux clauses du gateway : `bloc_dans_boule_diametrale ⇔ Phi_max<=0` et `bloc_hors_lentille ⇔ Delta_E_max<0 ou Delta_X_max<0` (avec `Phi=-H`, `Delta_E=D-E`, `Delta_X=D-X`). Classifieur trivalent : `DEAD_CERTIFIED / ALL_STRICT_OWNER_CERTIFIED / MIXED` ; **précision** : les six extrema sont exacts séparément, le verdict n'est pas un oracle d'existence d'un triplet commun (contre-exemple `A={0},B={10},C=[-1,11]` reste `MIXED`).
- **Théorème du cœur permanent de Jung** (§4) : `B°(c0, rho) ⊂ ∩_{tau∈J_f} B°(c(tau),R(tau))` avec `rho=RJ-sqrt(RJ^2-R0^2)` ; `B°(c0,|ab|/4)` ⊂ intersection de toutes les boules q4 admissibles du seed. Forme entière `4||v(z)||^2 < D G^2`, max AABB séparable (6 évaluations + 3 max), `BigInt<4>`.
- **`SeedJungPermanent16`** (§5) : `P_z(tau)=A_z - tau B_z` convexe en `z`, affine en `tau` ⇒ max sur `Z×[-tau_max,+tau_max]` atteint aux **8 coins × 2 bouts = 16 signes** ; si tous strictement négatifs, tout le nœud est permanent. Évaluable par `sgn_A_moins_Ytau` sans former `tau_max`.
- **Ordonnance coût-aware** : 1) `SeedCoreQuarter` ; 2) `SeedJungPermanent16` ; 3) crédits jusqu'à `p=8` → `DEAD_PERMANENT` sans aucun root ; 4) sinon `Q4SeedAxisTopR4` avec `k=8-p` ; 5) First_k/Last_k.
- P2 architecture : **ne jamais construire un tableau de carriers** — `C4_carrier` reste une masse logique de blocs, pas un buffer résident. Gates P3 : `two_lines_gateway_factorized` (formule analytique, `C4=0`, `PairId_cross_expanded=0`, pente CarrierBlocks < 1,3), `regular_tetra_owner_tie`, `lens_trichotomy` (`lens = q2_strict + H0_shell + acute_carrier`), `seed_core_eight`, `one_acute_incident_face_q4`.

### 1.11 AUDIT_SUIVI_POSITIF_P05_7493DECA_POISSON_20260815.md
Auditeur ; réception du P0.5 (balayage fusionné) + constantes Poisson exactes des trois fuseaux.

- **`pair_lane` exact** : avec `e=z-a, t=b-z, H=e·t, E=|e|^2|t|^2` (attention : ici `E` = produit des carrés) et `Xi=E-H^2` : `q2 : H>0 ; q3 : 3H^2>Xi ⇔ 4H^2>E ; q4 : 2H^2>Xi ⇔ 3H^2>E`. Ordre de retour `4,3,2` ; strictes correctes ; `z=a`/`z=b` → 0.
- Fusion des lanes propre (extinction à `h_q` licite, mutant `vivant-sans-extinction` réellement neutre) ; « redondance à huit » : moyenne mesurée `1,245–1,845` évaluations (`1+7 P(H>0)`) — le gain venait de la fusion des 3 passes.
- **P0** : la branche `D=0` n'est exercée par aucune famille (toutes dédupliquent) ; frontières exactes de `pair_lane` à graver (table de 6 cas entiers, mutants échange `3/4` et `>` → `>=`).
- **Constantes Poisson 3D des fuseaux** (fuseau : `x^2+r^2+alpha_q r < 1/4`, `alpha_2=0, alpha_3=1/sqrt(3), alpha_4=1/sqrt(2)` ; `v(alpha)=4 pi int_0^kappa r sqrt(1/4-r^2-alpha r) dr`, `kappa=(sqrt(1+alpha^2)-alpha)/2`) :
  - `v_2 = pi/6 = 0,523598776` → `E|V_2|/n → 40,000` ;
  - `v_3 = pi(27-4 sqrt(3) pi)/108 = 0,152262746` → `123,796` ;
  - `v_4 = 0,120480375` (forme close `pi(28-9 sqrt(2) pi+18 sqrt(2) asin(1/sqrt(3)))/96`) → **`139,070`**.
- **Validation empirique** : `|V_4|/n = 94,88 / 103,03 / 109,88 / 115,55` à `n=2000/4000/8000/16000` ; fit `C + beta n^(-1/3)` (correction de bord) → `C=136,019` (`-2,2 %` de 139,070), `beta=-520,248`. Les exposants `1,119→1,093→1,073` = convergence vers la loi linéaire, pas superlinéarité.
- **Lentille** : `R_4 = (5 pi/12)/v_4 = 10,864814574` ; `E[N_L | W4-vivante] = (R h + R - 2)/2 = 47,8917` (le naïf `R·h=86,9` est faux, biais vers les petites échelles) ; mesures : `uniform=38,94` (proche), `eight_clusters=87,82` (`1,83×` — inhomogénéité).
- Priorités : fixture `D=0` + frontières ; unités (`paires_uniques/scans_lane`) ; référence poissonnienne dans les campagnes (`V_q/n` contre `n^(-1/3)`) ; **requête octree exacte du fuseau** (`ALL/NONE/UNKNOWN`, arrêt à `h_q`) ; instrumenter la contraction `W-vivant → seeds positifs → supports exacts → fusions`.

### 1.12 CONTRE_AUDIT_POSITIF_Q4_PROPOSITIONS_EB42B574_20260815.md
Second auditeur contre-audite la note du premier ; verdict positif avec trois garde-fous.

- **Taxonomie des étages reçue** : `V4_pair_walive → C4_carrier → M4_apex → W4_positive → H4_rank`, doublée d'unités **physiques** : `F4_block, R4_bundle, T4_site, N4_event, Z4_const`. Chaîne : `NeutralPairTape → AcuteCarrierGateway-q4 → Q4SeedAxisTopR4 → owner6+barycentriques+primary → BallKey/RLE → census/rang → fold HGP`. **Lane4 reconstruit ses carriers ; ne consomme jamais Lane3**.
- **Domaine `two_lines`** : `V4 = n^2/4+8n-72` exact ssi `2(m-1)^2 <= H^2+1` (avec `H=65535` : `m<=46341, n<=92682` — couvre la cible 50k). **Sphère vide explicite** : centre `c_ij=(i, j, (H^2+i^2-j^2)/(2H))`, puissances `Pow(A_k)=(k-i)^2, Pow(B_l)=(l-j)^2` ; compatible Jung ssi `H^2(i^2+j^2)+2(i^2-j^2)^2 <= H^4` (uniforme pour `m <= floor(H/sqrt(2)) = 46340`). Conséquence : **aucun certificateur de profondeur universelle ne peut tuer ces paires** ; seule la positivité (carrier) le peut.
- **Preuve zéro carrier** : pour `i<k`, `(A_k-A_i)·(B_j-A_i) = -(k-i)i < 0` (obtus) ; symétrique sur l'autre droite ⇒ `C4=M4=W4=H4=0`.
- **Ce que borne `2·r4`** : `R4_bundle(seed) <= 2(r4-p) <= 2r4` ⇒ `R4_bundle <= 2·r4·C4_carrier` ; mais cela ne borne **ni** `T4_site`, `M4_apex`, `output4`, `bytes` (un groupe égal peut porter un nombre arbitraire d'IDs — co-sphéricité/quantification). Hors `RelevantGP`, range-report complet obligatoire, jamais tronqué à `2·r4` IDs.
- **Le coût de sélection n'est pas la taille de sortie** : un scan plat par seed coûte `sum_seed |WitnessRegion(seed)|` — best-first Morton/BVH requis, `OVERFLOW` en continuation, aucun `O(k log n)` promis avant mesure.
- Trois gates : `two_lines_q4_stage_separation` (aucun tableau de taille `V4`, `AcuteCarrierBlocks emis=0`, `R4_bundle=0`, pente des blocs `MIXED` publiée) ; `regular_tetra_q4_positive` (`p0=(0,0,0), p1=(2,2,0), p2=(2,0,2), p3=(0,2,2)`, arêtes²=8, `c=(1,1,1), R^2=3, lambda_i=1/4`, `BallForm=(1,-2,-2,-2,0)`, `I_B=0`, exact-once sous 24 permutations) ; `cube8_shell_plateau` (`{0,2}^3`, tous sur `R^2=3` — `max_bundle_ids>1`, aucune troncature).

### 1.13 CORRECTION_AUDITEUR_FORMULES_TWO_LINES_20260815.md
L'auditeur **se corrige** : le domaine des formules `two_lines` était énoncé trop largement dans 1.9.

- Témoin `A_k, k<i` : `q3 : 3k^2 > j^2+H^2 ; q4 : 2k^2 > j^2+H^2` (symétrique pour `B_l`).
- Domaines exacts : **q3** : `m>=9` et `3(m-1)^2 <= H^2+1` → à `H=65535` : `m<=37837, n<=75674` ; **q4** : `m>=8` et `2(m-1)^2 <= H^2+1` → `m<=46341, n<=92682` ; **q2** : `V_2=10n-55` pour `m>=10`, indépendant de `H`.
- Les quatre tailles publiées restent dans le domaine ; le golden doit **vérifier l'hypothèse `(m,H)` ou refuser**.
- Invariant du contre-régime préservé hors domaine : `W-vivant potentiellement quadratique, carrier aigu owner exactement nul` (la preuve d'absence de carrier ne dépend pas de `H`).

### 1.14 CORRECTION_AUDITEUR_Q4_CENTRE_UN_SEUL_CARRIER_20260815.md
L'auditeur **rétracte** sa propre affirmation « chaque centre géométrique compte au moins deux incidences, donc `centres_shallow <= r4*m_e` ».

- **Contre-exemple entier exact** : `p0=(6,2,5), p1=(0,3,3), p2=(1,4,6), p3=(5,3,1)` ; longueurs² : `41,30,18,11,29,42` → owner unique `e=p2p3` (42), sans tie. Bien centré : `c=(83,81,97)/26, R^2=7259/676`, barycentriques `(70,49,109,110)/338` toutes `>0`.
- Une seule face incidente aiguë : `(p2-p0)·(p3-p0)=3>0` (face `p2p3p0` aiguë) ; `(p2-p1)·(p3-p1)=-1<0` (face `p2p3p1` obtuse). La source `Q4Seed3` ne crée que `(p2,p3,p0)` : **une seule incidence proposée** — le lemme de complétude promet **au moins une** face aiguë, pas deux.
- Bornes corrigées : `N4_event <= R4_bundle <= 2·r4·C4_carrier` (pas de division par deux). Coût Lane4 : `O(B4+M4+T4+R4_bundle+output4)`.
- Gate permanente `one_acute_incident_face_q4` : `owner=EdgeKey(p2,p3)`, `acute_incident_faces=1`, `Q4Seed3=1`, exact-once sous 24 permutations ; mutant `q4-exige-deux-carriers-aigus` doit perdre le support.

### 1.15 NOTE_AUDITEUR_ACUTE_BOX24_GATEWAY_20260815.md
Auditeur ; le gateway de positivité par blocs — la réponse architecturale à `two_lines`.

- **Condition exacte du carrier** : face `abx` strictement aiguë avec `ab` maximale ssi `Phi=(a-x)·(b-x) > 0`, `D-E>=0`, `D-X>=0` (tie par `EdgeKey`) ; identité `2 Phi = E+X-D`.
- **Extrema exacts de `Phi` sur trois AABB** : séparables par axe ; **maximum = 24 produits** (8 combinaisons/axe : bilinéaire en `(a,b)` + convexe en `x` → coins) ; **minimum = 12 produits** supplémentaires (sommet de la parabole `x=(a+b)/2` projeté : `x2=clip(a+b, 2*Clo, 2*Chi)`, `4 phi_i = (2a-x2)(2b-x2)`). **Largeur : i64 suffit** (produits ×4 sous 34 bits, somme 3D sous 36 bits) — ni i128, ni racine, ni flottant.
- Owner par intervalles corrélés (dans cette première note : enclosures sûres, non exactes) via `Delta_E = (b-x)·(b+x-2a)`, `Delta_X = (x-a)·(2b-a-x)` — factorisations qui conservent l'annulation de la hauteur `H` de `two_lines`.
- **Classifieur** : `NONE si Phi_hi<=0 ou Delta_E_hi<0 ou Delta_X_hi<0 ; ALL_STRICT_OWNER si Phi_lo>0 et Delta_E_lo>0 et Delta_X_lo>0 ; MIXED sinon`.
- **`two_lines` meurt exactement** : `x=A_k, k<=i` : `Phi=-k(i-k)<=0` ; `k>i` : `Delta_X=i^2-k^2<0` ; symétrique sur `B_l`. Seuls les blocs coupant `k=i` ou `l=j` sont scindés — aucune expansion des `m^2` paires.
- **Ordonnance** : octree des troisièmes sommets dans la fenêtre de lentille (index spatial, **jamais** une boucle sur tous les sites par paire — « installer la sortie de secours derrière l'incendie »), prétests distances, `AcuteBox24` + deltas, `NONE`→drop, `ALL`→`AcuteCarrierBlock` sans `PairId`, `MIXED`→scinder le facteur le plus incertain, feuille→owner exact/primary, puis `Q4SeedAxisTopR4` et sweep 1D.
- Mutants clés : `acute-min-coins-seuls` (le minimum d'une convexe peut être intérieur), `acute-oublie-DeltaE/X`, `acute-angle-large`, `acute-tie-accepte-en-bloc`.

### 1.16 NOTE_AUDITEUR_ACUTE_OWNER_EXACT_AABB_20260815.md
Auditeur ; améliore 1.15 — les extrema d'owner sont eux aussi **exacts** sur AABB continues.

- Clé algébrique : `delta_E(a,b,x) = (a-b)^2-(a-x)^2 = b^2-x^2-2a(b-x)` — le carré de la variable commune `a` **s'annule** → linéaire en `a` → extrema aux 2 extrémités de `A` ; à `a` fixé, `b` et `x` indépendants : `delta_E=(b-a)^2-(x-a)^2`.
- Primitive 1D : `d2_min(I,t)=0 si t∈I, sinon min((l-t)^2,(h-t)^2)` ; `d2_max(I,t)=max((l-t)^2,(h-t)^2)`.
- Formules exactes : `delta_E_hi = max_{a∈{Alo,Ahi}} [d2_max(B,a)-d2_min(C,a)]`, `delta_E_lo = min_{a} [d2_min(B,a)-d2_max(C,a)]` ; `delta_X` symétrique en `b` ; somme sur les 3 axes exacte (indépendance cartésienne).
- **Largeurs** : `(u-t)^2 < 2^32`, différence sous 33 bits, somme 3 axes sous 35 bits signés → **i64 partout**, aucune division/racine/flottant.
- Signes asymétriques justifiés : `Phi` strictement positif (égalité = angle droit = `NONE`) ; `Delta` peuvent être nuls (tie gagnable par `EdgeKey`) → `NONE` seulement si max strictement négatif ; `ALL` exige les deux `Delta_lo>0`.
- Pseudocode C++ complet fourni (`D2Range`, `delta_E_axis`, `delta_X_axis`). Mutant essentiel : `owner-utilise-Dmax-moins-Emin` (soustraire deux intervalles indépendants perd la corrélation et transforme des blocs évidents de `two_lines` en `MIXED`).
- Résultat : `AcuteBox24/12 (Phi) + OwnerD2Exact (Delta_E, Delta_X)` = gateway **entièrement exact sur l'enveloppe cartésienne** de `A×B×C`, fail-open uniquement parce qu'une AABB peut contenir des positions absentes.

### 1.17 NOTE_AUDITEUR_ORDRE_EXECUTION_APRES_5CE2634_20260815.md
Auditeur ; fixe l'ordre pratique pour éviter « neuf chantiers ouverts et zéro oracle fermé ».

- **Ordre** : P0 fermer le tape pair-level de `5ce2634` (oracle `PairId`, fixture `D0`, frontières `pair_lane`, budget `evals==travail`, `max_rectangles`) ; P1 microprototype autonome `AcuteCarrierGateway` (critère : zéro faux `NONE`, zéro faux `ALL`, mutants tués — la performance vient après) ; P2 séparer les étages q4 avec les 4 fixtures permanentes (two_lines, tétraèdre régulier, `one_acute_incident_face`, cube `{0,2}^3`) ; P3 raccorder `Q4SeedAxisTopR4` (`OVERFLOW` jamais agrégé aux `DEAD_*`, égalités range-reportées, aucun tableau de taille `V4`/`C4`) ; P4 modèles quantitatifs comme oracles.
- **Constantes Poisson de référence (P4)** : q4 W-vivantes/point ≈ `34,624` (surface) / `139,070` (volume) ; **carriers aigus/point ≈ `190,170` (surface) / `4 079,607` (volume)** ; **vrais supports q4 positifs de profondeur < 8 en volume ≈ `45·pi^2/2 = 222,066` par point** → ~`6,66 milliards` de supports à `30 M` points : **le streaming et le fold par vague sont obligatoires même à espérance linéaire**.
- Décision immédiate : pas encore le BVH q4 — A) fermer les cinq portes P0 ; B) microprototype gateway exact ; C) démontrer sur `two_lines` l'élimination sans `PairId` ; D) démontrer sur les deux tétraèdres qu'il ne perd rien.

---

## 2. Synthèse transverse

### 2.a Résultats mathématiques établis

**Fuseaux de témoins universels** (pour `H(a,b,z)=(b-z)·(z-a)`, `Xi=||(b-a)×(z-a)||^2`) :
- `W_2={H>0}`, `W_3={H>0, 3H^2>Xi}`, `W_4={H>0, 2H^2>Xi}` ; emboîtés `W4⊂W3⊂W2` ; shell (`=`) jamais crédité.
- Forme ponctuelle équivalente (`e=z-a, t=b-z, H=e·t, E=|e|^2|t|^2`) : `q3 ⇔ 4H^2>E`, `q4 ⇔ 3H^2>E` (attention à la double convention `E`).
- Sens : `z∈W_q ⇒ z intérieur à toute complétion admissible` (minorant fail-open ; l'inverse est faux pour q3/q4).

**Seuil de mort** : `|sigma| >= q + |I_B|` (déf. 28, pas d'intrus dans l'intérieur ouvert) ⇒ mort à `h_q = s_max - q + 1` sites strictement intérieurs distincts (`10/9/8` à `s_max=11`), sans position générale. Factorisation maximale sous contrat cœur/A/B : `C_q(A,B), A_q(a;B), B_q(A;b)`, disjointes par `PointId`.

**Autorités AABB exactes par convexité séparée** : `F_q = ||(b-a)×(z-a)|| - sqrt(c_q) H` séparément convexe ⇒ `ALL` décidé par 8 coins (`h_a/h_b`), 64 (cœur, z fixé), 512 (`A×B×Z`). Exact sur l'enveloppe continue, seulement suffisant sur les points (coins fictifs) ; les enveloppes convexes donnent la maximalité discrète.

**Boules de cœur** : `kappa_2=1/2, kappa_3=1/(2 sqrt(3)), kappa_4=sin(15°)` (boules ouvertes) ; `R_dec,q = kappa_q(d-r)-r/2` ; borne couplée `R_coup,q = kappa_q d - sqrt((4 kappa_q^2+1)(r_A^2+r_B^2)/2)` ; cas équilibré `R*_q = kappa_q d - r sqrt(1+4 kappa_q^2)` ; seuils continus `s>2sqrt(2)-2 / s>2 / s>2,351` (q2/q3/q4, convention `d>=(s+2)r`). Convention du code : `d-r_A-r_B >= s·max(r_A,r_B)` ⇒ `R_dec,q/r >= kappa_q s/2 - 1/2`. Domination : sphère circonscrite à l'AABB ⇒ sous-certificat de Corner64, jamais un élagueur.

**Cône robuste exact face à une boule partenaire** : `Ball(c,r)` dans le cône de lane q ssi `s·sin(alpha_q) - rho·cos(alpha_q) > r` ; sans normalisation `q2 : J>r sqrt(E) ; q3 : sqrt(3)J-sqrt(Q)>2r sqrt(E) ; q4 : sqrt(2)J-sqrt(Q)>sqrt(3) r sqrt(E)` (`J=e·t0, Q=E|t0|^2-J^2`).

**Lemme du porteur aigu** : `ab` arête maximale (`E<=D, X<=D`) ⇒ `x` carrier aigu ⇔ `H<0` (identité `E+X-D=-2H` ; `2Phi=E+X-D` avec `Phi=-H`). Trichotomie de la lentille : `{H>0}⊔{H=0}⊔{H<0}` = témoin q2 strict / shell droit / carrier aigu. Complétude : un q4 positif d'owner `ab` a **au moins une** (pas deux) face incidente aiguë — contre-exemple à une seule face : `p0=(6,2,5),p1=(0,3,3),p2=(1,4,6),p3=(5,3,1)`.

**Gateway aigu exact sur AABB (`AcuteBox24` + `OwnerD2Exact`)** : `Phi_max` par 24 produits, `Phi_min` par 12 (clip du sommet de parabole) ; `Delta_E/Delta_X` exacts par annulation du carré de la variable commune + `d2_min/d2_max` intervalle-point ; classifieur `NONE/ALL_STRICT_OWNER/MIXED` **exact sur l'enveloppe continue, tout en i64**. `two_lines` : preuve d'annihilation par blocs (`Phi<=0` avant l'endpoint, `Delta<0` après).

**Cœur permanent de Jung d'un seed** : centres `c(tau)=a+(W+tau n)/(2G)`, `R(tau)^2=R0^2+s^2`, Jung `RJ=sqrt(3D/8)` ; intersection commune exacte `K_J={z : ||z-c0||^2+2T|(z-c0)·e_n|<R0^2}` décidée par `P_z(±tau_max)<0` ; boule centrée maximale `rho=RJ-sqrt(RJ^2-R0^2)` avec `sin(15°)|ab| < rho <= |ab|/sqrt(6)` ⇒ **`closed_ball(c0,|ab|/4) ⊂ K_J`** (le `<=` est sûr). Forme entière `4||v(z)||^2 <= D G^2` (`v(z)=2G(z-a)-W`), max AABB séparable (6 évaluations). `SeedJungPermanent16` : 8 coins × 2 bouts, exact sur l'AABB continue. Réduction `k = r4 - p`.

**Théorèmes de Poisson (Campbell–Mecke)** : `E[V_h]/E[n] → 2 pi h/(3v)` en 3D ; constantes : `v_2=pi/6 → 40,000` ; `v_3=pi(27-4 sqrt(3) pi)/108 → 123,796` ; `v_4=0,120480375 → 139,070` (forme close donnée). Lentille conditionnelle : `E[N_W|vivante]=(h-1)/2`, `E[N_L|vivante]=(Rh+R-2)/2 = 47,8917` pour q4 (`R_4=10,8648`). Version déterministe : Ahlfors-régularité ⇒ `|V_q|=O((C/c) h_q n)`. Références de dimension intrinsèque : W-vivant q4 `34,624` (plan) / `139,070` (volume) ; carriers aigus `190,170` / `4 079,607` ; **supports q4 positifs profondeur<8 : `45 pi^2/2 = 222,066`/point en volume**.

**`two_lines` (contre-famille canonique)** : `V_2=10n-55` (`m>=10`) ; `V_3=n^2/4+9n-90` (si `3(m-1)^2<=H^2+1`, soit `n<=75674` à `H=65535`) ; `V_4=n^2/4+8n-72` (si `2(m-1)^2<=H^2+1`, soit `n<=92682`) ; sphère vide explicite `c_ij=(i,j,(H^2+i^2-j^2)/(2H))` compatible Jung pour `m<=46340` ⇒ aucun certificateur de profondeur universel ne suffit ; zéro carrier aigu (preuve exacte). Cible architecturale : `W-vivant Theta(n^2), carriers = 0, aucune allocation par PairId`.

**Bornes de sortie q4** : `R4_bundle <= 2(r4-p) <= 2·r4·C4_carrier` ; `N4_event <= R4_bundle` ; **aucune** borne induite sur `T4_site/M4_apex/bytes` hors `RelevantGP`.

**Primitive J2 (`Q_theta`)** : `Q_theta(z)=q A_z - p B_z`, un moteur unique First/Last/Census ; min AABB par clamp, max aux coins ; **largeur ~278 bits ⇒ `BigInt<5>`**. Largeurs du cœur de Jung : `4||v||^2<2^177 ⇒ BigInt<4>`. Gateway aigu : **i64**. Trois régimes de largeur distincts à respecter.

### 2.b Mesures chiffrées et décisions d'architecture

- **Ledger J0 CPU 48 cœurs (G4 comme CPU)** : `uniform,n=50000` : 4 M candidats (smax=6, 4 s) / 21,4 M (smax=11, 38 s) ; candidats/point `80,1` vs `428,7` (facteur 5,35, pas 12) ; **`6,09 G` paires de lentille** à smax=11, ratio lentille/q4 = 623,5. Terrain/eight_clusters : codes 3 dès 12 500 points (rapports cutoff 0,864–0,940 ; jusqu'à `24,1 G` paires de lentille).
- **Pivot axial** : propositions ÷59 (`48,8 M` paires → `830 k` roots à n=6000) ; mais temps stable ~25–28 s → le mur est `sum_seed |B∩P|` + rescans. Multiplicité seeds/arête ≈ 11.
- **GPU (RTX PRO 6000 Blackwell, CUDA 12.9)** : parité host/device `ecarts=0` sur 18,6 M verdicts / 5,79 G incidences ; kernel flat scan `5,0–14,0 Gsites/s` ; `179–292×` vs 1 thread host ; extrapolation kernel-only `~0,48 s` à 50k (3 lots complets uniquement) ; **K=5 : 2,45 G incidences > INT_MAX, ~9,8 Go d'IDs → le CSR plat par seed est condamné ; production par tuiles device + descente `Q_theta` + compaction**.
- **Dual-tree** : gain 2,2–3,0× rétracté (baseline inéquitable) ; surcoût réel +0,22 % à +30 %. **Décision : baseline ponctuelle multi-lane fusionnée = référence**.
- **s=6 vs s=8** : 99,052 % du gain attribué au cap ; effet intrinsèque ~17,2–17,5 %. **Décision : jamais comparer des séparations sans univers `PairId` commun/complet.**
- **Poisson comme oracle quantitatif** : `V_4/n → 139,070` (mesuré `C=136,019`, écart 2,2 %) ; lentille conditionnelle `47,9` (mesuré uniform `38,9`, clusters `87,8`).
- **Échelle 30 M points** : ~6,66 G supports q4 positifs attendus (volume) → streaming/fold par vague obligatoires.
- **Décisions structurantes** : positivité (gateway aigu) **avant** l'expansion pair-level ; Lane4 autonome (ne consomme jamais Lane3) ; unités logiques vs physiques séparées (`V4/C4/M4/W4/H4` vs `F4/R4/T4/N4/Z4`) ; fates exclusifs avec `PENDING/OVERFLOW` jamais convertis en `DEAD` ; mesure en flux, jamais par catalogue global.

### 2.c Bugs, erreurs, rétractations documentés

1. **Double crédit q2 (bulk + feuilles)** dans `combined_prefilter_probe.cpp` : fermeture **fausse** d'ancres vivantes (573/713/353 faux rejets à n=160) ; portes vertes par vacuité (`recouvrements` jamais alimenté). Réparé par masque de lanes par frame (`3bf1bf3`).
2. **Apex-boule sans garde de signe** (`66b4f0c`) : `apex_sin2` met `gamma_q` au carré sans tester `3W>N^2`/`2W>N^2` → faux témoin q4 à `separation=1`, `oracle_faux_morts=1`. Non couvert par les portes à s=6.
3. **Biais du cap** : 99 % du « gain s=8 » = masse hors cap ; `cellule_max` calculé après le `continue` (argument inversé). Idem `--vrai-vivant` exact seulement si `masse_non_decide=0` (deux exécutions code 0 avec comptes différents 1594/1201).
4. **Baseline dual-tree inéquitable** → gain 2,2–3,0× rétracté.
5. **Sampler** : erreur de normalisation (erreur relative comparée à un σ absolu) → retrait injustifié, réhabilité.
6. **`V4_pair_walive` ≡ `S4_prefilter_survivor`** dans `--seeds` (mou non appliqué) ; **owner faible sans `EdgeKey`** (triangle équilatéral compté 3×, tétraèdre régulier 12 incidences au lieu de 2).
7. **Rétractation de l'auditeur lui-même** : « chaque centre a deux incidences » — faux (fixture à une seule face aiguë) ; domaine des formules `two_lines` trop large (conditions `(m,H)` ajoutées).
8. **Arithmétique/CLI** : `floor_sqrt+1` au lieu du vrai ceil ; « `R4+1` absorbé par la stricte » faux (normes irrationnelles) ; `--coord` non borné → overflow UBSan + faux morts ; `--points=4294967298`→`n=2` ; `--oracle=N` sans borne ; `r4=65` déborde `seuil[64]` ; offsets CSR en `int` (overflow à K=5) ; `code=\0` dans le reçu CUDA ; `RESULTATS.md` recopie 14,8 M au lieu de 18,6 M de verdicts.
9. **Claims de coût faux** : « exactement et sans O(n^3) » réfuté par `two_lines` (`Theta(n^3)`) ; « instruction O(h) » vraie seulement en espérance sous Poisson ; « quasi linéaire » non reçu sans hypothèse (Ahlfors) ; « descente logarithmique » non prouvée (`Theta(n)` possible par ancre).
10. **Infrastructure GCP** : verdict avant scp (brut perdu), transcript écrasé par la récupération, TTL de clé incompatible, pas de deadline globale ni `--kill-after`.

### 2.d Pistes fermées dans cette tranche et pourquoi

- **Produit de lentille q4 (`for carrier, for apex`)** : `sum_e binom(m_e,2)` — milliards de paires dès 12 500 points sur amas ; remplacé par la sélection axiale top-`r4` (`O(m·k)`).
- **Grille + cutoff par maximum observé comme route J0** : la session G4 réfute sa propre porte (codes 3 sur terrain/clusters) ; le cutoff « maximum observé » déjà réfuté par `two_lines`.
- **Dual-tree comme optimisation** (dans sa forme mesurée) : transformation sémantique exacte mais plus chère que la baseline fusionnée — reste diagnostic/candidate GPU.
- **Boule d'apex unique (`66b4f0c`)** : plus faible que le cône exact, plus lente sur les 3 nuages, et fausse sans garde de signe — remplacée par le cône robuste / 8 coins.
- **Boule extérieure du cœur** : redondante par théorème (subsumée par `h_any_upper`).
- **Certificateur de profondeur universelle comme tueur de `two_lines`** : impossible (sphère vide compatible Jung existe pour chaque paire croisée) — seule la **positivité** tue.
- **Resserrement indéfini des certificats universels W** : le levier est après le W-vivant (positivité/owner), pas dans un cinquième certificat marginal.
- **CSR plat par seed vers le GPU** : condamné par les octets (9,8 Go à K=5) et l'overflow d'offsets — production par tuiles + descente + compaction.
- **Ablation « WSPD fixe + split de C seulement »** (gain 1,005) : fermée, mais ne ferme pas le gateway adaptatif `A×B×C`.

### 2.e Ce que la V4 doit conserver / éviter absolument

**Conserver (mathématiques et architecture)** :
- Les fuseaux `W_q`, les seuils `h_q=s_max-q+1`, la factorisation `h_coeur+h_a+h_b` par `PointId` disjoints, le sens fail-open de l'implication.
- Le **gateway aigu exact i64** (`AcuteBox24/12` + `OwnerD2Exact`) placé **avant** toute expansion par `PairId` — c'est la réponse prouvée à `two_lines` et directement la brique « élimination rapide dans la zone cœur » de la feuille de route V4.
- Le **cœur permanent de Jung** (`|ab|/4` séparable en `BigInt<4>`, puis `SeedJungPermanent16` à 16 signes) → `k=r4-p`, seeds tués sans root.
- La sélection axiale top-`r4` (`Q4SeedAxisTopR4`) avec groupes d'égalité range-reportés et la primitive `Q_theta`/`RationalBallRange` (First/Last/Census unifiés, `BigInt<5>`) pour la descente BVH.
- Le tie-break **`EdgeKey` sur vrais `PointId`** partout (owner canonique) ; la trichotomie de la lentille ; la borne `R4_bundle <= 2·r4·C4_carrier` (et pas mieux).
- Les boules de cœur `kappa_q`, la borne couplée `R_coup`, le cône robuste, le test fixe Q30 — tous à arrondis dirigés, boules **ouvertes**, aucun `double` décideur.
- Les **oracles Poisson** (40 / 123,796 / 139,070 ; lentille 47,89 ; supports 222,066/point) comme détecteurs de perte/duplication de masse — jamais comme preuves.
- Les fixtures permanentes : `two_lines` (avec domaine `(m,H)` vérifié), tétraèdre régulier `{(0,0,0),(2,2,0),(2,0,2),(0,2,2)}`, `one_acute_incident_face` `{(6,2,5),(0,3,3),(1,4,6),(5,3,1)}`, cube `{0,2}^3`, fixture `D=0` à 5 IDs, `coeur5`, contre-fixture apex `separation=1`.
- La discipline : exact-once par identité (`PairId`/`PointId`+ledger), masques de lanes, planchers de couverture par lane, mutants causaux, portes à code exact doublant les regex, oracle indépendant, « même fonction exacte host/device ».
- L'échelle : production par tuiles device, streaming/fold par vague (222 supports/point en volume), unités logiques vs physiques séparées, publication systématique de `node_visits/sites_lus/ALL/OUT/MIXED/H2D/kernel/D2H/HWM`.

**Éviter absolument** :
- Toute matérialisation indexée par paire ou carrier (tableau de taille `V4`/`C4`, CSR global par seed, catalogue de supports) — la V4 doit rester en flux/blocs, cohérent avec l'interdit `∝ C(n,k)`.
- Confondre `S_q` (survivantes du préfiltre) et `V_q` (W-vivantes exactes) ; comparer des cardinaux au lieu d'ensembles d'identités ; comparer des séparations/caps sur univers non appariés.
- Les fermetures par crédit non-exact-once (le double crédit q2 a été le P0 le plus grave, et il a failli renaître trois fois : cœur-boule q3/q4, quart+Jung, `p_initial`).
- Mettre au carré sans garde de signe ; `floor+1` comme plafond ; larges/strictes inversées sur le shell ; extrema décorrélés (`Hmin/Ximax`, `Dmax-Emin`) là où une forme couplée exacte existe.
- Convertir `PENDING/OVERFLOW/DEBORDEMENT` en mort ou les agréger aux morts géométriques ; tronquer les groupes d'égalité.
- Promettre `O(log n)`, `O(h)` déterministe, `O(k log n)` BVH, ou une pente à partir de 3 points — toute complexité se reçoit par compteurs (`node_visits`, `visites_max/ancre`) et par famille, `two_lines` incluse.
- Croire un benchmark kernel-only ou un préfixe `cap=1` (verdict `PREFIX_PARITY` seulement) ; publier un temps sans H2D/D2H/allocations ; extrapoler K=10 depuis des lots tronqués.
- Le jitter face aux dégénérescences (refus explicite), les rangs Morton comme identités, les CLI non validées (bornes u16, parsing complet, préflight d'octets/offsets 64 bits).

---

## Questions ouvertes / ambiguïtés

1. **Gateway triple adaptatif `A×B×C` : jamais mesuré.** Le classifieur exact (1.15/1.16) est prouvé mais aucun prototype, aucune pente, aucun HWM n'existe à ce pin. La politique de split « facteur le plus incertain » et le nombre de tâches `MIXED` sur familles positives restent inconnus. C'est le plus gros pari non validé que la V4 hérite.
2. **q3 est orphelin dans cette tranche** : aucun juge brute q3 dans la sonde axiale (`--verifie` ne compare que q2/q4), owner q3 signalé cassé (1.3) mais le détail est dans un contre-audit hors tranche. La feuille de route V4 (q3 avec témoin x formant un triangle aigu) devra définir sa propre autorité q3.
3. **Double convention du symbole `E`** : `E=||a-x||^2` dans les notes carrier/gateway, mais `E=|e|^2|t|^2` (produit) dans `pair_lane` (1.11). Idem « témoin » (région de centres dans le manuscrit vs site intérieur ici). La V4 doit fixer un lexique unique.
4. **La parité host/device CUDA est bornée** aux champs de `SeedOut` (24 IDs/côté, sans permanents/shell/census) et à 3 lots complets `uniform,smax=6` — rien ne qualifie les amas ni `smax=11` sur GPU. Le contrat V4 « K=10 < 100 ms » n'a aucune mesure amont : la seule extrapolation existante (~0,48 s kernel-only à 50k, `K≈5`-like) vient d'une source sous cutoff et du mauvais algorithme (flat scan).
5. **Monotonie du minorant sous raffinement WSPD** : établi non monotone (1.9 §4), mais aucune quantification de la perte/gain par cap ou s — le sweep apparié `cap ∈ {1..512}` avec ensembles de `PairId` demandé n'a pas encore de résultat dans cette tranche.
6. **Statut de la chaîne aval** (owner6, positivité finale, census `I_B/U_B`, `BallKey/RLE`, fold, forêt) : tous les audits la citent comme « toujours ouverte » ; aucune identité de `SupportKey` ou d'événement de fusion n'a été confrontée à un oracle dans cette tranche. Le « contrat d'une seconde » (et a fortiori 100 ms) reste non mesuré bout-en-bout.
7. **Constantes Poisson non re-dérivées ici** : `v_4=0,120480375`, `222,066` supports/point, `4 079,607` carriers/point proviennent de notes de l'auditeur (dont une hors tranche, `NOTE_AUDITEUR_POISSON_DIMENSION_INTRINSEQUE`) ; je les recopie sans les avoir vérifiées.
8. **`h` de la V4 vs `h_q` de la v3** : la feuille de route V4 parle de « h témoins dans la zone cœur puis h_a/h_b » — si la V4 change `K` (10 vs 5) ou `s_max`, tous les seuils `10/9/8` et les constantes Poisson associées changent ; les fixtures analytiques (`two_lines`) dépendent explicitement de `h_2=10,h_3=9,h_4=8`.
9. **Ambiguïté sur `r4`** : dans le noyau axial, `r4` semble jouer le rôle de `h_4=8` (profondeur < 8), mais l'API accepte `1<=r4<=64` et la sélection en autorise 64 — le lien contractuel exact entre `r4`, `h_4` et `K` n'est pas énoncé dans ces fichiers.
10. **WSPD s=6/8/10 de la feuille V4** : les seuils continus de positivité des cœurs (q4 : `s>2,351` en convention `d>=(s+2)r` ; mais chemin entier exigeant `s>=6` pour `rmax2=1`) suggèrent que le choix de `s` interagit avec l'arithmétique entière des petits nœuds — aucun audit ne tranche le `s` optimal ; l'arbitrage propre (univers apparié, cap neutralisé) reste à faire.
