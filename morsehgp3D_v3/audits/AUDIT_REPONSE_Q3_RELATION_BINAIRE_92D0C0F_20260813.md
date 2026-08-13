# Audit constructif — la relation arête–porteur est la bonne source q3

Date : 13 août 2026 UTC.

Cadre : phase=exploration_v3_hors_registre,
backend=cpu_reference_bounded_oracles_and_g4_diagnostic,
profile=quantized_u16_input_only,
mode=audit_independant_math_and_architecture,
public_status=not_claimed.

Snapshot observé : HEAD
92d0c0f0f6fcf3956608bbd406eefe770cb6f892, worktree propre à
2026-08-13T21:25:01Z. Empreintes SHA-256 :

- prototype/wspd_wavefront_probe.cpp :
  3ada9a78d544e58c1969cd41faa25020484937ca029f6855dff354ed69a0ef90 ;
- CMakeLists.txt :
  349ee8560932044c343e72517cec738bbe6f21ae9c800974264e341acabb00a8 ;
- NOTE_CLAUDE_Q3_EST_UNE_RELATION_BINAIRE_20260813.md :
  de91bf08775030bd3f2a3f9fbc747616c47e087e88df3d13023cc300492915bd.

GCP non utilisé.

## Verdict direct

Oui : il existe une bonne voie pour générer q3 sans énumérer tous les triplets
et sans construire une mosaïque de Delaunay d'ordre supérieur. L'idée décisive
de Claude est juste : après choix canonique de l'arête maximale, un support q3
est une incidence entre une arête et un porteur, et ce porteur détermine un
unique centre, le pied métrique d'une forme affine.

La bonne source candidate est donc :

    fenêtre q3 certifiée d'arêtes
      -> blocs arête–porteur canoniques
      -> une clé de sphère primitive par incidence
      -> RLE des sphères égales
      -> range-count LBVH saturé à neuf
      -> un census complet par sphère survivante
      -> fold streamé

Cette ordonnance est exacte, factorisable et adaptée à un GPU. Elle est
conditionnellement sparse : son coût dépend des arêtes q3 ouvertes, de la masse
des porteurs, des sphères distinctes et de la sortie. L'acuité seule ne borne
aucune de ces masses. La vague live Q3AcuteCarrierWave-v0 est donc un broad
phase utile, mais pas encore la source reçue.

## 1. Théorème de réduction q3

Soient trois points distincts non colinéaires a, b et x. Posons
$d=b-a$, $u=x-a$, $D=d\cdot d$, $E=u\cdot u$,
$F=d\cdot u$ et $X=D+E-2F=\left\Vert x-b\right\Vert^2$.
Supposons que ab soit une arête maximale faible, donc $D\geq E$ et
$D\geq X$.

Le triangle est aigu si et seulement si l'angle en x est aigu. En effet, les
deux autres inégalités d'acuité sont strictes dès que les trois points sont
distincts, tandis que celle en x vaut $E+X>D$.

Avec $V=2x-a-b=2u-d$, on obtient l'identité exacte :

$$V\cdot V-D=2(E+X-D)=4(E-F).$$

Par conséquent, sous l'hypothèse d'arête maximale :

$$\text{positivité géométrique du support de cardinal trois}\quad\Longleftrightarrow\quad V\cdot V>D.$$

L'égalité est un angle droit et reste conservée hors q3. Le test live strict
est donc correct. Aucun tirage aléatoire n'était nécessaire pour le recevoir :
l'identité ci-dessus est la preuve.

Il faut toutefois distinguer deux sens du mot binaire. Le support porte encore
trois PointId ; c'est la relation de calcul qui est binaire,
EdgeKey(ab) × PointId(x). Cette réduction ne permet pas de fusionner les lanes
q2, q3 et q4.

## 2. L'owner doit être global, surtout sur les égalités

Le choix d'une arête maximale faible ne rend pas encore l'émission unique. Un
triangle isocèle peut avoir deux arêtes maximales et un triangle équilatéral
en a trois. L'owner exact est :

1. maximiser la longueur carrée ;
2. en cas d'égalité, choisir la plus petite EdgeKey formée des deux PointId
   triés.

Le juge live remet le compteur de couverture à zéro pour chaque terminal WSPD.
Il reçoit donc exactement une couverture du porteur relativement à une
incidence d'arête, mais il ne regroupe jamais les trois occurrences d'un même
SupportKey à travers les terminaux. Son accord n'est pas encore un accord
exact-once des supports q3.

La fixture minimale est l'équilatéral entier de $\mathbb{Z}^3$ :

- a=(100,100,100) ;
- b=(101,101,100) ;
- x=(101,100,101).

