# Spécification du tokenizer

26 septembre 2026. Le contrat algorithmique de la partie non apprise : ce qui
entre, ce qui sort, dans quel ordre, et ce qui doit être reproductible. Écrit
pour être implémenté sans avoir à deviner.

La motivation de chaque choix est dans [`ARCHITECTURE.md`](ARCHITECTURE.md) ;
ici, seulement le quoi.

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

Trois variables supplémentaires sont autorisées parce que la tour les produit ou
les suppose, et chacune est ablatable séparément : le **niveau de sortie**
$\hat\lambda_x$ rendu par la condensation, la **portée**, et l'**indice
d'anneau**.

## 2. Condensation

Entrée : pour un ordre $K$, la forêt de fusion et les populations. Sortie :
l'arbre condensé, sa stabilité par nœud, et $\hat\lambda_x$ par point.

### 2.1 Masse

La masse d'une facette est celle du § 9.1 du manuscrit :

$S_\tau = \sum_{\sigma \supset \tau} \psi(\rho(\sigma)), \quad \psi(t) = 1/t^{3}, \quad T_x = \sum_{\tau \ni x} S_\tau, \quad m_\tau = S_\tau \sum_{x \in \tau} 1/T_x,$

avec la convention $1/T_x = 0$ si $T_x = 0$. Jamais un comptage de facettes. La
masse d'un nœud est la somme des $m_\tau$ de ses facettes.

### 2.2 Règle d'élagage

Une scission n'est validée que si **chaque branche conserve au moins une
fraction $\alpha$ de la masse du parent**. Sinon les branches légères tombent —
leurs points reçoivent $\hat\lambda_x$ = le niveau de l'événement — et la
branche lourde garde l'identité du parent.

$\alpha$ est un **rapport** sans dimension ; sa transférabilité reste à
tester. Un seuil en nombre de points dépend directement de la densité des
retours et ne convient pas comme défaut inter-capteurs.

### 2.3 Multifusions

La tour publie des événements à trois parents ou plus au même niveau exact. La
règle s'y généralise : à une multifusion, **toutes** les branches sous le seuil
tombent simultanément, et les branches au-dessus créent chacune un nœud
condensé. **Ne jamais binariser** : cela inventerait un ordre qui n'existe pas et
détruirait la canonicité.

Conséquence : un nœud condensé peut avoir trois enfants ou plus, et le
programme dynamique de sélection (SEL) doit l'admettre — la forme du § 5.2 du
manuscrit, $\mathrm{loss}(\text{père})$ contre $\sum_i \mathrm{loss}(\text{fils}_i)$,
le fait déjà.

### 2.4 Stabilité

$\widehat{E}(C) = \sum_{x \in C} \left(\hat\lambda_x - \hat\lambda_{\min}(C)\right)$,
pondérée par la masse quand un point appartient à plusieurs branches. Sert
d'ordre de contraction et de coût de référence pour SEL.

### 2.5 Couplage entre ordres

Le même $\alpha$ pour tous les ordres, et **revérification de la naturalité des
cartes verticales après condensation** : l'image d'une fusion doit rester la
fusion des images. Ce n'est pas automatique — une fusion retirée à $K$ mais
conservée à $K-1$ peut rompre le carré. C'est une porte, pas une remarque.

### 2.6 Coût

Linéaire en nombre de nœuds de la forêt, une passe ascendante.

## 3. L'échelle

### 3.1 Sens de parcours

Grossir, c'est $r$ **croissant** et $K$ **décroissant** ; c'est la seule
direction où les ensembles de niveau s'emboîtent, donc la seule où un pooling
existe. Voir [`ARCHITECTURE.md`](ARCHITECTURE.md) § 4.2.

### 3.2 Règle de contraction

Quatre règles, toutes appliquées **sur l'arbre condensé** :

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
   deux : une multifusion est prise entière. Les cibles de compte sont donc des
   **bornes supérieures**, et le compte réalisé est publié.
2. **Départage par clé canonique, jamais par `PointId`.** À critère primaire
   égal, on ordonne par la **clé primitive de la boule** — la forme quadratique
   $(A, B, C)$ du catalogue — qui est une fonction pure des coordonnées.
   Ordonner par `PointId` casserait l'invariance au réétiquetage, que le moteur
   garantit et qu'il faut préserver.

### 3.4 Cibles de compte

$L$ niveaux, rapport $\approx 4$ par niveau, borne supérieure par niveau. Les
comptes réalisés varient d'une trame à l'autre ; c'est attendu et c'est la
mesure même de l'adaptativité, qu'il faut publier (histogramme du rayon effectif
par niveau et par tranche de portée).

## 4. Matrices de pooling

Trois interfaces, et **une seule est douce**.

### 4.1 Points vers niveau 1 — douce

L'arbre est un arbre de **facettes** ; une antichaîne le partitionne ; les poids
du § 9.1 poussent cette partition en une partition de l'unité sur les points :

$P_1[x, v] = \sum_{\tau \in v,\ \tau \ni x} S_\tau / T_x, \qquad \sum_v P_1[x, v] = 1.$

Cette identité suppose $T_x>0$ et une antichaîne qui couvre toutes les
facettes incidentes retenues. Si un retour n'a pas de facette au niveau choisi
ou perd sa branche à la condensation, il conserve un jeton de repli/connexion
de saut explicitement compté : aucun retour ne disparaît en silence. C'est
ici que le recouvrement pour $K \geq 2$ se manifeste. Publier les non-zéros
de $P_1$, la masse $M_v=\sum_xP_1[x,v]$ et la couverture par portée et K.
Une moyenne de jeton utilise $\sum_xP_1[x,v]h_x/M_v$ lorsque $M_v>0$ ; la
somme brute encode également la population et se mesure séparément.

### 4.2 Niveau $\ell$ vers niveau $\ell+1$ — dure

Deux antichaînes emboîtées du même arbre : chaque nœud fin a **exactement un**
ancêtre grossier. $P_{\ell+1}$ est une matrice $0/1$ à une entrée par ligne.

### 4.3 Ordre $K$ vers ordre $K-1$ — dure

La carte verticale FULL est publiée au niveau fermé de création. Deux
antichaînes adaptatives de K et K−1 ne possèdent une application dure que si
chaque nœud source a une image contenue dans un unique nœud cible aux rayons
retenus. Construire ce quotient et vérifier sa commutation avec les cartes
FULL ; sinon publier une incidence sparse entre branches pour OM, sans parent
unique inventé.

### 4.4 Dépliage

Transposée, avec connexion de saut. Aucune interpolation n'est introduite : le
retour final aux points est PUR, c'est-à-dire $P_1$ lui-même.

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
borné en CSR avec ses $r_{uv}$, et $\hat\lambda_x$ par retour couvert.
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
3. **Couverture et partition de l'unité** : chaque retour retenu a soit
   $\sum_v P_1[x,v]=1$, soit un repli explicite de masse 1 ; aucun orphelin.
4. **Emboîtement** : chaque nœud du niveau $\ell$ a exactement un ancêtre au
   niveau $\ell+1$.
5. **Naturalité après condensation** : l'image d'une fusion est la fusion des
   images.
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
