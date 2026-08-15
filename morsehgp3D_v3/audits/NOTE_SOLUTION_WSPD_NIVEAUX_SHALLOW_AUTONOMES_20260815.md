# Solution constructive — WSPD exact et niveaux shallow autonomes

Date : 15 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Cette note propose une route d'implémentation ; elle ne reçoit encore aucun
logiciel ni SLO. Elle n'utilise aucune structure de Delaunay, d'aucun ordre.
`Lane2`, `Lane3` et `Lane4` sont trois producteurs autonomes : ils ne partagent
ni record, ni verdict, ni cap, ni continuation, ni preuve de complétude.

## 1. Le pivot positif

La rampe J0 a localisé le mauvais produit. Sur
`uniform,50000,smax=11`, elle paie `6 091 112 797` couples de lentille après
`171 956 174` préfixes aigus, pour `9 768 840` q4 candidates. Sur
`eight_clusters,12500`, le même produit atteint `24 135 659 695` avant de
refuser sa coupure. Agrandir `dmax` ne peut donc pas être la réparation.

La route proposée conserve l'intuition de l'utilisateur — énumérer par WSPD
les supports dont **leur propre miniboule** reste shallow — mais remplace les
fenêtres empiriques par deux objets exacts :

1. une `NeutralPairPartition` qui conserve toute la masse des paires et dont
   chaque bloc est certifié mort, scindé ou continué ;
2. pour chaque arête-owner survivante, le niveau peu profond de l'arrangement
   de ses formes de puissance, sans former les couples de complétions.

Les seuils sont indépendants et sans ambiguïté :

```text
Lane2 : accepte I_B<=9 ; meurt dès I_B>=10
Lane3 : accepte I_B<=8 ; meurt dès I_B>=9
Lane4 : accepte I_B<=7 ; meurt dès I_B>=8
```

Ainsi `smax-2=9` est bien la capacité de mort de q3, jamais une profondeur q3
acceptée. Ce nombre n'autorise aucune transition q3 vers q4.

## 2. Certificat d'ancre qui répare la coupure

Soit `e=(a,b)` une arête de longueur `d`, maximale dans un support positif
d'arité `q`. Soit `B(c,R)` sa miniboule, `v=(b-a)/d` et `u=(c-a)/R`. L'égalité
de shell donne `v.u=d/(2R)`. La borne de Jung dans l'enveloppe affine du
support donne :

```text
q2 : v.u >= 1
q3 : v.u >= sqrt(3)/2
q4 : v.u >= sqrt(2/3)
```

Ces constantes proviennent respectivement de
`R<=d/2`, `R<=d/sqrt(3)` et `R<=sqrt(3/8)*d`. Pour un site `z`, poser
`t=z-a`. Sa puissance dans la boule réelle vaut
`||t||^2-2R(t.u)`. L'égalité de shell de l'arête donne plus précisément
`2R=d/(v.u)`. La condition stricte exacte du cœur universel est donc

```text
||t||^2 < d * min_{w : v.w>=kappa_q} (t.w)/(v.w)
```

implique que `z` est intérieur à **toute** miniboule positive d'arité `q`
ayant `e` pour arête maximale. Le minimum porte sur un cap sphérique de rayon
zéro pour q2, trente degrés pour q3 et environ `35,264` degrés pour q4. Cette
condition redonne exactement, après élimination du quotient, les fuseaux
polynomiaux `W2/W3/W4` déjà prouvés. En écrivant `t=alpha*v+t_perp`, elle vaut
respectivement `||t||^2<d*alpha`,
`||t||^2<d*(alpha-||t_perp||/sqrt(3))` et
`||t||^2<d*(alpha-||t_perp||/sqrt(2))`.

Conséquence constructive : dix, neuf ou huit `PointId` distincts satisfaisant
respectivement cette condition ferment l'arête avant toute complétion. Un
point du support réel ne peut pas être crédité par erreur : il est shell pour
la direction réelle, tandis que l'inégalité de crédit est stricte.

