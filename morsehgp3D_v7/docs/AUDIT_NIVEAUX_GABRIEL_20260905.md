# Quels niveaux sont nécessaires à la hiérarchie ?

5 septembre 2026. Audit constructeur demandé par l'utilisateur, sans
modification du moteur. Cadre inchangé :
`phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

## Verdict et objet conservé

**Tous les niveaux de Gamma ne sont pas nécessaires pour reconstruire les
hiérarchies HGP du manuscrit, isolés inclus. Sous régularité, les naissances
viennent de Gabriel de cardinal K, les fusions de Gabriel de cardinal K+1.
Les seules arêtes Gabriel, en revanche, ne suffisent pas à les construire.**
Il faut conserver l'information de rattachement des facettes réutilisées,
éventuellement sous forme de certificats plutôt que de cofaces silencieuses.

Cette conclusion distingue un objet mathématique plus petit d'un changement
de sémantique caché. Elle ne certifie ni une nouvelle implémentation ni son
coût. Les définitions 21–22 du manuscrit (PDF 84–85) prennent les K-polyèdres
comme les ensembles de points des composantes, **isolés inclus**. Il faut
garder la généalogie de ces composantes, sans les identifier par leur seul
recouvrement en points. La v7 actuelle qualifie leur restriction non triviale
aux ordres supérieurs : les deux objets sont donc séparés ci-dessous.

| Objet demandé | Information nécessaire |
| --- | --- |
| HGP complet, isolés inclus | Naissances Gabriel de cardinal K avec leurs K points ; vraies fusions aux niveaux Gabriel de cardinal K+1 avec leurs parents ; rattachements certifiés pendant la construction |
| Arbre réduit, naissances, parents, multifusions et évolution des points couverts | Niveaux Gabriel exacts et rattachements certifiés aux bonnes composantes |
| Même objet construit par le fold des seules cofaces Gabriel | Insuffisant : E5 produit une fausse naissance puis une fausse fusion |
| Toutes les facettes actives à chaque coupe réelle | Les temps d'activation omis restent nécessaires, explicitement ou implicitement |
| Tour réduite inter-K | Graduation commune exacte et ancres verticales certifiées, pas tous les niveaux Gamma |
| Scores du catalogue Gabriel du manuscrit | Toutes ses incidences contributrices avant réduction, pas seulement les arêtes d'un arbre couvrant |
| Masses des composantes à toute coupe et condensation correspondante | En plus des scores, politique et dates d'affectation des feuilles ; le quotient topologique seul ne les conserve pas |

## 1. Fondement : où peuvent changer les composantes ?

Notons $\beta(Q)=\rho(Q)^2$. Le manuscrit, définition 28 et théorème 4
(pages PDF 113–115), démontre que toute coface K-séparante est Gabriel.
Cette implication nécessaire ne suffit pas, seule, à prouver la suppression
de toutes les autres cofaces : la proposition 6 (PDF 116) oublie leur effet
sur les facettes simultanées. La contre-fixture E5 demeure valide.

Le [lemme des attaches silencieuses](../../docs/math/INCIDENCES_SILENCIEUSES_GAMMA.md#2-lemme-des-attaches-silencieuses)
donne l'énoncé plus précis utile ici. Pour une coface non-Gabriel régulière
Q, un intrus strict remplace un sommet essentiel. Les cofaces de remplacement
ont un niveau strictement inférieur et relient toutes les facettes strictes
de Q dans une même composante antérieure non triviale. Celle-ci couvre déjà
tous les points de Q. L'insertion ajoute éventuellement des facettes, mais
ni naissance, ni fusion, ni point.

Il faut aussi traiter les égalités simultanées. Deux cofaces silencieuses
au même niveau qui partagent une facette stricte touchent le même ancien
apex. Si leur facette commune a le même niveau, l'unicité de la miniball
et le remplacement de support donnent encore le même apex. C'est la
[confluence de plateau](../../docs/math/INCIDENCES_SILENCIEUSES_GAMMA.md#52-théorème-conditionnel-de-rétraction-sur-le-cœur-direct),
pas une hypothèse d'ordre favorable de leurs insertions.

### 1.1 Hiérarchie complète : minima puis fusions

Soit F une facette de cardinal K, au niveau $b=\beta(F)$.
Si F est non-Gabriel, il existe un intrus strict z ; la coface
$F\cup\lbrace z\rbrace$ a déjà le niveau b. F n'est donc pas une composante
isolée à sa naissance. Cette coface possède des facettes strictes déjà
existantes : le nouveau label rejoint donc un état antérieur, éventuellement
lors d'une fusion simultanée, sans nouvelle racine. Avec un seul intrus,
cette première coface peut être Gabriel ; ne pas lui appliquer abusivement
le lemme des cofaces silencieuses.

Si F est Gabriel et sa miniball régulière, aucun point étranger n'est dans
sa boule fermée. Une extension $F\cup\lbrace x\rbrace$ de même niveau
imposerait, par unicité, cette même miniball et contredirait ce fait.
Ainsi $\lambda(F)>\beta(F)$, avec l'infini autorisé : F est une vraie
naissance isolée, au niveau b. Les naissances complètes sont donc exactement
les facettes Gabriel de cardinal K. À K=1, ce sont les points au niveau zéro.

Toute coface régulière Q possède au moins deux facettes strictes, obtenues
en retirant ses essentiels, dont l'union vaut Q. Ces facettes existent déjà
dans l'état FULL strictement antérieur, qu'elles soient isolées ou non.
Une coface ne crée donc pas de composante ex nihilo dans FULL ; elle réunit
des composantes anciennes. Si elle ne rencontre qu'une seule composante,
tous ses points y sont déjà couverts : **aucun delta de points ne survient
dans une continuation FULL**. Le lemme silencieux et la confluence excluent
une vraie fusion portée seulement par un plateau non-Gabriel.

Par conséquent, les seules données événementielles nécessaires sont les
naissances isolées et les vraies multifusions. À l'ordre K, leurs niveaux
appartiennent respectivement aux catalogues Gabriel de cardinal K et K+1.
Une même boule de rang m peut ainsi servir de naissance à l'ordre m et
de connexion à l'ordre m−1, sans produire des événements visibles à tous
les ordres inférieurs. Pour la tour demandée 1..10, les points et les
catalogues de rangs 2..11 couvrent ces valeurs ; ils ne suffisent pas seuls
à calculer les parents, qui exigent les rattachements.

Le générateur v7 fournit déjà, pour chaque directe régulière de rang m,
son support, ses intérieurs et son niveau exact. Ces mêmes données peuvent
décrire la feuille de m points à l'ordre m et l'événement de connexion
à l'ordre m−1. Il n'est pas nécessaire de recalculer une seconde géométrie
pour ces deux rôles. Leur raccord au nouveau fold, leur durée de vie et
l'ordonnancement des niveaux restent à implémenter et qualifier.

Les plateaux sont traités atomiquement. Une facette Gabriel nouvellement
née au niveau b ne peut être incidente à une coface de ce même niveau,
par l'argument précédent. Elle n'est donc pas un parent artificiel d'une
fusion simultanée. Plusieurs cofaces peuvent ensemble produire une seule
multifusion entre les racines du snapshot strict.

La régularité est indispensable à cette forme du certificat de naissances.
Pour A=(0,0,0), B=(2,0,0), C=(1,1,0), AB est Gabriel au sens de l'intérieur
strict vide, mais C est sur son shell et
$\beta(AB)=\beta(ABC)=1$ : AB n'a aucune existence isolée à sa naissance.
L'émettre comme une feuille isolée persistante serait faux. Le traitement
général de ces plateaux demande un quotient certifié de leurs naissances
et connexions simultanées ; il n'est pas qualifié par cet argument régulier.

### 1.2 Hiérarchie réduite déjà qualifiée

**Corollaire de graduation réduit.** Sous ces hypothèses, tout plateau ne contenant
aucune coface Gabriel ne fait que continuer les composantes réduites sans
modifier leur couverture en points. Entre deux niveaux Gabriel consécutifs,
les applications d'inclusion sont donc des bijections sur ces composantes
et conservent leur couverture. L'arbre gradué et ses changements de couverture
se reconstruisent depuis les niveaux Gabriel, en distinguant toujours les
coupes ouvertes et fermées à ces niveaux.

L'identification reste une identification de composantes par les incidences,
jamais une fusion décidée par l'égalité ou le recouvrement de leurs points.
Deux composantes distinctes peuvent partager des observations.

La version régulière globale suffit à ce corollaire. Pour le domaine CPU
déjà qualifié, les boules hors fenêtre éventuellement irrégulières sont
traitées par le [théorème d'inertie de haut rang](../../docs/math/INCIDENCES_SILENCIEUSES_GAMMA.md#531-inertie-h_0-exacte-des-blocs-saturés-au-dessus-de-la-fenêtre-de-rang)
et les contacts du cœur du [certificat horizontal](../audits/CERTIFICAT_HORIZONTAL_COURANT.md).
Un futur constructeur paresseux doit rétablir les prémisses qu'il consomme ;
ne pas hériter automatiquement de la qualification des contrôles exécutés
par l'ancien constructeur. Les extra-shells refusés restent hors contrat.

## 2. E5 réfute le graphe brut, pas ce corollaire

La [fixture permanente](../../tests/fixtures/regressions/gabriel_point_set_counterexample.json)
prend A=(0,0,7), B=(0,9,6), C=(1,4,0), D=(0,0,1), E=(4,1,2).
À K=2, les niveaux ci-dessous sont des rayons **carrés** :

| Niveau | Cofaces | Effet dans Gamma réduit |
| --- | --- | --- |
| 162/25 | CDE, Gabriel | Naissance d'une composante |
| 189/17 | ADE, Gabriel | Continuation et ajout du point A |
| 33/2 | ACD et ACE, non-Gabriel | Rattachement de AC, sans point ni nœud nouveau |
| 83886/3563 | ABC, Gabriel | Continuation et ajout du point B |
| 24 | BCE, Gabriel | Continuation sans point nouveau |

La boule de AC a pour centre (1/2,2,7/2) ; D et E y ont des puissances
respectives −6 et −1. Leurs intrusions sont strictes, pas des égalités
flottantes. Au niveau 83886/3563, la référence possède une seule composante
couvrant ABCDE. Gabriel brut en possède deux, couvrant ABC et ACDE, et
introduit une fusion artificielle à 24. Même son arbre non gradué est faux.

En revanche, aucun nœud de l'arbre réduit ni changement de couverture
n'est à publier au niveau 33/2. Pour interpréter ABC correctement, il suffit
de savoir que sa facette AC appartient déjà à la composante antérieure.
Cela ne signifie pas que l'on peut simplement ignorer AC jusqu'à ABC et
la traiter alors comme une facette sans parent.

## 3. Piste de construction : portails avant consommation

La proposition est un changement de représentation à qualifier, pas un
patch exécuté. Lors du traitement atomique d'un niveau Gabriel a :

1. Figer les composantes strictement antérieures au lot.
2. Pour chaque facette nouvelle réutilisée par une coface directe, déterminer
   si elle doit être rattachée à une de ces composantes antérieures.
3. Obtenir ce rattachement par un certificat de chemin strict vers un
   terminal direct antérieur, ou une autorité équivalente. Normaliser
   l'ancre dans la composante courante ; un ancien identifiant non normalisé
   ne suffit pas après une fusion intermédiaire.
4. Contracter ensemble toutes les cofaces Gabriel du niveau, avec ces parents
   pré-lot. Publier les naissances, continuations et multifusions résultantes.

Un portail silencieux utile à une directe Q de niveau a est strictement
antérieur à a. En effet, une première incidence silencieuse de F exige
au moins deux points étrangers strictement intérieurs et vérifie
$\lambda(F)=\beta(F)$. Si $\beta(F)=\beta(Q)$, les deux miniballs sont
identiques ; Q ne peut absorber qu'un de ces deux intrus puisque
$Q=F\cup\lbrace x\rbrace$. L'autre contredit le caractère Gabriel de Q.
Les premières incidences de niveau exactement a restantes sont directes :
elles appartiennent au lot, sans créer de parent artificiel pré-lot.

Ce report conserve seulement l'objet annoncé au §1. Les calculs exacts de
support, d'intrusion et de descente peuvent encore être nécessaires pour
certifier le portail. Ne pas publier leur niveau comme événement ne prouve
pas que l'on peut cesser de le calculer. Le nombre de portails, les requêtes
évitées et la mémoire doivent être mesurés, sans promesse de coût linéaire.

Dans le moteur F, `Builder::run` construit aujourd'hui les occurrences du
cœur, trie et déduplique, calcule des MEB, émet les cofaces de descente puis
trie leurs niveaux avant le fold. Un portail pourrait supprimer des cofaces
intermédiaires et leur tri, et un traitement à la première utilisation
pourrait éviter une partie de ce travail. Le refus 32k à huit millions
d'occurrences motive cette piste, mais ne valide pas son gain. Les plafonds
de travail ne doivent pas être remplacés silencieusement par des plafonds
de facettes uniques ou de stockage.

## 4. Tour et poids : deux réserves différentes

La [verticale réduite](../audits/CONTRAT_VERTICAL_COURANT.md) est naturelle
par rapport aux inclusions horizontales. Une graduation par l'union des
niveaux Gabriel de tous les ordres demandés conserve donc ses changements,
à condition de transporter les bonnes ancres et de comparer les mêmes
coupes exactes. Des arbres indépendants par K, sans applications entre eux,
ne deviennent pas une tour par simple juxtaposition. Le port du lecteur
vertical doit également être qualifié pour le nouveau payload.

Pour les scores, l'Algorithme 1 du manuscrit (PDF 126) prend les cofaces
Gabriel, leurs facettes, puis agrège **avant** l'arbre couvrant. Le
[contrat des masses](../audits/CONTRAT_MASSES_VOTE_COURANT.md) distingue cet
univers d'un catalogue Čech complet. On ne doit donc pas imposer Gamma
exhaustif aux scores du profil Gabriel, ni ajouter les cofaces auxiliaires
de rattachement comme contributions de poids.

Mais connaître le score n'est pas connaître sa date d'affectation à une
composante. Dans E5, AC appartient aux facettes du catalogue Gabriel car
ABC est directe. Pour toute fonction de poids strictement positive, sa
masse fixe est positive. Avec une affectation à sa première incidence
Gamma, elle rejoint l'ancienne composante au niveau 33/2. Reporter cette
affectation à 83886/3563 change la masse sur l'intervalle intermédiaire,
sans changer les points couverts. Une condensation ou une intégrale
d'excès de masse utilisant cette évolution n'est donc pas automatiquement
conservée.

Le choix autorisé $\psi\equiv1$ rend cette perte rationnelle et explicite.
Sur le catalogue Gabriel complet d'E5, ABC, ADE, BCD, BCE et CDE donnent
$S_{AC}=1$, $T_A=4$, $T_C=8$ et $m_{AC}=3/8$. Sur l'intervalle indiqué,
la composante exacte a une masse de $73/24$, contre $8/3$ si AC reste
en réserve. Au seuil de masse 3, la première est visible et la seconde
ne l'est pas. Ce calcul est un exemple de portée sur la fixture existante,
pas un nouveau résultat de condensation exécuté par le moteur.

Deux contrats distincts sont possibles : conserver les transferts de masse
utiles avec leur date exacte dans un journal séparé, ou déclarer une autre
politique d'affectation sur l'ossature Gabriel. Le second choix ne peut pas
se présenter comme une optimisation transparente du premier. Aucun choix
produit n'est changé par cet audit.

## 5. « Gabriel tous ordres » n'est pas une borne de coût

La [saturation](../../docs/math/TOUR_BOULES_SATUREES.md) vérifie
$\beta(\mathrm{Sat}(Q))=\beta(Q)$. Le saturé est lui-même Gabriel puisque
tous les points de sa boule fermée y appartiennent. Tous les niveaux
Gamma sont donc aussi des niveaux de saturés Gabriel à un ordre
éventuellement bien supérieur à K. Cette identité ne réduit pas à elle
seule le travail : leur rang peut atteindre n.

Dans E5, le niveau 33/2 est déjà celui de la directe ACDE à K=3. Cela
n'autorise pas à tronquer une représentation saturée complète à K=10 :
une facette d'ordre inférieur peut avoir un saturé de rang supérieur à 11.
La voie industrielle à étudier reste une ossature limitée aux ordres utiles
et des rattachements certifiés à la demande, sans catalogue Gamma ni
énumération globale de supports de taille quatre.

## 6. Certificat suffisant : ce qui doit réellement rester

La précision utilisateur porte sur les **K hiérarchies HGP**, pas sur une
reconstruction de toutes les cellules de Gamma. Pour l'objet FULL défini
au §1.1, le certificat suivant est suffisant. « Suffisant »
ne veut pas dire optimum universel en nombre de bits : les encodages et
preuves de minimalité informationnelle sont une autre question.

### 6.1 Sortie persistante FULL par ordre

Conserver une identité d'entrée, les ordres disponibles, le contrat de coupe
et l'autorité terminale de complétude. Chaque feuille est une facette
Gabriel de cardinal K : conserver son identité, ses K PointId et son niveau
de naissance. Chaque nœud interne est une vraie multifusion : conserver
son identité, son niveau exact et ses parents distincts.

**Reconstruction.** Activer les feuilles à leur niveau, puis remplacer les
parents par leur fusion en traitant chaque niveau atomiquement. La couverture
d'un nœud est l'union des points de ses feuilles descendantes. Le §1.1
prouve que ces seules opérations sont tous les changements HGP possibles.
Les coupes, les recouvrements en points et la généalogie en découlent par
induction. Aucune continuation FULL, aucune coface silencieuse, aucun
journal de nouveaux points ni copie de la couverture de chaque nœud n'est
nécessaire à ce rejeu. On peut bien sûr indexer certaines unions pour
accélérer les lectures ; c'est un cache, pas une donnée mathématique nouvelle.

Ces feuilles ne sont pas toutes les facettes de Gamma : une facette
non-Gabriel apparaît déjà rattachée et n'est pas une nouvelle composante.
Elles ne sont pas non plus automatiquement l'univers des feuilles pondérées
du §9.1. Cette seconde projection demande le supplément du §4.

Si $L_K$ est le nombre de ces feuilles et $R_K$ le nombre de racines finales,
le nombre de nœuds internes vérifie $I_K\leq L_K-R_K$ : chaque multifusion
consomme au moins deux composantes et en rend une. La forêt possède
$L_K+I_K-R_K$ liens de parenté. Son stockage topologique est donc linéaire
en $L_K$, et les labels de feuilles coûtent $O(KL_K)$ identifiants.
**Il ne s'agit pas d'une borne linéaire en n** : aux ordres supérieurs,
$L_K$ est le nombre de minima Gabriel, pas le nombre de points. Aucun
contrat universel de n−1 fusions par ordre n'est introduit.

### 6.2 Restriction réduite : pourquoi ses deltas sont différents

Conserver une identité d'entrée, les ordres disponibles, le contrat de coupe
et la certification terminale de complétude. À K=1, conserver les racines
ponctuelles normatives, éventuellement implicitement par le manifeste des
PointId. Pour chaque ordre, conserver ensuite :

- chaque naissance : son identité, son niveau exact et ses points couverts ;
- chaque multifusion : son identité, son niveau exact, les identités de
  ses parents distincts et les éventuels points nouvellement couverts ;
- chaque continuation qui gagne des points : la composante concernée,
  le niveau exact et ces seuls points nouveaux.

Les nouvelles identités ne sont pas des hashes d'ensembles de points utilisés
comme test de connexité. Deux composantes gardent des identités distinctes
même si leurs couvertures se recouvrent. Les liens de parenté conservent
l'information que cette projection seule ne porte pas.

**Preuve de rejeu.** Initialiser les racines K1 et aucune racine réduite aux
ordres supérieurs. Par niveau atomique, lire les parents dans l'état strict
antérieur, les remplacer par la composante de sortie puis calculer sa
couverture par union des couvertures parentales et du delta. Une naissance
a zéro parent ; une continuation conserve sa composante abstraite ; une
multifusion en a au moins deux. L'induction reconstruit donc les composantes,
leur généalogie et leur couverture à toute coupe. Entre deux niveaux
conservés, cet objet ne change pas. Une continuation à un parent et sans
point nouveau peut être contractée sans modifier ce rejeu.

Chacune de ces catégories répond à une perte concrète : sans niveaux,
les coupes métriques sont perdues ; sans parents, le regroupement est perdu ;
sans deltas de continuation, E5 perd l'ajout de A puis de B ; sans identités
distinctes, le recouvrement de points peut fusionner des branches à tort.
Sans autorité de complétude, un préfixe ou un flot Gabriel brut peut imiter
syntaxiquement une telle sortie. La validation syntaxique du journal n'est
donc pas une preuve de sa fidélité à l'entrée géométrique.

### 6.3 Supplément pour la tour

Pour chaque vraie naissance à l'ordre supérieur, une référence certifiée à
sa composante cible dans l'ordre inférieur, à la même coupe fermée, suffit.
Porter ensuite cette référence par les continuations et la normaliser dans
l'histoire inférieure ; lors des multifusions, les images des parents
doivent coïncider. La naturalité prouvée du contrat vertical donne toutes
les cartes aux coupes ultérieures et toutes les compositions d'ordres.

Pour une naissance FULL de label F de cardinal K, F est elle-même une
coface directe de l'ordre inférieur K−1 au même niveau. Son groupe inférieur
fermé fournit donc une ancre naturelle. La preuve du scan `born` du lecteur
réduit ne doit pas être transposée à une feuille FULL sans ce raccord.

Une facette géométrique peut être le témoin utilisé pour certifier cette
référence pendant la construction. Il n'est pas nécessaire de conserver
simultanément toutes ses cofaces, tous ses chemins de preuve et une table
de cartes à chaque coupe dans la sortie destinée au seul rejeu. La
provenance d'audit reste liée séparément. Les premières matérialisations
qui ne créent aucune naissance ne demandent pas une nouvelle ancre verticale.

### 6.4 État de construction, à ne pas confondre avec la sortie

Le constructeur doit pouvoir résoudre les facettes qu'une coface directe
réutilise : facette incidente avant le lot, facette latente, première
incidence dans le lot, ou alias certifié vers une ancienne composante.
Ces distinctions sont nécessaires à la décision mais n'imposent pas un
catalogue exhaustif de facettes dans le résultat. La simple présence d'une
clé dans un dictionnaire n'est pas une incidence strictement antérieure.

En FULL, une facette Gabriel déjà née mais encore isolée est également
une vraie racine pré-lot. Le prédicat réduit « déjà incidente » ne convient
donc pas : il faut « racine FULL déjà née ». Les deux dispatchers doivent
être qualifiés séparément, sans réinterpréter le champ `seen` en silence.

Les chemins de descente validés peuvent être remplacés par leur ancre et
leur autorité ; leurs cofaces ne doivent pas être réémises pour le seul
rejeu de l'arbre. Le contrôle de shell ne disparaît pas parce que l'ancre
est trouvée. Un cache n'est réutilisable qu'après validation terminale et
avec normalisation de son ancien token dans le snapshot courant.

Le coût exact de ce constructeur reste à démontrer et mesurer : la petite
taille du certificat final ne borne pas le nombre de candidats nécessaires
pour le découvrir. Le nouveau schéma ne reprend pas automatiquement les
anciens compteurs, refus, digests ou identités publiques.

### 6.5 Ce que ce certificat ne prétend pas reconstruire

Il ne reconstruit ni toutes les facettes incidentes à chaque coupe, ni les
adjacences complètes, ni les carriers géométriques marqués, ni les masses
de feuilles sans le supplément du §4. Si l'un de ces objets fait partie
du contrat de consommation, son information irréductible doit être ajoutée
séparément, plutôt que réintroduire Gamma complet par défaut. En particulier,
la conservation de l'arbre et des points ne dispense pas de fixer le profil
pondéré avant de revendiquer la même condensation et les mêmes votes.

## 7. État de l'audit et prochaines portes

Le manuscrit pertinent, les preuves transverses, les contrats horizontal,
vertical et pondéré ainsi que le constructeur F ont été confrontés.
Les contre-fixtures citées sont déjà permanentes ; aucun nouveau résultat
produit n'est attribué aux petits recalculs rationnels de cette relecture.
La question et la proposition de portail ont été adressées à l'auditeur
indépendant dans la [coordination](../../audits/COORDINATION_MORSEHGP3D_V7.md).

Avant une intégration : preuve de fidélité des portails réellement produits,
plateaux atomiques, distinction facette absente/isolée/incidente, contrôles
de shell et budgets, qualification sur E5 et les contacts hors fenêtre,
conservation des ancres verticales et contrat pondéré explicite. Le statut
public et les contrats 50k restent inchangés. Aucun benchmark ni GCP utilisé
pour cet audit.
