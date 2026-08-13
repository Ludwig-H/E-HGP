# Audit du worktree — crédits coniques cellulaires

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Pin, portée et verdict

Le parent observé est
`HEAD=d3329fea4b595b7bbd283e509b0fa1955fcc3b06`. Le nouveau fichier non suivi
`prototype/cell_credits.hpp` porte le SHA-256
`5309870d8c22ef245daf0660ae0520c5690fafa49ce5610a486235bfa48cd948`.
Il n'est inclus par aucune cible, aucun probe et aucune porte CMake : aucun test
existant ne compile ces octets.

Verdict : **le théorème, les trois rayons et l'événement d'activation sont une
base mathématique recevable ; le fichier ne contient encore ni générateur de
crédits, ni enveloppe, ni ledger, ni ordonnance factorisée.** C'est néanmoins
la piste la plus directe pour transformer le complément conique mesuré sur les
amas en suffixes de cibles, sans `C(m,3)`.

## Théorème admis

Après translation de l'ancre en zéro, soit `d=b-a` et `s_i=z_i-a`. Si
`d=sum_i lambda_i s_i`, avec `lambda_i>=0`, et si chaque membre vérifie
`d dot s_i>||s_i||^2`, alors :

$$\sum_i\lambda_i\left(2c\mathbin{\cdot}s_i-\left\lVert s_i\right\rVert^2\right)=\left\lVert d\right\rVert^2-\sum_i\lambda_i\left\lVert s_i\right\rVert^2>0.$$

Ici `c` est le centre de n'importe quelle sphère passant par `a,b`, donc
`2c dot d=||d||^2`. Au moins un membre du groupe est strictement intérieur à
cette sphère. Le groupe peut avoir une taille quelconque. Des groupes de
`PointId` deux à deux disjoints donnent des intérieurs distincts.

Pour `C=cone(r0,r1,r2)` et `tau(r_j)=T`, toute direction de hauteur `x` est
`d=(x/T)u`, avec `u` dans le triangle des trois rayons. Poser :

$$m_C(s)=\min_{0\leq j<3}r_j\mathbin{\cdot}s.$$

Le minimum de `d dot s` sur la section est exactement
`x m_C(s)/T`. Si `m_C(s)>0`, l'événement cellulaire entier est donc :

$$X_s=\left\lfloor\frac{T\left\lVert s\right\rVert^2}{m_C(s)}\right\rfloor+1.$$

Pour toute hauteur `x>=X_s`, H2 est stricte sur la cellule entière. Le `+1`
est indispensable ; l'égalité reste résiduelle. « Exact » signifie ici seuil
minimal pour le **continuum cellulaire worst-case**, pas classifieur complet de
chaque direction du réseau.

Les largeurs annoncées tiennent en `i64` sous le profil u16 : produits
rayon--différence sous `589 815`, numérateur d'activation sous
`38 653 526 025` et déterminant de trois différences sous
`6*65535^3<2^51`.

## Enveloppe projective à recevoir à ancre feuille

Avec `w=r0+r1+r2`, `m_C(s)>0` implique `w dot s>0`. Couper chaque direction par
`w dot u=1` donne des points projectifs. Pour un pool `G` :

$$C\subseteq\mathrm{cone}(G)\iff\frac{r_j}{w\mathbin{\cdot}r_j}\in\mathrm{conv}\left\lbrace \frac{s}{w\mathbin{\cdot}s}:s\in G\right\rbrace\quad\text{pour }j=0,1,2.$$

Le signe d'une orientation projective est celui de
`det(s_i,s_j,s_k)`, car tous les dénominateurs sont positifs. Une enveloppe 2D
exacte puis un carrier de rang un, deux ou trois pour chacun des trois rayons
fournit un crédit d'au plus neuf IDs. Retirer ces IDs et recommencer est sûr et
incomplet ; l'échec reste `UNKNOWN`.

Les cas dégénérés font partie du contrat, pas d'une perturbation :

- un rayon sur une arête ou un sommet de l'enveloppe accepte des poids nuls ;
- des sites de même direction projective gardent une pile canonique de
  `PointId`, car ils peuvent alimenter plusieurs crédits disjoints ;
- les colinéaires du bord demandent un owner exact et un carrier de rang un ou
  deux ;
- H2 demeure strictement positive pour **chaque** ID, même lorsque
  l'appartenance conique est sur le bord ;
- après chaque retrait, l'enveloppe ou sa structure dynamique doit être mise à
  jour ; une enveloppe initiale ne certifie pas les crédits suivants.

