# P0 — addition des colonnes et intersection des résidus

13 septembre 2026. Troisième tranche, `implementation_v8_p0`, CPU mono,
entrée u16, `public_status=not_claimed`. Extension du
[filtre axial](P0_PARTAGE_ET_FILTRE_AXIAL.md), pas un nouveau moteur FULL.

## Ce qui change, simplement

Le premier filtre cherchait assez de témoins sur une seule colonne.
Le nouveau additionne les témoins disponibles sur les trois colonnes
exactes passant par l'ancre. Il rejette donc aussi certaines paires pour
lesquelles aucune colonne ne suffisait seule. Un deuxième changement
permet de conserver seulement les paires que **les deux filtres** jugent
encore indécises : ce filtre axial et un plan local Pool, Dual ou Tubes.
Cette intersection ne réintroduit aucune paire déjà éliminée par l'autre.

L'API garde le mode `Independent` par défaut. `Additive` est explicite,
avec une restriction q2 facultative appartenant exactement au même
propriétaire immuable. Le plan copie les crédits de la restriction une
seule fois ; déplacer ou réaffecter ensuite le plan source ne modifie
pas ses décisions. Une restriction de mauvaise voie ou d'autre propriétaire
est rejetée, même si le cœur élimine déjà toutes les paires.

## Pourquoi les additions sont permises ici, mais pas partout

Pour a fixé, la colonne d'axe i contient les sites ayant les deux autres
coordonnées égales à celles d'a. Les témoins sont ceux strictement entre
a et b sur cet axe : $c_i(b_i)=\#\{z\in A\setminus\{a\}:z_j=a_j\ (j\ne i),\ \min(a_i,b_i)<z_i<\max(a_i,b_i)\}$.
Chaque site vérifie $H=(z_i-a_i)(b_i-z_i)>0$. Les colonnes de deux axes
distincts se rencontrent seulement en a, exclu : leurs témoins sont
disjoints. Le cœur est extérieur à A et B, par certification du propriétaire.
Avec h égal au seuil moins le cœur, la somme des trois comptes permet
donc de rejeter dès qu'elle atteint h. Un site à l'égalité ne donne pas
de crédit ; d'autres sites peuvent néanmoins suffire à rejeter la paire.
Pour h=0, le résidu est vide sans préparer les colonnes ni copier les
crédits de la restriction ; son propriétaire et sa voie restent contrôlés.

En revanche, les crédits locaux peuvent compter ces mêmes témoins.
Leur somme avec les crédits axiaux serait fausse. La fixture
A={(0,0,0),(1,0,0)}, B={(100,0,0)}, h=2 le démontre : un seul site est
compté par les deux filtres, mais la paire doit rester. La gate permanente
vérifie l'intersection logique, jamais une addition de ces populations.
Les preuves des deux auditeurs sont conservées dans
[la note additive indépendante](../../audits/morsehgp3D_v8_complementaire/P0_SOMME_TEMOINS_AXIAUX.md)
et [la proposition de requêtes](../audits/P0_SOUS_RECTANGLES_ET_GROUPES.md#8-raffinement-axial-en-cours--additionner-les-colonnes-exactes).

## Sélection par blocs, sans développer les paires

Les trois tris fournissent les rangs des sites dans leurs colonnes. Pour
chaque ancre, le plan conserve une fenêtre de h voisins au plus de chaque
côté, sous forme de rangs dans les permutations partagées. Compter les
témoins à une borne demande O(log(h+1)) comparaisons, sans scanner la colonne.

Sur un intervalle qui contient la coordonnée de l'ancre, le compte minimal
vaut zéro. Sinon, il est atteint à la borne la plus proche. Le maximum
est atteint à une des deux bornes. Les axes étant séparés, leurs minima
et maxima s'additionnent sur une boîte. L'index de B porte aussi, si
nécessaire, les minima et maxima des crédits locaux de ses sites.

L'ordre réel des tests évite les recherches de rang dès qu'un rejet
plus simple suffit :

1. Rejeter si le crédit local d'a plus le minimum des crédits du bloc B
   atteint h. Ce test ne mélange pas crédits locaux et axiaux.
2. Rejeter si le bloc est disjoint de la boîte du premier filtre axial.
3. Calculer les bornes additives ; rejeter si leur minimum atteint h.
4. Accepter la plage entière si le maximum additif et, le cas échéant,
   la somme locale maximale restent tous deux inférieurs à h.
5. Sinon, visiter les deux enfants. Sur une feuille, les bornes sont exactes.

Les racines déjà acceptées ou rejetées évitent toute requête de l'index.
Si aucune ancre ne demande de partage, l'index n'est pas construit.
Des plages adjacentes pour la même ancre sont fusionnées sans changer
les IDs ni les paires représentées. Aucun quota ne tronque une recherche.

## Coûts et limites à ne pas confondre

La construction paie O(|A| log |A| + 48|B| + J log(h+1) + D), hors
propriétaire et plan local éventuel. J compte toutes les classifications,
y compris les racines acceptées/rejetées sans requête d'index :
`query_nodes + whole_factor_accepts + whole_factor_rejects`.
D compte les plages retenues après fusion. Mémoire O(|A|+|B|+D), avec un facteur constant
supérieur à celui du premier filtre. La copie des crédits et leur
lecture dans l'index sont explicitement comptées. Les plans locaux
actifs n'allouent plus deux tableaux nuls aussitôt remplacés ; les voies
inactives ou saturées gardent leurs tableaux nuls dimensionnés correctement.
La suppression de ces allocations n'est pas un gain de temps isolé mesuré.

Pour deux grilles planes alignées sur les axes, complètes et correspondantes,
h≥1 fixé, le résidu
additif vérifie $M\leq |A|(2h^2+6h+1)$. À h=10, la borne par ancre
passe donc de 441 à 261. Avec dimensions r,t, le compte fini est
$M=\sum_{i+j<h} f_r(i)f_t(j)$, où $f_r(0)=3r-2$ et
$f_r(i)=2\max(0,r-i-1)$ pour i>0. Les grilles 50×80, 80×100 et 125×128
donnent respectivement 918 160, 1 912 660 et 3 928 390 candidates.
La gate contrôle ces grandes sorties par formule et descripteurs, sans
développer toutes les paires ni faire un census à grande taille.

Cette borne ne vaut pas pour une nappe tronquée quelconque. Une rotation
qui supprime les colonnes exactes peut encore laisser tout A×B. J et D
n'ont pas de borne générale sous-quadratique établie. Un résidu plus
petit ne garantit pas une sélection plus rapide : le coût des recherches
supplémentaires doit se comparer au travail économisé dans le consommateur.
Les recettes et mesures restent des composants sur rectangle séparé,
pas des WSPD réelles ni une qualification de la tour 50k.

## Raccord suivant

Le census q2 doit traiter les candidats restants avec un index global
de témoins et des décisions strictes sur blocs, y compris la coquille.
Il ne doit ni scanner tout le nuage pour chaque paire, ni additionner
au cœur un compte qui pourrait revoir ses IDs. La
[proposition d'extrema q2](../audits/P0_SOUS_RECTANGLES_ET_GROUPES.md#9-census-q2--des-extrema-exacts-pour-partager-les-recherches)
prépare ce raccord. Les fenêtres A/B et groupes collectifs restent des
alternatives à comparer ; aucune généralisation q3/q4 ou qualification
GPU n'est déduite de cette tranche. GCP non utilisé.
