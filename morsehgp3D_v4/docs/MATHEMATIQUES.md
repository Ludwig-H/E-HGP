# MorseHGP3D v4 — Dossier mathématique

Date d'ouverture : 17 août 2026 UTC.

Cadre : `phase=exploration_v4_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Ce document est la source unique de l'aspect mathématique de la v4. Chaque
énoncé porte un **statut** :

- `theoreme_manuscrit` — énoncé et prouvé dans le manuscrit (référence exacte,
  pages PDF = pages imprimées + 26) ; il est *invoqué*, jamais re-vérifié
  exhaustivement (plan de test § 3.2) ;
- `theoreme_v3` — énoncé pendant l'exploration v3. **Ce statut n'est pas un
  acquis** : la v3 a commis et rétracté de nombreuses erreurs ; tout énoncé
  `theoreme_v3` est une piste étayée dont la preuve doit être re-déroulée
  (par l'auditeur ou par une fixture v4) avant d'être invoquée sans garde.
  Les fixtures d'égalité v3 sont reprises, elles ;
- `derive_v4` — dérivé ici même ; **à faire valider par l'auditeur** avant
  toute promotion ;
- `mesure` — fait empirique chiffré, jamais une garantie ;
- `ouvert` — question posée, non résolue.

---

## 1. L'objet normatif (manuscrit, Parties I–II)

### 1.1 La hiérarchie HGP

- **Complexe de Čech** (Déf. 20, p. PDF 83) : `Č(X,r) = { σ ⊆ X : ∩_{x∈σ} B̄(x,r) ≠ ∅ }`.
- **K-polyèdres** (Déf. 21, p. PDF 84) : composantes connexes du graphe
  `Γ_K(X,r)` dont les sommets sont les (K−1)-simplexes (K points) de `Č(X,r)`,
  adjacents quand leur union est un simplexe de `Č(X,r)`.
- **HGP-Clusterer** (Déf. 22, p. PDF 84) : `θ_K^{HGP}(r) =` {K-polyèdres de
  `Č(X,r)`}. Pour K=1, c'est exactement le Single-Linkage (Partie I). Les
  K-polyèdres ne font que croître et fusionner quand r augmente.
- **Théorème 2** (p. PDF 86) : les K-polyèdres coïncident avec les amas
  discrets de forte densité K-NN, `θ_K^{HGP}(r) = H^{discrets}_{f̂_K}(r)`.
- Pour K ≥ 2, l'objet est un **recouvrement** des points, mais une **partition
  des (K−1)-simplexes** ; la partition stricte des points est un
  post-traitement (§ 9.1, vote pondéré, Prop. 7).

Statut : `theoreme_manuscrit`. Convention capitale : **le niveau est un
rayon** (`r = d/2` pour une arête), jamais un diamètre.

### 1.2 La réduction : rayon de naissance, Gabriel, K-MST

- **Rayon de naissance** (Déf. 25, p. PDF 110) : `ρ(σ) = inf_y max_{x∈σ} ‖y−x‖`,
  le rayon de la **miniboule** `B_σ = B̄(c_σ, ρ(σ))`. C'est le niveau exact
  d'apparition de σ dans la filtration. Jamais le circumradius (les deux
  divergent dès qu'un simplexe est obtus — Remarque p. PDF 114).
- **Position générale pour la filtration de Čech** (Déf. 26, p. PDF 110) :
  aucun point de `X∖σ` sur `∂B_σ`, pour tout σ. Sur grille u16 elle peut être
  violée : la sortie honnête est alors `unsupported_degeneracy`, jamais un
  jitter.
- **Fait 12** (p. PDF 111) : la miniboule est unique ; son **support**
  `S(σ) = σ ∩ ∂B_σ` a au plus p+1 = 4 points en 3D, `c_σ` est intérieur
  (relatif) à `conv(S)`, et retirer un point du support fait strictement
  décroître ρ. `|S| ≥ 2` toujours.
- **Adjacences élémentaires** (Prop. 5, p. PDF 112) : restreindre les arêtes
  de `Γ_K` aux seules unions de taille K+1 (les K-simplexes) ne change aucune
  composante. L'événement atomique est donc toujours « un (K+1)-uplet naît ».
- **Simplexe K-séparant** (Déf. 27, p. PDF 112–113) : σ (K+1 points) est
  K-séparant si deux de ses **facettes actives** (`ρ(τ) < ρ(σ)`) sont dans
  deux composantes distinctes de `Γ_K(X)_{<ρ(σ)}`. Pour K=1 : les arêtes du
  MST.
- **Simplexe de Gabriel** (Déf. 28, p. PDF 113) : `B̊_σ ∩ (X∖σ) = ∅` — la
  miniboule **ouverte** est vide de tout point extérieur.
- **Théorème 4** (p. PDF 114–115) : K-séparant ⟹ Gabriel. Sa preuve donne
  plus : un σ avec intrus `z ∈ B̊_σ` a toutes ses facettes actives déjà
  connectées strictement avant ρ(σ) (chemin `τ_s ↔ η_{s,t}^z ↔ τ_t` par les
  simplexes `σ_s^z = (σ∖{s})∪{z}` de rayon < ρ(σ)). **L'élimination par témoin
  est donc sans perte pour la forêt.**
- **K-graphe de Gabriel** (Déf. 29, p. PDF 115–116) : sommets = facettes d'au
  moins un K-simplexe de Gabriel ; pour chaque K-simplexe de Gabriel σ, une
  clique sur ses facettes au poids ρ(σ). **Prop. 6** (p. PDF 116) : ses
  composantes non triviales élaguées à r = les K-polyèdres non triviaux.
  Relier *deux* facettes actives (un chemin) suffit — la clique est un confort,
  pas une nécessité (Alg. 1 étape 4, p. PDF 126).
- **K-MST** (Déf. 30) et **Théorème 5** (p. PDF 117) : un arbre couvrant
  minimum du K-graphe de Gabriel, élagué à r, redonne exactement les
  K-polyèdres non triviaux, pour tout r. C'est l'objet que la v4 rend :
  **la forêt HGP = ce K-MST par K**, avec les niveaux ρ.
- **Théorèmes 6–7** (p. PDF 118, 125) : tout K-simplexe de Gabriel est porté
  par la mosaïque de Delaunay d'ordre K (resp. se lit dans `Del_{K−1}`). La
  v4 les utilise comme **caractérisations et oracles**, jamais comme chemin de
  calcul (interdit d'architecture : aucune mosaïque d'ordre supérieur
  matérialisée).

Statut : `theoreme_manuscrit`.

### 1.3 Le rendu § 9.1 (partition stricte, poids)

Sur les facettes effectivement construites `F_K` : score
`S_τ = Σ_{σ⊃τ, |σ|=K+1} ψ(ρ(σ))` avec `ψ(t) = 1/t^p` (toute ψ décroissante
admissible ; le choix `1/r` vs `1/r²` change le comportement filiforme —
fixture `birch2`, p. PDF 128–129) ; normalisation `T_x = Σ_{τ∋x} S_τ` ;
masse `m_τ = S_τ Σ_{x∈τ} 1/T_x` (celle que consomme `min_cluster_size`) ;
vote `V_x(c) = Σ_{τ∋x, ℓ(τ)=c} S_τ/T_x`, étiquette argmax avec départage
déterministe (Prop. 7). Statut : `theoreme_manuscrit` (avec une ambiguïté
opérationnelle : quelles facettes entrent dans `F_K`, voir § 6, Q4).

---

## 2. La réduction v4 : événements-boules et taxonomie q2/q3/q4

### 2.0 Contrats gravés par l'audit du 17 août (ETAT_COURANT)

- **Sites distincts** : le profil exact d'autorité travaille sur des
  positions distinctes ; les positions dupliquées sont **refusées**
  (`unsupported_degeneracy`) tant qu'un HGP pondéré n'est pas défini et
  prouvé. La bucketisation reste une commodité d'index, pas une sémantique.
- **Seuils effectifs** : `K_eff = min(K_max, n)`, `s_max = min(K_eff+1, n)` ;
  la constante 11 vaut pour la cible `n ≥ 11, K_max = 10`, jamais comme
  vérité générale (les petits oracles ne reçoivent pas 11 par défaut).
- **Niveau public = rayon AU CARRÉ exact** (q2 : `‖ab‖²/4` ; q3/q4 :
  fraction rationnelle canonique). Une variable interne peut rester un
  rayon, elle ne contamine jamais `ExactLevel`.
- **Un événement critique est une hyperarête/multifusion** sur tous ses
  bras : un chemin couvrant compresse la connectivité, il n'autorise pas à
  binariser la chronologie d'un plateau. Deux BallKeys distinctes de même
  niveau se traitent simultanément (contraction des durées nulles).
- **La sortie complète conserve les applications verticales entre ordres** :
  dix forêts indépendantes ne représentent pas seules la tour ordre-échelle.
- **`F_K` du rendu § 9.1 = TOUTES les facettes distinctes des événements**
  (`F_K^render`), pas seulement les actives (`F_K^conn`, suffisantes pour la
  connectivité seule) ; une variante active-only est une heuristique nommée.

### 2.1 Boule-événement ≡ simplexe de Gabriel

**Énoncé (`derive_v4` — à auditer, Q1).** Sous position générale (Déf. 26),
l'application `σ ↦ (S(σ), B_σ)` est une bijection entre :

- les K-simplexes de Gabriel σ de `X` (K+1 points), et
- les couples (S, B) où S ⊆ X est le support d'une miniboule B (|S| ∈ {2,3,4}
  en 3D, `c_B` intérieur relatif à conv S, S ⊂ ∂B) dont l'intérieur ouvert
  contient exactement K+1−|S| points de X.

*Argument.* Direct : Gabriel dit `B̊_σ∩(X∖σ)=∅`, donc les points de X dans
`B̊_σ` sont exactement `σ∖S(σ)` ; d'où `σ = S(σ) ∪ (X∩B̊_σ)` et la
profondeur `|X∩B̊_σ| = K+1−|S|`. Réciproque : pour un tel couple (S,B), le
simplexe `σ = S ∪ (X∩B̊)` a pour miniboule B (ajouter des points intérieurs
ne change pas la plus petite boule englobante, dont le support reste S), il
est de Gabriel par construction, et `|σ| = K+1`.

**Conséquence structurante.** Énumérer les événements de TOUTES les forêts
K = 1..K_max revient à énumérer les couples (support, boule) **peu profonds** :

- arité q = |S| ∈ {2, 3, 4} — la taxonomie q2/q3/q4 ;
- profondeur `depth = |X ∩ B̊|` ≤ `K_max + 1 − q` ; au-delà, la boule ne sert
  aucun K ≤ K_max. Avec `s_max = K_max + 1`, le seuil de mort d'une ancre est
  `h_q = s_max − q + 1` témoins intérieurs : **10/9/8** à K_max = 10,
  **5/4/3** à K_max = 5 (identique v3).
- une boule de profondeur d et d'arité q est UN événement de la forêt
  `K = q + d − 1`, de niveau ρ(B), qui relie les facettes actives
  `σ∖{s}, s ∈ S` (Théorème 4/Prop. 6 : relier celles-là suffit).

### 2.2 Les trois arités en 3D

- **q2** : toute paire {a,b} est son propre support ; la miniboule est la
  boule diamétrale `B((a+b)/2, ‖ab‖/2)`. Niveau `ρ = ‖ab‖/2`.
- **q3** : S = {a,b,x} est un support ssi le triangle est **strictement
  aigu** ; la miniboule est alors la boule (3D) centrée au circumcentre du
  plan du triangle. Si le triangle est obtus ou rectangle, le support retombe
  à l'arité 2 de son côté le plus long (Prop. 8, p. PDF 124, et Fig. 8.1,
  p. PDF 111). Fait v3 (`theoreme_v3`, PROPOSITION § 6bis.6) : l'arête
  maximale (a,b) étant fixée, l'acuité du triangle abx équivaut à l'acuité du
  seul angle en x, qui se lit sur `V² > D²` avec `V = ‖2x−a−b‖`,
  `D² = ‖ab‖²` — le porteur vit dans `lentille(ab) ∖ boule diamétrale`.
- **q4** : S = {a,b,x,y} est un support ssi `c_σ` est strictement intérieur à
  conv(S) (poids barycentriques de Cramer strictement positifs). « Tétraèdre
  bien centré » et « à faces aiguës » sont deux notions **distinctes**,
  chacune réfutant l'autre (fixtures v3 gravées).

### 2.3 L'ancre : arête maximale canonique

Tout support S est possédé par son **arête maximale canonique** (a,b) :
la plus longue arête, départagée par la plus petite `EdgeKey = (min PointId,
max PointId)` (`theoreme_v3`, identités PROPOSITION § 2.1 ; la fixture
« tétraèdre aux six arêtes égales » grave le départage). Propriétés :

- **Domaine (lentille).** Tout sommet du support est dans
  `lentille(a,b) = B̄(a,D) ∩ B̄(b,D)` avec `D = ‖ab‖` ; tout point intérieur
  strict de la miniboule est à distance < `0,966 D` du milieu m ; une seule
  requête `B(m,D)` couvre sommets et intérieurs (`theoreme_v3`,
  PROPOSITION § 6bis.6).
- **Jung.** `ρ(S) ≤ κ'_q · D` où le rapport rayon/diamètre est borné par le
  théorème de Jung en dimension 3 : `ρ/D ≤ √(3/8) ≈ 0,6124` (q4, cas
  simplexe régulier), `ρ/D ≤ 1/√3` (q3 équilatéral), `ρ/D = 1/2` (q2).
  Utilisé par le Fait 13 (p. PDF 130) : `α_3 = √6/2`.

