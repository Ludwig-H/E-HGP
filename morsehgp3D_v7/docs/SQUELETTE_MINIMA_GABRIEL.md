# Peut-on prendre seulement les simplexes Gabriel comme sommets ?

6 septembre 2026. Étude mathématique constructive, indépendante d'un
chronométrage. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

## Réponse et distinction des objets

**Oui comme sommets d'un certificat filtré avec connexions transférées ;
non comme simple sous-graphe induit gardant les connexions géométriques
directes.** Le transfert doit conserver les composantes à chaque niveau,
pas seulement la connexité finale. La bonne réduction est un quotient
certifié, pas la suppression des facettes non-Gabriel.

Le [manuscrit](../../docs/references/MANUSCRIT_THESE_HAUSEUX.pdf) distingue :

- définition 21, PDF 84 : Gamma_K a tous les simplexes de Čech de cardinal K
  pour sommets ; deux sommets sont adjacents quand leur union est dans Čech ;
- proposition 5, PDF 112 : les seules unions de cardinal K+1 conservent les
  composantes, avec **tous** ces sommets intermédiaires disponibles ;
- définition 29, PDF 115–116 : le graphe de Gabriel a comme sommets les
  facettes des cofaces Gabriel de cardinal K+1. Ses sommets ne sont pas
  nécessairement eux-mêmes Gabriel.

La proposition utilisateur constitue donc une nouvelle réduction de
sommets, à distinguer de la réduction d'arêtes de la proposition 5.
La proposition 6 du manuscrit ne peut pas servir de preuve : sa suppression
brute des cofaces non-Gabriel est déjà réfutée par
[E5](../../docs/math/INCIDENCES_SILENCIEUSES_GAMMA.md).
« Hiérarchie K-NN » désigne ici la filtration de densité du théorème 2,
pas les composantes d'un graphe usuel reliant chaque point à K voisins.

Dans la suite, K est la cardinalité d'une facette, donc sa dimension
simpliciale est K−1. Posons $\beta(F)=\rho(F)^2$, et $\mathcal{M}_K$
l'ensemble des facettes Gabriel de cardinal K. Toutes les hypothèses
régulières de l'[audit FULL](AUDIT_NIVEAUX_GABRIEL_20260905.md) restent
requises ; elles ne résultent pas du seul choix de sommets.

## Contre-exemple régulier à quatre points

À K=2, prendre les coordonnées u16 suivantes, en dimension ambiante trois :

$$A=(1,1,7),\quad B=(5,2,1),\quad C=(7,2,2),\quad D=(5,2,8).$$

Les minima Gabriel sont exactement AB, AD, BC et CD. Leurs niveaux carrés
sont respectivement 53/4, 9/2, 5/4 et 10. BD n'est pas Gabriel : C a une
puissance −2 pour sa boule de diamètre. AC n'est pas Gabriel non plus :
B et D ont chacun une puissance −2 pour sa boule.

Les cofaces BCD et ABD sont Gabriel. BCD a pour support BD, pour intérieur
C et pour niveau 49/4. ABD a pour support ABD et pour niveau 477/34.
Dans Gamma_2, les facettes BC et CD sont connectées à BD dès 49/4.
À 477/34, ABD utilise ensuite BD pour réunir cette composante avec les
minima AB et AD. Le résultat fermé est **une composante contenant les
quatre minima**.

Si l'on ne garde que les minima et les adjacences héritées des cofaces
Gabriel, on obtient au contraire deux composantes :

| Composante erronée | Minima | Points couverts |
| --- | --- | --- |
| Première | AB, AD | A, B, D |
| Seconde | BC, CD | B, C, D |

Les couvertures se recouvrent, mais ce recouvrement n'est pas une règle de
fusion. Même la variante plus généreuse reliant deux minima g,h dès que
$\beta(g\cup h)\leq t$ échoue : toutes les unions transversales ont ici
le niveau 31/2, strictement supérieur à 477/34. Elle retarde la fusion.
Autrement dit, garder les régions témoins des seuls minima et leur graphe
d'intersection direct ne résout pas le problème.

