# Semis après échange : qualification CMake active CPU

Le header actif `6763a877…`, la gate `c4f39462…` et le probe `e96f8d36…`
ont été reconstruits en CMake Release CPU dans un répertoire neuf, avec
`--parallel 1`. Les 24 CTests pertinents de la qualification précédente
passent ; ce n'est pas une exécution de toute la suite CTest du dépôt.
Cadre : exploration v7 hors registre, profil u16, `cpu_reference`,
`public_status=not_claimed`. GCP et CUDA non utilisés.

[Vérifier les preuves](verify.py) : `python3 -B verify.py`, puis
`python3 -B -O verify.py`. Ces lecteurs ne compilent ni ne lancent aucun moteur.
Les [captures exactes](source_map.json) comprennent 21 commandes, leurs flux,
173 sources propres, 1 048 dépendances utilisées épinglées avant/après,
la configuration, les commandes de compilation et les identités du compilateur.
Les anciens Markdown sont conservés comme `.source`, sans liens historiques
présentés comme actifs. Aucun ELF ni vendor n'est distribué.

Les deux gates statiques du produit exercent 34 nuages, 150 ordres et
87 230 images verticales, avec Q=20 et H=T=4. Un probe n200, s8, K1..10,
amont1/statique1, retrouve tous les champs non temporels de la variante O2
qualifiée : Q=17 419, H=T=2 945 et M=48 618. Le hash de l'exécutable Release
est `ade5dbdd7b35fb1f66a8e9f7ea6f33c315458b1ce12897396686ce7782cd9512`.

Deux selftests Python normal/optimisé du format worker passent également :
ils ne lancent aucun worker GPU et ne qualifient pas une exécution cloud.
Les SAN propres de l'optimisation sont des témoins séparés, pas des résultats
de ce build Release. Cette qualification ne prouve ni gain de latence, ni
complétude WSPD universelle, ni garantie sous-quadratique pour tout nuage.
Le contrat 50k/1s ou 100ms et les grandes échelles G4 restent non acquis ici.

Les dépendances externes sont identifiées par leurs hashes avant/après, pas
embarquées. Les outils système sont identifiés, sans prétendre figer toute la
chaîne native ou garantir une reconstruction binaire bit-à-bit sur un autre
hôte. Les temps CTest et du probe n200 ne sont pas promus comme benchmarks.
