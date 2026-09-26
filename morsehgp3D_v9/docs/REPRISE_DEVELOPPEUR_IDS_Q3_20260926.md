# Reprise du développement — transport exact des intérieurs q3

26 septembre 2026, demande utilisateur : « Prends la place du développeur ;
tu as feu vert pour GCP G4 ». Cette demande remplace le rôle d'auditeur
seul pour cette tranche. Chantier sur `main`, pas de nouvelle branche.

## Base préservée

Le lot d'audit `fce85d823` est publié. Les quatre prototypes et leurs
captures restent inchangés ; les conclusions sont dans
[`AUDIT_B_RACCORD_PAYLOAD_CATALOGUE_20260926.md`](../audits/AUDIT_B_RACCORD_PAYLOAD_CATALOGUE_20260926.md).
Le travail v29 du précédent développeur (`3cf62b8ca`, sonde multitrames)
a été repris par cherry-pick, sans toucher à son worktree ni à ses
modifications v6 étrangères à cette tranche. Ses correctifs non publiés
du worker/autotest sont relus et repris explicitement avant v30.

Profil du contrat : grille entière 1 mm, FULL explicite K1..5 sans sol
prioritaire, G4 SPOT ; K10, trames brutes, s8/10/12 et croissance restent
des comparaisons séparées. Aucun nouveau contrat acquis par ce chantier.

Cadre : `phase=exploration_v9_hors_registre`,
`backend=cpu_reference` pour les portes locales, `cuda_g4` seulement pour
les exécutions CUDA reçues ; `profile=quantized_u18_input_only`,
`mode=implementation_q3_interior_payload`, `public_status=not_claimed`.

## Première implémentation

Levier `q3_interior_payload`, désactivé par défaut tant que son gain net
n'est pas mesuré. Trois éléments inséparables :

1. Conserver les IDs originaux pendant le census q3 existant, sans nouveau
   test géométrique. Sidecar possédé, `LaneRecord` restant à 128 octets.
2. Suivre exactement les placements, tâches, reports, staging, compactage
   et permutations de chaque record jusqu'à la présentation canonique.
3. Importer les boules régulières à partir de ces IDs complets et des
   comptes certifiés du producteur ; garder le census global pour q4,
   les coquilles étendues et les replis CPU sans paquet.

Le coût de collecte, synchronisation, mémoire, transfert, conversion et
import reste dans le temps de chaîne. La raison publiée doit distinguer
census global indépendant et validation locale du paquet ; pas de faux
sceau annonçant un second census absent. Le juge existant des voies
s'étend aux IDs et à la `BallData` comparée au census global.

La confiance reste celle du producteur interne : une liste accompagnée
d'un compte arbitrairement falsifié n'est pas un certificat externe de
complétude. Le nombre d'intérieurs et la taille de coquille sont certifiés
par le parcours q3 existant ; les IDs importés sont distincts et leurs
puissances revérifiées. Une coquille de trois supports et exactement autant
d'IDs que d'intérieurs suffit alors. Les paquets q4 ne sont pas importés.

## Vérifications avant la dépense G4

- CPU : objets/IDs/comptes exacts contre témoin, K2/3/5/10, coquilles
  supplémentaires, permutations, rejet après préfixe, limites de chunks,
  capacités/différés, voie fusionnée et non fusionnée ; mutants causaux.
- Catalogue/FULL : boules et tours explicites identiques, sceau activé
  et désactivé, q2 précoce, erreurs de plus petite clé, Euler et verticales.
- Release puis ASan/UBSan dans des builds neufs ; aucune réutilisation
  en écriture des builds de captures antérieures.
- Protocole G4 commité, préflight natif et lecteurs normal/−O ; chaque
  trame résidente publie statut et trois condensés, pas seulement celui
  de la tour.

La [séance G4 close](../receipts/g4_q3_payload_20260926/README.md) compare
le levier ON/OFF sur trames entières et au témoin moteur, avec les bras
alternés de 00/K5 et toutes les régressions conservées. Elle utilise le
contrôleur existant, l'arrêt ciblé et la relecture TERMINATED de la
génération exacte. Le paquet est construit depuis `f9f273bb0` ; ce n'est
pas une campagne supplémentaire sur tous les anciens leviers.

