# Dialogue actif avec le constructeur

11 septembre 2026, après **93f4d110**. La priorité reste la tour entière.
La [coordination](COORDINATION_AUDITEURS.md) répartit les écritures.

## Retirer le second parcours des marques

Le [nouveau prototype structurel](receipts_fused_marks_20260911/README.md)
fusionne les dates de marques avec celles des arêtes. Le premier DSU fournit
leur composante fermée dès que le plateau correspondant est entièrement
traité. Le second DSU, active_segment et le rejeu des parents disparaissent ;
les champs physiques de l’histoire et des marques restent identiques.

O2 et ASan/UBSan/LSan : 30 essais, 184 nœuds, 186 marques, 6 828 comparaisons
BFS, deux mutants et cinq entrées invalides. Les dates à représentants bruts
équivalents et les identifiants natifs non monotones en date sont exercés.
Les fixtures sont structurelles synthétiques ; le raccord aux vrais census
et ses mesures restent à effectuer. Cette porte ne remplace aucune preuve
géométrique ou de complétude.

Quatre détails du raccord doivent rester explicites :

- Fermer tout le plateau avant ses marques, avec les naissances de même
  date déjà émises. Une marque ne crée aucun nœud et n’effectue aucune union.
- Chercher le représentant dans forest.births trié par identifiant ;
  out.births est encore en ordre d’émission pendant le parcours.
- Garder pour la multifusion le niveau brut de la première arête et pour
  la marque sa propre admission, même si son segment est né plus tôt.
- Consommer les marques silencieuses et émettre toutes les naissances
  restantes, y compris après la dernière marque ou arête.

Les tris, copies de normalisation et groupes de plateau restent payés.
Les tableaux retirés représentent logiquement 24L octets par ordre lorsque
size_t et Id occupent huit octets ; aucun gain de RSS ou de temps n’est déduit.
La première comparaison utile porte sur toutes les marques et tous les champs
d’histoire de la voie précédente, avant d’activer leur réemploi à l’export.

## Consommer ensuite les réponses, dans leur ordre physique

Le développeur a retenu les deux deltas de 93f4d110. Une marque liée fournit
déjà la réponse fermée que l’export recalcule pour une contribution. Lier
MarkId au bloc, représentant à φ, admission au rang et segment à la même
histoire immuable. Ces seules identités ne certifient pas une marque forgée :
sa fermeture vient de la reconstruction ou d’une vérification indépendante.

Garder la boucle atlas.program(K) pour l’ordre des contributions. Le tri des
marques par MarkId est différent ; un futur parcours direct doit transporter
l’ordinal source. Les singletons K1 restent séparés. Les marques silencieuses
fournissent aussi tous les u_B des minima groupe/lot de l’
[export historique](receipts_historical_export_20260911/README.md) ; les
premières utilisations et permutations restent nécessaires.

Les [captures scellées](../receipts/parallel_birth_streaming_20260911/README.md)
donnent 2 396 646 / 5 010 402 / 10 348 964 consultations contributives
remplaçables à 8k/16k/32k. À 32k, les 34 205 965 consultations inférieures
et de naturalité gardent leur coupe propre : une image fermée à la naissance
ne vaut pas pour toutes les dates supérieures. Comptabiliser les réemplois
et les HLD réellement exécutées ; les anciens pas agrégés ne donnent aucun
gain de temps. La [dérivation précédente](ENTRETIEN.json) reste conservée.

Conserver les index Chains précédent/courant ramène 2m−1 préparations à m,
soit **19→10** pour K1..10, avec deux index adjacents vivants. Les histoires
restent validées, immuables et à adresses stables. Libérer chaque index après
sa dernière consultation ; ses trois tableaux, les scratch et la sortie
restent des résidences distinctes.

## Raccord par rangs et qualifications déjà prises en compte

Lecture favorable de rank_guard_streaming : gardes strictes, domaines,
premier consommateur et niveau exact passé au resolver sont conservés.
La gate ciblée lue couvre fractions équivalentes, ordre de rang incohérent,
sept mutants ciblés et débordement du compteur commun. K1 du helper
terminal est testé directement ; le scatter produit garde son chemin point.
Les captures constructeur sont qualifiées séparément de cette lecture.

Les [reçus dense/pool](../receipts/birth_streaming_20260911/README.md)
et [parallèle](../receipts/parallel_birth_streaming_20260911/README.md)
restent contre-vérifiés normal/−O, O2/SAN distincts et TSan failed conservé.
Les premiers raccords et triplets sont clos. Préparation commune, semis et
rangs restent détaillés dans le [paquet de préparation](receipts_prepared_catalogue_20260911/README.md).

## Acquis repris par le développeur

| Point clos | Preuve et portée conservées |
| --- | --- |
| Prototype MEB initial | [K7 et repli exact](receipts_meb_boundary_20260911/README.md) conservés ; cas de base réparé par le second auditeur, suivi du patch pris en charge dans abc960ac. |
| Lots et portes permanentes | Publication 324f6192, [40 CTests actifs](../receipts/full_ball_batch_active_cmake_20260911/README.md) ; lecteur contre-vérifié normal/−O. Les 65 cas callback comprennent 64 rejets et un cas positif sans requête. |
| Consommation cumulative hôte | [Deux correctifs contre-vérifiés](receipts_batch_work_20260911/README.md) ; [T2 par lots](../receipts/gpu_terminal_batch_t2_20260911/README.md) et [annotations HD](../receipts/gpu_terminal_batch_hd_20260911/README.md) relus sur leurs preuves publiées, sans exécution device par notre lecture. |
| Semis et vrai census→tour K10 | [Semis intégré](../docs/SEMIS_APRES_ECHANGE_20260911.md), [qualification K10](../docs/QUALIFICATION_TOUR_CENSUS_K10_20260911.md) ; ne plus demander le premier essai ni la porte permanente. |
| Journal incrémental et blocs 50k | [Premier raccord essayé puis non retenu](../receipts/incremental_full_trial_20260911/README.md), [qualification du second auditeur](receipts_cache_commit_20260911/README.md) conservée sur ses propres octets. |

Les [empreintes et lectures](ENTRETIEN.json) distinguent les autorités. Archive industrielle et contrats de temps de toute la tour restent ouverts ; aucun nouveau temps n’est déduit des compteurs locaux. GCP non utilisé par cet audit.
