# Verrous d'architecture — consignes au futur développeur v8

13 septembre 2026. À lire avant de porter un module v7. Cette note
consigne les obstacles discutés après l'audit et le choix de priorité P0.
Elle complète le [plan de refonte](PLAN_DE_REFONTE.md), sans le remplacer.

Cadre de l'audit initial : `exploration_v8_hors_registre`, `backend=none`,
`quantized_u16_input_only`, `audit_v7_math_and_architecture`,
`public_status=not_claimed`. Aucun moteur ou benchmark v8 lors de cet audit.
Les mesures restent celles des sources v7 publiées examinées par l'audit ;
les derniers changements documentaires ne sont pas de nouvelles mesures.

Suivi actuel : l'implémentation mono P0 est ouverte (`cpu_reference`,
`implementation_v8_p0`, toujours `not_claimed`). Le
[census q2 partagé](P0_CENSUS_Q2_PARTAGE.md) traite maintenant une partie
de B1 : compte uniforme et curseur DFS transmis entre sous-requêtes,
sans liste ni racine recommencée après crédit. Le coût de la couverture,
des tests sur groupes et de la collecte reste mesuré ; cette première
comparaison ne clôt ni B1 pour toute la WSPD ni les verrous suivants.
Les résultats v8 sont séparés dans leurs [reçus](../receipts/q2_census_20260913/README.md).
GCP non utilisé.

Suivi du 14 septembre : les [bornes préparées](P0_BORNES_PREPAREES_ET_PARALLELISATION.md)
factorisent une constante de coût, sans changer les visites ni clore B1.
La [sixième tranche](P0_NUAGE_ET_INDEX_PARTAGES.md) sépare maintenant
stockage/index global et contextes de rectangles ; le nouvel appel partagé
ne recopie/valide plus n sites par rectangle. L'ancien adaptateur doit
rester hors du chemin massif. Les restrictions
de crédits exigent le même rectangle/seuil ; les plages B exigent leur
permutation et les continuations leur index Z précis. Un même nuage
ne suffit pas à autoriser ces réemplois. Le contrat de reprise doit aussi
remplacer l'emprunt synchrone des buffers si l'émission devient asynchrone.

Le partage global laisse un verrou mesurable : Pool et Axis parcourent
encore B pour chaque A_i×B. Le seul compteur de copie de restriction vaut
|A|+R|B| dans ce régime ; R et |B| proportionnels à n donnent un terme
quadratique. La somme des facteurs et le vrai front avec rejets précoces
doivent maintenant être traités, pas seulement l'index Z. Le nouvel
[audit de régime WSPD](../audits/REGIME_WSPD_20260914.md) montre aussi la
nécessité d'un chemin sans préparation lourde sur les petits facteurs.

La [septième tranche](P0_FRONT_REEL.md) implémente ce front par nœuds
partagés et masques q2/q3/q4, sans plan ni scan de facteur par produit.
Elle retire ce poste du front, pas de tous les anciens adaptateurs.
Le proposeur ponctuel recommence encore une descente unique depuis la
racine : son coût O(D+K) est payé et compté par produit. B1 n'est donc
pas clos. Le test préalable de lentille et la transmission sans double
crédit proposés par l'auditeur B sont à comparer au coût total du raccord
census, pas à introduire comme nouvelles variantes sans consommateur.
Sur deux rangées parallèles, les seuls témoins ponctuels W3/W4 laissent
un résidu quadratique même s'ils sont tous examinés. Des objets collectifs
et une génération canonique sont requis pour éviter de développer ce résidu.

La [huitième tranche](P0_FRONT_ET_CENSUS_Q2.md) raccorde q2 directement :
un seul index, pas de plan ni d'arbre B par rectangle, compte et curseur
hérités au raffinement. Le coût des ancres Σmin(|A|,|B|), les recherches
Z et la collecte restent payés. Le partage entre plusieurs ancres par
une tâche (A,B,Z) est une piste suivante, pas une borne acquise. Le
proposeur WSPD et le premier groupe census repartent encore de la racine ;
B1 reste ouvert malgré l'absence de redémarrage des enfants B.

