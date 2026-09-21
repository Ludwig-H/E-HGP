# Tranche32 — rejeter avant de construire, puis compter par boîtes

Cadre : `exploration_v8_hors_registre`, `cpu_reference`,
`quantized_u16_input_only`, `implementation_v8_p0`, `not_claimed`.
Le chantier reste sur `main`. Cette tranche produit des candidats q3/q4,
pas un catalogue de boules ni la tour FULL. La cible de croissance porte
d'abord sur les scans LiDAR réellement considérés ; une contre-fixture
universelle ne remplace pas leur mesure.

## Pourquoi ce changement

La tranche31 développait les paires que sa petite fenêtre de témoins
WSPD ne rejetait pas. Sur le scan0/8k/K5, elle construisait780,66millions
de triangles et effectuait361,20milliards de tests ponctuels q3. Le tri
des coquilles n'expliquait pas ce volume. Les mesures indépendantes de B
dans `audits/front_lanes_lidar_20260921/` indiquent que beaucoup de ces
paires sont pourtant rejetables : leurs témoins existent, mais la fenêtre
ne les trouve pas. Ces mesures motivent le port, sans le qualifier.

L'idée est simple : chercher quelques témoins suffisants AVANT de bâtir
toutes les boules associées à une arête. L'index du nuage existe déjà et
permet de visiter des groupes entiers. Pour les boules restantes, le même
principe évite de relire un par un tous les points extérieurs.

## Ordre effectif du calcul

1. Préparer une seule fois le nuage immuable et son index spatial.
2. Parcourir la vraie WSPD multivoie, avec sa convention s8/10/12 actuelle.
3. En mode `rectangle-pair`, rechercher des témoins communs à toutes les
   paires du rectangle. Retirer chaque voie dont le seuil est atteint.
4. Pour chaque paire des voies restantes, rechercher des témoins propres
   à cette paire ; ne construire aucune couverture si toutes sont rejetées.
5. Construire une couverture partagée des voies survivantes, puis leurs
   seeds propriétaires. Aucune acceptation q2 ou q3 ne conditionne q4.
6. En option `boxes`, compter les intérieurs q3 par boîtes ; seulement
   après acceptation, faire un deuxième parcours pour la coquille entière.
   q4 conserve Local28 ou Window30, choisis explicitement.
7. Émettre supports, clés, profondeur et coquilles, sans matérialiser une
   liste globale de toutes les paires, faces ou incidences.

Les valeurs historiques `Disabled` et `ScalarCover` restent les défauts
de l'API pour permettre les comparaisons. Les nouvelles options sont
orthogonales : `Pair`/`RectanglePair` et `GlobalBoxes`.

## Certificat de rejet q3/q4

Avec $H=(z-a)\cdot(b-z)$ et $\Xi=|(b-a)\times(z-a)|^2$, un témoin
satisfait strictement $H>0$ et $\alpha_q H^2>\Xi$, où
$\alpha_3=3$ et $\alpha_4=2$. Les seuils sont respectivement K−1 et K−2.
Ce citron est intérieur à toute boule positive dont cette arête est une
arête maximale du SUPPORT ; le nombre de sites sur la coquille ne change
pas ce lemme. Employer3 à la place de2 pour q4 serait incorrect.

`filter_q34_witnesses` utilise les bornes conjointes existantes de4H et
un majorant de Xi sur les boîtes A/B/Z. Il partage une descente entre les
deux voies, en visitant d'abord l'enfant le plus proche du milieu.
Un nœud témoin est crédité une seule fois PAR VOIE : son bit disparaît du
masque transmis aux enfants si l'autre voie impose un raffinement.
Les nœuds ainsi crédités forment une partition disjointe pour chaque compte.
Hmin>0 exclut automatiquement tout endpoint potentiel situé dans A ou B.

Les boîtes singleton donnent la décision exacte du citron ponctuel.
Sur des facteurs multiples, la décision reste seulement un certificat
suffisant : un échec n'établit pas l'absence de témoins communs aux seuls
sites discrets. Tous les échecs sont traités en aval, sans omission.

Aucun crédit partiel n'est réutilisé : ni dans une autre recherche, ni
dans le census d'une boule. Cela évite tout double comptage entre étapes.
La pile de49cadres vient des48 subdivisions possibles des coordonnées
entières u16 dans cet index, pas d'un plafond de recherche ou de sortie.

## Census q3 par boîtes

La puissance d'une boule est $A|z|^2+B\cdot z+C$, avec A>0.
Elle se sépare par coordonnée. Le minimum entier de chaque parabole est
au plancher ou au plafond du sommet, rabattu dans l'intervalle ; le
maximum est à une extrémité. Les sommets sont préparés une fois par boule.
On n'évalue jamais B² : les coefficients u16 certifiés permettent toutes
les évaluations séparées dans i128, avec une borne360·65535⁶<2¹⁰⁵.

Premier parcours : minimum≥0 exclut le nœud du COMPTE strict ; maximum<0
ajoute son cardinal, avec saturation sans débordement ; sinon raffinement.
Le nœud de minimum de puissance le plus faible est visité d'abord.
Toutes les bornes des enfants préparées avant saturation sont payées,
même si ces enfants ne seront finalement pas visités.

