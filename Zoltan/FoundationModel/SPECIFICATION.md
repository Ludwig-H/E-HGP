# Spécification du tokenizer

26 septembre 2026. Le contrat algorithmique de la partie non apprise : ce qui
entre, ce qui sort, dans quel ordre, et ce qui doit être reproductible. Écrit
pour être implémenté sans avoir à deviner.

La motivation de chaque choix est dans [`ARCHITECTURE.md`](ARCHITECTURE.md) ;
ici, seulement le quoi.

Le [contrat des coupes et masses](CONTRAT_COUPES_ET_MASSES_20260926.md)
précise les conditions de composition, les réserves et les profils d'export.
Son petit oracle est un falsificateur d'algèbre ; l'export natif reste à faire.

## 0. Portée et invariants

Entrée : une trame LiDAR et son odométrie. Sortie : $L$ matrices de pooling,
un graphe de fusion par niveau, des variables par nœud, et les cibles de
pré-entraînement. **Aucun apprentissage n'intervient dans ce document.**

Quatre invariants doivent tenir, et chacun mérite sa porte :

| invariant | pourquoi |
| --- | --- |
| **Déterminisme bit à bit** — deux exécutions sur la même entrée donnent le même condensé | c'est ce qui rend l'étude de substitution reproductible |
| **Invariance au réétiquetage** — permuter les `PointId` ne change rien | les `PointId` sont arbitraires en v9, et le moteur a déjà une porte pour cela |
| **Totalité** — aucune trame ne sort sans jeu de niveaux | une trame rejetée en silence biaise le corpus |
| **Traçabilité** — chaque sortie porte le condensé de la tour dont elle vient | sinon aucun reçu n'est vérifiable |

## 1. Niveau 0 — les points

Pour l'étude de substitution, **les variables d'entrée au niveau des points sont
exactement celles de la référence**, c'est-à-dire $(x, y, z, \text{rémission})$
comme PTv3. Changer l'entrée en même temps que la hiérarchie rendrait tout écart
inattribuable.

Trois variables supplémentaires sont ablatables séparément : une agrégation
déclarée des **niveaux de sortie par incidence**, la **portée**, et l'**indice
d'anneau** fourni par l'acquisition. Un point couvert par plusieurs branches
n'a pas nécessairement un unique niveau de sortie.

## 2. Condensation

Entrée : pour un ordre K, la forêt FULL, l'univers pondéré et les naissances
propres de ses facettes. Sortie : arbre condensé, stabilité par nœud, réserves
et niveaux de sortie par incidence/branche. Les seules populations FULL ne
suffisent pas à calculer les poids.

### 2.1 Masse

Fixer les cofaces Gabriel contributrices C, leurs frontières F, l'horizon et
la fonction ψ. La masse d'une facette est celle du § 9.1 du manuscrit :

$S_\tau = \sum_{\sigma \supset \tau} \psi(\rho(\sigma)), \quad \psi(t) = 1/t^{3}, \quad T_x = \sum_{\tau \ni x} S_\tau, \quad m_\tau = S_\tau \sum_{x \in \tau} 1/T_x,$

avec la convention $1/T_x = 0$ si $T_x = 0$ et une réserve explicite pour ces
sites. La masse d'un nœud est la somme des masses de ses facettes affectées
à la coupe, avec une convention site/retour déclarée. Les poids restent gelés
entre les niveaux qui revendiquent une composition. Les niveaux carrés exacts
ne rendent pas les poids p3 rationnels ; publier le statut numérique.

La formule de $m_\tau$ ci-dessus est celle du site unitaire. Pour une mesure
μ différente, employer $m_\tau^\mu=\sum_{x\in\tau}\mu_xw_{x\tau}$ et garder
cette même convention dans la condensation, le pooling et la lecture.

### 2.2 Règle d'élagage

À une scission, calculer simultanément les branches dont la masse est au moins
une fraction α de celle du parent. Zéro branche lourde termine le segment ;
une seule continue son identité ; au moins deux créent une scission. Les
branches légères passent en réserve avec leurs niveaux de sortie par
incidence. La masse directement attachée à l'événement est traitée à part.

