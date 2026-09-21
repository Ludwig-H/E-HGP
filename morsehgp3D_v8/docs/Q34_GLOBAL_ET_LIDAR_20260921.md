# q3/q4 : mesurer les scans LiDAR et raccorder le front global

21 septembre2026. Tranche31 après c5308651, en développement.
`exploration_v8_hors_registre`, `cpu_reference`, `quantized_u16_input_only`,
`implementation_v8_p0`, `public_status=not_claimed`.

## Critère de progression précisé par l'utilisateur

L'objectif est l'efficacité sur les régimes visés, notamment les scans
LiDAR de SemanticKITTI. Un contre-exemple de coût quadratique reste utile
pour connaître les limites ; il ne bloque plus à lui seul le raccord
global, la parallélisation et les essais G4. L'exactitude mathématique
reste exigée sur toutes les entrées du profil, pas seulement en moyenne.

Nous comparons le travail discret et les temps à8k/16k/32k, puis50k,
sur chaque scan séparément, avec K5/10 et s8/10/12 au front global.
Un ratio de croissance observé ne devient pas un théorème universel.
Une arête productive choisie ne représente pas toutes les arêtes du scan.

## Deux mesures complémentaires

1. Rejouer les voies q4 par blocs28, couches29 et fenêtre30 sur les
   mêmes arêtes réelles, sans changer les IDs ou adapter les axes.
   Préparer cloud/index/cover une fois, payer les préparations propres
   de chaque voie et comparer les records complets. Alterner l'ordre
   des moteurs pour distinguer leur travail des effets de cache.
2. Consommer réellement les rectangles résiduels du front q3/q4 :
   chaque paire fournit une arête, un cover partagé et ses seeds.
   q3 est calculé séparément ; q4 utilise28 ou30 explicitement.
   Compter toutes les paires, tous les covers et le census, pas seulement
   le temps du front ou une projection de coûts sur quelques arêtes.

La seconde étape n'exige aucune paire q2 acceptée, ni face q3 acceptée
pour accéder à q4. Les voies gardent leurs masques de rejet distincts.
Elle ne matérialise pas la liste globale des paires ni la mosaïque de
Delaunay ; le traitement est un flux sur l'index propriétaire partagé.
Le développement du résidu peut néanmoins coûter cher : ses compteurs
et son temps sont indispensables pour décider du prochain traitement
par blocs ou du découpage parallèle.

## Contrat mathématique du raccord

Le support positif d'une boule définit son arête propriétaire : longueur
maximale, puis IDs originaux pour les égalités. Le certificat de citron
porte sur cette arête du support, pas sur une arête déjà acceptée en q2.
Les témoins stricts du front rejettent à K−1 pour q3 et K−2 pour q4.
Le nombre total de sites sur une coquille n'est pas l'arité du support.
Le census terminal repart de zéro ; les crédits du front ne sont pas
additionnés au compte exact. Les contacts sont conservés intégralement.

Le flux peut présenter plusieurs supports de la même boule. Les clés
exactes et coquilles seront regroupées au catalogue ; les intérieurs
et la reconstruction FULL restent des étapes distinctes, non acquises
par ce raccord. Le contrat50k porte toujours sur toute la tour sur G4,
pas sur un front, un scan de témoins ou une sélection d'arêtes.

## G4 et budget

Feu vert utilisateur confirmé. Une session G4 SPOT sera courte, avec
les deux arrêts vérifiés du dépôt et fermeture ciblée ; aucun démarrage
tant que le paquet de test utile n'est pas prêt. Une mesure CPU sur G4
sera annoncée CPU, jamais comme exécution GPU. Le backend v8 est encore
CPU à l'ouverture de cette tranche.

## Preuve et périmètre du raccord global

Cette section décrit le raccord de candidats, indépendamment des mesures
LiDAR et des essais G4. Son entrée est le même nuage u16 immuable et son
index spatial partagé. Elle ne remplace pas la construction du catalogue
de boules, la collecte des intérieurs ou la reconstruction des hiérarchies.

