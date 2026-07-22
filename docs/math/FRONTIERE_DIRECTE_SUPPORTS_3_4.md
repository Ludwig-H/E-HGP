# Frontière directe certifiée pour les supports trois et quatre

## 1. Objet et contrainte d'architecture

Pour une taille $m\in\left\lbrace 3,4\right\rbrace$, le but est d'énumérer exactement les supports minimaux bien centrés de rang fermé au plus $s_{\max}$ sans construire la mosaïque de Delaunay d'ordre supérieur, ses cellules, ses cofaces, les $\binom{n}{k}$ facettes de Gamma ni une liste $L$-NN dotée d'un pouvoir d'exclusion. La seule structure globale est le Morton-LBVH; la frontière contient des produits de nœuds de taille constante, les feuilles sont classifiées exactement et chaque produit omis possède un certificat rejouable.

L'oracle exhaustif $n\leq14$ reste la source différentielle. Il ne devient ni une dépendance du producteur, ni un index de candidats.

## 2. Partition canonique des sous-ensembles

Une entrée de frontière est une suite de groupes $(N_i,r_i)$, où les plages Morton des nœuds $N_i$ sont deux à deux disjointes et strictement ordonnées, $r_i\geq1$ et $\sum_i r_i=m$. Elle représente les supports prenant exactement $r_i$ feuilles distinctes dans $N_i$.

La racine de la taille $m$ est l'unique groupe $(R,m)$. Si un groupe $(N,r)$ est scindé en ses enfants $L$ et $D$, il est remplacé par tous les cas admissibles $(L,a),(D,r-a)$ pour $0\leq a\leq r$, en omettant les groupes de multiplicité nulle et les cas dépassant la cardinalité d'un enfant.

Cette subdivision est disjointe et exhaustive : chaque support représenté par le parent possède un unique entier $a=\lvert U\cap L\rvert$. Par induction, chaque sous-ensemble non ordonné de taille $m$ atteint exactement une feuille terminale. Une feuille terminale possède $m$ groupes feuilles de multiplicité un; aucun tuple ordonné ni permutation n'est matérialisé.

Le nombre de supports représentés par une entrée est exactement $\prod_i\binom{\lvert N_i\rvert}{r_i}$. Pour $n$ de l'ordre de dix millions, $\binom{n}{4}$ dépasse 64 bits : ce compte de preuve doit donc être un entier multiprécision ou être remplacé par une identité de partition, jamais tronqué dans `size_t`.

## 3. Certificats homogènes de bon centrage

Soit $U=\left\lbrace p_0,\ldots,p_d\right\rbrace$ avec $d=m-1\in\left\lbrace 2,3\right\rbrace$. Posons $e_i=p_i-p_0$, $G_{ij}=e_i\mathbin{\cdot}e_j$, $h_i=G_{ii}$, $\Delta=\det(G)$ et $M_i=\det(G[i\leftarrow h])$, où $G[i\leftarrow h]$ est obtenue en remplaçant la colonne $i$ par $h$.

Pour un support affine indépendant, $G$ est définie positive et $\Delta>0$. Les coordonnées barycentriques du centre circonscrit vérifient exactement

$$2\Delta\lambda_i=M_i\quad(1\leq i\leq d),\qquad 2\Delta\lambda_0=2\Delta-\sum_{i=1}^{d}M_i.$$

Le support est minimal bien centré si et seulement si ces $m$ numérateurs sont strictement positifs. Cette décision n'utilise ni quotient, ni centre flottant.

Pour un produit de boîtes dyadiques, on évalue $G$, $\Delta$ et les $M_i$ par intervalles rationnels fermés. L'extension naturelle de $+$, $-$ et $\times$ enferme chaque valeur du polynôme par induction syntaxique; le carré d'un intervalle est traité comme le carré d'une même variable, avec borne inférieure nulle lorsque l'intervalle contient zéro. Les dépendances perdues entre expressions ne peuvent qu'élargir les intervalles. Elles peuvent empêcher un prune, mais jamais en fabriquer un.