La [dixième tranche](P0_ORDRE_TEMOINS_Q2.md) change l'ordre Z avec un
contexte original et un bit de phase. Le préfixe n'est plus celui du
DFS global ; une tâche distribuée devra posséder contexte, phase,
curseur et compte ensemble. Le pointeur emprunté à la pile racine
actuelle n'est pas un format de file CPU/GPU. Les divisions des chemins
d'ancêtres peuvent être répétées après subdivision B : les mesurer
séparément au lieu d'attribuer tout gain aux seules visites géométriques.
Pour le futur groupe A×B, ne pas exclure A du compte : seuls les sites
égaux à une extrémité donnée contribuent nécessairement zéro à sa paire.

L'[onzième tranche](P0_CENSUS_CONJOINT_Q2.md) commence ce partage avant
le passage singleton. Le nombre de racines est maintenant celui des
rectangles dans ce mode ; la somme des petits facteurs reste publiée
pour comparaison, pas comme un nombre de recherches effectuées. La
preuve du relais de compte doit distinguer un préfixe déjà consommé
et l'exclusion ultérieure d'une contribution zéro. Aucun crédit WSPD
ni liste de témoins n'est importé dans ce relais.

Le partage ne doit pas retarder les certificats déjà utiles : diviser B
avant le relais peut multiplier les tâches et faire disparaître le
certificat frère, même si le compte initial était partagé. Le bras
SharedAnchors garde B entier pour mesurer cette différence. Sur les
amas, la prochaine [préparation Pool terminale](P0_POOL_TERMINAL_RACCORD.md)
doit traiter les gros produits avant ce census, sans recréer de
propriétaire, ni confondre rangs spatiaux et IDs. La réduction des
candidates ne suffit pas : F, fragmentation et coût aval restent payés.

La [neuvième tranche](P0_CERTIFICAT_FRERE_Q2.md) teste un seul bloc frère
après chaque division, avant le raffinement suivant. Sa population
suffisante certifie un rejet autonome, jamais un crédit à additionner.
O(1) par tâche n'est pas O(1) par rectangle : tant que le nombre de
tâches croît comme n², cette option ne clôt pas B1/P0. Le gain doit
inclure ses propositions et évaluations de bornes supplémentaires.

## 1. Ordre des travaux et sens de « rédhibitoire »

La [douzième tranche Pool terminal](P0_POOL_TERMINAL_Q2.md) raccorde le
filtre local aux vraies requêtes globales, sans reconstruire le nuage.
Elle ne doit pas déplacer le carré vers un consommateur individuel :
sur les rangées, un plan à crédits nuls retourne au parcours initial,
avec coûts de préparation visibles. Des crédits partiels ne garantissent
pas non plus que Pairwise batte Shared. Le travail F et les visites aval
restent donc les critères, en plus des bandes compactes. Les plans futurs
en vol devront être possédés jusqu'à leur dernier job, pas réempruntés à
un callback terminé ou reconstruits sur chaque tranche A_i×B.

**P0 reste la suppression du calcul systématique O(|A|²+|B|²) des
histogrammes de témoins locaux.** Les petits ensembles de témoins ne sont
pas imposés : plusieurs architectures sont en concurrence.

Immédiatement après, surveiller les cinq verrous ci-dessous. « Rédhibitoire »
signifie ici qu'on ne peut pas reconduire aveuglément cette organisation
pour viser nos contrats ; cela ne démontre pas une impossibilité de HGP.

| Repère | Verrou | Nature du problème |
| --- | --- | --- |
| B1 | Recherches de témoins constamment recommencées | Travail spatial cumulé non borné par le seul nombre de rectangles ou de succès |
| B2 | Triangles de départ × voisinages en q3/q4 | Produits de tailles intermédiaires, même lorsque beaucoup de candidates sont rejetées |
| B3 | Rattachements par MEB et recherches d'intrus répétées | Grosse constante liée à K et descentes de longueur variable |
| B4 | Histoire et export derrière un traitement central séquentiel | Limite de parallélisation et reconstructions répétées |
| B5 | Catalogues globaux, copies et aller-retour CPU/GPU | Résidence, trafic mémoire, formats et intégration industrielle |

Ces repères servent au suivi ; ils ne renomment pas P0 et ne prescrivent
pas cinq implémentations indépendantes. Une nouvelle structure peut
résoudre plusieurs verrous, ou déplacer du travail de l'un à l'autre.

## 2. B1 — ne pas repartir de zéro pour chaque recherche spatiale