## Résultats locaux clos

Le [reçu natif](../receipts/q3_payload_native_20260926/README.md) ferme
six portes Release, deux sous ASan/UBSan/LSan et trois anciennes portes
de non-régression. Les 31 tests du protocole corrigé passent en normal
et sous `-O`, après conservation du premier échec de fixture.

La [capture FULL locale](../receipts/q3_payload_local_20260926/README.md)
compare dix paires : uniforme, terrain et amas à 8k/16k/32k, puis la
trame 08/000000 sans sol entière. Les objets et travaux géométriques
ON/OFF sont identiques ; 5 890 091 clés et 11 642 678 IDs sont importés
au total. Sur LiDAR : chaîne 27,010 → 26,528 s, census 746,509 →
447,227 ms, visites 100,69 → 45,70 millions. Une seule observation
par cas, hôte partagé, quatre workers ; ce n'est pas un gain stable.
L15 est actif ici et inactif dans le plan G4 : aucun transfert de gain.
La régression uniforme 32k (20,786 → 22,399 s) est conservée.

Les amas développent 28,35 / 112,77 / 449,65 millions de paires :
croissance pratiquement quadratique, malgré un mur un peu moins croissant.
Certains replis q3/q4 dépassent également ×4 au dernier doublement.
Ce port supprime du travail de catalogue ; il ne résout pas ce verrou
du générateur et ne prouve pas une croissance sous-quadratique LiDAR.

## Résultats G4 clos et décision

26/26 cas terminent : 18 GPU et huit témoins moteur, 19 comparaisons
croisées égales et neuf paires ON/OFF. La gate CUDA réelle exerce aussi
les transports fusionnés/non fusionnés, permutations et reports. Les
sources, dépendances et binaires sont stables ; lecteurs normal/−O passent.
Il s'agit toujours d'exactitude relative aux témoins, `not_claimed`.

Sur 00/K5/s8, deux processus par bras : chaîne médiane 927,789→922,663 ms,
census 102,542→82,825 ms, q34 488,485→496,856 ms et tour
284,341→291,079 ms. Les différences de tour, dont le travail est inchangé,
illustrent le bruit et ne s'attribuent pas mécaniquement au payload.
Les deux différences appariées de chaîne sont −11,474 et +1,222 ms.
Sur 02/K5, ON est aussi plus lent de 4,533 ms. **Le défaut reste OFF.**

La collecte supprime pourtant un vrai travail : sur 00/K5, 691 282 clés
et 1 351 657 IDs sont importés, une seule clé q3 se replie ; visites
globales 100 689 614→45 695 806. Le gain net modeste indique que le port
ne suffit pas à lui seul. Les autres bras, s10/s12, K10 et brut sont tous
dans le reçu, sans ne retenir que leurs meilleurs temps.

Les trois trames sans sol K5 restent sous une seconde de chaîne dans cette
séance ; seulement la séquence 08, entrées 1 mm et masques figés. Ni 100 ms,
ni plusieurs séquences, ni segmentation/lecture comprise ne sont qualifiés.
Les échecs initiaux de transport sont archivés séparément, jamais promus en
réussites. Une unique campagne identique a été relancée ; trois générations
SPOT sont closes, allocation cumulée 737,423 s, sans facture estimée.

## Suite structurelle

### Ce que ce port peut et ne peut pas changer à la croissance

Il n'ajoute aucun nouveau couple ou support candidat. La collecte réutilise
les masques de chunks déjà parcourus ; le stockage est borné par K−2 IDs
par record q3 et un petit scratch par groupe. Le transport actuel réserve
également ce stride pour les records q4 (sentinelles inutilisées) : ce coût
est réel. L'import ajoute un tableau de rangs de taille n et au plus K
tests ponctuels par clé régulière importée, à la place d'une recherche
globale. Le contrôle exact de chaque ID reste volontairement payé.

