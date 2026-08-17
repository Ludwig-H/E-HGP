# Audit mathématique constructif : commits `0fb32c3` à `5072e23`

Date : 17 août 2026.  
Périmètre audité : les cinq commits postérieurs à l'audit `acd60d2`,
jusqu'au pin **`5072e235ba1194132f84a16420600f767fd7f811` inclus**.

Cadre : `exploration_v4_hors_registre`, `public_status=not_claimed`.
J'ai relu les Parties I–II du manuscrit à travers leur extraction versée dans
`audits/lectures_20260817/`, l'intégralité du dossier `morsehgp3D_v4`, les
audits antérieurs, les sources modifiées et les reçus. Le présent document est
une réception mathématique et statique indépendante ; les nombres `CTest`
restent des reçus de Claude, pas un substitut à la preuve.

Les commits examinés sont :

| commit | objet principal |
|---|---|
| `0fb32c3` | rayon couplé et raffinement `h_a/h_b` |
| `8f025cb` | reçu de sélectivité `h_a/h_b` |
| `214c2cc` | corrections contractuelles issues du premier audit |
| `68a33a0` | descente q2/q3/q4 fusionnée et comparaison Poisson |
| `5072e23` | instruction des supports q3 et oracle brut d'identités |

---

## 0. Verdict exécutif

Le progrès est réel et va dans la bonne direction.

### Reçu

1. **Le rayon couplé `R_coup` est sûr.** Sa dérivation et ses arrondis dirigés
   donnent bien une boule incluse dans tous les fuseaux du rectangle.
2. **Le critère `h_cœur+h_a+h_b` et son histogramme sont corrects** sous le
   contrat de sites distincts. Il ne matérialise pas le produit `A×B` pour
   compter les survivantes.
3. **La descente q2/q3/q4 fusionnée est fail-open.** Je n'ai trouvé aucune
   source de fausse mort dans les masques, les crédits de sous-arbres, les
   exclusions d'extrémités ou les comparaisons strictes.
4. **La forme de Gram q3 est exacte.** `q3_power` donne le signe exact de la
   puissance par rapport à la circum-boule du triangle aigu, et les bornes
   `axis_min/axis_max` sont orientées correctement.
5. **Le nouveau probe retrouve bien le même ensemble de supports q3 réguliers
   et peu profonds que son oracle brut partageant la même arithmétique.**
   C'est une étape utile et non un simple benchmark décoratif.

### À ne pas encore déclarer reçu

1. `q3_events_probe` ne construit pas encore des **événements HGP complets** :
   il construit des triplets de support. Il ne publie ni niveau exact, ni
   `BallKey`, ni profondeur, ni identités intérieures, ni hyperincidence.
2. Le « refus transactionnel » des coquilles n'est pas encore transactionnel :
   les supports dégénérés sont sautés et l'exécution retourne néanmoins
   succès. Le reçu `uniform, n=400` en compte déjà 15.
3. Le probe q3 annonce `h_cœur+h_a+h_b`, mais n'applique actuellement que
   `h_cœur` avant d'expanser toutes les ancres des rectangles survivants.
4. Les `PointId` du probe q3 sont des rangs de Morton (`ua,ub,ux`), pas les
   identités stables promises par le contrat.
5. La borne `O(s^3 n)` du front WSPD n'est pas encore reliée proprement au
   prédicat et au choix de scission effectifs.
6. Le census q3 reste « un porteur, une descente depuis la racine » ; les
   115,5 millions de porteurs de `eight_clusters, n=2000` mesurent ce verrou,
   mais ne constituent pas encore une architecture output-sensitive.

Le bon libellé actuel est donc :

> **énumérateur exact des supports q3 réguliers peu profonds, sous le même
> prédicat de puissance que l'oracle**, et non encore « instruction q3
> complète vers les événements ».

Cette restriction de vocabulaire n'enlève rien au cap franchi ; elle évite
simplement que le contrat public soit écrit par les commentaires avant de
l'être par le code.

---

## 1. Réception du rayon couplé

Écrivons les extrémités sous la forme

\[
a=c_A+u,\qquad b=c_B+v,\qquad \|u\|\le r_A,\quad \|v\|\le r_B,
\]

et posons

\[
c=\frac{c_A+c_B}{2},\qquad
p=\frac{u+v}{2},\qquad w=\frac{v-u}{2},\qquad d=\|c_B-c_A\|.
\]

Le milieu réel de `ab` est `c+p` et

\[
\|b-a\|\ge d-2\|w\|.
\]

Le fuseau d'arité `q` contient la boule centrée au milieu réel, de rayon
\(\kappa_q\|b-a\|\). Une boule centrée en `c` reste donc incluse dès que

\[
R \le \kappa_qd-\bigl(2\kappa_q\|w\|+\|p\|\bigr).
\]

