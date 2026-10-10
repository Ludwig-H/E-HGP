# Travail G : réponses mathématiques aux questions 1 et 2

10 octobre 2026 ; demande `QUESTION_CLAUDE_g_travail.md` au commit
`5f3318baa`. Proposition de politique, **aucun changement natif ni gain
mesuré**. Les fixtures ci-dessous sont synthétiques, exactes et autonomes.

## 1. Une résolution par composante locale suffit pour FULL

Soit une cellule de boule b=(c,λ), où λ est le rayon **carré**, avec intérieur
strict I et coquille U. Fixons k et t=k−|I|. Ses traces strictes sont
F_A=I∪A, où A⊂U, |A|=t et c∉conv(A). Considérons le graphe dont les sommets
sont ces A et les arêtes les paires telles que c∉conv(A∪A′).

**Énoncé.** Toutes les traces d'une même composante de ce graphe appartiennent
à la même composante de la couverture d'ordre k à la coupe **ouverte λ**.
Résoudre une trace canonique par composante locale suffit donc à obtenir
l'ensemble des composantes ouvertes incidentes à la cellule. Recopier la
cible obtenue aux autres traces préserve cet ensemble, sous la restriction
de date expliquée ci-dessous.

**Preuve.** Pour une arête, la séparation stricte fournit v avec
v·(u−c)>0 pour tout u∈A∪A′. Déplacer c vers c+εv diminue strictement toutes
ces distances au carré pour ε assez petit. Les distances des points de I
restent strictement inférieures à λ, par leur marge initiale et la finitude
de I. Il existe donc une boule de rayon carré r<λ contenant F_A∪F_A′.
Les ensembles X_F(r)=∩_{x∈F}B(x,√r), convexes, ont une intersection non vide.
Ils sont contenus dans la région couverte au moins k fois ; les deux traces
ont la même composante avant λ. Une chaîne d'arêtes donne le résultat par
transitivité, à un même r<λ choisi supérieur à tous les seuils de la
chaîne. Au noyau, deux témoins de cette composante ont déjà la même racine
avant la cellule. R relit la même composante ouverte. Les ensembles de
branches de la cellule, puis les multifusions et les verticales de la tour,
sont donc inchangés mathématiquement. □

Les échanges d'un seul site suffisent : si A∪A′ est séparable, toutes ses
sous-parties le sont, et les t-parties de A∪A′ sont reliées par échanges.
C'est précisément le graphe déjà parcouru par `cells.cpp::count_pieces`.
Le natif calcule son nombre de composantes, mais `classify_extended` garde
`shape.reps=s`, **toutes** les traces strictes. La référence constructive
`Reference._local` choisit déjà une trace par composante. Cette piste ne
requiert donc pas de conjecture nouvelle sur la géométrie.

**Fixture positive.** Sites XY, z=0 :
`(10,5), (9,8), (8,9), (5,10), (1,2)`. Boule c=(5,5,0), λ=25,
p=0, q_min=2, k=2. Neuf traces strictes ; composantes locales de tailles
**8 et 1**. Les huit traces de la première composante peuvent partager une
résolution pour l'usage à λ. Ajouter le site intérieur (5,5,0) et prendre
k=3 donne encore neuf traces et deux composantes. Le modèle vérifie aussi
l'ensemble des composantes globales incidentes, pas seulement les tailles.

**Limite décisive : aucun raccourci local régulier.** Pour une coquille
régulière m=q_min, la jonction a t=m−1 et m traces U\{u}. Deux traces
distinctes ont pour union U, qui contient c dans son enveloppe convexe :
aucune arête locale. Plus fortement, dans le domaine réduit aux seuls
I∪U, k=|I|+m−1 : les k-parties qui omettent un point intérieur contiennent
U et ne sont pas strictes ; les seules traces strictes sont les m parties
omettant un point de U. Deux d'entre elles n'ont aucune intersection avant
λ. Elles constituent donc **m composantes ouvertes distinctes**, pour tout
choix d'I intérieur. On ne peut en supprimer universellement une à partir
seulement de la structure locale régulière. Des chemins via des points
extérieurs peuvent les réunir, mais il faut alors un certificat global.

Partager k−1 sites ne suffit pas : le triangle `(0,0,0),(4,0,0),(2,4,0)`
a trois composantes d'ordre 2 avant λ=25/4, réunies à λ. Chacune des trois
paires partage un site avec une autre. Le tétraèdre ci-dessous donne quatre
traces d'ordre 3 partageant deux sites par paire, pourtant distinctes.

**Restriction de date et de livraison.** La cible copiée est prouvée correcte
à λ⁻ et ensuite, **pas au niveau propre β(F)** de chaque trace. Elle peut
différer de la cible exacte que la politique actuelle de `resolve_part`
renverrait pour F ; le digest brut de G n'est donc pas présumé identique.
`forest_kernel.cpp` consomme les témoins à la cellule et
`registry_branches.cpp` à sa coupe ouverte : ce sont les usages examinés. La publication d'un raccord reste conditionnée
à la vérification exhaustive de tous les consommateurs de `targets()` et
des témoins propagés, notamment les verticales et les vues futures : ce
reçu ne prétend pas une fermeture de cette obligation native.
Ne pas généraliser la copie à une API de résolution à une coupe antérieure.

