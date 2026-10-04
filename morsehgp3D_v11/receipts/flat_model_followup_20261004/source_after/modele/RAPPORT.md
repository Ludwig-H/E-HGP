# Sortie plate du modèle v11 : condensation, existence, sélection et complétion de $H^{r}_{k+1}$

4 octobre 2026, rédigé à partir de 07 h 56 UTC (heure lue par `date -u`). Rôle : mathématicien du modèle, workflow
« v11-points-select ».

```text
phase=exploration_v11_hors_registre
backend=cpu_reference (oracle exact de la définition, nuages de 5 à 27 sites ; scikit-learn 1.9.1 local, pas 1.7.2 de G4)
profile=quantized_u21_input_only
public_status=not_claimed
GCP non utilisé. Aucune commande git qui écrit. Aucune construction native. Aucun réseau.
Écritures : build/v11-points-select/modele/ seulement (RAPPORT.md, scripts/, sorties/, SHA256SUMS).
```

**Consigne suivie.** On travaille dans la v11 ; la v10 est une source de données et de contre-exemples, jamais une
autorité ; la thèse et l'auditeur non plus. Toute affirmation mathématique ci-dessous porte un statut : prouvé (preuve
écrite ici), contre-exemple exact (fixture et script), ou conjecture. Les petits nuages sont des oracles de
correction : ils n'établissent ni fréquence ni pente. Rien ici n'a été mesuré aux tailles d'intérêt : la section 7
écrit les prédictions **avant** la mesure.

## En bref

**Décisions proposées.**

1. **Condensation** : la hiérarchie condensée à mcs est l'**atteignabilité mutuelle de l'ultramétrique $u$ de
   $H^{r}_{k+1}$ à `min_samples` = mcs** : $\tilde u(i,j)=\max(u(i,j),R_i,R_j)$, où $R_i$ est la mcs-ième plus petite
   valeur de la ligne $i$ de $u$ (diagonale comprise). C'est exactement « blocs d'au moins mcs points » (prouvé,
   proposition 1, et contrôlé sur 103 428 comparaisons). Arbre N-aire, plateaux atomiques, jamais binarisé.
