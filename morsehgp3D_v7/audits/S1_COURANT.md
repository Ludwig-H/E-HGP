# S1 courant : du propriétaire à l'émission d'un support

Actualisé le 5 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

**Résultat.** Les clauses géométriques propriétaire, seed, cover, accès à la complétion et préfiltres entiers q4 ci-dessous se ferment directement sur le code examiné. Avec les preuves du front, des secteurs/cordes, des cellules et des filtres flottants, elles donnent le théorème géométrique conditionnel du § 6 : chaque boule minimale pertinente possède effectivement un représentant de bonne arité après RLE. Les primitives et leur raccord compilé sont désormais qualifiés sur E dans le domaine du [certificat horizontal réduit](CERTIFICAT_HORIZONTAL_COURANT.md), qui compose ce résultat avec la complétion et le lecteur des deltas. Cette actualisation n'ajoute aucun run ni code produit.

On considère un support minimal positif de taille q, sa boule B, son rayon R et une arête maximale AB de longueur D. Les positions et PointId sont distincts. Les assertions géométriques ci-dessous supposent les expressions entières exactes, un index d'arbre correct et l'absence de mutant.

## 1. Matrice des clauses

| Clause | q2 | q3 | q4 |
|---|---|---|---|
| Propriétaire | La paire AB elle-même; son unique rectangle relève de la partition WSPD. | Arête de longueur maximale, puis plus petite `EdgeKey`; les deux autres arêtes sont comparées dans [q3.hpp:89](../src/lanes/q3.hpp#L89). | Même règle sur les six arêtes dans [q4.hpp:127](../src/lanes/q4.hpp#L127); le propriétaire du tétraèdre est aussi celui de ses faces incidentes. |
| Seed et canonique | Aucun seed; émission immédiate après les gardes d'ancre, [generate.hpp:1342](../src/pipeline/generate.hpp#L1342). | Son troisième sommet est l'unique seed du support; acuité et propriétaire sont exactement les critères de [q3.hpp:98](../src/lanes/q3.hpp#L98). La boucle parcourt tout le cover, [generate.hpp:792](../src/pipeline/generate.hpp#L792). | Au moins une face ABX est aiguë, preuve dans le [retour mathématique](RETOUR_MATH_COURANT.md#4-un-raccord-autonome-supplémentaire-pour-s1--existence-dun-seed-q4-aigu). Le plus petit PointId parmi ces X est conservé : [generate.hpp:906](../src/pipeline/generate.hpp#L906), puis [1140](../src/pipeline/generate.hpp#L1140). |
| Inclusion dans le cover | Aucun cover dans cette lane. | Tout le contenu fermé de B satisfait le coefficient 3, preuve § 2. | Les sommets du support satisfont déjà le coefficient 3 et la lentille; tout le contenu fermé de B satisfait le coefficient 4, preuve § 2. |
| Complétion et filtres locaux de forme | Construction de la boule diamétrale à l'émission. | Le seed représente directement B; le test de profondeur exact n'écarte que p≥h3, sous certification correcte des signes flottants. | Y est présent dans le cover et sa racine appartient à la corde fermée, § 3. Les filtres de lentille, propriétaire, canonique, i64 et puissance de face préservent tout support positif propriétaire, § 4. |
| Arrêt et terminaison | Retour anticipé de lane seulement par élimination ou cap déclaré. | Parcours fini du cover; l'arrêt d'émission remonte au producteur. | Parcours fini des seeds et des groupes de racines; l'arrêt d'émission remonte au producteur. Le succès terminal global dépend du traitement complet des rectangles et de la propagation des caps : [generate.hpp:1384](../src/pipeline/generate.hpp#L1384), [1404](../src/pipeline/generate.hpp#L1404), [1439](../src/pipeline/generate.hpp#L1439). |

La clause canonique q4 mérite une précision : au moment du rejet `uy < ux`, la lentille et le propriétaire du tétraèdre ont déjà été vérifiés. Si Y est hors boule diamétrale, ABY est donc aussi un seed admissible. Le rejet ne peut éliminer le plus petit seed admissible; toute autre présentation est rejetée. L'ordre Morton du parcours ne change pas ce raisonnement, puisque la comparaison porte sur les PointId.

## 2. Inclusion géométrique et accès effectif au seed

Le centre c d'un support positif s'écrit $c=\sum_i\lambda_i u_i$ avec tous les poids positifs et leur somme égale à un. L'identité de variance donne :

$$R^2=\frac{1}{2}\sum_{i,j}\lambda_i\lambda_j\lVert u_i-u_j\rVert^2\leq\frac{D^2}{2}\left(1-\sum_i\lambda_i^2\right)\leq\frac{q-1}{2q}D^2.$$

Pour le milieu m de AB, l'équidistance donne $\lVert c-m\rVert^2=R^2-D^2/4$. Tout point z de B satisfait $\lVert z-m\rVert\leq R+\sqrt{R^2-D^2/4}$. À q=3, cela donne $4\lVert z-m\rVert^2\leq3D^2$. À q=4, on obtient la borne plus fine $4\lVert z-m\rVert^2\leq(2+\sqrt{3})D^2<4D^2$. Les coefficients effectivement passés au cover sont bien 3 et 4 : [generate.hpp:1312](../src/pipeline/generate.hpp#L1312).

Tout sommet X d'un support possédé par AB satisfait aussi la lentille, puisque AX et BX ne dépassent pas D. L'identité $\lVert2X-A-B\rVert^2=2AX^2+2BX^2-D^2\leq3D^2$ établit directement sa présence dans les deux covers. Le filtrage de lentille de [generate.hpp:900](../src/pipeline/generate.hpp#L900) ne le supprime pas.

Cette inclusion se compose avec les deux étages de l'index. La somme A+B appartient à la boîte des sommes du rectangle, et D² ne dépasse pas son Dmax². Le minimum de distance d'une boîte de points à cette boîte des sommes ne peut donc éliminer un point qui satisfait la borne de son ancre. `rect_cover_handles` ne rejette qu'une inégalité **strictement** supérieure à la borne et ses handles forment une antichaîne : [edge_cover.hpp:182](../src/lanes/edge_cover.hpp#L182). Chaque handle conservé est parcouru entièrement, puis le test d'ancre garde l'égalité : [edge_cover.hpp:219](../src/lanes/edge_cover.hpp#L219). Le tri par seaux est une permutation; il ne filtre aucun site.

Ainsi, **si l'ancre atteint la construction de son cover**, son seed et ses sommets de complétion y sont effectivement accessibles, sans hypothèse de densité ni de régularité globale du nuage.

## 3. Pourquoi la complétion q4 figure dans le sweep

Soient c0 et R0 le centre et le rayon de la face aiguë ABX, G le déterminant de Gram, et $n=(B-A)\times(X-A)$, avec $\lVert n\rVert^2=G$. Tout centre équidistant de A,B,X s'écrit $c=c_0+t n$. La forme `q3_power` vaut $P(z)=G(\lVert z-c_0\rVert^2-R_0^2)$; avec $B_z=n\cdot(z-A)$ et $\mu=2Gt$, l'incidence de z s'écrit $P(z)-\mu B_z=0$.

Une complétion q4 est affinement indépendante, donc $B_Y\ne0$. Elle possède la racine $\mu=P(Y)/B_Y$. Son rayon vérifie $R^2=R_0^2+\mu^2/(4G)$. La borne q4 du § 2 entraîne :

$$2\mu^2\leq J,\qquad J=D^2(3G-2AX^2BX^2).$$

C'est exactement le test fermé $2P(Y)^2\leq J B_Y^2$ de [generate.hpp:1059](../src/pipeline/generate.hpp#L1059). Y ne peut être classé hors corde. La borne q3 $R_0^2\leq D^2/3$ donne également $J\geq GD^2/3>0$; le refus d'invariant J<0 ne peut éliminer un vrai seed aigu en arithmétique exacte.

Le quotient entier `l_exact / 4` représente exactement P : développer `AffineSeed` avec $u=2(z-A)-(B-A)$ et $d\cdot W=GD^2$ donne `l_exact = 4*q3_power`. Ce n'est pas un arrondi admis. La construction de racine conserve chaque site non coplanaire sur la corde, avec son identité, [generate.hpp:1071](../src/pipeline/generate.hpp#L1071).

Le compte au groupe de racines est la somme des témoins constants, des entrées strictement antérieures et des sorties strictement postérieures. Les sorties du groupe sont retirées avant lecture et les entrées ajoutées après : [generate.hpp:1116](../src/pipeline/generate.hpp#L1116). Les points de shell ne contribuent donc pas. Le cover contenant tous les intérieurs et ne dupliquant aucun site, cette lecture est le vrai p de la boule au groupe. Les crédits d'amont ne sont pas réadditionnés à ce compte.

## 4. Les deux préfiltres i64 q4 sont nécessaires

Pour tout sommet Y du support, les mêmes poids barycentriques donnent $\sum_j\lambda_j\lVert Y-u_j\rVert^2=2R^2$. En posant $M=\max(AY^2,BY^2,XY^2)$, on obtient $2R^2\leq(1-\lambda_Y)M<M$. Comme AB est contenu dans B, $R\geq D/2$, donc $2M>D^2$. C'est la première condition de [q4.hpp:109](../src/lanes/q4.hpp#L109).

Pour la seconde, supposons au contraire $AX^2+AY^2\leq D^2$ et $BX^2+BY^2\leq D^2$. La condition XY²≤D² a déjà été vérifiée à [generate.hpp:1127](../src/pipeline/generate.hpp#L1127). Additionner les identités de variance centrées en X et Y donne alors :

$$4R^2=\lambda_A(AX^2+AY^2)+\lambda_B(BX^2+BY^2)+(\lambda_X+\lambda_Y)XY^2\leq D^2.$$

Le rayon serait D/2 et le centre le milieu de AB, incompatible avec la positivité stricte du support tétraédrique. Le maximum des deux sommes doit donc dépasser D². Cette preuve explique pourquoi le filtre est valide **après** le contrôle de propriété/lentille du tétraèdre.

Enfin, la puissance géométrique de Y dans la boule de la face vaut $2t B_Y$; `q3_power` la multiplie par G>0. Un centre strictement intérieur au tétraèdre est du même côté de cette face que Y, donc ce produit est strictement positif. Le filtre `q4_face_power_prefilter` de [q4.hpp:120](../src/lanes/q4.hpp#L120) est donc nécessaire. Les contrôles ultérieurs de déterminant et de centre certifient directement l'indépendance affine et la positivité recherchées.

## 5. Composition des éliminations

Les preuves précédentes commencent par une ancre ou un seed non éliminé. Pour transformer leur conclusion en S1, chaque suppression en amont doit impliquer, pour **toute** boule positive qu'elle représente, $p(B)\geq h_q=r_{\max}+1-q$. Le quantificateur porte sur toutes les complétions du seed ou de l'ancre; un compte correct au seul centre de la face ne suffit pas.

La [preuve du front et des témoins](FRONT_ET_TEMOINS_COURANT.md) ferme conditionnellement les clauses WSPD, cônes aux coins, boules de cœur, crédits de rectangle et histogrammes : elle s'applique aux morts de lane [generate.hpp:382](../src/pipeline/generate.hpp#L382) et aux tests de ligne/seuil [1298](../src/pipeline/generate.hpp#L1298). Sa terminaison utilise une hauteur d'arbre strictement décroissante à chaque scission. Son test minimax Hmax exclut un témoin **universel**, sans prétendre exclure les témoins de chacune des autres ancres. Ces clauses ne restent donc pas des réserves sur l'accès au propriétaire.

La [preuve des secteurs et cordes](PREUVE_CHORD_SECTOR_COURANTE.md) ferme les rejets d'ancre correspondants de [generate.hpp:1353](../src/pipeline/generate.hpp#L1353) et le prune par morceaux de corde de [998](../src/pipeline/generate.hpp#L998), sous son contrat arithmétique explicite. Elle justifie le crédit d'extrémités sur le disque des centres admissibles, sans l'exiger sur tout le polygone artificiel. Les comptes de cœur et de corde sont réunis par un OU, pas additionnés.

La [preuve des cellules](CELLULES_COURANT.md) ferme le rejet de l'ancre à [generate.hpp:789](../src/pipeline/generate.hpp#L789) et [904](../src/pipeline/generate.hpp#L904), le rejet du centre du seed q3 à [798](../src/pipeline/generate.hpp#L798) et celui de la corde du seed q4 à [916](../src/pipeline/generate.hpp#L916). Chaque compteur est strict sur sa cellule fermée; la grille couvre les centres admissibles. Le localisateur conserve toutes les cellules nécessaires au centre ou à tout le segment, sous son contrat binaire64 explicite. Un échec de construction ou une grille non demandée laisse poursuivre l'ancre.

La [preuve des filtres flottants](FILTRES_FLOTTANTS_COURANTS.md) ferme les décisions affine, Jung et corde sous les conversions et opérations binaire64 correctement arrondies au plus proche, FMA correcte et séquence sans réassociation. Elle compte explicitement les arrondis des extrémités `lh ± E`, des produits et des marges. Les égalités ne deviennent pas des témoins stricts. Ces exigences numériques sont des prémisses nommées du théorème, pas des décisions approximatives tacitement admises.

La composition de ces crédits est explicite : le cœur de rectangle exclut A∪B dans [witness_count.hpp:38](../src/spindle/witness_count.hpp#L38), tandis que `ha` compte dans A sans a et `hb` dans B sans b, [generate.hpp:495](../src/pipeline/generate.hpp#L495). Ces trois populations sont disjointes et leur autorité géométrique est détaillée dans la preuve du front. Leur emploi par les rejets suivants doit conserver les populations annoncées; il ne faut pas rouvrir l'existence du seed ou l'inclusion du cover pour traiter cette composition distincte.

## 6. Théorème géométrique conditionnel et RLE

Supposons un index valide de positions distinctes dans le profil u16, les opérations entières et les comparaisons larges conformes à leurs contrats sans débordement, les parcours et tris conformes aux boucles décrites, et le contrat binaire64 des deux preuves numériques précédentes. Supposons enfin une exécution terminale réussie, sans mutant ni cap ayant interrompu le flux. Alors, pour toute boule B de support minimal positif et de rang $p(B)+q(B)\leq r_{\max}$, le catalogue après RLE contient B avec son arité minimale q(B).

**Preuve.** Choisir un support positif de B de cardinal minimal q, puis son arête propriétaire AB. Le front partitionne les paires et atteint l'unique rectangle de cette ancre. Toute élimination de lane, de rectangle ou d'ancre imposerait au moins $h_q=r_{\max}+1-q$ intérieurs stricts de B, ce qui contredirait sa pertinence. Le propriétaire survit donc.

En q2, l'émission est immédiate. En q3, le troisième sommet est dans le cover et satisfait le test du seed propriétaire. En q4, choisir le plus petit seed aigu de ce support; il est dans le cover, ainsi que le quatrième sommet Y. Les certificats de secteur, cellule, cœur et corde ne peuvent éliminer ce seed, puisque chacun imposerait de nouveau $p(B)\geq h_q$. La racine de Y appartient au sweep fermé. Les filtres de forme et de canonique préservent cette présentation; le compte de profondeur strict est p(B)<hq. La boule est donc émise. Ses formes q2/q3/q4 et son niveau sont ceux de B par les identités exactes exposées et les contrôles de positivité.

Les seules émissions sont des supports positifs : paire distincte en q2, triangle aigu en q3, tétraèdre affinement indépendant et strictement bien centré en q4. Il ne peut donc exister pour B une émission d'arité inférieure à q(B). Les différentes formes de la même boule sont des multiples rationnels positifs; la division par le PGCD de leurs coefficients, avec coefficient quadratique positif, leur donne la même `BallKey` primitive, [keys.hpp:101](../src/lanes/keys.hpp#L101).

Le tri compare d'abord cette clé, puis l'arité croissante, [candidates.hpp:28](../src/pipeline/candidates.hpp#L28). Le dédoublonnage conserve le premier représentant de chaque clé, [candidates.hpp:40](../src/pipeline/candidates.hpp#L40). Puisqu'un représentant d'arité q(B) a effectivement été émis et qu'aucun plus petit n'est possible, l'arité conservée est exactement q(B). Le départage par représentation de niveau ne change pas le niveau géométrique commun. Cela ferme le raccord après RLE, sans exiger l'émission de tous les supports alternatifs de B.

Cette conclusion établit le parcours conditionnel complet. Les preuves locales des [lanes et de Cramer](ARITHMETIQUE_LANES_COURANTE.md), puis des [produits larges et réductions PGCD](ARITHMETIQUE_LARGE_COURANTE.md), sont désormais contre-lues et fermées sous leurs préconditions. Les portes C++ causales sont désormais intégrées et leurs [reçus contre-vérifiés](AUDIT_QUALIFICATION_20260905.md), sans changer les domaines de leurs preuves. [AxisBounds](CENSUS_AXIS_COURANT.md) conserve ses six portes indépendantes. La [preuve de l'index](AUDIT_INDEX_20260905.md) et son [raccord au front et aux covers](AUDIT_RACCORD_INDEX_FRONT_20260905.md) déchargent les invariants topologiques et la permutation locale du cover sous la garde produit existante. La [garde d'arrondi](AUDIT_ARRONDI_20260905.md) est exercée aux quatre modes ; le grand-livre est désormais complété pour les [fuseaux](ARITHMETIQUE_SPINDLE_COURANTE.md), [secteurs/cordes](ARITHMETIQUE_SECTEUR_CORDE_COURANTE.md) et [cellules](ARITHMETIQUE_CELLULES_COURANTE.md). Le [domaine CPU](DOMAINE_CPU_COURANT.md) identifie les commandes ; les [frontières nouvelles](receipts_front_compiled_20260905/README.md) sont désormais exécutées sur les helpers compilés en O2/UBSan, avec juges indépendants et mutants causaux. Aucun accord v6/v7 ne remplace ces contrats ni leur liaison aux octets exécutés. Aucune clause géométrique d'existence de seed, de cover, de corde ou de grille n'est laissée comme verrou ouvert par les notes citées. Le [certificat horizontal réduit](CERTIFICAT_HORIZONTAL_COURANT.md) ferme désormais aussi l'assemblage horizontal sur la route CPU E terminée dans son domaine accepté. Les sources F concurrentes, la verticale, les identités publiques du quotient et les coûts gardent leurs qualifications propres.

Les [hashes des fichiers relus](receipts_20260904/s1_sources.json) épinglent cette lecture. Les preuves sont autonomes et ne reprennent aucun reçu d'une autre lignée. Aucun statut public promu. **GCP non utilisé.**
