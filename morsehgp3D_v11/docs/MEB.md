# MEB bornée et raccord au census global

Cadre : `exploration_v11_hors_registre`, `cpu_reference`,
`quantized_u21_input_only`, `implementation_v11_meb`, `not_claimed`.
Construction neuve dans `src/tower`, fondée sur M1/M2 de
[MATHEMATIQUES](MATHEMATIQUES.md). La version exhaustive historique est
qualifiée sur G4 à `25792084e` :
1266/1266 portes, complément ASan18 55/55 et 18/18 essais aux trois profils.
[Preuves et limites](../receipts/meb_20261002/README.md).
L'arrêt au premier certificat décrit ci-dessous est une nouvelle tranche,
préparée après `9c883b93f` ; sa qualification native G4 reste à effectuer.
La campagne parallèle de cette source antérieure ne qualifie pas cet arrêt.
Cette brique ne construit pas encore la forêt FULL.

## Contrat et preuve

`bounded_meb(cloud, part)` accepte de 1 à 12 SiteIdx distincts du même Cloud.
Il copie puis trie ces indices et leurs coordonnées dans des tableaux fixes.
Une partie vide, un Cloud vide, un cardinal supérieur à 12, un indice hors
domaine ou répété est refusé ; rien n'est supprimé silencieusement.
Les indices sont les rangs Morton, pas les PointId externes.

La recherche examine les sous-parties dans l'ordre cardinal 1 à 4 puis
lexicographique des SiteIdx croissants, jusqu'au premier certificat. Une
circonsphère affinement dégénérée est écartée. Un triangle doit être strictement
aigu et un tétraèdre contenir strictement son centre. Les couples distincts
et les singletons portent déjà leur centre dans leur intérieur relatif.
Chaque candidat positif est testé contre **toute la partie**, avec arrêt au
premier site extérieur ; les contacts sont inclus. Dès qu'un candidat strict
contient toute la partie, les combinaisons restantes et les arités suivantes
ne sont plus examinées. Aucune comparaison de niveaux n'est nécessaire.

M1 garantit qu'un support strict minimal, affinement indépendant, de taille
au plus quatre existe dans la partie. Il est donc examiné. Toute boule
contenant la partie et portée positivement par un de ses supports est déjà
sa MEB, unique par M1 : si son centre est une combinaison convexe des sites
de frontière, la moyenne pondérée des distances carrées à tout autre centre
est son rayon carré plus le carré du déplacement du centre. Aucun autre
centre ne réduit donc le maximum. Tous les supports stricts contenants
portent cette même boule. L'ordre de visite garantit que le premier a le
cardinal minimal puis le plus petit tuple local ; arrêter conserve ainsi
la Sphere et le support canonique de la recherche exhaustive précédente.
Une positivité fausse pour un
triplet ne coupe **jamais** ses prolongements tétraédriques : les arités
sont énumérées indépendamment.

La Sphere retournée est reconstruite par la présentation positive gagnante.
Son support est **local à la partie**. Il n'est ni le support global S* du
catalogue, ni une clé d'identité géométrique. La fixture
`[(1,2,0),(0,5,0),(8,1,0),(8,9,0)]` a centre `(5,5,0)` et niveau 25 :
le triplet `(0,1,2)` est une présentation minimale avec un poids négatif,
mais le support strict local choisi est `(0,2,3)`. Ajouter `(9,8,0)` au
Cloud donne un support global de cardinal 2 sans changer cette MEB locale.

## Propriété, mémoire et domaine

Le résultat possède la Sphere, quatre indices et ses compteurs. Il ne garde
aucun emprunt aux entrées ; interpréter ses indices exige le même Cloud.
Les temporaires ont une capacité constante : 12 sites, une combinaison de
quatre positions et une récursion de profondeur quatre. Aucune allocation,
liste globale de parties, cache mutable ni calcul flottant n'intervient.

Pour 12 sites, au plus 793 présentations entraînent au plus 9 516 tests de
points et aucune comparaison de niveaux. Ce coût est local ; il ne borne ni
le nombre de requêtes ni le travail d'une tour. `MebLedger` publie les
présentations, les candidats non dégénérés, positifs et contenants, les
comparaisons et les tests effectivement exécutés. Tout succès a exactement
`containing=1` et `comparisons=0`. Les autres compteurs décrivent le préfixe
parcouru, sans créditer les présentations évitées. Les prédicats numériques
existants gardent leurs budgets propres ; aucune formule de degré dix ou
nouveau type arithmétique n'est introduit.