2. **Masse et existence (P4)** : critère **A**, sites entrés comptés 1. C'est le seul critère sans paramètre étudié qui
   soit à la fois monotone, 3ε-stable, conforme à T0 et à la règle « clusters = blocs d'au moins mcs points » (la maturité
   $E_\theta$ ne le reste que pour $\theta\leq 1/15$, et perd Q-Π2 comme A). Il **perd Q-Π2**, et ce
   n'est pas un défaut de réglage : toute règle de comptage locale au profil qualifié, à comptage immédiat sans rival,
   doit avoir une constante de stabilité supérieure à 6,99 pour tenir Q-Π2 (théorème 6). La perte vient d'ailleurs de
   la projection (x va à l'amas, perte Q3 déjà publiée), pas de la condensation.
3. **Sélection** : excès de masse N-aire (programme dynamique d'antichaîne, linéaire) avec $\lambda=r^{-3}$, le niveau
   de densité K-NN de $L_k(r^{2})$ dans $\mathbb{R}^{3}$ ; $z=1$ publié à côté (consigne). **z est un cadran de
   granularité, et c'est maintenant un théorème** : la sélection à $z_2>z_1$ raffine celle à $z_1$ (théorème 9 ;
   0 violation sur 9 000 paires). Racine non sélectionnable, parent aux ex æquo, décisions certifiées.
4. **Complétion** : aucune par défaut ; la complétion **par lignée** est une variante propre à la tour (fixture où elle
   rend à x son amas alors que la complétion ultramétrique générique le laisse bruit).
5. **Équité** : appliquer la même condensation N-aire et le même EOM à l'ultramétrique de HDBSCAN. Sans ex æquo, cela
   reproduit scikit-learn exactement (135/135) ; avec ex æquo (165 cas sur 300 dès `min_samples` ≥ 2), scikit-learn
   binarise dans l'ordre de `np.argsort`, n'est pas équivariant (cinq points : 60/60 selon l'ordre d'entrée) et
   sélectionne des amas de stabilité nulle (9 cas). La v10 avait trouvé la cause de sa « dépendance à la machine »
   (ordre de `np.argsort` aux égalités) ; l'étage N-aire la supprime, car l'ultramétrique ne dépend pas de cet ordre.

**Variantes à mesurer (deux).** V1 : existence **C** (sites *résolus* : un site ne compte qu'une fois réunies à sa
lignée toutes les composantes qualifiées qui le couvrent) ; elle tient T0 **et** Q-Π2 et compte au plus autant de
feuilles que A (prouvé), mais n'a aucune constante de stabilité (saut exact de 724,6 pour un déplacement de 1).
V2 : complétion par lignée.

**Prix publiés avec la tête.** Q3 (a) reste perdu au niveau plat (x va à l'amas ; HDBSCAN tient cette cellule) ;
une feuille peut naître de $k$ sites seulement, par une boule *mixte* qui qualifie leur composante avant sa fusion
(contre-exemple exact à la phrase « aucune structure isolée de $k$ sites n'est un bloc », à inscrire au registre).

**Ce qui reste conjectural** : le gain de la sortie plate sur HDBSCAN aux tailles d'intérêt (prédictions S1–S6) ; la
constante de stabilité du critère B (observée 1,68) ; l'équivalence asymptotique des critères A, B, C ; la constance
par morceaux de la sortie plate hors des coïncidences (esquisse sous hypothèse de position générale).

## 1. Cadre (a)

### 1.1 L'objet

À ordre $k\geq 2$ fixé, $T_k$ est l'arbre FULL$_k$ : nœuds $\nu$, rayons de naissance $b(\nu)$ (racines carrées de
niveaux rationnels), parent $\pi(\nu)$ avec $b(\pi(\nu))>b(\nu)$, fusions N-aires aux plateaux exacts. $H^{r}_{k+1}$
attache chaque site $i$ à un propriétaire $o_i$ à la date $e_i=t_i+D_i$ ($t_i$ première couverture qualifiée, $D_i$
marge), avec $b(o_i)\leq e_i<b(\pi(o_i))$. Le **bloc** du nœud $\nu$ au rayon $r\in[b(\nu),b(\pi(\nu)))$ est
$B_\nu(r)=\lbrace i : o_i\preceq\nu,\ e_i\leq r\rbrace$ ($\preceq$ : descendant ou égal), coupe fermée. L'ultramétrique
de rencontre est $u(i,j)=\min\lbrace r : i,j$ dans un même bloc au rayon $r\rbrace$, soit
$u(i,j)=\max(e_i,e_j,b(w))$ si $w=\mathrm{lca}(o_i,o_j)\notin\lbrace o_i,o_j\rbrace$ et $\max(e_i,e_j)$ sinon, avec
$u(i,i)=e_i$. C'est un *treegramme* (ultramétrique à diagonale non nulle) : les sites apparaissent à des dates
différentes, comme dans HDBSCAN où un point apparaît à sa distance de cœur.

Toutes les décisions des scripts sont exactes : un rayon est une somme rationnelle de racines de rationnels
(`modele_lib.Rad`), l'égalité est certifiée par classes de carrés, une différence non nulle est séparée par
encadrements, au-delà du budget : refus, jamais une égalité supposée. Les excès de masse sont des intervalles
rationnels raffinés ; sur les 252 décisions d'EOM des fixtures, aucune n'est restée non tranchée.

### 1.2 Amas candidat

Un **amas candidat** à mcs est une chaîne maximale de l'arbre condensé : un intervalle de rayons $[r_b(C),r_d(C))$ le
long d'une lignée de $T_k$, pendant lequel le bloc est « gros » (critère d'existence, section 2) sans qu'une **vraie
scission** l'interrompe. Une vraie scission est un plateau exact où au moins deux gros blocs se réunissent : ils meurent
et un amas parent naît, avec **autant d'enfants que de gros blocs réunis** (N-aire). Un petit bloc qui se réunit à un
gros est **absorbé** (pas de changement d'identité) ; un gros bloc qui apparaît sans gros bloc inclus est une
**feuille**. Les **membres** de $C$ sont les sites de son bloc juste avant $r_d(C)$ ; le **rayon d'adhésion**
$j_p(C)\in[r_b(C),r_d(C))$ d'un membre compté est le premier rayon où il est compté dans la chaîne.

Propriété (prouvée, plateaux atomiques) : **aucun amas n'a une durée de vie nulle**. Un gros bloc qui naît au rang $r$
d'un plateau où il rencontre un autre gros bloc est, à ce rang, déjà le bloc réuni ; il n'existe donc pas séparément.
C'est faux pour l'arbre binaire de scikit-learn (section 3.5).

### 1.3 Masses : sites entrés comptés 1

Un critère d'existence est donné ici par des **dates de comptage** $s_i\geq e_i$ : un site est membre de son bloc dès
$e_i$, mais il ne compte pour l'existence qu'à partir de $s_i$. La masse comptée d'un bloc au rayon $r$ est
$M(B,r)=\#\lbrace i\in B : s_i\leq r\rbrace$ et le bloc est gros si $M\geq$ mcs. Le critère par défaut est
$s_i=e_i$ (critère A : sites entrés, comptés 1). Raisons :

- **Règle de l'utilisateur** (Π1) : les clusters sont des blocs d'au moins mcs points.
- **Proposition 3 (masses entières contre fractionnaires).** Avec des poids entiers et des dates stables, le rayon
  d'existence d'un site est une statistique d'ordre de dates stables (proposition 2), donc stable. Avec des poids
  fractionnaires $w_j\in(0,1]$ dépendant continûment des positions (masses du § 9.1), le rayon d'existence est le premier
  rayon où une somme partielle atteint l'entier mcs : il saute quand une somme partielle franchit mcs. *Preuve.* Bloc
  recevant aux rayons $1<2<3$ des sites de poids $1,1,w$, mcs = 3, prochain événement au rayon 5 : $R=3$ si $w=1$,
  $R=5$ si $w<1$ ; si $w\to 1^{-}$ continûment, $R$ saute de 2. □ À cela s'ajoute le fait connu (v10, rejoué par le juge)
  que les masses du § 9.1 donnent à chaque triangle de T0 la masse $8/3<3$ : T0 est perdu à mcs 3.
- La factorisation du juge précédent (§ 5.1) tient avec les comptes entiers ; la proposition 1 la précise.
- Des poids **fixes** positifs (retours multiples, poids déclarés) restent admissibles : ce sont des masses entières
  pondérées, et la stabilité de $u$ s'étend aux poids fixes appariés (auditeur, Q1).

### 1.4 Proposition 1 : la condensation est l'atteignabilité mutuelle de $u$

**Énoncé.** Soit $R_i$ la mcs-ième plus petite valeur de $\lbrace\max(u(i,j),s_j)\rbrace_j$ (diagonale comprise).
Alors $i$ est dans un gros bloc au rayon $r$ si et seulement si $r\geq R_i$, et $i,j$ sont dans un même gros bloc au
rayon $r$ si et seulement si $\tilde u(i,j)=\max(u(i,j),R_i,R_j)\leq r$. $\tilde u$ est une ultramétrique de
diagonale $R_i$. Pour le critère A, $R_i$ est la mcs-ième plus petite valeur de la ligne $i$ de $u$ : $\tilde u$ est
la **distance d'atteignabilité mutuelle de $u$ à `min_samples` = mcs**.

*Preuve.* Le bloc de $i$ au rayon $r\geq e_i$ est $\lbrace j : u(i,j)\leq r\rbrace$ ; sa masse comptée est
$\#\lbrace j : \max(u(i,j),s_j)\leq r\rbrace$, qui atteint mcs exactement à $R_i$ ($R_i\geq e_i$ car chaque terme
majore $u(i,j)\geq e_i$). Deux sites du même bloc ont la même masse, d'où l'équivalence. Ultramétrie :
$\tilde u(i,l)\leq\max(u(i,j),u(j,l),R_i,R_l)\leq\max(\tilde u(i,j),\tilde u(j,l))$. Pour A,
$\max(u(i,j),e_j)=u(i,j)$. □

Lecture. HDBSCAN applique deux fois la même transformation : à `min_samples` = k sur la distance euclidienne
(densité), puis, par sa condensation, à `min_samples` = mcs sur l'ultramétrique obtenue (taille). La tête proposée fait
la seconde sur $H^{r}_{k+1}$ au lieu de la faire sur le lien simple de l'atteignabilité mutuelle. **La comparaison
équitable consiste à appliquer la même seconde transformation et le même EOM aux deux ultramétriques.**

Contrôle (`verif_condensation.py`) : 60 nuages, critères A, B, C, mcs 2 à 4 ; 4 041 sites, 103 428 comparaisons
(paire, rang) entre l'arbre condensé construit par événements exacts et la formule : 0 écart.

### 1.5 Proposition 2 : stabilité de la hiérarchie condensée

**Énoncé.** Sous déplacement apparié de pas $\varepsilon$ (hypothèses de H3 : identifiants appariés, sites distincts,
poids unitaires ou fixes, $k$ et $m$ fixés), $\vert\Delta\tilde u_A(i,j)\vert\leq 3\varepsilon$ pour tous $i,j$. Plus
généralement, si $\vert\Delta s_j\vert\leq c\varepsilon$, $\vert\Delta\tilde u_s\vert\leq\max(3,c)\varepsilon$.

*Preuve.* H3 donne $\vert\Delta u(i,j)\vert\leq 3\varepsilon$, diagonale comprise. Une statistique d'ordre est
1-lipschitzienne pour la norme du sup ; un maximum aussi. □

Contrôle (`stabilite_condensee.py`, 300 paires de nuages de 6 à 8 sites perturbés d'au plus une unité par
coordonnée) : rapport maximal 1,38 pour $\tilde u_A$, 1,68 pour le critère B, **9,08** pour le critère C (et 19,45
pour ses dates $\rho$). Ce n'est pas une vérification : la proposition est invoquée ; le contrôle grave la constante
observée et montre que C sort du cadre.

### 1.6 Proposition 4 : fidélité de la sortie plate

Tout cluster plat sélectionné est un bloc de $H^{r}_{k+1}$ à son rayon de mort, donc inclus dans l'amas discret de son
nœud FULL (F1) ; deux sites d'un même cluster plat sont dans une même composante de FULL$_k$ à ce rayon. **La sortie
plate ne réunit jamais ce que FULL sépare.** (Conséquence directe de F1 et de la proposition 1.)

## 2. Le critère d'existence (b)

### 2.1 Les critères, tous dérivés du modèle

| Code | Date de comptage $s_i$ | Lecture dans le modèle |
| --- | --- | --- |
| A | $e_i$ | site entré dans son bloc (pendaison fidèle) |
| B | premier rayon $\geq e_i$ où $x_i$ est un point de la composante $C$ du nœud de son bloc ($C\cap X$) | point de cœur **du nœud**, pas seulement $d_k(x_i)\leq r$ |
| C | $\rho_i=\max(e_i,\ \sup_q m(p_i,q))$, sup sur les rivaux qualifiés | site *résolu* : toutes les composantes qualifiées qui le couvrent ont rejoint sa lignée |
| D | (étage FULL) bloc non vide dont le nœud couvre au moins mcs sites | taille de l'amas discret, la lecture « sites couverts » |
| $E_\theta$ | $\max(e_i,(1-\theta)e_i+\theta d_k(x_i))$ | « maturité » interpolée de la v10, $\theta\in[0,1]$ |

C est la lecture littérale de « un point de bord ne fait pas exister un cluster » dans un objet qui est un
**recouvrement** (thèse, § 9.1) : un point de bord est un point de la zone de recouvrement de deux structures
qualifiées. C est anticipatif (comme $H^{r}_{k+1}$, théorème B) : c'est le prix de la monotonie.

### 2.2 Propriétés communes (prouvées)

- **Monotonie en $r$** pour A, B, C, $E_\theta$ : la masse comptée d'une lignée croît (blocs et ensembles de sites
  comptés croissent), donc l'arbre condensé est bien défini. Contrôle : 0 violation dans toutes les exécutions.
- **Monotonie en mcs** : à chaque rayon, les gros blocs à mcs + 1 sont des gros blocs à mcs.
- **Laminarité** : les clusters sont des blocs d'une hiérarchie laminaire ; toute antichaîne est une partition partielle.
- **Équivariance** : aucune règle ne lit un indice ; les ex æquo vont au parent. Contrôle exact (`equivariance.py`) :
  la tête rend une seule partition sur les 120 permutations des cinq points, les 720 de T0 exact et de P1, et 30 de Q3.