Deux exclusions universelles en découlent :

- si la borne supérieure de $\Delta$ est au plus zéro, chaque tuple réel du produit est affine dépendant, car un déterminant de Gram réel est toujours positif ou nul;
- si la borne supérieure d'au moins un numérateur barycentrique est au plus zéro, aucun tuple affine indépendant du produit n'est bien centré.

Une boîte ambiguë est subdivisée. Contrairement à une évaluation de quelques coins corrélés, le certificat porte sur l'enveloppe d'intervalle complète de chaque variable; aucune combinaison d'extrémités n'est présentée comme un point tridimensionnel commun lorsqu'elle ne l'est pas.

Pour les triangles, une spécialisation plus serrée est disponible. Le numérateur associé au sommet $p_i$ a le signe de $g_i=(p_j-p_i)\mathbin{\cdot}(p_k-p_i)$. Sur trois intervalles d'un même axe, $(b-a)(c-a)$ est convexe en $a$ et affine séparément en $b$ et $c$; son maximum continu exact est donc atteint parmi les huit triples d'extrémités. La somme des trois maxima axiaux donne une borne exacte de $g_i$ sur le produit 3D. Si l'une des trois bornes est au plus zéro, aucun triangle du produit n'est aigu.

## 4. Certificat universel d'intérieur strict

Pour $y=x-p_0$, définissons le polynôme homogène

$$P_U(x)=\Delta\left\Vert y\right\Vert^2-\sum_{i=1}^{d}M_i\,e_i\mathbin{\cdot}y.$$

Comme $c_U-p_0=\sum_i\frac{M_i}{2\Delta}e_i$ et $\beta(U)=\left\Vert c_U-p_0\right\Vert^2$, on a, pour tout support affine indépendant,

$$P_U(x)=\Delta\left(\left\Vert x-c_U\right\Vert^2-\beta(U)\right).$$

Le signe de $P_U(x)$ est donc exactement le signe de la puissance à la sphère circonscrite. Si l'évaluation par intervalles rationnels d'un produit de boîtes supports et d'une boîte témoin possède une borne supérieure strictement négative, tous les points de la boîte témoin sont strictement intérieurs à toutes les sphères des supports affinement indépendants du produit.

Pour un support fixé, la Hessienne de $x\mapsto P_U(x)$ vaut $2\Delta I$ et est positive semi-définie. Le maximum sur une AABB témoin est donc atteint à l'un de ses huit coins. Le producteur évalue les variables supports par intervalles séparément à chacun de ces coins et intersecte ce majorant avec l'intervalle naturel de la boîte requête; cette amélioration conserve tous les supports possibles et ne remplace pas leurs boîtes par leurs seuls coins.

Les plages témoins acceptées forment une antichaîne canonique, sont deux à deux disjointes et ne recouvrent aucune plage support. Pour une taille $m$, le seuil de rejet de rang est

$$t_m=s_{\max}-m+1.$$

Si l'union de ces plages contient au moins $t_m$ observations, chaque support affine indépendant a au moins $t_m$ points strictement intérieurs et au moins ses $m$ points sur le shell. Son rang fermé est donc au moins $t_m+m=s_{\max}+1$; le produit entier est hors de la fenêtre utile. Une égalité de borne ne compte jamais comme intérieur strict.

## 5. Classification terminale sparse

Une feuille non exclue est recertifiée avec les primitives exactes existantes : dépendance affine, centre circonscrit, barycentriques et réduction éventuelle du support. Un support dépendant, extérieur ou réduit sur sa frontière est résolu sans émission à cette taille.

Pour un support minimal, une requête LBVH de boule fermée agrège un sous-arbre extérieur lorsque sa distance minimale est strictement supérieure au niveau et un sous-arbre intérieur lorsque sa distance maximale est strictement inférieure. Toute égalité descend. La requête conserve au plus $s_{\max}-m$ identifiants intérieurs, compte le shell complet et ne conserve d'un extra-shell que son cardinal exact et son plus petit identifiant hors support.

