# Support positif, citron et complétude du front q3/q4

21 septembre 2026. Réponse mathématique indépendante à la question de la
tranche 31. Le certificat annoncé est correct. Il dépend de l'arité du
support positif choisi, sans hypothèse sur le nombre de sites de la coquille.
Cette preuve ne constitue pas une qualification du nouveau raccord C++.

## 1. Hypothèses et vocabulaire

Soit un support propre positif $S=\{v_1,\ldots,v_q\}$, $q=3$ ou $4$ :
les sommets sont affinement indépendants, sa circumboule a pour centre $c$
dans l'intérieur relatif de leur enveloppe convexe et pour rayon $R$.
Il existe donc des poids $\lambda_i>0$ tels que
$\sum_i\lambda_i=1$ et $c=\sum_i\lambda_i v_i$.
Tous les $v_i$ sont sur cette sphère. Poser

$$D=\max_{i,j}\|v_i-v_j\|,\qquad e=\{a,b\}\subset S,\quad \|b-a\|=D,\quad m=(a+b)/2.$$

Les égalités de longueur sont autorisées. L'arête propriétaire est la
première parmi les arêtes maximales selon la paire triée d'IDs originaux.
Elle est unique **pour ce support**. Elle n'a pas à être maximale parmi
tous les sites de la coquille $U=X\cap\partial B(c,R)$.

Le fait 12 du [manuscrit](../../../docs/references/MANUSCRIT_THESE_HAUSEUX.pdf),
chapitre 8, page imprimée 85 (page PDF 111), a été relu directement dans
l'extraction textuelle de cette page pendant cet audit. Il indique que le
centre de la plus petite boule appartient à l'enveloppe convexe d'un
support de cardinal au plus quatre en dimension trois. Le manuscrit
identifie ensuite support et frontière sous sa position générale ; cette
identification n'est pas utilisée ici. Les coquilles supplémentaires sont
explicitement autorisées dans le contrat v8
[q3/q4, §2](../../docs/Q3_Q4_OBJETS_ET_STRATEGIE_20260914.md).

## 2. Preuve directe de la borne annoncée

L'identité de variance pondérée donne

$$R^2=\sum_i\lambda_i\|v_i-c\|^2=\sum_{i<j}\lambda_i\lambda_j\|v_i-v_j\|^2\leq\frac{D^2}{2}\left(1-\sum_i\lambda_i^2\right)\leq\frac{q-1}{2q}D^2.$$

La dernière inégalité est $\sum_i\lambda_i^2\geq1/q$.
Comme $a$ et $b$ sont sur la sphère,
$(c-m)\perp(b-a)$ et $R^2=D^2/4+\|c-m\|^2$. Donc

$$\boxed{\|c-m\|^2\leq\frac{q-2}{4q}D^2.}$$

Pour un site arbitraire $z$, écrire $u=z-m$, $v=c-m$ et

$$H=(z-a)\cdot(b-z)=D^2/4-\|u\|^2,\qquad \Xi=\|(z-a)\times(b-z)\|^2=D^2\|u_\perp\|^2,$$

où $u_\perp$ est la projection orthogonale à $b-a$. Sa puissance vaut

$$\Pi_B(z)=\|z-c\|^2-R^2=-H-2u\cdot v\leq-H+2\|u_\perp\|\|v\|\leq-H+\sqrt{\frac{q-2}{q}\Xi}=\boxed{-H+\sqrt{\Xi/\alpha_q}},$$

avec $\alpha_3=3$ et $\alpha_4=2$.
Ainsi $H>0$ et $\alpha_qH^2>\Xi$ certifient $\Pi_B(z)<0$.
L'égalité n'est pas un témoin strict. La preuve ne parcourt pas $U$ et
ne remplace jamais $q$ par $|U|$.

Pour un rectangle $A\times B$, les bornes certifiées
$H_{\min}>0$ et $\alpha_qH_{\min}^2>\Xi_{\max}$ impliquent ce résultat
pour chaque paire du rectangle. Le témoin est universel sur le produit,
avant même sa séparation WSPD. Un témoin universel strict ne peut
appartenir à $A$ ou $B$ : le choisir comme extrémité donnerait $H=0$.

