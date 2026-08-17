# Lecture normative — MANUSCRIT_THESE_HAUSEUX.pdf, pages PDF 55–74

Tranche lue : pages PDF 55 à 74 (pagination livre 29 à 48 ; correspondance constante **page PDF = page livre + 26**). Cette tranche est entièrement dans la **Partie I** : fin du chapitre 3, chapitre 4 complet, chapitre 5 complet. L'objet HGP lui-même (K-polyèdres, HGP-Clusterer) n'y est **pas défini** : il est référencé par renvois avant (« Déf. 21, p. 58 » et « Déf. 22, p. 58 » en pagination livre, soit pages PDF ≈ 84, Partie II).

---

## 1. Structure section par section

| Page PDF | Page livre | Contenu |
|---|---|---|
| 55 | 29 | § 3.3–3.3.1 (fin) : item 3 « Séparation » d'un encadré commencé page précédente (vraisemblablement le Fait 7, axiomatique minimale) ; conclusion `T = T^SL` ; **Synthèse** du chapitre 3. |
| 56 | 30 | Page blanche. |
| 57 | 31 | **Chapitre 4 — Les limites du Single-Linkage** : introduction, contributions. |
| 58 | 32 | § 4.1 « Quand la notion de densité manque : l'effet de chaînage » ; Figure 4.1. |
| 59 | 33 | § 4.1.1 « Consistance et inconsistance du Single-Linkage » ; **Définition 14** ; **Fait (connu) 8** ; explication par la percolation continue. |
| 60 | 34 | § 4.2 « Les liaisons dangereuses : Complete- et Average-Linkage » ; **Définition 15** ; **Fait (connu) 9**. |
| 61 | 35 | § 4.3 « Introduire la densité locale : le Robust Single-Linkage » ; **Définition 16** (core distance) ; **Définition 17** (mutual reachability). |
| 62 | 36 | **Fait (connu) 10** (ultramétrique u^RSL en min max) ; § 4.3.1 « Critique du Robust Single-Linkage » ; § 4.4 « L'état de l'art : HDBSCAN » ; § 4.4.1 « DBSCAN » (début). |
| 63 | 37 | Suite § 4.4.1 ; **Définition 18** (catégorisation DBSCAN) ; § 4.4.2 « HDBSCAN ». |
| 64 | 38 | § 4.4.3 « Le seuil de percolation (`min_cluster_size`) » ; § 4.4.4 « Un clustering multi-échelle : le critère d'excès de masse » ; **Définition 19**. |
| 65 | 39 | Estimation `λ̂_x = 1/r_x` ; Figure 4.2 ; récurrence de stabilité `Ẽ(C)` ; § 4.4.5 « Les limites de l'approche : le paradoxe du choix de K ». |
| 66 | 40 | Fin § 4.4.5 ; **Synthèse** du chapitre 4 (annonce des hypergraphes/complexes de Čech et des K-polyèdres). |
| 67 | 41 | **Chapitre 5 — Une vue générale sur le clustering hiérarchique. Ou comment introduire des *a priori*** : introduction, contributions. |
| 68 | 42 | § 5.1 « L'arbre comme espace de résolution » ; § 5.1.1 « Formalisation convexe du problème de clustering ». |
| 69 | 43 | Figure 5.1 ; § 5.1.2 « Quand les problèmes NP-difficiles deviennent triviaux » ; **Fait (connu) 11** (k-Means résolu sur hiérarchie en temps de tri). |
| 70 | 44 | § 5.2 « "Hacker" HDBSCAN : l'exploration de l'arbre hiérarchique » ; Figure 5.2 ; règle générique loss(Père) vs Σ loss(Fils). |
| 71 | 45 | § 5.3 « Application I : Détection d'anomalies 3D guidée par un modèle théorique (©Naval Group) » ; § 5.3.1–5.3.3. |
| 72 | 46 | Figure 5.3 (benchmark 2 000 000 points) ; § 5.4 « Application II : Segmentation d'instance 4D sur SemanticKITTI ». |
| 73 | 47 | § 5.4.1 « La méthode » (segmentation spatiale 3D par HGP-Clusterer + tracking 4D par transport optimal). |
| 74 | 48 | Figures 5.4 et 5.5 ; **Synthèse** du chapitre 5. |