Le résultat régulier exige un shell exactement égal au support. Un extra-shell pertinent reste un diagnostic de dégénérescence. Les points extérieurs sont comptés sans être matérialisés. Ainsi, une classification terminale n'alloue ni la partition globale du nuage, ni une cellule, ni une liste de cofaces.

## 6. Bornes exactes issues de binary64

Tout binary64 fini s'écrit $z/T$ avec $T=2^{1074}$ et $\lvert z\rvert<2^{2098}$. Écrivons chaque différence $e_i=\widetilde{e}_i/T$, puis $G=\widetilde{G}/T^2$, $h=\widetilde{h}/T^2$, $\Delta=\widetilde{\Delta}/T^{2d}$ et $M_i=\widetilde{M}_i/T^{2d}$. On a $\lvert\widetilde{e}_i\rvert<2^{2099}$ et $\lvert\widetilde{G}_{ij}\rvert<2^{4200}$.

Pour un triangle, on obtient $\lvert\widetilde{\Delta}\rvert,\lvert\widetilde{M}_i\rvert<2^{8401}$. Une coordonnée du centre possède une représentation de numérateur inférieur à $2^{10502}$ et de dénominateur inférieur à $2^{9476}$. L'identité rationnelle et sa forme relevée sont

$$\beta(U)=\frac{\sum_i M_i h_i}{4\Delta}=\frac{\sum_i\widetilde{M}_i\widetilde{h}_i}{4\widetilde{\Delta}T^2}.$$

donne un numérateur de niveau inférieur à $2^{12602}$ et un dénominateur inférieur à $2^{10551}$.

Pour un tétraèdre, le développement à six termes du déterminant donne $\lvert\widetilde{\Delta}\rvert,\lvert\widetilde{M}_i\rvert<2^{12603}$. Une coordonnée du centre possède une représentation de numérateur inférieur à $2^{14704}$ et de dénominateur inférieur à $2^{13678}$; le niveau possède un numérateur inférieur à $2^{16805}$ et un dénominateur inférieur à $2^{14753}$.

La réduction canonique ne peut qu'abaisser ces bornes. En comptant le signe éventuel et le séparateur `/`, les longueurs maximales conservatives d'un champ rationnel canonique sont donc :

| support | coordonnée du centre | niveau carré |
|---|---:|---:|
| triangle | 6 017 octets | 6 973 octets |
| tétraèdre | 8 547 octets | 9 503 octets |

La limite 2 048 démontrée pour les paires ne couvre donc pas les supports trois et quatre. Un futur wire d'arité supérieure doit employer des caps distincts ou une limite individuelle d'au moins 9 503 octets, ainsi qu'un budget agrégé correspondant; augmenter silencieusement le cap du wire pair v1 est interdit.

## 7. Coût et limites honnêtes

Chaque entrée de frontière contient au plus quatre groupes et chaque certificat de rang au plus $t_m\leq9$ plages témoins utiles avant arrêt. La mémoire de travail dépend donc du LBVH, de la frontière explicitement budgetée, des certificats et de la sortie, jamais de $\binom{n}{3}$ ou $\binom{n}{4}$.

Cette propriété mémoire ne constitue pas une borne de temps favorable. Si les intervalles restent ambigus et si peu de supports dépassent le rang, la frontière peut atteindre toutes les feuilles et le travail reste combinatoire. Le jalon CPU certifie l'objet et les exclusions; il ne ferme ni le SLO 50 000 points, ni la voie dix millions. Les prochaines mesures doivent porter sur les nombres de produits, les prunes de bon centrage, les reçus de rang, les feuilles et le pic de frontière. Une croissance défavorable est un no-go pour la borne choisie, pas une raison de remplacer l'exactitude par une liste de voisins excluante.

## 8. Contre-exemples permanents aux coins supports

La fixture `higher-support-aabb-corner-regression-v1` fixe $p_1=(-1,0,0)$, $p_2=(1,0,0)$ et $p_0(t)=(t,2,0)$. Pour $t\in[-2,2]$, les deux triangles aux extrémités ne sont pas bien centrés, mais celui de paramètre $t=0$ est aigu. Le verdict commun des coins de la boîte support n'a donc aucun pouvoir d'exclusion universel.