La limite 12 couvre parties et cofaces nécessaires à une future tour K10.
Elle ne qualifie pas les cofaces de cardinal 13 d'une tour K12. Les poids
du Cloud sont ignorés par cette primitive géométrique ; la future tour doit
toujours certifier le régime unitaire et refuser les multiplicités non prises
en charge avant de construire ses cellules.

## Raccord synchrone au propriétaire global

`meb_census(index, part, threshold, budget)` calcule la MEB avec
`index.cloud()`, puis appelle le census **de ce même index**. Le seuil nul
est refusé avant la partie. Seuls les buffers du census sont alloués ; un
refus mémoire restitue les réservations de l'appel sans publier une MEB
partielle. Les résultats possèdent leurs valeurs et leurs listes.

Si la réponse est saturée, elle contient exactement threshold témoins
strictement intérieurs, sans coquille. À seuil k, leur MEB a un niveau
strictement inférieur à celui de la partie précédente (T5). La prochaine
descente pourra utiliser ces témoins sans exiger les k plus proches.
Sinon, tout I et toute U sont conservés ; la coquille n'a aucun plafond 12.
Le raccord ne choisit pas encore une trace séparable et ne résout aucune
cellule étendue, mémo daté, parent ou verticale.

Le contexte FULL commun liant index/catalogue/résultats reste à construire.
Ce raccord synchrone garantit son domaine SiteIdx par construction, sans
inventer de token global à partir d'une adresse d'objet déplaçable. Des
résultats retenus simultanément paient tous leurs buffers ; un budget privé
par fil n'est pas l'admission globale d'un lot FULL.

## Audits et protocole

Les revues indépendantes publiées à `a7a38137c` sont intégrées :
[raccord FULL](../receipts/audit_independant_20261002/full_index_descent_contract_review_10/README.md),
[capacité](../receipts/audit_independant_20261002/index_capacity_port_review_10/README.md)
et [recoupe de la campagne index](../receipts/audit_independant_20261002/index_campaign_review_10/README.md).
Elles imposent notamment le support positif local, la distinction
saturation/admission au catalogue et le relèvement du semis à sa coupe.
La [piste de bornes sur réseau entier](../receipts/audit_independant_20261002/index_lattice_bounds_review_10/README.md)
reste distincte : elle ne remplace pas le contrat continu de power_bounds
et n'est pas portée dans cette tranche.

L'oracle Gram/Gauss/Fraction conserve la minimisation exhaustive sur **toutes**
les circonsphères englobantes, sans filtrer leurs poids pour décider du minimum.
Il choisit ensuite le support strict minimal, puis additionne séparément les
coûts de son préfixe arité/lex. Les témoins fixes incluent la ligne de 12 sites
(23 présentations au lieu de 793), le cube (15 au lieu de 162), le support
local à poids négatif et le tétraèdre strict à face obtuse. Ce sont des
comptes exacts sur ces fixtures, pas des gains de temps mesurés. Les mutations
omettant chacun des deux arrêts, ou inversant l'ordre des supports, doivent
être détectées par les portes natives à venir.

Les portes utilisent aussi des tests natifs de
contrat/mémoire/concurrence et des mutants ciblés. Le banc prévu prend 48
parties locales ou dispersées par entrée entière, de tailles 1 à 12, aux
profils 18/21/24. Cloud, index, MEB, census et scan témoin ont des temps
distincts. Le scan partage num::side ; il contrôle le parcours, pas une
arithmétique indépendante. Ces parties choisies ne sont pas une descente
FULL réelle. Les événements et rapports nouveaux portent
`meb_search=first_strict_containing_v1`. Le décodeur vérifie également le
nombre exact de présentations à partir du rang combinatoire du support
dans la partie ; il refuse les événements exhaustifs historiques.
Les anciens reçus et leurs lecteurs restent inchangés. Tous les builds et
tests natifs sont réservés à G4 gardée.
