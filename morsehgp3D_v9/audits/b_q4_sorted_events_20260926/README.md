# q4 : prototype natif des fenêtres d'événements triés

26 septembre 2026, base `52ff41802`, CPU uniquement, `not_claimed`.
Programme autonome sous `audits/` ; aucun moteur, CMake ou défaut modifié.

Le [prototype](sorted_probe.cpp) réutilise le comparateur exact réduit
`q4_compare` du produit pour trier les racines d'une famille. Il compacte
deux listes : entrées et sorties. Un groupe lit le préfixe d'entrées
strictement antérieures et le suffixe de sorties strictement postérieures,
plus les intérieurs constants. Aucun réservoir borné ne perd des IDs
lorsque la profondeur redescend. Le comparateur évite le produit naïf
P×S de degré neuf qui ne rentre pas dans le domaine i128 du comparateur
réduit de degré cinq.

**Périmètre précis** : famille entière fournie, sans intervalles de lentille,
sans sélection de plus longue arête, sans choix du représentant positif,
sans générateur ni FULL. Le prototype trie avec `std::sort`, puis balaie
séquentiellement les groupes ; c'est un témoin du futur schéma à scans,
pas une implémentation parallèle. Les groupes non positifs sont aussi
interrogés : l'identité d'intérieur y reste algébriquement vraie.

## Vérifications

La capture [results/MANIFEST.json](results/MANIFEST.json) conserve onze
commandes, les compilateurs, sources/dépendances et binaires hachés,
les sorties brutes et deux refus d'arguments. Release GCC et Clang
ASan/UBSan : 46 fixtures, 3 083 groupes et 517 506 tests directs de
puissance, 114 requêtes superficielles à K3..10, 378 IDs, 84 égalités
de racines. Des fixtures dédiées exercent l'intérieur vide à K3,
une vraie coquille constante supplémentaire, les deux orientations,
IDs inversés, tailles31/32/33 et translations près de la limite u18.
Deux contrevoies natives (frontière non stricte, suffixe inversé)
échouent précisément sur la profondeur, pas par crash.

Le test natif de puissance et le comparateur partagent le déterminant
réduit : ils ne constituent **pas deux implémentations arithmétiques
indépendantes**. L'[oracle Fraction distinct](../b_q4_payload_math_20260926/README.md)
vérifie l'identité et les contacts sans importer le moteur ; il ne
qualifie pas pour autant un raccord CUDA absent.

La contrelecture a repéré deux trous du premier préflight : le prétendu
contact supplémentaire dupliquait un support et était dédupliqué ; K3
n'avait aucun groupe accepté. Les fixtures dédiées ci-dessus les
corrigent **avant** cette capture. Aucun défaut du moteur n'en découle.

## 8k / 16k / 32k : travail borné, mais tri pas toujours rentable

Trois répétitions, une famille synthétique dense, K5. Tous les groupes
sont profonds. Préparation géométrique exclue des deux bras. Le bras
diagnostic rejette dès trois intérieurs, mais n'emploie **pas** les
lentilles ni tous les contrôles du T1 produit : ce ne sont pas des temps
comparables à une trame LiDAR.

| Sites | Comparaisons tri/groupes | Tri + scan, médiane | Census à arrêt précoce, médiane |
| ---: | ---: | ---: | ---: |
| 8 000 | 134 799 | 2,033 ms | 0,430 ms |
| 16 000 | 291 639 | 4,813 ms | 0,908 ms |
| 32 000 | 601 341 | 10,307 ms | 1,816 ms |

Les comparaisons font ×2,164 puis ×2,062 ; les temps ×2,367 puis
×2,142. Le schéma a O(m log m) travail pour cette famille, mais le
rejet précoce est **environ cinq fois plus rapide** sur ce cas. Il serait
incorrect de vendre le tri comme un gain mesuré contre un témoin
quadratique qui ne rejette jamais. Aucun export d'IDs à K5 dans ces
benchmarks denses ; les exports sont testés dans les fixtures, pas
chronométrés sur un régime LiDAR représentatif.

Cela concorde avec R24-B : 00/K5 lit 475 299 chunks T1 pour 470 724
passages de classes, soit 1,010 en moyenne. Le passage de lentille,
lui, lit 11 680 121 chunks. Les coûts de ces chunks diffèrent ; ces
compteurs ne donnent pas un partage du temps. Le tri est une option
structurelle pour les listes difficiles, **pas la priorité immédiate
100 ms sans une distribution réelle des coûts par segment**.

## Relire

```sh
python3 -B morsehgp3D_v9/audits/b_q4_sorted_events_20260926/readback.py
python3 -B -O morsehgp3D_v9/audits/b_q4_sorted_events_20260926/readback.py
```

Lecteur LIVE : requiert les sources et les deux binaires temporaires
épinglés ; les deux modes rendent le même JSON. `capture.py` refuse
d'écraser les résultats. Aucune dépense GCP pour cette expérience,
aucune borne de croissance du générateur ni nouveau chrono FULL.
