# Variables de nœud

Ce qu'un nœud de l'échelle porte en entrée du réseau, comment on le normalise,
et comment on décide expérimentalement de ce qui mérite d'y rester.

L'hypothèse HGP-UNet privilégie l'apport structurel — échelle, voisinage,
biais de position, axe des ordres — sans avoir encore mesuré son poids face
aux variables de nœud. Ce document propose un jeu riche et la procédure pour
en retirer ce qui ne se paie pas. Le [protocole de guidage FULL](GUIDAGE_FULL_ET_PREENTRAINEMENT_20260926.md)
distingue ces entrées du tokenizer des variables enseignantes réservées à la
perte de pré-entraînement.

## 1. Cinq familles, et la règle qui les sépare

> Une seule famille est normalisée. Les quatre autres gardent leurs unités.

C'est la règle de la présentation du 16 septembre, § « Ce que le code ne
remplace pas ». L'hypothèse du poster porte sur la **forme** ; elle n'a jamais
prétendu que la taille, la portée ou la rémission étaient sans intérêt. Les
normaliser hors d'existence rendrait le modèle aveugle au monde métrique.

Cette règle conserve l'information physique ; elle n'interdit pas une
standardisation numérique réversible des canaux avec unités et constantes
déclarées. Pour les logarithmes, utiliser un rapport à une unité fixe,
par exemple $\log(r/r_0)$.

| famille | normalisée ? | ce qu'elle capture |
| --- | --- | --- |
| 1. forme | **oui** (translation, échelle) | résumé géométrique dont la stabilité sous changement d'acquisition reste à mesurer |
| 2. physique | non | dimensions, pose, position dans la scène |
| 3. filtration | non | la trajectoire d'échelle : naissance, mort, fusion, ordre |
| 4. acquisition | non | la qualité de la mesure |
| 5. sémantique | — | c'est la sortie, jamais l'entrée |

### Équivariance plutôt qu'invariance

Conserver la taille physique tout en normalisant la forme est le choix à
tester. Donner $\log(r/r_0)$ permet au modèle d'utiliser la métrique ; cela ne
rend pas à lui seul le réseau équivariant. Distinguer la covariance de la
tour, la transformation déclarée des variables et le comportement appris.
Un changement de capteur modifie aussi le balayage et la visibilité : il
n'est pas une homothétie. Ablater le canal métrique à acquisition appariée,
puis mesurer le transfert avec réglages gelés.

## 2. Famille 1 — forme

### La réalisation doit être choisie avant le descripteur

Le catalogue contient une boule canonique, son $q_{\min}$, ses intérieurs $I_b$
et toute sa coquille $U_b$ ; **un support représentant n'est pas une géométrie
canonique**. Pour quatre points aux coins d'un carré, chacune des deux
diagonales est un support q2 de la même boule. Choisir la première selon les IDs
modifie distances et moments, alors que FULL n'a pas changé.

Le premier pilote compare les mêmes snapshots avec trois encodages :

1. **Retours observés** : statistiques des sites couverts, avec la mesure de
   retours déclarée. C'est le socle peu coûteux, disponible aussi pour un
   singleton K1.
2. **Boules et supports** : d'abord histogramme de $q_{\min}$ **par boule
   distincte**, puis, en option, réalisation de **tous les supports positifs
   minimaux par inclusion**. Les arités de tous ces supports ne se déduisent
   pas de $q_{\min}$ seul.
3. **Surfels observés** : normales, incertitude et visibilité, calculées sur
   les retours ; cette mesure physique estimée reste séparée des témoins HGP.

Pour l'option 2, définir explicitement
$\mathcal Q_b=\{Q\subset U_b:c_b\in\mathrm{relint}(\mathrm{conv}(Q)),
Q\text{ affinement indépendant},2\leq |Q|\leq4\}$
et $P_v^{\mathrm{supports}}=
\bigcup_{b\in B_v}\bigcup_{Q\in\mathcal Q_b}\mathrm{conv}(Q)$.
Les clés de boules de $B_v$ sont dédupliquées et rattachées au **snapshot daté**
avec leur rôle d'événement déclaré. Les ensembles $\mathcal Q_b$ demandent
un export ou une reconstruction ; ils ne sont pas déjà des champs de FULL.
La [ShellTable native](../../morsehgp3D_v9/src/tower/forest/local_plateau.hpp)
énumère ces supports dans son domaine de coquille.

