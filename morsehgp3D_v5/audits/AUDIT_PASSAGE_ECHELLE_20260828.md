# Conception auditée de résolution — passage à 10–30 millions de points

- **Derniers commits techniques relus pour ce sujet :** `bc66ade7` pour le
  réducteur vivant durci, `fb7e9d40` pour la
  porte de préfixe durcie et `f4b554fe` pour le smoke T5
  `(catalogue, deltas) -> partition`, ainsi que `194a0bc2` pour la sonde miroir
  rejetée ci-dessous. Les changements G1 restent indépendants du passage à
  l'échelle du fold.
- **Cadre :** `phase=exploration_v5_hors_registre`,
  `backend=cpu_reference`, `profile=quantized_u16_input_only`,
  `mode=audit_independant_math_and_architecture`,
  `public_status=not_claimed`.
- **Périmètre :** résolution constructive des verrous T3–T6, amont externe,
  payload et reprise. Aucun résultat 1 M, 10 M ou 30 M n'est revendiqué.

## Verdict utile à Claude

Le plan révisé est nettement meilleur que sa première version. Il abandonne le
tuilage spatial du fold et ses halos, nomme le payload, distingue le digest du
flux du digest v4, relève les budgets de sortie et de disque, retient
`resume=replay_current_K` comme première reprise réaliste et fait du préfixe
K ≤ 5 un jalon autonome. Ces décisions sont conservées.

Le verrou restant n'exige pas un reroot compliqué de l'union-find. Il peut être
supprimé par une représentation plus simple :

> conserver uniquement les alias des facettes qui auront encore une incidence,
> stocker chaque composante comme une liste de ces alias et séparer sa racine
> sémantique de son conteneur physique.

Avec une prépasse PREMIÈRE/DERNIÈRE exacte sur les `FacetKey` complètes, cette
représentation donne un majorant **cardinal du noyau** : à toute frontière de
lot, toute composante conservée contient au moins une facette encore vivante.
Ainsi le nombre de composantes résidentes est au plus le nombre de facettes
vivantes. Cela ne majore pas encore index, scratch d'un macro-lot, sorties,
capacités d'allocateur ni octets engagés. T3, T4 et T6 sont remplacés utilement
par un invariant testable, puis par un budget par rôle à mesurer.

### Réception positive de la porte de préfixe

`ba31c169` a fermé l'ancien angle mort du digest v4 en couvrant le tuple
événement complet et tous les `batch_levels`. `fb7e9d40` ferme ensuite les trois
durcissements demandés : plancher correctement nommé `tie_excess`, fixture
explicitement décrite sans fausse cocircularité et mutants propres aux champs
d'événement et niveaux de lots. Ces remarques sont closes et ne doivent plus
être remises dans l'ordre de travail.

### Pin L2 `bc66ade7` — raccords reçus, autorité T5 encore trop faible

Le pin garde le différentiel nominal du réducteur et applique correctement la
majeure partie des raccords demandés : compte exact des alias avant et après les
morts à chaque lot, cohérence `idx.used`, balayages structurels ajoutés, vacuité
finale, deux témoins de fraction séparés, maximum de déplacements par alias,
cas small-to-large adverse et timeouts. Les cinq injections rendent rapidement
le code 4 annoncé. D et E opposent enfin l'ordre logique à l'ordre des clés. Ce
cœur doit être conservé.

La porte des fixtures doit toutefois comparer tout ce qu'elle annonce : son
rendu omet encore `level`, `batch_levels`, refus et compteurs ; A-300 et E-50
n'ont pas d'attente littérale, et le plancher global de 6 sur 7 appels ne
protège pas chaque motif. Ajouter un comparateur complet et des planchers
nommés, ou annoncer précisément cinq sorties littérales plus deux stresses
différentiels.

Le mutant de coût peut légitimement être tué par un plafond théorique, mais sa
neutralité sémantique doit précéder son code 4. Le chemin synthétique retourne
actuellement dès le dépassement après une comparaison plus courte que la porte
nominale. Comparer d'abord deltas complets, niveaux, compteurs, détecteurs et
partition ; toute divergence rend 1 ou 3, puis seulement le dépassement
non-vacant rend 4. Le maximum par alias est le témoin causal ; la borne agrégée
peut rester redondante.

