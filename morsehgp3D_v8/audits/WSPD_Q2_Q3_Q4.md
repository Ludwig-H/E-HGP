# Audit de la génération : WSPD, q2, q3 et q4

13 septembre 2026. Contrelecture constructeur pour la v8, sans modification
du moteur v7, compilation, benchmark ni GCP. Cadre v8 :
`exploration_v8_hors_registre / none / quantized_u16_input_only /
audit_v7_math_and_architecture / not_claimed`.

## 1. Réponse directe à la question de départ

**Oui, les rectangles peuvent être traités indépendamment. La v7 le fait
déjà sur CPU.** Une requête de témoins ne modifie pas le nuage ni l'index,
et le corps d'un rectangle écrit dans le brouillon de son ouvrier.
L'appel `parallel_items(nrect, ...)` est déjà présent dans
[generate.hpp](../../morsehgp3D_v7/src/pipeline/generate.hpp), ligne 1289.

En revanche, un rectangle n'est pas une opération de coût constant :

- Son cœur peut nécessiter une longue recherche de témoins dans l'index.
- Ses histogrammes actuels parcourent les produits A×A et B×B.
- Ses paires survivantes peuvent posséder beaucoup de troisièmes sommets.
- En q4, chaque triangle de départ survivant peut encore parcourir un grand
  voisinage et trier ses propres racines rationnelles.

Le problème n'est donc pas principalement « comment rendre indépendants
les rectangles ? », mais **comment éviter leur travail intérieur inutile,
puis distribuer les rectangles et les tâches intérieures de longue durée**.
La v7 possède les premiers niveaux de parallélisme, pas cette distribution
massive à toutes les échelles. Aucun résultat ci-dessous n'acquiert le
contrat de la tour 50k en une seconde.

## 2. Quatre paramètres différents

| Symbole | Sens | Valeur pour la première cible |
| --- | --- | --- |
| K | Ordre d'une hiérarchie HGP | 1 à 10 |
| Kmax | Dernier ordre de la tour demandée | 10 |
| q | Nombre de points du support minimal positif d'une boule | 2, 3 ou 4 en 3D ; les naissances-points sont séparées |
| smax | Fenêtre géométrique de génération | Kmax+1, donc 11 |
| s | Séparation géométrique de la WSPD | 8, 10 ou 12 à comparer |

Une boule pertinente dans cette fenêtre vérifie p+q≤smax, où p est son
nombre de sites strictement intérieurs. Il suffit donc de certifier :

$$p\geq h_q=s_{\max}-q+1=K_{\max}+2-q.$$

| Tour | q2 | q3 | q4 |
| --- | ---: | ---: | ---: |
| K=1..10 | 10 témoins | 9 témoins | 8 témoins |
| K=1..5 | 5 témoins | 4 témoins | 3 témoins |

C'est exactement `lane_h` dans
[spindle.hpp](../../morsehgp3D_v7/src/spindle/spindle.hpp), lignes 47–50,
appelé par le générateur ligne 1242. L'idée « h=K témoins » est donc juste
pour q2 lorsque K désigne Kmax. L'appliquer uniformément à q3/q4 resterait
conservateur, mais demanderait des témoins inutiles et raterait des rejets.
Un seuil plus petit que celui du tableau exigerait un autre argument.

## 3. Le certificat géométrique acquis

Fixons une arête propriétaire maximale ab. Posons H=(z−a)·(b−z) et
Ξ=|(b−a)×(z−a)|². Les tests de la v7 sont :

$$W_2:\ H>0,\qquad W_3:\ H>0\ \text{et}\ 3H^2>\Xi,\qquad W_4:\ H>0\ \text{et}\ 2H^2>\Xi.$$

W2 est exactement la boule diamétrale ouverte de la paire fixée. W3 et W4
sont des régions suffisantes : un site qui y appartient est intérieur à
toute boule pertinente de l'arité possédée par ab. L'inverse n'est pas
affirmé. Les fuseaux sont emboîtés, mais leurs seuils de mort sont différents.

