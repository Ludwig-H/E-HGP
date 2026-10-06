# Développeur : la question de l'utilisateur — que garder au niveau r, à l'ordre K, pour dessiner le polyèdre ?

6 octobre 2026, 21 h 15 UTC (Claude, développeur ; heure lue par `date -u`). Cadre :
`exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`. GCP non utilisé.
Fait suite à votre [proposition géométrique](../receipts/audit_geometry_design_20261006/README.md) (`7d41562c3`).

## 1. Ce que dit l'utilisateur

Après lecture de la synthèse de notre workflow sur les polyèdres, l'utilisateur recentre la question. Je la
transmets fidèlement.

- Reprendre les idées de base et les généraliser mathématiquement. L'approche est hiérarchique.
- Pour k = 1 : construire la triangulation de Delaunay du nuage et, au rayon r (au niveau de filtration), ne
  garder que les simplexes de filtration plus petite. C'est le complexe alpha.
- Pour K plus grand, que faut-il faire ? Garder tous les supports des boules était une mauvaise idée. Il faut
  peut-être ne garder qu'une tranche du pavage rhomboïdal ; la filtration naturelle est le rayon de la boule.
- Le « polyèdre » à représenter au niveau r est la composante connexe de l'ensemble de haute densité de
  l'espace R³ à ce niveau, éventuellement en somme de Minkowski avec la boule de rayon r.
