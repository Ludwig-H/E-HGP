# S2 : conserver les survivants sur le GPU jusqu'au tri

Prototype isolé, 27 septembre 2026. Aucun changement du moteur, aucun GCP
dans cette tranche. [Qualification locale r1](checks/r1/summary.json) close
PASS : 26 commandes, Release et ASan/UBSan/LSan, compilation CUDA 12.9 réussie,
**aucune exécution GPU**. Les préflights ne constituent pas cette preuve.

Précision de preuve ajoutée après clôture : les deux unités `.cu` utilisaient
`includes_CUDA.rsp`, dont le contenu n'avait pas de pin propre avant/après.
Le build CUDA est observé, mais sa fermeture des options indirectes est
incomplète. Les unités C++ Release/San n'utilisaient pas de fichier réponse
et ne sont pas affectées ; aucune reconstruction ni modification du reçu
n'est faite. Voir l'[addendum ciblé](../b_q34_resident_survivors_review_20260927/RESPONSE_FILES.md).
La prochaine compilation comparative G4 neuve fermera aussi ces options.

## Changement et limite

Le résident publié dans [le reçu G4](../../receipts/q34_resident_g4_20260927/r1/README.md)
téléchargeait les survivants de chaque vague, puis rétablissait leur ordre sur
le CPU. Ce prototype accumule **seulement les S survivants** sur le GPU,
trie leurs clés originales de 64 bits sur le GPU, vérifie l'absence de doublons,
puis télécharge une sortie compacte de 12 octets par survivant. Ni P, ni E,
ni une liste globale des vagues n'est matérialisée.

Seuls accumulation, ordre et transport changent. Les trois noyaux hérités
`filter_kernel`, `scatter_kernel`, `rectangles_kernel` sont textuellement
identiques ; [provenance.py](provenance.py) contrôle leurs hashes, les six
sources d'origine et expose le diff complet. Le coût du filtre rectangles,
les atomiques, le layout des vagues, les prédicats et les appels physiques E
ne sont pas optimisés simultanément. Le prototype ne prétend donc expliquer
ni corriger les 81 ms de filtre rectangles observées antérieurement.

L'objet final exporté est encore une sortie hôte. Il n'existe **pas encore
de propriétaire `DeviceSurvivors` transmis à S3** : « résident » signifie ici
conservé sur le GPU jusqu'au téléchargement final, pas résidence S2→S3.
Aucun gain G4, chrono FULL, contrat 100 ms ou passage à l'échelle nouveau
n'est qualifié par cette brique.

## Architecture et API

- [collector.hpp](collector.hpp) définit `Payload`, `OrderedRun`, les calculs
  de taille contrôlés et un collecteur portable indépendant du GPU.
- [device.hpp](device.hpp), [device_cpu.cpp](device_cpu.cpp),
  [device_cuda.cu](device_cuda.cu), [device_stub.cpp](device_stub.cpp) sont
  un port explicite étroit, dans le namespace `resident_survivors`.
- [host.hpp](host.hpp) conserve l'identité privée session/décision/plan et
  convertit `Payload` vers `Q34FilterBatch` sans nouveau tri CPU.
- [probe.cpp](probe.cpp) lie aussi la véritable ancienne implémentation :
  elle reste disponible dans le même processus pour une comparaison appariée.

La copie étroite est nécessaire car l'ancien propriétaire n'exporte pas de
vue publique permettant de substituer seulement son collecteur. Ni le front,
ni les préparations géométriques natives ne sont recopiés. CMake réutilise
explicitement les règles de liaison du résident gelé et ses deux archives
locales immuables ; les qualifications anciennes ne sont pas transférées.

`consume(plan, Q, debug_keys=false)` rend exactement la même sortie native,
les mêmes masques rectangles et les mêmes compteurs. L'option de diagnostic
télécharge aussi les clés, pour les petites portes seulement. `Payload` porte
`a:u32`, `b:u32`, `mask:u8`, trois octets réservés nuls : taille 12, alignement
4 et offsets vérifiés à la compilation. `Edge` reste l'objet de 24 octets
hérité pendant la collecte. Orientation et masque restent attachés à la clé.

## Algorithme et obligations conservées

