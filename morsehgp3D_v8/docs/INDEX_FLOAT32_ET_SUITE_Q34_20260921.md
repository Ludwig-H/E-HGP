# Nuage et index float32 : une base commune pour les voies géométriques

21 septembre 2026, après `36724438`. Exploration hors registre,
`backend=cpu_reference`, `profile=lossless_float32_input_only`,
`mode=native_index_preparation`, `public_status=not_claimed`.
Cette tranche prépare les objets natifs nécessaires au moteur float32 ;
elle ne calcule pas encore une WSPD ni la tour FULL.

## Ce qui change concrètement

Le [nouvel index](../src/spatial/float32_index.hpp) possède sa propre copie
des coordonnées, dans l'ordre d'entrée. Les IDs restent ceux de cette
entrée ; la permutation spatiale et son inverse sont des tableaux séparés.
Pour un morceau LiDAR, ses IDs locaux restent reliés aux sites de la trame
et aux retours bruts par les correspondances du préparateur, pas par un
changement implicite de la numérotation de l'index.
Modifier ou détruire le stockage appelant ne change donc pas les points
certifiés. Aucun tableau du propriétaire n'est exposé en écriture ; copie,
déplacement et affectation de ce propriétaire sont interdits.

Tous les float32 finis sont acceptés, sous-normaux compris. Deux sites
numériquement identiques sont refusés, avec `+0` et `−0` considérés égaux.
Les mots binaires des points uniques sont cependant conservés exactement,
y compris leurs zéros signés. Le préparateur Python normalise ces derniers ;
le propriétaire C++ n'a pas besoin de le faire pour les comparer correctement.
Aucun arrondi sur grille n'est introduit ici.

La factory réalise une copie privée des triplets bruts avant validation,
puis construit les points certifiés non assignables. Ce tampon temporaire
est libéré avant les tris et l'arbre. Il n'y a aucune adoption du vecteur
de l'appelant qui laisserait subsister des alias mutables.

Les boîtes gardent les **bits des coordonnées extrêmes exactes**, pas des
centres et rayons arrondis. Les comparaisons numériques utilisent un ordre
entier des mots float32 finis, indépendant du mode d'arrondi. La largeur
en double ne sert qu'à choisir l'axe d'une partition ; elle ne constitue
jamais un certificat de distance. Une différence de coordonnées float32
peut nécessiter plus de 53 bits significatifs malgré des extrémités
exactement représentables en double.

## Construire sans dépendre de l'étendue des exposants

Un découpage au milieu géométrique est inadapté à certains float32 : sur
les 278 points `0` et `2^e`, pour `e=-149..127`, il donne une chaîne de
profondeur 277. Une profondeur héritée des coordonnées u16 ne suffit plus.

La nouvelle construction :

1. Trie trois permutations d'IDs, une par axe. Les deux autres coordonnées
   puis l'ID départagent les égalités.
2. Choisit un axe et coupe son tableau au **rang médian**, pas au milieu
   de ses coordonnées.
3. Sépare stablement les deux autres tableaux avec les rangs du premier,
   sans les retrier, puis recommence dans les deux moitiés.

Chaque plage conserve ainsi trois ordres triés sur le même ensemble de
sites. Au bout, les trois tableaux convergent vers la même permutation de
feuilles ; les rangs inverses sont réécrits une fois. L'arbre contient
`2n−1` nœuds, de profondeur au plus `ceil(log2 n)` : **9** pour la fixture
des 278 points. Aucun plafond arbitraire de recherche n'est ajouté.

Les trois tris initiaux coûtent O(n log n). À chaque niveau, les partitions
parcourent des plages disjointes dont la somme des tailles vaut au plus n.
Il y a O(log n) niveaux : **construction O(n log n), stockage O(n)**.
Ce n'est pas le tri récursif O(n log² n), ni une promesse empirique fondée
uniquement sur trois chronométrages.