Les trois longueurs carrées valent 2. L'oracle global doit trouver trois
incidences admissibles mais exactement un owner et une seule émission du
SupportKey. Un mutant qui accepte toute arête maximale faible doit rendre trois
occurrences.

## 3. Pourquoi les quatre verts live ne prouvent pas le packing

Commande rejouée :

    cmake -S morsehgp3D_v3 -B build/v3 -DCMAKE_BUILD_TYPE=Release
    cmake --build build/v3 --parallel --target mhgp3v_wspd_wavefront_probe
    ctest --test-dir build/v3 --output-on-failure -R '^mhgp3v_wspd_wavefront_q3_'

Résultat : 4/4 tests passent, 0,12 s. Ils reçoivent utilement les deux prunes,
la partition de l'arbre des porteurs pour une incidence d'arête et un plancher
de non-vacuité.

Ils ne reçoivent pas la phrase « O(s³) cellules par arête ». Toutes les
commandes nominales passent --tight, et cell_of utilise alors tlo/thi. Une
boîte serrée certifie les extrema géométriques, mais elle n'est pas une cellule
dyadique de volume comparable. Des sous-arbres disjoints en PointId peuvent
avoir des AABB serrées qui se chevauchent et des diamètres arbitrairement plus
petits que le seuil. Un nombre non borné de feuilles de diamètre zéro tient
dans la même région. Le premier nœud serré qui satisfait seulement une borne
supérieure de diamètre ne fournit donc aucun argument de packing.

Le nombre mesuré de 23,6 à 32,4 blocs par arête sur quatre tailles est un signal
encourageant, pas une preuve. La racine relancée pour chaque terminal produit
déjà 65 millions de visites à n=2 000 ; ce produit ne doit pas être transcrit
en CUDA.

## 4. Réparation : Q3CarrierPrefixRange-v0

Le packing devient prouvable si le niveau deux utilise des préfixes Morton
alignés à une profondeur fixée par l'échelle de l'arête, et non le diamètre de
la boîte serrée courante.

Pour un terminal d'arêtes A×B, soit Dlo la distance carrée minimale certifiée.
Si Dlo est nul, le terminal est raffiné ou délégué. Sinon choisir le niveau
dyadique h tel que la diagonale carrée w2(h) vérifie
$s_3^2 w2(h)\leq Dlo$, tandis que le niveau parent échoue. Toutes les cellules
de niveau h ont alors la même taille, sont disjointes et sont comparables à
$\sqrt{Dlo}/s_3$.

Un porteur possible doit vérifier simultanément
$\mathrm{dist}(x,A)^2\leq Dhi$ et
$\mathrm{dist}(x,B)^2\leq Dhi$, où Dhi majore les longueurs d'arête du
terminal. La porte de packing vérifie aussi explicitement
$Dhi/Dlo\leq\kappa(s_1)$, conséquence attendue de la séparation de niveau un ;
si ce rapport n'est pas certifié, le terminal est raffiné ou délégué. On
énumère seulement les préfixes de niveau h qui rencontrent
l'intersection de ces deux voisinages nécessaires. À séparation et dimension
fixes, cette région coupe O(s³) cellules alignées. Pour chaque préfixe :

- deux recherches dans le tableau Morton trié donnent son intervalle non vide ;
- l'AABB serrée de cet intervalle sert seulement aux prunes non-maximale et
  obtuse ;
- la cellule alignée, jamais la boîte serrée, porte l'arrêt et la preuve de
  packing ;
- les intervalles émis sont disjoints et directement utilisables par
  count–scan–fill.

Cette énumération remplace les descentes depuis la racine par O(s³) recherches
de préfixes par terminal d'arêtes. Une autre réalisation acceptable est une
traversée persistante globale de tâches (EdgeBlock,CarrierCell) ; elle doit
conserver le même niveau canonique et sérialiser toute continuation.

La borne porte sur les enregistrements de blocs, jamais sur la masse logique
$\lvert A\rvert\lvert B\rvert\lvert C\rvert$.

## 5. Ne jamais développer aveuglément un bloc ALL

Le compteur exact suivant est EdgeActiveCarrierCounter-q3. Il classe les trois
marges :

- $D-E\geq 0$ ;
- $D-X\geq 0$ ;
- $E+X-D>0$.

Un bloc ALL conserve sa masse et son intervalle ; un NONE disparaît ; un MIXED
est raffiné ou délégué. Les égalités de longueur restent dans le tie-break
d'owner, et l'égalité d'acuité sort de q3.