### Pourquoi le front peut rejeter une arête sans avoir accepté q2

Considérons un support strictement positif de q sites, avec q égal à3 ou4.
Sa boule circonscrite a son centre O dans l'intérieur relatif du support.
L'arête ab est la plus longue de **ce support** ; les égalités sont
départagées par la plus petite paire d'IDs originaux. Posons D égal au
carré de sa longueur, m son milieu et t le vecteur allant de m à O.
Les poids barycentriques positifs du centre donnent directement la borne
de rayon suivante, sans hypothèse sur les autres sites du nuage.

$$ R^2=\frac{1}{2}\sum_{i,j=1}^{q}\lambda_i\lambda_j\lVert v_i-v_j\rVert^2\leq\frac{D}{2}\left(1-\sum_{i=1}^{q}\lambda_i^2\right)\leq\frac{q-1}{2q}D,\qquad \lambda_i>0,\quad\sum_{i=1}^{q}\lambda_i=1. $$

Comme a et b sont sur la boule, t est perpendiculaire à ab. La borne de
rayon contrôle donc le déplacement possible du centre hors de l'arête.

$$ \lVert t\rVert^2=R^2-\frac{D}{4}\leq\frac{D}{4\alpha_q},\qquad \alpha_3=3,\quad\alpha_4=2. $$

Pour un site z, notons u la composante de z−m perpendiculaire à ab. Les
deux quantités calculées par le front ont alors une interprétation simple.

$$ H=(z-a)\cdot(b-z)=\frac{D}{4}-\lVert z-m\rVert^2,\qquad \Xi=\lVert(z-a)\mathbin{\times}(b-z)\rVert^2=D\lVert u\rVert^2. $$

La puissance de z par rapport à n'importe laquelle des boules positives
considérées est majorée par une expression ne dépendant que de l'arête
et du témoin. Une puissance négative signifie strictement intérieur.

$$ \lVert z-O\rVert^2-R^2=-H-2u\cdot t\leq-H+\sqrt{\frac{\Xi}{\alpha_q}}. $$

Sur un rectangle A×B, le front calcule un minorant Hmin valable pour
toutes les paires et un majorant Ximax également commun. Il peut donc
créditer z dans la voie q lorsque les deux inégalités sont strictes.

$$ H_{\min}>0,\qquad \alpha_q H_{\min}^2>\Xi_{\max}. $$

Ce certificat n'utilise ni une paire q2 acceptée ni une face q3 acceptée.
Un témoin proposé qui serait un sommet du futur support ne peut satisfaire
ce certificat pour cette boule : il serait simultanément sur sa coquille
et strictement intérieur. Les propositions utilisent des rangs distincts
du même index ; aucun compte partiel n'est additionné au census terminal.
Un témoin non trouvé par la proposition heuristique fait seulement perdre
un rejet possible, jamais une solution.

### Seuils, masques et grandes coquilles

La voie q accepte exactement lorsque sa profondeur stricte p reste sous
son seuil. Autant de témoins distincts suffisent donc à la rejeter.

$$ h_q=K-q+2,\qquad p<h_q,\qquad h_3=K-1,\quad h_4=K-2. $$

À K1, ce raccord q3/q4 est vide ; à K2, seule q3 est active. Ces cas sont
validés avant le retour vide, y compris les options et les paramètres du
répartiteur. La plage K1…10 est celle du front existant, pas un quota de
recherche ni de sorties.

Bien que le citron q4 soit inclus dans le citron q3, son seuil est plus
petit. On peut donc rejeter q4 et conserver q3, ou rejeter q3 et conserver
q4. Chaque bit suit sa propre preuve et reste retiré après subdivision.
Un contact exact, y compris l'égalité dans la borne du citron, n'est pas
crédité comme intérieur.

