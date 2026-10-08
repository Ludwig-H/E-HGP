# A6b retiré : clôture de la sélection CTest locale

8 octobre 2026. Complément au [raccord source du retrait](../a6b_retrait_r1/README.md).
Le commit de retrait est `8a0716e74` ; ses 163 fichiers déclarés sont identiques à R1 `47feedc96`,
dont 137 fichiers `src/`. Ce contrôle vise ce commit et les journaux conservés, pas l'état de travail
ultérieur où le prototype B3 est déjà en cours.

**La sélection locale est close : 747 tests sélectionnés, 746 passés, une sentinelle LiDAR sautée,
zéro échec.** `logs_main/ra6b.etat` conserve `build_ok` et `ctest 0`. Les 747 lignes du conducteur
sont uniques ; leurs noms concordent avec les 747 blocs de `LastTest.log`, lequel contient 746
verdicts `Test Passed.` et aucun `Test Failed.`. La durée murale publiée est **587,74 secondes**.
Le journal détaillé commence à 17:43 UTC et finit à 17:53 UTC ; son fichier est clos à 17:53:41.

Le cache CMake désigne `main/morsehgp3D_v12`, en **Release, u21, CUDA désactivé**. Ce n'est pas
une exécution GPU ni une nouvelle porte LiDAR : `mhgp12_support_lidar_sentinel` est précisément
le seul test sauté. La sélection comprend les portes de terminaison et de carte de lien restaurées
avec CST-0241 ; leurs noms sont publiés par le lecteur. Elle ne constitue pas une campagne des
55 mutants : aucun nouveau verdict causal sur ceux-ci n'est déduit de cette sélection.

## Limite du raccord binaire

Les six traces primaires ont été copiées puis relues stables hors Git : état du conducteur, configuration,
construction, synthèse CTest, journal détaillé et cache CMake. Les empreintes et tailles sont conservées
dans `capture.json`. Le raccord source du retrait a été observé pendant puis après cette sélection,
et est maintenant vérifiable sur Git `8a0716e74` ; il n'atteste pas continûment les octets du compilateur.

Le même répertoire de construction est ensuite réutilisé pour B3. Lors de la lecture, les exécutables
de région, tour et catalogue portent des dates de liaison **17:57–17:58**, après la fin des tests.
**Ils sont exclus du raccord de cette sélection.** Aucune empreinte de ces ELF courants ne reçoit donc
le résultat des 746 tests précédents. La présente clôture porte sur les journaux, la configuration
déclarée et le raccord des sources ; elle n'est pas une reconstruction indépendante ni une fermeture
avant/après des ELF historiques. Aucun moteur ou test natif n'a été lancé par l'auditeur.

```sh
python -B check.py DEPOT_GIT SNAPSHOT_PRIMAIRE
python -B -O check.py DEPOT_GIT SNAPSHOT_PRIMAIRE
```

Le lecteur ne lance que des lectures Git, vérifie les six traces et compare les deux vues de la
sélection. Les journaux volumineux, chemins locaux de construction et sorties détaillées restent
dans le snapshot externe ; le reçu versionné ne contient que des métadonnées sélectionnées.
