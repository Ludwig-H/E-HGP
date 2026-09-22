# Contre-audit B — résidence mémoire du premier raccord v9

22 septembre 2026. Lecture du commit moteur `d2700314` (documents d'audit
ultérieurs exclus). Le premier appel complet sur 08/000000 sans sol à
1 mm est annoncé à 39 885 sites, 1 306 696 boules, ~1,06 Go RSS et
~131 s mur, **sans reçu épinglé** ; ces chiffres ne sont pas un contrat.
La présente note compte les octets du code, sans extrapoler un temps
sur G4 ni imposer un plafond de points ou de sorties.
Ses planchers de capacités/payload sont **logiques sur l'ABI LP64** :
`reserve` peut ne pas toucher immédiatement toutes les pages et le RSS
physique dépend de l'allocateur/OS. Les mesurer séparément.

| Frontière de vie actuelle | Plancher logique, hors allocateur et surcapacités |
| --- | ---: |
| Entrée appelante, nuage/index générateur et index tour | `376n−172` octets |
| Construction du second index avec ses temporaires | `448n−172 + 112P + 8B` octets |
| Recensus des clés distinctes | `376n−172 + 112P + 232B` octets |
| Après configuration du cache temporel par défaut | `388n−172 + 240B + 48·nextpow2(16n)` octets |
| Résultat publié de K1 seul | `216n` octets |

Ces lignes ne sont **pas à additionner** : elles correspondent à des
instants et objets distincts. À 30 M sites, la première ligne vaut déjà
11,28 Go décimaux ; la ligne de sortie K1 vaut 6,48 Go, et la ligne
de cache dépasse 37,41 Go **plus `240B`**, si sa réservation réussit.
Le nombre de clés `B` n'est pas extrapolé depuis une seule trame.
Le scénario 30 M suppose aussi que tous ces sites distincts tiennent
effectivement dans le cube translaté u18 : à 1 mm, chaque axe couvre
au plus 262,143 m. Un agrégat multi-trames plus étendu est hors du
domaine de ce port, quel que soit le budget mémoire.

## Copies simultanées avant et pendant le catalogue

Sur l'ABI LP64 du build lu, `Presentation` occupe **112 octets** et
`BallData` **224 octets** ; noter `P` le nombre total de présentations
q2+q3+q4, `B` le nombre de BallKeys distinctes et `n` le nombre de sites.
Les callbacks accumulent d'abord `P` présentations dans les vecteurs
`slots[W]`. Puis `all.reserve(P)` alloue la capacité d'un second vecteur
de `P` éléments **avant** de libérer les slots, et `all.insert` les
copie. Le seul chevauchement nécessaire représente donc **au moins
`224P` octets** de capacités de présentations, hors surcapacités,
allocations des `W` vecteurs, index et autres objets. Avec `P≥B` et le
`B` exploratoire ci-dessus, ce plancher est **292 699 904 octets**.

Après dédoublonnage, `balls(unique)` alloue encore `224B` octets :
**292 699 904 octets** pour le même `B`, avant ses copies éventuelles.
La liste `groups` ajoute typiquement `8B` octets de positions ; pendant
le census, `all` reste détenu jusqu'après le dernier job. Le plancher
`112P+232B≥449 503 424` octets coexiste alors avec points, deux index,
états travailleurs et sorties, sans compter les capacités supplémentaires.
Avec la base de points et index, le minimum de payload vivant au recensus
est `376n−172+112P+232B`, soit **464 500 012 octets** pour
`n=39 885` et le minimum `P=B=1 306 696` ; cette valeur n'est pas le
RSS observé ni la mémoire totale du moteur.
`keep_catalogue=true` recopie ensuite `balls` en entier dans le résultat ;
le probe de temps n'active pas ce choix, mais une gate d'inventaire oui.

Le census recoupe chaque clé avec un vecteur de coquille par worker et
appelle aujourd'hui `ball_census(..., shell_cap=SIZE_MAX, ...)` avant le
refus `u>12`. Une seule grande coquille peut donc réserver `O(n)` IDs
**par worker qui la rencontre**, uniquement pour annoncer ensuite le
refus de domaine. Tant que le plafond 12 est encore le contrat du port,
un arrêt dès le 13e site réduit cette résidence, à condition de
documenter la priorité entre `unsupported_degeneracy` et divergence
de coquille du générateur. Ce n'est qu'un soin transitoire : le chantier
de quotient exact vise précisément à supprimer ce plafond artificiel.

