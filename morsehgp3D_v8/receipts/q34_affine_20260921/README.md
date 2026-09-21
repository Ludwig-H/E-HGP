# Reçus33 — bornes affines et exclusion des blocs de témoins

Exploration CPU, `quantized_u16_input_only`, `not_claimed`, 21 septembre2026.
Lire le [contrat33](../../docs/Q34_BORNES_AFFINES_ET_EXCLUSION_20260921.md).
Il s'agit du flux global de candidats q3/q4, pas du catalogue, de la tour
FULL ni d'un contrat50k/GPU. GCP non utilisé.

## Qualification fonctionnelle

211 sources épinglées avant/après chaque capture, entrées et binaires
hashés, commandes et sorties brutes conservées. Aucun script32 modifié.
Le protocole33 réutilise la capture32 dans une instance privée ; ses
nouveaux verdicts lisent le schéma3 réel et ses deux registres de12champs.
Une projection de comparaison n'est jamais une ancienne exécution.

| Capture candidate | Périmètre | Résultat |
| --- | --- | --- |
| `candidate/regression_gxkxxjxq` | 95 CTests Release | PASS |
| `candidate/smoke_jeqq5w9w` | Quatre portes +24sondes Release | PASS |
| `candidate/smoke_nm_g9o7b` | Quatre portes +24sondes Clang ASan/UBSan/LSan | PASS |
| `candidate/mutations/compiled_1cp9ywhr` | Trois mutations compilées, dix commandes | Trois réfutations géométriques |

Les95CTests n'ont PAS tous été exécutés sous sanitizer. La nouvelle porte
de bornes totalise10193 contrôles sur268cas ; les portes de recherche et
du raccord global sont enrichies. Les oracles comparent contacts, seuils,
masques, coquilles et sorties complètes ; les grandes mesures ne conservent
que les digests, pas une nouvelle preuve exhaustive à32k.

Les mutants remplacent Xi_min par Xi_max dans l'exclusion, diminuent le
facteur d'admission de16à4 dans le nouveau chemin, ou retirent abusivement
le bit global lors d'une exclusion locale. Chacun perd une décision
géométrique, pas seulement un compte de travail.

Les préflights restent distincts : `preflight/smoke_8dhs98j4` Release,
`preflight/smoke_284ctuzk` sanitizer et
`preflight/mutations/compiled_ztqh7g1n` passent. Le premier sanitizer
`preflight/smoke_2j1ieuq1` échoue dans LeakSanitizer sous ptrace ; ce reçu
reste FAILED et conservé, sans défaut géométrique inféré ni promotion.
Les premières commandes de développement sans chemin Boost sont aussi
conservées dans `preflight/gates_release_developer.json`.

[CANDIDATE_READBACK.json](CANDIDATE_READBACK.json) ferme12commandes :
huit lectures des quatre captures en normal/−O, puis quatre autotests
des deux smokes en normal/−O (126corruptions chacun).211sources,
174entrées et87artefacts restent identiques avant/après.

[DEFAULT_COMPATIBILITY.json](DEFAULT_COMPATIBILITY.json) conserve huit
paires de véritables commandes32/33 au schéma2, sans nouvel argument :
n64/128, K5/10, W1/4, RectanglePair/Boxes/Legacy. Sorties complètes,
front et travail logique égaux ; état privé3392→3584octets par worker,
soit+192 réellement payés. Les capacités dépendant de l'attribution aux
workers sont décrites individuellement, pas cachées ni assimilées au RSS.
[La clôture](DEFAULT_COMPATIBILITY_READBACK.json) relit quatre fois
(normal/−O ×historique/live), avec211sources, neuf entrées et six artefacts
inchangés. Le schéma1 est préservé statiquement, pas testé par ces16commandes.

## Première comparaison mono puis quatre workers

`performance/lidar_cuwsvxnv`, six mesures closes. Scan0, K5/s8, Local28,
RectanglePair et census GlobalBoxes, n8000. Même travail géométrique
mono/multi pour chaque mode, mêmes sorties entre modes.

| Bornes | Visites rectangles | Visites paires | Tests Xi totaux | Temps W1 / W4 (s) |
| --- | ---: | ---: | ---: | ---: |
| Legacy | 22 852 374 | 38 642 100 | 24 652 851 | 20,504 /7,599 |
| Exclusion | 17 264 048 | 17 615 856 | 26 249 070 | 18,416 /7,489 |
| Affine | 17 010 781 | 16 745 481 | 25 628 790 | 17,974 /7,740 |

