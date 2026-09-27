# Piste de compression : dépôts de masse datés sur FULL

27 septembre 2026. Audit mathématique préalable, **sans implémentation ni
qualification de performance**. Aucun fichier ou reçu gelé n'est modifié.
La proposition concerne la mesure Gabriel complète à K,z fixés et la
connectivité FULL correcte, pas le graphe Gabriel seul. Elle complète le
[plan de partitions ponctuelles](PLAN_PARTITIONS_POINTS.md), sans confondre
ce futur routage avec le vote plat actuellement évalué.

## Verdict et hypothèses

Il est possible, en arithmétique exacte, de condenser sans représenter chaque
facette par un atome persistant. Regrouper les facettes qui ont le même
événement d'attachement conserve les masses, les stabilités, la sélection
EOM et les votes sous les conditions suivantes :

- catalogue contributif complet et dédupliqué, poids strictement positifs ;
- attache de chaque facette dans le bon segment fermé de T_K à sa naissance
  MEB exacte, y compris les attaches silencieuses ;
- seuil m strictement supérieur à **chaque** masse de facette individuelle ;
- dépôts traités comme des sorties datées, jamais comme des enfants
  admissibles ; événements de même niveau traités atomiquement ;
- mêmes règles de continuation, racine exclue, sélection EOM et départages.

La borne mτ≤1 de la mesure complète rend m>1 suffisant ; le profil actuel
m≥2 satisfait donc la condition sans plafond dépendant de K. Cette condition
porte sur les facettes individuelles, pas sur la masse totale d'un dépôt.
Une faible masse moyenne ne suffirait pas. Le cas m≤max mτ est hors de
cette équivalence : une facette peut alors devenir une branche admissible.

Cette équivalence mathématique ne donne **pas** automatiquement une égalité
bit à bit avec l'implémentation binary64 gelée ; voir la section numérique.

## 1. Les données agrégées et leur conservation

Pour une facette τ, définir e(τ)=(segment fermé, βτ), avec βτ son rayon carré
MEB. Deux dates distinctes sur le même segment sont deux événements.
À une fusion, normaliser dans l'état fermé avant de former la clé.
L'unité géométrique et le squelette T_K restent communs et épinglés.

Pour un événement e, les données de mesure sont :

$$A_{e,x}=\sum_{\tau:e(\tau)=e,\,x\in\tau}S_{\tau},\qquad v_{e,x}=\frac{A_{e,x}}{T_x},\qquad m_e=\sum_x v_{e,x}=\sum_{\tau:e(\tau)=e}m_{\tau}.$$

On ne stocke que les couples (e,x) non nuls. Pour T_x=0, il n'existe aucune
contribution positive correspondante et aucune division n'est effectuée.
Les deux invariants de conservation sont :

$$\sum_e v_{e,x}=1\quad\text{si }T_x>0,\qquad \sum_e m_e=\#\{x:T_x>0\}.$$

Conserver A plutôt que seulement v permet de comparer les votes bruts avant
normalisation, comme la référence. Les dépôts suffisent à la condensation
et aux votes à ce z ; ils ne permettent plus de restituer les identités de
toutes les facettes ni de refaire leurs vérifications géométriques.
Les preuves du producteur et un petit oracle de facettes restent distincts.

## 2. Sens exact de la sortie datée

On parcourt la filtration à l'envers, avec λ=β^(-z/2) croissant. À un
événement de rayon carré β, les composantes géométriques de rayon inférieur
sont les seuls enfants susceptibles d'être admissibles. Les facettes qui
naissent à β constituent un dépôt qui disparaît à cette transition.
Pour β=0, notamment les points à K1, λ=+∞ est une valeur symbolique,
pas une soustraction flottante implicite de deux infinis. Les petites
branches éliminées auparavant ne doivent jamais atteindre cette sortie.

Un dépôt de masse m_e≥m **ne devient pas un enfant**. Dans l'arbre de
référence, ce sont plusieurs feuilles distinctes dont chacune a une masse
strictement inférieure à m. Les remplacer par une grosse feuille virtuelle
changerait la condensation et pourrait créer une stabilité infinie fictive.

Nuance indispensable : λ(β_e) est la sortie propre du dépôt **si sa branche
est encore explorée**. Lorsqu'un sous-arbre de masse totale inférieure à m
est élagué plus tôt, tous ses dépôts sortent au niveau de cet élagage, pas à
leurs dates propres ultérieures. Il ne faut pas continuer à les compter
comme présents après la disparition de la branche.

