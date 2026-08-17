# Lecture normative — MANUSCRIT_THESE_HAUSEUX.pdf, pages PDF 95–114 (pages imprimées 69–88)

Tranche couverte : fin du chapitre 7 (« Le phénomène de percolation au cœur des performances », § 7.2–7.5 + Synthèse, pages PDF 95–107) puis ouverture du chapitre 8 (« À la recherche de la généralisation de l'arbre minimum couvrant », § 8.1–8.2 jusqu'au début de la preuve du Théorème 4, pages PDF 109–114). Correspondance : page PDF = page imprimée + 26.

---

## 1. Structure section par section

| Page PDF | Page imprimée | Contenu |
|---|---|---|
| 95 | 69 | Fin de preuve (seuils de percolation, domination par percolation de sites) ; Remarque (transition *abrupte* pour les K-polyèdres) ; **Proposition 2** (DBSCAN et K-Robust Single-Linkage : même seuil critique) ; début § 7.2 « Percolation et non-consistance » |
| 96 | 70 | Mécanisme de non-consistance ; **Figure 7.1** (inconsistance du Single-Linkage en dimension p ≥ 2) ; § 7.2.1 « Lecture générale avec la densité f » ; formule (7.1) |
| 97 | 71 | § 7.2.2 « Le cas simple des niveaux de densité constants » ; **Proposition 3** (fusion asymptotique fond supercritique) ; début **Théorème 3** |
| 98 | 72 | Fin et preuve du **Théorème 3** ; § 7.3 « La vitesse de percolation » ; **Définition 24** (vitesse critique de percolation) |
| 99 | 73 | Suite Déf. 24 ; version quantile (7.2) ; conversion intensités/rayons ; § 7.4 « Comparaison empirique entre K-polyèdres, K-Robust Single-Linkage et DBSCAN » (protocole) |
| 100 | 74 | Remarques K=1, K=2 ; **Figure 7.2** (Θ̂ poly, p=2, K∈{1..5}, λ_c ≈ 1.44 pour K=1 en 2D) ; **Tableau 7.1** (vitesses de percolation p=2,3,4, K=1..5) |
| 101 | 75 | **Figures 7.3** (core) et **7.4** (dbscan) ; Remarque sur la pente strictement positive au seuil (Single-Linkage) |
| 102 | 76 | Discussion (les K-polyèdres dominent dès K ≥ 2 ; Robust SL se dégrade pour K ≥ 3) ; § 7.5 « Et lorsque K → +∞ ? » ; § 7.5.1 « Le champ gaussien limite » (intensité normalisée μ, champ de comptage Z_μ) |
| 103 | 77 | **Proposition 4** (théorème central limite local du champ de comptage) + preuve |
| 104 | 78 | Fenêtre μ = K + a√K ; ensemble d'excursion gaussien E_a ; § 7.5.2 « Comportements sur le champ gaussien de HGP-Clusterer, (H)DBSCAN et Robust Single-Linkage » (événements A_a^core, A_a^dbscan, A_a^poly) |
| 105 | 79 | Récapitulatif formel (résultat admis : Θ → P(A_a^cc) quand K → +∞) ; les deux défauts du Robust SL |
| 106 | 80 | § 7.5.3 « Et à haut rappel ε → 0 » ; vitesse 1 − O(1/√K) ; obstructions ; quantiles conjecturés a^poly vs a^core ; Remarque (Cameron–Martin, RKHS, Schilder) |
| 107 | 81 | **Synthèse** du chapitre 7 |
| 108 | 82 | Page blanche |
| 109 | 83 | **Chapitre 8** : introduction, chaîne d'idées, contributions annoncées (Déf. 27, Théorème 4, Théorème 5 p. 91, mosaïque de Delaunay d'ordre K) |
| 110 | 84 | § 8.1 « L'approche persistante : les simplexes K-séparants » ; **Définition 25** (rayon de naissance) ; remarque miniball/Welzl ; **Définition 26** (position générale pour la filtration de Čech) |
| 111 | 85 | Remarques sur Déf. 26 ; **Fait (connu) 12** (boule minimale et ensemble de support) ; **Figure 8.1** (triangle aigu vs obtus) |
| 112 | 86 | § 8.1.1 « Le graphe élémentaire des facettes » ; **Proposition 5** (les adjacences élémentaires suffisent) + preuve ; Notation Γ_K(X)_{≤r}, Γ_K(X)_{<r} ; **Définition 27** (simplexe K-séparant) |
| 113 | 87 | Suite Déf. 27 (interprétation) ; Remarque (au moins deux facettes actives en position générale) ; lien avec l'arbre minimum couvrant (K=1) ; § 8.2 « Une condition nécessaire pour un simplexe séparant : être de Gabriel » ; **Définition 28** (simplexe de Gabriel) |
| 114 | 88 | Remarque (miniball ≠ circumball, différence avec [37]) ; **Théorème 4** (K-séparant ⇒ Gabriel) ; début de la preuve ; **Figure 8.2** (illustration de la preuve, K=2, triangle obtus, intrus z) |

---

## 2. Définitions et notations normatives

### Définition 24 — Vitesse critique de percolation (page PDF 98–99, p. 72–73)

Pour une famille cc, on définit le quantile de rappel au niveau $\alpha \in [0;1]$ :

