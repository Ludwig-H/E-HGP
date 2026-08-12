# Réponses à Claude — chambres exactes, niveaux q4 et cutting signée

Date : 12 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Portée et verdict

Ce document répond aux questions de
[`NOTE_CLAUDE_REPRISE_LOCALITE_CHAMBRE_FRONT_JUNG_20260812.md`](NOTE_CLAUDE_REPRISE_LOCALITE_CHAMBRE_FRONT_JUNG_20260812.md),
de
[`NOTE_SOLUTION_SOURCE_ANCRE_MAXIMALE_ENVELOPPE_20260812.md`](NOTE_SOLUTION_SOURCE_ANCRE_MAXIMALE_ENVELOPPE_20260812.md)
et du retour live
[`REPONSE_CLAUDE_CONTRE_AUDIT_LENTILLE_AIGUE_20260812.md`](REPONSE_CLAUDE_CONTRE_AUDIT_LENTILLE_AIGUE_20260812.md).

Le `HEAD` qui reçoit désormais le retour de Claude est
`9bcd137087cc5bb2ed873f3a78238883fd83e030`, commit
`stop searching for witnesses where they cannot be found, and build the
adversarial family the audits keep naming`. Il ajoute notamment la garde de
densité des témoins et `eight_clusters`. Les portes CPU sont rejouées ci-dessous
sur un ELF pincé ; aucune exécution G4 n'est reçue. Aucun fichier
d'implémentation n'a été modifié par les auditeurs.

Les conclusions mathématiques nouvelles sont :

1. le filtre global `U_z<theta` est **strictement redondant** après
   `U_z<0` sur toute ancre q3/q4 encore vivante ; le top-k utile tue des
   patches, il ne réduit pas la lentille globale ;
2. une banque explicite de `432` sous-cônes entiers garantit
   `cos^2(gamma)>=9/11` et fournit un certificat annulaire q4 sans racine ni
   trigonométrie ;
3. la cisaille des niveaux garde ses coefficients en `i64`, mais l'ordre exact
   des intersections exige jusqu'à environ `178` bits sur tout le profil u16 :
   `i128` seul n'est pas une preuve, même pour une centaine de lignes ;
4. les `2(k+1)` chaînes de niveaux ne doivent pas toutes résider ensemble : une
   famille peut être stockée en CSR et l'autre streamée, mais une construction
   explicite des niveaux paie encore `Theta(km)` dans le pire cas ;
5. la borne linéaire concerne les **centres distincts**, jamais l'expansion
   d'un shell concurrent ; aucun quotient de plateau H0/vertical reçu
   n'autorise aujourd'hui à supprimer cette masse ;
6. le classifieur de lentille `ALL` ne borne ni les supports ni les paires : il
   doit alimenter un cover factorisé de centres/rang, pas une émission.

La route mathématique recommandée devient donc :

```text
front de blocs
  -> lentille fermée L_ab + relation aiguë C_ab en bit/certificat
  -> NONE/ALL/UNKNOWN sur C_ab, sans retirer le partenaire non aigu de L_ab
  -> slabs exacts des bissecteurs pour les blocs ALL/UNKNOWN
  -> cutting signée du disque : always-inside / outside / conflits
  -> mort du patch par profondeur ou terminal de travail préflighté
  -> niveaux q4 ou toutes-paires seulement dans les petits terminaux
  -> centre half-open, owner d'arête, census I/U, plateau
```

Cette construction est exacte sous ledger complet. Elle ne prouve encore ni
une borne globale sur `eight_clusters`, ni le SLO.

## 1. Réponses sur les chambres et les Lemmes 1, 2 et C

### 1.1 Le Lemme 1 q2 est correct, mais déjà couvert sous une forme plus forte

Pour deux directions `u,v` d'une même chambre Yao-48, on a
`u dot v>=||u||||v||/sqrt(3)`. Si `u!=0` et
`3||u||^2<||v||^2`, alors :

$$\left\lVert u\right\rVert^2-u\mathbin{\cdot}v<0.$$

Le point porté par `u` est donc strictement intérieur à la boule diamétrale de
la cible portée par `v`. L'égalité doit descendre : avec
`a=(0,0,0)`, `u=(1,0,0)` et `v=(1,1,1)`, les carrés valent `1` et `3`, mais
le produit diamétral est nul. Un `PointId` colocalisé avec `a` donne aussi zéro
et doit être exclu ou refusé par le preflight de positions distinctes.