Cette fixture isole la **perte d'un sommet non-Gabriel** : BD se rattache
ici par BCD, qui est une coface Gabriel. Il n'est même pas nécessaire
d'invoquer une coface silencieuse pour réfuter la suppression naïve.
E5 porte l'obstruction supplémentaire des cofaces silencieuses.
Les vérifications rationnelles et la régularité sont portées par le
[test autonome](../tests/full_gabriel_minima_quotient_gate.py) ; ce test
borné ne devient jamais un constructeur produit.

## Pourquoi les minima suffisent néanmoins

Sous régularité, une facette F Gabriel ne possède aucun point étranger
dans sa boule fermée. Par unicité de sa MEB, aucune extension ne peut
avoir le même niveau. F naît donc isolée à beta(F).

Si F n'est pas Gabriel, un intrus strict z donne une coface F+z au même
niveau. Le retrait de chacun des points essentiels fournit des facettes
strictement antérieures, dont l'union couvre F+z. F rejoint des composantes
anciennes, éventuellement pendant leur fusion, sans nouvelle naissance.
Le lemme des attaches silencieuses et sa confluence de plateau montrent
que seules des cofaces Gabriel de cardinal K+1 peuvent porter de vraies
multifusions. L'induction par niveaux donne alors :

1. toute composante contient au moins un minimum déjà né ;
2. sa couverture de points est l'union des labels de ses minima descendants ;
3. les seules modifications de sa généalogie sont les naissances de minima
   et les vraies multifusions, traitées par lots atomiques.

La deuxième propriété ne dit pas que toutes les régions témoins sont
recouvertes par celles des minima : le contre-exemple interdit cette
inférence. Elle concerne les points du nuage et les identités des
composantes, pas leur carrier continu complet.

### Un graphe filtré exact sur les seuls minima

Pour deux minima distincts g,h, définir la hauteur de connexion dans
Gamma_K **élémentaire** (cofaces de cardinal K+1) :

$$u_K(g,h)=\min_{\pi:g\leadsto h\text{ dans }\Gamma_K}\ \max_{Q\text{ coface du chemin }\pi}\beta(Q).$$

On active chaque sommet g à beta(g), et l'arête g–h à u_K(g,h).
Par définition du minimum sur les chemins, deux minima sont connectés
à la coupe fermée t exactement lorsqu'ils appartiennent à la même
composante de Gamma_K à cette coupe. Le même raisonnement vaut pour la
coupe ouverte. La surjectivité sur les composantes vient du point 1 ;
la couverture en points vient du point 2. Les inclusions entre coupes
respectent ces identifications : toute la hiérarchie, pas seulement une
partition finale, est conservée.

En général, $u_K(g,h)\leq\beta(g\cup h)$, avec inégalité stricte possible.
La coface géométrique g∪h ne donne donc pas toujours le bon poids.
Cette définition est une preuve de suffisance, **pas un algorithme
efficace pour calculer toutes les paires**. Matérialiser ce graphe complet
serait une mauvaise architecture pour v7.

Une version sparse se construit depuis les vraies multifusions : si un
lot réunit q composantes anciennes, choisir un minimum représentant dans
chacune et les relier par q−1 arêtes au niveau du lot. Conserver ces arêtes
avec les naissances restitue les mêmes coupes, indépendamment du choix
des représentants. Les arêtes du même niveau se lisent atomiquement :
aucune généalogie binaire artificielle n'est introduite dans la sortie.
Avec L minima et R racines finales, cette forêt a exactement L−R arêtes.
Le certificat de multifusions existant encode la même information sans
imposer le choix de ces représentants.

Le nombre q−1 concerne les **liens entre q parents**, pas une permission
de résoudre seulement deux facettes d'une coface. Dans la fixture n4,
ABD a trois bras essentiels AB, AD et BD, qui atteignent trois parents
distincts avant 477/34. N'importe quel choix de deux bras en perd un.
En 3D régulière, le support borne le nombre de bras essentiels par quatre,
pas par deux ; il ne borne pas le coût des chemins pour les résoudre.

## Portée pour MorseHGP3D v7

Le [certificat FULL](CONTRAT_CERTIFICAT_FULL.md) suit déjà cette réduction :
ses feuilles sont les minima Gabriel, non toutes les facettes incidentes.
Le [producteur horizontal](CONTRAT_PRODUCTEUR_FULL_GABRIEL.md) calcule
les parents via les facettes réutilisées, leurs rattachements certifiés
et la normalisation des ancres. Le cache lazy est facultatif ; les
facettes intermédiaires ne deviennent pas des feuilles de sortie.

