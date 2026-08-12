# Déblocage q2 : état produit adaptatif et localité par inversion

Date : 12 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

`HEAD` de référence : `df9dc7768156cfb24cf8e011f55f215115b22ca1`.
Le prototype non suivi `prototype/certified_locality_probe.cpp` a été observé
le 12 août 2026 à 08:23:59 UTC, hors CMake, avec le SHA-256
`f13d5d5b8fd8252e981ad81f397f1938b4791b59f951e3c825cda881c2f5143e`.
Le présent audit ne modifie aucun code.

## Verdict

Le prédicat q2 n'est pas le verrou principal. Les deux couples de bornes déjà
présents sont exacts ou conservateurs; c'est l'ordre du travail qui est rouge.
La traversée duale raffine d'abord la frontière témoin pour une boîte cible
encore large, recopie cette frontière, puis recommence sur ses descendants.
Les rampes reçues, avec `0,516` à `1,023` milliard de visites témoins à 50 k et
six pentes successives rouges dans la matrice v2, refusent cet ordonnancement
avant G4. Elles ne réfutent ni les bornes, ni les successeurs ci-dessous.

Deux déblocages indépendants sont disponibles :

1. rendre la machine q2 réellement adaptative et persistante, sans rescan ni
   copie de frontière;
2. utiliser la localité par inversion pour borner, ancre par ancre, l'univers
   dans lequel les activations utiles peuvent exister.

La première voie est immédiatement spécifiable. La seconde possède un lemme
exact prometteur, mais le prototype apparu pendant l'audit n'est pas recevable
en l'état et ne doit pas encore orienter un port CUDA.

## 1. Machine q2 adaptative exacte

Pour une ancre `p`, une cible `q` et un témoin `w`, poser

$$A(p;q,w)=(q-p)\mathbin{\cdot}(w-p)-\left\Vert w-p\right\Vert^{2}.$$

Le point `w` est strictement intérieur à la boule diamétrale de `(p,q)` si et
seulement si `A>0`. Pour un bloc cible `Q` et un bloc témoin `W`, les bornes
`L(Q,W)` et `U(Q,W)` vérifient

$$L(Q,W)\leq A(p;q,w)\leq U(Q,W)\qquad\text{pour tout }q\in Q,\ w\in W.$$

Ainsi `L>0` accepte tout `W`, `U<=0` le rejette de la recherche d'intérieurs
stricts, et le reste est ambigu. Pour tous sous-blocs `Q'` et `W'`,

$$L(Q',W')\geq L(Q,W),\qquad U(Q',W')\leq U(Q,W).$$

Les décisions sont donc monotones sous **tout** raffinement. Il est exact de
scinder `Q` ou `W` dans n'importe quel ordre; le choix de split ne porte aucune
autorité scientifique.

### État minimal

Un état est `(Q,A,F)` :

- `A` est une antichaîne de plages témoins universellement acceptées,
  deux à deux disjointes, disjointes de `Q` et de l'ancre, avec masse `m`;
- `F` est une antichaîne lossless de plages encore ambiguës;
- les plages rejetées par `U<=0` sont omises;
- `m>=10` clôt immédiatement tout `Q` par une tombstone exacte;
- `m+mass(F)<10` autorise seulement un abandon de ce prune, jamais une
  décision sur la paire.

Une feuille partiellement acceptée ne peut pas disparaître. Avec au plus huit
points, elle porte trois masques disjoints `accepted`, `rejected` et
`ambiguous`, ou descend logiquement en singletons. C'est la correction du
contre-exemple `p=0`, `Q=[10,20]`, `W={5,15}` : `5` est universel sur le
parent, tandis que `15` devient témoin sur l'enfant `{20}`.

### Ordre de travail proposé

À chaque état ambigu, un look-ahead d'un niveau évalue les splits possibles de
`Q` et de `W`. La politique choisit celui qui maximise la masse de produit
résolue par `L/U`, avec un départage canonique. Cette heuristique est libre :
seuls le ledger de partition et les décisions `L/U` sont certifiants.