Pour un nœud interne de m sites, les compteurs paient m écritures de rang,
5m lectures et 4m écritures d'IDs dans la partition. Les comparaisons des
trois tris, copies, validation, extrémités de boîtes et rangs inverses sont
publiés séparément. Les initialisations de vecteurs et tous les coûts de
construction restent inclus dans le temps mesuré, même lorsqu'ils n'ont
pas un compteur d'opérations dédié.

## Objets partageables et parcours

Les nœuds sont stockés en préordre. Chaque nœud connaît sa plage d'IDs,
ses deux enfants et la position suivant tout son sous-arbre (`escape`).
Une requête peut donc accepter/rejeter tout un bloc puis sauter directement
au suivant, sans pile de recherche ni allocation interne.

La première requête disponible est une **boîte fermée**. Ses décisions
d'inclusion/exclusion sont exactes sur les mots float32. Elle transmet au
consommateur des plages empruntées d'IDs d'origine, sans les recopier.
Il n'existe aucun compteur, curseur ou résultat mutable dans le propriétaire.
Plusieurs workers peuvent lire le même index avec leurs callbacks et
compteurs privés. Le callback reste synchrone ; ses exceptions conservent
les émissions et le travail déjà tentés, sans endommager l'index.

Les tableaux contigus et les plages permettent de futurs descripteurs de
tâches compacts. Ce n'est **pas encore** une construction parallèle, une
file GPU ou un port CUDA. Une requête peut visiter tout l'arbre ; multiplier
les requêtes par n n'est pas une preuve sous-quadratique du census.

La mémoire publiée est celle des capacités des vecteurs simultanés :
maximum du tampon initial avec les points, puis des points, nœuds, rangs,
trois permutations et tampon de partition. Le transfert final d'une
permutation ne la recopie pas. Objet fixe, bloc de contrôle partagé,
allocateur, pile de construction et mémoire du consommateur ne sont pas
du RSS inclus dans ce compteur.

## Vérification et mesures

La gate rationnelle indépendante vérifie les mots d'entrée, l'unicité,
les extrema de chaque boîte, toutes les plages et leurs liens, la médiane,
la profondeur, la permutation inverse et les réponses complètes aux
requêtes. Des fixtures couvrent les exposants extrêmes, sous-normaux,
zéros signés, égalités d'axes, tailles impaires et IDs désordonnés.
La sonde native exerce aussi les alias, destructions, exceptions de callback
et lectures concurrentes. Les preuves et résultats de la qualification
sont publiés dans les [reçus de cette tranche](../receipts/float32_index_20260921/README.md).
Release et Clang ASan/UBSan passent les 27 fixtures, 404 requêtes,
35 refus et 151 contrôles natifs ; les lectures normal/−O concordent.
Un premier avertissement signé/non signé de la sonde a arrêté la compilation
stricte ; l'échec est conservé et seul le cast du test a été corrigé.
Trois variantes du code recompilées séparément sont aussi détectées :
unicité numérique des zéros signés, fermeture des boîtes, liens escape
internes. Les diagnostics attendus sont exigés, pas un simple crash.

Les mesures séparent lecture/génération d'entrée, construction, checksums
et requêtes. Le constructeur inclut ses copies, tris, allocations, partitions
et validation ; les checksums sont hors de ce temps. Les requêtes incluent
la consommation de leurs IDs. Les synthétiques uniformes, terrain et amas
utilisent trois tailles 8k/16k/32k ; les LiDAR restent entiers, ou coupés
en moitiés/quarts selon le protocole du capteur, jamais sous-échantillonnés.
Les trois répétitions conservent les mêmes comptes et empreintes de points,
permutation et requêtes ; les grandes constructions n'ont pas ici d'oracle
exhaustif de tous leurs nœuds, contrairement aux petites fixtures.

| Construction mono CPU, médiane de trois essais | 8 000 | 16 000 | 32 000 |
|---|---:|---:|---:|
| uniforme | 7,071 ms | 15,355 ms | 33,173 ms |
| terrain | 7,097 ms | 15,375 ms | 33,259 ms |
| amas | 7,241 ms | 15,376 ms | 33,433 ms |

