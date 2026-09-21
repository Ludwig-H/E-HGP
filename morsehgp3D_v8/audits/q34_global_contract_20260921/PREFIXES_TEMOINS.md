# Deux préfixes de témoins pour partager les recherches q3/q4

21 septembre 2026. Proposition indépendante pour la suite de32 (`d1b4dbc6`),
sans changement du produit. Elle précise l'objet à transmettre entre
sous-produits évoqué dans la [note constructeur](../../docs/Q34_PISTES_APRES_INDEXATION_20260921.md).
Deux comptes et deux curseurs suffisent, **si la subdivision du produit
précède la consommation de toute feuille témoin encore ambiguë**.
Ce résultat porte sur l'exactitude de l'état, pas sur un gain de temps.

## 1. Pourquoi le résultat actuel ne suffit pas

La recherche32 consomme aussi les feuilles Z pour lesquelles aucun témoin
universel du rectangle n'est certifié. Après restriction d'A ou de B,
une telle feuille peut pourtant devenir un témoin strict. Un compte
terminal et un curseur épuisé ne décrivent donc pas un préfixe entièrement
classifié pour les paires descendantes.

Exemple u16 : A={(30,30,30),(40,30,30)}, B={(50,30,30)}, avec le site
z=(35,31,30). Pour la première paire, H=74 et Xi=400 : z est témoin
q3 et q4. Pour la seconde, H=−76 : il ne l'est pas. Le site (40,30,30)
est également témoin de la première paire mais endpoint de la seconde.
Une recherche de témoins communs peut finir avec zéro crédit, alors que
la première paire atteint les seuils q3 et q4 à K3. Transmettre zéro/end
comme un compte exact serait faux ; le code32 ne le fait pas.

## 2. État et invariant

Fixer le même propriétaire immuable, son index Z et un ordre DFS global
gauche puis droite. Pour chaque voie active q, poser Tq=K+2−q, alpha3=3,
alpha4=2 et conserver `(c_q, cursor_q)`. Une voie inactive par K ou par
certificat antérieur ne crée pas de seuil entier négatif.

Le rang r_q est `nodes[cursor_q].range.first`, ou n au sentinelle final.
Le préfixe porte sur les **sites de la permutation spatiale**, pas sur
les nœuds visités. L'invariant d'une voie non saturée est :

$$\forall(a,b)\in A\times B,\quad c_q=\#\{z\text{ de rang }<r_q:H(a,b,z)>0\text{ et }\alpha_q H(a,b,z)^2>\Xi(a,b,z)\},\qquad 0\leq c_q<T_q.$$

Chaque site consommé doit avoir le même booléen témoin pour toutes les
paires du produit. Cette condition suffit à l'invariant et reste vraie
sous restriction des facteurs. Les préfixes des deux voies peuvent différer.
L'état porte aussi les références A/B, les bits actifs et le propriétaire
de l'index ; une tâche asynchrone doit conserver cette propriété, pas un
emprunt à une pile disparue.

## 3. Transitions exactes

| Situation pour une voie | Transition |
|---|---|
| Tout le bloc Z est témoin pour toutes les paires | Si sa taille atteint Tq−c_q, rejeter la voie ; sinon ajouter la taille et avancer à `escape`. |
| Aucun site du bloc n'est témoin pour aucune paire | Avancer à `escape`, sans crédit. |
| Bloc Z ambigu et non singleton | Descendre à gauche ; r_q ne change pas. Le retour futur est porté par `escape`. |
| Feuille Z ambiguë et produit multiple | Scinder un facteur **avant** de consommer Z ; transmettre les deux états inchangés aux enfants. |
| Feuille Z et paire singleton | Le prédicat entier strict décide exactement ; une égalité n'est pas un témoin. |
| `cursor_q=end` et c_q<Tq | Conserver la voie, désormais décidée ; ne plus lancer sa recherche. |

Un échec du certificat positif ne justifie jamais la deuxième ligne.
Hmax≤0 est une exclusion suffisante. Un minorant Xi_min permet aussi
alpha·max(0,Hmax)²≤Xi_min. Dans les unités du code32, comparer
alpha·max(0,Hmax4)²≤16·Xi_min. Un crédit q3 ne devient pas un crédit q4.

