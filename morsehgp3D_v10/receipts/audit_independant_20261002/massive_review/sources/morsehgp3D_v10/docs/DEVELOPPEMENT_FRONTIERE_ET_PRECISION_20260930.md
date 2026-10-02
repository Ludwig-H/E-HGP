# Développement de la frontière et de la précision

Rôle courant, sur la dernière instruction du 30 septembre : **audit**.
Les correctifs décrits ci-dessous ont été livrés lors de la reprise
développeur ; les preuves isolées ultérieures ne changent pas le moteur.

**Priorité de correction de la tête :** le
[contre-exemple de condensation](../receipts/audit_continu_20260929/point_condensation_20260930/README.md)
montre que la branche ne termine pas quand les départs de points font
tomber sa masse sous `min_cluster_size`. Cela peut changer EOM, même
mcs5/racine exclue. Réparer les cohortes de rang exact et juger la masse
active avant de comparer les bras frontière ; ne pas toucher aux fusions
FULL. Les six sondes natives et HDBSCAN réel sont des preuves bornées de
tête/API, pas un nouveau benchmark de qualité sur nuages 3D.
Un [complément géométrique](../receipts/audit_continu_20260929/point_condensation_cover_r2_20260930/README.md)
rejoue seulement la tête sur la traduction exacte d'un export natif 3D
historique, recoupé Γ3 : stabilité erronée mcs6, sans renversement EOM.
Le défaut est donc pertinent pour cover réel ; sa fréquence et son impact
statistique ne sont pas mesurés. Aucun nouvel appel générateur ni gain G4.

Pour le catalogue, le [crédit quantitatif de groupe](../receipts/audit_continu_20260929/group_moments_20260930/README.md)
est désormais éprouvé mathématiquement sur deux supports 3D q3/q4.
Il peut rejeter une famille sans témoin individuellement universel :
préparation des moments une fois, petit nombre de tests par ancre.
Le candidat « groupe entier + six retraits d'extrêmes XYZ » reste à tester ;
masques de supports q3/q4 séparés, population/census intacts. Aucun gain,
profil natif large, borne sous-quadratique ou contrat100ms n'est acquis.

30 septembre 2026. Sur demande de l'utilisateur, l'auditeur continu était
repassé développeur. Cette tranche a implémenté un correctif de la tour et un candidat
d'attache des points. La nouvelle demande de précision supérieure à u18
devient prioritaire. Les mesures et prédicats u18 existants restent historiques ;
le moteur complet à précision supérieure, la qualité statistique et une nouvelle
performance G4 ne sont pas qualifiés. `public_status=not_claimed`, hors registre.

## Ce qui a été implémenté

La recherche de rang de la tour utilisait deux expressions débordantes :
`(a+b)/2` et `lo*64` avant limitation à la taille du catalogue. Le
[correctif](../src/tower/rank_search.hpp) emploie un milieu par différence et
élargit le produit avant de le borner. La fonction pure est utilisée par le
vrai `RankIndex`, sans allocation supplémentaire ni changement d'ordonnancement.
La nouvelle porte `mhgp10_rank_search` réalise 36 047 contrôles, dont des tailles
virtuelles proches de 2^32 et des niveaux répétés comparés à `upper_bound`.
Elle tue séparément les deux erreurs réintroduites. Ce n'est pas une preuve
de capacité mémoire ni une correction de toutes les conversions de cardinalité.

Le [bras frontière par bande](../bench/frontier/README.md) est implémenté en
Python exact, hors tête de production et hors protocole A0–A6 scellé. Sur un
petit cas K3 réellement 3D, A5/A6 changent fortement l'affectation d'un point
strictement intérieur sous un déplacement maximal d'une unité : à l'égalité
ils diffèrent son entrée, puis l'affectent immédiatement après perturbation.
L'unicité à la seule première date ne corrige donc pas les quasi-égalités.
La bande regarde aussi les composantes légèrement plus tardives et conserve
une attache irrévocable par ancêtre commun.