Le « rejeu T5 » local est une connectivité finale conditionnelle, pas encore le
replayer indépendant annoncé. Il ignore `output`, lots, niveaux et clés hors
catalogue, puis compare au `final_canon_fid` du même résident sans contrôle
négatif. Extraire le replayer strict de `delta_replay_gate.cpp` dans l'oracle et
le partager avec la porte vivante ; une clé absente ou un delta incohérent doit
être rejeté. Cela ferme T5 sans mettre l'oracle sur le chemin produit.

Les fixtures demandent encore deux dents distinctes. Le hash constant ne touche
que `FacetIntern`, jamais les collisions et le décalage arrière de `LiveIndex` :
ajouter une petite table traversant 15 vers 0, supprimer tête, milieu et queue,
puis vérifier `get`, `used_` et réinsertion. Les trois triangles de D reçoivent
l'ordre des sorties mais portent neuf alias ; conserver D et ajouter un unique
simplex K = 2 qui vérifie explicitement le pic transitoire 3 et la vacuité.

Les scans structurels doivent rester un oracle de test. Le stride entier actuel
peut produire jusqu'à 127 scans malgré le commentaire « au plus 64 » et le
parcours déréférence `av[x]` avant d'avoir borné `x`. Utiliser un stride plafond,
contrôler bornes, backlinks, nombre de composantes, table et free-lists, puis
désactiver ces parcours dans le chemin mesuré. Les compteurs de vie exacts
restent, eux, une garde O(1) par frontière.

La formule de porte doit aussi être distinguée du théorème. Le code autorise
`ceil(log2(F + 2)) + 1`, alors que le doublement small-to-large donne la borne
plus nette `floor(log2(F))` par alias. La première peut rester une marge sûre,
mais les documents ne doivent pas les écrire comme une seule formule.

La mesure mémoire reste un ratio de structures choisies. `g_alloc_bytes` prend
un maximum global et l'imprime avec l'ordre du maximum d'alias ; les témoins
peuvent diverger. Échantillonner aussi après les morts, où les free-lists
grandissent, et conserver chaque valeur avec son ordre. Scratchs, sortie,
FIRST/LAST, catalogue et `ev_fid` restent hors de ce compte : ne jamais le
nommer RSS ni mémoire bout en bout.

`free-on-absorb` ne recycle plus ses alias mais recycle encore leur composante,
dont ils gardent l'indice. Son code 4 Release est reçu ; le vert ASan/UBSan est
rapporté par Claude mais n'a pas été rejoué par cet audit. Le mutant reste
diagnostiquement sale. Enfin, aucune
accélération CPU/RSS n'est reçue. La sonde de `194a0bc2` relance un fold
depuis `on_forest`, donc mesure le résident déjà vivant plus le second
réducteur. Un miroir attribuable exécute dans deux processus le même flux
d'événements avec exactement un réducteur par processus, replayer strict et
digest commun inclus côté vivant.

## Solution 1 — fold vivant sans ancienne forêt union-find

### État minimal

Pour un ordre K, le réducteur conserve :

- un `Alias` par facette réutilisable, avec `fid_u64`, `last_batch`, `seen`,
  rôles du lot, pointeur de composante et liens intrusifs ;
- un `Component` par composante ayant au moins un alias, avec
  `logical_root_fid`, `canon_fid`, `historical_mass` et la liste de ses
  alias ;
- les scratchs du lot : facettes touchées, parents pré-lot, naissances et
  sorties post-lot.

`logical_root_fid` reproduit exactement la règle actuelle « la racine du
composant de `first` absorbe ». `canon_fid` reste le minimum de toutes les
facettes historiques de la composante. Le record qui porte physiquement la
liste n'a aucune autorité sémantique. Sur le chemin massif, aucune `FacetKey`
n'est nécessaire dans cet état si les fid ont été attribués par le tri exact
des clés complètes et si le join des incidences est collision-safe : le
catalogue externe `fid -> FacetKey` reste l'autorité de rematérialisation.

