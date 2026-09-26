# Le moteur E-HGP : proposer par descente, certifier en rationnels

> [!IMPORTANT]
> `phase=exploration_ehgp_hors_registre`, `backend=python_reference`,
> `profile=any_dimension_rational_exact`, `mode=audit_independant_math_and_architecture`,
> `public_status=not_claimed`. GCP non utilisé.

## 1. Le principe

MorseHGP3D **énumère** ses sphères critiques : tri de Morton, arbre radix,
WSPD par vagues, trois voies génératrices q2/q3/q4 sur les supports de
cardinal au plus $d+1=4$. Toute cette architecture a une constante
$2^{\Theta(d)}$ et un catalogue $\Theta(n^{d+1})$ : elle ne se transporte
pas.

E-HGP renverse la charge :

| étape | MorseHGP3D | E-HGP |
| --- | --- | --- |
| trouver les points critiques | énumération combinatoire des supports | **descente** (MEB-Lloyd exacte, ou gradient du niveau de Fermi) |
| décider un prédicat | entier exact (i64/i128/U192) | rationnel exact (`fractions.Fraction`) |
| prouver une connexité | flot de Gabriel, cofaces | **segment certifié** à une variable |
| élaguer | fuseaux $W_q$, seuils $h_q$ | encadrement entropique, seuils de masse |
| coût visé | quasi-linéaire | quadratique, cubique toléré |

La doctrine d'exactitude est conservée telle quelle : **le flottant ne
propose, il ne décide jamais**. La descente entropique est un proposeur ; le
certificat est rationnel.

## 2. Les quatre briques

### 2.1 Catalogue critique par descente

`src/ehgp/engine/critical.py`. Itération MEB-Lloyd pour une masse entière
$m$ : prendre les $m$ observations les plus proches de $y$, aller au centre
de leur boule englobante minimale exacte, recommencer. Le théorème C de
[`REGULARISATION_ENTROPIQUE.md`](REGULARISATION_ENTROPIQUE.md) donne la
décroissance de $a_m$, l'arrêt, et l'identification des points fixes :
$m=k$ donne les **naissances** d'ordre $k$ (indice 0), $m=k+1$ les
**fusions** (indice 1).

Coût : $O(nd)$ par pas pour le voisinage, plus une boule englobante de $m$
points ($O(2^m(m^3+md))$ dans l'implémentation de référence, qui énumère les
supports ; un Welzl exact ramènerait ce facteur à une espérance linéaire en
$m$).

**Mesuré** : zéro faux positif (tout point fixe est une sphère critique
exacte du catalogue) ; couverture complète en $d=2$ et $3$, partielle
au-delà, bornée par le nombre de départs.

### 2.2 Segment certifié

`src/ehgp/engine/segment.py`. Le long d'un segment, les $n$ paraboles
$\left\Vert y-x_l\right\Vert^2$ **partagent leur coefficient dominant**, donc
$a_k$ y vaut $At^2$ plus la $k$-ième plus petite de $n$ fonctions affines ;
sur chaque morceau c'est convexe, donc le maximum est atteint en $t=0$,
$t=1$, ou à un croisement de droites (§ 5 de
[`OBJET_ET_DIMENSION.md`](OBJET_ET_DIMENSION.md)).

Conséquence : le plus petit $a$ tel que le segment entier soit dans
$L_k(a)$ se calcule **exactement**, et c'est un **majorant certifié** du
niveau de fusion des deux extrémités. Les extrémités peuvent être des
observations ou des témoins ; le nuage qui définit $a_k$ reste, lui, celui
des observations.

Coût par segment : $O(n^2)$ temps candidats, $O(n\log n)$ par évaluation.
Un balayage événementiel qui ne maintient que les rangs $k-1,k,k+1$ ramène
l'évaluation à $O(1)$ par événement.

### 2.3 Liaison simple sur le graphe des témoins