La proposition est donc pertinente pour fixer le **bon objet minimal
de sortie**, mais supprimer les résolutions de parents du constructeur
serait incorrect. Les gains encore possibles portent sur leur calcul :
réutilisation de terminaisons réellement certifiées, réduction des
recalculs MEB et rejet géométrique sûr de blocs inutiles. Cette étude
ne mesure aucun de ces gains et ne les déclare pas intégrés.

Pour toute la tour 1..Kmax, une directe de cardinal m≤Kmax est aussi une
naissance obligatoire de l'ordre m, même si elle ne fusionne rien à
l'ordre m−1. Écarter sa géométrie parce qu'elle est une continuation
inférieure perdrait cette feuille supérieure. Une seule découverte
géométrique doit servir les deux rôles ; le code partage déjà les
catalogues successifs. Seul le rang terminal Kmax+1 n'a pas le rôle de
naissance demandée. Une recherche limitée aux connexions utiles à ce
dernier ordre est donc une piste distincte, à prouver et mesurer, pas
une suppression générale des directes sans fusion.

Un calcul non circulaire des attaches peut utiliser la MEB et le support
exact d'un petit label, une recherche d'intrus strict dans l'index global,
puis une descente strictement décroissante et une ancre certifiée
normalisée. C'est le type de primitive existant ; aucun examen exhaustif
de toutes les paires de minima n'est requis par la preuve du quotient.
Une alternative par recherche de coupes devrait certifier elle aussi
l'absence de toute connexion omise plus précoce : la nommer Borůvka ou
MST ne fournit pas cet oracle géométrique.

### Variante nouvelle : descendre à cardinal K constant

La [contrelecture indépendante](../audits/receipts_gabriel_vertices_20260906/README.md)
précise une variante qui n'est pas le resolver produit actuel. Pour une
facette non-Gabriel F, choisir un intrus strict z et un essentiel u ;
remplacer u par z donne une facette F′ **de même cardinal K**, avec
beta(F′)<beta(F). L'union F∪F′ est la coface F+z, née à beta(F).
Répéter jusqu'à un minimum donne donc une ancre valide. Les minima
terminaux peuvent dépendre des choix ; leurs composantes normalisées à
la coupe de consommation sont identiques, ce qui est l'invariant utile.

Un lookup du label dans le catalogue complet des minima **avant** sa MEB
peut éviter le recalcul du terminal. Le resolver courant passe plutôt
par des cofaces de cardinal K+1 et leurs ancres fermées, après le cas J=1.
La variante pourrait donc réduire le cardinal des MEB intermédiaires et
supprimer le besoin de ces ancres directes comme autorité horizontale.
Elle ne supprime ni la découverte des cofaces de fusion ni les ancres
verticales de la tour. Les deux cas J=1 et J≥2 doivent rester comparés,
sans retirer un raccourci actuel favorable sur une simple intuition.

La preuve de terminaison est finie et stricte, pas une borne logarithmique
de longueur. Les nouvelles boules visitées exigent leurs propres contrôles
de support et de coquille ; la qualification d'une fenêtre de rang de
l'ancien trajet ne se transfère pas automatiquement. Le gain de temps,
la mémoire réellement économisée et les refus ne sont pas établis par
ce seul argument. La qualification différentielle de cette variante
est la prochaine piste de simplification horizontale, avant tout port
multi-CPU ou GPU.

Le [différentiel rationnel permanent](../tests/full_gabriel_descent_comparison_gate.py)
fige les deux sens du compromis. Sur E5/K2, le modèle P=0 passe de deux
MEB à une, et de cinq ordinaux de supports F à un. Sur la fixture régulière
A=(0,3,3), B=(3,2,9), C=(8,6,12), D=(12,9,3), E=(13,6,11), le parent
BD consommé par ABD à 1909/41 est obtenu par J=1 via BCD en une MEB ;
la descente BD→CD→DE en exige deux, avec deux census au lieu d'un.
Les deux chemins donnent le même parent. Ces comptes rationnels ne
mesurent ni les visites d'index ni la latence du moteur C++.