Deux tableaux sont **déclarés et alimentés sans être relus**
dans `src/chain/tower_chain.cpp` : `geo_of_id[n]` (~4n octets,
lignes 325–326) et `keep[B]` (~B octets, lignes 332 et 387). Les
supprimer ne résout pas le pic principal, mais évite une dette inutile
et clarifie le flux de propriété ; un optimiseur peut déjà élider ces
stockages morts, qui ne sont donc **pas** dans les planchers ci-dessus.
Un transfert *move* des slots vers
un seul `all` n'évite pas à lui seul la capacité finale `P` ; le vrai
objectif massif est une fusion en **runs possédés et bornés**, avec
dédoublonnage exact par BallKey et pression de retour vers les émetteurs.
La fenêtre borne la résidence, **jamais** le nombre de candidats ni la
complétude.
Chaque groupe doit encore valider les profondeurs, coquilles,
incidences et doublons, puis recenser sa clé une seule fois et attribuer
un BallId déterministe. Le FULL actuel demande ensuite un accès
arbitraire par BallKey et un tri global des niveaux exacts : les runs
peuvent borner la RAM de l'amont, **pas** supprimer le catalogue ou ses
index. Un catalogue indexé sur disque/mmap et un tri externe par lots
d'égalité seraient une refonte distincte, avec coûts d'I/O à compter.

## Cache temporel de la tour : coût structurel à grande échelle

Par défaut (`tower_static_threads=0`, `K>1`), `ResolverCache::configure`
réserve la première puissance de deux `S≥16n` entrées directes de
**48 octets** chacune. À `n=39 885`, cela fait `S=1 048 576` et
**50 331 648 octets** (48 Mio). À `n=30 000 000`, `S=536 870 912`
et **25 769 803 776 octets** (24 Gio), avant le catalogue, les index
et les sorties. Si l'allocation échoue, le cache facultatif se désactive
silencieusement : l'exactitude est conservée, mais le profil de coût
change. Le probe publie seulement ses *hits*, pas `resolver_cache_slots`,
`resolver_cache_bytes` ni `resolver_cache_reset_slots` pourtant comptés
dans `FullBallStats`.
La marche de puissance de deux est brutale : de `n=16 777 216` à
`n=16 777 217` sites, la capacité passe de **12 Gio à 24 Gio pour un
seul site supplémentaire**. Une porte d'échelle doit franchir ce seuil,
sans transformer un éventuel échec d'allocation du cache facultatif en
un succès de performance non comparable.

`ResolverCache::reset()` parcourt les **S entrées à chaque ordre K>1**
pour effacer leur `token`. Sur 30 M sites, l'espace adressé est 24 Gio
par parcours et les seules écritures de tokens de 8 octets représentent
**4 Gio par ordre** : 16 Gio à K5, 36 Gio à K10, hors trafic des lignes
de cache et lectures. Ce travail est payé même si peu de facettes
consultent effectivement le cache. La solution n'est pas un plafond de
recherche : mesurer taux de hits et coût du reset, puis comparer une
invalidation paresseuse par génération/époque, une capacité adaptée au
travail et une voie sans cache. Chaque option doit conserver le même
terminal exact et publier les octets réellement résidents.

`resolver_cache.release()` précède la construction de la banque de
populations et l'encodage final des ordres : **ne pas additionner**
automatiquement les 24 Gio du cache et les 6,48 Go de sortie K1 publiée
comme un même pic. Le cache coexiste néanmoins avec les drafts, les
populations originales, les historiques et le catalogue pendant le fold ;
les copies de banque et la sortie créent un autre pic après sa libération.
Il faut mesurer ces phases séparément.