La même famille avec $t\in[-1/2,1/2]$ et $x=(0,33/16,0)$ possède la puissance exacte

$$\left\Vert x-c(t)\right\Vert^2-\beta(U(t))=\frac{41}{256}-\frac{33}{32}t^2.$$

Elle vaut $-25/256$ aux deux extrémités mais $41/256>0$ au centre. Tester seulement les sphères des coins supports certifierait donc faussement que $x$ est toujours intérieur. Le test C++ exige que l'intervalle polynomial complet reste ambigu, tandis que l'oracle Python recalcule indépendamment les cinq valeurs rationnelles. Toute future borne Bernstein, Taylor, McCormick ou GPU doit conserver cette fixture.

## 9. Induction exacte de reprise

Pour une frontière canonique $F$, notons $\mathcal{U}(e)$ la famille de supports représentée par l'entrée $e$, $c(e)=\lvert\mathcal{U}(e)\rvert$ son cardinal exact et $C(F)=\sum_{e\in F}c(e)$. Aux racines, les familles d'arités trois et quatre sont disjointes et donnent exactement l'univers visé :

$$\bigsqcup_{e\in F_0}\mathcal{U}(e)=\binom{P}{3}\sqcup\binom{P}{4},\qquad C(F_0)=\binom{n}{3}+\binom{n}{4}.$$

Une expansion remplace une entrée $e$ par ses enfants $E(e)$. L'unicité de $a=\lvert U\cap L\rvert$ démontrée en section 2 donne une vraie partition, pas seulement une égalité numérique :

$$\mathcal{U}(e)=\bigsqcup_{f\in E(e)}\mathcal{U}(f),\qquad c(e)=\sum_{f\in E(e)}c(f).$$

Un prune certifié ou une classification feuille retire au contraire $e$ et ajoute exactement $c(e)$ au compte résolu. Une interruption au milieu de l'analyse ne retire rien : le produit actif reste égal au dos de la frontière et sa famille est donc comptée une seule fois. Si $R_j$ est le compte résolu après une transition ancrée et $F_j$ sa frontière, l'induction conserve

$$R_j+C(F_j)=\binom{n}{3}+\binom{n}{4}.$$

Cette identité est nécessaire mais non suffisante à elle seule. Pour $n=4$, cinq copies de la racine tétraédrique ont la même somme cinq que les quatre triangles et l'unique tétraèdre, tout en dupliquant un support et en omettant les quatre autres. Un checksum recalculé, des groupes localement valides et l'égalité précédente ne peuvent donc pas établir la provenance d'une frontière arbitraire.

L'autorité en mémoire est par conséquent une induction d'états et non une propriété isolée du payload. La session construit $F_0$, conserve un unique checkpoint fiable $Q_j$, exige que la source réinjectée lui soit exactement égale, puis recalcule la transition déterministe $T_{b_j}$ pilotée par le budget fiable $b_j$. Le commit autorisé est exclusivement

$$Q_{j+1}=T_{b_j}(Q_j).$$

Le candidat mutable n'est jamais la source de $Q_{j+1}$. Un échec conserve $Q_j$; un retry reproduit le même candidat; un état terminal n'a aucun successeur no-op. Cette règle ferme simultanément les duplications, omissions, raccourcis de curseur et doubles charges, car toute altération doit alors contredire au moins une transition rejouée depuis les racines.

Le checkpoint persiste seulement les groupes, l'étape active, les reçus de nœuds, les comptes et les digests. Les analyses Gram--Cramer et de puissance sont des champs dérivés recalculés. La chaîne de sortie engage la projection minimale d'un prune — produit, raison, comptes et reçus ordonnés — plutôt que le texte potentiellement volumineux de ces analyses. Le rejeu exact compare néanmoins le certificat riche entier. Cette séparation prépare un wire supérieur compact sans confondre engagement de la revendication, recertification géométrique et provenance ancrée.
