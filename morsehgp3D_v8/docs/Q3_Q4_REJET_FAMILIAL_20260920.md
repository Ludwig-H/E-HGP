# q3/q4 : rejeter une famille avant de construire ses événements

20 septembre 2026, tranche25. Cadre inchangé :
`exploration_v8_hors_registre`, `cpu_reference`,
`quantized_u16_input_only`, `implementation_v8_p0`,
`public_status=not_claimed`.

## Ce qui change, simplement

Le [parcours couvert précédent](Q3_Q4_COVERS_PARTAGES_20260920.md)
partageait une région de points entre toutes les faces d'une arête. Mais
chaque face relisait cette région pour préparer son comptage et ses
événements. Quand il y a beaucoup de faces et beaucoup de points dans
la région, ce travail peut être quadratique.

Cette tranche ajoute une question préalable, peu coûteuse : **un petit
ensemble partagé contient-il déjà assez de témoins pour rendre toute la
famille inutile ?** Un témoin q4 est ici un point dont on prouve qu'il est
strictement intérieur à toutes les boules positives possibles de la face,
pour l'arête propriétaire considérée. Plusieurs témoins distincts peuvent
donc supprimer la famille entière, sans créer ni trier ses événements.

La réponse négative n'élimine rien. La voie non certifiée reprend son
traitement exact habituel. Le nombre de propositions est un choix de coût,
pas un plafond de recherche ni une limite sur les sorties.

Les deux voies restent indépendantes : q3 concerne la boule de la face ;
q4 concerne ses complétions tétraédriques. Leurs seuils diffèrent. Rejeter
q3 ne permet jamais, à lui seul, de rejeter q4 ; rejeter q4 ne permet pas
non plus de rejeter automatiquement q3.

## Objets et contrat des nouvelles entrées

- `Q34FamilyCertificate` est une valeur géométrique immuable. Sa factory
  exige une face strictement aiguë et une arête ab de longueur maximale
  dans cette face. Les égalités de longueur sont admises. La primitive
  n'utilise pas les IDs et ne tranche donc pas la propriété canonique.
- `Q34WitnessPool` possède le même `Q34EdgeCover` immuable, lequel possède
  l'index et le nuage. Ses IDs sont originaux, distincts, dans l'ordre
  spatial sélectionné. Le pool est non copiable et non déplaçable ; on
  partage son pointeur.
- `run_q34_pruned_seed_candidates` traite une face fournie avec ce pool.
  `run_q34_pruned_edge_candidates` l'utilise pour toutes les faces
  propriétaires générées par le parcours d'une arête fournie.

Les anciennes entrées couvertes restent disponibles comme références.
Un pool vide conserve exactement leurs compteurs de travail. Les règles
de positivité, de propriété par plus longue arête puis IDs, de face
canonique et de première présentation valide d'une boule restent celles
du raccord précédent. Aucune acceptation q2 ou q3 n'est requise pour
accéder aux complétions q4.

Les appels possèdent le pool pendant toute leur exécution. Les buffers de
travail sont privés ; les coquilles transmises au callback sont empruntées
uniquement pendant cet appel synchrone. Une exception propage l'échec sans
annuler les sorties déjà transmises. Le partage immuable autorise des
appels indépendants concurrents ; aucune équipe parallèle interne ni
implémentation GPU n'est ajoutée ici.

## Preuve du certificat géométrique

### 1. La famille de sphères d'une face

Dans toute cette note, **D est une distance au carré**, contrairement à
certaines sections de la note précédente. Pour une face aiguë abx, poser

$$ d=b-a,\quad u=x-a,\quad D=d\cdot d,\quad E=u\cdot u,\quad F=d\cdot u,\quad X=|x-b|^2=D+E-2F. $$

$$ N=d\times u,\qquad G=DE-F^2=|N|^2>0,\qquad W=E(D-F)d+D(E-F)u. $$

Le centre de la boule de la face est
$c_0=a+W/(2G)$ et son rayon au carré vaut
$R_0^2=DEX/(4G)$. Tous les centres des sphères passant par a, b et x
s'écrivent