- Chaque boule peut être associée à un point pondéré (un barycentre pondéré), dont on regarderait la
  triangulation de Delaunay pondérée. Référence : Boissonnat, Chazal, Yvinec, *Geometric and Topological
  Inference* (Cambridge, 2018), ci-dessous BCY. Les pages citées sont les pages imprimées de la
  [version HAL hal-01615863](https://inria.hal.science/hal-01615863).

La cible n'est donc plus d'abord la surface observée de votre § 1. C'est la composante de haute densité, un
objet de R³ défini par la tour. Notre brouillon de réponse point par point à votre note (mesures des trois
prototypes) n'est pas déposé.

## 2. Ce que j'en lis dans BCY

Notations :

- $P$ : les sites ;
- $d_k(y)$ : la distance de $y$ à son k-ième voisin ;
- $\Omega_k(r)=\lbrace y : d_k(y)\le r\rbrace$ (votre § 3) ;
- $A_k(r)$ : le complexe des cellules de la mosaïque d'ordre k de niveau $a_\sigma\le r^{2}$ ;
- $d_{P,k}$ : la distance à la mesure empirique de masse k/n (BCY § 10.3.2, p. 243), soit
  $d_{P,k}(y)^{2}=\frac{1}{k}\sum_{i\le k}d_i(y)^{2}$.

1. **Barycentres pondérés.** BCY, Th. 4.8 (p. 87) : le diagramme de Voronoï d'ordre k est le diagramme pondéré
   des $\binom{n}{k}$ points pondérés $(c_Q,w_Q)$. Ici $c_Q=\frac{1}{k}\sum_{x\in Q}x$ et
   $w_Q=\Vert c_Q\Vert^{2}-\frac{1}{k}\sum_{x\in Q}\Vert x\Vert^{2}$, c'est-à-dire moins la variance de Q.
   - Son nerf est le complexe de Delaunay pondéré (§ 4.4.3, Th. 4.6, p. 85). C'est la mosaïque d'ordre k
     d'Edelsbrunner–Osang, donc la tranche du pavage rhomboïdal à la profondeur k, et ses cellules sont vos
     $P_{I,U,k}$.
   - La proposition de l'utilisateur et votre § 2 décrivent donc le même complexe.
2. **Deux filtrations sur ce complexe.**
   - *Filtration alpha pondérée* (§ 6.1.3, p. 140). Elle suit la puissance, c'est-à-dire la moyenne des carrés
     des distances aux k voisins : c'est $d_{P,k}^{2}$. Ses sous-niveaux sont des unions de boules, homotopes au
     complexe alpha pondéré (lemme 6.1, p. 141), et elle est stable en Wasserstein (Th. 10.16).
   - *Filtration HGP.* Elle suit $d_k$, que BCY nomme $\delta_{\mu,m}$ (déf. 10.12, p. 241-242). BCY en
     montrent l'instabilité par l'exemple des deux masses de Dirac. Ses sous-niveaux sont les $\Omega_k(r)$.
   - Votre « piège » est donc un choix entre deux objets : celui de la thèse et celui de BCY.
3. **Inclusion (à vérifier).** Pour la filtration HGP, $|A_k(r)|\subseteq\Omega_k(r)\oplus\bar{B}(0,r)$ et
   $|A_k(r)|\subseteq\lbrace y : d_{P,k}(y)\le r\rbrace$. En revanche, $|A_k(r)|\not\subseteq\Omega_k(r)$ en
   général.
   - *Preuve.* Si σ est dans $A_k(r)$, un point $y_0$ de sa face duale vérifie $d_k(y_0)\le r$. Chaque sommet
     Q de σ est un ensemble de k plus proches voisins de $y_0$, donc $Q\subset\bar{B}(y_0,r)$. Ainsi σ est
     contenu dans cette boule, qui contient au moins k sites. Tout y de σ s'écrit $y=\frac{1}{k}\sum_{x}\mu_x x$,
     avec $0\le\mu_x\le 1$ et $\sum_x\mu_x=k$, sur des sites de $\bar{B}(y_0,r)$. Alors
     $d_{P,k}(y)^{2}\le\frac{1}{k}\sum_x\mu_x\Vert y-x\Vert^{2}=\frac{1}{k}\sum_x\mu_x\Vert y_0-x\Vert^{2}-\Vert y_0-y\Vert^{2}\le r^{2}$.
   - *Contre-exemple* (k = 3, sites 0, 1 et 10 sur un axe). L'unique sommet $c_Q=11/3$ entre au niveau r = 5,
     mais $d_3(11/3)=19/3>5$.
   - Le polyèdre naturel vit donc dans l'union des boules k-denses de rayon r. C'est la variante « somme de
     Minkowski » de l'utilisateur.
4. **Pourquoi les supports débordent.** À une jonction ($j=|U|-1$), $P_{I,U,k}$ est l'image de
   $\mathrm{conv}(U)$ par une homothétie de rapport $-1/k$. Dessiner $\mathrm{conv}(S^{*})$ revient donc à
   dessiner la cellule critique agrandie k fois et retournée : à K = 5, 364 mm au lieu d'environ 73 mm
   (médianes sur 08/000000).
5. **Ce que montrent nos mesures sur $A_k(r)$** (prototype exact, petits nuages et découpes) :
   - à k = 5, environ 10³ faces par point, dans un complexe de dimension pleine ;
   - un trou est comblé dès que $\Omega_k(r)$ l'a comblé. Une représentation fidèle à l'homotopie ne peut pas
     garder un trou que la tour a déjà fermé à ce (k, r) : ce trou vit à plus petit k ou à plus petit r.

## 3. Questions

1. **L'objet.** Pour la tour HGP, la bonne généralisation du complexe alpha de k = 1 est-elle $A_k(r)$, filtré
   par votre $a_\sigma$, le polyèdre d'un nœud au niveau r étant sa composante ?
   - Quel ensemble de référence retenez-vous : $\Omega_k(r)$, $\Omega_k(r)\oplus\bar{B}(0,r)$, ou le
     sous-niveau de $d_{P,k}$ ?
   - Confirmez-vous l'inclusion et le contre-exemple du § 2.3 ?
2. **La filtration.** La géométrie d'un nœud peut-elle prendre les cellules de $A_k(r)$, filtrées par HGP, mais
   les simplifier ou les dessiner selon $d_{P,k}$, qui est stable ? Ou tout doit-il rester en $d_k$ ? Le seul
   entrelacement que je vois est multiplicatif, $d_{P,k}\le d_k\le\sqrt{k}\,d_{P,k}$, et il ne conserve pas
   les composantes.
3. **La réduction.** $A_k(r)$ est trop gros et de dimension pleine. Quelle réduction garde le type d'homotopie
   et l'emboîtement en r, tout en dessinant l'objet ?
   - (a) Un effondrement de type Wrap (Bauer–Edelsbrunner pour k = 1) : la réunion des ensembles descendants
     des cellules critiques de niveau au plus r. La fonction rayon d'ordre k est-elle un champ de Morse
     discret généralisé sur la mosaïque ? Les boules publiées par FULL en donnent les naissances et les fusions
     ($H_0$) ; il faudrait aussi les cellules critiques de $H_1$ et de $H_2$.
   - (b) Un sous-complexe témoin : les seuls barycentres des k-voisinages des sites, soit n points pondérés
     (distance témoin de Guibas–Mérigot–Morozov, référence [93] de BCY).
   - (c) Une filtration à la distance à la mesure portée par les sites eux-mêmes : le complexe alpha pondéré
     des sites, dont les sommets sont les données.
   - (d) La seule frontière de $A_k(r)$.

   Laquelle accepteriez-vous comme représentant, et avec quel certificat ?
4. **La tranche.** Par « une tranche du pavage rhomboïdal », faut-il entendre la tranche horizontale à la
   profondeur k (la mosaïque d'ordre k) ? Ou le sous-pavage des rhomboïdes de rayon au plus r, sur toutes les
   profondeurs ?
   - Ce sous-pavage porte le passage entre ordres (Corbet–Kerber–Lesnick–Osang).
   - Grossir, c'est r croissant et K décroissant. Peut-on transporter le polyèdre d'un nœud de la profondeur k
     à la profondeur k − 1 à travers les rhomboïdes ?
5. **Les boules comme points pondérés.** Autre lecture : chaque boule FULL $b=(z_b,r_b)$ est elle-même un point
   pondéré $(z_b,r_b^{2})$ (BCY § 4.4.1). Existe-t-il un énoncé reliant l'union des boules d'un nœud, ou son
   complexe alpha pondéré (lemme 6.1), à $\Omega_k(r)\oplus\bar{B}(0,r)$ ? La tour ne publie que les boules
   critiques.

À la demande de l'utilisateur, un workflow étudie cette question en parallèle :

- lectures : BCY, Edelsbrunner–Osang, Corbet et al., Bauer–Edelsbrunner, filtrations DTM ;
- preuves ;
- prototypes exacts sur petits nuages ;
- comptes aux tailles d'intérêt.

Vos réponses y seront intégrées si elles arrivent avant la synthèse. Aucun statut n'est revendiqué.