`F` doit être immuable et partagée entre enfants, ou gérée par
checkpoint/rollback entre siblings. Une représentation device plausible est
`state_id + frontier_span_id + accepted_mask`, avec arènes par époque et
`count--scan--fill`; aucune copie `assign`, aucun suffixe append-only mort et
aucun tableau global de paires ne sont admis.

Le maximum entier de `U` doit aussi remplacer le majorant continu arrondi. Par
axe, pour chaque extrémité entière `u` de la cible, le maximum de
`u*v-v^2` sur l'intervalle entier témoin se trouve aux deux bords ou aux deux
entiers bornés voisins de `u/2`. En particulier, son maximum non contraint vaut
`floor(u^2/4)`; le `ceil` courant peut manquer un rejet sans créer de faux
prune.

Si la route par ancre reste rouge, le même invariant se lève en triple-tree
symétrique `(P,Q,W)`. La partition canonique des paires vérifie

$$T(N)=T(N_L)\mathbin{\dot\cup}(N_L\times N_R)\mathbin{\dot\cup}T(N_R).$$

Les bornes exactes déjà présentes sur
`Phi(w,x,y)=(w-x) dot (w-y)` sont monotones sous les trois raffinements. Les
nœuds `W` chevauchant `P` ou `Q` restent ambigus : après split ils peuvent
devenir disjoints et admissibles. Ce triple-tree ne prouve aucune complexité;
il fournit une ordonnance falsifiable qui évite le redémarrage du témoin à la
racine pour chaque bloc.

## 2. Lemme exact de localité par inversion

Fixer une ancre `x`. Pour `z!=x`, poser

$$\zeta_x(z)=\frac{z-x}{\left\Vert z-x\right\Vert^{2}}.$$

La boule de diamètre `[x,x+Du]`, avec `D>0` et `||u||=1`, contient strictement
`z` si et seulement si

$$u\mathbin{\cdot}\zeta_x(z)>\frac{1}{D}.$$

Soit `C_z(r)` la calotte des directions `u` satisfaisant cette inégalité pour
`D=r`. Si toute direction appartient strictement à au moins dix calottes, alors
toute boule passant par `x` et ayant au plus neuf intérieurs possède un diamètre
strictement inférieur à `r`. En effet, les calottes croissent avec `D`; une
boule de diamètre `D>=r` aurait donc au moins dix intérieurs stricts.

Conséquence correcte : si cette couverture est certifiée au rayon `r`, tout
support et tout intérieur d'une activation utile contenant `x` est dans la
boule ouverte `B(x,r)`. Cela vaut simultanément pour q2, q3 et q4, car les
activations de rang fermé au plus onze ont respectivement au plus `9/8/7`
intérieurs.

Cette propriété est une **borne de localité**, pas encore une preuve de coût.
Elle ne peut pas certifier toute ancre : si `x` est sur le bord de l'enveloppe
convexe, une direction de support vérifie `u dot (z-x)<=0` pour tous les autres
points, donc aucune calotte stricte ne couvre cette direction. Un nuage fini
possède toujours de telles ancres; un nuage coplanaire possède même une normale
non couverte pour toutes ses ancres. La couverture sphérique globale est donc
un filtre pour certaines ancres intérieures, jamais une source générale sans
fallback. Elle doit aussi traiter les ex æquo, l'owner exact-once, les supports
q3/q4 et le census fermé, sans matrice globale de voisinage.

Une variante plus utile garde un seuil **par cellule angulaire** au lieu d'un
verdict tout-ou-rien par ancre. Pour une cellule géodésique `C` de sommets
entiers `g_l` et un témoin `s=z-x`, lorsque tous les produits scalaires sont
positifs, poser

$$\tau_C(s)=\max_l\frac{\left\Vert g_l\right\Vert^{2}\left\Vert s\right\Vert^{4}}{(g_l\mathbin{\cdot}s)^2}.$$