Le q de la preuve est le nombre de sommets du **support positif**, jamais
le nombre de sites sur la coquille. La coquille peut comporter30 sites ou
davantage : toute sa liste doit être conservée sur une émission acceptée.
Une même boule peut avoir plusieurs présentations, voire une présentation
positive d'arité plus petite. Le raccord conserve le contrat de chaque
voie ; il ne déduit pas le rejet global d'une boule du rejet d'une seule
présentation ou d'une seule arité.

La décomposition du front couvre chaque paire non ordonnée une fois avant
les rejets. Chaque support accepté possède donc son arête dans un rectangle
résiduel de sa voie. Le raccord développe les **paires de sites** de ce
rectangle, pas des paires de rectangles. La génération exacte des seeds
et le balayage complet du moteur choisi fournissent alors ses candidats.
Pour q4, le contrat reste la première présentation valide par seed et
racine, avec seed canonique : ni toutes les incidences ni une déduplication
globale des boules ne sont promises.

### Un cover par arête, deux calculs indépendants

Une seule préparation du cover fermé de l'arête est partagée entre les
voies encore actives. Il contient tous les sites vérifiant la borne suivante.

$$ \lVert 2z-a-b\rVert^2\leq4D. $$

Les bornes précédentes donnent, pour q3 comme pour q4, un rayon total
depuis m inférieur à la longueur de ab. Toute la boule positive propriétaire
est donc incluse dans ce cover. Son census et sa coquille deviennent
globaux après certification de positivité et de propriété ; aucun site
extérieur au cover ne peut changer ces deux résultats.

q3 dispose d'un chemin propre : génération par boîtes des triangles aigus
propriétaires, clé de boule exacte, compte repartant de zéro, saturation à
K−1 et tri de la coquille entière seulement sur acceptation. Il n'effectue
aucun balayage q4 caché. Son tampon de coquille est réutilisé entre les
seeds et entre les arêtes. Indépendamment, q4 appelle soit le moteur local28,
soit la fenêtre30 explicitement choisie. Un rejet du census q3 ne lui ferme
jamais l'accès.

### Équipe parallèle et comptabilité

L'entrée mono reste inchangée. L'entrée parallèle explicite prépare les
jobs Coarse du même front, puis exécute une seule équipe, avec un moteur
privé et une copie du callback par slot. Un slot appelle séquentiellement
son callback ; plusieurs slots peuvent l'appeler simultanément. Les objets
partagés capturés par référence restent à synchroniser par l'appelant.

Le préfixe et les jobs ne répètent pas leurs tests. Les rectangles, arêtes,
covers, seeds et événements géométriques comptés sont ceux du mono, pas
une estimation du travail parallèle. Une arête en cours n'est pas divisée.
Même W1 utilise cette entrée par jobs ; le nombre de jobs visé règle la
granularité, pas la profondeur de recherche. Tous les fils sont joints
avant retour ou propagation d'une exception, et le nuage/index reste
possédé jusqu'à cette jointure. Les émissions antérieures à une exception
ne sont pas annulées.

Les comptes s'additionnent, tandis que les maxima par arête prennent le
maximum. Deux pics peuvent différer du mono à cause de l'ordre de réemploi
des tampons privés : la capacité de coquille q3 et le pic couplé
cover+coquille+travail q4. Ils restent publiés ; la porte ne les confond
pas avec des invariants de travail. La somme des pics des workers est
également rapportée, sans être présentée comme un pic simultané ni du RSS.
Les jobs, visites du préfixe, capacités et données par worker ont leurs
propres compteurs.

### Porte indépendante et mutations causales

La porte constructeur a passé le préflight Release : **11 440 contrôles**,
166 appels mono/globaux et46 appels parallèles W1/2/4. Sur les petits
nuages bornés, elle résout indépendamment les supports par élimination
rationnelle de Gram, recalcule profondeur et coquille sur tous les sites,
puis applique le contrat de présentation canonique. Elle ne se limite pas
à vérifier un digest ou les seules boules déjà émises.

