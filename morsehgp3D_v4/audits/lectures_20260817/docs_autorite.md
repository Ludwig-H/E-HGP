# Lecture des documents d'autorité — dossier de conception V4

Sources lues (état au 2026-08-17, dépôt `/home/user/E-HGP`) :

- `docs/SPECIFICATION_MORSEHGP3D.md` (≈369 Ko, 1526 lignes) — lu en détail : §1–§9.3, §10–§13 (intro), §15–§18.
- `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md` (≈478 Ko, 1263 lignes) — lu en détail : §1–§6, §8 (GPU/perf), §9 (verrous V0–V6, dont V5, V5bis, V5ter, V5quater, V5quinquies, V5nonies, V5decies, V5undecies, V5duodecies, V5octodecies quinquies→decies), §10.
- `docs/TEST_PLAN_MORSEHGP3D.md` (≈640 Ko, 2851 lignes) — lu en détail : §1–§4.6, §7, §9–§12, §14.1–14.6, §18–§20.
- `docs/ROADMAP_IMPLEMENTATION_MORSEHGP3D.md` (≈631 Ko, 2741 lignes) — lu en détail : mission, cap produit, §1–§3, Phase 9 (9.0–9.3, invariants, portes), Phase 14 (optimisations autorisées/interdites, porte de sortie), directives des 6–8 août 2026, sous-portes P15-HOCUDA-P0/P1/P1a.
- `docs/implementation_status.toml` (≈1,08 Mo, 4074 lignes) — structure complète des phases extraite, entrées 0–4 lues intégralement, identités backend/profile/mode relevées.
- `docs/archive/abandoned/README.md` — lu intégralement.
- Complément : `tools/check_scope.py` et règles de `tools/check_docs.py` (pour §6 du présent rapport).

Vu le volume total (~3,2 Mo), les longues sections 8.6–9.26 de la spécification (jalons v3 : ingress fenêtré, ledger sparse, locator historique…) et les jalons 14x/15x du registre des preuves ont été lus par leurs entrées de registre et leurs résumés normatifs plutôt que ligne à ligne ; tout énoncé cité ci-dessous provient d'un passage effectivement lu.

---

## 1. L'objet à calculer (SPECIFICATION_MORSEHGP3D.md)

### 1.1 Règle de tête

> « Ce document fixe l'objet à calculer. Une optimisation n'a le droit de modifier ni cet objet, ni les niveaux, ni les inclusions entre ordres. Toute exécution publie son profil, ses hypothèses satisfaites et son statut de certification. » (bandeau IMPORTANT, §0)

### 1.2 Entrée et sémantique numérique (§2)

- Entrée : $X=\{x_1,\ldots,x_n\}\subset\mathbb{R}^3$ fini, points distincts, ordre maximal $1\leq K_{\max}\leq10$ ; $K_{\mathrm{eff}}=\min(K_{\max},n)$. Toutes les tranches et bornes utilisent $K_{\mathrm{eff}}$, jamais $K_{\max}$ seul.
- Coordonnées IEEE-754 interprétées comme **dyadiques exacts**. Toute décision combinatoire certifiante doit coïncider avec l'arithmétique exacte sur ces dyadiques ; le flottant guide, ne décide pas.
- **Tous les niveaux publics sont des rayons au carré**. $\beta(A)=\min_{y}\max_{x\in A}\Vert y-x\Vert^2$ (miniball), centre $c_A$ unique.
- Formats normatifs : `ExactCenter` $=(C_x,C_y,C_z,D_c)$, $D_c>0$ (homogène — le centre d'un support 3/4 est rationnel non dyadique) ; `ExactLevel` $=(N,D)$, $D>0$, fraction canonique. Comparaison de niveaux par le signe de $N_1D_2-N_2D_1$, jamais par quotient flottant. FP32/FP64/expansions = filtres de signe avec bornes d'erreur ; ambiguïté → fallback big-int/rationnel.

### 1.3 Objet continu (§3) : tour ordre–échelle

$D_k(y)=a_k(y)$ ($k$-ième distance carrée), $L_k(a)=\{y : D_k(y)\leq a\}$ = région couverte par au moins $k$ boules fermées de rayon $\sqrt{a}$. Bifiltration : $a\leq b\Rightarrow L_k(a)\subseteq L_k(b)$ et $k<\ell\Rightarrow L_\ell(a)\subseteq L_k(a)$. **Sortie complète** : le foncteur $\mathcal{H}_X(k,a)=\pi_0(L_k(a))$ pour $1\leq k\leq K_{\mathrm{eff}}$, avec les applications induites par les deux inclusions. Une représentation finie doit conserver **naissances, multifusions, niveaux exacts et applications verticales**.

### 1.4 Modèle discret (§4) : graphe $\Gamma_k(a)$

- Sommets : $F\subseteq X$, $|F|=k$, $\beta(F)\leq a$ ; adjacence : $|F\cup F'|=k+1$ et $\beta(F\cup F')\leq a$ (proposition 5 du manuscrit : les adjacences élémentaires suffisent).
- Théorème 2 du manuscrit : correspondance naturelle composantes de $L_k(a)$ ↔ composantes de $\Gamma_k(a)$. Pour $k\geq2$ les unions de points **se recouvrent** : la sortie n'est **pas** une partition de $X$.
- $\Gamma_k$ est un **oracle conceptuel** ($\binom{n}{k}$ sommets potentiels), jamais une structure à matérialiser. Le cas terminal $k=n$ reste au contrat.

### 1.5 Catalogue critique (§5) — cœur de la conception q2/q3/q4

Pour une sphère $(c,a)$ : $I(c,a)=X\cap B^{\circ}$, $U(c,a)=X\cap\partial B$, rang fermé $s=|I|+|U|$. Reani–Bobrowski : $c$ critique pour $D_k$ ssi $c\in\mathrm{relint}\,\mathrm{conv}(U)$ et $|I|<k\leq s$ ; indice $\mu_k(c)=s-k$ ; multiplicité locale $\Delta_\mu(c)=\binom{|U|-1}{\mu}$. À l'indice 1, le sous-niveau strict local possède les $|U|$ bras $F_u=(I\cup U)\setminus\{u\}$ ; **un seul centre peut tuer jusqu'à $|U|-1$ classes de $H_0$** ($|U|=3$ autorise une triple fusion). L'hyperarête/multifusion est la représentation normative — jamais de binarisation.

- Une sphère de rang $s$ donne : **naissance** à l'ordre $k=s$ (si $1\leq s\leq K_{\mathrm{eff}}$) et **selle** (indice 1) à l'ordre $k=s-1$ (si $2\leq s\leq\min(K_{\mathrm{eff}}+1,n)$).
- Il suffit de cataloguer $s\leq s_{\max}=\min(K_{\mathrm{eff}}+1,n)$. En 3D et position générale, $U$ affinement indépendant, $1\leq|U|\leq4$ → description de taille constante même à $k=10$. Pour $K_{\max}=10$ : $s_{\max}=11$ (si $n\geq11$), profondeur intérieure maximale $m_{\star}=s_{\max}-2=9$.
- L'événement canonique `CriticalEvent` : `event_id, closed_rank (1..s_max), interior_ids (≤9 utiles), shell_ids (≤4), minimal_support_ids, center_witness_homogeneous, squared_level_exact, barycentric_signs, degeneracy_class, predicate_status`. **L'identité canonique = shell complet + intérieur + support minimal + témoins exacts, jamais le centre approché.**

### 1.6 Verdict d'architecture (§1) et énumérateur produit (§5.1)