$\lambda_\alpha^{\mathrm{cc}}(K,p) \triangleq \inf \left\lbrace \lambda > 0 \mid \Theta_{K,p,1/2}^{\mathrm{cc}}(\lambda) \geq \alpha \right\rbrace .$

La *vitesse critique de percolation* est le rapport :

$v_{\mathrm{crit},K,p}^{\mathrm{cc}}(\varepsilon) \triangleq \dfrac{\lambda_c^{\mathrm{cc}}(K,p)}{\lambda_{1-\varepsilon}^{\mathrm{cc}}(K,p)}.$

Version pratique (le seuil critique $\lambda_c$ étant difficile à estimer), formule (7.2) :

$v_{K,p}^{\mathrm{cc}}(\varepsilon) \triangleq \dfrac{\lambda_\varepsilon^{\mathrm{cc}}(K,p)}{\lambda_{1-\varepsilon}^{\mathrm{cc}}(K,p)}.$

Remarque (conversion en rayons) : $\dfrac{\lambda_\varepsilon^{\mathrm{cc}}(K,p)}{\lambda_{1-\varepsilon}^{\mathrm{cc}}(K,p)} = \left( \dfrac{r_\varepsilon^{\mathrm{cc}}(K,p)}{r_{1-\varepsilon}^{\mathrm{cc}}(K,p)} \right)^{p}.$

### Définition 25 — Rayon de naissance d'un simplexe (page PDF 110, p. 84)

Pour tout sous-ensemble fini non vide $\sigma \subseteq \mathcal{X}$, on note :

$\rho(\sigma) \triangleq \inf \left\lbrace r \geq 0 \mid \sigma \in \check{C}(\mathcal{X}, r) \right\rbrace .$

Autrement dit :

$\rho(\sigma) = \inf_{y \in \mathbb{R}^{p}} \max_{x \in \sigma} \Vert y - x \Vert .$

La boule fermée réalisant cet infimum sera notée $B_\sigma = \overline{B}(c_\sigma, \rho(\sigma))$, avec $c_\sigma$ son centre.

**Remarques attachées (normatives)** : cette plus petite boule englobante est la *miniball* de σ ; la méthode standard pour calculer $\rho(\sigma)$ et $c_\sigma$ est l'**algorithme de Welzl** [92]. Le rayon de naissance $\rho(\sigma)$ est *exactement* le niveau d'apparition de σ dans la filtration de Čech. Pour $K = 1$, $\sigma = \lbrace x, x' \rbrace$ : $\rho(\sigma) = \frac{1}{2}\Vert x - x' \Vert$.

### Définition 26 — Position générale pour la filtration de Čech (page PDF 110, p. 84)

On dit qu'un nuage $\mathcal{X} \subset \mathbb{R}^{p}$ localement fini est en *position générale pour la filtration de Čech* si, pour tout simplexe $\sigma \subseteq \mathcal{X}$ avec $|\sigma| \geq 2$, aucun point de $\mathcal{X} \setminus \sigma$ n'appartient à la frontière de sa plus petite boule englobante :

$\partial B_\sigma \cap (\mathcal{X} \setminus \sigma) = \varnothing .$

Autrement dit, aucun point du nuage extérieur à σ ne se trouve sur la frontière de la plus petite boule englobante de σ.

Remarques (page PDF 111) : (1) cette définition est **plus contraignante** que la Déf. 4.2 de [37] ; (2) sous cette hypothèse, les points de $\sigma \cap \partial B_\sigma$ forment exactement l'ensemble de support de la boule minimale $B_\sigma$ (Fait 12) ; en particulier ils sont affinement indépendants et $c_\sigma$ appartient à l'intérieur (relatif) de $\mathrm{conv}(\sigma \cap \partial B_\sigma)$.

### Notation Γ_K et ses coupes (page PDF 112, p. 86)