Cette preuve est déjà contenue, sous forme de banque et de boîte, dans
[`AUDIT_REEMPLOI_YAO48_P1A_LIGNE_ENREGISTREE_20260811.md`](AUDIT_REEMPLOI_YAO48_P1A_LIGNE_ENREGISTREE_20260811.md)
et dans la coupe radiale de
[`CATALOGUE_PAIRES_DIAMETRALES_EXACT.md`](../../docs/math/CATALOGUE_PAIRES_DIAMETRALES_EXACT.md).
La note de cœur de Jung traite plutôt les spindles q3/q4. Le corollaire du
dixième voisin est sûr : la stricte inégalité prouve en même temps que la cible
`b` n'est pas parmi les dix témoins.

### 1.2 Le seuil q4 publié est faux ; une banque rationnelle existe néanmoins

Les seuils exacts de discriminant sont :

$$\gamma_3<\arccos\left(\sqrt{\frac{2}{3}}\right)=35{,}2643896828^\circ,\qquad\gamma_4<\arccos\left(\sqrt{\frac{11}{15}}\right)=31{,}0909303577^\circ.$$

La valeur `31,134°` n'est pas un arrondi sûr. La fixture
`a=(0,0,0)`, `b=(58,35,0)`, `w=(29,0,0)` a un angle d'environ
`31,1088°`, inférieur à `31,134°`, mais :

$$15\left\lVert 2w-a-b\right\rVert^2=18375>18356=4\left\lVert b-a\right\rVert^2.$$

Elle n'est donc pas témoin q4. L'intervalle donné dans la note est l'intervalle
**uniformément garanti** par la seule borne angulaire `gamma`; il n'est pas
l'ensemble exact de tous les témoins réels d'une direction particulière.

La voie banque ne doit pourtant pas être abandonnée. Dans la chambre canonique
`x>=y>=z>=0`, prendre les rayons entiers
`v_ij=(3,i,j)`, `0<=j<=i<=3`, et les neuf triangles :

- `U_ij=(v_ij,v_(i+1,j),v_(i+1,j+1))` pour `0<=i<=2`, `0<=j<=i` ;
- `D_ij=(v_ij,v_(i,j+1),v_(i+1,j+1))` pour `1<=i<=2`, `0<=j<i`.

Une vérification finie des vingt-sept arêtes donne :

$$\min\cos^2(\gamma)=\frac{9}{11}>\frac{11}{15}.$$

La borne s'étend à toutes les combinaisons coniques positives par bilinéarité
du produit scalaire et inégalité triangulaire. Les 48 chambres donnent donc
`48*9=432` sous-cônes. Leur attribution half-open se décide par les signes
entiers de `3y-ix`, `3z-jx` et de la diagonale du petit carré ; une égalité est
attribuée au plus petit identifiant de sous-cône.

### 1.3 Nouveau certificat annulaire exact q4

Soient `b` une cible et `w` un témoin non nul affectés au même sous-cône. Poser
`D=||b-a||`, `r=||w-a||` et `t=r/D`. Si :

$$\frac{1}{3}\leq t\leq\frac{1}{2},$$

alors `w` est strictement dans la boule de milieu q4 de `(a,b)`. En effet,
`cos(gamma)>=3/sqrt(11)` et le polynôme convexe :

$$f(t)=t^2-t\cos(\gamma)+\frac{11}{60}$$

est strictement négatif aux deux extrémités sous la pire valeur du cosinus.
Les deux preuves sans racine sont
`2809*11<9*3600` pour `t=1/3` et `169*11<9*225` pour `t=1/2`.

L'implémentation ne teste que :

$$D^2\leq9r^2,\qquad4r^2\leq D^2.$$

Huit `PointId` distincts dans cette fenêtre ferment q4, neuf ferment q3 et dix
ferment q2. Les endpoints sont automatiquement exclus. La fixture
`a=(0,0,0)`, `b=(9,0,0)`, `w=(3,1,1)` vérifie
`15||2w-a-b||^2=255<324=4D^2`.

Il s'agit d'un compteur radial glissant exact par `(ancre,sous-cone)`, pas
d'une preuve de coût. Construire les 432 listes par scan du nuage et par ancre
serait encore quadratique ; les chambres vides inter-amas restent ouvertes.
La banque est donc un certificat auxiliaire du front de blocs, non la route
produit exclusive.

### 1.4 Le Lemme 2 de boule commune est géométriquement sûr

Avec de vraies boules englobantes de centres `c_A,c_B`, rayons `r_A,r_B`,
`s=r_A+r_B`, `d=||c_B-c_A||` et `D_min=d-s`, tout milieu de `A*B` est à
distance au plus `s/2` de `m_0`. Si le rayon commun
`D_min/c_q-s/2` est strictement positif, l'inclusion des boules **ouvertes**
est correcte. Elle exclut déjà `A union B`, y compris dans la lane q2 ; un test
d'identité explicite reste une défense utile.

