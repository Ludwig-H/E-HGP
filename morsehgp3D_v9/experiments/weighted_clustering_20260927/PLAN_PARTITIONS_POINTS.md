# Étape suivante proposée : partitions de points emboîtées

27 septembre 2026. **Plan initial**, conservé comme spécification.
Une [référence exacte sparse](POINT_ROUTING_REFERENCE.md) est maintenant
implémentée et contrôlée sur petits arbres et trois objets FULL qualifiés.
Cela ne transforme pas ce plan en benchmark de qualité ou de performance.
Cadre expérimental v9, K fixé, z fixé, statut `not_claimed`. Aucun changement
du moteur ou des captures gelées n'est requis par cette note. Les scores du
pilote en cours ne serviront pas à choisir rétrospectivement le routage.

La [correction des attaches silencieuses](AUDIT_SILENT_ATTACHMENTS.md) reste
le préalable : les masses proviennent des cofaces Gabriel, mais les facettes
suivent la connectivité Čech / FULL correcte. Ni le graphe des seules cofaces
Gabriel ni les seules unions de points des composantes ne suffisent.

## 1. Données suffisantes après agrégation, pour K et z fixés

Noter C le catalogue dédupliqué des cofaces Gabriel contributrices et F sa
frontière de K-facettes. Les rayons des cofaces contributrices sont positifs
sur les sites distincts du profil actuel. Pour chaque facette τ :

$$S_{\tau}(z)=\sum_{\sigma\in C,\,\tau\subset\sigma}r_{\sigma}^{-z},\qquad T_x(z)=\sum_{\tau\in F,\,x\in\tau}S_{\tau}(z),\qquad m_{\tau}(z)=\sum_{x\in\tau}\frac{S_{\tau}(z)}{T_x(z)}.$$

Un supplément suffisant au squelette daté T_K comporte :

- un identifiant de facette et ses **K PointId**, pas seulement son numéro ;
- son score agrégé Sτ(z), avec le contrat arithmétique employé ;
- sa naissance MEB exacte βτ, en rayon carré et dans l'unité déclarée ;
- son segment de T_K, normalisé dans l'état fermé à βτ ;
- l'univers des points, K, z, les unités et les identités/hashes de la source.

Le squelette conserve les niveaux exacts, parents/successeurs et racines.
Les naissances silencieuses subdivisent ses segments avant le traitement
pondéré. La première incidence Gabriel ne remplace jamais βτ. Les données
de résolution des ancres restent des preuves/provenances distinctes.

T_x et mτ se déduisent en deux parcours des incidences point–facette.
Si T_x=0, le point n'a pas de masse représentée : le marquer explicitement,
sans fabriquer de division ni de facette. La somme des masses est le nombre
de points dont T_x est positif, et chaque masse de facette est au plus un.

Après cette agrégation, **C n'est plus nécessaire pour rejouer** les masses,
la condensation, EOM et les votes à ce z. Cela ne permet pas de revalider
indépendamment la complétude de C ou le calcul initial de Sτ à partir du seul
supplément. Conserver les preuves de capture ; ne pas appeler T_K nu un
résumé suffisant de la mesure. Les masses mτ seules ne permettent pas non
plus de retrouver les votes par point.

### Changer z exige davantage d'information

Un unique Sτ(z₀) ne détermine pas Sτ(z) pour un z libre. Il faut conserver
les rayons contributifs et leurs multiplicités, éventuellement agrégés en
un histogramme exact par facette :

$$H_{\tau}(\beta)=\#\{\sigma\in C:\tau\subset\sigma,\ r_{\sigma}^{2}=\beta\},\qquad S_{\tau}(z)=\sum_{\beta}H_{\tau}(\beta)\beta^{-z/2}.$$

Les identités complètes des cofaces ne sont pas nécessaires à ce seul
recalcul si cet histogramme est fidèle ; elles le restent pour d'autres
requêtes géométriques ou de provenance. Une liste finie de z préannoncés
peut utiliser un vecteur de scores, pas prétendre couvrir tous les z.

Multiplier tous les scores par une même constante positive ne change ni
les masses ni l'argmax d'un vote. À z fixé, une normalisation globale
positive de λ multiplie toutes les stabilités par la même constante et ne
change pas EOM en arithmétique exacte. En revanche, passer de z1 à z2 change
**la mesure et λ**, pas seulement l'échelle. La géométrie rationnelle ne
certifie pas automatiquement les sommes, comparaisons et stabilités z1
en binary64 ; les cas proches ou égaux restent déclarés.