---

## 2. Définitions et notations normatives (énoncés complets)

### Encadré tronqué en tête de tranche (page PDF 55, livre 29)

Fin d'un encadré (début sur la page PDF 54, hors tranche ; d'après les renvois ultérieurs il s'agit très probablement du **Fait 7 : axiomatique minimale du Single-Linkage**, cité page PDF 57 comme « Fait 7, p. 28 »). Le texte visible :

> **3. Séparation.** Pour tout (𝒳, d), et tous x ≠ x′ ∈ 𝒳,
>
> u(x, x′) ≥ sep(𝒳, d) ≜ (1/2) · min_{a≠b∈𝒳} d(a, b).
>
> Alors T coïncide nécessairement avec le Single-Linkage hiérarchique : **T = T^SL**.

Les items 1–2 et l'hypothèse sur T sont hors tranche (page PDF 54).

### Définition 14 (page PDF 59, livre 33) — (Hartigan)-Consistance d'un algorithme de clustering hiérarchique [1]

> Un algorithme de clustering hiérarchique θ (Déf. 10, p. 25) est dit *consistant* (au sens de Hartigan) si, quels que soient f une fonction de densité suffisamment régulière sur ℝ^p, λ > 0, et A, B deux amas disjoints de forte densité de f au niveau λ (et de mesure strictement positive), alors θ sépare asymptotiquement parfaitement A et B ; c'est-à-dire qu'avec probabilité tendant vers 1 lorsque n → ∞, si 𝒳_n est un nuage de n points tirés i.i.d. selon f, il existe r_n > 0 tel que θ(r_n) contient deux clusters disjoints A_n et B_n satisfaisant A_n ⊇ A ∩ 𝒳_n et B_n ⊇ B ∩ 𝒳_n.

### Définition 15 (page PDF 60, livre 34) — Complete-Linkage [46] et Average-Linkage [47]

> Soient C et C′ deux clusters d'un espace métrique (𝒳, d).
> — **Complete-Linkage** (liaison complète) : d_CL(C, C′) = max_{x∈C, y∈C′} d(x, y).
> — **Average-Linkage** (liaison moyenne) : d_AL(C, C′) = (1 / (|C| |C′|)) · Σ_{x∈C} Σ_{y∈C′} d(x, y).

### Définition 16 (page PDF 61, livre 35) — Distance de cœur (*core distance*) [48]

> Soit 𝒳 ⊂ ℝ^p un nuage de points de taille n et K ≤ n. La distance de cœur d'un point x ∈ 𝒳, notée r_K(x), est la distance de x à son K-ième plus proche voisin dans le nuage :
>
> **r_K(x) = min{ r ≥ 0 | |B(x, r) ∩ 𝒳| ≥ K }.**

Commentaire du texte : r_K est (à un exposant dimensionnel près) inversement proportionnelle à l'estimateur de densité K-NN évalué en x.

### Définition 17 (page PDF 61, livre 35) — Distance d'atteignabilité mutuelle (*mutual reachability distance*) [8]

> La distance d'atteignabilité mutuelle entre deux points x, x′ ∈ 𝒳 pour un paramètre K est définie par :
>
> **d_mreach,K(x, x′) ≜ max{ r_K(x), r_K(x′), d(x, x′) }.**

