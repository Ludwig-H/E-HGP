# Suite au développeur : robustesse, réduction et niveaux du polyèdre

6 octobre 2026. Réponse à
[ee2df0362](../../audits/REPONSE_CLAUDE_POLYEDRE_ORDRE_K_20261006.md),
dans le cadre `exploration_v11_hors_registre / cpu_reference /
quantized_u21_input_only / not_claimed`.
La [première réponse](../audit_hartigan_delaunay_20261006/README.md) reste
figée. Les propositions ci-dessous ne qualifient aucun constructeur natif.
Aucun build, banc LiDAR ou appel GCP.

**Je recommande un premier représentant réduit qui conserve tous les
sommets étiquetés de la mosaïque, avec des effondrements polyédriques
certifiés sur la filtration entière.** Cela donne simultanément
l'homotopie filtrée, la couverture exacte des sites et une borne de forme
par composante. Une réalisation très compacte, ou géométriquement stable
sous toute petite perturbation, ne découle pas automatiquement de ces garanties.

La précision transmise par l'utilisateur est prise en compte : le manuscrit
donne une analogie et un modèle à prolonger ; il n'impose pas une forme
unique de dessin. L'ombre proposée est ainsi acceptable comme représentation
déclarée, avec les garanties et les limites établies au §4.

## 1. Q1 — Trois sens distincts de « robuste »

Utilisons δ pour la perturbation des données, η_v pour l'erreur de
simplification et θ pour le budget relatif. Distinguer la stabilité de la
filtration, la précision numérique de sa construction et la stabilité
géométrique de son dessin.

### 1.1 Déplacement apparié : oui pour la filtration, pas pour la forme à coupe fixe

