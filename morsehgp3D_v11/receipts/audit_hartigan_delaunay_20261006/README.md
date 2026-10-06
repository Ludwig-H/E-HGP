# Réponse mathématique : le polyèdre d'un nœud de Hartigan

6 octobre 2026. Réponse aux cinq questions du développeur,
[figées en fd85f3bb5](../../audits/QUESTION_CLAUDE_POLYEDRE_ORDRE_K_20261006.md).
Cadre : `exploration_v11_hors_registre / cpu_reference /
quantized_u21_input_only / not_claimed`.
Lecture des sources primaires, preuves ci-dessous et contre-épreuves indépendantes
en `Fraction`. Aucun build, test natif, banc LiDAR ou appel GCP. Aucun constructeur
de mosaïque ni algorithme de simplification n'est qualifié par cette réponse.

**Oui : pour représenter la tour HGP, je retiens le complexe alpha d'ordre k,
sur la mosaïque de Delaunay d'ordre k, filtré par le rayon du k-ième voisin.**
La composante dense reste l'objet de référence ; le complexe en est un représentant
polyédrique homotope. L'offset demandé est une enveloppe supplémentaire, et la
distance à la mesure définit une autre hiérarchie. Ces distinctions ont des
conséquences effectives, démontrées plus bas sur quatre points seulement.

Cette réponse prend acte du nouveau cadrage transmis par le développeur :
représenter la **région de haute densité**, éventuellement dilatée. La
[proposition précédente](../audit_geometry_design_20261006/README.md), relative
à la surface LiDAR observée, reste une réponse à une autre cible. Un trou absent
de la région dense à la coupe choisie ne peut pas être conservé par une
représentation qui en revendique exactement le type d'homotopie.

## 0. Raccord aux deux premières parties du manuscrit

**La généralisation doit prolonger la construction du manuscrit.** À la
demande explicite de l'utilisateur, les parties I–II ont été relues dans le
[PDF source](../../../docs/references/MANUSCRIT_THESE_HAUSEUX.pdf), pages PDF
35–76 et 77–134. Les numéros ci-dessous sont les pages imprimées.

La partie I fixe successivement l'objet statistique — composantes des
surniveaux, définition 6, pp.18–19 —, son estimateur K-NN — définition 7,
pp.19–20 —, puis sa lecture sur les données par couverture — définition 8,
p.21. La partie II établit le passage exact par Čech et Γ_K, définitions
20–22, pp.57–58, théorème 2, pp.60–61. Le sens de construction est donc
**modèle de Hartigan → témoins géométriques → structure calculable**.
Le changement d'estimateur pour faciliter le dessin inverserait ce principe.

Voici le prolongement direct de ces définitions. Pour toute K-partie Q,
reprendre la région témoin du §6.3.1 :

\[
W_Q(r)=\bigcap_{q\in Q}\bar B(q,r).
\]

Introduire le nerf de ces régions, que l'on peut noter \(\mathcal N^W_K(r)\).
Une famille {Q₀,…,Q_m} en est un simplexe exactement lorsque
Q₀∪⋯∪Q_m est un simplexe du Čech de rayon r. **Son 1-squelette est
précisément le Γ_K de la définition 21**, avant la restriction ultérieure
aux adjacences élémentaires. Les W_Q sont convexes et couvrent Ω_K(r),
donc leur nerf prolonge la preuve du théorème 2 à un modèle homotopique
complet. Il peut être énorme : c'est une définition et un oracle conceptuel,
pas la proposition de tout énumérer sur LiDAR.

La réduction géométrique naturelle consiste alors à restreindre chaque
région témoin à sa cellule de Voronoï d'ordre K : C_Q=W_Q∩V_Q. Ces pièces
couvrent encore **exactement la même région dense**. Le nerf restreint et
la mosaïque duale filtrée du §1 représentent ce même objet. Ainsi le raccord
avec le manuscrit impose, à chaque coupe et de façon compatible en r :

\[
\pi_0\Gamma_K(P,r)
=\pi_0\mathcal N^W_K(r)
\cong\pi_0\mathcal N^C_K(r)
\cong\pi_0 A_K(r)
\cong\pi_0\Omega_K(r).
\]

Le polyèdre géométrique enrichit donc les nœuds existants ; il ne redéfinit
pas leurs naissances, fusions ou appartenances de couverture. Les singletons
de Γ_K restent présents dans ce contrat complet. Leur exclusion dans le
théorème 5, p.91, ne justifie pas leur retrait de FULL.

Quatre obligations précisent ce que signifie « dans le même esprit » :

