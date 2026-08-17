# Lecture normative — MANUSCRIT_THESE_HAUSEUX.pdf, pages PDF 115–134 (pages imprimées 89–107)

Correspondance : page PDF 115 = page imprimée 89 ; PDF 120 = p. 94 (blanche) ; PDF 121 = p. 95 ; PDF 133 = p. 107 ; PDF 134 = p. 108 (blanche). Tranche = fin du Chapitre 8 (« À la recherche de la généralisation de l'arbre minimum couvrant ») + Chapitre 9 complet (« HGP-Clusterer en pratique »). Fin de la Partie II (qui se clôt page imprimée 107 / PDF 133).

Références *hors tranche* mais invoquées ici (à récupérer dans les tranches précédentes) : Déf. 27 (simplexe $K$-séparant, p. imprimée 86), Théorème 4 (les simplexes $K$-séparants sont de Gabriel), Fait 1 (Gabriel classique $\subseteq$ Delaunay), Fait 2 (composantes d'un graphe pondéré seuillé = composantes de son MST seuillé), Fait 12 (propriété de boule utilisée dans les preuves des Théorèmes 4, 6, 7 — non énoncé dans cette tranche), $\Gamma_K(\mathcal{X})$ (graphe des facettes de Čech), $\check{C}(\mathcal{X},r)$ (complexe de Čech), $K$-polyèdres, $\rho(\sigma)$ (rayon de naissance de $\sigma$ dans la filtration de Čech = rayon de la plus petite boule englobante $B_\sigma = \overline{B}(c_\sigma, r_\sigma)$), algorithme de Welzl [92].

---

## 1. Structure section par section

| Pages PDF | Pages imprimées | Contenu |
|---|---|---|
| 115 | 89 | § 8.2 (fin) : fin de la preuve du Théorème 4 ; **Définition 29** ($K$-graphe de Gabriel) |
| 116 | 90 | Fin Déf. 29 ; **Proposition 6** ; **Définition 30** ($K$-MST) |
| 117 | 91 | **Théorème 5** + preuve ; § 8.3 « La bonne structure à observer : la mosaïque de Delaunay d'ordre $K$ » ; **Définition 31** (début) |
| 118 | 92 | Fin Déf. 31 ; § 8.3.1 ; **Théorème 6** + début de preuve |
| 119 | 93 | Fin preuve Théorème 6 ; **Synthèse du chapitre 8** |
| 120 | 94 | Page blanche |
| 121 | 95 | **Chapitre 9 : HGP-Clusterer en pratique** — introduction |
| 122 | 96 | § 9.1 « Et lorsqu'on impose une partition stricte des données ? » : $\mathcal{F}_K$, $\ell(\tau)$, $S_\tau$, $T_x$, $m_\tau$ |
| 123 | 97 | Suite § 9.1 : $V_x(c)$, $\widehat{\ell}(x)$, **Proposition 7** ; § 9.2, § 9.2.1 (2-graphe de Gabriel) |
| 124 | 98 | **Proposition 8** (triangles aigus/obtus) + preuve + Fig. 9.1 ; § 9.2.2 « Digression au cas $K \geq 2$ » |
| 125 | 99 | Fin § 9.2.2 ; **Théorème 7** ; § 9.2.3 ; **Proposition 9** ($O(n \log n)$ dans le plan) + Remarque sur les approximations |
| 126 | 100 | **Algorithme 1** (HGP-Clusterer $K=2$, partition stricte) ; § 9.2.4 huiles d'olive (début) |
| 127 | 101 | Fin § 9.2.4, Tableau 9.1 ; § 9.2.5 SIPU (protocole) |
| 128 | 102 | Tableaux 9.2 et 9.3 (résultats exhaustifs et SIPU) |
| 129 | 103 | Analyse SIPU (cas `birch2`) ; § 9.3 Vietoris–Rips ; **Définition 32** |
| 130 | 104 | Remarque flag complex ; **Fait (connu) 13** (encadrement Čech–Rips, Jung) ; **Proposition 10** (clique percolation) |
| 131 | 105 | Les 4 opérations de graphes du pipeline Rips/GPU ; § 9.4 « vers la réduction de dimension ? » |
| 132 | 106 | § 9.4.1 UMAP ; **Fait (connu) 14** |
| 133 | 107 | Fin Fait 14 ($\mathcal{L}_{\mathrm{UMAP}}$) ; faiblesses d'UMAP ; **Synthèse du chapitre 9** |
| 134 | 108 | Page blanche |

---

## 2. Définitions et notations normatives

### Fin de la preuve du Théorème 4 (p. PDF 115 / impr. 89) — notations réutilisables

Soit $\sigma$ un $K$-simplexe, $B_\sigma$ sa plus petite boule englobante, $S = \sigma \cap \partial B_\sigma$ son ensemble de support. Les **facettes actives** de $\sigma$ (celles existant strictement avant $r = \rho(\sigma)$) sont exactement $\tau_s \triangleq \sigma \setminus \{s\}$, $s \in S$. Pour $s, t \in S$, $s \neq t$, et $z$ un point extérieur, la preuve introduit les $K$-simplexes $\sigma_s^z \triangleq (\sigma \setminus \{s\}) \cup \{z\}$ et $\sigma_t^z \triangleq (\sigma \setminus \{t\}) \cup \{z\}$, contenant les facettes $\tau_s$, $\eta_{s,t}^z \triangleq (\sigma \setminus \{s,t\}) \cup \{z\}$ et $\tau_t$. Il suffit de vérifier $\rho(\sigma_s^z) < r$ et $\rho(\sigma_t^z) < r$ : $\sigma_s^z$ est contenu dans $B_\sigma$ et ses points sur la frontière de $B_\sigma$ sont dans $S \setminus \{s\}$ ; d'après le **Fait 12**, $\rho(\sigma_s^z) < r$. Toutes les facettes $\tau_s, \tau_t$ sont alors reliées dans $\Gamma_K(\mathcal{X})_{<r}$ par le chemin $\tau_s \leftrightarrow_r^{\sigma_s^z} \eta_{s,t}^z \leftrightarrow_r^{\sigma_t^z} \tau_t$. Conclusion : un simplexe qui contient un point intrus dans $\mathring{B}_\sigma$ n'a aucune influence sur les $K$-polyèdres ; il n'est pas $K$-séparant.

### Définition 29 : $K$-graphe de Gabriel (p. PDF 115–116 / impr. 89–90)

Le $K$-graphe de Gabriel de $\mathcal{X}$, noté $\mathcal{G}_K^{\mathrm{Gab}}(\mathcal{X})$, est le sous-graphe pondéré de $\Gamma_K(\mathcal{X})$ défini ainsi :
- Ses sommets sont les $(K-1)$-simplexes de $\mathcal{X}$ qui sont facettes d'au moins un $K$-simplexe de Gabriel.
- Pour chaque $K$-simplexe de Gabriel $\sigma$, on relie entre elles toutes les facettes de $\sigma$ (formant une clique).
- Toutes les arêtes induites par $\sigma$ reçoivent le poids $\rho(\sigma)$.

Remarque du texte : pour $K = 1$, ce graphe n'est autre que le graphe de Gabriel classique, pondéré par la moitié de la longueur des arêtes. Pour $K \geq 2$, ce n'est plus un graphe sur les points, mais un graphe sur les $(K-1)$-simplexes.

### Définition 30 : $K$-arbre minimum couvrant (p. PDF 116–117 / impr. 90–91)

Un $K$-arbre minimum couvrant, noté $K$-MST, est un arbre minimum couvrant du graphe pondéré $\mathcal{G}_K^{\mathrm{Gab}}(\mathcal{X})$. Pour $r \geq 0$, on note $K\text{-MST}_{\leq r}$ le sous-graphe obtenu en supprimant les arêtes de poids strictement supérieur à $r$.

### Définition 31 : Cellules de Voronoï d'ordre supérieur et mosaïque de Delaunay duale [62, 93] (p. PDF 117–118 / impr. 91–92)

Soit $\mathcal{X} \subseteq \mathbb{R}^p$ un ensemble fini, et $k \in \{1, \dots, |\mathcal{X}|-1\}$. Pour tout sous-ensemble $Q \subset \mathcal{X}$ de cardinal $k$, la cellule de Voronoï d'ordre $k$ est
$$\mathrm{Vor}_k(Q) \triangleq \left\lbrace y \in \mathbb{R}^p \;\middle|\; \max_{q \in Q} \Vert y - q \Vert \leq \min_{x \in \mathcal{X} \setminus Q} \Vert y - x \Vert \right\rbrace.$$
Autrement dit, $\mathrm{Vor}_k(Q)$ est l'ensemble des points de l'espace pour lesquels les points de $Q$ sont les $k$ plus proches voisins dans $\mathcal{X}$.

La mosaïque de Delaunay d'ordre $k$, notée $\mathrm{Del}_k(\mathcal{X})$, est le complexe dual du diagramme de Voronoï d'ordre $k$ : ses sommets sont les $k$-uplets $Q \subset \mathcal{X}$ tels que $\mathrm{Vor}_k(Q) \neq \emptyset$, et plus généralement
$$\{Q_0, \dots, Q_m\} \in \mathrm{Del}_k(\mathcal{X}) \iff \bigcap_{i=0}^m \mathrm{Vor}_k(Q_i) \neq \emptyset.$$
En particulier, deux sommets $Q, Q'$ de $\mathrm{Del}_k(\mathcal{X})$ sont **adjacents** dès qu'il existe un point $y \in \mathbb{R}^p$ pour lequel $Q$ et $Q'$ sont deux choix possibles de $k$ plus proches voisins.

**Notion de portage** : un $k$-simplexe $\sigma \subset \mathcal{X}$ est *porté par la mosaïque de Delaunay d'ordre $k$* s'il existe deux sommets adjacents $Q, Q'$ de $\mathrm{Del}_k(\mathcal{X})$ tels que $Q \cup Q' = \sigma$.

### § 9.1 — Notations de la partition stricte (p. PDF 122–123 / impr. 96–97)

Point fondamental : pour $K \geq 2$, l'objet naturel de HGP-Clusterer n'est **pas** une partition de $\mathcal{X}$, mais un **recouvrement** de $\mathcal{X}$ (ou bien une **partition des $(K-1)$-simplexes**). Un point peut appartenir à plusieurs $K$-polyèdres, parce qu'il peut apparaître dans plusieurs $(K-1)$-simplexes situés dans des composantes différentes de $\Gamma_K(\mathcal{X})_r$ ; c'est précisément l'information géométrique portée par les intersections d'ordre supérieur.

- $\mathcal{F}_K$ : ensemble des $(K-1)$-simplexes effectivement construits par l'algorithme (les sommets du graphe dual utilisé par HGP-Clusterer ; dans la version standard, $\mathcal{F}_K$ correspond aux **simplexes de Gabriel**).
- Étiquette de cluster après condensation de l'arbre et sélection par excès de masse (comme HDBSCAN [10]) : $\ell(\tau) \in \{0, \dots, q_{\mathrm{clust}} - 1\}$, ou $\ell(\tau) = -1$ (bruit).
- **Score local de face** :
$$S_\tau \triangleq \sum_{\substack{\sigma \supset \tau \\ |\sigma| = K+1}} \psi\big(\rho(\sigma)\big), \qquad \psi(t) = \frac{1}{t^p},$$
où $\rho(\sigma)$ désigne le rayon de naissance du $K$-simplexe $\sigma$ dans la filtration. Plus généralement, $\psi$ peut être remplacée par toute fonction de poids **décroissante**. Le choix uniforme $\psi \equiv 1$ est possible, mais $1/t^p$ reflète plus exactement la densité locale, au cœur du modèle mathématique derrière HGP-Clusterer.
- **Normalisation par point** :
$$T_x \triangleq \sum_{\substack{\tau \in \mathcal{F}_K \\ x \in \tau}} S_\tau.$$
- **Masse d'une face dans l'arbre condensé** :
$$m_\tau \triangleq S_\tau \sum_{x \in \tau} \frac{1}{T_x},$$
avec la convention $1/T_x = 0$ lorsque $T_x = 0$. Lorsqu'un point appartient à au moins une face, il distribue une masse totale égale à $1$ entre les faces qui le contiennent. C'est ce poids $m_\tau$, et **non** le simple comptage des faces, qui est utilisé par le seuil `min_cluster_size` dans l'arbre condensé (mêmes idées algorithmiques que HDBSCAN).
- **Vote pondéré** : pour tout point $x \in \mathcal{X}$ et tout cluster $c$,
$$V_x(c) \triangleq \sum_{\substack{\tau \in \mathcal{F}_K \\ x \in \tau \\ \ell(\tau) = c}} \frac{S_\tau}{T_x},$$
et le point reçoit l'étiquette $\widehat{\ell}(x) \in \operatorname{argmax}_c V_x(c)$.

(NB : c'est ici que vit la « partition de l'unité » $w_{x\tau} = S_\tau / T_x$ citée par CLAUDE.md à propos du § 9.1 — le terme individuel du vote.)

### Définition 32 : Complexe de Vietoris–Rips [37, 63] (p. PDF 129 / impr. 103)

Soit $(\mathcal{X}, d)$ un espace métrique fini et $r \geq 0$. Le complexe de Vietoris–Rips de rayon $r$ est
$$\mathrm{VR}(\mathcal{X}, r) \triangleq \left\lbrace \sigma \subseteq \mathcal{X} \;\middle|\; \forall x, x' \in \sigma,\ d(x, x') \leq 2r \right\rbrace.$$
Autrement dit, $\sigma \in \mathrm{VR}(\mathcal{X}, r)$ si et seulement si tous ses sommets sont deux à deux reliés dans le graphe seuil $G_r = (\mathcal{X}, E_r)$, $E_r \triangleq \{\{x, x'\} \subseteq \mathcal{X} \mid d(x, x') \leq 2r\}$.

Remarque (p. impr. 104) : Čech et Vietoris–Rips ont le même 1-squelette (le graphe seuil) ; VR est le plus grand complexe constructible sur ce graphe (*flag complex* / complexe de drapeau). Généralisable à un graphe pondéré avec arêtes absentes de poids $+\infty$ (matrices de distance creuses).

---

## 3. Théorèmes, propositions, faits

### Proposition 6 : Le graphe de Gabriel contient toutes les fusions utiles (p. PDF 116 / impr. 90)

**Énoncé.** Pour tout $r \geq 0$, les ensembles de points associés aux composantes connexes non réduites à un sommet isolé de $\mathcal{G}_K^{\mathrm{Gab}}(\mathcal{X})_{\leq r}$ coïncident avec les $K$-polyèdres de $\check{C}(\mathcal{X}, r)$ non réduits à un $(K-1)$-simplexe isolé.

**Preuve (esquisse, algorithmiquement éclairante).** Inclusion triviale $\mathcal{G}_K^{\mathrm{Gab}}(\mathcal{X})_{\leq r} \subseteq \Gamma_K(\mathcal{X})_{\leq r}$. Induction sur les valeurs critiques $r$ croissantes : au niveau $r$, si un $K$-simplexe $\sigma$ ajouté est de Gabriel, il est ajouté dans les deux graphes. S'il ne l'est pas, la preuve du Théorème 4 montre que toutes ses facettes nées **strictement avant** $r$ sont déjà reliées entre elles par des chemins de poids strictement plus petits que $r$. Les autres facettes de $\sigma$ naissent au niveau $r$ lui-même ; elles peuvent être ajoutées comme sommets, mais ne changent pas l'ensemble de points de la composante, car l'union des facettes actives $\{\sigma \setminus \{s\} \mid s \in \sigma \cap \partial B_\sigma\}$ contient déjà tous les sommets de $\sigma$. Ajouter $\sigma$ ne crée donc aucune fusion nouvelle entre ensembles de points. Les simplexes non-Gabriel peuvent être supprimés sans modifier les $K$-polyèdres non triviaux. $\square$

### Théorème 5 : Complexe de Čech ≡ $K$-arbre minimum couvrant élagué (p. PDF 117 / impr. 91)

**Énoncé.** Supposons $\mathcal{X} \subset \mathbb{R}^p$ en position générale pour la filtration de Čech. Pour tout $r \geq 0$, les ensembles de points associés aux composantes connexes de $K\text{-MST}_{\leq r}$ qui ne sont pas réduites à un sommet isolé coïncident avec les $K$-polyèdres de $\check{C}(\mathcal{X}, r)$ qui ne sont pas réduits à un $(K-1)$-simplexe isolé.

**Preuve.** Fait 2 appliqué au graphe pondéré $\mathcal{G}_K^{\mathrm{Gab}}(\mathcal{X})$ donne, pour tout $r \geq 0$, l'égalité des composantes connexes de $K\text{-MST}_{\leq r}$ et de $\mathcal{G}_K^{\mathrm{Gab}}(\mathcal{X})_{\leq r}$. La Prop. 6 identifie ensuite ces composantes non triviales avec les $K$-polyèdres non triviaux de $\check{C}(\mathcal{X}, r)$. $\square$

### Théorème 6 : Les $K$-simplexes de Gabriel sont portés par la mosaïque de Delaunay d'ordre $K$ (p. PDF 118–119 / impr. 92–93)

**Énoncé.** Soit $\mathcal{X} \subset \mathbb{R}^p$ un nuage fini en position générale. Tout $K$-simplexe de Gabriel $\sigma \subseteq \mathcal{X}$ est porté par la mosaïque de Delaunay d'ordre $K$, au sens de la Déf. 31.

**Preuve (complète, éclairante pour l'identification des événements).** $B_\sigma = \overline{B}(c_\sigma, r_\sigma)$ plus petite boule fermée contenant $\sigma$. Gabriel $\Rightarrow$ $\mathring{B}_\sigma \cap (\mathcal{X} \setminus \sigma) = \varnothing$ ; en position générale, aucun point de $\mathcal{X} \setminus \sigma$ sur la sphère $\partial B_\sigma$. Décomposition des sommets : $I \triangleq \sigma \cap \mathring{B}_\sigma$, $S \triangleq \sigma \cap \partial B_\sigma$. Les points de $I$ sont strictement plus proches du centre $c_\sigma$ que les points de $S$ ; tous les points de $S$ sont à distance exactement $r_\sigma$ de $c_\sigma$. **La boule $B_\sigma$ étant minimale, $|S| \geq 2$.** On choisit $s, t \in S$ distincts et on considère les deux $K$-uplets $Q_s \triangleq \sigma \setminus \{s\}$, $Q_t \triangleq \sigma \setminus \{t\}$. Au point $c_\sigma$, tous les points de $I$ sont strictement plus proches que les points de $S$, et aucun point extérieur à $\sigma$ n'est plus proche que les points de $S$. Ainsi $Q_s$ et $Q_t$ sont deux choix possibles de $K$ plus proches voisins de $c_\sigma$ : $c_\sigma \in \mathrm{Vor}_K(Q_s) \cap \mathrm{Vor}_K(Q_t)$, donc $Q_s$ et $Q_t$ sont adjacents dans $\mathrm{Del}_K(\mathcal{X})$, et $Q_s \cup Q_t = (\sigma \setminus \{s\}) \cup (\sigma \setminus \{t\}) = \sigma$. $\square$

### Synthèse du chapitre 8 (p. PDF 119 / impr. 93) — texte normatif de la chaîne de réduction

« Le rôle de l'arbre minimum couvrant dans le Single-Linkage repose sur un principe plus général : dans une filtration, seules les cellules qui changent effectivement la connectivité doivent être conservées pour reconstruire les composantes à toutes les échelles. Pour les $K$-polyèdres, ces cellules sont les simplexes $K$-séparants. Le Théorème 4 montre qu'ils satisfont nécessairement une condition de vacuité : ils sont de Gabriel. On peut donc remplacer le graphe complet des facettes de Čech par le graphe beaucoup plus mince des simplexes de Gabriel, puis calculer un $K$-arbre minimum couvrant. L'élagage de cet arbre redonne exactement les mêmes $K$-polyèdres non triviaux que la hiérarchie complète sur le complexe de Čech (Théorème 5). La structure géométrique qui organise ces simplexes est la mosaïque de Delaunay d'ordre $K$ (Théorème 6). »

### Proposition 7 : Partition stricte induite par le vote pondéré (p. PDF 123 / impr. 97)

**Énoncé.** Fixons une sélection de clusters sur les faces de $\mathcal{F}_K$ et une règle **déterministe** de départage des égalités. La procédure précédente définit une partition disjointe
$$\mathcal{X} = \widehat{C}_{-1} \sqcup \widehat{C}_0 \sqcup \cdots \sqcup \widehat{C}_{q_{\mathrm{clust}}-1}, \qquad \widehat{C}_c \triangleq \{x \in \mathcal{X} \mid \widehat{\ell}(x) = c\},$$
où $\widehat{C}_{-1}$ est l'ensemble des points non classés.

Commentaire du texte : pour $K = 1$, les faces sont les points eux-mêmes ; le vote est trivial et restitue exactement les étiquettes du Single-Linkage après sélection des composantes. Pour $K \geq 2$, il remplace l'appartenance multiple aux $K$-polyèdres par une décision locale, pondérée par la densité de naissance des faces incidentes.

### Proposition 8 : Le 2-graphe de Gabriel se lit dans la triangulation de Delaunay [65] (p. PDF 124 / impr. 98)

**Énoncé.** Les triangles **aigus** de Gabriel sont des triangles de Delaunay, puisque leur sphère minimale est leur sphère circonscrite. Les triangles **obtus** de Gabriel ont pour boule minimale le disque ayant pour diamètre leur plus long côté ; dans ce cas, les **deux petites arêtes** appartiennent à la triangulation de Delaunay, ce qui suffit pour énumérer les triangles de Gabriel (aigus et obtus) sans parcourir tous les triplets de points.

**Preuve.** La boule ayant pour diamètre le plus grand côté ne contient qu'un seul point (le troisième sommet du triangle obtus). Il existe donc des boules contenant les plus petites arêtes et vides d'autres points du nuage $\mathcal{X}$ ; par conséquent ces arêtes sont de Delaunay. (Fig. 9.1 : triangle obtus $\sigma = \{x_1, x_2, x_3\}$, boule minimale $B_\sigma$ de diamètre $[x_2, x_3]$ vide de tout « intrus » ; deux disques $D_{x_3}$ et $D_{x_2}$ inscrits dans $B_\sigma$, donc vides, attestent que $[x_1, x_2]$ et $[x_1, x_3]$ sont de Delaunay.) $\square$

### § 9.2.2, digression $K \geq 2$ (p. PDF 124–125 / impr. 98–99) et Théorème 7

Généralisation de la mécanique de Prop. 8 : la mosaïque $\mathrm{Del}_{K-1}(\mathcal{X})$ d'ordre $K-1$ a pour sommets les $Q \subset \mathcal{X}$ de cardinal $K-1$ à cellule de Voronoï d'ordre $K-1$ non vide. Les **arêtes élémentaires** de cette mosaïque (reliant deux sommets $Q, Q'$ tels que $|Q \cup Q'| = K$) portent un $(K-1)$-simplexe $\tau = Q \cup Q'$. Ces arêtes élémentaires fournissent des candidats pour les **sommets** de $\Gamma_K(\mathcal{X})$. Pour les **arêtes** de $\Gamma_K(\mathcal{X})$ : soit $\sigma$ un $K$-simplexe de Gabriel, $|\sigma| = K+1$, $B_\sigma = B(c_\sigma, \rho(\sigma))$ sa plus petite boule englobante, $S = \sigma \cap \partial B_\sigma$ son ensemble de support, $|S| \geq 2$. Pour tout $s \in S$, la facette active $\tau_s = \sigma \setminus \{s\}$ est portée par $\mathrm{Del}_{K-1}(\mathcal{X})$ (Fait 12) : il existe deux sommets adjacents $Q, Q'$ de $\mathrm{Del}_{K-1}(\mathcal{X})$ tels que $Q \cup Q' = \tau_s$. En choisissant $s, t \in S$ distincts, $\tau_s = \sigma \setminus \{s\}$, $\tau_t = \sigma \setminus \{t\}$ et $\tau_s \cup \tau_t = \sigma$.

**Théorème 7 : Le $K$-graphe de Gabriel se lit dans la mosaïque de Delaunay $\mathrm{Del}_{K-1}$ d'ordre $K-1$** (p. PDF 125 / impr. 99).
**Énoncé.** Tout $K$-simplexe de Gabriel peut être identifié à partir de deux $(K-1)$-simplexes portés par des arêtes élémentaires de $\mathrm{Del}_{K-1}(\mathcal{X})$.

Corollaire algorithmique du texte : « pour retrouver la hiérarchie complète des niveaux K-NN, il suffit de construire la mosaïque $\mathrm{Del}_{K-1}(\mathcal{X})$ d'ordre $K-1$ ». **Note de bas de page i)** : il y a tout de même un coût supplémentaire non négligeable en grande dimension : trouver le rayon de filtration $r_\sigma$ des simplexes $\sigma$ candidats, par exemple au moyen de l'algorithme de Welzl [92] (et éventuellement tester qu'ils sont bien de Gabriel).

### Proposition 9 : Extraction planaire du 2-graphe de Gabriel [65] (p. PDF 125 / impr. 99)

**Énoncé.** Soit $\mathcal{X} \subset \mathbb{R}^2$ un nuage fini en position générale. Tous les triangles de Gabriel de $\mathcal{X}$ peuvent être extraits à partir de la triangulation de Delaunay ordinaire de $\mathcal{X}$. En particulier, le graphe dual des arêtes reliées par les triangles de Gabriel peut être construit en temps $O(n \log n)$ dans le plan.

**Remarque normative associée (p. impr. 99–100)** : en dimension plus grande, la mise en œuvre exacte devient rapidement limitée par la construction de la mosaïque $\mathrm{Del}_K$. Des approximations de type Rips, une réduction de dimension préalable, un sous-échantillonnage suivi d'une propagation par $k$-plus proches voisins, ou une recherche restreinte à des listes de voisins sont des options praticables. « Elles doivent toutefois être distinguées de la construction géométrique exacte : **dès qu'une approximation remplace la mosaïque exacte, les garanties d'équivalence avec les $K$-polyèdres exacts ne valent plus sans hypothèses supplémentaires.** »

### Algorithme 1 : HGP-Clusterer pour $K = 2$ avec sortie en partition stricte (p. PDF 126 / impr. 100)

Entrée : nuage de points $\mathcal{X} \subset \mathbb{R}^p$, seuil `min_cluster_size`, choix de poids $\psi$. Sortie : clustering sur $\mathcal{X}$.
1. Construire la triangulation de Delaunay $\mathrm{Del}(\mathcal{X})$.
2. Extraire les triangles de Gabriel : triangles **aigus** de $\mathrm{Del}(\mathcal{X})$ satisfaisant le test de vacuité, puis triangles **obtus** énumérés à partir des couples d'arêtes de Delaunay incidentes (voir Prop. 8).
3. Construire le graphe dual $G_2$ dont les sommets sont les arêtes $\tau = \{x_i, x_j\}$ apparaissant comme facettes d'au moins un triangle de Gabriel.
4. Pour chaque triangle $\sigma = \{x_i, x_j, x_k\}$, relier **(deux de)** ses trois arêtes-facettes dans $G_2$ avec une fusion de poids $\rho(\sigma)$.
5. Calculer les scores de faces $S_\tau = \sum_{\sigma \supset \tau} \psi(\rho(\sigma))$ et les masses $m_\tau = S_\tau \sum_{x \in \tau} T_x^{-1}$.
6. Extraire un arbre couvrant minimal $T_2$ de $G_2$ par Kruskal.
7. Condenser $T_2$ avec le seuil `min_cluster_size` [10] et sélectionner les clusters par excès de masse.
8. Propager les étiquettes des clusters aux points par le vote pondéré.
9. Optionnellement, réaffecter les points non classés par 1-plus proche voisin parmi les points déjà étiquetés.

### Fait (connu) 13 : Encadrement Čech–Rips dans $\mathbb{R}^p$ [37] (p. PDF 130 / impr. 104)

Soit $\mathcal{X} \subset \mathbb{R}^p$ un nuage localement fini. Pour tout $r \geq 0$,
$$\check{C}(\mathcal{X}, r) \subseteq \mathrm{VR}(\mathcal{X}, r) \subseteq \check{C}(\mathcal{X}, \alpha_p r), \qquad \alpha_p \triangleq \sqrt{\frac{2p}{p+1}} \leq \sqrt{2}.$$
En particulier, tout $K$-polyèdre de $\check{C}(\mathcal{X}, r)$ est contenu dans un $K$-polyèdre de $\mathrm{VR}(\mathcal{X}, r)$, et tout $K$-polyèdre de $\mathrm{VR}(\mathcal{X}, r)$ est contenu dans un $K$-polyèdre de $\check{C}(\mathcal{X}, \alpha_p r)$.

**Preuve.** Première inclusion immédiate (même 1-squelette, VR flag complex maximal). La seconde est une conséquence du **théorème de Jung** [101] : un ensemble de diamètre au plus $2r$ dans $\mathbb{R}^p$ est contenu dans une boule de rayon au plus $\alpha_p r$. $\square$
(En dimension $p = 3$ : $\alpha_3 = \sqrt{6}/2 \approx 1{,}2247$.) L'approximation devient de plus en plus mauvaise quand $p$ croît.

### Proposition 10 : Les $K$-polyèdres de Rips comme percolation de cliques (p. PDF 130 / impr. 104)

**Énoncé.** Fixons $r \geq 0$ et notons $G_r$ le graphe seuil associé à $\mathrm{VR}(\mathcal{X}, r)$. Les $K$-simplexes de $\mathrm{VR}(\mathcal{X}, r)$ sont exactement les cliques de taille $K+1$ de $G_r$. Les $K$-polyèdres non triviaux de $\mathrm{VR}(\mathcal{X}, r)$ correspondent donc aux usuelles composantes de *clique percolation* [66].

### Pipeline GPU en quatre opérations de graphes (p. PDF 130–131 / impr. 104–105)

« L'intérêt pratique majeur de Vietoris–Rips est qu'il ouvre la porte à des implémentations massivement parallèles, en particulier sur GPU. La géométrie euclidienne exacte disparaît au profit de quatre opérations de graphes, dont les trois premières (qui représentent le coût véritable de l'algorithme) sont massivement parallélisables. »
1. **Construction du graphe de voisinage.** Données euclidiennes de grande dimension : graphe (approché) de voisinages, se parallélise naturellement. Entrée déjà en matrice creuse : étape sautée.
2. **Énumération des cliques.** Pour $K = 2$, il suffit d'énumérer les triangles du graphe : intersections de listes d'adjacence triées (cas le plus favorable). Pour $K$ fixé plus grand : $(K+1)$-cliques par intersections récursives de voisinages. La complexité dépend du nombre réel de cliques du graphe parcimonieux (*sparse*), non de $\binom{n}{K+1}$.
3. **Construction du graphe dual.** Chaque $(K+1)$-clique génère $K+1$ faces de taille $K$. Produire ces faces, les trier ou les hacher, agréger leurs poids $S_\tau$, puis écrire les arêtes du graphe dual.
4. **Percolation, arbre couvrant et condensation.** Union–Find [29] + Kruskal [27], ou de façon plus élaborée (et parallélisable) un algorithme de type Borůvka.

### Fait (connu) 14 : Principe algorithmique d'UMAP [102] (p. PDF 132–133 / impr. 106–107)

Graphe orienté des $k_{\mathrm{nn}}$ plus proches voisins ; pour chaque $x_i$, distance locale minimale $\rho_i$ et échelle $\sigma_i > 0$, puis $p_{j|i} = \exp\left(-\frac{\max\{0,\, d(x_i, x_j) - \rho_i\}}{\sigma_i}\right)$ ; symétrisation par union floue $w_{ij} = p_{j|i} + p_{i|j} - p_{j|i} p_{i|j}$ ; dans l'espace latent, $\widetilde{w}_{ij}(Y) = \frac{1}{1 + a \Vert y_i - y_j \Vert^{2b}}$ ($a, b > 0$ paramètres) ; l'optimisation minimise l'entropie croisée
$$\mathcal{L}_{\mathrm{UMAP}}(Y) = -\sum_{i<j} \left[ w_{ij} \log \widetilde{w}_{ij}(Y) + (1 - w_{ij}) \log\big(1 - \widetilde{w}_{ij}(Y)\big) \right].$$
Faiblesse : malgré le vocabulaire topologique, UMAP exploite essentiellement un graphe (1-squelette, relations binaires). La mosaïque d'ordre $K$, elle, encode nativement des configurations de $K+1$ points, leurs rayons de naissance, leurs cellules duales et leurs adjacences. § 9.4 = direction de recherche **ouverte** (réduction de dimension d'ordre supérieur), sans algorithme fini, `public_status` implicitement non revendiqué.

---

## 4. Résultats empiriques (chiffres exacts, chapitre 9)

### Huiles d'olive italiennes (§ 9.2.4, p. PDF 126–128 / impr. 100–102)

Jeu [96] : 572 échantillons, 8 concentrations d'acides gras, $\mathcal{X} \subset \mathbb{R}^8$ (matrice $572 \times 8$), colonnes centrées puis normalisées en norme $\ell^2$ comme dans [25]. Neuf provinces, trois macro-régions (Italie du Sud, Sardaigne, Centre-Nord). HDBSCAN : les trois macro-régions, pas les neuf provinces. Persistable [25] (état de l'art) : huit des neuf provinces (Sicile non identifiée), classe directement 279 points ($\approx 49\%$). HGP-Clusterer [65], $K = 2$ : huit groupes interprétables (Sicile absorbée par Calabre et Pouilles du Nord), classe directement 412 points ($\approx 72\%$), avec $396/412 \approx 96{,}1\%$ d'étiquettes correctes. Extension exhaustive par 1-plus proche voisin : $521/572 \approx 91{,}1\%$ correctes, ARI $= 0{,}890$ ; Persistable exhaustif : $499/572 \approx 87{,}2\%$, ARI $= 0{,}848$.

### SIPU (§ 9.2.5, p. PDF 127–129 / impr. 101–103)

Benchmark SIPU [99] via ClustBench [100]. Écartés : `compound`, `flame`, `pathbased`, `r15` (vérité terrain multiple, ambiguë) et `worms_64` (dimension 64 : Delaunay incalculable). Protocole strictement apparié avec HDBSCAN [10] : même `min_cluster_size` $= \sqrt{n}$, même estimateur de densité $\widehat{\rho} = 1/r$, même critère d'excès de masse ($\psi(t) = \frac{1}{t}$). **Seule différence : le type de connexité** — connexité standard de graphe géométrique pour HDBSCAN, connexité par triangles de Čech pour HGP-Clusterer avec $K = 2$.

Tableau 9.3 (HDBSCAN → HGP-Clusterer, format $k$ / ARI / % classés) :
- a1 ($n=3000$, $k=20$) : 17 / 0,745 / 94,9% → 17 / **0,778** / 97,7%
- a2 ($5250$, $35$) : 33 / 0,762 / 92,1% → 32 / **0,834** / 96,2%
- a3 ($7500$, $50$) : 43 / 0,696 / 95,0% → 45 / **0,831** / 97,1%
- aggregation ($788$, $7$) : 5 / 0,809 / 100% → 5 / 0,809 / 100%
- birch1 ($100\,000$, $100$) : 98 / 0,215 / 79,7% → 100 / **0,298** / 83,1%
- birch2 ($100\,000$, $100$) : **100 / 0,996 / 99,7%** → 84 / 0,441 / 83,9%
- d31 ($3100$, $31$) : 27 / 0,659 / 90,0% → 30 / **0,835** / 94,5%
- jain ($373$, $2$) : 3 / 0,935 / 98,4% → 3 / 0,935 / 98,7%
- s1 ($5000$, $15$) : 15 / 0,961 / 97,6% → 15 / **0,976** / 99,0%
- s2 ($5000$, $15$) : 15 / 0,783 / 88,9% → 15 / **0,842** / 91,9%
- s3 ($5000$, $15$) : 15 / 0,251 / 65,9% → 15 / **0,418** / 77,2%
- s4 ($5000$, $15$) : 15 / 0,258 / 67,4% → 15 / **0,284** / 69,6%
- spiral ($312$, $3$) : 3 / 1,000 / 100% → 3 / 1,000 / 100%
- unbalance ($6500$, $8$) : 8 / 1,000 / 99,9% → 8 / 1,000 / 100%
- worms_2 ($105\,600$, $35$) : 15 / 0,055 / 74,2% → 33 / 0,076 / 66,4%

Analyse (p. impr. 103) : amélioration nette sur a2, a3, d31, s2, s3 ; proportion classée plus grande car la connexité triangulaire évite les filaments instables (« trous » dont les points intérieurs sont « perdus »). **Exception `birch2`** : clusters essentiellement **filiformes**, mieux identifiés avec de simples graphes ; HGP-Clusterer fusionne intempestivement. Poser $\widehat{\rho} = 1/r^2$ résout le problème (HGP-Clusterer quasi-parfait pour le nombre de clusters). — Cohérent avec la contrainte connue du dossier SemanticKITTI : HGP retarde la naissance des objets filiformes.

### Coûts cités

- Prop. 9 : $O(n \log n)$ pour tout le graphe dual $G_2$ dans le plan.
- Triangulation de Delaunay ordinaire : complexité au pire cas $O(n^{\lceil p/2 \rceil})$ dans $\mathbb{R}^p$ [37] (p. impr. 103) — soit $O(n^2)$ au pire dans $\mathbb{R}^3$.
- Rips/cliques : dépend du nombre réel de cliques du graphe parcimonieux, pas de $\binom{n}{K+1}$.
- Welzl [92] pour $r_\sigma$ (note i, p. impr. 99).

---

## 5. Pertinence pour la conception V4

### Ce que la tranche impose (chaîne normative de l'objet à calculer)

1. **L'objet exact est entièrement déterminé par les $K$-simplexes de Gabriel et leurs poids $\rho(\sigma)$.** Théorème 5 : la forêt/hiérarchie des $K$-polyèdres non triviaux de Čech = composantes du $K$-MST élagué de $\mathcal{G}_K^{\mathrm{Gab}}(\mathcal{X})$ (Déf. 29–30). La V4 n'a donc besoin que de : (i) l'ensemble des $K$-simplexes de Gabriel (ou un sur-ensemble contenant toutes les fusions utiles, cf. Prop. 6 qui autorise des sommets surnuméraires nés au niveau $r$ tant qu'ils ne créent pas de fusion), (ii) leurs rayons $\rho(\sigma)$ exacts, (iii) un MST/forêt sur le graphe dual. C'est exactement « identification exacte des événements + reconstruction de la forêt » de la feuille de route.
2. **Test de Gabriel = test de vacuité de la miniboule** : $\mathring{B}_\sigma \cap (\mathcal{X} \setminus \sigma) = \varnothing$. C'est la justification normative de l'« élimination par témoins » de la V4 : un témoin dans la zone cœur (la boule ouverte $\mathring{B}_\sigma$) tue le candidat. La preuve du Théorème 4 (p. impr. 89) montre plus : un simplexe avec intrus n'est pas $K$-séparant, ses facettes actives sont déjà connectées **strictement avant** $r$ — l'élimination est donc sans perte pour la forêt.
3. **Structure de support $S = \sigma \cap \partial B_\sigma$, $|S| \geq 2$** (preuve Théorème 6). En $\mathbb{R}^3$, le support d'une miniboule a 2, 3 ou 4 points : c'est le fondement exact de la taxonomie q2/q3/q4 de la V4. La Prop. 8 en est le cas $K=2$ planaire : support de taille 2 (triangle obtus, $\rho = $ moitié du plus long côté, la miniboule est diamétrale) vs support de taille 3 (triangle aigu, $\rho = $ circumradius). La condition V4 « troisième témoin $x$ formant un triangle aigu » est exactement la condition du manuscrit pour qu'un support d'arité 3 existe ; de même q4 exige que le circumcentre du tétraèdre soit « du bon côté » de chaque face (non explicité dans la tranche, mais c'est la généralisation directe de aigu/obtus).
4. **Seules les facettes actives fusionnent** : les facettes $\tau_s = \sigma \setminus \{s\}$, $s \in S$, existent strictement avant $r$ ; les facettes $\sigma \setminus \{i\}$, $i \in I$ (points intérieurs) naissent au niveau $r$ lui-même et **ne créent aucune fusion nouvelle** (preuve Prop. 6). Conséquence V4 importante : un événement de poids $\rho(\sigma)$ ne doit relier entre elles que les facettes indexées par le support $S$ — relier les autres au même poids ne change pas les composantes (elles sont couvertes par l'union des facettes actives), mais la sémantique fine (qui naît, qui fusionne) est portée par $S$. L'Alg. 1, étape 4, montre qu'un chevauchement en chemin (« relier (deux de) ses trois arêtes-facettes ») suffit pour la connectivité : pas besoin de matérialiser la clique complète de la Déf. 29.
5. **Rendu partition stricte (§ 9.1)** : la V4 doit pouvoir produire, par $K$ : les scores $S_\tau = \sum_{\sigma \supset \tau} \psi(\rho(\sigma))$ (agrégation par facette sur les seuls simplexes retenus $\mathcal{F}_K$), $T_x$, $m_\tau = S_\tau \sum_{x \in \tau} 1/T_x$ (masse utilisée par `min_cluster_size`), et le vote $V_x(c) = \sum S_\tau / T_x$. $\psi$ décroissante paramétrable, défaut $1/t^p$ ; le choix de $\widehat{\rho}$ ($1/r$ vs $1/r^2$) change le comportement sur les clusters filiformes (birch2) — à exposer comme paramètre, pas à figer.
6. **Position générale non supposée** : les Théorèmes 5–7 sont énoncés « en position générale ». Le profil d'entrée v3/v4 (u16 quantifié, dégénérescences → refus explicite) est compatible : hors position générale, la tranche ne garantit rien, le refus est la seule sortie honnête.
7. **Pipeline GPU de référence (p. impr. 104–105)** : le manuscrit lui-même décrit le schéma « énumération de candidats → génération des $K+1$ faces → tri/hachage + agrégation de $S_\tau$ → arêtes duales → Union–Find/Kruskal ou Borůvka (parallélisable) ». La V4 (WSPD + dual-tree au lieu de cliques de Rips) doit produire les mêmes interfaces de sortie : flux d'événements $(\sigma, \rho(\sigma))$, graphe dual, MST par Borůvka côté GPU.

### Ce que la tranche interdit ou met en garde

1. **Interdit de promouvoir un chemin approché en `exact`** : Remarque p. impr. 99–100 — dès qu'une approximation (Rips, kNN restreint, sous-échantillonnage) remplace la construction exacte, « les garanties d'équivalence avec les $K$-polyèdres exacts ne valent plus sans hypothèses supplémentaires ». La V4 par WSPD n'est acceptable en `exact` **que si** un théorème de complétude garantit que tout simplexe de Gabriel (ou au moins tout simplexe $K$-séparant) est retrouvé par la recherche d'arête maximale WSPD — c'est le théorème à écrire/prouver, il n'est pas dans le manuscrit.
2. **Le Théorème 7 propose la voie $\mathrm{Del}_{K-1}$, la V4 la refuse à raison** : le manuscrit dit « il suffit de construire $\mathrm{Del}_{K-1}(\mathcal{X})$ », mais cette voie matérialise la mosaïque d'ordre supérieur (interdite par l'invariant d'architecture du dépôt, coût prohibitif). Les Théorèmes 6–7 restent utiles à la V4 comme **caractérisations** (tout Gabriel est porté par deux facettes actives dont l'union redonne $\sigma$, chaque facette active portée par une arête élémentaire de $\mathrm{Del}_{K-1}$), donc comme oracles de contrôle sur petits $n$ — pas comme chemin de calcul.
3. **Vietoris–Rips n'est pas l'objet** : Fait 13 borne l'écart ($\alpha_3 = \sqrt{6}/2 \approx 1{,}2247$ en 3D) mais un $K$-polyèdre de Rips n'égale pas un $K$-polyèdre de Čech. Utilisable seulement en `mode` non-exact, jamais pour `public_status=exact`.
4. **Ne pas confondre partition des points et partition des facettes** : l'objet naturel pour $K \geq 2$ est un recouvrement des points / une partition des $(K-1)$-simplexes ; la partition stricte des points est un **post-traitement** (vote de la Prop. 7, avec règle de départage déterministe obligatoire — attention à la reproductibilité GPU des argmax en cas d'égalité).
5. **Coûts de référence à battre** : $O(n \log n)$ n'est prouvé que pour $K=2$ en 2D (Prop. 9) ; Delaunay 3D est $O(n^2)$ au pire. Le contrat V4 (K=10 < 100 ms sur G4, dizaines de millions de points) n'a aucun précédent dans le manuscrit : la tranche ne fournit ni ne contredit ces chiffres.
6. **Fixture birch2** : le comportement sur clusters filiformes (fusion intempestive avec $\widehat{\rho} = 1/r$, corrigé par $1/r^2$) est un phénomène documenté à transformer en test de non-régression du rendu, pas un bug du réducteur.

---

## Questions ouvertes / ambiguïtés

1. **Fait 12 non énoncé dans la tranche** : invoqué trois fois (pp. impr. 89, 99) comme l'argument-clef « points sur la frontière de $B_\sigma$ contenus dans un sous-ensemble strict du support $\Rightarrow \rho < r$ » et « facette active portée par $\mathrm{Del}_{K-1}$ ». Son énoncé exact est dans une tranche antérieure (probablement § 8.1–8.2, autour des pp. impr. 85–88) ; à récupérer, car c'est lui qui justifie mathématiquement les tests de support q2/q3/q4.
2. **Déf. 27 (simplexe $K$-séparant) et Théorème 4 hors tranche** (p. impr. 86 et suivantes) : la présente tranche n'en donne que la fin de preuve. La caractérisation exacte de « $K$-séparant » conditionne ce que la V4 a *le droit* d'éliminer.
3. **« Relier (deux de) ses trois arêtes-facettes » (Alg. 1, étape 4)** : le texte ne précise pas *lesquelles* deux ni si le choix importe. Pour la connectivité c'est indifférent (chemin ⇔ clique au même poids) ; mais pour un triangle obtus, seules les deux facettes actives ($S$ = paire diamétrale ⇒ facettes actives = les deux petites arêtes... en fait $\tau_s = \sigma \setminus \{s\}$ pour $s \in S$ : pour un obtus de support $\{x_2, x_3\}$, les facettes actives sont $[x_1,x_3]$ et $[x_1,x_2]$, les deux **petites** arêtes) préexistent ; la grande arête $[x_2,x_3]$ naît exactement à $\rho(\sigma)$. Le texte ne dit pas explicitement si l'arête diamétrale doit être un sommet de $G_2$ reliée au même niveau — la Prop. 6 implique que cela ne change pas les composantes, mais la convention retenue par l'implémentation de référence n'est pas fixée ici.
4. **Cas $|S| \geq 2$ avec facettes actives et non-actives** : la tranche établit que les facettes non-actives « peuvent être ajoutées comme sommets » sans changer les ensembles de points. La V4 doit choisir une convention (les inclure ou non dans $\mathcal{F}_K$) ; ce choix modifie $S_\tau$, $T_x$ et donc le vote du § 9.1. Le manuscrit définit $S_\tau$ sur « les $(K-1)$-simplexes effectivement construits par l'algorithme », ce qui est une définition *opérationnelle*, pas géométrique — ambiguïté assumée par le texte lui-même.
5. **Généralisation 3D de la Prop. 8 (q3/q4)** : la tranche ne donne la classification aigu/obtus que pour les triangles ($K=2$). La condition exacte pour qu'un tétraèdre ait un support d'arité 4 (l'analogue de « aigu ») n'est pas énoncée dans ces pages ; la feuille de route V4 (« q4 avec x et y ») devra la formaliser (circumcentre intérieur au tétraèdre côté de chaque face / conditions de Welzl) et la faire valider par oracle.
6. **$\psi(t) = 1/t^p$ : quel $p$ ?** Le texte utilise $p$ = dimension ambiante (cohérent avec « reflète la densité locale »), et le protocole SIPU utilise $\psi(t) = 1/t$ en 2D avec $\widehat{\rho} = 1/r$ — mais birch2 est « résolu » avec $\widehat{\rho} = 1/r^2$ (soit $\psi(t)=1/t^2$, le $1/t^p$ canonique en 2D). L'articulation exacte entre $\widehat{\rho}$ et $\psi$ (le texte les identifie implicitement) n'est pas formalisée dans la tranche.
7. **« Portage » vs appartenance à $\mathrm{Del}_K$** : Théorème 6 dit *porté par* (union de deux sommets adjacents), pas *simplexe de* $\mathrm{Del}_K$ au sens du complexe dual. La réciproque (tout simplexe porté est-il de Gabriel ?) n'est pas affirmée — le portage est une condition **nécessaire**, pas suffisante ; un filtre Gabriel (vacuité) reste requis après toute énumération de candidats.
8. **Numérotation des pages** : les en-têtes des pp. impr. 89, 91, 93, 103 contiennent des artefacts LaTeX visibles (« noms]Gabriel@Gabriel », « noms]Delaunay@Delaunay ») — défaut typographique du manuscrit, sans portée normative.