Les six tests natifs passent en normal et avec `-O` : 815 contrôles par
exécution, quatre nuages K3 de quasi-égalité et les deux entrées internes
K3/K5. Les exports et réponses exactes appariés sont identiques entre modes.
Une revue mathématique indépendante établit couverture, emboîtement et
monotonie avec la largeur de bande. **Cela ne démontre pas une meilleure
robustesse générale ou un meilleur clustering.** Les prochains tests doivent
mesurer le rappel frontière avant fusion parasite, la masse différée et EOM
z=1/2 avec condensation, puis comparer équitablement à HDBSCAN.

Le [helper de quotas](../bench/frontier/dev_quotas.py) corrige séparément le
plan de génération : 642 allocations, 51 886 contrôles et 19 refus, normal
et `-O`. Le générateur privé doit encore l'appeler ; ses reçus antérieurs
ne sont pas réécrits.

## Précision supérieure à u18

u18 désigne une plage de coordonnées entières, pas une résolution physique.
À 1 mm, sa plage est 262,143 m par axe ; à 0,1 mm elle ne serait que
26,2143 m. Elle ne convient donc pas comme limite de la nouvelle cible.
La proposition est une grille isotrope paramétrable, 0,1 mm par défaut,
avec coordonnées stockées en u32. Le choix entre cette grille et le
float32 original sans perte a été demandé à l'utilisateur. La
[note du développeur](../audits/NOTE_CLAUDE_REPRISE_ET_PRECISION_20260930.md)
consigne désormais la décision : grille u32 par paliers, float32 natif
différé. Le port géométrique reste à réaliser ; le pas reste paramétrable.

Deux domaines exacts doivent être distingués dans ce contenant u32 :

| Domaine géométrique proposé | Étendue à 0,1 mm par axe | Port à réaliser |
| --- | ---: | --- |
| u24 | 1 677,7215 m | première voie large plausible pour une trame LiDAR ; centre encore i128, census/orientations et niveaux élargis |
| u32 complet | 429 496,7295 m | centres larges, distances 128 bits, clés Morton 96 bits et comparateurs de niveaux jusqu'à 512 bits |

u24 est un palier proposé, **pas un moteur déjà porté**. Il ne doit jamais
être annoncé comme u32 complet. Une entrée hors domaine est refusée, pas
renormalisée ou requantifiée implicitement. Chaque reçu devra publier pas,
origine, domaine, nombres de retours/sites/fusions et correspondance des IDs.

Il est faux de relever seulement le plafond actuel : la clé Morton64
masque chaque axe à 21 bits et le regroupement fusionne par clé seule.
Les positions `(0,0,0)` et `(2^21,0,0)` deviendraient une fausse fusion.
Les distances du SiteTree calculées en i64/u64 ne couvrent pas le domaine
u32 : le carré de la diagonale peut dépasser 2^65. Les boîtes en double
stockent en revanche déjà exactement toutes les coordonnées u32.

Une première brique large est désormais implémentée dans
[grid32_primitives.hpp](../src/cloud/grid32_primitives.hpp) : distance carrée
exacte sur u128 et clé Morton de 96 bits conservant les 32 bits de chaque
axe. Le décodeur refuse les bits au-delà de 96 au lieu de les tronquer.
La porte indépendante réalise 212 684 contrôles, dont 10 162 distances,
10 203 aller-retours et 64 clés hors domaine. Les oracles de distance
(entiers multi-mots) et d'encodage (construction du flux de bits) sont
distincts des implémentations. Deux relectures indépendantes ne trouvent
pas de défaut.
Normal et UBSan passent avec les mêmes résultats ; les deux troncatures
historiques réintroduites séparément sont rejetées numériquement, sans crash.

Ces primitives sont de coût constant, sans allocation ; elles ne sont
**pas encore raccordées** au propriétaire de nuage, au SiteTree, aux
prédicats ou au GPU. Elles n'enlèvent donc pas le refus u18 actuel.
Elles ne définissent ni le pas physique ni un nouveau format de fichier.

L'[audit de largeur](../receipts/development_frontier_precision_20260930/precision_math/AUDIT.md)
propose pour u32 des centres q3/q4 sur trois mots,
des côtés de sphère jusqu'à 201 bits, des orientations de centre jusqu'à
233 bits et des niveaux jusqu'à 266/200 bits. Leur produit croisé peut
demander 466 bits ; le produit générique de tableaux de cinq et quatre mots
ne compile pas avec la limite actuelle de huit mots. Ces bornes doivent
être dérivées et testées dans le port, pas transposées par remplacement
global de types. Le palier u24 évite ce dernier obstacle et garde le centre
i128, mais ne garde pas les prédicats de census i128 actuels.