- **Même paramètre statistique.** Pour K fixé, λ=K/(nω_p r^p), comme dans
  la définition 7 : r croissant correspond à λ décroissant. Les verticales
  Ω_K(r)⊂Ω_{K−1}(r) utilisent un rayon commun ; elles ne comparent pas les
  estimateurs à un seuil λ commun. Ne pas confondre ces deux coupes.
- **Même couverture des points.** Le théorème 2 donne
  P_C=P∩(C⊕B_r), avec recouvrements possibles. L'offset intervient après
  identification de C ; ses contacts ne sont pas de nouvelles fusions HGP.
  Une simplification ou un rendu garde les incidences de couverture du
  nœud. Il ne les recalcule pas par appartenance au dessin simplifié.
- **Même discipline de réduction.** Les théorèmes 4–6, pp.88–93, expliquent
  pourquoi Gabriel et le K-MST suffisent aux fusions pertinentes. Ce sont
  des garanties de connexité. Exiger en plus les trous ou le type
  d'homotopie du volume demande les preuves supplémentaires des §§1 et 4,
  pas une nouvelle interprétation des seuls supports du K-MST.
- **Même distinction entre hiérarchie et extraction.** Le chapitre 5 permet
  des critères adaptés à l'application sur l'arbre ; le §9.1, pp.96–97,
  place le vote de partition des points après la hiérarchie de recouvrements.
  Les « facettes » de cette section sont les K-parties, pas les triangles
  d'un maillage d'affichage. Raffiner ou réduire celui-ci ne doit changer
  ni les masses ni les votes du modèle conservé.

Le chapitre 7 donne aussi le bon critère de fidélité : récupérer l'amas
dense **avant sa fusion avec le fond**, en tenant compte de la couverture
des points de frontière (§§7.2–7.3, pp.70–73). Une réduction purement
représentative doit donc laisser inchangés cette coupe, son propriétaire
et l'ensemble de points couverts. Cela préserve le modèle fini ; les
énoncés asymptotiques de percolation gardent leurs hypothèses et ne sont
pas, à eux seuls, une garantie sémantique sur une scène LiDAR.

Enfin, la définition 31, pp.91–92, écrit Del_K comme un **nerf abstrait**.
Cette convention porte bien les incidences utilisées par le manuscrit.
Pour en tirer un objet plongé dans R³, il faut expliciter le passage au
dual polyédrique : le §1.2 montre pourquoi les deux ne sont pas des
complexes littéralement identiques. C'est une précision nécessaire à
la généralisation géométrique, sans changer la hiérarchie du théorème 2.

## 1. Q1 — Définir l'objet et construire son représentant

Soit P un ensemble fini de n sites distincts de poids unitaire, 1≤k≤n, r≥0.
Toutes les boules et toutes les coupes sont fermées ; le niveau stocké est a=r².
On note

\[
\Omega_k(r)=\{y:d_k(y)\le r\},\qquad
f_k(y)^2=\frac1k\sum_{i=1}^k d_i(y)^2.
\]

Le nœud vivant v représente une composante C=C_v(r) de Ω_k(r). Distinguer :

| Objet | Ce qu'il représente |
| --- | --- |
| C | La composante spatiale du modèle de Hartigan/HGP à (k,r). |
| \(\lvert A_{k,v}(r)\rvert\) | Son représentant polyédrique dual, homotope à C. |
| \(C\oplus\bar B_r\) | Son enveloppe dilatée, si ce choix de dessin est voulu. |
| \(P\cap(C\oplus\bar B_r)\) | L'ensemble discret des sites couverts ; pas un volume rempli. |
| \(\{f_k\le r\}\) | Le sous-niveau de la distance à la mesure, autre filtration. |

Le troisième et le quatrième objets correspondent à une couverture par la
composante. Ne pas confondre l'ensemble de sites appelé « polyèdre » dans
le [contrat des points](../../docs/HIERARCHIE_POINTS.md) avec son enveloppe
convexe ou avec un solide. Deux composantes distinctes peuvent couvrir des
sites communs.

### 1.1 La construction exacte, y compris aux contacts

Pour une k-partie Q⊂P, poser

\[
V_Q=\{y:\|y-q\|^2\le\|y-p\|^2\quad(q\in Q,p\notin Q)\},
\qquad C_Q(r)=V_Q\cap\bigcap_{q\in Q}\bar B(q,r).
\]

V_Q est un polyèdre convexe et C_Q(r) un compact convexe, éventuellement vide.
Leur union est exactement Ω_k(r) : un choix des k plus proches voisins donne
un Q convenable à chaque point. C'est une couverture dont toutes les
intersections non vides sont convexes. Son nerf fournit donc un modèle
homotopique, avec les inclusions en r. Cette construction est également celle
du complexe d'ordre k de [Corbet–Kerber–Lesnick–Osang, §2.3](https://link.springer.com/article/10.1007/s00454-022-00476-8).

