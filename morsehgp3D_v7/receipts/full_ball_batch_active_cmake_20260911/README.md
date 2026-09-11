# Qualification active CPU : 40 CTests après raccord du callback

Exploration v7 hors registre, profil u16, `cpu_reference`, `public_status=not_claimed`. La capture active reconstruit douze cibles avec CMake Release (`-O3 -DNDEBUG`, C++20 strict), un seul compilateur, CUDA désactivé. **Les 40 CTests sélectionnés passent ; ce n'est pas une exécution des 449 tests de la suite complète.** GCP non utilisé.

Le header actif est exactement `83f1c78e0656f08cd42522e4cd36d153ce283a6082246a36fe5225b3790c6366`. Les cinq nouveaux fichiers de test sont chacun égaux au candidat privé qualifié plus un unique octet de fin de ligne supplémentaire. La composition CMake est exactement la baseline et les deux fragments qualifiés. Le lecteur contrôle ces différences ; cette nouvelle capture Release reste distincte des preuves privées O2/SAN.

Les 16 nouveaux CTests regroupent les onze tests census→tour et les cinq tests du callback. Les preuves O2/SAN complètes restent dans les paquets séparés `census_tower_permanent_20260911` et `full_ball_batch_permanent_20260911` : le présent paquet ne les duplique pas et ne transfère pas leur SAN au build Release actif. Il importe seulement leurs manifestes historiques épinglés, quatre reçus et les candidats/CMake/origines nécessaires. Une éventuelle révision purement physique de ces paquets conserve ces anciens manifestes et les mêmes sources logiques.

Les résultats actifs des nouveaux tests sont égaux aux résultats privés. Le census exerce 54 tours K1..10, line12/shell14/spatial12, identités appariées, s8/10/12 et CPU0/1/4. Les callbacks CPU1/4 exercent chacun 34 nuages, 150 ordres, 49 lots et 103 terminaux directs. **Erratum de libellé historique : les 65 cas de la suite de refus sont 64 refus et un cas positif sans requête à K=n.** Les champs anciens `cases:65` et `rejections_each:65` ne sont pas réécrits ; ils ne doivent pas être interprétés comme 65 refus effectifs. Le calcul scalaire des attentes BallId est du travail de juge séparé, pas une validation géométrique du cœur ni un gain de performance.

Un probe n=200, s8, toute la tour K1..10, amont1/statique1, est comparé au témoin O2 du semis après échange. Tous les champs de sortie et de travail sont égaux après exclusion explicite de huit temps et de **seulement deux capacités** : `static_worker_capacity_peak_bytes` et `static_sampled_retained_capacity_peak_bytes`. Les nouveaux champs de statistiques modifient la taille de `Worker`, donc ces deux capacités ne sont pas prétendues égales. Les valeurs brutes des deux exécutions et la liste exacte d'exclusions sont conservées. Q=17 419, H=T=2 945 et M=48 618 restent égaux. Aucun temps de CTest ni du probe n200 n'est un benchmark ; aucun contrat 50k/1 s, 100 ms ou multi-millions n'est acquis.

```bash
python3 -B verify.py
python3 -B -O verify.py
python3 -B verify.py --extract /chemin/neuf
```

Le lecteur portable n'exécute ni compilateur, ni moteur, ni service. Il vérifie les 23 commandes, intentions et flux, les sources propres avant/après, les dépendances effectivement utilisées et épinglées, les douze hashes d'ELF, la sélection CTest et les sorties physiques JUnit/LastTest, les cinq différences de fin de ligne, la composition CMake et la comparaison complète n200 avec les exclusions déclarées. Les 178 sources propres et 1 053 dépendances utilisées sont attribuées à cette capture. Les dépendances externes sont seulement épinglées, aucun vendor ni ELF n'est distribué ; les outils système sont identifiés sans garantie de reconstruction binaire bit-à-bit sur un autre hôte. Les deux selftests de format worker Python normal/optimisé ne lancent aucun worker GPU.

`sources/` expose les sources actives réellement compilées, `capture/` les flux et reçus, `source_map.json` le mapping logique dédupliqué. Les anciens Markdown de source sont conservés en `.md.source` pour ne pas les présenter comme documentation actuelle. L'extraction est create-only et conserve les noms logiques ; les chemins absolus des commandes historiques servent à l'attribution, pas à une promesse de reconstruction automatique sur un autre hôte.