Exemple géométrique : le triangle équilatéral entier à K2 possède trois
facettes de masse un, nées à β=2 et réunies à β=8/3. Pour m=2, elles sortent
toutes à la séparation β=8/3. Attendre β=2 pour leur sortie condensée
surestimerait la durée. Ce cas distingue naissance géométrique et sortie
après élagage ; il n'invalide pas le dépôt daté correctement traité.

## 3. Récurrence de condensation et preuve d'équivalence

Insérer conceptuellement les dates de dépôts dans les segments FULL.
Il n'est pas nécessaire de créer une feuille par facette ; les événements
portent des masses sortantes, et les branches portent les sous-arbres de
dépôts. Les branches de masse nulle ne transmettent pas de masse mais leur
connectivité ne doit pas être réinventée à partir des votes ponctuels.

À un événement visité à λ, pour le cluster condensé courant :

1. Faire sortir le dépôt local à λ. Pour chaque enfant géométrique de masse
   inférieure à m, faire sortir **toute** sa masse à λ et ne pas le visiter.
2. S'il ne reste aucun enfant admissible, terminer le cluster courant.
3. S'il en reste exactement un, conserver l'identité du cluster courant et
   continuer dans cet enfant ; aucune nouvelle naissance condensée.
4. S'il en reste au moins deux, terminer le cluster courant à λ et créer
   les clusters enfants admissibles à λ avec leurs masses respectives.

Dans l'arbre à facettes, les feuilles directement déposées à cet événement
sont toutes petites et subissent exactement l'étape 1. Les masses des
autres sous-arbres sont inchangées par regroupement. Le nombre d'enfants
admissibles, les continuations et les naissances sont donc les mêmes.
L'induction depuis la racine donne la même arborescence condensée, à
renommage canonique près, et les mêmes masses sortantes aux mêmes dates.

La stabilité étant linéaire dans la masse sortante à date donnée, les
stabilités sont identiques. Une même récurrence EOM, avec exclusion de la
racine et même choix parent/enfants en cas d'égalité, donne la même sélection.
La borne sur les masses individuelles évite ici les durées terminales
infinies des feuilles virtuelles ; ce raisonnement ne les rend pas valides
pour des seuils plus petits.

Les facettes d'un même e ont le même parent de sortie condensée et héritent
du même label sélectionné, y compris si leur sous-arbre est éliminé en
amont. Le vote d'un cluster est donc la somme des A_e,x qui portent ce label,
divisée par T_x. Ignorer les dépôts étiquetés bruit sans renormaliser les
autres votes reproduit la règle de la référence.

Les numéros arbitraires de clusters ne sont pas une identité mathématique.
Pour reproduire les égalités du vote actuel « plus petit label », conserver
le même classement des groupes, par exemple via leur plus petite facette
lexicographique. Ce minimum peut être agrégé sans garder toutes les facettes.
Changer l'ordre des labels peut changer l'affectation d'un point exactement
à égalité, même si les masses, les groupes et les votes sont corrects.

La suffisance annoncée porte sur cette condensation et ce **vote plat**.
Elle ne transfère pas automatiquement le routage futur du plan ponctuel :
comparer un enfant géométrique à chaque facette terminale n'est pas comparer
cet enfant à la somme de toutes ces facettes. Ce dernier changement pourrait
modifier le chemin choisi. Définir et qualifier ce routage séparément, sans
faire passer le dépôt pour une branche dans l'une ou l'autre opération.

## 4. Éliminer Sτ après échange des sommes

Avec ψσ=rσ^(-z), chaque point x d'une coface σ appartient exactement à K
de ses K+1 facettes. D'où, pour le catalogue complet sans doublons :

$$T_x=K\sum_{\sigma\in C,\,x\in\sigma}\psi_{\sigma}.$$

Les numérateurs des dépôts peuvent aussi être calculés directement :

$$A_{e,x}=\sum_{\sigma\in C}\psi_{\sigma}\,\#\{\tau\subset\sigma:|\tau|=K,\ x\in\tau,\ e(\tau)=e\}.$$

Un calcul de T_x puis une accumulation sparse de A_e,x permet donc de
déduire v et m_e sans conserver les Sτ. On peut aussi accumuler T et A
pendant un même flux puis normaliser à la fin. Chaque incidence
coface–facette compte une fois : la répétition d'une facette dans plusieurs
cofaces est une contribution légitime, pas une coface dupliquée à tolérer.

Pour éviter d'émettre chaque triplet (σ,τ,x), grouper déjà la frontière de
σ par événement. Poser qσ,e le nombre de ses facettes attachées à e :

$$A_{e,x}\mathrel{+}=\psi_{\sigma}\left(q_{\sigma,e}-\mathbf{1}\{e(\sigma\setminus\{x\})=e\}\right),\qquad x\in\sigma.$$