Si une bijection des sites de P vers P' les déplace d'au plus δ, alors
\|d_k^P−d_k^{P'}\|∞≤δ, d'où

\[
\Omega_k^P(r)\subseteq\Omega_k^{P'}(r+\delta),\qquad
\Omega_k^{P'}(r)\subseteq\Omega_k^P(r+\delta).
\]

Les inclusions composées sont celles de rayon r vers r+2δ. Grâce aux
équivalences naturelles, cela donne le même entrelacement pour les
composantes et les modules d'homologie de A_k ; pour les espaces, on parle
d'entrelacement **à homotopie près**. Ce ne sont pas des inclusions
géométriques entre les mosaïques de P et de P', qui peuvent avoir des
cellules et des combinatoires différentes. Une réduction filtrée certifiée
hérite de cette stabilité topologique, même si son appariement change.

La quantification isotrope au plus proche sur une grille de pas h satisfait
δ≤√3 h/2 pour chaque retour. La valeur √3/2 mm suppose donc un arrondi
au plus proche à 1 mm. **Le regroupement des retours en sites unitaires
peut ensuite changer les comptes de voisins.** Si deux sites distincts
deviennent un seul site, il n'existe plus la bijection exigée ci-dessus.
Conserver les multiplicités donnerait un modèle pondéré de même masse ;
le FULL v11 courant exige des sites unitaires. Publier les fusions et
séparer ces deux modèles, comme le prévoit le contrat d'entrée.

Il n'existe pas de borne générale de Hausdorff du dessin à rayon identique
qui tende vers zéro avec δ. Une composante peut disparaître à sa naissance.
Plus fortement, **la géométrie d'alpha peut sauter sans aucun changement
du type d'homotopie de sa composante** :

- P={(-4,0),(4,0),(1,2)}, k=1, r²=377/16 ;
- pour les nuages (1−t)P et (1+t)P, t>0 assez petit, le premier alpha
  contient le triangle plein, le second seulement ses deux petits côtés ;
- les deux sont connexes et contractiles ; le déplacement apparié maximal
  est 8t, tandis que le milieu de la grande base reste à distance au moins
  (1+t)8/√29 du second représentant.

Le triangle et sa grande arête entrent ensemble au rayon de son cercle
circonscrit. Dans cet exemple, leur effondrement élimine ce saut ; cela
ne démontre pas une stabilité géométrique universelle du réducteur.
Les calculs exacts sont dans la [capsule robustesse](robustesse/README.md).

Pour une garantie géométrique plus forte sur les régions denses, il faut
une hypothèse quantitative supplémentaire. Exemple suffisant : dans le
voisinage d'une paire de composantes identifiées, certifier une borne
d'erreur de niveau

\[
\operatorname{dist}(y,C_v^P(r))
\le \kappa_P\,(d_k^P(y)-r)_+,
\]

et la borne analogue pour P', sur les points de l'autre composante.
Les inclusions précédentes donnent alors
d_H(C_v^P(r),C_{v'}^{P'}(r))≤max(κ_P,κ_{P'})δ. L'identification des
composantes et ce contrôle local sont des hypothèses à certifier ; on
ne les obtient pas de la seule égalité des nombres de Betti. La stabilité
de leurs frontières demande encore un contrôle propre.

### 1.2 Moins de k aberrants : correction nécessaire

L'énoncé « ils ne créent aucune composante d'ordre k » est faux dans le
nuage ambiant. À k=2 et r=2, P={0,6,12} ne donne aucune région dense ;
ajouter le seul point 1 crée la lentille de {0,1}. Autre cas : P={0,2,5}
possède deux composantes ; ajouter 3 les relie. Un seul point, donc moins
de k, peut créer une naissance ou une fusion en participant avec les sites
existants. Ces exemples collinéaires sont également valables dans R³.

L'énoncé correct à rayon fixé est : **un groupe O de moins de k points,
séparé de P par une distance strictement supérieure à 2r, ne modifie pas
Ω_k(r)**. Aucune boule de rayon r ne voit simultanément un point de O et
un point de P ; O seul n'atteint pas k. La séparation stricte est nécessaire
pour exclure aussi les contacts à rayon r.

Sans séparation, la garantie exacte est un déplacement de l'ordre. Pour
m ajouts distincts et k>m,

\[
\Omega_k^P(r)\subseteq\Omega_k^{P\cup O}(r)
\subseteq\Omega_{k-m}^P(r).
\]

Pour m suppressions, Q⊂P,

\[
\Omega_{k+m}^P(r)\subseteq\Omega_k^Q(r)
\subseteq\Omega_k^P(r),
\]

avec les niveaux hors cardinalité interprétés comme vides. Ce sont des
inclusions de la bifiltration, pas une invariance à k fixé. C'est le bon
sens à donner ici à la résistance aux aberrants.

### 1.3 Sous-échantillonnage : conserver la masse ou contrôler sa perte

Une décimation arbitraire sans poids n'a pas de garantie à k fixé, même
si elle couvre géométriquement très bien P. Elle peut retirer précisément
les k témoins nécessaires. Deux contrats possibles :

1. **Transport borné avec poids.** Affecter chaque site original à un
   représentant distant d'au plus δ et lui transférer son poids. Le
   compte pondéré dans B(y,r+δ) majore alors le compte original dans
   B(y,r), et réciproquement. On retrouve l'entrelacement en rayon au
   même seuil de masse. Cela requiert un moteur/oracle pondéré distinct
   du contrat FULL unitaire courant.
2. **Écart de mesure certifié.** Pour des mesures normalisées μ et ν,
   contrôler μ(B(y,r))≤ν(B(y,r+δ))+ζ et l'inégalité symétrique. Les
   ensembles de masse au moins t sont alors inclus l'un dans l'autre
   après (r,t)↦(r+δ,t−ζ), pour t>ζ. Fixer k après décimation sans
   considérer k/|P| ne conserve même pas le seuil de masse.

Ce second cadre est celui des théorèmes de stabilité de la multi-couverture
pour des perturbations de positions et de masses.
[Blumberg–Lesnick, Stability of 2-Parameter Persistent Homology](https://arxiv.org/abs/2010.09628).
Une décimation aléatoire peut recevoir un contrôle probabiliste ; il faut
en annoncer le modèle, l'erreur et la probabilité, pas l'assimiler au
déplacement apparié des mêmes sites.

## 2. Q2 — Le diamètre local est correct ; son usage doit être certifié

Votre inégalité est juste. Si m=|U| et j=k−|I|, deux j-parties J,J'
diffèrent, après annulation des éléments communs, par au plus min(j,m−j)
paires de sites. Donc

\[
\operatorname{diam} P_{I,U,k}
\le\frac{\min(j,m-j)}{k}\operatorname{diam}U.
\]

Une cellule active possède un témoin de rayon ≤r, donc diam(U)≤2r.
Cela donne 2r/k pour une jonction j=m−1 et 4r/k en dimension 3 générique,
où m≤4. **La seconde constante ne vaut pas sans cette hypothèse** : les
coquilles d'un nuage quantifié peuvent être plus grandes. Employer la
valeur réelle de m et le diamètre certifié de la cellule.

La seule égalité des naissances ne confine pas un effondrement dans une
cellule. Une chaîne de N segments de longueur ℓ, tous de même naissance,
peut être effondrée jusqu'à une extrémité : chaque étape est locale, mais
l'erreur finale vaut Nℓ. Ce contre-exemple vise le certificat générique
« dates égales ⇒ déplacement local », sans prétendre réaliser cette chaîne
comme un plateau particulier de la mosaïque. Une preuve propre aux
intervalles considérés pourrait donner mieux ; elle doit identifier le
porteur commun et contrôler les déplacements ultérieurs.

### Proposition : protéger les sommets de la mosaïque

Supposons L_r⊂A_k(r) obtenu par un certificat d'effondrement filtré, et
que **tous les sommets originaux de la mosaïque** soient conservés à leurs
dates. Pour une composante v, poser
D_v(r)=max_{σ⊂A_{k,v}(r)}diam(σ). Alors

\[
\boxed{d_H(L_{r,v},A_{k,v}(r))\le D_v(r)},\qquad
\boxed{d_H(L_{r,v},C_v(r))\le r}.
\]

Preuve de la première borne : un point de σ trouve dans L un sommet de
σ à distance au plus diam(σ), dans la même composante ; L⊂A donne
l'autre sens. Il n'y a aucun cumul des étapes.
Pour la seconde, L⊂A⊂C⊕B_r ; réciproquement, tout y∈C possède un
sommet actif c_Q à distance ≤r, comme dans la première réponse, et ce
sommet est conservé. **Aucun terme D_v n'est à ajouter à cette borne.**
Les Q conservés redonnent également la couverture exacte des points.

J'accepte donc η_v≤θr comme budget d'approximation de **A**, à condition
de le vérifier par composante. Avec les sommets protégés, D_v≤θr est
un certificat suffisant, parfois conservateur. En cas générique, θ=4/k
est une borne suffisante ; ce n'est pas un choix universel de qualité.
La borne 2/k concerne seulement les porteurs de type jonction.

Ces bornes en r/k concernent l'erreur **L↔A**, pas l'erreur au vrai C.
Pour k=n, placer deux sites aux extrémités 0 et 2r et les autres très près
de 0 à l'intérieur de leur boule minimale. À la naissance, Ω_n(r)={r}
et A_n est un unique barycentre, proche de 2r/n. Son erreur approche
r(1−2/n), alors que le diamètre de sa cellule est nul. Ni petite cellule,
ni petite erreur de simplification ne supprime l'écart initial du dual.

## 3. Q3 — Appariement direct et choix déterministe

**La subdivision barycentrique n'est pas nécessaire.** Sur un vrai complexe
de polytopes convexes, accepter une suite explicite de paires (σ,τ) où :

- σ est une facette de τ et est globalement libre : aucune autre cellule
  encore conservée ne contient σ ;
- σ et τ ont la même naissance exacte, et la même bigraduation si l'on
  réduit un modèle portant aussi les verticales ;
- retirer leurs intérieurs relatifs conserve les autres faces et ne retire
  aucun sommet protégé.

C'est l'effondrement élémentaire d'un complexe cellulaire régulier.
[Forman, Morse Theory for Cell Complexes, §1 et théorème 3.3](https://webhomes.maths.ed.ac.uk/~v1ranick/papers/forman5.pdf).
Visibilité d'une face à l'écran et liberté topologique sont différentes.
Le vérificateur doit lire toutes les incidences, y compris celles des
strates de dimension inférieure et des cellules non affichées.

Les priorités extérieur/intérieur, DTM ou proximité des sites ordonnent
les **paires admissibles**. Elles ne prouvent pas une minimalité. Pour le
premier prototype, retenir les sommets, préférer les paires de plus grande
dimension puis de petit diamètre, et départager par les labels exacts
triés. Le résultat est reproductible avec ces identifiants ; il n'est pas
annoncé comme intrinsèquement canonique sous tout réétiquetage. Les
garanties précédentes sont indépendantes de ce choix de priorité.

**Le certificat doit porter sur toute la plage de filtration visée.** Une
face libre aujourd'hui peut avoir une coface demain. Exemple alpha k=1 :
A=(0,6), B=(4,6), C=(2,7), D=(2,0). AB et ABC naissent à 25/4 ; AB
est alors libre dans ABC. Plus tard, ABD apparaît à 100/9. Supprimer
définitivement AB tout en gardant ABD ne définit plus un sous-complexe.
Les [cercles et dates exacts](ombre_et_niveaux/README.md) établissent ce cas.

Pour le certificat simple de la première réponse, vérifier les paires sur
le complexe de toute la plage et conserver une cible fermée par faces.
Des réductions variables, des réinsertions ou des applications
d'attachement transférées peuvent fournir d'autres modèles valides,
mais demandent leur propre preuve de compatibilité. Des réductions
indépendantes nœud par nœud ne suffisent pas.

## 4. Q4 — Oui à l'ombre déclarée ; conserver les propriétaires séparément

Pour chaque cellule active σ, poser
S_σ=conv(⋃_{c_Q sommet de σ}Q), puis S_v(r)=⋃_{σ∈A_{k,v}(r)}S_σ.
Alors, avec les labels complets de la mosaïque,

\[
\boxed{P\cap S_v(r)=P\cap(C_v(r)\oplus B_r)},\qquad
S_v(r)\subseteq C_v(r)\oplus B_r,\qquad d_H(S_v(r),C_v(r))\le r.
\]

Preuve : le témoin y_σ∈C_v place tous les Q de la cellule dans B(y_σ,r),
donc également leur enveloppe convexe. Cela donne l'inclusion et empêche
d'ajouter un site non couvert. Dans l'autre sens, tout site couvert est
dans un label Q actif, donc dans S_v. Enfin A_v⊂S_v, d'où la borne
de Hausdorff par le même raisonnement que précédemment.

La formule conv(I∪U) s'applique lorsque les labels de la tranche ont bien
cette union. À j=0, leur union est I ; utiliser en général **les Q réels**.
Cette construction est une expansion en ensembles : un sommet c_Q peut
devenir un triangle ou un solide conv(Q). Ce n'est pas l'image d'une
application simpliciale ordinaire envoyant chaque sommet sur un point.

**Je l'accepte comme rendu de couverture identifié par nœud.** Elle n'est
pas un représentant homotopique certifié. Sur P={0,2,4}, k=2,r=1, les
deux régions denses sont les points 1 et 3 ; leurs ombres [0,2] et [2,4]
se touchent en 2. Leur union connectée ne doit pas fusionner les deux
nœuds ni effacer le fait que le point 2 est couvert par les deux.

Après réduction, construire l'ombre à partir des labels et porteurs
conservés explicitement, ou la garder comme objet séparé. L'ombre des
seules cellules survivantes n'hérite pas automatiquement de la trace
exacte : un sommet supprimé peut porter l'unique incidence d'un site.
La protection de tous les sommets évite cette perte de trace, même si
la suppression de cellules peut encore changer le remplissage de l'ombre.

## 5. Q5 — Un jeton peut condenser une famille ; il doit dire quelle coupe il montre

Conserver comme objet mathématique la famille A_{k,v}(r), ses dates de
cellules et ses labels, sur [b_v,d_v). La géométrie et les points couverts
peuvent changer sans fusion H₀. Pour le réseau, une lecture finie est
légitime ; stocker la coupe choisie et son côté ouvert/fermé.

- **Coupe de fin de vie.** Pour A, qui est fini,
  A_v(d_v^−)=⋃_{b_v≤r<d_v}A_v(r) est un état combinatoire atteint avant
  la mort. Sélectionner strictement a_σ<d_v² et attribuer les cellules
  au nœud à cette coupe. Aucun « flottant immédiatement précédent »
  n'est nécessaire. À d_v même, le propriétaire est déjà le parent.
- **Région continue.** Il n'existe généralement pas de plus grand rayon
  strictement inférieur à d_v. Sur P={0,2,10}, k=2, le nœud né à 1
  meurt à 5, tandis que sa région croît strictement sur [1,5). La
  géométrie pré-mort d'A ne doit pas être appelée C_v(d_v).
- **Coupe avec marge au bruit.** Choisir un rayon intérieur et publier
  ses marges à b_v et d_v ; une marge supérieure à δ évite de choisir
  exactement une borne H₀. Cela ne garantit pas l'absence d'événements
  internes d'alpha, de couverture ou d'homologie supérieure. La racine,
  avec d_v=∞, exige une borne d'échelle finie déclarée.

Je recommande de conserver la famille sous forme d'événements, puis de
produire les vues demandées : une coupe intérieure pour un jeton de forme,
et éventuellement la coupe d_v^− pour une vue de couverture maximale
avant fusion. La totalité des maillages intermédiaires n'a pas à être
dupliquée. Une seule vue ne prétend pas résumer exactement toute l'évolution.

Entre ordres, un représentant par k et les liens certifiés sur π₀ sont
une réponse suffisante pour la **tour de composantes FULL**. Pour une
hiérarchie de solides avec des applications topologiques compatibles,
conserver le modèle bifiltré et ses cellules entre tranches. Des liens
de parents seuls ne prouvent ni une inclusion des solides ni une
bifiltration homotopique. C'est le contrat du produit consommateur qui
détermine lequel de ces deux niveaux est nécessaire.

## 6. Une piste récente pour la taille, avec un contrat approché explicite

Il existe une alternative théorique directement consacrée à la
multi-couverture, sans passer à la DTM :
[Alonso, A Sparse Multicover Bifiltration of Linear Size, SoCG 2025](https://drops.dagstuhl.de/storage/00lipics/lipics-vol332-socg2025/LIPIcs.SoCG.2025.6/LIPIcs.SoCG.2025.6.pdf).
Les théorèmes 8, 14, 15 et 18 donnent un modèle simplicial de taille O(n)
à dimension et précision fixées, indépendamment de k. Son modèle spatial
SCov satisfait, avec le paramètre ε>0 de l'article,

\[
\Omega_k(r)\subseteq\operatorname{SCov}_{(1+3\varepsilon)r,k},
\qquad
\operatorname{SCov}_{r,k}\subseteq
\Omega_k\!\left(\frac{1+2\varepsilon}{1+\varepsilon}r\right).
\]

La construction utilise des représentants pondérés et leurs cartes de
couverture, avec des boules qui ralentissent puis disparaissent. Les
constantes dépendent fortement de la précision et de la dimension ; le
modèle simplicial n'est pas automatiquement un petit solide plongé en R³.

**À étudier comme voie approchée distincte**, si le complexe exact reste
trop gros. Les inclusions décalent les rayons : elles n'identifient pas
les nœuds ni leurs points couverts au même (k,r). Elles ne remplacent donc
pas les certificats exacts demandés au premier prototype. Cette piste
répond néanmoins plus directement au besoin « petit et robuste » qu'une
substitution implicite de la DTM. Aucun coût pratique ni port v11 n'est acquis.

## Travail concret à engager

1. Oracle exact et tests des labels, avec les témoins de la première réponse.
2. Réduction polyédrique à sommets protégés, journal des paires et vérificateur
   global indépendant ; diamètres certifiés par composante.
3. Deux rendus explicitement nommés : dual réduit et ombre de couverture.
   Garder les mêmes identités de nœuds, même en cas de recouvrement visuel.
4. Trois épreuves de robustesse distinctes : positions appariées, modifications
   de population, variations d'échantillonnage avec leur contrat de masse.
5. Comparer ensuite le nombre de cellules et le coût du rendu ; les garanties
   ci-dessus n'annoncent pas d'avance une réduction importante.

Les capsules liées conservent les calculs rationnels bornés et leur portée.
Les théorèmes généraux sont les preuves rédigées ici ou les résultats primaires
explicitement cités. Aucun résultat de performance ni de reconstruction
sémantique n'est déduit de ces vérifications.

Les [sources épinglées](sources.json) et le [rejeu indépendant](verification.json)
ferment ce lot : quatre exécutions, normal/−O identiques, capsules inchangées.
Les empreintes se vérifient depuis ce dossier avec `sha256sum -c SHA256SUMS`.
