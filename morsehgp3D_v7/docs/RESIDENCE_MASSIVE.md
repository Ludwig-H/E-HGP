# Tour massive : résidence du code C et première frontière externe

4 septembre 2026. Port documentaire revérifié depuis l'analyse locale de la
v7 C : **aucune compilation, aucun benchmark, aucun changement du code
produit, GCP non utilisé**. Cadre :
`phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`. Cette note ne rouvre pas la géométrie S1 fermée
conditionnellement ; elle distingue représentation, cardinalité et résidence.

Conclusion sur le snapshot C : **10 millions de points passent le verrou
d'indices spatiaux, pas nécessairement ceux des catalogues intermédiaires**. Ce moteur
doit conserver en RAM l'index et les boules survivantes globales, puis un
catalogue complet d'événements/facettes pour chaque ordre traité. « Streamé
par K » n'est pas un calcul externe. L'étape 1 proposée s'arrête à un index
immutable et un catalogue externe de candidats ; elle ne livre pas une
tour exacte 10M complète.

## Complément du 5 septembre : résidence du certificat FULL à concevoir

L'[audit des niveaux utiles](AUDIT_NIVEAUX_GABRIEL_20260905.md) et sa
[contrelecture indépendante](../audits/NIVEAUX_ET_CERTIFICAT_HGP_COURANT.md)
séparent désormais l'état de construction du résultat HGP complet.
Sous régularité, celui-ci conserve les minima Gabriel de cardinal K avec
leurs K PointId et niveaux, puis les vraies multifusions aux niveaux Gabriel
de cardinal K+1 avec leurs parents. Les couvertures sont des unions de
feuilles descendantes, pas des copies obligatoires dans chaque nœud.
Les cofaces de descente et les continuations FULL ne sont pas un payload
persistant nécessaire. Les portails restent un travail interne à certifier.

Avec L minima et R racines finales à un ordre, I multifusions vérifient
$I\leq L-R$ et le nombre de liens vaut $L+I-R$. Le stockage topologique est
linéaire en L, avec $O(KL)$ identifiants pour les labels des feuilles.
L n'est pas borné par n aux ordres supérieurs. Cette borne du résultat ne
borne ni les candidats, ni le cache de portails, ni la longueur des descentes,
ni la résidence du générateur WSPD ; elle ne démontre aucun palier massif.

Le [premier jalon livré](CONTRAT_CERTIFICAT_FULL.md),
`src/forest/full_certificate.hpp`, qualifie uniquement un certificat
structurel et son lecteur. Le [producteur horizontal suivant](CONTRAT_PRODUCTEUR_FULL_GABRIEL.md)
raccorde désormais les catalogues Gabriel fournis aux minima et parents,
sous autorité relative et budgets propres. Il ne livre ni format de
checkpoint durable, ni reprise du moteur, ni ancres inter-K publiées.
Les limites et mesures C ci-dessous restent historiques ; elles ne se
transfèrent pas à ce nouveau schéma.

Pour les masses et le vote, déclarer un supplément avec son univers de
feuilles et sa politique d'affectation. Les feuilles pondérées du catalogue
Gabriel ne sont pas les seuls minima FULL ; leur information utile peut
demander un journal distinct, sans imposer toutes les facettes Gamma.
Les frontières externes proposées aux §§4–5 restent des pistes relatives
au snapshot C, pas des étapes obligatoires avant le certificat FULL.

### Première observation de résidence FULL horizontale

La sonde indépendante de la CLI F partage les boules censusees entre
ordres et ne garde que deux catalogues Gabriel adjacents. Elle détruit
chaque certificat après sa lecture sentinelle : ce n'est pas la mémoire
d'une tour dont toutes les forêts seraient simultanément conservées.
Sur uniforme/seed3/u16, n=8000, s=8, K1..10, le premier reçu mono compte
3 113 381 boules et 6 209 024 alias à K10 ; le pic RSS du processus est
1 837 632 KiB. À n=16000, le plafond inchangé de huit millions d'alias
refuse K9, après huit ordres diagnostiques. Ces nouvelles observations
sont conservées dans les [reçus FULL mono](../receipts/full_gabriel_mono_20260905/README.md),
distincts du snapshot C et sans qualification G4.