La [preuve indépendante du front](../../morsehgp3D_v7/audits/FRONT_ET_TEMOINS_COURANT.md)
déduit ces régions de la variance du support positif et de son arête maximale.
Elle justifie aussi l'autorité des coins par convexité séparée dans les
extrémités. Les [bornes arithmétiques](../../morsehgp3D_v7/audits/ARITHMETIQUE_SPINDLE_COURANTE.md)
couvrent H en i64 et les carrés/produits nécessaires en i128 sous le profil
u16. Un point du bord n'est jamais un témoin strict.

Pour A×B, les trois populations additionnées sont disjointes :

| Compte | Population | Ce qui doit être vrai |
| --- | --- | --- |
| h_cœur(A,B) | Nuage privé de A∪B | Le site est témoin pour toute paire du rectangle |
| h_a(a;B) | A privé de a | Le site est témoin pour tout b de B, a fixé |
| h_b(b;A) | B privé de b | Le site est témoin pour tout a de A, b fixé |

La mort est sûre lorsque leur somme atteint h_q. Il n'est pas nécessaire
que les témoins h_a soient identiques entre deux lignes : chaque paire doit
disposer du bon nombre d'identités distinctes.

Nuance importante : les tests universels portent sur les **boîtes continues**
des facteurs. Un test exact sur cette boîte peut rester conservateur pour
les seuls sites de A ou B. Ainsi « q2 exact » ne signifie pas que toutes
les paires q2 survivantes sont pertinentes. Le générateur émet directement
ces candidates, puis le census mesure leurs vrais intérieurs. En q3/q4,
la restriction aux fuseaux ajoute encore une différence entre certificat
suffisant et description de toutes les possibilités.

La [preuve S1](../../morsehgp3D_v7/audits/S1_COURANT.md), sections 1–6,
compose propriétaire, accès au support, rejets stricts et dédoublonnage.
Elle est conditionnelle au domaine arithmétique, à l'index et au succès
terminal complet. Elle ne donne pas un temps industriel.

## 4. Ce que les boucles exécutent réellement

Les repères de lignes ci-dessous désignent le fichier lu, non une nouvelle
version compilée. Les commentaires de tête parlent encore souvent de v6 :
la qualification v7 vient de ses preuves et reçus propres, pas de ces noms.

| Étape | Travail réellement écrit | Distribution actuelle |
| --- | --- | --- |
| WSPD pure | Tester séparation ; sinon scinder le facteur de plus grand diamètre de boîte | [wavefront.hpp](../../morsehgp3D_v7/src/wspd/wavefront.hpp), lignes 95–146, primitive séquentielle |
| Front fusionné avec témoins | Une requête initiale par tâche ; retirer les lanes mortes ; requête avec coins q3/q4 aux terminaux survivants | [generate.hpp](../../morsehgp3D_v7/src/pipeline/generate.hpp), lignes 326–484 ; tranches d'une vague en parallèle |
| Recherche de témoins | Repartir de la racine de l'index ; rejeter/créditer des sous-arbres ; sinon descendre et tester les coins aux feuilles | [witness_count.hpp](../../morsehgp3D_v7/src/spindle/witness_count.hpp), lignes 53–125 ; DFS local séquentiel par requête |
| Histogrammes | Pour chaque a, tester chaque z∈A privé de a ; symétriquement côté B | `generate.hpp`, lignes 494–510 ; séquentiel dans un rectangle et une lane |
| Sélection des paires | Sauter une ligne si h_a≥need ; sinon tester chaque b avec h_a+h_b≥need | `generate.hpp`, lignes 1332–1342 ; séquentiel dans le rectangle |
| q2 | Former clé et rayon de chaque paire survivante | `generate.hpp`, lignes 1350–1355 ; pas de cover q2 à ce stade |
| Covers q3/q4 | Une collecte de handles par rectangle/lane ; filtre de leurs sites par ancre, puis 32 seaux radiaux stables | `generate.hpp`, lignes 1320–1330 et 1384–1386 ; [edge_cover.hpp](../../morsehgp3D_v7/src/lanes/edge_cover.hpp), lignes 165–238 |
| Rejets par ancre | Fuseau, secteurs et éventuellement grille des centres | `generate.hpp`, lignes 663–751, 781–797 et 872–910 |
| q3 | Parcourir les troisièmes sommets aigus ; pour chaque seed survivant, compter sa profondeur en parcourant le cover | `generate.hpp`, lignes 798–841 ; boucle seed×cover, arrêt à h3 succès |
| q4, première passe | Pour chaque seed aigu, accumuler témoins communs de Jung et des morceaux de corde ; arrêter si le seed est entièrement rejeté | `generate.hpp`, lignes 912–1041 ; boucle seed×cover avec arrêt anticipé |
| q4, seconde passe | Recalculer P et B sur le cover ; former les racines admissibles ; trier ; regrouper les égalités ; tester les complétions à leur profondeur | `generate.hpp`, lignes 1043–1180 ; tri et balayage séquentiels par seed |
| Collecte finale | Brouillons par ouvrier, puis copie vers le vecteur global | `generate.hpp`, lignes 1404–1454 ; fusion finale hôte |