$$ c_{\mu}=c_0+\frac{\mu N}{2G},\qquad R_{\mu}^2=R_0^2+\frac{\mu^2}{4G}. $$

Pour un site z, avec $v=z-a$, les formes déjà utilisées par
`Q4FamilySeed` sont

$$ P(z)=G|v|^2-W\cdot v,\qquad B(z)=N\cdot v. $$

La puissance de z relativement à cette sphère est
$(P(z)-\mu B(z))/G$. Puisque G est positif, le signe de cette expression
décide exactement si z est intérieur, sur la coquille ou extérieur.
Un quatrième sommet non coplanaire donne la racine $\mu=P(z)/B(z)$.

### 2. Une borne sur tous les paramètres utiles

La borne de rayon ne s'applique pas à toutes les sphères de la famille :
elle exige un support **strictement positif** dont ab est une arête
maximale. Voici une preuve directe de la borne nécessaire.

Pour q sommets de diamètre au carré D et un centre de sphère intérieur
à leur simplexe, écrire ce centre avec ses poids barycentriques positifs
$\lambda_i$, de somme 1. L'identité de variance donne

$$ R^2=\sum_{1\leq i<j\leq q}\lambda_i\lambda_j|p_i-p_j|^2\leq\frac{D}{2}\left(1-\sum_{i=1}^{q}\lambda_i^2\right)\leq\frac{q-1}{2q}D. $$

En q3, l'acuité stricte place le centre dans la face ; ainsi
$R_0^2\leq D/3$, donc $3EX\leq4G$. En q4, la positivité stricte
impose $R_\mu^2\leq3D/8$. En remplaçant le rayon par sa formule,

$$ 2\mu^2\leq J,\qquad J=D(3G-2EX). $$

Le signe de J est acquis, et non supposé :

$$ J\geq D\left(3G-\frac{8}{3}G\right)=\frac{DG}{3}>0. $$

L'intervalle fermé $[-\sqrt{J/2},\sqrt{J/2}]$ contient donc tous les
paramètres des complétions positives propriétaires utiles. Il peut aussi
contenir des sphères inutiles : ce sur-ensemble affaiblit le filtre mais
ne compromet pas sa sûreté.

### 3. Un test entier, sans racine approchée

Choisir le plus petit entier U satisfaisant $2U^2\geq J$, soit

$$ U=\left\lceil\sqrt{\left\lceil\frac{J}{2}\right\rceil}\right\rceil. $$

Pour tout paramètre utile, $|\mu|\leq U$, d'où

$$ P(z)-\mu B(z)\leq P(z)+U|B(z)|. $$

Le certificat q4 est donc l'inégalité **stricte**
$P(z)+U|B(z)|<0$. Elle prouve que z appartient à l'intérieur de toutes
les boules q4 utiles de cette famille. L'égalité ne fournit aucun crédit.
Le certificat q3, distinct, est simplement $P(z)<0$.

Le test q4 implique le test q3 pour un même site. Cela n'implique pas le
rejet simultané des deux voies, car elles n'ont pas le même seuil.
Les sommets a, b et x ont P=B=0 : ils ne peuvent pas être comptés comme
témoins. Une coquille de la boule q3 ne fournit pas non plus de crédit.
On ne déduit jamais d'un échec du test qu'un site serait extérieur.

## Pourquoi l'arithmétique tient dans i128

Cette garantie concerne le profil d'entrée u16, pas des coordonnées
flottantes ou des entiers arbitraires. Poser M=65535. Les différences de
coordonnées sont de valeur absolue au plus M. On dispose des bornes
conservatrices suivantes, également valables pour les sommes partielles
évaluées de cette manière :

| Quantité | Borne suffisante |
|---|---:|
| D, E, X et valeur absolue de F | $3M^2$ |
| G positif | $9M^4$ |
| Chaque composante de W | $36M^5$ |
| Valeur absolue de P | $135M^6$ |
| Valeur absolue de B | $6M^3$ |
| J positif | $81M^6<2^{103}$ |
| U | $7M^3<2^{51}$ |
| $U\lvert B\rvert$ | $42M^6$ |
| $\lvert P\rvert+U\lvert B\rvert$ | $177M^6<2^{104}$ |

