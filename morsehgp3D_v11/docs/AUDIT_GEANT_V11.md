# Audit géant de la v11 de Morse HGP 3D

7 octobre 2026. Contre-audit indépendant de la clôture, et dossier de compréhension pour la v12.

```text
phase=exploration_v11_hors_registre (close le 7 octobre 2026)
backend=cpu_reference ; voie de banc cuda_g4 pour le lot de feuilles du catalogue
profile=quantized_u21_input_only
public_status=not_claimed
```

**Demande de l'utilisateur** (7 octobre 2026) : « On va se lancer dans la v12 de Morse HGP 3D. Mais pour l'instant, je
te demande de faire un audit géant de la v11 de Morse HGP 3D dans le dossier morsehgp3D_v11 et de tout ce qui est
afférant pour comprendre dans les détails le modèle mathématique, les enjeux, les applications, les contraintes, les
contrats en objectifs et tout ce qui a pu marcher ou n'a pas marché jusque là. » Puis : « Tu as feu vert pour utiliser
la GCP G4. Le but est de faire une v12 aussi propre, simple et efficace que possible. »

**Rapport à l'audit du matin.** La [passation](../PASSATION.md) et l'[audit final](AUDIT_FINAL_V11.md), écrits le
même jour par une autre session, restent des sources. Le présent audit les a traités comme des affirmations à
vérifier : il les confirme pour l'essentiel, en corrige une trentaine de points (§ 8), et les complète sur cinq plans
qu'ils ne couvraient pas : le modèle mathématique expliqué de bout en bout et rattaché à la thèse ; la lignée entière,
de la ligne produit à la v11 ; une vérification par exécution ; une réanalyse des mesures sur les données brutes ; et
le constat que la conception d'origine de la v11 n'a pas été implantée (§ 7.5).

**Méthode.**
- Huit lectures indépendantes, en lecture seule sur `e968aba8d` (moteur gelé `ac081a06f`) : A, l'objet normatif (la
  thèse, lue en entier sur ses parties I–II) ; B, le contrat mathématique et ses preuves (refaites) ; C, le catalogue,
  l'index et la voie GPU ; D, la tour ; E, les sorties, la comparaison à HDBSCAN et les applications ; F, la lignée
  v2 → v10 ; G, la vérification par exécution locale ; H, les chiffres, la qualification, la mesure et le canal
  d'audit. Leurs rapports bruts sont dans [`receipts/audit_geant_v11_20261007/`](../receipts/audit_geant_v11_20261007/README.md).