`src/ehgp/engine/witness_tower.py`. Nœuds = observations et centres
critiques trouvés par descente ; arêtes = segments avec leur niveau certifié ;
liaison simple, puis restriction aux observations. Ajouter des témoins ne
peut que faire **baisser** le majorant.

**Mesuré** (campagne appariée, `bench/witness_campaign.py`) : sur la fixture
du carré de côté 10 à l'ordre 3, le majorant passe de 125 (segments entre
observations seules) à **100**, qui est la valeur exacte — le chemin qui
réalise 100 passe par le centre du carré, qui est précisément une sphère
critique. Campagne appariée complète
(`bench/witness_campaign.py --ns 8 --dims 2,3,5,10,20,50 --k-max 3 --seeds 17,23,31 --families uniform,clusters`,
reçus dans `receipts/witness_campaign_20260925/`) :

| départs de descente | cas | paires | égales à l'exact | strictement majorées | violations |
| --- | --- | --- | --- | --- | --- |
| observations et milieux de paires | 34 | 2856 | 2843 | 13 | **0** |
| plus les barycentres de triplets | 34 | 2856 | **2856** | **0** | **0** |

Avec les barycentres de triplets comme départs supplémentaires, le majorant
certifié **coïncide avec la projection exacte sur la totalité des paires
mesurées**, en dimension 2, 3, 5, 10, 20 et 50, et jamais en dessous. Les 13
cas résiduels de la première ligne étaient donc un manque de **départs** de
descente, non une limite structurelle : deux d'entre eux ont été réduits,
puis fermés par l'ajout des triplets (19 égales et 9 majorées deviennent 28
et 0 ; 24 et 4 deviennent 28 et 0).

**L'égalité reste mesurée, jamais déclarée** : rien ne démontre que la
descente trouve toujours les témoins nécessaires. Ce qui est démontré est la
direction de l'inégalité (un chemin polygonal certifié majore le niveau vrai)
et, pour les paires couvertes par un certificat de séparation (§ 2.4),
l'égalité elle-même.

### 2.4 Certificat de separation : la borne inferieure

`src/ehgp/engine/separation.py`. Les segments donnent un majorant ; il
manquait la direction opposee. Le certificat suivant la fournit, exactement
et sans dependance en dimension.

> **Certificat de tranche.** Soit $H$ une hypersurface qui separe strictement
> $x_i$ de $x_j$. Si au plus $k-1$ observations ont leur boule
> $B(x_l,\sqrt{a})$ qui rencontre $H$, alors $H$ ne rencontre pas $L_k(a)$ :
> tout chemin de $x_i$ a $x_j$ traverse $H$ et sort donc de $L_k(a)$. Le
> niveau de fusion vrai est au moins $a$.

Deux familles d'obstacles sont implementees, toutes deux decidees en
rationnels exacts.

**Hyperplan.** La quantite utile est la $k$-ieme plus petite distance au
carre des observations a l'hyperplan, et le decalage optimal dans une
direction donnee se calcule exactement (la fonction est quadratique par
morceaux, ses ruptures sont les milieux $(t_l+t_m)/2$ des projections).
**Mesure** : le certificat est **correct** — l'encadrement contient la valeur
exacte de l'oracle sur 21 paires sur 21, pour $d=2,3,5,20$ et $k\leq3$ — mais
**faible** : il est serre 15 fois sur 21 en $d=2$ a l'ordre 1, et jamais
au-dela de $d=2$, le rapport majorant sur minorant atteignant une mediane de
$8{,}5$ en $d=20$ a l'ordre 2. C'est attendu : en grande dimension presque
toutes les boules rencontrent un hyperplan donne.

**Sphere.** L'obstacle naturel d'un objet fait de boules est une sphere. La
condition « la boule $B(x_l,\sqrt{a})$ rencontre la sphere de centre $c$ et de
rayon au carre $R$ » s'ecrit **sans racine carree** : avec
$s=\left\Vert x_l-c\right\Vert^2$,

$$(a-s-R)^2\leq4sR\qquad\text{ou}\qquad a>s+R,$$