Pour préserver le débit, la piste pertinente est un dispatch **certifié par
opération** : conserver la voie courte lorsque les valeurs effectivement
impliquées sont dans sa borne, replier vers les mots larges sinon. Une petite
étendue du support seule ne justifie pas le test contre un témoin très loin.
Les marges flottantes 0,02 héritées de u18 ne sont pas transportables ; les
recopies de boîtes ne sont pas des filtres numériques qualifiés.

### Contre-épreuves du port précis

Le [nouveau reçu](../receipts/audit_continu_20260929/precision_port_20260930/README.md)
est distinct des fondations déjà closes. Il exerce les corps actuels sur des
entrées volontairement hors contrat, sans supprimer le refus du moteur.

- 24 appels géométriques C++ normal/UBSan, 20 sorties jugées en paires
  Fraction normal/−O : huit désaccords numériques et quatre arrêts UBSan.
  Le rayon q4 d'un tétraèdre régulier u24 devient zéro au lieu de
  `844424829468675/4`, sans alerte UBSan. Déjà u21 tronque son dénominateur.
- Trois contacts à centres réduits isolent les erreurs du filtre flottant :
  coquille perdue ou faux intérieur, y compris après translation d'une
  géométrie de diamètre local u18 près de 4e9. Les côtés exacts du cas restent
  dans i128. Le paquet scalaire compte 614 contrôles par passage ; son
  préflight à attente erronée reste rejeté et conservé.
  Le complément C++ copie le vrai SiteTree et confirme les trois erreurs
  sans diagnostic UBSan : neuf requêtes au total, dont six dans la paire
  finale normal/UBSan ; quatre lectures Fraction à 72 contrôles chacune.
  Les nuages sont construits directement hors factory u18, sans changer
  le contrat de production. Code0 signifie ici « erreurs attendues confirmées ».
- La réduction par PGCD peut accélérer certains supports, mais deux
  tétraèdres stricts u24 gardent après réduction des niveaux 194/146 bits.
  Elle ne dispense pas des largeurs sûres ni du contrôle de `resize(false)`.

La correction recommandée prépare le centre **relatif à l'ancre**, traduit
exactement les écarts avant conversion, puis encadre distances et rayons
par intervalles extérieurs. Un contact ambigu passe au prédicat exact.
Nearest retient les candidats dont la borne inférieure ne dépasse pas la
borne supérieure du rang recherché, égalités comprises. Toutes les voies
`0.02` de SiteTree **et** de la tour doivent suivre le même contrat.
Le contre-cas nearest prouve un mauvais départage à distance égale, pas
une mauvaise valeur du rayon K-NN.

Pour le catalogue, recadrer les polynômes **avant** leur multiplication peut
conserver une voie courte sans entier large partout. Si L, exprimé en T6,
borne la largeur de Q et les écarts de tous les sites parentaux à Q.lo,
L≤2^29 suffit aux intermédiaires D-loc en i64 ; la droite des centres u32/T6
reste dans i128 avec L≤2^38. Ces bornes sont mathématiques et conditionnelles
au recadrage et aux promotions avant calcul, pas des ports exécutés ni des
gains mesurés. La petite largeur de Q seule ne borne pas les sites parentaux.

Le préparateur exact v8 accepte déjà un pas décimal paramétrable : ne pas
réécrire une conversion flottante ni affiner une ancienne grille entière.
Il manque un consommateur v10 contrôlant le manifeste, les hashes et les
correspondances retour/site avant de lancer un profil déclaré supporté.
L'origine signée peut sortir de u32 sans que les coordonnées locales stockées
le fassent ; elle doit rester une métadonnée exacte, distincte des indices.