**Constat.** Le compteur de témoins initialise sa pile avec la racine
de l'index pour chaque requête. Des rectangles proches ou apparentés
peuvent donc revisiter les mêmes régions. Voir
[witness_count.hpp](../../morsehgp3D_v7/src/spindle/witness_count.hpp),
lignes 53–125, et le [volet WSPD](../audits/WSPD_Q2_Q3_Q4.md), sections 4–5.

**Piège.** Le seuil h borne les succès nécessaires, pas les échecs
examinés pour les obtenir. Ni une pile courte, ni une WSPD de petite
taille, ni dix témoins recherchés ne donnent une borne faible sur le
nombre total de visites. Même la somme des tailles des facteurs est une
quantité différente du nombre de rectangles.

**À comparer.** Requêtes groupées, certificats de blocs réutilisés,
propagation d'identifiants parent→enfants et front plat à équipe persistante.
La propagation doit conserver les exclusions et éviter de créditer un
même site plusieurs fois ; un simple entier h transmis ne suffit pas.

**Avant de déclarer le verrou traité :**

- Vérifier les rejets et les cas indécis sur un petit juge indépendant,
  notamment absence de témoin, frontière et double crédit parent/enfant.
- Compter visites, échecs, blocs crédités, réemplois et pire requête,
  y compris le coût de préparation des certificats partagés.
- Comptabiliser tout déplacement vers davantage de rectangles ou de
  candidates et établir un gain net sur le travail total ; une hausse
  de leur nombre peut être acceptable si elle réduit suffisamment l'aval.

Une autre question reste ouverte : la borne de packing de la construction
WSPD réelle doit être justifiée. La borne classique Callahan–Kosaraju
ne se transfère pas automatiquement au trie Morton v7. C'est une lacune
de preuve à fermer, pas une preuve que ce trie est quadratique.

## 3. B2 — ne pas remplacer le carré des histogrammes par un carré de voisinages

**Constat.** En q3, chaque triangle de départ ayant passé les premiers
filtres peut parcourir le cover, c'est-à-dire la région qui contient les
sites à examiner. En q4, chaque triangle de départ peut encore payer des
parcours de ce cover et le tri de ses racines rationnelles. Voir
[generate.hpp](../../morsehgp3D_v7/src/pipeline/generate.hpp),
lignes 798–841 et 912–1180.

Noter S le nombre de triangles de départ traités et C_i le nombre de
sites de leur cover. Les parcours peuvent payer une somme des C_i sur
ces S triangles ; avec des covers comparables de taille C, cela représente
un travail de type S×C. Les tris q4 ajoutent leur propre coût. Ce constat
sur les boucles **n'est pas une nouvelle preuve d'une famille géométrique
quadratique en n à petite sortie**.

**Acquis à garder.** Le balayage q4 évite déjà de rescanner le cover pour
chaque tétraèdre de complétion. Le perdre serait une régression. Mais
distribuer les triangles entre threads ne réduit pas à lui seul S×C.

**À comparer.** Rejets collectifs avant développement des seeds,
résumés ou structures de requêtes partagés, profondeur q3 par blocs,
racines et scans q4 segmentés. Préserver le cover qui contient tous les
intérieurs utiles, pas seulement les sommets de complétion.

**Avant de déclarer le verrou traité :**

- Confronter q3/q4 à leurs oracles, avec racines égales, profondeur qui
  redescend, supports non positifs et témoin perdu par un cover trop étroit.
- Compter triangles de départ, incidences triangle–site effectivement
  testées, racines, comparaisons, complétions rejetées et boules retenues.
- Mesurer séparément réduction du travail et accélération parallèle ;
  un produit de tailles simplement distribué reste le même produit.

Un faible nombre de boules finalement retenues n'établit pas un faible
coût du générateur. C'est aussi pourquoi P0 doit payer son résidu aval.

## 4. B3 — l'exactitude n'impose pas la recherche lexicographique des supports