- Lectures propres de l'auteur de cette synthèse : `MATHEMATIQUES.md` en entier, `ARCHITECTURE.md`,
  `CONCEPTION_MOTEUR.md`, `AUDIT_V10_SYNTHESE.md`, la passation, l'audit final, le plan 100 ms et les pistes de rupture
  du 4 octobre, les conceptions du 2 octobre ; recoupement direct des affirmations les plus lourdes (compteurs de
  l'ordre 10, cibles des points, seuil de la thèse, registre des preuves, reçu de la v8, jalon de 200 ms).
- Les documents de conception de la v11 et l'audit de la v10, jusqu'ici hors dépôt, sont versés dans
  [`receipts/conception_v11_20261002/`](../receipts/conception_v11_20261002/README.md) après revue.
- **Légende** : sans marque, un énoncé est vérifié dans le code, un reçu, un commit ou une exécution ; **[I]** marque une
  inférence. Les temps sont des mesures G4 (W48, trames sans sol ng00 / ng01 / ng02, grille 1 mm), sauf mention.

## 0. L'essentiel

1. **L'objet est juste, bien défini et bien compris.** La tour FULL est l'arbre des amas de Hartigan de l'estimateur
   $K$-NN, pour tous les ordres à la fois, avec les applications entre ordres : c'est l'objet de la thèse (Th. 2),
   enrichi de la tour multi-ordres et des verticales, que la thèse ne contient pas. Le contrat mathématique de la v11 le
   calcule sans aucune position générale ; toutes ses preuves ont été refaites pour cet audit, sans erreur de fond (§ 2).
2. **L'exactitude est le meilleur acquis.** Aucun résultat FULL faux n'a été établi : 3 303 vidages LiDAR comparés sur
   81 rapports de banc, 0 écart ; voie GPU identique au CPU à l'octet. Cet audit a reproduit localement, à l'octet, les
   empreintes de référence (§ 7.10).
3. **Le contrat de 100 ms n'a jamais été tenu, par aucune version, pour l'objet vrai.** Au gel, K5 à chaud :
   212–255 ms (voie GPU), 255–314 ms (voie CPU) ; la v10 faisait 204–254 ms. Le jalon de 200 ms a été franchi une fois
   (196,8 ms sur ng01). K10 : 1,3 à 1,8 s. Le chiffre de 100 ms date du 7 août et n'a jamais été fondé par une mesure
   (§ 5).
4. **La lenteur ne vient pas du volume de travail, mais du coût par unité et de chemins séquentiels.** À K10, la v11
   fait à peine plus de pas de descente que la v10 (×1,18) mais chaque pas coûte ×3 à ×4 : elle calcule chaque plus
   petite boule par énumération exacte exhaustive, là où la v10 la proposait en flottant puis la certifiait. Le
   catalogue et les forêts s'enchaînent en série, le publieur de l'ordre K est séquentiel, et le GPU ne travaille que
   50 ms sur 138 (§ 7).
5. **La v11 avait, le 2 octobre au matin, une conception plus rapide sur le papier, en partie prototypée, et ne l'a pas
   implantée.** Plus petite boule certifiée par le catalogue, noyau union-find sans lots, contraction parallèle des
   plateaux, verticales en $O(1)$, feuille vectorisée par lots : ses estimations (18–30 ms de catalogue, 22–32 ms de
   tour à K5) n'ont jamais été mesurées, et l'implantation a dérivé vers un autre moteur, trois à huit fois plus lent
   que ces cibles, sans que l'écart soit documenté (§ 7.5).
6. **La lignée enseigne une seule chose sur la vitesse** : douze générations en trois mois, et seuls deux changements
   d'algorithme (la chaîne v9, les boîtes de centres de la v10) ont fait gagner du temps ; les reprises « plus
   propres » à algorithme constant n'ont jamais rien rendu (§ 6).
7. **Contre HDBSCAN, la supériorité est établie au niveau de la hiérarchie en synthétique, faible et locale sur LiDAR,
   et pas établie au niveau de la sélection plate**, où se trouve le plafond ; aucun test préenregistré n'a été mené
   jusqu'au bout (§ 3.4).
8. **Le contrat est ambigu depuis deux mois** : froid ou résident, latence ou cadence, périmètre (le produit utile coûte
   aujourd'hui plus que la tour), K10, GPU, plage de tailles. Ces décisions précèdent tout code (§ 5.5, § 9.2).
9. **Le socle d'ingénierie est solide mais la qualification et la mesure étaient mal dimensionnées** : le code exact du
   gel n'a passé aucune matrice ; les seuils des leviers étaient 2 à 6 fois au-delà de l'effet, sans défaut de puissance
   (§ 7.7, § 7.8).
10. **Hygiène** : un scan KITTI brut est versionné dans un reçu de la v8 ; l'identité du compte GCP est dans 528
    fichiers de reçus ; `CLAUDE.md` est en retard (u18, pas de v10, un seuil relatif attribué à tort à la thèse) (§ 7.9).
11. **Pour une v12 propre, simple et efficace** : décider le contrat ; déclarer d'avance les changements d'algorithme
    (catalogue résident sur le GPU, plus petite boule certifiée, forêt sans lots, registre d'événements dont les sorties
    sont des vues) ; porter tout le reste explicitement ; mesurer d'abord par microbancs sur G4 ; un seul chemin
    produit, qui est le chemin mesuré (§ 9).

## 1. L'objet normatif : ce que dit la thèse

Source : `docs/references/MANUSCRIT_THESE_HAUSEUX.pdf`, Parties I–II (p. 1–108 et conclusion p. 165–169, lues en entier
pour cet audit). Les pages citées sont les pages imprimées (page PDF = page imprimée + 26).

### 1.1 La question et la réponse

**HGP signifie « Hypergraphe-Percol' »** (p. 5, p. 41, p. 52, Déf. 22). Le cadre est celui de Hartigan (Déf. 6) : on
estime une densité, et les amas sont les composantes connexes de ses sur-niveaux. Avec l'estimateur $K$-NN (Déf. 7),
$\hat{f}_K(y)=K/(n\,\omega_p\,r_K(y)^{p})$, où $r_K(y)$ est la distance au $K$-ième voisin ; le sur-niveau
$\lbrace\hat{f}_K\geq\lambda\rbrace$ est la région $L_K(r)=\lbrace y : \lvert\overline{B}(y,r)\cap X\rvert\geq K\rbrace$.

| Définition | Contenu |
| --- | --- |
| Déf. 8 (p. 21) | un point $x$ est couvert par une composante $C$ s'il est à distance au plus $r$ de $C$ ; l'amas discret est $X\cap\delta_r(C)$, dilatation **fermée** ; pour $K\geq 2$, ce n'est pas une partition |
| Déf. 20 (p. 57) | complexe de Čech : $\sigma\in\check{C}(X,r)$ si les boules fermées de rayon $r$ centrées sur $\sigma$ s'intersectent |
| Déf. 21 (p. 58) | graphe $\Gamma_K(X,r)$ : sommets = $K$-parties de Čech ; arêtes quand la réunion est dans Čech ; un **$K$-polyèdre** est l'ensemble des points d'une composante |
| Déf. 22 (p. 58) | $\theta_K^{HGP}(r)$ = les $K$-polyèdres de $\check{C}(X,r)$, pour tout $r\geq 0$ |
| Déf. 25–30 (p. 84–91) | rayon de naissance, position générale, $K$-simplexe séparant, de Gabriel, $K$-graphe de Gabriel, $K$-MST |

**Théorème 2 (p. 60), sans position générale** : les composantes de $\Gamma_K$ sont celles de $L_K(r)$, leurs ensembles de
points sont les amas discrets, et $\theta_1$ est la liaison simple. **Proposition 5** (p. 86) : les adjacences
élémentaires (réunions de $K+1$ points) suffisent. **Théorème 4** (p. 88) : tout $K$-simplexe séparant est de Gabriel
(sous position générale ; la v11 l'étend sans, lemme W.4).

### 1.2 Le § 6.1 : HDBSCAN échoue dès K = 2

Deux triangles équilatéraux de côté $2r$, dont deux sommets se font face à $2r$. La liaison simple robuste et
(H)DBSCAN à $K=2$ fusionnent les six points au niveau $r$. La hiérarchie de Hartigan à $K=2$ garde au niveau $r$ sept
segments isolés, puis trois amas qui se recouvrent à $r'=2\sqrt{3}r/3$ ($\lbrace A,B,C\rbrace$, $\lbrace C,D\rbrace$,
$\lbrace D,E,F\rbrace$), puis une seule composante à $r''\approx 1{,}932r$ par une **fusion ternaire simultanée**
(Fig. 6.4) : la thèse contient elle-même une multifusion. Le calcul exact le confirme (fixture
`tests/fixtures/exact/chapter6_six_points.json`).

### 1.3 Le § 9.1 : masses, condensation, vote

- L'objet naturel « n'est pas une partition de $X$, mais un recouvrement de $X$ (ou bien une partition des
  $(K-1)$-simplexes) ».
- Score d'une face : $S_\tau=\sum_{\sigma\supset\tau,\,\lvert\sigma\rvert=K+1}\psi(\rho(\sigma))$, avec $\psi(t)=t^{-p}$
  par défaut ($z=3$ en dimension 3) ; masse $m_\tau=S_\tau\sum_{x\in\tau}1/T_x$ où $T_x=\sum_{\tau\ni x}S_\tau$. Les
  poids $S_\tau/T_x$ forment une partition de l'unité : $\sum_\tau m_\tau$ compte les points couverts. **$m$ est un
  comptage doux de points, pas un comptage de faces.**
- Seuil (p. 97) : « c'est ce poids $m_\tau$, et non le simple comptage des faces, qui est utilisé par le seuil
  `min_cluster_size` dans l'arbre condensé ». **C'est un seuil absolu**, en unités de points ($\sqrt{n}$ dans les
  expériences SIPU, p. 101).
- Vote : $V_x(c)=\sum_{\tau\ni x,\,\ell(\tau)=c}S_\tau/T_x$, étiquette par argmax (Prop. 7).
- Le § 9.1 ne dit rien d'un seuil relatif ni d'une interdiction de binariser, et sa famille de faces dépend de
  l'algorithme (les faces de Gabriel dans la version standard) : **la masse n'est pas définie sur FULL tant que la
  famille de faces n'est pas choisie.**

### 1.4 Ce que la thèse contient de faux, et ce qu'elle ne contient pas

| Énoncé | Statut | Témoin |
| --- | --- | --- |
| Prop. 6 (p. 90) : les composantes non triviales du graphe de Gabriel ont les points des $K$-polyèdres | **faux** | E5, cinq sites, $K=2$ (`tests/fixtures/regressions/gabriel_point_set_counterexample.json`) |
| Th. 5 (p. 91) : même énoncé pour le $K$-MST | **faux sous ses propres hypothèses** : E5 est en position générale au sens de la Déf. 26 | E5 ; et le contre-exemple plan L02, où le graphe de Gabriel reste déconnecté pour toujours (**non gravé dans le dépôt**) |
| Déf. 30 : « le » $K$-MST est un arbre | mal posée : le graphe de Gabriel peut ne pas être connexe | L02 |
| Alg. 1 (p. 100), base des expériences | calcule une « HGP-Gabriel », pas exactement $\theta^{HGP}$ | 11 désaccords sur 1 500 nuages aléatoires (audit v10, L02) |
| Th. 7 (p. 99) | preuve esquissée, avec une lacune (une facette active n'est pas forcément de Gabriel) | — |
| Th. 1 (p. 17) | hypothèse de position générale superflue | registre, § 2 |
| Th. 3 (p. 71–72), Prop. 4 | conditionnels (continuité supposée, limites admises), déclarés comme tels | — |

**Ce que la thèse ne contient pas** : ni tour multi-ordres ni verticales (chaque $\theta_K$ est séparé ; la tour et
sa naturalité viennent de `docs/SPECIFICATION_MORSEHGP3D.md` § 3 et de `docs/math/DEFINITION_HGP_3D.md`) ; aucun
théorème de stabilité (« HGP-Clusterer n'est pas plus consistant que le Single-Linkage ; il est seulement
fractionnellement consistant », p. 168) ; aucune multiplicité ($X$ est un ensemble) ; aucune règle pour les
événements simultanés ; aucune borne de coût en 3D.

### 1.5 Le pont entre la thèse et la v11

La tour FULL de la v11 **est l'objet de la thèse, enrichi** : à chaque ordre, ses composantes sont celles de
$\Gamma_K$ (Th. 2), ses ensembles de points sont les $K$-polyèdres (lemme H). Elle ajoute les niveaux carrés exacts,
l'identité des composantes (et non seulement leurs points), les multifusions N-aires atomiques, les coupes ouvertes et
fermées, et les verticales entre ordres. Trois différences d'objet sont à garder en tête :

1. **Rayon ou rayon carré.** L'arbre est le même, mais toute grandeur qui lit la **valeur** du niveau en dépend :
   marges, scores d'EOM, masses $\psi(\rho)$, constantes de stabilité. La marge en niveau carré (règle $Q_1$ de la v10)
   n'a aucune constante uniforme ; la v11 l'a corrigée en mesurant en rayon. Règle pour la v12 : stocker en niveau carré
   exact, déclarer le rayon comme unité de toute grandeur métrique.
2. **Les facettes isolées.** La Déf. 22 et la Fig. 6.5 comptent $\lbrace C,D\rbrace$ comme un 2-polyèdre ; le profil
   `hgp_reduced` de la spécification les retire, avec la formulation fausse de la Prop. 6. La qualification
   $\Pi_{k+1}$ des points v11 (une composante ne reçoit de points que si elle a au moins deux sommets) est exactement
   ce critère : **le profil réduit vit déjà dans la v11, comme vue aval de FULL**.
3. **Le mot `profile` est surchargé** : profil d'objet (`hgp_reduced`, `full_pi0`, `generic_core`) dans `CLAUDE.md`,
   profil de quantification (`quantized_u21_input_only`) dans le cadre v11. La v12 doit annoncer les deux axes.

## 2. Le modèle mathématique, de l'objet au calcul

Cette partie se lit sans le code. Elle suit `docs/MATHEMATIQUES.md` (lu en entier pour cet audit), dont elle garde
la numérotation des énoncés (M, G, T, P, J, lemmes A à H), et la complète par la lecture de la thèse (§ 1) et par
l'audit critique des preuves (§ 2.10).

### 2.1 L'objet : la tour FULL

**Données.** Un ensemble $X$ de $n$ sites distincts de $\mathbb{R}^{3}$, à coordonnées entières dans
$[0,2^{B})^{3}$ ($B=21$ par défaut, soit une grille de 1 mm sur 2 km), chacun de poids un. Un ordre maximal $K$, avec
$1\leq K\leq\min(n,12)$.

**Fonction de distance d'ordre $k$.** Pour un point $y$ de l'espace, $D_k(y)$ est le carré de la distance de $y$ à son
$k$-ième plus proche site. Le sous-niveau

$$ L_k(a)=\lbrace y\in\mathbb{R}^{3} : D_k(y)\leq a\rbrace=\bigcup_{\lvert F\rvert=k}\ \bigcap_{x\in F}\overline{B}(x,\sqrt{a}) $$

est la région couverte par au moins $k$ boules fermées de rayon $\sqrt{a}$ centrées sur les sites. Elle croît avec $a$
et décroît avec $k$ : $L_{k+1}(a)\subseteq L_k(a)$.

**Lecture statistique.** $D_k$ est, à une transformation monotone près, l'inverse de l'estimateur de densité des
$k$ plus proches voisins. Les composantes connexes de $L_k(a)$ sont donc les « amas de densité » de Hartigan de cet
estimateur, à tous les seuils à la fois. L'arbre de fusion de ces composantes est l'arbre des amas (cluster tree) de
la densité $k$-NN. La tour les empile pour $k=1,\ldots,K$ : c'est une bifiltration ordre–échelle, dont l'axe $k$ joue
le rôle d'un lissage. À $k=1$, on retrouve exactement la liaison simple (single linkage), c'est-à-dire l'arbre
couvrant euclidien minimal (juge J2).

**La tour FULL** est la donnée, pour chaque $k\leq K$, de la forêt $T_k$ des composantes de $L_k(a)$ quand $a$ croît
(arbre de fusion), et des applications verticales induites par $L_{k}(a)\subseteq L_{k-1}(a)$. Conventions :

- un niveau est un **rayon au carré** rationnel exact, jamais réduit, comparé par produits croisés ;
- une fusion simultanée de plusieurs composantes est **un seul nœud N-aire** (plateau atomique) ; aucun parent n'a
  le niveau de son enfant ;
- un nœud est vivant à la coupe fermée $a$ si $a_v\leq a<a_{\mathrm{parent}(v)}$, à la coupe ouverte si
  $a_v<a\leq a_{\mathrm{parent}(v)}$ ; les deux coupes sont distinctes et toutes deux exploitées ;
- la numérotation est canonique : naissances par (niveau, centre exact), fusions par (niveau, plus petite naissance
  du sous-arbre) ; d'où des sorties identiques à l'octet quel que soit le nombre de fils.

**Modèle fini (T1).** Soit $\beta(F)$ le rayon carré de la plus petite boule englobante de $F$. Le graphe
$\Gamma_k(a)$ a pour sommets les $k$-parties $F$ avec $\beta(F)\leq a$ ; chaque $(k+1)$-partie $G$ avec
$\beta(G)\leq a$ relie toutes ses faces de cardinal $k$. Les composantes de $\Gamma_k(a)$ et de $L_k(a)$ sont en
bijection, compatible avec les niveaux et les verticales. La preuve tient en deux faits : chaque
$W_F(a)=\bigcap_{x\in F}\overline{B}(x,\sqrt{a})$ est convexe, et $W_F(a)\cap W_{F'}(a)\neq\varnothing$ si et
seulement si $\beta(F\cup F')\leq a$. C'est un nerf. Ce graphe a $\binom{n}{k}$ sommets potentiels : il définit
l'objet et sert d'oracle borné ($n\leq 14$), il n'est **jamais** matérialisé à l'échelle (règle absolue de
`CLAUDE.md` : ni mosaïque de Delaunay d'ordre supérieur, ni catalogue en $\binom{n}{k}$).

### 2.2 Les boules critiques

Tous les changements de topologie de la tour ont lieu sur des **boules critiques** : les plus petites boules
englobantes de parties d'au moins deux sites.

**M1.** La plus petite boule englobante $B(F)$ existe et est unique ; une boule contenant $F$ est $B(F)$ si et
seulement si son centre est dans l'enveloppe convexe des sites de $F$ situés sur sa sphère. Un sous-ensemble minimal
qui porte ce centre est affinement indépendant : au plus quatre sites.

**Anatomie d'une boule critique** $b=(c,\lambda)$ :

$$ I_b=\lbrace x : \lVert x-c\rVert^{2}<\lambda\rbrace,\quad U_b=\lbrace x : \lVert x-c\rVert^{2}=\lambda\rbrace,\quad P_b=I_b\cup U_b,\quad p=\lvert I_b\rvert,\quad m=\lvert U_b\rvert. $$

Un **support** est une partie $Q\subseteq U_b$ affinement indépendante dont le centre est dans l'intérieur relatif de
l'enveloppe convexe. Son cardinal minimal $q\in\lbrace 2,3,4\rbrace$ donne trois familles :

| Famille | Support | Condition exacte |
| --- | --- | --- |
| q2 | paire diamétrale | le centre est le milieu |
| q3 | triangle | strictement aigu, centre circonscrit dans son plan |
| q4 | tétraèdre | le centre circonscrit est strictement intérieur (quatre coordonnées barycentriques positives) |

Le **support canonique** $S^{*}(b)$ est le premier support de cardinal $q$ dans l'ordre lexicographique des rangs de
Morton. Il ne sert qu'à l'encodage déterministe : la quantification casse de toute façon l'équivariance par rotation.
Une coquille est **régulière** si $m=q$, **étendue** si $m>q$ (sites cosphériques supplémentaires : fréquents sur une
grille entière).

**M2 (certificat combinatoire).** Si $S$ est un support certifié de $b$ et $S\subseteq F\subseteq P_b$, alors
$B(F)=b$. C'est ce qui permet de proposer une boule vite, puis de la vérifier par un recensement exact.

**T2 (trace stricte), la clé de voûte.** Pour $F\subseteq P_b$ : $\beta(F)<\lambda$ si et seulement si $F\cap U_b$ est
**séparable**, c'est-à-dire si le centre n'est pas dans l'enveloppe convexe de $F\cap U_b$. T2 remplace partout
l'hypothèse de position générale de la thèse : les dégénérescences (cosphéricités, coplanarités, égalités de
niveaux) sont traitées exactement, sans perturbation symbolique ni jitter.

### 2.3 Le catalogue $\mathrm{Cat}_K$ et la fenêtre d'événements

**T3 (événement d'une boule à l'ordre $k$).** Si $\lvert P_b\rvert<k$, rien. Si $\lvert P_b\rvert=k$, la $k$-partie
$P_b$ naît isolée. Si $\lvert P_b\rvert\geq k+1$, toutes les $k$-parties de $P_b$ sont reliées au seuil fermé
$\lambda$ : les composantes strictes qu'elles touchent fusionnent, et les nouveaux sommets s'y rattachent.

Comme toute partie non séparable de la coquille contient un support, les $t$-parties de coquille avec
$t=k-p\leq q-2$ sont toutes séparables : aucun événement. D'où la **fenêtre d'événements**
$[p+q-1,\,p+m]\cap[1,K]$, et le catalogue utile jusqu'à l'ordre $K$ :

$$ \mathrm{Cat}_{K}=\lbrace b\ \text{critique} : p+q\leq K+1\rbrace. $$

À l'ordre $K$, les **boules d'événement** sont $W_K=\lbrace b\in\mathrm{Cat}_K : p+m\geq K\rbrace$. Parmi elles, les
**naissances** (aucune $K$-partie stricte, ce qui force $p+q\leq K$) et les **cellules** ($\lvert P_b\rvert\geq K+1$),
elles-mêmes **fortes** ($p+q\leq K\leq p+m$) ou **faibles** ($K=p+q-1$, qui ne portent que des fusions : le triangle
équilatéral $(0,0,0),(2,2,0),(2,0,2)$ réunit à $K=2$ trois paires nées au niveau 2, au niveau $8/3$).

**Taille.** Sur les trames LiDAR, $\mathrm{Cat}_5$ compte 31 à 33 boules par site (1,31 M boules sur ng00) et
$\mathrm{Cat}_{10}$ 120 à 138 (5,5 M sur ng00 et ng02) : une croissance en $nK^{2}$, cohérente avec des points
échantillonnés sur des surfaces (dimension intrinsèque deux). C'est l'objet qui fixe ce volume, pas l'algorithme
(`PISTES_DE_RUPTURE.md`, § 0).

### 2.4 Calculer le catalogue sans le graphe : boîtes de centres (G1 à G4)

On partitionne l'espace des **centres** possibles en boîtes demi-ouvertes. Chaque boîte $Q$ porte une liste $L$ de
sites candidats.

- **G1 (invariant de liste).** $L$ contient tous les $K$ plus proches voisins, ex æquo compris, de tout centre de
  $\overline{Q}$. Un site $x$ est retiré dès qu'il a $K$ **dominateurs** distincts, $z\prec_Q x$ signifiant que $z$ est
  strictement plus proche que $x$ en tout point de la boîte fermée. Le critère exact est affine :
  $\lVert x-l\rVert^{2}-\lVert z-l\rVert^{2}>\sum_i\max(0,2s_i(x_i-z_i))$, avec $l$ le coin inférieur et $s_i$ les
  côtés.
- **G2 (recensement local exact).** Si le vrai nombre d'intérieurs d'une boule centrée dans $Q$ est $p<K$, alors
  $P_b\subseteq L$ : un recensement sur $L$ qui trouve moins de $K$ intérieurs est globalement exact, coquille entière
  comprise.
- **G3 (élagage d'un support partiel).** Les dominateurs des sommets d'un support partiel de cardinal $r$ sont des
  intérieurs : leur nombre minore $p$. On rejette dès qu'il dépasse $\theta_r=K+1-r$. Aucun préfixe de $S^{*}$ d'une
  boule admise n'est rejeté.
- **G4 (propriété, complétude conditionnelle).** Chaque centre a une unique boîte propriétaire. Si la partition finale
  est **finie et entièrement traitée**, et si chaque feuille énumère tous ses supports, applique les prédicats exacts,
  recense la coquille entière et ne publie que $S^{*}$ dans sa boîte propriétaire, la sortie est exactement
  $\mathrm{Cat}_K$.

**Ce que G4 ne prouve pas** : ni la terminaison d'une politique arbitraire de subdivision, ni une borne
sous-quadratique du nombre de candidats. Un plafond de ressources produit un refus explicite, jamais une sortie dite
complète.

**Formules exactes (§ 2 de `MATHEMATIQUES.md`).** Le centre d'un support ancré en $a$ s'écrit $c=a+N/D$ avec $N$ et $D$
entiers ($D=2$ pour q2, $2\lVert u\times v\rVert^{2}$ pour q3, $2\det(u,v,z)$ pour q4) ; le côté d'un site se lit au
signe de $H_b(y)=D\lVert y-a\rVert^{2}-2N\cdot(y-a)$. Seul q3 est vraiment dur : le niveau d'un triangle est un
rationnel de degré six, $\lVert u\rVert^{2}\lVert v\rVert^{2}\lVert u-v\rVert^{2}/(4\lVert u\times v\rVert^{2})$.

### 2.5 Construire les forêts : cellules, descentes, plateaux

**Descente valide (T5).** Partir d'une $k$-partie $F$, calculer $b=B(F)$. Si $p\geq k$, prendre une $k$-partie de
$I_b$ ; sinon, si une $t$-partie séparable $A\subseteq U_b$ existe, prendre $I_b\cup A$ ; sinon s'arrêter : $b$ est une
naissance. Chaque pas diminue strictement $\beta$ (T2) et reste dans la composante de $F$ à la coupe fermée
$\beta(F)$. La descente termine, mais son terminal dépend des choix (sur $\lbrace 0,2,4\rbrace$ à $k=2$, la paire
extrême descend vers l'une ou l'autre paire voisine) : une politique déterministe est fixée.

**Graines (lemme D).** Pour une cellule $b$ et une trace stricte $F=I_b\cup A$, la naissance $g$ atteinte par une
descente donne directement la branche touchée : $\mathrm{anc}^{<}_{\lambda_b}(g)\in\mathrm{ant}(b)$, et le rattachement
$\mathrm{att}(b)=\mathrm{anc}_{\lambda_b}(g)$. Les descentes ne lisent jamais la structure d'union : **toute la
résolution peut se calculer d'avance, en parallèle**. C'est la propriété la plus importante pour la vitesse.

**Plateau atomique (T4, précisé par le lemme P).** À un niveau $\lambda$, on forme le graphe biparti entre les
composantes strictes ($\Gamma_K^{<}(\lambda)$) et les cellules de $W_K$ de ce niveau qui les touchent. Chaque
composante connexe qui réunit au moins deux composantes strictes est **une** multifusion, avec exactement ces enfants ;
les naissances du niveau restent isolées. Le nombre d'unions faites par chaque cellule dans un union-find dépend de
l'ordre de traitement (tétraèdre de l'audit : 2, 2, 1, 0 selon l'ordre) et n'est jamais publié.

**Reconstruction (T6, conditionnelle).** Catalogue complet, populations exactes, représentants couvrant chaque
composante stricte, descentes valides et T4 suffisent à reconstruire FULL. En termes algorithmiques, chaque forêt
$T_k$ est l'**arbre de fusion de Kruskal** d'un hypergraphe dont les sommets sont les naissances et les hyperarêtes les
cellules (pondérées par leur niveau, extrémités données par les graines), avec contraction des plateaux.

**Pièges de date, établis par des témoins.**
- Un mémo de cellule n'est valide qu'à partir du niveau de la cellule, jamais avant ($X=\lbrace 0,2,4,6\rbrace$).
- Une trace stricte peut naître **après** le niveau de rang $r_b-1$ : témoin D2, $41<64<1681/25$. Seuls les ensembles de
  nœuds vivants coïncident entre coupe ouverte à $\lambda_b$ et coupe fermée au rang $r_b-1$, pas les composantes.
- Les boules hors de $W_K$ ne créent ni ne fusionnent rien, mais leurs liaisons appartiennent au vrai $\Gamma_K$ :
  les retirer change $T_K$ (lemme W, fixture E5). Les descentes et le test strict lisent donc le vrai graphe.

**Verticales.** Pour une naissance d'ordre $k$, toutes les faces de cardinal $k-1$ de sa population sont connectées à
son niveau : résoudre une face puis remonter au nœud vivant à la coupe fermée donne son image d'ordre $k-1$. Pour une
fusion, les images de tous ses enfants doivent coïncider : c'est un invariant contrôlé dans le produit.

### 2.6 Juges

- **J1 (restriction).** Filtrer $\mathrm{Cat}_{K'}$ par $p+q\leq K+1$ redonne $\mathrm{Cat}_K$.
- **J2 (ordre un).** Les composantes de $\Gamma_1(a)$ sont celles de l'arbre couvrant euclidien minimal coupé aux
  arêtes de longueur carrée au plus $4a$.
- **J3 (Euler).** Pour chaque ordre, $n[k=1]+\sum_b e_k(b)=1$, avec une contribution $e_k(b)$ explicite par boule ;
  $\mathrm{Cat}_{K+2}$ suffit à juger tous les ordres jusqu'à $K$. C'est un **filet**, pas un certificat : deux
  omissions peuvent se compenser (cinq sites, triangle $+1$ et paire $-1$ au niveau 25).
- **Oracle borné à deux étages** (`reference/`) : l'étage A est la définition exhaustive de $\Gamma_k$ en `Fraction`
  ($n\leq 14$, $K\leq 10$), l'étage B une construction indépendante ; le juge exige B = A champ par champ.

### 2.7 Des forêts aux points

Une composante de $L_k(a)$ n'est pas un groupe de sites : pour $k\geq 2$, les amas se recouvrent, et la thèse
l'assume (§ 9.1 : l'objet naturel est un recouvrement de $X$, ou une partition des $(K-1)$-simplexes).

- **P1 (core).** Le site $x$ entre à $D_k(x)$ dans l'unique composante de $L_k(D_k(x))$ qui contient $x$.
- **P2 (cover).** $A_k(x)=\min_{F\ni x,\,\lvert F\rvert=k}\beta(F)$, avec $D_k(x)/4\leq A_k(x)\leq D_k(x)$ ; l'ensemble
  $E_k(x)$ des composantes qui couvrent $x$ à cette date n'est pas un singleton en général.
- **P3 (témoins forts).** La première entrée $A_k(x)$ a un témoin fort contenant $x$ ; le lemme H en donne la forme
  datée : le $K$-polyèdre d'un nœud est la réunion des populations des boules fortes rattachées à son sous-arbre.
- **P4 (laminarité).** À ordre fixé, attacher chaque site une fois donne une famille laminaire ; **entre ordres,
  non** : sur l'axe, $\lbrace 0,10,11,26,27,45,46\rbrace$ donne des groupes qui se croisent.
- **P5 (stabilité en rayon).** Un déplacement apparié d'au plus $\varepsilon$ entrelace les tours à
  $r\pm\varepsilon$, verticales comprises. Une grille de pas $h$ donne $\varepsilon\leq\sqrt{3}h/2$. En revanche, la
  projection par plus petit ancêtre commun n'hérite pas de cette stabilité ($\lbrace 0,2,4\rbrace$ contre
  $\lbrace 0,2,4+\delta\rbrace$ : un saut de rayon 1 pour un déplacement $\delta$ arbitrairement petit).

La règle retenue par la v11 est $H^{r}_{k+1}=P_1\circ\Pi_{k+1}$ : l'ancrage persistant de la v10 (marge mesurée en
rayon) appliqué à la couverture qualifiée (une composante ne reçoit des points que si sa composante de $\Gamma_k$ a au
moins deux sommets). Elle est fidèle (aucune réunion de points avant la fusion FULL), laminaire, stable en
$3\varepsilon$ pour les dates et les hauteurs, indépendante de la taille minimale de groupe. Son prix est prouvé
aussi : une entrée est retardée d'au plus $d_k/2$, le respect du cœur est perdu à $k=2$, une structure isolée de $k$
sites n'est jamais un bloc. La sortie plate applique ensuite une condensation (critère A) et une sélection EOM N-aire
exacte, sans aucune décision flottante.

### 2.8 La hiérarchie des supports (sortie `supports`)

Le § 10 de `MATHEMATIQUES.md` démontre sans position générale les lemmes A (une boule de $W_K$ relie toutes ses
$K$-parties), P (plateau), W (périmètre ; le théorème 4 de la thèse étendu), B (rôles : naissance, fusion, interne),
C (branches), D (graines), E (juge par descente), F (supports minimaux, Carathéodory strict), G (comptes) et H
(polyèdres datés). La sortie publiée depuis le 6 octobre ne garde que l'**arbre couvrant de Kruskal** : chaque
naissance, et pour chaque fusion les boules de fusion qui réalisent au moins une union dans l'ordre (niveau, $S^{*}$),
avec $S^{*}$ seul. Un filtre par rôle ne suffit pas : trois sites équidistants ferment un cycle de plateau.

### 2.9 Doctrine numérique

« Le flottant propose, l'entier décide. » Chaque expression implantée porte un budget de bits calculé en `constexpr`
(un débordement est une erreur de compilation) ; trois voies (native, contrôlée, entiers larges) ; les clés
flottantes de tri et les filtres de signe n'existent que sous des bornes d'erreur propagées **par expression** (F3,
F6), avec repli exact dans la bande d'incertitude et sous tout mode d'arrondi. Les leçons payées sont précises : une
borne sur le résultat final ne borne pas les intermédiaires (premier produit $6s^{6}\geq 2^{127}$ pour
$s=2^{21}-1$ alors que la puissance finale tient dans un `i128`) ; une borne par nombre d'instructions est fausse
(témoin $N=2^{53}+1$) ; un minimum sur les points entiers d'une boîte n'est pas le minimum continu.

Le prédicat de côté d'un support q3 (somme des magnitudes $216M^{6}$ avec $M=2^{B}$) est le seul qui ne tienne pas
dans un `i128` natif aux profils u21 et u24 : il passe par un certificat calculé une fois par la fabrique, sinon par
une voie contrôlée, sinon par des entiers larges. Les comparaisons de niveaux ($14B+20$ bits) et de centres
($9B+11$ bits) sont toujours larges : elles passent par les clés de tri F3/F4, avec repli exact.


### 2.10 Audit des preuves

Toutes les preuves du contrat ont été refaites pour cet audit (lecture B). **Aucune erreur de fond.** T2 remplace bien
la position générale partout. Mais les preuves n'ont pas toutes le même degré de rédaction, et le registre couvre mal
la v11.

| Énoncés | Degré de preuve dans `MATHEMATIQUES.md` | Au registre des preuves |
| --- | --- | --- |
| M1, M2, G1–G3, T1, T2, T3, T5, P1–P5, J1, J2 ; lemmes A, P, W, B–H | complets | seulement les quatre sections V11 (points, sortie plate, Euler, supports) ; **aucune ligne** pour M1–M2, G1–G4, T1–T6, P1–P5, J2 |
| T4 (plateau) | recette au § 5 ; la preuve est le lemme P du § 10.3 | — |
| T6 (suffisance, verticales) | récurrence esquissée, correcte | — |
| J3 (Euler) | résumé ; preuve complète seulement dans la v9 (`CONTRELEC_EULER_PAR_NERF_20260923.md`) | `proved_here` |
| Suffisance de Kruskal (§ 10.10 bis) | conclusion juste, mais la preuve cite C.3 alors que la connexité vient de P.3 | **absente** (contrairement à ce que disent la passation et l'audit final) |
| Lemmes de calcul : table de populations, R, V3, diamètre, cliques, zonogone, M3/E4, niveaux différés, réemploi vertical, MST/contraction | complets, dispersés dans des documents de conception | aucune ligne |
| G4, T6 | **conditionnels par construction** : ni terminaison de la subdivision ni borne de travail | — |

Défauts à corriger : la ligne du lemme E est collée à celle du témoin D2 par un `\n` littéral (registre, l. 1370) ;
deux § 10.10 ; le § 10 décrit encore la sortie `supports` v1 (plafond de 24 sites, invariance) après la décision du
6 octobre ; l'en-tête annonce u18 ; plusieurs contradictions n'ont pas leur ligne au registre (cycle de Kruskal au
plateau, « feuilles ≤ sites », F3 par nombre d'instructions, F2 par degré, mémo valide dès la date terminale, sept
sites, $\lbrace 0,2,5\rbrace$). Le juge J2 est énoncé mais **jamais implanté**. Les identifiants entrent en collision :
J2, J3, P1, G4, V3, R, E1, D2 et T1 désignent chacun plusieurs objets dans les documents, le code et le canal d'audit.

**L'oracle borné tient ses comptes** (rejoués pour cet audit) : 342 nuages, 1 362 ordres, 48 234 coupes, 13 029 nœuds
en 21 s ; 22 mutants tués et 3 équivalents justifiés ; oracle des supports S1 : 210 nuages, 951 ordres, 13 mutants
tués. Sa suite complète (5 617 nuages, 38 633 ordres, 3,4 M coupes) a passé sur G4. Ses limites : $n\leq 14$, profil
18 bits pour l'essentiel, S1 limité à K ≤ 5 et à des coquilles de 12 sites, 145 sauts de descente seulement contre
des millions sur une trame. Son étage B réutilise les formules du moteur : c'est l'étage A, la définition exhaustive,
qui juge.

### 2.11 Galerie des contradictions

Chaque défaut grave de la v11 s'est vu sur un témoin de quelques sites ou à une borne exacte. Les témoins sont la
mémoire du projet.

| Témoin | Ce qu'il réfute |
| --- | --- |
| **E5** : $(0,0,7),(0,9,6),(1,4,0),(0,0,1),(4,1,2)$, $K=2$ | Prop. 6 et Th. 5 de la thèse ; toute réduction par le graphe de Gabriel ; « retirer les liaisons hors $W_K$ ne change pas $T_K$ » |
| Sept sites sur l'axe $\lbrace 0,10,11,26,27,45,46\rbrace$ | une hiérarchie de points commune à tous les ordres qui garde les groupes de chaque ordre |
| $\lbrace 0,2,4\rbrace$ contre $\lbrace 0,2,4+\delta\rbrace$, $k=2$ | stabilité de la projection par plus petit ancêtre commun ; existence d'un propriétaire équivariant |
| $\lbrace 0,2,5\rbrace$, $k=2$ | cover = MR₂-bord (atteignabilité mutuelle) |
| Cercle à quatre points, puis un point déplacé | stabilité de la réalisation par supports (saut de Hausdorff ≥ 1/4) |
| D2 : $(2,10),(18,10),(10,20),(9,3),(11,3)$ | « une trace stricte naît avant le rang précédent » ($41<64<1681/25$) |
| $X=\lbrace 0,2,4,6\rbrace$ | un mémo valide dès la date terminale de la descente |
| Triangle équidistant $(0,0,0),(1,1,0),(1,0,1)$ à K1 | « les boules de rôle fusion forment un arbre couvrant » (cycle de plateau) |
| Triangle $(0,1,1),(1,0,1),(1,1,0)$ à K1, translaté | l'équivariance par translation de la sortie SPv2 (nouveau, non gravé) |
| Cube $\lbrace 0,16\rbrace^{3}$ et son centre, K5 | « feuilles ≤ sites » (80 feuilles pour 9 sites ; modèle seulement, non gravé) |
| Cinq sites portant un triangle $+1$ et une paire $-1$ au niveau 25 | Euler comme certificat de complétude |
| $N=2^{53}+1$ ; $x=32767$, $A=512x^{3}$ | borne flottante par nombre d'instructions (F3) ; borne par degré (F2) |
| $s=2^{21}-1$, triangle aigu | « une borne sur le résultat borne les intermédiaires » ($6s^{6}\geq 2^{127}$) |
| Tétraèdre régulier en u24 | export du niveau sur trois mots (196/148 bits) |
| Racine $2^{127}-1$ | conversion d'une racine u128 en i128 |
| $(2,162,50)$ et $(8,98,32)$, tous deux $5\sqrt{2}$ | départage de sommes de radicaux en décimal ou en binary64 |
| Trois réfutations du polyèdre (`1fbeea5b8`) | « moins de $k$ aberrants n'agissent pas » ; identité des chaînes contractées ; emboîtement des réalisations entre ordres |

### 2.12 Questions mathématiques ouvertes, par priorité pour la v12

**Haute priorité (elles conditionnent le contrat ou les sorties).**
1. **Identité des nœuds sous quantification.** 61 à 67 % des nœuds K5 vivent moins de $2\delta$
   ($\delta=\sqrt{3}/2$ mm) ; P5 ne garantit une image stable qu'au-delà. Publier la vie de chaque nœud rapportée à
   $\delta$ ; n'appuyer sélection et jetons que sur des vies supérieures à $2\delta$ ; reformuler « garder les
   séparations fugaces » en critère de stabilité.
2. **Multiplicités.** Écrire et prouver le contrat pondéré (modèle par copies : T2, T3, G1–G3 avec poids, naissances
   nulles, fenêtres non contiguës), déjà implanté dans l'oracle ; d'ici là, refuser avec un compteur publié.
3. **Départage canonique invariant par translation** pour $S^{*}$ et pour l'ordre de Kruskal (ordre lexicographique des
   coordonnées).
4. **Forêt parallèle** : inscrire au registre le lemme MST/contraction et le théorème T4 de la conception, avec leurs
   fixtures (ternaire $\lbrace 0,2,4\rbrace$, E5, triangle des compteurs, contraction d'hyperarête entière), et le
   **contrat des compteurs** (logiques ou physiques) avant tout port.

**Priorité moyenne.**
5. Coquilles étendues : un quotient polynomial prouvé (la conception de la tour en propose un, théorème T7, en
   $O(m^{3})$ prédicats, contrôlé sur 5 937 cas) ; d'ici là, refus explicite au-delà d'un plafond.
6. Borne de travail et terminaison de la subdivision des boîtes.
7. Masses du § 9.1 sur FULL : choisir la famille de faces ; trancher la tension avec la fixture des deux triangles.
8. Points : une règle stable par insertion, locale au profil, qui passe T0 et Q1–Q4 ; synthèse multi-K par décalage en
   $k$ et verticales.

**En aval.**
9. Polyèdre d'ordre $k$ : un représentant petit, de topologie certifiée et robuste (huit questions à l'auditeur).
10. Partition T > 0 compatible avec u24 (sans effet mesuré sur LiDAR).

## 3. Enjeux et applications

### 3.1 Pourquoi cet objet

La thèse pose une question simple (p. 2) : la hiérarchie des niveaux de densité est donnée par un graphe quand on
estime la densité avec un seul voisin ; que devient-elle quand $K\geq 2$ ? Sa réponse : remplacer le graphe par le
complexe de Čech, et la composante connexe par le $K$-polyèdre. Pour $K=2$, « ce ne sont plus les points qui
percolent directement, mais les arêtes, recollées entre elles par des triangles » (p. 5). Le Théorème 2 (p. 60)
identifie les $K$-polyèdres aux amas discrets de forte densité de l'estimateur $K$-NN, niveau par niveau. La tour
n'est donc pas un regroupement de plus : c'est le calcul exact d'un objet statistique défini, l'arbre des amas de
Hartigan de l'estimateur $K$-NN, pour tous les $K\leq K_{\max}$ à la fois.

Ce que l'axe $K$ apporte, et ce qu'il n'apporte pas :

- **Il retarde la percolation parasite.** Les ponts de bruit et les contacts ténus cèdent quand $K$ monte (Tab. 7.1 :
  la vitesse de percolation de HGP croît avec $K$, celle de la liaison simple robuste décroît).
- **Il fait fondre les parties minces.** Les roues lisibles à $K=2$ ne le sont plus à $K=5$. Aucun $K$ unique ne voit
  à la fois un objet mince et un objet bruité : d'où la tour entière, et non une tranche.
- **Une tranche à $K$ fixé n'est pas robuste aux aberrants.** Ajouter moins de $k$ sites peut créer ou réunir des
  composantes ; la forme juste est un décalage en $k$ :
  $\Omega_k^{P}\subseteq\Omega_k^{P\cup O}\subseteq\Omega_{k-m}^{P}$ pour $m=\lvert O\rvert<k$ (registre, l. 154,
  fixture `polyhedron_order_k_counterexamples.json`). C'est la raison mathématique de garder les verticales.
- **La densité ne sépare pas ce qui se touche.** Un vélo contre un mur, une voiture sur l'asphalte sont reliés à tout
  $(K,r)$ : ni la tour ni HDBSCAN ne les séparent (0 objet exact sur 42 objets à moins de 0,1 m, mémoire du
  1er octobre). Le retrait du sol est lui-même une constante posée à la main.

### 3.2 Les quatre usages et ce qu'ils exigent

| Usage | Sortie consommée | Exigences | Ce que la v11 fournit |
| --- | --- | --- | --- |
| Clustering hiérarchique (HGP-Clusterer, thèse § 9) | hiérarchie de points, condensation, sélection ; ou recouvrement et vote du § 9.1 | fidélité à FULL ; multifusions jamais binarisées ; masse du § 9.1 pour le seuil | `points` ($H^{r}_{K+1}$) et `plat`, mais condensation au comptage entier, pas à la masse du § 9.1 |
| Segmentation d'instances LiDAR | partition plate | découper plutôt que fusionner ; cadence de 10 trames par seconde ; objets en contact | `plat` (EOM, $z=1$, mcs 20) ; coût hors tour non mesuré sur G4 ; contacts avec un mur inséparables |
| Enseignant d'un modèle de fondation 3D (Zoltan, pilote A0G1) | `coverage_v1` : états datés, cartes à la coupe, unions de sites | exact, déterministe, recalculé par vue augmentée, mis en cache | **non livré** |
| Jetons du modèle de fondation (Zoltan, pilote A1) | `weighted_gabriel_v1` : incidences coface–facette, $S_\tau$, $T_x$, masses, condensation N-aire, verticales | environ 160 000 unités par branche ; départage canonique jamais par `PointId` | **non livré** ; impossible depuis SPv2, qui n'a ni $I_b$, ni $U_b$, ni boules internes |

Le projet Zoltan (`Zoltan/FoundationModel/`) porte la thèse applicative : tout encodeur 3D code en dur une échelle
métrique (taille de voxel, liste de rayons, taille de patch), et c'est elle qui casse quand la portée ou le capteur
changent ; la tour fournit une échelle canonique dérivée des données, qu'on **substitue** au composant qui porte la
constante, à budget apparié et avec des témoins négatifs. Aucune expérience apprise n'existe encore.

### 3.3 Pourquoi 100 ms, et ce que ce chiffre mesure vraiment

**L'origine** est dans la thèse (§ 5.4.1, p. 47–48) : la segmentation 4D sur SemanticKITTI prend « de l'ordre de la
seconde pour l'instant lorsqu'une version industrielle impose de pouvoir traiter […] dix trames par seconde ».
L'application de la thèse tournait par trame **et par classe d'intérêt**, avec des a priori de taille : elle était
plus facile que la cible de la v11 (trames entières sans sol, toutes classes confondues).

**Trois distinctions jamais posées ensemble** :

1. **Latence ou cadence.** Un capteur à 10 Hz exige un débit de dix trames par seconde, pas une latence de 100 ms. Si
   le catalogue tourne sur le GPU et la tour sur le CPU, deux trames successives peuvent se recouvrir : la cadence est
   alors bornée par le plus lent des deux étages, pas par leur somme. Au gel, les deux étages valent déjà 91 à 141 ms
   chacun à K5 en voie GPU.
2. **Froid ou résident.** Posée dès le 7 août (contrat 50 000 points : contexte CUDA 1 242 ms à froid, 18 ms à chaud),
   jamais tranchée. Elle décide si l'ouverture du contexte (75 à 150 ms) entre dans le budget.
3. **Périmètre.** Le produit utile à la segmentation n'est pas FULL. Hors tour, `plat` coûte environ 0,4 s et
   `points` 0,75 s en local à W8 (`attach`, sortie, écriture), et l'écriture de `full` sur G4 coûte 2,16 s pour
   301 Mo à K5 (14,4 s pour 1,46 Go à K10), limitée par un SHA-256 logiciel sériel. Même avec une tour gratuite, `plat`
   ne tient pas en 100 ms aujourd'hui.

Pour Zoltan, la latence n'est pas l'enjeu : la tour se met en cache. Ce sont le débit et le stockage qui comptent. À
301 Mo par trame en `MHGP11FUL1` à K5, une passe sur les 19 130 scans d'entraînement de SemanticKITTI pèserait
environ 5,8 To (28 To à K10). **Pour cette application, le format compact est le verrou, pas le temps.**

### 3.4 HGP contre HDBSCAN : ce qui est établi

La comparaison se lit à trois niveaux, à ne jamais confondre : A, la tour contient-elle l'objet (meilleur amas
discret) ; B, la hiérarchie le contient-elle (meilleur bloc) ; C, la sélection plate le rend-elle.

| Constat | Valeur | Portée |
| --- | --- | --- |
| Niveau B, synthétique, n = 8 000 | +0,008 / +0,026 / +0,050 / +0,078 à k = 2, 3, 5, 10, intervalles hors de 0, deux sessions G4 | **établi** ; mais niveaux « medium » et « hard » calibrés sur l'échec de HDBSCAN, et gain dû à l'entrée par couverture plus qu'à la connexité d'ordre supérieur (MR₂-bord, sans tour, rattrape aussi le vélo C ; le bras MR$_k$-bord n'a jamais tourné) |
| Niveau B, LiDAR, trames voisines des échecs | sauvetages/pertes +19/−8, +22/−7, +27/−4, +35/−1 sur 859 instances | cohortes enrichies en échecs ; **témoins** (20 trames) : 0 sauvetage, 0 perte ; estimateur pondéré v10 sur 299 trames : +0,0002 à K5, intervalle contenant 0 |
| Borne de l'enjeu LiDAR | HDBSCAN n'échoue au niveau B que pour 33 instances sur 2 918 à K5 (1,1 %), surtout des vélos garés (16 %) | le gain possible au niveau B est de quelques pour cent des instances |
| Niveau C, LiDAR, EOM $z=1$ | 68,5 % des couples (scène, k) retrouvent tous les objets, contre 56,6 % pour `sklearn` | population **conditionnée au succès** de $H^{r}$ ; 74 % de découpes de voitures, qui portent 87 % de l'écart ; à k = 10 : 0,779 contre 0,765 ; 31 bouts comptés deux fois |
| Niveau C, synthétique | test préenregistré S2b : le gain vient de la **tête certifiée** (z = 2, EOM N-aire exacte), qui s'applique aussi à l'arbre de `sklearn` ; la hiérarchie perd à k = 10 | sans reçu dans le dépôt |
| PQ sur trames entières (19 objets) | tête v11 0,845 contre HDBSCAN 0,756 à K5 ; **0,769 contre 0,781 à K10** | échantillon minuscule |
| Démos | 13 gains, 3 pertes sur 360 bouts | règle « il existe un k » avec priorité au gain ; par ordre : 4/2, 5/2, 10/1, 7/0 |
| Préenregistrement E1 | S3b et P08 jamais lancées ; S2b et S3a sans reçu ; corrigendum manquant | aucun test préenregistré décisif mené jusqu'au bout |

**Jugement.** Au niveau de la tour et du meilleur bloc, la supériorité est établie en synthétique, sans être
attribuée à la connexité d'ordre supérieur. Sur LiDAR, elle est faible et locale. Au niveau de la sélection, elle
n'est pas établie. La directive « HDBSCAN ne peut pas battre la tour » reste compatible avec les données aux niveaux
A et B ; les données montrent pourtant HDBSCAN égal ou devant dans plusieurs réglages. **Le plafond est dans la
sélection** : une antichaîne oracle retrouverait 252 objets sur 258, l'EOM à $z=1$ en retrouve 198.

**Une précision de fond sur les « séparations fugaces ».** Le vélo 3 n'est séparé qu'entre 107,4 et 107,8 mm ; les
formes reconnaissables du vélo synthétique tiennent sur des nœuds qui vivent 0,1 à 0,4 mm ; 61 à 67 % des nœuds K5
vivent moins de $2\delta$, avec $\delta=\sqrt{3}/2$ mm, le déplacement maximal dû à la grille. Par P5, seule une vie
supérieure à $2\delta$ garantit qu'un nœud a une image stable sous la quantification. **Ces séparations ne sont pas
des traits identifiables d'une donnée quantifiée au millimètre** : l'objectif « garder les séparations fugaces » est
mal posé tel quel, et doit devenir un critère de stabilité explicite.

**Fragilités du banc.** `np.argsort` est instable dans `_process_mst` de `sklearn` (étiquetages dépendants de la
machine) ; `sklearn` binarise les plateaux (la tête N-aire sur son arbre diffère de ses étiquettes sur 520
configurations sur 2 400) ; versions 1.7.2 sur G4 et 1.9.1 sur le codespace, jamais qualifiées sur un même banc ;
séquence 08 utilisée pour le développement alors que `Zoltan/FoundationModel/MESURE.md` la réserve au bilan.

### 3.5 Les polyèdres d'ordre k

Demandés le 6 octobre pour un jeton de forme, ils ont été fixés avec l'auditeur : $A_k(r)$ est la mosaïque de
Delaunay d'ordre $k$ filtrée par $d_k$ (le complexe alpha pour $k=1$), qui prolonge le nerf des régions témoins et a
$\Gamma_K$ pour 1-squelette. Garanties : même type d'homotopie que $\Omega_k(r)$, $\pi_0$ égal aux nœuds de FULL
(0 désaccord sur 91 ordres et 37 993 coupes de l'oracle borné). Limites mesurées : environ 1 000 faces par site à K5
(62 à 82 fois la tour), dessin instable à une coupe critique, un seul site proche crée une composante, vélo réel
08/002852 illisible à tout K ; trois affirmations réfutées et gravées (`1fbeea5b8`). Huit questions restent posées à
l'auditeur. Leur place est en aval, à la demande, hors du chemin des 100 ms : c'est aussi une pièce qui touche à
l'invariant d'architecture (pas de mosaïque d'ordre supérieur dans le chemin produit).

## 4. Les contraintes

Les contraintes de ce projet sont de quatre natures. Elles ne se négocient pas au même niveau : les premières viennent
de l'objet, les deuxièmes de la doctrine de preuve, les troisièmes de l'exploitation, les dernières des données.

### 4.1 Contraintes de l'objet

- **Exactitude.** L'objet calculé est la tour exacte, pas une approximation : niveaux rationnels exacts, plateaux N-aires
  exacts, verticales exactes. Un résultat partiel ou approché n'est jamais publié comme complet ; un dépassement de
  ressources est un refus explicite.
- **Aucune position générale.** Les grilles entières produisent des cosphéricités et des égalités de niveaux en
  masse ; T2 les traite exactement. Aucun jitter, aucune perturbation symbolique.
- **Sites distincts de poids un.** Les retours LiDAR qui tombent sur le même millimètre forment un site de
  multiplicité $w\geq 2$ ; la tour pondérée n'est pas écrite, donc une entrée pondérée est refusée
  (`unsupported_degeneracy`). Décision utilisateur en attente (passation, § 6.1, point 6).
- **Invariant d'architecture.** Ni mosaïque de Delaunay d'ordre supérieur, ni catalogue global de cellules ou de
  cofaces en $\binom{n}{k}$. Les oracles exhaustifs restent bornés ($n\leq 14$) et hors du chemin produit. Les pistes
  fermées ne se rouvrent qu'avec un nouveau théorème de complétude et une fixture, jamais sur un banc.
- **Volume imposé par l'objet.** 1,3 à 1,4 M boules et 1,5 à 1,7 M nœuds de tour par trame à $K=5$ ; 5,5 M boules et
  7,5 M nœuds à $K=10$ (trame ng02). Aucune optimisation ne réduit ce volume : elle réduit le coût par unité.

### 4.2 Contraintes de preuve et de test (`CLAUDE.md`, `AGENTS.md`, `ARCHITECTURE.md`)

- **Statuts.** Aucun banc, accord moyen ou sortie plausible ne promeut `public_status=exact`. La v11 est hors registre
  (`public_status=not_claimed`) et ne touche pas `docs/implementation_status.toml`.
- **Contradiction → fixture.** Toute contradiction mathématique devient une fixture minimale permanente et met à jour
  `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md` avant de continuer.
- **Tailles d'intérêt.** Toute conclusion de coût, de sélectivité, de mémoire ou d'échelle se mesure à
  $n=8\,000$, $16\,000$, $32\,000$ et, depuis le 6 octobre, d'abord sur les trames LiDAR réelles ; les petites tailles
  ne servent que d'oracle de correction.
- **Pas de vérification exhaustive.** Ce qu'un théorème garantit est invoqué, pas re-parcouru ; on teste la faute
  d'implémentation par invariants globaux, juges d'échantillon et mutants. Les oracles bornés T2 établissent la
  vérité au lieu de la re-vérifier.
- **Portes à code exact** (0 conforme, 1 désaccord, 2 refus, 3 invariant violé, 4 mutant tué ; un signal est un
  échec), planchers contre le vert par vacuité, mutants causaux, déterminisme à tout nombre de fils.
- **Propreté** (`ARCHITECTURE.md`, § 1) : un module par dossier, fichiers de 500 lignes et fonctions de 100 lignes au
  plus, aucun état global, `Result` sans exception, transactions sans préfixe publié, budget mémoire réservé avant
  toute allocation, chaque prédicat avec son budget de bits et chaque élagage avec son lemme.
- **Équations Markdown** sur une ligne physique, sans `\operatorname`, contrôlées par `tools/check_docs.py`.

### 4.3 Contraintes d'exploitation

- **Machine du contrat** : VM GCP `g4-standard-48` (AMD EPYC 9B45, 24 cœurs physiques et 48 fils, trois L3 de
  32 Mio ; GPU RTX PRO 6000 Blackwell sm_120). GCC 11.4, CMake 3.22.1, Python 3.10 **sans pip** sur la VM.
- **Sécurité GCP** : jamais de VM hors des scripts gardés (`gcp-migration/v11_session.py`), SPOT, label
  `project=e-hgp`, double coupe-circuit, `maxRunDuration` borné, arrêt `TERMINATED` certifié sur la cible exacte après
  chaque session ; « GCP non utilisé » sinon.
- **Codespace** : 8 cœurs, 31 Go, `/workspaces` plein à 97 %, `/tmp` vidé aux redémarrages (environ toutes les 6 h,
  et hors calendrier) ; le local ne prédit pas G4, ni les instructions le temps.
- **Git** : commits sur `main`, jamais de branche, un worktree par acteur, index partagé à vérifier avant tout
  `git add`.

### 4.4 Contraintes de données et de licence

- **SemanticKITTI** (CC BY-NC-SA) : aucun octet de données dans le dépôt ; les trames sont reconstruites par script
  hors dépôt (`build/v11-full-data-20261002/`, avec empreintes).
- **Licences** : MIT pour le code actif ; `HGP-old/` (code historique de la thèse, licence non commerciale) n'est
  jamais importé ni pris pour oracle ; les poids Pointcept (CC-BY-NC 4.0) sont hors de la ligne produit.
- **Données de test du contrat** : trois trames d'**une seule** séquence (08/000000, 000100, 000200), sans sol par
  Patchwork++ épinglé (39 885, 35 551 et 45 845 sites). Ce n'est pas « plusieurs séquences » (`AGENTS.md`) : toute
  conclusion de vitesse reste conditionnelle à ce petit échantillon.

## 5. Contrats et objectifs

### 5.1 Généalogie du contrat de temps

| Date | Énoncé | Objet réellement calculé à l'époque |
| --- | --- | --- |
| 7 août | 50 000 points, $K_{\max}=10$, p95 « warm » < 100 ms ; contexte CUDA hors chronomètre (`docs/research/CONTRAT_50K_BILAN.md`) | substitut point-MST, rejeté depuis |
| 17 août (v4) | K1..10 < 100 ms sur G4 ; secondaire K5 < 1 s ; « dizaines de millions » | fold de Gabriel, faux en général (E5) |
| 27 août (v5) | contrat 50 000 mesuré sans seuil ; cible 10 à 30 M points | fold |
| 4–5 sept. (v7) | 50 000 points : toute la tour K1..10 < 1 s sur G4, repli K1..5, puis 100 ms | FULL |
| 21 sept. | **trame SemanticKITTI entière**, sans sous-échantillonnage, K1..10 < 1 s, repli K5, puis 100 ms ; puis, le soir, **LiDAR sans sol de 30 000 à 60 000 points** comme régime prioritaire | flux de candidats seul (v8) |
| 22 sept. | entier 18 bits (grille 1 mm) ; float32 arrêté ; multi-millions secondaire | — |
| 2 oct. (v11) | « 100 ms sur nuages LiDAR sans sol (éventuellement avec sol), K = 5 et si possible K = 10 » ; comparer à HDBSCAN | FULL |
| 3 oct. | jalon : « essaie de respecter 200 ms à K5 sans sol » | FULL |
| 4 oct. | « l'objectif du contrat est toujours 100 ms » | FULL |

Le chiffre de 100 ms a traversé trois objets (substitut, fold, FULL), deux régimes (synthétique 50k, puis LiDAR sans sol)
et un changement de taille. **Aucune mesure n'a jamais fondé sa faisabilité pour FULL.** Les jugements de faisabilité
sont restés des estimations : « atteignable en CPU seul à K = 5, 65 à 91 ms » (2 octobre), démenti ; « non démontré,
pas exclu » (4 octobre).

### 5.2 Le contrat tel qu'il se lit aujourd'hui

- **Objet** : la tour FULL des ordres 1 à K, verticales comprises (un seul ordre, un sous-nuage, un catalogue seul ou une
  tour sans verticales ne la remplacent pas).
- **Données** : trames SemanticKITTI sans sol, de 30 000 à 60 000 sites, grille 1 mm, profil entier (u21 depuis le
  2 octobre).
- **Machine** : G4 (48 fils, GPU Blackwell), « sessions gardées seulement ».
- **Temps** : 100 ms à K = 5 ; K = 10 « si possible » ; jalon intermédiaire de 200 ms.
- **Exactitude** : décisions entières exactes, aucune position générale, refus plutôt que dégradation, identité à
  l'octet quel que soit le nombre de fils.
- **Comparaison** : HDBSCAN (`sklearn`, tel quel) sur synthétique et sur réel, avec des exemples où HDBSCAN échoue et
  où HGP réussit.

### 5.3 Où en est-on

| Mode | À froid | À chaud | `domain` / forêts (à chaud) |
| --- | --- | --- | --- |
| v11, K5, voie GPU | 335 / 301 / 345 ms | 251 / 212 / 255 ms | 138 / 114, 120 / 91, 141 / 114 |
| v11, K5, voie CPU | 343 / 272 / 329 ms | 314 / 255 / 313 ms | 200 / 113, 163 / 92, 195 / 116 |
| v11, K10, voie GPU | 1 824 / 1 395 / 1 602 ms | 1 782 / 1 336 / 1 536 ms | 489 / 1 293, 398 / 939, 471 / 1 065 |
| v10, K5, CPU (une passe chaude) | — | 252 / 204 / 254 ms | 164 / 89, 137 / 67, 164 / 89 |
| v10, K10, CPU (une passe chaude) | — | 1 125 / 861 / 1 024 ms | 653 / 472, 528 / 334, 618 / 406 |

Trames ng00 / ng01 / ng02 (08/000000, 000100, 000200 ; 39 885, 35 551 et 45 845 sites). Session `claudeg1`, moteur
du gel ; « à froid » est la médiane haute de six processus (trois à K10). La référence v10 est une seule passe (la
troisième), hors préparation (6,7 à 8,3 ms), en u18 : la comparaison est descriptive, pas un A/B.

**Le jalon de 200 ms a été franchi une fois** : 196,8 ms à chaud sur ng01, voie GPU, réglage glibc `@tas` (session
`claudetas1`) ; le cache de blocs du moteur qui l'a remplacé a ensuite mesuré 212 ms. **Le contrat de 100 ms n'a
jamais été tenu, par aucune version, pour l'objet vrai.**

**Attention : les temps publiés ne sont pas ceux du produit.** L'exécutable `mhgp11` et l'API jouent une autre voie
(masque 278523, feuilles de 16 à tout K, ni GPU ni cache) ; leur `domain` à K10 coûte environ 1,7 s, contre 0,83 à
0,90 s au banc.

### 5.4 Ce que 100 ms imposent

Le volume de travail est fixé par l'objet : 1,3 à 1,4 M boules et 1,5 à 1,7 M nœuds à K5. Pour 100 ms à 48 fils, il
faut environ 2 à 2,2 s de CPU d'un fil par trame, là où la v11 en dépense 9 à 12 (voie GPU 9,4 CPU·s, voie CPU 12,2
CPU·s sur ng00). Deux étages s'enchaînent sans recouvrement, chacun déjà au-dessus de 90 ms à K5. Décomposition d'un
budget de 100 ms à K5, consolidée à partir des lectures C et D :

| Étage | Au gel (K5, ng00, chaud) | Budget v12 | Condition |
| --- | ---: | ---: | --- |
| préparation, index | < 1 ms | ≤ 5 ms | structures ouvertes une fois par Session |
| `domain` : frontière + étages de fin | 48,6 ms | ≤ 10 ms | les fins sont linéaires (19 ns par boule) : il faut les faire sur l'appareil ou les fusionner au tri |
| `domain` : parcours + feuilles | 89 ms | ≤ 30–40 ms | catalogue résident sur le GPU et en flux, feuille data-parallèle |
| forêts : préambule | 13–18 ms | ≤ 5 ms | table de populations produite par le catalogue ou résidente |
| forêts : résolution | 63–83 ms | ≤ 25–30 ms | coût du pas divisé par 2,5 à 3 (plus petite boule proposée puis certifiée, filtres, registres légers) |
| forêts : publication de l'ordre K | 81–99 ms (cohabitation) | 8–12 ms, recouvert | noyau union-find sans lots, contraction parallèle des plateaux |
| verticales | collées au publieur | ≤ 5 ms | historique d'attache, image O(1) des naissances |

**La frontière et les étages de fin du catalogue coûtent déjà, à eux seuls, tout le budget de l'étage** (48,6 ms en
voie GPU) : même avec un parcours et des feuilles gratuits, la structure actuelle ne tient pas 40 à 50 ms. **Un
catalogue sur CPU seul ne descend pas sous 140 à 170 ms à K5** même en cumulant tous les leviers connus (lecture C) :
le budget exige un `domain` résident sur le GPU.

**K = 10.** Toutes les estimations (conception du 2 octobre, plan du 4 octobre, lectures C et D) placent la tour
seule au-dessus de 100 ms en CPU (150 à 300 ms au mieux) : K = 10 à 100 ms est hors de portée de cette famille
d'algorithmes ; viser 0,3 à 0,5 s est réaliste, ce qui serait déjà ×3 à ×6 sur le gel.

### 5.5 Ce qui n'a jamais été tranché, et doit l'être avant tout code

1. **Froid ou résident** (posé le 7 août, reposé le 4 et le 7 octobre). Il décide si le contexte CUDA (75 à 150 ms)
   entre dans le budget.
2. **Latence ou cadence.** À 10 Hz, deux trames peuvent se recouvrir si le catalogue et la tour vivent sur des
   ressources différentes.
3. **Maximum ou médiane de la plage**, et la plage elle-même. Sur les 132 trames sans sol distinctes de la séquence 08
   du reçu `pts4_review_20261003`, **89 dépassent 60 000 sites** et aucune n'est sous 30 000 : médiane 68 049 sites,
   maximum 126 267 (vérifié pour cet audit). Les trois trames du contrat (36 000 à 46 000 sites) sont parmi les plus
   petites ; la plage déclarée exclut la majorité des trames réelles.
4. **Le GPU est-il dans le chemin contractuel ?**
5. **Périmètre** : FULL seul, ou aussi `points`, `plat`, l'écriture du format ?
6. **K = 10** : contrat ou objectif ?
7. **Trame brute avec sol** : le contrat principal du 21 septembre (« trame entière ») n'a jamais été retiré, mais le
   2 octobre dit « éventuellement avec sol ».
8. **Séquences** : trois trames d'une seule séquence ne sont pas « plusieurs scènes » (`AGENTS.md`). Toute décision
   de vitesse devrait se prendre sur plusieurs séquences.

## 6. La lignée : ce qui a marché, ce qui n'a pas marché

### 6.1 Douze générations en trois mois

`HGP-old` (Python et Cython de la thèse, 7 juillet), la ligne produit `morsehgp3d/` (14 juillet au 9 août), puis les
versions v2 à v11, du 8 août au 7 octobre : dix bases de code en 60 jours, environ six jours chacune.

| Version | Dates | Objet calculé | Idée d'architecture | Meilleur résultat sur le contrat du moment | Fin |
| --- | --- | --- | --- | --- | --- |
| v2 | 8 août | arbre de fusion de $d_K$ (objet juste) | catalogue local par point, force brute « en attendant » | n = 200, K = 10 : 92,5 s | générateur condamné |
| v3 | 8–16 août | juge $\Gamma_k$ exhaustif ; dix forêts jamais livrées | WSPD, lanes q2/q3/q4, certificats de blocs | 50k uniforme : 78,8 s pour la chaîne ; NO-GO | post-mortem WSPD |
| v4 | 17–27 août | fold des cofaces de Gabriel, **faux en général (E5)** | arbre radix de Karras, WSPD par vagues | 8 000 points : K10 343 s en local ; aucune exécution G4 | audit : « garder comme oracle », non suivi |
| v5 | 27–31 août | même fold | réécriture propre de la v4 | 50k : 56,6 s sur G4 | Θ(n²) boules sur `linked_arcs_u16` |
| v6 | 31 août–2 sept. | même fold | génération « sensible à la sortie » | 50k : K10 47,4 s, K5 9,07 s | changement d'objet |
| v7 | 4–11 sept. | **FULL** (retour au bon objet) | tour par boules, MEB à coquille libre | 50k, un fil : K1..10 418,9 s | refonte |
| v8 | 13–22 sept. | **aucune tour** : générateur q2/q3/q4 seul | 34 tranches sur le front WSPD ; float32, puis u18 | flux seul, sans sol, K5 : 104,6 s en local | « pas la tour » |
| v9 | 22–28 sept. | FULL v7 portée | générateur v8 + tour v7, voies GPU | K5 : 0,76–0,98 s (chaîne GPU ; mur de processus 1,5–1,9 s) | audit critique : générateur quasi quadratique |
| v10 | 28 sept.–2 oct. | FULL + tête de clustering | **boîtes de centres** (le WSPD disparaît), descentes, `SiteTree` | K5 : 204–254 ms ; K10 : 0,86–1,12 s (CPU, chaud) | « repartir de zéro pour quelque chose de plus propre » |
| v11 | 2–7 oct. | FULL (K ≤ 12) + `supports`, `points`, `plat` | catalogue en passe unique, feuilles GPU, pipeline des forêts | K5 : 212–314 ms à chaud ; K10 : 1,34–1,78 s | « tout reconstruire à neuf pour une v12 » |

### 6.2 Les leçons de la lignée

**Seuls les changements d'algorithme ont fait gagner du temps.** Les deux sauts réels sont la chaîne v9 (assemblage
v8 + v7) et les boîtes de centres de la v10 (×3,7 à 3,9 sur la v9 GPU, environ ×11 sur son moteur CPU, en environ
29 heures de code moteur). Les reprises « plus propres » à algorithme constant (v4 → v5, v5 → v6, v10 → v11) n'ont
jamais rendu de vitesse : la v11 était ×5,7 à 6,0 plus lente que la v10 le 3 octobre, ×1,5 à 1,7 le 4, et l'est encore
de ×1,5 à K10 au gel.

**L'objet a oscillé.** La v2 calculait déjà le bon objet ; les v4 à v6 ont calculé le fold de Gabriel, que la fixture
E5 réfutait depuis le 14 juillet ; la v7 est revenue à FULL. E5 est revenue cinq fois dans la lignée. La v11 l'a enfin
gravée dans sa référence ; il faut l'imposer à tout producteur et à tout lecteur.

**Les différentiels n'ont été fermés que sur un objet faux** (v4 ≡ v5 ≡ v6, sur un digest de filtre). Celui de la v10
à la v11 n'a porté que sur 14 petites fixtures, alors que l'architecture de la v11 l'exigeait sur trames entières.

**La qualification a mangé le calendrier.** 99 % des fichiers de la v7 sont des reçus ; la v8 a fait 34 tranches sans
tour ; une tranche locale de la v11 a pris 4 h 30. L'utilisateur est intervenu trois fois : « trop de garde-fous »
(2 septembre), « pas de pinaillage » (22 septembre), « pourquoi si long ? » (5 octobre).

**Le GPU a été greffé six fois sur un étage**, et l'hôte l'a toujours borné : les noyaux pèsent 1 à 4 % de l'étage
(v6 : 154 ms de noyaux sur 7,7 s d'étage ; v7 : 29,8 ms sur 846 ms ; v11 : le GPU ne travaille que 50 ms sur 138).

**Le contrat est resté ambigu pendant deux mois** : froid ou résident (posé le 7 août), maximum ou médiane, GPU
contractuel, périmètre, trame brute, vérité terrain.

**Ce qui a marché, et doit survivre** : la doctrine entière exacte (flottant seulement en filtre certifié à repli
exact) ; les oracles bornés qui établissent la vérité avec une arithmétique différente ; les fixtures minimales ; les
portes à code exact et les mutants causaux (nés des portes vacueuses de la v3) ; le registre des preuves, seul document
vivant à travers toutes les versions ; les sessions G4 gardées ; et les audits, qui ont porté les deux vrais sauts de
vitesse (la v9 est issue de l'audit de la v8, les boîtes de centres de la lentille L13 de l'audit de la v9).

### 6.3 Registre des pistes fermées, par force de fermeture

Une fermeture par **preuve ou fixture** est définitive ; une fermeture par **mesure**, **modèle de coût** ou
**consigne** ne vaut que pour son régime (distinction de la v9, `FAUSSES_PISTES.md`). Ne se rouvre qu'avec un théorème
de complétude nouveau et une fixture qui falsifie le motif d'abandon.

**Définitives (preuve, fixture, invariant)** :
- toute réduction sans attaches silencieuses : graphe des cofaces de Gabriel, fold v4, K-MST élagué (Prop. 6 et Th. 5
  de la thèse, fixtures E5 et L02) ;
- mosaïque de Delaunay d'ordre supérieur, $\Gamma$ global, catalogue en $\binom{n}{k}$ dans le chemin produit ;
- fenêtre de Morton fixe ou préfixe $k$-NN comme autorité exhaustive ;
- MST d'atteignabilité mutuelle sur les points pour $K\geq 2$ (oublie l'identité des facettes) ; le substitut
  point-MST du produit ;
- rang héréditaire, fermeture par les paires de rang utile ; sous-quadratique pour toute entrée (borne $\Omega(N^{2})$
  de la sortie dès $K=2$) ;
- Euler comme certificat (omissions qui se compensent) ;
- marge en niveau carré pour les points (aucune constante uniforme) ;
- supports v1 et `kparties_reliees` comme réalisation stable ;
- seuil de condensation relatif au parent, comme règle par défaut (rejet structurel : un objet qui fusionne avec plus
  massif que lui est jugé léger, quelle que soit sa persistance).

**Par mesure, pour leur régime** :
- générateur WSPD et toutes ses micro-variantes (caduc depuis les boîtes de centres) ;
- Borůvka à étiquette minimale de la v9 (1 264,7 ms en local), Kruskal par lots de la v10, lots ordre par ordre et
  mémos de lane de la v11 ;
- feuille GPU un fil par feuille, feuille coopérative par paires, sous-lots L4, arène N1, pages de 2 Mio sur G4 ;
- partition T > 0 sur LiDAR (ablation T = 6/3/0 : vidages identiques, ±0,3 % de travail).

**Par consigne** : float32 natif (22 septembre) ; micro-variantes q2 ; séparation WSPD sous 8.

**Rouvertes à tort ou fragiles** : E5 (cinq fois) ; le GPU en étage (six fois) ; « reconstruire plutôt que garder
l'ancienne version comme oracle » ; deux recommandations de la passation sans fondement mesuré (« partition T > 0 » et
« feuille J3 de la v10 », qui n'a jamais été dans la v10 mesurée).

## 7. La v11 en détail

### 7.1 Ce qu'est le moteur

Environ 22 200 lignes de C++20 sous `src/` (douze modules : `core`, `sched`, `num`, `cloud`, `io`, `index`, `catalogue`,
`tower`, `supports`, `points`, `head`, `api`), 50 300 lignes de tests, 4 700 lignes d'oracle Python, 19 100 lignes de
bancs ; 362 commits en six jours, dont une centaine d'auditeurs. Un seul exécutable, `mhgp11`, à sortie obligatoire
(`full`, `supports`, `points`, `plat`), et une sonde de banc, `mhgp11_full_bench`, qui porte les modes mesurés.

Le calcul enchaîne deux étages : `domain` (le catalogue $\mathrm{Cat}_K$ et sa table support → boule), puis les forêts
(pour chaque ordre : classification des cellules, naissances, résolution des graines, publication par plateaux,
verticales).

### 7.2 L'étage `domain` (catalogue)

**L'algorithme au gel.** Une frontière adaptative (racine filtrée en série, puis 27 à 28 rondes « lourds d'abord » sur
la population intérieure aux boîtes, au plus 1 024 tâches, réclamées par taille de liste décroissante) ; une passe
unique où chaque tâche poursuit le parcours en profondeur des boîtes (filtre G1 sur un réservoir des $3K$ témoins les
plus proches, ajustement de la boîte à l'enveloppe de sa liste, bissection au milieu du plus long côté) ; des feuilles
de 32 sites au plus (dominances en masques, graphe de paires, lignes vivantes, préfixes q2 → q4 en cliques sous G3,
droites J2 avec mémo, census par masques du lemme R, émission du seul $S^{*}$, niveaux q3 et q4 différés) ; enfin tri
indirect par clés binary64 F3/F4 à repli exact, balayage des niveaux, assemblage par blocs, table à sondage linéaire
remplie par CAS.

**Ce qu'il coûte** (ng00, K5, une passe chaude) :

| Sous-étage (ms) | Voie GPU `868347:400`, feuilles 24 | Voie CPU `802811`, feuilles 16 | K10, voie GPU |
| --- | ---: | ---: | ---: |
| Frontière | 20,7 | 19,9 | 26,4 |
| Passe unique | 37,5 | 152,1 | 141,3 |
| Lot de feuilles (GPU 91 679 feuilles, hôte 31 902) | 51,8 | — | 207,2 |
| Tri / balayage / assemblage / compactage | 10,0 / 4,4 / 3,4 / 3,2 | 10,0 / 4,4 / 3,3 / 2,8 | 42,8 / 19,4 / 14,7 / 11,5 |
| Divers (Level hôte, restitution, table) | 6,9 | 6,5 | 26,5 |
| **`domain`** | **138,0** | **199,2** | **489,9** |
| dont frontière + fin | **48,6** | **46,9** | **141,3** |

Volumes : 1 306 696 boules à Cat₅ sur ng00 (32,8 par site ; 30,7 à 30,8 sur les deux autres trames), 5,5 M à Cat₁₀
(120 à 138 par site) ; 4,6 incidences par boule à K5, 8,2 à K10 ; 353 456 feuilles à K5/16, et chaque site figure dans
environ 124 feuilles. Sur nuages uniformes 3D, 75 à 79 boules par site à K5 : la géométrie surfacique du LiDAR divise le
volume par 2,4.

**Ce qui a marché** : la frontière possédée et le Pool (26 s → 4,5 s) ; le tri indirect puis les clés F3/F4
(0,9–1,4 s → 9–13 ms) ; l'assemblage par blocs (167–250 ms → 4–5 ms) ; la frontière adaptative et la passe unique
(×1,25–1,6 puis ×1,65–1,8) ; « lourds d'abord » et LPT (plus longue tâche 0,885 → 0,045 s) ; le graphe de paires ; les
niveaux q4 (99,7 % évités) puis q3 différés ; le cache de blocs du budget (restitution 14 → 0,8 ms ; mur chaud ×0,923).

**Ce qui n'a pas marché** : la frontière fixe de profondeur 8 ; le filtre flottant F6 des puissances (≈ 1 %) ; les
enveloppes M3/E4 sur CPU (+1,2 à 1,4 %, retrait approuvé mais jamais fait) ; l'arène N1 ; la frontière par tranches ;
les pages de 2 Mio (−17 % en local, +6,4 % sur G4) ; les feuilles de 24 sur CPU à K5.

**Ce que la v10 faisait autrement, et qui n'a pas été porté** : une frontière en largeur jusqu'à $64P$ tâches (3 072 à
48 fils) au lieu de 27 rondes étroites ; une feuille « v2 » à masques avec une table des triplets vivants H et des
quadruplets par ET de trois lignes de H ; la table des tailles de feuille M(K) (16 jusqu'à K = 6, 24 jusqu'à K = 10) ;
des boîtes à $1/64$ de maille (T = 6), sans effet mesurable sur LiDAR. **La « feuille J3 » n'a jamais été dans le
moteur v10** (`777406b82`) : c'est un prototype mesuré seulement en local (×1,37 à K5 sur l'étage des boîtes, au plus
quatre fils). Elle n'explique donc pas la vitesse de la v10, contrairement à ce que disent la passation (§ 3) et
l'audit final (§ 14.2). L'écart du catalogue CPU (×1,2) a d'autres causes, non isolées : profil u18 contre u21 (×1,05),
feuille v2 à table H, frontière plus large.

**Le plancher CPU.** Le `domain` coûte 5,3 s de CPU d'un fil à K5 sur ng00, et passe à l'échelle ×24 de 1 à 48 fils
(24 cœurs physiques). Même en cumulant les leviers connus, il ne descendrait que vers 3,5–4 s, soit 140 à 170 ms à
48 fils. **Le budget de 40 à 50 ms est hors d'atteinte sans GPU.**

### 7.3 La voie GPU

**Ce qui existe.** Une feuille en source unique hôte/CUDA (`leaf_device.hpp`, port fidèle mais distinct de `leaf.cpp` :
deux implantations à tenir égales) ; le contrat R7 : seuls les chemins `i128` dont la borne est prouvée décident, sinon
la feuille rend `unresolved` et est rejouée entière sur l'hôte avant admission ; un exécuteur CUDA (un fil par feuille,
CUB pour les préfixes) ; un exécuteur partagé qui donne 40 % des feuilles (les plus lourdes, au poids $m^{3}$) à l'hôte.

**Ce qui est acquis** : l'exactitude. 1 602 prises à froid et 378 processus à chaud en voie GPU, 0 écart de vidage
(sur 3 303 vidages LiDAR de 81 rapports de banc, toutes voies confondues). Compute Sanitizer sans erreur.

**Pourquoi elle est sous-employée** :
1. **Amdahl** : le GPU ne fait que les feuilles, environ 50 ms sur 138, et reste inactif pendant la frontière, le
   parcours, les étages de fin et toutes les forêts.
2. **Join global** : le lot attend la fin du parcours.
3. **Un fil par feuille** : 3,1 à 3,4 fils actifs sur 32 par warp, occupation 17 à 22 %, pile locale de 3,3 Kio lue à
   2,2 octets utiles par secteur, ALU à 24 %. La moitié des feuilles n'émet rien ; d'autres émettent jusqu'à 609 boules.
4. **Contexte par processus** : 69 à 78 ms seul, 124 à 151 ms en concurrence du parcours. À froid, la voie GPU perd
   (`domain` 224/205/231 ms contre 208/190/209 pour la voie CPU).
5. **Mémoire proportionnelle aux feuilles** (cases de 2 Kio) : 243 Mo à K5, 1,42 Go à K10 pour 40 000 sites ;
   environ 30 Go extrapolés pour un million de sites à K10.

**Ce qui a échoué** : le lot sur l'hôte seul ; trier les fils par taille de feuille (m prédit mal le travail) ; les
blocs d'un warp ; les 16 sous-lots recouverts (L4, ×1,6 à 3,5 plus lent) ; la feuille coopérative par paires (×1,15 à
1,38, divergence intacte) ; le réservoir sans appel (meilleur SASS statique, ×1,11 sur G4). **Ces échecs réfutent des
découpages, pas le principe d'un catalogue résident sur le GPU.**

