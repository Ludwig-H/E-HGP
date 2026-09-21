# Reçus32 — témoins indexés et census q3 par boîtes

Exploration CPU, quantized_u16_input_only, not_claimed. Aucun contrat FULL,
GPU ou50k n'est qualifié ici. GCP non utilisé pour cette tranche.
Lire le [contrat32](../../docs/Q34_TEMOINS_INDEXES_ET_CENSUS_BOITES_20260921.md).

**Clôture constructeur PASS le 21 septembre 2026**, dans ce périmètre :
29 grandes mesures LiDAR, 36 petites mesures avec records complets,
8 comparaisons ancien/nouveau du défaut, 94 CTests Release et six mutations
compilées réfutées. La qualification Clang ASan/UBSan/LSan porte sur
**trois portes natives et douze petites mesures**, pas sur les94CTests.
[FINAL_READBACK.json](FINAL_READBACK.json) ferme les huit dernières lectures
normal/−O, sources/entrées/artefacts avant-après, et contrôle les preuves
antérieures réutilisées :206sources,205entrées et107artefacts inchangés.
Aucun benchmark natif n’a été réexécuté pour cette clôture.

## Sources et protocole

206 sources explicitement épinglées, manifeste avant chaque campagne,
commandes/entrées/binaires et hashes de fermeture. Les données LiDAR sont
lues dans les préparations indépendantes de l'audit, sans hériter de ses
verdicts : scan0 quantifié2cm, préfixes imbriqués8k/16k/32k.
Les compteurs paient chaque recherche de témoins, même infructueuse,
chaque borne préparée même non visitée, puis tout le calcul aval.
Les chronos locaux ont subi la charge concurrente des qualifications et
des campagnes ; ce ne sont ni des gains stables ni des temps GPU/G4.

Builds désormais épinglés : `build/v8_q34_indexed_20260921` et
`build/v8_q34_indexed_sanitize_20260921`. Aucun build31 n'est écrasé.
Ne pas reconstruire dans ces deux répertoires pour une tranche suivante.

Le premier smoke `qualifications/smoke_5vvacjai` est FAILED : le lecteur
gardait le plancher historique de deux cas parallèles inactifs, alors que
la nouvelle gate en exerce huit. Ce n'est pas une divergence géométrique.
Les deux scripts antérieurs sont conservés exactement dans
`preflight/before_empty_calls_fix/`. Seul le lecteur global a ensuite
été corrigé ; sources natives et binaires inchangés.

Les captures achevées avant cette correction restent closes à leurs propres
hashes, et se relisent historiquement sans `--check-live`. Les nouvelles
captures peuvent être relues avec `--check-live` tant que leurs206sources
et les binaires n'ont pas évolué. Un changement de lecteur n'est pas une
requalification rétroactive de ses anciennes captures.

## Première série complète

`lidar/lidar_2x8pm0nw`, trois commandes, clôture PASS, lecteurs normal/−O
identiques : RectanglePair + GlobalBoxes, Local28, K5/s8, quatre workers.

| n | Temps local (s) | Seeds q3 | Bornes de profondeur q3 préparées | Visites de requêtes q4 |
| --- | ---: | ---: | ---: | ---: |
| 8000 | 4,960 | 1 911 457 | 67 516 973 | 24 982 029 |
| 16000 | 18,139 | 6 033 272 | 213 665 826 | 100 371 602 |
| 32000 | 60,661 | 20 362 738 | 735 508 008 | 562 408 387 |

q3 devient beaucoup moins coûteux que31 : à8k,1,911M seeds contre780,662M,
avec les mêmes104670 candidats q3/q4 et les mêmes digests. Ces chiffres
comparent des travaux discrets ; l'ancien temps local1360,996s était lui
aussi sous charge et ne définit pas un accélérateur stable.
Les rapports de croissance q3 sont inférieurs à4 dans cette série, mais
ceux des visites de requêtes q4 sont×4,018/×5,603. La correction n'est
donc PAS annoncée comme sous-quadratique sur toutes ses étapes.