Aux quatre frontières d'un lot `b`, l'état exact est :

```text
avant le lot       : F(x) <  b <= L(x)
après rôles/FIRST  : F(x) <= b <= L(x), nouvelles facettes singletons
après unions       : mêmes alias, partition post-lot
après émission     : F(x) <= b <  L(x)
```

### Union exacte et compacte

Pour chaque union ordonnée `unite(first, other)` :

1. capturer comme nouvelle racine logique celle du composant **courant** de
   `first`, à chaque union réussie ;
2. garder physiquement le record de plus grande `historical_mass` ;
3. relier les alias du plus petit record au plus grand ;
4. écrire le minimum des deux canoniques et la somme des masses ;
5. détruire le record physique devenu vide.

Le choix small-to-large peut donc être opposé à l'absorption sémantique sans
changer le résultat. Chaque alias déplacé entre dans un composant dont la masse
historique a au moins doublé, d'où au plus un nombre logarithmique de
relocalisations par alias. Les historiques des composantes vivantes restent
disjoints et leur masse totale est au plus le nombre de facettes ; l'addition
est néanmoins gardée contre le débordement. Le coût cumulé des déplacements est
donc `O(F log F)`. Le tri des deltas par `logical_root_fid` reproduit
le tri actuel de `post_list`, même lorsque le gagnant physique est l'autre
composant ou que la facette racine n'a plus d'alias vivant.

Les pré-composants gelés ne doivent jamais conserver un pointeur vers un
`Component`, puisque le record perdant peut être détruit. Employer
`PreRef{witness_alias, pre_canon_fid}` : un alias ACTIVE reste vivant jusqu'à
l'émission et son pointeur de composante suit chaque déplacement. La
construction post-lot retrouve ainsi le composant final par le témoin stable.

Le lot doit rester une opération en deux passes **physiquement relisibles** :

1. calculer tous les rôles et figer les composantes/canoniques pré-lot ;
2. rechercher ou relire exactement le même byte-range et rejouer les unions
   dans l'ordre total des événements.

Une réduction événement par événement changerait les multifusions à niveau
égal et n'est pas admissible. Garder le macro-lot en RAM annulerait également
la borne vivante : le scratch doit rester proportionnel aux alias touchés, pas
au nombre d'événements du niveau.

### Libération prouvable

Les alias dont `last_batch == b` sont supprimés seulement après l'émission du
lot `b`. Si une composante n'a alors plus aucun alias, aucun événement futur
ne peut l'atteindre : une connexion future devrait nécessairement réutiliser
une de ses facettes, en contradiction avec leurs dernières incidences. Le
record est donc définitivement libérable.

Pendant le lot, les facettes telles que `first_batch <= b <= last_batch` sont
comptées. Après le lot ne subsistent que celles telles que
`first_batch <= b < last_batch`. Les facettes nées et mortes dans le même lot
sont ainsi bien budgétées, contrairement à la sonde historique échantillonnée
après le lot.

Conséquences utiles :

- aucune chaîne de parents morte, aucun reroot et aucun refcount d'ancêtres ;
- `components <= aliases <= peak_live_exact` ;
- les listes touchées, parents et naissances sont bornées par les alias du lot,
  mais un macro-lot peut être trop grand pour la RAM et doit alors être relu en
  deux passes depuis une tranche externe ;
- la partition finale n'impose pas de garder les membres morts en RAM, si son
  rejeu depuis le catalogue et les deltas est reçu séparément.

Cette borne n'est effective que si les slots `Alias` et `Component` sont
recyclés. Une arène append-only conserverait les morts et annulerait le gain.
L'index vivant est préalloué sur le pic exact, et la porte compare aussi octets
engagés et high-water des scratchs au budget de la phase.

## Solution 2 — PREMIÈRE/DERNIÈRE et préflight exacts

L'empreinte 64 bits peut servir à router des fichiers, mais jamais à décider
l'identité. Aucune marge fixe appliquée au pic par empreinte ne constitue un
majorant déterministe : une collision adversariale peut agréger un nombre
arbitraire de clés.