À saturation, seul le bit de la voie disparaît. À EOF non saturé, son bit
survit, même lorsque l'autre voie impose ensuite un split. Quand toutes
les voies restantes sont décidées, émettre le rectangle résiduel ; si
aucun bit ne survit, le rejeter. Les échantillons WSPD ne peuvent initialiser
c_q : ils ne forment pas ce préfixe certifié. Le census repart de zéro.

## 4. Preuve et raccord limité

Initialement le préfixe est vide et le compte nul. Admettre ou exclure
un nœud ajoute sa population contiguë au préfixe, une seule fois. Descendre
Z ne change ni population ni compte. Restreindre A/B conserve tous les
booléens classifiés ; copier les états aux deux enfants est donc valide.
Les produits enfants partitionnent le parent. À saturation, Tq sites
distincts sont des témoins stricts pour chaque paire ; à EOF, c_q est son
compte exact. Cela prouve rejet, survie et absence de double comptage.
Les arbres finis assurent la terminaison, même sans certificat de bloc.

Un premier raccord peut travailler **dans les rectangles déjà terminaux
de la WSPD**, à la place des recherches rectangle puis paire32. Scinder
leurs nœuds A/B préserve la séparation : les boîtes rétrécissent. Population
initiale et masques du front restent les mêmes ; les rectangles résiduels
sont plus fins. Le jeu final des paires/voies doit être celui du filtre
ponctuel exhaustif au même seuil. Aucun changement simultané du producteur
de boules n'est nécessaire pour tester ce partage.

## 5. Coût à confronter au LiDAR

Aucune liste d'IDs ni frontière Z de taille O(n) n'est nécessaire par
produit. Cela ne borne ni le nombre de produits, ni le coût de leurs bornes,
ni leur mémoire pendante. Propriété partagée, pile/file des produits et
payload restent à payer. Aucune taille C++ mesurée n'est annoncée ici.

Le DFS fixe peut perdre l'avantage « proche du milieu d'abord » de32.
Une exclusion faible peut provoquer tôt beaucoup de subdivisions A/B.
Le [modèle](prefix_model.py) prend ses décisions de bloc par énumération
exacte : il vérifie le protocole, **pas le coût d'un minorant géométrique**.
Un port doit mesurer splits, nouveaux tests, populations héritées,
préparation, covers/seeds, sorties et temps complet sur les mêmes scans.
Distinguer crédits hérités et nouveaux témoins ; conserver Pair32 comme
référence. Une copie de crédit n'est pas une nouvelle visite.

## 6. Complément optionnel : minimum exact de Xi pour une paire

Après échec du minorant affine proposé par le constructeur et B, on peut
calculer le minimum exact sur la boîte continue Z. Avec d=b−a et D=|d|²,
l'identité distance à la droite donne :

$$\min_{z\in Z}\Xi(z)=D\min_{t\in\mathbb{R}}\mathrm{dist}(a+td,Z)^2.$$

La dernière fonction est la somme de trois distances quadratiques à des
intervalles : au plus six ruptures rationnelles, donc sept morceaux
At²−2Bt+C. Le morceau contenant t=B/A donne le minimum ; traiter aussi
les morceaux plats et les bornes partagées. La convexité évite de comparer
toutes les valeurs rationnelles candidates. Les axes d_i=0 n'ont aucune
rupture et donnent une contribution constante.

Le minimum vaut D(AC−B²)/A si A>0, et DC sur un morceau plat. Pour
M=65535, A,C,|B|≤3M². Comparer alpha·max(0,Hmax4)²·A à
16D(AC−B²) reste sous432M⁶<2¹⁰⁵ : i128 suffit. Le minimum continu
est un minorant sûr pour les sites entiers du nœud.

Fixture : a=(100,100,100), b=(200,200,200),
Z={114}×{186}×[114,186]. Hmax=4908. Le minorant par composantes
affines vaut51 840 000 ; le minimum exact vaut77 760 000, atteint en
z=(114,186,150). Or3Hmax²=72 265 392 : seul le second exclut q3.
Les [73 évaluations entières et l'identité quadratique](XI_FIXTURE_CHECKS.json)
contrôlent cette fixture ; elles ne testent pas un solveur général.
Ce complément ne prouve pas un gain CPU ; ses sept morceaux peuvent
coûter davantage que les descentes évitées. Le tester conditionnellement,
sans retarder le premier port de la borne affine.

Proposition et modèle indépendants, hors moteur constructeur, FULL et GPU.
GCP non utilisé.