Les comparaisons de tri sont multipliées par 2,113 à 2,214 à chaque
doublement, les lectures de partition par 2,154 puis 2,143. Sur les six
relations spatiales réelles de la scène0, les exposants de tri valent
1,093 à 1,125 ; aucun poste de construction publié n'atteint le seuil
quadratique dans cette campagne. La preuve O(n log n) ne dépend pas de
ces observations. Cela ne borne pas le nombre futur de candidats q3/q4.

Les trames complètes 0/100/200 de 123389/124479/125526 sites demandent
126,906/129,596/95,983 ms pour **cet index seul**, avec 19,25/19,42/19,58 Mo
de vecteurs conservés. Les pics vectoriels valent environ 22,2 à 22,6 Mo.
Machine locale partagée, pas G4 : ni gain stable inter-scènes ni contrat
100ms/FULL ne découle du troisième temps. Construction encore mono-thread ;
la lecture concurrente testée ne la transforme pas en construction parallèle.

## q3/q4 : décision numérique et objets à préparer ensuite

La contrelecture du code u16 a confirmé les formules géométriques, **pas
leur capacité arithmétique une fois les coordonnées élargies**. Séparer :

- un support immuable et ses coefficients d'intervalles, partagés ;
- un repli entier exact, privé au worker quand le filtre hésite ;
- une clé de boule canonique, construite seulement lorsqu'elle doit être
  émise ou insérée dans le catalogue, pas pour chaque candidat rejeté.

Pour q3, le signe vient de `G·|z−a|²−W·(z−a)` ; acuité et non-dégénérescence
restent des conditions exactes séparées. Pour q4, conserver la puissance
relative et les quatre signes barycentriques stricts de positivité. Le
comparateur de racines q4 réduit de degré 5 évite les produits naïfs de
degré 9 déjà refusés dans le moteur u16.

Dans l'unité entière commune `2^-149`, un float32 fini a une magnitude
inférieure à `2^277`, et une différence inférieure à `2^278`. Les bornes
conservatrices du code existant donnent les dimensions suivantes :

| Objet | Degré | Borne de magnitude | Mots u32 suffisants en magnitude |
|---|---:|---:|---:|
| puissance q2 | 2 | <2^558 | 18 |
| puissance q3 | 6 | <2^1677 | 53 |
| puissance q4 | 5 | <2^1398 | 44 |
| positivité q4 | 6 | <2^1677 | 53 |
| comparaison réduite q4 | 5 | <2^1397 | 44 |

Ce sont des bornes de conception, pas de nouvelles primitives implémentées.
Elles n'incluent pas arbitrairement toute opération ultérieure sur des
fractions ou des clés : chaque comparateur garde sa propre preuve.
Les 18 mots de la primitive q2 ne se réutilisent pas aveuglément.

Le chemin rapide proposé évalue les polynômes en **unités physiques** par
intervalles double arrondis vers l'extérieur. Les expressions de degré au
plus 6 ont un majorant conservateur inférieur à `2^783` sur les float32
finis ; pas de débordement supérieur binary64 pour ces formules. Leurs
annulations et éventuels petits intervalles nécessitent toujours un
encadrement sûr et un repli exact. Aucun epsilon ne décide un contact.
Ces filtres, leurs bornes par blocs et leur version GPU restent à qualifier.

La clé globale reste un quintuplet entier primitif `(A,Bx,By,Bz,C)`,
`A>0`, pgcd commun 1, représentant `A|z|²+B·z+C` dans l'unité fixée.
Elle identifie la même boule entre supports et arités. Une clé traduite
localement doit être retranscrite dans le repère global ; centre/rayon
arrondis et IDs de support ne remplacent pas cette identité géométrique.
Extraire les puissances de deux communes ou stocker significatif/exposant
peut réduire le repli. Une translation par soustraction float32 arrondie
est interdite : elle modifierait les données que l'utilisateur veut garder.

La suite reste le partage des blocs de graines q3 et des tâches intérieures
q3/q4, avec ces nouveaux contrats numériques. Cet index ne transfère aucun
chrono20mm à la précision brute, ni aucune qualification FULL/G4.