Le header live ne code encore aucun de ces objets. `kOneRayOnly` et
`kShareIds` sont seulement des valeurs d'enum, pas des mutants exécutés.
`cell_rays` exige aussi un domaine `0<=cell<432` qui n'est pas vérifié par
l'API. Une porte différentielle doit confirmer que chaque direction non nulle
appartient au cône des trois rayons rendus par son `cell_of`, y compris les
axes, plans, diagonales et les 48 actions signées.

Un premier `cell_credits_probe.cpp`, SHA-256 observé
`ad3fe1b236dae713a2ac5583bc5c1ace123b3bcb2e443e737b3799209b3cff31`, est
apparu ensuite sans raccord CMake. Il reste `ancre times 432`, trie tous les
sites, énumère `C(m,3)` dans un pool borné, puis reboucle sur toutes les cibles.
Son défaut `pool=16` rend huit crédits q4 pleins structurellement impossibles :
chaque crédit qui contient un cône 3D exige au moins trois directions, donc il
faut au moins `24` IDs disjoints. `smax` est accepté mais les besoins restent
`10/9/8`. Le contrôle ponctuel dit lui-même qu'il ne juge pas le certificat et
les mutants retournent code nul sans différentiel interne. Ce probe peut
mesurer une primitive après réparation du pool et ajout d'un vrai juge, mais ne
reçoit ni l'enveloppe ni l'ordonnance 50 k.

### Successeur raccordé et rejoué

Le successeur rend le pool configurable et raccorde le probe, sans encore
changer le header. Pins observés :

| objet | SHA-256 |
| --- | --- |
| `prototype/cell_credits.hpp` | `5309870d8c22ef245daf0660ae0520c5690fafa49ce5610a486235bfa48cd948` |
| `prototype/cell_credits_probe.cpp` | `ad3fe1b236dae713a2ac5583bc5c1ace123b3bcb2e443e737b3799209b3cff31` |
| `CMakeLists.txt` | `836d5bbe969ef9e3ea24669184cdcb35384caa229ca03235ec0ac6e052d7ff2c` |
| ELF Release | `1fa1ba72...` |

Le Build ID de l'ELF est `6224ae10...`. En Release/CUDA OFF, les huit CTests
`mhgp3v_credits_` rendent **`7/8`** en environ `78,4 s`. La porte
`mhgp3v_credits_rangs` est rouge : `6 985` crédits émis sous le plancher
`10 000`, malgré `68 289` succès de carriers de rang deux. Ce compteur de rang
compte les recherches intermédiaires répétées, pas des crédits distincts ; il
ne peut donc pas recevoir une sortie scientifique.

Les ablations exposent surtout le coût du catalogue de carriers :

| pool | tests coniques | crédits émis | `credits_hwm` | fermetures q4 | temps |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 16 | 43 956 521 | 19 932 | 5 | 0 | 4,80 s |
| 32 | 350 267 629 | 29 705 | 10 | 0 | 39,75 s |

Le doublement du pool multiplie ici les tests par `7,97`, signature du mur
cubique `C(m,3)`. La porte `pool=32` exige explicitement
`min_residuel=3540=n(n-1)`, donc zéro fermeture q4 ; aucune porte ne planche
`credits_hwm>=8` ni une fermeture nominale. Le commentaire CMake « un crédit
consomme un à trois PointId » confond le carrier d'un rayon avec le crédit
couvrant trois rayons : un crédit full-dim consomme au moins trois et au plus
neuf IDs.

Aucun des huit CTests n'exécute un mutant ou `--judge-echantillon`; il y a un
selftest, trois mesures et quatre refus/planchers. Le selftest couvre les 432
milieux de cellule, mais sa vérification d'activation bornée porte uniquement
sur la cellule zéro. Le successeur est donc **rouge et non reçu**. Ces mesures
confirment que l'enveloppe projective/output-sensitive est un préalable, pas une
optimisation postérieure.

## Extension nouvelle : crédit commun à un bloc d'ancres

Rester à `AnchorId` feuille risque de recréer une tâche par ancre et, pour les
deux orientations, un join de relations presque pairwise. La géométrie permet
une recertification exacte d'un même groupe absolu `G` sur un bloc d'ancres.

Soit `K=conv(G)` et un rayon non nul `r`. Pour une ancre `a` :

$$r\in\mathrm{cone}(G-a)\iff\text{il existe }t>0\text{ tel que }a+t r\in K.$$

