# Préparer les bornes q2, puis distribuer le travail restant

14 septembre 2026. Cinquième tranche mono, `cpu_reference`,
`quantized_u16_input_only`, `implementation_v8_p0`, `not_claimed`.
Les contrats 50k concernent la **tour entière sur G4**, pas le CPU local.
Cette tranche optimise une primitive du census ; elle ne livre pas un
nouveau constructeur WSPD, un ordonnanceur parallèle ou une tour FULL.

## Économie implémentée

L'ancre a et la boîte B ne changent pas pendant le parcours des témoins Z
d'une tâche. Pour chaque coordonnée et chaque extrémité e de B, préparer
une fois $C=a+e$ et $D=(e-a)^2$ donne $4H=D-(2z-C)^2$.

L'ancien calcul refaisait notamment les carrés de e−a dans les recherches.
Le nouveau utilise les deux distances carrées aux extrémités de 2Z pour
obtenir les deux extrema :

- Le minimum soustrait à D la plus grande distance carrée.
- Le maximum soustrait la plus petite distance carrée, ou zéro si C est
  dans l'intervalle 2Z. Cela conserve les sommets demi-entiers des paraboles.
- On prend ensuite min/max sur les deux extrémités e, et on somme les
  trois coordonnées indépendantes.

Cette égalité conserve les extrema continus exacts du
[contrat q2](P0_CENSUS_Q2_PARTAGE.md). Elle ne remplace pas le maximum par
un test aux seuls coins de Z, qui perdrait des témoins intérieurs.
Le parcours individuel, l'ordre des témoins, les subdivisions, la saturation,
les intérieurs et les coquilles ne changent pas. L'ancienne routine n'est
pas conservée comme second moteur de production.

## Taille et arithmétique

Les douze constantes occupent **48 octets**, sans pointeur, allocation ou
vue empruntée. Sur u16, C≤131070 et D≤4294836225 : chacune tient en u32.
Les chargements sont ensuite promus en i64 **avant** soustraction et produit.
Les distances absolues à 2Z sont au plus 131070, mais leurs carrés peuvent
dépasser u32 ; les produits ne sont jamais effectués dans ce type.
Les sommes restent entre −12·65535² et 3·65535².

Le constructeur public vérifie B et la requête publique vérifie Z.
L'usage interne au census repose sur les boîtes construites par ses index
immuables et ne revalide pas leurs coordonnées à chaque visite.
Une tâche à B singleton conserve son chemin de boule fixé ; elle ne prépare
pas ces constantes de groupe.

48 octets est la taille des **constantes seulement**, pas de toute une tâche
ni d'une sortie. Le caractère copiable sans pointeur ne constitue pas une
compilation CUDA, un format de transport qualifié ou un gain GPU mesuré.
Les coûts de chargement et la pression sur les registres restent à tester
sur G4. Les bénéfices du compilateur sont aussi à distinguer : GCC déroulait
déjà l'ancienne routine et pouvait différer le maximum jusqu'à l'échec du
minimum positif ; économiser des produits ne garantit pas un gain net.

## Contrôle de complexité

La nouvelle préparation coûte O(1) par tâche de groupe et garde O(1)
données par niveau actif. Elle ne construit aucune matrice A×B, A×A ou B×B.
Les ensembles de tâches et de visites doivent être **identiques** à ceux
de la révision f4815cd4 ; leur comparaison est une condition d'acceptation.
Cette optimisation change une constante, pas la classe de complexité.

Le coût total reste séparé en validation/index global, préfiltre,
construction de l'arbre B, couverture des descripteurs, classifications J,
collecte et IDs émis. Les bornes déjà démontrées — index u16 à profondeur
≤48, au plus 97n visites de préparation, couverture O(D(1+log|B|)) — ne
bornent pas J ni les grandes coquilles. À chaque doublement 8k→16k→32k,
comparer temps, visites, tâches et volume de sortie, pas seulement le temps.

Une évolution inférieure à ×4 est une observation sur ces familles, pas
une preuve générale. Certaines sorties FULL explicites peuvent elles-mêmes
être quadratiques en précision croissante ; cela ne justifie aucun travail
quadratique évitable. Voir les [limites et verrous](VERROUS_ARCHITECTURE.md).

## Prochain raccord massif : ce qui doit être partagé

Le propriétaire actuel copie et valide le nuage entier pour un rectangle.
L'index global est lié à cette identité. Répéter cette API pour tous les
rectangles WSPD répéterait une préparation en n pour chacun : ne pas porter
cette boucle sur GPU telle quelle.