1. Chaque vague conserve le même filtrage et compactage. Ses survivants sont
   ajoutés par copie device→device à un seul tableau cumulatif `Edge`.
2. Sa capacité grandit géométriquement selon **S cumulé**, non selon le
   nombre de vagues non vides. Les anciennes et nouvelles allocations
   coexistent seulement le temps de la copie ; le pic le comptabilise.
3. À épuisement de toutes les plages, les métadonnées et buffers de vague
   sont libérés. Une passe sépare clés u64 et payloads, tout en comptant les
   inversions `>=` de l'ordre d'arrivée, y compris entre deux vagues non vides.
4. Le tri radix porte sur les 64 bits non signés. Les payloads suivent leurs
   clés. Une passe exige ensuite des clés **strictement croissantes** : un
   doublon est une erreur de couverture, jamais une déduplication silencieuse.
5. Seuls les payloads triés, et éventuellement les clés de diagnostic, sont
   téléchargés. Les buffers de tri sont libérés avant le retour ; le wrapper
   convertit vers la sortie native puis libère les payloads intermédiaires.

Le compteur physique de toutes les E requêtes, les rejets par voie et les
visites sont établis avant la collecte : ce changement ne modifie aucun de
ces travaux. P reste la masse logique du contrat natif. Le tri ne change que
l'ordre de S, lequel est celui requis avant S3. Les clés sont les ordinaux
originaux hérités du résident, pas les positions regroupées des classes.

Une exception d'allocation ou un dépassement de type ne publie pas de succès
partiel. Le collecteur portable devient fermé après échec ; il n'offre pas
de reprise. Sa porte injecte `bad_alloc` **au point de croissance réel**, avant
l'appel à l'allocateur, et vérifie ce refus. Cela ne teste pas un allocateur
CUDA défaillant. Le backend CUDA ferme son état en cas d'échec et détruit ses
buffers privés ; aucune reprise GPU n'est revendiquée.

## Mémoire : ce qui est compté

Pour une croissance, la capacité est au plus `2*S + Q` ; le pic transitoire
ancien+nouveau reste O(S+Q). Les vagues vides ne réservent rien, les vagues
clairsemées n'accumulent pas Q octets chacune. Les copies de croissance sont
amorties linéairement dans le volume de sortie, et mesurées séparément des
copies d'ajout. Le tri portable coûte O(S log S) ; le tri GPU utilise la
primitive radix explicitement compilée ci-dessous. Cela ne prouve aucune
borne sous-quadratique du nombre E ou S du générateur.

Après libération des buffers de vague et du tableau `Edge`, deux buffers de
clés u64 et deux buffers de payloads12 totalisent **40*S octets**, auxquels
s'ajoutent le scratch réel demandé par CUB et un contrôle de trois u64.
Le split possède temporairement aussi l'ancien `Edge` : ce chevauchement
est compté. Le scratch est mesuré, non supposé nul.

Les champs `size`, `capacity`, `growths`, `growth_copy_bytes`,
`append_copy_bytes`, `edge_peak_bytes`, `sort_scratch_bytes`,
`host_payload_bytes`, `debug_key_bytes` rendent ces coûts observables.
`host_conversion_peak_bytes` compte payloads + clés facultatives + sortie
native + masques rectangles coexistants. La sortie native et ces masques
restent retenus par l'appelant.

**Les périmètres CPU et CUDA de `array_peak_bytes` ne sont pas comparables
directement.** En portable : tableaux du collecteur, payloads/clés et scratch
compact de vague ; pas l'index, l'arène, le runtime ou la pile du tri. En CUDA :
ledger des allocations device du résident (index inclus), métadonnées, vague,
croissances, split, tri et scratch, avec les vrais chevauchements. Aucun des
deux n'est le RSS ni le pic complet hôte+device du pipeline. Les tests appariés
gardent aussi plusieurs sorties de comparaison simultanément ; leur RSS
ne représente pas le pic d'un opérateur isolé.

## CUB et taille globale