### 2.4 Le fuseau de mort `W_q(a,b)`

Définitions et bornes héritées de la v3 (`theoreme_v3`, PROPOSITION § 6bis,
re-déclarées ici comme fondation v4) :

- `W_q(a,b)` = intersection des **intérieurs** de toutes les miniboules
  admissibles d'arité q d'ancre (a,b). Implication utile :
  `z ∈ W_q(a,b)` ⟹ z tue tout support d'arité q d'ancre (a,b). La réciproque
  est **fausse** en q3/q4 (W est une sous-région de la zone de mort réelle
  d'un support donné ; c'est un minorant de témoins, fail-open).
- Formes closes, avec `w = z−a`, `d = b−a`, `H = d·w − ‖w‖²`,
  `Ξ = ‖d×w‖²` : `W_2 : H>0` ; `W_3 : H>0 et 3H²>Ξ` ; `W_4 : H>0 et 2H²>Ξ`.
  Emboîtement `W_4 ⊂ W_3 ⊂ W_2` ; (H,Ξ) ne dépendent pas de l'arité : une
  évaluation sert les trois lanes.
- **q2 est exact** : `W_2(a,b) = B̊(m, ‖ab‖/2)` (la boule diamétrale
  ouverte), et le certificat de mort est une équivalence :
  `(a,b) q2-morte ⟺ r_{h_2}(m) ≤ ‖ab‖/2` (h_2-ième plus proche voisin du
  milieu). Le rang de voisinage d'une arête vivante n'est PAS borné
  (contre-exemple v3 : arête vivante joignant a à son 1001-ième voisin) —
  aucune source kNN de petit préfixe n'est complète (fixture 50 000 points
  gravée, PROPOSITION § 10).
