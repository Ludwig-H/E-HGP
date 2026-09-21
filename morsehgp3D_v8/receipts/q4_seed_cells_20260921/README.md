# Qualification34 — graines et cellules q4

21 septembre2026, CPU u16, exploration hors registre, `not_claimed`.
Implémentation et [contrat](../../docs/Q4_GRAINES_ET_CELLULES_20260921.md)
distincts de l'audit indépendant A `f45e27c0`. Aucun contrat FULL/GPU/G4.

## Captures et correction du harnais

Les premières captures R1 sont conservées intégralement :

- `qualification/smoke_o0m_lvjj` : quatre portes,24petites sondes Release.
- `qualification_sanitize/smoke_i7ezt4gx` : mêmes quatre portes et24sondes
  Clang ASan/UBSan, environnement par défaut, pas96tests instrumentés.
- `regression/regression_s74hd4yw` :96 CTests Release PASS.
- `disabled_scalar/lidar_qp4ptdxk` : trois sondes64points, trois modes,
  sans filtre de témoins et census q3 scalaire, payloads complets identiques.
- `performance/lidar_gnk4p2va` : six cas8k/K5/s8, trois modes ×W1/4.

Ces succès ne masquent pas les deux échecs du harnais :
[historique](preflight/BUILD_INTEGRATION.md),
[mutant survivant](preflight/mutations/compiled_9pi54doq/COMPLETION.json),
[autotests lecteur en échec normal/−O](preflight/SELFTEST_R1_FAILURE.json).
Les216 sources initiales sont préservées dans `preflight/SOURCES_R1.tar.gz`.
Les deux builds R1 restent épinglés, sans remplacement de leurs binaires.

R2 renforce uniquement la fixture de contact canonique et l'invariant du
lecteur « quatre enfants par branche visitée ». Les moteurs et sondes
restent inchangés. La fixture échange deux IDs de complétions sans changer
l'arête propriétaire : le seul support touche le coin inférieur gauche
de sa cellule propriétaire. Le mutant `minimum>=0` perd ce support.

## Qualification corrigée R2

- `qualification_r2/smoke_ewedfs4y` : quatre portes et24sondes Release.
- `qualification_sanitize_r2/smoke_lqs4dx7b` : quatre portes et24sondes
  instrumentées, mêmes sorties complètes, environnement par défaut.
- `qualification_sanitize_explicit_r2/` : reprise distincte avec
  `ASAN_OPTIONS=detect_leaks=1:halt_on_error=1` et
  `UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`. Cette capture seule
  donne l'activation explicite du contrôle des fuites dans l'environnement.
- `regression_r2/regression_hcudjmag` : suite96CTest dans le build neuf.
- `disabled_scalar_r2/lidar_is050o4p` : les trois voies sans filtre et
  avec census scalaire, sorties complètes comparées.
- `mutations_r2/compiled_lds5cf_8` : dix commandes, trois mutations
  compilées réfutées par l'oracle géométrique avant les registres : contact
  positif perdu, atlas à une feuille jeté, quatrième enfant oublié.

La porte locale R2 compte5943 contrôles,450 requêtes de bornes,
129appels des trois parcours et147candidats q4, avec coquille30,
réutilisation exacte des familles, deux pannes d'allocation tardives,
exceptions, remise à zéro des handles propriétaires et partage concurrent.
La porte globale ajoute dix appels mono et douze parallèles W1/2/4,
avec q3 inactif/actif, rejets d'options et jointure après exception.

Les lecteurs sont clos en normal et−O dans [CANDIDATE_READBACK.json](CANDIDATE_READBACK.json)
et [MUTANTS_R2_READBACK.json](MUTANTS_R2_READBACK.json) ;145corruptions de
reçus sont détectées par les autotests corrigés. Le défaut d'autotest initial
n'est pas un défaut du calcul géométrique, et sa correction n'efface pas
les premières captures. Les hashes avant/après font autorité pour chaque
qualification, jamais le simple nom d'un répertoire.