- **Proposition 5 (dates plus tardives, moins de feuilles).** Si $s\leq s'$ site par site, l'arbre condensé de $s'$ a
  au plus autant de feuilles que celui de $s$. *Preuve.* Deux feuilles $L_1,L_2$ de $s'$, nées aux rangs
  $r_1\leq r_2$ : à $r_2$, le bloc de la lignée de $L_1$ est gros (monotonie) ; s'il était le bloc de naissance de
  $L_2$, celui-ci contiendrait un gros bloc du rang précédent et ne serait pas une naissance. Les blocs de naissance des
  feuilles de $s'$ sont donc disjoints deux à deux. Chacun est gros pour $s$ (masse plus grande) et contient le bloc de
  naissance d'une feuille de $s$ ; l'application est injective. □ Corollaire : feuilles(C) ≤ feuilles(A),
  feuilles(B) ≤ feuilles(A). Contrôle : 0 violation sur 80 petits nuages (`sur_segmentation.py`).

### 2.3 Fixtures exactes (oracle de la définition, $k=2$, $m=3$)

Fenêtres des cellules de l'utilisateur ; « conforme » signifie : à tout rayon de la fenêtre (coupe fermée), les gros
blocs sont exactement ceux de la cible (`fixtures_existence.py`, sortie `sorties/fixtures_existence.txt`).

| Fixture (cible) | A | B | C | D | $E_{1/4}$ | $E_{1/2}$ | $E_{3/4}$, $E_1$ |
| --- | --- | --- | --- | --- | --- | --- | --- |
| T0 exact, mcs 2 et 3, $[1{,}3r_0;1{,}7r_0]$ : ABC \| DEF | ✓ | ✗ | ✓ | ✓ | ✗ | ✗ | ✗ |
| T0 P1, P2, S_plan, mcs 2 et 3, $[1300;1700]$ | ✓ | ✗ | ✓ | ✓ | ✗ | ✗ | ✗ |
| T0 S_3D, mcs 2 et 3, $r^{2}\in[845000;1445000]$ | ✓ | ✗ | ✓ | ✓ | ✗ | ✗ | ✗ |
| T0 exact, mcs 4 : aucun cluster (forcée) | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| Q1 (pont 1 700), mcs 2 et 3, $[19f/20;f)$ : ABC \| DEF | ✓ | ✗ | ✓ | ✓ | ✓ | ✓ | ✗ |
| cinq points, mcs 2, $r=\sqrt{35}$ : {1,2} \| {3,4} | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| Q3 (a), mcs 2 et 6, $[705;790]$ : {x,f1..f5} \| {c0..c7} | ✗ | ✗ | ✗ | ✗ | ✗ | ✗ | ✗ |
| Q3 Π2 dérivée, mcs 7 et 8 : {c0..c7, x} | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| **Q-Π2**, mcs 9, $[705;790]$ : aucun cluster | **✗** | ✓ | **✓** | ✗ (dès 453,47) | ✗ | ✗ | ✗ |

Les quatre variantes d'une unité de Q3 (x vers l'amas, x vers le filament, c0 vers x, f1 vers x) donnent les mêmes
verdicts pour Q-Π2 (A ✗, B ✓, C ✓, $E_\theta$ ✗). Q3 (a) est perdu par **tous** les critères : c'est la projection
qui met x dans l'amas (prix publié de $H^{r}_{k+1}$) ; aucun critère d'existence ne déplace un point.

Valeurs exactes utiles. T0 exact : $e=\sqrt{2/3}$ pour les six sites, $d_2=\sqrt 2$, racine $\sqrt{3/2}$. Q3 :
$e_x=\sqrt{205636}+\sqrt{2535209/4}-\sqrt{1962805/4}\approx 549{,}0873$, $t_x=\sqrt{205636}\approx 453{,}4711$,
$D_x\approx 95{,}6163$, $\rho_x=\sqrt{2535209/4}\approx 796{,}1170$ (la racine), $d_2(x)\approx 700{,}0064$ ; les sites
de l'amas ont $\rho=e=t\leq 141{,}43$ (aucun rival).

### 2.4 Théorème 6 : Q-Π2 contre stabilité (cadre intrinsèque)