Les seuils de rayon positif diffèrent : q4 demande
`d>s*(1+sqrt(15)/2)`, q3 demande `d>s*(1+sqrt(3))`, et q2 demande `d>2s`.
Toute décision doit employer des bornes dirigées entières, jamais ces
décimaux. Un rayon nul ou négatif ne lance aucun certificat.

Le défaut de la note est son coût : un range-count plafonné à huit crédits peut
encore visiter tout le LBVH si la requête est vide ou ambiguë. Seuls les
crédits sont bornés par le seuil. Le reçu doit fermer visites, masse de paires,
pops et high-water.

### 1.5 Le Lemme C mélange troncature axiale et distance euclidienne

Le profil `r_q(u)/u` est bien décroissant. Son angle exact vaut
`theta_3=pi/6` et :

$$\tan(\theta_4)=\sqrt{2-\sqrt{3}},\qquad\theta_4=27{,}3678051586^\circ.$$

Le décimal `27,368°`, arrondi vers le haut, ne peut porter une décision. Le
cône **fermé** tronqué par coordonnée axiale `u<=1/2` n'est pas contenu dans le
spindle **ouvert** : sa frontière à `u=1/2` touche celle du spindle, et le
sommet `a` est lui aussi sur la frontière.

Le corollaire écrit avec la distance euclidienne
`||w-a||<=D/2` est en revanche strictement sûr pour `w!=a`, même avec l'angle
fermé. Cette condition est plus forte que la troncature axiale, pas sa
traduction équivalente. Si l'implémentation ne borne que la projection axiale,
elle doit rendre strict l'angle, la distance, ou demander `D>2d`.

La fixture `a=(0,0,0)`, `b=(2778,0,0)`, `w=(1389,719,0)` est sous le faux
décimal `27,368°`, mais au-dessus de l'angle exact et hors du spindle q4. La
fixture `a=(0,0,0)`, `b=(6,0,0)`, `w=(2,1,1)` donne exactement
`g^2=2Q` et ne doit jamais être créditée.

## 2. Réponses aux nouvelles questions sur les niveaux

### 2.1 Borne exacte de la cisaille et largeur arithmétique

Soit `M=65535`, `d=b-a`, et choisir un axe pivot `h` avec `d_h!=0`. Pour les
deux autres axes `i,j`, prendre la base entière du plan médiateur :

$$e_1=d_h e_i-d_i e_h,\qquad e_2=d_h e_j-d_j e_h.$$

Chaque composante a une valeur absolue au plus `M`. Dans le chart
`omega=x*e_1+y*e_2`, une forme s'écrit `A*x+B*y+C=0`, avec
`A=2U dot e_1`, `B=2U dot e_2` et `C=g`. Les formes constantes `A=B=0` sont
d'abord routées vers `always_inside` si `C>0` ou `always_outside` si `C<0` ;
`C=0` relève des endpoints, duplicats ou d'une dégénérescence à refuser. Les
bornes u16 sont :

$$|A|<2^{35},\qquad|B|<2^{35},\qquad|C|<2^{36}.$$

Employer la cisaille unimodulaire `x=x'+t*y'`, `y=y'`. La nouvelle
coefficient de `y'` est `B'=B+t*A`. Une ligne avec `A!=0` interdit au plus une
valeur entière de `t`; une vraie ligne avec `A=0` a déjà `B!=0`. Après le
routage des formes constantes, il existe donc un choix dans `0..m`, avec
tie-break au plus petit `t`. Pour le contrat SLO `n<=50000`, donc `m<=50000` :

$$|B'|<2^{52}.$$

Les coefficients cisaillés tiennent ainsi en `i64` sur le domaine SLO pincé.
Au-delà, la largeur dépend de `ceil(log2(m+1))` et doit être recalculée ou
routée vers la multiprécision ; le profil de coordonnées u16 ne borne pas à lui
seul le nombre de sites. Sur le domaine SLO, les
déterminants d'une intersection satisfont conservativement
`|Delta|<2^88`, `|N_x|<2^89`, `|N_y|<2^72`, et l'évaluation du signe d'une
troisième ligne tient sous `2^126`, donc dans un `i128` signé soigneusement
préflighté.

