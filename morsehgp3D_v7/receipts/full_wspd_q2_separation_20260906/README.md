# Réutilisation q2 et comparaison s=8/10/12 — 6 septembre 2026

Nuage uniforme n=8 000, seed 3, u16, P=`unlimited`, lazy C=1 000 000,
CPU 6 mono-thread, dix ordres horizontaux. Une mesure par bras seulement.
Le nouveau binaire est `23646a32…` ; l'ancien témoin s=8 est `4938b94b…`.

| Temps de sonde, secondes | Ancien s=8 | Nouveau s=8 | Nouveau s=10 | Nouveau s=12 |
| --- | ---: | ---: | ---: | ---: |
| Avant terminal, sortie provisoire incluse | 133,038 | 131,482 | 132,138 | 137,247 |
| Génération | 59,640 | 58,698 | 60,038 | 62,025 |
| WSPD | 29,567 | 28,938 | 31,358 | 32,890 |
| Rectangles | 30,073 | 29,761 | 28,680 | 29,135 |
| FULL | 50,477 | 50,456 | 49,943 | 52,392 |

Ancien/nouveau s=8 : configuration, dix digests, digest final et tous les champs
non mesurés sont identiques. Les compteurs `wspd_witness_nodes=563616452` et
`wspd_corner_evals=167115088` sont également identiques : **aucune réduction
de visites n'est observée ici**. La réutilisation q2 évite son recomptage, mais
le parcours fusionné q3/q4 demeure. Le total observé recule de 1,17 %, WSPD
de 2,13 %, rectangles de 1,04 % et FULL de 0,04 %. Aucun gain robuste n'en
est déduit sans répétitions ; l'économie sur fixtures ne vaut pas mesure 8k.

À binaire neuf fixé, s=10/s=12 donnent les mêmes dix digests, le même digest
final et les mêmes champs d'ordre non mesurés que s=8, avec 3 113 381 boules
après census. Le travail de génération diffère :

| Compteur | s=8 | s=10 | s=12 |
| --- | ---: | ---: | ---: |
| Candidats bruts | 3 144 017 | 3 129 992 | 3 123 497 |
| Visites témoins WSPD | 563 616 452 | 625 850 731 | 664 703 087 |
| Évaluations de coins WSPD | 167 115 088 | 110 073 830 | 76 064 822 |

Par rapport à s=8, WSPD coûte +8,36 % / +13,66 % ; les rectangles coûtent
−3,63 % / −2,10 %. La génération totale augmente de +2,28 % / +5,67 %, et
le temps avant terminal de +0,50 % / +4,38 %. Les variations de FULL, dont
les compteurs sont identiques, ne sont pas attribuées causalement à s sur
ces seules observations. Les deltas de chaque poste figurent dans `results.json`.

Le micro n=8 est conservé séparément (huit ordres effectifs, pas dix).
`build.json`, le depfile et les 42 sources copiées documentent la compilation
directe constatée par ROOT ; ils ne constituent pas une chaîne d'outils
hermétique ni une capture avant/après de compilation. L'ELF nommé est omis
avec son SHA. Le runner et les douze fichiers bruts des quatre runs sont
conservés à l'identique. Le triplet historique reste un paquet voisin intact.

Recalcul : `python3 read_results.py` ; intégrité : `sha256sum -c SHA256SUMS`.
Le lecteur descriptif ne juge ni la géométrie ni la complétude des catalogues.
`public_status=not_claimed` ; pas de tour inter-K intégrée, de contrat 50k,
de résultat GPU ni de conclusion générale sur le meilleur s.