La prochaine piste est de séparer les autorités obligatoires — minima,
ancres de toutes les directes, successeurs — du cache de facettes résolues.
Une facette non mémorisée pourrait rejoindre sa directe antérieure par le
cas à un intrus, sans matérialiser tous les alias égaux dès chaque coface.
Cette piste a reçu une [contrelecture favorable](../audits/receipts_full_producer_20260905/lazy_alias_next_step_review.md)
sur le domaine régulier ; elle n'est pas intégrée aux octets mesurés.
Elle peut réduire la résidence tout en augmentant les MEB
et census sur les absences du cache. Aucun gain de temps ni palier massif
n'est déduit de cet échange.

## 1. Notation et nature des conclusions

- **[B] Borne de code et argument d'entiers** : indépendante d'une mesure,
  sous les préconditions explicitement gardées du pipeline.
- **[R] Résidence structurelle** : conteneurs effectivement possédés et
  simultanéité constatée dans les portées du code ; pas une mesure de RSS.
- **[E] Estimation conditionnelle** : calcul d'ABI ou dimensionnement qui
  demande un reçu de machine/allocateur. Aucune extrapolation de temps.
- **[P] Proposition** : travail futur, sans implémentation ni qualification.

Notations : n points, m positions uniques ; E candidats bruts ; C candidats
uniques après RLE ; S boules survivantes ; pour un ordre K, e_K événements
finaux, i_K incidences `Σ(q+d)`, f_K facettes distinctes, b_K lots de niveaux.
Les événements finaux comprennent la complétion silencieuse lorsqu'elle est
activée. Sur les entrées acceptées actuellement, m=n : les positions
dupliquées sont refusées, pas fusionnées silencieusement.

## 2. Plafonds exacts : ne pas les traduire en n sans preuve

| Objet / identifiant | Type et borne acceptée [B] | Autorité du code |
|---|---|---|
| Entrée complète | `n <= 2^30-1 = 1 073 741 823` | `run.hpp:416`, `caps.hpp:36`, avant construction d'index |
| Point externe | `PointId=u32`, valeur quelconque dans `[0, 2^32-1]`, unique ; distinct du rang Morton | `types.hpp:32`, `cloud_index.hpp:143` ; pas de sentinelle PointId qui retire la valeur maximale |
| Géométrie | Coordonnées entières `[0,65535]^3`, m clés Morton distinctes de 48 bits | `types.hpp`, `cloud_index.hpp:164` ; u16 borne la grille, pas n |
| Référence d'arbre / rang | `NodeRef=i32`, interne non négatif, feuille `-1-u` ; m-1 internes | `cloud_index.hpp:38–72` ; la garde n protège aussi les recherches de Karras |
| Buckets | Débuts `u32`, IDs `u32`, préfixes de multiplicité `u64` | `cloud_index.hpp:66–70` ; tous les offsets <=n |
| Candidats bruts publiables | `E <= 2^32-1` ; cap configurable seulement à la baisse | `caps.hpp:44`, `run.hpp:320`, `generate.hpp:1399` |
| Dépassement coopératif brut | Avant refus, au plus `H + 4096*T` émissions pour plafond H et T workers | `EmitThrottle`, garde exacte avant fusion ; ce n'est pas un budget anti-OOM des shards |
| Vague et rectangles vivants | Chacun <=`2^32-1` à la fusion globale ; shards de prochaine vague <=`2*wave` | `caps.hpp:49–50`, `generate.hpp:422–450` ; aucune borne pratique linéaire avec constante mesurée n'est déduite |
| Index du préfiltre | `Survivor::idx=u32`, C<=`2^32-1` | `expand.hpp:35`, `candidates_capacity_ok`, garde amont `run.hpp:556` |
| Ordre HGP | `1<=K<=10`, `q+d=K+1<=11` | `run.hpp:332`, `fold_event_ok` ; coquille census<=12, intérieur<=9, indices géométriques i32 |
| Événements par fold | `e_K < floor((2^32-1)/11)=390 451 572`, donc maximum 390 451 571 | `fold_capacity_ok`, `fold.hpp:369–380` ; comparer le code, pas le commentaire historique moins restrictif |
| Incidences par fold | `i_K <= 2^31-1` | Même garde, avant préparation ; comme K est constant, `i_K=(K+1)*e_K` |
| Facettes et DSU | `f_K <= i_K <= INT32_MAX`, parents i32, canon/fid u32 | Une facette internée provient d'au moins une incidence ; `FidState`, `fold.hpp:877` |
| Slots d'incidence | Position u32 `11*e + slot`, tableaux de longueur `11*e_K` | `fold.hpp:536–570` ; la garde d'événements protège le pas fixe 11 |
| IDs temporaires d'internement | tid u32 et partition u8 dans **deux tableaux** | `fold.hpp:603–610` ; aucune limite restante de 26 bits par partition malgré un ancien commentaire de passe |
| Lots / époques | b_K<=e_K, batch CSR u32, `UINT32_MAX` réservé aux époques | `DeltaMeta`, `FidState` ; la borne d'événements garde la sentinelle hors des lots réels |
| Arènes CSR | Offsets u32, majorant total des clés émises <=i_K ; gardes de taille, octets et append | `fold.hpp:724–754`, arènes `parents_keys` et `born_keys` |
| Totaux de tour | Compteurs u64, sommes par K | `run.hpp:930–936` ; ce ne sont pas des identifiants ni des allocations de toutes les facettes de la tour |