Elle couvre les trois séparations s8/10/12, les masques2/4/6, K1/2/3/5/10,
les deux moteurs q4, les tangences des deux citrons, les coquilles30, un
centre peu profond isolé, les IDs permutés, les extrêmes u16, les pannes
mémoire et de callback, le reset du propriétaire, les appels imbriqués
et les appels concurrents. Le parallèle est comparé sur l'intégralité des
279 compteurs u64 du travail, après normalisation des deux seuls pics
privés décrits ci-dessus, ainsi que sur tout le travail du front et les
sorties complètes. Le nombre de workers réellement actifs est une
observation de scheduling, pas un invariant déterministe.

Le [runner des mutations](../tests/wspd_q34_mutations.py) compile trois
copies modifiées dans un répertoire temporaire, sans changer le produit
ni son build : compter la coquille q3 comme intérieure ; conditionner
q4 à une acceptation q3 sur l'arête ; supprimer q4 quand le front a rejeté
q3. Les trois sont rejetées au premier essai par une différence de
boules/profondeurs/supports/coquilles avec l'oracle rationnel, **avant**
les contrôles de compteurs. Un crash, un échec de compilation ou un
simple plancher de non-vacuité n'aurait pas qualifié ces mutations.

La [capture close](../receipts/lidar_global_20260921/mutations/compiled_5slu1ek1/COMPLETION.json)
conserve les commandes, sorties brutes et sources originales/modifiées.
Les196 sources et les artefacts d'origine sont inchangés avant/après.
Les [quatre relectures normal/−O, historique/live](../receipts/lidar_global_20260921/mutations/MUTANTS_READBACK.json)
passent avec229 entrées inchangées et aucune erreur de fermeture. Cela
qualifie ces contrôles bornés ; les autres campagnes et les contrats de
tour ne sont pas déduits de ce seul préflight.

### Trois coûts encore à surveiller sur les nuages réels

1. **Nombre d'arêtes et préparations de covers.** Le front peut laisser
   beaucoup de paires ; chacune construit son cover par l'index partagé.
   Le nuage n'est pas recopié, mais cette préparation répétée est bien
   payée. `expanded_pairs` et la somme `cover_sites` ne sont pas des
   nombres de sites distincts et n'ont pas ici de borne sous-quadratique.
2. **Census q3 répétés.** Chaque seed propriétaire peut relire son cover
   jusqu'au rejet saturé ou jusqu'au bout, puis trier une coquille acceptée.
   La saturation aide les boules profondes ; elle ne supprime ni le nombre
   de seeds ni le coût des familles peu profondes ou des grandes coquilles.
3. **Travail q4 et équilibre de l'équipe.** Le local28 peut conserver de
   grandes frontières actives ; la fenêtre30 garde un premier scan des
   témoins retenus pour chaque seed. Les couches peuvent ne rien retirer.
   Une arête coûteuse reste atomique et peut déséquilibrer l'équipe ;
   produire, trier ou copier une grande coquille reste également un coût
   réel du callback. Paralléliser ce travail ne prouve pas sa croissance
   sous-quadratique et ne constitue pas une implémentation GPU.

## Premier diagnostic global : ce n'est pas le tri

La première commande globale LiDAR8k terminée, scan0/K5/s8/Local28/W4,
prend1360,996s sur la machine locale sous charge concurrente. Elle émet
104 670 candidats après expansion de2 285 750 arêtes. La campagne prévue
8k/16k/32k a été interrompue volontairement pendant16k ; son statut reste
FAILED, sans résultat16k/32k ni ratio de croissance inventé.

| Poste | Travail à8k |
|---|---:|
| Boules q3 construites |780 661 556|
| Tests ponctuels du census q3 |361 201 093 303|
| Parmi eux, sites extérieurs |357 937 525 263|
| Candidats q3 émis |93 914|
| Comparaisons du tri des coquilles q3 |327 815|
| Seeds q4 |439 969 682|
| Comparaisons du tri d'événements q4 |16 402 922|