Rappel (Déf. 21, p. 58, hors tranche) : les K-polyèdres sont les composantes connexes du graphe $\Gamma_K(\mathcal{X}, r)$ dont les sommets sont les $(K-1)$-simplexes de $\check{C}(\mathcal{X}, r)$, deux sommets étant adjacents lorsque leur union est encore un simplexe de $\check{C}(\mathcal{X}, r)$ (sans restriction sur la taille de l'union).

**Notation normative** : à partir de la Prop. 5, $\Gamma_K(\mathcal{X}, r)$ désigne uniquement la version élaguée du graphe, réduite aux **adjacences élémentaires** (les seuls K-simplexes créent les liaisons). Notations : $\Gamma_K(\mathcal{X})_{\leq r}$ (indifféremment $\Gamma_K(\mathcal{X}, r)$), et $\Gamma_K(\mathcal{X})$ sans indice quand $r = +\infty$. On note $\Gamma_K(\mathcal{X})_{<r}$ le sous-graphe de $\Gamma_K(\mathcal{X})_{\leq r}$ **privé de tous les simplexes σ (sommets et arêtes) ayant un rayon ρ(σ) exactement égal à r**.

Convention d'arité implicite dans tout le chapitre : un « K-simplexe » compte $K+1$ points, ses « facettes » comptent $K$ points (ce sont les $(K-1)$-simplexes, sommets de Γ_K).

### Définition 27 — Simplexe K-séparant (page PDF 112–113, p. 86–87)

Soit $\sigma \subseteq \mathcal{X}$ un K-simplexe, et posons $r_\sigma \triangleq \rho(\sigma)$. On appelle *facettes actives* de σ les facettes déjà présentes **strictement avant** la naissance de σ :

$\mathcal{F}_{<}(\sigma) \triangleq \left\lbrace \tau \subset \sigma \mid |\tau| = K \text{ et } \rho(\tau) < r_\sigma \right\rbrace .$

On dit que σ est **K-séparant** s'il existe deux facettes actives $\tau, \tau' \in \mathcal{F}_{<}(\sigma)$ qui appartiennent à deux composantes connexes **distinctes** de $\Gamma_K(\mathcal{X})_{< r_\sigma}$.

« Autrement dit, σ est K-séparant lorsqu'il provoque effectivement une fusion entre deux K-polyèdres déjà présents. » (p. 87)

### Définition 28 — Simplexe de Gabriel (page PDF 113, p. 87)

Un K-simplexe $\sigma \subseteq \mathcal{X}$ est dit de **Gabriel** si l'intérieur de sa plus petite boule englobante ne contient aucun point de $\mathcal{X}$ extérieur à σ :

$\mathring{B}_\sigma \cap (\mathcal{X} \setminus \sigma) = \varnothing .$

**Remarque normative** (page PDF 114) : cette généralisation de la notion de simplexe de Gabriel à l'aide de la **miniball** diffère de celle de [37], où l'on impose une condition de sphère circonscrite vide (la *circumball*). Pour $K = 1$, les deux notions coïncident avec l'arête de Gabriel classique.

### Notations du chapitre 7 (champ gaussien, § 7.5)

- Intensité normalisée : $\mu \triangleq \lambda |B(0;1/2)| = \lambda 2^{-p} \omega_p$ (page PDF 102).
- $N_\mu(x) \triangleq |\mathcal{X}_{\mu/|B(0,1/2)|} \cap B(x, 1/2)|$ suit une loi de Poisson de paramètre μ ; $\mathbb{E}[N_\mu(x)] = \mu$, $\mathbb{V}[N_\mu(x)] = \mu$. Invariance des vitesses : $\lambda_c / \lambda_{1-\varepsilon} = \mu_c / \mu_{1-\varepsilon}$.
- Champ de comptage centré réduit : $Z_\mu(x) \triangleq \dfrac{N_\mu(x) - \mu}{\sqrt{\mu}}$.
- Fenêtre d'intérêt : $\mu = K + a\sqrt{K}$ ; $\lbrace x \mid N_\mu(x) \geq K \rbrace = \lbrace x \mid Z_\mu(x) \geq \frac{K - \mu}{\sqrt{\mu}} \rbrace$ avec $\frac{K-\mu}{\sqrt{\mu}} \to -a$.
- Ensemble d'excursion gaussien : $E_a \triangleq \lbrace x \in \mathbb{R}^{p} \mid G_p(x) \geq -a \rbrace$.
- Événements asymptotiques (pages PDF 104–105) : $A_a^{\mathrm{core}} = \lbrace 0 \in E_a \cap \mathcal{C}_\infty(\delta_{1/2}(E_a)) \rbrace$ ; $A_a^{\mathrm{dbscan}} = \lbrace 0 \in \mathcal{C}_\infty(\delta_{1/2}(E_a)) \rbrace$ ; $A_a^{\mathrm{poly}} = \lbrace 0 \in \delta_{1/2}(\mathcal{C}_\infty(E_a)) \rbrace$ — la seule différence de HGP-Clusterer est que la dilatation $\delta_{1/2}$ est appliquée **après** l'extraction de la composante infinie, pas avant.
- $\mu_K(a) := K + a\sqrt{K}$, $\lambda_K(a) \triangleq \mu_K(a)/|B(0;1/2)|$, $F_K^{\mathrm{cc}}(a) \triangleq \Theta_{K,p,1/2}^{\mathrm{cc}}\left( \frac{K + a\sqrt{K}}{|B(0;1/2)|} \right)$.

### Modèle à deux densités (§ 7.2.2, page PDF 97)

$f(x) = \rho \mathbf{1}_{A \cup B}(x) + \rho_0 \mathbf{1}_{F}(x)$, avec $\rho, \rho_0 > 0$ et $\rho_1 \triangleq \rho + \rho_0$. $A, B \subset \mathbb{R}^{p}$ compacts convexes de mesure strictement positive, $F \supset A \cup B$ fond contenant un corridor macroscopique reliant A à B.

### Lecture locale par la densité (§ 7.2.1, page PDF 96)

Échelle locale $r_n \to 0$, normalisation $r = 1/2$ : autour d'un point x où f est régulière, le nuage vu à l'échelle $r_n$ ressemble à un Poisson homogène d'intensité $\lambda_n(x) \simeq n (2 r_n)^{p} f(x)$. La fonction $\Theta_{K,p,1/2}^{\mathrm{cc}}(\lambda_n(x))$ donne la probabilité asymptotique qu'un point situé dans une région de densité $f(x)$ appartienne à la composante géante observée à cette échelle. Formule (7.1), proportion récupérable de l'amas C juste avant fusion :

$\dfrac{\int_C \Theta_{K,p,1/2}^{\mathrm{cc}}\left( \lambda_c^{\mathrm{cc}}(K,p) \frac{f(x)}{\tau} \right) f(x)\,\mathrm{d}x}{\int_C f(x)\,\mathrm{d}x}.$

---

## 3. Théorèmes, propositions, faits

### Proposition 2 — DBSCAN et K-Robust Single-Linkage ont le même seuil critique (page PDF 95, p. 69)

Pour tout $K \geq 1$ et $p \geq 2$ : $\lambda_c^{\mathrm{dbscan}}(K,p) = \lambda_c^{\mathrm{core}}(K,p)$, où les deux seuils sont ceux définis en Prop. 1. De plus, pour tout $\lambda > 0$ : $\Theta_{K,p,1/2}^{\mathrm{dbscan}}(\lambda) \geq \Theta_{K,p,1/2}^{\mathrm{core}}(\lambda)$.

Preuve : une composante DBSCAN est une composante de cœurs à laquelle on ajoute les points à moins de 1/2 d'un cœur ; elle est infinie ssi la composante de cœurs l'est. L'inégalité : tout point cœur d'une composante K-Robust appartient à la composante DBSCAN correspondante.

Contexte immédiat (fin de preuve précédente, page PDF 95) : pour $p \geq 2$, $\lambda_c^{\mathrm{cc}}(K,p) \geq \lambda_c^{\mathrm{bool}}(p) > 0$ ; finitude du seuil par domination d'une percolation de sites sur une grille $a\mathbb{Z}^{p}$. Remarque : pour les K-polyèdres la transition est *abrupte* (sharp) — décroissance exponentielle des tailles de composantes en régime sous-critique ([76], [68]).

### Proposition 3 — Fusion asymptotique lorsque le fond devient supercritique (page PDF 97, p. 71)

Soit cc une famille de composantes, $\lambda_c^{\mathrm{cc}}(K,p)$ son seuil critique. Si le rayon $r_n$ est choisi de sorte que le fond soit strictement supercritique, $n(2r_n)^{p}\rho_0 > \lambda_c^{\mathrm{cc}}(K,p) + \eta$ pour un $\eta > 0$ fixé, alors les deux composantes géantes présentes dans A et B fusionnent avec probabilité tendant vers 1 lorsque $n \to +\infty$.

### Théorème 3 — Fraction récupérable avant fusion parasite (pages PDF 97–98, p. 71–72)

Soit cc une méthode de clustering persistant, de probabilité de percolation $\Theta^{\mathrm{cc}}$ (supposée continue) et de seuil critique $\lambda_c^{\mathrm{cc}}$. Dans le modèle à deux densités, la fraction asymptotiquement récupérable dans A et B juste avant la percolation du fond est :

$\Theta_{K,p,1/2}^{\mathrm{cc}}\left( \lambda_c^{\mathrm{cc}}(K,p) \dfrac{\rho_1}{\rho_0} \right).$

En particulier, un rappel asymptotique d'au moins $1 - \varepsilon$ avant fusion parasite est possible **si et seulement si** :

$\dfrac{\rho_0}{\rho_1} \leq \dfrac{\lambda_c^{\mathrm{cc}}(K,p)}{\lambda_{1-\varepsilon}^{\mathrm{cc}}(K,p)}, \qquad \lambda_{1-\varepsilon}^{\mathrm{cc}}(K,p) \triangleq \inf \left\lbrace \lambda > 0 : \Theta_{K,p,1/2}^{\mathrm{cc}}(\lambda) \geq 1 - \varepsilon \right\rbrace .$

Preuve : quand le fond devient critique, l'intensité normalisée dans A et B vaut $\lambda_c^{\mathrm{cc}}(K,p)\rho_1/\rho_0$ ; la proportion de points dans la composante géante converge vers la probabilité de Palm correspondante.

### Proposition 4 — Théorème central limite local du champ de comptage (page PDF 103, p. 77)

Pour tous $x_1, \ldots, x_m \in \mathbb{R}^{p}$, le vecteur $(Z_\mu(x_1), \ldots, Z_\mu(x_m))$ converge en loi, lorsque $\mu \to +\infty$, vers un vecteur gaussien centré $(G_p(x_1), \ldots, G_p(x_m))$ de covariance :

$\mathbb{E}[G_p(x_i) G_p(x_j)] = \rho_p(x_i - x_j) \triangleq \dfrac{|B(x_i, 1/2) \cap B(x_j, 1/2)|}{|B(0,1/2)|}.$

$G_p$ désigne le champ gaussien stationnaire centré de covariance $\rho_p$. En particulier $\rho_p(z) = 0$ dès que $\Vert z \Vert \geq 1$.

Preuve : partition de $\bigcup_i B_i$ en atomes $A_J \triangleq \left( \bigcap_{j \in J} B_j \right) \setminus \left( \bigcup_{j \notin J} B_j \right)$, $\varnothing \neq J \subseteq \lbrace 1, \ldots, m \rbrace$ ; les $Y_J \triangleq \mathcal{X}_{\mu/|B(0,1/2)|}(A_J)$ sont Poisson indépendantes ; approximation Poisson–Normale ; $\mathrm{Cov}(Z_\mu(x_i), Z_\mu(x_j)) = |B_i \cap B_j|/|B(0,1/2)|$.

### Résultat admis, § 7.5.2 (page PDF 105, p. 79)

En tout point de continuité de $a \mapsto \mathbb{P}(A_a^{\mathrm{cc}})$ :

$\Theta_{K,p,1/2}^{\mathrm{cc}}\left( \dfrac{K + a\sqrt{K}}{|B(0;1/2)|} \right) \xrightarrow[K \to +\infty]{} \mathbb{P}(A_a^{\mathrm{cc}}).$

Deux défauts du Robust SL vs HGP-Clusterer : (1) la dilatation $\delta_{1/2}$ est appliquée **avant** l'identification des composantes géantes $\mathcal{C}_\infty$, provoquant une percolation précoce ($a_c^{\mathrm{core}} = a_c^{\mathrm{dbscan}} \leq a_c^{\mathrm{poly}}$) ; (2) la condition de point cœur (appartenir à l'excursion $E_a$) est beaucoup trop restrictive. DBSCAN évite le second défaut, pas le premier.