En particulier, l'incidence limite e_10 à **195 225 786** et e_5 à
**357 913 941**, même si le cap général des événements est supérieur. Ce
sont des plafonds par ordre, pas des nombres de points supportés.

Vérification littérale de la frontière : le code refuse `events >= cap`,
où `cap = UINT32_MAX / 11` en division entière ; il n'accepte donc pas
l'égalité à 390 451 572. Au maximum accepté général, `11*e_K=4 294 967 281`
et le dernier slot occupable `11*(e_K-1)+10=4 294 967 280` reste inférieur
à UINT32_MAX. Un fid de facette vaut au plus `f_K-1 <= 2 147 483 646` ;
sa conversion en i32 est définie. Les tableaux temporaires séparent
partition u8 et tid u32 : aucun empaquetage `partition<<26 | tid` n'est
employé par les écritures actuelles. Ces arguments ne prouvent pas que
les allocations correspondantes tiendraient en RAM.

Argument pour Karras : avec m<=2^30-1, une portée valide ne peut excéder
m-1 ; le premier doublement qui sort atteint au plus 2^30. Pour un index
i<m, `i+lmax <= 2^31-2` et `i-lmax >= -2^30`, donc les additions signées
restent définies. Ce verrou porte sur l'appel produit ; la primitive publique
`build_cloud_index` n'ajoute pas elle-même cette garde de cardinalité.

### Complétion silencieuse : limites propres, souvent plus basses

`SilentIncidenceLimits` (`silent_incidence.hpp:46`) fixe par défaut :
8 000 000 records de cœur, 2 000 000 pas de chaîne, 2 000 000 cofaces
ajoutées, 1 000 000 000 visites de nœuds et autant de supports de MEB.
La limite de records borne aussi la taille initiale du catalogue direct.
Les incréments sont prospectifs ; l'égalité du compteur au plafond est
permise, le prochain incrément refuse. La longueur pratique des chaînes
n'a pas de borne indépendante de ces caps démontrée ici.