La convexité géodésique de la calotte stricte prouve que, pour toute cible de
direction dans `C` et de distance carrée `D^2`, l'inégalité
`D^2>tau_C(s)` certifie `z` intérieur. Le dixième plus petit seuil provenant
de dix identifiants distincts tombstone donc toutes les cibles de la cellule
au-delà de ce seuil. Une cible ne s'auto-crédite pas : pour `s=q-x`, son seuil
est au moins `D^2`. Dix slots suffisent. Une cellule sous-pleine reste
fail-open et peut retomber vers le dual-tree. Cette banque par cellule évite
l'impossibilité aux ancres de bord et généralise la coupe Yao; elle exige
encore une preuve complète du range-report LBVH et un différentiel exact.

## 3. Audit du prototype de localité apparu dans le worktree

Le fichier non suivi matérialise cette idée avec une subdivision de l'octaèdre
et une couverture conservatrice des cellules. Son mode directionnel marque
désormais une cellule ouverte lorsque la fenêtre kNN tronquée ne peut la
fermer et possède désormais un fallback univers complet avec compteur
`full_scans`; ces mécanismes sont utiles. La condition finie est inversée : le
fallback est pris lorsque `r^2<d_M^2`, mais doit l'être lorsque
`d_M^2<=r^2`, égalité comprise, sauf certification stricte par la distance
`M+1`. Le compteur d'incomplétude compare encore une cible lue à la dernière
distance de sa propre liste et ne peut s'allumer; un cas incomplet peut donc
rendre zéro. Il faut une borne du prochain voisin omis ou un ledger résiduel
par cellule. Le lemme de couverture
mérite des fixtures indépendantes, mais le programme n'est ni reçu ni
constructible par le build courant. Quatre blocages précèdent toute mesure :

1. **P0 — signe q4 inversé.** Pour une orientation positive, le déterminant
   InSphere écrit par le probe est négatif à l'intérieur. Le juge conserve au
   contraire les points dont le signe est celui de l'orientation. Exemple :
   la fixture u16 `a=(3,3,3)`, `b=(3,1,1)`, `c=(1,3,1)`, `d=(1,1,3)` a
   l'orientation `-16`; son centre `z=(2,2,2)` donne le déterminant `+48`,
   tandis que l'extérieur `(4,4,4)` donne `-144`. La lane q4 rejette donc le
   centre et accepte l'extérieur.
2. **P0 — mauvais niveau de census.** Le mode nommé `census q2 EXACT` refuse
   d'abord si une ancre globale n'est pas certifiée, condition impossible pour
   toute ancre extrême d'un nuage fini. Lorsqu'il s'exécute, il s'arrête après
   dix intérieurs et publie seulement activations, tombstones et comptes. Il ne
   matérialise ni liste fermée, coquille, rang, `BallKey` ni déduplication des
   supports cosphériques; ce n'est ni le census ni la taille du payload.
3. **P1 — coût annoncé trop faible.** Pour chaque paire locale, le code rescane
   jusqu'à toute sa fenêtre locale. Le nombre de tests ponctuels est donc en
   général quadratique dans la taille locale; la phrase `O(sum M*) états` ne
   décrit pas le coût du census exécuté. La recherche kNN est en outre répétée
   une seconde fois en mode census. En directionnel, `8m^2*M*n` atteint déjà
   26,214 milliards de tests cellule--point pour `n=50 000`, `M=4 096`, `m=4`,
   hors kNN et classification.
4. **P1 — causalité et facteur non prouvés.** Le facteur `384` commenté mélange
   le rapport volumique de la boule diamétrale à `B(x,D)` et une restriction
   directionnelle. Aucun reçu ne prouve que cette constante est la cause de la
   superlinéarité observée.

À corriger ensuite : casts CLI vers `int` avant validation, certificat de
couverture cellulaire indépendant, gestion explicite des égalités de distance,
preuve d'absence de trou de la grille octaédrique, et fermeture d'un ledger
`local_candidates + locality_pruned = C(n,2)` avant tout claim de complétude.
La source compile isolément, mais aucune cible ni aucun CTest CMake ne la
construit; ses codes de mutants et son injection `census-drop-ties` doivent
aussi être repris, cette dernière ne changeant aucune paire admise sous le
filtre strict actuel. Une lane annoncée incomplète doit rendre un code non nul.

## 4. Réponses aux six questions de Claude