**Cadre.** Comme pour le théorème C du workflow précédent : le profil qualifié d'un site est la partie remontante
$\Pi_{k+1}(R_x)$ de l'espace d'arbre ; deux profils sont $\delta$-entrelacés s'il existe des cartes décalant les rayons
de $\delta$, commutant aux remontées, de composées $\mathrm{Up}(2\delta)$. **Axiomes** d'une règle de dates de
comptage : (L) $s_x$ ne dépend que du profil qualifié de $x$ ; (I) comptage immédiat sans rival : un profil réduit à une
lignée née à $\alpha$ donne $s=\alpha$ ; ($S_L$) $\vert s(P)-s(P')\vert\leq L\delta$ pour des profils
$\delta$-entrelacés. L'existence est « au moins mcs sites comptés » sur les blocs de $H^{r}_{k+1}$.

**Énoncé.** Sur Q3 (et ses quatre variantes), toute règle (L), (I), ($S_L$) avec $L<6{,}99$ fait exister à mcs 9 le
cluster {c0..c7, x} en $r=790$ : Q-Π2 est perdu. Sur la variante de base, $L<7{,}039$ suffit.

*Preuve.* Le profil de x est la lignée de l'amas née à $\alpha=t_x$ plus des barres rivales qualifiées (le filament)
qui rejoignent la lignée en $M=796{,}117$. Une barre $[c,M)$ s'efface par un entrelacement de $\delta=(M-c)/2$
(envoyer le point rival $(q,r)$ sur $(\mathrm{lineage},r+\delta)$ ; la composée vaut $\mathrm{Up}(2\delta)$ dès que
$2\delta\geq M-c$). Avec $c$ la plus basse naissance rivale, le profil est $\delta$-entrelacé avec la lignée seule,
de date $\alpha$ par (I) ; donc $s_x\leq\alpha+L\delta$. Base : $\alpha=453{,}4711$, $\delta=47{,}8081$,
$(790-\alpha)/\delta=7{,}0392$. Les huit sites de l'amas n'ont aucune barre rivale ($\rho=e=t\leq 141{,}43$) : par
(I) ils comptent dès $t\leq 141{,}43$. Le bloc {c0..c7, x} existe de $549{,}09$ à $796{,}12$ ; si $s_x\leq 790$, il a 9
sites comptés en 790. Valeurs des cinq variantes : 7,039 ; 7,087 ; 6,992 ; 7,044 ; 7,076 (`qpi2_borne.py`). □

**Conséquences.**

- $H^{r}_{k+1}$ a la constante 3 : **aucune date de comptage de même qualité que ses dates ne tient Q-Π2.** La famille
  stable $P_\kappa$ de la v10 ($\kappa\geq 1$, constante $1+2\kappa$) donne au plus $e_x=549{,}09$ ; tenir Q-Π2 demande
  $\kappa\leq 0{,}0248$, hors du domaine stable.
- Ce n'est pas une impossibilité absolue : $s=t+4D$ (constante 9) tient Q-Π2 sur la base (835,94 > 790). Mais c'est
  un réglage sur la cellule, sans lecture dans le modèle.
- Le critère C sort du cadre ($S_L$) : aucune constante (2.5). B sort de (I) : il ne compte pas un site sans rival à son
  entrée (et perd T0).

### 2.5 Les autres critères, un par un

**B (cœur du nœud) perd T0, partout, et Q1.** *Preuve.* À $k=2$, un site n'est dans $L_2(r^{2})$ que si $r\geq d_2$,
distance à son plus proche voisin. Dans T0 exact et ses quatre variantes, la plus petite distance entre deux sites
dépasse le rayon de la racine (P1 : 1 999,96 contre 1 931,83 ; P2 : 1 998 contre 1 930,86 ; S_plan : 2 000 contre
1 897,37 ; S_3D : 1 414,21 contre 1 224,74 ; exact : $\sqrt2$ contre $\sqrt{3/2}$) : aucun site n'est point de cœur
avant la racine, aucun triangle n'existe. Dans Q1, A, B, E, F ont $d_2=1\,999{,}96>1\,787{,}36$ ; C et D sont points de
cœur dès 1 700, mais de la composante de la paire CD, qui ne rejoint ABC qu'à la racine : ABC n'a aucun site compté
avant la racine. □ C'est le défaut de l'« existence par taille de cœur » de la v10, qui gagnait
pourtant +0,110 à mcs = K : le mécanisme anti-fragmentation est réel, la règle est fausse sur la figure-étalon de la
thèse. Stabilité observée 1,68 (conjecture : constante ≤ 3).

**$E_\theta$ (maturité interpolée) perd Q-Π2 pour tout $\theta\in[0,1]$ et T0 dès $\theta>1/15$.** *Preuve.*
$s_x\leq d_2(x)=700{,}006<705$ : x compte sur toute la fenêtre. Pour T0, à l'entrée de la fenêtre $r=1300$ : S_plan
($e=1250$, $d_2=2000$) exige $(1-\theta)\cdot 1250+\theta\cdot 2000\leq 1300$, soit $\theta\leq 1/15$ ; P1
($e=1154{,}68$, $d_2=1999{,}96$) exige $\theta\leq 0{,}1719$, comme la version exacte à l'échelle près. □ La forme géométrique de la
v10 ($x$ mûr si $\mathrm{dist}(x,C_r)\leq\theta r$) échoue aussi : T0 exige $\theta\geq 0{,}538$ (pour P1,
$\mathrm{dist}(A,L_2(r^{2}))\geq 1999{,}956-r$, car tout point de $L_2$ a un site autre que A à distance au plus $r$) ;
Q-Π2 exige $\theta<0{,}139$ (le point de l'axe à distance $900-r$ de x, témoin de la paire {x, c0}, est dans la
composante de l'amas pour $r\in[453{,}47;800)$). Les deux intervalles sont disjoints. □ « Point de bord » ne veut donc
pas dire « point peu profond » : x est plus profond dans l'amas à 790 que A dans ABC à 1 300.

**D (sites couverts) viole Π1.** Sur Q3 à mcs 9, le bloc {c0..c7} (8 membres) est déclaré cluster dès 453,47, avant
l'entrée de x (fixture). Par la factorisation du juge, la lecture « absorption » de D redonne A ; prise comme critère de
bloc, elle fabrique des clusters de moins de mcs membres.

**C (résolus) tient T0 et Q-Π2, mais n'a aucune constante de stabilité.** Contre-exemple exact (`saut_resolution.py`) :
témoin R1 de l'auditeur à l'échelle 100, paire $w_1,w_2$ en $(0,2000\pm h,0)$. Pour $h=1702$, une barre rivale
qualifiée de persistance 0,386 naît tard ($[1724{,}201;1724{,}587)$) : $\rho_x=1724{,}587$. Pour $h=1703$ (chaque $w$
déplacé de 1), elle n'existe plus : $\rho_x=e_x=1000$. $\vert\Delta e_x\vert=0{,}386$ (H3 respecté), mais
$\vert\Delta\rho_x\vert=724{,}587$ ; à mcs 4, la naissance de l'amas condensé passe de 1 200 à 1 000. Le saut grandit
avec l'échelle du nuage à déplacement fixé : aucune borne uniforme. Une barre rivale courte et tardive est exactement
ce que la marge $P_1$ neutralise ($m-\kappa(c-\alpha)$ avec $\kappa\geq 1$) et ce que C ne neutralise pas.

### 2.6 Sur-segmentation à mcs = k (une feuille d'une seule boule ?)

**Proposition 7.** Pour A (donc B et C, plus tardifs) : (i) aucune feuille ne repose sur une composante de $L_k$ réduite
à un seul $(k-1)$-simplexe (une seule boule de $k$ points) : un nœud non qualifié ne reçoit aucun site ; (ii) un
groupe isolé de $k+1$ sites, sans rival qualifié, fonde une feuille à mcs ≤ $k+1$ ; (iii) **un groupe de $k$ sites peut
fonder une feuille** à mcs ≤ $k$ : sa composante est qualifiée par une boule *mixte* ($k$ sites du groupe et un site
extérieur) avant la fusion avec la composante de ce site extérieur. (i) découle de la définition de $H^{r}_{k+1}$ ; (ii)
et (iii) sont des fixtures exactes (`sur_segmentation.py`).

Fixture (iii) : six sites $G$ et une paire $\lbrace p_0,p_1\rbrace$ à 100 unités, $k=2$. La boule $\lbrace
g_2,p_0,p_1\rbrace$ qualifie la composante de la paire en $\sqrt{6893/4}\approx 41{,}512$, la fusion avec $G$ a lieu en
42,741 : le bloc {p0, p1} vit sur $[41{,}512;42{,}741)$, c'est une feuille à mcs 2 (sélectionnée par l'EOM, comme par
HDBSCAN). Même chose à $k=3$ pour un triangle isolé. **Correction** : la phrase « aucune structure isolée de $k$ sites
n'est un bloc » (note du développeur, juge précédent) n'est vraie que si la fusion précède toute boule mixte.

HDBSCAN (`min_samples` = k) fonde une feuille sur $k$ points mutuellement proches. La qualification supprime les
feuilles d'une seule boule de $k$ points (le défaut de `cover` en v10 : 5 732 clusters par trame) mais pas les
feuilles courtes nées d'une boule mixte. Illustration sur 40 petits nuages uniformes de 12 sites (oracle de
correction, aucune portée d'échelle) : feuilles moyennes à $k$ = mcs = 2 : A 2,80, B 1,90, C 2,52, HDBSCAN N-aire
3,45 ; à $k$ = mcs = 3 : A 1,60, B 1,07, C 1,20, HDBSCAN N-aire 1,45.

### 2.7 Décision pour P4

| Critère | Monotone | Stable | T0 | Q-Π2 | Π1 | Paramètre |
| --- | --- | --- | --- | --- | --- | --- |
| **A, entrés** | ✓ | **3ε** (prouvé) | ✓ | ✗ (théorème 6) | ✓ | aucun |
| B, cœur du nœud | ✓ | 1,68 observé (conjecture) | ✗ (prouvé) | ✓ | ✓ | aucun |
| **C, résolus** | ✓ | **aucune constante** (fixture) | ✓ | ✓ | ✓ | aucun |
| D, couverts | ✓ | — | ✓ | ✗ | ✗ | aucun |
| $E_\theta$, maturité | ✓ | $\leq 3\varepsilon$ (combinaison de $e$ et $d_k$) | ✗ si $\theta>1/15$ | ✗ (tout $\theta$) | ✓ | $\theta$ |

**Proposé** : A comme critère principal, par cohérence avec le choix de $H^{r}_{k+1}$ (la stabilité y a primé sur Q2,
Q3, Q4). Q-Π2 est publié comme perdu, avec sa cause (la projection de x, puis le théorème 6). C est mesuré comme
variante : parmi les critères étudiés, c'est le seul sans paramètre qui tienne T0 et Q-Π2, et la proposition 5 garantit
qu'il n'a jamais plus de feuilles que A. L'adopter serait un choix de l'utilisateur contre le principe de stabilité ; la mesure E1 doit l'éclairer.

## 3. La sélection (c)

### 3.1 Excès de masse N-aire et antichaîne optimale

Pour $\lambda=\varphi(r)$ décroissante, la stabilité d'un amas est
$S_\varphi(C)=\sum_{p}(\varphi(j_p)-\varphi(r_d))=\int_{r_b}^{r_d}M_C(r)\,\vert d\varphi(r)\vert$, somme sur les membres
comptés, $M_C$ masse comptée du bloc de la chaîne ($\varphi(\infty)=0$ pour la racine). C'est la stabilité de HDBSCAN
quand $\varphi(r)=1/r$, et la forme de Zoltan ($\int M_C\,d\lambda$ « dans la coordonnée de densité déclarée »).

**Proposition 8 (§ 5.1 de la thèse, forme exacte).** Sur un arbre enraciné N-aire à poids $w\geq 0$, la récurrence
$\widehat W(C)=\max(w(C),\sum_{D}\widehat W(D))$ sur les enfants $D$ calcule en temps linéaire le maximum de
$\sum_{C\in A}w(C)$ sur les antichaînes $A$ ; un maximum est atteint par une coupe (antichaîne qui rencontre chaque
chemin racine-feuille). *Preuve.* Une antichaîne du sous-arbre de $C$ est $\lbrace C\rbrace$ ou une réunion
d'antichaînes des sous-arbres des enfants, sans contrainte entre sous-arbres ; récurrence. Avec $w\geq 0$ on complète
une antichaîne en coupe sans perdre de poids. □ Aucune binarisation n'est nécessaire : la somme porte sur tous les
enfants. La sélection de HDBSCAN est cette récurrence avec $w=S$, la racine exclue et le parent gardé aux ex æquo ; le
« coût sur mesure » du § 5.2 de la thèse est le même programme avec un autre $w$.

Contrôle d'implantation (`verif_eom_contre_sklearn.py`, 60 nuages gaussiens de 40 à 160 points, mcs 2 à 20) : sur les
**135** cas dont l'arbre de scikit-learn n'a aucune hauteur ex æquo, la condensation N-aire et l'EOM de ce rapport
rendent **exactement** les étiquettes de scikit-learn (z = 1).

### 3.2 Théorème 9 : z est un cadran de granularité monotone

**Énoncé.** Avec $\varphi_z(r)=r^{-z}$, pour $0<z_1<z_2$, tout amas sélectionné à $z_2$ est descendant ou égal d'un amas
sélectionné à $z_1$. Quand $z\to\infty$ la sélection tend vers les feuilles ; quand $z\to 0^{+}$ vers l'EOM
logarithmique $S_0(C)=\sum_p\ln(r_d/j_p)$.

*Preuve.* Soit $C$ un nœud d'enfants $D_1,\dots,D_m$, $a=r_b(C)=r_d(D_i)$. Multiplions toutes les stabilités par
$a^{z}/z>0$, ce qui ne change aucune comparaison en $C$ :
$a^{z}S_z(C)/z=\int_a^{r_d}M_C(r)(a/r)^{z}\,dr/r$ est décroissant en $z$ ($a/r\leq 1$) ; pour tout descendant $E$,
$a^{z}S_z(E)/z=\int M_E(r)(a/r)^{z}\,dr/r$ sur $r\leq a$ est croissant en $z$ ; donc $a^{z}\widetilde S_z(D_i)/z$
(maxima et sommes de fonctions croissantes) est croissant. Le prédicat « les enfants gagnent en $C$ »
($\sum\widetilde S_z(D_i)>S_z(C)$, inégalité stricte puisque le parent gagne aux ex æquo) est donc monotone : vrai à
$z_1$, il est vrai à $z_2$. Un nœud est sélectionné si et seulement s'il gagne en lui-même (ou est une feuille) et si
les enfants gagnent en chacun de ses ancêtres stricts non racines ; la sélection est une coupe. Soit $C_2$ sélectionné
à $z_2$ et $C_1$ l'unique sélectionné à $z_1$ sur le chemin de $C_2$. Si $C_1$ était un descendant strict de $C_2$, les
enfants gagneraient en $C_2$ à $z_1$, donc à $z_2$, et $C_2$ ne serait pas sélectionné. Limites : $(a/r)^{z}\to\infty$
sous $a$ et $\to 0$ au-dessus (convergence dominée, durées de vie positives, 1.2) ; $S_z/z\to S_0$. □

Contrôle (`z_monotonie.py`, cherche un contre-exemple, ne vérifie pas) : 120 arbres de HDBSCAN (100 à 400 points),
10 valeurs de $z$ de 0,25 à 20, **5 400 paires, 0 violation**, 326 sélections distinctes (z change réellement la
sortie) ; 240 arbres exacts de $H^{r}_{k+1}$ (A et C), $z\in\lbrace\log,1,2,3,6,\text{feuilles}\rbrace$, 3 600 paires,
0 violation.

### 3.3 Ce qui dépend de $\varphi$ et ce qui n'en dépend pas

- **Dépend** : la décision en tout nœud dont la durée de vie propre et celle d'un enfant sont positives. *Preuve.* Une
  $\varphi$ presque constante au-dessus de $r_b(C)$ fait gagner les enfants, presque constante en dessous fait gagner le
  parent. □ Dans la famille $r^{-z}$, chaque nœud a un seuil $z^{*}(C)$ (théorème 9).
- **Ne dépend pas** : l'arbre condensé (existence), les feuilles, la fidélité (proposition 4), l'équivariance,
  l'invariance par homothétie (toutes les stabilités sont multipliées par $c^{-z}$), et **l'ordre** des sélections : la
  famille $\lbrace\mathrm{Sel}_z\rbrace_{z>0}$ est une chaîne pour le raffinement, de $\mathrm{Sel}_{0^{+}}$ aux
  feuilles, avec au plus autant d'éléments distincts que d'amas. Choisir $z$, c'est choisir un point de cette chaîne.
- Seule l'échelle logarithmique ($z\to 0$) est invariante par toutes les reparamétrisations $r\mapsto cr^{a}$. La v10
  l'a mesurée moins bonne qu'un $z$ bien choisi : l'invariance maximale n'est pas le critère de l'utilisateur.

### 3.4 Le choix du modèle : $\lambda=r^{-3}$, $z=1$ publié

$L_k(r^{2})=\lbrace y : \hat f_k(y)\geq k/(n\,\omega_3 r^{3})\rbrace$ pour l'estimateur K-NN de la densité dans
$\mathbb{R}^{3}$ : le niveau de densité du modèle au rayon $r$ est $\lambda(r)\propto r^{-3}$. Avec ce $\lambda$, la
somme de type HDBSCAN estime $\int_{\lambda_b}^{\lambda_d}P(C(\lambda))\,d\lambda$, l'excès de masse en **contenu de
probabilité** des amas discrets (la constante $k/(n\omega_3)$ ne change aucune décision). D'où **$z=3$ comme choix du
modèle**. Trois réserves, à publier :

1. La définition 19 de la thèse est l'excès de masse **de Lebesgue** $\int_C(f-\lambda)_{+}\,dx$ ; l'estimateur
   $\sum_x(\lambda_x-\lambda_{min})$ qu'elle cite estime le contenu de probabilité $\int_C(f-\lambda)_{+}f\,dx$. Ce sont
   deux fonctionnelles ; la thèse les confond. Nous retenons la seconde (celle de HDBSCAN et de Zoltan).
2. $\lambda=1/r$ (HDBSCAN, $z=1$) n'est une densité qu'en dimension 1 ; elle reste publiée (consigne du 1er octobre)
   comme point de la chaîne du théorème 9, pas comme modèle.
3. Pour des sites portés par une surface (LiDAR), la densité intrinsèque est $\propto r^{-2}$ : $z=2$ serait le choix
   d'un modèle surfacique, que la v11 n'adopte pas ($L_k$ est défini par des boules de $\mathbb{R}^{3}$). Aucun $z$
   global n'est cohérent pour un support de dimension mélangée ; c'est une limite du modèle, pas un réglage à faire.

Ce choix coïncide avec celui de la v10 (z = 3), pour la même raison ; il n'en est pas repris : la v10 avait lu z sur
ses scènes de développement et l'optimum y dépendait de $n$ (3 à 4 à 2 000 points, 6 à 8 000). Le théorème 9 explique
pourquoi ce réglage est un réglage de granularité. `cluster_selection_epsilon` est une constante métrique hors modèle
(contraire à la thèse de Zoltan) : $\varepsilon=0$.

### 3.5 Plateaux exacts : jamais binariser

- Tous les événements d'un même rayon exact sont traités ensemble (coupe fermée) ; une entrée au rayon d'une fusion va
  au parent, jamais à l'enfant mort.
- Ex æquo de stabilité : parent (convention de scikit-learn, équivariante) ; comparaison certifiée, refus au-delà du
  budget.
- **Ce que fait scikit-learn aux plateaux** (même contrôle, 165 cas avec hauteurs ex æquo sur 300, fréquents dès
  `min_samples` ≥ 2 car une arête d'atteignabilité mutuelle vaut souvent la distance de cœur d'un de ses bouts) :
  25 désaccords avec l'étage N-aire, 35 points, dont **9 amas sélectionnés de stabilité nulle** (nés et morts au même
  $\lambda$, artefact de l'ordre de binarisation, choisis parce que la racine est exclue). Exemple tracé : 89 points,
  `min_samples` = 3, mcs = 20 : scikit-learn rend un amas de 20 points né et mort au rayon 1,122811, l'étage N-aire n'en
  rend aucun.
- **Non-équivariance** : sur les cinq points de l'auditeur, le point 0 est équidistant de 1, 2, 3, 4 ; scikit-learn le
  met avec {1, 2} ou avec {3, 4} selon l'ordre d'entrée (60 permutations sur 120 chacune). L'étage N-aire le laisse bruit,
  sur les 120 (`equivariance.py`).

### 3.6 Continuité

**Proposition 10 (connexité).** Toute sortie plate qui prend au moins deux valeurs sur un ensemble connexe de
configurations est discontinue (sortie à valeurs discrètes). □

**Proposition 11 (équivariance et symétrie, analogue plat du trilemme F2).** Soit $X$ une configuration invariante par une
isométrie $\sigma$ qui fixe un site $x$ et échange deux groupes $G_1,G_2$. Si une règle équivariante met $x$ avec $G_1$
pour des configurations $X_t\to X$, elle met $x$ avec $G_2$ pour $\sigma X_t\to X$ ; elle ne peut être continue en $X$.
□ Les cinq points réalisent l'hypothèse : 0 en $(5,2,0)$ rejoint {1, 2}, son symétrique $(7,2,0)$ rejoint {3, 4}
(fixtures, `completion.py`), et en $(6,2,0)$ la tête rend 0 bruit. Pour tout $t>0$, 0 en $(6-t,2,0)$ est couvert
d'abord par {0, 1, 2} et entre avant la racine ($e=t_1+(M-t_2)<M$ dès que $t_1<t_2$) : l'hypothèse vaut pour tout $t$. Toute règle doit choisir entre équivariance et continuité en un tel point ; scikit-learn
perd l'équivariance, la tête la garde.

**Proposition 12 (constance par morceaux, esquisse).** La sortie de la tête dépend de l'ordre faible des rayons
d'événements de $\tilde u_A$ et des signes des marges d'EOM. Ces quantités étant continues (proposition 2), la sortie
est localement constante hors des configurations où deux événements distincts coïncident ou une marge s'annule, pourvu
que FULL$_k$ soit combinatoirement stable au voisinage (inégalités strictes entre rayons de boules minimales). Statut :
esquisse, l'hypothèse de position générale n'étant pas démontrée ici pour FULL.

**Le saut de création de scission** (`saut_eom.py`). Nuage 1D, B = 0..5, S à $5+g$, $+3{,}9$, $+4{,}0$, D = 100..105,
`min_samples` = 2, mcs = 3. Pour $g=4{,}05$, S devient un amas avant de toucher B : vraie scission, stabilités (z = 1)
parent 2,114 contre enfants 4,519 + 0,009 ; scikit-learn et l'étage N-aire rendent {B}, {S}, {D}, S n'ayant que 0,009 de
stabilité. Pour $g=3{,}95$ : {B ∪ S}, {D} (stabilité 6,648). Le basculement n'est pas un ex æquo d'EOM (marge large
des deux côtés) : c'est la coïncidence $g=\max(a,b)$, un zéro de marge d'existence. Il est commun à HDBSCAN et à la tête,
car il vient de l'étage condensé. Aucune règle qui rend S séparé pour $g$ grand et absorbé pour $g$ petit ne l'évite
(proposition 10) ; ce qu'on peut choisir, c'est où il a lieu.

## 4. La complétion (d)

Après sélection, un site est **bruit** s'il n'est dans le bloc de mort d'aucun amas sélectionné : il a rejoint une
lignée sélectionnée après sa mort, ou n'a jamais rejoint qu'un ancêtre non sélectionné.

| Complétion | Définition | Propriétés | Prix |
| --- | --- | --- | --- |
| **aucune** (défaut) | le bruit reste bruit | équivariante, laminaire, sans paramètre, fidèle | rappel des sites retardés et de bord |
| **par lignée** (V2) | $x$ va à l'amas sélectionné que traverse la remontée de ses points qualifiés les plus bas, s'il est unique ; sinon bruit | équivariante (ex æquo : bruit), sans paramètre, fidèle à FULL ($x$ est dans l'amas discret du nœud de l'amas), ne crée aucune réunion | annule la marge pour la sortie plate (le retard servait la stabilité des dates) ; propre à la tour : pas d'analogue exact pour HDBSCAN |
| **ultramétrique** (témoin équitable) | $x$ va à l'amas sélectionné le plus proche en $u$ (premier rencontré en remontant) ; ex æquo : bruit | générique, s'applique aux deux hiérarchies | aveugle à la couverture : ex æquo dès que $x$ entre au-dessus de deux amas |
| **vote du § 9.1** | $\arg\max_c V_x(c)$, $V_x(c)=\sum S_\tau/T_x$ | dépend des faces construites (Gabriel) et de $p$ ; non laminaire par niveau ; demande un départage | exige les cofaces et leurs boules : contraire à l'invariant d'architecture v11 (pas de catalogue global de cofaces) ; écarté |

Fixture (`completion.py`) : R1 de l'auditeur plus un site $b_3=(74,30,0)$ qui fait de $\lbrace b_1,b_2,b_3\rbrace$ un
amas avant la fusion parasite $F=12$, $k=2$, mcs = 3, EOM z = 3. Sélection {b1, b2, b3}, {y, s2, s3} ; x (point de cœur
de l'amas, $d_2=10$) entre en $\sqrt{100}+\sqrt{250}-\sqrt{625/4}\approx 13{,}311$, après la mort des deux amas :
bruit. **Lignée** : x rejoint {y, s2, s3} (comme HDBSCAN). **Ultramétrique** : x reste bruit ($u$ égal vers les deux
amas). Sur les cinq points, toutes les complétions laissent 0 bruit (Can).

## 5. Fixtures (e) : scènes où HDBSCAN et la tête diffèrent

Tête : $H^{r}_{3}$ → A → EOM. HDBSCAN : scikit-learn, `min_samples` = 2, même mcs (et étage N-aire, qui coïncide ici
sauf aux cinq points).

| Scène | mcs | Tête (z = 1 et 3) | HDBSCAN scikit-learn | Lecture |
| --- | --- | --- | --- | --- |
| T0 exact (mcs 2 et 3), P2, S_plan, S_3D (mcs 3) | 2, 3 | **ABC \| DEF** | aucun cluster | cible de la thèse (§ 6.1) tenue par la tête seule |
| T0 P1 | 3 | ABC \| DEF | ABC \| DEF | côtés 1 999,956 < pont 2 000 : HDBSCAN sépare aussi |
| Q1 (pont 1 700) | 2, 3 | **ABC \| DEF** | aucun cluster | réponse Q1/Q1bis de l'utilisateur |
| cinq points | 2 | {1,2} \| {3,4}, 0 bruit | {0,1,2} \| {3,4} **ou** {1,2} \| {0,3,4} selon l'ordre | équivariance |
| Q3 | 3 | {c0..c7, **x**} \| {f1..f5} | {c0..c7} \| {**x**, f1..f5} | Q3 (a) de l'utilisateur : HDBSCAN conforme, la tête non (prix de $H^{r}_{k+1}$) |
| Q3 | 6 | aucun cluster (le filament n'a que 5 sites entrés) | {c0..c7} \| {x, f1..f5} | idem |
| Q3 + groupe lointain Z | 6 | {Q3 entier} \| {Z} | {c0..c7} \| {x, f1..f5} \| {Z} | idem |
| R1 | 2 | {b1,b2,s2,s3,x,y} \| {w1,w2} | {b1,b2} \| {s2,s3,x,y} \| {w1,w2} | la paire {b1,b2} n'est qualifiée qu'à la fusion parasite |

Q-Π2 au niveau plat : sur Q3 de base et ses variantes, aucune tête ne sort de cluster à mcs 9 (le bloc de 9 est la
chaîne de la racine, non sélectionnable). Une tentative pour rendre la différence visible au niveau plat (filament
prolongé à 9 sites et groupe Z, `qpi2_plat.py`) donne la même sortie plate pour A et C : le neuvième site du filament
n'est pas entré avant la fusion. **Q-Π2 est une propriété de la hiérarchie condensée par rayon** ; je n'ai pas
d'exemple où elle change la sortie plate, ce qui n'en prouve pas l'absence.

La cause est instructive. L'extrémité f9 est couverte par le filament dès 700 ($d_2=700$), mais la composante du groupe
lointain Z, à 13 700 unités, la couvre aussi vers 6 850 et ne rejoint sa lignée qu'à la racine, 7 200 : barre rivale
de persistance 350,09, soit presque le retard maximal $d_2/2=350$ de H5. f9 entre donc à 1 050,09, après la fusion du
filament et de l'amas (796,12). C'est l'anticipation non bornée de $H^{r}_{k+1}$ (H7) et le prix R1 (H5) sur une
figure simple : la date d'un site dépend d'un groupe très éloigné, né bien après. Le critère C en hérite au carré
($ho_{f9}=7\,200$).

## 6. Lecture critique

**Thèse.**

- § 4.4.4 (déf. 19) : excès de masse de Lebesgue, estimé par une somme qui estime le contenu de probabilité (3.4) ; et
  « HDBSCAN estime la densité avec l'inverse de la distance » n'est vrai qu'en dimension 1. Son propre modèle (densité
  K-NN de $\mathbb{R}^{p}$) donne $\lambda\propto r^{-p}$.
- § 5.1 : juste et utile ; la forme exacte est la proposition 8, et elle ne demande aucune binarisation.
- § 5.2 : la règle « parent si $loss(Père)<\sum loss(Fils)$ » doit comparer aux **optima** des sous-arbres (c'est ce que
  fait $\tilde E$ ; la figure le laisse ambigu). Le « hack » est sain : c'est le même programme dynamique.
- § 9.1 : masses fractionnaires incompatibles avec T0 à mcs 3 ; vote dépendant des faces construites et de $p$ ; les
  cofaces nécessaires ne sont pas dans l'architecture v11 ; l'algorithme 1 condense un arbre de Kruskal binaire « comme
  HDBSCAN » : il s'expose aux artefacts de binarisation de la section 3.5 ; sa ligne 9 (réaffectation au 1-plus proche voisin) est
  extérieure au modèle.

**Auditeur.**

- Sa demande méthodologique (Q6 : une impossibilité doit nommer ses axiomes) est respectée par le théorème 6 :
  localité au profil qualifié, comptage immédiat sans rival, stabilité $L$ ; sans eux, sa construction par nombre de
  sites montre qu'on peut passer des cellules.
- Sa chaîne de stabilité (Q1, $\psi\varphi=\mathrm{Up}(2\varepsilon)$) est ce qui rend la proposition 2 immédiate ;
  rien à objecter.
- Sa doctrine « refus plutôt qu'égalité supposée » est appliquée aux comparaisons d'EOM, ce que ni la v10 ni
  scikit-learn ne font.

**Juge précédent (synthèse du 3 octobre).**

- « Aucune structure isolée de $k$ sites n'est un bloc » : **faux tel qu'écrit** (fixture de la paire, bloc vivant sur
  $[41{,}512;42{,}741)$) ; à corriger en « … tant qu'aucune boule mixte ne la qualifie avant sa fusion ». À inscrire au
  registre des statuts avec la fixture.
- Masses entières : confirmé et motivé (proposition 3). Factorisation : confirmée et précisée (proposition 1).
- Complétion par lignée : définition reprise, avec la clause « s'il est unique » nécessaire à l'équivariance.
- Prédiction E1 (la qualification réduit la sur-segmentation de `cover` à mcs = k) : soutenue par le mécanisme
  (proposition 7 (i)), affaiblie par les feuilles de boule mixte (7 (iii)).

**v10 (données, pas autorité).**

- Tête « couverture + EOM z = 3 » : la couverture fonde une feuille sur chaque boule de K points ; $H^{r}_{k+1}$ les
  supprime (7 (i)). Le z = 3 est retrouvé ici par dérivation, mais la v10 n'en avait ni la preuve de monotonie ni la
  limite surfacique.
- Existence par taille de cœur (`VC[coeur,W1]`) : perd T0 (prouvé, toutes variantes). Son gain à mcs = K est l'effet
  anti-fragmentation que C cherche à garder sans perdre T0.
- Existence mûre : perd Q-Π2 pour tout $\theta$ (forme interpolée) et ne peut tenir T0 et Q-Π2 ensemble (forme
  géométrique) : prouvé (2.5).
- « HDBSCAN dépend de la machine » : la v10 en avait trouvé la cause (ordre de `np.argsort` aux arêtes égales) ; ce
  rapport montre qu'elle produit aussi des amas de stabilité nulle et une sortie non équivariante, et que l'étage
  N-aire appliqué à l'ultramétrique (indépendante de cet ordre) l'élimine (3.5).
- « z est un cadran de granularité » : c'était une observation ; c'est désormais le théorème 9.

**`Zoltan/` (conception, pas autorité).** La spécification de condensation de Zoltan prescrit les masses $m_\tau$ du
§ 9.1 (« jamais un comptage ») et un seuil **relatif** $\alpha$ au parent. Pour la sortie plate jugée contre HDBSCAN à
mcs, ces deux choix sont contraires aux résultats ci-dessus : les masses $m_\tau$ perdent T0 et rendent l'existence
discontinue (proposition 3), et un seuil relatif ne fixe aucune taille minimale (la spécification le dit : un arbre
binaire équilibré garde toutes ses scissions pour $\alpha\leq 1/2$), ce qui contredit la règle Π1 de l'utilisateur.
Ils peuvent convenir à un jeton appris ; la tête de clustering ne doit pas les reprendre sans mesure. En revanche, la
règle « zéro, une, plusieurs branches lourdes, en une seule fois » de Zoltan est exactement l'étage N-aire de 1.2.

**Ce rapport.** La recherche d'un basculement plat sur $H^{r}_{3}$ dans la famille du saut de scission n'a rien trouvé
(S1 est couvert par B avant que S ne soit qualifié) : le phénomène est démontré sur HDBSCAN et vaut pour la tête par
construction (même étage), sans fixture propre à $H^{r}_{3}$. La constante de B reste une conjecture.

## 7. Recommandation (f) et prédictions écrites d'avance

### 7.1 La tête et ses deux variantes

| | Hiérarchie | Existence | Sélection | Complétion |
| --- | --- | --- | --- | --- |
| **T (principale)** | $H^{r}_{k+1}$, $\kappa=1$, $m=k+1$ | A, sites entrés comptés 1 | EOM N-aire, $\lambda=r^{-3}$, racine exclue, parent aux ex æquo ; **z = 1 publié à côté** | aucune |
| V1 | idem | **C, sites résolus** | idem | aucune |
| V2 | idem | A | idem | **par lignée** |

**Protocole équitable** (conséquence de la proposition 1) : mêmes mcs $\in\lbrace k,10,20,\sqrt n\rbrace$,
`min_samples` = k ; la même condensation N-aire et le même EOM (z = 3 et z = 1) appliqués à l'ultramétrique de
HDBSCAN (« HDBSCAN-N »), scikit-learn brut publié à côté ; racine exclue pour tous ; même complétion pour tous (aucune ;
ultramétrique en témoin ; la lignée seulement pour la tour, déclarée comme bras propre à la tour) ; même machine ;
statistiques d'ex æquo publiées. Métrique plate : pour chaque objet vrai, IoU du meilleur cluster plat (les clusters
étant disjoints, au plus un dépasse 1/2 : appariement unique au-dessus de 1/2), moyenne par objet, part des objets au-
dessus de 1/2 (rappel), part des clusters au-dessus de 1/2 avec un objet (précision), nombre de clusters ; points void
exclus comme au niveau B.

### 7.2 Prédictions déterministes (théorèmes : une violation est un défaut d'implantation)

- **D1** : pour toute scène et toute hiérarchie (T, V1, HDBSCAN-N), $\mathrm{Sel}_{z=3}$ raffine $\mathrm{Sel}_{z=1}$, et
  le nombre de clusters à z = 3 est au moins celui à z = 1.
- **D2** : feuilles(V1) ≤ feuilles(T) à chaque (scène, k, mcs) ; sur les cinq variantes de Q3, T rend le bloc de 9 sur
  $[705;790]$ à mcs 9, V1 non.
- **D3** : HDBSCAN-N à z = 1 rend les étiquettes de scikit-learn sur toute scène dont l'arbre n'a aucune hauteur ex
  æquo (contrôlé ici avec scikit-learn 1.9.1 ; à rejouer avec la 1.7.2 de G4, sur la même machine que la tête).
- **D4** (E2) : sous requantification ou gigue appariées de pas $\varepsilon$, $\vert\Delta\tilde u_A\vert\leq 3\varepsilon$
  sur l'échantillon de paires (juge d'échantillon, jamais tableau par paire).