Le Robust Single-Linkage (RSL) = Single-Linkage classique (extraction de l'arbre minimum couvrant), mais avec les poids d'arêtes d_mreach,K au lieu de d. C'est une dissimilarité, « en fait » pas une métrique.

### Définition 18 (page PDF 63, livre 37) — Catégorisation des points dans DBSCAN [49]

> 1. **Points cœurs** (*core points*). Points possédant au moins K voisins dans leur r-voisinage (r_K(x) ≤ r).
> 2. **Points accessibles** (*reachable points* ou *border points*). Points n'étant pas des cœurs, mais situés dans le r-voisinage d'un point cœur. Ils forment la frontière du cluster.
> 3. **Bruit** (*noise* / *outliers*). Tous les autres points, considérés comme isolés.

Algorithme : connecter les points cœurs entre eux s'ils sont à une distance mutuelle inférieure à r (ossature des clusters), puis rattacher les points accessibles.

### Définition 19 (page PDF 64, livre 38) — Excès de Masse (*excess of mass*) [9]

> Pour une fonction de densité continue f, l'excès de masse d'un cluster C apparu à un niveau de densité de base λ est défini par :
>
> **E(C) = ∫_C ( f(x) − λ ) · 1_{{f(x) ≥ λ}} dx.**

Estimation empirique (page PDF 64, livre 38) : si le cluster C commence à exister au seuil de densité λ̂_min (moment où il se sépare de son parent) et qu'un point x ∈ 𝒳 quitte le cluster (devient du bruit ou forme un sous-cluster) à la densité λ̂_x, l'excès de masse cumulé de C est estimé par :

> **Ê(C) ∝ Σ_{x∈C} ( λ̂_x − λ̂_min ).**

Et (page PDF 65, livre 39) : HDBSCAN estime la densité en un point x simplement avec l'inverse de la distance à laquelle le point se détache : **λ̂_x = 1/r_x**.

### Récurrence de stabilité HDBSCAN (page PDF 65, livre 39 — formule non numérotée)

> Ẽ(C) = Ê(C) si C est une feuille ;
> Ẽ(C) = max( Ê(C), Σ_{C_i ∈ Fils(C)} Ẽ(C_i) ) sinon.

Si la stabilité des enfants est supérieure, la scission est validée et on parcourt les nœuds enfants ; sinon le parent est sélectionné comme cluster unique et robuste (Ẽ(C) = Ê(C)). Exemple de la Figure 4.2 : feuilles Ẽ(C₁), Ẽ(C₂), Ẽ(C₃) ; au nœud C₄ la somme verte + rouge dépasse l'aire violette → scission conservée ; à la racine C₅, Ẽ(C₄) + Ẽ(C₃) dépasse l'aire bleue → partition finale {C₁, C₂, C₃}.

### Règle de `min_cluster_size` (page PDF 64, livre 38, § 4.4.3 — pas de numéro)

Élagage systématique : suppression de tous les clusters de taille strictement inférieure à `min_cluster_size`. À chaque scission d'un parent en deux branches : si une seule branche a suffisamment de points → pas une véritable scission, la branche survivante conserve l'identité du parent ; si les deux branches ont une taille ≥ `min_cluster_size` → scission validée, deux nouveaux clusters naissent. Le texte y voit un **seuil de percolation** implicite : seules les composantes « macroscopiques » sont conservées.

### Convex clustering (page PDF 68, livre 42, § 5.1.1 — formule non numérotée) [52, 53]

> min_{U ∈ ℝ^{n×p}}  (1/2) Σ_{i=1}^{n} ‖x_i − u_i‖²  [Fidélité]  + γ Σ_{i<j} w_ij ‖u_i − u_j‖  [Pénalité de fusion]

avec γ ≥ 0 paramètre de régularisation, w_ij poids positifs encodant la géométrie locale. Analyse duale [54] : variables duales v_ij avec conditions de capacité ‖v_ij‖ ≤ γ w_ij, interprétées comme *forces de tension* ; fusion (u_i = u_j) quand l'équilibre des forces sature la capacité γ w_ij. Note de bas de page : le Single-Linkage se formule comme un cas très particulier de ce problème [54]. En faisant varier γ, la séquence temporelle de coalescence reconstruit un arbre hiérarchique (Fig. 5.1).

### Règle générique d'extraction sur l'arbre (page PDF 70, livre 44, § 5.2 — formule non numérotée)

> Si loss(C_Père) < Σ_i loss(C_Fils,i)  ⟹  Conserver le nœud parent, sinon continuer d'explorer.

Pour HDBSCAN : loss(C) = −Ẽ(C). En substituant une fonction de coût personnalisée (alignement d'une structure, volume maximal attendu d'un objet, conformité à un modèle 3D), l'algorithme devient un extracteur guidé géométriquement.

---

## 3. Théorèmes, propositions, lemmes (« Faits »)

### Fait (connu) 8 (page PDF 59, livre 33) — Consistance et inconsistance du Single-Linkage [1]

> — En dimension p = 1, l'algorithme de Single-Linkage est un estimateur (Hartigan)-consistant des amas de forte densité.
> — En dimension p ≥ 2, l'algorithme de Single-Linkage est (Hartigan)-inconsistant pour recouvrer les amas de forte densité. Il n'est que *fractionnellement consistant*.

Explication (esquisse, même page) : phénomène de **percolation continue** [6, 18]. En dimension p ≥ 2, à mesure que le rayon de connexion augmente, des composantes géantes apparaissent presque sûrement ; avant qu'une composante géante n'absorbe l'intégralité des points de son amas de densité, elle *percole* (fusionne) inévitablement avec les composantes géantes voisines via des ponts de bruit (= *chaining effect*). On ne peut espérer recouvrer qu'une *fraction* des points du cluster. En p = 1, le phénomène n'existe pas : l'espace vide maximum sépare toujours parfaitement les modes. Renvoi : chapitre 7, mesure de la **vitesse de percolation** (*percolation rate* [41, 45]).

### Fait (connu) 9 (page PDF 60, livre 34) — Instabilité des Complete- et Average-Linkage [16]

> Contrairement au Single-Linkage, les algorithmes de *Complete Linkage* et d'*Average Linkage* sont *instables*. Ils ne satisfont pas la propriété de continuité 1-lipschitzienne au sens de la distance de Gromov–Hausdorff. Une infime perturbation de l'espace métrique initial peut engendrer des dendrogrammes radicalement différents.

Conclusion normative du texte : « La solution au *chaining effect* ne réside donc pas dans la modification de la fonction de liaison entre ensembles (qui détruit les garanties de stabilité métrique). »

### Fait (connu) 10 (page PDF 62, livre 36) — Ultramétrique u^RSL du Robust Single-Linkage exprimée comme un min max [14]

> Soit 𝒳 ⊂ ℝ^p un nuage de n points, et K ∈ ℕ*, K ≤ n un entier. Soit d_mreach,K la distance d'atteignabilité mutuelle sur 𝒳. Alors l'ultramétrique u^{RSL_K} induite par le Robust Single-Linkage sur 𝒳 peut s'exprimer ainsi :
>
> **∀ x, x′ ∈ 𝒳,  u^{RSL_K}(x, x′) = (1/2) · min_{ω : x ⇝ x′} max_{(u,v) ∈ ω} d_mreach,K(u, v)**
>
> où le minimum est pris sur l'ensemble des chemins ω de x à x′.

Remarque du texte : pour que deux points soient connectés au niveau r, il faut proximité géométrique **et** que chacun d'eux réside dans une région de haute densité (r_K(x) ≤ r et r_K(y) ≤ r).

**Critique (§ 4.3.1, page PDF 62)** — décisive pour la suite : à K fixé, le RSL « ne permet absolument pas de retrouver les amas de forte densité pour le K-NN ». Le min max garantit que chaque *sommet* du meilleur chemin ω est dans un amas de forte densité K-NN, mais **pas que le chemin ω lui-même soit entièrement compris dans un amas de forte densité**. « La bonne façon d'introduire l'estimateur K-NN, sans briser la topologie des ensembles de niveau, consiste à utiliser des hypergraphes (les complexes de Čech) et à étendre la notion de connectivité *via* les K-polyèdres (Déf. 21, p. 58 [livre]). »

### Fait (connu) 11 (page PDF 69, livre 43) — Résolution optimale du k-Means contraint par une hiérarchie [14]

> Pour n'importe quelle hiérarchie ultramétrique donnée, les objectifs de clustering fondés sur les centres (tels que le k-Means, k-Median, ou k-Centre) contraints par cette hiérarchie peuvent être résolus de manière exacte. L'ensemble complet des solutions optimales pour tout k ∈ [[1 ; n]] est alors calculable en temps de tri, soit 𝒪(n log n) opérations.

---

## 4. Éléments demandés explicitement par la mission — présence/absence dans la tranche

- **Ordre K** : présent partout comme paramètre de densité K-NN (`min_samples` de HDBSCAN). Chiffres exacts : « K ≳ 10 » dégrade (pages PDF 57–58 : « la recherche min max sur le graphe de voisinage muni de la distance de cœur ne représente plus rien pour K ≳ 10, et certainement pas les véritables amas discrets de forte densité K-NN ») ; « ne pas excéder une dizaine (même pour des données dépassant les milliers de points) » (page PDF 57) ; « un compromis faible comme K = 10 est souvent privilégié » (page PDF 66). Raison de fond (page PDF 66) : le graphe construit sur 𝒳 « n'a pas de raison de refléter les amas de forte densité lorsque K ≥ 2 ».
- **Simplexes, filtration, rayon ρ(σ), miniboule, circumradius, rayon de couverture, théorème de Jung, triangles aigus/obtus** : **ABSENTS de cette tranche.** Seules annonces : hypergraphes = complexes de Čech, K-polyèdres (Déf. 21, p. 58 livre = page PDF ≈ 84), interactions d'ordre supérieur (Synthèse ch. 4, page PDF 66). Le seul simplexe mentionné est le (n−1)-simplexe régulier de la clique dense (Fig. 4.1).
- **Événements de naissance/fusion** : présents sous forme HDBSCAN : naissance d'un cluster au seuil λ̂_min (séparation du parent), départ d'un point à λ̂_x, scission validée/invalidée par `min_cluster_size`. Pas encore la version HGP.
- **Arbre / forêt de fusion** : la notion de **forêt hiérarchique** apparaît page PDF 73 (§ 5.4.1) : la première passe « fournit non pas un clustering du nuage de points mais une forêt hiérarchique, un ensemble d'arbres hiérarchiques dont la racine représente le cluster qu'aurait choisi l'algorithme sur une unique trame. Cette forêt est ensuite passée au *tracker*. »
- **Partition des (K−1)-simplexes (§ 9.1), poids w_{x,τ} = S_τ/T_x** : **ABSENTS** (Partie II, hors tranche).
- **Algorithmes proposés et coûts** :
  - RSL = MST sous d_mreach,K (page PDF 61) ; coût non chiffré ici.
  - Extraction HDBSCAN : élagage `min_cluster_size` + excès de masse + récurrence Ẽ (pages PDF 64–65) ; parcours de l'arbre condensé de bas en haut.
  - Fait 11 : extraction k-Means/k-Median/k-Centre exacte sur arbre en 𝒪(n log n).
  - « Hacker » HDBSCAN : remplacer loss(C) = −Ẽ(C) par un coût a priori (pureté Gini contre modèle 3D — § 5.3.2 ; dimensions physiques attendues par classe — § 5.4.1).
  - Tracking 4D : transport optimal déséquilibré [60] régularisé par l'entropie, algorithmes « hautement parallélisables de type Sinkhorn [61] », filtre de Kalman [59].
  - Chiffres de performance rapportés : benchmark synthétique ©Naval Group de **2 000 000** de points (Fig. 5.3), données réelles « de l'ordre de 10 000 000 de points » (page PDF 72) ; temps de réponse du pipeline SemanticKITTI « de l'ordre de la seconde » par trame alors qu'une version industrielle exige « dix trames par seconde » (pages PDF 73–74).
- **HGP-Clusterer** : nommé pages PDF 67 et 70 — « HGP pour Hypergraphe-Percol' », « notre généralisation du Single-Linkage aux interactions d'ordre supérieur, qui sera définie formellement dans la partie suivante, voir Déf. 22, p. 58 [livre] ». Aucune définition dans la tranche.
- **Renvois structurants relevés** (utiles pour cibler les autres tranches, en pagination livre) : Déf. 8, p. 21 (amas *discrets* de forte densité, points *couverts* par un amas sans lui appartenir) ; Déf. 10, p. 25 (clustering hiérarchique θ) ; Fait 4, p. 25 (impossibilité de Kleinberg) ; Fait 6, p. 27 (bijection dendrogrammes/ultramétriques) ; Fait 7, p. 28 (axiomatique minimale) ; § 2.4.3 (SL reconstruit exactement la hiérarchie des amas de forte densité pour 1-NN) ; Déf. 21, p. 58 (K-polyèdres) ; Déf. 22, p. 58 (HGP-Clusterer) ; chapitre 6 (critique approfondie du RSL) ; chapitre 7 (vitesses de percolation, *percolation rate* [41, 45]).

### Modèle minimal de l'effet de chaînage (page PDF 58, livre 32 — fixture canonique)

Deux nuages de taille n :
1. **La chaîne** (sur 𝒳) : d_𝒳(x_i, x_j) = 2δ|i − j| (points alignés espacés de 2δ).
2. **La clique dense** (sur 𝒴) : d_𝒴(y_i, y_j) = 2δ pour toute paire i ≠ j (sommets d'un (n−1)-simplexe régulier).

Le dendrogramme Single-Linkage est rigoureusement identique sur les deux : fusion simultanée de tous les points au niveau δ. (Cas n = 3 illustré Fig. 4.1 : trois points alignés espacés de 2δ vs triangle équilatéral de côté 2δ.)

---

## 5. Pertinence pour la conception V4

**Ce que la tranche impose :**

1. **K ≈ 10 est le point de fonctionnement scientifiquement motivé.** Le contrat V4 « forêt complète K = 10 en < 100 ms » coïncide exactement avec le K critique du manuscrit : c'est précisément là où HDBSCAN s'effondre (« ne représente plus rien pour K ≳ 10 ») et où l'exactitude HGP a sa valeur différenciante. K = 5 (contrat secondaire) est dans la zone où HDBSCAN reste à peu près utilisable — l'argument de vente de V4 est donc surtout K = 10.
2. **Interdiction implicite du surrogate min max sur graphe.** La critique du § 4.3.1 (Fait 10 + discussion) est normative : tout algorithme qui se réduit à un MST sous une distance d'atteignabilité (poids par paire) reproduit le défaut du RSL — sommets du chemin dans la densité, chemin pas dedans. La connectivité HGP est une connectivité d'ordre supérieur (hypergraphe/Čech, K-polyèdres). La V4 (supports q2/q3/q4 avec témoins x, y) doit produire *exactement* ces événements d'ordre supérieur, pas une approximation par paires.
3. **La sortie doit être une forêt hiérarchique, pas une partition** (§ 5.4.1) : ensemble d'arbres dont chaque racine est un cluster ; elle est consommée en aval par des extracteurs. Cela valide le livrable V4 « reconstruction de la forêt HGP ».
4. **L'arbre doit porter les niveaux de naissance/mort par point** pour alimenter les rendus aval : excès de masse Ê(C) ∝ Σ_{x∈C}(λ̂_x − λ̂_min) avec λ̂_x = 1/r_x, récurrence Ẽ(C) = max(Ê(C), Σ Ẽ(C_i)), élagage `min_cluster_size`, et la règle générique loss(Père) vs Σ loss(Fils) avec coûts a priori (Gini/modèle 3D, dimensions par classe). Concrètement : chaque nœud de la forêt V4 doit exposer (niveau de naissance, niveau de fusion/mort, liste de départs de points avec leurs niveaux) — c'est ce que consomment `select_excess_of_mass` / `select_lambda_cut` / `select_dbscan_radius` de l'API produit.
5. **Toute la valeur est dans l'exactitude de l'arbre** (Fait 11) : une fois la hiérarchie exacte disponible, k-Means/k-Median/k-Centre contraints se résolvent en 𝒪(n log n) pour tous les k simultanément. Donc aucun compromis d'exactitude sur la construction de la forêt n'est justifiable par l'aval — cohérent avec la règle du dépôt (« aucun benchmark ne promeut exact »).
6. **Cibles de performance confirmées par les applications** : nuages de 2 M (synthétique) à ~10 M de points (réel, Naval Group) → l'exigence V4 « dizaines de millions de points » est le régime applicatif réel ; temps actuel ~1 s/trame vs besoin capteur 10 trames/s (SemanticKITTI) → l'exigence < 100 ms est exactement le facteur 10 manquant identifié page PDF 73–74.
7. **Stabilité métrique** (Fait 9) : la généralisation doit préserver le caractère min-max/single-linkage (stable au sens Gromov–Hausdorff) ; toute variante « complete/average-like » dans la conception des événements de fusion détruirait les garanties. Le cadre percolation (ch. 7, hors tranche) est l'outil d'évaluation théorique, pas un oracle d'implémentation.
8. **Fixtures de test canoniques offertes par la tranche** : (a) chaîne 2δ|i−j| vs clique 2δ (dendrogrammes SL identiques, fusion simultanée à δ — un algorithme K ≥ 2 doit les distinguer) ; (b) l'exemple de sélection Fig. 4.2 (partition finale {C₁, C₂, C₃}) pour tester une extraction excès-de-masse.
9. **Distinction couvert / membre** (Déf. 8 p. 21 livre, rappelée via les points accessibles de DBSCAN) : un point peut *participer* à un amas (être couvert) sans lui *appartenir*. La V4 devra respecter cette distinction dans la sémantique des sorties (elle annonce les poids w_{x,τ} = S_τ/T_x du § 9.1, hors tranche).

**Ce que la tranche n'impose pas (car hors périmètre)** : la définition de ρ(σ), miniboule/circumradius, Jung, triangles aigus, les K-polyèdres, la partition des (K−1)-simplexes — tout cela est en Partie II (pages livre ≥ 49, PDF ≥ 75). Rien dans cette tranche ne contraint donc directement les mécanismes q2/q3/q4/WSPD de la feuille de route V4 ; elle en fixe le cahier des charges amont (K, exactitude, forêt, niveaux, performance).

---

## Questions ouvertes / ambiguïtés

1. **Encadré tronqué page PDF 55** : le début de l'énoncé (items 1–2 et l'hypothèse portant sur T) est sur la page PDF 54, hors tranche. L'identification comme « Fait 7 » est une inférence à partir du renvoi « Fait 7, p. 28 » (page PDF 57), non une lecture directe.
2. **Constante de proportionnalité de Ê(C)** : le texte écrit « Ê(C) ∝ Σ_{x∈C}(λ̂_x − λ̂_min) » sans préciser la constante ni la normalisation ; l'implémentation de référence (hdbscan/scikit) doit servir de désambiguïsation si nécessaire.
3. **Définition exacte de r_x dans λ̂_x = 1/r_x** : « la distance à laquelle le point se détache » — vraisemblablement le niveau (en d_mreach) auquel x quitte le cluster dans l'arbre condensé, mais le lien formel avec d_mreach,K n'est pas explicité dans la tranche.
4. **Fait 11** : le « temps de tri, soit 𝒪(n log n) » est énoncé sans les hypothèses précises de Draganov et al. [14] (modèle de calcul, forme des objectifs admis) ; à vérifier dans [14] si la V4 veut s'en prévaloir.
5. **« K ≳ 10 »** : seuil qualitatif, non un théorème ; la justification théorique annoncée (« Nous expliquerons théoriquement ce phénomène dans la partie suivante », page PDF 66) est hors tranche (chapitres 6–7).
6. **Numérotation des pages dans les renvois internes** : les renvois « p. 58 » (Déf. 21, Déf. 22) sont en pagination livre → pages PDF ≈ 84 ; attention au décalage +26 pour toute future lecture ciblée.
7. **Facteur 1/2 dans u^SL et u^{RSL}** : les formules min max portent un facteur 1/2 (de même que sep(𝒳,d) ≜ ½ min d) — convention de « rayon » plutôt que « diamètre » vraisemblablement fixée aux chapitres 2–3, hors tranche ; à confirmer pour que la V4 étiquette ses niveaux de fusion dans la même unité que le manuscrit.
8. **RSL vs HDBSCAN dans le détail algorithmique** : la tranche décrit l'extraction HDBSCAN (élagage + stabilité) mais pas la construction précise de l'« arbre condensé » ; la sémantique candidate exacte à reproduire pour `select_excess_of_mass` doit être croisée avec [9, 10].