## Qualifications

`qualifications/regression_ucul7g41` :94 CTests Release PASS, avant le
seul correctif du lecteur décrit plus haut. Les deux nouvelles portes
comparent citrons et census à des oracles indépendants ; la porte globale
compare clés, supports, profondeurs et coquilles complètes, y compris
les modes témoins et census, puis mono/équipeCPU.

`mutations/compiled_ydf5mg_v` et `compiled_vypq6tjd` : six mutations
compilées réfutées géométriquement (alpha q4 faux, contact crédité,
double comptage d'un bloc, mauvais arrondi du minimum entier, contacts
comptés intérieurs, contacts exclus de la coquille). Dix commandes par
capture, aucun crash ni simple erreur comptable accepté comme réfutation.
Leur reprise après correction du lecteur est dans `mutations_final/`.

Les reprises finales [témoins](mutations_final/compiled_foznl6jv/COMPLETION.json)
et [census](mutations_final/compiled_yh3lps69/COMPLETION.json) sont closes PASS,
dix commandes chacune, trois mutations causales réfutées chacune. Leurs
lecteurs normaux/−O avec `--check-live` passent dans FINAL_READBACK.

Les trois smokes finaux sont clos :

| Capture | Qualification effective |
|---|---|
| [smoke_2x8dvljr](qualifications/smoke_2x8dvljr/COMPLETION.json) | Release, census Boxes :3portes+12mesures |
| [smoke_nc5osoja](qualifications/smoke_nc5osoja/COMPLETION.json) | Release, census Scalar :3portes+12mesures |
| [smoke_qbh9yxw5](qualifications/smoke_qbh9yxw5/COMPLETION.json) | Clang ASan/UBSan/LSan, Boxes :3portes+12mesures |

Ces36mesures utilisent n64, K5/10, s8, W1/4, Local28 et les trois modes
témoins. Les records complets concordent entre les modes de census,
témoins, workers et builds. Les trois portes couvrent le filtre citron,
le census q3 et la chaîne globale, avec respectivement31009,475 et14483
contrôles dans le smoke sanitizer. `detect_leaks=1` reste activé.
Les lecteurs normaux/−O et leurs90/86/90corruptions sont reçus dans
[READBACK.json](READBACK.json), expliqués dans [READBACK.md](READBACK.md).
La couverture sanitizer n’est ni une nouvelle suite complète94, ni
un TSan de cette tranche.

L’échec [smoke_yclsnvf8](qualifications/smoke_yclsnvf8/COMPLETION.json)
est conservé : LeakSanitizer ne pouvait fonctionner sous le traçage du
sandbox. Il précède la reprise distincte avec LSan réellement actif.
Le smoke antérieur `smoke_5vvacjai` conserve également son statut FAILED,
pour l’erreur de contrat du lecteur décrite plus haut.

## Ensemble des grandes mesures closes

| Capture | Mesures | Configuration |
|---|---:|---|
| [lidar_2x8pm0nw](lidar/lidar_2x8pm0nw/COMPLETION.json) |3| n8k/16k/32k, K5, s8, W4, Local28, RectanglePair+Boxes ; avant correction du lecteur |
| [lidar_86twby55](lidar/lidar_86twby55/COMPLETION.json) |12| n8k/16k/32k, K5/10, s8, W4, Local28/Window30, RectanglePair+Boxes |
| [lidar_2w21ewdx](separation/lidar_2w21ewdx/COMPLETION.json) |6| n8k/16k/32k, K5, s10/12, W4, Window30, RectanglePair+Boxes |
| [lidar_tgo9mbax](lidar/lidar_tgo9mbax/COMPLETION.json) et [lidar_b7fo1a0i](lidar/lidar_b7fo1a0i/COMPLETION.json) |8| n8k, K5, s8, W1/4, Local28, Pair/RectanglePair × Scalar/Boxes |