Un raccord éventuel conserve toutes les traces, leurs offsets et leur compte
d'objet, et distingue résolutions physiques, traces copiées et compteurs de
la politique. Garder une trace canonique déterministe par composante locale.
La file préchargée `kLag` de `passes.cpp` retarde les réponses : publier les
copies seulement après la résolution effective du leader, puis publier la
tranche lorsque **toutes** ses cibles sont écrites. Budget et refus restent
à requalifier, sans allocation universelle inutile sur les cellules régulières.

Portes nécessaires avant produit : composantes ouvertes par trace contre la
référence ; FULL sémantique et registre de branches ligne par ligne ; W1/W48,
voies séquentielle/recouverte, coquilles étendues et plateaux ; test négatif
sur triangle/tétraèdre ; digest G qualifié selon la nouvelle politique.
Les 76 cas Python ne qualifient aucun raccord natif. **Compter d'abord**
traces et composantes des cellules étendues : leur rareté peut rendre ce
levier sans intérêt dans les scènes mesurées. Aucun pourcentage de temps
économisé ne découle du témoin 9→2.

## 2. Partage entre ordres : géométrie oui, cible non en général

**Obstruction explicite à l'inversion des verticales.** Sites
`(0,0,0),(0,2,2),(2,0,2),(2,2,0)` : arêtes de niveau 2, faces de niveau
8/3, tétraèdre de niveau 3. À la coupe 17/6, la couverture d'ordre 2 a
**une composante**, celle d'ordre 3 en a **quatre**. Toutes les 2-parties
de chaque 3-partie sont dans cette même composante inférieure. Deux cibles
ayant la même image à l'ordre inférieur peuvent donc être distinctes à
l'ordre supérieur. La naturalité donne une application vers le bas, pas
son inverse ni l'identification des fibres.

**Lemme positif de transfert de certificat MEB.** Soit b la plus petite
boule de F, certifiée par un support S⊆F avec MEB(S)=b. Pour toute autre
partie H telle que **S⊆H⊆b**, MEB(H)=b. En effet, son rayon minimal est au
moins celui de S et au plus celui de b ; l'unicité de la plus petite boule
donne le même centre. En particulier si F⊂H, il suffit de certifier
exactement que les nouveaux points sont dans b. Dans l'autre direction,
S⊆H⊆F suffit sans nouveau test d'enclos. La taille de H peut changer.

Il s'agit d'un transfert de **sphère minimale**, pas de cible : à nouvel
ordre, la table de naissances, le seuil du census, la fenêtre et les cibles
changent. On peut réutiliser un census **complet** I,U de la même sphère
sur le même domaine immuable ; les décisions dépendant de k doivent être
recalculées. Une réponse saturée à k prouve seulement p≥k, jamais p≥k+1.
Les clés exactes de sphère ou supports certifiés doivent identifier la même
boule ; aucun rapprochement flottant ni simple ressemblance des parties.
Les contrôles de rang propres à chaque résolution restent obligatoires.

**Fixtures de séparation des obligations.** Pour
`A=(0,0,0), B=(4,0,0), C=(2,1,0), D=(2,2,0)`, F={A,B,C} et H={A,B,C,D}
ont la même plus petite boule, centre (2,0,0), λ=4, support {A,B}.
Son intérieur est {C}, sa coquille {A,B,D}. La cellule d'ordre 3 est une
jonction à deux composantes locales, celle d'ordre 4 une naissance : même
sphère, **types de cibles différents**. Remplacer D par (2,3,0) fait croître
la plus petite boule : conserver le support sans vérifier l'enclos est faux.

Pour `(0,5,0),(10,5,0),(4,5,0),(6,5,0)`, la sphère des deux extrémités a
p=2 : elle est saturée au seuil 2, pas au seuil 3, bien qu'ajouter l'un des
points intérieurs à la partie conserve sa plus petite boule. Une continuation
de census demanderait compte et curseur certifiés pour cette même sphère ;
le seul statut « saturé » ne suffit pas.

Ces lemmes permettent un partage de certificats/censuses indépendamment de
l'achèvement des forêts. Ils ne fournissent pas de correspondance directe
des cibles entre ordres. Le catalogue et `LEM-T1` partagent déjà une partie
de cette géométrie ; tout cache supplémentaire doit être mesuré avec ses
comparaisons exactes, sa mémoire, ses synchronisations et la latence FULL.
Aucun gain n'est acquis ici.

## Rejeu indépendant

`python3 -B -S [-O] check.py --repo <repo>`

Énumération exacte des supports de taille ≤4 par systèmes de Gram rationnels,
puis plus petite boule par inclusion ; composantes du nerf des intersections
de k boules via β(F∪F′)<λ. Pas d'import du moteur ni de la référence produit.
76 cellules locales bornées, deux fixtures positives 9→2 et cinq familles
de gardes réfutées. Sources Git épinglées ; résultats exacts dans
`results.json`. Relectures normales et optimisées réussies.