Ces bornes ne rendent pas sous-quadratique un front qui produit déjà
quadratiquement trop de paires. La première série FULL locale expose
justement les amas : 28,35 M puis 112,77 M paires à 8k et 16k, soit ×3,98.
Sur terrain synthétique, q4 émet 1 141 / 4 646 / 19 429 sorties à
8k/16k/32k (ratios >4), même si les principaux coûts restent inférieurs
au quadruplement. Il faut publier ces postes, pas seulement le total ou
la baisse du census. Ce sont des observations finies, pas des bornes
asymptotiques ni des mesures LiDAR. La géométrie ON/OFF est inchangée.

Après la mesure du raccord, prioriser le front GPU compact, le travail
résiduel q34 avant expansion et la construction événementielle FULL de
tous les K. Le port q4 par lentilles + masques T1 est prêt à être spécifié,
mais n'est pas automatiquement la prochaine modification : sur R24-B
00/K5, même supprimer les 100,273 ms de **tout** le census tardif laisserait
816,031 ms de chaîne, à autres phases figées. q4 ne représente que 18,65 %
des clés tardives ; cela ne fournit pas son temps, le coût par clé pouvant
varier. Les 98,6 ms de front, 282,7 ms de tour et 211,6 ms de noyaux
S2/S3/S4 restent des obstacles indépendants au contrat 100 ms. Les
surcoûts du transport q4 doivent être mesurés avant son port systématique.
La voie q4 triée reste une option pour les longues listes difficiles :
son test dense montre qu'elle peut perdre contre le rejet précoce.
Les améliorations doivent être mesurées sur le travail complet et les
coupes spatiales LiDAR ; la borne d'une primitive ne qualifie pas la
croissance de toute la chaîne.

## Prochain verrou du front

Lecture ciblée au `f9f273bb0`, sans nouveau benchmark ni modification du
moteur. La [capture FULL locale](../receipts/q3_payload_local_20260926/README.md)
montre 28,35 / 112,77 / 449,65 M expansions q34 sur les huit amas à
8k/16k/32k : environ 0,44 n². Le cache de témoins par tuile peut réduire
leurs visites, pas cette masse. Sur la trame sans sol 08/000000/K5/s8,
23,69 M paires sont développées pour 2,04 M survivantes S2. Ce sont deux
raisons concrètes de traiter les produits **avant** leur expansion ; pas
une promesse de gain sur toutes les trames.

**Ne pas réinventer P0.** `gen/pipeline/local_credits.cpp:36,383` possède
déjà les crédits Pool q2/q3/q4, puis leur regroupement ;
`tube_credits.hpp` possède déjà projections, bornes transverses et suffixes
pour les trois voies. En revanche le raccord aux vrais nœuds/rangs de
l'index global, sans préparation locale du nuage, existe seulement pour
q2 dans `q2_node_pool.hpp:58,167`. Les plages du vieux `PreparedRectangle`
sont des plages d'IDs : leur passer directement une plage de rangs Morton
serait faux. La priorité est un **plan q34 possédé par rectangle réel**,
pas un nouveau calcul de crédits q2 ou une reconstruction par job.

Comparer trois sélections, avec le même raccord et tout leur coût payé :

- Pool existant adapté aux nœuds : sélection des O(K) plus fortes
  projections dans chaque facteur, puis certification des ancres. Une
  sélection commune peut servir q3 et q4 ; une projection ne certifie rien.
- [Palette octant par ancre](../audits/SHADOW_HA_OCTANT_Q34_LIDAR_20260923.md),
  préparée une fois : elle a supprimé 5,73 M / 23,69 M paires sur cette
  trame K5, mais zéro cover, et seulement 0,193 CPU·s net sur 08/000100.
  Ses résultats ne qualifient ni le raccord FULL ni les amas.
- Tubes existants : tri O(m log m), balayages linéaires partagés. Leur
  largeur actuelle `4*maximum_component` peut produire des cellules pauvres ;
  ne pas présumer leur sélectivité sur un LiDAR ou des amas 3D. Plusieurs
  largeurs/projections sont plusieurs préparations à compter ; prendre le
  maximum de crédits de palettes concurrentes, pas leur somme sans preuve.

