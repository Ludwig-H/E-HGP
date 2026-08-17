# Lecture du manuscrit Hauseux — pages PDF 75 à 94

Fichier : `/home/user/E-HGP/docs/references/MANUSCRIT_THESE_HAUSEUX.pdf`.
Correspondance constante observée sur la tranche : **page PDF = page livre + 26** (PDF 75 = livre 49, PDF 86 = livre 60, PDF 94 = livre 68).

---

## 1. Structure section par section de la tranche

| Page PDF | Page livre | Contenu |
|---|---|---|
| 75 | 49 | Fin du § 5.4 / 5.4.1 « La méthode » (Partie I) : conclusion motivante — le Robust Single-Linkage (moteur de HDBSCAN) « ne reflète pas la topologie des densités K-NN lorsque K ≥ 2 » ; annonce de HGP-Clusterer. |
| 76 | 50 | Page blanche. |
| 77 | 51 | **Page de titre de la Partie II** : « La généralisation du Single-Linkage avec des interactions d'ordre supérieur ». Cadre euclidien X ⊂ R^p ; les trois axes hérités de la Partie I et leurs trois généralisations visées. |
| 78 | 52 | **Plan de la seconde partie** : chapitres 6 (HGP-Clusterer), 7 (Percolation), 8 (K-arbre couvrant minimal et mosaïque de Delaunay d'ordre K), 9 (Conclusion pratique : partition stricte, hors-euclidien, grande dimension, réduction de dimension). |
| 79 | 53 | **Chapitre 6** « HGP-Clusterer : la généralisation du Single-Linkage dans l'espace euclidien » — introduction : effet de chaînage, asymétrie points/arêtes du Robust Single-Linkage et de (H)DBSCAN ; le bon objet est le complexe de Čech. |
| 80 | 54 | Suite intro ch. 6 (K-polyèdres = composantes de (K−1)-simplexes ; recouvrements et non partitions ; contributions, reformulation de [65]) ; **§ 6.1** « Retour sur la non prise en compte de la densité » ; **§ 6.1.1** « L'échec du Robust Single-Linkage et de (H)DBSCAN sur le modèle de Hartigan » : exemple des six points A…F. |
| 81 | 55 | Fig. 6.1 (nuage + dendrogramme dégénéré) ; **§ 6.1.2** « La hiérarchie exacte de Hartigan lorsque K = 2 » : naissance des sept amas-segments au niveau r (Fig. 6.2). |
| 82 | 56 | Fusions au niveau r′ = (2√3/3) r par 3-intersections (Fig. 6.3) ; clôture au niveau r″ = AD/2 (Fig. 6.4) ; nécessité de relâcher l'hypothèse de partition. |
| 83 | 57 | **§ 6.2** « Du graphe géométrique au complexe de Čech : les K-polyèdres » ; **§ 6.2.1** « Le complexe de Čech » : **Définition 20**. |
| 84 | 58 | **§ 6.2.2** « Les K-polyèdres » : **Définition 21** (graphe Γ_K des (K−1)-simplexes, K-polyèdres) et **Définition 22** (HGP-Clusterer, θ_K^HGP). |
| 85 | 59 | Fig. 6.5 (2-polyèdres {A,B,C}, {C,D}, {D,E,F} qui se recouvrent) ; **§ 6.3** « K-polyèdres de Čech et amas discrets de forte densité K-NN » ; **§ 6.3.1** « Les régions témoins d'un simplexe » : T_r(σ). |
| 86 | 60 | **Théorème 2** « K-polyèdres de Čech ≡ amas discrets de forte densité K-NN » + début de preuve. |
| 87 | 61 | Fin de preuve du Théorème 2 ; remarque K = 1 ; **Synthèse** du chapitre 6 (annonce de l'« arbre minimum couvrant » généralisé, objet du ch. 8). |
| 88 | 62 | Page blanche. |
| 89 | 63 | **Chapitre 7** « Le phénomène de percolation au cœur des performances » — introduction : Θ^cc_{K,p}(λ), cc ∈ {core; dbscan; poly}, vitesse de percolation. |
| 90 | 64 | Fenêtre gaussienne μ = K + a√K quand K → +∞ ; excursions E_a d'un champ gaussien ; **Contributions** du ch. 7 ; convention r = 1/2. **§ 7.1** « Introduction à la percolation » (historique : Broadbent & Hammersley 1957 [70], revue Duminil-Copin [71]). |
| 91 | 65 | Percolation continue [6, 18] ; **§ 7.1.1** « Le cadre mathématique » : processus de Poisson X_λ, processus de Palm X_λ^0 = X_λ ∪ {0} ; **Définition 23** (probabilité de percolation Θ). |
| 92 | 66 | Invariance d'échelle Θ^cc_{K,p,r}(λ) = Θ^cc_{K,p,1/2}(λ (2r)^p) ; intensité normalisée μ = λ ω_p 2^{−p} ; traduction « percolation » des trois algorithmes : poly (niveaux L_K^λ puis dilatation), core (X_λ^core = L_K^λ ∩ X_λ^0 puis dilatation), asymétrie du Robust Single-Linkage. |
| 93 | 67 | DBSCAN = cœurs + points accessibles (même seuil critique que core, rappel différent) ; **Proposition 1** « Existence d'un seuil critique (et absence de régime fantôme) » + début de preuve (couplage). |
| 94 | 68 | Fin de preuve de Prop. 1 : monotonie, formule de Campbell–Palm, ergodicité, λ^cc_∃ = λ^cc_c ; début de la non-dégénérescence des seuils (comparaison au modèle booléen — la suite déborde de la tranche). |

---

## 2. Définitions et notations normatives

### 2.1 Notations préalables rappelées dans la tranche (définies plus tôt dans le manuscrit)

- **Def. 6, p. livre 18** et **Def. 8, p. livre 21** (référencées, non recopiées ici) : modèle de Hartigan des amas de forte densité ; amas *discrets*. Rappel opérationnel donné p. PDF 81 (livre 55) : « un point x ∈ X est *couvert* au niveau r par un amas de forte densité C ∈ H_{f̂_K}(r) si x est à moins de r de C. On note C^discret ⊆ X le cluster discret correspondant. »
- **Def. 7, p. livre 19** (référencée) : estimateur de densité K-NN ; ses ensembles de niveau supérieur, rappelés p. PDF 85 (livre 59) :
  L_K(r) ≜ { y ∈ R^p : |B̄(y,r) ∩ X| ≥ K }.
  Les clusters (continus) sont les composantes connexes de L_K(r) ; les clusters *discrets* sont les ensembles de points couverts (à moins de r) par ces clusters continus.
- **Def. 11, p. livre 26** (référencée) : Single-Linkage θ^∼ via l'analyse persistante des graphes géométriques G(X,r).
- B̄(x,r) : boule fermée de centre x, rayon r. ‖·‖ : norme euclidienne.

### 2.2 Définition 20 : Complexe de Čech [réfs. 37, 63, 64] — page PDF 83 (livre 57)

> Soit X ⊂ R^p un nuage fini et r ≥ 0. Le *complexe de Čech* de rayon r est le complexe simplicial
> Č(X, r) ≜ { σ ⊆ X : ⋂_{x ∈ σ} B̄(x,r) ≠ ∅ }.
> Autrement dit, un ensemble fini non vide σ ⊆ X est un simplexe de Č(X,r) si et seulement s'il existe un point y ∈ R^p situé à distance au plus r de tous les sommets de σ, i.e. σ ⊂ B̄(y,r).

**Remarque normative attenante** (même page) : le 1-squelette de Č(X,r) est le graphe géométrique G(X,r) : une arête {x, x′} apparaît exactement lorsque B̄(x,r) ∩ B̄(x′,r) ≠ ∅, c'est-à-dire lorsque ‖x − x′‖ ≤ 2r. Plus généralement, un K-simplexe de Č(X,r) encode une interaction entre K+1 points « qui peuvent être simultanément vus depuis une même boule de rayon r ». (C'est l'information de densité locale qui manque au Single-Linkage, source de l'effet de chaînage.)

**Lecture implicite « miniboule »** : la condition « ∃ y : σ ⊂ B̄(y,r) » équivaut à ρ(σ) ≤ r où ρ(σ) est le rayon de la plus petite boule englobante (miniboule) de σ. La tranche n'introduit **pas** explicitement la notation ρ(σ) ni le mot « miniboule » — la caractérisation est donnée sous la forme existentielle ci-dessus.

### 2.3 Définition 21 : Graphe des (K−1)-simplexes et K-polyèdres — page PDF 84 (livre 58)

> Soit K ∈ ⟦1, |X|⟧ et r ≥ 0. On note Č_{K−1}(X,r) l'ensemble des simplexes de dimension K−1 du complexe de Čech Č(X,r).
> On leur associe le graphe Γ_K(X,r) dont les sommets sont les éléments de Č_{K−1}(X,r), deux sommets σ et τ étant reliés dès que σ ∪ τ est encore un simplexe de Č(X,r).
> Un *K-polyèdre* de Č(X,r) est alors l'ensemble des points de X apparaissant dans une composante connexe de Γ_K(X,r).

**Remarque attenante** : pour K = 1, les sommets de Γ_1(X,r) sont les points de X, ses arêtes sont celles telles que ‖x − x′‖ ≤ 2r ; les 1-polyèdres sont les composantes connexes ordinaires de G(X,r) = Γ_1(X,r). Pour K = 2, les sommets de Γ_2(X,r) sont les arêtes de Č(X,r) et deux arêtes partageant un sommet deviennent adjacentes « dès que le triangle qu'elles forment apparaît ». Filiation : *clique percolation* [66], *q-connectivity* d'Atkin 1972 [67], *(up) face percolation* [68].

**Point important (à ne pas simplifier abusivement)** : la définition ne restreint pas l'adjacence σ ∼ τ aux paires partageant K−1 sommets. Toute paire σ, τ ∈ Č_{K−1}(X,r) avec σ ∪ τ ∈ Č(X,r) est adjacente (σ ∪ τ peut être de cardinal K+1 jusqu'à 2K si les simplexes sont disjoints). La remarque K = 2 décrit le cas typique (arêtes partageant un sommet, fusion par triangle), pas une restriction.