Ces caps configurables ne sont pas des largeurs d'ID. Les relever ne résout
ni la RAM des catalogues, ni les gardes du fold, ni les sorties refusées
pour dégénérescence. Il ne faut pas appeler une route `verified_events_only`
un repli exact si la complétion est trop coûteuse.

Pour K>=2, `core_records=Σ_direct q` compte les émissions **avant** dédoublonnage
des facettes, et 2<=q<=4. Le plafond par défaut de 8M records impose donc
déjà au plus 4M événements directs, indépendamment du test initial à 8M
cofaces. Il ne s'agit pas d'un plafond de 8M facettes uniques.

### Formats : distinguer cardinal et identifiant

Dans le code C lu, `digest_forest_v4` écrit les **tailles en u64** et les
valeurs de `final_canon_fid` en u32 (`digest.hpp:125–128`). L'archive v7 fait
de même (`archive.hpp:272–275`). Le digest des candidats encode également
leur nombre en u64. Le vieux tableau v6 « facettes au-delà de 2^32, format
du digest » ne peut donc pas être repris comme une borne de compteur v7.
Un futur élargissement des fid exige néanmoins un format versionné : un
canon u64 ne doit pas être tronqué dans les quatre octets actuels. Les
`PointId` des clés de facettes restent une question distincte.

## 3. Ce qui doit résider en RAM dans le code C actuel

« Doit » décrit ici les API `std::vector` et les portées actuelles, pas une
nécessité mathématique. Les tailles ci-dessous sont logiques ; les capacités,
les copies transitoires et l'allocateur s'ajoutent.

| Phase | Catalogues globaux possédés [R] | Fin de vie / transitoires importants |
|---|---|---|
| Construction d'index | Entrée `InputPoint[n]`, index Morton/positions/buckets/arbre | Tri de contrôle `PointId[n]` puis, dans une autre portée, `Rec[n]` pour Morton ; ce dernier reste vivant pendant les tableaux d'index et le postfixe des m-1 nœuds |
| Toute l'exécution | `CloudIndex` complet : keys[m], upos[m], bucket_start[m+1], bucket_ids[n], wsum[m+1], nodes[m-1] ; entrée empruntée non libérable par le pipeline | Accès aléatoires aux positions, boîtes, enfants et plages par WSPD, témoins, census, complétion silencieuse |
| Génération | Vagues `wave/next`, tous les rectangles terminaux `alive`, puis candidats `louts[T]` et destination | `alive` reste jusqu'à la fin de génération. Scratch par worker : histogrammes, handles, cover/lens/query, quatre tableaux affines, grilles, racines de corde ; pas de petit majorant global garanti par n seul |
| Tri/RLE | Tableau global de candidats E, puis C éléments mais capacité brute conservée | Tri par permutation pour gros records ; route mono : ordre u32 et un record déplacé, multi : ordre + tampon de permutation. Le RLE efface des tailles, pas la capacité |
| Préfiltre/census | Candidats de capacité héritée, `Survivor[S]`, destination privée `BallData[S]` | Census direct v7 : une seule destination puis swap terminal, sans copie globale régulière ; l'ancienne double résidence v6 n'est pas héritée |
| Toute la tour de folds | Toutes les boules survivantes `BallData[S]` et l'index, plus petits compteurs par K | Survivants libérés à `run.hpp:622`, candidats à `run.hpp:671` ; `balls` reste pour chaque scan K |
| Expansion de K | `ForestEvent[e_K direct]`, shards d'expansion puis fusion | Chaque K rescane les boules ; la proposition D par swap n'est pas intégrée dans C |
| Complétion silencieuse K | Direct events, index ID→rang trié de taille m, catalogue trié de cofaces directes, records de cœur puis facettes de cœur uniques, set exact `completed`, chaîne courante, événements ajoutés | `silent_incidence.hpp:271–367`. Les catalogues privés meurent au retour ; `added.events` coexiste encore avec la destination pendant son append |
| Préparation du fold K | Événements, ordre u32[e_K], lots, clés globales de facettes, `ev_fid[11*e_K]` | Temporaire `ev_part[11*e_K]`, records d'incidence par tranche puis `parts[i_K]`, tables privées, pools de facettes, mapping tid→fid ; certaines tables meurent avant le suivant, mais les 64 pools sont conservés jusqu'à la fusion |
| Réduction K | Événements, ordre, lots, clés[f_K], `ev_fid`, `FidState[f_K]`, scratch du lot, deltas/CSR accumulés, niveaux des lots | `FidState` est réellement figé à 32 octets et `DeltaMeta` à 96 par static_assert ; tables, pools, scratch et deltas ne sont pas tous couverts par le budget partiel |
| Publication K | Clés de facettes, canon final par fid, deltas et événements encore visibles au callback | Le callback arrive après construction du résultat complet K. Le CLI écrit le fichier puis laisse libérer K ; conserver une copie dans un callback utilisateur ajoute une résidence hors garantie du pipeline |