$P_v^{\mathrm{supports}}$ est un **squelette de témoins**, distinct des
populations, des facettes pondérées du § 9.1 et d'une surface physique. Dans
le carré, il forme les deux diagonales ; $\mathrm{conv}(U_b)$ forme le carré
rempli : ce serait une autre réalisation, à tester sous un autre identifiant.
Un point intérieur à une boule peut manquer au squelette. Un snapshot sans
boule n'a aucun support : utiliser le canal retours et un masque
`has_support_geometry=false`, sans fabriquer de distance finie à l'ensemble
vide. La [fixture rationnelle](reference/verify_support_carrier.py) montre
l'ambiguïté de choix et la différence avec le remplissage convexe.
Dans les formules de distances ci-dessous, $P_v$ désigne la réalisation
**non vide** explicitement retenue, avec son identifiant de construction.

**Signature d'arité** — trois comptes de boules par $q_{\min}$ et leurs
proportions. Cette signature canonique peu coûteuse n'est ni le nombre de
supports positifs ni une estimation de dimension physique. Une signature des
supports énumérés porte un autre nom, un autre dénominateur et son coût.

**Moments par dimension** — pour chaque famille de primitives, choisir soit
la mesure d'incidences $\nu_d=\sum_{Q:|Q|=d+1}a_Q
\mathcal H^d|_{\mathrm{conv}(Q)}$, avec $a_Q$ déclaré, soit la mesure de leur
union. La première compte les recouvrements avec multiplicité ; elle est
additive et plus simple à calculer. La seconde exige de traiter les
intersections. Elles ne donnent pas les mêmes moments. Une aire de bord de
tétraèdres demande encore une autre construction.

Jusqu'à l'ordre 3, une mesure a **19 monômes non constants** (3+6+10) plus
sa masse ; trois dimensions séparées donnent jusqu'à 60 canaux. Garder les
moments dans le repère capteur au premier pilote. Un repère de vecteurs propres
devient ambigu aux valeurs propres multiples et peut changer de signe :
aucune stabilité de la normalisation par PCA n'est acquise.

**Histogramme sphéro-radial** des supports bidimensionnels ou des surfels
observés, explicitement distingués, sur $\mathbb{S}^2 \times \mathbb{R}$.
Il résume les couches présentes dans la mesure choisie, y compris plusieurs
intersections par direction. Il ne reconstitue pas la surface physique cachée.

**Grille de distances à sondes fixes** — $8^3 = 512$ sondes,
$D_P[j] = d(c + s\,b_j, P_v)/s$, la distance étant prise aux primitives
entières. La distance continue vérifie
$\left\Vert d_P-d_Q\right\Vert_\infty=d_H(P,Q)$ ; pour un repère
$(c,s)$ **commun**, 512 sondes ne donnent que
$\max_j|D_P[j]-D_Q[j]|\le d_H(P,Q)/s$,
avec des collisions possibles. L'identité d'union
$d_{P\cup Q}=\min(d_P,d_Q)$ s'applique aux **mêmes requêtes physiques** :
des grilles de parents recentrées ou renormalisées ne se combinent pas case
à case, et une fusion peut ajouter de nouveaux supports. Une grille finie peut
manquer un petit élément. Garder donc les primitives et les niveaux fins,
et comparer la grille aux descripteurs moins coûteux.

Pour deux réalisations dans le **même** cube normalisé $[-1,1]^3$ et les centres des
$8^3$ cellules, poser $E=\max_j|D_P[j]-D_Q[j]|$. Le rayon de couverture
des sondes est $\delta=\sqrt{3}/8$ ; les distances sont 1-Lipschitz, donc
$E\leq d_H(P,Q)/s\leq E+2\delta$. Cette borne dérivée ici est grossière :
l'incertitude maximale vaut environ $0{,}433s$ en mètres. Elle ne se
transfère pas aux normalisations indépendantes. Mesurer la sensibilité aux
éléments minces ; des sondes adaptatives près des primitives sont une option
à comparer, avec leur coût.

## 3. Famille 2 — physique

Rayon de naissance en mètres ; dimensions de la boîte ; aires des supports
bidimensionnels ou des surfels observés, identifiées séparément ; dispersion
transverse (racine de la plus petite valeur propre de la covariance, en
longueur, avec la mesure déclarée) ;
centre en repère capteur ; hauteur au-dessus du capteur ; **portée** et azimut ;
nombre de retours couverts, séparé en intérieur et coquille.