- **Boules de milieu universelles, formes entières** (piste v3, audits du
  12 août) : avec `u = 2z−a−b` et `D² = ‖ab‖²`, les tests témoins i64 sous
  u16 sont `u·u < D²` (q2, rayon D/2), `3(u·u) < D²` (q3, rayon `D/√12`),
  `15(u·u) < 4D²` (q4, rayon `D/√15`) ; en marge `g = D²−u·u` : `g > 0`,
  `3g > 2D²`, `15g > 11D²`. `g = 0` est l'extra-shell q2. Aucun décimal ne
  décide ; seules ces formes entières sont l'autorité. **Rectificatif reçu
  par l'audit du 17 août (ETAT_COURANT § 6)** : `D/√12 = D/(2√3)` est LE
  rayon q3 exact (mes deux « familles » n'en étaient qu'une) ; `D/√15`
  (test `15U < 4L`) est une **sous-approximation sûre mais stricte** du cœur
  q4 exact `D·sin 15°` (car `4/15 < 2−√3` ; test exact : `Y = 2L−U`,
  `Y > 0` et `Y² > 3L²`). Fixture discriminante gravée :
  `a=(10000,10000,0)`, `b=(20000,10000,0)`, `z=(15000,12585,0)` — dans le
  cœur exact q4, hors de `15U < 4L`. Les dérivations de `W_3/W_4`, des
  `κ_q` (atteints, disques de Jung réalisables) et de la borne couplée
  `R_coup` sont désormais **prouvées dans l'audit** — statut relevé de
  piste à énoncé reçu.
