# Réponses mathématiques ciblées au plan GPU, 6 octobre 2026

Lecture au pin `cf5da0e91fb8740c1d748577fefb129c37b02d3d`. Sources et ancrages dans `sources.json`.
Cette capsule ne construit ni n'exécute le moteur, CUDA, G4 ou un oracle natif. Les graphes sont de petits
modèles exacts stdlib ; le triangle est vérifié avec `Fraction`. Aucune performance n'est mesurée.

## O7 : Kruskal, MST et plateau fermé

Pour un ordre k fixé, garder une feuille par naissance avec son rang et les étoiles de graines de chaque cellule
stricte, pondérées par le rang de cette cellule. Toutes les graines sont des naissances de rang strictement
inférieur au rang de la cellule dans une exécution FULL réussie : voie étendue `forest_plateau.cpp:54–59`
(date initiale puis naissance), voie régulière `forest_parallel.cpp:68–113` et garde redondante
`forest_plateau.cpp:111–114`. Il ne suffit pas de conserver le niveau terminal d'une descente.

Pour chaque rang λ, tout MST de ce graphe a les mêmes composantes que le graphe original restreint aux arêtes
de rang ≤ λ. Preuve : le chemin du MST entre les extrémités de toute arête e ne contient pas d'arête de rang
strictement supérieur à celui de e ; sinon la substitution par e diminue le poids du MST. L'inclusion inverse
vient de ce que les arêtes du MST appartiennent au graphe. Les étoiles remplacent exactement les hyperarêtes
des cellules pour les composantes, y compris leurs répétitions de graines.

Kruskal peut produire plusieurs fusions binaires au même rang. Contracter toutes les chaînes parent/enfant
de **nœuds de fusion** de même rang donne une fusion dont les enfants sont exactement les composantes
préplateau réunies à la coupe fermée. Préserver les naissances et supprimer les continuations sans nouvelle
fusion. Renuméroter les fusions par (rang, plus petite naissance descendante), et trier leurs enfants comme
`ForestBuilder::close`, rend la même forêt canonique. La contraction d'une arête entre rangs distincts change
la filtration : le modèle le démontre sur les niveaux 1 puis 2. Ce lemme ne permet aucun départ sur un
catalogue incomplet, ni un mélange des graphes de deux ordres k.

Le petit graphe de trois naissances et deux arêtes de rang 1 produit une fusion ternaire ; les deux ordres
des arêtes rendent la même fusion après contraction. Une requête d'ancêtre **fermée** au rang 1 doit voir
cette fusion entière. Cela rejoint le contrat actuel d'activation de toutes les fusions égales avant requête
(`forest_ancestor_sweep.hpp:44–54`).

## Le MST ne transporte pas tous les compteurs logiques

Témoin géométrique K1 : A=(0,0), B=(3,0), C=(1,2). Les trois boules diamétrales sont Gabriel strictes :
le troisième site est extérieur, vérifié exactement. Niveaux AC=5/4, BC=2, AB=9/4.
À 9/4, AB touche une composante déjà fusionnée ; le moteur compte un plateau et une continuation.
Le MST retire AB et conserve exactement la même hiérarchie, mais perd ces deux compteurs et une composante
touchée. Modèle complet : `unions=2,touched_components=5,continuations=1,plateaus=3` ; modèle MST seul :
`2,4,0,2`. Pour conserver les registres logiques, garder les incidences/cellules originales ou leur
métadonnée suffisante ; le seul dendrogramme ne les reconstitue pas.

En revanche, pour le balayage actuel de la forêt basse jusqu'au dernier rang demandé μ :

- `ancestor_activations` = nombre de **fusions**, hors naissances, de rang ≤ μ ;
- `ancestor_unions` = somme des arités de ces fusions ;
- `ancestor_find_steps` dépend des chemins DSU et de la compression.

Les deux premières égalités viennent directement de `advance` et `unite`
(`forest_ancestor_sweep.hpp:45–54,82–91`) ; elles se calculent sans simuler un DSU. Les qualifier toutes
trois de physiques dans O7 est trop large. La requête par sauts binaires doit garder la coupe fermée et
les contrôles de naturalité de chaque enfant. Pour un nœud haut v, choisir une naissance descendante b(v),
prendre son image basse puis son plus haut ancêtre de rang ≤ rang(v), et vérifier que chaque image d'enfant
y remonte, suffit ; ces contrôles doivent encore être payés et comptés. Aucun changement du registre formel
n'est effectué ici.

## T1 : borne et porte d'arène

Avec les racines Ready possédées hors arène, une tâche de profondeur d et de compte R réserve au plus
`R*(3B-d)` mots pour ses buffers descendants (`frontier.cpp:107–124`). Le potentiel
Σ ceil(log2 largeur) baisse à chaque coupe et les listes descendantes ont chacune capacité ≤ R.
Les allocations du parent doivent vivre jusqu'au retour du fils droit. `(3B+2)*R` est donc sûr mais non
serré dans ce régime ; `(3B+1)*R` peut rester sûr, et la seule profondeur 36 ne tue pas ce prétendu mutant.
Une porte causale préférable impose une capacité effective d'arène de pic_exact−1 à un parcours dont le pic
est connu ; elle exige un refus propre avant l'écriture excessive. Frontière, workspaces et sorties
coexistants restent comptés séparément. Cette remarque ne juge pas les trois fichiers WIP du développeur.

## O8 : limite du certificat et repli

Un échec de certificat de largeur device est `unresolved`, pas une graine approximative ni un refus
géométrique. La trace entière peut être rejouée sur CPU avant admission ; c'est la voie simple qui conserve
la date initiale, les refus exacts et le ledger, sans compter deux fois les vagues déjà exécutées.
Une reprise intermédiaire exigerait un état exact complet, la date initiale et les deltas logiques.
La descente doit conserver strictement décroissants ses niveaux successifs et imposer
date_initiale < niveau_cellule, même si la naissance finale est plus basse. Les succès de table device
requièrent cardinal et liste complète des SiteIdx, pas seulement l'étiquette de hachage.

La capsule est une preuve du lemme de graphes et de ces témoins bornés, pas une qualification du futur port
O7/O8 ni une preuve d'accélération. Aucun snapshot massif, donnée LiDAR, build ou test natif n'est inclus.
