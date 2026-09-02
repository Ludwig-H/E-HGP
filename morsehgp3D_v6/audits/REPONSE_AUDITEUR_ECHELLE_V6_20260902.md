# Réponse aux six verrous du plan d'échelle v6

Date : 2 septembre 2026. Pin documentaire jugé : `4d79dbd3`.

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
base de travail, mais pas encore une référence de mesures : plusieurs phrases
transforment des extrapolations ou des limites futures en faits présents et les
sources exactes des marqueurs `[M]` ne sont pas données.

Décisions compactes :

| Verrou | Décision |
|---|---|
| V1, positions dupliquées | conserver le refus ; mesurer d'abord le cas réel, puis spécifier soit un quotient par sites avec reprojection, soit un vrai HGP pondéré/multiensemble |
| V2, statuts | conserver les cinq statuts du moteur ; porter la continuation dans un état de job/checkpoint distinct si un chemin disque existe |
| V3, digest | conserver `mhgp4-digest-v1` sur son domaine actuel ; introduire un v2 64 bits double-signé avant tout élargissement effectif |
| V4, disque | GO de conception et de selftests factices seulement ; NO GO pour créer ou attacher un disque sans autorisation et scripts gardés dédiés |
| V5, attachement | le lemme est prouvable, mais ses prémisses doivent être certifiées ; ne pas supprimer le détecteur sur la seule télémétrie observée |
| V6, ordre | lecture confirmée pour le résident ; une fusion externe doit conserver explicitement le rang stable global |

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

Le profil G4 d'échelle reste soumis au **NO START** de
`ALERTE_G4_ECHELLE_V6_20260902.md` jusqu'à son pin propre et à la fermeture de
ses raccords. Aucun de ces arbitrages ne promeut `public_status`.
