# G-APP étape 2 : D1 et D2 rejetées, primaires admises

Relecture locale des primaires de `v12.20261008.gapp2`, source **`9815c19b9a69b00cc927dc61837ef7c5dac63615`**. Le recalcul indépendant des médianes, rapports, bootstrap et critères confirme **D1 rejetée, D2 rejetée, aucun refus de campagne**. La décision préannoncée reste « aucune tranche sur cette base » ; ces résultats ne justifient ni l'amendement conditionnel proposé ni une adoption de G sur l'appareil. Aucun seuil n'est modifié.

Les **18 journaux** sont complets : 15 processus décisifs K5 × six passes (une initiale + cinq chaudes), un processus informatif K10 × trois passes, deux mutants × deux passes, soit **97 passes** dont **75 chaudes décisives**. W48, u21 ; trames ng00 / médiane / maximum, respectivement 39 885 / 64 740 / 99 099 sites déclarés par les tailles des entrées et rendus par les journaux. La cohorte exacte, les codes entiers, phases ordonnées, indices et booléens de passes, métadonnées, couvertures de comparaisons, bilans d'issues et compteurs ont été relus directement. Les déclarations de données sont vérifiées sans ouvrir les payloads.

Les contrôles A/A décisifs sont dans **[0,984643 ; 1,004003]**, à l'intérieur de [0,90 ; 1,10]. Les identités CPU/GPU, issues entre politiques et census non résolus sont conformes ; les processus K5 nominaux n'ont aucun repli. Les rejets proviennent des seuils de temps.

| Trame | Proposition p64 CPU (ms) | p64 GPU (ms) | L4 GPU (ms) | L4+F32 GPU (ms) | IC95 haut L4/CPU p64 | IC95 haut F32/CPU p64 |
|---|---:|---:|---:|---:|---:|---:|
| ng00 | 2,5122 | 1,8933 | 1,3072 | 2,7345 | **0,525392** | **1,095806** |
| médiane | 3,7222 | 2,7287 | 1,8971 | 3,6840 | **0,510707** | **0,993415** |
| maximum | 7,4412 | 5,5053 | 3,7854 | 7,1164 | **0,511489** | **0,959990** |

Les temps affichés sont les médianes des cinq médianes de processus ; les IC portent sur les moyennes géométriques des rapports par processus (10 000 rééchantillonnages, graine 20261008). **D1 exige ≤0,50 sur les propositions**, échoué sur les trois trames. Son ratio synthétique total a pour bornes hautes 0,156850 / 0,155198 / 0,144196, avec seuil 0,15 ; ng00 et la médiane échouent aussi. **D2 exige ≤0,20 sur les propositions et ≤0,10 sur le total** ; ses bornes totales 0,247148 / 0,239835 / 0,218225 échouent également. La neutralité CPU de D1 passe : L4/p64 vaut 0,92109 / 0,92391 / 0,93552 en moyenne géométrique, information sur ce microbanc seulement.

Le « total » suit strictement la formule préannoncée : pour chaque processus, somme des médianes de census, sondes et propositions, divisée par la somme correspondante du témoin CPU, puis agrégation des rapports. **Ces postes sont joués séparément**, sur données déjà résidentes pour l'appareil ; ce total n'est ni une latence intégrée de G, ni FULL, ni une addition d'étages mesurés ensemble. Construction, préparation/récolte et transferts ont d'autres frontières. Aucun gain mural FULL ne découle de ce tableau.

Les deux mutants satisfont leurs critères distincts : côté nul, **code 1** et 232 162 écarts de census CPU partagé/GPU ; omission du test diamétral, **code 0 et issues encore identiques**, mais **429 726 replis / 847 125 parties = 50,73 %** pour L4 et L4+F32, au-delà de 0,1 %. Ce second mutant est tué par le travail de repli, pas par une erreur d'objet ou un crash.

K10 demeure une information : un processus, deux passes chaudes sur ng00. Les issues sont identiques (4 375 853 table / 995 202 census), mais les mécanismes diffèrent : p64 compte 4 375 849 routes T1, L4 4 375 850, F32 4 375 845 et **cinq replis** sur 5 371 055 parties. Ne pas présenter les compteurs physiques comme universellement identiques. Les ratios informatifs propositions L4/p64 CPU et F32/p64 CPU valent respectivement 0,734067 et 1,514304 ; aucun verdict K10 n'est transféré. La portée mathématique de l'identité d'issue est distincte, voir [la lecture du contrat](../gapp2_issues_contrat/README.md).

La fermeture locale est acquise : archive **47 340 octets**, SHA-256 `be04bee319506b4eb525fd7e542b68d8ae14bf9f1cebbc149c808dd1b1d1b2ea`, **92 entrées** de manifeste couvertes, worker et `DONE` à zéro ; commandes auto-test et pilote à code 0 (1,388 s et 68,172 s). Arrêt ciblé certifié à la première tentative, garde intacte, réserve libérée, état final `TERMINATED`, zéro erreur. Ces durées de commandes ne sont pas des latences des primitives. Le paquet `32a1d591…` et le plan `c9c1e2d2…` ferment **367 fichiers identiques aux objets Git** (359 du socle, huit du microbanc). Les six commandes de construction déclarent code 0, CPU `-O3 -DNDEBUG` u21, CUDA `sm_120 -fmad=false` ; les sources du microbanc et sources produit portées concordent avec le paquet.

Limites de preuve conservées : les 36 observations d'isolation de l'appareil sont vides et le pilote déclare les empreintes stables ; les hashes initiaux de trois binaires sont archivés, **pas leurs valeurs finales séparées**. Cela n'ajoute pas une fermeture ELF indépendante avant/après. Les propriétés de GPU proviennent de la sonde. Aucun test natif, build, moteur ou appel distant n'est exécuté par cet audit.

Le pilote livré reste permissif sur des journaux contrefaits, comme l'a vérifié l'audit du lecteur ; ce reçu n'en clôt pas les défauts généraux. Les présents primaires passent les gardes explicites supplémentaires ci-dessus. Le rejeu du juge livré et le calcul indépendant confirment le même verdict : deux seuls rapports affichés diffèrent du worker de trois et une ULP, détaillés dans `results.json` ; les IC, seuils, refus et décisions du rejeu livré sont identiques. La comparaison arithmétique indépendante tolère au plus `2e-15` en relatif pour l'arrondi des floats, jamais un déplacement des seuils.

`check.py` réutilise les primitives de lecture/statistique publiées de [GAPP1](../gapp_admission/README.md), épinglées, puis relit le nouveau format et ses nouvelles règles. Normal et `-O` rendent le même JSON. `capture.json` ne contient que des champs publics choisis et des hashes ; les bruts restent hors Git. Rejeu local :

```sh
python check.py --repo /chemin/depot --session /chemin/session-gapp2
python -O check.py --repo /chemin/depot --session /chemin/session-gapp2
```