## 2. Un routage cohérent, distinct du vote plat

La proposition porte d'abord sur l'arbre augmenté **avant condensation**,
avec les attaches correctes et les plateaux atomiques. La condensation et
EOM pondérés gardent leurs masses de facettes : aucun remplacement implicite
par le nombre de points finalement affectés.

Pour un point x et un nœud v, le vote de sous-arbre utilise les facettes
descendantes de v, et non une multiplication par leurs masses :

$$A_x(v)=\sum_{\tau\in F,\,\tau\preceq v,\,x\in\tau}S_{\tau}(z).$$

Les enfants forment des sous-arbres disjoints. À la racine, leurs votes
incluent tous les descendants, puis sont restreints à la branche retenue.
Diviser par T_x ne change pas leur classement : comparer les sommes avant
division évite des égalités artificielles supplémentaires.

Le routage est calculé **une fois pour chaque K,z**, indépendamment de la
coupe demandée et du seuil de condensation :

1. Affecter chaque point à une seule vraie racine du squelette. Avec une
   racine unique et T_x>0, cette étape est immédiate. Dans une forêt, comparer
   les votes globaux des racines une seule fois. Une égalité non résolue
   laisse le point non affecté ; ne pas inventer de fusion entre racines.
2. Depuis un parent déjà choisi, descendre exclusivement vers l'enfant au
   plus grand vote positif. Le point n'entre jamais dans un frère ensuite.
3. En cas d'égalité exacte, ou d'ambiguïté numérique déclarée, arrêter
   conservativement le point au parent. Un départage par ID serait un autre
   contrat, non canonique et potentiellement sensible au réindexage.
4. Arrêter également à la feuille-facette choisie. Conserver le nœud d'arrêt
   et la raison ; la chaîne d'ancêtres est implicite dans l'arbre commun.

Ce choix parent→enfant ne maximise pas nécessairement le score du meilleur
descendant global. C'est une décision statistique explicite ; ce n'est ni
une nouvelle conséquence de la proposition 7, ni une promesse de meilleur
ARI. Les sommes exactes z2 peuvent servir de juge sur petits cas ; une
politique binary64 z1 doit annoncer ses seuils d'ambiguïté et ses limites.

Revoter séparément entre les composantes de chaque coupe autorise un point
à changer de branche. Revoter entre les racines **sélectionnées par EOM**
à chaque seuil m a le même problème. Ces votes plats produisent une
partition pour une sélection donnée, pas une famille emboîtée. L'exclusivité
doit être globale dès les vraies racines et maintenue à chaque descente.

## 3. Résidus, dates, bruit et sens exact de l'emboîtement

Le résultat complet peut être représenté par un arbre/une forêt de points
ayant n feuilles singleton, de rayon zéro, avec une règle de rattachement
explicite :

- si le routage s'arrête à un nœud interne v, rattacher la feuille du point
  à la date exacte de v ; à des rayons inférieurs, elle reste singleton ;
- s'il atteint une facette τ, rattacher à βτ, sa naissance MEB, pas au rayon
  virtuel zéro utilisé séparément par l'adaptateur EOM des facettes ;
- plusieurs résidus au même nœud restent des singletons séparés avant
  cette date, jamais un groupe artificiel « résidu » anticipé ;
- les points sans racine attribuée ou avec T_x=0 restent des singletons
  extérieurs. Leur éventuelle étiquette -1 signifie non affecté, pas qu'ils
  forment ensemble un cluster géométrique.

Le nœud d'arrêt peut retarder un point par rapport à sa première couverture
géométrique, notamment lors d'une égalité. Ne pas présenter ses dates comme
des premières naissances natives ni ses blocs comme toutes les composantes
de L_K(r). C'est une nouvelle projection exclusive, construite sur FULL.

À toute coupe, prendre les blocs de cet arbre daté et les singletons
extérieurs donne une partition de tous les points. Quand le rayon augmente,
seules des unions sont possibles ; quand λ augmente, seules des subdivisions
sont possibles. La preuve est structurelle : une feuille a un parent unique,
chaque bloc est l'union disjointe de ses descendants et les niveaux sont
monotones. Les événements de même niveau sont traités atomiquement.

On peut supprimer les nœuds sans point ou unaires, en conservant les dates
des fusions restantes et le raccord aux identifiants source. Il ne faut pas
supprimer les singletons résiduels pour ensuite revendiquer une partition
totale. Une sortie qui les masque est une partition **partielle**, assortie
du masque de non-affectation.