La portée doit être testée comme canal explicite : elle peut aider à séparer
effet de l'acquisition et structure locale. Son apport propre est à ablater
avec profils de rayons K-NN, comptes multi-rayons et anisotropie. Le score
$K/(n\omega_3r_K^3)$ est une concentration ambiante, pas automatiquement une
densité physique de surface ; le témoin T2 de [MESURE](MESURE.md) explicite
ces comparaisons conditionnelles.

## 4. Famille 3 — filtration

Les variables dérivées de l'histoire FULL sont aussi des cibles des
tâches de pré-entraînement, ce qui impose une précaution : **une variable
utilisée comme cible en pré-entraînement ne doit pas être donnée en entrée au
même moment.**

La restriction porte aussi sur les parents, degrés, choix des voisins et
biais d'attention qui révèlent la réponse. Dans le bras tokenizer, ces objets
viennent de la vue élève recalculée ; les objets de la tour enseignante restent
dans la perte. Élaguer cette dernière après masquage ne suffit pas.

Naissance, mort, persistance et écart logarithmique ; rang et quantile du
niveau dans la trame — une moindre dépendance au capteur est une hypothèse,
car visibilité et composition de scène modifient aussi les rangs ; gain de
masse à la fusion ; **degré de multifusion** ;
nombre d'enfants dans l'arbre condensé ; **ordre $K$** et image verticale — son
identité, son niveau, sa taille ; excès de masse du nœud.

## 5. Famille 4 — acquisition

Rattachée par identifiant de point, hors moteur : rémission (moyenne,
écart-type, quantiles) ; anneaux couverts et couverture angulaire ; incidence
estimée depuis les moments ; proxy d'occultation, part du secteur angulaire
sans retour. Un secteur sans retour n'établit ni absence de surface ni espace
libre ; conserver cette incertitude dans les comparaisons entre vues.

## 6. Taille et format

| famille | dimensions |
| --- | --- |
| grille de distances $8^3$ | 512 |
| moments jusqu'à l'ordre 3 | 20 par mesure, jusqu'à 60 pour trois dimensions |
| histogramme sphéro-radial | 96 à 128 |
| signature d'arité | 6 |
| physique | ≈ 20 |
| filtration | ≈ 20 |
| acquisition | ≈ 15 |
| **total** | **≈ 690 à 760**, selon les mesures effectivement retenues |

Stockage envisagé : float16 pour la famille 1, entiers exacts pour ce qui
l'est déjà (arités, comptes, ordre $K$, rangs). Les descripteurs flottants sont
calculés approximativement **après l'export exact** ; leur erreur et leur
saturation doivent être mesurées. Ils ne font pas partie de la chaîne de
prédicats exacts du moteur.

**La grille de distances à 512 dimensions est le poste à interroger en
premier.** Elle vaut à elle seule les trois quarts du descripteur ; si la
signature d'arité et les moments la remplacent sans perte, on divise le cache
par quatre.

## 7. Comment on décide ce qui reste

Par ablation XGBoost, porte 0.4 de [`MESURE.md`](MESURE.md) : chaque famille
seule ; toutes sauf une ; contre une base triviale (hauteur, taille, portée) ;
contre une base « points » sans hiérarchie ; **avec et sans la famille
filtration**. Un canal qui ne bat pas la base triviale d'une marge annoncée
d'avance est retiré, quelle que soit son élégance.

## 8. Références du codage

- [BPS 19] Prokudin, Lassner, Romero, *Efficient Learning on Point Clouds with
  Basis Point Sets*, ICCV 2019 — le cadrage et les sondes fixes.
- [Schneider 13] Schneider, *Convex Bodies : The Brunn–Minkowski Theory*,
  2e éd., 2013 — la fonction support, et la raison de ne pas l'employer seule :
  $h_P = h_{\mathrm{conv}(P)}$.
- [PolyhedronNet 25] Yu, Zhang, Zhao, *Representation Learning for Polyhedra
  with Surface-Attributed Graph*, ICLR 2025 — l'antécédent le plus proche pour
  un encodeur natif sur polyèdres.
- Manuscrit, parties I–II et § 9.1 — l'objet, les supports de Gabriel, le vote
  pondéré.
