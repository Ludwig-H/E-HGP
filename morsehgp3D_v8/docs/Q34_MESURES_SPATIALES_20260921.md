# Mesurer q3/q4 sur la scène, ses moitiés et ses quarts

21 septembre2026. Raccord du [protocole spatial](PROTOCOLE_LIDAR_SPATIAL_20260921.md)
au moteur34 gelé. Cadre : `exploration_v8_hors_registre`, `cpu_reference`,
`quantized_u16_input_only`, `implementation_v8_p0`, `not_claimed`.
Il s'agit du flux global des candidats q3/q4, **pas** du catalogue final,
du census des intérieurs pour FULL ou de la tour K-NN.
Le nouveau [contrat principal](CONTRAT_TRAMES_SEMANTICKITTI_20260921.md)
porte sur la tour d'une **trame entière**, sur plusieurs scènes et sur G4 ;
les présentes mesures CPU de composants ne peuvent pas le qualifier.

## Ce qui change, et ce qui ne change pas

Le nouveau [lanceur](../bench/run_q34_spatial.py) consomme les sept
fichiers entiers d'une préparation spatiale validée. `n=source_n` est
obligatoire, le hash du nuage est recalculé et le chemin de la commande
doit être exactement celui du morceau. Aucun ancien préfixe de taille
imposée n'est utilisé. Un morceau vide reste inscrit comme tel, sans
invocation native ni exposant inventé.

Chaque invocation native prépare son propre index et calcule toutes ses
candidates avec ses seuls sites. Les quatre quarts passent d'abord, puis
les deux moitiés, enfin la scène complète ; cet ordre ne modifie pas les
six relations parent/enfant analysées. Les dépendances à la densité et
les effets de frontière restent ceux de la vraie géométrie, sans correction
artificielle des sorties entre morceaux.

Le code C++ et les builds34 ne changent pas. Le lanceur vérifie son lien
avec les sources216 et les artefacts du build Release R2 qualifié ; les
nouveaux scripts et tests sont épinglés comme protocole séparé. La
validation de forme/compteurs reprend explicitement les contrôles31/34,
en remplaçant les seules hypothèses de nom de fichier et de préfixe.
Les lignes natives et leurs commandes ne sont pas maquillées pour passer
un ancien lecteur. Aucun verdict d'audit indépendant n'est hérité.

Configuration initiale : K5, s8, Local28, témoins rectangles puis paires,
bases affines, census q3 par boîtes et parcours q4 LiveOnly. Les autres
valeurs K5/10, s8/10/12 et parcours Individual/LiveOnly/Joined restent
explicites ; elles ne représentent pas des campagnes déjà exécutées.
Les campagnes sont séquentielles ; les processus indépendants déjà actifs
sur la machine ne sont pas arrêtés ni attribués au constructeur.

## Comparaisons et lecture des coûts

L'[analyseur](../bench/analyze_q34_spatial.py) conserve six comparaisons
par répétition : scène→chaque moitié, puis chaque moitié→ses deux quarts.
Il vérifie les configurations et la conservation des cardinalités.
Pour les compteurs entiers, la décision par rapport au carré compare
exactement `Wparent*Nenfant²` et `Wenfant*Nparent²` ; l'exposant logarithmique
n'est qu'un affichage. Effectifs identiques, morceaux vides et coûts nuls
restent publiés comme non estimables. Aucune série n'est fabriquée en
triant des quartiers indépendants par effectif.

Tous les compteurs sont conservés, avec les temps et capacités séparés.
Ne pas additionner les totaux et leurs sous-compteurs : les admissions,
crédits, issues de branche et masses candidates ne sont pas des tests
géométriques supplémentaires. En particulier :

- q3 : séparer génération, boules préparées, bornes du compte, bornes de
  coquille, tri et payload. Les enfants préparés mais non visités sont
  déjà compris dans `count_bounds_prepared` ;
- q4 : distinguer préparation de l'atlas, visites, tests de blocs/points,
  balayages et tris. `partition.node_visits` inclut aussi les nœuds
  laissés indécis par le budget de représentation ;
- Joined : ne pas ajouter les visites historiques des graines aux
  visites antichaîne/produits qui les contiennent déjà ;
- mémoire : un pic par worker ou une somme de pics n'est ni du travail,
  ni le pic simultané, ni le RSS.

Les sommes de coûts des quatre quarts et des deux moitiés sont publiées
séparément comme effet du découpage, pas comme un exposant de croissance.
Un résultat sous-quadratique sur ces relations est une observation du
régime testé, pas une preuve asymptotique ni une qualification des autres
scans. Des temps mesurés sous concurrence ne fondent pas un gain stable.

## Vérification et portée

La porte dédiée confronte les sept morceaux d'une petite scène à des
circonsphères calculées par élimination rationnelle indépendante, avec
profondeurs et coquilles globales. Le contrat q4 reste le regroupement par
arête propriétaire, graine et boule du flux34 ; ce n'est pas l'énumération
de toutes les incidences de supports cosphériques. Deux exécutions
Individual/W1 et LiveOnly/W2 contrôlent aussi la conservation des sorties.
Les tests du lecteur doivent refuser notamment un préfixe, le mauvais
morceau, un hash incohérent, des compteurs altérés ou une scène incomplète.

