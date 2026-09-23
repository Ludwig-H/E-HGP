# Piste B — tuer des rectangles q3/q4 avant d'énumérer les arêtes

23 septembre 2026. Proposition mathématique indépendante, **non
implémentée et non mesurée**. Les deux pistes existantes de [domination
par blocs](DOMINATION_Q4_PARESSEUSE_PAR_BLOCS_20260923.md) et de
[témoins avant cover](PISTE_B_Q34_NOEUDS_AVANT_COVER_20260923.md)
économisent surtout du travail **après** expansion, sur une arête donnée.
Le [filtre intermédiaire d'une ligne `a×B`](CONTRAT_COUTS_ET_PARALLELISATION.md),
proposé ensuite par l'auditeur A, est une expérience plus simple à
qualifier d'abord : il peut déjà éviter la boucle sur B quand son masque
est vide. La présente piste ne vaut son surcoût en cellules que si cette
étape laisse encore des produits lourds.
Ici l'objet est le produit WSPD résiduel `A×B` lui-même : certifier qu'une
voie q3 ou q4 est vide pour **toutes** ses paires, avec des groupes de
témoins qui peuvent changer selon la région du centre. Le filtre de
rectangle actuel recherche déjà des témoins universels à tout le
« citron » ; cette variante est plus fine, mais potentiellement plus
coûteuse. Aucun succès sur LiDAR n'est présumé.

## Certificat exact

Pour une arête propriétaire `ab`, soit `D=|a−b|²`, `m=(a+b)/2` et `c`
le centre de sa boule positive. Le centre est dans le cube d'entrée
`[0,M]³` (`M=262143`) car il appartient à l'enveloppe convexe du
support. Comme `a,b` sont sur la sphère,
`|c−m|²=R²−D/4`. La formule barycentrique de variance et le fait que
`ab` est une arête la plus longue donnent `R²≤D/3` pour q3 et
`R²≤3D/8` pour q4. Donc toute voie q3 tient dans
`|c−m|²≤D/12`, toute voie q4 dans `|c−m|²≤D/8`. Une boîte extérieure
de tous ces centres pour `a∈A,b∈B` se construit à partir des boîtes A/B
et d'un majorant entier de D, **sans** parcourir `A×B` ; si elle est
large, le certificat peut devenir inutile. La partitionner en cellules
convexes, par exemple des boîtes axiales fermées `C` égales à l'enveloppe
de leurs sommets testés, qui couvrent la région possible avec contacts
conservés.

Pour un nœud témoin `Z` de l'index, dont la population est disjointe des
deux facteurs, tester à **chaque sommet** `v` de `C` l'inégalité stricte

`max_{g∈box(Z)} |g−v|² < min_{x∈box(A)} |x−v|²`.

Les deux extrema sur boîtes axiales se calculent exactement axe par axe.
Pour chaque paire fixe `(g,a)`, la différence
`|g−c|²−|a−c|²` est affine en `c`. Si elle est négative à tous les
sommets, elle l'est dans toute la cellule fermée ; le test des extrema
est plus fort et le prouve simultanément pour **tout** `g∈Z,a∈A`.
Comme une boule candidate passe par `a`, tout site de Z est strictement
intérieur à cette boule, quels que soient `b∈B` et le centre admissible
dans C. Des nœuds Z en antichaîne spatiale donnent des populations
distinctes ; **K−1** témoins dans chaque cellule possible tuent la voie
q3 du produit, **K−2** tuent q4. Les gardes peuvent différer d'une
cellule à l'autre, mais leurs comptes ne s'additionnent pas entre
cellules ; on ne crédite ni les extrémités, ni les témoins au contact.
Exiger des gardes hors A/B est volontairement conservateur : un
raffinement ultérieur peut exploiter des témoins propres à chaque `a`
ou `b` (`h_a`,`h_b`), mais doit alors retirer exactement l'extrémité
choisie et prouver la disjonction de ses crédits pour chaque paire.

Une cellule peut aussi être exclue avant recherche de gardes : si à
tous ses sommets `min_{x∈box(A)}|x−v|² >
max_{y∈box(B)}|y−v|²` (ou symétriquement B>A), l'affinité des
différences prouve qu'aucun `a∈A,b∈B` ne peut être équidistant d'un
centre de cette cellule. Les cellules non exclues et non certifiées
restent **indécises** : les subdiviser, subdiviser A ou B par vrais
enfants de l'index, ou revenir exactement à l'expansion actuelle.
Cette dernière option garantit l'exactitude sans quota de recherche.
Une division A/B partitionne le produit initial en sous-produits
disjoints, sans perte ni doublon de paire ; ne pas émettre une même
arête une fois par cellule de centres.

Les points u18 permettent des bornes entières étroites aux coins
entiers. Une grille dyadique de centres exige cependant de **prouver la
largeur selon la profondeur choisie** avant toute arithmétique i128 ou
de recourir à un repli exact ; aucune conversion flottante approximative
ne décide un signe. Les égalités sont indécises, jamais des témoins
stricts. La partition et les antichaînes doivent être possédées par la
tâche et compatibles avec les workers privés/GPU, sans span de pile
transféré.

## Pourquoi cela vaut un test, et pourquoi rien n'est acquis

R3 sur 08/000000/K10 laissait environ **153,94 M** de masse de paires
après le front, puis **30,78 M** paires développées, **4,51 M** covers,
**7,80 Md** formes chargées et **25,31 Md** tests uniformes. Pour réduire
**structurellement l'énumération**, le certificat doit tuer des produits
non singleton de masse significative **avant** la double boucle de
`wspd_q34.cpp`. S'il ne reste que de petits facteurs ou si les cellules
de centres sont trop larges, il ajoutera simplement du travail. Même la
suppression idéale de q3/q4 ne résoudrait pas le temps FULL : R3/K10
prenait 35,56 s au total, dont 9,70 s pour ce poste.

Première porte, sans modifier les rejets : publier l'histogramme des
rectangles résiduels par `|A|`, `|B|` et masse, puis exécuter le nouveau
certificat **en observation** sur coupes capteur 8k/16k/32k d'une même
trame sans sol. Compter tâches, cellules, nœuds/témoins, tests de coins,
masse de paires prouvée avant expansion, replis, temps et RSS. Juger
ensuite le **travail total** `certificats + paires terminales + covers +
formes + catalogue + FULL`, avec identités de sorties et de coquilles,
avant activation. Répéter sur trames entières et brutes de plusieurs
séquences, K5/K10, s8/10/12. Ni le coût des cellules ni la masse
résiduelle ne bénéficient aujourd'hui d'une borne sous-quadratique
générale ou d'une validation LiDAR.

## Contrelecture B du filtre de ligne proposé par A

La preuve de `filter_q34_witnesses(index, {a}, box(B_node), …)` est sûre :
la surcharge boîte passe par les bornes générales H/Xi, un nœud admis
est témoin strict pour **tous** les vrais `b∈B_node`, et son DFS crédite
des populations disjointes. Le splitter actuel coupe seulement des
plages A et conserve B entier, donc la ligne `a×B` a un propriétaire
unique même si la file refuse une tâche. L'identité de masse proposée
par A tient si retraits complets de rectangle/ligne et expansions sont
des classes exclusives ; les retraits d'une seule voie restent séparés.

Attention à la couture : `expand` reçoit aujourd'hui les rangs
`b_first,b_last`, **pas** l'identifiant ni la boîte certifiée du nœud B.
Transporter `b_node` dans `rectangle` et `rectangle_range` avant la
requête de ligne, sans reconstruire une boîte approximative à partir
d'une tranche de permutation. Chaque ligne paie un DFS supplémentaire,
souvent sans rejet ; comparer cache ON/OFF et stratifier le gain par
`|B|`. La fixture collinéaire d'A sépare les deux filtres, mais ne
mesure pas un gain de pipeline LiDAR. Le filtre de ligne est la première
expérience pertinente ; le certificat par cellules ci-dessus reste une
option si des produits lourds survivent.
Publier aussi la distribution des **visites par ligne indécise** :
`Σ|A|≤Σ|A||B|`, mais une ligne peut visiter O(n) nœuds de l'index
sans preuve, si bien que le surcoût théorique peut dépasser la double
boucle actuelle. Un déclenchement conditionnel mesuré (notamment selon
`|B|`) ou un arrêt de cette seule tentative suivi du chemin exact des
paires peut limiter une régression ; il ne doit jamais plafonner les
candidats ni changer leur complétude.
Pour `|B|=1`, la ligne est la paire : ne pas payer son filtre puis le
rejouer dans `edge`. Réutiliser exactement ce résultat ou sauter
l'étape ligne ; mesurer également la redondance à `|B|=2` avec le cache
R4b actif.

### Repli exact par coins et réemploi du cache — piste à tester

Pour q3/q4, écrire `H=(z−a)·(b−z)`,
`Xi=|(z−a)×(b−z)|²` et `F=√α·H−√Xi`, avec `α=3` pour q3 et
`α=2` pour q4. Le témoin strict équivaut exactement à `F>0` : en
arithmétique entière, tester `H>0` **puis** `α·H²>Xi`, sans calculer de
racine. `F` est **concave séparément** en `a`, `b` et `z` : `H` est
affine en `a`/`b` et concave en `z`, tandis que le produit vectoriel
est affine dans chacun de ces trois arguments et sa norme est convexe.
Par concavité successive, le minimum de `F` sur trois boîtes fermées
est atteint à un triplet de sommets. Tester les au plus **64** couples
de coins pour `{a}×box(B)×box(Z)`, ou **512** triplets pour
`box(A)×box(B)×box(Z)`, est donc un certificat **nécessaire et suffisant
pour ces boîtes continues**. Pour les ensembles discrets qu'elles
contiennent, un coin non strict laisse seulement le cas indécis : il
ne rejette aucune paire. Une admission uniforme stricte interdit
automatiquement qu'un nœud Z contienne une extrémité choisie ; l'égalité
reste non créditée. Vérifier les bornes de largeur i128 sur la grille
u18 avant port et ajouter des oracles entiers/contact sur boîtes
dégénérées.

Ce test de coins est plus serré que séparer `Hmin` et `Xi_max`, dont les
extrêmes peuvent provenir de coins incompatibles. Il coûte toutefois
jusqu'à 64 ou 512 prédicats par nœud : réserver d'abord le repli aux
nœuds **ambigus à forte masse**, en observation, sans annoncer de gain.
Une voie possiblement moins chère que le DFS de ligne neuf consiste à
conserver les au plus `2K−3` nœuds de témoins admis pour `K≥3`
(un pour K2, aucun pour K1) par le filtre/cache
du rectangle ou de la première paire, les retester uniformément contre
`{a}×B_node`, puis ne diviser B ou ne revenir aux paires que si le seuil
n'est pas atteint. Les antichaînes et masques restent séparés par voie ;
un nœud non admis n'apporte **aucun** crédit. Mesurer aussi validation
d'antichaîne, nombre de blocs indécis, coins testés, masses réellement
épargnées et temps q3/q4 complet. Le sidecar A échantillonné sur LiDAR
fournit 15,7–29,8 visites DFS par paire évitable : il ne justifie pas
l'activation du DFS de ligne tel quel. Sa mesure du ticket issu du seul
filtre rectangle ne trouve qu'environ 0,83–1,06 crédit par rectangle
échantillonné, en sommant les deux voies ; le réemploi de ce ticket
seul n'est donc pas non plus un gain présumé. La trace d'une paire
pourrait être plus riche, mais n'est pas encore mesurée.

Une variante bornée descend `(a,B_node,masque,ticket)` : elle reteste
le ticket sur la boîte du nœud, saute `|B_node|` paires si les deux
voies sont certifiées, transmet le sous-masque si une seule l'est,
sinon scinde B ; à la feuille, le filtre de paire normal peut renouveler
le ticket. Un arbre binaire B a au plus `2|B|−1` nœuds : pour un ticket
de taille O(K), le coût supplémentaire de classification est au pire
`O(K·Σ|A||B|)` sur les rectangles, **sans** DFS `O(n)` par ligne et sans
plafond de candidats. Cette borne ne prouve évidemment pas le
sous-quadratique si la masse résiduelle reste quadratique ; le but est
de rejeter assez de blocs pour réduire cette masse et le travail aval.
Ne jamais fusionner deux traces de paires sans dédoublonnage et preuve
d'antichaîne par voie.

### Retour du prototype de ligne du développeur (03 h 45)

La [coordination produit](../../audits/COORDINATION_MORSEHGP3D_V9.md)
rapporte une ablation **hors dépôt, non archivée comme reçu** sur
08/000000/K5/W8 : le DFS `a×B` rejette 15–17 M des 23,7 M paires
résiduelles et réduit le filtre par paire de 114 à 57 Gcycles, mais
consomme lui-même 49–87 Gcycles ; le CPU total passe de 156 à
164–171 s sur hôte partagé. Le flux/condensé reste égal. Ce résultat
écarte **le DFS neuf par ligne tel quel** : réduire le nombre de paires
ne suffit pas si l'on déplace leur coût dans une recherche par ligne.
La variante à ticket borné ci-dessus est différente — elle ne relance
pas ce DFS global — mais demeure une hypothèse sans ablation ni preuve
de gain sous-quadratique. Prioriser l'instrumentation du coût total et
une porte shadow avant toute activation.
Précision de code : le prototype passe bien l'**option** `Affine`, mais
pour `{a}×box(B)` avec B non singleton, la primitive actuelle revient
aux bornes générales ; la spécialisation Affine exige deux boîtes
singleton. Le coût négatif mesure donc cette implémentation, pas une
éventuelle borne singleton×boîte plus serrée. Celle-ci demanderait sa
preuve et sa propre ablation, sans hériter du gain ponctuel.

### Cellules de centres : travail évitable en amont du cover

Pour une cellule convexe `C` de **centres mondiaux** et des boîtes
`A,B,Z`, comparer aux huit sommets `c` de C les extrema de distance
sur les boîtes. Si
`max_{z∈box(Z)}|z−c|² < min_{a∈box(A)}|a−c|²` à chaque sommet, tout
Z est strictement intérieur à chaque boule admissible du bloc ; pour
chaque `z,a` fixe, la différence des distances carrées est affine en
`c`, ce qui étend le signe à toute C. Les nœuds Z en antichaîne peuvent
atteindre K−1/K−2 et tuer la voie **dans cette cellule** avant de
construire un cover ; tuer le produit entier exige le certificat sur
**toutes** les cellules où un centre candidat peut se trouver. La
condition opposée
`min_{z∈box(Z)}|z−c|² > max_{a∈box(A)}|a−c|²` aux huit sommets
certifie un nœud strictement extérieur aux boules de cette cellule,
sans possibilité de contact/coquille. Les extrema sur boîtes sont
conservateurs ; égalité et boîtes trop larges restent indécises.

Sur les six premiers cas R5, l'identité de ledger
`covers_vide = cover_builds−q3_edges−q4_edges+both_edges` donne
**64,7–68,4 %** de covers dont aucune voie ne survit, **après** avoir
payé leur construction et les formes. C'est une masse à viser en
amont, pas un pourcentage que le certificat de cellule saurait déjà
éliminer. Une cellule scindée garde ses crédits propres : ne jamais
additionner ceux de deux cellules ni éliminer un Z de la coquille
globale d'une autre. Il faut posséder/partitionner les centres, conserver
un fallback exact pour les zones ambiguës et comparer temps, formes,
sorties et RSS sur le même appel FULL ; tuiler seulement la matrice
arête×témoin sur GPU laisserait Θ(Σ|cover_e|) travail et ne ferme pas
le problème de complexité.

Pour la **grille u18/1 mm seulement**, les centres q3 aigus et q4
strictement positifs restent dans l'enveloppe convexe de leurs supports,
donc dans le cube global entier `[0,262143]^3`. Une partition de ce
cube par cellules mondiales à coins entiers évite les coins dyadiques
fractionnaires de la construction précédente : aux coins, chaque
distance carrée coin↔boîte appartient à `[0,3·262143²]`, et la
différence signée tient sous `2^38`, donc en `i64`. Les centres réels
entre coins sont couverts par l'affinité du signe. À largeur 1,
terminer par le fallback exact ; si les cellules de **preuve** sont
fermées et se chevauchent au bord, leurs **émissions** doivent avoir
un propriétaire unique (par exemple une convention demi-ouverte),
sans perdre les contacts. Cette preuve de largeur ne s'étend ni au
float32 exact, ni aux coins rationnels/dyadiques mis à l'échelle, ni
aux autres prédicats résiduels q3/q4.