La prépasse exacte par ordre est :

1. attribuer un `event_rank_u64` au flux déjà trié par
   `(ExactLevel, BallKey, emit_rank)` et un `batch_id` par égalité sémantique
   `compare_exact_level` ;
2. émettre chaque incidence sous la forme logique
   `(FacetKey complète, event_rank_u64, slot)` ;
3. partitionner éventuellement par hash pour les E/S, puis trier chaque
   partition par `(FacetKey, event_rank, slot)` et comparer l'identité sur la
   `FacetKey` complète ;
4. fusionner les partitions en ordre lexicographique, attribuer les `fid_u64`,
   prendre les extrema de chaque groupe de clé et les convertir en lots pour
   marquer exactement une incidence FIRST et une LAST ;
5. retrier le join `(event_rank, slot, fid, flags)` dans l'ordre du fold.

Le pic inclusif par lot est ensuite calculé sans heuristique :

```text
live += first_count[batch]
peak = max(peak, live)
live -= last_count[batch]
```

La première porte doit injecter un hash constant. Le résultat, FIRST/LAST et le
pic doivent rester identiques ; un mutant `lifetime-by-hash-only` doit
diverger.

Cette exactitude a un coût de wire. Le reçu `uniform` 200 k contient
769 871 673 incidences, soit 38,49 milliards par extrapolation ×50 à 10 M. Un
wire logique compact `4K + rank_u64 + slot_u8` vaut déjà environ **1,60 To
brut tous K**, dont environ 569 Go pour K = 10 seul, avant cadrage, tri et join.
La ligne « 620 Go par empreinte + position » et le minimum d'E/S de 107 minutes
sont donc obsolètes. L2 doit graver la taille sérialisée de
`FacetOccurrenceWire`, les octets écrits/lus et le facteur temporaire du tri
K par K, en distinguant pic disque par K et volume cumulé. Une compression
préfixe après tri est possible ; elle ne remplace pas la comparaison exacte.

## Solution 3 — prouver d'abord que les deltas suffisent

`f4b554fe` extrait désormais une porte de rejeu substantielle : union-find
frais, 58 ordres, 733 029 deltas, 5 194 737 facettes et deux mutants
d'intégration tués. Son claim utile est exactement :

```text
catalogue de facettes + deltas -> final_canon_fid reconstruit
```

Cette implication reste **conditionnée à un flux accepté**, notamment
`attach_violations == birth_violations == partition_violations == 0`. Elle est
fausse sur une entrée dont le détecteur de rôles lève déjà un invariant. Le
contre-exemple minimal K = 1 est :

```text
lot 0 : événement {A,B}, active_mask=0b11
lot 1 : événement {A,C}, active_mask=0b01
```

Au lot 1, `A` est un attachement déjà vu et `C` est active. Le résident a pour
partition finale `{A,B,C}`, mais son delta `parents=[C], born=[A], output=A`
fait remplacer au rejeu le bloc `A={A,B}` par `{A,C}` et perd `B`. La porte
doit donc rejeter ce flux, et l'énoncé T5 doit porter explicitement sa
précondition au lieu de promettre une reconstruction inconditionnelle.

Attention en gravant cette fixture : dans `ForestEvent`, le bit `s` qualifie la
facette obtenue en retirant `support[s]`. Les commentaires actuels de
`detector_gate` inversent donc les noms des facettes ; les compteurs existants
sont bien déclenchés, mais pas par les facettes que ces commentaires annoncent.

La porte actuelle reçoit un **smoke de cohérence**, pas encore toute cette
autorité : catalogue, deltas et partition attendue proviennent du même
`ForestResult`, et sa validation d'états/batches reste permissive. La renforcer
avant le réducteur streamé, sans bloquer G0/G1 :

- borner explicitement le claim à
  `(catalogue, deltas) -> final_canon_fid`, jamais au `ForestResult` complet ni
  à la source sémantique du catalogue ;
- suivre `unseen`, `introduced`, `alive`, `absorbed`, avec singleton implicite
  distinct d'une naissance ;