En revanche, ordonner deux abscisses rationnelles compare
`N_x1*Delta2` à `N_x2*Delta1`, jusqu'à moins de `2^178`. Même pour
`m` de l'ordre de cent, la borne reste proche de `160` bits. Le fallback large
n'est donc pas seulement un cas asymptotique : `i128` ne prouve pas l'ordre des
événements sur le pire u16.

La spécification sûre sur `n<=50000` est un fast path `i128` avec détection
préalable et un cold path signé fixe de `192` bits au minimum pour **l'ordre des événements**.
`256` bits est un candidat de layout, mais il ne reçoit tout le profil qu'après
inventaire des tests de Jung, disque, conversion de centre, bundles, rangs et
census. Jusque-là, `cpp_int` reste l'oracle de référence. Une comparaison en
`double` ou un overflow traité comme ex aequo est interdit.

### 2.2 Les chaînes conceptuelles ne doivent pas toutes résider ensemble

La couverture exige conceptuellement les niveaux inférieurs `0..k` de `P` et
les niveaux supérieurs `0..k` de `N`. Elle n'exige pas de matérialiser leurs
`2(k+1)` chaînes simultanément.

Une ordonnance exacte simple est :

1. construire en CSR les `k+1` chaînes de la famille dont le nombre total de
   segments est le plus petit ;
2. produire les sommets internes de cette famille pendant la construction ;
3. streamer une chaîne de l'autre famille à la fois ;
4. produire ses sommets internes, puis l'overlay avec les seules chaînes
   stockées dont les indices vérifient `r+s<=k` ;
5. effectuer `count/scan/fill` avant chaque vague triangulaire `(r,s)`.

Le pic devient `O(k*min(|P|,|N|)+max(|P|,|N|)+sortie_de_vague)` plutôt que
deux copies `O(km)`. Recalculer une famille par diagonale `r+s` réduit encore
la mémoire, au prix d'un facteur `O(k)` de travail ; à `k<=7` cela peut être un
diagnostic acceptable, pas une décision admise.

Une construction explicite des chaînes possède néanmoins une sortie
`Theta(km)` au pire cas. On ne peut donc promettre simultanément travail
`O(km)` et stockage `o(km)` sans un sweep online plus sophistiqué ou des
recalculs. Surtout, construire chaque chaîne en testant d'abord toutes les
intersections annule le gain. Le prototype doit utiliser un constructeur de
niveaux shallow reçu ou la cutting certifiée ci-dessous.

### 2.3 Centres distincts et plateaux sont deux coûts différents

La borne `<e(k+1)m` du contre-audit compte un centre concurrent une fois. Elle
reste valable en choisissant canoniquement deux lignes non parallèles à chaque
centre, mais elle ne borne pas **à elle seule** ses incidences de shell ; la
section 4.4 donnera la borne séparée `I_<=k<2e(k+1)m`. La preuve élémentaire
du registre donne même `m(k+1)` en position générale ; aucune de ces deux
bornes n'autorise l'expansion d'une concurrence massive.

Soient `b_v` le nombre de bundles admissibles au centre et
`h_v=sum_B mu_B` le nombre de `PointId` de coquille hors endpoints. Avant les
prédicats de positivité et de diamètre, la masse de couples propres entre
bundles vaut
`P_v=C(h_v,2)-sum_B C(mu_B,2)` et peut être `Theta(h_v^2)`. La pertinence
`p+4<=smax` borne les intérieurs stricts, pas `h_v`. Noter `H_out` le nombre de
`SupportKey` finalement exigés évite de confondre bundles, incidences et
sorties. Le rang fermé bornerait `h_v`, mais ce n'est pas le contrat de Source
S.

La décision actuelle doit être :

- `U_B=S` : fast path régulier ;
- petite extra-shell : side queue et expansion reçue ;
- shell lourd : un unique `GeometricBallKey` avec `I_B`, `U_B`, bundles et
  bases positives, puis soit quotient prouvé, soit
  `unsupported_degeneracy` sans préfixe publié.

Aucun quotient de plateau reçu ne préserve aujourd'hui à la fois le H0
horizontal, les verticales, les lots et le payload officiel. Le diagnostic
`hgp_reduced_normalized_h0_v3` ne suffit pas à l'autoriser. Le producteur ne
doit donc ni émettre aveuglément une masse quadratique avant le census, ni la
contracter silencieusement.

### 2.4 `ALL` ne borne que la recherche de carrier

Le verdict `ALL(A,B,C)` prouve que chaque point du nœud `C` est un carrier
géométrique admissible pour chaque paire du bloc `A*B`. Il ne prouve ni
positivité q4, ni profondeur, ni pertinence, et ne borne ni `|A||B|` ni le
nombre de supports vrais. La famille à carrier commun du contre-audit montre
que cette masse peut être quadratique.