la premiere inegalite caracterisant
$(\sqrt{s}-\sqrt{R})^2\leq a\leq(\sqrt{s}+\sqrt{R})^2$ et la seconde le cas
ou la boule avale la sphere. Tout est rationnel. Le rayon optimal, en
revanche, vit dans l'espace des rayons et non des rayons au carre : entre
deux coquilles de rayons $u$ et $v$ il vaut $(u+v)/2$, donc
$(s_u+s_v+2\sqrt{s_us_v})/4$, en general irrationnel. Le flottant propose
donc le rayon, le rationnel certifie — et un rayon legerement sous-optimal
donne une borne legerement plus faible, jamais une borne fausse.

**Mesure** ($n=7$, trois graines de nuages uniformes, spheres candidates
centrees sur les observations et sur les centres critiques) : **aucun faux
certificat**, et le niveau de fusion est **certifie exact** pour

| $d$ | $k=1$ | $k=2$ | $k=3$ |
| --- | --- | --- | --- |
| 2 | 9/21 | 0/21 | 4/21 |
| 3 | 7/21 | 0/21 | 0/21 |
| 5 | 5/21 | 0/21 | 0/21 |
| 20 | **21/21** | 0/21 | 0/21 |

À l'ordre 1 et en grande dimension, **toutes** les paires sont certifiées
exactement : ce n'est plus un accord mesuré avec un oracle, c'est une preuve
produite par le moteur lui-meme. Aux ordres supérieurs le certificat reste
trop faible, et la raison est identifiée : le rayon doit éviter la $k$-ieme
coquille, ce que la famille de candidats courante ne cherche pas encore.

### 2.5 Applications verticales gratuites

$L_k(a)\subseteq L_{k-1}(a)$ : la composante d'une observation à l'ordre $k$
s'envoie sur sa composante à l'ordre $k-1$, à la même observation. Sur la
tour projetée, les flèches verticales sont donc immédiates et exactes — alors
qu'elles sont un chantier entier en v4 (« dix forêts sans verticales ne sont
pas une tour »).

## 3. Ce que la tour projetée n'est pas

Il faut être précis sur ce que la projection perd, sinon le chantier
mentirait.

1. **Les composantes sans observation.** Une composante de $L_k(a)$ peut ne
   contenir aucune observation (une lentille entre deux points éloignés, à
   l'ordre 2, n'en contient aucune). La projection ne la voit pas. Elle est
   néanmoins *détectable* : toute composante contient un minimum de $a_k$,
   donc un point fixe de la descente. La tour projetée est donc un
   **quotient** de la tour FULL, et le catalogue des naissances par descente
   est ce qui permettrait de le compléter.
2. **L'égalité avec la projection exacte n'est pas démontrée.** Le moteur
   livre un majorant certifié. La campagne mesure l'écart. Un cas
   strictement majoré n'est pas un bug : c'est un chemin que les témoins
   courants ne réalisent pas.
3. **Le recouvrement des composantes.** Dès $k\geq2$ la sortie n'est pas une
   partition de $X$ : deux composantes peuvent partager des observations. La
   projection par ultramétrique écrase cette information ; la tour FULL
   exacte (`src/ehgp/exact/tower.py`) la conserve et sert de juge.

## 4. Coûts

Pour $n$ observations en dimension $d$, ordre maximal $K$, $W$ témoins :

| brique | coût | exact ? |
| --- | --- | --- |
| distances par paires, niveaux d'entrée | $O(n^2d)$ | oui |
| tour d'ordre 1 (arbre couvrant minimal) | $O(n^2d)$ | oui, **complète** |
| une descente MEB-Lloyd | $O\!\left(\text{pas}\cdot(nd+2^m m^3)\right)$ | oui |
| un segment certifié | $O(n^2\log n)$ | oui |
| tour à témoins, un ordre | $O\!\left((n+W)^2\,n^2\log n\right)$ | majorant certifié |
| tour FULL exacte (oracle) | $\Theta\!\left(\binom{n}{K+1}2^{K}\right)$ | oui, borné à $n\leq14$ |