Pour la **tour**, l'ancre fermée d'une directe Q de cardinal m dans
l'ordre m−1 est également l'image verticale de la feuille Q de l'ordre m,
pour 2≤m≤Kmax. Les deux rôles peuvent partager une valeur et un index ;
supprimer l'usage horizontal ne supprime donc pas nécessairement son
stockage dans la sortie verticale. Cette ancre inférieure n'identifie
jamais les parents distincts de l'ordre supérieur. Le rang Kmax+1 est
à nouveau distinct. Un hybride conservant le raccourci J=1 lorsque cette
ancre partagée est disponible, puis descendant les facettes sinon, est
une piste à qualifier ; aucun gain universel n'est déduit de son nom.

Première expérience produit proposée, plus petite qu'un remplacement
complet : après J≥2, tester uniquement si la première facette F′ est déjà
un minimum connu. Sur succès, vérifier son niveau strict et normaliser ;
sur absence, reprendre la chaîne actuelle. J=1 reste inchangé. Cette
variante ne visite aucune nouvelle boule et ajoute un seul lookup par
demande éligible. Son taux de succès, son coût et ses refus sont encore
à mesurer ; elle n'est pas intégrée par la présente étude.

Il faut toujours conserver les labels et niveaux de naissance, les
multifusions et leurs parents distincts, ainsi que l'autorité de complétude.
Pour la tour inter-K, ajouter l'ancre de chaque naissance dans l'ordre
inférieur **après fermeture de son plateau**, puis la normaliser.
Pour les poids de la section 9.1, les facettes contributrices non-Gabriel
et leur politique d'affectation restent un supplément distinct : on ne
peut pas remplacer silencieusement cet univers pondéré par les minima.

À K=1, tous les points sont des minima et on retrouve le single-linkage.
À K=n, X est une feuille sans coface. Hors régularité, les naissances et
connexions simultanées demandent un quotient de plateau propre : le
triangle rectangle déjà documenté exclut une feuille Gabriel faible
automatiquement persistante.

Enfin, L peut lui-même être quadratique en n en dimension trois, même à
K fixé : voir la [borne de sortie](CROISSANCE_ET_BORNE_DE_SORTIE.md) et
son [complément indépendant](../audits/receipts_probe_meb_review_20260906/full_output_growth.md).
Le stockage linéaire en L ne prouve pas un coût sous-quadratique en n.
Les mesures 8k/16k/32k doivent distinguer volume de sortie et travail
intermédiaire, sans traiter un refus comme un temps de complétion.

## Rapport à la littérature et autorité

Les effondrements certifiés de complexes étudiés par
[Bauer et Edelsbrunner](https://arxiv.org/abs/1312.1231) fournissent un
contexte de théorie de Morse discrète. Leur équivalence d'homotopie des
complexes de Čech/Delaunay n'est pas utilisée ici comme preuve des
composantes du graphe des K-facettes : l'objet et l'obligation de transfert
des incidences sont différents. Les preuves ci-dessus reposent sur les
définitions du manuscrit et les lemmes FULL explicitement liés.

La contrelecture a été demandée à l'auditeur via la
[coordination](../../audits/COORDINATION_MORSEHGP3D_V7.md). La qualification
du petit oracle rationnel reste séparée de celle du producteur C++ et
des contrats industriels. Aucun nouveau claim d'exactitude publique,
de performance, de verticale ou de poids. GCP non utilisé.

Le test rationnel constructeur est exécuté normalement et sous `python -O`.
Ses [captures et sources rejouables](../receipts/full_minima_quotient_20260906/README.md)
sont conservées séparément du moteur : 17 modèles aux ordres 1..n,
640 contrôles de coupe, 68 histoires de lots
et dix mutants. Les sorties sont identiques entre modes ; les invocations
inconnues rendent 2. Il compare Gamma élémentaire, quotient minimax,
transfert d'arêtes par ancres et arbre du quotient, y compris les identités
des minima et leurs couvertures. Il ne qualifie pas un nouveau resolver C++.

Le différentiel des descentes passe aussi en normal/`-O`, avec codes 0/2
pour selftest/argument inconnu. Ses 17 modèles et trois capacités de cache
donnent 74 demandes par capacité, dont quatre résolutions non minimales.
Il n'y a aucun cache hit : les économies de cache ne sont pas qualifiées
par ce corpus. La contre-fixture J=1 ci-dessus est un cas supplémentaire.