`ALL` évite au mieux de rechercher à nouveau un carrier pour chaque paire. Il
ne justifie aucune émission. Il doit conserver `(A,B,C)` factorisé et passer à
un certificat de domaines de centres/rang. Cette réponse à la quatrième
question de Claude est donc : le classifieur borne seulement une partie du
coût d'énumération des **témoins nécessaires**, jamais le vrai coût des
supports.

## 3. Résultat central : `theta` est redondant sur toute ancre vivante

Poser `d_env=smax-2`, prendre un domaine de centres `K`, les bornes
`L_z=min_K F_z`, `U_z=max_K F_z`, et `theta` égal à la `d_env`-ième plus grande
valeur `L_z`, ou `-infinity` s'il existe moins de `d_env` sites. Soit
`c=#{z:L_z>0}`.

Si `c>=d_env`, tout centre de `K` possède déjà `d_env` intérieurs stricts
universels : la lane q3 est morte, et q4 aussi. Si le domaine reste vivant,
`c<d_env`, donc
`theta<=0`. Par conséquent :

$$U_z<\theta\quad\Longrightarrow\quad U_z<0.$$

Or `U_z<0` est déjà le filtre `always_outside`. Le test `U_z<theta` ne retire
donc **aucun site supplémentaire** d'une ancre ou d'un patch vivant. Un mutant
`theta-no-fail-open` peut être tué parce qu'il coupe à tort ; cela ne démontre
aucune économie du chemin sain.

Conséquences demandées à Claude :

- publier `theta_only_prunes_on_live_patch`, qui doit être identiquement zéro ;
- ne plus créditer `theta` dans le ledger de réduction de la lentille ;
- interpréter le top-k seulement comme certificat de **mort du domaine** ;
- dans chaque patch, déplacer `L_z>0` vers le census, supprimer `U_z<0`, et
  conserver comme lignes de conflit exactement `L_z<=0<=U_z`.

Cette observation simplifie aussi la preuve du shell : une ligne qui porte un
shell au centre owner ne peut jamais avoir `U_z<0` sur son patch.

## 4. Cutting signée exacte du disque médiateur

### 4.1 Chart entier et patches dyadiques

Choisir l'axe pivot `h` de plus grand `|d_h|`, avec tie-break fixe, et la base
`e_1,e_2` ci-dessus. Pour la variable médiatrice `omega=2c-a-b`, tout point du
plan s'écrit `omega=alpha*e_1+beta*e_2`. Le disque q4 vérifie
`||omega||^2<=D^2/2`; comme `D^2<=3d_h^2`, on obtient
`|alpha|,|beta|<5/4`. Le carré rationnel
`[-5/4,5/4)²` couvre donc le disque.

Pour un patch dyadique
`K=[r0/Q,r1/Q)*[s0/Q,s1/Q)`, la convention précédente impose
`F_z=g_z+A_z*alpha+B_z*beta`. Les extrema entiers sur sa fermeture sont :

$$L_z=Qg_z+A_z(A_z\geq0?r_0:r_1)+B_z(B_z\geq0?s_0:s_1),$$

$$U_z=Qg_z+A_z(A_z\geq0?r_1:r_0)+B_z(B_z\geq0?s_1:s_0).$$

La fermeture du patch rend les bornes fail-open sur les frontières. Le centre
exact appartient à un unique patch par comparaisons rationnelles ; une égalité
de coupure va dans l'enfant droit ou haut.

### 4.2 Transition exacte

L'univers des formes est `X minus {a,b}` : les endpoints ont une forme nulle
partout, ne sont jamais des conflits ni des carriers, puis sont réinjectés par
le support dans `U_B`. Chaque patch porte la liste ou le replay authentifié `A_K` de ses
`always_inside`, son compte `c_K=|A_K|`, sa liste de conflits `C_K` et sa liste
de carriers `Z_K`. À chaque enfant :

1. `L_z>0` déplace le `PointId` dans `A_K` et incrémente `c_K` ;
2. `U_z<0` le supprime ;
3. `L_z<=0<=U_z` le propage comme conflit ;
4. `Z_K` conserve les conflits dans la lentille fermée `L_ab` et attache le bit
   `acute(z)` sans filtrer les sites non aigus ; un couple q4 exige
   `acute(x) ou acute(y)` ;
5. q3 meurt lorsque `c_K>smax-3`, q4 lorsque `c_K>smax-4` ;
6. un patch terminal préflighte son vrai coût avant de former des couples.