EOM fournit ensuite une sélection d'antichaîne dans l'arbre pondéré de
facettes ; les points dont le chemin exclusif ne traverse aucun groupe
sélectionné sont du bruit pour cette sortie. Exclure la racine d'EOM ne
signifie pas effacer la racine géométrique de la hiérarchie. Les cardinalités
après routage ne sont pas garanties supérieures au seuil massique m.

La garantie d'emboîtement concerne les **coupes d'un même arbre à K,z fixés**.
Elle ne compare ni différents K, ni z1/z2, ni les partitions plates EOM pour
différents m. Un routage sur l'arbre déjà condensé serait une variante
dépendante de m, à spécifier séparément avec ses sorties de points datées ;
réattacher sans date tous les points à leur cluster de sortie ne suffit pas.

## 4. Coûts à compter, sans tableau dense point × nœud

Noter F le nombre de facettes, V le nombre de nœuds du squelette augmenté,
I=K·F le nombre d'incidences et d_x le nombre de facettes incidentes à x.
L'agrégation de T_x et des masses prend O(I+n) après calcul des scores.
Stocker le supplément coûte O(I+F+V+n), sans prétendre borner F ou V par n.
L'énumération initiale des cofaces et les attaches restent des coûts séparés.

Il faut éviter deux implémentations apparemment simples : un tableau de
n·V votes, et une montée indépendante de chaque incidence vers tous ses
ancêtres. Cette dernière peut coûter I fois la profondeur ; la profondeur
n'est pas automatiquement logarithmique. Un routage naïf qui inspecte tous
les enfants pour chaque point peut aussi recréer n·V sur une multifusion.

Deux pistes bornées à examiner sur un petit prototype seulement :

- ordonner les feuilles-facettes en DFS ; pour chaque point, conserver ses
  incidences triées et leurs sommes préfixes. Le score d'un sous-arbre est
  une requête d'intervalle sparse, sans tableau de tous les votes ;
- construire pour chaque point le petit arbre virtuel des facettes
  incidentes et de leurs LCA. Il a O(d_x) nœuds après compression des chemins
  sans branche positive. Les sommes ascendantes permettent de comparer
  seulement les enfants ayant un vote non nul et de sauter ces chemins.

Avec un index LCA standard, une cible de coût est le prétraitement de
l'arbre plus la somme des tris/incidences et requêtes LCA, par exemple
O(V log V + I log V + Σ_x d_x log d_x + n). C'est une borne de conception
conditionnelle pour cette représentation, **pas celle du code actuel**.
La mémoire d'un index LCA O(V log V) doit être payée ou remplacée par un
index plus compact. Aucun de ces termes ne garantit un coût sous-quadratique
en n puisque I et V sont eux-mêmes des sorties géométriques.

Conserver un seul rattachement terminal par point, plus l'arbre partagé,
évite de matérialiser n chemins complets. Si l'utilisateur demande tous les
ensembles de descendants ou toutes les coupes, facturer leur volume de
sortie : ce volume peut lui-même être quadratique. Les recherches doivent
être sensibles aux incidences réellement visitées et à la sortie demandée,
pas cacher un parcours dense derrière un cache.

## 5. Conditions minimales avant une éventuelle réalisation

Commencer, seulement après décision de poursuivre, par une référence sur
petits arbres et nuages ; pas de nouvelle campagne LiDAR ou G4 annoncée.
Les contrôles nécessaires sont :

- identité des masses/votes à partir du supplément et du catalogue complet,
  et refus d'un changement de z sans statistiques suffisantes ;
- E5 et l'attache silencieuse à 33/2, naissance isolée du triangle,
  plateaux, permutation des PointId et réordonnancement des enfants ;
- exclusivité aux racines et à chaque niveau, singletons résiduels,
  points non représentés, égalités et ambiguïtés numériques ;
- raffinement exact des coupes ouvertes/fermées et conservation des dates
  après suppression des nœuds vides/unaires ;
- comparaison sparse/dense uniquement sur petits cas, avec compteurs
  d'incidences, LCA, chemins comprimés, mémoire et volume de sortie ;
- distinction publiée entre vote plat du §9.1, ce routage cohérent et une
  éventuelle nouvelle condensation à masses ponctuelles unitaires.

La garantie à viser est une construction reproductible de partitions
emboîtées avec les choix ci-dessus. La pertinence statistique, l'efficacité
sur grand nuage et tout avantage face à HDBSCAN resteraient à évaluer
séparément, sans réglage sur la vérité terrain de l'évaluation.