Les 64 partitions par empreinte ne constituent ni 64 sous-problèmes HGP
indépendants ni une borne d'équilibrage : une partition peut contenir tout
le catalogue. L'égalité est décidée par clé exacte, pas par l'empreinte.

### Sommes de tour, simultanéité et G4

`total_facets` et `total_events` additionnent les cartes de chaque K. Les
19,47 millions de facettes à 8k et environ 84 millions à 32k transmis par le
pilote sont des **sommes sur les ordres**, pas la preuve de cette quantité
simultanément allouée. Les multiplier par `sizeof(FidState)` ne donne pas
le pic DSU.

Avec `threads=1, fold_inflight=1, fold_join_before_next_k=true`, le fold K
termine avant A(K+1) : un seul ordre réside, en plus de l'index et des boules.
Sans jointure, A(K+1) peut coexister avec B(K), même si inflight=1 ; avec
inflight L, la préparation se fait avant le reaping et peut ajouter un
ordre aux L slots. La taille des K diffère : le pic est une somme des
ordres réellement en vol, pas L fois une moyenne de tour.

La mémoire GPU n'agrandit pas automatiquement ces vecteurs CPU. C6a est une
route sous stub à lots de wire bornés, pas un backend externe ni une preuve
que le catalogue global `BallData` disparaît. Les résidences hôte, mémoire
épinglée, device et cache de pages demandent des budgets distincts. Aucun
inventaire ou chiffre de RAM/VRAM G4 courant n'a été interrogé dans ce travail.

### Ordre de grandeur spatial seul [E], pas un dimensionnement global

La taille logique exacte des tableaux d'index s'écrit
`8*m + sizeof(P3)*m + 4*(m+1) + 4*n + 8*(m+1) + sizeof(RadixNode)*(m-1)`.
Sur l'ABI x86-64 habituelle, le calcul de disposition donne P3=24,
InputPoint=32, RadixNode=120 et Rec=40 octets. **Sans nouvelle compilation
dans cette tâche, ce sont des calculs d'ABI, pas un reçu sizeof G4.** Pour
m=n=10M, l'index logique serait environ 1,68 Go et l'entrée 0,32 Go ; le
Rec de construction ajoute 0,40 Go, plus postfixe et surcapacités. À 50M,
le même calcul linéaire donne 8,4 Go pour l'index seul.

Ces valeurs ne portent ni E, C, S, e_K, i_K, f_K ni les scratchs : elles ne
prouvent aucune tour 10M/50M tenable. Les ratios historiques candidats ou
facettes par point ne sont pas extrapolés dans cette note.

## 4. Frontières de disque/checkpoint compatibles avec l'objet [P]

1. **Index spatial immutable.** Pagination des mêmes rangs et NodeRef,
   boîtes exactes et mappings PointId/rang, avec racine globale. Un cache
   change les accès, pas les requêtes : une page absente entraîne lecture
   ou refus, jamais « sous-arbre vide ». Aucune décision ne se limite à un
   halo spatial fixe. Une API d'accès remplace les vecteurs ; `mmap` seul
   ne fournit pas une borne RSS ni une politique de cache.