L'ensemble `K-{t r:t>0}` est convexe. Si les huit sommets d'une AABB d'ancres
y appartiennent pour chacun des trois rayons, toute l'AABB y appartient. La
stricte garde `t>0` est nécessaire : avec `t=0`, `G={a}` passerait sans fournir
aucune direction.

La construction complète n'exige même pas de hull 3D. Pour chaque rayon `r_j`,
choisir deux formes entières qui engendrent son plan orthogonal et projeter le
pool en 2D. Sous la garde stricte
`min_G r_j dot z>max_A r_j dot a`, on a :

$$r_j\in\mathrm{cone}(G-a)\iff\pi_j(a)\in\mathrm{conv}\left(\pi_j(G)\right).$$

En effet, l'égalité des projections donne `k-a=t r_j` pour un point
`k in conv(G)`, et la garde impose `t>0`. Pour chacun des `8*3` couples
`(coin a_v,rayon r_j)`, un point-in-hull 2D extrait un carrier canonique d'au
plus trois IDs. Leur union `G` emploie au plus `72` IDs et certifie la cellule
pour toute l'AABB, puisque la projection de la boîte est l'enveloppe de ses
huit coins. Retirer `G` et répéter au plus `h` fois est sûr et fail-open. Les
carriers peuvent différer entre coins ; la convexité est précisément ce qui
permet leur union.

H2 se recertifie sans décorréler les extrema. Pour chaque ID absolu `z`, chaque
coin `a_v` et chaque rayon, poser :

$$D_{v,j}(z)=r_j\mathbin{\cdot}(z-a_v).$$

Exiger tous les `D_{v,j}>0`, puis définir :

$$X_{A,z}=\max_{v,j}\left(\left\lfloor\frac{T\left\lVert z-a_v\right\rVert^2}{D_{v,j}(z)}\right\rfloor+1\right).$$

Cette borne est exacte sur toute la boîte : pour `x` fixé,
`x r_j dot(z-a)-T||z-a||^2` est concave en `a`, donc son minimum est à un coin,
et le minimum directionnel est à un rayon. Le seuil d'un crédit est le maximum
de ses événements.

Pour un nœud cible `B` dont toutes les différences `B-A` sont dans la cellule,
la hauteur minimale à comparer au seuil est
`min_B ell-max_A ell` — et non `min_B ell-min_A ell`. Après
`h=smax+1-q` crédits disjoints reçus, le record factorisé naturel devient
`(AnchorNodeKey,CellId,lane,X,TargetSuffixNodeKeys,CreditKeys)`.

Cette extension maintient les deux orientations au niveau des mêmes rectangles
et évite de joindre après coup des records `a times B` avec `A times b`. Les
boîtes trop larges échouent et se scindent ; aucune ancre représentative ne
sert d'autorité.

La construction à 24 intersections est complète mais peut employer jusqu'à
`72` IDs par crédit, donc `720` à `h=10`. Elle doit rester un tier de secours ou
un oracle de rappel, pas la première baseline G4. Un tier 1 beaucoup plus léger
laisse une ancre canonique **proposer** un carrier plein rang d'au plus trois
IDs pour chaque rayon, soit une union d'au plus neuf IDs, puis recertifie ces
mêmes carriers sur les huit coins. Pour un triple absolu `z1,z2,z3`, le
déterminant `det(z1-a,z2-a,z3-a)` et chacun des trois numérateurs de Cramer sont
affines en `a`. Un déterminant strict de signe constant et des numérateurs
faibles de ce signe aux huit coins certifient donc l'appartenance conique dans
toute l'AABB. Le représentant n'est jamais l'autorité ; l'échec scinde `A` ou
tombe au tier complet.

Pour un nœud cible déjà fixé, on peut fusionner l'activation et le test H2 sans
division. Poser `x0=min_B ell-max_A ell>0`. Pour chaque ID `z` et rayon `r_j`,
la forme suivante est concave en `a` :

$$F_{z,j}(a)=x_0 r_j\mathbin{\cdot}(z-a)-T\left\lVert z-a\right\rVert^2.$$

Si `F>0` aux huit coins, elle est positive dans toute la boîte. Le tier 1 coûte
au plus, pour dix crédits de neuf IDs, `960` déterminants coniques et `2 160`
marges de coins, bien moins qu'un hull 3D à 720 IDs. Mesurer rappel et splits
contre le tier complet avant tout choix device.

Il existe un classifieur H2 de rectangle encore plus direct, qui doit servir de
gate de référence. L'identité exacte est :