**Invariant de rejet.** Pour chaque voie séparément, un crédit extérieur h
doit porter ses vrais IDs ou nœuds d'une antichaîne du même index, avec
plages disjointes d'A et B. Le filtre actuel ne renvoie que son masque :
on ne récupère pas un h implicite en regardant ce masque. Commencer avec
h=0 reste sûr ; réutiliser ensuite les admissions effectivement tracées.
Pour a, h_a compte des sites distincts de **A privé de a**, certifiés par
`universal_witness(q,a,box(B),z)` ; symétriquement h_b ne compte que B privé
de b. Les trois populations sont ainsi réellement disjointes. La somme
h+h_a+h_b rejette q3 à K−1 et q4 à K−2, jamais par addition entre voies.
Les huit coins et les signes stricts de `spindle/predicates.hpp:161`
certifient toutes les extensions positives **possédées par l'arête ab** ;
le seul minimum affine de H employé en q2 ne suffit pas en q3/q4. Un
minimum/majorant numérique préparé doit conserver cette preuve et son coût.
Les contacts restent indécis, le propriétaire canonique de plus longue
arête reste inchangé, et aucun crédit ne précharge le census exact.

**Raccord structurel.** CPU : intercaler le plan avant les doubles boucles
`wspd_q34.cpp:1502` (batch) et `:541` (moteur). GPU : après
`rectangle_kernel` (`filter_runner.cu:112`) mais **avant** de fixer les
masses/prefixes et de réserver un masque par paire (`:417–432`, analogue
résident), remplacer le produit complet par des descripteurs résiduels.
Grouper les deux crédits q3/q4 conjointement et donner à chaque sous-produit
son masque évite d'émettre une paire deux fois. Les buffers de rangs et
preuves appartiennent au plan immuable partagé, jamais à une pile ou à une
slab réutilisée ; les workers consomment des handles sans rescanner les
facteurs. Garder l'ordre public rectangle/ligne, ou payer explicitement une
remise en ordre des seules survivantes. Modifier uniquement le corps de
`pair_kernel` tout en conservant le préfixe et les tableaux de masse P
laisserait une allocation et un balayage quadratiques cachés.

Pour R plans tentés, F=Σ(|A|+|B|), une palette de M=O(K) propositions et E
paires résiduelles, annoncer séparément sélection/certification O(MF),
regroupement O(F+J), où J compte les couples de classes effectivement
testés (au pire O(K⁴R) pour la variante naïve à signatures conjointes),
descripteurs, éventuelle remise en ordre, puis filtre/aval sur E. Ne pas
matérialiser toutes les classes vides de chaque petit rectangle. Un plan
Pool paie les projections une fois par facteur et jusqu'à huit coins par
proposition et par voie, pas un minimum gratuit. Les tubes paient leur tri.
Les palettes préalables par site ajoutent leur préparation et O(nM) mémoire.
F peut lui-même mal croître : rien ici ne prouve une borne globale. Compter
aussi les plans sans réduction et ne les reconstruire ni par tranche A_i
ni par worker. Si le plan ne retire rien, conserver le chemin actuel après
avoir facturé la tentative ; aucun candidat n'est plafonné.

Les [paires de gardes uniformes](../audits/rect_pair_shadow_b_20260923/README.md)
restent un recours plus fort lié aux centres possédés, pas le premier port :
leur shadow brut K5 a payé 26,85 M tests de coins et ne supprimait que
0,904 % des formes du cœur. La première porte est donc une ablation du
plan q34 sur les **mêmes rectangles réels**, Pool/octant/tubes, avec F,
projections, coins, mémoire, E et tous les replis ; juger chaque rejet contre
le filtre ponctuel exact, puis les trois condensés et la tour FULL. Mesurer
amas 8k/16k/32k et coupes capteur/trames entières LiDAR avec et sans sol,
K5/K10 et s8/10/12. Pas d'héritage des gains P0 ou des shadows, pas de
qualification « sous-quadratique » sur le seul rapport du temps total.