- **Cœur-boule** (rayons sûrs autour du milieu, `theoreme_v3` corrigé) :
  la plus grande boule centrée en m incluse dans le fuseau ouvert d'une ancre
  ponctuelle est `B̊(m, κ_q ‖ab‖)` avec `κ_2 = 1/2`, `κ_3 = 1/(2√3)`,
  `κ_4 = sin(15°)`. Pour des extrémités dans deux boules (rayons r_A, r_B,
  centres distants de d) : `R_dec,q = κ_q(d−r)−r/2` et la borne couplée
  `R_coup,q = κ_q d − √((4κ_q²+1)(r_A²+r_B²)/2)` (saturée : tout arrondi vers
  le haut crée de faux crédits ; arithmétique dirigée obligatoire, plancher
  du rayon, comparaisons strictes `<R`).
- **Bornes de bloc exactes.** Sur `Box(A)×Box(B)×Box(Z)` : le minimum de H
  est **séparable par axe** (bilinéaire en (a_i,b_i) : 4 coins par axe ;
  concave en z_i : 2 bouts par axe) — minimum exact sur le produit continu
  des boîtes. Pour Ξ, le produit vectoriel est **affine en a** donc `Ξ` est
  convexe en chaque variable : l'évaluation aux coins décide exactement
  l'enveloppe continue (`corner512`/`corner64`), tandis que le majorant par
  composantes (`xi_max_over_box`) est sûr mais **pas** serré — le claim « le
  plus serré possible » a été retiré par contre-audit v3. Sur les PointId
  réellement stockés, le statut coins reste **suffisant** (ALL), jamais
  nécessaire.
- **Autorité cône pour h_a/h_b** (`theoreme_v3`, vérifié par l'auditeur v3) :
  à extrémité a fixée et partenaire dans `Ball(c,r)`, avec `e = z−a`,
  `J = e·(c−z)`... (formes sans normalisation) : `J > r√E` (q2),
  `√3·J − √Q > 2r√E` (q3), `√2·J − √Q > √3·r√E` (q4), où `E = ‖e‖²`,
  `Q = E‖t_0‖² − J²`, `t_0 = c−z`. C'est une équivalence
  continue-sphère ; les directions d'arrondi sont prescrites (minorer le
  membre gauche, majorer le droit, `ceil_sqrt` exact).