Or l'identité du parallélogramme donne

\[
\|p\|^2+\|w\|^2=\frac{\|u\|^2+\|v\|^2}{2}
\le \frac{r_A^2+r_B^2}{2},
\]

puis Cauchy–Schwarz donne

\[
2\kappa_q\|w\|+\|p\|
\le
\sqrt{(4\kappa_q^2+1)\frac{r_A^2+r_B^2}{2}}.
\]

Ainsi

\[
R_{\mathrm{coup},q}
=
\kappa_qd-
\sqrt{(4\kappa_q^2+1)\frac{r_A^2+r_B^2}{2}}
\]

est bien un rayon universel sûr. La branche historique

\[
R_{\mathrm{dec},q}
=
\kappa_q(d-r_A-r_B)-\frac{r_A+r_B}{2}
\]

est également sûre. Les deux boules ayant le même centre, prendre
`max(R_dec,R_coup)` reste sûr.

L'implémentation entière respecte les bonnes directions :

- distance des centres minorée ;
- diagonales des boîtes majorées ;
- `A_q/D` sous-approche `2κ_q` ;
- `C_q/E` sur-approche `4κ_q²+1` ;
- terme positif arrondi vers le bas ;
- terme soustrait arrondi vers le haut.

La nouvelle fixture q4 discrimine utilement l'ancien rayon trop petit. Je
reçois donc ce bloc comme **`derive_v4` validé**.

Amélioration mineure, non bloquante : calculer explicitement le numérateur
`2*cq*s2` en `i128`, même s'il reste actuellement sous `2^57`, afin que la
preuve de largeur survive à une future extension du profil sans dépendre
d'une lecture attentive des constantes.

---

## 2. `h_a/h_b` : mathématique reçue, branche de production encore à factoriser

Pour un rectangle `A×B`, posons

\[
r_q=h_q-h_{\mathrm{cœur},q}.
\]

Pour une ancre `(a,b)`, le minorant de profondeur vaut

\[
h_{\mathrm{cœur},q}+h_{a,q}(a)+h_{b,q}(b).
\]

Les trois ensembles de témoins sont disjoints par identité. L'ancre survit
donc exactement au filtre lorsque

\[
h_{b,q}(b)<r_q-h_{a,q}(a).
\]

Avec les valeurs de `h_b` saturées à `r_q`, l'histogramme cumulatif

\[
C(t)=\#\{b\in B:h_b(b)<t\}
\]

donne bien, pour chaque `a`, le nombre de partenaires survivants sans former
`A×B`. Le code implémente la bonne inégalité stricte.

### 2.1 Divergence concrète dans le probe q3

Le commentaire de `q3_events_probe.cpp` annonce :

> rectangles vivants → ancres survivantes
> (`h_cœur+h_a+h_b<h_3`)

mais la boucle d'instruction parcourt directement tous les couples

```text
ua ∈ range(A), ub ∈ range(B)
```

des rectangles survivants du seul `h_cœur`. Elle n'appelle ni la phase
`h_a/h_b`, ni son histogramme.

Cela ne crée aucune omission : le probe fait simplement trop de travail.
Mais le reçu ne mesure donc pas encore la chaîne annoncée. Sur
`eight_clusters, n=8000`, la phase `h_a/h_b` déjà codée laisse 74,7 % des
ancres q3 survivantes ; son branchement retirerait immédiatement environ
25,3 % de la masse d'ancres après le cœur, avant toute lentille ou circum-boule.

### 2.2 Énumérer les survivantes sans produit cartésien

L'histogramme actuel compte les survivantes. Pour les instruire, il suffit de
le rendre énumératif, toujours en coût sortie-sensible :

1. ranger les `b` dans les `r_q+1≤10` seaux de valeur `h_b` ;
2. pour chaque `a`, calculer `t(a)=r_q-min(h_a(a),r_q)` ;
3. émettre les `b` des seaux `0,...,t(a)-1`.

Le coût est