La seule facette de σ qui ne contient pas x est σ privé de x, ce qui prouve
la formule. Si toutes ses facettes ont le même événement, chaque point de
σ reçoit Kψσ dans cet événement. Plus généralement, la dimension trois
donne une borne uniforme bien meilleure que K+1 événements.

### Au plus cinq événements par coface en dimension trois

Choisir un support de la miniball Q **inclus dans σ**, de cardinal au plus
quatre ; son existence découle de la caractérisation du centre par
l'enveloppe convexe des points de contact et de Carathéodory en dimension
trois. Il ne faut pas choisir un support arbitraire de la coquille globale
qui contiendrait des points absents de σ.

Pour tout x dans σ hors Q, on a Q⊂σ privé de x⊂σ. La monotonie du rayon
MEB et l'égalité MEB(Q)=MEB(σ) imposent le même rayon pour la facette
σ privée de x ; l'unicité de la miniball donne aussi le même centre.
À cette date βσ, la coface σ relie toutes ses facettes dans la composante
fermée de FULL. Toutes ces suppressions hors Q ont donc à la fois la même
naissance βσ et le même nœud fermé normalisé : un événement e commun.

Seules les suppressions des points de Q peuvent donner d'autres événements :

$$g_{\sigma}\leq\min(K+1,|Q|+1)\leq5.$$

Si Q=σ, il n'y a pas de suppression hors Q, et la borne K+1 suffit.
Un support plus petit améliore la borne ; les exceptions peuvent aussi
coïncider entre elles. Ni les plateaux ni une coquille non régulière ne
modifient la preuve. La convention fermée et le choix Q⊂σ sont essentiels.
Ce raisonnement vaut pour la connectivité Čech/FULL, pas pour le graphe
incomplet des seules incidences Gabriel.

Après connaissance des attaches, calculer les qσ,e puis la formule précédente
coûte O(K) mises à jour par coface en dimension trois, avec au plus cinq
événements. Ce résultat borne l'accumulation combinatoire, pas le coût de
calcul des bonnes attaches ni la taille C du catalogue.

Cela n'autorise ni à utiliser e(σ) pour toutes ses facettes, ni à prendre
leur première coface Gabriel comme naissance. La [régression E5](AUDIT_SILENT_ATTACHMENTS.md)
reste obligatoire. Le regroupement n'est correct qu'après résolution des
attaches dans FULL ; il ne fournit pas lui-même ce résolveur.

## 5. Contre-exemples et invariants à tester

Une petite qualification différentielle, si cette piste est implémentée,
doit tuer au moins les erreurs suivantes :

- **Dépôt devenu branche.** Exemple combinatoire, pas nouvelle fixture
  géométrique : m=2, cluster P né à λ=1 ; à λ=2, six atomes de masse 1/2
  sortent, tandis qu'un enfant de masse 3 reste jusqu'à λ=4. Il n'y a qu'un
  enfant admissible : P continue et sa stabilité vaut 3·1+3·3=12. Transformer
  le dépôt de masse 3 en feuille persistante crée deux enfants admissibles
  et une fausse branche de stabilité infinie.
- **Dates fusionnées.** Sur une continuation portant une masse 2 jusqu'à
  λ=6, deux dépôts de masse 1/2 sortent à λ=2 et λ=4, après une naissance
  à λ=1. La stabilité est 12 ; déplacer les deux dépôts à λ=2 donne 11,
  les déplacer à λ=4 donne 13. Un segment seul n'est pas une clé suffisante.
- **Élagage ignoré.** Le triangle K2,m2 décrit plus haut doit sortir à 8/3,
  pas à 2. Tester aussi un sous-arbre comprenant plusieurs dates internes,
  toutes supprimées par le même rejet en amont.
- **Seuil illégitime.** Si un atome individuel atteint m, sa suppression
  obligatoire n'est plus équivalente à la référence ; refuser le profil
  ou garder cette branche, sans invoquer seulement la masse moyenne.
- **Mauvaise topologie.** E5, plateaux fermés et naissances silencieuses
  doivent conserver leurs partitions de facettes indépendamment des votes.
- **Réindexage ou arrondi.** Tester permutations, ordre des contributions,
  égalités de votes, masse au seuil et stabilité proche de l'égalité EOM.

Comparer les groupes condensés par leur provenance, leurs dates et masses,
pas seulement les labels finaux. Vérifier conservation par événement,
par point, par sous-arbre et pour chaque cluster condensé ; racine exclue
des sélections des deux bras. Une forêt inattendue ne reçoit pas une
super-racine destinée à faire passer le test.

## 6. L'algèbre exacte ne qualifie pas les arrondis actuels