- Chaîne de vérité bornée ($n\leq14$ ; atlas cellulaire gelé à $n\leq8$) : oracle de supports → catalogue critique → $\Gamma$ exhaustif → hyper-Kruskal gradué. Elle **définit et falsifie**, mais n'est pas le chemin produit.
- Chemin produit ciblé : « Morton/LBVH et Yao48 → paires exactes multi-ordre → triangles aigus → tétraèdres bien centrés → flux Morse sparse ».
- **Interdits durs** : tableau indexé par $\binom{n}{k}$, conservation de toutes les cellules top-$m$, construction de toutes les cofaces de Gamma, persistance d'une mosaïque d'ordre supérieur. États durables autorisés : points + index spatial, frontière de travail explicitement dimensionnée, événements et attaches produits, lots triés, sorties. « Une cellule ou un polytope local est un témoin temporaire évincé après certification. »
- **Chaque sous-arbre de candidats omis doit porter un certificat d'exclusion rejouable.** Frontière non épuisée → `budgeted` sans assertion d'absence.
- §1.1 (directive du 7 août 2026) : **la version industrielle exacte est sans budget configuré**. Trois notions distinctes : budget configuré (interdit en industriel), borne structurelle scellée (à énumérer et justifier une à une), coupe-circuit opérationnel (garde de session, pas un test). Toute mesure produit (50 000 pts, $K_{\max}=10$, p95 `warm_e2e` < 100 ms ; < 1 s secondaire seulement) se mesure **sur la version sans budget et sur elle seule**.
- §5.1 (énumérateur paires) : points indexés `(MortonCode, PointId)`, ancre $j$ pour $i<j$, tuile de $B$ ancres, 48 banques Yao, identité de masse fermée `candidate_pair_mass + certified_pruned_pair_mass + unresolved_pair_mass = n(n-1)/2` ; seule `unresolved_pair_mass=0` autorise la complétude. `requested_order=K` cible les rangs fermés $\leq K+1$ et le certificat de rejet requiert **$K$ témoins distincts en plus des supports** ; « au plus $K_{\mathrm{total}}$ points au total » requiert $K_{\mathrm{total}}-1$ témoins. Le cutoff est **seulement suffisant pour rejeter**.
- Frontière multiplicitaire arités 3–4 : partition canonique en groupes $(N_i,r_i)$, plages Morton disjointes ordonnées, $\sum r_i=m$ ; scission $(N,r)\to(L,a),(D,r-a)$, cardinal $\prod_i\binom{|N_i|}{r_i}$ en **BigInt** (jamais tronquer $\binom{n}{4}$ dans un `size_t`).
- Gram–Cramer (formules exactes, §5.1) : $U=(p_0,\ldots,p_d)$, $e_i=p_i-p_0$, $G_{ij}=e_i\cdot e_j$, $h_i=G_{ii}$, $\Delta=\det(G)$, $M_i=\det(G[i\leftarrow h])$. Si $\Delta>0$ : numérateurs barycentriques $M_i$ ($i\geq1$) et $2\Delta-\sum_iM_i$ ($i=0$) sur dénominateur $2\Delta$. Borne sup $\leq0$ de $\Delta$ ⇒ dépendance universelle ; borne sup $\leq0$ d'un numérateur ⇒ mauvais centrage universel. Polynôme de puissance $P_U(x)=\Delta\Vert x-p_0\Vert^2-\sum_iM_i\,e_i\cdot(x-p_0)$, de signe exactement celui de la puissance à la sphère circonscrite ; borne sup strictement négative sur une plage témoin ⇒ intérieur strict pour **tous** les supports du produit. Seuil de prune de rang : **$s_{\max}-m+1$ témoins distincts** ($m=|U|$).
- Certificat diamétral paires : $\phi(x,u,v)=(x-u)\cdot(x-v)$ ; $M(A,B,C)=\sum_{d=1}^{3}\max(x_d-u_d)(x_d-v_d)$ sur les 8 triplets d'extrémités **par axe** (jamais 8 coins 3D corrélés). $M(A,B,C)<0$ ⇒ tout $C$ strictement intérieur à toutes les boules diamétrales du produit $A\times B$. Certificat de rang = **antichaîne canonique** de plages témoins deux à deux disjointes et disjointes des plages supports ; cardinalité de l'union $\geq s_{\max}-1$ ⇒ rang fermé $\geq s_{\max}+1$ ⇒ produit exclu. **Interdit** : tester seulement des tuples de coins supports (fixtures rationnelles permanentes dans `math/FRONTIERE_DIRECTE_SUPPORTS_3_4.md`).
- Deux portes transversales : le [contrat des paires diamétrales](`math/CATALOGUE_PAIRES_DIAMETRALES_EXACT.md`) restitue le payload fermé complet ; la [frontière supports 3–4](`math/FRONTIERE_DIRECTE_SUPPORTS_3_4.md`) reste **indépendante** : « la complétude des triangles aigus ne découle pas des seules paires de rang utile ».

### 1.7 Équivalence événements ↔ Gabriel (§6) et voie saturée (§6.1)

Sphère critique de rang $s=k+1$, $S=I\cup U$ ⇒ $S$ est un $k$-simplexe de Gabriel ; réciproque sous position générale. Un événement de rang $k+1$ émet : les $k+1$ facettes $S\setminus\{x\}$, une hyperarête de niveau $\beta(S)$, l'union des observations, et (pour `full_pi0`, si $k+1\leq K_{\mathrm{eff}}$) une ancre verticale $k+1\to k$. **L'étoile de $k$ unions remplace la clique** (mêmes composantes à tout seuil).

Voie candidate §6.1 (boules saturées, théorèmes S.1–S.6 de `math/TOUR_BOULES_SATUREES.md`) : $\mathrm{Sat}(Q)=X\cap B_Q$ ; les saturés engendrent exactement le Čech, composantes de Gamma via arêtes $|S\cap T|\geq k$, forêt couvrante de poids maximum (Kruskal décroissant). Voie indépendante, **non backend, non base de preuve v2**.

### 1.8 Profils et sortie (§7, §8, §10, §11, §17)