Pour la réalisation polyédrique, prendre la **subdivision régulière exacte**
duale aux domaines V_Q de dimension pleine dans R³, leurs faces comprises.
Les domaines pleins fermés couvrent R³, même si les sites sont collinéaires.
Les étiquettes Q dont le domaine est seulement de dimension inférieure peuvent
être utiles comme incidences, mais ne créent pas chacune un sommet géométrique.
Pour une cellule σ de cette mosaïque et sa face duale F_σ, définir

\[
a_\sigma=\min_{y\in F_\sigma}d_k(y)^2,
\qquad A_k(r)=\{\sigma:a_\sigma\le r^2\}.
\]

Sur F_σ, cette fonction est le maximum des distances carrées aux sites d'un
Q actif. Le minimum existe. Si τ est une face de σ, F_σ⊆F_τ, donc
a_τ≤a_σ : la sélection est bien un sous-complexe, emboîté en r. C'est
la filtration alpha d'ordre supérieur étudiée par
[Edelsbrunner–Osang](https://pub.ista.ac.at/~edels/Papers/2020-J-07-SimpleAlgorithm.pdf).

**Pourquoi le passage du nerf au polyèdre reste correct en cas dégénéré.**
Voici une preuve finie, sans perturber les données. Dans le nerf N des C_Q(r)
des domaines pleins, toute famille qui s'intersecte a une face commune F des
V_Q et un témoin y∈F∩Ω. La cellule duale σ_F est active ; tous ses sommets
Q ont le même témoin admissible. La famille initiale appartient donc au
simplexe plein Δ(vert σ_F) de N. Réciproquement, chaque cellule active σ
fournit ce simplexe plein. Ainsi N est l'union des Δ(vert σ), pour les
cellules maximales actives σ. A est l'union des polytopes σ correspondants.
Les intersections des polytopes sont des faces communes ou sont vides ;
les intersections des simplexes sont les simplexes de leurs sommets communs.
Les deux couvertures ont exactement le même nerf et des intersections
contractiles. Par le lemme du nerf,

\[
\lvert A_k(r)\rvert\simeq\lvert N_k(r)\rvert\simeq\Omega_k(r).
\]

Cette preuve concerne une vraie subdivision polyédrique et ses incidences
complètes. Elle ne certifie pas une collection de supports, une triangulation
locale incompatible sur les faces communes ou une sélection à tolérance.
Sur π₀, la correspondance est donnée par les étiquettes Q et leurs incidences ;
elle commute avec r croissant. Pour la version filtrée de cette preuve,
indexer les couvertures intermédiaires par toutes les cellules actives,
plutôt que seulement les maximales : les indices ne font alors que s'ajouter
et les nerfs communs sont compatibles avec les inclusions. Conserver ces
applications naturelles, et pas seulement une liste de nombres de Betti.
La géométrie d'un même nœud peut changer entre deux événements H₀ ; sa
géométrie à la naissance ne représente pas toutes ses coupes ultérieures.

### 1.2 Barycentres pondérés : même dual, mais attention au mot « nerf »

Les coordonnées et poids proposés sont corrects :

\[
c_Q=\frac1k\sum_{q\in Q}q,\qquad
w_Q=\|c_Q\|^2-\frac1k\sum_{q\in Q}\|q\|^2,
\quad
\|y-c_Q\|^2-w_Q=\frac1k\sum_{q\in Q}\|y-q\|^2.
\]

Minimiser cette puissance sur Q choisit précisément les k plus proches voisins.
Le diagramme de puissance est donc le diagramme d'ordre k, comme dans
[BCY, théorème 4.8](https://inria.hal.science/hal-01615863).
**Son nerf abstrait n'est toutefois pas littéralement sa mosaïque plongée.**

Exemple exact : les quatre sites (1,1,1), (1,−1,−1), (−1,1,−1), (−1,−1,1),
à k=2, donnent six barycentres ±e₁, ±e₂, ±e₃, tous de poids −2.
Leurs six domaines de puissance se rencontrent à l'origine : le nerf contient
un simplexe de dimension 5. Leur cellule duale plongée est un octaèdre de
dimension 3. Cela arrive alors que les quatre sites sont en position générale
usuelle. La position générale des sites ne rend pas génériques tous les
barycentres pondérés induits. Une triangulation cohérente de l'octaèdre est
possible ; l'assimilation directe au simplexe abstrait ne l'est pas.

À une sphère de sites intérieurs I et de coquille U, la cellule de la tranche
j=k−|I| est bien

\[
P_{I,U,k}=\operatorname{conv}\left\{
\frac{\sum_{x\in I}x+\sum_{x\in J}x}{k}:
J\subset U,\ |J|=j\right\}.
\]

Pour j=|U|−1, elle est l'image de conv(U) par
u↦(ΣI+ΣU−u)/k. L'homothétie de rapport −1/k est correcte.
L'énoncé porte sur la coquille complète U et cette tranche ; un support
positif S* peut être plus petit que U et ne remplace pas cette donnée.

### 1.3 Retrouver exactement le K-polyèdre discret du manuscrit

Le raccord est plus fort qu'une simple bijection des composantes. Pour C et
sa composante duale A_C, les étiquettes Q de ses sommets actifs suffisent :

\[
P\cap(C\oplus\bar B_r)
=\bigcup_{\substack{Q:\ \dim V_Q=3,\ C_Q(r)\ne\varnothing\\ C_Q(r)\subset C}}Q
=\bigcup_{c_Q\ \mathrm{sommet\ de}\ A_C}Q.
\]

**Preuve.** Chaque Q à droite a un témoin dans C, qui couvre tous ses
sites. Réciproquement, soit p couvert par y∈C. Suivre le segment y→p
jusqu'au premier point z où p peut être choisi parmi les k voisins.
Avant z, au moins k sites sont plus proches que p, donc d_k≤distance à p≤r :
le segment reste dans Ω et z appartient à C par fermeture. On peut choisir
un domaine plein fermé V_Q contenant z, avec p∈Q. Pour le voir même en
présence d'égalités, avancer infinitésimalement de z vers p : pour tout
q≠p ex æquo à z,

\[
\|z+s(p-z)-q\|^2-\|z+s(p-z)-p\|^2=s\|p-q\|^2>0.
\]

Une perturbation générique encore plus petite sélectionne un domaine plein
dont la fermeture contient z et dont le label garde p. Ces déplacements
ne servent qu'à choisir Q ; ils n'ont pas besoin de rester dans Ω. Au
contact z, les sites de Q sont tous à distance ≤r, donc z∈C_Q(r), et la
convexité de C_Q(r) l'attribue à C. Cela couvre aussi le contact initial
z=y et les cas k=1 ou k=n.

Cette identité est une déduction des définitions, valable sans position
générale. Elle permet de retrouver **exactement** la définition 8 et le
théorème 2 par les labels de la mosaïque. Après réduction, conserver ces
labels avec leurs dates et leurs nœuds ; les seuls sommets survivants ne
suffisent plus automatiquement. Ni P∩A_C ni P∩(A_C⊕B_r) ne doit être
substitué à cette formule.

## 2. Q1 — Les inclusions sont justes, l'égalité avec l'offset est fausse

Je confirme votre preuve. Si y₀∈F_σ réalise un niveau ≤r², tout sommet
c_Q de σ est la moyenne de k sites de B(y₀,r). Pour y∈σ, il existe donc
des coefficients 0≤μ_x≤1 de somme k, avec y=Σμ_x x/k, et

\[
f_k(y)^2\le\frac1k\sum_x\mu_x\|y-x\|^2
=\frac1k\sum_x\mu_x\|y_0-x\|^2-\|y-y_0\|^2
\le r^2-\|y-y_0\|^2.
\]

La première inégalité vient de la minimisation d'une forme linéaire sur
{0≤μ≤1,Σμ=k}, dont les sommets sélectionnent k sites. De plus σ⊂B(y₀,r).
Si σ appartient au nœud v par ses incidences, y₀ appartient à C_v(r). Ainsi,
**composante par composante**,

\[
\lvert A_{k,v}(r)\rvert\subseteq C_v(r)\oplus\bar B_r,
\qquad \lvert A_k(r)\rvert\subseteq\{f_k\le r\}.
\]

On peut ajouter une borne dans l'autre sens. Pour y₀∈C_v(r), choisir un
domaine plein fermé V_Q contenant y₀. Son sommet c_Q est actif, appartient
à la composante polyédrique correspondante, et \|y₀−c_Q\|≤r. Donc

\[
C_v(r)\subseteq\lvert A_{k,v}(r)\rvert\oplus\bar B_r,
\qquad d_H(C_v(r),\lvert A_{k,v}(r)\rvert)\le r.
\]

Ce contrôle métrique d'échelle r ne garantit pas une frontière proche, ni la
fidélité d'un détail beaucoup plus petit que r. La fonction d_k est
1-Lipschitz ; on obtient aussi A_k(r)⊂Ω_k(2r). Combinée à l'inégalité
d_k≤√k f_k, la meilleure de ces deux bornes est Ω_k(min(2,√k)r).

**Votre contre-exemple est exact.** P={0,1,10} sur un axe, k=3, r=5 :
Ω₃(5) est le singleton (5,0,0), A₃(5) le singleton (11/3,0,0),
et d₃(11/3)=19/3. L'offset de Ω est une boule de rayon 5 autour de 5 ;
le sous-niveau DTM est une boule centrée en 11/3, de rayon carré 43/9.
Les trois objets diffèrent. **Être inclus dans l'offset ne signifie pas
être sa représentation homotopique.**

Il existe un danger plus direct pour l'implantation. Prendre
P={0,1,2,11}, k=3, r=21/4. Les domaines pleins portent
T={0,1,2} et Q={1,2,11}, avec frontière x=11/2. La mosaïque active a
deux sommets, 1 et 14/3, sans arête : son arête naît au rayon 11/2.
Ω a deux composantes, dont les projections sur l'axe sont
[−13/4,21/4] et [23/4,25/4]. Or **le barycentre 14/3 de la composante de
droite est géométriquement dans la composante dense de gauche**.
L'attribution par inclusion du barycentre, ou par sa proximité d'une
composante, est incorrecte. Utiliser Q, les régions C_Q et leurs incidences.
[Dérivation et rejeu exacts](proofs/README.md).

## 3. Q2 — Utiliser la DTM sans changer silencieusement la hiérarchie

L'identité de puissance ci-dessus donne min_Q π_Q=f_k². Un alpha pondéré
standard sur ces barycentres suit donc la DTM. Sur le même dual, ses dates
b_σ=min_{F_σ}f_k² satisfont b_σ≤a_σ≤k b_σ. Dans l'espace,

\[
\Omega_k(r)\subseteq\{f_k\le r\}\subseteq\Omega_k(\sqrt{k}\,r).
\]

Ces inclusions autorisent des comparaisons de filtrations ; elles ne disent
pas que les composantes à rayon identique coïncident. Dans l'exemple
{0,1,2,11}, la date carrée DTM de l'arête est 251/12, contre 121/4
pour HGP. À r²=441/16, **DTM a déjà fusionné les deux composantes HGP**.

J'accepte donc f_k comme attribut, couleur, score de priorité pour proposer
une simplification, ou indicateur d'erreur. Les décisions d'activation et
les rattachements au nœud doivent continuer d'être certifiés avec a_σ et les
incidences HGP. Déplacer des sommets selon un gradient DTM demande en plus
un contrôle géométrique : absence de retournements et d'intersections
introduites, compatibilité des faces partagées et erreur de déplacement.
La stabilité d'une fonction scalaire ne prouve aucune de ces propriétés.

La stabilité Wasserstein de la DTM à masse fixée est le
[théorème 10.16 de BCY](https://inria.hal.science/hal-01615863).
Ne pas en déduire que d_k est instable sous toute perturbation : si une
bijection déplace chacun des n sites d'au plus ε, l'ordre des distances
donne directement \|d_k^P−d_k^{P'}\|∞≤ε. Les filtrations HGP sont alors
entrelacées additivement de ε en rayon. C'est un autre modèle de bruit que
le déplacement d'une petite masse arbitrairement loin.

Précision de convention : la définition 10.12 de BCY utilise une masse
strictement supérieure à m ; à m=k/n elle sélectionne le voisin k+1
(si k<n). Prendre le quantile fermé adéquat pour d_k. L'intégrale DTM
à m=k/n donne bien la moyenne des k premières distances carrées : la
valeur au seul point terminal ne change pas l'intégrale.

## 4. Q3 — Ce que chaque réduction peut réellement certifier

**Premier choix pratique : conserver le complexe et ne dessiner que ses
faces exposées, puis développer une réduction filtrée certifiée.** Cela
sépare le coût du dessin de la question, beaucoup plus difficile, d'une
représentation géométrique compacte possédant toutes les garanties.

| Proposition | Avis pour le représentant exact HGP |
| --- | --- |
| Wrap transféré directement à l'ordre k | Pas de théorème applicable fourni ; la prémisse Morse est fausse en général. |
| Effondrements certifiés, compatibles avec les dates | Oui pour l'homotopie et la filtration ; la forme nécessite un certificat supplémentaire. |
| Barycentres témoins des k-voisinages | Approximation DTM déclarée ; aucune identité HGP au même rayon. |
| Sites pondérés par leur DTM | Autre approximation DTM ; même réserve. |
| Frontière seule | Oui comme affichage du solide conservé ; non comme remplacement homotopique. |

### 4.1 Wrap et information critique

Pour k=1, les effondrements vers Wrap s'appuient sur un gradient cohérent
indépendant de r et sur des hypothèses explicites :
[Bauer–Edelsbrunner, théorème 5.10 et §6.1](https://pub.ista.ac.at/~edels/Papers/2017-J-03-DCech.pdf).
Pour k>1, la fonction rayon sur la mosaïque **n'est pas nécessairement une
fonction de Morse discrète généralisée**.
[Edelsbrunner–Nikitenko–Osang](https://link.springer.com/article/10.1007/s00022-021-00577-4)
classent des étapes critiques et non critiques, mais précisent que les seules
étapes critiques ne déterminent pas toute la topologie. Cela ne ferme pas
la recherche d'un Wrap d'ordre k ; cela exclut son adoption par analogie.

Ajouter une liste de « cellules H₁/H₂ » au H₀ de FULL ne suffit donc pas.
Il faut leurs applications d'attachement, les cellules de leurs frontières
et les relations entre étapes. Des cellules de dimension 3 peuvent tuer H₂.
La théorie récente de la distance k-NN montre même qu'un point critique
peut simultanément fusionner des composantes et créer un cycle ; ses
théorèmes ont leurs propres hypothèses de généricité et de niveaux distincts.
[Reani–Bobrowski, théorèmes 1–2 et figure 2](https://drops.dagstuhl.de/storage/00lipics/lipics-vol293-socg2024/LIPIcs.SoCG.2024.75/LIPIcs.SoCG.2024.75.pdf).

### 4.2 Un certificat suffisant que l'on peut vérifier

Proposition constructive indépendante du transfert de Wrap :

1. Construire une triangulation compatible de toute la mosaïque considérée,
   par exemple sa subdivision barycentrique. Donner à chaque simplexe la
   naissance exacte de sa plus petite cellule porteuse. Les tranches restent
   exactement des triangulations des A_k(r), sans événement intermédiaire.
2. Produire un sous-complexe L fermé par faces et un appariement face/coface
   acyclique sur son complément. Tout simplexe hors de L est apparié, et
   les deux membres de chaque paire ont **la même naissance exacte**.
3. Vérifier indépendamment les incidences, l'unicité et l'acyclicité des
   paires, les dates et la fermeture de L. Le critère d'effondrement donne,
   pour tout r, K_r↘L_r avec L_r=L∩K_r. Une liste explicite d'effondrements
   élémentaires, chacun sur une face libre, est un certificat encore plus direct.

Le critère de passage d'un appariement à un effondrement vers un
sous-complexe est donné dans
[Bauer–Edelsbrunner, théorème 2.1](https://pub.ista.ac.at/~edels/Papers/2017-J-03-DCech.pdf).
L'application filtrée ci-dessus est une déduction : avant la naissance
commune, la paire est absente ; après, elle est présente entière. Les
inclusions L_r→K_r commutent avec les inclusions en r. Pour conserver
aussi k, imposer la même **bigraduation** aux paires dans un modèle bifiltré.

Un appariement quelconque ne fournit pas nécessairement un sous-complexe
plongé ; une réduction de matrices de chaînes ne certifie pas à elle seule
le type d'homotopie. Et même un effondrement correct peut réduire une grosse
boule à un point : **homotopie et emboîtement ne contrôlent pas la forme**.
Pour dessiner L, ajouter une borne géométrique déclarée **pour chaque
composante identifiée**, par exemple d_H(L_{r,v},A_{k,v}(r))≤ε_v.
Elle entraîne d_H(L_{r,v},C_v(r))≤r+ε_v, mais toujours pas une borne de
frontière. Une borne globale pourrait utiliser la proximité d'une autre
composante et ne suffit pas à cette conclusion. Aucune taille petite
de L ni réduction efficace n'est acquise par ce seul certificat.

### 4.3 Les deux approximations DTM

La distance témoin utilise au plus n barycentres de k-voisinages :
[Guibas–Mérigot–Morozov, Witnessed k-Distance](https://arxiv.org/abs/1102.4972).
La construction sur les sites utilise, dans notre convention de puissance,

\[
g(y)^2=\min_{p\in P}\bigl(\|y-p\|^2+f_k(p)^2\bigr),
\qquad w_p=-f_k(p)^2.
\]

Ce signe négatif est essentiel. Pour l'échantillon euclidien, les bornes
f_k/√2≤g≤√3 f_k et f_k≤f_témoin≤√6 f_k sont établies par
[Buchet–Chazal–Oudot–Sheehy, théorèmes 4.11 et 4.16](https://donsheehy.net/research/buchet16efficient.pdf).
Elles concernent la DTM, pas l'égalité avec HGP à (k,r).
Exemple élémentaire : P={0,2,100}, k=2, r=1. Ω₂(1)={1}, tandis que
min g=√2 : la filtration des sites pondérés est encore vide.

Recalculer une triangulation régulière après sélection de barycentres peut
créer de nouvelles arêtes et cellules. Ce n'est donc pas automatiquement
un sous-complexe de la mosaïque initiale. Ces voies sont acceptables comme
produits approchés identifiés, ou comme propositions ensuite vérifiées
contre la filtration HGP ; elles ne remplacent pas son représentant exact.

### 4.4 La frontière et les strates à conserver

Un tétraèdre plein est contractile ; sa frontière est une sphère. Une coque
solide connexe peut avoir deux composantes de frontière. Les frontières
successives ne sont généralement pas emboîtées. On ne doit donc pas
remplacer l'objet topologique par sa seule frontière.

Pour le rendu, conserver les incidences du solide et masquer les faces
intérieures aux cellules de dimension 3. Dessiner aussi les cellules
maximales de dimensions 0, 1 et 2 qui ne bordent aucun volume actif : elles
existent aux naissances, aux contacts et sur des nuages dégénérés. Une
extraction de volume ouvert, ou l'hypothèse systématique de surface
manifold, perdrait ces parties de l'objet. Si la géométrie exacte de C
est demandée, ses morceaux de frontière sont sphériques ; un maillage
polygonal doit annoncer son erreur au lieu de revendiquer une égalité.

## 5. Q4 — Une coupe pour le polyèdre, un modèle bifiltré pour les verticales

À k fixé, la coupe horizontale du pavage rhomboïdal à profondeur k donne
la mosaïque d'ordre k. Avec les sommets usuels (ΣQ,−|Q|), diviser les
coordonnées spatiales de cette coupe par k pour obtenir les barycentres.
Le rayon de la cellule est celui de son plus petit rhomboïde porteur.
[Edelsbrunner–Osang, construction rhomboïdale](https://drops.dagstuhl.de/storage/00lipics/lipics-vol099-socg2018/LIPIcs.SoCG.2018.34/LIPIcs.SoCG.2018.34.pdf).

Pour r croissant **et** k décroissant, la construction publiée conserve
les coupes et les cellules entre elles. En notant d_min la profondeur
minimale d'une cellule découpée η :

\[
\operatorname{S\!\! -Rhomb}_{r,k}
=\{\eta:R_\eta\le r,\ d_{\min}(\eta)\ge k\}.
\]

Ce complexe croît dans les deux sens voulus. Ses équivalences avec la
multi-couverture sont naturelles ; elles portent sur les types d'homotopie
et leurs applications, pas sur l'égalité des réalisations dans R³.
[Corbet et al., définition p.27:11, théorèmes 9 et 11](https://drops.dagstuhl.de/storage/00lipics/lipics-vol189-socg2021/LIPIcs.SoCG.2021.27/LIPIcs.SoCG.2021.27.pdf).

Ainsi **on peut transporter l'identité de la composante**, par l'application
sur π₀ qui représente Ω_k(r)⊂Ω_{k−1}(r). Cela ne donne ni une inclusion
géométrique des deux mosaïques barycentriques ni un déplacement canonique
de leurs sommets. Plusieurs composantes peuvent arriver dans le même
parent. Pour le dessin, calculer le représentant à chaque ordre et garder
le lien de nœuds certifié ; ne pas reconstruire ce lien par projection ou
plus proche barycentre.

Ne pas conserver indistinctement tous les rhomboïdes de petit rayon en
oubliant la contrainte de profondeur. À l'inverse, une troncation naïve des
porteurs à la profondeur K peut supprimer des morceaux nécessaires à la
coupe K. En dimension 3 générique, un porteur peut monter jusqu'à k+3 pour
une cellule positive de la coupe k ; une grande coquille dégénérée invalide
cette marge fixe. Cela suit de p<k<p+|U| et |U|≤4 dans le cas générique.
Pour borner le modèle à K, employer une troncation démontrée, telle celle
de S-Del par télescopes, et non cette suppression locale.
[Corbet et al., version longue, §3.2](https://arxiv.org/pdf/2103.07823).

## 6. Q5 — Les boules critiques pondérées représentent leur propre union

Oui, (z_b,+r_b²) est le point pondéré associé à B(z_b,r_b). L'alpha
pondéré au paramètre zéro représente l'union des boules fournies ; au
paramètre de puissance s, leurs rayons deviennent √(r_b²+s).
C'est le lemme 6.1 de [BCY, §6.1.4](https://geometrica.saclay.inria.fr/team/Fred.Chazal/papers/CGLcourseNotes/main.pdf).
Ce résultat ne complète pas la géométrie manquante entre les événements H₀.

Pour des boules déjà nées, éligibles à l'ordre k, et correctement rattachées
au nœud v vivant à r, leurs centres appartiennent à C_v(r). On a seulement

\[
\bigcup_b\bar B(z_b,r_b)
\subseteq\bigcup_b\bar B(z_b,r)
\subseteq C_v(r)\oplus\bar B_r.
\]

Ces inclusions peuvent être strictes. Sur P={0,2}, k=2, l'unique boule
de naissance est B(1,1). À r=2, le point 4 appartient à Ω₂(2)⊕B₂,
mais ni à B(1,1) ni à B(1,2). Le catalogue des événements ne suit pas
la croissance continue entre leurs dates.

La perte peut aussi être topologique. À k=1, P={(0,0,0),(2,0,0),(1,2,0)}
et r²=5/4, les trois boules originales se rencontrent deux à deux,
sans intersection triple : Ω possède un cycle. Les boules diamétrales
critiques correspondantes ont au contraire un point commun (1,3/4,0) :
leur union est contractile. Cette comparaison porte sur Ω, pas sur son
offset, qui peut lui-même fermer des trous.

Enfin, dilater les composantes peut les fusionner : pour P={0,2,4}, k=2,
r=1, Ω={1,3}, mais B(1,1)∪B(3,1) est connexe. Conserver les labels HGP
avant dilatation. [Quatre preuves exactes et leur portée](proofs_ball_union/README.md)
incluent aussi deux nuages ayant la même boule éligible de naissance d'ordre 3
et des offsets différents ; ils n'ont pas la même tour entière.

Une construction exacte de l'offset est disponible :

\[
C_v(r)\oplus\bar B_r
=\bigcup_{Q\ \mathrm{attribué\ à}\ v}\bigl(C_Q(r)\oplus\bar B_r\bigr).
\]

Ces pièces sont convexes et leur nerf modélise **cet offset**. La dilation
ne commute pas en général avec l'intersection des boules : remplacer
chaque rayon r par 2r dans C_Q ne calcule pas cette formule.
Pour une approximation par boules, un ε-net certifié E_v⊂C_v(r) donne
E_v⊕B_r⊆C_v(r)⊕B_r⊆E_v⊕B_{r+ε}. Les centres critiques ne possèdent
pas automatiquement ce certificat ; la proximité métrique seule ne
garantit pas la conservation des trous.

## 7. Suite proposée au développeur

Construire d'abord un oracle géométrique borné de A_k(r), puis seulement
son accélération et sa réduction. Le raccord utile à FULL est précis :

1. Garder l'identité des k-parties, cellules, faces duales et incidences.
   Les coordonnées d'un barycentre ne sont pas un identifiant suffisant.
   Construire la mosaïque pour le nuage ambiant P ; la recalculer sur les
   seuls points attribués à un nœud change les concurrents de Voronoï.
2. Certifier a_σ par la minimisation convexe sur F_σ : témoin admissible,
   contraintes actives et optimalité. Avec les données rationnelles, un
   certificat exact peut utiliser les conditions de stationnarité et des
   multiplicateurs non négatifs. Ne pas remplacer cette date contrainte
   par la seule MEB du support dessiné. Traiter les égalités en un plateau.
3. Attribuer les cellules par C_Q et leurs incidences, puis vérifier que les
   composantes et applications entre coupes coïncident avec FULL. Le chemin
   « MEB de Q → descente datée → ancêtre à r » est également justifiable :
   l'intersection des k boules de Q reste convexe et contient C_Q(r).
   Ce raccord doit être qualifié ; tester la position de c_Q est exclu.
4. Ajouter les contre-épreuves {0,1,10}, {0,1,2,11}, le tétraèdre donnant
   l'octaèdre, les coquilles dégénérées et les contacts de dimension basse.
   Contrôler la couverture des faces et les incidences, pas seulement un
   volume total ou la caractéristique d'Euler.
5. Publier d'abord un rendu des faces exposées, avec les strates isolées et
   l'identité du nœud. Introduire ensuite les effondrements filtrés munis
   de leur certificat et d'un budget d'erreur géométrique si on les dessine.

La grande question encore ouverte n'est pas la définition du bon objet :
elle est fixée ci-dessus. C'est l'obtention, au coût visé sur LiDAR, d'un
représentant suffisamment petit **avec simultanément** une preuve de
topologie, les applications de la hiérarchie et une qualité géométrique
explicitement mesurée. Les boules FULL restent utiles pour les événements
et les rattachements ; elles ne fournissent pas, seules, ces trois garanties.

## Portée des vérifications

Les [preuves Q1/Q2](proofs/README.md) passent en Python normal et `-O`,
avec 265 contrôles rationnels ; les [quatre contre-épreuves Q5](proofs_ball_union/README.md)
passent également dans les deux modes. Elles corroborent les témoins et les
identités, sans prétendre tester un constructeur 3D général ou l'homotopie
par calcul. Les preuves générales sont celles rédigées ici ; les théorèmes
externes sont identifiés près de leur utilisation. Les empreintes du contexte
et des références locales figurent dans `sources.json`. Les captures
historiques restent inchangées.