Le module gelé arrondit chaque mτ en binary64 puis additionne exactement
les valeurs dyadiques de ces entrées. Calculer directement m_e à partir
des A_e,x ne reproduit pas nécessairement ces arrondis individuels.
Même avec une masse groupée identique, multiplier une fois m_e par une
durée ne reproduit pas toujours la somme des produits flottants par facette.
Le regroupement de Sτ vers T_x ou vers les votes peut lui aussi modifier
les dernières unités et les égalités.

Trois contrats possibles doivent rester distincts :

- référence rationnelle z2 : équivalence algébrique exacte sur petits cas,
  avec coût arithmétique et dénominateurs explicitement payés ;
- conservation de la sémantique binary64 précédente : préserver les arrondis
  nécessaires, ce qui peut imposer un passage transitoire par les facettes ;
- nouveau calcul regroupé binary64 : vérifier erreurs/marges et désaccords
  contre la référence, sans revendiquer automatiquement des labels identiques.

Une égalité bit à bit sur quelques cas n'est pas une preuve générale. La
géométrie rationnelle certifie les événements, pas les décisions de masse,
les stabilités ni l'argmax flottant. Aucune qualification antérieure du
module à facettes n'est transférée implicitement au futur module à dépôts.

## 7. Coûts et portée d'une éventuelle réalisation

Noter D le nombre de couples (segment,β), J le nombre de couples (e,x)
non nuls, C le nombre de cofaces, F celui des facettes et V la taille de
FULL. On a D≤F et J≤K·F, sans garantie que ces inégalités soient strictes.
Les données persistantes deviennent O(V+D+J+n) hors preuves géométriques,
au lieu d'une exportation répétée des facettes et de leurs sorties EOM.

Après préparation, la condensation peut viser un parcours O(V+D), hors
tris, index et coût numérique, si les masses des sous-arbres sont connues.
Quand un petit sous-arbre est rejeté, sa stabilité est payée avec sa masse
totale ; son propriétaire de sortie peut être propagé une seule fois aux
dépôts. Ne pas rescanner tous ses descendants depuis plusieurs ancêtres.
Le vote plat paie J incidences et les sommes par cluster réellement présent.
Changer m peut réutiliser les mêmes dépôts ; changer z exige les statistiques
supplémentaires déjà décrites dans le plan, ou une nouvelle agrégation.

En amont, les attaches des facettes ne disparaissent pas algébriquement.
Un cache dédupliqué coûte encore O(F), mais peut rester transitoire, natif,
sans export Python répété. Sans cache, on peut recalculer la même attache
à chaque incidence : jusqu'à (K+1)C résolutions, dont le coût doit être payé.
Un tri externe, un flux canonique ou une autre compression sont des pistes,
pas des mécanismes déjà livrés.

L'accumulation directe naïve coûte O(K²C) mises à jour. Le regroupement
par coface proposé plus haut coûte O(Σσ (K+1)gσ). La borne gσ≤5 prouvée
en dimension trois donne donc O(K·C), en plus de la résolution des attaches
et des coûts numériques/dictionnaires. Le nombre de couples (e,x) produits
est au plus cette somme ; aucun terme n'impose que C soit petit en n.
Le terme K²C compare deux écritures de la formule directe, **pas** les
additions du module actuel : celui-ci agrège déjà Sτ en O(K·C), puis
parcourt K·F incidences. Le gain visé ici est d'éviter les atomes persistants
et leurs sorties répétées, pas d'attribuer au code existant une expansion
arithmétique qu'il ne fait pas. La construction des tuples, les tris et
la matérialisation Python doivent être mesurés séparément.

Le [comptage désormais capturé et rejouable](DEPOTS_MESURES.md) de
G2,δ8,s1,n1200,z1 donne F=108 909 vers D=55 915 et K·F=544 545 vers
J=309 138 à K5 ; F=689 531 vers D=174 331 et K·F=6 895 310 vers
J=1 828 667 à K10. La note liée fournit le script épinglé, les deux rapports
privés et leurs hashes, la fermeture des six fichiers consommés et les
contrôles combinatoires. Ce sont deux unités closes, **pas** la campagne
complète ni une qualification d'EOM à dépôts ou un gain de temps.
Les masses maximales des dépôts y sont inférieures à un : ces seuls cas
ne testent donc pas l'erreur de promotion d'un dépôt de masse au moins m.

La première étape de comptage est ainsi disponible. Une suite éventuelle
consisterait à confronter un petit prototype aux références de facettes,
sans héritage implicite des garanties de cette seule capsule. Pas de grand port,
de nouvelle campagne GPU ni de promesse sous-quadratique ou temps réel
avant ces contrôles.