### Version bloc WSPD

Pour une ancre singleton `a` et un nœud cible `B`, calculer :

- une minoration exacte `d_min` de `||b-a||` pour tout `b` de `B` ;
- un cône sphérique `Omega_q(a,B)` contenant les caps de directions possibles
  de tous ces `b` ;
- pour chaque nœud témoin, une majoration de `||z-a||^2` et une minoration du
  quotient positif `(z-a).u/(v.u)` sur les directions et arêtes du bloc.

Si un témoin vérifie strictement l'inégalité avec `d_min`, il crédite tout le
bloc. Une minoration plus simple de `(z-a).u`, sans diviser par `v.u<=1`, reste
sûre mais plus conservatrice lorsque son signe est positif. Dès
`h_q=10/9/8` vrais IDs crédités par une antichaîne de nœuds témoins, le bloc
entier est `DEAD_DEPTH`. En cas d'intervalle indécis, on scinde ; les coins
seuls ne décident jamais `ALL`.

La première version peut scinder le côté ancre jusqu'au singleton et garder le
côté cible factorisé. Une extension où les deux côtés varient exige une vraie
enclosure corrélée ou Bernstein ; elle n'est pas une précondition du raccord.

Ce certificat est plus directement exploitable que la première note des
directions admissibles : l'arête candidate fournit elle-même le partenaire et
son cône de centres. Il ne suppose aucune monotonie fausse entre deux
diamètres et ne limite jamais les partenaires à `B(a,r)`.

### Accélérateur endpoint optionnel, désormais implémentable

Le théorème global réparé reste utile pour fermer d'un coup les longues paires.
Une cellule sphérique rationnelle `C` est potentielle à l'échelle `r` dès
qu'elle intersecte, pour au moins un `s=z-x`, la calotte
`2(s.u)>max(r,||s||)`. Cette intersection se décide fail-open par un BVH
global : pour un nœud de boîte `S`, l'identité

```text
max_{s dans S,u dans C} s.u = max_{coin c de S} max_{u dans C} c.u
```

réduit le majorant aux huit coins. Le maximum sur `C` teste la direction de
`s`, ses projections sur les trois arcs, puis les trois sommets. Si deux fois
ce maximum est au plus `max(r,dist_min(S,x))`, le nœud n'est pas potentiel ;
sinon il descend.

Pour compter les témoins couvrant **toute** la cellule, prendre des sommets
rationnels `u_j=g_j/L_j`. Le site couvre la cellule si, pour chacun d'eux,

```text
r * (g_j.s) - L_j * ||s||^2 > 0.
```

Chaque membre est une somme de trois quadratiques concaves séparables sur une
AABB. Son maximum continu donne un `NONE` unilatéral, son minimum est atteint
aux extrémités de chaque axe et donne `ALL`; l'égalité descend. À `h_q` IDs
distincts, la cellule est fermée. Une calotte d'intériorité non vide a un rayon
angulaire strictement inférieur à quatre-vingt-dix degrés, donc couvrir les
sommets d'une cellule géodésiquement convexe couvre bien toute la cellule.

Quand `r` augmente, la région potentielle rétrécit et les calottes témoins
grandissent. Un certificat endpoint vert à `r` reste donc vert aux échelles
supérieures. Il fournit un rayon `r_q(x)` propre à chaque lane ; un bloc WSPD
de distance minimale supérieure au maximum des rayons certifiés de tout un de
ses côtés peut être fermé sans ouvrir ses paires. Une cellule non résolue ou
un endpoint sans rayon garde simplement la continuation.

## 3. Partition neutre et preuve de complétude

La WSPD ne doit pas être un filtre qui oublie silencieusement ses blocs. Elle
porte une partition exacte des paires non ordonnées de `PointId` :

```text
mass(parent) = mass(DEAD_DEPTH) + sum mass(children) + mass(PENDING)
```