- `hgp_reduced` : pour $k\geq2$, hiérarchie des K-polyèdres **non réduits à une facette isolée** ; ordre 1 = exception normative : les deux profils coïncident, $n$ feuilles singleton de niveau zéro, $T_1$ = arbre de fusion du single-linkage/EMST (niveaux = distances carrées divisées par quatre). Base exacte v2 **définitionnelle** : Gamma exhaustif sur `reference_cpu` (`proof_basis=gamma_exhaustive_reference`). Comptage par lot : $q$ racines publiques antérieures → $q=0$ naissance, $q=1$ prolongement, $q\geq2$ multifusion ; journal `coverage_delta` obligatoire.
- La proposition 6 et le théorème 5 du manuscrit **ne sont plus une base autorisée** (contre-exemple 5 points, cf. §2.3 infra). Le flot Gabriel brut = `gabriel_positive_connectivity`, `partial_refinement`, jamais `exact`.
- `full_pi0` : toutes les composantes, y compris facettes isolées. **Obligation M.1** (statut `proof_obligation`) : catalogue complet + tous les bras attachés + lots de niveau égal traités simultanément ⇒ la tour de forêts représente $\mathcal{H}_X(k,a)$. Tant que M.1 est ouverte, `full_pi0` ne publie pas `exact`.
- Événements simultanés (§10) : par (ordre, niveau exact) — figer, typer carriers/racines/minima, étoiles sans union immédiate, composantes de l'hypergraphe sur sommets typés $R(r)$/$L(h)$, compter les seules racines réduites antérieures, unir ensuite, journaliser `coverage_delta`, poser les ancres verticales après l'état fermé. Regroupement par intervalles certifiés puis comparateur exact — **une tolérance flottante n'est pas une égalité**.
- Morphismes verticaux (§11) : $L_{k+1}(a)\subseteq L_k(a)$ ; cible via coface $Q$ de cardinal $k+1$ ; carrés de naturalité ordre–échelle = **invariants de correction, pas métadonnées**.
- Domaine exact (§12) : points distincts, shell utile $\leq4$, support affinement indépendant, centre en intérieur relatif à barycentriques non nulles, pas de point extérieur sur la frontière d'une miniball traitée, prédicats exacts. Position générale « pertinente » : $\mathrm{RelevantGP}(X,K_{\mathrm{eff}})$ quantifiée sur les seuls $A$ de taille $2\leq|A|\leq s_{\max}$, à support propre et sans intrus strict. Violation → `unsupported_degeneracy`, **jamais un jitter**. Extensions listées : doublons (multiplicités), $>4$ cosphériques (arrangement sur $S^2$), supports dépendants, plateaux, overflow (repli multiprécision).
- Sortie (§17) : `MorseHGP3DResult` = `forests[1..Keff]`, `vertical_maps[1..Keff-1]`, `equal_level_batches`, `critical_catalog`, `gamma_cofaces`, `gabriel_hyperedges`, `coverage_log` (obligatoire), `optional_condensed_view`, `run_certificate` (avec `catalog_complete_by_rank[1..s_max]`, `attachments_complete_by_order`, `gamma_complete_by_order`, `batches_complete_by_order`, `vertical_maps_complete`, `relevant_gp_complete`, budgets et raison d'arrêt). Pour $k>1$, l'univers horizontal est celui des **facettes de cardinal $k$**, pas les $n$ points : « une sortie qui remplace cet univers par $n$ sommets points et force exactement $n-1$ fusions par ordre est un surrogate de type point-MST, pas une hiérarchie Morse-HGP 3D ». `min_cluster_size=20` (relation `at_least`, testée **après** lot complet, sur la cardinalité de l'union des `PointId`) borne la vue visible, jamais la source.
- Critères de réception (§18), notamment : (2) $k=1$ = arbre de fusion EMST ; (3) tout triplet à $\geq2$ arêtes Delaunay a ses trois facettes-paires connectées dans une même composante de $\Gamma_2$ au plus tard au niveau $\beta(T)$ (une connexion de sommets points ne suffit pas) ; (6) équivariance par permutation ; (7) indépendance à l'ordonnancement GPU des lots égaux ; (8) tout carré ordre–échelle commute ; (9) aucun `exact` si un prune/frontière/attache/`relevant_gp_complete` reste indécis.
- §15 (limites) : une liste $L$-NN fixe n'est jamais complète (paire de Gabriel à boule vide absente de toute liste bornée). Delaunay 3D peut être quadratique ⇒ impossible de garantir simultanément sortie exacte complète, temps quasi linéaire, mémoire bornée et latence < 100 ms (ni < 1 s) pour tout nuage ; le contrat porte sur le **régime volumique sparse** avec publication des tailles intermédiaires. Résultats moyens Poisson = crédibilité, jamais borne déterministe.
- §16 : DTM/entropie = oracles locaux de proposition (classement, préchauffage, mode budgété) ; interdits pour `exact` : exclure une région, conclure une absence, décider une attache, remplacer un lot égal.

---

## 2. Le registre des preuves (STATUT_PREUVES_ET_HEURISTIQUES.md)

### 2.1 Échelle de statut (§1)

`theorem_external` (manuscrit/publié) · `proved_here` (preuve complète dans la doc active) · `conditional_theorem` (preuve sous oracles/hypothèses vérifiés) · `proof_obligation` (énoncé plausible, preuve manquante) · `heuristic` (proposition sans pouvoir d'exclusion) · `experimental_target` (benchmark réfutable) · `false_in_general` (contre-exemple ou obstruction). Statuts d'exécution distincts : `exact`, `conditional`, `budget_exhausted`, `unsupported_degeneracy`, `numeric_failure`, `perturbed`. En pratique le registre emploie aussi : `validated_host_software`, `validated_software`, `validated_real_G4_component` / `validated_real_G4_bounded_component`, `proved_from_program_invariants`, `contractually_forbidden`, `false_under_regular_hypotheses`, `not_proved`, `heuristic_only`, `experimental_component_profile`, `validated_test_protocol_only`, `implemented_and_freshly_certified`.

### 2.2 Entrées décisives pour q2 (paires)

- **`proved_here`** : à $k=1$, le graphe des paires à boule diamétrale fermée exactement réduite à ses extrémités contient tout EMST et préserve toutes les composantes de seuil (sans position générale).
- **Yao48** : les minima exacts par point dans 48 cônes (< 60°) contiennent l'EMST canonique (`proved_here`). Les **trois projections du $K$-ième rayon Yao48** donnent une coupe exacte pour les paires de rang $\leq K+1$ : dans la chambre $x\geq y\geq z\geq0$, $x^2\geq D_K$, $(x+y)^2\geq2D_K$, $(x+y+z)^2\geq3D_K$ ; facteur radial 3 optimal ; une seule borne radiale $r^2\geq3D$ implique les trois cutoffs (`proved_here`). **`false_in_general`** : l'échec des trois comparaisons ne caractérise pas le rang (fixture $p=(0,0,0)$, $q=(2,0,0)$, $w=(1,1,0)$) — le cutoff rejette, ne classe pas.
- **Comptes de témoins** (`proved_here`) : `requested_order=K` ⇒ $K$ témoins ; « $\leq K_{\mathrm{total}}$ points au total » ⇒ $K_{\mathrm{total}}-1$. **`false_in_general`** : « neuf témoins stricts suffisent au prune ancré pour $K=10$ » — une paire de rang fermé exactement 11 a 9 intérieurs ; la fixture P8b exige **dix** témoins et toute égalité est inconclusive. `proved_here` : $s_{\max}-1$ intérieurs stricts concluent `above_rank` pour une paire fournie ; on ne stocke jamais plus de $s_{\max}-2\leq9$ intérieurs.
- **Prune de produit** (`proved_here`) : antichaîne de plages Morton disjointes (des supports et entre elles), $\max\phi<0$, cardinalité $\geq s_{\max}-1$ ⇒ rang fermé $\geq s_{\max}+1$. Une borne **nulle** n'accepte jamais un témoin. **Paire-ancre réelle** (`proved_here`) : un sous-arbre $C$ dont la distance minimale exacte au milieu de l'ancre $\geq$ rayon diamétral est exclu de la recherche de témoins universels (l'égalité shell est excluante ici, contrairement à l'acceptation de témoin qui exige le strict).
- **Couverture de centres doublés** (`proved_here` + `validated_host_software`) : pour boîtes $A,B$, tout centre doublé $s=u+v\in A+B$ vérifie $L(C)\leq\Vert u-v\Vert^2$ avec $L(C)=\max(d^2(A,B),\,d^2(C,2A),\,d^2(C,2B))$ ; cellules $C_j$ couvrant $A+B$ + antichaînes $W_j$ avec $\max\Vert2x-s\Vert^2<L(C_j)$ et $|W_j|\geq s_{\max}-1$ ⇒ produit exclu. **`false_in_general`** : remplacer l'inégalité stricte par l'appartenance aux boules fermées (paire cosphérique : diagnostic extra-shell requis, fixture P7).
- **Demi-espaces stricts** (`proved_here`) : $x\in B(p,q)^{\circ}\Leftrightarrow r\cdot(q-p)>\Vert r\Vert^2$ avec $r=x-p$ ; minimum sur AABB par extrémité par axe ; $m=s_{\max}-1$ témoins ⇒ $s_{\max}+1$ points fermés.
- **Self-produit LBVH** (`proved_here`) : la partition $(A,A)\mapsto(L,L),(L,R),(R,R)$ couvre chaque paire non ordonnée exactement une fois. Parcours self-dual : pile $\leq2H+1$, visites $\leq n(n+1)-1$ par ronde ; **travail sous-quadratique général = `proof_obligation`** ; « le branch-and-bound des paires possède déjà une borne quadratique » est **`false_in_general`** (risque $\Theta(n^3)$ via parcours témoin par paire).
- **Corrélation des coins** (`false_in_general`, garde-fou permanent) : fixture `pair_aabb_corner_correlation` — à $x=0$, $A=\{(2,1),(1,2)\}$, $B=\{(1,-3),(-3,1)\}$ : $\phi$ réels $\{-1,-5,-5,-1\}$ mais coins artificiels $(2,2),(1,1)$ donnent $\phi=4$ ; l'inférence inverse depuis les coins est interdite.
- **`false_in_general`** : une liste top-$L$ fixe comme condition terminale ; une fenêtre Morton bornée comme autorité ($\;$`heuristic_only`) ; un préfixe global de $M=f(k)$ voisins symétrisés (faux dès $k=1$ : deux amas colinéaires de $M+1$ points) ; la politique $M\sim cK\ln n$ comme preuve de complétude ($M=\lceil5k\ln n\rceil$ n'est qu'une heuristique de rappel observée jusqu'à 30 M).
- **Records fermés** (`proved_here`) : un record exact $(u,v,S)$ certifie tous les $Q$ avec $\{u,v\}\subseteq Q\subseteq S$ (même miniball) ; pour un ball-record de support $T$, intérieur $I$, extra-shell $E$ : $Q$ porté est Gabriel ssi $I\subseteq Q$, compte exact $\binom{|E|}{q-|T|-|I|}$.
- **Composant G4 réel** : le pruneur pair par blocs (`ExactBlockRankPruneReceipt`, antichaîne $\leq10$ nœuds à $K=10$) et le compositeur CUDA d'antichaînes sont qualifiés en composants bornés ; à 50 000 points la frontière device ferme `1 249 975 000 = 7 962 604 + 1 242 012 396 + 0` au rang 11 (SHA `cb5f2d3`) ; l'étage paire élague **99,64 % de $C(n,2)$** au rang fermé 6 avec **5,04 visites de nœud par record produit** (`phase15_product_floor_diag_g4_68f656b/full50k.json`) — c'est le précédent que la V4 doit égaler.

### 2.3 Entrées décisives pour q3 (triangles aigus)

- **Stratification** (`proved_here`) : en 3D, les supports critiques utiles se stratifient en **paires, triangles strictement aigus, tétraèdres strictement bien centrés** (3 points : les trois angles strictement aigus ⟺ bon centrage ; 4 points : quatre barycentriques strictement positives).
- **q=3 par intérieurs** (`proved_here`) : à cardinalité $q=3$, les carriers de supports minimaux 2 et 3 se décident par $|I|$ seul : $|T|=2$ : $|I|=0$ émet $|E|$ triangles, $|I|=1$ en émet un, $|I|\geq2$ aucun ; $|T|=3$ : seul $I=\varnothing$ émet $T$ (fixture `gabriel_carrier_strict_interior_extra_shell_matrix.json`).
- **Obstructions à dériver q3 depuis q2** (toutes **`false_in_general`**, fixtures permanentes) :
  1. « un triangle aigu de rang fermé $R$ possède un côté de rang $\leq R$ » — faux dès $R=3$ : `hartigan_triangle_all_side_ranks_above_k.json`, triangle $ABC$ de rang 3 et niveau $800/3$ dont les trois boules diamétrales ont rang 4 ;
  2. « les sous-arêtes des saturations des paires de rang $\leq R$ engendrent les triangles aigus de rang $R$ » — même fixture : 18 paires de rang $\leq3$ sur 36, aucune ne contient $AB$, $AC$ ni $BC$ ;
  3. « une borne universelle sur les rangs des côtés suffit » — calottes ouvertes extérieures à la miniboule : rangs des côtés arbitrairement grands.
  ⇒ **La frontière q3/q4 est indépendante de la frontière q2** ; la V4 ne peut pas dériver la complétude q3 d'un catalogue q2.
- **RNG–Jung** : « un RNG ponctuel d'ordre fini + cascade bornée $\alpha_2$ puis $\alpha_3$ contient tous les supports utiles » — **`false_in_general` y compris au rang fermé 11** (fixture `rng_order_q_rank11_support4_counterexample.json` : tétraèdre régulier bien centré de rang 11, arêtes de support $D^2=8$, mais $\lambda_{11}^2=801/256$ au RNG d'ordre 11 ; les six arêtes absentes). Le graphe **certifiable** est $G_\tau$ : $\Vert p-q\Vert\leq\min(\tau(p),\tau(q))$ avec $\tau(p)\geq2R(p)$ **certifié** ⇒ contient toutes les arêtes de tout support accepté (`proved_here` conditionnel à la majoration fournie ; la construction sparse de $\tau$ reste une **obligation produit**).
- **Arrangement peu profond** (`proved_here`) : pour une paire diamètre $pq$, Jung confine le centre au disque médiateur de rayon $D/\sqrt{8}$ ; chaque tiers point $x$ y définit un demi-plan via l'identité $h_x(v)=r^{2}-\Vert x-(M+v)\Vert^{2}$ (signe = puissance) ; un support 4 générique de rang fermé $\leq s_{\max}$ est un sommet de profondeur $\leq\kappa=s_{\max}-4$ ; un arrangement de $m$ demi-plans a $\leq m(\kappa+1)$ tels sommets ; au rang fermé 11 : $\leq8m$ propositions par ancre au lieu de $\binom{m}{2}$ ; construction $O(m\log m+m(\kappa+1))$ espéré (real-RAM ; clipping, exactitude, dégénérescences à porter).
- **Bornes d'ancres** : `proof_obligation` central — « la source complète des ancres et la somme de leurs voisinages satisfont $a,M=O(n\,\mathrm{poly}(K))$ » ; conditionnellement, cœur shallow quasi linéaire à $K$ fixé ; pire cas $O(n^{3}\log n)$ à $K$ fixé, $O(n^{4})$ si $K=\Theta(n)$.
- Oracle borné : `ExactQ3GabrielTriangleOracle` limité à $n\leq14$, jamais une source $\Gamma_2$ (les cofaces non Gabriel des incidences silencieuses n'y sont pas). Prune local q3 : pour un triangle Gabriel q=3 à support minimal **paire**, deux témoins strictement intérieurs certifiés suffisent (`strict_interior_threshold`) ; un témoin à borne inférieure nulle est sur le shell et **ne compte pas** ; l'implication ne dit rien d'un triangle à support 3 contenant la même arête ; obligation `gamma2_silent_handoff_required` (le prune ne tue jamais la paire dans $\Gamma_2$).

### 2.4 Entrées décisives pour q4 (tétraèdres bien centrés)

- **`false_in_general` dans les deux directions** : « quatre faces aiguës caractérisent les tétraèdres de support minimal quatre » — `tetrahedron_face_filter_counterexamples.json` : tétraèdre bien centré de barycentriques $(1/8,3/8,3/8,1/8)$ avec **deux faces obtuses** ; et tétraèdre à quatre faces aiguës de barycentrique $-1/12$ (miniboule portée par une face). ⇒ La V4 ne peut pas filtrer q4 par l'aiguïté des faces.
- Intervalles Gram–Cramer (`proved_here`) : prunes de bon centrage et de rang des supports 3–4 ; backend `int1024` P6a : coordonnées $\leq124$ bits après alignement dyadique ; directions/Gram/déterminants/numérateurs/puissances $\leq125/252/759/762/1013$ bits ; hors enveloppe → rationnel arbitraire.
- **Décision cellule–sphères** (V5octodecies octies, `proved_here` + `validated_host_software`) : pour $\mathcal{U}$ produit de 3–4 boîtes supports (tuples affinement indépendants) et $Q$ cellule AABB dyadique, bornes rationnelles exactes $\overline{\pi}(Q,\mathcal{U})<0\Rightarrow$ cellule incluse dans l'intérieur de toutes les sphères ; $\underline{\pi}(Q,\mathcal{U})\geq0\Rightarrow$ aucun témoin strict dans la cellule ; zéro dans l'intervalle ⇒ `inconclusive`.
- **Sonde Morton locale** (V5octodecies nonies) : pour $m\in\{3,4\}$, témoins requis $H=s_{\max}-m+1$, $1\leq H\leq9$ ; $\leq(2\cdot4+2)H+1\leq91$ positions proposées ; ancêtres dédupliqués en antichaîne ; masse $\leq H$ ⇒ $\leq2H-1$ nœuds. **Morton = politique de proposition, jamais autorité.** Le profil pré-sonde falsifie la recherche globale depuis la racine : à $n=32$, $K=5,10$, 4 859–4 863 des 5 000 unités absorbées par le rang, 53–62 supports résolus sur 40 920 (`validated_negative_performance_evidence`).
- **Center-cover P1** (roadmap, sous-porte `P15-HOCUDA-P1`) : pour blocs d'extrémités $A,B$ de séparation coordonnée maximale $H$ : $D^{2}\leq3H^{2}$ ; Jung ⇒ $\Vert c-(p+q)/2\Vert^{2}\leq D^{2}/8$ donc $|c_i-(p_i+q_i)/2|<5H/8$ (car $3/8<25/64$) ; boîte $C_{AB}=M_{AB}\oplus[-5H/8,5H/8]^{3}$ (arrondis extérieurs), partagée en $4\times4\times4=64$ patches éphémères. **8 témoins stricts certifiés par patch faisable + 4 points de shell ⇒ rang fermé $\geq12$** ⇒ bloc impossible au rang fermé 11 (support 4). Le support 3 emploie **9 témoins** et la borne de déplacement $H/2$. Un futur carrier n'a pas à être exclu a priori des témoins : au centre réel, son égalité de shell contredirait la stricte positivité. Identité committée en permanence : $P_{\mathrm{prune}}+P_{\mathrm{emit}}+P_{\mathrm{pending}}=\binom{n}{2}$.

### 2.5 Attaches, réduction, verticalité (résumé des entrées utiles)

- **V0 (contre-exemple)** : `gabriel_point_set_counterexample.json` — 5 points génériques, ordre 2, niveau fermé $83886/3563$ : Gamma a l'unique union $(0,1,2,3,4)$, le flot Gabriel élagué a $(0,1,2)$ et $(0,2,3,4)$ ; la facette $(0,2)$ est attachée dès $33/2$ par deux cofaces **non-Gabriel** (incidence silencieuse) ; le flot élagué retarde la fusion jusqu'à 24. C'est ce qui a réfuté Proposition 6/Théorème 5 du manuscrit comme base exacte.
- `proved_here` : une coface non-Gabriel stricte ne fait qu'une attache silencieuse $q=1$ (support essentiel unique) ; Gabriel **complété par toutes les attaches silencieuses** reconstruit Gamma (induction par lots) ; toute croissance silencieuse $q=1$ reste dans `coverage_log` (fixture `gamma_q1_coverage_delta.json`).
- **`false_in_general`** : « les seules unions de points forment un invariant inductif suffisant pour élaguer les cofaces non-Gabriel ».
- **Étoile à une étape incomplète** (`false_in_general`) : fixture `moment_curve_k3_one_step_star_incompleteness_n9` ($p_i=(10000i,i^2,i^3)$) : la facette `048` (support essentiel `08`, niveau $1600066560$) possède six cofaces silencieuses `0481…0487` dont aucune n'est visible depuis une suppression directe ; une fermeture récursive recréerait Gamma global (interdit) — il faut une **frontière demand-driven avec certificats de terminaison/exclusion, ou un générateur saturé implicite**.
- **O5** (`proved_here`, local) : dans un lot fourni, $D=\sum_C\max(q_C-1,0)=|R_{\mathrm{touch}}|-N_{+}\leq\sum_e(|U_e|-1)=\sum_e\Delta_1(e)$ ; total canonique par lot, pas d'attribution par événement. M.1 global reste `proof_obligation` (verrou V1).
- Rétraction conditionnelle $H_0$ (`conditional_theorem`) : flot direct + $\mathrm{Star}(D_k)$ + descentes strictes + quotient atomique préservent $H_0$ horizontal **sous régularité globale** ; mais publier `hgp_reduced` exact v2 sans cofaces silencieuses est **`contractually_forbidden`** (les `batch_id` engagent les `gamma_coface_ids` exhaustifs).
- Descente K-NN–miniball : préclassification fail-closed (`already_active_at_own_center` / `strict_descent_admissible` / `unsupported_degeneracy`) ; chaîne finie $L\leq\binom{n}{k}-1$ ; germe initial d'un bras via $\tau_u=\min(\{1\}\cup\{A_p/(-2B_p): p\in X\setminus S,\ B_p<0\})$ (identifie exactement $F_u$ sur $(0,\tau_u]$).
- Borůvka canonique $\kappa(e)=(d^2(e),u,v)$, sur-ensemble à graine fixe, sur-ensembles recertifiés : `proved_here` — mais **hors chemin produit** (oracle $k=1$).
- V5ter : la restriction $\Gamma_2$ aux wedges Delaunay à deux arêtes est **réfutée** (fixture 6 points `delaunay_two_edge_gamma2_counterexample.json`, deux cofaces de niveau $281/4$ manquantes) ; les fermetures étoile/clique-du-carré/fan sont fausses (fixture 8 points, facette `01` au niveau $13\,956\,479\,554$ absente) ; la famille « au moins une arête Delaunay » est suffisante sous position générale (lemme radial, `DELAUNAY_ORDINAIRE_GAMMA2.md`) mais engendre $19\,256\,829\,696$ occurrences à 50 k — certificat conceptuel, pas architecture.
- Performance (§8) : `experimental_target_primary` = 50 000 points, p95 `warm_e2e` **< 100 ms** ; `experimental_target_secondary` = < 1 s (même protocole, ne ferme pas la porte principale). « Un million de points exacts tient en VRAM » : `false_in_general`. Diffusion frontières/événements/attaches : `conditional_theorem` (remplacement atomique, merge exact, trois frontières vides pour la complétude).
- Règles de publication (§10) : `exact` exige la publication du profil, hypothèses, complétudes par rang/ordre, statut des lots et verticales, `relevant_gp_complete`, sémantique de forêt, fallbacks, budgets, tailles intermédiaires, pic mémoire, versions. « Un bon ARI, une stabilité sous bruit ou un accord moyen ne remplace aucune de ces preuves. »

---

## 3. Le plan de tests (TEST_PLAN_MORSEHGP3D.md)

### 3.1 Tailles de nuage (§3.1) — normatif

- **Les tailles qui comptent : $n=8\,000$, $16\,000$, $32\,000$.** Toute conclusion sur coût, sélectivité, mémoire, échelle s'y mesure ; quelques centaines de points ne les remplacent pas.
- Deux rôles exclusifs : **oracle** ($n\leq400$ ; $n\leq12$–$14$ pour l'énumération de catalogue) = correction exacte, identités, mutants tués — jamais une pente ; **échelle** ($8\,000/16\,000/32\,000$) = coût, sélectivité, mémoire, pentes — jamais la correction.
- Conséquences chiffrées : un juge $O(n^3)$ vaut $5\cdot10^{11}$ opérations à $n=8\,000$ (**hors de portée par construction**) ; une matrice $n\times n$ vaut $10^{9}$ octets à $n=32\,000$ (**interdite**). À l'échelle : **invariants globaux** (masse $\binom{n}{2}$ exacte, aucune paire sans décision, aucun dépassement de cap, aucun point de shell perdu) + **juge d'échantillon** ($K$ paires déterministes, $O(Kn)$). Compteurs agrégés + échantillon suivi, jamais un tableau de décisions.
- Une campagne qui n'atteint pas $8\,000$ doit le déclarer par un **code de sortie**, pas un commentaire.

### 3.2 Pas de vérification exhaustive (§3.2)

- « Aucune porte ne vérifie exhaustivement ce qu'un théorème garantit déjà. » Ce qui est démontré est *invoqué*. Remplacement, par ordre de force : (1) **le théorème et ses fixtures d'égalité** (les cas où le strict se sépare du large) ; (2) **invariants globaux** en mémoire constante ; (3) **juge d'échantillon** exact, $K$ tirages déterministes, $O(Kn)$ ; (4) **mutants** (« la seule mesure de ce qu'une porte voit réellement »).
- **Exception explicite** : les oracles exhaustifs bornés **T2** ($n\leq12$, étendu $n\leq14$) sont conservés — « ils ne vérifient pas un théorème, ils **sont** la vérité terrain » ; distinction entre *établir* et *re-vérifier*.

### 3.3 Niveaux T0–T6 et oracle (§3, §4)

- T0 schémas/déterminisme ; T1 prédicats (arithmétique exacte indépendante) ; T2 catalogue/hyperarêtes (oracle exhaustif $n\leq12$→$14$, PR puis nocturne) ; T3 forêts/couvertures/flèches (graphe exhaustif à chaque seuil) ; T4 différentiels/métamorphiques CPU/GPU (nocturne G4) ; T5 dégénérescences/adversarial ; T6 performance/streaming/reprise Spot.
- Oracle indépendant (§4.1) : **aucune réutilisation** des noyaux du backend, décisions rationnelles/multiprécision ; « une comparaison CPU/GPU n'est pas un oracle indépendant si les deux chemins utilisent le même prédicat fautif ». Énumération §4.2 : tous les $U$ de taille 1–4, centre exact dans $\mathrm{aff}(U)$, signes barycentriques, classification complète de $X$, dédup par `(interior_ids, boundary_ids, level_exact)`.
- §4.3 (flux direct) : fixtures obligatoires — produit entièrement prunable, borne témoin **nulle jamais acceptée**, égalité shell exclue par paire-ancre, plages supports superposées, **paire longue absente des listes locales**, prune faux après mutation d'une borne, exécution sans prune retombant sur toutes les feuilles ; certificats à témoin répété / ancêtre+descendant / recouvrement de plage support **rejetés** ; égalité à l'énumération exhaustive pour chaque $n\leq14$ indépendamment du chunking.
- §4.5 : comparaison à **trois états par valeur critique** (strictement sous $a$, fermé $a$, strictement au-dessus) sur partitions de labels, unions de points, parents, flèches verticales. §4.6 : toute graine fautive devient **fixture permanente avant correction** ; minimisation automatique des contre-exemples.

### 3.4 Invariants, générateurs, différentiels (§7, §9–§11)

- Invariants horizontaux (§7.1, 9 items) : niveaux non décroissants enfant→parent ; acyclicité ; coïncidence avec le graphe exhaustif à chaque seuil ; fusion de lot = composantes incidentes juste sous le niveau ; croissance $q=1$ conserve la racine + `coverage_delta` exact ; aucune scission quand $t$ croît ; comptabilité naissances–fusions ; union de points = union des facettes/enfants ; **recouvrements d'ordre $k\geq2$ préservés, non résolus en partition**.
- Invariants verticaux (§7.2) : existence/unicité de cible, inclusion de couverture, cohérence avec l'ancre de rang, pas de cible à niveau strictement supérieur, et le carré $v^k_{t'}\circ h^{k+1}_{t,t'}=h^k_{t,t'}\circ v^k_t$ à tous les couples de seuils consécutifs.
- §7.3 : cohérence des préfixes en ordre — $K_{\max}=10$ restreint = exécutions indépendantes $K_{\max}=1..9$.
- Générateurs (§9) : familles volumiques (uniforme, Poisson inhomogène à rapport d'intensité jusqu'à 100, mélanges gaussiens 1–64 amas, non gaussiens, bruit 0/1/5/20 %, Thomas/Neyman–Scott) ; familles de dimension intrinsèque faible (courbes, plans, tores, tubes, **scènes LiDAR synthétiques avec sol/façades/objets minces/occlusions**) ; familles adversariales (courbe des moments, cosphères, grille exacte, duplications ×2–1000, quasi-coplanarité $2^{-p}$, offset $10^{12}$ avec écarts $10^{-6}$, échelles $10^{-12}$–$10^{12}$, contre-exemple aux listes L-NN). Chaque générateur : version, graine, paramètres sérialisés, hash du nuage.
- Matrice des tailles (§10) : micro 4–14 ; petite $10^3$, $3\times10^3$ ; moyenne $10^4$, $3\times10^4$, $5\times10^4$, $10^5$ ; grande $10^6$, $3\times10^6$ ; extrême $10^7$. Tous les ordres 1–10 demandés en une exécution dès $n\geq10$.
- Métamorphiques exactes (§11.2) : permutation des points, permutation signée des axes, translation dyadique, homothétie $2^q$ (niveaux ×$2^{2q}$), permutation des lots/blocs CUDA, variation du budget de streaming (si les deux `exact`), restriction de $K_{\max}$ (préfixe identique), changement de pivot d'hyperarête, changement d'étages de filtre. Déterminisme (§11.4) : sorties canonicalisées **bit à bit identiques** sous 20 ordonnancements de blocs.
- Dégénérescences (§12) : jitter/tolérance interdits en mode certifié ; passage `exact` → plateau/`unsupported_degeneracy` **exactement au zéro certifié**.

### 3.5 Performance G4 (§14) et portes (§19–§20)

- Trois chronométrages : `cold_e2e`, `warm_e2e` (inclut validation, H2D, **construction de l'index**, calcul, D2H), `resident_core` (diagnostique).
- Objectifs réfutables (§14.4) : $1\,000$ pts $\leq25$ ms ; $10\,000\leq200$ ms ; **$50\,000$ : $p95<100$ ms strict (principal), $<1$ s secondaire** ; $100\,000\leq3$ s ; $10^6\leq60$ s ; $10^7\leq600$ s (streaming autorisé). Porte structurelle **avant** tout chrono : zéro allocation indexée par les univers de facettes/cofaces, zéro cellule top-$m$ persistée, zéro Gamma dans le target. Smoke 12 500/25 000/50 000 : deux exposants successifs $>1{,}35$ sur boîtes/frontière/feuilles ⇒ suspension des micro-optimisations. 30 répétitions à 50 k ; p50/p95/max/MAD + valeurs brutes ; moyenne seule interdite.
- Régimes (§14.5) : les objectifs ne sont évalués que sur (1) Poisson uniforme volumique et (2) mélange équilibré de 8 amas ; les familles 32 amas/surface/pont/adversarial mesurent la dégradation honnête.
- Portes (§19) : G1 prédicats, G2 catalogue ($n=14$), G3 hiérarchie, G4 verticalité, G5 GPU — **absolues** ; G6 (50k interactif : $p95<100$ ms sur les deux familles favorables, le seuil 1 s aussi publié), G7 (million), G7b (3 M, trois graines `exact` + streaming transactionnel + reprise) — décisions de produit. G9 : VM `TERMINATED` certifiée.
- Jalons (§20) : `v1-correctness` (dont $k=1$ = EMST, mesure G4 reproductible à 50 k) ; `v1-interactive-scalable` exige v1-correctness + Phase 12 + **M.1 fermées** + migration contractuelle versionnée + G6/G7/G7b + une campagne 10 M sans corruption.
- GCP (§18) : garde de durée (`maxRunDuration`), checkpoints atomiques (6 points d'interruption), préemption simulée, condition terminale `TERMINATED` obligatoire sur exactement la cible.

---

## 4. Le registre `implementation_status.toml` et la place d'une V4

### 4.1 État des phases (au 2026-08-08, `current_phase = "15"`, `current_track = "performance"`)

| id | titre | statut |
|---|---|---|
| 0 | Correction v2 de l'énoncé et des schémas | completed |
| 1 | Oracle CPU exhaustif | completed |
| 2A | Prédicats exacts CPU | completed |
| 3 | Environnement CUDA G4 reproductible | completed |
| 2B | Prédicats exacts GPU | completed |
| 4 | Canonisation et oracle spatial | completed |
| 5 | Ancre $k=1$ et EMST | **ready** |
| 6 | Miniballs et descentes | **ready** |
| 7 | Audit de la primitive de puissance | completed |
| 8 | Raffinement ancré initial | **ready** |
| 9 | Flux direct de supports H0 sans mosaïque | completed |
| 10 | Journal Morse et réduction directe | completed |
| 11 | Tour verticale réduite | completed |
| 12 | Preuve et implémentation `full_pi0` générique | **blocked** |
| 13 | Dégénérescences et multiplicités | **blocked** |
| 14 | Latence 50 000 points | **ready** |
| 15 | Streaming à dix millions et davantage | **in_progress** (`reference_cpu / hgp_reduced / budgeted`, `architecture_only`, `public_status=not_claimed`) |
| 16 | Campagne à plusieurs millions | blocked |
| 17 | Tour de boules saturées sensible à H0 | ready |
| 18 | Durcissement et releases | blocked |

Jalons : `v1_correctness` **blocked** (exige phases 0–5, 7, 9–11, 18 et portes G0–G5, G9) ; `v1_interactive_scalable` **blocked** (exige en plus 12, 14, 15, 16, 18, G6/G7/G7b, **M.1**, la migration contractuelle `versioned_direct_morse_exact_proof_basis_activation` et la preuve `three_sparse_3m_exact_runs`).

Conventions du registre : `allowed_status = ["blocked","ready","in_progress","completed"]` ; `completion_requires = ["entry gate satisfied","exit gate evaluated","tests recorded","artifacts recorded","evidence commit recorded"]`. `tools/check_implementation_status.py` valide le TOML **contre les en-têtes `## Phase N` de la roadmap** (les ids doivent correspondre).

### 4.2 Ce qu'implique une V4 dans un nouveau dossier

- Le registre ne connaît **que** la ligne produit `morsehgp3d/` et ses phases 0–18. `morsehgp3D_v3/` n'y apparaît pas : c'est le statut `exploration_v3_hors_registre`, « aucun statut public » (CLAUDE.md). Une V4 dans un nouveau dossier serait dans la même situation : **hors registre**, `public_status=not_claimed`, sans droit d'ouvrir/fermer une phase du TOML ni de toucher un statut public.
- Conséquences concrètes : (a) aucune conclusion V4 ne promeut `public_status=exact` — seuls certificats et oracles prévus le peuvent (règle absolue CLAUDE.md/AGENTS.md) ; (b) si la V4 devait un jour entrer dans la ligne produit, cela passerait par les phases du registre, leurs portes d'entrée, la mise à jour du TOML **dans le même commit** et `python tools/check_implementation_status.py` ; (c) tout ajout d'une « Phase » nouvelle exigerait un en-tête correspondant dans la roadmap (le checker les apparie) ; (d) une contradiction mathématique découverte pendant la V4 doit devenir **fixture minimale permanente** et mettre à jour `STATUT_PREUVES_ET_HEURISTIQUES.md` avant de continuer ; (e) les axes de statut restent obligatoires dans toute annonce : `backend` ∈ {reference_cpu, cuda, cuda_g4}, `profile` ∈ {hgp_reduced, full_pi0, generic_core}, `mode` ∈ {certified, budgeted, benchmark_only}, `public_status` ∈ {exact, conditional, budget_exhausted, unsupported_degeneracy, numeric_failure} — un mode `budgeted` n'obtient jamais `exact`.
- La cycle documentaire v3 (NOTE_SOLUTION → prototype+portes CMake → AUDIT_LIVE/RECEPTION ancrés au hash → REQUALIFICATION ; portes négatives à codes exacts 1/2/3/4 via `mhgp3v_add_expected_code_test_for` ; planchers `--min-*` contre le vert-par-vacuité ; mutants `--inject`/`--force-*` ; équivariance ; oracle à arithmétique réécrite + selftest `mhgp3v_arith_selftest`) n'est pas imposé par le registre mais constitue le précédent méthodologique attendu pour une exploration V4.

---

## 5. Pistes abandonnées (docs/archive/abandoned/README.md)

« Abandonnée » = interdite comme **architecture ou mécanisme de complétude** du produit ; une identité locale, une fixture ou un oracle borné issu de la piste peut rester utile.

| piste | raison | trace |
|---|---|---|
| Fenêtre Morton fixe / préfixe fini de voisins comme autorité exhaustive | aucun rayon universel ne garantit toutes les paires/simplexes utiles | contre-exemples du registre ; Morton reste un ordre de stockage/parcours |
| Forêt de MST de mutual reachability sur les points comme hiérarchie Morse exacte | dès l'ordre 2, oublie l'identité des simplexes/facettes/cofaces/incidences | instantané point-MST v6 (`morsehgp3d/archive/surrogates/point_mst_v6/`) retiré du build ; fixtures falsificatrices conservées |
| Fermeture par les seules paires de rang utile | un triangle aigu de rang 3 peut avoir ses trois côtés de rang 4 | fixture `hartigan_triangle_all_side_ranks_above_k.json` + checker exact |
| Wedges Delaunay, étoile fermée, carré du graphe, fan de faces pour $\Gamma_2$ | fixtures exactes 6 et 8 points ; la correction `one_edge` explose avant déduplication | `PHASE15_DELAUNAY_GAMMA2_FALSIFICATION.md`, `math/DELAUNAY_ORDINAIRE_GAMMA2.md` |
| PDEL/Geogram et flot Gabriel comme chemin produit ou correction | dépendance à une triangulation, décisions binary64 partielles, incidences silencieuses, arènes massives | cibles retirées du build ; diagnostics conservés hors ligne |
| Subdivision `prune-only` pilotée par l'hôte | 99,634 % de paires prunées à 3 125 pts mais 3,229 s à chaud (vagues, revisites, synchros) | `PHASE15_PRUNE_ONLY_FRONTIER_G4.md` |
| Parcours stackless relancé par paire/ancre/callback | coût répété, trafic hôte incompatible 50 k ; frontière résidente commune requise | artefacts `phase15_pair_rank_*` |
| Mosaïque d'ordre supérieur, Gamma global, catalogue cellulaire comme produit | matérialise ce que MorseHGP3D doit éviter | oracles bornés de `reference/` seulement |
| Tour globale de boules saturées énumérée exhaustivement | univers de supports/memberships incompatibles avec l'échelle (jusqu'à $O(n^4)$ supports, $O(n^5)$ memberships) | repli de preuve borné, `research/` |
| Grille, DTM, lissage entropique, ANN comme définition exacte | modifie la filtration ou ne certifie pas la complétude | historique |

**Règle de réouverture** (verbatim) : « Une piste archivée ne peut revenir dans la voie active qu'avec un **nouveau théorème de complétude**, une **fixture qui falsifie le motif d'abandon** sans casser les contre-exemples existants, une **architecture sans structure globale interdite** et un **gate de performance distinct**. Un benchmark moyen ou un bon rappel empirique ne suffit pas. »

À noter aussi (roadmap, 7–8 août 2026) : la frontière tuilée higher 3–4 actuelle est **rouverte comme verrou au niveau échelle** — ~0,02 % de $C(512,3)+C(512,4)$ résolus par 150 s, taux **invariant sous le partitionnement**, débit ~$2\cdot10^3$–$4\cdot10^3$ supports/s identique à $n=32$ et $n=512$ ; « ce qui doit baisser est le **nombre de produits examinés** » ; porte préalable : **recensement de la sortie utile** (combien de supports 3–4 minimaux bien centrés de rang fermé $\leq K$ existent réellement). Et la mesure diagnostique du 8 août : frontière paire CUDA 2,395883 s (`frontier_ns`) / 3,927585 s à froid à 50 k, ~15,9 M objets, 40 lancements, 66 synchronisations — « ces mesures ferment l'espoir du chemin courant sous la seconde ». Budget indicatif 50 k dans P1 : primaire 100 ms = 25 (transfert+LBVH) + 15 (source+center-cover) + 10 (cordes) + 20 (shallow+exact) + 20 (reducer) + 10 (réserve) ; secondaire 1 s = 40/200/200/300/200/60 ms.

---

## 6. Contraintes transverses pour la V4

### 6.1 Équations et documentation (`tools/check_docs.py`)

- Une équation Markdown = **une seule ligne physique** ; sur une ligne, le nombre de `$$` vaut 0 ou 2.
- Accolades explicites obligatoires après `\mathbb`, `\mathbf`, `\frac`, `\sqrt` (regex `EXPLICIT_BRACES`).
- Interdits : `\operatorname`, `\left\|`, `\left\{` (utiliser `\left\Vert`, `\left\lbrace`) ; les tokens amputés `qquad`, `pi_0`, `mathrm` sans backslash sont rejetés.
- Vérification : `python tools/check_docs.py` (câblé en CI), avec `check_contracts`, `check_references`, `check_scope`, `check_gcp_workflows`, `check_oracle_independence`, `check_implementation_status`.

### 6.2 Noms bannis (`tools/check_scope.py`)

- Motifs bannis dans toute la documentation active (hors `docs/HISTORIQUE.md`) : **`Perg-HGP`** (regex `\bperg(?:_|-)hgp\b`, insensible à la casse), **`PowerCover3D`**, **`HomogeneousLensTower`**.
- Chemins retirés qui doivent rester physiquement absents : `perg_hgp/`, `experiments/powercover3d/`, `gpu_geogram_low_order_diagnostic.cu`, `gpu_morton_window_h0_surrogate.cpp`, `binary64_lbvh_top_k.{hpp,cpp,cu}` (+ interne phase14), `gpu_guarded_industrial_e2e.cpp`, `direct_morse_unified_resident_vertical_stream_bridge.{hpp,cpp}`, `direct_normalized_h0_retraction_authority.{hpp,cpp}` (+ son test).
- Identifiants bannis des fichiers de build produit (`ci.yml`, `morsehgp3d/CMakeLists.txt`, presets, tests de configuration, campagne de profilage) : `MORSEHGP3D_ENABLE_GEOGRAM_LOW_ORDER_DIAGNOSTIC`, `morsehgp3d_gpu_geogram_low_order_diagnostic`, `morsehgp3d_gpu_morton_window_h0_surrogate`, `morsehgp3d_gpu_binary64_lbvh_top_k`, `morsehgp3d_gpu_guarded_industrial_e2e`.
- ⇒ Une V4 ne doit ni réutiliser ces noms, ni réintroduire ces chemins/cibles, y compris dans sa documentation.

### 6.3 Licences

- MIT (racine) sur le code actif et la documentation, **sauf** : `HGP-old/` (licence historique **non commerciale**, jamais importé — la V4 ne peut pas en copier du code) ; PDF de `docs/references/` (conditions fichier par fichier dans `references.toml`, intégrité par `python tools/check_references.py`) ; poids pré-entraînés lignée Pointcept (Sonata, Concerto, Utonia) en **CC-BY-NC 4.0**, cantonnés à `tests/SemanticKITTI/`, jamais dans la ligne produit.

### 6.4 Git, GCP, cycle de phase (CLAUDE.md / AGENTS.md, normatif)

- Jamais de branche Git sans accord explicite ; commits sur `main`.
- Jamais de VM GCP hors des scripts gardés de `gcp-migration/` (`start_and_verify.sh`/`stop_and_verify.sh`, `g4-standard-48` SPOT, label `project=e-hgp`, double coupe-circuit, `maxRunDuration` 30 s–8 h) ; certifier `TERMINATED` après toute session ; aucune commande GCP mutante hors usage.
- Ouverture/fermeture de phase : TOML **dans le même commit** + `python tools/check_implementation_status.py`.
- Toute contradiction mathématique → fixture minimale permanente + mise à jour du registre des preuves **avant** de continuer.
- Discipline de tests (directive du 6 août 2026, roadmap) : « Pas de tests smoke. Plus de caps de budget artificiels dans les validations. […] la vérification exhaustive exacte se fait sur de tout petits nuages ($n=32$ suffit pour les supports 3–4), puis directement **50 000 points**, puis **dizaines de millions si raisonnable** — rien entre, rien au-delà de l'utile. » (À réconcilier avec §3.1 du plan de tests, cf. Questions ouvertes.)
- Rôle strict de Delaunay (6 août 2026) : **garde-fou de vérification externe uniquement** — $k=1$ : l'EMST doit être retrouvé ; $k=2$ : tout triangle de deux arêtes Delaunay doit relier des clusters d'arêtes à un niveau conforme ; « Delaunay ne doit jamais apparaître dans l'algorithme Morse HGP 3D lui-même ».
- Optimisations autorisées (Phase 14) : fusion de kernels sans fusionner proposition et certification, CUDA Graphs, classes d'unités de frontière, double buffering, culling directionnel, cache de violateurs, radix sort vérifié, chevauchement GPU/CPU, réduction de copies. Interdites : taille fixe de voisinage sans fermeture, epsilon d'égalité, limite de faces tronquée, suppression d'un fallback, conditionnel étiqueté exact, exclusion des cas lents du p95 sans règle préenregistrée.

---

## 7. Correspondances et écarts entre la feuille de route V4 et les autorités

(Analyse de conception, pour cadrer ; les faits cités viennent des sections précédentes.)

1. **« Recherche de l'arête maximale »** : appuyée par les autorités — pour tout support accepté, le circumball est témoin dans $R(p)$ de chacun de ses sommets, donc le diamètre du support est $\leq2R(p)$ ($G_\tau$, `proved_here` conditionnel) ; et Jung confine le centre près du milieu de l'arête diamètre ($\Vert c-(p+q)/2\Vert^2\leq D^2/8$). Attention : la « plus grande arête RNG incidente ne majore pas $2R(p)$ » (fixture rang 11) — l'ancrage par l'arête maximale exige la majoration certifiée $\tau$, pas un RNG.
2. **WSPD de Callahan–Kosaraju ($s=6/8/10$)** : **absente des documents d'autorité.** Les analogues existants sont le self-produit LBVH $(A,A)\mapsto(L,L),(L,R),(R,R)$ (`proved_here`), les produits de blocs $A\times B$ avec reçus P8g/P8l, et le center-cover P1 ($C_{AB}=M_{AB}\oplus[-5H/8,5H/8]^3$, 64 patches). Une WSPD est compatible avec l'esprit (partition exacte des paires, certificats par rectangles) mais devra fournir : (a) une preuve de partition/couverture exacte des paires non ordonnées ; (b) des certificats d'exclusion rejouables par rectangle ; (c) l'identité de masse $\binom{n}{2}$ fermée. Rien dans le registre ne fixe $s=6/8/10$.
3. **« h témoins dans la zone cœur »** : les seuils exacts sont gravés — paires : $s_{\max}-1$ (soit **10** à $K=10$, 9 est faux) ; support $m\in\{3,4\}$ : $H=s_{\max}-m+1$ (soit **9** pour q3, **8** pour q4 à $s_{\max}=11$), avec la sémantique stricte (égalité = inconclusive, borne nulle jamais acceptée) et antichaînes de plages disjointes des supports.
4. **« h_a/h_b, témoins dans A et B, dual-tree, jamais quadratique A×B »** : le registre exige pour les témoins **universels** des plages disjointes des plages supports ; un témoin pris dans $A$ ou $B$ interfère avec les paires dont il est support — la note P1 précise seulement qu'« un futur carrier n'a pas à être exclu a priori des témoins » via l'argument d'égalité de shell au centre réel. Tout schéma h_a/h_b devra donc porter sa propre preuve (nouvelle entrée au registre). Le risque quadratique/cubique est documenté : interdiction du scan dense, `false_in_general` sur la borne quadratique du B&B, et le verrou « nombre de produits examinés » rouvert le 7 août 2026.
5. **« Identification exacte et GPU-friendly des événements »** : motif imposé partout — **proposition GPU (arrondi extérieur) / recertification CPU exacte** ; `ExactCenter`/`ExactLevel` homogènes ; jamais de quotient flottant ; lots de niveau égal par comparateur exact.
6. **« Reconstruction de la forêt HGP »** : la sortie normative n'est pas un arbre de points — univers des facettes, recouvrements, `coverage_delta`, lots simultanés, incidences silencieuses (E5), M.1/O5, morphismes verticaux, laminarité seulement en aval (`build_exact_point_hierarchy`, routage descendant irréversible).
7. **Contrats chiffrés V4** : « forêt complète $K=10$ < 100 ms » = l'objectif principal documenté (p95 `warm_e2e`, 50 000 pts, familles favorables, payload `BenchmarkOutputContract-v1`, version **sans budget**) ; « $K=5$ / < 1 s en secondaire » : le secondaire documenté est **< 1 s au même $K=10$ et même payload** ; le repli $K_{\max}=5$ existe seulement comme mesure de composant (SHA `3be0d42`, `maximum_closed_rank=6`, lanceur 1,249 s). Dizaines de millions : paliers documentés $10^6\leq60$ s, $10^7\leq600$ s, profils 10 M/30 M existants.

---

## Questions ouvertes / ambiguïtés

1. **WSPD ($s=6/8/10$)** : aucune des six sources ne mentionne Callahan–Kosaraju ni un paramètre de séparation $s$. Les valeurs 6/8/10 viennent de la feuille de route utilisateur seulement. Il faudra une NOTE_SOLUTION prouvant la couverture exacte des paires par la WSPD retenue et le lien entre $s$ et les certificats (Jung : $5H/8$ pour q4, $H/2$ pour q3 dans le formalisme P1) — je ne peux pas confirmer que $s=6$ suffit pour quel certificat.
2. **Témoins h_a/h_b dans A et B** : non couverts par les entrées existantes (qui exigent des témoins **hors** plages supports pour l'universalité). La seule ouverture est la remarque P1 sur l'égalité de shell d'un carrier au centre réel. Statut de preuve à créer.
3. **Tension 100 ms / « sous la seconde »** : la spécification §1.1, le plan §14.4 et la Phase 14 font du p95 < 100 ms l'objectif **principal** et de < 1 s le **secondaire** ; mais la directive du 6 août 2026 (roadmap, discipline de tests) écrit « La cible produit à 50 000 points est **sous la seconde** ». Je ne tranche pas : les deux textes coexistent, le plus récent semble assouplir la cible opératoire sans réviser les portes G6/14.4.
4. **Tailles 8000/16000/32000 vs 50 000 vs « n=32 puis 50 000 »** : §3.1 du plan impose 8k/16k/32k pour toute conclusion d'échelle ; les portes de performance historiques sont à 50 000 (et 12 500/25 000/50 000 pour le smoke) ; la directive du 6 août dit « $n=32$ … puis directement 50 000 … rien entre ». Lecture cohérente possible (8k/16k/32k = campagnes de sélectivité/pente du présent dossier ; 50 k = porte SLO), mais la hiérarchie exacte entre ces trois prescriptions n'est pas écrite. Le « voire 64000 » de la feuille V4 n'apparaît nulle part (seul $n\leq64$ existe, pour l'oracle de condensation $k=1$).
5. **Régimes « uniforme/terrain/clusters »** : « terrain » n'est pas une famille nommée du plan ; les plus proches sont « surface bruitée » et « scènes LiDAR synthétiques avec sol, façades, objets minces et occlusions » (§9.2, §14.5). À nommer précisément dans le plan V4.
6. **« Troisième témoin x formant un triangle aigu » (q3) et « x et y » (q4)** : compatibles avec la stratification `proved_here` (triangles strictement aigus, tétraèdres strictement bien centrés), mais attention aux deux contre-exemples : l'aiguïté des **faces** ne caractérise pas q4 (deux directions fausses), et le rang des côtés ne borne rien pour q3. La caractérisation correcte passe par barycentriques strictement positives (Gram–Cramer), pas par des tests d'angles composés.
7. **Sections non lues ligne à ligne** : SPEC §8.6–§9.26 (jalons v3 : ingress fenêtré, projection forestière sparse, ledger, locator historique, autorité chronologique, producteur Morton, projection binary64) et la longue liste des jalons 14A–14AB/15A–15K. Elles décrivent l'architecture v3 abandonnée ; si la V4 veut réutiliser un de ces contrats (p. ex. la reprise durable 9.3c–9.3e ou le commit vivant 15D), une lecture dédiée s'imposera.
8. **Compte de témoins — deux conventions** : `requested_order=K` ⇒ rang fermé $\leq K+1$ ⇒ $K$ témoins ; V5octodecies nonies donne $H=s_{\max}-m+1$ pour $m\in\{3,4\}$ ; pour $m=2$ le seuil est $s_{\max}-1$ (formellement $s_{\max}-m+1$ avec $m=2$ donne $s_{\max}-1$ : cohérent). Mais §5.1 énonce le seuil « $s_{\max}-m+1$ » et P1 « 8 témoins + 4 shell ⇒ rang ≥ 12 » à $s_{\max}=11$ : cohérent aussi. La seule subtilité est $s_{\max}=\min(K_{\mathrm{eff}}+1,n)$ (dépend de $n$ pour $n\leq10$) — les petites fixtures doivent utiliser $s_{\max}$ effectif, pas $K+1$.
9. **Recensement de la sortie utile** : la porte préalable exigée depuis le 7 août 2026 (« combien de supports 3–4 minimaux bien centrés de rang fermé $\leq K$ existent réellement » aux tailles cibles) n'a pas encore de résultat publié dans les documents lus. La V4 devrait la traiter comme première mesure aux tailles 8k/16k/32k.