$\alpha$ est un **rapport** sans dimension ; sa transférabilité reste à
tester. Un seuil en nombre de points dépend directement de la densité des
retours et ne convient pas comme défaut inter-capteurs.

### 2.3 Multifusions

La tour publie des événements à trois parents ou plus au même niveau exact.
La règle 0/1/plusieurs branches lourdes s'y applique en une seule fois.
**Ne jamais binariser** : cela inventerait un ordre qui n'existe pas.

Conséquence : un nœud condensé peut avoir trois enfants ou plus, et le
programme dynamique de sélection (SEL) doit l'admettre — la forme du § 5.2 du
manuscrit, $\mathrm{loss}(\text{père})$ contre $\sum_i \mathrm{loss}(\text{fils}_i)$,
le fait déjà.

### 2.4 Stabilité

La stabilité intègre la masse du segment dans la coordonnée de densité
déclarée : $\widehat{E}(C)=\int M_C(\hat\lambda)\,d\hat\lambda$.
Le calcul discret somme les durées de présence **par incidence pondérée**,
avec leurs entrées et sorties propres. Une moyenne des niveaux de sortie par
point ne remplace pas cette trace. La stabilité sert de coût de référence
pour SEL et de variante d'ordre de contraction.

### 2.5 Couplage entre ordres

Le même α peut être testé pour tous les ordres ; cela n'assure pas la
compatibilité des quotients. Vérifier la factorisation
$q_{K-1}\circ f_K=V_K\circ q_K$ sur les états datés effectivement consommés.
Une fusion peut déjà avoir eu lieu dans l'ordre inférieur. La commutation
pondérée $P_KV_K=P_{K-1}$ est une condition supplémentaire, jamais déduite
de la seule naturalité.

### 2.6 Coût

La passe de condensation peut être linéaire en nœuds **après** calcul des
masses et affectations. Compter séparément le flux de cofaces/incidences,
les résolutions temporelles et les réserves.

## 3. L'échelle

### 3.1 Sens de parcours

Grossir avec garantie d'inclusion géométrique demande r croissant et K
décroissant. Cela ne garantit pas la composition des poids entre K.
Voir [`ARCHITECTURE.md`](ARCHITECTURE.md) § 4.2 et le contrat des masses.

### 3.2 Règle de contraction

Le pilote de raccord commence par des coupes globales sans condensation.
Les quatre règles de recherche seront ensuite comparées sur les arbres
condensés, avec univers et masses déclarés :

| règle | clé primaire de contraction |
| --- | --- |
| `E-global` | niveau $r$ de l'événement, seuils globaux |
| `E-rang` | niveau $r$, jusqu'à une cible de compte |
| `E-persistance` (défaut) | persistance croissante |
| `E-relative` | niveau normalisé $r / r_K(x)$ |

Pour `E-relative`, définir $r_K(x)$ sur les sites uniques et imposer un
dénominateur positif : si la convention des voisins inclut $x$ et donne
$r_1(x)=0$, utiliser la distance au plus proche site **distinct** ; pour une
trame à un seul site, déclarer cette règle indisponible et employer `E-global`.
Publier la convention et le nombre de replis.

### 3.3 Départage, et pourquoi il n'est pas un détail

Les niveaux sont des rationnels exacts, et **beaucoup d'événements partagent
exactement le même niveau** (plateaux cosphériques). L'ordre de contraction
n'est donc pas total, et la canonicité — qui est l'argument de vente du projet —
dépend entièrement de la façon dont on tranche.

Deux règles :

1. **Contraction atomique par événement.** On ne coupe jamais un événement en
   deux : une multifusion est prise entière. Publier le compte réalisé et le
   statut de faisabilité du budget.
2. **Départage par clé canonique, jamais par `PointId`.** À critère primaire
   égal, on ordonne par une **clé canonique d'événement** ; si plusieurs
   boules contribuent au même événement, une seule clé de boule ne suffit
   pas. La clé peut être définie par le tuple canonique des boules concernées.
   Ordonner par `PointId` casserait l'invariance au réétiquetage, que le moteur
   garantit et qu'il faut préserver.