2. **Front WSPD par vagues et journal de rectangles.** Déposer les mêmes
   tâches/masques, mêmes critères de mort/séparation/scission, ordre canonique
   et grand-livre par lane. Une frontière de vague est simple à reprendre.
   Les rectangles terminaux peuvent être consommés puis déposés sans garder
   tout `alive`, à condition de vérifier la couverture exacte des tâches.
3. **Runs de candidats puis fusion/RLE externes.** Tri exactement selon
   `ball_candidate_less` : BallKey entière, arité minimale, représentation
   entière du niveau. Le premier de chaque groupe BallKey est le représentant
   actuel. Le fichier canonique permet le digest par streaming, avec le
   nombre connu avant le corps. Une frontière de run ne coupe pas un groupe
   d'égalités lors du RLE final. Pas de seau spatial ni hash comme autorité.
4. **Boules census / événements par K.** Étape ultérieure possible : blocs
   validés contre l'index global, puis fichiers d'événements et comptage
   exact. Les boules restent rejouables pour les K ; ce n'est pas une
   nouvelle obligation de les conserver toutes en RAM.
5. **Facettes et reduction.** Étape ultérieure plus importante : catalogues
   et dictionnaires exacts externes, DSU avec état persistant ou frontier
   d'incidences. Un checkpoint simple se place **après un lot entier de
   niveaux exacts égaux**, pas après une sous-liste arbitraire du plateau.
   Il doit conserver parent, canon, rôles/époques, curseurs, ordre stable
   des événements et deltas atomiques. Oublier une facette exige son dernier
   contact futur exact, avec comparaison de clé complète ; un hash seul
   ne suffit pas.

La laminarité est celle des composantes sur les facettes d'un K : garder
leurs identités stables, leurs unions et leurs niveaux exacts la préserve.
Une tuile de points traitée isolément perd les incidences transfrontières ;
elle ne peut pas reconstruire la même forêt par simple concaténation.
Le recouvrement projeté sur les points doit rester distinct de la partition
des facettes. Ces coupes ne créent ni cellules de Delaunay d'ordre supérieur,
ni catalogue exhaustif de tous les simplexes.

Un fichier K déjà écrit reste provisoire jusqu'au succès terminal. L'archive
actuelle offre une publication create-only après la tour, mais pas de
reprise d'un DSU ni du générateur. Ses champs `vertical_maps=none` et son
autorité horizontale ne deviennent pas un certificat vertical par l'ajout
de checkpoints. Le statut d'un checkpoint reste séparé de celui de l'objet.

## 5. Étape 1 bornée proposée pour viser 10M+

**Livrable : `index immutable + catalogue canonique de candidats externes`,
pas « MorseHGP 10M complet ».** Ne pas intégrer la chaîne du fold aujourd'hui.

- Entrée streamée validée, conservation de l'ordre original et des PointId ;
  tri externe des IDs pour détecter les doublons, puis tri Morton/PointId.
  Construire les mêmes buckets, rangs, liens et boîtes, dans un format explicite
  indépendant du padding C++. Les positions dupliquées gardent le refus actuel.
- Première variante : index global résident, candidat/front externes, afin
  d'isoler le gain des catalogues. Variante paginée seulement si le budget
  spatial constaté l'exige ; les invariants de requête sont identiques.
  Aucune réduction arbitraire de boîte, aucun changement d'owner ou de filtre.
- Remplacer seulement l'accumulation globale du front/candidat par des
  écrivains de runs bornés et un manifeste exact-once de tâches. Un run de
  débordement est obligatoire ; une tâche qui excède le budget de scratch
  refuse encore proprement. Les covers et racines d'une ancre ne sont pas
  magiquement bornés par cette première étape.