### § 7.5.3 — Haut rappel ε → 0 (page PDF 106, p. 80 ; résultats non rigoureux, « ébauche d'intuitions »)

À ε > 0 fixé, quel que soit cc ∈ {poly ; dbscan ; core}, la vitesse de percolation est asymptotiquement $1 - \mathcal{O}\left( \frac{1}{\sqrt{K}} \right)$. En rappel parfait ($\varepsilon \to 0$, $a \to \infty$, $a = o(\sqrt{K})$) : pour les cœurs, l'obstruction dominante est $0 \notin E_a$ ; pour les polyèdres, l'obstruction est beaucoup plus coûteuse — il faut que **toute** la boule $B(0;1/2)$ soit hors de $\mathcal{C}_\infty(E_a)$. Quantiles conjecturés :

$a_{1-\varepsilon}^{\mathrm{poly}}(p) \sim \sqrt{2 U_p \log(1/\varepsilon)} \; < \; a_{1-\varepsilon}^{\mathrm{core}}(p) \sim \sqrt{2 \log(1/\varepsilon)}$

où $U_p < 1$ est associé au coût de l'obstruction $\forall x \in B(0,1/2), \; G_p(x) < -1$. De plus $a_c^{\mathrm{poly}} \geq a_c^{\mathrm{core}}$. (Conjecture : le coût de la réduction de l'excursion à sa composante infinie est négligeable devant celui de la dilatation $\delta_{1/2}$.)