### 7.3 Prédictions statistiques (falsifiables, écrites avant toute mesure de la sortie plate)

Tailles : synthétique $n$ = 8 000, 16 000, 32 000 (huit familles, 5 % de bruit, deux graines) ; LiDAR : les cinq démos de
`Zoltan/`, les 71 trames voisines ; $k\in\lbrace 2,3,5,10\rbrace$.

- **S0 (ex æquo)** : sur LiDAR (grille 1 mm, distances carrées entières), au moins la moitié des scènes ont des hauteurs ex
  æquo dans l'arbre de HDBSCAN, et scikit-learn diffère de HDBSCAN-N sur au moins une scène. *Réfutée si* moins de la
  moitié.
- **S1 (grand mcs)** : à mcs $\in\lbrace 20,\sqrt n\rbrace$, $k\in\lbrace 5,10\rbrace$, z = 3, la moyenne d'IoU plate de T
  dépasse celle de HDBSCAN-N, intervalle bootstrap à 95 % au-dessus de 0, à $n$ = 8 000 et 16 000 ; à $k=2$,
  $\vert\Delta\vert<0{,}01$. *Raison* : au niveau B, $H^{r}_{k+1}$ gagne +0,050 et +0,078 à $k$ = 5 et 10 ($n$ = 8 000) ;
  une sélection commune en transmet une partie.