### 2.4 Définition 22 : HGP-Clusterer (HGP pour Hypergraphe-Percol') — page PDF 84 (livre 58)

> Pour K ∈ ⟦1, |X|⟧, on définit *Hypergraphe-Percol'* (acronyme : HGP) par
> θ_K^{HGP}(r) ≜ { K-polyèdres de Č(X,r) }, pour tout r ≥ 0.
> Autrement dit, HGP-Clusterer observe l'évolution des K-polyèdres au fil de la filtration du complexe de Čech.

**Remarque attenante** : la filtration r ↦ Č(X,r) est croissante, donc le graphe Γ_K(X,r) l'est aussi. **Les K-polyèdres ne peuvent ainsi que croître et fusionner lorsque r augmente.** Pour K = 1, on retrouve exactement le Single-Linkage de la partie I.

### 2.5 Région témoin d'un simplexe (§ 6.3.1) — page PDF 85 (livre 59)

> Pour tout (K−1)-simplexe σ ∈ Č_{K−1}(X,r), on introduit
> T_r(σ) ≜ ⋂_{x ∈ σ} B̄(x,r).
> On l'appelle la *région témoin* du simplexe σ au niveau r. Par construction, T_r(σ) est une intersection finie de boules fermées : c'est donc un compact convexe, en particulier connexe.