\[
O(|A|+|B|+\#\text{ancres survivantes}),
\]

et non `O(|A||B|)`. Cette routine devrait devenir une primitive commune,
utilisée à la fois par le probe q234 et par l'instruction q3/q4. Cela évitera
que deux copies de la même étape divergent à nouveau.

### 2.3 La formation de `h_a/h_b` reste exploratoire

Le probe actuel calcule encore les comptes par auto-produit dans chaque
facteur, avec un cap de taille. Pour la version de production, la bonne
factorisation est la jointure dual-tree déjà esquissée en v3 :

- état `(U,Z)` pour les ancres `a∈U` et témoins `z∈Z`, le partenaire vivant
  dans la boîte fixe `B` ;
- si le prédicat `corner512` certifie
  `U×B×Z`, ajouter `weight(Z)` à toute la plage Morton de `U` ;
- si les boîtes sont séparées du prédicat, élaguer ;
- sinon scinder le plus grand facteur ;
- sur la diagonale, dérouler
  `(L,L),(L,R),(R,L),(R,R)` et retirer seulement la feuille identité ;
- matérialiser les mises à jour par tableau de différences/range-add ;
- saturer chaque lane à `r_q`.

Le même parcours symétrique donne `h_b`.

---

## 3. Descente q2/q3/q4 fusionnée

La fusion de `count_universal_witnesses_234` est conceptuellement saine :

- un seul nœud témoin est chargé ;
- le masque de lanes est local au sous-arbre ;
- un crédit ferme seulement la lane correspondante pour ce sous-arbre ;
- `Hmax≤0` élague simultanément les trois lanes ;
- q2 utilise son autorité `Hmin` ;
- q3/q4 utilisent leur boule-cœur puis, en mode terminal, l'autorité des
  coins ;
- aux feuilles, `(H,Ξ)` est calculé une fois ;
- les extrémités `A∪B` sont retirées de chaque crédit ;
- les comptes sont saturés au seuil.

Je ne vois pas de double comptage ni de fermeture injustifiée.

### 3.1 Deux corrections de coût faciles

**Lane à rayon nul.** En mode `with_corners=false`, si la boule q3 ou q4 a
`radius4==0`, cette lane n'a plus aucune autorité possible. Elle peut être
fermée à la racine. Aujourd'hui elle reste ouverte et peut traverser l'arbre
jusqu'aux feuilles, lesquelles ne créditent rien dans ce mode. C'est sûr mais
inutilement coûteux.

**Reprise terminale.** La tentative interne puis la tentative terminale
repartent encore chacune de la racine. La fusion est réelle à l'intérieur
d'un appel, pas encore entre les deux appels ni entre parent et enfants du
front. Après les portes de correction ci-dessous, le prochain gain naturel
est de retourner une continuation de frontière témoin pour reprendre
l'autorité des coins sur les seuls nœuds encore indécis.

### 3.2 Portes à ajouter avant optimisation

1. Parité bit à bit entre la fonction fusionnée et les trois parcours séparés,
   avec `with_corners=false` puis `true`, sur tous les rectangles de petits
   nuages.
2. Porte d'activité : au moins une fermeture par boule q3/q4 et au moins une
   évaluation de coins dans les fixtures calibrées.
3. Mutant « lane à rayon nul laissée active » : il doit changer les compteurs
   de travail, jamais les résultats.
4. Pour les blocs déclarés morts, juger exhaustivement tous les couples sur
   petites boîtes ; sur grandes boîtes, choisir en priorité la paire morte de
   marge minimale
   \[
   \Delta=h_{\mathrm{cœur}}+h_a+h_b-h_q\ge0,
   \]
   et non la paire `argmax/argmax`, qui est précisément la plus facile à tuer.
5. Porte anti-vacuité `h_a/h_b` : nombre d'évaluations positif, aucun rectangle
   silencieusement sauté, et gain minimal calibré sur `eight_clusters`.

---

## 4. Réception mathématique de la forme q3

Soient

\[
d=b-a,\quad u=x-a,\quad
D=d\cdot d,\quad E=u\cdot u,\quad F=d\cdot u,\quad
G=DE-F^2.
\]

Pour un triangle non dégénéré, le circumcentre s'écrit

\[
c-a=\alpha d+\beta u
\]

avec

\[
\alpha=\frac{E(D-F)}{2G},\qquad
\beta=\frac{D(E-F)}{2G}.
\]

En posant

\[
W=E(D-F)d+D(E-F)u,
\]

on a donc `W=2G(c-a)`. Pour `v=z-a`,

\[
\begin{aligned}
P(z)
&=G\|v\|^2-v\cdot W\\
&=G\bigl(\|z-c\|^2-\|a-c\|^2\bigr).
\end{aligned}
\]

Ainsi :

- `P(z)<0` : intérieur strict ;
- `P(z)=0` : coquille ;
- `P(z)>0` : extérieur.

La formule de `q3_power` est donc exacte.

La fonction est séparable et convexe par coordonnée. Le minimum sur les
points entiers d'un intervalle est bien atteint à l'un des deux entiers
voisins de `w_i/(2G)`, après clipping ; le maximum est atteint à une
extrémité. Les crédits et élagages de boîte ont les bons sens.

Le cast du sommet rationnel vers `i64` est sûr sous la précondition actuelle :
pour un triangle strictement aigu, le circumcentre appartient à
`relint conv{a,b,x}`, donc chaque coordonnée de `c-a` reste dans l'enveloppe
des deltas u16. Cette dépendance mérite néanmoins un `assert(G>0)` et un
commentaire explicite, ou mieux un constructeur vérifié renvoyant un statut,
car `axis_min` divise par `2G`.

---

## 5. Ce qui manque pour passer du support q3 à l'événement HGP

Le vecteur `events` contient seulement un `Key3`. Or l'événement du manuscrit
est la boule peu profonde, donc le simplexe

\[
\sigma=S\cup I_B,\qquad S=\{a,b,x\}.
\]

Il faut au minimum publier :

1. le `SupportKey` en **PointId stables** ;
2. la `BallKey` canonique ;
3. le niveau public exact `r²` ;
4. la profondeur `d=|I_B|` ;
5. les identités de `I_B` ;
6. les identités de coquille `U_B` ou, sous position générale, la preuve
   qu'il n'y en a aucune hors support ;
7. l'ordre `K=d+2` ;
8. le payload d'hyperincidence/multifusion et les facettes de rendu.

Les formules exactes sont déjà presque gratuites :

\[
r^2=\frac{D\,E\,\|b-x\|^2}{4G},
\qquad
c=\frac{2Ga+W}{2G}.
\]

Elles donnent directement une `ExactLevel` rationnelle et une `BallKey`
normalisée.

Comme `h_3≤9`, un événement survivant contient au plus huit points intérieurs.
On peut donc conserver les identités sans mur mémoire :

- premier passage : compte saturé à `h_3`, et détection d'une coquille ;
- seulement si le compte reste `<h_3`, second passage ou expansion des
  nœuds crédités pour récolter les quelques `PointId` intérieurs ;
- publication transactionnelle après validation complète du slab.

### 5.1 Le refus de coquille doit réellement refuser

Aujourd'hui :

```text
if (shell > 0) {
    ++shell_refused;
    continue;
}
```

puis le programme retourne `0`. Cela produit le sous-ensemble régulier, pas
le résultat exact du profil annoncé. Le reçu `uniform, n=400` contient déjà
15 tels cas : cette situation n'est donc pas théorique sur grille u16.

À court terme, je conseille trois modes explicitement distincts :

- `exact_reject_degeneracy` : accumuler les `DegeneracyRecord`, ne publier
  aucun événement, retourner `unsupported_degeneracy` si la liste est non
  vide ;
- `regular_subset_exploration` : comportement actuel, mais nom et statut
  `incomplete` visibles ;
- plus tard `exact_plateau_quotient` : grouper par `BallKey`, conserver tout
  le shell et traiter la cosphéricité comme une hyperincidence simultanée,
  sans jitter.

Le compteur doit être par `BallKey` dégénérée, avec au moins un représentant
de coquille, et non seulement par support rencontré.

### 5.2 Les identités ne sont pas les rangs de Morton

`anchor_owns_q3` et `Key3` reçoivent actuellement `ua,ub,ux`. Ce sont des
indices de positions uniques triées, pas les `PointId` stables du contrat.

Avant q4, il faut séparer définitivement :

```text
DenseIndex / MortonRank  : adressage interne ;
PointId                  : identité externe stable ;
SupportKey, EdgeKey      : exclusivement en PointId.
```

Même sans doublon, le `PointId` du site unique `u` doit venir du bucket
correspondant. À terme, `build_cloud_index` doit accepter des couples
`{PointId, position}` et vérifier l'unicité des identités. Une porte de
permutation doit préserver les mêmes `SupportKey` externes, pas seulement le
même arbre géométrique à renommage près.

---

## 6. Réponse Q9 : analogue axial exact pour q3

### 6.1 Le bon objet est un arrangement affine dans le plan médiateur

Fixons l'ancre `(a,b)`, son milieu `m=(a+b)/2`, et

\[
\Pi=\{c:(c-m)\cdot(b-a)=0\}.
\]

Pour tout site `z`, posons

\[
\phi_z(c)
=
\|z-c\|^2-\|a-c\|^2
=
\|z-m\|^2-\frac{\|b-a\|^2}{4}
-2(c-m)\cdot(z-m).
\]

Sur le plan `Π`, `φ_z` est **affine**. La profondeur de la boule centrée en
`c` et passant par `a,b` est

\[
N(c)=\#\{z:\phi_z(c)<0\}.
\]

Le census q3 d'une ancre est donc exactement une requête de niveau dans un
arrangement de droites du plan médiateur. C'est l'analogue 2D du balayage de
racines 1D de q4.

### 6.2 Factorisation exacte par droites radiales

Pour un porteur `x`, posons

\[
L=\|b-a\|^2,\qquad
w_x=2x-a-b,\qquad
V_x=Lw_x-(w_x\cdot(b-a))(b-a).
\]

`V_x` est entier, orthogonal à `b-a`, et non nul pour un triangle
non dégénéré. Le circumcentre de `{a,b,x}` appartient à la droite

\[
m+\mathbb R V_x.
\]

Normalisons `V_x` en une direction primitive canonique `P`. Sur cette droite,
écrivons `c=m+τP`. Pour tout site `z`, avec

\[
A_z=\|2z-a-b\|^2-L,\qquad
B_z=P\cdot(2z-a-b),
\]

on obtient l'identité entière

\[
4\phi_z(m+\tau P)=A_z-4\tau B_z.
\]

Chaque site donne donc une racine rationnelle unique

\[
\tau_z=\frac{A_z}{4B_z}
\]

si `B_z≠0`. Les cas `B_z=0` sont permanents : intérieur si `A_z<0`,
extérieur si `A_z>0`, dégénéré si `A_z=0`.

Si `p` est le nombre de permanents intérieurs, alors, hors racines égales,

\[
N(\tau)
=
p
+\#\{B_z>0:\tau_z<\tau\}
+\#\{B_z<0:\tau_z>\tau\}.
\]

Le porteur `x` est lui-même une racine. Par conséquent, si sa profondeur est
`<h_3`, sa racine appartient nécessairement :

- aux `h_3-p` premières racines d'entrée (`B_z>0`), ou
- aux `h_3-p` dernières racines de sortie (`B_z<0`).

**Conclusion : sous position générale, une droite radiale ne peut porter que
`2(h_3-p)` groupes de porteurs q3 peu profonds.** C'est exactement la
structure « racines extrémales » recherchée.

### 6.3 Ce que je coderais

La factorisation par direction est mathématiquement forte mais, sur un nuage
générique, beaucoup de directions primitives peuvent être singletons. Je
conseille donc un hybride.

#### Chemin A : groupes de directions suffisamment peuplés

- grouper les porteurs d'une ancre par `DirKey=primitive(V_x)` ;
- pour les groupes au-dessus d'un seuil, réutiliser la primitive q4 de
  sélection des premières racines d'entrée et dernières racines de sortie ;
- comparer les racines par produits croisés exacts ;
- traiter les égalités comme une dégénérescence de `BallKey`, jamais par ordre
  flottant.

#### Chemin B : dual-tree `CenterBlock × SiteNode`

Pour toutes les autres directions :

1. construire un petit arbre 2D sur les circumcentres candidats de l'ancre ;
2. le traverser conjointement avec l'arbre de sites ;
3. pour un bloc de centres `C` et un bloc de sites `Z`, borner
   \[
   \min_{c\in C,z\in Z}\phi_z(c),\qquad
   \max_{c\in C,z\in Z}\phi_z(c);
   \]
4. si le maximum est `<0`, créditer `weight(Z)` à tout le bloc de centres ;
5. si le minimum est `>0`, élaguer ; garder l'égalité ouverte pour détecter
   les coquilles ;
6. sinon scinder le bloc le plus large ;
7. saturer chaque centre à `h_3` et retirer immédiatement les blocs morts.

La puissance est affine en `c` ; pour un site ponctuel, les extrema sur une
boîte de centres sont aux coins. Pour un bloc de sites, une arithmétique
d'intervalles dirigée fournit d'abord un certificat sûr, puis la subdivision
rend la décision exacte. Le backend CPU peut utiliser des bornes rationnelles
`int256/cpp_int` ; le GPU n'a besoin que de la version fixe une fois les
largeurs reçues.

C'est préférable à « une descente de boule par porteur » : les témoins
universels d'un bloc de centres tuent simultanément des centaines de
circum-boules profondes, précisément le régime `eight_clusters`.

Le simple fait d'utiliser une grande boule couvrant toutes les circum-boules
ne suffit pas : elle donne des candidats témoins, pas des témoins universels.
Le bon prédicat partagé est `max_C φ_z<0`.

---

## 7. Réponse Q10 : continuation output-sensitive

Oui, un contrat à continuation est acceptable, car la sortie q3 peut être
quadratique. Mais il faut distinguer clairement :

- **ordonnancement utile** ;
- **preuve de complexité** ;
- **complétude mathématique**.

Trier les ancres par coût de lentille estimé améliore le temps jusqu'aux
premiers événements. Cela ne réduit pas le travail total et ne doit pas être
présenté comme une borne output-sensitive.

### Contrat conseillé

1. ordre déterministe par
   `(estimated_lens_mass, EdgeKey)` ou par nombre exact de candidats retourné
   par l'arbre ;
2. budget exprimé en unités reproductibles
   (`node_pair_visits`, `exact_power_tests`, octets de slab), pas en secondes ;
3. statut explicite `complete` ou `resource_exhausted`;
4. token contenant le digest du nuage/profil, le curseur d'ancre et la
   frontière dual-tree encore ouverte ;
5. reprise idempotente : tout découpage de budget produit le même digest final
   que l'exécution monolithique ;
6. publication par slabs transactionnels ;
7. tant que le statut n'est pas `complete`, aucune forêt ni partition ne peut
   être labellisée « HGP exacte ».

Le flux partiel d'événements peut être rendu, mais il doit porter
`coverage=incomplete`. Une forêt partielle est un objet différent.

### Deux certificats à développer avant de s'en remettre au budget

1. **Avant matérialisation des porteurs** :
   jointure `AnchorBlock × CarrierNode` avec les trois tests de lentille,
   d'extérieur de boule diamétrale et d'owner. Cela élimine les blocs qui ne
   peuvent porter aucun triangle aigu.
2. **Après matérialisation des centres** :
   jointure `CenterBlock × SiteNode` de la section précédente, qui élimine les
   grands blocs de circum-boules profondes.

Le second est le verrou principal ici : `eight_clusters, n=2000` dépense
environ

\[
115\,512\,175 / 249\,093 \simeq 464
\]

porteurs testés par événement, contre environ 23 sur `uniform`. Il existe donc
un facteur algorithmique important avant la simple fatalité de la taille de
sortie.

---

## 8. Réponse Q11 : oracle indépendant maintenant

Priorité à l'oracle indépendant **avant q4 et avant l'optimisation du
census**.

Le juge courant valide très utilement :

- la couverture des supports ;
- l'owner ;
- l'absence de doublons d'identités internes ;
- l'effet du préfiltre sur la complétude.

Mais le sujet et le juge appellent tous deux `q3_form` et `q3_ball_depth`. Ils
ne testent donc pas le calcul le plus délicat.

### Oracle proposé

Sur `n` petit :

1. énumérer tous les triplets de `PointId` stables ;
2. tester l'acuité par les trois produits scalaires, sans utiliser le test
   `V²>D²` du sujet ;
3. déterminer l'arête owner avec une implémentation séparée ;
4. construire le circumcentre avec des rationnels
   `boost::multiprecision::cpp_int`/`boost::rational`, par résolution explicite
   du système `2×2`, sans appeler `Q3Form` ;
5. comparer exactement chaque distance `|z-c|²` à `r²` ;
6. produire un enregistrement complet
   `(SupportKey, BallKey, ExactLevel, InteriorIds, ShellIds)` ;
7. comparer le multiensemble trié au sujet.

Fixtures et mutants minimaux :

- triangle aigu, obtus, rectangle et quasi-colinéaire ;
- point intérieur, extérieur et exactement sur coquille ;
- égalité de longueurs où le tie-break `EdgeKey` change l'owner ;
- permutation des entrées avec `PointId` conservés ;
- mutants `<→≤`, signe de puissance, oubli d'un support, owner par rang
  Morton, et arrêt précoce masquant une coquille.

Cet oracle servira ensuite de socle à q4, aux `BallKey`, aux niveaux rationnels
et au protocole transactionnel. Le construire après q4 reviendrait à
optimiser deux fois une arithmétique qui n'a été vérifiée qu'une fois, ce qui
est une économie assez théorique.

---

## 9. Correction de bord des constantes de Poisson

La phrase de la note Claude selon laquelle « à densité fixée, la fraction de
bord est invariante d'échelle » est fausse.

À intensité fixée, le rayon d'interaction typique reste `O(1)`, tandis que le
côté du cube croît comme `L∼n^{1/3}`. La fraction de points dans une couche de
bord d'épaisseur fixe est donc `O(1/L)=O(n^{-1/3})`. Le rapport aux constantes
sans bord doit converger vers `1`, à effets de grille près.

### 9.1 Constantes de volume infini

Soit

\[
p_h(t)=e^{-t}\sum_{j=0}^{h-1}\frac{t^j}{j!}.
\]

Si le fuseau normalisé a pour volume `v_q r³`, le nombre de paires vivantes
par point dans le Poisson homogène infini vaut

\[
C_{\infty,q,h}
=
\frac{2\pi h}{3v_q}.
\]

Ici

\[
v_2=\frac{\pi}{6},
\]

\[
v_3=\frac{\pi(27-4\sqrt3\,\pi)}{108},
\]

\[
v_4=
\frac{\pi\left(28-9\sqrt2\,\pi+
18\sqrt2\,\arcsin(1/\sqrt3)\right)}{96}.
\]

Pour `(h_2,h_3,h_4)=(10,9,8)` :

\[
C_\infty=(40.000,\ 123.796243,\ 139.069627).
\]

### 9.2 Terme de surface

Pour un cube de côté `L` et un Poisson d'intensité `λ`,

\[
C_{L,q,h}
=
C_{\infty,q,h}+\frac{6\beta_{q,h}(\lambda)}{L}
+O(L^{-2}).
\]

Posons

\[
I_h=\int_0^\infty t^{1/3}p_h(t)\,dt
=\sum_{j=0}^{h-1}\frac{\Gamma(j+4/3)}{j!}.
\]

Soit `g_q(μ,δ)` la fraction du fuseau conservée par un demi-espace, lorsque
l'axe de la paire a un cosinus normal absolu `μ` et que le milieu est à la
distance normalisée `δ/2` de la face. Les extrémités sont dans le cube pour
`δ≥μ`, et le fuseau est entièrement dedans pour `δ≥1`. Alors

\[
J_q=
\int_0^1\int_\mu^1
\left(g_q(\mu,\delta)^{-4/3}-1\right)
\,d\delta\,d\mu,
\]

et

\[
\beta_{q,h}(\lambda)
=
\frac{\pi}{3}\lambda^{-1/3}v_q^{-4/3}I_h
\left(J_q-\frac12\right).
\]

Le terme `-1/2` est la perte de paires dont une extrémité sort du cube ; `J_q`
est le gain de survie dû au fuseau tronqué près de la face.

Pour q2,

\[
g_2(\delta)=\frac{2+3\delta-\delta^3}{4},
\qquad
J_2=0.0920631601.
\]

Une quadrature déterministe des profils radiaux exacts donne

\[
J_3\simeq0.03172531,\qquad
J_4\simeq0.02557809.
\]

Dans le reçu `n=2000`, `coord=125`, donc
\(\lambda=2000/125^3=0.001024\). On obtient :

| lane | `C∞` | `β` | prédiction `C∞+6β/125` | observé |
|---|---:|---:|---:|---:|
| q2 | 40.000 | -165.82 | 32.04 | 32.3 |
| q3 | 123.796 | -860.51 | 82.49 | 86.3 |
| q4 | 139.070 | -1021.04 | 90.06 | 94.9 |

Le q2 est presque entièrement expliqué au premier ordre. L'écart résiduel
q3/q4 est compatible avec les arêtes/coins `O(L^-2)`, le modèle binomial à
`n` fixé et l'anisotropie de la grille entière.

Cette formule est une bonne explication et une bonne vérification de signe,
mais pas encore une porte dure sur la famille actuelle, qui est un échantillon
sans remise sur réseau entier, pas un Poisson continu.

### 9.3 Porte torique recommandée

Pour une porte de non-régression plus propre :

1. cube périodique de côté `L` ;
2. déplacement canonique par image minimale ;
3. seulement les paires `r<R<L/2` ;
4. pour chaque témoin, tester l'unique image périodique susceptible
   d'appartenir au fuseau ;
5. plusieurs graines et intervalle statistique.

Pour `n` points continus i.i.d. sur le tore, le nombre de témoins d'une paire
est binomial. Avec

\[
x_R=\frac{v_qR^3}{L^3},
\]

la constante tronquée exacte vaut

\[
C^{\mathrm{tor}}_{n,q,h}(R)
=
\frac{2\pi}{3v_q}
\sum_{k=0}^{h-1}
I_{x_R}(k+1,n-1-k),
\]

où `I_x` est la beta incomplète régularisée. Le résidu Poisson au-delà de `R`
est

\[
C_\infty-C(R)
=
\frac{2\pi}{3v_q}
\sum_{k=0}^{h-1}Q(k+1,\lambda v_qR^3),
\]

avec `Q` gamma supérieure régularisée.

Sur grille sans remise, soit on utilise cette cible comme limite à haute
résolution/faible taux d'occupation, soit on construit un petit oracle
hypergéométrique discret. À défaut de tore, une régression
`C_n=C∞+B n^{-1/3}+D n^{-2/3}` sur plusieurs tailles est plus honnête qu'une
comparaison brute à `C∞` à `n=2000`.

---

## 10. Raccord propre de la borne WSPD

Je suis d'accord avec Claude pour que **la scission soit pilotée par la cellule
de préfixe**, mais je ne conseille pas d'abandonner la boîte serrée pour le
terminal.

La route qui conserve à la fois la preuve et les bonnes constantes est :

1. stocker la cellule de préfixe exacte de chaque nœud ;
2. choisir le facteur à scinder par diamètre de cellule ;
3. déclarer terminal si
   \[
   \operatorname{SepCell}(A,B)
   \quad\textbf{ou}\quad
   \operatorname{SepTight}(A,B).
   \]

Définissons alors une récursion « ombre » utilisant les mêmes graines et les
mêmes scissions, mais seulement `SepCell`. C'est la WSPD de packing standard,
donc `O(s^3 n)`. La récursion réelle peut s'arrêter plus tôt grâce à
`SepTight` : son front terminal est un coarsening d'antichaîne de la récursion
ombre. Par conséquent

\[
\#\mathrm{rectangles}_{\mathrm{réel}}
\le
\#\mathrm{rectangles}_{\mathrm{ombre}}
=
O(s^3n).
\]

Pourquoi le `OR` est important : le prédicat entier sur les centres et les
diagonales est suffisant mais pas nécessaire. L'inclusion de la boîte serrée
dans la cellule ne garantit pas que **ce prédicat particulier** reconnaisse
toute séparation déjà reconnue par la cellule. Sans le `OR`, la domination
combinatoire n'est pas automatique.

Enfin, `cell_of_prefix` ignore actuellement les un ou deux bits résiduels du
préfixe Morton et stocke un cube extérieur répété. Deux choix propres :

- implémenter la cellule rectangulaire exacte du préfixe, de rapport d'aspect
  au plus 4 ;
- ou prouver explicitement qu'un même cube extérieur n'est répété que sur un
  nombre borné de raffinements et intégrer ce facteur dans le packing.

Le premier choix est plus simple à auditer. Les boîtes serrées restent
ensuite disponibles pour tous les certificats métriques.

---

## 11. Contre-audit de `ETAT_COURANT.md`

Je confirme les conclusions structurantes de l'autre auditeur :

- la bijection boule–événement sous sites distincts et position générale ;
- les rayons q2/q3/q4 corrigés ;
- la sûreté de l'autorité des coins par convexité séparée ;
- la nécessité de distinguer `F_K^conn` de `F_K^render` ;
- le niveau public au carré exact ;
- la multifusion des plateaux ;
- les applications verticales entre ordres ;
- le refus honnête des doublons tant qu'une sémantique pondérée n'est pas
  prouvée.

Mes raffinements sont les suivants :

1. les constantes de Campbell–Mecke sont des constantes de volume infini ; le
   reçu cubique exige le terme de bord ci-dessus ;
2. pour la WSPD, « scission cellule + terminal cellule OU serré » donne une
   preuve plus forte sans payer le facteur mesuré de la cellule seule ;
3. le commit q3 reçoit la géométrie du support, mais pas encore le contrat
   complet d'événement ;
4. le refus de coquille doit être un vrai statut transactionnel ;
5. les identités stables doivent être corrigées avant q4.

Je ne vois pas de conclusion antérieure à rétracter sur les certificats de
mort.

---

## 12. Ordre de travail conseillé à Claude

### P0 : verrouiller la vérité avant d'optimiser

1. Oracle q3 BigInt/rationnel indépendant.
2. Vrai statut `unsupported_degeneracy` ou mode
   `regular_subset_exploration` explicitement incomplet.
3. `PointId` stables dans `EdgeKey`, `SupportKey` et les sorties.
4. Renommer le probe actuel en « shallow q3 supports » tant que le payload
   complet n'est pas produit.

### P1 : fermer la chaîne q3

5. Brancher réellement `h_a/h_b` et énumérer ses survivantes par seaux.
6. Publier `BallKey`, `ExactLevel`, profondeur et `InteriorIds`.
7. Ajouter les portes de parité de la descente fusionnée.
8. Fermer immédiatement les lanes à rayon nul en mode sans coins.
9. Construire le dual-tree `CenterBlock × SiteNode` ; ajouter ensuite la voie
   radiale extrémale pour les groupes de directions rentables.

### P2 : recevoir la complexité et les mesures

10. Raccorder la WSPD par la récursion ombre `cell OR tight`.
11. Ajouter la porte torique ou la régression de bord.
12. Transformer le cap de ressources en continuation déterministe et
    transactionnelle.
13. Nettoyer les contradictions documentaires résiduelles :
    `MATHEMATIQUES.md` conserve encore l'ancienne ambiguïté active-only dans
    Q4, `types.hpp` annonce simultanément doublons admis et profil exact qui
    les refuse, et plusieurs commentaires gardent le statut « piste » pour
    des énoncés désormais reçus.

---

## Conclusion

Claude a correctement déplacé le projet du stade « front géométrique mesuré »
vers un début de chaîne d'événements exacte. Les deux avancées les plus
solides sont la descente de témoins réellement partagée et la puissance q3
entière.

Le prochain gain ne viendra pas d'un rayon supplémentaire ni d'un ordre de
boucle plus ingénieux. Il vient de trois opérations nettes :

1. **ne plus instruire les ancres déjà tuées par `h_a/h_b`** ;
2. **partager le census entre des blocs de circumcentres** ;
3. **publier un véritable événement transactionnel, avec identité, boule,
   niveau et intérieur**.

C'est un cours mathématiquement cohérent et implémentable. Je recommande de
poursuivre dans cet ordre plutôt que d'ouvrir q4 sur les contrats encore
implicites de q3.