Chaque paire propre apparaît dans un seul bloc neutre. Les paires d'IDs à la
même position sont conservées dans les buckets ; seule une paire endpoint de
distance nulle est impropre comme ancre géométrique. Un split déterministe
préserve l'union disjointe et l'owner final emploie le vrai `EdgeKey`, jamais le
rang Morton.

Les trois lanes lisent séparément l'identifiant du bloc neutre, puis créent
leurs propres tâches :

```text
NeutralPairPartition
  |- PairBlock2 : W2Depth10 -> Lane2
  |- PairBlock3 : W3Depth9  -> Lane3
  `- PairBlock4 : W4Depth8  -> Lane4
```

Un crédit géométrique pur peut réutiliser une fonction arithmétique commune ;
son compteur, son seuil, son fate et son tape appartiennent à une seule lane.
Le vecteur partagé `acu` du probe J0 n'est donc pas conservé.

## 4. Le plan médiateur unifie les calculs, pas les lanes

Fixer une arête `e=(a,b)`, son milieu `m`, sa longueur `d`, et le plan
médiateur `V=(b-a)^perp`. Toute sphère passant par `a,b` a un centre
`c=m+w`, `w` dans `V`, et un rayon vérifiant
`R^2=d^2/4+||w||^2`.

Pour chaque site `z`, sa puissance est la forme **affine** suivante sur `V` :

```text
pi_z(w) = ||z-m||^2 - d^2/4 - 2 * proj_V(z-m).w
```

Le site est intérieur, shell ou extérieur selon `pi_z(w)<0`, `=0` ou `>0`.
Chaque troisième site définit donc une ligne `L_z : pi_z(w)=0`. Ce fait donne
deux consommateurs indépendants :

- Lane3 évalue un point particulier de `L_x`, le centre ambiant du triangle
  `abx` ;
- Lane4 évalue les intersections `L_x inter L_y`, centres des sphères
  `abxy`, mais seulement au niveau de profondeur inférieur à huit.

Ce partage est une identité mathématique et éventuellement une fonction pure.
Lane4 ne lit jamais une face, une sortie ou un verdict de Lane3.

## 5. Lane2 — paire directe

1. Consommer `PairBlock2` et fermer par `W2Depth10` lorsque possible.
2. Scinder le résiduel jusqu'à une paire exacte, sans cutoff silencieux.
3. Construire sa `BallForm` diamétrale.
4. Faire un range-count Morton exact, avec arrêt au dixième intérieur ; en cas
   de survie, range-report complet du shell et des IDs intérieurs.
5. Émettre `BallKey/SupportKey` seulement si le rang fermé et la disposition
   contractuelle passent.

Cette lane ne construit aucun triangle.

## 6. Lane3 — triples autonomes, sans Lane2

1. Consommer son propre `PairBlock3` et tenter `W3Depth9`.
2. Pour une arête résiduelle `e`, interroger l'`AcuteLens3(e)` dans l'index
   Morton : les deux autres côtés ne dépassent pas `d`, l'angle opposé à `e`
   est strictement aigu et le tie des longueurs est tranché par l'`EdgeKey`
   complet.
3. Chaque `Third3` donne directement le centre ambiant `w_x` et la `BallForm`
   q3 ; aucun couple de troisièmes points n'existe.
4. Un range-count exact meurt au neuvième intérieur. Les survivants transportent
   `I_B/U_B`, shell compris.

Les nœuds `ALL_ACUTE` peuvent rester des `Third3Block` de scheduling, mais un
verdict uniforme sur leurs centres variables exige une enclosure reçue ;
sinon ils sont scindés fail-open. Aucune sortie q2 n'intervient.

## 7. Lane4 — niveaux shallow, sans produit de lentille

### 7.1 Source interne complète des seeds

Lane4 consomme son propre `PairBlock4`, tente `W4Depth8`, puis construit ses
propres `Q4Seed3` aigus. Cette restriction ne dépend pas de Lane3 et elle est
complète.

Preuve. Placer le milieu de l'arête maximale `ab` à l'origine, écrire
`a=-s/2`, `b=s/2` et le circumcentre `c=w` avec `w.s=0`. Pour les deux autres
sommets `x,y` sur la sphère, la face `abx` est aiguë exactement lorsque
`||x||^2-d^2/4=2(w.x)>0`, et de même pour `y`. Comme le circumcentre est dans
l'intérieur strict du tétraèdre, ses poids barycentriques sont positifs et
`w` n'est pas le milieu de l'arête. En prenant le produit scalaire de la
relation barycentrique avec `w`, on obtient
`||w||^2=lambda_x(w.x)+lambda_y(w.y)>0`. Au moins une des deux faces contenant
`ab` est donc strictement aiguë.

Si les deux le sont, le plus petit vrai `PointId` de `x,y` choisit le seed
primaire ; si une seule l'est, elle est primaire. Avec l'`EdgeKey` maximal
canonique parmi les six arêtes, tout q4 positif possède ainsi exactement un
producteur.

### 7.2 Sélection du niveau inférieur à huit

Sur une ligne `L_x`, toute autre forme se restreint à une fonction affine
`A_z-tau*B_z` : son intérieur est une demi-droite, avec shell au root exact.
Si `p` sites sont intérieurs sur tout le segment positif de la face, une
intersection de profondeur au plus sept ne peut utiliser qu'un des
`8-p` premiers groupes entrants ou des `8-p` derniers groupes sortants.

Le noyau `Q4SeedAxisTopR4` devient donc le générateur, pas un filtre placé après
un produit :

```text
Q4Seed3
  -> permanents/shell persistants
  -> top-(r4-p) entrants et sortants par BVH
  -> DEAD_PERMANENT ou DEAD_GAP ou groupes roots shallow
  -> distinct-ID4 + owner6 + positivité + primary
  -> replay exact I_B/U_B