$$\left(b-a\right)\mathbin{\cdot}\left(z-a\right)-\left\lVert z-a\right\rVert^2=\left(z-a\right)\mathbin{\cdot}\left(b-z\right).$$

Pour des AABB `A,B` et un ID `z`, le minimum continu se sépare coordonnée par
coordonnée et chacun des quatre couples d'endpoints doit être essayé :

$$L_z(A,B)=\sum_{i=0}^{2}\min_{a_i\in\left\lbrace A_i^-,A_i^+\right\rbrace,\ b_i\in\left\lbrace B_i^-,B_i^+\right\rbrace}\left(z_i-a_i\right)\left(b_i-z_i\right).$$

Ainsi `L_z(A,B)>0` pour chaque membre du crédit est un H2-`ALL` exact sur le
rectangle, en douze produits par ID, sans hauteur, racine ni `PairId`.
L'égalité reste `MIXED`. Le seuil cellulaire `X_G` est le fast path comprimé ;
ce test bilinéaire traite son résiduel avant tout split et évite les ambiguïtés
d'extrema décorrélés.

## Ordonnance CPU puis G4 proposée

1. Partir des `RectId` disjoints du dual-tree et certifier la cellule de toutes
   les différences par extrema half-open.
2. Pour chaque `(AnchorNodeKey,CellId)` réellement actif, collecter sous cap un
   pool candidat et construire ses trois hulls 2D projetés et ses événements.
3. Extraire d'abord un crédit proposé de taille au plus neuf et le recertifier
   par formes affines/concaves aux huit coins ; employer le crédit complet de
   taille au plus 72 par les 24 intersections seulement en tier de secours ou
   oracle. Retirer les IDs et répéter au plus `h<=10` fois.
4. Fermer par seuil des nœuds cibles entiers, puis appliquer `L_z(A,B)>0` sur le
   résiduel du seuil. En cas de cap, dégénérescence non traitée ou crédit
   manquant, conserver le rectangle dans le front ; ne jamais repartir de la
   racine par ancre.
5. Employer les crédits du cœur et de dominance seulement avec un ledger
   commun de `PointId` disjoints. Une première composition sûre les consomme
   séquentiellement et exclut les IDs déjà crédités ; additionner des compteurs
   opaques est interdit.

Sur G4, un CTA peut traiter un pool borné en SoA : événements, tri par seuil,
orientations `i64`, petite enveloppe, Cramer de coins, puis count--scan--fill des
records de suffixe. Comparer au moins deux variantes : enveloppe reconstruite
après chaque crédit et couches convexes qui prennent tout le bord comme crédit
plus gros. La seconde perd du rappel mais simplifie la baseline exacte.

## Gates et compteurs

- oracle petit pool : toutes les fermetures cellulaires incluses dans le juge
  exhaustif de sphères, puis comparaison du nombre de crédits au packing exact
  seulement comme mesure de rappel ;
- fixtures singleton, paire antipodale, triangle, rayon sur bord, directions
  projectives dupliquées, H2 égale, `h-1/h`, ID réutilisé et groupe qui contient
  le barycentre mais omet un rayon extrême ;
- bloc d'ancres : huit coins dont un seul rate l'ombre `K-cone(r)`, `t=0`, rang
  deux positif, H2 qui échoue seulement à un coin/rayon, égalité `L_z=0`, boîte
  large à scinder ;
- mutants effectifs : oubli du `+1`, un seul rayon, partage d'ID, acceptation de
  `m<=0`, omission d'un coin d'ancre, `min_A` employé au lieu de `max_A` pour la
  hauteur cible ;
- compteurs : pools proposés/actifs/tronqués, événements, tris, hull rebuilds,
  orientations, tailles de bord, carriers rang 1/2/3, intersections de coins,
  crédits, conflits ID, seuils, nœuds cibles `ALL/MIXED`, front résiduel, bytes
  et HWM ;
- conservation par lane : masses fermées des étages séquentiels plus masse du
  front égale à la masse d'entrée, avec `RectId` disjoints et aucune expansion
  de `PairId` hors juge borné ;
- rampes mêmes octets à `12 500/25 000/50 000`, deux exposants au plus `1,35`
  pour chaque compteur physique dominant, puis seulement lowering CUDA.

Le GO de ce header n'est donc pas « il compile » : c'est la démonstration que
les crédits survivent à la recertification par blocs et que le nombre de tâches,
visites, records et octets baisse réellement sur `eight_clusters` sans reporter
le travail dans le front résiduel.

GCP non utilisé.
