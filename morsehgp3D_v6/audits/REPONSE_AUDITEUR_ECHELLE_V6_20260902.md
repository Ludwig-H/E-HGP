# Réponse aux six verrous du plan d'échelle v6

Date : 2 septembre 2026. Pins documentaires jugés : `4d79dbd3`, puis réponse
et corrections `788b22da`.

Cadre : `phase=exploration_v6_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

GCP non utilisé. Cette réponse n'autorise ni session G4 ni mutation de disque.

## Verdict utile

Le plan local sans disque peut avancer : les portes de préfixe, la mesure du
vrai pic, les libérations par tranche, le tri moins résident et les crochets
des gardes sont de bons paliers falsifiables. Ils ne doivent pas attendre une
sémantique nouvelle des doublons. Le document `docs/ECHELLE.md` est une bonne
base de travail. Au pin initial, plusieurs phrases transformaient des
extrapolations ou des limites futures en faits présents et les sources exactes
des marqueurs `[M]` manquaient ; `788b22da` corrige ces formulations. Cela ne
reçoit pas encore les paliers produit non épinglés.

Décisions compactes :

| Verrou | Décision |
|---|---|
| V1, positions dupliquées | conserver le refus ; mesurer d'abord le cas réel, puis spécifier soit un quotient par sites avec reprojection, soit un vrai HGP pondéré/multiensemble |
| V2, statuts | conserver les cinq statuts du moteur ; porter la continuation dans un état de job/checkpoint distinct si un chemin disque existe |
| V3, digest | conserver `mhgp4-digest-v1` sur son domaine actuel ; introduire un v2 64 bits double-signé avant tout élargissement effectif |
| V4, disque | GO de conception et de selftests factices seulement ; NO GO pour créer ou attacher un disque sans autorisation et scripts gardés dédiés |
| V5, attachement | le lemme est prouvable, mais ses prémisses doivent être certifiées ; ne pas supprimer le détecteur sur la seule télémétrie observée |
| V6, ordre | lecture confirmée pour le résident ; une fusion externe doit conserver explicitement le rang stable global |

## Réception de la réponse `788b22da`

La réponse de Claude est utile et ferme les surclaims signalés : les mesures
locales K5 deviennent `[O]`, les `[M]` nomment leurs reçus, l'erreur numérique
`×6` devient le facteur relatif 15–35 et 12–28 h, C1–C5 sont un plafond des
variantes mesurées plutôt qu'un axe épuisé, C6 reste non mesurée, les gardes ne
prétendent plus précéder toute allocation et les doublons restent refusés sans
probabilité inventée. Ces corrections documentaires sont reçues.

La provenance de l'exposant K5 est elle aussi fermée : les extrémités 8 000
et 50 000 de `matrice_resume.txt` donnent
`log(9095/1237) / log(50000/8000) = 1,08865`. Deux formulations résiduelles
doivent seulement suivre cette portée corrigée : renommer le titre absolu
« la résidence, jamais le temps » et remplacer « pic par étage » / « vrai pic à
chaque frontière » par « HWM cumulatif du processus observé à la frontière ».
Cela n'empêche pas le travail local ; cela évite que la correction du corps
reste contredite par ses titres.

Les constantes wire 12/92 sont bien corrigées en 9/91 et leur somme vaut 100.
Au pin `788b22da`, l'assertion ne lie encore que ces trois constantes entre
elles : le pilote CUDA recalcule toujours 100 par une formule séparée. Le WIP
C6 commence à consommer `kWireOutBytesPerBall`; avant son pin, faire aussi
consommer cette autorité par les compteurs/allocation du pilote, ou poser une
assertion croisée avec `kOutIdsPerBall`, évitera le retour de deux autorités.

## Contre-lecture des paliers P1--P3 en cours

Snapshot courant observé : `HEAD=788b22da` avec un worktree produit non épinglé ; ce
paragraphe aide à fermer le lot, mais ne le reçoit pas. La construction Release
séparée réussit. Après retrait de toute charge concurrente, 206/206 portes
`gate` hors résidence passent en 954,20 s, puis les quatre portes de résidence
passent isolément en 18,01 s : 210/210 au total. Les trois idées sont utiles :
la comparaison de préfixes couvre enfin `smax < 11`, le HWM de processus révèle
des pics invisibles aux instantanés, et la libération anticipée des tranches
conserve l'ordre de fusion. Trois claims doivent toutefois être corrigés avant
le pin ; une suite entièrement verte n'élève pas la force de ses assertions.

### P1 — exiger l'autorité complète, pas seulement une référence plus longue

Le juge vérifie bien `K=1..kmax_eff`, puis demande seulement que les maps
`forest` et `cards` contiennent plus de `kmax_eff` entrées. Une référence
composée de `K1..K5` et `K10` satisfait donc les deux conditions à `smax=6`.
La contre-fixture a été exécutée en retirant `K6..K9` de
`uniform_400.txt` : le binaire rend code 0 et annonce
`5 ordre(s) sur 6 de la reference`.

La fermeture est locale : en mode préfixe du profil courant, exiger pour les
deux maps l'ensemble exact `{1,...,10}`, puis ajouter une fixture clairsemée
`K1..K5 + K10`. La fixture courte à cinq ordres reste pertinente, mais ne tue
pas ce faux vert. Si un profil de référence variable est voulu plus tard, il
faut porter son `full_kmax` signé au lieu de l'inférer de la taille d'une map.
Enfin, le CTest de référence courte n'exige aujourd'hui que le code 2 : une
fixture absente produit le même code et le rendrait vert. Lui poser
`REQUIRED_FILES` et juger le diagnostic exact « référence non prefixable »
sépare le rejet voulu d'une erreur de plomberie.

### P2 — HWM de processus reçu comme télémétrie, pas comme pic d'étage

`VmHWM` est cumulatif depuis la création de l'espace mémoire du processus. Un
relevé après un étage signifie donc « plus haut pic observé jusqu'ici » ; il ne
donne pas le pic propre de cet étage et un ancien record masque tous les pics
ultérieurs plus petits. `ru_maxrss` est également cumulatif et reste préservé
à travers `execve`, alors que le nouveau `/proc/self/status` repart avec le
nouvel espace mémoire. Ce ne sont pas deux sources indépendantes nécessairement
égales.

La porte ne rejette actuellement que `ru_maxrss + 1 < VmHWM`. Une reproduction
contrôlée alloue et touche 300 MiB dans Python, les libère, puis fait `execve`
du test K5 : `VmHWM=25 MiB`, `ru_maxrss=309,8 MiB`, code 0. La comparaison
bilatérale ne doit pas être ajoutée telle quelle : elle échouerait légitimement
dans ce cas. Soit `ru_maxrss` reste un simple majorant historique et le texte
cesse d'annoncer une égalité indépendante, soit un petit superviseur démarre un
enfant dont la baseline est explicitement contrôlée.

La porte ne distingue pas non plus un vrai HWM de
`max_{i<=j}(rss_mb[i])`. Ce contre-mutant satisfait non-vacuité, monotonie,
domination des six instantanés et écart final. À l'inverse, le mutant courant
`hwm-instant-rss` survit sous une politique glibc qui retient les pages : avec
`MALLOC_ARENA_MAX=1`, `MALLOC_TRIM_THRESHOLD_=-1` et
`MALLOC_MMAP_THRESHOLD_=1000000000`, ses six RSS restent monotones. Dans ce
même environnement, un run correct est refusé uniquement parce que
`min_ecart_fin_mb` exige une chute de RSS. Cette exigence teste donc
l'allocateur, pas l'instrumentation.

Fermeture minimale conseillée :

1. publier `process_hwm_cumulative_mib` avec
   `measurement_scope=process_lifetime`, exiger un processus frais pour les
   comparaisons de campagne, et conserver les six valeurs comme une chronologie
   cumulative utile plutôt que parler de « vrai pic à chaque étage » ;
2. remplacer le plancher de chute finale par une micro-fixture dédiée, exécutée
   dans un enfant frais, qui alloue, touche puis libère une zone contrôlée, et
   ajouter le contre-mutant `hwm-sampled-rss-max` ;
3. exiger le masque de jalons attendu (`0x3f` sur la route CPU), car
   `rss == hwm == 0` est aujourd'hui silencieusement ignoré ;
4. publier le HWM déjà acquis aussi sur `resource_exhausted`, qui est précisément
   la frontière recherchée par la campagne ; utiliser de préférence
   `getrusage` ou un champ non allouant dans le chemin de secours ;
5. sérialiser ou poser un `RESOURCE_LOCK` sur les portes d'environ 500 MiB ;
6. exiger des valeurs finies et ne jamais déclarer le mutant tué sur un simple
   statut moteur non complet. Le code courant transforme tout refus/OOM sous
   `--inject=hwm-instant-rss` en code 4, même si le mutant n'a causé aucune
   divergence d'instrumentation ; un tel échec doit rester inconclusif ou
   conserver son code moteur ;
7. exercer `print_run` ou la vraie CLI : la porte actuelle lit directement
   `RunResult` et ne prouve ni la présence, ni l'unicité, ni la cohérence des
   deux nouvelles lignes sérialisées.

`VmHWM` et `ru_maxrss` sont deux interfaces vers un high-water noyau, pas deux
mesures indépendantes. Le contrôle unilatéral courant ne prouve donc pas leur
égalité ; parler de deux lectures et garder `ru_maxrss` comme majorant suffit,
ou bien un enfant à baseline contrôlée doit permettre une borne bilatérale.
Deux autres corrections de contrat sont secondaires mais simples : `statm` doit
employer `sysconf(_SC_PAGESIZE)` plutôt que 4096 en dur, et les unités calculées
sont des MiB. Pour attribuer réellement un pic à un étage, ajouter des compteurs
logiques internes ou un sampler de processus tagué par l'étage ; les six lectures
de frontière ne peuvent pas fournir cette attribution à elles seules.

### P3 — gain plausible sur les pages touchées, aucune borne d'allocation

Dans les trois fusions, `reserve(total)` est appelé alors que toutes les tranches
sources existent encore ; les `swap` n'arrivent qu'après la copie de chaque
tranche. Cela peut réduire le RSS simultanément touché si l'allocateur restitue
effectivement les pages au fil de la copie. Cela ne retire pas la coexistence
virtuelle `sources complètes + destination complète` au moment de la réserve,
donc ne ferme pas directement un mur `RLIMIT_AS`, et la bibliothèque standard
ne garantit pas que `free` abaisse le RSS du noyau. La phrase « une copie plus
une tranche » doit rester une hypothèse d'allocateur à mesurer, pas une borne
d'architecture.

Un A/B exploratoire en processus frais, trois alternances sur
`uniform n=2000, threads=4`, illustre pourquoi il ne faut pas conclure sur un
run : WIP `650536, 660672, 641208 KiB`, pin `659976, 570924, 679972 KiB`.
Médiane légèrement favorable au WIP, moyenne légèrement défavorable ; aucun
gain stable n'est démontré.

Le chemin propre pour une vraie baisse de capacité vivante est déjà accessible :

- census : `resize(survivors.size())`, puis écriture parallèle directe à
  l'indice, avec effacement transactionnel en cas d'erreur ;
- expansion : conserver les comptes par tranche de `count_events_by_k`, former
  leurs offsets et remplir des plages finales disjointes ;
- préfiltre : passe compte/offset/remplissage, ou fusion producteur-consommateur
  explicitement bornée.

Ajouter `VmPeak`/`VmSize` et un compteur déterministe des capacités sources et
destination vivantes rendra la décision indépendante du bruit de RSS. Un mutant
qui retarde la libération des tranches doit alors augmenter ce compteur et être
tué ; les planchers HWM actuels rendraient au contraire une rétention plus forte
plus facile à accepter.

## V1 — ne pas confondre capacité de l'index et sémantique du produit

`CloudIndex` sait ranger plusieurs identités dans un bucket, mais le pipeline
ne sait pas encore produire l'objet correspondant. En particulier,
`point_id(u)` choisit le plus petit identifiant du bucket et la frontière
événement→forêt ne publie que ce représentant. Accepter simplement les buckets
ferait donc disparaître des sommets étiquetés. Les seuils pondérés de l'index,
les listes bornées du census, l'ownership, K=1, les facettes, les digests et la
reprojection doivent être requalifiés ensemble ; le census n'est pas l'unique
précondition concernée.

L'affirmation « presque sûrement » n'est pas acquise. Dans le modèle uniforme
sur les 2^48 positions u16³, dix millions de tirages n'ont qu'environ 16 % de
probabilité de contenir au moins une collision ; vers 480 000 points, la
probabilité est voisine de 0,04 %. Un capteur et sa quantification peuvent être
très non uniformes, mais cela se mesure au lieu de se déduire du nombre de
cases.

Palier utile et peu coûteux : ajouter un probe lecture seule sur les données
cibles après la quantification exacte. Il publie par scan le nombre de sites
uniques, la masse dans les buckets non unitaires, la multiplicité maximale et
la stabilité de la correspondance PointId→site. Ensuite seulement :

- soit le profil définit un quotient déterministe par sites, conserve la masse
  et une table réversible vers tous les PointId, puis précise que la hiérarchie
  calculée est celle des sites et comment elle est reprojetée ;
- soit les points coïncidents restent des sommets distincts, ce qui demande une
  définition mathématique pondérée ou multiensemble, un oracle et des fixtures
  neuves.

Jusqu'à cette décision, `unsupported_degeneracy` est le comportement sûr. Ce
verrou peut être instruit en parallèle des cinq paliers mémoire ; il ne les
bloque pas.

## V2 — trois vocabulaires, pas un enum omnibus

La doctrine v6 active et le code concordent aujourd'hui sur cinq résultats du
moteur : `complete_regular`, `unsupported_degeneracy`,
`resource_exhausted`, `invalid_input` et `invariant_violated`. Ce dernier est
indispensable : une contradiction interne ne doit être classée ni comme donnée
non supportée ni comme ressource manquante.

`numeric_failure` appartient à la doctrine générale et aux oracles qui peuvent
ne pas décider un prédicat. Le chemin produit u16 à arithmétique exacte n'a pas
à l'ajouter sans site réel qui puisse rendre cette issue. De même,
`incomplete_continuation` décrit un artefact durable reprenable, pas le résultat
sémantique d'un appel en mémoire.

La séparation recommandée est :

- `PipelineStatus` pour le résultat terminal de l'objet ;
- un état de tentative de campagne pour `completed`, `refused`, `timeout` ou
  `signal` ;
- un état de checkpoint pour `absent`, `in_progress`, `resumable`, `committed`
  ou `invalid` si le disque est un jour ouvert.

On peut donc différer `replay_current_K` sur les paliers RAM. En revanche, le
manifeste atomique et l'invalidation des sorties provisoires restent utiles
même sous huit heures : panne, préemption et OOM ne sont pas des prédictions de
temps. Les « quinze jours » annoncés n'ont pas de devis reproductible et doivent
être retirés du raisonnement.

## V3 — la limite du digest est future, pas le quatrième verrou actuel

Le format sérialise déjà les cardinalités en u64. Sa largeur u32 pertinente est
`final_canon_fid` ; les PointId des `FacetKey` sont, eux, u32 par contrat
d'entrée. Or le fold courant refuse dès que la somme des incidences dépasse
2^31−1, donc bien avant qu'un nombre de facettes supérieur à 2^32 puisse être
construit. À HEAD, il n'existe ni troncature du digest à 4,3 millions de points
ni deux objets courants rendus égaux par cette troncature : l'objet hypothétique
n'entre pas dans la représentation.

La bonne anticipation reste de versionner avant d'élargir :

1. garder `mhgp4-digest-v1` inchangé et ses rejets exacts ;
2. définir un tag v2 avec références denses u64 et longueurs u64 ;
3. émettre v1 et v2 sur tout le domaine de recouvrement et graver leur
   correspondance ;
4. refuser exactement v1 hors recouvrement, sans cast ni hash de données
   tronquées ;
5. seulement ensuite élargir le fold ou un wire massif.

`PROVENANCE.md` doit donc décrire un domaine par largeurs et gardes, pas une
taille `n` extrapolée depuis une seule famille. Classic et CSR doivent garder le
même objet et refuser sans repli ; cela n'implique pas logiquement la même
capacité de ressource. Si le profil promet une capacité commune, élargir les
deux routes dans le même palier et la tester. Sinon, déclarer leurs plafonds
séparés est compatible avec « aucune route de repli ».

## V4 — ouvrir l'architecture disque, pas une ressource facturable

Le chantier local peut définir le format de run, l'ordre de merge, la
transaction, les budgets et les mutants sur un répertoire temporaire. Il ne
faut pas encore attacher de disque. Le droit permanent aux VM SPOT via les
scripts gardés ne couvre pas une ressource persistante qui peut survivre à
l'arrêt de la VM et continuer à coûter.

Avant toute mutation réelle, il faut une autorisation utilisateur explicite et
un point d'entrée gardé qui fixe et vérifie au minimum : projet, zone, instance
et génération, nom/label de session, type et taille, coût maximal, durée de vie,
création/attachement/montage, trap de fermeture, démontage/détachement/suppression
ciblés et preuve finale de suppression. Le reçu utile doit être rapatrié avant
la suppression. Aucun appel brut ne remplace ce cycle.

Le préflight doit mesurer sur le disque exact le débit soutenu, les fsync et
renommages atomiques, l'espace simultané ancien+temporaire+merge+checkpoint+marge
et l'amplification d'écriture. « Des centaines de Go » reste une estimation
tant que le format et ces cinq volumes ne sont pas comptés.

## V5 — lemme reçu sous prémisses, suppression du détecteur non reçue

Le lemme demandé est court. Soit une facette `tau` marquée attachement dans un
événement `sigma` de niveau exact `a`. Par définition de `active_mask`, la
miniboule de `tau` est encore celle de `sigma`, donc son niveau de naissance
vaut `a`. Toute autre coface contenant `tau` a une miniboule de rayon au moins
égal à celle de `tau`, par monotonie de la boule englobante minimale. L'événement
courant fournit une incidence au niveau `a` : aucune incidence stricte plus tôt
n'est possible, et la première incidence est dans le même macro-lot exact.

Les prémisses sont néanmoins fortes : census complet, rôles exacts du plateau,
flux complet, comparaison de niveaux exacte, macro-lots non scindés. Le compteur
`attach_violations` vérifie actuellement une partie de ces prémisses quand le
fold reçoit un flux arbitraire ou corrompu. Le supprimer parce qu'il vaut zéro
serait circulaire.

Avant l'oubli, graver le lemme dans `MATHEMATIQUES.md`, une porte causale
« attachement tardif », et un recoupement indépendant. Pour une voie externe,
la passe exacte première/dernière incidence peut porter le certificat
`first_batch == attachment_batch`; le reduce ne recycle une clé qu'après ce
contrôle. La preuve d'attachement ne résout d'ailleurs ni le tri global des
`FacetKey`, ni les canoniques finaux, ni le digest : elle ne justifie à elle
seule aucun effacement du catalogue entier.

## V6 — ordre résident confirmé, stabilité externe à rendre explicite

Au pin jugé, la chaîne conserve bien l'ordre annoncé :

1. les candidats sont triés par `BallKey`, puis arité et représentation du
   niveau ; le RLE garde une boule par clé ;
2. préfiltre et census fusionnent leurs tranches dans l'ordre des indices ;
3. l'expansion fusionne les tranches dans ce même ordre et énumère les masques
   de plateau dans un ordre déterministe ;
4. `sort_events_by_level` est stable et ne compare que le niveau exact.

Ainsi, dans le résident, les ex aequo sémantiques restent ordonnés par rang de
boule post-RLE puis rang d'émission intra-boule. C'est un résultat utile.

Il n'est gratuit sur disque que si le merge externe garde cette stabilité. Un
record d'événement ne contient plus sa `BallKey`; une partition en runs peut
donc perdre l'ordre sans désaccord sur sa clé primaire. Porter un ordinal global
ou le couple `ball_rank, emission_rank`, et trier par
`niveau exact, ball_rank, emission_rank`, rend le contrat explicite. La porte
doit opposer résident et runs minuscules sur plusieurs découpages et nombres de
fils, avec plusieurs boules de même niveau et un plateau multi-événements.

## Corrections de portée pour `docs/ECHELLE.md`

Avant de l'appeler « référence », chaque `[M]` doit pointer vers le reçu, le pin,
la commande, le fichier et le champ. Les mesures 400k/800k sont retrouvables
dans le reçu G4 historique ; les valeurs locales K=5 100k/200k ne sont, dans le
dépôt au pin jugé, sourcées que par une réponse de Claude. La sécante 1,088 et
le bracket 2,4–3,9 millions en dépendent aussi. Les archiver ou les reclasser
comme observations non opposables. La conformité de préfixe appariée porte sur
32k, deux répétitions et quatre familles ; les comptes 50k viennent de runs sans
digest et ne doivent pas être présentés comme la même preuve.

Quatre formulations doivent être resserrées :

- remplacer « le temps n'est jamais le verrou » par « sur les runs uniformes
  observés, le temps extrapolé est secondaire au mur RAM actuel » ; la réserve
  à exposant 1,60–1,76 interdit déjà la formule absolue. L'arithmétique « environ
  six » est fausse : appliqués de 50k à 10M, ces exposants multiplient la durée
  par environ 15–35 relativement à 1,088, donc transforment 48 minutes en
  environ 12–28 heures. Même depuis 200k, le facteur relatif vaut 7,4–13,9 ;
- remplacer « multi-CPU et GPU épuisés » par le plafond mesuré des variantes
  C1–C5. C6 vise précisément la résidence hôte et n'a pas encore été mesurée ;
  aucune donnée ne prouve que ces axes ne déplacent pas le mur ou l'exposant ;
- présenter les pourcentages de composition mémoire comme une décomposition
  estimée à fermer par les nouveaux checkpoints. « Rétention 12 % indépendante
  de n [M] » n'est pas établie par les reçus cités ;
- remplacer « tous les refus avant allocation » par « chaque garde précède les
  allocations qu'elle protège ». Le comptage et les structures amont sont déjà
  alloués lorsque les gardes du fold décident, le cap brut est coopératif après
  la matérialisation possible de shards locaux, et la limite du digest est
  latente.

De même, trois points qui augmentent ne démontrent pas une loi « croît avec n »,
et « les libérations ne déplacent pas le mur à 48 fils [C] » est une hypothèse
de mesure, pas un résultat calculé depuis le code. Ces corrections ne changent
pas l'ordre utile des paliers.

## Ordre de livraison conseillé

1. Terminer les cinq paliers RAM avec fixtures et mesures avant/après.
2. Ajouter le probe de doublons sans changer l'objet accepté.
3. Graver et tuer les dents V5/V6 sur le résident puis sur des runs locaux
   minuscules.
4. Décider la sémantique des doublons à partir du probe.
5. N'ouvrir digest v2 et disque que lorsqu'un palier mesuré les rend nécessaires.

Le profil G4 d'échelle est désormais épinglé par `d8d7a7f7`, mais reste soumis
au **NO START** resserré de `ALERTE_G4_ECHELLE_V6_20260902.md` jusqu'à la
fermeture de son chemin mémoire, de ses créations de fils et des deux liaisons
causales restantes. Aucun de ces arbitrages ne promeut `public_status`.