- valider entièrement un delta avant union : parents/born triés uniques,
  racines pré-delta distinctes et vivantes, clé présente, sortie minimale,
  refus du no-op ;
- juger `batch`, `level`, `batch_levels` et `batches`, en gelant les canoniques
  vivants au début du lot pour interdire une chaîne intra-lot artificielle ;
- graver de petites fixtures pour doublon, clé hors catalogue, sortie non
  minimale, parent ressuscité, singleton puis naissance, chaîne intra-lot,
  niveau/batch invalide et catalogue incomplet.

Les deltas seuls ne reconstruisent pas les clés : le contrat demeure
`(catalogue, deltas) -> final_canon_fid`.

Le wire massif peut alors référencer les facettes par `fid_u64` et contenir,
par K :

- le catalogue des `FacetKey` uniques, trié une fois ;
- les lots critiques avec `legacy_batch`, niveau exact et deltas
  `output/parents/born` ;
- un sidecar explicite et versionné si les niveaux des lots sans delta restent
  contractuels ;
- un digest logique indépendant du découpage physique ;
- dans le manifeste, tailles et SHA-256 des fichiers physiques.

Le digest v4 n'est pas un format massif : `ForestResult::final_canon_fid` et
`mhgp4-digest-v1` sérialisent des `u32`. Pour `uniform`, les 182 530 632
facettes K = 10 à 200 k extrapolent à 9,13 milliards à 10 M, au-delà de
`UINT32_MAX`; ce n'est pas une loi pour toute famille. Le convertisseur v4
reste donc une porte différentielle bornée ; le flux à grande échelle doit
avoir son wire et son digest u64 propres. Aucun cast silencieux n'est acceptable.

## Solution 3 bis — paralléliser le fold sans rendre les unions commutatives

Le verrou de `logical_root_fid` n'est pas une impossibilité mathématique. Il
interdit seulement une union parallèle naïve par minimum ou par taille. Le
premier candidat exact, encore à confronter au fold reçu, est un **quotient
local par lot** à `W=1` :

1. agréger les occurrences par fid, avec OR des rôles, FIRST/LAST et une seule
   résolution `fid -> alias` ;
2. créer un sommet local dense par composante pré-lot touchée et par nouvelle
   facette ; porter sur ce sommet racine logique, canonique historique et masse,
   puis geler parent et naissance avant toute union ;
3. rejouer dans ce petit DSU les étoiles dans l'ordre stable actuel. Pour
   chaque slot, la racine locale contenant `first` absorbe l'autre, sans
   union-by-size ;
4. grouper ensuite en parallèle par racine finale les parents gelés, les
   naissances, le canonique minimum et la masse historique ;
5. trier les groupes par `logical_root_fid`, émettre les mêmes deltas, puis
   matérialiser une seule fois dans le plus grand record physique les groupes
   qui gardent au moins une facette après le lot ; émettre puis jeter depuis le
   scratch les groupes entièrement éphémères. Les morts persistantes ne sont
   appliquées qu'après l'émission.

Une facette avec `FIRST == LAST == lot` peut rester une feuille de scratch si
ses rôles et détecteurs, son éventuel parent ou naissance, son canonique et sa
racine logique sont transférés avant sa disparition. Elle n'entre alors jamais
dans `LiveIndex`, l'arène d'alias ou une composante durable. Les facettes
persistantes ne paient qu'une recherche d'index par lot. Le cœur séquentiel
subsiste, mais il devient un DSU dense cache-local ; agrégations, tris et
mouvements groupés deviennent indépendants.

L'égalité se prouve par induction sur les étoiles ordonnées. À chaque arête,
les DSU globaux et locaux joignent les mêmes classes ou ne font rien ; lorsqu'ils
joignent, la classe de `first` gagne dans les deux. Partition et racine logique
restent donc identiques. Le canonique est le minimum des **canoniques
historiques gelés** des pré-composantes et des nouvelles facettes, pas le
minimum des seuls fid encore vivants ; parents et naissances sont ceux gelés
avant le lot. Leurs tris et celui des racines restituent alors exactement
`ComponentDelta`. Choisir le record de plus grande masse historique pour la
matérialisation conserve aussi small-to-large : tout alias déplacé rejoint une
masse au moins double de celle qui le contenait.