La porte physique est $M_3=\sum_e m_e$, nombre d'incidences owner-edge–porteur
réellement nécessaires, accompagnée d'un majorant exact avant fill. Si M3 ou
la mémoire de sortie dépasse la ressource admise, le chemin refuse
atomiquement par resource_exhausted. Il ne développe pas le bloc, ne le
transforme pas en sortie et ne perd aucun support.

La masse live est stockée dans un long long signé par le produit ka×kb×kc.
L'ABI industrielle doit employer un compteur saturant large ou tester les
multiplications avant accumulation ; un wrap ne peut jamais rendre une gate
verte.

## 6. Le pied fournit directement la sphère primitive

Pour une incidence owner (ab,x), posons
$G=DE-F^2$. Le rang affine deux exige $G>0$. Définissons :

$$W=E(D-F)d+D(E-F)u.$$

Le centre de la circumsphère minimale q3 vaut
$c=a+\frac{W}{2G}$. Pour un site $z=a+v$, la puissance mise à l'échelle est :

$$P_x(z)=G\left\Vert v\right\Vert^2-v\cdot W.$$

Le site est strictement intérieur si $P_x(z)<0$ et sur le shell si
$P_x(z)=0$.

La déduplication avant census ne doit pas employer une BallKey qui contient
shell_min ou une donnée dérivée du census. La clé pré-census correcte est la
PrimitiveSphereKey obtenue avec les coefficients absolus :

$$A=G,\qquad B=-(2Ga+W),\qquad C=G\left\Vert a\right\Vert^2+a\cdot W.$$

Ils représentent le polynôme
$A\left\Vert z\right\Vert^2+B\cdot z+C$. Réduire
$(A,B_x,B_y,B_z,C)$ par leur pgcd et imposer $A>0$. Cette clé identifie le
centre et le rayon sans connaître le shell. Le run garde une side-list des
SupportKey et de leurs owners. Après census seulement, il dérive la BallKey
sémantique, I_B et U_B.

Sous u16, P demande environ 105 bits dans les bornes déjà dérivées. Le chemin
device doit exposer une ABI signée à deux limbs, ou un entier 128 bits
explicitement vérifié, et des mutants d'overflow i64.

## 7. Q3FootPowerRange-v0 : premier backend GPU, simple et exact

Après radix/RLE des PrimitiveSphereKey, une vague LBVH traite une tâche
(SphereRun,NodeKey,count). Sur une AABB entière, P est la somme de trois
quadratiques convexes $Gv_i^2-W_iv_i$ :

- le minimum entier est atteint à une borne ou aux deux entiers voisins de
  $W_i/(2G)$, après clamp exact et division signée ;
- le maximum est atteint à une extrémité.

Si max P<0, toute la population du nœud est créditée. Si min P>=0, aucun point
strictement intérieur n'est présent. Sinon la tâche se scinde. Le compteur
s'arrête au neuvième intérieur strict : q3 sous smax=11 exige p<=8. Les
égalités P=0 ne comptent jamais dans p.

Une sphère rejetée au neuvième intérieur ne paie aucun census complet. Une
sphère survivante paie exactement un census global, qui collecte les petits
I_B et le shell U_B pour tous les SupportKey du run. Les tâches sont
persistantes ; count=fill=consumed et pending=0 sont des identités de
réception, pas des paramètres de performance.

Ce backend supprime le double scan live : edge_shallow.hpp rescane aujourd'hui
toutes les formes pour chaque pied, puis le nuage pour chaque émission.

## 8. Successeur seulement si les visites LBVH sont rouges

Q3FootLevelLocate-v1 réutilise le LineFormTape local d'une arête. Chaque
carrier interroge un seul point, son pied. Avec
$k=8-c_{\mathrm{inside}}$, construire seulement les niveaux 0 à k des formes
P et les niveaux supérieurs symétriques des formes N, puis localiser tous les
pieds. Pour k fixé, la complexité combinatoire des premiers niveaux de lignes
est O(mk), ce qui rend plausible un coût local
O(m log m+mk+H).

Cette variante est une ablation à recevoir, pas le premier jalon. Les lignes
coïncidentes forment des bundles pondérés, toutes les concurrences sont
atomiques, la forme incidente au porteur est exclue du rang strict, et le shell
reste séparé. Une tape qui omet une forme seulement proposée n'est pas un
juge. Le backend LBVH v0 demeure l'autorité différentielle.

## 9. Pourquoi cette réponse reste positive malgré le pire cas

Une WSSD de triplets aigus peut compresser un broad phase, mais l'acuité seule
n'est pas sparse : trois petits amas autour des sommets d'un triangle
équilatéral portent une masse cubique de triplets aigus.