```

Les fates de sélection ne sont pas interchangeables. `DEAD_*` retire une masse
prouvée ; `OPEN` produit des roots ; `OVERFLOW` conserve toute la masse dans
une continuation dynamique, un fallback exact ou un refus. L'agréger à
`DEAD_*` rendrait la source incomplète. De même, l'API fixe doit refuser avant
écriture tout `r4` hors de `[1,64]` et préflighter l'injectivité des IDs
témoins, leur disjonction du seed et les trois IDs distincts du seed.

Il n'existe aucun record `CellPair`, `Sym2` ou `carrier x apex`. Pour `m_e`
lignes d'une arête, le nombre de groupes d'incidences proposés est au plus
`2*r4*m_e`; chaque centre géométrique compte au moins deux incidences, donc le
nombre de centres shallow est au plus `r4*m_e`. Sous `RelevantGP`, cette borne
se transfère aux `SupportKey`; un groupe concurrent de grande multiplicité est
sinon un plateau à reporter ou refuser, jamais un tableau tronqué.

### 7.3 Best-first exact sur Morton

Pour un seed singleton, `A_z` est convexe séparable en `z` et `B_z` linéaire.
Sur une AABB témoin, le minimum de `A` vient du clamp exact, son maximum des
huit coins, et les bornes de `B` des coins. Un nœud où `B` change de signe ou
où un quotient touche le cutoff est scindé. Les autres donnent une borne
rationnelle du root et alimentent une sélection best-first de taille au plus
huit.

Plus concrètement, pour un cutoff rationnel `theta=p/q`, `q>0`, poser
`Q_theta(z)=q*A_z-p*B_z`. En coordonnées relatives au seed,
`Q_theta` est une somme de trois quadratiques
`q*G*s_i^2-H_i*s_i`. Son minimum continu sur une boîte est obtenu en clampant
`H_i/(2qG)` sur chaque intervalle ; après multiplication par `4qG`, le signe
est entier. Pour un entrant `B>0`, `Q_theta<0` signifie `rho<theta`; pour un
sortant `B<0`, la même inégalité signifie `rho>theta`. Le même moteur traite
donc `First` et `Last`, avec seulement le demi-espace de signe de `B` changé.
Une première passe maintient les `k<=r4` meilleurs sites, une seconde
range-reporte le groupe entier égal au root d'ordre `k`. Le flottant peut
ordonner la queue, jamais décider un prune.

La version u16 peut fermer sa largeur exactement. Normaliser le cutoff par
`p=A_y*sgn(B_y)`, `q=abs(B_y)`, puis poser `C=q*G` et
`H_i=q*W_i+p*n_i`. Après multiplication par `4*C`, la contribution minimale
d'un axe vaut `4*C*(C*e^2-H_i*e)` quand le clamp tombe sur un bout `e`, et
`-H_i^2` quand il tombe à l'intérieur. Les bornes u16 portent le plus grand
intermédiaire à environ 278 bits : réserver un entier signé de 320 bits, soit
`BigInt<5>` avec le type actuel. Une largeur 256 bits n'est pas un contrat sûr
pour ce prune.

La même forme donne le census sans rescan plat. Seul un minimum strictement
positif taille le nœud ; l'égalité descend pour préserver le shell. Comme
`Q_theta` est convexe, son maximum est à l'un des huit coins et un maximum
strictement négatif crédite toute la population. Le parcours s'arrête au
huitième intérieur ; en dessous, il range-reporte les vrais IDs et descend sur
zéro pour le shell complet. Cette étape est un bon jalon séparé entre le top-k
`O(m*r4)` déjà esquissé et le futur best-first complet.

Sur GPU, les tâches sont `(SeedId,WitnessNode,side,cutoff)` en SoA, avec petits
heaps par warp, queues persistantes et `count--scan--fill`. Un `SeedBlock` ne
sert qu'au scheduling tant qu'aucune extension d'intervalles ou Bernstein ne
prouve universellement ses ordres ; le mutant `corners_order_implies_all`
reste interdit.

Une optimisation ultérieure pourrait orienter l'arête et le seed pour ne
chercher qu'un seul signe de roots. Elle doit rester un jalon distinct : le
seed d'orientation positive peut être la face non aiguë d'un q4 positif. La
route reçue la plus simple conserve donc d'abord le primary aigu et les deux
côtés `First/Last`, puis compare toute variante orientée aux mêmes ensembles
de `SupportKey`.

### 7.4 Jalon intermédiaire sans nouveau BVH

Le raccord ponctuel actuel scanne les témoins une fois pour sélectionner les
roots, puis rescane `inner` pour chaque apex. Le premier patch utile est plus
petit que la wavefront : après `owner6`, positivité et primary, appeler le
`census_replay` de la sélection. Pour un apex shallow il reconstruit exactement
`I_B` avec les permanents et les extrêmes stricts, et `U_B` avec le seed, le
shell persistant et le groupe égal. `EXACT` seul émet ; `HORS_DOMAINE`,
`UNSUPPORTED_DEGENERACY` et `PENDING_CAP` ont des routes distinctes.

Cette étape supprime le second scan sans toucher au générateur et fournit
immédiatement les identités nécessaires à une gate multiensemble. Elle ne
supprime pas le premier terme `sum_seed |B(m,D) inter P|` ; c'est précisément
la responsabilité du jalon BVH `Q_theta` suivant.

Avant ce BVH, le lot peut aussi cesser de recopier le même voisinage. Pour une
arête Lane4 `e=(a,b)`, stocker une fois
`S_ab={z!=a,b:||2z-a-b||^2<=4D^2}` et une liste séparée de `Third4`. Le seed
référence `(edge_id,Third4_id)` et masque ce dernier pendant le parcours. Cette
factorisation est strictement interne à Lane4 ; aucune sortie q3 n'y participe.
Elle réduit stockage et transfert selon le ratio pondéré
`sum_e c_e*(m_e-1) / sum_e m_e`, sans prétendre réduire les visites
arithmétiques. Le simple nombre moyen de seeds par arête n'est qu'un indicateur
du potentiel, pas ce ratio d'octets.

La baseline plate peut alors fusionner les cinq scans en deux. Le premier
classifie chaque site une fois, compte les permanents et maintient les `r4`
meilleurs roots des deux signes. Après `k=r4-p`, le second range-reporte les
deux cutoffs avec leurs groupes égaux complets. `census_replay` reconstruit
ensuite le payload shallow sans scan supplémentaire. La gate développe le lot
factorisé vers l'ancien CSR et compare les IDs seed par seed avant toute mesure.

Pour le futur BVH, `max(Q_theta)<0` est un `ALL` direct du census. Dans la
sélection top-r, ce même verdict exige aussi un signe uniforme de `B` et une
classe entrant/sortant certifiée sur le nœud ; sans ces deux preuves, il faut
descendre. Un AABB BVH peut visiter tous les témoins au pire : aucun
`O(k log n)` ni facteur constant n'est promis avant les compteurs de visites.

## 8. Coût à mesurer honnêtement

Poser `B_q` le nombre de blocs de paires visités, `M_3` les `Third3` résiduels,
`M_4` les lignes q4 résiduelles et `T_q` les visites de nœuds témoins. La cible
physique devient :

```text
Lane2 : O(B2 + T2 + output2)
Lane3 : O(B3 + M3 + T3 + output3)
Lane4 : O(B4 + M4 + T4 + root_groups + output4)
         avec root_groups <= 2*r4*M4
