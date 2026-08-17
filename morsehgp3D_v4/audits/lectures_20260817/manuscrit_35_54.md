# Lecture normative — MANUSCRIT_THESE_HAUSEUX.pdf, pages PDF 35–54 (pages imprimées 9–28)

Tranche : ouverture de la Partie I (« Du Single-Linkage à ses fondements »), chapitre 2 complet (« Le Single-Linkage : trois points de vue »), chapitre 3 jusqu'au Fait 7 (coupé en pleine page 28 ; la suite du Fait 7 est page PDF 55).

Convention de citation : « p. PDF n » = numéro de page du fichier PDF ; « p. n » = numéro imprimé dans le manuscrit (décalage constant de 26 : p. PDF 35 = p. 9).

---

## 1. Structure section par section

| p. PDF | p. imprimée | Contenu |
|---|---|---|
| 35 | 9 | **Partie I — Du Single-Linkage à ses fondements** : page d'ouverture. Paragraphes « Le cadre euclidien X ⊂ R^p » et « Le cadre axiomatique ». |
| 36 | 10 | Paragraphe « Le Single-Linkage aujourd'hui » : formule min-max de d_SL, effet de chaînage, HDBSCAN, annonce du chapitre 5 (SemanticKITTI). |
| 37 | 11 | **Chapitre 2 — Le Single-Linkage : trois points de vue (heuristique, géométrique & statistique)**. Intro : k-Means vs Single-Linkage, génératif vs discriminatif. |
| 38 | 12 | Fin intro (complexité O(n²) pire cas), « Contributions », **§ 2.1 Introduction au Single-Linkage**, début de la **Définition 1**. |
| 39 | 13 | Fig. 2.1 (arbre de Darwin), fin de la Définition 1, Remarque (versions à k clusters / seuillée à ε). |
| 40 | 14 | **§ 2.2 Le Single-Linkage en pratique : l'arbre minimum couvrant** ; **§ 2.2.1 Différents algorithmes pour l'arbre minimum couvrant** (Borůvka, Kruskal, Jarník–Prim–Dijkstra) ; « Améliorations (théoriques) et avancée récente » ; début **§ 2.2.2 Graphe de Gabriel et triangulation de Delaunay**. |
| 41 | 15 | **Définitions 2 (RNG), 3 (Gabriel), 4 (Delaunay)**, Fig. 2.2. |
| 42 | 16 | **Fait (connu) 1** (chaîne d'inclusions MST ⊆ RNG ⊆ Gabriel ⊆ Delaunay) ; « Le cas du plan p = 2 » ; **§ 2.3 Le Single-Linkage en théorie : persistance de graphes géométriques** ; **Définition 5** (graphe géométrique). |
| 43 | 17 | **Fait (connu) 2** (MST et sous-niveaux) + preuve ; **Fait (connu) 3** (coupe et MST) ; **Théorème 1** (graphe géométrique ≡ MST élagué). |
| 44 | 18 | Remarque sur la position générale (renvoi Déf. 26, p. 84 pour les hypergraphes), Fig. 2.3, usage pratique du Théorème 1 ; **§ 2.4 Le modèle statistique du Single-Linkage : Hartigan et les amas de forte densité** ; **§ 2.4.1** ; début **Définition 6**. |
| 45 | 19 | Fin Définition 6, Fig. 2.4 ; **§ 2.4.2 L'estimateur de densité des K-Plus Proches Voisins (K-NN)** ; **Définition 7**. |
| 46 | 20 | Fin Définition 7 + 3 Remarques (dont le changement de variable λ ↔ r) ; **§ 2.4.3 Lien entre le Single-Linkage et la hiérarchie de Hartigan 1-NN** ; **§ 2.4.4 Amas de forte densité *discrets*** (début). |
| 47 | 21 | **Définition 8** (amas discrets) + 3 Remarques (couverture par dilatation, RSL vs DBSCAN, non-partition pour K ≥ 2) ; début « Synthèse ». |
| 48 | 22 | Fin « Synthèse » : triple identité C^{r_t} = composantes de G(X, r_t) = composantes de MST_{≤ r_t} = H^discrets_{f̂₁}(λ_t) ; annonce de la généralisation K ≥ 2. |
| 49 | 23 | **Chapitre 3 — Le Single-Linkage : un point de vue axiomatique**. Intro (Kleinberg, Carlsson & Mémoli, Culbertson–Guralnik–Stiller : recouvrements, crible catégoriel), « Contributions ». |
| 50 | 24 | **§ 3.1 Rappels : partitions, relations d'équivalence** ; **§ 3.1.1 Le clustering, ou partitionnement** ; **Définition 9**. |
| 51 | 25 | **§ 3.2 Le théorème d'impossibilité de Kleinberg** ; **Fait (connu) 4** ; **§ 3.3 Le besoin d'ajouter une vue hiérarchique** ; début **Définition 10**. |
| 52 | 26 | Fin Définition 10 ; reformulation du Single-Linkage comme dendrogramme θ^SL ; début **Définition 11**. |
| 53 | 27 | Fin Définition 11 ; **Fait (connu) 5** (θ^SL ≡ θ^∼) ; **§ 3.3.1 Équivalence entre les dendrogrammes et les ultramétriques** ; **Définition 12** (ultramétrique) ; début **Fait (connu) 6**. |
| 54 | 28 | Fin Fait 6 (Éq. 3.1) ; **Définition 13** (clustering hiérarchique au sens de Carlsson & Mémoli) ; début **Fait (connu) 7** (Single-Linkage, unique solution fonctorielle — les conditions 1 et 2 seulement ; la suite est hors tranche). |

---

## 2. Définitions et notations normatives (énoncés complets)

### Notations d'ambiance (p. PDF 35–36)

- Nuage fini X ⊂ R^p, espace euclidien de dimension p ∈ N*, distance d(x, x') = ‖x − x'‖. Espace métrique fini (X, d), n points.
- Poids d'une arête {x, x'} du graphe complet : ‖x − x'‖.
- Distance (ultramétrique) de Single-Linkage entre deux points (p. PDF 36, p. 10), vue min-max sur les chaînes ω : x ⇝ x' :

  d_SL(x, x') = min_{ω = e₁,…,e_l} max_{a ∈ {1;…;l}} |e_a|.

### Définition 1 : L'arbre hiérarchique du Single-Linkage (p. PDF 38–39, p. 12–13)

Étant donné un espace métrique fini (X, d) de n points (dont on supposera les distances deux à deux distinctes), l'algorithme construit un dendrogramme (un arbre hiérarchique indexé par un niveau r) de la façon suivante :
- **Initialisation** (t = 0, niveau r₀ ≜ 0). On part de la partition triviale en singletons, C^{r₀} ≜ {{x_i}}_{i=1}^n.
- **Itération** (étape t, niveau r_t). On fusionne les deux clusters C et C' présents dans le clustering C^{r_{t−1}} de l'étape précédente qui minimisent la distance de *single linkage* :

  d_SL(C, C') = min_{x ∈ C, x' ∈ C'} d(x, x').

  On obtient alors un nouveau clustering C^{r_t} constitué de n − t clusters associé au niveau **r_t ≜ ½ d_SL(C, C')**.
- **Arrêt**. L'algorithme se termine à l'étape n − 1, lorsque tous les points sont réunis dans un unique cluster global (la racine de l'arbre) associé au niveau r_connect ≜ r_{n−1}.

**Remarque** (p. PDF 39) : version originale = on reçoit k et on renvoie le clustering de l'étape n − k. Version seuillée à ε : clustering de l'étape t, unique entier vérifiant r_t ≤ ε < r_{t+1} (convention r_n ≜ +∞).

⚠️ **Convention capitale : le niveau est un RAYON, moitié de la distance de fusion** (r_t = ½ d_SL). Toute la suite (graphes géométriques d ≤ 2r, modèle booléen, K-NN) est cohérente avec cette convention.

### Définition 2 : Graphe de voisinage relatif (RNG) (p. PDF 41, p. 15)

Le graphe de voisinage relatif (*relative neighborhood graph*) relie deux points x, x' ∈ X si l'intersection de leurs boules ouvertes de rayon ‖x − x'‖ est vide de tout point de X :

∀z ∈ X \ {x, x'},  max(‖x − z‖, ‖x' − z‖) ≥ ‖x − x'‖.

### Définition 3 : Graphe de Gabriel (p. PDF 41, p. 15)

Le graphe de Gabriel relie deux points x, x' ∈ X si la boule ouverte de diamètre [x, x'] ne contient aucun point de X :

∀z ∈ X \ {x, x'},  ‖x − z‖² + ‖x' − z‖² ≥ ‖x − x'‖².

### Définition 4 : Triangulation de Delaunay (p. PDF 41, p. 15)

Le graphe de Delaunay (le 1-squelette de la triangulation de Delaunay) relie deux points x, x' ∈ X s'il existe au moins une boule ouverte, vide de tout point de X, dont la frontière contient simultanément x et x'.

### Définition 5 : Graphe géométrique (au sens de Gilbert-Penrose) (p. PDF 42, p. 16)

Soit (X, d) un espace métrique. Étant donné un rayon r ≥ 0, le graphe géométrique G(X, r) est le graphe dont les sommets sont les points X, et où deux points x, x' ∈ X sont reliés par une arête si et seulement si

d(x, x') ≤ 2r.

(Nouveau : convention rayon, arête ssi les deux boules fermées de rayon r se touchent.)

### Définition 6 : Amas de forte densité, modèle de Hartigan (p. PDF 44–45, p. 18–19)

Les données X ⊂ R^p sont supposées être des réalisations indépendantes issues d'une densité par rapport à la mesure de Lebesgue f : R^p → R₊. Soit λ > 0. Ensemble de niveau supérieur à λ :

L(λ) ≜ {x ∈ R^p | f(x) ≥ λ}.

Les amas de forte densité (*high-density clusters*) H_f(λ) de la densité f au niveau λ sont définis comme les composantes connexes de L(λ) :

**H_f(λ) ≜ π₀(L(λ)).**

### Définition 7 : Estimateur de densité des K-Plus Proches Voisins (K-NN) (p. PDF 45–46, p. 19–20)

Soit X ⊂ R^p un nuage de n ∈ N* points et K ∈ N* un entier tel que K ≤ n. L'estimateur de densité des K-Plus Proches Voisins f̂_K (noté plus simplement λ̂) est défini pour tout point y ∈ R^p par

**λ̂ = f̂_K(y) ≜ (K/n) · 1/(ω_p r_K(y)^p)**

où r_K(y) = inf{r ≥ 0 | |B(y, r) ∩ X| ≥ K} représente la distance de y à son K-ième plus proche voisin, et ω_p le volume de la boule unité dans R^p.
Niveaux supérieurs associés, pour λ > 0 : L_K(λ) ≜ {y ∈ R^p | f̂_K(y) ≥ λ}.

**Remarques normatives** (p. PDF 46) :
1. Cas particulier K = 1 et y ∈ X : on pose f̂₁(y) = +∞.
2. Au facteur K/n près, λ̂ est l'inverse du volume de la plus petite boule centrée en y englobant K points de X.
3. **Changement de variable λ ↔ r** : f̂_K(y) ≥ λ ⟺ r_K(y) ≤ r, avec

   **λ = K/(n ω_p r^p).**

   « Dans la suite, on manipulera indifféremment les variables λ ou r, étant sous-entendu ce changement de variable. »

### § 2.4.3 — modèle booléen, K = 1 (p. PDF 46, p. 20)

L₁(r) ≜ {y ∈ R^p | r₁(y) ≤ r} = ⋃_{x ∈ X} B̄(x, r).

Les composantes connexes de cette union de boules correspondent aux composantes connexes du graphe géométrique G(X, r), elles-mêmes en correspondance avec le MST élagué à r (Théorème 1).

### Définition 8 : Amas de forte densité discrets (p. PDF 47, p. 21)

Soit X ⊂ R^p un nuage fini de taille n et K ≤ n un entier. Soit f̂_K l'estimateur K-NN associé et

H_{f̂_K} : r > 0 ↦ π₀(L_K(r))

les amas de forte densité de f̂_K (la fonction qui à un niveau r > 0 associe les composantes connexes du niveau supérieur L_K(r)).
On dira que x ∈ X est **couvert** par un cluster C ∈ H_{f̂_K}(r) s'il se trouve à moins de r de ce même cluster. Le cluster C^discret associé à C est alors l'ensemble des points x ∈ X qui sont couverts par C. Finalement :

**H^discrets_{f̂_K}(r) ≜ {C^discret | C ∈ H_{f̂_K}(r)}.**

**Remarques normatives** (p. PDF 47) :
1. Poser C^discret = C ∩ X **aurait été une erreur** (erreur quantifiée au chapitre 7, percolation). C'est la différence essentielle entre le Robust Single-Linkage et DBSCAN (renvoi Déf. 18, p. 37) — et ce qui explique la supériorité de DBSCAN sur le Robust Single-Linkage.
2. Équivalence : C^discret = X ∩ δ_r(C), où δ_r(C) désigne la **dilatation** de l'ensemble C de rayon r.
3. **Sauf dans le cas K = 1, les amas discrets de forte densité ne forment pas une partition de X** : à r fixé, certains points ne sont couverts par aucun cluster, d'autres par plusieurs.

### Définition 9 : Clustering sur un espace métrique (X, d) (p. PDF 50, p. 24)

Un clustering C est une fonction qui prend en entrée un espace métrique fini (X, d)

C : (X, d) ↦ ∼ ∈ Part(X)

et retourne une partition de X, c'est-à-dire une relation d'équivalence ∼ ∈ Part(X). (Rappels § 3.1 : partition = sous-ensembles non vides, disjoints deux à deux, recouvrant X ; équivalence réflexive/symétrique/transitive ; les deux notions sont substituables.)

### Définition 10 : Clustering hiérarchique ≡ Dendrogramme (p. PDF 51–52, p. 25–26)

Un clustering hiérarchique sur X est la donnée d'une application θ : R₊ → Part(X) telle que :
1. **Croissance.** Si 0 ≤ r ≤ r', alors θ(r) ⪯ θ(r').
2. **Continuité à droite.** Pour tout r ≥ 0, il existe ε > 0 tel que θ(r) = θ(r') pour tout r' ∈ [r; r + ε[.
3. θ(0) est la partition en singletons {{x_i}}_{i=1}^n.
4. Il existe r_connect tel que θ(r) = {X} pour tout r ≥ r_connect.

(⪯ = ordre de raffinement : P ⪯ P' si tout C ∈ P est inclus dans un C' ∈ P'.)

Reformulation du Single-Linkage (p. PDF 52) : θ^SL(0) = singletons ; θ^SL(r) stable jusqu'au niveau r_{t+1}, associé à la fusion des deux clusters C, C' les plus proches pour la distance **d_SL(C, C') ≜ ½ min_{x∈C, x'∈C'} d(x, x')** (le facteur ½ est ici intégré dans d_SL, cohérent avec r_t = ½·min de la Déf. 1), et

θ^SL(r_{t+1}) = {C ∪̇ C'} ∪̇ {C''}_{C'' ∈ θ^SL(r_t), C'' ∉ {C, C'}} ; r_connect = r_{n−1}.

### Définition 11 : Dendrogramme de persistance des graphes géométriques θ^∼ (p. PDF 52–53, p. 26–27)

Soit (X, d) un espace métrique fini, r ≥ 0. Relation de voisinage ↔_r : x ↔_r x' si d(x, x') ≤ 2r. La relation d'équivalence ∼_r est la clôture transitive de cette relation de voisinage. Le dendrogramme de persistance des graphes géométriques est

θ^∼ : R₊ → Part(X), r ↦ ∼_r.

### Définition 12 : Ultramétrique (p. PDF 53, p. 27)

Une application u : X × X → R₊ est une ultramétrique si u est une distance (métrique) sur X vérifiant l'inégalité triangulaire forte : pour tous x, y, z ∈ X,

**u(x, y) ≤ max{u(x, z), u(z, y)}.**

Remarque : tous les triangles d'un espace ultramétrique sont isocèles (en la plus longue arête).

### Définition 13 : Clustering hiérarchique (Carlsson & Mémoli) (p. PDF 54, p. 28)

Soit **X** la collection de tous les espaces métriques finis et **U** la collection de tous les espaces ultramétriques finis. Un clustering hiérarchique T est une application qui à un espace métrique (X, d) associe une ultramétrique sur le même ensemble fini (X, u) :

T : **X** → **U**, (X, d) ↦ (X, u).

Remarque (p. PDF 54) : vu ainsi, le Single-Linkage u^SL est la **plus grande ultramétrique sur X sous-dominante**, c'est-à-dire sous la contrainte **u ≤ d/2** (le facteur ½ encore).

---

## 3. Faits et théorèmes (énoncés complets)

### Fait (connu) 1 : L'arbre minimum couvrant (euclidien) est un sous-graphe de graphes géométriques (p. PDF 42, p. 16)

L'arbre minimum couvrant euclidien est un sous-graphe du graphe de voisinage relatif (RNG), lui-même inclus dans le graphe de Gabriel, qui est à son tour un sous-graphe de la triangulation de Delaunay :

**MST ⊆ RNG ⊆ Gabriel ⊆ Delaunay.**

Corollaire pratique (« Le cas du plan p = 2 ») : dans R², Delaunay a au plus 3n − 6 arêtes et se calcule en O(n log n) ; le Single-Linkage hérite de cette complexité.

### Fait (connu) 2 : Arbre minimum couvrant et sous-niveaux d'un graphe pondéré (p. PDF 43, p. 17)

Soit G = (V, E, w) un graphe fini pondéré, et soit T une forêt minimum couvrante de G (un arbre minimum couvrant si G est connexe). Alors, pour tout r ≥ 0, les composantes connexes de

T_{≤r} = (V, {e ∈ T : w(e) ≤ r})

sont exactement les mêmes que celles de

G_{≤r} = (V, {e ∈ E : w(e) ≤ r}).

(« Composante connexe » = ensemble des nœuds du sous-graphe induit, par abus.)

**Preuve** (complète, p. PDF 43 — éclairante pour tout algorithme d'élagage) : T_{≤r} ⊆ G_{≤r} est immédiat. Réciproquement, si u, v sont connectés dans G_{≤r} mais pas dans T_{≤r}, l'unique chemin de T reliant u à v contient une arête e de poids strictement supérieur à r. Retirer e de T définit une coupe (A, V \ A). Comme u et v sont connectés dans G_{≤r}, le chemin les reliant dans G_{≤r} traverse cette coupe par au moins une arête e', avec w(e') ≤ r < w(e) ; remplacer e par e' donnerait une forêt couvrante de poids strictement plus petit — contradiction avec la minimalité de T. ∎

### Fait (connu) 3 : Coupe et arbre minimum couvrant (p. PDF 43, p. 17)

Avec les notations du Fait 2, soit V = V₁ ∪̇ V₂ une partition de l'ensemble des sommets ; alors il existe une arête e ∈ E de poids minimum reliant V₁ à V₂ qui soit aussi dans T.

### Théorème 1 : Graphe géométrique ≡ Arbre minimum couvrant élagué (p. PDF 43, p. 17)

(Pour avoir l'unicité de l'arbre minimum couvrant, l'on suppose que les points de X sont en position générale.) Pour tout rayon r ≥ 0, les ensembles de sommets des composantes connexes de l'arbre minimum couvrant élagué de ses arêtes de longueur strictement supérieure à 2r (noté MST_{≤r}) coïncident exactement avec ceux du graphe géométrique G(X, r).

**Remarque normative sur la position générale** (p. PDF 43–44) : pour un graphe sur X ⊂ R^p, « en position générale » signifie que **les distances entre paires de points sont distinctes deux à deux**, assurant l'unicité du MST. Le manuscrit annonce qu'il reviendra sur cette notion « lorsqu'il s'agira de généraliser aux hypergraphes (voir Déf. 26, p. 84) » — donc la définition de position générale pour l'objet HGP d'ordre K est en Partie II, hors de cette tranche.

**Usage algorithmique explicite** (p. PDF 44) : « si l'on souhaite construire le clustering produit par le Single-Linkage pour un petit seuil ε, le Théorème 1 montre qu'il suffit d'identifier les composantes connexes du "petit" graphe géométrique de rayon r ← ε ; il n'y a pas besoin de reconstruire tout l'arbre minimum couvrant. »

### Fait (connu) 4 : Théorème d'impossibilité de Kleinberg (p. PDF 51, p. 25)

Aucune fonction de clustering C (Déf. 9) sur un espace métrique (X, d) ne peut satisfaire simultanément :
1. **Invariance d'échelle.** ∀λ > 0, C(X, d) = C(X, λd).
2. **Richesse.** ∀P ∈ Part(X), il existe une distance d sur X telle que C(X, d) = P.
3. **Cohérence.** Si P = C(X, d) et que d' vérifie d'(x, x') ≤ d(x, x') pour x, x' dans un même cluster et d'(x, x') ≥ d(x, x') sinon, alors P = C(X, d').

Fait remarquable noté : pour chacune des trois combinaisons de deux propriétés, Kleinberg exhibe un clustering qui les satisfait — dans les trois cas une variante du Single-Linkage.

### Fait (connu) 5 : Dendrogramme du Single-Linkage ≡ Dendrogramme de persistance des graphes géométriques (p. PDF 53, p. 27)

Soit (X, d) un espace métrique fini. Le dendrogramme θ^SL produit par le Single-Linkage sur X et θ^∼, celui de persistance des graphes géométriques (Déf. 11), sont identiques : **θ^SL ≡ θ^∼.**

### Fait (connu) 6 : Hiérarchie ≡ Ultramétrique (p. PDF 53–54, p. 27–28)

Il existe une bijection canonique entre l'ensemble des ultramétriques u et l'ensemble des dendrogrammes θ sur X.
1. **De l'ultramétrique vers le dendrogramme (u ↦ θ).** La partition θ(r) est définie par la relation d'équivalence : x ∼_r y ⟺ u(x, y) ≤ r.
2. **Du dendrogramme vers l'ultramétrique (θ ↦ u).** La distance ultramétrique est le seuil d'apparition du premier cluster contenant le couple :

   **u(x, y) = inf{r ≥ 0 | ∃C ∈ θ(r), x ∈ C et y ∈ C}.**  (Éq. 3.1)

Remarque : le cluster C associé à u(x, y) est le **plus petit ancêtre commun** à x et y.

### Fait (connu) 7 : Single-Linkage, unique solution fonctorielle (p. PDF 54, p. 28 — TRONQUÉ dans cette tranche)

Soit T une méthode de clustering hiérarchique qui à tout espace métrique fini (X, d) associe une ultramétrique T : (X, d) ↦ (X, u), et supposons que T vérifie :
1. **Normalisation à deux points.** Si |X| = 2 et d(x, x') = 2δ, alors u(x, x') = δ.
2. **Fonctorialité.** Pour toute application 1-lipschitzienne φ : (X, d_X) → (Y, d_Y), on a u_Y(φ(x), φ(x')) ≤ u_X(x, x') pour tous x, x' ∈ X ;

[La condition 3 et la conclusion (T = Single-Linkage) sont en page PDF 55, hors tranche. Le corps du texte (p. PDF 35 et 49) affirme la conclusion : « Le Single-Linkage est l'unique clustering hiérarchique satisfaisant cette axiomatique (Fait 7, p. 28) ».]

### Résultats algorithmiques cités (§ 2.2.1, p. PDF 40 — non numérotés)

- **Borůvka (1926)** [26] : à chaque itération, chaque composante connexe identifie et contracte son arête sortante la plus courte ; le nombre de composantes diminue de moitié à chaque passe → O(m log n). Qualifié de « **Single-Linkage massivement parallélisé** ».
- **Kruskal (1956)** [27] : arêtes triées par poids croissant, insérées sous réserve de ne pas former de cycle ; étape limitante = tri, O(Tri(m)) (QuickSort O(m log m) en moyenne) ; Union-Find en mémoire auxiliaire O(n). En pratique la méthode la plus utilisée.
- **Jarník–Prim–Dijkstra (1930/1957/1959)** : croissance d'un unique arbre par rattachement du sommet non visité le plus proche ; O(n²) sur un espace géométrique dense, asymptotiquement optimal dans ce cas.
- Améliorations : raffinements du facteur log n [30] ; complexité optimale (non calculée explicitement) = celle d'un certain arbre de décision [31] ; algorithmes aléatoires [32] ; Veldt et al. [33] : complétion de forêts par apprentissage (*learning-augmented*) en temps sous-quadratique, esquivant le parcours exhaustif de la matrice des distances.

### Synthèse du chapitre 2 (p. PDF 47–48, p. 21–22)

Soit t ∈ {0; …; n−1} l'étape d'arrêt, r_t le rayon de coupure, λ_t = 1/(n ω_p r_t^p) la variable de densité associée (cas K = 1). La partition C^{r_t} peut être obtenue de trois façons différentes :

C^{r_t} = Composantes connexes du graphe géométrique G(X, r_t)
       = Composantes connexes de l'arbre minimum couvrant élagué MST_{≤ r_t}
       = Amas discrets de forte densité H^discrets_{f̂₁}(λ_t).

« Cette synthèse intègre le Single-Linkage dans la famille de clusterings *density-based* [43] et **trace la voie naturelle pour sa généralisation aux estimateurs d'ordre supérieur avec K ≥ 2** » (dernière phrase du chapitre 2, p. PDF 48).

---

## 4. Points demandés par la mission : présents / absents dans cette tranche

- **Ordre K** : présent uniquement via l'estimateur K-NN (Déf. 7), le changement de variable **λ = K/(n ω_p r^p)** (p. PDF 46), la non-partition des amas discrets pour K ≥ 2 (Déf. 8, Remarque 3) et l'annonce finale de la généralisation K ≥ 2 (p. PDF 48). La construction HGP proprement dite (ordre K, simplexes) n'est PAS dans cette tranche.
- **Simplexes et filtration, rayon ρ(σ)** : ABSENT. Aucune définition de simplexe ni de fonction de filtration dans les pages 35–54.
- **Miniboule / circumradius / rayon de couverture** : ABSENT (seule apparaît la « plus petite boule centrée en y englobant K points », Déf. 7 Rem. 2, qui est un objet différent : boule centrée en un point de requête, pas miniboule d'un simplexe).
- **Théorème de Jung** : ABSENT.
- **Triangles aigus / obtus** : ABSENT (la seule mention de triangles est la remarque « tous les triangles ultramétriques sont isocèles », Déf. 12).
- **Événements de naissance / fusion** : présents seulement au niveau K = 1 : naissance = tous les singletons à r = 0 (Déf. 1, Déf. 10 (iii)) ; fusion = arête du MST au niveau r_t = ½ d_SL ; « naissance et fusion de composantes lorsque l'on fait varier un paramètre d'échelle r » (§ 2.3, p. PDF 42) — le vocabulaire de l'« analyse persistante » est posé ici.
- **Arbre/forêt de fusion** : le dendrogramme (Déf. 1, 10, 11) et son équivalence avec l'ultramétrique (Faits 5, 6, Éq. 3.1 : u(x,y) = niveau du plus petit ancêtre commun) — c'est la sémantique de sortie que la forêt HGP devra généraliser.
- **Partition des (K−1)-simplexes (§ 9.1), poids w_{x,τ} = S_τ/T_x** : ABSENT — Partie II (le § 9.1 est bien au-delà de la p. PDF 54).
- **Position générale pour hypergraphes** : renvoi explicite à **Déf. 26, p. 84** (imprimée), hors tranche.
- **DBSCAN / Robust Single-Linkage** : renvoi à Déf. 18, p. 37 (imprimée), hors tranche ; mais la remarque normative « C^discret = C ∩ X aurait été une erreur » (Déf. 8, Rem. 1) est ici.
- **Algorithmes proposés et coûts** : § 2.2.1 (Borůvka O(m log n), Kruskal O(m log m) + Union-Find O(n), Prim O(n²)) ; Fait 1 + cas du plan (Delaunay O(n log n), ≤ 3n − 6 arêtes) ; Théorème 1 (composantes à seuil fixé sans MST complet) ; Veldt et al. sous-quadratique.

---

## 5. Pertinence pour la conception V4

Ce que cette tranche **impose** :

1. **Convention rayon (facteur ½) partout.** r_t = ½ d_SL (Déf. 1), arête ssi d ≤ 2r (Déf. 5, Déf. 11), u ≤ d/2 (Déf. 13, remarque), normalisation u(x,x') = δ pour d = 2δ (Fait 7). La V4 doit fixer et documenter cette convention dans ses événements q2 : le niveau d'un événement de fusion d'ordre 2 est la **demi-longueur** de l'arête WSPD retenue. Toute confusion rayon/diamètre casserait la comparabilité avec les niveaux d'ordre supérieur (λ = K/(n ω_p r^p) fait tout vivre dans la même variable r).
2. **La sortie normative est un dendrogramme au sens de la Déf. 10** (croissance, continuité à droite, singletons en 0, racine à r_connect) — équivalent à une ultramétrique (Fait 6) dont la valeur est le niveau du plus petit ancêtre commun (Éq. 3.1). La forêt HGP de V4, restreinte à K = 1 (i.e. q2 seul), doit reproduire exactement θ^SL : c'est l'oracle de correction gratuit — comparaison à un MST de référence sur n ≤ 12–14, et par invariants à l'échelle.
3. **Propriété de coupe (Faits 2 et 3) = fondement théorique de l'approche WSPD.** Le Fait 3 dit exactement que pour toute bipartition (V₁, V₂), l'arête de poids minimum traversant la coupe est dans le MST : c'est ce qui justifie qu'une recherche d'« arête maximale/minimale par paire bien séparée » (Callahan–Kosaraju) capture les arêtes du MST sans graphe complet. La preuve par échange du Fait 2 est le modèle de l'argument à généraliser en ordre K.
4. **Théorème 1 = élagage sans reconstruction.** Autorisation explicite de travailler à seuil fixé via les composantes du graphe géométrique, sans matérialiser l'arbre complet — utile pour les rendus par coupe (select_lambda_cut) et pour valider des tranches de la forêt V4 indépendamment.
5. **Borůvka est désigné « Single-Linkage massivement parallélisé »** (nombre de composantes divisé par 2 par passe, O(m log n)) : c'est le squelette naturel du contrat GPU de V4 (K=10, < 100 ms) — passes Borůvka sur candidats WSPD, chaque composante cherchant son arête sortante minimale, plutôt que Kruskal séquentiel.
6. **Sémantique discrète par couverture, pas par intersection.** Déf. 8 : C^discret = X ∩ δ_r(C) (dilatation), et « C^discret = C ∩ X aurait été une erreur » — quantifiée par la percolation (chap. 7). Pour K ≥ 2 les amas discrets **ne forment pas une partition** (points couverts par 0 ou plusieurs clusters). La V4 ne doit donc pas supposer qu'une coupe à r fixé partitionne le nuage ; le recouvrement est normatif.
7. **Position générale = distances deux à deux distinctes** (unicité du MST, Théorème 1). La généralisation hypergraphe est en Déf. 26 p. 84 : la V4 doit lire cette définition avant de figer sa politique de dégénérescence (le profil u16 quantifié de v3 avec refus explicite reste la référence tant que Déf. 26 n'est pas lue).
8. **Chaîne MST ⊆ RNG ⊆ Gabriel ⊆ Delaunay (Fait 1)** : filtres de candidats corrects à l'ordre 2. La V4 (WSPD) n'en dépend pas, mais tout témoin d'élimination q2 « z dans la lentille de Gabriel/RNG » a ici sa forme exacte (Défs. 2–3 : conditions max(‖x−z‖,‖x'−z‖) ≥ ‖x−x'‖ et ‖x−z‖² + ‖x'−z‖² ≥ ‖x−x'‖²) — directement réutilisable comme test de témoin en zone cœur.

Ce que cette tranche **interdit** :

- Promouvoir une sortie qui ne serait pas un dendrogramme/ultramétrique au sens strict (croissance et continuité à droite comprises) pour la restriction K = 1.
- Confondre niveaux r (rayon) et distances (2r) dans les événements.
- Définir le rendu discret par C ∩ X.
- Supposer la partition des amas discrets pour K ≥ 2.

Ce que cette tranche **ne couvre pas** (à chercher en Partie II, p. PDF 77–134) : la définition de l'objet HGP multi-ordre lui-même, ρ(σ), miniboules, Jung, triangles aigus, Déf. 26 (position générale hypergraphe), § 9.1 (partition des (K−1)-simplexes, w_{x,τ} = S_τ/T_x).

---

## Questions ouvertes / ambiguïtés

1. **Fait 7 tronqué** : seules les conditions 1 (normalisation à deux points) et 2 (fonctorialité) sont dans la tranche ; la condition 3 et la conclusion formelle sont p. PDF 55. L'énoncé complet doit être vérifié dans la tranche suivante avant toute invocation.
2. **Déf. 26, p. 84 (position générale pour hypergraphes)** : référencée mais non lue. La politique de dégénérescence V4 (égalités de distances, u16 quantifié) ne peut être figée qu'après lecture.
3. **Déf. 18, p. 37 (Robust Single-Linkage / DBSCAN)** : référencée (Déf. 8, Rem. 1) mais hors tranche ; l'écart exact RSL/DBSCAN (et sa quantification au chap. 7) reste à lire.
4. **Ambiguïté bénigne de notation d_SL** : en Déf. 1 (p. PDF 38), d_SL(C, C') = min d(x, x') (sans ½, le ½ étant dans r_t ≜ ½ d_SL) ; en § 3.3 (p. PDF 52), d_SL(C, C') ≜ ½ min d(x, x') (le ½ intégré). Les deux conventions coexistent dans le manuscrit ; le niveau de fusion est identique (½·min) dans les deux cas, mais toute citation de « d_SL » doit préciser laquelle.
5. **H_{f̂_K} indexé par r vs λ** : la Déf. 8 écrit H_{f̂_K} : r > 0 ↦ π₀(L_K(r)) alors que L_K est défini (Déf. 7) comme fonction de λ ; c'est le changement de variable λ = K/(n ω_p r^p) (Déf. 7, Rem. 3) qui est sous-entendu, comme annoncé (« on manipulera indifféremment λ ou r »). Pas de contradiction, mais une implémentation doit choisir une variable canonique (r semble le choix du manuscrit pour la suite).
6. **« Moins de r » dans la Déf. 8** : « x est couvert […] s'il se trouve à moins de r de ce cluster » — inégalité stricte ou large non précisée typographiquement dans la définition ; la Rem. 2 (C^discret = X ∩ δ_r(C), dilatation fermée usuelle) et le modèle booléen B̄(x, r) (fermé, § 2.4.3) suggèrent l'inégalité large (≤ r), mais ce point mérite confirmation sur les pages suivantes.
7. **Continuité à droite vs événements** : la Déf. 10 (ii) impose θ constant sur [r_t, r_{t+1}[ ; les fusions sont donc effectives **au** niveau r_t (inclusivité à gauche de l'intervalle). L'identification exacte des événements V4 doit adopter cette convention (événement actif à son propre niveau).