### 2.5 L'élagage : trois comptes disjoints

(`theoreme_v3`, PROPOSITION § 6bis.1–6bis.2, repris comme fondation v4.)

Par rectangle WSPD `A×B` : `h_coeur` (témoins hors A∪B universels sur tout le
rectangle), `h_a(a)` (témoins de A universels sur `{a}×B`), `h_b(b)` (témoins
de B universels sur `A×{b}`).

- **Théorème de disjonction** : les trois ensembles sont deux à deux
  disjoints — automatique, sans hypothèse géométrique (pour z ∈ A, le choix
  a = z donne H = 0 : z n'est jamais témoin universel du rectangle).
- **Corollaire (fail-open)** : `|P ∩ W_q(a,b)| ≥ h_coeur + h_a(a) + h_b(b)` ;
  l'ancre meurt dès que la somme atteint `h_q`. Le filtre ne ferme jamais à
  tort.
- La décision par paire ne touche jamais une paire : histogramme de `h_b` à
  `h_q` cases, coût `O(|A|+|B|)` par rectangle une fois les comptes connus.
- **Masques de lanes obligatoires** : les fuseaux étant emboîtés, un bloc
  crédité pour la lane q et redescendu pour les autres doit masquer q
  (`reste &= ~(1<<q)`) — sans le masque, fausses morts mesurées en régime
  tendu (mutant v3 `dual-sans-masque` : 212 et 1525 fausses morts).
- **Dual-tree h_a/h_b** : dépliage de la diagonale
  `(U,U) → (U_l,U_l),(U_l,U_r),(U_r,U_l),(U_r,U_r)`, suppression de la
  diagonale au seul couple feuille-feuille singleton ; **cutoff** vers le
  ponctuel obligatoire (~256 couples : tout le gain vient de là) ; coins
  **distincts** seulement (une boîte plate a 4 coins, un point 1). La sûreté
  vient du prédicat, pas d'une garde de disjonction.

### 2.6 Qualité du minorant de témoins (la question mathématique ouverte)

Le contrat de l'élagage est que `h_coeur + h_a + h_b` approche
`|P ∩ W_q(a,b)|` d'assez près pour fermer la quasi-totalité des ancres
mortes. Ce qui est établi :

- `mesure` (v3, 36 configurations) : fermeture des ancres q4 de 76,5 % à
  99,3 % selon famille/s/n ; `s = 8` domine `s = 6` partout (jusqu'à un
  facteur 6 sur le résiduel de `terrain` à n = 32 000) ; `s = 10` retire
  encore 9–27 % du résiduel de `s = 8` pour 1,38–1,55× de rectangles ;
  le « mou » (survivantes/vivantes réelles) vaut 1,10 (`uniform`, travail de
  certificat **terminé**), 1,31 (`terrain`), 2,12 (`eight_clusters`,
  52,8 % du résiduel encore retirable à s = 12). Le vrai vivant est un
  invariant du nuage, PAS de la partition — contrôle obligatoire.
- `ouvert` (Q2 pour l'auditeur) : borne théorique sur le mou en régime
  Poisson homogène (calcul de Campbell–Mecke esquissé côté auditeur v3, non
  abouti) ; et le cas `eight_clusters` (paires inter-amas au milieu vide — le
  cœur ne peut rien, seuls h_a/h_b mordent).

---

## 3. La source : WSPD de Callahan–Kosaraju

- **Théorie.** Sur un fair split tree (ou l'arbre radix sur octree comprimé,
  au prix d'une constante bornée — les nœuds de préfixe non multiple de 3 ont
  un rapport d'aspect 2 ou 4, l'argument d'empilement se dégrade d'un facteur
  au plus `8^d`), la WSPD à séparation s produit `O(s³ n)` rectangles en 3D
  et partitionne exactement les paires de positions distinctes.
  `O(n log n + s^d n)` : temps de construction + taille de sortie. Statut :
  théorème externe (Callahan–Kosaraju 1995), constat de conformité v3.
- **Les deux interdits** (post-mortem v3, arbitrage du 16 août 2026) :
  terminal ⟺ séparé (jamais de cap dans le critère : un cap de masse C force
  `#rect ≥ C(n,2)/C²`, quadratique par construction) ; scission du facteur de
  plus grand **diamètre géométrique**, jamais du plus peuplé (−14,7 % de
  rectangles mesuré, et c'est l'invariant de la preuve de packing).
- **Prédicat de séparation entier** : `q²·D2 ≥ (p+2q)²·max(W2_A, W2_B)` avec
  D2 = carré de la distance des centres doublés et W2 = carré du diamètre de
  boîte, implique `d − r_A − r_B ≥ s·max(r_A, r_B)` pour `s = p/q`. Il peut
  manquer une séparation, jamais en inventer (le front grossit, la borne
  tient). i64 sous u16.
- `mesure` (v3) : régime linéaire non atteint aux tailles d'intérêt sauf
  `scanline` (l'exposant local décroît 1,31 → 1,22 → 1,18 jusqu'à n = 64 000,
  `terrain` plafonne à ~1,27) ; 236–887 rectangles/point selon famille et s.
  Toute mesure de pente sous n ≈ 8 000 est fausse (plafond `C(n,2)`).
- **Aveuglement à la sortie** : ~10⁷ rectangles pour 6,6·10⁵ arêtes q2
  vivantes à n = 32 000 (0,13 %, fraction décroissante avec n). La réponse
  v4 est le **critère d'arrêt métrique fusionné dans la descente** : une
  paire de nœuds dont le cœur compte déjà `h_q` témoins universels meurt SANS
  descente (§ 2.5) ; c'est exactement la question laissée en arbitrage à la
  fin de la v3, résolue ici par le corollaire fail-open. La partition des
  paires reste exacte : la masse d'un rectangle tué est comptabilisée morte,
  le ledger total est inchangé.

---

## 4. Identification exacte des événements (au-delà de l'ancre)

Pour chaque ancre survivante (a,b), par arité :

- **q2** : l'événement existe ssi `depth(B(m,‖ab‖/2)) < h_2` ; census
  I_B/U_B par requête de boule. Décision par comparaisons i64.
- **q3** : porteurs x dans `lentille(ab) ∖ boule diamétrale`
  (deux comparaisons i64 + le test `V² > D²` qui subsume l'acuité ET la
  positivité — `theoreme_v3`) ; owner : (a,b) doit rester l'arête maximale de
  {a,b,x} avec départage EdgeKey ; circumcentre par Cramer en i128, census.
- **q4** : *lemme du préfixe ternaire* (`theoreme_v3`, contre-fixture
  gravée) : tout q4 bien centré d'arête maximale ab possède AU MOINS UN
  préfixe abx ou aby strictement aigu, jamais nécessairement deux — le seed
  se cherche dans la lentille privée de la boule diamétrale, le quatrième
  sommet dans la lentille **entière**. *Théorème axial*
  (`theoreme_v3`) : à seed (a,b,x) fixé, le centre parcourt la droite normale
  au plan du seed issue de son circumcentre ; la puissance de chaque site y
  est **affine** ; avec `u = x−a`, `E = u·u`, `F = d·u`, `G = D²E−F²`,
  `T² = D²(G−2(E−F)²)` est la borne de Jung 3D pour un support de diamètre
  ‖ab‖ (un préfixe owner strictement aigu a toujours T² > 0 ⟺ sin²C > 2/3) ;
  pour un préfixe à p intérieurs permanents, tout quatrième point régulier
  peu profond est parmi les `8−p` premières racines entrantes ou les `8−p`
  dernières sortantes : au plus **seize groupes** par seed, avec
  reconstruction exacte de I_B/U_B. Mesuré v3 : 59× moins de propositions.
  Puis positivité par les quatre poids de Cramer sans former le centre.
- **Ce qui reste le poste cher** : le census (test de rang en 3D, par BallKey
  après RLE). Ordre obligatoire : formes positives → BallKey+SupportRecord →
  count/sort/RLE → range-count par clé unique → census complet par clé
  survivante → jonction I_B/U_B (`census_calls = unique_BallKeys`, jamais un
  census par support).

### 4.5 q4 exact, chemin de réception v4 (`derive_v4`, baseline jugée)

La sélection axiale § 4 (seize groupes par seed) est l'accélérateur d'échelle
(`theoreme_v3`, à re-recevoir). La *réception* v4 passe d'abord par la
complétion énumérée dans le cover, jugée par force brute — même philosophie
que le scan site-major reçu avant l'arbre de centres.

**Source (audit du 17 août, `bc5b05d`)** : la lane q4 du front partagé,
JAMAIS le flux q3 (fixture 13 points gravée : ancre q3-morte/q4-vivante avec
tétraèdre de profondeur 0). Rectangles q4 vivants → ancres survivantes par
`h_coeur,4 + h_a,4 + h_b,4 < h_4` (le § 2.5 est indépendant de la lane) →
seeds aigus canoniques → complétions.

**Forme du circumcentre** (Cramer 3×3 relatif). Lignes `M = (2(b−a),
2(x−a), 2(y−a))`, second membre `r = (|b−a|², |x−a|², |y−a|²)` ; alors
`M·(c−a) = r`, `c−a = N'/det` avec `N' = adj(M)·r`. **Canonisation
d'orientation** : si `det < 0`, négation simultanée `(det, N') →
(−det, −N')` — le centre est inchangé, `det > 0` gravé. `det = 0` (quatre
points coplanaires) : jamais un support q4, le candidat est ignoré.

**Puissance affine** (le carré est évité, comme pour la forme de Gram q3) :
avec `dz = z−a`, `P_4(z) = det·|dz|² − 2·N'·dz = det·(|z−c|² − R²)` ; sous
`det > 0` : `P_4 < 0` intérieur strict, `= 0` coquille, `> 0` extérieur.
Largeurs u16 : `|det| < 6·2^54 < 2^57` ; `|N'_i| ≤ 3·2·2^36·(3·2^32) <
2^72` ; `det·|dz|² < 2^57·(3·2^34) < 2^93` ; `2|N'·dz| < 2^92` → **i128**.

**Arité 4 stricte (centre strictement intérieur)** : pour chaque face
`(p,q,r)` de sommet opposé `s`, le signe de `det3(q−p, r−p, N'−det·(p−a))`
doit être NON NUL et égal à celui de `det3(q−p, r−p, s−p)` (l'égalité
`c−p = (N'−det·(p−a))/det` et `det > 0` rendent le test homogène). Un zéro
⟹ le centre est sur une face ⟹ le support retombe à l'arité ≤ 3 : le
candidat q4 est ignoré (il appartient à une autre lane). Largeurs :
`|N'−det·(p−a)| < 2^75` ; `det3(2^17, 2^17, 2^75) < 6·2^109 < 2^112` →
**i128**.

**Owner (6 arêtes)** : `ab` doit être maximale parmi les six longueurs
carrées, tout ex æquo départagé par la plus petite EdgeKey — la
généralisation directe de `anchor_owns_q3` (cinq comparaisons).

**Exact-once du seed — lemme du préfixe ternaire (PROUVÉ, audit du 17 août
« lemme préfixe et niveau », reprend et clôt le statut `theoreme_v3`)** :
tout q4 bien centré d'arête maximale `ab` a AU MOINS UNE face `abv`
strictement aiguë (v ∈ {x,y}), jamais nécessairement deux.
*Preuve (auditeur).* Centre à l'origine : `u = a-c`, `v = b-c`, `p = x-c`,
`q = y-c`, tous de norme `R`. Centre strictement intérieur ⟹ il existe
`alpha, beta, gamma, delta > 0` de somme 1 avec
`alpha·u + beta·v + gamma·p + delta·q = 0`. Si les deux faces sont non
aiguës, l'angle fautif est opposé à `ab` (arête maximale), donc
`(u-p)·(v-p) <= 0` et `(u-q)·(v-q) <= 0`. Avec `s = u+v` et
`tau = R² + u·v = |s|²/2 >= 0`, ces inégalités donnent `p·s >= tau` et
`q·s >= tau`, tandis que `u·s = v·s = tau`. Le produit scalaire de la
relation barycentrique par `s` donne `0 >= tau`, donc `tau = 0`, `s = 0`,
`v = -u` : le centre serait le milieu de `ab`, sur le bord du tétraèdre —
contradiction avec l'intériorité stricte. L'angle opposé à `ab` d'une des
deux faces est donc strictement aigu, et `ab` y étant maximale, la face
entière est strictement aiguë. CQFD.
La source par `AcuteSeed` est donc COMPLÈTE sans aucun héritage q3. Le seed
canonique est le carrier de plus petit PointId parmi les faces incidentes
aiguës du tétraèdre FORMÉ ; une complétion n'émet que si son seed est ce
minimum — vérification en temps constant, sans sort/unique global.

**Cover census à coefficient 4** (audit § 3.4, preuve v4 par Jung) : le
support étant l'arité 4, la circum-boule EST la miniball du tétraèdre, donc
`R ≤ √(3/8)·D` (Jung 3D, diamètre ≤ D car ab est maximale). Le centre est
sur le plan médiateur de `ab` : `|c−m|² = R² − D²/4`. Tout point intérieur
ou de coquille vérifie `|z−m| ≤ R + √(R²−D²/4) ≤ (√(3/8)+√(1/8))·D < D`,
soit `|2z−(a+b)|² ≤ 4·D²` — le même cover rectangulaire paramétré par le
coefficient 4 au lieu de 3 (les SOMMETS restent dans la lentille, coefficient
3). Le paquet `base_4 = h_coeur,4 + h_a,4(a) + h_b,4(b)` initialise la
profondeur : chaque ID certifié est dans `W_4(a,b)`, donc strictement
intérieur à toute circum-boule q4 possédée par l'ancre (même preuve que q3,
fuseaux emboîtés).