```

Il n'y a plus de terme `sum_e binom(m_e,2)`. Le pire cas de `B_q`, `M_3` ou
`M_4` peut rester quadratique et les sorties q3 peuvent elles-mêmes être
quadratiques ; aucun SLO universel sur tout nuage n'en découle. Le preflight
doit rendre `resource_exhausted` avant fill lorsque tâches, sorties ou octets
sortent de l'enveloppe reçue.

Les `171 956 174` seeds du ledger J0 sont un diagnostic partagé et non le vrai
`M_4` autonome. Ils donnent néanmoins la prochaine expérience causale :
remplacer les `6,09` milliards de couples par le top-r axial, puis publier
`W4_dead`, `M4`, visites BVH, morts permanentes/gap, groupes roots, sorties,
octets et HWM. Aucun « facteur quarante restant » ne doit être déduit avant ce
rejeu et l'aval complet.

### 8.1 Tuilage exact du résiduel borné

Le calcul peut être sharded sans perdre le census. Fixer un cutoff reçu
`D<=dmax` sur l'arête maximale et attribuer chaque support à la boîte cœur qui
contient son sommet de coordonnées lexicographiquement minimal. Les boîtes cœur
sont demi-ouvertes et disjointes ; un support positif propre a des sommets de
positions distinctes, donc un propriétaire unique. En production, le vrai
`PointId` tranche également toute égalité de représentation.

Si `x` est un sommet du support et `y` appartient à sa miniboule fermée, alors
`|y-x|<=2R`. Jung donne les trois bornes :

```text
q2 : 2R <= D
q3 : 2R <= 2D/sqrt(3)
q4 : 2R <= sqrt(3/2)*D
```

Le halo L-inf entier

```text
H = ceil(3*dmax/2)
```

est donc conservatif pour les sommets, tous les intérieurs et tout le shell des
trois lanes. Il évite une racine carrée et se calcule par
`(3*dmax+1)/2` pour un `dmax` entier non négatif. Chaque tuile charge son cœur
plus ce halo, exécute les mêmes prédicats globaux, puis n'émet que les supports
dont le propriétaire est dans le cœur. Les IDs restent globaux ou passent par
une table locale-vers-globale monotone ; les compacter sans cette table
changerait l'ABI des sorties.

La gate exacte est : multiensemble global égal à la somme disjointe des
multiensembles de tuiles, avec égalité de `SupportKey`, owner, primary,
`BallKey`, `I_B/U_B` et multiplicité. Tester milieux demi-entiers, supports au
coin de huit tuiles, shell exactement sur le halo, positions dupliquées hors
support et permutations d'IDs. Une tuile ne peut pas être sautée parce que son
halo contient moins de cinq points : deux points peuvent porter une q2 vide,
trois une q3 vide et quatre une q4 vide. Le seul raccourci commun sûr est
`|halo|<2`; mieux, chaque lane applique son propre plancher `2/3/4`.

Ce lemme reçoit le **recollement conditionnel au cutoff** ; il ne transforme
jamais `dmax` en certificat de complétude et ne remplace pas
`NeutralPairPartition/PENDING` pour les ancres plus longues. Un batch tronqué
par cap n'a pas non plus un préfixe canonique si l'ordre de visite de la grille
diffère du scan : il peut prouver la parité host/device de son propre préfixe,
pas l'égalité à une source globale.

## 9. Ordre d'implémentation conseillé à Claude

1. Recevoir la conservation de masse de `NeutralPairPartition` sur petit `n`,
   avec positions dupliquées et permutations de `PointId`.
2. Créer trois tapes réellement disjoints et raccorder `W2/W3/W4Depth` avant
   toute complétion ; tuer le mutant qui réutilise `acu`.
3. Dans Lane4 seulement, remplacer la boucle de paires de lentille par
   `Q4SeedAxisTopR4`, router `OVERFLOW` sans perte, puis brancher son
   `census_replay`. Factoriser ensuite `S_ab` par arête et recevoir la sélection
   plate en deux passes. Comparer les vrais `I_B/U_B` au brute.
4. Remplacer ensuite le premier scan témoin par le best-first Morton
   `Q_theta`. C'est le changement qui attaque directement les milliards
   observés et le temps résiduel.
5. Recevoir séparément Lane3 par `Third3 -> centre ambiant -> Depth9`, sans
   jamais consulter Lane2.
6. Raccorder les trois sorties à `BallKey/RLE`, puis seulement porter les
   queues reçues sur G4. Le tuilage à halo ci-dessus peut alors borner la HWM
   de chaque batch sans modifier les ensembles.

La première gate n'a pas besoin de 50 000 points. Sur `n<=140`, comparer pour
chaque lane les **ensembles** de `SupportKey`, owners, `BallKey`, `I_B/U_B` et
multiplicités au brute force. Ensuite seulement mesurer
`1500/3000/6000`, puis `12500/25000/50000` si les deux pentes physiques et la
HWM restent vertes.

## 10. Fixtures qui protègent la construction

- q3 avec huit intérieurs : accepté ; avec neuf : mort ; le mutant `I<=9`
  doit mourir ;
- q4 shallow dont les seeds géométriques correspondants sont profonds pour
  Lane3 : Lane4 doit tout de même l'émettre ;
- q3 shallow dont les trois arêtes q2 sont mortes : Lane3 doit tout de même
  l'émettre ;
- q4 positif avec une seule face `ab*` aiguë, puis avec deux : une émission
  dans les deux cas ;
- égalité de longueur entre arêtes partageant le même premier ID : owner par
  `EdgeKey` complet ;
- roots égaux, shell persistant, positions dupliquées et IDs clairsemés ;
- chaque fate WSPD retire sa masse exacte ; retirer un enfant ou créditer deux
  fois parent et enfant doit être détecté ;
- bloc de directions indécis : split obligatoire, jamais décision par coins.

Ces fixtures sont des garde-fous de la solution. Elles ne remplacent pas la
mesure de ses tâches physiques.

GCP non utilisé pour cette note.