Un petit contrôle mathématique complémentaire vérifie 117 arrondis et
183 couples de rayons MEB sur trois nuages 3D, aux pas 1/0,1/0,01 mm.
À univers de retours étiquetés fixé, l'erreur des rayons est au plus √3·h/2.
Ce lemme donne un entrelacement des composantes Γ_K en rayon, pas la
stabilité des attaches, de l'EOM ou de l'ARI. Fusionner des retours sans
leurs multiplicités change l'estimateur ; diffuser leurs étiquettes ensuite
ne rétablit pas sa densité. Le décalage est en rayon, non en rayon carré ;
les coupes capteur à univers d'IDs différent ne sont pas appariées par ce lemme.
Aucun test de croissance ou FULL large ne découle
de cet oracle borné. GCP non utilisé dans ces contre-épreuves.

### Filtre relatif : première réparation isolée

Le [prototype d'audit](../receipts/audit_continu_20260929/relative_filter_20260930/README.md)
prépare maintenant N/D et le rayon dans le repère de l'ancre, puis encadre
les distances de sites u32 et de boîtes fermées. Les coefficients ont une
magnitude de 192 bits ; les contacts restent `AMBIGU`, à transmettre au
futur repli exact. Les boîtes ne certifient que le rejet extérieur, pas
l'intériorité de tous leurs points.

Le panel Fraction indépendant comprend 3 600 requêtes : 2 299 conversions,
1 051 sites et 250 boîtes. Normal/UBSan donnent les mêmes sorties ; 196
contacts et 40 translations identiques bit à bit passent. Deux mutants
code0 sont tués mathématiquement : reste de conversion perdu et marge
fixe réintroduite. Les 147 contrôles du lemme de rang restent sur listes
statiques Python, pas un nouveau nearest natif.

Le prototype vérifie les modes courants FE_TONEAREST/SSE2 et refuse
FTZ/DAZ ; environnement x87 non piégeant requis. Son descripteur const
semble partageable en lecture, mais aucun test concurrent, TSan ou GPU
n'est acquis. L'ancien smoke au gel incomplet reste historique ; la porte
v2 figée exécutée passe ses 50 contrôles. La contre-vérification adversariale
du juge conserve ses trois versions et leurs limites, sans effacer les
tolérances initiales ni relancer les captures natives.

Le repli exact doit être qualifié selon son domaine : la valeur P du
protocole générique peut nécessiter 258 bits, contre les bornes suffisantes
201/168 bits des supports q3/q4 u32 réellement certifiés. Un calcul sur
256 bits sans garde ne couvre donc pas toutes les entrées du prototype.
Un dispatch de domaine, ou un certificat de dépassement pour le signe
seul, reste à implémenter et à éprouver. Le filtre isolé n'est pas encore
un port du propriétaire, du SiteTree, des niveaux ou de la tour.
Il ne lève pas le refus u18 et ne transfère aucun chrono historique.

Le [lecteur renforcé R2](../receipts/audit_continu_20260929/relative_filter_reader_r2_20260930/README.md)
vérifie les fichiers avant de charger le juge et exige toutes les empreintes,
avec zéro exécution native aux rejugements. Cinq altérations structurelles
sont refusées. La première clôture est conservée à octets identiques.
Les quatre documents et ce nouveau complément passent le contrôle ciblé ;
le contrôle global des espaces signale seulement une ligne vide finale
dans le script adversarial R1 clos, conservée pour la traçabilité.

### Ordre exact des niveaux : candidat large éprouvé

Le [comparateur entier isolé](../receipts/audit_continu_20260929/level_order_20260930/README.md)
compare les futurs niveaux N/D sous les gardes N<2^266 et 0<D<2^200.
Les produits croisés restent inférieurs à 2^466 et tiennent sur huit mots ;
la retenue vers un neuvième mot est conservée et contrôlée, pas tronquée.
Il évite ainsi l'instanciation générique `Wide<9>` aujourd'hui interdite.
L'échec de compilation de cette instanciation a été capturé sur les vrais
headers ; ce n'est pas un échec du moteur u18 actuel.

2 444 requêtes passent en normal et UBSan, avec les mêmes sorties : 2 172
comparaisons rationnelles et 272 produits bruts. Parmi elles, les 729
comparaisons géométriques portent sur 27 niveaux q2/q3/q4, calculés en
Python par deux méthodes rationnelles distinctes jusqu'à u32. Six refus
de domaine et 14 débordements de produits bruts sont contrôlés. Les deux
mutants code0 — égalité flottante indue et retenue haute perdue — sont
refusés numériquement par le juge. Cela qualifie le comparateur isolé,
pas les constructeurs de niveaux, le tri natif ou la tour large.

Le raccord doit aussi changer `level_at_most` : son retour vrai après
échec de réduction vers 192 bits dépend de l'ancienne borne du numérateur.
Avec N=2^200, D=2^193 et seuil1, conserver cette règle après élargissement
donnerait vrai alors que le niveau vaut128. C'est un contre-exemple au
port naïf, pas un défaut démontré du profil u18 protégé. Les niveaux
entiers K-NN, caches, recherches de rang et exports doivent suivre les
distances carrées u32 sur 66 bits, sans réduction implicite vers u64.

Le catalogue vérifie exactement tous les voisins après son tri approché
et ses réparations locales ; ce filet refuse une inversion restante,
mais ne répare pas une clé tronquée. La table double de `point_dendrogram`
fusionne délibérément certains niveaux exacts distincts et n'est donc pas
un export sans perte de tous les événements FULL. Préserver un rang exact
commun aux niveaux géométriques et K-NN, puis exposer séparément sa vue
métrique approchée.

Pour accélérer sans perdre l'ordre, des intervalles certifiés peuvent
séparer les niveaux disjoints, avec comparaison entière dans les cas
ambigus. Les groupes doivent utiliser le maximum cumulé des bornes hautes,
pas le seul intervalle précédent ; le recouvrement n'est pas une égalité
transitive. Le modèle Fraction passe 33 fixtures et 2 051 intervalles,
normal/−O, sans qualifier de convertisseur ou trieur natif. Le coût
O(M log M) du tri n'établit aucune borne sous-quadratique en nombre de
points : il faut toujours mesurer le nombre M d'événements produits.
GCP, performances et croissance 8k/16k/32k ne sont pas testés dans ce lot.

## Ordre des travaux suivants

1. Préparer le profil large : identité sans collision, conversion exacte
   avec pas/origine explicites, distances et largeur des prédicats. Déclarer
   chaque brique portée et chaque refus ; ne pas activer une entrée large
   devant un census ou une tour restés u18.
2. Qualifier les côtés, orientations, supports et niveaux sur les extrêmes,
   les égalités et un oracle rationnel indépendant. Raccorder le même profil
   au générateur, à la tour, aux attaches et aux exports.
3. Comparer les bras frontière sur scènes dev à tailles et nombres de
   communautés variables, puis geler un test distinct. Conserver les contrôles
   A0/core et A1/cover, EOM z=1/2 et le même min_cluster_size chez HDBSCAN.
4. Sur G4 SPOT, mesurer les trames sans sol entières et les coupes spatiales
   au capteur, puis avec sol et plusieurs séquences. Mesurer les travaux
   8k/16k/32k, incidences et mémoire, pas seulement le chrono du bras.
   FULL K5 explicitement matérialisée reste la cible 100 ms ; aucun nouveau
   profil ne reçoit les qualifications ou chronos de u18 par héritage.

Les correctifs R2 du développeur restent dans leurs copies séparées ; cette
tranche ne prétend pas résoudre leur intégration commune. GCP n'a pas été
utilisé : les nouvelles régressions bornées ne nécessitaient pas de VM.
Les [preuves de cette tranche](../receipts/development_frontier_precision_20260930/README.md)
conservent aussi les échecs de préparation et la restriction LeakSanitizer.

Validation documentaire : les six documents modifiés et les nouveaux
documents de preuve passent le contrôle ciblé. Le contrôle global du dépôt
reste en échec sur des liens relatifs de captures historiques déjà présentes ;
ces archives closes n'ont pas été réécrites pour faire passer le contrôle.
Le registre formel, inchangé, passe ses 20 phases.

La publication des contre-épreuves de précision vérifie six documents ciblés
et 159 empreintes du nouveau reçu, puis ses six manifestes internes, lecteurs
normal/−O. Le contrôle des espaces est propre sur les documents et scripts
rédigés ; celui de toutes les données signale une ligne vide finale dans
le `compiler.txt` natif clos. Ces octets de preuve restent inchangés.