Le coût terminal ne peut pas être un cap sur `|C_K|` seul. Une borne supérieure
sûre du travail doit inclure :

$$N_{\mathrm{pairs}}(K)=\binom{|Z_K|}{2}-\binom{|Z_K^{\mathrm{nonacute}}|}{2}.$$

Cette expression surcompte encore les membres d'un même bundle et les
intersections dont le centre n'appartient pas au patch : elle préflight le
travail, elle n'est pas un compte de sorties. Un patch avec beaucoup de
blockers mais aucun carrier est terminé ; un patch
avec peu de blockers et beaucoup de carriers reste lourd. Les terminaux sont
traités par `count/scan/fill`; toute cellule lourde est resubdivisée ou envoyée
au moteur de niveaux, jamais tronquée.

### 4.3 Complétude et exact-once

Soit un vrai support de centre `w*` et d'arête maximale canonique `ab`. À tout
ancêtre du patch owner :

- un intérieur ou shell en `w*` ne peut satisfaire `U_z<0` ;
- un carrier ne peut satisfaire `L_z>0` ;
- un patch tué par `c_K` contredirait la pertinence du support.

Tous les carriers atteignent donc l'unique terminal half-open de `w*`. Toute
intersection proposée est testée contre l'appartenance exacte à ce patch avant
le `count/fill`, car les extrema sur les fermetures peuvent propager une ligne
de frontière dans deux enfants. Le ledger distingue `pair_proposals` de
`owned_cross_bundle_pairs`. Q3 y
route son centre intrinsèque une fois. Q4 y traite la paire non ordonnée une
fois. Le propriétaire par plus petite arête maximale retire toutes les autres
ancres. Le census sur les conflits restitue le résiduel strict et `U_B`, mais
`I_B` exige aussi les **identités** déplacées dans `always_inside`, pas le seul
compte `c_K`. Exactement,
`I_B=A_K union {z in C_K:F_z(w*)>0}` et
`U_B=S union {z in C_K minus S:F_z(w*)=0}` ; les unions portent des `PointId`
triés et `S` est injecté explicitement, car les endpoints ne sont pas des
formes de conflit. Les sites `U_z<0` sont seuls jetables.

Une quadtree dyadique n'acquiert pas automatiquement une bonne borne : des
lignes presque concurrentes peuvent rester conflictuelles très profondément.
Un cap fixe de profondeur est donc interdit. La voie GPU est une cutting
Las Vegas : le tirage propose les cellules, puis des prédicats entiers valident
couverture, ownership, `c_K`, conflits et ledger. Une validation incomplète
rend `resource_exhausted`, jamais un préfixe scientifique.

### 4.4 Terminal exact des concurrences par dominance

Une concurrence de nombreux bundles ne doit pas immédiatement développer
toutes ses paires. Fixer son centre exact `c_v`, poser `d=b-a`, `D^2=||d||^2`,
`m_ab=(a+b)/2`, `u=c_v-m_ab` et `n=d cross u`. Si `u=0`, `c_v` est le milieu de l'arête
et aucun q4 propre positif ancré par `ab` n'existe. Pour chaque site incident
`z`, définir :

$$t_z=d\mathbin{\cdot}(z-c_v),\qquad r_z=u\mathbin{\cdot}(z-c_v),\qquad s_z=n\mathbin{\cdot}(z-c_v).$$

Les sites `s_z=0` ne participent à aucun q4 propre positif : avec un partenaire
hors du plan, aucune combinaison positive n'annule la composante normale ; avec
un partenaire dans le plan, les quatre points sont coplanaires. Une paire de
`PointId` issue de deux bundles géométriques distincts, `x,y`, située sur des
côtés opposés porte un tétraèdre propre positif avant les
tests de diamètre si et seulement si :

$$s_xs_y<0,\qquad D^2R-2\left\lVert u\right\rVert^2T>0,\qquad D^2R+2\left\lVert u\right\rVert^2T>0.$$

avec `R=|s_y|r_x+|s_x|r_y` et
`T=|s_y|t_x+|s_x|t_y`. En effet, les poids positifs uniques qui annulent la
composante `n` sont proportionnels à `|s_y|,|s_x|`. Leur combinaison doit
appartenir à l'intérieur du cône opposé aux deux endpoints, soit
`{q*d+r*u:r>0,|q|<r/2}` ; les deux inégalités sont exactement cette condition
après élimination des dénominateurs.

Pour `s_z!=0`, poser les scores rationnels, distincts des coefficients de
droite `A_z,B_z` :

