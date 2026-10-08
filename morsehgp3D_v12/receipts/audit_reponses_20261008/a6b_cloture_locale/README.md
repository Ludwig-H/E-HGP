# A6b : livraison et clôture des portes locales v2

8 octobre 2026. **A6b est livré en `f2c106d93f1c835f129cef60e4e5e65ae185bd65`, sans adoption de performance.**
Les quatorze fichiers de [la capture v2](../a6b_portes_v2/README.md) sont identiques au Git livré.
Ce complément conserve les reçus précédents ; aucune compilation, exécution native, GPU/GCP ou lecture de
coordonnées de jeu par l'audit. Seuls sources, empreintes et journaux déjà produits sont relus.

Le conducteur développeur ferme la sélection v2 à **15:52:15 UTC**, code CTest zéro : **753 tests passés,
une sentinelle LiDAR sautée, zéro échec**, sur 754 sélectionnés. Le « 754/754 » annoncé ne doit donc pas devenir
754 portes effectivement jouées pour cette trace. À **16:00:33**, la campagne ciblée des deux nouveaux mutants
se ferme avec code zéro et témoin vert : `fermeture_sans_attente` et `aide_sans_garde` sont tués **par code**,
sans signal, délai ni erreur de construction. Ce n'est pas une nouvelle campagne des 66 mutants du manifeste.

Les substitutions visent chacune une occurrence : rendre fausse la condition d'attente des aides actives ;
remplacer la lecture de `hint_closed` par vrai. Leur causalité attendue est décrite et contre-lue dans le reçu v2 :
premier événement de fermeture incorrect, ou nombre d'indices tardifs positif, avec buffers encore vivants.
Le rapport primaire classe les retours de porte ; les sorties détaillées de chaque assertion mutante ne sont
pas conservées dans ce lot. La portée mémoire reste celle du reçu v2 : aide appelée directement, tranche déjà
consommée, sémaphores supplémentaires ; aucune preuve exhaustive de weak memory ou du planificateur n'en découle.

## Raccord précis à Git

L'inventaire des **413 fichiers** copiés par le conducteur de mutants donne l'empreinte `9eed9388…`, identique
au rapport. Git `f2c106d93` possède les mêmes 413 chemins : **408 contenus identiques**, cinq différences :
`bench/g4_catalogue_flux_lecteur.py`, `g4_catalogue_t1d_judge.py`, `g4_catalogue_t1d_selftest.py`,
`tests/catalogue/tests.cmake` et `docs/PLAN.md`. Tous les fichiers `src/` et les portes de tour sont donc raccordés.
La sélection CTest complète du prototype ne devient pas automatiquement une nouvelle qualification intégrale
du Git, notamment de ses juges T1-d corrigés séparément.

Le cache CMake désigne le prototype w2, Release/u21. Les exécutables `mhgp12_tower_region` et
`mhgp12_tower_levers`, ainsi que la carte de lien, ont été hachés après exécution et relus stables. Cela borne
les artefacts observés ; ce n'est ni une reconstruction indépendante ni une attestation continue des octets
avant/pendant les tests. Les journaux primaires sont épinglés et restent hors Git ; les sources intégrales aussi.

## Correction documentaire proposée

`proposition.patch` s'applique au Git livré, sans changer l'ordonnancement ni le calcul : commentaires du pont
acquire/release, réserve sur G encore en cours et texte de sortie de `levers.priorite`, précision du README.
L'épuisement de `g_next` prouve seulement que toutes les tranches sont **réclamées**. Le message « toutes après G »
est remplacé par cette formulation ; l'historique du commit n'est pas réécrit. Les empreintes des postimages et
l'application en copie temporaire sont vérifiées. Aucun gain ni fermeture du contrat FULL 100 ms n'est inféré.
La réserve et la proposition de réduction du budget N restent dans [a6_admission_n](../a6_admission_n/README.md).

```sh
python -B check.py DEPOT_GIT SNAPSHOT_CLOTURE
python -B -O check.py DEPOT_GIT SNAPSHOT_CLOTURE
```

Lecteur de métadonnées et application textuelle uniquement, sorties identiques à `results.json`.