Le régime visé est donc **cubique à quartique en $n$ et polynomial en $d$**,
conformément au budget accordé au chantier. Aucune de ces briques n'a de
constante exponentielle en $d$.

## 5. La couche statistique : l'estimateur spectral, et sa limite

L'obstruction de signal (§ 2 de
[`OBSTRUCTION_GRANDE_DIMENSION.md`](OBSTRUCTION_GRANDE_DIMENSION.md)) impose
de changer la mesure de proximité. Le comptage doux
$C_{\varepsilon}(y,a)=\sum_ig\!\left((a-e_i(y))/\varepsilon\right)$ est une
estimation à noyau ; remplacer ce noyau par un **modèle de log-densité
estimé** est le seul moyen connu de restaurer un contraste en grande
dimension.

L'estimateur spectral de log-densité relative de Francis Bach fournit
exactement l'objet dont le moteur a besoin :

* il estime $\log(dp/dq)$ en **forme close**, par une décomposition en
  valeurs propres généralisée du couple $(\Sigma_p,\Sigma_q)$ dans un espace
  de descripteurs $\varphi$ de dimension $m$, via la famille de khi-deux
  pondérés $f_{\rho}$ et l'identité intégrale qui reconstruit
  Kullback–Leibler ;
* le potentiel obtenu, $v(x)=\varphi(x)^{\top}M\varphi(x)+2c^{\top}\varphi(x)$,
  est **évaluable partout**, avec gradient analytique en $O(md)$ ;
* le coût est $O(m^2n+m^3)$ en descripteurs explicites, $O(n^3)$ en version
  à noyau — exactement le budget du chantier ;
* **une seule factorisation $O(m^3)$ donne toute la famille $(\rho,\lambda)$
  de potentiels** à $O(m)$ chacun : un axe de lissage gratuit, candidat
  naturel à l'axe d'ordre de la tour ;
* et le fait de segment se transpose : pour des descripteurs
  $\varphi_j(x)=(w_j^{\top}x+b_j)_+^{\kappa}$, la restriction
  $t\mapsto v(\gamma(t))$ est **polynomiale par morceaux de degré $2\kappa$
  sur au plus $m+1$ cellules** dont les ruptures sont explicites : la
  certification exacte de connexité le long d'un segment survit au changement
  de modèle.

**Ce qui bloque, et il faut le dire.** Trois obstacles, dont deux mesurés :

1. « grande dimension » chez Bach ne signifie pas $d$ grand : le taux à
   noyau est $n^{-t/(t+d/2)}$, donc maudit sauf si la régularité $t$ croît
   avec $d$ ; l'échappatoire est la **structure latente de faible
   dimension** ($\log(dp/dq)$ ne dépendant que d'une projection de dimension
   $d_{\mathrm{eff}}$), avec des constantes possiblement exponentielles en
   $d_{\mathrm{eff}}$ ;
2. **mesuré** : avec des descripteurs ReLU aléatoires **fixes** ($m=50$,
   $n=2000$, $p$ bimodal), la corrélation entre le potentiel estimé et le
   vrai log-rapport tombe à $-0{,}04$ en $d=20$, et 40 départs
   d'optimisation atteignent 40 maxima locaux distincts : l'arbre de fusion
   des sur-niveaux serait du **bruit pur**. Seul l'apprentissage de
   descripteurs préserve l'adaptativité, et cet apprentissage n'est pas dans
   le périmètre acquis ;
3. la monotonie verticale $L_{k+1}(a)\subseteq L_k(a)$, qui fait de HGP une
   tour et non une famille de dendrogrammes, doit être **préservée par le
   modèle** : un modèle de densité quelconque ne la garantit pas. C'est une
   contrainte de conception à vérifier, pas un acquis.