**Constat mesuré.** La tour 50k/K10 du 10 septembre paie 41 986 201 appels
MEB du résolveur et 3 898 856 828 essais de supports. Le constructeur FULL
CPU représente 389,668 s sur 418,873 s de sonde.
[Mesures et périmètre](../audits/CONTRATS_ET_MESURES.md#2-les-quatre-vrais-runs-50k-full).

Le [chercheur MEB](../../morsehgp3D_v7/src/forest/anchor_meb.hpp),
lignes 118–170, essaie des paires, triples puis quadruples. À dix sites,
375 supports sont possibles ; il s'arrête au premier succès certifié.
Le [résolveur statique](../../morsehgp3D_v7/src/forest/full_ball_tower.hpp),
lignes 574–627, peut enchaîner plusieurs MEB et recherches d'intrus.

**Distinction essentielle.** À K fixé, ces 375 possibilités sont une
constante, pas un O(n²) démontré. En revanche, la longueur des descentes
n'est pas bornée par K seul ; leur terminaison ne prouve pas leur faible
coût. Support de taille≤4 ne signifie pas quatre opérations.

**À comparer.** Propositions de supports issues du contexte géométrique,
certification exacte avec repli complet, réemploi des résultats et
regroupement par facettes entières. Ne pas certifier uniquement la clé
proposée : vérifier aussi positivité, confinement et coquille.

**Avant de déclarer le verrou traité :**

- Comparer la terminale géométrique avant sa normalisation historique ;
  une mauvaise terminale peut rejoindre la bonne racine finale.
- Conserver les mutants de semis, cas de même rayon/coquille et repli
  après proposition rejetée. Ne jamais tronquer une descente inachevée.
- Mesurer demandes totales/uniques, MEB, supports, puissances, intrus,
  distribution des longueurs de descente et coût des replis.
- Vérifier le coût total : moins d'essais MEB ne suffit pas si la nouvelle
  méthode paie davantage de recherches spatiales ou de préparation.

## 5. B4 — le calendrier séquentiel n'est pas la définition de la hiérarchie

**Constat.** Le Builder actif traite ordres et plateaux successivement.
Le prototype privé de graphes distribue de la géométrie, mais consomme
encore les résultats et construit l'histoire avec des étapes séquentielles.
Voir [l'audit d'implémentation](../audits/IMPLEMENTATION_PARALLELISATION.md),
sections 5–6, qui distingue ces deux chemins.

À 32k, le dernier prototype à rangs publié consomme 50,467 s pour les
histoires et 92,044 s pour l'export avec consultations. La reconstruction
d'histoire reste séquentielle ; l'export comporte déjà des consultations
parallèles. Ces chiffres montrent donc le poids de l'aval, **pas 142,5 s
de travail démontré strictement sériel**.
[Décomposition de cette capture](../audits/CONTRATS_ET_MESURES.md#4-triplets-locaux-complets--même-famille-versions-distinctes).

**À comparer.** Rattachements géométriques préparés indépendamment,
graphes datés par K, forêt minimale puis contraction parallèle,
contributions et verticales consultées dans les histoires achevées.
Réutiliser les marques valides et préparer chaque index historique
une seule fois, sans imposer tous les index simultanément en mémoire.

**Avant de déclarer le verrou traité :**

- Vérifier toutes les coupes sur petits cas, plateaux disjoints de même
  date, grandes multifusions, histoires en peigne et plateaux traversant
  plusieurs lots. Préserver parents, couvertures et verticales.
- Tester les marques forgées ou appartenant à une autre histoire ;
  leur plausibilité structurelle ne prouve pas leur provenance.
- Séparer les temps de résolution, consommation, construction d'histoire,
  consultations et export. Mesurer le chemin séquentiel restant.
- Montrer que le parallélisme porte sur la construction elle-même,
  pas seulement sur les requêtes d'un index construit en série.

Un tri rapide ne constitue ni une forêt minimale ni une histoire FULL.
La contraction parallèle proposée reste à adapter et qualifier ;
les performances d'un autre algorithme ne sont pas des temps HGP.

## 6. B5 — concevoir la résidence et les échanges, pas seulement les kernels

**Constats mesurés.** À 50k/K10, le processus CPU a un RSS de 16 206 376 KiB,
soit environ 15,5 Gio. Le census hybride mesure 0,189 s de kernels mais
4,540 s pour sa phase complète, dont 2,932 s de reconstruction hôte.
Ce sont des observations historiques : le RSS n'est ni une VRAM ni
un minimum mathématique. [Sources chiffrées](../audits/CONTRATS_ET_MESURES.md).

Fenêtrer les requêtes ne borne pas automatiquement la résidence du
catalogue, des candidates, de la forêt et des contributions conservées.
Passer des identifiants en 64 bits ne réduit pas ces volumes.

**À comparer.** Un propriétaire immuable des données communes, tableaux
plats et écritures à destinations calculées, références partagées au lieu
de copies, résidence GPU et échanges par lots lorsque nécessaires.
Spécifier le format FULL et la reprise séparément de l'archive F.

**Avant de déclarer le verrou traité :**

- Mesurer les pics réellement simultanés de RAM/VRAM, les capacités
  nommées séparément, les octets copiés/transférés et les allocations.
- Inclure préparation, transferts, synchronisations et reconstruction
  dans les temps de phase et de tour ; distinguer les seuls kernels.
- Tester les largeurs d'indices des objets dérivés, pas seulement celles
  des points, ainsi que les échecs d'allocation et la publication partielle.
- Traiter explicitement les grandes coquilles : le support≤4 ne justifie
  pas la limite active de coquille 12 ni le masque u16.
- Qualifier la complétion et la résidence aux tailles massives demandées ;
  une extrapolation depuis 50k ne constitue pas ce test.

Ne pas supprimer des nœuds FULL utiles pour réduire artificiellement
la mémoire ou le temps : changer de représentation ou de contrat doit
être explicite. Les copies évitables et la sortie requise sont distinctes.

## 7. Ce qui relève d'une limite mathématique, pas d'une mauvaise boucle

La [borne de sortie FULL](../audits/FONDEMENTS_ET_OBJET.md#8-le-sous-quadratique-universel-explicite-est-une-fausse-cible)
porte sur de vraies feuilles : dès K2, certaines familles régulières
exactes à précision croissante en ont un nombre quadratique en n.

Elle interdit de promettre l'énumération explicite universellement
sous-quadratique, indépendamment de la taille de sortie. Elle ne prouve
ni une asymptotique infinie dans l'univers u16 fini, ni une impossibilité
du contrat 1 s à 50k. Une représentation implicite exigerait son propre
contrat de requêtes et d'expansion, pas une substitution silencieuse.

Pour B1–B5, ne pas confondre ce coût nécessaire avec les interactions
rejetées, la répétition des recherches et les copies intermédiaires.

## 8. Feuille de route de reprise

1. Lire P0 et comparer ses architectures avant le port général de la v7.
2. Instrumenter les coûts B1/B2 dès cette comparaison : une meilleure
   présélection ne doit pas augmenter davantage leur travail.
3. Utiliser les petits oracles et une tranche FULL minimale pour vérifier
   l'effet aval ; traiter B3 et préparer les bons objets pour B4/B5.
4. Mesurer mono puis multi-CPU sur n=8 000/16 000/32 000, s=8/10/12,
   avec uniforme, amas équilibrés/déséquilibrés et géométries adverses.
5. Après raccord complet, qualifier la tour 50k 1..10 sous 1 s, avec repli
   sur 1..5, puis 100 ms si le jalon est atteint ; la grande échelle G4
   reste une qualification séparée sur sessions SPOT gardées.

Pour chaque décision : nommer l'objet exact conservé, le travail supprimé,
le résidu, les contre-fixtures et les métriques. Compter séparément
préparation, raffinements, candidates, validation, histoires et export.
Ne pas additionner des compteurs d'unités différentes ni des durées
GPU qui se recouvrent ; mesurer aussi le temps réel de bout en bout.

Toute nouvelle gate doit exercer un succès, un rejet ou mutant ciblé
et un plancher de non-vacuité : réemploi réellement utilisé pour B1,
passes q3/q4 effectivement traversées pour B2, proposition réussie et
repli pris pour B3. Pour B4, comparer aussi les coupes ouvertes/fermées
et les tableaux d'histoire, pas uniquement le digest. Pour B5, varier les
tailles de lots à sortie égale, injecter un échec après un lot réussi et
tester la reprise dans un nouveau processus. Les tests d'offsets aux
frontières peuvent éviter des allocations géantes ; ils ne qualifient
pas pour autant la résidence massive. Ces tests sont à porter, pas
présentés comme exécutés dans cette note.

Réduire huit tests à trois, réutiliser un index ou éviter une allocation
peut être utile. Aucun de ces gains constants ne prouve à lui seul la
résolution d'un produit de tailles, de la sérialisation ou de la résidence.

**État à la passation : P0 et B1–B5 ouverts.** Les propositions ne sont
pas des gains acquis ; les chiffres cités ne mesurent pas une v8 exécutée.
Les contrelectures du constructeur restent distinctes de l'auditeur
indépendant et les reçus anciens ne sont pas réécrits.