(Attention à l'homonymie : ce T_r(σ) n'a rien à voir avec le T_x de la partition de l'unité w_{x,τ} = S_τ/T_x du § 9.1, hors de cette tranche.)

### 2.6 Définition 23 : Probabilité de percolation Θ — page PDF 91 (livre 65)

> Soient K ∈ N* et cc ∈ {poly, core, dbscan, …} une famille de composantes croissante en une variable rayon r.
> 1. poly désigne les polyèdres.
> 2. core désigne les cœurs (au sens du Robust Single-Linkage).
> 3. dbscan désigne les composantes connexes de DBSCAN [49].
> On définit la probabilité de percolation par
> Θ^{cc}_{K,p,r}(λ) ≜ P[ 0 ∈ une composante connexe infinie de cc construite sur X_λ^0 ].
> Lorsque la composante infinie existe et est unique, cette quantité est la probabilité (de Palm) qu'un point typique appartienne à la composante géante.

**Remarque attenante** : les *cœurs* du Robust Single-Linkage ne sont pas les cœurs au sens usuel de la théorie des graphes (plus grands sous-graphes induits où chaque sommet a un degré ≥ K, par élagage récursif) ; les cœurs du Robust Single-Linkage n'ont **qu'un seul tour d'élagage**.

### 2.7 Notations du cadre percolation (§ 7.1.1, pages PDF 91–92, livre 65–66)

- X_λ : processus ponctuel de Poisson homogène d'intensité λ > 0 sur R^p ; X_λ^0 ≜ X_λ ∪ {0} (processus de Palm ; la loi de X conditionnée à 0 ∈ X est la loi de X ∪ {0} ; formule de Campbell–Palm).
- Invariance d'échelle : pour tout r > 0, Θ^{cc}_{K,p,r}(λ) = Θ^{cc}_{K,p,1/2}(λ (2r)^p). Convention du chapitre : **r = 1/2 fixé** ; l'analyse persistante se fait sur l'intensité λ à rayon fixé.
- Intensité normalisée : μ ≜ λ ω_p 2^{−p}, où ω_p ≜ |B(0,1)| est le volume de la boule unité de R^p (μ = nombre moyen de points dans une boule de rayon 1/2).
- Niveaux K-NN poissoniens : L_K^λ ≜ { y ∈ R^p : |B̄(y,1/2) ∩ X_λ^0| ≥ K }.
- Dilatation : δ_r(A) ≜ { x ∈ R^p : d(x,A) ≤ r } ; C_∞(·) : réunion des composantes connexes non bornées.
- Traduction des trois algorithmes :
  - **poly** : si C_∞ est la composante non bornée de L_K^λ, Θ^{poly}_{K,p,1/2}(λ) = P[0 ∈ δ_{1/2}(C_∞)] (événement faux si aucune composante non bornée). Point clé : 0 n'a pas besoin d'être lui-même dans L_K^λ ; il suffit qu'il soit *couvert* (dilatation δ_{1/2}) par la composante de forte densité.
  - **core** : points cœurs X_λ^{core} ≜ { x ∈ X_λ^0 : |B̄(x,1/2) ∩ X_λ^0| ≥ K } = L_K^λ ∩ X_λ^0 ; le Robust Single-Linkage identifie les composantes connexes de δ_{1/2}(X_λ^{core}). Les polyèdres, eux, « n'appliquent de dilatation qu'une fois les composantes de L_K^λ correctement identifiées » — c'est l'asymétrie fondamentale.
  - **dbscan** : (H)DBSCAN ajoute aux composantes de cœurs les points accessibles (à distance au plus 1/2 d'un cœur) ; même structure de cœurs et même seuil critique que le Robust Single-Linkage (renvoi à une Prop. 2 hors tranche), mais pas la même fonction de rappel au niveau 1−ε.
- Fenêtre gaussienne (annonce, page PDF 90, livre 64) : lorsque K → +∞, la bonne fenêtre d'intensité normalisée est μ = K + a√K ; dans cette fenêtre les niveaux de forte densité K-NN sont remplacés par les excursions E_a = { x ∈ R^p : G_p(x) ≥ −a } d'un champ gaussien stationnaire.

---

## 3. Théorèmes, propositions, lemmes

### 3.1 Théorème 2 : K-polyèdres de Čech ≡ amas discrets de forte densité K-NN — page PDF 86 (livre 60)

> Soit X ⊂ R^p un nuage fini de taille n, K ∈ ⟦1, n⟧ et r ≥ 0. Alors les K-polyèdres du complexe de Čech Č(X,r) sont en correspondance avec les amas discrets de forte densité H^{discrets}_{f̂_K}(r).
> Plus précisément, si C est une composante connexe de
> L_K(r) = { y ∈ R^p : |B̄(y,r) ∩ X| ≥ K }
> et si P_C désigne le K-polyèdre correspondant, alors
> P_C = C^{discret} = { x ∈ X : ∃ y ∈ C, ‖x − y‖ ≤ r }.
> En particulier,
> θ_K^{HGP}(r) = H^{discrets}_{f̂_K}(r) pour tout r ≥ 0,
> et θ_1^{HGP} coïncide avec le Single-Linkage (θ_1^{HGP} = θ^∼, voir Def. 11, p. 26).

**Preuve (complète dans la tranche, pages PDF 86–87 ; elle éclaire directement l'algorithmique)** :

1. *Recouvrement du niveau supérieur par les régions témoins* :
   L_K(r) = ⋃_{σ ∈ Č_{K−1}(X,r)} T_r(σ).
   En effet, si y ∈ L_K(r), la boule B̄(y,r) contient au moins K points de X ; en en choisissant K on obtient un (K−1)-simplexe σ tel que y ∈ T_r(σ). Réciproquement, si y ∈ T_r(σ) pour σ de cardinal K, alors B̄(y,r) contient les K sommets de σ, donc y ∈ L_K(r).
2. *Γ_K est le graphe d'intersection de ce recouvrement* : pour deux (K−1)-simplexes σ et τ,
   T_r(σ) ∩ T_r(τ) ≠ ∅ ⟺ ⋂_{x ∈ σ ∪ τ} B̄(x,r) ≠ ∅ ⟺ σ ∪ τ ∈ Č(X,r),
   ce qui est précisément la règle d'adjacence définissant Γ_K(X,r).
3. *Bijection composantes ↔ polyèdres* : la famille { T_r(σ) } étant finie et formée de compacts **connexes**, les composantes connexes de son union correspondent exactement aux unions des régions témoins de chaque composante connexe du graphe d'intersection. D'où une bijection naturelle entre composantes connexes de L_K(r) et K-polyèdres de Č(X,r).
4. *P_C ⊆ C^{discret}* : si x ∈ P_C, x appartient à un (K−1)-simplexe σ avec T_r(σ) ⊆ C ; en choisissant y ∈ T_r(σ), comme x est un sommet de σ, ‖x − y‖ ≤ r, donc x est couvert par C.
5. *C^{discret} ⊆ P_C* : si x ∈ C^{discret}, il existe y ∈ C avec ‖x − y‖ ≤ r. Comme y ∈ L_K(r), B̄(y,r) ∩ X contient au moins K points, dont x. On choisit K−1 autres points x_2, …, x_K ∈ B̄(y,r) ∩ X et on forme σ = {x, x_2, …, x_K} ∈ Č_{K−1}(X,r). Comme y ∈ T_r(σ) ⊆ C, σ appartient à la composante de Γ_K(X,r) associée à C ; l'un de ses sommets étant x, on obtient x ∈ P_C. □

**Remarque post-preuve (page PDF 87)** : pour K = 1, les régions témoins sont les boules B̄(x,r) centrées sur les données ; le graphe d'intersection redevient le graphe géométrique.

**Synthèse du chapitre 6 (page PDF 87, livre 61)** — normative pour la suite : HGP généralise exactement (1) le modèle géométrique d'analyse persistante (graphe géométrique → complexe de Čech, composante connexe → K-polyèdre) et (2) le modèle statistique de Hartigan (correspondance niveau par niveau, Théorème 2). La définition reste théorique ; il reste à retrouver le troisième volet : « définir HGP-Clusterer au moyen d'un objet combinatoire (beaucoup) plus mince jouant le rôle de l'**arbre minimum couvrant**. C'est sur cet arbre couvrant généralisé qu'est calculée en pratique la hiérarchie K-NN des amas de forte densité. » (objet du chapitre 8, hors tranche). Note de bas de page : alternative de [69] (graphe géométrique + densité échantillonnée le long des arêtes).

### 3.2 Proposition 1 : Existence d'un seuil critique (et absence de régime fantôme) — page PDF 93 (livre 67)

> Pour tout p ≥ 2, K ∈ N* et cc ∈ {poly, core, dbscan}, la fonction λ ⟼ Θ^{cc}_{K,p,1/2}(λ) est croissante. On définit le seuil critique par
> λ_c^{cc}(K,p) ≜ inf{ λ > 0 : Θ^{cc}_{K,p,1/2}(λ) > 0 }.
> Alors 0 < λ_c^{cc}(K,p) < +∞, et
> Θ^{cc}_{K,p,1/2}(λ) = 0 si λ < λ_c^{cc}(K,p), tandis que Θ^{cc}_{K,p,1/2}(λ) > 0 si λ > λ_c^{cc}(K,p).
> De plus, ce seuil coïncide avec le seuil d'existence d'une composante infinie. Soit
> λ_∃^{cc}(K,p) ≜ inf{ λ > 0 : P_λ(∃ une composante infinie de type cc) > 0 },
> alors λ_∃^{cc}(K,p) = λ_c^{cc}(K,p).
> (Il n'existe pas d'intervalle non vide d'intensités λ pour lesquelles il existerait presque-sûrement une composante infinie qui soit de taille négligeable.)

**Preuve (pages PDF 93–94, partielle dans la tranche)** :
- *Croissance* par couplage de processus de Poisson : si λ′ ≥ λ, on construit X_{λ′} = X_λ ∪ X_{λ′−λ} (processus indépendants). **Ajouter des points ne supprime ni simplexe de Čech, ni point cœur, ni point accessible** ; les composantes ne peuvent que croître ou fusionner.
- *Identité des seuils* : pour une configuration localement finie ξ, I_∞^{cc}(ξ) = points de ξ rattachés à une composante infinie (pour poly : x ∈ I_∞^{poly}(ξ) ssi il existe une composante non bornée C de L_K(ξ) ≜ { y : |B(y,1/2) ∩ ξ| ≥ K } avec d(x,C) ≤ 1/2). Avec Q = [0,1]^p et N_∞^{cc}(Q) ≜ |I_∞^{cc}(X_λ) ∩ Q|, la formule de Campbell–Palm donne E[N_∞^{cc}(Q)] = λ |Q| Θ^{cc}_{K,p,1/2}(λ) = λ Θ^{cc}_{K,p,1/2}(λ). Si Θ = 0, alors N_∞^{cc}(z+Q) = 0 p.s. pour tout z ∈ Z^p, donc (réunion dénombrable) aucune composante infinie p.s. Réciproquement, une composante infinie de probabilité > 0 fournit un point de I_∞^{cc} dans un cube ; par stationnarité et Campbell–Palm, Θ > 0. L'événement d'existence étant invariant par translation, il est de probabilité 0 ou 1 par ergodicité du processus de Poisson, d'où λ_∃^{cc} = λ_c^{cc}.
- *Non-dégénérescence des seuils* (0 < λ_c < +∞) : la stricte positivité vient de la comparaison avec le modèle booléen classique — **la preuve se poursuit au-delà de la page PDF 94** (hors tranche).

### 3.3 Renvois à des résultats hors tranche

- **Prop. 2** (mentionnée page PDF 93) : le K-Robust Single-Linkage et (H)DBSCAN ont le même seuil critique de percolation (même structure de cœurs). Énoncé hors tranche.
- **Théorème 2 s'appuie sur** Def. 6 (p. livre 18), Def. 7 (p. 19), Def. 8 (p. 21), Def. 11 (p. 26) — Partie I, hors tranche.
- Le chapitre 8 (K-arbre couvrant minimal, mosaïque de Delaunay d'ordre K, « simplexes K-séparants », réf. [62]) et le chapitre 9 (partition stricte, § 9.1, poids w_{x,τ} = S_τ/T_x) sont **annoncés** dans le plan (page PDF 78) mais **hors de cette tranche**.

---

## 4. Points ciblés par la mission

### 4.1 Ordre K
Défini opérationnellement par les Définitions 21–22 : K ∈ ⟦1, |X|⟧ ; les sommets de Γ_K sont les (K−1)-simplexes (K points), l'adjacence est l'appartenance de σ ∪ τ au complexe de Čech. K = 1 redonne le Single-Linkage. La feuille de route V4 « forêt complète K = 10 » signifie donc, dans le vocabulaire du manuscrit, disposer des hiérarchies θ_K^{HGP} pour K = 1, …, 10 — chaque K étant une filtration de graphe Γ_K distincte sur des sommets qui sont des K-uplets.

### 4.2 Simplexes et filtration (rayon d'apparition)
La tranche ne nomme pas ρ(σ), mais la Définition 20 en donne la caractérisation exacte : σ entre dans la filtration au premier r tel qu'il existe y avec σ ⊂ B̄(y,r), c'est-à-dire au **rayon de la plus petite boule englobante de σ** (miniboule). Conséquences vérifiées sur l'exemple des six points :
- une arête {x,x′} naît à r = ‖x − x′‖/2 (les sept segments de longueur 2r naissent au niveau r, Fig. 6.2) ;
- un triangle équilatéral de côté 2r naît à r′ = (2√3/3) r = (côté)/√3, son **circumrayon** — cas *aigu* : rayon de Čech = circumrayon (implicite, via l'exemple de la Fig. 6.3) ;
- le triangle ACD naît à r″ = AD/2, la **demi-longueur de sa plus longue arête** — cas *obtus* (implicite, via la note de bas de page de la page PDF 82 : « à ce niveau, le triangle ACD apparaît connectant les arêtes AC et CD »).
La dichotomie aigu/obtus et le théorème de Jung ne sont **pas énoncés** dans cette tranche ; seuls les deux cas numériques ci-dessus les illustrent.

### 4.3 Miniboule / circumradius / rayon de couverture / Jung
Voir 4.2 : présents seulement implicitement (Def. 20 + valeurs r′ = 2√3 r/3 et r″ = AD/2). Aucune mention explicite du théorème de Jung dans les pages 75–94.

### 4.4 Événements de naissance / fusion
Illustrés sur l'exemple K = 2 (§ 6.1.2, pages PDF 81–82) :
- **Naissance** d'un amas 2-NN au niveau r : apparition simultanée des sept segments AB, BC, AC, CD, DE, EF, DF (chacun est un 1-simplexe, sommet de Γ_2). Par convention (Fig. 6.2), le multi-ensemble des feuilles apparaît au niveau 0.
- **Fusion** au niveau r′ = (2√3/3) r : des 3-intersections de boules apparaissent (triangles ABC et DEF dans le Čech), rendant adjacentes les arêtes qui les composent ; les 2-polyèdres deviennent {A,B,C}, {C,D}, {D,E,F} (Fig. 6.5), **qui se recouvrent** (C et D appartiennent à deux polyèdres).
- **Clôture** au niveau r″ = AD/2 : « l'apparition simultanée des quatre points de tangence relie les trois amas » ; le triangle ACD apparaît connectant les arêtes AC et CD (et symétriquement BCD, ACD/BCD côté droit — la figure marque quatre points rouges).
Structurellement (remarque de la Def. 22) : les K-polyèdres **ne peuvent que croître et fusionner** quand r augmente — c'est ce qui fonde l'existence d'une hiérarchie (dendrogramme de recouvrements) et, en aval, d'une forêt de fusion.

### 4.5 Arbre / forêt de fusion
Pas encore construits dans la tranche. Deux annonces normatives :
- Plan (page PDF 78) : le chapitre 8 extraira « le K-arbre minimum couvrant (qui contient toute l'information hiérarchique nécessaire à HGP-Clusterer) » de « la mosaïque de Delaunay d'ordre supérieur, cette structure portant les simplexes K-séparants ».
- Synthèse du ch. 6 (page PDF 87) : l'objet pratique est « un objet combinatoire (beaucoup) plus mince jouant le rôle de l'arbre minimum couvrant ».

### 4.6 Partition des (K−1)-simplexes (§ 9.1) et poids w_{x,τ} = S_τ/T_x
**Absents de la tranche** (chapitre 9, plus loin dans la Partie II). Seule trace dans la tranche : le plan (page PDF 78) annonce le chapitre 9 (« imposer un clustering qui soit une partition stricte des données X et non un recouvrement »), et la page PDF 84–85 établit le fait générateur : pour K ≥ 2 les polyèdres se recouvrent (C ∈ {A,B,C} ∩ {C,D}).

### 4.7 Algorithmes proposés et coûts
Aucun algorithme effectif ni analyse de coût dans la tranche : le chapitre 6 est explicitement « toute théorique » (dit deux fois, pages PDF 80 et 87) ; le chapitre 7 traite des performances **statistiques** (consistance fractionnaire), pas du coût de calcul. Les optimisations algorithmiques sont réservées au chapitre 8.

### 4.8 Chapitre 7 : ce que la percolation dit de l'objet
- Le Single-Linkage n'est pas consistant en dimension p ≥ 2 (phénomène de seuil) ; en p = 1 seulement, pas de phénomène de seuil. On ne récupère qu'une **fraction** des points d'un amas avant fusion ; cette fraction asymptotique se lit dans λ ⟼ Θ^{cc}_{K,p}(λ).
- Indice comparatif : la **vitesse de percolation** (définie plus loin, hors tranche) ; conclusion annoncée : « les polyèdres percolent plus vite dans les amas denses, et sont donc *fractionnellement* plus consistants » — obstruction ponctuelle pour les cœurs, obstruction de rayon fini pour (H)DBSCAN, obstruction géométrique de connexion pour les K-polyèdres.
- Régime K → +∞ : fenêtre μ = K + a√K, limite en excursions gaussiennes E_a = { x : G_p(x) ≥ −a }.

---

## 5. Pertinence pour la conception V4

### Ce que la tranche impose

1. **L'objet exact à calculer est θ_K^{HGP}(r) = { K-polyèdres de Č(X,r) }, r ≥ 0** (Def. 22). Toute V4 doit produire, pour chaque K ≤ 10, la trajectoire de fusion des composantes connexes de Γ_K(X,r) le long de la filtration. Le Théorème 2 fournit l'oracle sémantique équivalent (composantes de L_K(r) + couverture à distance r) : deux définitions indépendantes du même objet, idéales pour un juge/oracle V4 (calcul par régions témoins vs calcul par Γ_K).
2. **Sémantique d'adjacence exacte de Γ_K** : σ ∼ τ ⟺ σ ∪ τ ∈ Č(X,r) ⟺ ⋂_{x ∈ σ∪τ} B̄(x,r) ≠ ∅ ⟺ (rayon de miniboule de σ ∪ τ) ≤ r. L'union σ ∪ τ n'est **pas** limitée à K+1 points ; une implémentation qui ne testerait que les paires partageant K−1 sommets (fusion par (K+1)-simplexe, i.e. « up face percolation » stricte) doit prouver l'équivalence pour la connectivité des composantes — la tranche ne fournit pas cette réduction. C'est un point à trancher avec le chapitre 8 (simplexes « K-séparants ») avant de figer les événements q3/q4 de la V4.
3. **Rayons d'événements** : les naissances/fusions se produisent aux rayons de miniboules d'ensembles de points (arête : demi-distance ; triangle aigu : circumrayon ; triangle obtus : demi-arête max). Les cibles V4 (q2 = arête max ; q3 = triangle aigu avec témoin x ; q4 avec x, y) correspondent exactement aux valeurs critiques de miniboules de 2, 3, 4 points — la tranche valide les deux premiers cas numériquement (2√3 r/3 et AD/2).
4. **Monotonie** : la filtration est croissante ; les polyèdres ne font que croître et fusionner. Toute structure d'événements V4 peut donc être triée par rayon et traitée par union-find ; aucun événement de scission n'existe.
5. **Recouvrement, pas partition** : pour K ≥ 2 les K-polyèdres se recouvrent (un point peut appartenir à plusieurs polyèdres au même niveau ; ex. C ∈ {A,B,C} et {C,D}). La « forêt HGP » V4 est une forêt sur les (K−1)-simplexes (sommets de Γ_K), pas directement sur les points ; la projection sur les points produit des recouvrements. La partition stricte est un post-traitement (chapitre 9), à ne pas mélanger avec le calcul de la forêt.
6. **Correspondance discret/continu** (Théorème 2) : P_C = C^discret = points à distance ≤ r d'une composante de L_K(r). Un test V4 peut vérifier, sur petites tailles, l'égalité entre (i) composantes de Γ_K et (ii) clusters discrets calculés par un oracle géométrique sur L_K(r) — deux chemins de calcul réellement indépendants, conforme à la doctrine oracle du dépôt.
7. **Simultanéité des événements** : l'exemple de la Fig. 6.4 (« apparition simultanée des quatre points de tangence ») montre que des événements de même rayon critique existent dans des configurations symétriques — la V4 doit définir un ordre total reproductible (tie-breaking) sans supposer la position générale, en cohérence avec le profil u16/refus des dégénérescences du dépôt.

### Ce que la tranche interdit ou met en garde

- **Interdit de confondre HGP avec Robust Single-Linkage/(H)DBSCAN** : la contrainte de densité doit porter sur la *connectivité* (simplexes), pas seulement sur les sommets. Un raccourci « cœurs + arêtes » reproduit exactement le contre-exemple des six points (dendrogramme plat à r au lieu de la hiérarchie r < r′ < r″).
- **Interdit de forcer une partition par niveau** pendant le calcul de la hiérarchie (page PDF 80 : « Nous ne chercherons pas ici à forcer une partition à chaque niveau »).
- La tranche ne fournit **aucune borne de coût ni aucun algorithme** : tout choix algorithmique V4 (WSPD, dual-tree, élimination par témoins) est une optimisation qui doit préserver l'objet des Def. 20–22 et du Théorème 2, jamais le redéfinir (conforme à la chaîne d'autorité du dépôt : l'optimisation « ne modifie ni l'objet, ni les niveaux, ni les inclusions »).
- Le chapitre 7 justifie le *choix* de HGP face aux concurrents (consistance fractionnaire supérieure) mais n'ajoute aucune contrainte d'implémentation ; il fournit en revanche un cadre de test statistique (régimes uniformes ~ Poisson homogène ; seuils critiques λ_c) exploitable pour les régimes « uniforme » des campagnes n = 8000/16000/32000.

---

## Questions ouvertes / ambiguïtés

1. **Réduction de l'adjacence de Γ_K** : la Def. 21 admet l'adjacence σ ∼ τ pour tout σ ∪ τ ∈ Č(X,r), y compris |σ ∪ τ| > K+1. Or si σ ∪ τ ∈ Č(X,r) avec |σ ∪ τ| = m > K+1, tous les sous-ensembles de σ ∪ τ de taille K+1 sont aussi dans Č(X,r) (propriété de complexe simplicial), ce qui suggère qu'un chemin de fusions par (K+1)-simplexes existe alors dans Γ_K entre σ et τ. La tranche ne démontre pas cette équivalence pour les composantes connexes ; elle est vraisemblablement traitée au chapitre 8 (« simplexes K-séparants »). **À vérifier avant de restreindre les événements V4 aux seuls (K+1)-simplexes (q3/q4).**
2. **Valeur de AD** : la configuration des six points donne CD = 2r (triangles « à distance 2r l'un de l'autre »), mais la valeur numérique de AD (donc de r″ = AD/2) n'est pas explicitée dans le texte de la tranche. Géométriquement, avec deux triangles équilatéraux de côté 2r opposés par le sommet et CD = 2r, on peut la calculer, mais le manuscrit ne la donne pas ici.
3. **« Distance 2r l'un de l'autre »** : l'interprétation exacte (distance CD entre les sommets se faisant face) est déduite des figures, pas d'une définition formelle de la configuration.
4. **ρ(σ), miniboule, Jung, dichotomie aigu/obtus** : aucune de ces notions n'est *nommée* dans les pages 75–94 ; elles n'y existent que sous forme de la caractérisation existentielle de la Def. 20 et des deux valeurs numériques r′, r″. Les énoncés normatifs correspondants (s'ils existent) sont ailleurs (probablement chapitre 8).
5. **§ 9.1, partition des (K−1)-simplexes, poids w_{x,τ} = S_τ/T_x** : hors tranche. Attention à la collision de notation avec la région témoin T_r(σ) du § 6.3.1.
6. **Prop. 2** (égalité des seuils critiques core/dbscan) : citée page PDF 93 mais énoncée hors tranche.
7. **Convention du niveau 0** : la Fig. 6.2b indique « Par convention, le multi-ensemble des feuilles apparaît au niveau 0 » — la nature exacte des feuilles du dendrogramme HGP pour K ≥ 2 (les (K−1)-simplexes ? en multi-ensemble ?) est illustrée (feuilles = les sept segments) mais non définie formellement dans la tranche ; la naissance « réelle » d'un sommet de Γ_K est au rayon de sa miniboule (les segments naissent à r sur la figure, avec traits pointillés au-dessus des naissances).
8. **Fonction de rappel au niveau 1−ε** (page PDF 93) : la notion est utilisée pour distinguer core et dbscan mais sa définition formelle n'apparaît pas dans la tranche.