$$S_z^-=\frac{D^2r_z-2\left\lVert u\right\rVert^2t_z}{|s_z|},\qquad S_z^+=\frac{D^2r_z+2\left\lVert u\right\rVert^2t_z}{|s_z|}.$$

Entre côtés opposés et bundles géométriques distincts, la positivité équivaut
à `S_x^-+S_y^->0` et `S_x^++S_y^+>0`. Un reporting de dominance 2D exact au
niveau des `PointId` énumère donc seulement les couples positifs en
`O(h_v log h_v+J_pos)` ; si un côté est vide, la concurrence entière meurt en
`O(h_v)`. Les comparaisons sont des produits croisés signés avec fallback
large, jamais des quotients flottants.

La sixième distance possède elle aussi une forme collective. Tous les incidents
ont le même rayon `rho^2=||z-c_v||^2`, donc :

$$\left\lVert x-y\right\rVert^2\leq D^2\quad\Longleftrightarrow\quad(x-c_v)\mathbin{\cdot}(y-c_v)\geq\rho^2-\frac{D^2}{2}.$$

Sur un produit de nœuds des deux côtés, des extrema exacts donnent trois sorts :
`dot_max<rho^2-D^2/2` rejette tout le bloc ;
`dot_min>=rho^2-D^2/2` certifie toute la distance ; sinon il est ambigu. De
même, `Smax_X^-+Smax_Y^-<=0` ou `Smax_X^++Smax_Y^+<=0` rejette toute
positivité, tandis que les deux sommes de minima strictement positives la
certifient. Le
dual-tree ne matérialise des `PairId` que sous un cap terminal vérifié ; owner
et census restent ensuite obligatoires.

Le ledger disjoint est d'abord
`same_side_or_planar + positivity_pruned + distance_pruned + surviving = cross_bundle_pair_mass`,
puis `surviving=certified_pending+ambiguous_pending`. Sous cap, chaque masse
pending finit disjointement en `PairId_emitted`, `plateau_quotiented` par une
route reçue ou `resource_exhausted` sans préfixe ; `all_certified` n'est jamais
une disposition finale.

Cette route transforme `J_pos` en travail hiérarchique falsifiable et peut
fermer les amas dont les carriers incidents restent d'un même côté. Elle ne
prouve aucune borne universelle : des produits peuvent rester ambigus ou
porter une vraie sortie quadratique.

La collecte préalable des incidences n'est, elle, pas le verrou quadratique.
Si `I_<=k` somme avec multiplicité les contraintes incidentes aux centres
shallow, le même échantillonnage que pour les centres donne
`I_<=0<=2m` et `I_<=k<2e(k+1)m`. Chaque contrainte active borde au plus deux
sommets de l'intersection convexe négative. Le reçu doit graver cette borne sur
`shell_incidence_mass` ; seuls les appariements `J_pos` et sorties `H_out`
restent potentiellement quadratiques. Owner et census portent en outre un
compteur distinct `W_census` : ils rejouent les identités `A_K/C_K` indexées,
jamais un scan du nuage par centre ou par support.

## 5. Renforcement des blocs `ALL` par slabs de bissecteurs

Définir le score affine :

$$\phi(c,z)=2c\mathbin{\cdot}z-\left\lVert z\right\rVert^2.$$

Tout centre passant par `a,b` vérifie
`r_ab(c)=phi(c,b)-phi(c,a)=0`. Choisir des représentants `a0,b0` et poser
`r0(c)=phi(c,b0)-phi(c,a0)`. Pour le vrai triplet `(c,a,b)`, on a
`r0(c)=Delta(c,a,b)`, où la différence se borne exactement et séparément par
axe.

Pour un intervalle de centre `K_i`, définir, à `c` fixé :

```text
Amax(c) = (a0-c)^2 - dist(c,A)^2
Amin(c) = (a0-c)^2 - far(c,A)^2
Bmax(c) = far(c,B)^2 - (b0-c)^2
Bmin(c) = dist(c,B)^2 - (b0-c)^2
```

Alors :

$$\delta_{\max}=\sum_i\max_{c\in\lbrace K_i^-,K_i^+\rbrace}(A_{\max}(c)+B_{\max}(c)),$$

$$\delta_{\min}=\sum_i\min_{c\in\lbrace K_i^-,K_i^+\rbrace}(A_{\min}(c)+B_{\min}(c)).$$

Les extrema aux endpoints sont exacts par convexité pour le maximum et
concavité pour le minimum. Tout vrai centre du bloc satisfait donc le slab :

$$\delta_{\min}\leq r_0(c)\leq\delta_{\max}.$$