### Fait (connu) 12 — Boule minimale et ensemble de support [37, 92] (page PDF 111, p. 85)

Soit $A \subset \mathbb{R}^{p}$ fini non vide. Il existe une **unique** plus petite boule fermée contenant A. Son centre appartient à l'enveloppe convexe d'un sous-ensemble $S(A) \subseteq A \cap \partial B_A$ de cardinal **au plus $p+1$**. Sous l'hypothèse de position générale (Déf. 26), cet ensemble de support est exactement l'ensemble des points de A situés sur la frontière de $B_A$. En particulier, si $s \in S(A)$, alors :

$\rho(A \setminus \lbrace s \rbrace) < \rho(A).$

### Proposition 5 — Les adjacences élémentaires suffisent (page PDF 112, p. 86)

Pour tout $r \geq 0$, retirer de $\Gamma_K(\mathcal{X}, r)$ les liens entre deux sommets τ et τ′ dès que $|\tau \cup \tau'| > K + 1$ laisse inchangées les composantes connexes. Elles correspondent donc toujours aux K-polyèdres du complexe de Čech $\check{C}(\mathcal{X}, r)$ définis en Déf. 21.

Preuve : si τ, τ′ adjacents alors $\rho(\tau \cup \tau') \leq r$. Si $|\tau \cup \tau'| = K+1$, l'adjacence est déjà élémentaire. Si $|\tau \cup \tau'| > K+1$, on relie τ à τ′ par un chemin de K-sous-simplexes de $\tau \cup \tau'$ différant à chaque fois d'un seul élément ; ces liaisons sont toutes présentes dans le graphe élagué (la monotonie de ρ sur les sous-ensembles donne des rayons ≤ r).

### Remarque structurelle après Déf. 27 (page PDF 113, p. 87)

Sous position générale pour la filtration de Čech, un K-simplexe σ possède **toujours au moins deux facettes actives**. En effet, si $S_\sigma \triangleq \sigma \cap \partial B_\sigma$ désigne l'ensemble de support de sa miniball, alors $|S_\sigma| \geq 2$ et pour tout $s \in S_\sigma$ : $\rho(\sigma \setminus \lbrace s \rbrace) < \rho(\sigma)$. Les facettes actives sont donc au moins les $\sigma \setminus \lbrace s \rbrace$, $s \in S_\sigma$. Les autres facettes, obtenues en retirant un sommet **intérieur** de $B_\sigma$, peuvent en revanche naître **exactement au même niveau** que σ.

Interprétation (p. 87) : un simplexe K-séparant est un simplexe qui change réellement la connectivité persistante des K-polyèdres. Pour $K = 1$, les 1-simplexes séparants sont exactement les arêtes qui relient deux composantes distinctes du graphe géométrique au moment où elles apparaissent : **ce sont les arêtes de l'arbre minimum couvrant**. Le Fait 2 appliqué à $\Gamma_K(\mathcal{X})$ dit qu'un arbre couvrant minimum de $\Gamma_K(\mathcal{X})$, élagué à l'échelle r, redonnerait les K-polyèdres de $\check{C}(\mathcal{X}, r)$ — mais ce graphe contient encore $\binom{n}{K}$ sommets ; il faut un sous-graphe beaucoup plus mince.

### Théorème 4 — Condition nécessaire pour que σ soit K-séparant : σ doit être de Gabriel (page PDF 114, p. 88)

**Supposons $\mathcal{X}$ en position générale pour la filtration de Čech. Tout simplexe K-séparant est de Gabriel.**

Début de preuve (le reste dépasse la page 114) : par contraposée. Soit σ un K-simplexe non Gabriel, $r = \rho(\sigma)$, $B_\sigma = \overline{B}(c, r)$ ; il existe un intrus $z \in \mathcal{X} \setminus \sigma$ avec $z \in \mathring{B}_\sigma$. On pose $S \triangleq \sigma \cap \partial B_\sigma$ ; par Déf. 26 et Fait 12, S est l'ensemble de support de $B_\sigma$, $|S| \geq 2$, c est intérieur (relatif) à $\mathrm{conv}(S)$, et retirer n'importe quel point de S diminue strictement le rayon de naissance. La Figure 8.2(b) (K = 2, triangle obtus en $x_3$, $S = \lbrace x_1, x_2 \rbrace$) montre le mécanisme : les K-simplexes $\sigma_1^{z}$ et $\sigma_2^{z}$ (obtenus en substituant z à un sommet de σ) ont des rayons < r ; le chemin $\tau_1 \leftrightarrow_{\sigma_1^{z}} \eta_{1,2}^{z} \leftrightarrow_{\sigma_2^{z}} \tau_2$ connecte les facettes actives dans $\Gamma_K(\mathcal{X})_{<r}$, donc σ ne sépare rien : la fusion a déjà eu lieu strictement avant $r$ grâce à l'intrus.

### Chaîne d'idées du chapitre 8 (page PDF 109, p. 83)

« Complexes de Čech ⇝ Simplexes responsables des fusions ⇝ K-Graphe de Gabriel ⇝ Mosaïque de Delaunay d'ordre K. »

Contributions annoncées : (i) K-polyèdres retrouvés comme composantes connexes d'un graphe pondéré *élémentaire* sur les $(K-1)$-simplexes (arêtes restreintes aux K-simplexes) ; (ii) Déf. 27 (K-séparants) ; (iii) Théorème 4 (K-séparant ⇒ Gabriel) ; (iv) **Théorème 5, p. 91 (PDF 117, hors tranche)** : le graphe de Gabriel contient toutes les fusions utiles et un arbre couvrant minimum construit sur ce graphe préserve, à chaque échelle, les K-polyèdres ; (v) construction pratique de la hiérarchie K-NN via la mosaïque de Delaunay d'ordre K. Rappel de coût motivant : le complexe de Čech complet manipule tous les K-simplexes sur $\mathcal{X}$, au nombre de $\binom{n}{K+1}$. Pour K = 1 : MST ⊂ graphe de Gabriel ⊂ triangulation de Delaunay (Fait 1, p. 16).

---

## 4. Couverture thématique demandée

- **Ordre K** : partout. Convention d'arité : K-simplexe = $K+1$ points, facette = $K$ points = $(K-1)$-simplexe = sommet de Γ_K. $K \in [[1, |\mathcal{X}|-1]]$ (page PDF 110). Empiriquement (Tableau 7.1, page PDF 100), la vitesse de percolation des K-polyèdres croît strictement avec K (p=2 : 0,735 → 0,826 pour K=1→5 ; p=3 : 0,563 → 0,732 ; p=4 : 0,362 → 0,534), et domine Robust SL et DBSCAN dès K ≥ 2. Asymptotiquement, vitesse = $1 - \mathcal{O}(1/\sqrt{K})$ pour les trois familles, avec constante meilleure pour poly (§ 7.5.3).
- **Simplexes et filtration, rayon ρ(σ)** : Déf. 25 — ρ(σ) est le rayon miniball, minimax $\inf_y \max_{x \in \sigma} \Vert y - x \Vert$, et c'est *exactement* le niveau d'apparition dans la filtration de Čech. Monotonie implicite : $\tau \subset \sigma \Rightarrow \rho(\tau) \leq \rho(\sigma)$ (utilisée dans la preuve de Prop. 5).
- **Miniball / circumradius / rayon de couverture** : miniball = objet normatif (Déf. 25, Welzl [92]). Le manuscrit **écarte explicitement le circumradius** : la Remarque page PDF 114 distingue le Gabriel-miniball (retenu) du Gabriel-circumball de [37] (non retenu). Rien dans cette tranche ne porte le nom « rayon de couverture ».
- **Théorème de Jung** : **absent de cette tranche** (aucune occurrence pages PDF 95–114). Le Fait 12 borne le support à $p+1$ points, ce qui est l'ingrédient de même nature, mais Jung n'est pas cité ici.
- **Triangles aigus/obtus** : Figure 8.1 (page PDF 111) : triangle **aigu** → $c_\sigma$ intérieur à l'enveloppe convexe, tous les sommets définissent la boule minimale, $S = \sigma$ (support d'arité 3) ; triangle **obtus** → le centre appartient à une arête, $S = \lbrace x_1, x_2 \rbrace \subsetneq \sigma$ (support d'arité 2, la miniball est la boule diamétrale de l'arête la plus longue). Figure 8.2 (page PDF 114) exploite le cas obtus dans la preuve du Théorème 4.
- **Événements de naissance/fusion** : naissance = ρ(σ) (Déf. 25) ; fusion = simplexe K-séparant (Déf. 27, avec facettes actives à inégalité **stricte** ρ(τ) < r_σ et connectivité évaluée dans $\Gamma_K(\mathcal{X})_{<r_\sigma}$, graphe privé des simplexes de rayon exactement r_σ). Condition nécessaire de fusion : Gabriel (Théorème 4).
- **Arbre/forêt de fusion** : pour K = 1, séparants = arêtes du MST (page PDF 113) ; le Fait 2 appliqué à Γ_K promet qu'un MST de Γ_K élagué à l'échelle r redonne les K-polyèdres ; le Théorème 5 (p. 91, **hors tranche**, annoncé page PDF 109) affirme que le MST sur le graphe de Gabriel préserve les K-polyèdres à chaque échelle.
- **Partition des (K−1)-simplexes (§ 9.1) et poids $w_{x\tau} = S_\tau / T_x$** : **hors de cette tranche** (chapitre 9, au-delà de la page PDF 114). Rien à en citer ici.
- **Algorithmes et coûts** : (a) algorithme de Welzl pour miniball (page PDF 110) ; (b) comptages : $\binom{n}{K+1}$ K-simplexes du Čech complet, $\binom{n}{K}$ sommets de Γ_K (pages PDF 109, 113) — motivation de l'élagage Gabriel ; (c) protocole empirique § 7.4 : T = 100 tests, n = 100 000 points (10 000 en p = 4), ε = 3 %, hypercube $[0, n^{1/p}]^{p}$, cc ∈ {poly, core, dbscan}, estimateur $\widehat{v}^{\mathrm{cc}} = \widehat{\lambda}_\varepsilon^{\mathrm{cc}} / \widehat{\lambda}_{1-\varepsilon}^{\mathrm{cc}}$ ; (d) seuil K = 1 en 2D : $\lambda_c \approx 1.44$ [77, 78] (Figure 7.2) ; (e) pente au seuil (K=1) : $\exists C > 0, \forall \lambda \geq \lambda_c, \; \Theta(\lambda) \geq C(\lambda - \lambda_c)$ [79–81] (page PDF 101). Aucune complexité asymptotique d'algorithme de construction n'est donnée dans cette tranche.

