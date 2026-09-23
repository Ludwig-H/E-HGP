# Densité LiDAR à boîte fixe : quart chaud de 08/000200

23 septembre 2026. Contre-épreuve de la [matrice de densité](../CROISSANCE_LIDAR_PLANS_ET_DENSITE_20260923.md) sur le quart sans sol `x≥0,y<0`, grille 1 mm. Même binaire CPU v12 que le reçu initial (`4530644b`, SHA-256 `e1ba126fbcea8ad483ebff2265f446c18e90eaefe04bf20021e76cd0df09af80`), K10/s8/W8, huit fils statiques, mêmes leviers. Les cinq sondes sont `complete_relative` ; les trois cas originaux rejoués reproduisent leurs digests et leurs compteurs.

Aux effectifs 3 630, 7 339 et 14 829, on impose aux deux sous-échantillons les **six IDs d'extrema x/y/z du quart plein**, puis on remplit par le même ordre `splitmix64(ID original XOR seed)`. Cela échange trois IDs à 1/4 et deux à 1/2, garde `1/4⊂1/2⊂plein` et fixe exactement la boîte encodée `x=[80271,159832]`, `y=[0,22220]`, `z=[0,16573]` aux trois densités. Le script [generate.py](generate.py) et [INPUT_MANIFEST.json](INPUT_MANIFEST.json) vérifient effectifs, emboîtement, boîtes, sources et SHA des entrées.

| Sélection | Formes cœur 1/4 / 1/2 / plein | Pente 1/4→1/2 | Pente 1/2→plein | CPU chaîne 1/4 / 1/2 / plein |
| --- | ---: | ---: | ---: | ---: |
| Hash initial | 32 314 012 / 137 636 442 / 579 000 541 | 2,058490 | 2,042542 | 27,469 / 73,786 / 195,157 s |
| Six extrema imposés | 32 312 580 / 137 603 744 / 579 000 541 | 2,058215 | 2,042880 | 27,847 / 73,274 / 195,157 s |

La pente est `log(W_b/W_a)/log(n_b/n_a)` avec les **effectifs réels** ; `W` est ici `dead_core_form_sites`, le nombre de formes du cœur effectivement matérialisées. Les paires développées restent à `p≈1,70–1,74` et les pentes CPU appariées à `p≈1,37–1,40`. Le changement de boîte, notamment celui dû à l'ID 122516, **n'explique pas à lui seul** les deux pentes de formes supérieures à 2 dans ce quart. La distribution intérieure, l'index et les certificats peuvent encore changer ; ce résultat fini sur une trame, un secteur et une graine ne prouve aucune borne asymptotique.

[RESULTS.json](RESULTS.json) contient les pentes et les SHA ; les cinq sorties brutes sont conservées dans ce dossier. [run.py](run.py) est le lanceur exact : il vérifie le SHA du binaire, les entrées, l'identité, les options, les ordres, les digests historiques et les compteurs. Ses chemins `/workspaces` et `/tmp`, comme ceux du générateur, sont ceux de la capture ; les entrées source v8 sont versionnées, et un rejeu ailleurs doit d'abord vérifier leurs SHA. Les murs ont varié sur l'hôte CPU partagé ; ces temps et le statut `complete_relative` ne qualifient ni FULL absolu, ni G4. La sonde v13 n'est pas mélangée à cette série v12.