- Fusion externe avec fan-in et buffers bornés, compte exact u64, tailles
  de fichiers et offsets vérifiés avant écriture. Le catalogue entier n'est
  pas chargé pour comparer son digest. Les runs bruts et uniques ont des
  statuts distincts ; le RLE ne doit pas masquer une tâche rejouée deux fois
  dans le digest brut ou le grand-livre.
- Ne pas contourner le cap produit E<=UINT32_MAX. Si le catalogue externe
  exige un compteur supérieur, ouvrir un **format/stage distinct versionné**
  et le qualifier ; le pipeline C en mémoire reste explicitement inapte à
  l'importer. Même sous ce cap, C ne promet pas de charger toutes les boules
  puis tous les folds. Relever les constantes n'est pas cette étape.
- Checkpoint : source/profil/paramètres/contrat flottant, identité de l'entrée,
  index et schémas, tâches closes et restantes, grands-livres, taille/compte/
  hash de chaque run, ordre de fusion. Publication après fermeture et sync
  des fichiers ; EIO, ENOSPC, interruption et préemption gardent le dernier
  manifeste valide, jamais un catalogue annoncé complet avec une queue
  manquante. Le contrat de durabilité doit être déclaré, pas hérité du mot
  « atomique ».

**Critères de passage avant toute campagne massive :** comparaison appariée
avec C sur petites scènes non vacues, y compris refus ; mêmes arrays d'index,
grands-livres, records triés et digests brut/unique ; frontières de run
coupant volontairement des clés égales ; runs vides, flux très déséquilibrés,
hash constant pour les éventuels index d'adressage ; task perdue/dupliquée,
fichier tronqué/corrompu, reprise après chaque frontière, disque plein et
cache froid. La mémoire du writer/merge et le nombre de fichiers ouverts
ont un plafond mesuré ; l'espace disque de haute eau est préflighté.

La réussite de cette étape lève un catalogue global, pas les limites de
complétion silencieuse ni celles du fold. Un refus à 10M reste un résultat
valide de qualification, pas une excuse pour changer d'objet ou de garde.

## 6. Ancrage des sources lues

Le [reçu d'analyse](../receipts/residence_massive_20260904/analysis.json)
et son [inventaire de sources](../receipts/residence_massive_20260904/source_snapshot.json)
épinglent ce port documentaire : fichiers lus, hashes avant/après, gardes
littérales et calculs d'entiers. Ils ne remplacent pas les arguments ci-dessus
par un test ni ne constituent un reçu de build.

Hashes SHA-256 des fichiers centraux au moment de la lecture (sélection,
pas fermeture complète de dépendances ni reçu de build) :

```text
core/caps.hpp d388c877bf4ca8c0fc164148f176959530a0ceff3052640402db3dcd3c256912
tree/cloud_index.hpp 8c5acf166ce378b0271e15850c54ca1740a8f6cb899d34a60c832a533504ad95
pipeline/run.hpp 1999f901fb44caf3ca743e77e64bb3e5765070fa01a369447b9e89be21ce728c
pipeline/generate.hpp ee2a4a1f96875c7db1fbd054700a22db6eabb8f62379c71c0ed6728f1b18de59
pipeline/expand.hpp 7cafb0341344fbc7d1584001e4685e2e5bf0122fe3b7e37277f5468d5c5e1cf0
pipeline/digest.hpp d0def105ebb969c9e95b10f68e0dd480ca9fa05b2553c4a6307a0c460893e668
forest/fold.hpp b11d02c86db5f8ae8cb12965f12e425548f9f049fb4626259790b32cd584928c
forest/silent_incidence.hpp fddde6e233eea8e80d23af4d42b50952e7c49a50ed84357e973279ff14d555e8
io/archive.hpp cc2243aaa1bdbe63b69f165d65152cf62d7fac32ff6c641343542c247d989430
```

Les mentions historiques de tailles ou de débit dans les commentaires v6
restent historiques ; cette note prend les branches actuelles comme autorité.
Le candidat D d'adoption de shards reste un overlay séparé et n'entre pas
dans cet ancrage C.