**Mesure décisive : la tour régularisée contre la tour empirique.** Mélange
de 4 amas de dimension intrinsèque 2, $n=300$, séparation 8, trois graines,
indice de Rand ajusté à la coupe au vrai nombre de classes
(`bench/spectral_tower.py --dims 2,10,50,200 --intrinsic 2 --n 300 --orders 1,5 --noise-modes none,per_coordinate --seeds 3`,
reçu `receipts/spectral_tower_20260926/`) :

| bruit | $d$ | $k$ | ARI empirique | ARI spectral | écart |
| --- | --- | --- | --- | --- | --- |
| aucun | 2 | 1 | 1,000 | 0,667 | $-0{,}333$ |
| aucun | 2 | 5 | 0,914 | 0,667 | $-0{,}248$ |
| aucun | 10 | 5 | 0,914 | 0,997 | $+0{,}083$ |
| aucun | 50 | 5 | 0,914 | 0,997 | $+0{,}083$ |
| aucun | 200 | 5 | 0,914 | 1,000 | $+0{,}086$ |
| ambiant | 10 | 1 | 0,907 | 0,997 | $+0{,}090$ |
| ambiant | 10 | 5 | 0,105 | 0,997 | $+0{,}892$ |
| ambiant | 50 | 1 | 0,166 | 0,994 | $+0{,}828$ |
| ambiant | 50 | 5 | **0,000** | **0,994** | $+0{,}994$ |
| ambiant | 200 | 1 | $-0{,}000$ | 0,362 | $+0{,}362$ |
| ambiant | 200 | 5 | $-0{,}000$ | 0,362 | $+0{,}362$ |

Trois lectures, dans cet ordre.

1. **Sous bruit ambiant et en dimension moyenne, la voie régularisée fait
   exactement ce qu'on lui demande** : la tour empirique s'effondre
   ($0{,}105$ puis $0{,}000$ aux ordres élevés dès $d=10$), la tour du modèle
   tient ($0{,}997$ puis $0{,}994$). L'écart atteint $+0{,}994$ à $(d=50,k=5)$.
   C'est la justification applicative du changement d'objet.
2. **En dimension 2 elle est moins bonne** ($-0{,}33$) : la régularisation
   coûte de la résolution là où le comptage empirique de boules fonctionne. La
   voie régularisée n'est donc pas un remplacement universel mais un régime.
3. **À $d=200$ les deux tombent** ($0{,}362$ contre $0{,}000$) : mieux, pas
   résolu. Avec des descripteurs fixes, l'effondrement annoncé par la théorie
   finit par arriver ; c'est exactement la limite de l'apprentissage de
   descripteurs annoncée ci-dessus.

**Conclusion de conception, assumée.** La couche statistique de E-HGP n'est
viable que sous une hypothèse explicite de **structure latente de faible
dimension**. Cette hypothèse doit être énoncée, mesurée
([`MESURES_CONCENTRATION_20260925.md`](MESURES_CONCENTRATION_20260925.md)),
et jamais dissimulée dans un choix de descripteurs.

## 5 bis. L'axe d'ordre ne paie pas sur des distances euclidiennes brutes

C'est le résultat le plus dérangeant du chantier, et il est mesuré.

`bench/clustering_compare.py run --profile quick --orders 1,2,5` balaye 48
cellules (toutes les familles synthétiques, $d=2$, $50$, $1000$, bruit de fond
$0$ et $30$ pour cent, deux graines) et compare la tour E-HGP projetée à la
liaison simple, à la *reachability* mutuelle façon HDBSCAN, et à des témoins
non densitaires. Reçu `receipts/clustering_20260926/`.

**Contrôle de cohérence d'abord** : `ehgp_k1`, `single_linkage` et `reach_k1`
donnent des chiffres **identiques** à toutes les dimensions, ce qui confirme
par une troisième voie que l'ordre 1 de la tour EST la liaison simple.

**Verdict, indice de Rand ajusté du meilleur $k>1$ moins celui de $k=1$ :**