## 3. Seuils et coquille complète

Noter $p=|X\cap\mathring B|$. Une présentation d'arité $q$ est utile
jusqu'à $K$ dans ce contrat si $p+q\leq K+1$, soit

$$p<h_q,\qquad h_q=K+2-q.$$

$h_q$ témoins **distincts**, strictement intérieurs pour toute la famille,
permettent donc de supprimer sa voie : $K-1$ pour q3, $K-2$ pour q4.
La voie q3 existe à partir de $K=2$, q4 à partir de $K=3$ ; les seuils
inactifs ne doivent pas être fabriqués par soustraction entière non signée.
Les crédits ne sont pas ajoutés au census terminal, qui repart de zéro.

Une même boule peut posséder des supports positifs de plusieurs arités.
Si $q_{\min}$ est l'arité minimale, son utilité est
$p+q_{\min}\leq K+1$. La disparition d'une présentation non minimale
ne supprime donc pas nécessairement la boule : sa voie minimale doit
rester disponible. Le raccord q3/q4 seul ne remplace pas la voie q2.
Les sites supplémentaires de $U$ ne sont ni des intérieurs ni une
augmentation obligatoire de l'arité du support. Leur collecte entière
reste nécessaire au catalogue et à FULL.

## 4. Du front à un flux complet de présentations canoniques

Les produits du front partitionnent les paires non ordonnées :
$V\times V$ devient $L\times L$, $L\times R$, $R\times R$ ; un produit
disjoint est partitionné en scindant un facteur. Pour chaque voie, une
paire est donc soit rejetée avec certificat, soit présente dans un unique
rectangle terminal. L'égalité des masses est un contrôle utile, mais la
partition disjointe est la raison de la complétude.

Prendre un support positif admissible $S$ et son arête propriétaire $e$.
Si la voie de $e$ était rejetée, ses $h_q$ témoins seraient tous
strictement intérieurs à la boule de $S$, en contradiction avec $p<h_q$.
$e$ atteint donc un terminal où sa voie reste active. Le consommateur doit
développer toutes ses paires et transmettre ce masque sans condition
« q2 accepté » ou « q3 accepté ».

Pour q3, le support positif est un triangle aigu : son troisième sommet
est une seed admissible appartenant au cover de $e$.
Pour q4, au moins une des faces adjacentes $abx$, $aby$ est aiguë.
Voici une preuve qui couvre les égalités de longueur. Avec les poids du
tétraèdre et le même milieu $m$,

$$\sum_i\lambda_i H(v_i)=D^2/4-(R^2+\|c-m\|^2)=-2\|c-m\|^2<0.$$

La dernière inégalité est stricte car le centre intérieur d'un tétraèdre
propre ne peut être sur son arête $ab$. Comme $H(a)=H(b)=0$, on a
$H(x)<0$ ou $H(y)<0$, exactement l'acuité au sommet opposé à $ab$.
Les deux autres angles de cette face sont strictement aigus : par
maximalité de $ab$,
$(b-a)\cdot(x-a)=(D^2+\|x-a\|^2-\|x-b\|^2)/2>0$, et de même en $b$.
L'arête reste propriétaire dans toute face contenant $ab$.

La seed aiguë canonique, départagée par ID, conduit ainsi à la racine de
la boule ; la quatrième extrémité appartient à son groupe exact. Les
consommateurs peuvent émettre le premier support positif/canonique du
groupe puis s'arrêter : la boule reste représentée, mais **toutes les
incidences de supports ne sont pas promises**. Deux seeds ou supports
différents peuvent encore présenter la même boule. L'oracle du raccord
doit distinguer la couverture des boules admissibles, la convention de
présentation et la collecte de coquille. Comparer seulement le nombre
de supports ne suffit pas.

Ce raisonnement suppose les contrats déjà établis des voies locales :
cover contenant toute boule positive possédée par $e$, seeds exhaustives,
racines coïncidentes regroupées exactement, contacts conservés, profondeur
et coquille globales après certification positive/propriétaire. Il ne
qualifie pas à lui seul leur nouvel assemblage ou ses workers.