Il existe aussi une obstruction régulière, et pas seulement cosphérique.
Edelsbrunner et Pach construisent N=2n+2 points réels de dimension trois avec
2n(n+1) triangles Delaunay critiques. Ils sont aigus, de profondeur zéro et
portent des sphères vides distinctes. Tout catalogue explicite de SupportKey ou
de sphères q3 a donc un pire cas quadratique.

Cette construction exige une résolution qui croît avec n et ne prouve pas un
pire cas quadratique à 50 000 sur la grille u16 fixe. Elle n'interdit surtout
pas le bon chemin demandé : un générateur exact output-sensitive peut être très
rapide sur les familles sparse du contrat. Elle impose seulement de publier
les compteurs de sortie et de refuser proprement une instance qui ne tient pas
dans la ressource, à moins qu'un quotient directement consommable par le fold
soit prouvé.

La fixture u16 cosphérique déjà archivée reste complémentaire : 384 points
portent 2 322 560 supports aigus pour une seule sphère. Le RLE économise le
census, jamais la side-list des SupportKey.

## 10. Portes et ordre de réalisation remis à Claude

Le premier jalon borné est CanonicalOwnerEdgeFootStream-q3 :

1. oracle rationnel indépendant BallFormToBallEvent sur petit n ;
2. fenêtre E3 complète, ou continuations consommées, avant publication ;
3. Q3CarrierPrefixRange-v0 et owner global exact ;
4. compteur M3 avant toute allocation de fill ;
5. PrimitiveSphereKey, radix/RLE et side-list SupportKey ;
6. Q3FootPowerRange-v0 persistant, arrêt au neuvième intérieur ;
7. census unique des survivants et comparaison
   (BallKey,SupportKey,I_B,U_B,owner) ;
8. stream vers le fold, sans mosaïque globale ;
9. Q3FootLevelLocate-v1 seulement si le ledger des visites v0 est rouge.

Le ledger bloquant contient au minimum E3, blocs ALL/NONE/MIXED, M3, cellules
de préfixe interrogées et non vides, pieds, PrimitiveSphereKey brutes et
uniques, tâches LBVH, populations créditées, tests feuille, rejets au neuvième,
survivants, visites de census, multiplicité shell, opérations larges,
octets/HWM, continuations et temps par phase.

Fixtures prioritaires :

- équilatéral et isocèle avec égalités d'arête, owner exact-once ;
- angle droit, obtus, rang affine G=0 et signe V²-D ;
- frontière de cellule Morton et adversaire tight-versus-cell ;
- P=-1, P=0, P=1 et seuil huit/neuf ;
- deux supports pour une PrimitiveSphereKey, un census et deux SupportKey ;
- lignes coïncidentes, concurrence et exclusion de la ligne incidente ;
- fixture q3 dont le partenaire dépasse le rang 128 ;
- cosphère u16 de 384 points sans troncature ;
- permutations de PointId, tilings et continuations ;
- correction de la fixture q4 de rang : 4 381 satellites plus x et y donnent
  4 383 voisins plus proches, donc le rang 1-indexé de b est 4 384, jamais
  4 382.

## Réponse synthétique aux questions de Claude

- Oui, la relation niveau deux est la bonne idée ; elle remplace avantageusement
  une WSSD ternaire générique.
- Non, les tests live ne reçoivent pas encore l'unicité globale : ils reçoivent
  l'unicité d'un porteur dans la partition associée à une incidence d'arête.
- Non, le packing O(s³) ne suit pas de boîtes tight. Il devient défendable avec
  un niveau Morton aligné fixé par Dlo, ou une représentation équivalente de
  cellules virtuelles non comprimées.
- Non, la masse d'un bloc ne doit jamais être développée parce que le nombre de
  blocs est plat. M3 est la porte causale.
- Oui, le niveau deux se place avant le range-count/census, mais après une vraie
  fenêtre E3 certifiée.
- Le meilleur premier microkernel n'est pas un arrangement : c'est
  PrimitiveSphereKey plus Q3FootPowerRange-v0. Les niveaux shallow q3 sont le
  successeur adaptatif si les visites le justifient.

Sources primaires :

- Edelsbrunner et Pach, Maximum Betti Numbers of Čech Complexes,
  https://doi.org/10.1007/s00454-025-00796-5 ;
- Kerber et Sharathkumar, Approximate Čech Complexes in Low and High
  Dimensions, https://arxiv.org/abs/1307.3272 ;
- Halperin et al., The Maximum-Level Vertex in an Arrangement of Lines,
  https://arxiv.org/abs/2003.00518, utilisé seulement comme contexte pour
  l'ablation de niveaux, pas comme réception de son ABI dégénérée.
