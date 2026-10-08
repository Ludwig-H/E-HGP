# B sur G4 : admission du lot sur la base 902

8 octobre 2026. **Les 272 journaux sont admis**, avec les six verdicts annoncés :
lot B, report des compteurs, témoins et census combiné adoptés ; garde et proposition seules rejetées.
Le veto A/A passe : moyennes géométriques 1,001262 / 1,000400 / 0,998699, dans ±1,5 %.
Les critères sont ceux déclarés avant campagne ; aucun seuil ni régime n'est révisé ici.

Source du pilote `41d4d828b`, SHA `f374a89b…`, lecteur FULL `3594c5d3…`, manifeste des bras
`3999a137…`. Les huit bras sont **902, son même binaire A/A, puis six substitutions B sur 902**.
Le paquet courant fournit le pilote, pas un bras mesuré. Les octets d'archive, constructions et arrêt
sont documentés dans [la provenance séparée](../session_b_provenance/README.md), avec le
[protocole déjà relu](../session_b_protocole/README.md). A, le nouveau catalogue C et le pool récent
ne font pas partie de ces bras. Aucun résultat n'est transféré au FULL recouvert courant.

## G décisif, CPU

Trois trames, 39 885 / 35 551 / 45 845 sites, K5, W48, dix tours de huit bras ; chaque processus
joue dix passes et exclut la première. Soit 240 processus, 2 400 passes, 2 160 chaudes. La statistique
de G est la médiane des neuf chaudes par processus, puis la médiane des dix processus par bras.
Les rapports appariés sont calculés par tour ; leur moyenne géométrique et leur bootstrap
10 000 tirages, graine 20261008, suivent exactement l'ordre préannoncé.

| Trame | G avant, ms | G après, ms | Rapport géométrique | IC 95 % |
| --- | ---: | ---: | ---: | --- |
| ng00 | 53,0921405 | 48,5758565 | 0,915529 | [0,913772 ; 0,917300] |
| ng01 | 42,0225715 | 38,8853035 | 0,926253 | [0,923489 ; 0,929245] |
| ng02 | 48,2216240 | 45,0066685 | 0,932399 | [0,930977 ; 0,933853] |

La baisse géométrique du lot est donc de 8,45 / 7,37 / 6,76 %. Les bornes supérieures du census combiné
sont 0,926081 / 0,935985 / 0,941413, celles des témoins seuls 0,939229 / 0,948283 / 0,953722.
Le report seul donne 0,997297 / 0,998397 / 0,996663. La garde échoue au critère sur ng01
(borne haute 1,000445) ; la proposition seule sur ng00 et ng01 (1,002720 / 1,000205).
« Rejeté » signifie ici critère non satisfait sur toutes les trames, pas preuve d'un ralentissement général.
La proposition après census a des bornes sous un dans les trois trames, mais cette comparaison est
**informative** dans le protocole ; elle ne change pas le verdict de la proposition seule.

Les empreintes complètes et objets par ordre sont identiques entre tous les bras et processus K5,
ainsi que les compteurs de travail publiés. ng00 rejoint le préfixe gravé `e5a81154fb1b15f1`.
Le lecteur ne prétend pas que les diagnostics de travail observent tout coût physique.

## Informations séparées

Douze processus K10, six uniformes K5 et deux profils instrumentés W1 ajoutent 96 passes G,
dont 76 chaudes. Tous sont lus et leurs identités vérifiées. K10 avant→après :
441,355→413,867 / 321,731→304,579 / 354,426→340,106 ms. Ces deux processus par bras/trame
ne constituent pas le protocole décisif K5. Les uniformes rejoignent les trois empreintes des portes
8k/16k/32k ; le profil W1 rejoint l'objet K5 W48, avec instrumentation et régime propres.

**Les routes ne sont pas universellement invariantes.** À K10, sur ng00 ordre9,
`route_t1` passe de 1 127 044 à 1 127 045 et `route_cert_table` de 2 à 1 ; sur ng01 ordre6,
486 320→486 321 et 1→0. Chaque différence est identique aux deux processus. Aucun autre compteur
publié ne change, les objets et empreintes restent identiques ; ng02 est inchangé. Cela concorde
avec le changement possible de support proposé déjà signalé dans [le raccord B](../t2db_raccord/README.md).
L'énoncé général « les routes ne changent pas » du pilote doit être borné aux prises où c'est observé.

FULL utilise le catalogue GPU, le schéma **séquentiel** de 902, K5/W48, deux processus par bras et trame,
dix passes dont neuf chaudes : 12 processus, 120 passes, 108 chaudes. L'empreinte FUL1 est constante
sur chaque trame, entre bras, passes et processus. Les chiffres suivants sont la médiane des deux
médianes processus ; les maxima des médianes et des passes restent distincts dans `results.json`.

| Trame | FULL avant, ms | FULL après, ms | G dans FULL avant, ms | G dans FULL après, ms |
| --- | ---: | ---: | ---: | ---: |
| ng00 | 161,898877 | 157,180732 | 58,568384 | 53,566814 |
| ng01 | 129,755167 | 126,940253 | 44,1300395 | 40,866044 |
| ng02 | 165,7563955 | 164,278955 | 52,9748235 | 49,355973 |

Ces prises sont informatives, sans IC d'adoption FULL, et dépassent 100 ms. Les médianes d'étages
ne sont pas sommées pour reconstruire le mur. Le pic est celui du `MemoryBudget` partagé
hôte/appareil/épinglé de cette sonde, pas un RSS ni un pic VRAM séparé ; les capacités appareil et
épinglée ainsi que le RSS cumulatif processus sont conservés séparément. Aucune comparaison v11.

## Preuve et rejeu

`check.py` réutilise explicitement `lire_prise`/`lire_full` déjà contre-éprouvés au pin41d4,
extrait depuis Git dans un temporaire. Il ferme en plus cohortes/noms/fichiers, configurations,
sites, types des codes, hashes, identités, résumés et inventaire des 272 JSONL. Les codes individuels
sont ceux archivés par le pilote, cohérents avec les fins réussies des sondes ; aucun code externe
individuel n'est inventé. La provenance distingue les quatre commandes extérieures.
Les durées et résumés se recalculent exactement. Les ratios, IC et verdicts sont reconstruits par
une fonction indépendante, puis comparés au rapport et au juge épinglé. Trois valeurs flottantes
diffèrent d'une ULP du rapport distant ; leurs chemins et valeurs figurent dans `results.json`.
Aucune ne change une décision, et aucune tolérance n'est appliquée aux durées, identités ou cohortes.

```sh
python3 -B check.py --repo DEPOT --returned RETOUR_B
python3 -B -O check.py --repo DEPOT --returned RETOUR_B
```

Les sorties normale et −O sont identiques à `results.json`. Total : 2 616 passes, 2 344 chaudes.
`capture.json` épingle sources, rapport et inventaire SHA des journaux, revérifiés après lecture.
Le rejeu dépend de Git et des journaux extérieurs conservés, pas d'une copie de données dans ce reçu.
Aucun moteur, compilation, lecture de coordonnées, archive de scènes ou action GCP exécuté par l'audit.