- **S2 (fragmentation à mcs = k)** : à $k$ = mcs = 5 sur les cinq démos, à z = 1 (condition de la mesure v10), T rend
  moins de 2 866 clusters par trame (moitié des 5 732 de `cover` en v10) mais plus que HDBSCAN-N, et une IoU plate
  inférieure à HDBSCAN-N au même z. *Raison* : 7 (i) supprime les feuilles d'une boule de K points, 7 (iii) en laisse.
- **S3 (V1)** : à mcs = k ($k$ = 5) sur LiDAR, feuilles(V1) ≤ 0,7 × feuilles(T), et IoU plate de V1 ≥ celle de HDBSCAN-N −
  0,01 ; à mcs ≥ 20, $\vert$V1 − T$\vert$ ≤ 0,01. *Réfutée si* V1 ne réduit pas les feuilles d'au moins 30 % ou reste
  sous HDBSCAN-N de plus de 0,01 à mcs = k.
- **S4 (V2)** : à mcs ≥ 20, la complétion par lignée réétiquette au plus 3 % des sites ; variation d'IoU plate dans
  $[-0{,}01;+0{,}01]$ en synthétique (le bruit vrai y est complété à tort), positive sur les instances LiDAR.
- **S5 (z)** : pour T comme pour HDBSCAN-N, l'IoU plate à z = 3 est au moins celle à z = 1 à $n\geq$ 8 000.
- **S6 (Zoltan)** : démo 04, vélo C (instance 56), $k=3$, mcs = 20, z = 3 : IoU plate de T ≥ 0,45 et supérieure à
  HDBSCAN-N (niveau B : 0,557 contre 0,348).