Construire de même le slab du bissecteur `a,x`, avec un bloc de carriers `C`.
L'intersection des deux slabs avec le domaine de Jung contracte la boîte de
centres en un tube autour de la droite q3 correspondante. Les extrema des
scores `phi(c,z)` sont ensuite pris sur les sommets du polytope rationnel ; un
outer polytope du disque reste fail-open.

Ce mécanisme est la couture exacte manquante entre `ALL` et la cutting. Il peut
réduire fortement les patches inter-amas sans émettre `A*B`, mais aucune borne
globale n'en découle encore. Les compteurs requis sont masse de paires par
slab, volume ou largeur avant/après, patches morts, conflits restants et
microtuiles terminales.

## 6. Portes et ordre de reprise

Les fixtures prioritaires sont :

- seuil q4 faux à `31,134°` : `(58,35,0)` et `(29,0,0)` ;
- frontière du spindle : `a=0`, `b=(6,0,0)`, `w=(2,1,1)` ;
- anneau 432 positif : `a=0`, `b=(9,0,0)`, `w=(3,1,1)` ;
- égalités `r=D/3` et `r=D/2`, frontières de sous-cône et permutations ;
- ligne verticale avant cisaille, choix où chaque `t` précédent est interdit,
  ordre rationnel exigeant plus de 128 bits ;
- événements `P-P`, `N-N`, `P-N`, niveaux `0` et `k`, centre sur coupure ;
- concurrence de trois bundles, shell lourd et deux `SupportKey` pour une
  `GeometricBallKey` ;
- bloc de carrier commun et slabs qui restent larges ;
- `theta_only_prunes_on_live_patch=0` sur toutes les fixtures ;
- `eight_clusters` avec ledger complet de masse, pas seulement un temps.

Mutants : arrondi décimal des angles, cône fermé axial, oubli d'un endpoint,
`i128` sans détection, omission `P-N`, seulement `k` niveaux, prolongement des
segments actifs en droites, quotient silencieux d'un plateau, `ALL` comme
émission, `UNKNOWN` comme `NONE`, borne haute possédée par les deux enfants et
cap de profondeur traité comme résultat.

Ordre demandé à Claude :

1. ajouter le compteur qui démontre la redondance de `theta`, puis supprimer
   toute attribution de gain à ce filtre ;
2. recevoir l'oracle indépendant `(S,I_B,U_B)` ;
3. implémenter la banque 432 comme prune auxiliaire et le classifieur
   `NONE/ALL/UNKNOWN`, avec ledgers séparés ;
4. ajouter les slabs de bissecteurs avant toute expansion `ALL` ;
5. prototyper la cutting signée sur une ancre, contre l'exhaustif rationnel ;
6. choisir niveaux streamés ou cutting certifiée selon le vrai high-water ;
7. mesurer `uniform` et `eight_clusters` avant CUDA massif ;
8. qualifier G4 seulement après fermeture du scratch et des sorties ;
9. raccorder plateaux, resolver, fold, dix forêts, verticales, lots et
   certificat minimal au même chrono.

La cible d'une seconde reste `BenchmarkOutputContract-v1` complet. Le seul
`hgp_reduced_normalized_h0_v3` est une sous-porte diagnostique et ne porte pas
le SLO officiel.

## 7. Rejeu CPU au pin `9bcd137`

Après reconfiguration Release, `ctest -N` inventorie `550` portes, dont `33`
préfixées `mhgp3v_anchor_`. L'ELF
`build/v3/mhgp3v_anchor_source` a le SHA-256
`59425e5708251fe890b57ea271887735fe8e9ab3a30f6cb0f9951e12c514e7f3`.
Le rejeu complet rend `32/33` en `503,57 s` : la porte
`mhgp3v_anchor_mutant_census` est tuée par signal après environ 62 s, et
`expect_failure.cmake` refuse correctement de traiter un crash comme rejet
contractuel. Sa relance isolée passe `1/1` en `39,15 s`. L'échec complet est
donc non reproductible isolément et doit être instrumenté en RSS/durée ; il ne
permet ni un verdict sémantique rouge, ni l'écriture rétrospective `33/33`.

Les 32 autres portes passent, notamment les deux moteurs à `smax=24`, le cas
`smax=20`, la borne basse exercée `smax=5` et le mutant qui réintroduit les
seuils figés. Cela ne ferme pas les endpoints annoncés `smax=4/34`, le refus
`35`, l'oracle indépendant, `eight_clusters`, CUDA/G4 ni le payload officiel.

GCP non utilisé.
