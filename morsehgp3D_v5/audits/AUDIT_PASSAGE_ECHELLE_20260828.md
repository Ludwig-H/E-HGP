# Conception auditée de résolution — passage à 10–30 millions de points

- **Derniers commits techniques relus pour ce sujet :** `fb7e9d40` pour la
  porte de préfixe durcie et `f4b554fe` pour le smoke T5
  `(catalogue, deltas) -> partition` ; HEAD observé `556c421e`, dont les
  changements G1 sont indépendants du passage à l'échelle du fold.
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

### Prototype L2 concurrent — cœur prometteur, porte à corriger

Le prototype non committé `fold_live.hpp` reproduit nominalement le résident :
58 ordres, 5 194 737 facettes, mêmes deltas/compteurs et zéro invariant violé.
Après contre-audit, sa porte nominale passe aussi : 5 660 568 relocalisations,
zéro invariant, 15 grands ordres mesurés et un pic d'alias de 7,29 % sur ceux-ci.
La fausse borne quatre fois trop stricte a été remplacée par la vraie borne
small-to-large. Le commentaire initial sur `--reloc-ratio` reste à retirer ;
une fixture où un nouveau singleton logique absorbe répétitivement une grande
composante encore vivante doit encore tuer causalement
`physical-root-is-logical-root`, de préférence via un compteur maximum par
alias plutôt qu'un ratio empirique agrégé.

Le mutant `free-on-absorb` initial recyclait un slot `Component` encore
référencé et bouclait à 100 % CPU. Sa réécriture ne boucle plus, mais l'essai
courant termine par un core dump au lieu du code 4 : elle n'est toujours pas
structurellement sûre. Remplacer ce mutant par une faute qui conserve listes et
index mais viole l'invariant attendu, auditer la structure avant recyclage et
borner tous les CTests du réducteur par `TIMEOUT` ; ni hang ni crash ne doit
tenir lieu de mutant tué.

Deux contrats sont encore plus faibles que les commentaires : T6 compare les
alias de chaque frontière au **pic global** des durées de vie, alors que les
tableaux déjà construits permettent l'égalité
`live_aliases == live_exact[b]`. Et la porte compare les vecteurs de deltas sans
les rejouer : T5 reste donc `(catalogue externe, deltas) -> partition`, jamais
`deltas -> facet_keys`, notamment à cause des singletons implicites. Raccorder
le rejeu existant avec le catalogue du résident ferme cette couture sans
modifier le chemin produit.

Ce prototype ne démontre pas encore le gain CPU visé. Son chrono `reduce`
commence après la passe FIRST/LAST et ses allocations ; `live_bytes_peak` omet
ces tableaux, les capacités d'arènes/free-lists, scratchs, `keys` et `ev_fid` ;
la porte ne compare aucun temps résident/vivant. Mesurer les deux chemins sur
le même périmètre complet, avec RSS, et ajouter plateau mono-lot, chaîne à
racine morte, absorption adverse et hash constant. L'exactitude nominale est un
progrès réel ; le gain de mur et de mémoire reste à établir. En attendant,
renommer `live_bytes_peak` en `logical_live_bytes` évite de lui attribuer une
mesure d'allocation qu'il ne porte pas.

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