## Compatibilité et capacité privée

La [comparaison33→34R1](DEFAULT_COMPATIBILITY.md) exécute huit paires,
16commandes, schémas1/2/3, K5/10, backends28/30 et W1/4. Records complets
et travail géométrique restent identiques. Les écarts de capacités liés
à l'attribution des workers restent visibles.

Le tableau `WorkerState`, aligné sur64octets, passe de3584 à3840octets
par worker, soit **+256octets réellement mesurés**. Le registre brut
supplémentaire a296octets, mais absorbe une partie du padding existant.
Ce tableau n'inclut pas tous les objets fixes présents sur les piles de
threads ; ce n'est pas un RSS. Les nouvelles capacités dynamiques par
arête sont comptées séparément et couplées au pic atlas/balayage.

## Mesures de travail

Entrées SemanticKITTI préparées et hachées en lecture seule, scans0/100/200
séparés, sous-échantillons8k/16k/32k imbriqués comme dans33. Elles ne sont
pas un nouveau oracle exhaustif. Les petites portes comparent les objets
complets à un oracle rationnel ; les grandes tailles comparent des digests
canoniques et les registres. s8/10/12 désigne `box_gap_diameter_v1`.

Les premières six observations R1 montrent à8k/K5/s8 :

| Poste q4 | Individual | LiveOnly | Joined |
| --- | ---: | ---: | ---: |
| Familles préparées | 2 312 013 | 777 987 | 414 248 |
| Visites individuelles d'atlas | 24 982 029 | 5 920 711 | 0 |
| Produits graines×cellules | 0 | 0 | 11 697 896 |
| Entrées de cache initialisées | 0 | 0 | 11 899 020 |
| Balayages de feuilles | 553 560 | 553 560 | 553 560 |

LiveOnly/Joined écartent48707 des155605 atlas entiers avant de préparer
un résumé ; ce dernier visite581822 cellules restantes. Tous les coûts de
construction, tests spatiaux, formes, cache, bornes, balayages et tris
restent payés. Les premiers chronos ne déterminent pas un gain stable :
compilations, qualifications et autres campagnes partagent la machine.

Les24 mesures R2 sont closes :

- `performance_r2/lidar_kk34w49h` :9cas, scan0/K5/s8, trois modes et
  trois tailles, W4.
- `performance_r2/lidar_5vvvq0bh` :3cas, scan0/K10/s8, LiveOnly, W4.
- `performance_r2/lidar_a__301v8` :6cas, scan0/K5/s10 et12, LiveOnly, W4.
- `performance_r2/lidar_kluvqih4` :6cas, scans100/200/K5/s8, LiveOnly, W4.

La [synthèse de croissance](ANALYSE_CROISSANCE.md) et
[GROWTH_READBACK.json](GROWTH_READBACK.json) ferment les30observations34
(6R1+24R2), comparées aussi aux30observations33 sans confondre les versions.
Les six références33 à s10/12 sont Window30, pas Local28 : seuls les
digests et l'amont/q3 y sont directement comparés, pas une navigation28
historique inexistante. Les données et périmètres restent explicites.

Sur scan0/K5, les visites d'atlas LiveOnly valent5,921/15,912/38,846M,
soit×2,687 puis×2,441. Mais les bornes de blocs d'atlas conservent×4,112
au dernier doublement ; sur scan200, elles font×4,355 et le censusq3
fait×4,317 au premier. La navigation réduite ne rend donc pas tous les
postes sous-quadratiques. Les ratios des petites branches payées restent
aussi publiés, même lorsque leurs sauts dépassent largement4.

Individual reste le défaut ; Joined reste une option expérimentale.
Le coût q3 et la construction d'atlas n'ont pas été optimisés par34.
La redistribution d'une arête n'est pas implémentée.
GCP non utilisé, aucune VM créée ou démarrée.