Total **29mesures**, toutes sur le scan0 ; les mesures répétées restent
distinctes et ne sont pas présentées comme29configurations uniques.
Les sorties globales et leurs digests concordent pour chaque entrée/K,
au travers des choix comparés. Les grands fichiers ne contiennent pas
tous les records : cette égalité de digests n’est pas un nouvel oracle
exhaustif, contrairement aux comparaisons complètes des petites portes.

Dans la matrice principale12, les temps locaux payés (préparation
nuage/index+pipeline, hors chargement/sérialisation/libération) sont :

| K / voieq4 |8k (s)|16k (s)|32k (s)|
|---|---:|---:|---:|
|5 / Local28|3,849|22,364|45,000|
|5 / Window30|4,244|32,776|40,543|
|10 / Local28|18,373|91,485|153,418|
|10 / Window30|44,663|130,914|240,957|

Sous charge concurrente et sans répétitions exclusives, ces temps ne
qualifient pas un facteur d’accélération stable ni le remplacement du
défaut. [L’analyse de croissance](ANALYSE_CROISSANCE.md) publie aussi les
postes défavorables : plusieurs coûtsq4 dépassent×4 au doublement ; la
sous-quadraticité globale reste ouverte. [Le comparatif ciblé8k](BENCH_8K_PAIR_RECTANGLE_SCALAR_BOXES.md)
montre le gain de sélection par rectangles, mais un résultat temporel
mixte du census Boxes ; moins de tests n’implique pas automatiquement
un meilleur temps. Ses huit lectures sont dans [BENCH_8K_READBACKS.json](BENCH_8K_READBACKS.json).

## Défaut conservé et limites

[DEFAULT_COMPATIBILITY.md](DEFAULT_COMPATIBILITY.md) ferme huit comparaisons
réelles de l’ancien probe31 épinglé et du probe32 **sans nouveaux arguments**,
n64/128, K5/10, s8, W1/4, Local28. Front, géométrie et records complets sont
identiques ; l’état interne paie explicitement+704octets par worker.
Les différences de capacités privées dépendantes de l’ordonnancement
restent publiées. Le premier échec du comparateur, puis les16commandes
de sa reprise sont conservés :20commandes natives au total, sans modification
du produit. Les quatre lecteurs normal/−O historique/live passent dans
[DEFAULT_COMPATIBILITY_READBACK.json](DEFAULT_COMPATIBILITY_READBACK.json).

Cette tranche qualifie un flux de candidatsq3/q4, pas encore la déduplication
en catalogue, les parents FULL ni la tour desKhiérarchies. Elle n’atteint
donc aucun contrat50k/1s ou100ms, ne porte pas sur des dizaines de millions
de sites et ne fournit aucune mesure GPU/G4. Les deux nouvelles options
restent explicites ; le défaut historique demeure `disabled + scalar`.

Le helper [close_final_reads.py](close_final_reads.py) ferme les nouvelles
lectures et vérifie aussi les empreintes de tous les reçus mentionnés,
y compris les historiques en échec. Son préflight de lecture JUnit hors
régression est [documenté](preflight/FINAL_CLOSURE_HELPER.md) ; aucun
benchmark, source206 ou binaire n’a été relancé/modifié pour le corriger.

Lecture d'une campagne (adapter le chemin) :

```sh
python3 -B morsehgp3D_v8/bench/run_q34_indexed_lidar.py read morsehgp3D_v8/receipts/q34_indexed_20260921/lidar/lidar_2x8pm0nw --compact
python3 -B -O morsehgp3D_v8/bench/run_q34_indexed_lidar.py read morsehgp3D_v8/receipts/q34_indexed_20260921/lidar/lidar_2x8pm0nw --compact
```

Le rapport des lectures contient tous les rapports par compteur et leur
liste de dépassements du quadruplement, pas seulement les colonnes favorables.