À `W=1`, le scratch peut déjà atteindre toutes les facettes et composantes
touchées par un plateau. Il doit donc être borné en octets avec un chemin de
repli vers le fold vivant actuel. Une fenêtre de plusieurs lots complets
pourrait ensuite retarder les seuls mouvements physiques tout en rejouant lots,
deltas et morts logiques dans l'ordre ; elle ne doit jamais couper un niveau
exact. Commencer par `W=1`, car des lots étroits peuvent rendre le scratch plus
cher que la boucle actuelle et `W>1` n'est pas encore reçu.

Une voie entièrement parallèle est ensuite un **candidat sous preuve**. Donner
à chaque arête étoile la clé totale `(niveau exact, rang stable événement, rang
du slot)` permet de retrouver les unions acceptées par une forêt de Kruskal.
Cela ne restitue toutefois pas à lui seul parents, naissances, morts et
`batch_levels` à chaque préfixe de lot. Un arbre de reconstruction qui préfère
à chaque fusion le fils contenant `first` est une piste pour
`logical_root_fid`, et le minimum des canoniques historiques gelés une piste
pour le canonique. Ce MSF/KRT peut permettre des fenêtres plus larges, mais
risque de rematérialiser un état proportionnel au flux. Ne le coder que si le
profil du quotient dense montre que le replay séquentiel reste dominant et
qu'une preuve de reconstruction préfixe par lot a été écrite.

Avant branchement produit, comparer lot par lot classes, racines logiques,
canoniques, deltas, compteurs et représentation des niveaux ; exercer les
fenêtres `1,2,3,7,31`. Ajouter un plateau connecté où inverser deux étoiles
change l'ordre des deltas sans changer partition ni canoniques, ainsi que les
mutants `parallel-union-by-min-or-size`, coupure de plateau et gel du lot
suivant trop tôt. Mesurer séparément index/rôles, gel, replay, mouvements,
deltas et morts, plus la fraction de feuilles éphémères. Les ouvriers intra-B
doivent consommer le même budget global que les B concurrents entre K.

## Solution 4 — amont externe plus simple

Le Morton du centre reste une bonne optimisation de localité, mais il n'est pas
nécessaire à l'exactitude et ne borne pas un seau chaud. Le premier amont
streamé peut être plus direct :

1. traiter les rectangles par vagues bornées et écrire des runs de
   `BallCandidate` triés avec le comparateur produit actuel ;
2. effectuer une fusion externe globale et le RLE exact sur la clé complète ;
3. calculer `digest_balls` au passage ;
4. préfiltrer, censer puis expanser chaque boule unique une seule fois ;
5. envoyer chaque événement directement dans le buffer/run de son K avec
   `BallKey` source et `emit_rank` ;
6. trier extérieurement chaque flux K par sa clé totale.

Sur l'extrapolation `uniform`, cette couture évite de matérialiser d'abord
environ 1 To de `BallData` et évite la ré-expansion répétée par ordre. Le centre Morton pourra ensuite
partitionner le census pour la localité, avec spill obligatoire, sans devenir
une autorité de complétude ou de capacité.

La première porte de cet amont est petite : découper les mêmes candidats en
runs de tailles 1, 2, 3 et 7, fusionner/RLE, puis comparer au
`rle_candidates` résident. Elle ferme une couture réelle avant tout pilote
SSD.

## Fixtures qui résolvent les risques au lieu de les reporter

1. **Étoile K = 1 de 300 arêtes à niveaux croissants** : tue tout compteur
   d'incidences `u8`, conserve un canonique dont la facette est morte et force
   souvent gagnant physique et racine logique à différer.
2. **Chaîne K = 1** : événements `{0,1}`, puis `{0,2}` ; l'ancienne racine
   n'a plus d'incidence propre mais la composante continue.
3. **Deux triangles K = 2 partageant une arête** : ferme l'analogue d'ordre
   supérieur. Deux tétraèdres partageant une face relèveraient de K = 3.