**BallKey q4** : la forme développée `det·|z|² − 2(det·a + N')·z +
(det·|a|² + 2N'·a)` donne `(A, B, C) = (det, −2(det·a+N'), det|a|²+2N'·a)`,
`A > 0`, largeurs `A < 2^57`, `|B_i| < 2^74`, `|C| < 2^90` → i128, réduite
pgcd/signe : le MÊME gabarit à cinq coefficients que la `Q3BallKey` (une
boule est une boule).

**Niveau q4 — la largeur dépasse i128** : `R² = |N'|²/det²` avec
`|N'|² < 3·2^144 < 2^146` : le numérateur ne tient PAS en i128 (le carré
q3 restait sous `2^104`). Représentation v4 : `num` en trois mots (U192,
via `mul_level_192`, précondition `< 2^146` respectée), `den = det² <
2^114` en i128. La comparaison croisée q4/q4 demande `2^146 × 2^114 =
2^260` → **cinq mots (U320)** ; q3/q4 mixte < `2^218`. La canonisation
pgcd 192 bits est DIFFÉRÉE (question Q12).

Arithmétique : toutes les décisions en entiers dimensionnés (i64 sous 2^34,
i128 au-delà ; largeurs déclarées par prédicat) ; les rationnels du centre ne
sont jamais gonflés (`A = den, B = −2N, C = 2N·p − den‖p‖²`) ; la positivité
q3/q4 se décide sur les numérateurs barycentriques Gram–Cramer.