### 3.4 Cibles de compte

L niveaux, rapport indicatif de 4, budget annoncé par niveau. Les multifusions
peuvent faire sauter une cible de compte ; les racines indépendantes et réserves
persistantes imposent un plancher. Un budget inférieur à ce plancher est déclaré
irréalisable, sans fusion artificielle. Publier comptes et rayons effectifs.

## 4. Matrices de pooling

Trois interfaces : affectation des points, quotient horizontal, échange entre K.

### 4.1 Points vers niveau 1 — douce

L'arbre porte des facettes pondérées ; une frontière **couvrante** de l'univers
retenu, réserves comprises, donne une partition de l'unité sur les points :

$P_1[x, v] = \sum_{\tau \in v,\ \tau \ni x} S_\tau / T_x, \qquad \sum_v P_1[x, v] = 1.$

Cette identité suppose T positif et couverture de l'univers pondéré. Ajouter
une réserve pour **toute masse manquante**, même si une partie du vote subsiste.
Exemple : survivant 1/4 et réserve 3/4, sans renormaliser le survivant.
Déclarer la mesure μ, par site ou par retour ; publier les non-zéros et
$M_v=\sum_x\mu_xP_1[x,v]$.
La moyenne vaut $\sum_x\mu_xP_1[x,v]h_x/M_v$ pour M positif. Les colonnes de
masse nulle sont supprimées ou masquées.

### 4.2 Niveau $\ell$ vers niveau $\ell+1$ — dure

À univers et poids gelés, deux partitions atomiques dont chaque bloc fin a
une unique image grossière fournissent un quotient Q à une entrée 1 par ligne.
Vérifier le témoin sur les atomes puis $P_g=P_fQ$. Transporter les masses
avec les moyennes : $M_g=Q^\top M_f$ et
$h_g=D_{M_g}^{-1}Q^\top D_{M_f}h_f$.

Le pilote fige les atomes représentés depuis le niveau fin et conserve leurs
réserves ; les facettes qui naissent ensuite alimentent une lecture latérale.
Une réserve par site ne peut pas être scindée par un quotient dur.

### 4.3 Ordre K vers K−1 — échange latéral par défaut

La carte verticale FULL est publiée au niveau fermé de création. Deux
antichaînes adaptatives de K et K−1 ne possèdent une application dure que si
chaque nœud source a une image contenue dans un unique nœud cible aux rayons
retenus. Construire ce quotient et vérifier sa commutation avec les cartes
FULL ; sinon publier une incidence sparse entre branches pour OM, sans parent
unique inventé.

Cette propriété géométrique ne prouve pas $P_KV_K=P_{K-1}$ : les univers
de facettes et leurs poids diffèrent entre ordres. Les branches gardent leurs
masses propres. Un pooling conservatif inter-K doit fermer ce contrôle
supplémentaire.

### 4.4 Dépliage

La prolongation du niveau grossier vers le fin utilise Q ; le retour final
utilise P1. PUR mélange les **probabilités par jeton**, puis applique le
vote/argmax. Mélanger les logits est un autre opérateur à nommer.
La composition moyenne puis prolongation conserve les constantes mais ne
reconstruit généralement pas les points ; les connexions de saut restent
nécessaires à une représentation fine.

## 5. Graphe de fusion et biais ultramétrique

Par niveau : sommets = nœuds du niveau. Définir une **relation éparse** liée
au prochain événement de fusion, avec traitement atomique des multifusions,
identité et budget des arêtes publiés. La relation « fusionnent un jour »
formerait un clique dans chaque arbre et ne suffit pas comme spécification.

Le voisinage d'attention est borné par un budget déclaré. Le biais
$b_{uv}=\varphi(\log r_{uv}-\log r_u)$ garde un sens relatif sous
homothétie commune des rayons positifs ; l'invariance à une raréfaction des
retours avec la portée reste à mesurer. Définir séparément le code des niveaux
à rayon zéro, notamment à K=1.