Par exemple, $G|v|^2\leq27M^6$ et les trois termes de $W\cdot v$
contribuent au plus $108M^6$. Pour J, calculer
$D(3G-2EX)$ après promotion des opérandes ; les termes internes sont
au plus $27M^4$ et $18M^4$. Enfin l'entier $7M^3$ satisfait déjà
$2(7M^3)^2\geq81M^6$, donc le plus petit U ne le dépasse pas.

Le code calcule $T=J/2+J\bmod2$ sans flottants, puis le plus petit
entier dont le carré est au moins T par dichotomie entière. La longueur
en bits de T donne un majorant initial de la racine ; au plus 51 étapes
suffisent sous u16. Les deux carrés adjacents sont vérifiés avant de
publier U. Ces carrés, les produits et l'addition du certificat tiennent
dans i128. Le compteur `sqrt_iterations` rend ce travail visible.

Il faut promouvoir **avant** les multiplications. La valeur absolue de B
est également calculée après promotion. Il ne faut ni développer un
test contenant $P^2$ dans i128, ni remplacer U par sa large borne
arithmétique $7M^3$ : cette dernière sert à prouver l'absence de
débordement, pas à sélectionner efficacement des témoins.

## Pool partagé, compteurs séparés et reprise exacte

Soit m le nombre de sites du cover et C le minimum entre le budget de
propositions demandé et m. Le pool retient les positions

$$ \left\lfloor\frac{im}{C}\right\rfloor,\qquad i=0,\ldots,C-1. $$

dans la concaténation des plages spatiales du cover. Si C=0, le pool est
vide. Comme C ne dépasse pas m, ces positions sont distinctes. Elles
sont converties en IDs originaux une fois pour toutes. La progression
utilise quotient et reste : elle ne forme pas le produit potentiellement
débordant `i*m` dans un entier machine.

La préparation parcourt les plages nécessaires, pas les m coordonnées.
Son travail est O(nombre de plages+C), son stockage supplémentaire O(C).
Le même pool est réutilisé pour toutes les faces de l'arête ; aucune
recherche de C témoins par scan de m sites n'est cachée dans chaque face.
Ce choix spatial est une stratégie de propositions, pas une preuve de
qualité du pool ni une condition de complétude.

Pour K=Kmax, les seuils sont $h_3=K-1$ et $h_4=K-2$. q3 n'est active
qu'à partir de K=2, q4 à partir de K=3. Les crédits de chaque voie
s'arrêtent à son propre seuil ; les propositions s'arrêtent quand toutes
les voies disponibles sont rejetées, ou lorsque le pool est épuisé.

- Si les deux voies sont rejetées, aucun événement ni scan de cover
  n'est ouvert pour cette face.
- Si q3 seule est rejetée, q4 construit sa famille complète. Son compte
  initial repart de zéro : aucun crédit q3 ou q4 du pool n'est transmis.
- Si q4 seule est rejetée, q3 utilise un vrai parcours seul. Il peut
  s'arrêter dès que son compte intérieur atteint h3. S'il accepte la
  boule, il a parcouru tout le cover et collecte toute sa coquille.
- Si les deux survivent, le parcours fusionné exact est conservé, avec
  des comptes également remis à zéro.

Ce sont des **masques de rejet**, pas des comptes partiels à additionner.
Le même point pourra donc être revisité dans le repli sans être compté
deux fois. Les coquilles restent complètes, avec leurs parties disjointes
triées par IDs originaux. Comme auparavant, un compte couvert q4 ne
devient un compte global publié qu'après positivité et propriété de la
boule. Aucun arrêt au premier groupe q4 profond n'est introduit : les
profondeurs ultérieures peuvent redescendre.

Les treize compteurs `pruning` sont distincts du travail `covered` restant.
Pour une requête certifiée, les quatre états de sortie partitionnent
`seed_queries` : `both_rejected`, `q3_only_survivors`,
`q4_only_survivors`, `both_survivors`. Ici `both_rejected` signifie
« toutes les voies disponibles rejetées » : à K=2 il peut valoir 1
alors que `q4_rejected` reste nul. K=1 et le pool vide ne construisent
aucun certificat et laissent les compteurs de pruning nuls.