---

## 5. Reconstruction de la forêt

Le flux d'événements (support S, profondeur d, niveau ρ, facettes actives
`σ∖{s}`) alimente, pour chaque K = 1..K_max, un Kruskal/Borůvka sur les
facettes hachées (clé = K-uplet trié de PointId) :

- seuls les événements de profondeur `d = K+1−q` servent la forêt K ;
- relier les facettes actives par un chemin suffit (Prop. 6) ; les facettes
  nées au niveau même (retrait d'un intérieur) n'apportent aucune fusion ;
- les égalités de niveau se décident exactement (comparaison de rationnels/
  entiers), l'ordre total est reproductible ; hors position générale → refus.
- oracle : `gamma_forest_judge` indépendant (arithmétique distincte), et
  l'équivalence Théorème 2 (composantes de `L_K(r)` + couverture) comme
  second chemin de vérité sur petits n.

Statut : squelette `theoreme_manuscrit` + conventions v3 ; la spécification
détaillée v4 (clés, lanes, statuts transactionnels) est dans
`ARCHITECTURE.md`.

---

## 6. Questions ouvertes pour l'auditeur

- **Q1 (bijection événements-boules).** Valider § 2.1, y compris aux bords
  (support de cardinal 2 avec points cosphériques → hors position générale →
  refus ; multiplicités de positions).
- **Q2 (qualité du minorant).** Donner une borne, même conditionnelle
  (Poisson homogène), sur le mou `survivantes/vivantes` du préfiltre § 2.5,
  et sur sa dépendance en s. Le calcul Campbell–Mecke esquissé en v3 peut-il
  aboutir ?
- **Q3 (complétude WSPD).** Écrire proprement le théorème de complétude de la
  source : « toute ancre d'événement peu profond est l'arête maximale d'un
  support dont les deux extrémités tombent dans exactement un rectangle
  A×B ; l'instruction de ce rectangle (lentille + seed axial) retrouve le
  support ». La partition CK des paires le donne trivialement — le point à
  auditer est qu'aucune étape d'élagage (§ 2.5) ni d'identification (§ 4) ne
  perd un support (fail-open de bout en bout).
- **Q4 (convention `F_K`).** Pour le rendu § 9.1 : inclut-on les facettes
  non-actives (nées au niveau du simplexe) dans `F_K` pour `S_τ`/`T_x` ? Le
  manuscrit laisse la définition opérationnelle. Proposition v4 : les
  facettes actives seulement, convention à graver et à déclarer.
- **Q5 (ex æquo).** Politique exacte des événements de même niveau
  (cosphéricités u16) : refus global (`unsupported_degeneracy`) ou quotient
  local prouvé ? La v3 a documenté le refus ; la v4 reprend le refus tant
  qu'aucun quotient complet n'est reçu.
- (Q6–Q11 : posées et traitées dans les notes `audits/` — census q3, bord
  torique, oracle indépendant.)
- **Q12 (forme canonique du niveau q4) — TRANCHÉE** (audit du 17 août
  « lemme préfixe et niveau ») : l'option (b) est reçue — représentant NON
  réduit `(|N'|² en U192, det² en i128)`, ordre et plateaux par produits
  croisés U320 (`< 2^260`), le même comparateur recevant q3 (et q2) après
  promotion. CONTRAT INDISPENSABLE gravé : `Q4Level::operator==` n'est
  qu'une égalité de REPRÉSENTATION — deux boules distinctes de même rayon
  portent des couples différents ; la seule égalité sémantique autorisée
  pour les macro-lots est `same_exact_level(x,y) := compare_exact_level ==
  0` ; jamais un groupement par `operator==` ni par hash du couple brut.
  La réduction pgcd 192 bits n'est requise qu'à une éventuelle
  sérialisation canonique des seuls survivants — jamais sur le chemin des
  candidats.