### 7.4 Coût et budget G4

L'étage condensé et l'EOM sont en $O(n\log n)$ par (hiérarchie, $k$, mcs, z) sur des événements déjà produits par le
banc ; $\rho_i$ (V1) se lit sur les incidences rivales déjà calculées par `points_radius.py` (rencontre maximale au lieu
de persistance maximale) ; HDBSCAN-N part de `_single_linkage_tree_`. Estimation (non mesurée) : moins de 10 % des
1 491 s de la session F (démos, voisines, synthétique 2 000 et 8 000). Les tailles 16 000 et 32 000 demandent une session
dédiée, l'export FULL dominant.

## 8. Statut des énoncés

| Énoncé | Statut | Où |
| --- | --- | --- |
| Condensation = atteignabilité mutuelle de $u$ à mcs (prop. 1) | prouvé ; contrôlé, 0 écart sur 103 428 | 1.4 |
| $\tilde u_A$ 3ε-stable (prop. 2) | prouvé (sur H3) ; observé 1,38 | 1.5 |
| Masses fractionnaires : seuil discontinu (prop. 3) | prouvé (exemple) | 1.3 |
| Aucun amas de durée nulle en N-aire | prouvé | 1.2 |
| Moins de feuilles pour des dates plus tardives (prop. 5) | prouvé ; 0 violation sur 80 | 2.2 |
| Q-Π2 exige $L>6{,}99$ (théorème 6, cadre intrinsèque) | prouvé | 2.4 |
| B perd T0 (toutes variantes) | prouvé | 2.5 |
| $E_\theta$ perd Q-Π2 pour tout $\theta$ ; forme géométrique incompatible avec T0 et Q-Π2 | prouvé | 2.5 |
| C sans constante de stabilité | contre-exemple exact (724,6 pour 1) | 2.5 |
| D viole Π1 | contre-exemple exact | 2.5 |
| Feuilles de $k$ sites par boule mixte (prop. 7 (iii)) | contre-exemple exact | 2.6 |
| Antichaîne optimale linéaire, N-aire (prop. 8) | prouvé | 3.1 |
| EOM monotone en $z$ (théorème 9) | prouvé ; 0 violation sur 9 000 paires | 3.2 |
| Équivariance et continuité incompatibles aux symétries (prop. 11) | prouvé ; fixture des cinq points | 3.6 |
| Constance par morceaux hors coïncidences (prop. 12) | esquisse sous position générale | 3.6 |
| Stabilité de B ≤ 3 | conjecture (observé 1,68) | 2.5 |
| Équivalence asymptotique de A, B, C (couche de bord négligeable) | conjecture | — |
| S0–S6 | prédictions | 7.3 |