Les visites par paire diminuent de56,7% avec Affine, mais le nombre total
de tests Xi augmente de4,0% : ils sont désormais aussi payés lorsque
Hmin≤0. Les admissions passent de38,899M à7,589M comparaisons de voie,
auxquelles s'ajoutent42,522M comparaisons d'exclusion. Ne pas annoncer que
tous les postes baissent, ni sommer ces unités comme un coût CPU pondéré.

Les [conditions locales](ENVIRONMENT.md) incluent un lourd benchmark
d'audit concurrent. Les temps ne prouvent pas un gain stable : Affine
est ici légèrement plus lent que Legacy avec quatre workers malgré
moins de visites. Les préparations et tout l'aval restent payés.

## Grandes tailles et clôture

Lire la [synthèse de croissance](ANALYSE_CROISSANCE.md) pour les tableaux
détaillés de travail, de séparation et de limites.

**Trente grandes mesures33 closes PASS**, à périmètres distincts :

| Capture | Mesures | Configuration |
| --- | ---: | --- |
| `performance/lidar_cuwsvxnv` |6| Comparaison ci-dessus, trois modes, W1/W4, n8k |
| `performance/lidar_gladtapx` |12| Scan0, n8k/16k/32k, K5/10, s8, W4, Local28/Window30, Affine |
| `performance/lidar_8yueb3v7` |6| Scan0, trois tailles, K5, s10/12, W4, Window30, Affine |
| `performance/lidar_h2s4wo62` |6| Scans100/200, trois tailles, K5, s8, W4, Local28, Affine |

Ce n'est pas le produit cartésien de tous les scans/K/s/backends. Il n'y
a pas de nouvelle mesure Window30 sur100/200, ni de K10/s10/12 ici.
Le cas Affine8k/K5/s8/Local28/W4 est répété dans deux captures : tous
ses chronos restent conservés, un seul travail déterministe intervient
par taille dans la croissance. La première comparaison n'invente pas
des exécutions Legacy33/Exclusion33 à16k/32k qui n'ont pas eu lieu.

[GROWTH_ANALYSIS.json](GROWTH_ANALYSIS.json) et sa
[version −O](GROWTH_ANALYSIS_OPTIMIZED.json) confrontent30mesures33 et
18vraies mesures32, sans convertir leurs schémas en fausses exécutions.
28rapports de doublement sont détaillés ; les sous-compteurs payés et
les zéros→positifs restent distincts des maxima et des masses couvertes.
Les36appariages de digests et23comparaisons complètes du travail aval
(dont21interversions) passent ; les deux seuls pics de réemploi dépendant
de l'ordonnancement sont explicitement neutralisés pour cette comparaison.
Le travail avant filtrage, susceptible de changer, reste publié séparément.
Les grands essais ne sauvegardent pas les records complets : cette égalité
de digests ne remplace pas les oracles des petites portes.

La [clôture normal/−O](GROWTH_READBACK.json) contrôle292fichiers,
dont211sources produit/protocole, les six captures utilisées, entrées,
binaires, analyseur et deux rapports, sans nouvelle exécution native.

| Régime Affine | Visites de témoins par paire :8→16 /16→32 | Bornes census q3 | Visites q4 Local28 |
| --- | ---: | ---: | ---: |
| Scan0/K5/s8 |2,603 /2,773|3,165 /3,442|4,018 /5,603|
| Scan100/K5/s8 |2,692 /2,708|2,455 /1,928|2,712 /3,813|
| Scan200/K5/s8 |3,047 /2,510|4,317 /1,402|6,540 /2,489|

Le filtre progresse nettement, **pas tous les postes importants**. Sur
scan0/K5, les bornes de blocs de construction Local28 font aussi×4,112
au dernier saut. Sur scan200, les constructions q3 font×4,210 au premier
saut ; les raffinements terminaux et décisions d'arrêt de profondeur ont
des sauts encore plus grands, conservés dans le relevé exhaustif. Ce sont
des opérations payées, pas des maxima gratuits. Les grands agrégats ne
les effacent pas. Window30/s8/10/12 garde ses principaux postes sous×4
sur le scan0, mais paie de lourdes préparations ; pas de choix universel
de backend déduit d'un rapport favorable.

La suite vise les [produits graines×cellules q4](../../docs/Q4_BLOCS_SEEDS_PISTE_20260921.md)
et le travail commun de blocs de graines q3, désormais motivé par scan200.
Le partage de témoins parentaux reste une autre piste, non implémentée.

Les builds `build/v8_q34_affine_20260921` et
`build/v8_q34_affine_sanitize_20260921` sont désormais épinglés ; ne pas
les reconstruire pour la tranche suivante. Les builds32 restent épinglés,
sans réexécution pour l'analyse ni transfert de qualification. Aucune
borne générale, tour FULL, qualification GPU/G4/50k ou massif acquise.
