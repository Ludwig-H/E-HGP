# L'algorithme entier, expliqué simplement

13 septembre 2026. Ce texte décrit la cible mathématique et les mécanismes
de la v7, puis leur organisation souhaitée en v8. Il ne décrit pas un moteur
v8 déjà implémenté. Les détails de preuve sont dans
[l'audit mathématique](../audits/FONDEMENTS_ET_OBJET.md).

## 1. Ce que l'on veut obtenir

À un rayon donné, imaginons déplacer une boule dans le nuage. On garde les
positions où elle contient au moins K points. Ces positions forment des
régions connexes. En augmentant le rayon, des régions apparaissent et
fusionnent : c'est la hiérarchie de densité K-NN recherchée.

À K=1, on retrouve le single-linkage : les points se raccordent en augmentant
la distance autorisée. À K=2, les éléments à relier sont des paires de points,
pas simplement les points. Un même point peut appartenir à plusieurs groupes
sans que ces groupes doivent fusionner. C'est une propriété voulue, pas un bug.

« Toute la tour jusqu'à 10 » signifie les dix hiérarchies, ainsi que les
applications qui les relient aux mêmes rayons. Dix résultats indépendants
sans ces correspondances ne suffisent pas au contrat actuel. Il faut aussi
conserver les éléments isolés, les naissances et les vrais parents des fusions.

## 2. Pourquoi ne pas construire tous les simplexes ?

Un simplexe désigne ici un ensemble de points. Il existe énormément de
sous-ensembles de K points : les énumérer tous serait prohibitif. Le manuscrit
donne une condition géométrique utile : une connexion qui fusionne réellement
des groupes doit être Gabriel, sous les hypothèses régulières déclarées.

Un ensemble est Gabriel lorsque sa plus petite boule englobante ne contient
pas de point extérieur à cet ensemble dans son intérieur. On cherche donc
ces événements géométriques particuliers, et non la totalité du complexe de
Čech ou d'une mosaïque de Delaunay d'ordre élevé.

Mais conserver seulement les connexions Gabriel brutes est faux : une
connexion silencieuse peut avoir déjà rattaché une paire à un groupe, et ce
rattachement devient indispensable lorsqu'un événement ultérieur la réutilise.
On peut éviter de publier ce chemin silencieux ; on ne peut pas oublier son
résultat. C'est précisément ce qui explique une partie du coût de construction.

## 3. La chaîne complète

```text
points
  → index spatial
  → rectangles WSPD et élimination par témoins
  → supports candidats à 2, 3 ou 4 points
  → géométrie exacte et points dans chaque boule
  → catalogue partagé entre les ordres K
  → rattachement des facettes réutilisées
  → graphes datés sur les naissances
  → histoires des multifusions
  → contributions et correspondances entre ordres
  → sortie vérifiable et publiée
```

Ce schéma résume les rôles, pas un exécutable v7 unique : le programme
principal, la sonde FULL et les derniers prototypes n'intègrent pas tous
les mêmes étapes. Voir [la carte d'implémentation](../audits/IMPLEMENTATION_PARALLELISATION.md).

## 4. Index et WSPD : éliminer des familles de paires

L'index range les points spatialement et associe des boîtes à des groupes.
La WSPD représente les paires de points par des rectangles A×B : toutes les
paires a,b avec a dans A et b dans B. Les deux facteurs sont assez éloignés
l'un de l'autre par rapport à leur diamètre. Cela permet de raisonner sur
tout un rectangle, avant d'examiner ses paires une à une.

Un rectangle n'est pas un tableau de toutes ses paires : on doit le garder
implicite aussi longtemps que possible. Une fois la décomposition obtenue,
ses rectangles sont bien des unités indépendantes de calcul géométrique.
La v7 utilise déjà cette indépendance sur CPU. Ce qui manque est une
granularité suffisante *à l'intérieur* des gros rectangles, et une réduction
du travail total qu'ils déclenchent.

Le paramètre s règle la séparation. Un s plus élevé donne des facteurs plus
petits relativement à leur écart : les garanties par boîte peuvent être
plus précises. En contrepartie, il peut produire davantage de rectangles et
de travail de décomposition. « Plus grand » n'est donc pas synonyme de
« plus rapide ». Les expériences doivent comparer s=8,10,12, par étape et
par famille de nuages, sans déplacer le problème vers un autre poste.

## 5. Pourquoi seulement q2, q3 et q4 ?

En dimension trois, une plus petite boule de rayon positif sur des positions
distinctes est déterminée par au plus quatre points de son bord : une paire,
un triangle ou un tétraèdre de support. Les singletons sont traités à part.
La boule peut pourtant contenir beaucoup plus de points. K=10 ne signifie
donc pas qu'il faut un support géométrique à dix points.

| Symbole | Signification | Exemple pour la cible principale |
| --- | --- | --- |
| K | Ordre d'une hiérarchie | De 1 à 10 |
| Kmax | Dernier ordre demandé | 10 |
| q | Taille du support géométrique | 2, 3 ou 4 |
| smax | Fenêtre de cardinalité examinée | Kmax+1, donc 11 |
| s | Séparation de la WSPD | 8, 10 ou 12 |

Deux tâches différentes doivent être distinguées : trouver ces supports
candidats dans tout le nuage, puis recalculer une petite boule lors du
rattachement d'une facette donnée. La v7 paie les deux ; accélérer seulement
la première ne supprime pas les millions de calculs de la seconde.

## 6. q2 : le rôle exact des témoins

Pour une paire a,b, la boule de support q2 est la boule de diamètre ab.
Si l'on prouve que beaucoup d'autres points sont à l'intérieur de toutes
les boules d'un rectangle, aucune de ces paires ne peut produire une boule
dans la petite fenêtre de cardinalité recherchée. On rejette alors le
rectangle sans développer A×B.

Le premier filtre cherche des témoins communs hors de A et B. S'il est
insuffisant, on peut ajouter les témoins de A autres que a, valables pour
tous les b, et symétriquement ceux de B autres que b. Ces trois populations
doivent être disjointes. Les boîtes donnent des garanties suffisantes,
parfois conservatrices ; un échec du filtre ne prouve pas que la paire est
utile. Le census exact décide ensuite.

Pour toute la tour 1..Kmax, la fenêtre demande p+q_min≤Kmax+1, même avec
une coquille supplémentaire. Ici p est le nombre de points strictement
intérieurs et q_min la taille minimale d'un support positif, pas le nombre
total de points sur la coquille. Pour une présentation de support q, les
seuils de génération sont Kmax+2−q témoins distincts :

| Tour | q2 | q3 | q4 |
| --- | ---: | ---: | ---: |
| 1..10 | 10 | 9 | 8 |
| 1..5 | 5 | 4 | 3 |

Votre seuil h=K est donc bien le seuil q2 lorsque K est le dernier ordre
de la tour. En q3/q4, on peut arrêter plus tôt. Le coût difficile n'est
pas la dernière comparaison au seuil : c'est parfois de trouver et de
certifier ces témoins. La v7 calcule encore certains tableaux h_a/h_b
avec des doubles boucles ; cela n'est pas imposé par la définition HGP.

## 7. q3 et q4 : indépendants, mais pas de même coût

Une paire canonique sert d'ancre pour énumérer les complétions. Un fuseau
plus fin — le « citron » — fournit des témoins qui restent intérieurs à
toutes les boules admissibles de cette famille. Un point hors de ce fuseau
peut néanmoins être intérieur à certaines boules : le filtre est suffisant,
pas exhaustif.

En q3, il faut trouver et tester les troisièmes points candidats. En q4,
la v7 évite certains rescans par un balayage des positions où les points
entrent ou sortent des boules le long d'une corde. Les égalités exactes de
positions doivent rester groupées : casser ces groupes changerait la
profondeur comptée.

Ces tâches peuvent être parallélisées. Mais un rectangle peut contenir
beaucoup de paires, une paire beaucoup de triangles, et un triangle beaucoup
d'événements q4. Une seule tâche GPU pour chacun de quelques gros rectangles
laisserait une longue fin de calcul mal distribuée. Il faut exposer les
paires, les triangles et les segments d'événements comme sous-tâches, avec
comptages puis destinations préfixées. Le [rapport WSPD](../audits/WSPD_Q2_Q3_Q4.md)
détaille les conditions permettant de le faire sans matérialiser tout A×B.

## 8. Census : quelles boules avons-nous vraiment trouvées ?

Les candidats sont canonisés, triés et dédoublonnés. Pour chaque boule, le
census vérifie la géométrie et trouve les points strictement intérieurs
et ceux sur la coquille. Une même boule doit être conservée une seule fois
et servir à plusieurs ordres de la tour.

Sur un nuage régulier, ses rôles sont particulièrement simples : naissance
à un ordre et connexion à l'ordre voisin, lorsque ces ordres appartiennent
à la tour demandée. Avec des points supplémentaires
sur la coquille, il faut un quotient local et des contributions datées.
Un grand plateau ne doit pas être tronqué ou artificiellement séparé par
des arrondis. Les limites de format de la v7 ne deviennent pas des théorèmes
géométriques pour la v8.

## 9. Le poste cher que le mot « Kruskal » cache

Une boule de connexion réutilise des facettes. Avant de relier leurs
composantes, il faut savoir où ces facettes sont déjà rattachées. La v7
calcule leur petite boule, cherche un intrus, échange un point de support,
puis recommence jusqu'à un terminal certifié. Des caches et des semis
réduisent le travail, sans l'annuler.

Le prototype plus récent rassemble les demandes géométriques identiques,
les résout par fenêtres indépendantes, puis remet leurs réponses dans
l'ordre utile. Le résultat est un graphe daté sur les naissances.
C'est **après** cette découverte coûteuse qu'une forêt couvrante et son
histoire peuvent être calculées. Trier des identifiants ou appliquer un
algorithme rapide de forêt ne découvre pas gratuitement ces rattachements.

## 10. Des arêtes triées à une vraie histoire

Le tri ordonne les connexions. Kruskal maintient en plus les composantes
et retient celles qui les réunissent. Construire l'histoire ajoute encore
les identités des nœuds et leurs parents. Toutes les connexions de même
date doivent former de vraies multifusions, sans succession binaire
arbitraire visible dans la sortie.

Le calendrier séquentiel n'est pas une obligation mathématique. À partir
d'une forêt pondérée correcte, des algorithmes de contraction d'arbres
permettent un calcul parallèle. Ils devront être raccordés aux naissances,
plateaux et marques HGP, plutôt qu'importés comme si une hiérarchie binaire
ordinaire couvrait déjà tout le contrat.

## 11. Les verticales et l'export ne sont pas gratuits

Une naissance d'ordre K≥2 possède une référence à la composante d'ordre K−1
à la même date fermée. Les autres images se déduisent des histoires
inférieures ; ce ne sont pas de nouvelles recherches de MEB. Les requêtes
sont indépendantes lorsque ces histoires et leurs index sont disponibles.

Il reste à numéroter les nœuds, écrire les parents, partager les populations,
associer les contributions et publier un objet complet. La v7 évite déjà
de stocker un ensemble complet de points par nœud. Mais elle construit
encore beaucoup de tableaux intermédiaires et n'a pas intégré une archive
industrielle FULL avec reprise de calcul.

## 12. La règle de conception v8

Réduire les objets inutiles ; rendre chaque étape restante indépendante
autant que possible ; garder ses données résidentes ; compter ses octets,
ses visites et ses calculs exacts ; mesurer enfin la tour complète.

Ce n'est ni « tout se résume à un tri », ni « l'exactitude empêche le GPU ».
Le défi est de produire les bons objets avec peu de travail, puis de
distribuer ce travail à une granularité suffisante. Les propositions
concrètes et leurs preuves encore nécessaires sont dans
[le plan de refonte](PLAN_DE_REFONTE.md).