La compilation utilise les vrais headers locaux CUDA 12.9.86 / CUB 2.8.2,
pas une hypothèse sur une ancienne signature. `DeviceRadixSort::SortPairs`
accepte ici `NumItemsT`; le port fournit `unsigned long long`. Le choix de
type d'offset est vérifié par `static_assert(sizeof(...) == 8)`.
La [source NVIDIA CUB 2.8.2](https://github.com/NVIDIA/cccl/blob/v2.8.2/cub/cub/device/device_radix_sort.cuh)
documente cette surcharge ; les headers et outils réellement employés sont
épinglés dans la capture locale.

Il n'y a **pas de plafond global INT_MAX sur S**. Les tailles sont contrôlées
en u64 et contre `SIZE_MAX/sizeof(T)` avant allocation. La limite int locale
du scan **d'une vague**, déjà présente dans la référence, reste inchangée :
toutes les vagues sont traitées jusqu'à EOF. Les portes au-delà de INT_MAX
vérifient les formules et types sans allouer des milliards d'éléments ; elles
ne constituent pas un test device d'une telle cardinalité. Les clés hautes
et les doublons sont réellement exercés dans le collecteur **portable**.
Même lancé avec `--cuda`, le harnais conserve cette sous-porte portable :
ses lots géométriques CUDA ont de petits ordinaux. Une porte de tri device
avec clés hautes injectées reste donc explicitement à ajouter.

## Temps : ne pas additionner les champs emboîtés

Le portable conserve un `waves_ms` englobant l'ajout, tandis que le CUDA
publie l'ajout D2D dans `append_ms` séparément de `waves_ms`.
Le portable `sort_ms` de backend englobe la finalisation du collecteur ; le
CUDA sépare split, radix, validation et téléchargement. Le `order_ms` hôte
ajoute ensuite la conversion native. `output_times.release_ms` et le champ
de libération du backend peuvent être emboîtés. Il faut utiliser les bornes
totales publiées, non une somme uniforme de toutes ces sous-durées.

Les libérations internes sont payées avant le retour de `consume`. La
fermeture de session, l'initialisation, la construction d'arène et la sortie
retenue devront être situées explicitement dans le futur chrono opérateur
froid ; aucune soustraction « warm » ne sera utilisée. Le présent harnais
teste l'identité en ordre ABBA, mais ne publie pas de gain chronométrique.

## Qualification locale neuve

[run.py](run.py) crée trois builds neufs Release, Clang ASan/UBSan/LSan et
CUDA compilé. Le binaire CUDA est exécuté **en portable seulement** ici.
Le protocole effectue 26 commandes, dont deux mutants compilés causalement
réfutés, refus CLI et refus explicite d'un faux appel CUDA sur le stub.
La configuration sanitizer contient `-O1` puis le `-O3` du mode Release
CMake : l'optimisation effective est **O3**, avec ASan/UBSan/LSan actifs.

Avant chaque build, le runner rejoue `-M` pour ses cinq unités de traduction
candidate et baseline avec les **flags exacts de `compile_commands.json`**,
y compris l'optimisation et les sanitizers. Il préépingle les headers système
et locaux. Les dix unités des deux mutants et le contrôle d'API C++17 sont
aussi préinventoriés. Après compilation, chaque dépendance `.o.d` doit avoir
été épinglée avant le build et être inchangée. Commandes, sorties, dépendances,
sources, outils, archives et binaires sont liés à la capture et rejugés par
les lecteurs LIVE normaux et `python -O` ; aucun `assert` ne porte le verdict.
Les outils/headers/archives énumérés sont épinglés ; ce n'est pas un système
hermétique (chargeur et toutes les bibliothèques système dynamiques ne sont
pas intégralement archivés).

Portes exécutées : 18 cas du collecteur (clés >2^32 et >2^63, extrême u64,
payloads distincts, vagues vides/clairsemées, doublons, croissance, pannes),
85 lots natifs, quatre capacités Q, K1/2/5/10, s8/10/12, permutations, voies
q3/q4/mixte, replis singleton, plans préparés, E=0 et E>0/S=0. L'ancien backend
et le nouveau sont comparés en ABBA dans le même processus ; la sortie
native CPU complète constitue en outre le juge endpoints+masques+ordre.
Le travail physique résiduel est comparé à la véritable baseline, pas
incorrectement au nombre de visites du juge qui parcourt P.

Les trois binaires donnent les mêmes 85 lots, 1360 consommations appariées,
33 refus, 101 320 requêtes et 39 868 survivantes comptabilisées dans les
appels candidats suivis. Non-vacuité : 376 inversions d'ordre, 72 appels
E=0, quatre appels E>0/S=0, 4134 vagues vides du sous-gate de collecte,
2076 clés hautes et 52 croissances. Ce sont des compteurs de **corpus**,
pas les tailles d'une trame SemanticKITTI. Le maximum de tableaux observé
dans ce corpus portable vaut 57 204 octets ; ce n'est pas un pic de tour.

Fermeture : 1981 pins initiaux, 1132 pins de builds/dépendances ; inventories
pré-compilation Release 367 headers/16 commandes, sanitizer 366/5, CUDA
1080/5. Les cinq dépendances compilées de chaque cible sont incluses dans
ces ensembles avant/après. Les builds globaux immuables sont
`/workspaces/E-HGP/build/v9-resident-survivors-20260927-r1_{release,sanitize,cuda}`.

Le [test du lecteur](reader_checks/r1/summary.json), séparé et ajouté sans
modifier les sources gelées, passe en normal et `-O` : un reçu valide accepté
et onze falsifications refusées à la cause attendue par exécution. Cela
inclut faux succès CUDA, compteur flottant, champ ajouté, profils/binaires
incohérents, sources changées, commande absente, compilation prétendue avant
le pré-épinglage, inventaire de headers forgé et crash présenté comme mutant
causal. Les sorties falsifiées sont repinnées pour atteindre les contrôles
sémantiques ; seuls des duplicatas temporaires sont altérés.

Le mutant `LOW32` détruit l'ordre u64 et est rejeté par
`survivors.duplicate_or_unsorted`. `PAYLOAD` détache l'extrémité de sa clé et
est réfuté par `survivors_gate.key_payload`. Un crash ne serait pas une preuve
causale acceptable pour ces mutants.

Préflight conservé : un premier corpus ne produisait aucune inversion
d'ordre, donc échouait volontairement sur `survivors_gate.nonvacuity` en
portable et dans le binaire lié CUDA. Des cas mixtes réels ont été ajoutés ;
la porte impose maintenant réordonnancement, replis et sortie vide non
vacuels. Aucun changement géométrique n'a été déduit de cet échec de corpus.
Les builds `build/preflight_*` sont exploratoires, exclus du dépôt ; seuls
les reçus clos r1 et les builds globaux neufs font autorité.

## Étape suivante bornée

Après clôture Release/San et compilation CUDA : gate **device réelle**, puis
comparaison appariée sur le même processus G4 avec le résident publié,
Q=Qr=262144, W4 puis W48 et même trame sans sol entière ng00. Conserver
P/E/S, digest, compteurs physiques et sorties strictement identiques.
Mesurer le coût froid complet et le pic, y compris les destructions et la
conversion native ; ne pas transférer le chrono historique à ce port.
Cette campagne et son protocole ne sont pas encore exécutés dans ce lot.

Avant cette campagne, petite porte device à construire dans un **nouveau**
harnais, sans changer ces sources gelées : une unité CUDA peut inclure
explicitement le `device_cuda.cu` gelé et ajouter dans la même unité un point
d'entrée d'audit appelant ses vrais `append_survivors` et `finish_survivors`.
Elle remplace alors, pour cette cible seulement, la compilation séparée de
ce fichier ; aucun second exemplaire du backend n'est lié. Cette couture
réutilise exactement les fonctions, sans reproduire leur algorithme ni
modifier les prédicats. Son inclusion, sa compilation et ses dépendances
devront avoir des pins neufs ; ce n'est pas une API produit exportée.

Le corpus device devra injecter 0, 2^32−1, 2^32, 2^63−1, 2^63, 2^64−2,
avec payloads et masques distincts, dans plusieurs vagues désordonnées.
Vérifier champ par champ clés u64 téléchargées, payloads, inversion aux
frontières, Q=1/7/64, vagues majoritairement vides et cas S=0/S=1. Un doublon
inter-vagues doit être refusé après le vrai radix CUDA, puis tous les buffers
doivent être libérés sans sortie réussie. Vérifier capacités, bytes D2D/D2H,
pic ancien+nouveau et ledger revenu à son niveau initial. Une allocation de
plus de INT_MAX éléments n'est pas nécessaire pour tester les **bits** hauts
des clés et ne doit pas être feinte dans la qualification.