Les99,1% de tests q3 qui concluent « extérieur » montrent un travail
massivement évitable par des décisions sur des groupes. Ces comptes ne
sont pas des temps par phase : ils ne donnent pas à eux seuls la fraction
exacte du temps CPU. Ils réfutent néanmoins l'idée qu'une optimisation du
seul tri suffirait : celui-ci ne crée ni ces seeds ni leurs census.

La suite est donc structurelle et reste à implémenter/qualifier :

1. Rejeter davantage de rectangles et de sous-produits AVANT de préparer
   leurs covers et leurs seeds, avec des témoins distincts certifiés.
Comparer le coût total des propositions, pas le seul taux de rejet.
2. Pour q3, interroger les boîtes de l'index avec la puissance entière
   de la boule. Une boîte extérieure évite toutes ses lectures ; une
   boîte strictement intérieure contribue son cardinal et permet de
   saturer. Une boîte ambiguë est subdivisée. Les contacts ne sont jamais
   comptés comme intérieurs et toute coquille acceptée reste collectée.
   Les bornes arithmétiques de ce nouveau chemin restent à prouver.
3. Réduire aussi le nombre de seeds q3/q4 : accélérer chaque census ne
   suffit pas si780 millions de clés de boule sont encore construites.
   Étudier les certificats de familles et de blocs de seeds, sans faire
   dépendre q3/q4 de l'acceptation q2 et sans nouveau histogramme carré.
4. Mesurer ensuite le gain du parallélisme et sa répartition. Le relais
   48CPU puis GPU doit consommer ces mêmes objets certifiés ; il ne doit
   pas servir à masquer le travail intermédiaire actuel.

L'auditeur A a été interrogé sur ces deux étages de rejet. Le résultat
actuel n'est pas compatible avec le contrat50k, même avant catalogue et
FULL ; ce constat porte sur un scan du régime cible, pas sur un adversaire
artificiel. Les nouvelles tailles2k/4k/8k sur G4 servent au diagnostic
rapide ; elles ne remplacent pas la campagne finale8k/16k/32k.

### Proposition précise pour le census q3 par boîtes

Cette proposition n'est pas encore du code qualifié. La clé actuelle fournit
A>0, B et C entiers pour la puissance. Sur une boîte u16, la fonction est
séparable en trois polynômes convexes f_i(t)=A*t²+B_i*t. Son maximum
par axe se trouve à une extrémité. Son minimum sur les coordonnées entières
se trouve parmi les deux entiers voisins du sommet −B_i/(2A), rabattus
dans l'intervalle de l'axe. Il suffit d'évaluer ces positions ; **ne pas
carrer B_i**, ce qui augmenterait inutilement la largeur arithmétique.

La somme des minima plus C donne un minorant exact sur la boîte entière
de la grille, la somme des maxima plus C son majorant. Minimum>0 rejette
le nœud comme entièrement extérieur ; maximum<0 ajoute son cardinal aux
intérieurs. Aux égalités, raffiner pour préserver tous les contacts.
Les bornes existantes q3 A≤12M⁴, |B_i|≤60M⁵, |C|≤144M⁶, M=65535,
donnent360M⁶<2¹⁰⁵ pour ces sommes et leurs termes : i128 suffit pour
ces évaluations. Le calcul de division/arrondi et les cas sommet hors
boîte, bornes négatives et singleton exigent leurs propres tests.

Contrelecture mathématique interne : pour une boule q3 positive valide,
le centre appartient à l'enveloppe de son support u16, donc à[0,M]³.
Le numérateur −B_i est ainsi non négatif et la division entière positive
donne directement le plancher. Un helper plus général devrait traiter
explicitement le plancher d'un quotient négatif ; ne pas prétendre
exercer cette branche avec un centre q3 valide hors du domaine u16.
Toujours rabattre les coordonnées en i128 avant conversion et évaluation.
Promouvoir aussi **avant** t*t : une coordonnée u16 est promue en int
signé et65535² n'y tient pas. Évaluer par exemple (A*t)*t+B_i*t
avec t déjà élargi. Pour saturer, tester cardinal≥seuil−compte avant
l'addition ; un nœud doit être compté une fois seulement.

