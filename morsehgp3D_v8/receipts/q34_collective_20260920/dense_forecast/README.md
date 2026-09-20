# Prévision auxiliaire du résidu dense — tranche 26

Cette expérience adapte explicitement l'auxiliaire de la tranche 25, sans modifier ses sources ni ses preuves. Elle est **hors de l'inventaire produit de 163 sources** ; le manifeste ajoute ses deux sources propres, soit 165 hashes.

Pour chaque nuage déterministe de 8000/16000/32000 sites, l'arête fixée contient tous les sites dans son cover exact et possède strictement les n−2 triangles aigus connus. Ces propriétés sont vérifiées par des inégalités scalaires indépendantes. Le propriétaire, l'index, le cover et le pool spatial C32/C64 sont réellement préparés et leurs coûts comptés.

La sonde appelle le vrai `assess_q34_family_pool` pour **tous les n−2 triangles**, avec un seul workspace réutilisé. Elle compare Jung/Variance × Universal/Collective, K=5/K=10. Les trente compteurs sont agrégés : sommes, sauf `max_group` et `peak_event_bytes`, qui sont des maxima. Les tris des petits pools sont donc réellement exécutés et comptés.

En revanche, elle n'appelle ni le générateur complet de seeds, ni le census et le tri des familles survivantes, ni les callbacks de candidats. Le champ `future_scan_lower_bound = cover_sites × q4_survivors` décrit seulement le nombre minimal de lectures du fallback actuel **non exécuté**. Il exclut son tri, les coquilles, q3, les sorties et tout assemblage global. Aucun temps de tour, contrat G4 ou résultat GPU n'en découle.

Le runner conserve compilation, commandes, sorties brutes, erreurs et hashes avant/après des 165 sources, de l'archive Release, du cache CMake, du compilateur et de son exécutable temporaire. Son lecteur vérifie 49 commandes, leurs48 résultats et les partitions de compteurs ; `--check-live` vérifie en plus les artefacts encore présents. La lecture historique n'exige pas de conserver `/tmp`. Les essais en échec restent conservés. Le selftest comprend 16 corruptions de reçus, pas 16 tests géométriques.

## Préflight — non qualifié

Les 48 configurations ont été exécutées avant le gel. C64/Variance+Collective laisse 961/1195/5792 familles q4 à K=5 et 1771/3598/11195 à K=10. Cela donne respectivement 7,688/19,120/185,344 millions et 14,168/57,568/358,240 millions de lectures minimales futures à n8k/16k/32k. Le dernier doublement dépasse encore largement×4, avant le tri du fallback : l'amélioration du filtre ne prouve pas une croissance globalement sous-quadratique. Aucun balayage dense complet n'a été lancé.

## Capture close

`forecast_0nz9r_pb/` : **49 commandes / 48 configurations PASS**, avec fermeture inchangée des 163 sources produit, des 2 sources auxiliaires, de l'archive, du cache, du compilateur et du binaire temporaire. Les quatre lectures historique/live en Python normal/−O concordent. Les deux selftests tuent chacun les 16 corruptions de reçus. Les commandes et sorties sont conservées dans `READBACK.json`.

La table rapporte les familles q4 survivantes **après exécution du filtre**, puis les millions de lectures minimales du fallback actuel **non exécuté**. JU = Jung/Universal, JC = Jung/Collective, VU = Variance/Universal, VC = Variance/Collective. La dernière colonne compte uniquement le tri réellement payé dans les petits pools à 32k, pas celui des familles survivantes.

| Pool | K | Mode | Familles q4, 8k / 16k / 32k | Lectures futures minimales (M), 8k / 16k / 32k | Dernier ratio | Comparaisons de tri du filtre à 32k |
|---:|---:|:---:|:---|:---|---:|---:|
| 32 | 5 | JU | 1448 / 2680 / 13142 | 11.584 / 42.880 / 420.544 | ×9.807 | 0 |
| 32 | 5 | JC | 1008 / 2058 / 10555 | 8.064 / 32.928 / 337.760 | ×10.258 | 1367898 |
| 32 | 5 | VU | 1328 / 2493 / 12502 | 10.624 / 39.888 / 400.064 | ×10.030 | 0 |
| 32 | 5 | VC | 995 / 1956 / 10439 | 7.960 / 31.296 / 334.048 | ×10.674 | 1214073 |
| 32 | 10 | JU | 4195 / 9940 / 22815 | 33.560 / 159.040 / 730.080 | ×4.591 | 0 |
| 32 | 10 | JC | 3948 / 8169 / 17442 | 31.584 / 130.704 / 558.144 | ×4.270 | 2576786 |
| 32 | 10 | VU | 4103 / 9622 / 21775 | 32.824 / 153.952 / 696.800 | ×4.526 | 0 |
| 32 | 10 | VC | 3930 / 8141 / 17114 | 31.440 / 130.256 / 547.648 | ×4.204 | 2334793 |
| 64 | 5 | JU | 1141 / 1685 / 8064 | 9.128 / 26.960 / 258.048 | ×9.572 | 0 |
| 64 | 5 | JC | 974 / 1340 / 6068 | 7.792 / 21.440 / 194.176 | ×9.057 | 2166186 |
| 64 | 5 | VU | 1106 / 1500 / 7564 | 8.848 / 24.000 / 242.048 | ×10.085 | 0 |
| 64 | 5 | VC | 961 / 1195 / 5792 | 7.688 / 19.120 / 185.344 | ×9.694 | 1918328 |
| 64 | 10 | JU | 2035 / 4647 / 14862 | 16.280 / 74.352 / 475.584 | ×6.396 | 0 |
| 64 | 10 | JC | 1803 / 3640 / 11604 | 14.424 / 58.240 / 371.328 | ×6.376 | 4191701 |
| 64 | 10 | VU | 1955 / 4349 / 14003 | 15.640 / 69.584 / 448.096 | ×6.440 | 0 |
| 64 | 10 | VC | 1771 / 3598 / 11195 | 14.168 / 57.568 / 358.240 | ×6.223 | 3729870 |

À C64/32k, VC réduit le minimum futur de 28,2 % à K=5 et de 24,7 % à K=10 par rapport à JU. Mais le dernier doublement vaut encore **×9,694 à K=5 et ×6,223 à K=10**, avant les tris et coquilles. Les valeurs n seules ne décrivent pas un modèle aléatoire stationnaire ; ces ratios sont des observations sur cette recette déterministe, pas une borne asymptotique. Ils interdisent néanmoins de présenter le résidu mesuré comme sous-quadratique.

Le filtre VC/C64 a réellement payé 1 918 328 comparaisons de tri à 32k/K=5 et 3 729 870 à 32k/K=10. Les coûts propriétaire, index, cover, pool, certificats et propositions restent également dans chaque JSON. Ses temps sont des mesures auxiliaires sous qualification concurrente ; aucun gain chronométrique global n'est revendiqué. Le minimum futur ne comprend aucun de ces coûts déjà payés et ne représente pas un temps prédit.

Décision : **pas de balayage dense complet** pour cette qualification. Le partage du pool et le filtre collectif sont utiles, mais le produit « familles survivantes × population du cover » reste le verrou à réduire. Aucun plafond, abandon de famille ou quota de recherche n'est ajouté au moteur.

La limite des valeurs n/K/C acceptées par cet exécutable concerne cette recette expérimentale uniquement : aucun quota n'est introduit dans le produit.