## 9. Reproduction

Depuis `build/v11-points-select/modele/scripts/`, avec `PYTHONDONTWRITEBYTECODE=1 python3 -B` (numpy 2.5.3,
scikit-learn 1.9.1). Oracle de la définition lu en place (`contexte/oracle_hierarchy.py`), aucun fichier de la v11
modifié. Temps total inférieur à 3 minutes.

| Script | Sortie | Contenu |
| --- | --- | --- |
| `modele_lib.py` | — | rayons exacts, $H^{r}_{k+1}$, critères, treegramme, condensation N-aire, EOM certifié, complétions, HDBSCAN-N |
| `verif_eom_contre_sklearn.py 20261004 60` | `sorties/verif_eom_contre_sklearn.json` | 135/135 sans ex æquo ; 25 désaccords sur 165 avec ex æquo, 9 amas de stabilité nulle |
| `artefact_sklearn.py` | `sorties/artefact_sklearn.txt` | l'amas de 20 points né et mort au rayon 1,122811 |
| `verif_condensation.py 20261004 60` | `sorties/verif_condensation.json` | proposition 1 : 0 écart |
| `fixtures_existence.py` | `sorties/fixtures_existence.{txt,json}` | tableau 2.3, sorties plates, HDBSCAN |
| `qpi2_borne.py` | `sorties/qpi2_borne.txt` | théorème 6 : 7,039 ; 7,087 ; 6,992 ; 7,044 ; 7,076 ; $P_\kappa$ |
| `saut_resolution.py` | `sorties/saut_resolution.txt` | saut de $\rho$ de 724,587 pour un déplacement de 1 |
| `stabilite_condensee.py 20261004 300` | `sorties/stabilite_condensee.json` | 1,38 (A) ; 1,68 (B) ; 9,08 (C) |
| `sur_segmentation.py` | `sorties/sur_segmentation.txt` | proposition 7, paire sur $[41{,}512;42{,}741)$, proposition 5 |
| `z_monotonie.py 20261004 30 40` | `sorties/z_monotonie.json` | théorème 9 : 0 violation sur 9 000 paires |
| `equivariance.py` | `sorties/equivariance.txt` | cinq points : scikit-learn 60/60, HDBSCAN N-aire et tête 120/120 |
| `saut_eom.py` | `sorties/saut_eom.txt` | saut de création de scission |
| `completion.py` | `sorties/completion.txt` | R1 + b3 : lignée contre ultramétrique |
| `qpi2_plat.py` | `sorties/qpi2_plat.txt` | tentative plate de Q-Π2 (négative) |

Empreintes : `SHA256SUMS` (scripts et sorties).