Les grandes scènes ne disposent pas d'un oracle exhaustif dans cette
campagne. Leurs preuves portent sur entrées, commandes, fermeture,
structure et registres ; les comparaisons de sorties entre configurations
se font uniquement à nuage identique, jamais parent contre enfant.
La référence scan0/K5/s8/W4 est maintenant close : sept mesures,
lectures et analyses normal/−O identiques. La trame complète119142sites
prend383,311s de pipeline q3/q4 ; ce n'est toujours pas FULL. Le
[bilan détaillé](../receipts/q34_spatial_20260921/README.md) publie les six
comparaisons. Malgré les bons rapports moitié/quart des postes dominants,
trame→moitié positive garde des exposants2,502 pour les bornes q3 et2,332
pour les bornes de partition q4. Aucun sous-quadratique de toute la chaîne
n'est donc acquis dans cette campagne. Les moitiés sont géométriquement
asymétriques ; ces nombres ne constituent pas davantage une borne globale.

Les deux portes locales, Python normal et `-O`, sont maintenant closes :
14 appels natifs chacune,238 records comparés,29 contrôles de l'analyseur.
L'oracle couvre sept nuages,688 triangles,1964 tétraèdres et3648 puissances
rationnelles. Ses54 tétraèdres positifs donnent50 groupes q4 canoniques :
le regroupement est réellement exercé. Les sources et artefacts n'ont pas
changé pendant ces deux vérifications. Ces portes ne sont pas des oracles
exhaustifs des grandes scènes.

## Première référence sur G4

Une session SPOT close mesure le même moteur gelé, K5/s8/LiveOnly,
avec48workers sur48CPU logiques (24cœurs/2SMT). Les trames entières
08/000000,08/000100 et08/000200 prennent respectivement
**165,214/34,319/505,479s**, le quart de contrôle45,594s. Ce sont des
résultats **CPU**, pas GPU ni FULL. Quatre portes natives et quatre
commandes terminées ; relectures normal/−O concordantes. Le quart et
la trame0 conservent les sorties et comptes géométriques locaux.
Les [reçus G4](../receipts/q34_spatial_20260921/README.md) contiennent
les sources, commandes, durées et l'arrêt ciblé certifié TERMINATED.

Sur les trois trames, occupation moyenne4,19/11,13/1,93CPU logiques
malgré48workers réellement démarrés. Les jobs Coarse sont répartis
dynamiquement, mais leur intérieur et les arêtes restent indivisibles.
Il faut donc à la fois réduire le travail et rendre les descendants
redistribuables. La campagne ne sépare pas encore la part temporelle de
chaque arête ; ne pas transformer ces moyennes en traces par worker.
Un unique passage par scène ne qualifie pas un gain stable. NiK10 ni
s10/s12 ne sont encore couverts sur les trames entières.

## Suite moteur

L'[audit A du relais q3](../audits/q3_prefix_relay_20260921/README.md)
valide la transmission privée du compte avec son curseur, puis le
traitement successif des racines du suffixe. Il ne valide pas encore
le partage par enveloppes de centres ni son gain. Le prochain port doit
préparer l'enveloppe une fois par bloc de graines, garder les graines
invalides dans les témoins, et collecter la coquille sur l'index global
après acceptation, même si le compte est déjà à EOF. Il ne doit pas
recommencer à la racine avec un crédit, empiler tout le suffixe dans
la pile49 ou transférer un compte q3 à q4.

Raccord à réaliser comme une tranche complète : nouvelle option
`SharedPrefix` dans `Engine::q3_edge`, **avant** `ExactBall::make_q3`.
Un cadre X garde son propre `(count,cursor)` ; une feuille Z ambiguë
force la division de X avant consommation. Le contexte possède index,
arête et seuilK−1. Les petites plages sont relayées avant la préparation
des centres. Préparer aussi les six sommets des paraboles une fois par X,
pas à chaque couple X×Z. Factoriser le census individuel en suffixes
successifs, avec l'entrée historique équivalente au ticket `(0,racine)`
et ses26 compteurs inchangés à cette couture. Le ticket ne doit pas être
une API publique librement fabricable.

Le registre partagé doit payer préparations, divisions rationnelles,
visites X×Z, divisions X, héritages et relais. Une plage saturée avant
examen des graines supprime une **population brute**, pas un nombre connu
de triangles aigus : ne pas l'ajouter artificiellement aux anciennes
graines ou profondeurs rejetées. Les produits croisés de fractions sont
interdits par la borne i128 ; utiliser quotients/restes. Garder l'arête
atomique dans la première comparaison, mais posséder son contexte pour
le futur partage des descendants X. Cette proposition n'est pas encore
implémentée ; son coût complet devra être comparé sur les mêmes découpes.