## Coût et limite de cette réduction

Avec S faces et C propositions partagées, le filtre ajoute O(S(1+C))
travail, dont la construction géométrique et la racine entière mesurées.
Le travail total doit inclure la préparation du nuage, de l'index, du
cover, du pool, la génération des faces, puis tout le résidu et les
callbacks. On n'évalue pas ce changement par le seul nombre de rejets.

Une face dont q4 survit conserve le coût couvert
O(m log(1+m)), tris d'événements **et de coquilles** compris. Une face
avec seulement q3 peut rejeter après un préfixe, mais une acceptation
exige son scan complet et le tri de sa coquille. Les buffers temporaires
restent O(m) par appel actif, en plus du pool et des objets partagés.
L'entrée actuelle par arête traite les faces successivement.

Ainsi, le coût devient celui des propositions C·S **plus celui des
familles restantes**, et non une nouvelle borne sous-quadratique globale.
Un pool inefficace conserve le terme S·m et ajoute son propre travail.
Choisir C=m réintroduit un scan de tous les témoins par face dans le
filtre lui-même : ce n'est pas la solution industrielle recherchée.
Même un petit pool efficace sur un régime ne borne ni S, ni le résidu,
ni le nombre d'arêtes à traiter dans un futur front global.

## Vérification actuelle et suites non portées

La vérification fonctionnelle passe 86 CTests Release, ainsi que quatre
portes et 18 sondes Clang ASan/UBSan. La nouvelle gate compte 5 176
contrôles. Les captures et leur périmètre sont décrits dans les
[reçus de la tranche25](../receipts/q34_pruning_20260920/README.md).
Aucun chiffre de performance non clos n'est revendiqué dans cette note.

L'oracle de test calcule centres et rayons par élimination de Gram sur
rationnels multiprécision. Il en déduit J par
$8G(3D/8-R_0^2)$, indépendamment de la formule du produit. Il vérifie les
témoins sur toutes les complétions positives propriétaires des petits
nuages, les tangences, les arrondis, les extrêmes u16 et les permutations.
Le raccord est comparé au parcours couvert précédent et à un census
rationnel exhaustif de petites tailles : q3 seule rejetée, q4 seule
rejetée, comptes de repli non crédités, coquille30, pool nul, exceptions,
durée de vie des propriétaires et appels indépendants concurrents.
Les anciennes qualifications ne sont pas transférées à ces nouvelles
sources.

Les96 mesures closes comprennent36 essais de fonds8k/16k/32k à deux
faces, sans rejet supplémentaire. Sur le petit adversaire256, le pool64
réduit65 024 lectures à15 616 (K5) ou33 536 (K10), mais le coût du pool
et des racines entières est ajouté. Une sonde auxiliaire dense teste le
préfixe sur8k/16k/32k avec S=n−2 : à C64/K10, les familles q4 restantes
imposeraient au moins16,280M/74,352M/475,584M lectures de repli avant
leurs tris. Ce repli n'a pas été exécuté dans cette sonde : elle produit
un minorant de travail, pas des résultats de hiérarchie ou une durée.
Les [reçus détaillés](../receipts/q34_pruning_20260920/README.md)
séparent ces périmètres et conservent les ratios défavorables.

L'[auditeur indépendant](../audits/DIALOGUE_COURANT.md) propose deux
renforts : minimiser la profondeur collective du pool sur toute la corde,
pour permettre aux témoins de changer le long du paramètre ; et employer
une borne géométrique de corde plus serrée. Ces propositions restent
**futures, non portées et non qualifiées** dans cette tranche. Leur
arithmétique, leur coût supplémentaire et leur effet sur le résidu
devront être jugés séparément. Le chemin q3 seul avec saturation, en
revanche, est bien inclus dans le raccord décrit ici.

L'accès global q3/q4 par WSPD, le catalogue canonique, les incidences
nécessaires aux hiérarchies, la reconstruction FULL, la parallélisation
massive, GPU/G4 et les contrats de tour restent ouverts. Le paramètre
WSPD s8/10/12 n'intervient pas dans cette primitive d'arête fournie.
GCP n'a pas été utilisé pour cette tranche.
