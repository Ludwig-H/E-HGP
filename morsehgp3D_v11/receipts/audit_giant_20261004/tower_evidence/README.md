# Revue tower, export, bancs et couverture globale

4 octobre 2026. Cadre :
`exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`.
Sources exactes0f5 ; complément publié0af sans changement de moteur.
49 blobs copiés dans `source/`, trois documents récents dans `latest_source/`.
`SOURCES.json`, `LATEST_SOURCE.json` et `INVENTORY.json` fixent les octets
et le périmètre. [Couverture et limites](SCOPE.md).

## Défaut nouveau : réveil du pipeline sur abandon

`forest_vertical.cpp:202–208` vérifie `low.abandoned` dans le corps de
`while(!follow_lower_ready(...))`, puis attend et relit. Or
`ForestProgress::finish(count,false)` publie `abandoned=true` puis
`closed=kNone`. Après ce réveil, `level<closed` devient vrai : le contrôle
d'abandon situé dans le corps est sauté, et le lecteur atteint
`sweep.advance`, `birth_image` et `visit` sur l'ordre refusé.

Le modèle [pipeline_abort_check.py](pipeline_abort_check.py) extrait le
prédicat du blob actuel, protège l'ordre des instructions relues et vérifie
neuf niveaux, les réveils normaux et la correction proposée : **72 gardes**
identiques normal/−O. Une garde `if (low.abandoned) return {};` après la
boucle d'attente, avant `advance`, supprime ce chemin tout en conservant
les décisions de publication normale dans ce modèle.

La conséquence certaine est la **rupture du protocole de lecture sur refus**.
La graine régulière n'est plus garantie publiée lorsque `find` lit son
tableau non atomique. Une résolution d'un bloc ultérieur peut encore être
active : les résolveurs et le Pool continuent après une erreur. Ce dernier
point motive une porte native d'abandon contrôlé et TSan. Le test présent
ne fabrique pas un Cloud déclenchant une course et ne revendique ni race
native reproduite ni FULL erroné rendu avec succès. Le résultat final est
encore refusé par la distribution du Pool.

## Banc et preuves

`NATIVE_PARITY.json` compare les blobs des **101 src** à b872 : tous
identiques. Seul `tests/tower/tests.cmake` change dans src/tests. Les portes
G4666, TSan7 et mutants11 b872 sont des résultats antérieurs recoupés,
pas de nouveaux tests exécutés. Les qualifications c40 plus larges gardent
leur domaine ; voir les autres capsules géantes.

Les mocks des vrais contrôleurs Python passent normal/−O :
`test_g4_matrix.py` **37 contrôles** (signal tardif, budget sanitizer,
provenance, sonde absente) et `g4_prepare_host_test.py` **87 contrôles**
(identité, garde et outillage). Aucun compilateur, natif ou cloud appelé.
`RUNS.json` et les stdout/stderr conservent toutes les commandes.

Le [récent lot de bouts](LATEST_G4_REVIEW.json) a été contrôlé séparément :
360 sorties `ok`, 10 séquences représentées, 107..17593sites,
789 observations d'objets corrélées, quatre ordres2/3/5/10.
Les **427 payloads** de l'archive conservée sont rehachés ; session au
commit f1a, code0, arrêt ciblé certifié. Les comptes publiés dans Zoltan
sont recalculés à partir des résultats et du manifeste des bouts.
Ceci reste une mesure de **meilleurs blocs**, avec découpage des données
par annotations. Aucun nouveau contrat de trame entière, 100ms, tête
plate ou représentativité statistique n'en découle. Aucune coordonnée,
étiquette brute ou archive de données KITTI n'est copiée dans cette capsule.

Cette capsule conserve seulement les sources, métadonnées et petits
contrôles nécessaires à ces constats. Aucun reçu clos antérieur, produit,
chantier privé ou fichier d'un autre acteur n'a été réécrit. `LEDGER.json`
et `SHA256SUMS` ferment exhaustivement les payloads de ce dossier.