Le raccord doit séparer un `PreparedCloud` immuable, possédant une seule
copie validée et l'index Z global, de vues de rectangles référant leurs
facteurs dans une permutation partagée. Les IDs originaux restent distincts
des rangs de ces permutations. Le nombre et le coût cumulé des arbres B
actifs doivent aussi être mesurés ; partager le seul index Z ne suffit pas.

Deux identités restent nécessaires : même nuage pour index Z/requête,
mais même rectangle certifié pour réemployer une restriction de crédits.
L'auditeur donne le contre-exemple [100,101,200,0] sur une droite : pour
A={100,101}, le crédit de 100 face à B={200} vaut 1, mais vaut 0 face
à B={0}. Les transférer parce que le nuage est le même supprimerait un
diamètre Gabriel utile. Le seuil doit appartenir à la requête, pas à
l'index global partagé. Ce risque concerne le futur changement d'API ;
l'identité stricte de propriétaire actuelle refuse déjà ce mélange.
Une troisième identité porte l'ordre B : l'auditeur montre qu'un même
ensemble ordonné [3,4] ou [4,3] n'autorise pas le partage aveugle de boîtes
indexées par rang. Une continuation référence donc le contexte de requête,
son index Z précis et sa permutation B, pas seulement le nuage.

Une continuation proposée porte `(contexte, ancre, groupe B, curseur Z,
compte acquis)`. Une étape classe un bloc puis rend une continuation,
deux enfants, un rejet ou une collecte. Les enfants héritent du curseur et
du compte, jamais du compte avec retour à la racine. Les constantes C/D
peuvent être préparées à la prise en charge, ou transportées si cela gagne.
À une subdivision B, préparer les constantes de chaque enfant : conserver
celles du parent resterait conservateur mais ne préserverait pas les mêmes
décisions ni les mêmes visites que la référence plus précise.
Ce choix reste à mesurer, pas à figer sur la seule taille de la structure.

La première exécution de cette interface doit rester mono et comparer
physiquement les sorties en suspendant après chaque type de transition.
Ensuite : quelques workers CPU avec compteurs/buffers privés, puis tableaux
résidents et files de continuations sur GPU. Découper aussi l'intérieur
d'un gros rectangle ; un rectangle par thread ne résout pas le déséquilibre.
Un quantum ou une file pleine peut suspendre ou exécuter localement, jamais
abandonner une partie de la recherche.

Le callback actuel emprunte des buffers réutilisés : il ne peut pas devenir
asynchrone sans changer son contrat. Les sorties parallèles demanderont
des segments possédés, des identités de supports et une reprise explicite
si une coquille excède un segment. Kmax borne le compte intérieur retenu,
pas la coquille. Canoniser des boules identiques doit conserver leurs
incidences de supports et éviter de payer d'abord toutes les collectes.

Le [raccord Pool seul](../audits/P0_SOUS_RECTANGLES_ET_GROUPES.md#92-raccorder-pool-seul-sans-reconstruire-le-filtre-axial)
est également une étape précise : au plus une plage B par ancre, obtenue
depuis les classes de crédit, sans reconstruire le filtre axial. Il devra
s'intégrer à une vue résiduelle commune, sans copie supplémentaire du nuage.

## Qualification

Les 34 CTests passent en Release et Clang ASan/UBSan. Le nouvel oracle
exact couvre 1 424 cas, 61 803 contrôles, 243 167 valeurs multiprécision,
dont 57 675 évaluations de grille demi-entière ; huit modèles faux et
six boîtes inversées sont refusés. Les 277 cas du census, ses sorties
physiques et ses compteurs restent inchangés.

Les [124 mesures](../receipts/q2_prepared_bounds_20260914/README.md) comparent
62 appels de chaque révision, sans réécrire les sources f4815cd4.
À 32k/K10/s8, trois répétitions par ordre indiquent une baisse de 4,0–5,7 %
du total Shared sur grille et 4,2–9,5 % sur nappe. Le bras individuel
inchangé varie aussi : il s'agit de mesures exploratoires, pas d'une
garantie de gain. Les doublements du temps sont entre 1,58 et 2,41 ;
ceux des visites entre 1,55 et 2,20, sur ces familles seulement.
Les deux versions gardent exactement le même travail discret.

Décision : conserver cette économie constante, ne pas forcer Shared,
puis passer aux objets partagés et aux continuations décrits ci-dessus.
Les captures de la quatrième tranche restent épinglées à leurs sources :
elles ne deviennent pas automatiquement des mesures de cette optimisation.
Les builds `v8_prepared_bounds_20260913` et sa variante sanitizer sont
désormais épinglés. GCP non utilisé.