4. **Plateau mono-lot** : un seul événement q3 donne trois facettes
   FIRST = LAST et un pic transitoire de trois, jamais zéro.
5. **Grand composant absorbé logiquement par un singleton** : le stockage
   small-to-large garde le grand record, tandis que `logical_root_fid` doit
   rester celui du singleton.
6. **Frontières externes** : même facette dans plusieurs runs et hash forcé
   constant.
7. **Chemin mono-plateau `{A,B}`, `{B,C}`** : les trois pré-composants sont
   gelés avant toute union et les deux événements sont relus, jamais réduits
   séparément.
8. **Attachement déjà vu** ci-dessus : tue l'énoncé T5 inconditionnel et exige
   le rejet du décodeur strict.

Mutants prioritaires : `last-mark-shifted`, `free-on-absorb`,
`root-key-mutable`, `canon-not-min-on-union`,
`lifetime-by-hash-only` et `physical-root-is-logical-root`.

## Reprise minimale sûre

Conserver `resume=replay_current_K` :

1. entrées de phase immuables et hashées ;
2. sortie du K courant dans un temporaire unique ;
3. `fdatasync` ou `fsync`, relecture et validation du hash ;
4. renommage vers le nom final puis `fsync` du répertoire ;
5. manifeste temporaire synchronisé, renommé atomiquement, puis répertoire
   synchronisé.

Une coupure laisse au pire un orphelin non référencé. Le K courant est rejoué ;
un K précédent n'est repris que si son manifeste est `committed`. Ne pas
annoncer de reprise au lot tant que toute la map vivante, les composants,
l'état du digest et les offsets de sortie ne sont pas sérialisés.

## Ordre de commits proposé

1. **Durcir T5 sans bloquer les lanes** : promouvoir, sur flux accepté,
   `catalogue + deltas -> final_canon_fid`, puis graver les fixtures d'états et
   de lots ci-dessus.
2. **Réducteur vivant en RAM** : FIRST/LAST par clé exacte dans une
   `std::map`, composants small-to-large et égalité complète avec le résident.
3. **Coutures externes** : RLE multi-runs, lifetime avec hash constant, puis
   join retour vers les événements.
4. **Payload/reprise** : wire u64, digest logique et publication atomique par K.
5. **Pilote 1 M** seulement après mesure des octets, du pic inclusif et du
   débit du disque réellement attaché.

Cet ordre donne à Claude trois petits commits falsifiables avant le chantier
SSD. Il conserve son architecture générale tout en retirant les deux
inconnues les plus risquées : la fermeture union-find et la marge
probabiliste.

## Corrections documentaires restantes

- **fermés :** le cadre, le statut différentiel v3/v4, la direction fold vivant
  et clé complète à `46f9f8c7`; la porte de préfixe et ses renforcements à
  `ba31c169` puis `fb7e9d40`; le smoke T5 à `f4b554fe` ;
- supprimer du premier § 4.3 la compression/reroot, l'ancien T3/T6 et « même
  union-find séquentiel », en contradiction avec le fold vivant ajouté plus bas ;
- fusionner T3 et T6 en lemme du noyau vivant, corriger sa vieille fixture
  « deux tétraèdres à K = 2 », et écrire T5 comme
  `(catalogue, deltas) -> final_canon_fid` **sous invariants de rôles et de
  partition acceptés** ;
- synchroniser § 3.4, la table § 4.2 et L0–L4 : `22/pt ×2,5`,
  `composants=?`, RAM 110–140 Go et l'ancien coût PREMIÈRE/DERNIÈRE ne
  décrivent plus le plan retenu ; marquer ces capacités à recalculer après L2 ;
- choisir un seul nom entre `mhgp5-forests-stream-v1` et
  `facet_hierarchy_stream-v1` ;
- renommer T8 : le découpage déterministe n'est pas un théorème d'équilibre ;
- retirer « surgénération aux frontières de seaux » si chaque rectangle est
  émis exactement une fois ;
- ne plus promettre le digest v4 au-delà du domaine u32 ;
- qualifier explicitement les extrapolations `uniform`, et séparer pic disque
  K-par-K du volume cumulé.

GCP non utilisé pour cet audit.