Le paramètre `need=h_q−h_cœur` est calculé ligne 1304. Les histogrammes
sont encore construits avant la branche `need==0` ; dans le chemin nominal
une lane dont le cœur atteint le seuil a déjà été retirée du masque, donc
ce détail n'explique pas les mesures de dizaines de secondes.

### Pourquoi le front peut payer des milliards de visites

Le front nominal entrelace construction de la WSPD et élimination : il
n'attend pas d'avoir matérialisé toute la WSPD pour chercher des témoins.
Cela permet de tuer tôt des produits entiers. Mais chaque tâche repart de
la racine de l'index, et ses enfants peuvent refaire des recherches proches.
Le seuil borne les **témoins trouvés**, pas les échecs examinés pour les
trouver. Lorsque peu de sites sont universels, le parcours peut être long.

En q3/q4, la première passe utilise des boules-cœurs bon marché. Seuls les
terminaux encore vivants paient les coins. À une feuille, jusqu'à 64 couples
de coins peuvent être examinés ; H et Ξ sont partagés entre q3 et q4.
Les masques retirés après crédit empêchent les doubles comptes de sous-arbres.
La pile en ligne évite ses allocations ordinaires, mais ne réduit pas le
nombre de nœuds visités. Voir
[l'optimisation de pile](../../morsehgp3D_v7/docs/OPTIMISATION_PILE_TEMOINS.md).

### Pourquoi q3/q4 ne sont pas seulement q2 avec un citron plus fin

Le citron permet de rejeter toute une famille, mais il faut traiter ce
qui survit. Une arête q2 détermine une boule unique. Une arête q3 peut
porter de nombreux triangles aigus. Une arête q4 peut porter de nombreux
triangles de départ et des complétions différentes.

Le cover doit contenir **tous les intérieurs**, pas seulement les sommets
éligibles à la complétion. Son coefficient est 3 en q3 et 4 en q4. Utiliser
3 pour q4 a déjà perdu des témoins : le contre-exemple est rappelé dans
`generate.hpp`, lignes 1312–1319, et justifié dans S1, section 2.
La lentille des sommets n'est donc pas un remplacement du cover q4.

La bonne économie déjà acquise en q4 est le balayage par seed : il évite
de recalculer la profondeur en rescannant le cover **pour chaque complétion**.
Le coût restant est de l'ordre de m log m plus les complétions, par seed
survivant de cover m, et non un coût constant par rectangle. Les racines
sont rationnelles et leurs comparaisons mobilisent des produits larges ;
ce tri n'est pas directement un tri radix de clés 32 bits.

La profondeur au groupe de racines est calculée après retrait des sorties
du groupe, avant ajout de ses entrées. Les incidents sont sur le bord et
ne comptent pas comme intérieurs. On ne peut pas arrêter un seed au premier
groupe profond : la profondeur peut redescendre. La contre-fixture à cinq
points et sa boule survivante sont conservées dans S1, section 7.

## 5. Ce que signifie réellement la taille d'une WSPD

Le théorème classique donne une WSPD de O(s³n) paires de **sous-ensembles**
en dimension trois, avec l'arbre et l'algorithme de construction requis.
Il ne donne pas O(s³n) paires de points développées. Le produit des tailles
de ses rectangles couvre au contraire toutes les paires de points.

L'article primaire de [Callahan et Kosaraju, 1995](https://graphics.stanford.edu/courses/cs468-03-winter/Papers/callahan_kosaraju.pdf),
sections 3–4, fonde sa borne sur un *fair split tree*. Sa section 5,
page imprimée 75, montre aussi que même la somme des tailles des facteurs
sur les rectangles peut être quadratique. L'indépendance des tâches et
la petite taille de leur liste ne bornent donc pas leur travail intérieur.

Deux obligations distinctes pour la v8 :

1. Fermer explicitement le raccord de packing du trie Morton employé et
   de la scission par diamètre de boîte serrée, ou employer une construction
   dont ce raccord est établi. Le commentaire `wavefront.hpp`, ligne 3,
   annonce O(s³n), mais
   [l'audit d'index](../../morsehgp3D_v7/audits/AUDIT_INDEX_20260905.md),
   dernière section, précise que la couverture par `cell_of_prefix` ne
   démontre pas ce packing. C'est une obligation de preuve, pas ici une
   réfutation de la borne pour ce trie.
2. Borner séparément les visites de témoins, les histogrammes, les covers,
   les incidences seed–site et les sorties. La partition exacte des paires
   et son bilan de masse n'établissent aucune de ces bornes.

Une limite de taille appliquée aux facteurs terminaux ferait développer
la WSPD en davantage de rectangles ; ce n'est pas un remède théorique au
travail quadratique. Découper **l'exécution** d'un rectangle en tâches bornées
sans changer sa propriété WSPD est une autre opération, utile pour l'équilibrage.

## 6. Augmenter s : utile, mais pas automatiquement plus rapide

Les facteurs deviennent petits relativement à leur séparation. Cela réduit
l'incertitude sur les extrémités et peut renforcer leurs certificats communs.
Mais le nombre de rectangles et de requêtes tend à augmenter : il faut
mesurer ce compromis, pas seulement le nombre de candidats finaux.

La v7 refuse s<8 dans son profil produit. Le commentaire de
[wavefront.hpp](../../morsehgp3D_v7/src/wspd/wavefront.hpp), lignes 35–42,
dit explicitement que c'est une marge, **pas une condition d'exactitude**.
Avec M le maximum des rayons des boîtes, une borne du rayon-cœur est
(κ_q s−1)M. Demander un rayon supérieur à M conduit en q4 à
s>2/sin(15°), soit environ 7,727. Cela ne garantit ni l'existence de
h_q sites dans ce cœur ni une bonne performance. Modifier ce profil reste
un choix v8 à documenter et tester, pas une correction appliquée par cet audit.

La comparaison existante sur le même uniforme 8k donne :

| Mesure historique, une exécution par s | s=8 | s=10 | s=12 |
| --- | ---: | ---: | ---: |
| Visites de témoins WSPD | 563 616 452 | 625 850 731 | 664 703 087 |
| Couples de coins évalués | 167 115 088 | 110 073 830 | 76 064 822 |
| Temps du front | 28,938 s | 31,358 s | 32,890 s |
| Temps de génération | 58,698 s | 60,038 s | 62,025 s |

Les dix objets horizontaux concordaient ; il s'agit du
[reçu historique du 6 septembre](../../morsehgp3D_v7/receipts/full_wspd_q2_separation_20260906/README.md),
pas d'une mesure de la future v8 ni d'une qualification inter-K.
Le citron plus fin motive bien la comparaison s=8/10/12, mais ces mesures
n'autorisent pas à choisir systématiquement le plus grand s.

## 7. Acquis, essais négatifs et trous encore ouverts

| Piste | Ce qui est acquis dans les preuves/reçus v7 | Limite actuelle |
| --- | --- | --- |
| Front fusionné, masques, populations disjointes | Preuves du front et d'index ; qualifications bornées de la descente et de l'arithmétique | Pas de borne globale du travail ni de kernel nominal de génération exécuté sur GPU |
| Réutiliser le compte q2 au terminal | Intégré ; différentiel O2/SAN de 174 appels, mutant du cœur positif et CTests ciblés | Aucun recul des visites du cas 8k : q3/q4 imposent encore les parcours |
| Un seul passage systématique avec coins | Mêmes 754 686 rectangles sur le cas uniforme 8k | Front 37,767→38,287 s ; visites en baisse mais coins 167,115→335,510 millions : non retenu |
| Histogrammes par blocs positifs | 126 comparaisons O2/SAN et 8 436 096 valeurs égales à 8k | À petits facteurs, scalaire 93,819 ms contre blocs forcés 186,560 ms ; non intégré |
| Grands facteurs, blocs positifs | Valeurs identiques sur le triplet deux amas | Gain constant insuffisant ; q4 ralentit à 32k ; ne ferme pas le régime quadratique |
| Rejets négatifs + saturation à need | 432 comparaisons O2/SAN, mutation Ξ_max réfutée | Helper privé ; gain sur grands facteurs et raccord produit non mesurés |
| Sélection stable par classes de crédit | Preuve O(|A|+|B|+need+survivants) après histogrammes et contre-modèles | Pas de qualification C++ ni gain intégré |
| Front hôte optionnel par lots | 431 010 contrôles O2/SAN, 1 728 cas, 311 968 requêtes | Non appelé par le générateur nominal |
| Batch device de témoins | Stub hôte et compilation/lien NVCC 12.9/sm_120 | Aucun lancement CUDA ; un thread prévu par rectangle reste exposé aux longues tâches |

Les reçus correspondants sont rassemblés dans
[l'étude d'élimination](../../morsehgp3D_v7/docs/ELIMINATION_BLOCS_WSPD.md),
et détaillés pour le
[passage unique négatif](../../morsehgp3D_v7/receipts/wspd_terminal_once_negative_20260906/README.md),
les [histogrammes par blocs](../../morsehgp3D_v7/receipts/wspd_histogram_blocks_20260906/README.md),
les [grands facteurs](../../morsehgp3D_v7/receipts/wspd_large_factor_histograms_20260906/README.md),
le [rejet négatif saturé](../../morsehgp3D_v7/receipts/wspd_noncredit_saturation_20260906/README.md),
le [front par lots](../../morsehgp3D_v7/receipts/witness_front_20260910/README.md)
et le [batch device](../../morsehgp3D_v7/receipts/wspd_device_batch_20260910/README.md).
Ce tableau relit ces résultats ; il ne prétend pas les avoir réexécutés.

Le cas deux amas est particulièrement révélateur. Le rectangle racine
séparé couvre tout le nuage, donc son cœur extérieur est vide. Les trois
histogrammes scalaires de ce **seul rectangle** prennent respectivement
2,042 s, 8,393 s et 31,077 s à n=8k,16k,32k. Les blocs positifs prennent
1,462 s, 6,333 s et 27,092 s. Le seuil faible ne borne pas les échecs
examinés, et une tâche de cette taille occupe un seul ouvrier nominal.
Ce témoin ne dit pas que l'uniforme est dominé par les histogrammes : ses
facteurs observés ne dépassaient pas sept points et ce poste y coûtait
environ un dixième de seconde.

Autre échelle, autre reçu : l'étude d'élimination, lignes 230–235, rappelle
un historique CPU48 à 50k, s8, K10 avec 7,639 s de front et 4,621 s de
rectangles, 4 946 403 888 visites de témoins, 1 293 436 130 évaluations
de coins, 1 536 766 250 tests de cœur q4 et 276 996 927 complétions.
Ce sont déjà des postes amont excédant la seconde ; un tri final gratuit
ne suffirait donc pas à atteindre le contrat de cette exécution. Ces
composants historiques ne constituent pas une tour complète qualifiée.

## 8. Les faux raccourcis à ne pas réintroduire

1. **Additionner des comptes dont les populations se recouvrent.** Pour
   A={0,1}, B={10,11}, l'enfant {0}×{11} peut compter 1 et 10 dans son
   nouveau cœur et dans les anciens histogrammes : quatre crédits pour
   deux sites. Garder les exclusions parentales, reconstruire des populations
   disjointes ou prendre un maximum de minorants recouvrants.
2. **Remplacer les histogrammes par leurs minima sur tout le rectangle.**
   Les deux minima globaux valent zéro : l'extrémité de A la plus proche
   d'un b fixé ne peut posséder un témoin interne à A dans W2. Il faut
   utiliser des sous-groupes de lignes/colonnes, pas leurs minima globaux.
3. **Saturer h_b au seuil particulier d'une ligne puis le réutiliser.**
   Saturer au need global est sûr ; le cap local peut laisser vivre une
   paire qui devrait être rejetée. La contre-fixture A={0,1},
   B={100,101,102}, q2/smax=3 est conservée.
4. **Employer un rejet universel de boîte comme rejet de toutes ses ancres.**
   `hmax4_boxes(U,B,Z)≤0` peut seulement montrer qu'un site n'est pas
   universel lorsque U varie. Cela ne dit pas qu'il échoue pour chaque a.
5. **Compter les sites exclus d'un histogramme comme des ancres mortes.**
   Le rejet négatif dit que Z ne contribue pas au certificat, pas que
   toutes les boules de l'ancre sont trop profondes.
6. **Réduire le cover q4 à la lentille ou arrêter à un premier groupe profond.**
   Cela perd respectivement des témoins ou une complétion ultérieure valide.

Les preuves et contre-fixtures des cinq premiers points sont dans
[ELIMINATION_BLOCS_WSPD](../../morsehgp3D_v7/docs/ELIMINATION_BLOCS_WSPD.md),
sections 1–4 ; celles du dernier point sont dans S1.

## 9. Proposition de refonte, par priorité

### A. Réduire le travail du front avant de le déporter

**A1 — Chercher quelques témoins probables, puis les certifier exactement.**
Un petit ensemble de sites proposé par proximité du centre du cœur ou par
une liste associée à l'index peut être essayé d'abord. Chaque identité
distincte doit satisfaire le prédicat universel strict et les exclusions.
Si h_q sites sont certifiés, tout le rectangle est éliminé. Sinon, poursuivre
par le parcours exact existant : l'absence de succès de ce raccourci ne
peut entraîner aucune mort. La borne sur le nombre d'essais rapides est
un choix d'ordonnancement, pas une limite sur le calcul exact. Proposition,
pas preuve d'un taux de rejet suffisant.

**A2 — Propager les identités des témoins déjà acquis vers les enfants.**
Pour une lane survivante, moins de h_q témoins suffisent à représenter le
crédit acquis. Si A′⊆A et B′⊆B, un témoin extérieur à A∪B et universel
sur A×B reste extérieur et universel sur A′×B′. Transmettre ses IDs est
donc géométriquement sûr. Le nouveau parcours doit exclure exactement ces
IDs de tout crédit additionnel ; il peut alors découvrir d'autres sites,
notamment ceux devenus extérieurs aux facteurs enfants. Ce n'est pas
l'addition interdite d'un cœur enfant aux histogrammes parentaux.

Il faut concevoir le représentant borné des crédits de sous-arbre, les
masques par lane, la déduplication et les conventions de positions/poids.
Sur le profil produit sans positions répétées, un crédit survivant contient
au plus h_q−1 IDs. Cette proposition n'a pas reçu ici de contrelecture
indépendante, de fixture ni de mesure ; elle peut coûter plus qu'elle ne
fait économiser lorsque presque aucun témoin n'est réutilisé.

**A3 — Séparer décision, compte et écriture des vagues.**
Réutiliser le front optionnel par lots. Une décision par rectangle, puis
des préfixes allouent enfants et terminaux à des offsets disjoints. Cela
évite les copies de shards successives et conserve un ordre stable sans
ordre d'achèvement imposé. Un pool persistant évite de recréer les threads
à chaque vague : [pool.hpp](../../morsehgp3D_v7/src/parallel/pool.hpp),
lignes 60–86 et 140–161, construit aujourd'hui un groupe par appel.
Les gains restent à mesurer séparément des requêtes géométriques.

### B. Corriger le régime des grands facteurs

**B1 — Saturation globale, crédits positifs et rejets négatifs ensemble.**
Repartir des helpers déjà qualifiés, mais ne pas présenter le crédit
positif seul comme solution : le triplet deux amas le réfute économiquement.
Conserver le scalaire sur les petits facteurs ; les tests de blocs sont
plus coûteux et n'y sont pas amortis.

**B2 — Énumérer les seuls produits de classes de crédit admissibles.**
Après histogrammes saturés à need, classer les colonnes selon h_b et
réutiliser leurs listes stables par seuil. La sélection devient sensible
au nombre de survivants ; elle ne fait pas repayer les paires rejetées.
Qualifier le procédé stable déjà prouvé, sa mémoire par ouvrier et son ordre.

**B3 — Découper une tâche lourde sans raffiner artificiellement la WSPD.**
Créer des tâches de lignes d'histogramme, puis des tuiles implicites de
produits survivants. L'objet A×B et ses exclusions restent identiques.
La décision de distribution peut utiliser les tailles des facteurs et
la charge observée, sans changer un prédicat géométrique ni tronquer le
résultat. Ne jamais matérialiser A², B² ou toutes les paires pour préparer
le parallélisme.

### C. Changer les unités de travail de q3 et q4

**C1 — Covers partagés, tâches de seeds.** Les handles et le cover de
l'ancre sont immuables et partagés. Les seeds référencent ce stockage au
lieu de le copier. Distribuer les grosses ancres en seeds empêche une
seule tâche de monopoliser un CPU ou un thread GPU.

**C2 — Profondeurs q3 par segments de sites.** Les tests d'intérieur sont
indépendants une fois la forme du triangle fixée. Une réduction saturée
par seed est possible ; les sorties anticipées et leur perte éventuelle
d'efficacité doivent être comparées à la voie séquentielle, particulièrement
lorsque les premiers témoins suffisent.

**C3 — q4 : racines segmentées, puis préfixes/suffixes par groupe.** Les
P(z), B(z), racines et contributions constantes se calculent indépendamment
par site du cover. Après tri exact segmenté et regroupement des racines
égales, la profondeur de chaque groupe se déduit d'un préfixe exclusif
d'entrées et d'un suffixe exclusif de sorties, plus le compte constant.
Les tests de complétion redeviennent indépendants. Les coordonnées et
racines exactes demeurent l'autorité ; une clé flottante ne suffit pas à
décider l'égalité. L'architecture v7 avait déjà identifié cette couture
dans [OBJETS_PARALLELES_TOUR](../../morsehgp3D_v7/docs/OBJETS_PARALLELES_TOUR_20260911.md),
section 4 ; le générateur nominal ne l'exécute pas encore.

**C4 — Sauter les cascades des groupes profonds.** La profondeur actuelle
est connue avant les tests d'ownership, de positivité et de Cramer. L'avis
statique favorable de S1, section 7, permet de sauter leur groupe quand il
est profond, tout en conservant les mises à jour et les groupes suivants.
Ce delta ne supprime ni racines, ni tri, ni coût de la première passe ; ses
compteurs de groupes/racines doivent rester distincts des anciens rejets.

### D. Le GPU après ces séparations d'objets

Traiter un rectangle complet par thread n'est qu'un premier port : les
longueurs de parcours varient, les accès à l'index sont indirects et les
longues tâches ne sont pas subdivisées. Un ordonnanceur à plusieurs tailles
de tâches peut réserver un thread aux petites requêtes, un groupe coopératif
aux longues, et distribuer les seeds/covers séparément. Une telle politique
ne peut être choisie sur le seul nombre de rectangles.

Garder l'index résident, des files bornées de requêtes et des écritures à
offsets disjoints ; pas d'appel CUDA par rectangle, pas de matrice
rectangle×nœud visité, pas de duplication de cover par seed. Les garanties
mathématiques ne changent pas, mais les comparaisons exactes, les égalités,
les identités et la fermeture des lots doivent être requalifiées sur carte.

## 10. Ce qu'il faudra mesurer pour choisir, et non seulement espérer

Pour chaque s=8/10/12, rejouer n=8k/16k/32k sur au moins les régimes uniforme,
deux amas à grands facteurs, géométrie mince et coquilles/plateaux adverses.
Les courbes de temps doivent être accompagnées de :

- rectangles visités/émis/tués par lane, niveaux et pic de vague ;
- visites de témoins, coins, succès et échecs, réutilisations d'IDs ;
- histogrammes : nœuds visités, crédits, rejets négatifs, positions réellement
  testées et positions non visitées après saturation ;
- paires sélectionnées et rejetées sans développement, tailles des facteurs ;
- distributions et maxima des covers, des seeds et des sites par seed ;
- q3 : tests de profondeur ; q4 : première passe, seconde passe, racines,
  comparaisons, groupes et cascades ;
- débit, longue traîne des durées de tâches, attente des workers et coûts
  de préparation/copie ;
- résidence réelle et capacités, sorties finales, temps de tour complet.

Comparer mono puis quelques CPU, enfin GPU. Les identités de boules et les
forêts doivent rester appariées, mais les compteurs de travail physique
doivent pouvoir changer : c'est précisément l'objectif. Le contractuel
50k exige toute la tour ; le régime plusieurs dizaines de millions de
points exige en plus une architecture de résidence et un coût compatible
avec sa sortie. Une multiplication de débit GPU ne prouve pas que le
travail devient sous-quadratique.

## 11. Périmètre de cette contrelecture

Lecture complète de `wavefront.hpp`, `spindle.hpp`, `witness_count.hpp`,
`q2.hpp`, `q3.hpp`, `q4.hpp`, `chord_kill.hpp`, `parallel/pool.hpp`, des
preuves front/S1/index/arithmétique spindle, de l'étude d'élimination et
des README de reçus cités. Lecture des chemins chauds de `generate.hpp`,
de la collecte de covers et des repères de secteurs/grilles ; ceci n'est
pas un nouvel audit ligne par ligne de tous les filtres flottants,
cellules, appels indirects et tests de la v7. Les qualificatifs « acquis »
ci-dessus signifient acquis dans les preuves/reçus nommés, sous leurs
prémisses, pas une promotion automatique en v8.

La preuve de complétude FULL, l'extraction des parents, l'export et le
statut global des contrats sont traités dans les autres volets de l'audit.
Aucun fichier v7 ni reçu antérieur n'a été modifié par cette contrelecture.
GCP non utilisé.