$r_{uv}$ est une **ultramétrique** (équivalence dendrogramme–ultramétrique,
chapitre 3 du manuscrit) : $r_{uw} \leq \max(r_{uv}, r_{vw})$. L'implémentation
peut s'en servir pour élaguer.

## 6. Ordres

$K_{\max}$ est fixé **par la mesure**, pas par le domaine du moteur : la porte
0.8 de [`MESURE.md`](MESURE.md) dit si les ordres $7$ à $10$ servent. Par
défaut, $K \in \lbrace 1, 2, 3, 5 \rbrace$ en branches parallèles, fusionnées
latéralement par les cartes verticales.

$K = 1$ reste le témoin de sensibilité maximale : c'est le Single-Linkage.
Les ordres qui préservent effectivement les objets minces et lointains seront
choisis par la mesure.

## 7. Vues augmentées

**On recalcule la tour par vue.** La quantification à 1 mm casse la
commutation : `tourner → quantifier → tour` n'est pas `tour → tourner`. Les
prédicats étant exacts, un accrochage différent à la grille peut faire basculer
une égalité.

Banque par trame : rotations autour de $z$, décimations en portée selon le
modèle de balayage, retraits d'anneaux, occultations par secteur. Plus, pour
FM-6, la tour de l'**agrégat multi-trames** recalé par l'odométrie.

## 8. Format et lots

Par trame et par ordre : l'arbre condensé en CSR, les niveaux exacts et leur
code flottant pour le réseau, les populations en CSR de sites uniques, les
cartes verticales, les $L$ matrices de pooling en CSR, le graphe de fusion
borné en CSR avec ses $r_{uv}$, et les sorties par incidence avec leur
agrégation déclarée par retour.
Conserver la table **retour original → site unique**, les attributs capteur,
les retours retirés par un éventuel masque de sol, la provenance de la vue et
les condensés du brut, de la préparation, de FULL et de l'export.

Mise en lots : concaténation avec décalages et matrices diagonales par blocs,
comme tout réseau épars. Les comptes par niveau étant des bornes supérieures,
les formes sont proches d'une trame à l'autre ; on complète et on masque.

Stockage : entiers exacts pour ce qui l'est (arités, comptes, ordre, rangs),
`float16` pour les variables de forme. Le flottant est une sortie, jamais un
maillon de la chaîne d'exactitude.

## 9. Portes du tokenizer

Avant tout apprentissage, et chacune à code de sortie exact, dans la discipline
du dépôt :

1. **Déterminisme** : deux exécutions, même condensé.
2. **Réétiquetage** : permuter les `PointId`, même sortie — la porte qui
   attrape un départage illicite.
3. **Couverture et partition de l'unité** : pour chaque retour, affectations
   et réserves **partielles** totalisent 1 ; aucun orphelin.
4. **Emboîtement** : chaque nœud du niveau $\ell$ a exactement un ancêtre au
   niveau $\ell+1$.
5. **Naturalité après condensation** : factorisation des cartes sur les
   états datés retenus ; contrôle pondéré distinct entre K.
6. **Totalité** : aucune trame du corpus ne sort sans jeu de niveaux.
7. **Traçabilité** : chaque sortie porte le condensé de la tour dont elle vient.
8. **Morphismes des coupes** : composition des matrices entre niveaux et
   commutation des quotients inter-K avec les verticales FULL, ou incidence
   sparse explicitement déclarée.
9. **Budget** : comptes réalisés de jetons, non-zéros CSR, arêtes, octets et
   temps d'export sur une trame brute entière.
10. **Stabilité du tokenizer** : comparer affectations sous décimation,
    rotation suivie d'une nouvelle quantification et changement de capteur.
11. **Mutants** : au moins un mutant causal par porte — départage par `PointId`,
   binarisation d'une multifusion, seuil absolu au lieu de relatif — compilé et
   tué.

## 10. Ce que ce document ne couvre pas

Le réseau, ses pertes et ses recettes : [`ARCHITECTURE.md`](ARCHITECTURE.md).
Les variables de nœud : [`JETON.md`](JETON.md). Les expériences :
[`MESURE.md`](MESURE.md).