| coupe | $d=2$ | $d=50$ | $d=1000$ |
| --- | --- | --- | --- |
| au vrai nombre de classes | $-0{,}060$ | $-0{,}110$ | $-0{,}125$ |
| spontanée | $-0{,}032$ | $-0{,}207$ | $-0{,}154$ |

**L'axe d'ordre $k$ n'apporte rien et nuit**, à toutes les dimensions testées.
Et la famille densitaire entière est dominée par des témoins qui ignorent la
densité :

| méthode | $d=2$ | $d=50$ | $d=1000$ |
| --- | --- | --- | --- |
| `ehgp_k1` = liaison simple | $0{,}543$ | $0{,}352$ | $0{,}366$ |
| `ehgp_k2` | $0{,}482$ | $0{,}242$ | $0{,}241$ |
| `ehgp_k5` | $0{,}437$ | $0{,}241$ | $0{,}232$ |
| Ward | $0{,}669$ | $0{,}791$ | $\mathbf{0{,}861}$ |
| $k$-moyennes | $0{,}665$ | $0{,}699$ | $0{,}749$ |
| DBSCAN à rayon oracle | $\mathbf{0{,}910}$ | $0{,}818$ | $0{,}848$ |

Avec 30 pour cent de bruit de fond et $d\geq50$, **toute** la famille de
liaison tombe à un indice de Rand de $-0{,}02$, c'est-à-dire une partition
dégénérée, tandis que Ward tient $0{,}597$ à $0{,}722$.

**Confirmation à cinq graines.** Le profil `noise`
(`bench/clustering_compare.py run --profile noise --orders 1,2,5`, 180
cellules, $d=10$ et $200$, trois niveaux de bruit de fond, cinq graines,
toutes les familles) donne exactement la même conclusion :

| grandeur | $d=10$ | $d=200$ |
| --- | --- | --- |
| ARI(meilleur $k>1$) moins ARI($k=1$), coupe vraie | $-0{,}083$ | $-0{,}108$ |
| idem, coupe spontanée | $-0{,}126$ | $-0{,}156$ |
| `ehgp_k1` = liaison simple | $0{,}272$ | $0{,}271$ |
| Ward | $0{,}776$ | $\mathbf{0{,}886}$ |
| $k$-moyennes | $0{,}758$ | $0{,}810$ |

Une réserve honnête subsiste : le DBSCAN cité utilise un rayon **oracle**
($0{,}871$ et $0{,}821$), donc il majore ce qu'une méthode réglable
obtiendrait.

**Conséquence, à mettre en face du § 5.** La valeur de la voie E-HGP n'est
pas dans l'axe d'ordre appliqué à des distances euclidiennes brutes — mesuré
négatif, deux fois, par deux bancs indépendants. Elle est dans la
**régularisation de la densité** : c'est la même expérience, avec le modèle
spectral à la place du comptage de boules, qui passe de $0{,}000$ à $0{,}994$
d'indice de Rand sous bruit ambiant. L'ordre $k$ de HGP est un axe de
robustesse **du comptage**, et en grande dimension le comptage n'a plus de
contraste à offrir : c'est le noyau qu'il faut changer, pas $k$.

## 6. Ce qui reste ouvert

1. Une garantie de couverture pour la descente MEB-Lloyd sous hypothèse de
   séparation, ou l'aveu définitif que la couverture est seulement mesurée.
2. Renforcer le certificat de séparation aux ordres $k\geq2$ : chercher le
   rayon de sphère qui évite la $k$-ième coquille, au lieu de balayer des
   lacunes de rayons. Le certificat existe, il est correct et exact (§ 2.4) ;
   ce qui manque est la recherche du bon obstacle.
3. L'apprentissage de descripteurs, avec preservation de la monotonie
   verticale.
4. Le raffinement des chemins (méthode de la corde) pour abaisser les
   majorants résiduels, et sa certification exacte par morceaux.
5. Un contrat de sortie versionné propre à la dimension $d$ : le contrat v2
   du dépôt est verrouillé à $d=3$ et ne doit pas être touché.