---

## 5. Pertinence pour la conception V4

### Ce que la tranche impose

1. **La métrique d'événement est la miniball, pas la circumball.** ρ(σ) = rayon de la plus petite boule englobante (Déf. 25), calculable par Welzl. Toute identification GPU des événements q2/q3/q4 doit décider le rayon miniball exact. La Remarque page PDF 114 interdit de substituer le circumradius : les deux notions divergent dès K ≥ 2 (simplexes obtus).
2. **Typologie des supports = exactement les arités q2/q3/q4 de la feuille de route.** Fait 12 : le support $S(A)$ a un cardinal au plus $p + 1 = 4$ en 3D, et sous position générale $S_\sigma = \sigma \cap \partial B_\sigma$ exactement. Figure 8.1 : arité 3 ⇔ triangle **aigu** (centre dans l'intérieur relatif de conv(S)) ; triangle obtus ⇒ le support retombe à l'arité 2 (boule diamétrale de l'arête maximale). C'est la justification normative de « q3 avec troisième témoin x formant un triangle aigu » et « q4 avec x et y » (le tétraèdre-support exige que le centre soit intérieur à conv des 4 points). La stratégie « recherche de l'arête maximale via WSPD » est cohérente : pour q2 la miniball est la boule diamétrale de la paire, et pour q3/q4 le diamètre du support borne le rayon (mais la tranche ne donne pas de borne type Jung — voir Questions ouvertes).
3. **La condition d'élimination par témoins est exactement la condition de Gabriel** (Déf. 28 + Théorème 4) : un candidat σ est éliminable dès qu'un témoin $z \in \mathcal{X} \setminus \sigma$ vérifie $\Vert z - c_\sigma \Vert < \rho(\sigma)$ (intérieur **ouvert** de la miniball). La « zone cœur » des h témoins de la V4 doit être $\mathring{B}_\sigma$, dépendante de l'arité (q2 : boule diamétrale de l'arête ; q3 : boule circonscrite au triangle aigu dans son plan… en fait boule 3D de centre le circumcentre planaire ; q4 : boule de support tétraédrique). Attention : Gabriel est **nécessaire, pas suffisant** — après le filtre Gabriel, il reste à départager quels simplexes de Gabriel sont réellement K-séparants (Déf. 27 : deux facettes actives dans deux composantes distinctes de $\Gamma_K(\mathcal{X})_{<r_\sigma}$) ; c'est un test de connectivité au moment de la naissance, résolu classiquement par un traitement type Kruskal/union-find des candidats triés par ρ croissant. La suffisance opérationnelle (le MST sur le graphe de Gabriel préserve les K-polyèdres) est le Théorème 5, p. 91 — **à lire dans la tranche suivante**.
4. **Sémantique stricte des égalités de rayon.** Déf. 27 utilise ρ(τ) < r_σ (facettes actives strictement antérieures) et $\Gamma_K(\mathcal{X})_{<r}$ retire les simplexes de rayon **exactement** r. Une implémentation exacte doit donc décider les égalités de rayons sans tolérance — cohérent avec le FP certifié et le refus des dégénérescences du dépôt. Les facettes obtenues en retirant un sommet intérieur de $B_\sigma$ peuvent naître exactement au niveau de σ : les ex æquo existent structurellement même en position générale.
5. **Position générale (Déf. 26) est l'hypothèse porteuse du Théorème 4** : aucun point extérieur sur ∂B_σ, pour *tout* simplexe. Elle est plus forte que la position générale usuelle [37]. Sur entrée u16 quantifiée, elle peut être violée (points cosphériques) : conformément au profil du dépôt, cela doit produire un refus explicite (`unsupported_degeneracy`), jamais un jitter — le théorème ne s'applique pas sinon.
6. **Le graphe des fusions est élémentaire** (Prop. 5) : sommets = facettes (K points), arêtes = K-simplexes (K+1 points) uniquement. La V4 n'a jamais besoin d'adjacences d'ordre supérieur ; l'événement atomique est toujours « un (K+1)-uplet naît et relie deux de ses facettes ».
7. **Le choix K = 10 (contrat V4) est motivé par le chapitre 7** : la vitesse de percolation des K-polyèdres croît avec K (Tableau 7.1, jusqu'à K=5 mesuré ; asymptotique $1 - \mathcal{O}(1/\sqrt{K})$), et poly domine core/dbscan parce que la dilatation est appliquée après extraction de la composante. Ceci justifie l'objet mais n'ajoute aucune contrainte d'implémentation.

### Ce que la tranche interdit

- Matérialiser le complexe de Čech complet ($\binom{n}{K+1}$ simplexes) ou le graphe Γ_K complet ($\binom{n}{K}$ sommets) : le manuscrit lui-même le déclare « brutal algorithmiquement » (page PDF 109) et tout le chapitre 8 est l'élagage de cette explosion. La V4 (candidats via WSPD + filtre Gabriel par témoins) est dans l'esprit exact du chapitre.
- Confondre miniball et circumball dans le test de Gabriel (page PDF 114).
- Traiter les égalités de rayon avec une tolérance, ou considérer comme « actives » des facettes nées au même niveau (strictes exigences de Déf. 27).
- Conclure la fusion sur la seule vacuité Gabriel : sans le test de connectivité (ou sans le Théorème 5 pour la réduction MST), Gabriel seul sur-approuve les événements.

---

## Questions ouvertes / ambiguïtés

1. **Le Théorème 5 (p. 91, PDF 117) est hors tranche.** L'introduction du chapitre (page PDF 109) l'annonce : « le graphe de Gabriel contient toutes les fusions utiles et un arbre couvrant minimum construit sur ce graphe préserve, à chaque échelle, les K-polyèdres ». L'énoncé exact (pondération du MST, gestion des ex æquo, forêt vs arbre) doit être lu pages PDF 115+ avant de figer le pipeline « candidats → tri → union-find » de la V4.
2. **Le théorème de Jung n'apparaît pas dans cette tranche.** La borne « rayon miniball ≤ f(diamètre) » utile pour dimensionner s = 6/8/10 dans la WSPD devra être trouvée ailleurs dans le manuscrit (ou invoquée comme résultat externe) ; ici seul le Fait 12 (support ≤ p+1) est disponible.
3. **§ 9.1, partition des (K−1)-simplexes et poids $w_{x\tau} = S_\tau / T_x$ : hors tranche** (chapitre 9). Rien dans les pages 95–114 n'en parle ; la laminarité citée dans CLAUDE.md s'appuie sur des pages ultérieures.
4. **Fin de la preuve du Théorème 4 non lue** : la page PDF 114 s'arrête au cadre (contraposée, intrus z, support S). Le mécanisme complet de connexion des facettes actives via z (chemin $\tau_1 \leftrightarrow_{\sigma_1^{z}} \eta_{1,2}^{z} \leftrightarrow_{\sigma_2^{z}} \tau_2$, visible en Figure 8.2) est déduit de la figure ; la preuve textuelle continue page PDF 115. À vérifier notamment : le traitement du cas où toutes les facettes actives ne contiennent pas S, et la construction exacte de $\eta_{1,2}^{z}$ (facette contenant z ?).
5. **« Fait 2 » et « Déf. 21 » (p. 58), « Théorème 2 » (p. 60), « Fait 1 / Déf. 3 » (p. 16)** sont invoqués mais définis hors tranche. En particulier la définition précise des K-polyèdres (Déf. 21) n'est ici que paraphrasée (§ 8.1.1).
6. **Prop. 1 (seuils critiques)** est en amont de la tranche (référencée par Prop. 2) ; la définition exacte de $\Theta_{K,p,1/2}^{\mathrm{cc}}$ et du seuil par « Θ > 0 » n'est pas re-donnée ici.
7. **Statut des K-séparants d'égal rayon simultané** : Déf. 27 rend K-séparant tout simplexe reliant deux composantes de $\Gamma_K(\mathcal{X})_{<r_\sigma}$ ; si plusieurs K-simplexes de même rayon r relient les mêmes composantes, tous sont K-séparants au sens de la définition (le graphe strict ignore leurs congénères de rayon r). La façon dont la hiérarchie/le MST tranche ces ex æquo n'est pas précisée dans cette tranche — probablement dans le Théorème 5 ou via la position générale ; ambiguïté à lever page PDF 115+.
8. **Les événements A_a^cc de § 7.5.2 sont admis** (« on admettra ») et § 7.5.3 est explicitement une « ébauche d'intuitions » (conjectures, $U_p$ non calculé) : rien de cette section ne doit être traité comme normatif pour la V4, seulement comme motivation du choix de K.
