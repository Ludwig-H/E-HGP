# Représentants petits alternatifs des niveaux d'un nœud HGP, avec garanties

6 octobre 2026, 22 h 14 – 22 h 50 UTC (heures lues par `date -u`). Rôle : théoricien du workflow « généraliser le
complexe alpha à l'ordre K ». Cadre :

```text
phase=exploration_v11_hors_registre
backend=cpu_reference
profile=quantized_u21_input_only
public_status=not_claimed
```

GCP non utilisé. Aucun commit. Statuts employés : **prouvé** (preuve complète ici ou chez l'auditeur, citée),
**vérifié borné** (calcul exact sur fixture), **mesuré** (flottant, indicatif), **conjecture**, **réfuté**
(contre-exemple exact). Rien ici ne qualifie un constructeur ni un statut public.

## 0. En bref

- L'objet est fixé par l'auditeur (`d2be6bdc7`, `28d70f8ab`) : $A_k(r)$ et ses nœuds. Je ne le redémontre pas. Je
  cherche à côté des effondrements des représentants **petits**, et je vérifie chacun contre la filtration exacte.
- **Ombre** $S_v(r)$. Elle a une définition intrinsèque : la réunion, sur $y\in C_v(r)$, des enveloppes convexes des
  boules des k plus proches voisins (O1). À K = 1 elle **est** le complexe alpha (O2). Elle s'emboîte en r le long
  de l'arbre (O3), mais **pas entre ordres** (O4, réfuté par une fixture à quatre points). Elle n'est **pas**
  homotope au nœud, même pour un nœud isolé (O5, deux fixtures exactes, dont l'exemple du § 6.1 de la thèse). Elle
  garde les trous « libres » (O6, critère de dualité d'Alexander), et elle perd les trous bouchés par
  moins de k sites (un moyeu).
- **Complexe de Delaunay des sites couverts** $D_v(r)=\alpha_1(P,r)[P_v(r)]$. C'est le meilleur rapport garanties/coût
  trouvé (§ 4).
  - Il est exact à K = 1 et garde la trace exacte (le K-polyèdre de la thèse).
  - Il forme une vraie bifiltration d'**un seul** complexe $\mathrm{Del}_1(P)$, emboîtée en r, en k et le long de
    l'arbre.
  - Sandwich $C_v(r)\subseteq R_v(r)\subseteq C_{v\uparrow 3r}(3r)$ ; le facteur 3, atteint, ne dépend pas de k.
  - $d_H\le 2r$, et la même garde des trous libres que l'ombre.
  - Taille mesurée : ≈ 28 faces par point, soit 6,45 tétraèdres, à n = 8 000, 16 000 et 32 000 sur trois trames
    sans sol. C'est environ 35 fois moins que la mosaïque d'ordre 5, et aucune mosaïque d'ordre supérieur n'est
    nécessaire.
  - Ce n'est pas un représentant homotopique : l'entrelacement (×3) le remplace. C'est une restriction, à la trace
    HGP, du « Delaunay core » de Blaser–Brun–Gardaa–Salbu (2024).
- **Offset** : $\Omega_k(r)\oplus\bar{B}_r=\{g_k\le r\}$, où $g_k(z)$ est le rayon de la plus petite boule contenant z et
  k sites. Il est 2-entrelacé, avec un facteur atteint (F1). C'est un rendu d'enveloppe, pas un représentant petit.
- **Bifiltration creuse d'Alonso**. Elle a la meilleure constante, $(1+3\varepsilon)$ et $(1+2\varepsilon)/(1+\varepsilon)$, et sa taille est
  O(n). Mais sa constante simpliciale est inutilisable en dimension 3 : chaînes de parties d'un ensemble de
  ≥ 2,9·10⁴ points dans la borne de pire cas (A2). Son modèle spatial à r fixé est petit aux échelles grossières, et
  admet un dual polyédrique exact (A4, proposition). L'identité des nœuds n'est garantie qu'en dehors des fenêtres
  d'événements $(r,cr]$ (A1).
- **Classement (§ 9)** :
  1. le complexe exact réduit à sommets protégés (référence de l'auditeur) ;
  2. $D_v$, comme représentant petit vérifiable par entrelacement, et l'ombre comme rendu de couverture ;
  3. Alonso, comme piste pour les niveaux grossiers ;
  4. l'offset et les approximations DTM, comme attributs ou enveloppes.

## 1. Cadre, notations, acquis cités

$P\subset\mathbb{R}^3$ fini, sites distincts de poids unitaire, $1\le k\le n$, boules fermées. $d_k(y)$ est la distance
au k-ième site ; $\Omega_k(r)=\{d_k\le r\}$ ; $C_v(r)$ est la composante du nœud v vivant à r ; $v\uparrow s$ est
l'ancêtre de v au rayon $s\ge r$ (application $\pi_0$ de FULL).

- $N_k(y)=P\cap\bar{B}(y,d_k(y))$ : sites de la boule des k plus proches voisins, égalités comprises.
- $P_v(r)=P\cap(C_v(r)\oplus\bar{B}_r)$ : sites couverts (Déf. 8, Th. 2 de la thèse).
- $g_k(z)=\min_{\lvert Q\rvert=k}\mathrm{meb}(Q\cup\{z\})$, où $\mathrm{meb}$ est le rayon de la plus petite boule englobante.
  Pour un site p, $g_k(p)\le r$ si et seulement si p est couvert à l'ordre k au rayon r.
- $A_k(r)$, $A_{k,v}(r)$, cellule σ, face duale $F_\sigma$, date $a_\sigma$, réunion des labels
  $U_\sigma=\bigcup_{Q\ \mathrm{sommet\ de}\ \sigma}Q$ : comme chez l'auditeur.
- $\alpha_1(P,r)$ : complexe alpha ordinaire, cellules de la subdivision de Delaunay, sans position générale.

**Acquis cités, non refaits** (auditeur, `d2be6bdc7` § 1–6 et `28d70f8ab` § 1–6) :

- $\lvert A_k(r)\rvert\simeq\Omega_k(r)$, sans position générale ;
- $\pi_0$ coïncide avec FULL ;
- couverture exacte par les labels ;
- $\lvert A_{k,v}\rvert\subseteq C_v\oplus\bar{B}_r$, $C_v\subseteq\lvert A_{k,v}\rvert\oplus\bar{B}_r$ ;
- la DTM est un attribut ;
- l'ombre : trace exacte, $S_v\subseteq C_v\oplus\bar{B}_r$, $d_H\le r$, et la fixture {0, 2, 4} entre deux nœuds ;
- stabilité par entrelacement et décalage d'ordre ;
- sommets protégés, et le contre-exemple du triangle $\{(-4,0),(4,0),(1,2)\}$, sans stabilité de Hausdorff à r fixé.

Toutes les fixtures planes valent dans $\mathbb{R}^3$. Pour des sites coplanaires, la subdivision de Delaunay, les
faces de Voronoï (prismes) et les dates sont celles du plan : le minimum de la distance sur une face prismatique est
atteint dans le plan.

## 2. Deux lemmes élémentaires

**L1 (enveloppe dans les boules).** Soit $S\subseteq\bar{B}(y,r)$ fini et non vide. Alors
$\mathrm{conv}(S\cup\{y\})\subseteq\bigcup_{s\in S}\bar{B}(s,r)$.

*Preuve.* Soit $x\ne y$ dans l'enveloppe, à distance > r de tous les $s$. Pour tout $s\in S$, on a
$\lVert s-y\rVert^2=\lVert s-x\rVert^2+\lVert x-y\rVert^2-2(s-x)\cdot(y-x)$. Le membre de gauche est $\le r^2$ et
$\lVert s-x\rVert^2>r^2$, donc $(s-x)\cdot(y-x)>0$. Comme $(y-x)\cdot(y-x)>0$, tout $S\cup\{y\}$ est dans le demi-espace
ouvert $\{u:(u-x)\cdot(y-x)>0\}$, qui est convexe et ne contient pas x : contradiction. Le cas x = y est immédiat,
puisque $\lVert y-s\rVert\le r$. ∎ (prouvé)

**L2 (lentille).** Pour $\lvert Q\rvert\ge1$ de rayon englobant $\rho_Q$ et $r\ge\rho_Q$,
$\mathrm{diam}\bigcap_{q\in Q}\bar{B}(q,r)\le2\sqrt{r^2-\rho_Q^2}$.

*Preuve.* Si $y_1,y_2$ sont dans l'intersection, alors Q est dans $\bar{B}(y_1,r)\cap\bar{B}(y_2,r)$. Ce dernier
ensemble est contenu dans la boule de centre le milieu et de rayon $\sqrt{r^2-\lVert y_1-y_2\rVert^2/4}$. ∎ (prouvé)

Conséquence de L2 : les témoins $y_Q=\arg\min_{V_Q}d_k$ forment un ε-net de $C_v(r)$ avec
$\varepsilon_v(r)=\max_{Q\in v}2\sqrt{r^2-\rho_Q^2}$. C'est fin quand r est proche des $\rho_Q$, et grossier (≤ 2r) ensuite.

## 3. L'ombre

$S_v(r)=\bigcup_{\sigma\in A_{k,v}(r)}\mathrm{conv}(U_\sigma)$ (définition de l'auditeur, avec les Q réels).

**O1 (définition intrinsèque ; prouvé).** $S_v(r)=\bigcup_{y\in C_v(r)}\mathrm{conv}(N_k(y))$.

*Preuve.* Pour $y\in\mathbb{R}^3$, soit $\sigma(y)$ la cellule duale à la plus petite face de Voronoï d'ordre k
contenant y. Ses sommets sont les domaines pleins $V_Q\ni y$. Montrons que $U_{\sigma(y)}=N_k(y)$.

- (⊆) Chaque tel Q est formé de k plus proches voisins de y.
- (⊇) Notons I les sites strictement plus proches que $d_k(y)$ et U ceux à distance $d_k(y)$, avec
  $\lvert I\rvert<k\le\lvert I\cup U\rvert$. Soit $u\in U$. On avance de y vers u. L'identité de l'auditeur,
  $\lVert w-q\rVert^2-\lVert w-u\rVert^2=s\lVert u-q\rVert^2$ pour $w=y+s(u-y)$ et q ex æquo, rend u strictement plus
  proche que les autres sites de U. Une perturbation générique choisit alors un domaine plein contenant $I\cup\{u\}$.
  Par fermeture, y lui appartient : $u\in U_{\sigma(y)}$. I est contenu dans tout label.

Ensuite :

- Si $y\in C_v(r)$, la cellule $\sigma(y)$ est active, car $a_{\sigma(y)}\le d_k(y)^2\le r^2$. Elle est attribuée à v :
  sur $F_\sigma$, $d_k$ est un maximum de distances, donc $F_\sigma\cap\Omega_k(r)$ est convexe et tient dans une
  seule composante.
- Réciproquement, le témoin $y_\sigma\in F_\sigma\cap C_v(r)$ d'une cellule active de v est dans l'intérieur relatif
  d'une face $F_\tau\subseteq F_\sigma$. Alors $U_\sigma\subseteq U_\tau=N_k(y_\sigma)$.

∎

**O2 (K = 1 ; prouvé, sans position générale).** $S_v(r)=\lvert\alpha_{1,v}(r)\rvert=\lvert A_{1,v}(r)\rvert$ :
l'ombre d'ordre 1 est exactement le complexe alpha.

*Preuve.* $N_1(y)$ est l'ensemble des sites de la cellule de Delaunay duale à la face de Voronoï contenant y. Ces
sites sont cosphériques, donc en position convexe, et leur enveloppe est cette cellule. O1 donne alors la réunion des
cellules actives de la composante. ∎

**O3 (emboîtements et inclusions ; prouvé).**

- $S_v(r)\subseteq S_{v\uparrow r'}(r')$ pour $r\le r'$ : les cellules actives croissent et leur attribution suit $\pi_0$.
- $\lvert A_{k,v}(r)\rvert\subseteq S_v(r)\subseteq\bigcup_{p\in P_v(r)}\bar{B}(p,r)\subseteq\Omega_1(r)$ : la seconde
  inclusion vient de L1 appliqué à $N_k(y)\subseteq\bar{B}(y,r)$.
- Trace exacte $P\cap S_v(r)=P_v(r)$ (auditeur).
- **En dimension 1**, $S_v(r)=\mathrm{conv}\,P_v(r)$, car une réunion connexe d'intervalles à extrémités couvertes est un
  intervalle. On en déduit $S^{(k)}_v(r)\subseteq S^{(k-1)}_{v'}(r)$, où v' est le nœud vertical.

**O4 (pas d'emboîtement entre ordres en dimension ≥ 2 ; réfuté par fixture).** Prendre $P=\{(-4,0),(4,0),(0,4),(0,1)\}$,
$r^2=16$, $z=(0,1/2)$ ; chaque ordre n'a qu'un nœud (vérifié).

- Au témoin y = (0, 0), les distances carrées valent 16, 16, 16 et 1. Donc $d_2(y)^2=16$ et $N_2(y)=P$, d'où
  $z\in\mathrm{conv}P\subseteq S^{(2)}(4)$.
- Le triangle $((0,1),(-4,0),(4,0))$ est de Delaunay : son cercle circonscrit, de centre $(0,-15/2)$ et de rayon carré
  289/4, laisse (0, 4) dehors. Il contient z en son intérieur, et sa date $289/4>16$.
- Donc, par O2, $z\notin\lvert\alpha_1(P,4)\rvert=S^{(1)}(4)$.

Conséquence : un rendu d'ombre par ordre n'est pas une famille emboîtée verticalement. Le lien entre ordres reste
celui de l'auditeur : un représentant par k, plus les liens $\pi_0$.

**O5 (pas d'homotopie, même pour un seul nœud ; réfuté par fixtures).**

- **Moyeu carré** : $P=\{(0,0),(\pm2,0),(0,\pm2)\}$, k = 2, $r^2=3$.
  - Le nerf $N^W_2$ donne $(\beta_0,\beta_1)=(1,1)$ : un nœud avec un trou autour du moyeu, car $d_2(0)^2=4>3$.
  - Les témoins $y=(\pm1,\pm1)$ ont $N_2(y)=$ {moyeu, deux rayons} et $d_2(y)^2=2\le3$.
  - L'ombre est donc le losange $\mathrm{conv}P$, qui est contractile.
- **Six points du § 6.1 de la thèse**, à r = 39/20 (§ 8).
  - Le lacet de $C_v$ autour de C est essentiel.
  - L'ombre contient $ABC\cup ACD\cup BCD$, réunion étoilée depuis C qui contient ce lacet : il y est contractile.

La cause est la même dans les deux cas : un site isolé, moins de k sites, se trouve dans le trou. Les labels qui le
portent remplissent le trou dans l'ombre, alors que la filtration d'ordre k le garde. C'est précisément la robustesse
aux aberrants de la multicouverture.

**O6 (garde des trous libres ; prouvé).** Soit $Z\subset\mathbb{R}^3$ compact avec $Z\cap\Omega_1(r)=\varnothing$, donc à
distance > r de tous les sites. Tout cycle de $C_v(r)$ non nul dans $H_*(\mathbb{R}^3\setminus Z)$ a une image non nulle
dans $H_*(S_v(r))$ par $C_v\xrightarrow{\varphi}\lvert A_{k,v}\rvert\subseteq S_v$ (φ : équivalence du nerf).

*Preuve.*

1. Prendre ε > 0 assez petit pour trois choses : les ε-voisinages des $C_Q(r)$ ont le même nerf ; les $C_Q(r)$ qui
   approchent $C_v$ à moins de ε y sont contenus ; et $Z\cap\Omega_1(r+\varepsilon)=\varnothing$.
2. Une partition de l'unité subordonnée à ces voisinages donne l'équivalence du nerf
   $\varphi(y)=\sum_Q\lambda_Q(y)c_Q\in\lvert A_{k,v}(r)\rvert$. Les Q portés ont une face commune active (argument des
   cellules maximales de l'auditeur), et $Q\subseteq\bar{B}(y,r+\varepsilon)$.
3. Le segment $[y,\varphi(y)]$ est dans $\mathrm{conv}(\{y\}\cup\bigcup Q)\subseteq\Omega_1(r+\varepsilon)$ par L1. Donc
   l'inclusion $C_v\hookrightarrow\mathbb{R}^3\setminus Z$ est homotope à $\varphi$ suivi de
   $\lvert A_{k,v}\rvert\subseteq S_v\subseteq\Omega_1(r)\subseteq\mathbb{R}^3\setminus Z$.
4. Le cycle reste donc non nul dans $S_v$. ∎

Pour une roue LiDAR, le trou de la jante est gardé par l'ombre dès qu'une courbe qui enlace la jante traverse le
disque intérieur à plus de r de tout site (rayons, moyeu, autres objets). Il peut être perdu sinon, comme dans O5.

**Ce qui manque à l'ombre pour être un plongement.** Un sommet devient un ensemble, $c_Q\mapsto\mathrm{conv}(Q)$.

- Les enveloppes de cellules disjointes peuvent se chevaucher, entre nœuds ({0, 2, 4}) comme dans un même nœud (O5).
- Le plongement associé existe : c'est la mosaïque barycentrique $c_Q\in\mathrm{conv}(Q)$, section de l'ombre
  ($\lvert A_v\rvert\subseteq S_v$).
- L'ombre est la réalisation par enveloppes du **complexe témoin d'ordre k**, dont les simplexes sont les $N_k(y)$,
  $y\in C_v(r)$. L'ombre géométrique a le type du nerf des convexes $\mathrm{conv}(U_\sigma)$, σ maximales ; elle est
  donc homotope à $A_v$ **dès que** ce nerf égale celui des cellules maximales. Ce critère suffisant se vérifie par
  tests d'intersection d'enveloppes (Helly : familles d'au plus 4 en dimension 3), mais il est coûteux.
- Taille : autant de pièces que de cellules maximales. C'est un rendu de la mosaïque, pas une réduction.
- Stabilité géométrique : aucune à r fixé. À K = 1 l'ombre est le complexe alpha, et le triangle de l'auditeur
  s'applique tel quel.

## 4. Le complexe de Delaunay des sites couverts

Définitions :

- $D_k(r)=\alpha_1(P,r)[P_k(r)]$, avec $P_k(r)=\{p:g_k(p)\le r\}$, sous-complexe induit ;
- par nœud, $D_v(r)=\alpha_1(P,r)[P_v(r)]$ ;
- pièces $R_v(r)=\bigcup_{p\in P_v(r)}(V(p)\cap\bar{B}(p,r))$, avec V(p) la cellule de Voronoï d'ordre 1 dans P.

Chaque simplexe τ de $\mathrm{Del}_1(P)$ entre, à l'ordre k, au rayon
$\rho_k(\tau)=\max(\alpha(\tau),\max_{p\in\tau}g_k(p))$, croissant en k. La seule donnée d'ordre k est la date de couverture
$g_k(p)$ de chaque site, que FULL fournit par ses populations.

**D1 (K = 1 ; prouvé).** $g_1\equiv0$, donc $D_1(r)=\alpha_1(P,r)$.

**D2 (nerf ; prouvé).** $\lvert D_v(r)\rvert\simeq R_v(r)$. Le complexe induit est le nerf des pièces convexes
$V(p)\cap\bar{B}(p,r)$, $p\in P_v(r)$ (Edelsbrunner 1995), et le lemme du nerf s'applique au sous-recouvrement. En
dégénéré : nerf abstrait, ou réalisation par la subdivision de Delaunay avec l'argument des cellules maximales de
l'auditeur.

**D3 (sandwich ; prouvé, facteur atteint).** $C_v(r)\subseteq R_v(r)\subseteq C_{v\uparrow3r}(3r)$, et $R_v(r)$ est connexe.
Globalement, $\Omega_k(r)\subseteq R_k(r)\subseteq\Omega_k(3r)$.

*Preuve.*

- Soit $y\in C_v$ et p son site le plus proche : $\lVert y-p\rVert\le d_1(y)\le r$, donc p est couvert par v et y est
  dans sa pièce.
- Soit x dans la pièce de p couvert : une boule de rayon r contient p et k sites, donc x a k sites à moins de 3r.
- Connexité : si p est couvert par $y\in C_v$, un point x de $[p,y]$ a pour plus proche site un q tel que
  $\lVert q-y\rVert\le\lVert x-p\rVert+\lVert x-y\rVert=\lVert p-y\rVert\le r$. Donc q est couvert et $[p,y]\subseteq R_v$.
- La composante de $\Omega_k(3r)$ qui contient le connexe $R_v\supseteq C_v$ est celle de $v\uparrow3r$.

Facteur 3 atteint : $P=\{0,2\}$, k = 2, r = 1 donne $R=[-1,3]=\Omega_2(3)$ (fixture F4, $d_2(-1)=3$). ∎

Conséquence : l'application $H_*(C_v(r))\to H_*(C_{v\uparrow3r}(3r))$ se factorise par $H_*(D_v(r))$. Tout trait de v qui
persiste sur $[r,3r]$ est donc vu par $D_v(r)$. Globalement, la distance d'entrelacement multiplicative est ≤ 3, et les
barres de rapport > 9 sont appariées.

**D4 (structure ; prouvé).**

- $D_k(r)\subseteq D_k(r')$, $D_k(r)\subseteq D_{k-1}(r)$ et $D_v(r)\subseteq D_{v\uparrow r'}(r')$ : c'est une
  **bifiltration de sous-complexes d'un seul complexe**, $\mathrm{Del}_1(P)$, contrairement à l'ombre (O4).
- Trace exacte : $P\cap\lvert D_v(r)\rvert=P_v(r)$, car une cellule de Delaunay ne rencontre P qu'en ses sommets
  cosphériques.
- Lien avec la littérature. Le « Delaunay core » de Blaser–Brun–Gardaa–Salbu (arXiv 2405.01214v4, th. 4.5 ; β = 1/2)
  donne $\mathrm{Cov}_{r,k}\subseteq\mathrm{DelCore}_{r,k}\subseteq\mathrm{Cov}_{3r,k}$, avec les sommets
  $\{d_k(p)\le2r\}$. Comme $g_k\le d_k\le2g_k$, on a $D_k(r)\subseteq\mathrm{DelCore}^{1/2}_{r,k}$ : D est la
  restriction à la trace HGP exacte, avec les mêmes constantes. La stabilité de Prohorov de leur § 5 se transfère par
  l'entrelacement.

**D5 (géométrie ; prouvé).** $d_H(\lvert D_v(r)\rvert,C_v(r))\le2r$.

- Un simplexe actif τ est dans une boule de rayon r, donc tout point de τ est à ≤ r d'un sommet (L1), lui-même à ≤ r
  de $C_v$.
- Dans l'autre sens, le site le plus proche d'un point de $C_v$ est un sommet couvert.

Tous les sommets sont des sites : proximité aux données nulle, alors que $A_k$ place ses sommets en barycentres,
déplacés vers l'intérieur des surfaces courbes.

**D6 (stabilité ; prouvé).**

- Sous un appariement de déplacement ≤ δ : $\lvert g_k-g'_k\rvert\le\delta$ (même centre, rayon + δ), et
  $R^P_k(r)\subseteq\Omega^P_k(3r)\subseteq\Omega^{P'}_k(3r+\delta)\subseteq R^{P'}_k(3r+\delta)$ : entrelacement
  (×3, +δ).
- Pour m ajouts ou retraits, on compose avec les décalages d'ordre de l'auditeur.
- La variante sans restriction de Voronoï, $R'_k(r)=\bigcup_{g_k(p)\le r}\bar{B}(p,r)$, est le sous-niveau de
  $h(x)=\min_p\max(\lVert x-p\rVert,g_k(p))$. Elle est 1-lipschitzienne et δ-stable additivement (filtration de type
  DTM « p = ∞ » d'Anai et al.), mais exige le Delaunay du sous-ensemble couvert à chaque niveau.

**D7 (taille ; mesuré, flottant).** Qhull (`Qt Qbb Qc Qz`), sur les trois trames sans sol `trame_08_000000/100/200`,
avec des recadrages de 8 000, 16 000 et 32 000 sites uniques : **6,44–6,53 tétraèdres et 27,8–28,1 faces (toutes
dimensions) par point**, stable avec n ([mesure_del1.json](mesure_del1.json)). C'est indépendant de k : une date par
simplexe et par k. À comparer aux ~10³ faces par point de la mosaïque d'ordre 5 (workflow précédent). Le pire cas en
dimension 3 est $O(n^2)$, et rien n'est certifié (prédicats flottants, triangulation des cellules dégénérées).

**D8 (pas d'homotopie ; réfuté par fixtures).** Mêmes trous bouchés qu'en O5. Moyeu carré : quatre triangles de
Delaunay de rayon carré 2 ≤ 3, donc losange plein. Roue à moyeu : huit triangles de rayons carrés 25/9 et 25/8 ≤ 6.
Six points : $D_2$ est contractile, alors que $\beta_1=2$. La garde des trous libres de O6 vaut aussi, avec
$C_v\subseteq R_v\subseteq\Omega_1(r)$.

**D9 (ombre et D incomparables ; réfuté par fixtures F3 et F6).** $S_v\not\subseteq D_v$ : dans F3, $z\in S^{(2)}(4)$
mais $z\notin\lvert\alpha_1(P,4)\rvert\supseteq\lvert D_2(4)\rvert$. $D_v\not\subseteq S_v$ : fixture F6, k = 3, r = 1.

- Sites : $p=(0,0)$, $q=(9/5,0)$, $(-1/2,\pm3/10)$, $(23/10,\pm3/10)$, et un arc de neuf sites $(-1/2,4/5)$,
  $(-2/5,13/10)$, $(-1/10,7/4)$, $(2/5,2)$, $(9/10,21/10)$, $(7/5,2)$, $(19/10,7/4)$, $(11/5,13/10)$, $(23/10,4/5)$.
- Les triplets $\{p,(-1/2,\pm3/10)\}$ et $\{q,(23/10,\pm3/10)\}$ sont dans la même composante de $N^W_3(1)$ : p et q
  sont couverts par le même nœud v.
- L'arête pq est de Gabriel, de demi-longueur 9/10 ≤ 1 : elle est dans $D_v(1)$.
- Aucune partie S de sites avec $\lvert S\rvert\ge3$, $\mathrm{meb}(S)\le1$ et $m=(9/10,0)\in\mathrm{conv}(S)$ n'existe
  (énumération exacte des parties des dix sites à distance ≤ 2 de m). Comme $N_3(y)$ est une telle partie dès que
  $d_3(y)\le1$, on obtient $m\notin S_v(1)$ par O1.

Les deux rendus portés par les données dessinent donc des objets différents, sans relation d'inclusion.

## 5. L'offset

**F1 (caractérisation ; prouvé, facteur atteint).** $\Omega_k(r)\oplus\bar{B}_r=\{z:g_k(z)\le r\}$. En effet, z est dans
l'offset si et seulement s'il existe y et Q avec $Q\cup\{z\}\subseteq\bar{B}(y,r)$. De plus $g_k\le d_k\le2g_k$, donc
$\Omega_k(r)\subseteq\Omega_k(r)\oplus\bar{B}_r\subseteq\Omega_k(2r)$. Le facteur 2 est atteint en z = 0 pour
$P=\{0,2\}$, k = 2. Par nœud, $C_v(r)\oplus\bar{B}_r\subseteq C_{v\uparrow2r}(2r)$. À K = 1 : $\Omega_1(r)\oplus\bar{B}_r=\Omega_1(2r)$.

**F2 (représentation).** Les pièces $C_Q(r)\oplus\bar{B}_r$ de l'auditeur sont convexes mais bordées de sphères, et
modélisent l'offset, pas $C_v$ : un trou de rayon intérieur ≤ r est rempli. L'ε-net de L2 s'en déduit, avec un ε
qui tend vers 2r. L'offset est une enveloppe de rendu, 2-entrelacée. Il ne réduit rien, car il exige les $C_Q$.

## 6. La bifiltration creuse d'Alonso (SoCG 2025)

Téléchargée : [alonso_socg2025.pdf](telechargements/alonso/alonso_socg2025.pdf), sha256 `4266e831…`. Th. 8 :
$\mathrm{Cov}_{r,k}\subseteq\mathrm{SCov}_{(1+3\varepsilon)r,k}$ et $\mathrm{SCov}_{r,k}\subseteq\mathrm{Cov}_{\delta r,k}$, avec
$\delta=(1+2\varepsilon)/(1+\varepsilon)$. Th. 14 : SSub ≃ SCov. Th. 15 : O(n) simplexes. Th. 18 : O(1) grades critiques.

**A1 (identité des nœuds à rayons décalés ; prouvé).** Posons $c=(1+3\varepsilon)(1+2\varepsilon)/(1+\varepsilon)$.

- La composante $X_v$ de $\mathrm{SCov}_{(1+3\varepsilon)r,k}$ qui contient $C_v(r)$ est bien définie, et
  $C_v(r)\subseteq X_v\subseteq C_{v\uparrow cr}(cr)$.
- $X_v$ ne contient aucun $C_w(r)$ d'un nœud w qui ne fusionne avec v qu'après cr ; il peut contenir ceux qui
  fusionnent dans la fenêtre $(r,cr]$.
- L'identité est donc exacte, au sens $X_v\cap\Omega_k(r)=C_v(r)$, **dès que** la lignée de v ne subit aucune fusion
  dans $(r,cr]$ (condition suffisante). C'est la « coupe
  intérieure à marge » de l'auditeur, avec la marge multiplicative c (≈ 3,33 à ε = 1/2, ≈ 1,42 à ε = 0,1).

**A2 (constantes en dimension 3 ; prouvé à partir de l'article).** Le lemme 17 borne les « amis » d'un point par
empilement. Leurs points sont $\varepsilon\alpha/((1+\varepsilon)(1+3\varepsilon))$-séparés dans une boule de rayon 2α, donc
$\lvert A\rvert\le(1+4(1+\varepsilon)(1+3\varepsilon)/\varepsilon)^3$. Le minimum sur ε est atteint en $\varepsilon=1/\sqrt{3}$ et vaut
$(1+16+8\sqrt{3})^3\approx2{,}9\cdot10^4$. Les simplexes de SSub sont des chaînes de parties de A : le « O(1) » du
th. 15 n'est pas une taille utilisable en dimension 3. Le modèle simplicial est écarté pour LiDAR (non mesuré à
l'échelle).

**A3 (modèle spatial et échelle ; prouvé + estimation).** À r fixé, SCov est la k-couverture pondérée d'au plus
$\lvert C_r(X)\rvert$ boules, de rayon $\rho_x(r)\le r$, de poids $c_r(x)$, et à centres
$\varepsilon r/((1+\varepsilon)(1+3\varepsilon))$-séparés.

- La sparsification n'agit que si $r\gtrsim$ espacement × $(1+\varepsilon)(1+3\varepsilon)/\varepsilon$, soit ≥ 7,46 fois
  l'espacement à l'optimum : aux niveaux des objets (vélo, piéton), pas aux naissances.
- À K = 1, SCov est l'union creuse de Cavanna–Jahanseir–Sheehy, et non le complexe alpha. Elle tend vers Cov quand
  ε → 0.

**A4 (dual polyédrique exact de SCov à r fixé ; proposition prouvée, non publiée).**

- Notons $\pi_x(y)=\lVert y-x\rVert^2-\rho_x(r)^2$, et répétons x selon son poids $c_r(x)$.
- La « k-ième puissance pondérée » vaut $\max_{x\in M}\pi_x$ sur chaque cellule de puissance d'ordre k du multi-ensemble,
  qui est convexe. Elle est convexe sur la cellule, et $\mathrm{SCov}_{r,k}$ en est le sous-niveau 0.
- La construction de l'auditeur (nerf des pièces convexes, cellules maximales) s'applique mot pour mot.
- Aux échelles grossières, $c_r(x)\ge k$ pour la plupart des centres. Les multi-ensembles M utiles se réduisent alors à
  $\{x^k\}$ et à des paires, et le dual a une taille O(|net|), comparable à un alpha pondéré d'ordre 1 ou 2 du net.

Ce n'est pas une filtration d'un seul complexe : net, poids et rayons changent avec r. C'est un représentant par
niveau.

## 7. Autres constructions de la littérature

- **DTM** (BCY ch. 10 ; Guibas–Mérigot–Morozov ; Buchet et al.). Le sous-niveau exact a le même dual que $A_k$ ; ce
  n'est pas plus petit. En composant les bornes citées par l'auditeur ($f_k\le f_{\mathrm{t}}\le\sqrt{6}f_k$ ;
  $f_k/\sqrt{2}\le g\le\sqrt{3}f_k$) avec $\Omega_k(r)\subseteq\{f_k\le r\}\subseteq\Omega_k(\sqrt{k}r)$, les versions
  de taille O(n) (barycentres témoins, sites pondérés) sont entrelacées avec $\Omega_k$ d'un facteur produit
  $\sqrt{6k}$ : ≈ 5,5 à K = 5 et ≈ 7,7 à K = 10. C'est pire que D (3, indépendant de k). Elles restent des attributs.
- **Cœur** (Blaser et al.). Pour toute valeur de β, le produit des deux facteurs vaut au moins 3 (atteint à β = 1/2,
  leur remarque après le th. 3.7). D atteint cette borne avec la trace HGP.
- **Subdivision de Čech** (Sheehy) et **S-Del** (Corbet et al.) : modèles exacts de la bifiltration, de taille
  $O(n^{d+1})$ ou plus. Ce sont des références, pas de petits représentants.
- **Sous-complexes témoins restreints aux labels.**
  - (a) Le complexe témoin d'ordre k, engendré par les $N_k(y)$, est l'ombre abstraite de O1–O6.
  - (b) Garder, dans $A_v(r)$, un sommet par site couvert et un arbre qui les relie : ce sont les supports SPv2 et le
    K-MST de la thèse (Déf. 27–30, Th. 4–6).
  - (b) garantit $\pi_0$ et la couverture datée (R2), mais aucun $H_1$/$H_2$ ni aucune forme. C'est l'analogue du MST,
    et non du complexe alpha, comme l'impose l'analogie de la thèse (§ 3 de la consigne).

## 8. Exemples exacts

Script : [verif_alternatives.py](verif_alternatives.py), 25 portes vertes sous `python -O`
([verif_sortie.json](verif_sortie.json)). La vérité HGP est donnée par $\beta_0$ et $\beta_1$ du 2-squelette du nerf
$N^W_k(r)$ (homotope à $\Omega_k(r)$ selon l'auditeur, § 0), en arithmétique exacte : Fraction, et le corps
$\mathbb{Q}(\sqrt{3})$ pour les six points.

**Six points du § 6.1** : $A=(-\sqrt{3},1)$, $B=(-\sqrt{3},-1)$, $C=(0,0)$, $D=(2,0)$, $E=(2+\sqrt{3},1)$,
$F=(2+\sqrt{3},-1)$, côté 2.

- Ordre 2 (vérifié borné aux rayons rationnels 1, 11/10, 6/5, 39/20 et 21/10 ; dates exactes) :
  - 7 nœuds sur $[1,2/\sqrt{3})$ (les sept segments) ;
  - 3 nœuds sur $[2/\sqrt{3},\sqrt{2+\sqrt{3}})$, avec $\mathrm{meb}(ACD)^2=2+\sqrt{3}=(AD/2)^2$ ;
  - à r = 39/20, un nœud avec $\beta_1=2$ (trous autour de C et de D, $d_2(C)=2$) ;
  - à r = 21/10, (1, 0).
- Ordre 1 : 6 composantes à r = 1/2 ; (1, 2) à r = 1, deux trous éphémères sur $[1,2/\sqrt{3})$ ; (1, 0) à r = 6/5.
- Représentants à r = 39/20 :
  - $A_2$ garde $\beta_1=2$ (homotopie, auditeur).
  - L'ombre contient ABC, ACD et BCD. Les témoins sont le centre de ABC et les milieux de AD et BD, avec
    $N_2=ABC,ACD,BCD$ et $d_2^2=4/3,\ 2+\sqrt{3}$. Le lacet à six sommets (milieux de CD, AD, AC, centre de ABC,
    milieux de BC, BD) est dans $\Omega_2(r)$, puisque chaque segment garde deux sites à ≤ r. Il fait un tour autour de
    C et devient contractile dans l'ombre : **l'homotopie de l'ombre est réfutée sur l'exemple même de la thèse
    (O5)**.
  - $D_2$ : $\alpha_1$ n'a autour de C que ABC, car ACDE est une cellule cocirculaire de rayon carré $8+4\sqrt{3}$.
    $D_2$ est contractile.
- Ces deux trous ont un rapport $2/\sqrt{2+\sqrt{3}}\approx1{,}035$. Aucune garantie d'entrelacement (D ×3, offset
  ×2, Alonso 1 + O(ε) pour un ε praticable) ne couvre un rapport aussi petit, et D les perd effectivement. Seule la
  voie exacte les garantit.

**Anneau** (octogone $(\pm1,\pm3),(\pm3,\pm1)$, cocirculaire de rayon carré 10, k = 2).

- Huit lentilles sous $\sqrt{5}$ ($\beta=(8,0)$ à $r^2=4{,}9$).
- L'anneau se ferme à $\sqrt{5}$, avec meb² = 5 pour trois sites consécutifs : (1, 1) à $r^2=6$.
- Le trou meurt à $\sqrt{10}$ : (1, 0) à $r^2=11$.
- Comme $d_1(0)^2=10>6$, le centre est un trou libre : l'ombre et D le gardent pendant **toute** sa vie (O6).

**Roue à moyeu** : l'anneau plus le site (0, 0), k = 2.

- HGP garde le trou : (1, 1) à $r^2=6$, car $d_2(0)^2=10$.
- L'ombre et D remplissent l'octogone. Ses huit triangles (moyeu, $q_i$, $q_{i+1}$) sont de Delaunay, avec des
  rayons carrés de 25/9 et 25/8, et leurs centres sont des témoins d'ordre 2. Le trou est **perdu**.
- C'est le cas « trou de roue avec moyeu » : seule la filtration d'ordre k, donc $A_k$ ou ses réductions certifiées,
  le garde.

## 9. Tableau et classement

| Représentant | K = 1 | R1 homotopie | R2 emboîtements, trace | R3 géométrie | R4 stabilité | R5 taille, coût |
| --- | --- | --- | --- | --- | --- | --- |
| $A_{k,v}$ + effondrements à sommets protégés (auditeur) | alpha | exacte, certifiée | r, arbre, trace (labels) ; k par $\pi_0$ | $d_H\le r$ à C, $\le D_v$ à A | filtration δ ; pas de Hausdorff | ~10³ faces/pt à k = 5, avant réduction |
| Ombre $S_v$ | = alpha (O2) | non (O5) ; trous libres oui (O6) | r oui ; k non (O4) ; trace exacte | $d_H\le r$, sommets sur les sites | pas de Hausdorff | taille de la mosaïque |
| $D_v$ (Delaunay des sites couverts) | = alpha (D1) | ×3-entrelacé (D3), trous libres oui | r, k, arbre oui ; trace exacte | $d_H\le2r$, sommets sur les sites | (×3, +δ) ; variante R' δ-stable | ≈ 28 faces/pt mesuré, indépendant de k |
| Offset | $\Omega_1(2r)$ | ×2 (F1) | r, arbre | enveloppe | δ | exige les $C_Q$ |
| SCov d'Alonso | union creuse | ×(1+3ε), δ ; identité hors fenêtres | bifiltration | non contrôlée | multicouverture | simplicial inutilisable en d = 3 ; spatial petit aux grandes échelles |
| DTM taille O(n) | non (autre hiérarchie) | ×$\sqrt{6k}$ | — | — | Wasserstein | O(n) |
| Supports / K-MST | MST | $\pi_0$ seulement | r, arbre, couverture datée | aucune | — | ≈ 14,5 nœuds/site à K = 5 |

**Ce que chacun dessine sur une surface LiDAR** (échantillon d'une nappe, espacement h, $r\gtrsim h$) :

- l'ombre est une feuille posée sur les sites mesurés, d'épaisseur celle du bruit, remplie là où les enveloppes des
  k-voisinages se chevauchent ;
- D est le maillage alpha de la nappe (triangles sur les sites), avec des tétraèdres là où elle est épaisse ;
- $A_k$ est une feuille de barycentres, décalée vers l'intérieur de la courbure d'une flèche de l'ordre de
  $\mathrm{diam}(Q)^2/(8R)$ pour un rayon de courbure R ;
- l'offset est l'enveloppe gonflée de r.

L'ombre et D gardent le trou d'une roue tant qu'il est libre au sens de O6. S'il est bouché par moins de k sites
(moyeu, rayons peu échantillonnés), seul $A_k$ le garde.

**Classement argumenté.**

1. **Référence exacte** : $A_{k,v}$ réduit à sommets protégés (auditeur). C'est la seule voie qui garde les trous
   bouchés (roue à moyeu, six points) : les fixtures montrent que l'ombre et D les perdent.
2. **$D_v$, premier représentant petit**.
   - Garanties : exact à K = 1, trace HGP exacte, emboîtements en r, en k et dans l'arbre, sommets sur les sites.
   - Coût : ≈ 28 faces par point, indépendant de k, sans mosaïque d'ordre supérieur (invariant respecté).
   - Constante 3, atteinte, et la meilleure possible dans la famille « cœur ».
   - Usage : comme jeton à une **coupe intérieure de marge ×3**. Tout trait vivant sur $[r,3r]$ y figure. Il faut
     vérifier contre FULL l'identité ($\pi_0$, couverture), qui est exacte par construction, et publier le facteur.
3. **L'ombre**, comme rendu de couverture sur les données, plus serré que D ($d_H\le r$). On la construit quand la
   mosaïque l'est déjà ; ce n'est pas une réduction.
4. **Alonso (A4)**, piste pour les niveaux grossiers des grands objets, où le net est petit. L'identité y est
   garantie hors des fenêtres $(r,cr]$. Le modèle simplicial est écarté.
5. **Offset, DTM**, comme enveloppes, attributs et couleurs ; **supports**, comme squelette $\pi_0$.

## 10. Questions ouvertes

1. Existe-t-il un représentant porté par les données (sommets dans P, trace exacte), homotope à $C_v(r)$ et calculable
   sans la mosaïque d'ordre k ? Rien ne l'interdit : dans la roue à moyeu, le cycle de la jante plus une arête pendante
   vers le moyeu convient. Mais ni l'ombre ni D ne font ce choix, qui demande l'information d'ordre k.
2. Pour k = 2, a-t-on $D_v\subseteq S_v$ ? La fixture F6 utilise k = 3. Pour $\lvert\tau\rvert\ge k$, on a
   $\tau\subseteq N_k(y_\tau)$ au témoin alpha, mais ce témoin peut tomber dans un autre nœud.
3. Peut-on certifier localement, par nœud et par niveau, l'égalité $H_*(D_v(r))\cong H_*(C_v(r))$ sans la mosaïque
   d'ordre k ? Par exemple par les seules boules critiques de FULL, et un test de « trous libres » au sens de O6.
4. La date de couverture $g_k(p)$ est-elle lue exactement dans les populations FULL actuelles, pour tous les sites et
   tous les k ≤ 10, au coût visé ? Non mesuré.
5. A4 à l'échelle : taille du dual pondéré par niveau, et entrelacement effectif avec FULL sur trames. Non mesuré.

## 11. Fichiers

- `NOTE.md` (ce fichier) ;
- `verif_alternatives.py` et `verif_sortie.json` : 25 portes exactes, vertes sous `-O` ;
- `mesure_del1.py` et `mesure_del1.json` : taille de $\mathrm{Del}_1$, mesurée en flottant ;
- `telechargements/alonso/` : PDF SoCG 2025 et texte ;
- `telechargements/core_bifiltration/` : arXiv 2405.01214v4, sha256 `6858c18c…`.

Rejeu :

```bash
cd /workspaces/E-HGP/build/v11-persist/polyedres_ordre_k/theorie_alternatives
/workspaces/E-HGP/build/v11-persist/videos/venv/bin/python -O verif_alternatives.py
```