## 5. Fixtures u16 discriminantes

Toutes les coordonnées suivantes sont entières et positives. Les valeurs
ci-dessous ont été recalculées exactement, sans flottants.

**q3, contact et coquille plus grande que le support.**
$a=(30,30,30)$, $b=(36,36,30)$, $x=(36,30,36)$.
Le support est équilatéral, $D^2=72$, $c=(34,32,32)$, $R^2=24$,
avec trois poids $1/3$. Ajouter $z=(32,34,28)$ : $\Pi_B(z)=0$,
$H(z)=12$, $\Xi(z)=432=3H(z)^2$.
À $K=2$, la présentation q3 de profondeur zéro est admissible.
Remplacer $>$ par $\geq$ peut la rejeter avec ce faux témoin.
La paire $xz$ a longueur carrée $96>72$ et constitue un support q2
de cette même boule ; $ab$ reste pourtant propriétaire de $\{a,b,x\}$
si les IDs sont attribués dans l'ordre donné.

**q4, contact exactement sur la frontière du citron.**
Prendre les mêmes $a,b$, puis $x=(30,36,24)$, $y=(36,30,24)$.
Le tétraèdre régulier a $D^2=72$, $c=(33,33,27)$, $R^2=27$ et
quatre poids $1/4$. Ajouter $z=(32,32,32)$ :
$\Pi_B(z)=0$, $H(z)=12$, $\Xi(z)=288=2H(z)^2$.
La présentation q4 de profondeur zéro est admissible à $K=3$.
Le test non strict est faux ; réutiliser le coefficient q3, soit
$3H^2>\Xi$, compte également ce site de coquille à tort en q4.

Une coquille de huit sites s'obtient en ajoutant encore
$(30,36,30)$, $(36,30,30)$, $(30,30,24)$ : les huit distances carrées
à $c$ valent $27$. Son diamètre carré vaut $108$, tandis que le support
régulier conserve diamètre carré $72$. Elle contient le support q2
$\{(30,36,30),y\}$. Cette fixture réutilise les coordonnées du contact
isolé de l'audit précédent ; le présent contrôle concerne le front,
pas une nouvelle qualification de l'atlas.

**Les deux hypothèses géométriques sont nécessaires.**

| Hypothèse retirée | Triangle et témoin | Valeurs exactes |
|---|---|---|
| Arête $ab$ maximale | $a=(30,30,30)$, $b=(42,30,30)$, $x=(36,48,30)$ ; $z=(36,27,30)$ | Triangle positif, $c=(36,38,30)$, $R^2=100$ ; $H=27$, $\Xi=1296<3H^2$, mais $\Pi_B(z)=21>0$. $\|ab\|^2=144<360=\|ax\|^2$. |
| Support positif | $a=(30,30,30)$, $b=(46,30,30)$, $x=(38,32,30)$ ; $z=(38,34,30)$ | $ab$ est maximale, mais triangle obtus ; circumcentre $c=(38,15,30)$, $R^2=289$ ; $H=48$, $\Xi=4096<3H^2$, mais $\Pi_B(z)=72>0$. Cette circumboule n'est pas sa plus petite boule englobante. |

Les contre-fixtures ne réfutent pas le citron sous ses hypothèses. Elles
permettent de tuer les mutations qui omettraient propriété ou positivité.

## 6. Coût et suites constructives

Le front peut donc être raccordé aux deux voies sans attendre un nouvel
index de couches. Cette preuve de couverture ne borne ni la masse des
paires résiduelles, ni la somme des préparations de covers, ni le nombre
de seeds ou d'événements. Mesurer ces sommes sur les scans LiDAR complets
et payer séparément la collecte des grandes coquilles est le bon test du
nouveau raccord. Un compteur de rectangles seul ne mesure pas ce travail.
La canonicité par support n'élimine pas les boules dupliquées ; leur
catalogue, les intérieurs et la tour FULL restent des obligations distinctes.

GCP non utilisé pour cette vérification.