### 7.4 La tour : forêts, graines, publieurs, verticales

**L'algorithme au gel.** Pour chaque ordre $k$ : (A) classification des cellules (une cellule régulière, $m=q$, est
analytique) ; (B) naissances, cohortes de même rang triées par centre exact ; (C) résolution des graines, d'abord par la
table de populations (une $k$-partie égale à $P_b$ est un pas terminal), sinon par descente (MEB bornée, puis census sur
l'index radix, support canonique), avec la garde « date initiale strictement inférieure au niveau de la cellule » ;
(D) publication par plateau de rang (`find`, `touch`, `unite_roots`, puis `close`, une seule multifusion N-aire par
plateau) ; (E) verticales par balayage des ancêtres à coupe fermée, avec réemploi des graines régulières (857 771
descentes verticales évitées sur 857 891 à K5).

**Le parallélisme.** Une seule distribution de $W$ tâches : $L=W-(2K-1)$ résolveurs, $K$ publieurs (un par ordre),
$K-1$ suiveurs verticaux, synchronisés par époques et futex. À 48 fils : 39 résolveurs à K5, 29 à K10. Les octets sont
identiques à tout $W$ grâce à cinq invariants : résolution en fonction pure du domaine immuable ; écritures à positions
fixées par ordinal ; un seul écrivain par ordre, dans l'ordre canonique des boules ; union par minimum ; clôtures triées.

**Le chemin critique.** À K5 : un préambule de 13 à 18 ms (dont la table de populations de 44 Mo, reconstruite à chaque
appel), puis le maximum entre la résolution (63 à 83 ms) et le publieur de l'ordre 5 (81 à 102 ms). Ce publieur ne
coûte que 35 à 46 ms seul sur un fil : la cohabitation avec 47 autres fils le double. Les plateaux sont presque
singletons (448 698 cellules pour 438 011 clôtures à l'ordre 5) : une barrière par plateau est sans espoir. À K10, la
résolution domine (1 214 / 894 / 988 ms, 26 à 35 CPU·s) ; le préambule vaut 61 à 78 ms (table de populations
d'environ 360 Mo, estimation).

**Pourquoi la tour v11 est ×2,6 à 2,8 plus lente que celle de la v10 à K10.** Ce n'est pas le travail : à l'ordre 10
sur ng00, la v11 fait 6,18 M pas contre 5,26 M (×1,18), 2,35 M MEB contre 1,97 M (×1,20) et 1,16 M census contre
0,95 M (×1,22), pour exactement les mêmes 3,83 M graines. C'est **le coût du pas** : ×3 à ×4 à un fil, ×2,65 en temps
de fil à 48 fils, multiplié par ×1,2 de pas et environ ×1,25 de parallélisme effectif (29 voies contre 48 fils). La
cause la mieux établie : **la v11 calcule chaque plus petite boule par énumération exacte exhaustive** de toutes les
paires, puis tous les triplets, puis tous les quadruplets (74,2 présentations de supports par MEB à l'ordre 10, contre
4,6 à l'ordre 5 : la croissance suit $\binom{k}{3}+\binom{k}{4}$), alors que **la v10 propose la boule en flottant
(Welzl) et la certifie en exact** (0 repli sur 7,66 M). Viennent ensuite le census exact sur l'index et des surcoûts
fixes par pas (comparaisons exactes de niveaux de 180 à 200 bits, 37 registres transactionnels recopiés à chaque pas,
sonde d'une table de plusieurs centaines de mégaoctets).

Les causes avancées par l'audit final (§ 3.2) sont surestimées ou fausses : le choix des $k$ plus proches ne vaut que
+3 % de MEB (mutant `JUMP_ANY` de la v10) ; le mémo daté vaut 7 à 9 % des pas ; la partition T6 concerne les boîtes du
catalogue, pas les descentes.

**Ce qui a marché** : le mémo daté (×1,27–1,32, supplanté ensuite) ; les lots réguliers par voies (14,8 → 6,2 s) ; le
census réutilisé et les verticales parallèles ; le réemploi vertical ; le mode 16379 (table de populations, ordres
concurrents, environ 600 barrières supprimées) ; le pipeline futex ; le placement O1 (×0,917) ; les bornes entières et
l'arbre radix V3 (tests de points ×0,26, mais forêts ×0,925 seulement) ; les constantes du pas (×0,966) ; les parents
DSU denses O2 (×0,904) ; le tri des cohortes par tranches (naissances ×0,529).

**Ce qui n'a pas marché** : les mémos de lane (2,6 % de succès) ; les lots ordre par ordre ; les annonces tous les
1 024 plateaux, le préchargement des graines, les préchargements combinés (retirés ; aucun effet établi au niveau de
l'étage, § 7.8).

### 7.5 Le constat central : la conception d'origine n'a pas été implantée

Le 2 octobre au matin, avant toute ligne de moteur, trois documents de conception ont été écrits
(`build/v11-persist/conception/`, **jamais versionnés** avant cet audit) : `CONCEPTION_GENERATEUR.md`,
`CONCEPTION_TOUR.md` et `PISTES_DE_RUPTURE.md` (seul ce dernier a été versé le 7 octobre). Ils prévoyaient :

| Décision de conception | Fondement à l'époque | Ce que la v11 a fait à la place |
| --- | --- | --- |
| D-G3 : plus petite boule proposée en flottant (non décidante), puis certifiée par le catalogue ($S^{*}\subseteq F\subseteq P_b$, lemme T1, sans arithmétique pour 76 % des boules), repli exact | lemme T1 prouvé ; part de 76 % mesurée sur la v10 | énumération exacte exhaustive de tous les supports |
| D-G4 : recensement borné aux $k$ plus proches | lemme 3 de L02 ; boule fermée v10 jusqu'à 1 258 sites | census sur l'index avec bornes exactes, arrêt au seuil $k$ dans l'ordre de Morton |
| D-G1 : résolution en fonction pure, arrêt à la première cellule de fenêtre, pointeurs suivis après coup | lemme T3 | descentes complètes, sans mémo de cellule |
| D-F1 : forêt sans lots, noyau union-find par taille et événements binaires, un fil par ordre recouvert par la résolution, puis contraction parallèle des plateaux (théorème T4) | prototype ×2,5 à 2,9 plus rapide que le Kruskal par lots de la v10, forêts identiques sur six ordres réels | publieurs par plateau dans un pipeline futex |
| D-F3 : requêtes d'ancêtre par historique d'attache | lemme T5 ; 1,3 à 1,7 saut mesuré | balayage DSU suivi des ancêtres |
| D-V1 : image d'une naissance en $O(1)$ depuis la jonction de la même boule | lemme T6 ; 0 écart sur 2,4 M jonctions | suiveurs verticaux collés aux publieurs |
| Feuille par étages sur lots, noyaux vectoriels, arithmétique binary64 exacte par paliers, filtres F6 semi-statiques | sélectivités mesurées ; 12 replis sur 1,86 M triangles | parcours en profondeur des cliques sur CPU ; un fil par feuille sur GPU |
| Émission de 16 octets, catalogue résident de 33 octets par boule, tri par seaux | mesures privées | émissions de 104 octets copiées deux fois |

Ses estimations — 18 à 30 ms pour le catalogue et 22 à 32 ms pour la tour à K5 sur G4 — **n'ont jamais été mesurées**.
L'implantation a dérivé vers un autre moteur. Une partie de l'écart est assumée dans `PROVENANCE.md` (« ni les masques
de triplets, ni le mémo partagé par cellule, ni les filtres flottants des MEB de la v10 ne sont repris ») ; l'abandon
des décisions D-G3, D-F1, D-F3 et D-V1 n'est documenté nulle part. Le résultat mesuré est trois à huit fois plus lent
que ces cibles au total (catalogue 138–200 ms contre 18–30 ; tour 91–116 ms contre 22–32). **Pour la v12, ces trois
documents sont la meilleure source de conception disponible**, à condition de traiter leurs chiffres comme des
hypothèses à mesurer d'abord par microbancs.

### 7.6 Les sorties

Un socle solide : dossier transactionnel (`D.pending`, manifeste écrit en dernier, publication par
`renameat2(RENAME_NOREPLACE)`), codes 0, 2 et 3, signature `tree_k_sha256` commune. Mais :
- `points` et `plat` passent encore par la voie lente `build_order`, pas par la voie L2b des supports ;
- aucune des trois sorties dérivées n'a de coût mesuré sur G4 ; l'écriture de `full` coûte 2,16 s à K5 (SHA-256
  logiciel sériel, tampon de 4 Kio au lieu des 64 Kio annoncés) ;
- **SPv2 n'est pas équivariante par translation** : $S^{*}$ et l'ordre de Kruskal suivent les rangs de Morton (triangle
  $(0,1,1),(1,0,1),(1,1,0)$ à K1 : les arêtes gardées changent avec la translation) ; et sur le cercle, $S^{*}$ saute
  d'un diamètre à l'autre, un saut plus grand que celui du porteur v1 qu'elle a remplacé. `MATHEMATIQUES.md` § 10.10 et
  `SORTIES.md` § 10 surdéclarent ;
- les exports dont Zoltan a besoin (`coverage_v1`, `weighted_gabriel_v1`) et l'exportateur vers la bibliothèque
  produit (`CertifiedTowerInput`) n'existent pas.

### 7.7 Tests, qualification, exécution locale

**Le harnais** est l'acquis d'ingénierie le plus réutilisable : portes à code exact (0 conforme, 1 désaccord, 2 refus,
3 invariant violé, 4 mutant tué ; un signal est un échec ; `add_test` direct, `PASS_REGULAR_EXPRESSION`, `WILL_FAIL`
interdits), planchers contre le vert par vacuité, mutants causaux appliqués à une copie, matrice G4 qui classe
`vacuous`, `incomplete` ou `floor_violated`.

**L'état réel de la qualification au gel.** La « qualification complète » de `98a009550` (3 695 portes, 485/485
mutants) est l'union de douze sessions sur deux commits. La dernière matrice Release, échelle et LiDAR date de
`38faaf272` (6 octobre), y compris SPv2 aux trois profils. Depuis, seules les portes ordinaires sous sanitizers ont
tourné : **le code exact du gel n'a passé aucune matrice G4**. La campagne de mutants force le profil u18 alors que le
produit est en u21 ; 17 des 45 mutants ajoutés depuis n'ont jamais été joués sur G4 ; `--check` ne vérifie que la forme
du nom d'une porte. Les portes du différentiel v10 ne s'enregistrent que si `MHGP11_V10_FROZEN_DIR` est défini, ce
qu'aucune matrice ne fait : elles ne sont ni jouées ni déclarées absentes. L'oracle borné, lui, a passé sa suite
complète sur G4 (5 617 nuages, 38 633 ordres, 3,4 M coupes).

**Vérification par exécution, pour cet audit** (codespace, 8 cœurs, sans GCP) : voir § 7.10.

### 7.8 Mesure et protocole

**Ce qui était bon** : l'identité du vidage exigée à chaque prise ; la règle écrite avant les données et jamais
réécrite (31 `plan.json` hachés au lancement) ; le bras A/A ; G4 seul juge des temps (le local ne prédit pas : pages de
2 Mio −17 % en local, +6,4 % sur G4 ; V3 instructions ×0,565, temps ×0,925).

**Ce qui était mal réglé, chiffré à partir des données brutes** :
- Le bruit « ±9 à 12 % » est celui d'un seul rapport de médianes. La statistique de décision (moyenne géométrique de six
  rapports) a un écart-type de log d'environ 1,0 à 1,2 % sur les étages (onze A/A naturels). Le défaut n'était pas la
  puissance, mais **des seuils placés 2 à 6 fois au-delà de l'effet** (calés sur les gains locaux).
- Des cinq leviers retirés, **un seul l'a été clairement à tort** : le filtre G1 en AVX2 (phase −10,5 %, `domain`
  −3,6 % avec p = 0,005, mur −1,7 % avec p = 0,03). La frontière par tranches a un effet réel d'environ 3 ms ; le
  préchargement des graines agit sur sa phase mais pas sur l'étage ; les préchargements combinés et les annonces ne
  montrent aucun effet établi. « Retirés de justesse » (passation § 4) est donc faux pour deux d'entre eux.
- À chaud, un seul processus par bras dans un ordre fixe : la variance entre processus vaut 10 à 27 fois celle que
  prédit la dispersion des passes. La première prise GPU paie environ 300 ms d'initialisation et tombe dans le premier
  mode listé.
- Les juges sont des scripts par session, non hachés au lancement ; « à froid » est une médiane haute ; les modes sont
  des masques entiers opaques (un même masque a désigné trois variantes le même jour).

**Protocole chiffré pour la v12** (σ des log-rapports appariés tirés des données) : unité de réplication = le processus ;
à froid, K5 : 20 processus par bras, par trame et par voie (effet minimal détectable de 1,3 à 2,5 % sur le mur, pour
environ cinq minutes de G4) ; à chaud, au moins cinq processus par bras et par trame, en ordre entrelacé ; log-rapports
appariés, intervalle par bootstrap stratifié ou permutation ; adoption si la borne haute sur l'étage visé est sous 1,
non-infériorité de 1 % sur le mur ; bras A/A dans chaque session ; leviers algorithmiques à un fil ou sur compteurs ;
petits gains cumulés en lot, jugés en bloc puis par ablation ; juge unique en bibliothèque, haché au lancement.

### 7.9 Exploitation et hygiène

- **Sessions G4** : 138 paquets de session, 132 arrêts ciblés certifiés (dont 2 par `--recover`), 4 échecs de
  démarrage par épuisement de capacité. Le script gardé est mûr ; sa reprise ne rapatrie rien.
- **Reçus** : 357 Mo (et non 190), dont 228 Mo d'archives de paquets et 2 041 copies de sources C++ ; la moitié vient
  des reçus d'audit. L'identité du compte GCP figure dans 528 fichiers (143 adresses, plus des chemins dérivés du nom du
  compte) et dans 91 archives sur 92.
- **Données KITTI** : un reçu de la v8 suivi par Git (`morsehgp3D_v8/receipts/q34_spatial_20260921/gcp_package_r1/snapshot.tar.gz`)
  contient un scan brut (`data/scan000000/RAW.bin`, 1,97 Mo) et des coordonnées dérivées, contre la règle « aucun octet
  KITTI dans le dépôt ». Purger l'historique est une décision de l'utilisateur.
- **Documents de cadrage en retard** : `CLAUDE.md` annonce la v11 en u18 (u21 en réalité), n'a aucune section v10,
  garde des cadres v4, v5 et v9 « actifs » ; il attribue au § 9.1 de la thèse un « seuil relatif » que la thèse ne
  prescrit pas (p. 97 : seuil absolu `min_cluster_size` sur la masse $m_\tau$) ; `AGENTS.md` garde le contrat float32
  du 21 septembre. Le registre formel est figé en phase 15 depuis le 8 août ; la bibliothèque produit `morsehgp3d/`
  n'a aucun producteur depuis le 9 août, mais la CI principale la qualifie encore.

### 7.10 Vérification par exécution locale

Faite pour cet audit sur le codespace (AMD EPYC 7763, 4 cœurs × 2 SMT, 31 Gio ; GCC 13.3, CMake 3.28, Python 3.12),
sur le moteur exact du gel (`git diff ac081a06f HEAD` vide sur `src/`, `cli/`, `cmake/`, `tests/`, `reference/`). Les
données sont les trames préparées hors dépôt (`build/v11-full-data-20261002/`, empreintes conformes à leur manifeste).
**GCP non utilisé.** Journaux et empreintes : `receipts/audit_geant_v11_20261007/verification_locale/`.

| Contrôle | Résultat |
| --- | --- |
| Construction Release u21 | 206 s à 6 fils, **0 avertissement** sous `-Werror`, 1 087 portes enregistrées ; CUDA non détecté |
| `ctest -LE long` (commande documentée) | 1 038 portes : 976 réussies, **60 sautées en silence** (portes LiDAR, faute de `MHGP11_DATA_DIR`), 2 délais dépassés (`mhgp11_tower_full_campaign` et sa jumelle `_opt`, à 120 s) ; 43 min de mur |
| Les deux délais | dus à la charge des autres processus (charge 14 à 28 sur 8 fils) : rejouées seules, **réussies** en 77,9 et 77,7 s ; le délai de 120 s n'a qu'une marge d'environ 1,5 hors G4 |
| Portes LiDAR, avec `MHGP11_DATA_DIR` | **60/60 réussies** (28 min) |
| Différentiel contre la v10 figée (`c764e121a`, reconstruite depuis l'archive épinglée) | **26/26 réussies** : 190 nuages, 640 tours, 26 530 lignes identiques à l'octet ; 10 mutants de sérialisation tués. Ces portes sont absentes par défaut |
| Empreintes de référence (§ 3.4 de l'audit final) | **9/9 reproduites à l'octet**, dont les trois K10 (par la voie CPU, feuilles 24, et par le CLI, feuilles 16 : le résultat ne dépend ni de la voie ni de la taille de feuille) ; reconfirmées sur la dernière de 10 passes à chaud |
| « Uniforme u18 » | désigne la **donnée** : les empreintes gravées viennent du moteur u21 ; le moteur u18 rend d'autres octets mais la **même empreinte sémantique** (lecteur strict), comme u24 : les trois profils calculent le même objet |
| Exécutable `mhgp11`, ng00 K5 | `--sortie=full` rend le même fichier que la sonde (`cmp`) et le même manifeste que G4 le 5 octobre ; `supports`, `points` et `plat` rendent le code 0, les comptes gravés et le même `tree_k_sha256` que sur G4 |
| Oracle Python sous `python3 -S` et `-S -O` | toutes les lignes gravées retrouvées (342 nuages, 1 362 ordres, 48 234 coupes ; S1 : 210 nuages, 951 ordres) |
| Python nu | aucune porte non `long` ne dépend d'un paquet tiers ; numpy et sklearn n'apparaissent que dans les différentiels S9 et S10 (`long`) |
| Contrôles racine | `tools/check_docs.py` **échoue au HEAD** (213 liens morts, tous dans `morsehgp3D_v10/receipts/`, aucun dans la v11, que ce contrôle n'examine d'ailleurs pas) ; `check_polyhedron_order_k_counterexamples.py` réussit |

**Conclusion.** Les 1 038 portes non longues du profil u21 sont conformes au HEAD, à deux conditions (fournir les
données LiDAR ; rejouer seules les deux portes de campagne), et le moteur gelé reproduit exactement ses empreintes de
référence. Les portes `long` (dont les 530 mutants), les sanitizers et les profils u18/u24 complets n'ont pas été
rejoués ici. Repères locaux, sans valeur de décision : FULL K5 sur ng00 en 2,4 s à chaud à 8 fils (environ 7,5 fois
G4) ; `mhgp11 --sortie=full` à K10 en 31,8 s, dont 10,4 s d'écriture pour 1,46 Go.

**Dette révélée par l'exécution** : les portes LiDAR se sautent en silence sans la variable d'environnement ; les données
et l'archive v10 vivent hors dépôt ; aucune porte ne grave les empreintes FULL de référence ; une porte lit
`receipts/` ; la suite dite rapide prend 43 minutes en local ; l'outillage local diffère de celui de la VM (un vert
local n'est pas un vert G4).

## 8. Corrections à l'audit final et à la passation du 7 octobre au matin

Ces documents restent des sources ; ils ne sont pas réécrits. Les corrections ci-dessous sont vérifiées dans le code,
les reçus, les rapports bruts ou par une exécution ; la lecture qui les établit est indiquée entre crochets.

**Mathématiques et objet**
1. La suffisance de Kruskal **n'est pas inscrite** au registre des preuves (audit final § 4.2) ; la ligne du lemme E y
   est collée à celle du témoin D2 par un `\n` littéral (registre, l. 1370), que `tools/check_docs.py` ne détecte pas.
   [A, B]
2. Le « modèle par copies » des multiplicités est **implanté et jugé dans l'oracle borné** (étage A sur les copies,
   étage B par le quotient de Gordan, B = A sur 34 nuages) ; ce qui manque est la preuve dans le contrat du moteur et le
   port natif (passation § 7 : « ni relu ni implanté »). [A, B]
3. Le Théorème 5 de la thèse n'« hérite » pas seulement du contre-exemple à la Prop. 6 : E5 est en position générale
   au sens de la Déf. 26, il le réfute **directement**. Le contre-exemple plan L02 (graphe de Gabriel déconnecté pour
   toujours) n'est pas gravé dans le dépôt. [A]
4. Il n'y a pas de `tests/fixtures/` dans la v11 (passation § 3) : les témoins sont écrits en ligne dans les tests ; seuls
   E5 et le polyèdre sont gravés à la racine. Le témoin « 9 sites pour 80 feuilles » n'existe que comme modèle dans un
   reçu. [B]
5. « Garde i64 du test cubique dépassée dès u21 » : trou de couverture d'une porte, pas défaut du produit, qui élargissait
   déjà. « LevelSource » et « racine $2^{127}-1$ » sont un seul et même défaut. [B]
6. SPv2 (`supports` de Kruskal) **n'est pas le remède à l'instabilité** du porteur v1 : son $S^{*}$ saute davantage sur
   le cercle, et la sortie n'est pas équivariante par translation. [B, E]
7. Le juge J2 (ordre un contre l'arbre couvrant euclidien minimal) est énoncé mais **jamais implanté**. [B]

**Catalogue, GPU, tour**
8. **La « feuille J3 complète » n'a jamais été dans le moteur v10** ; ce n'est pas un mécanisme de vitesse de la v10
   (passation § 3, audit final § 14.2). [C, F]
9. **La partition T > 0 n'agit pas sur le LiDAR** (ablation T = 6/3/0 : vidages identiques, ±0,3 % de travail) ; elle
   concerne les boîtes du catalogue, pas les descentes. [C, D, F]
10. Les « extrema q2 couplés » sont un levier du census des descentes (1 à 2 ms estimés), pas du catalogue ; le
    « repli U2 » est une recette des auditeurs v11, pas un mécanisme v10. [C]
11. La « condition U2 du plan GPU » ne fixe pas 40–50 ms au `domain`, mais « sous 55 ms ». [C]
12. **Cause principale de l'écart des descentes à K10** : le coût du pas (×3 à ×4), d'abord la MEB exacte exhaustive ;
    pas les $k$ plus proches (+3 % de MEB), ni la partition T6, ni seulement les 29 voies (×1,2 à 1,3). La v10 proposait
    la MEB en flottant et la certifiait ; c'est le mécanisme v10 le plus important à porter, absent de la passation. [D]
13. Les « précédents négatifs » de la forêt parallèle sont faibles (mesure locale d'un Borůvka à étiquette minimale ;
    couture d'un découpage spatial) ; le précédent positif (noyau D-F1, ×2,6 sur le Kruskal v10, forêts identiques) est
    omis. Le registre contient déjà le Borůvka sur l'expansion étoilée (`proved_here`, l. 237) et le piège de la
    contraction d'hyperarête entière (`false_in_general`, l. 238). [D]
14. La formule du workspace de feuille est fausse dans **deux** documents (`docs/CATALOGUE.md:158`,
    `docs/CATALOGUE_PARALLELE.md:50`) ; le code réserve bien $16C\lceil C/64\rceil$ octets. « Coupe médiane » est une
    bissection au milieu ; « 13 feuilles par site » est un rapport feuilles/sites, le recouvrement réel est d'environ 124.
    [C]
15. Le préambule des forêts vaut 61 à 78 ms à K10 (omis). Le publieur de l'ordre K « balaie » un tableau d'un octet par
    boule, pas les fiches de boules. [D]

**Sorties et comparaison à HDBSCAN**
16. $H^{r}_{K+1}$ **tient** 70 des 125 cibles de l'utilisateur et en perd 55 (l'audit final dit « perdues : 70 »). [E,
    vérifié]
17. « 68,5 % contre 56,6 % » : population conditionnée au succès de la hiérarchie HGP, 74 % de découpes de voitures,
    écart concentré à k = 2 et 3 ; à k = 10, 0,779 contre 0,765. [E]
18. « Trois vélos sauvés » : deux vélos physiques ; les démos 01 à 04 restent toutes « les deux échouent ». [E]
19. « 13 gains / 3 pertes » : règle « il existe un k » avec priorité au gain ; par ordre 4/2, 5/2, 10/1, 7/0. [E]
20. Seuil de condensation : seul `CLAUDE.md` (l. 134) impose un seuil relatif, et l'attribue à tort au § 9.1 de la
    thèse ; la doctrine Zoltan est neutre ; la thèse prescrit un seuil **absolu** sur la masse $m_\tau$ (p. 97). Et la
    masse du § 9.1 est en tension avec la fixture des deux triangles (chaque triangle pèse moins de 3). [A, E]
21. `points` et `plat` utilisent `build_order` (la voie lente), pas L2b. [E]

**Chiffres, qualification, exploitation**
22. Le jalon de 200 ms a été **franchi une fois** : 196,8 ms à chaud sur ng01 (réglage glibc `@tas`). [H, vérifié]
23. « À froid » est une médiane haute (4e de 6) ; à K10, trois processus et non six ; la référence v10 est une seule passe,
    hors préparation, en u18. [H]
24. Les empreintes K10 ne sont pas dans `records.json` (premières dans `claudegpu3`, 4 octobre) ; l'empreinte dite
    « uniforme u18 » est produite par le moteur **u21** sur ces nuages (le moteur u18 rend une autre empreinte). [G, H]
25. Identité CPU/GPU : 1 602 prises à froid et 378 processus à chaud en voie GPU, 0 écart (et non « 372 / 84 »). [H]
26. SPv2 est qualifiée en u18, u21 et u24 et sous ASan u24 (à `38faaf272`), pas seulement en Release u21 ; mais **le code
    exact du gel n'a passé aucune matrice**, et 17 mutants n'ont jamais été joués sur G4. [H]
27. Leviers retirés : seul G1 AVX2 est une perte claire ; les préchargements combinés et les annonces n'ont aucun effet
    établi ; le défaut du protocole tient aux seuils, pas à la puissance. [H]
28. Sessions : 138 paquets, 132 arrêts certifiés, 4 échecs de démarrage (et non 126 / 124 / 2). Reçus : 357 Mo (et non
    190). Identité du compte : 528 fichiers et 91 archives (et non 143). Environ 100 commits d'auditeurs ; quatre
    commits portent l'auteur `Claude`. Dérive entre sessions le 7 octobre : ±1 à 3,5 %, pas ±5 à 10 %. [H]
29. Six commits sensibles n'ont pas été relus, pas deux : `13a4a0a4c`, `2045ec27c`, `5734ca6e8`, `ccdd4db75`,
    `0ff64512a`, `c80c12012`. Le cache de blocs (`ccdd4db75`) porte un **risque moyen sur le contrat mémoire** : blocs
    inactifs et arrondi de classe hors du budget compté, jamais testés sous limite finie ; et l'exécuteur partagé fait
    piloter le même budget par deux fils, contre le contrat « un seul pilote ». [C, H]
30. La conception d'origine de la v11 (`CONCEPTION_TOUR.md`, `CONCEPTION_GENERATEUR.md`) et l'audit de la v10
    (L01–L10) n'étaient pas versionnés ; ils le sont avec cet audit. [D, F]

## 9. Pour une v12 propre, simple et efficace

Demande de l'utilisateur (7 octobre) : « Le but est de faire une v12 aussi propre, simple et efficace que possible. »
Cette partie tire les conséquences de l'audit ; le dossier [`morsehgp3D_v12/`](../../morsehgp3D_v12/README.md), créé le
même jour, les développe (décisions, contrat à porter, architecture, plan, mesure, provenance). Tout ce qui suit est une proposition **[I]**, fondée sur les constats
ci-dessus ; les chiffres d'objectif sont des hypothèses à confirmer par microbancs avant tout port.

### 9.1 Le diagnostic en trois phrases

1. **L'objet, ses preuves et son outillage de vérité sont acquis** : il n'y a rien à réinventer en mathématiques, et
   presque rien dans la doctrine numérique ni dans le harnais.
2. **La lenteur ne vient pas du volume de travail, mais du coût par unité de travail et de chemins séquentiels par
   construction** : plus petite boule calculée par énumération exhaustive, catalogue et forêts en série, un publieur
   séquentiel par ordre, un GPU employé comme un fil de plus.
3. **La v11 avait, le 2 octobre au matin, une conception plus rapide sur le papier, en partie prototypée (noyau de
   forêt ×2,6 sur le Kruskal de la v10, forêts identiques), et ne l'a pas implantée.** Les seuls gains de vitesse de
   toute la lignée sont venus de changements d'algorithme, jamais d'une réécriture plus propre.

Une v12 « à neuf » ne doit donc pas être une sixième reprise de propreté. **Elle doit déclarer d'avance les changements
d'algorithme qu'elle apporte, porter explicitement tout le reste, et mesurer chaque changement contre la v11 dans la
même session.**

### 9.2 Décisions à obtenir avant tout code

| # | Décision | Recommandation de cet audit | Pourquoi |
| --- | --- | --- | --- |
| 1 | Régime : processus neuf ou Session résidente | **Session résidente**, temps à chaud contractuel ; temps à froid publié à côté | 10 Hz est un flux ; le contexte CUDA (75–150 ms) et les arènes ne se paient qu'une fois |
| 2 | Latence ou cadence | **latence par trame** comme contrat ; cadence par recouvrement de deux trames comme repli déclaré | la latence est plus exigeante et plus simple à mesurer ; le recouvrement catalogue GPU / tour CPU borne la cadence par le plus lent des deux étages |
| 3 | Périmètre des 100 ms | **FULL K1..5 en mémoire, verticales comprises** ; écriture et sorties dérivées mesurées à part | le produit utile coûte aujourd'hui plus que la tour (écriture de `full` : 2,16 s) |
| 4 | K = 10 | **objectif**, cible 0,3 à 0,5 s | aucune estimation ne place la tour K10 sous 100 ms en CPU |
| 5 | GPU | **dans le chemin contractuel**, pour le catalogue | un catalogue CPU seul ne descend pas sous 140–170 ms à K5 |
| 6 | Profil | **u21 seul** ; u24 en matrice ; u18 abandonné | trois profils ont multiplié les défauts de qualification pour 5 % de vitesse |
| 7 | Plage et données | trames de **plusieurs séquences** (00–10), plage à redéfinir (89 des 132 trames sans sol de la séquence 08 dépassent 60 000 sites ; médiane 68 049) ; maximum ou médiane à fixer | trois trames d'une séquence ne valident aucune décision de vitesse |
| 8 | Multiplicités | refus explicite, compteur publié de bout en bout, aucun dédoublonnage silencieux en préparation | zéro doublon au millimètre sur les trames ; le modèle par copies attend sa preuve |
| 9 | Seuil de condensation | **absolu par défaut** (thèse, p. 97), relatif comme bras déclaré ; amender `CLAUDE.md` | rejet structurel du seuil relatif (vélo contre mur) ; la thèse ne prescrit pas le relatif |
| 10 | Polyèdres d'ordre k | **en aval**, à la demande, hors des 100 ms | 1 000 faces par site ; touche l'invariant « pas de mosaïque d'ordre supérieur » |
| 11 | Application prioritaire | à dire : segmentation en ligne (latence) ou modèle de fondation (débit, stockage, exports Zoltan) | elle fixe l'ordre des vues à livrer |
| 12 | Hygiène | décider du sort du scan KITTI versionné dans la v8 et de l'identité du compte dans 528 fichiers de reçus ; archiver ou réactiver `morsehgp3d/` | décisions de l'utilisateur |
| 13 | Ouverture | mettre à jour `AGENTS.md` et `CLAUDE.md` (cadre v12, u21, décisions v10–v11, cadres obsolètes retirés) | la v10 n'y a jamais été déclarée ; la v11 y est en u18 |

### 9.3 Ce qui change d'algorithme (à déclarer d'avance)

1. **Catalogue résident sur le GPU et en flux.** Contexte, flux, pool et mémoire épinglée appartiennent à la Session ;
   parcours des boîtes en largeur sur l'appareil (filtre G1 par couple nœud–site en i64 natif) ; file de feuilles
   consommée pendant le parcours par une **feuille data-parallèle** (phases J3 ou forme « cohérente », choisie par
   microbanc) écrite en source unique, compilée en SIMD sur l'hôte et en CUDA sur l'appareil ; arènes proportionnelles
   aux émissions ; tri radix sur clés F3, chaînes non certainement ordonnées résolues en exact, rangs, CSR et table
   $S^{*}$ → boule sur l'appareil ; rapatriement compact ; niveaux exacts matérialisés à la demande. Le contrat R7 (exact
   sur l'appareil ou `unresolved` rejoué en exact avant admission) est porté tel quel, avec un repli parallèle.
2. **Plus petite boule proposée puis certifiée** (D-G3 de la conception, mécanisme de la v10) : proposition en flottant
   qui ne décide rien ; certificat combinatoire par le catalogue quand le support proposé est le $S^{*}$ d'une boule
   $b$ et $F\subseteq P_b$ (lemme T1, aucune arithmétique) ; sinon certificat exact du support proposé ; repli exact.
   L'énumération exhaustive reste dans l'oracle.
3. **Forêt sans lots** (D-F1, D-F3, D-V1) : un noyau union-find par taille et par ordre, recouvert par la résolution ;
   contraction parallèle des plateaux (théorème T4, à inscrire au registre) ; requêtes d'ancêtre par historique
   d'attache ; image $O(1)$ des naissances. Le pipeline futex des publieurs et des suiveurs disparaît. La forêt comme
   arbre couvrant minimal parallèle n'est qu'un second recours, si la résolution passe sous le temps du noyau.
4. **Un registre d'événements par ordre, dont toutes les sorties sont des vues** : nœuds (rang exact, parent, genre,
   référence de la boule de naissance), hyperarêtes de Kruskal ($\mathrm{ant}(b)$), verticales, vies rapportées à
   $\delta$, pendaisons de points, et les incidences coface–facette nécessaires au § 9.1 une fois la famille de faces
   fixée. Vues : `full` compact, squelette (SPv2 avec départage invariant par translation), `points`, `condense`,
   `plat`, `coverage_v1`, `weighted_gabriel_v1`.

### 9.4 Ce qui se porte tel quel (ports explicites, épinglés à `ac081a06f`, requalifiés)

- Le contrat mathématique (`MATHEMATIQUES.md` § 1–8 et § 10), réécrit en un seul document à identifiants uniques et
  préfixés, preuves complètes (T4 énoncé comme le lemme P, T6 avec ses verticales, J3 par le nerf, Kruskal par P.3),
  une ligne de registre par énoncé et par contradiction.
- L'oracle borné à deux étages et son juge, S1, l'oracle d'intervalles, les faits de projection ; étendus aux profils
  u21/u24, à K ≥ 6, aux coquilles de 13 à 24 sites, aux longues descentes.
- La doctrine numérique (`src/num/`) : budgets `constexpr` par expression, trois voies, certificats dans les fabriques,
  `Level` non réduit, F1–F6 avec F3 par expression, clés F3/F4.
- L'index radix V3, la table de populations (rendue compacte et produite par le catalogue), le journal des graines, le
  plateau atomique, la numérotation canonique, les contrôles de naturalité, le réemploi vertical, le tri des cohortes.
- Le harnais (portes à code exact, mutants causaux, planchers), le budget mémoire transactionnel, le dossier de sortie
  atomique, la session G4 gardée (complétée d'une reprise qui rapatrie).
- La hiérarchie $H^{r}_{K+1}$ et la tête plate exacte, comme vues.
- De la v10 : la proposition de MEB (`DWelzl`) et son certificat, le saut vers les $k$ plus proches avec un
  comparateur exact, le mémo de cellule sous forme déterministe (pointeurs datés), la table M(K) des tailles de feuille.

### 9.5 Ce qui rend la v12 plus simple

- **Un seul chemin produit, qui est le chemin mesuré.** Aucun mode à masque d'options ; des variantes nommées
  n'existent que dans les microbancs, hors du produit.
- **Une seule implantation de la feuille**, en source unique, au lieu de `leaf.cpp` et `leaf_device.hpp`.
- **Un seul profil** (u21) dans le produit.
- **Des compteurs logiques indépendants de l'ordre de visite**, écrits avant la forêt parallèle, séparés des
  diagnostics physiques ; plus de 37 registres transactionnels recopiés à chaque pas de descente.
- **Un départage canonique et invariant par translation** partout (ordre lexicographique des coordonnées).
- **Des identifiants uniques** pour les énoncés, les témoins et les leviers (fin des collisions J2, J3, P1, R, V3, U2).
- **Des reçus sans copies d'arbres sources ni identité de compte**, une note vivante d'une page par acteur, un registre
  de constats lisible par une machine.
- **Une qualification dimensionnée** : matrice en lots de 30 minutes au plus, au profil produit, mutants compris ; dette
  de qualification tenue commit par commit ; « sans résultat » jamais vert ; portes conditionnelles déclarées absentes.

### 9.6 La première tranche : mesurer avant de construire

Chaque mesure a sa règle écrite d'avance, jugée sur G4 par le protocole du § 7.8, sur les trois trames et sur de
nouvelles trames d'autres séquences.

| # | Mesure | Règle d'adoption proposée |
| --- | --- | --- |
| M0 | Fermer le différentiel : la v12 reproduit les empreintes `MHGP11FUL1` de la v11 (§ 2.4 de l'audit final ; reproduites localement par cet audit) | identité à l'octet, ou chaque écart expliqué par un témoin exact |
| M1 | Vider les entrées de la v11 gelée : feuilles (sites, boîtes), listes par niveau, parties de descente, graines par cellule | outil hors produit, empreintes gravées |
| M2 | Microbanc de la feuille sur GPU : noyau un-fil de la v11 (témoin), phases J3, forme cohérente | comptage ≤ 1/3 du témoin, identité avec `leaf.cpp` |
| M3 | Microbanc de la plus petite boule proposée et certifiée, sur les parties vidées | CPU de résolution à K10 réduit d'au moins 40 % à un fil, résultats identiques |
| M4 | Microbanc du noyau D-F1 et de la contraction, sur les graines vidées | noyau ≤ 10 ms à K5 et ≤ 35 ms à K10 à un fil ; contraction ≤ 3 ms ; forêts identiques |
| M5 | Microbanc du parcours des boîtes en largeur sur GPU | même ensemble final de feuilles que le CPU |
| M6 | Coût de session : contexte, modules, transferts épinglés, attente bloquante ou active | publié, pour fixer le budget du régime résident |

Seulement ensuite : intégrer, étage par étage, avec le différentiel à chaque tranche. Puis la qualification et la
campagne de mesure sur plusieurs séquences ; puis la comparaison à HDBSCAN, préenregistrée et menée jusqu'au bout,
avec ses trois niveaux séparés, le bras MR$_k$-bord, la même tête sur l'arbre de `sklearn`, et une population
représentative (développement sur les séquences 00–07 et 09–10, bilan sur la 08).

### 9.7 Ce qu'il ne faut pas refaire

- Écrire le moteur avant le contrat, l'oracle et les microbancs ; déclarer l'objet avant la fin de la lecture.
- Une reprise de propreté à algorithme constant ; laisser l'implantation dériver de la conception sans le documenter.
- Greffer le GPU sur un étage piloté par le CPU (contexte par processus, join global, un fil par feuille).
- Juger des leviers un par un avec des seuils calés sur le local ; un seul processus par bras à chaud.
- Laisser le chemin produit différer du chemin mesuré.
- Toute réduction sans attaches silencieuses (E5) ; les substituts rapides d'un autre objet ; les marges flottantes
  figées ; le Kruskal par lots ; les mémos de lane ; une barrière par plateau.
- Un index Git partagé, des reçus qui dépendent de `/tmp` ou de `build/`, un octet de données KITTI dans le dépôt.

## 10. Références

**Rapports bruts de cet audit** : [`receipts/audit_geant_v11_20261007/`](../receipts/audit_geant_v11_20261007/README.md)
(huit rapports, scripts de recalcul et de rejeu, journaux de la vérification locale).

**Sources versées par cet audit** : [`receipts/conception_v11_20261002/`](../receipts/conception_v11_20261002/README.md)
(conceptions du 2 octobre, brouillon mathématique, audit de la v10 en lentilles).

**Objet et preuves.**
- Thèse : `docs/references/MANUSCRIT_THESE_HAUSEUX.pdf` (racine), Parties I–II, § 6.1, § 9.1.
- `docs/SPECIFICATION_MORSEHGP3D.md`, `docs/math/DEFINITION_HGP_3D.md`, `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md`
  (racine).
- [`MATHEMATIQUES.md`](MATHEMATIQUES.md), [`ARCHITECTURE.md`](ARCHITECTURE.md),
  [`CONCEPTION_MOTEUR.md`](CONCEPTION_MOTEUR.md), [`HIERARCHIE_POINTS.md`](HIERARCHIE_POINTS.md),
  [`SORTIE_PLATE.md`](SORTIE_PLATE.md), [`SORTIES.md`](SORTIES.md), `reference/README.md`.
- Fixtures racine : `tests/fixtures/regressions/gabriel_point_set_counterexample.json`,
  `tests/fixtures/regressions/polyhedron_order_k_counterexamples.json`, `tests/fixtures/exact/chapter6_six_points.json`.

**Moteur et mesures.**
- [`CATALOGUE.md`](CATALOGUE.md), [`FULL_FORESTS.md`](FULL_FORESTS.md), [`PERFORMANCE_FULL.md`](PERFORMANCE_FULL.md),
  [`PROVENANCE.md`](PROVENANCE.md), [`AUDIT_V10_SYNTHESE.md`](AUDIT_V10_SYNTHESE.md).
- Reçus : `receipts/developpement_20261007/filtre_g1_avx2/` (temps au gel), `developpement_20261007/diagnostic_domaine/`,
  `developpement_20261007/retention_tas/` (196,8 ms), `developpement_20261004/gpu_g4/` (compteurs de l'ordre 10),
  `developpement_20261005/qualification_finale/`, `developpement_20261006/v3_qualification/`,
  `audit_deep_20261004/performance/context/TABLE_VERITE_G4.md` (référence v10), `pts4_review_20261003/` (tailles des
  trames de la séquence 08).
- Notes de travail : `receipts/notes_hors_depot_20261007/` (plan 100 ms, pistes de rupture, audit des transpositions,
  cartes GPU).

**Applications.** `Zoltan/FoundationModel/` (README, OBJET, SPECIFICATION, MESURE, PLAN), `Zoltan/demos/`.

**Lignée.** `AGENTS.md`, `CLAUDE.md`, les `PISTES_FERMEES.md` (v3, v5, v6) et `FAUSSES_PISTES.md` (v7, v8, v9),
`morsehgp3D_v9/docs/AUDIT_V8_SYNTHESE.md`, `morsehgp3D_v10/receipts/audit_v9_20260928/`,
`docs/research/CONTRAT_50K_BILAN.md`, `docs/archive/abandoned/README.md` (racine).

**Canal d'audit.** [`../audits/`](../audits/README.md) : notes des auditeurs (`AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md`,
`AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md`), questions et réponses du développeur.