Deuxième parcours, uniquement si la profondeur est sous le seuil :
conserver toutes les boîtes dont minimum≤0≤maximum et collecter les IDs
de contact. Le support fait partie de cette coquille complète ; un futur
consommateur voulant les seuls autres contacts doit effectuer la différence
en connaissance de cause. Le census ne construit pas d'index par seed et
ne reçoit aucun crédit de la WSPD ou du citron.

L'entrée de census accepte toute `ExactBall` non forgeable ; le raccord
présent lui fournit des boules q3. La clé n'encode pas l'arité de son support.
Une exception laisse les compteurs/buffers partiels ; elle n'est pas un
résultat accepté ni une capture complète.

## Coûts et parallèle

Le coût payé inclut front, recherches de témoins, expansion restante,
couvertures, seeds, census, tris et payloads. Chaque recherche est linéaire
en son nombre de nœuds VISITÉS ; cela ne borne pas à lui seul la somme
globale de ces visites. Les couvertures et seeds survivantes peuvent
encore croître vite. La mesure8k/16k/32k est donc nécessaire après le port.

L'équipe Coarse existante garde l'index partagé et des moteurs privés.
Les nouvelles recherches sont locales aux rectangles/arêtes de chaque
worker ; aucun verrou géométrique ni tableau global de faces n'est ajouté.
L'arête reste atomique : la queue lourde et le partage des blocs de seeds
demeurent un sujet distinct. Ce n'est pas encore un backend CUDA.

`witness` sépare masses rejetées par rectangle/paires et visites des deux
étages ; les masses q3/q4 se recouvrent et ne s'additionnent pas en masse
de paires distinctes. `expanded_pairs` est mesuré après le filtre rectangle,
avant le filtre paire ; `cover_builds` et `q3_edges/q4_edges` après les deux.
`q3_blocks` porte tous les coûts du nouveau census ; les anciens compteurs
de parcours scalaire restent alors nuls. Les maxima de piles/capacités sont
distincts des sommes de travail et du RSS. `owner_tests` peut compter deux
comparaisons pour un même seed aigu.

## Validation et mesures

94 CTests Release passent. Trois portes et12sondes en Clang
ASan/UBSan/LSan passent, pas94tests instrumentés. Le citron est comparé
à un oracle entier indépendant (2335requêtes), le census à un oracle
rationnel global (44census), le raccord à l'oracle des supports/profondeurs/
coquilles (168nouveauxappels filtrés,84en modeboxes, plus les166appels
historiques). Six mutants compilés sont réfutés géométriquement.

Trois smokes Release scalar/boxes et SANboxes conservent36mesures et
les sorties complètes appariées. Le premier lecteur avait encore le
plancher31 de deux appels parallèles inactifs au lieu des huit nouveaux :
échec conservé et correctif lecteur seul. Une première tentative LSan
échouée sous ptrace est conservée, reprise distincte LSan actif réussie.
Lire les [reçus32](../receipts/q34_indexed_20260921/README.md) et leur
[relecture close](../receipts/q34_indexed_20260921/READBACK.md).

Les tests8k W1/W4 montrent que RectanglePair économise les recherches
par paires :151,391M→61,494Mvisites, sans changer les177415couvertures
finales ni les sorties. Boxes remplace278,544Mtests scalaires par72,936M
bornes préparées de coût différent ; aucun gain CPU général n'est prouvé
à cette taille sous charge concurrente.

Première série complète8k/16k/32k scan0/K5/s8/Local28 :1,911/6,033/20,363M
seedsq3,67,517/213,666/735,508Mbornes de profondeur. Les derniers rapports
sont sous4 pour ces postes ; mais les requêtes q4 font×4,018 puis×5,603.
Ne pas confondre ce progrès avec une correction de tous les coûts ou un
transfert aux autres scans. Les comparaisons globales28/30 et s8/10/12
sont rapportées dans les reçus avec les contre-résultats conservés.

Les nouveaux builds n'écrasent aucune capture31 ni les builds épinglés.
La hausse du registre augmente réellement la taille du WorkerState privé,
y compris en mode désactivé :2688→3392octets, soit704octets par worker.
Huit comparaisons du défaut31/32 (16commandes, sans nouvelles options)
retrouvent sorties complètes et géométrie identiques. Les capacités privées
dépendantes du scheduling sont publiées séparément, sans les faire passer
pour une différence de travail.29grandes mesures sont closes : trois dans
la première série, douze pourK5/10 ×28/30 ×8k/16k/32k, six àK5/s10/12,
huit pourPair/RectanglePair ×scalar/boxes ×W1/4 à8k. Ces mesures portent
sur le scan0, pas encore sur toute la diversité SemanticKITTI.

Les builds `build/v8_q34_indexed_20260921` et
`build/v8_q34_indexed_sanitize_20260921` sont désormais épinglés.
La [note de suite](Q34_PISTES_APRES_INDEXATION_20260921.md) reprend les
propositions utiles des auditeurs et précise les nouveaux certificats à
porter, sans leur attribuer un gain acquis. GCP non utilisé dans32.
Aucun contrat50k, GPU ou FULL n'est acquis par ces tests.