Une préparation du sommet entier par boule peut éviter les divisions par
nœud. Le parcours porte un seul compteur saturant et des nœuds disjoints ;
pas de crédit ajouté deux fois, ni de nouvel index par seed. Définir
explicitement le traitement des plages du cover ou repartir de l'index
global : les deux variantes doivent être comparées sur leur travail total.
Cela reste indépendant de la réduction indispensable du nombre de seeds.

Fixtures proposées pour ce futur port (pas une qualification exécutée) :

- Triangle((0,0,0),(2,2,0),(2,0,2)), clé[3,−8,−4,−4,0] : sommets
  des paraboles4/3,2/3,2/3, choix du plancher ou du plafond selon l'axe.
- Triangle((0,0,0),(2,0,0),(1,1,1)), clé[2,−4,−2,−2,0] : sommets
  demi-entiers, égalités ; la boîte entière x=z=0,y∈[0,1] ne contient
  que deux positions de coquille. Min=max=0 ne donne aucun intérieur.
- Triangle((65535,65534,65533),(0,0,65532),(2,65531,0)) : grandes
  composantes B dont le carré dépasse128bits, bien que l'évaluation
  séparée précédente reste sûre. Confirmer la clé par l'oracle rationnel.
- Sommet de parabole hors de la boîte, singletons intérieur/contact/
  extérieur, bloc atteignant exactement ou dépassant K−1, grandes
  coquilles et permutation des IDs. Compter les nœuds et les cardinalités
  rejetées/admisses pour ne pas mesurer seulement les tests ponctuels restants.

### Deuxième chantier : exposer davantage de travail parallèle

Le raccord31 est volontairement une référence Coarse : un worker garde
son sous-arbre de front, puis un rectangle et chaque arête en cours.
L'équipe comporte plusieurs threads, mais ces unités peuvent masquer un
travail très inégal. Les temps GNU time distants permettent de mesurer
l'occupation CPU moyenne, séparément des comptes d'arêtes par worker.

Objets à comparer pour la suite, sans les déclarer déjà implémentés :

- produit de front non visité, avec masques/profondeur conservés ;
- plage de paires non commencées d'un rectangle, sans liste globale de
  toutes les arêtes et sans repayer les preuves de ses ancêtres ;
- bloc de seeds d'une arête coûteuse, partageant son cover et, pour q4,
  sa préparation géométrique immuable, pas une nouvelle copie par tâche.

Conserver une seule équipe persistante et des tampons de sortie privés.
Un coût par arête ou par famille doit pouvoir être redistribué sans
réinitialiser son census ni recompter des témoins. La collecte d'une
coquille complète et la durée atomique d'un callback restent explicites.
Mesurer le travail total et l'occupation CPU : un meilleur équilibrage ne
doit pas réintroduire des préparations, copies de frontières ou tris répétés.

La campagne G4 R3 est ensuite terminée et la cible arrêtée : voir les
[mesures closes](../receipts/lidar_global_20260921/README.md). À48workers,
1k/2k/4k/8k prennent0,863/8,470/59,274/614,744s ; l'occupation CPU
moyenne à8k est3,23, pas48. Le census q3 croît de plus de×10 à chaque
doublement. Les visites d'atlas q4 font également×9,38/×10,91/×10,39 :
le petit résidu final q4 ne doit pas masquer sa préparation et ses requêtes.
Les deux voies ont donc un travail mesuré superquadratique sur cette série,
sans prétendre établir leur loi asymptotique à toutes les tailles.
Les comptes/digests8k sont égaux entre localW4 et G4W48. Aucun GPU ou
FULL n'est exécuté, aucun nouveau gain de constante q2 n'est revendiqué.