1. **Boule de bord et seuil.** Toute boule euclidienne dont `x` est sur la
   sphère possède bien l'antipode unique `x+D*u`; l'identité par inversion et
   la monotonie des calottes sont exactes. Dix intérieurs constituent un seuil
   uniforme sûr pour q2/q3/q4, mais non optimal : les seuils H0 spécifiques
   restent `10/9/8`. Le lemme borne seulement les activations incidentes à une
   ancre effectivement certifiée.
2. **Triangles sphériques.** Une calotte ouverte de rayon strictement inférieur
   à 90 degrés est géodésiquement convexe. La projection radiale du
   triangle positif d'une face d'octaèdre est son enveloppe sphérique courte;
   trois sommets strictement contenus suffisent, y compris pour `m=1`. La
   comparaison quadratique complète exclut déjà `||z-x||>=r`; tester seulement
   le signe du produit scalaire ne suffirait pas.
3. **Égalité du seuil.** La couverture stricte exige `D>rho`. À `D=r_c`, les
   témoins au seuil peuvent être sur la sphère et ne comptent pas comme
   intérieurs. Conserver les cibles avec `D^2<=r_c^2` est donc le côté
   fail-open correct. Les dix seuils doivent provenir de dix `PointId`
   distincts et hors cible.
4. **Directions ouvertes.** Aucune borne radiale finie universelle ne suit de
   la seule existence d'un second point de support : un point arbitrairement
   lointain dans une direction sortante peut former un support q2 avec peu
   d'intérieurs. Il faut une requête exacte, une boîte cible possédée ou un
   ledger résiduel. Une requête de cône sur le LBVH n'a pas par elle-même un
   coût égal au nombre de réponses; ses visites doivent être comptées.
5. **Niveau inversé local.** Le niveau `<=9` de `M` points inversés n'a pas une
   taille `O(M)` démontrée. Un arrangement tridimensionnel peut avoir une
   complexité combinatoire et `M` n'est pas borné indépendamment de `n` par les
   mesures à 2 000 points. Cette piste est recevable comme oracle local ou
   fallback falsifiable; elle ne rouvre une route produit qu'avec borne de
   travail/mémoire, ledger de complétude et pentes d'échelle, sans atlas global
   caché.
6. **Facteur 384.** `8*48` est une intuition volumique, pas une analyse causale.
   Elle multiplie une borne angulaire pessimiste et un nombre de chambres sans
   modèle de distribution, et ne décrit pas le dual-tree qui utilise toutes les
   directions. Les reçus prouvent des retests superlinéaires, pas que le
   certificat Yao est irrécupérable. La cascade affine/duale et une ordonnance
   adaptative restent donc des expériences légitimes.

## 5. Gates avant G4

1. Sorts q2 et census fermés différentiels au moins jusqu'à `n=256`, plus
   fixtures du maximum entier impair/négatif et des deux clips.
2. Fixtures `partial-leaf`, masque régional et invariance de schedule
   `Q/W`, puis `P/Q/W` si le triple-tree est retenu.
3. Reçu authentifiant plages, masques, disjonction, bornes, masses, LBVH,
   epoch et digest du nuage.
4. Compteurs complets : états/splits `P/Q/W`, tests `L/U`, look-ahead, copies,
   scans, tests ponctuels, octets alloués et high-water.
5. Un seul ELF sur les quatre familles à `12 500/25 000/50 000`; les deux
   pentes de chaque compteur dominant doivent être au plus `1,35`.
6. Pour la localité, contre-fixtures q2/q3/q4 indépendantes, traitement exact
   des ancres non certifiées et mesure séparée de `sum M`, `sum M^2`, sortie et
   mémoire. Un gain moyen ne remplace pas la fermeture du ledger.
7. Ensuite seulement : port device, parité, Compute Sanitizer et session G4
   gardée. Le SLO demande toujours trente répétitions `warm_e2e` sur une
   famille volumique favorable sparse et le payload `BenchmarkOutputContract-v1`.

Ni la machine adaptative ni la localité ne matérialisent cellule, coface,
incidence, Gamma ou mosaïque de Delaunay d'ordre supérieur. Elles restent des
candidates jusqu'à leurs différentiels, leurs ledgers et leurs pentes.

GCP non utilisé.