Un plancher plus exigeant existe **au même instant**, après fermeture
de K1 et avant K2, lorsque `Kmax>1` et que le cache est présent : aux
points, index, catalogue et `48S` du cache s'ajoutent `52n` octets de
populations K1, `64n` d'actions/références, `8n` de nœuds inférieurs,
`56n` d'historique, au moins `8n` de compression et `8B` d'ancres.
Le payload/capacité source est alors au moins
`576n−172+248B+48S`, soit **43,05 Go + 248B** à 30 M sites et
`S=536 870 912`, avant tout overhead d'allocation et les autres ordres.
Ce n'est ni un RSS mesuré ni la sortie publiée ; un échec d'allocation
du cache change ce profil de travail.

## Plancher de la sortie K1 et contrat de possession

Même si tous les calculs devenaient instantanés, la représentation FULL
actuelle matérialise déjà à K1, pour chaque site, un `FullNode` (64 o),
un successeur (8 o), un `lower_node` (8 o), une contribution datée
(80 o), une ligne de population singleton (48 o), son ID de coquille
(4 o) et un ID dans le domaine (4 o). C'est **au moins `216n`
octets publiés**, hors autres ordres, capacités et allocateur :
~8,62 Mo à 39 885 sites et **6,48 Go décimaux à 30 M**. Le draft K1
retient temporairement aussi une action et une référence par site ; la
banque de populations recopie des lignes encore détenues par le
constructeur. C'est un coût de format explicite, à conserver ou à
réencoder de manière prouvée ; le cacher dans un temps de kernel GPU ne
le retire pas. Mesurer séparément octets de la tour retenue, copies de
publication et temps d'export.

La résidence en octets sous-estime ici une autre charge certaine : la
branche K1 fait `populations.push_back({{}, {id}})` et
`initial.actions.push_back({{}, {{population, 1, false}}})` **pour chaque
site** (`src/tower/forest/full_ball_tower.hpp:315–324`). Chacune alloue
un petit vecteur interne ; `build_full_coverage_populations` copie ensuite
toutes les lignes par `rows_.assign(rows.begin(), rows.end())`, donc aussi
leurs vecteurs de coquille singleton
(`src/tower/forest/full_coverage_certificate.hpp:67–70`). Cela impose
**au moins trois petites allocations par site au cours de la construction**,
soit au moins 90 millions pour 30 M sites, indépendamment des boules et
des réallocations des grands vecteurs. Le coût temporel exact n'est pas
mesuré ; une représentation en arène/CSR des actions et populations K1
est un préalable crédible au régime massif, avec le même certificat FULL.

`gen::prepare_cloud(points)` fait une copie privée des coordonnées,
mais `run_tower_chain` réutilise ensuite le **span appelant** pour les
clés q2, le recalcul des clés/niveaux et l'index FULL. Tant que le
contrat exige explicitement une entrée immuable pendant tout l'appel,
cela peut fonctionner ; une mutation concurrente de l'appelant créerait
une course et non un snapshot cohérent. La route possédée v9 devrait
réutiliser sa copie certifiée comme autorité géométrique unique, ou
documenter et tester fermement cette précondition. Ce point est
distinct des bornes de complexité.

## Ce qu'un reçu doit exposer

Publier `P`, `B`, `sizeof`/capacités des slots et de `all` au **même
instant**, `groups`, `balls`, copies du catalogue, points/index jumeaux,
résolutions et certificat final (`FullNode` 64 octets et contribution
datée 80 octets sur l'ABI lu). Pour le cache : `S`, octets, resets,
requêtes/hits/évictions, temps de remise à zéro et coût FULL par ordre.
Mesurer `RSS` et pic HWM par phase, pas une somme de maxima indépendants.
Le `ru_maxrss` du probe est un high-water du **processus entier** ; son
lecteur charge tout le fichier avant d'appliquer `--n=préfixe`. Il ne
permet donc pas d'attribuer le pic au générateur, au catalogue ou à FULL,
et un préfixe n'est de toute façon pas le diagnostic spatial LiDAR
contractuel.

Pour le profil multi-dizaines de millions, le coût `224P` de la seule
duplication des présentations et le cache `48·nextpow2(16n)` sont des
postes d'ordre de grandeur à traiter **avant** un port GPU qui garderait
ces tableaux hôte. Le temps reste dominé par q3/q4 sur la première
trame exploratoire ; ce constat de résidence ne remplace pas la
réduction des tests géométriques ni la qualification FULL/G4.
