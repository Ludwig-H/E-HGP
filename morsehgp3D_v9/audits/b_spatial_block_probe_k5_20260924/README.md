# Sonde spatiale par blocs avant S3 — quartier K5

Audit autonome sur le quartier physique sans sol `08/000200`, 1 288 sites
u18/1 mm, K5/s8/W8. La source S2→S3 est la
[`trace K5 gelée`](../b_s2_trace_k5_20260924/README.md), publiée sur
`main` au commit `e447847ae4ed803d7121ed2f4176d2eb23d2a442`.
Ses 27 099 survivants, ordinals et empreintes sont propres à K5 ; aucun
ordinal de la trace K10 n'est réutilisé. Il s'agit d'une sonde audit-only,
sans port produit, GCP, trame entière ni contrat de tour.

## Certificat et entrée

[`probe.cpp`](probe.cpp) construit une fois `PreparedCloud` et le BVH public
du nuage entier, ainsi qu'une permutation inverse ID→rang. Il lit les deux
entrées brutes et le TSV K5, refuse toute divergence avec leurs SHA-256 et
avec celui du `RESULT.json` K5, puis contrôle les 27 099 ordinals contigus,
IDs locaux/bruts, masques et masses du core. Avant l'examen d'une arête, il
reconstruit la même base entière `A,B` que le prouveur core ; aucune forme
par site n'est préchargée.

Le test de maximum aux 8 coins spatiaux × 4 coins de cellule crédite tous
les sites distincts d'un nœud seulement si les 32 formes sont **strictement
négatives**. Un minorant séparé peut exclure un nœud ; un simple échec du
maximum le laisse ambigu. Les nœuds transmis forment une antichaîne de
plages disjointes et les crédits sont hérités séparément par cellule fille.
La disjonction avec le disque est conservative, y compris au contact. La
voie q3 emploie le seuil **4** et q4 le seuil **3**. Un centre témoin dans
le disque ne réfute une preuve uniforme qu'après un compte exact des vrais
sites. Une arête inachevée émettrait masque zéro. La justification
mathématique complète est dans la
[`contrelecture BVH`](../CONTRELECTURE_FORMES_BVH_AVANT_S3_20260924.md).

Le choix de 256 ordinals est déterministe : strates masque S2 × `F_e`
(`≤8`, `9–32`, `33–128`, `>128`) × état de preuve core, dix tirages par
strate disponible puis remplissage par hachage stable. Toutes les 256
arêtes ont été terminées sous les caps globaux de 1 000 000 tests
nœud-cellule, 50 000 cellules et 30 secondes mur. `complete=1` dans
[`pilot_detail.tsv`](pilot_detail.tsv) veut dire parcours achevé, même
avec `proved_mask=0`.

## Résultat K5 borné

| Mesure sur les 256 arêtes | Valeur |
| --- | ---: |
| `ΣF_e` de l'échantillon | 5 634 |
| Fermetures de tous les bits S2, bloc / core / les deux | 133 / 87 / 87 |
| Fermetures bloc supplémentaires au core | 46, masse `F_e=1 214` |
| `F_e` éligible à éviter le core entier | 4 098 |
| Cellules / tests nœud-cellule | 7 588 / 133 190 |
| Coins spatiaux-paramètres évalués | 4 262 080 |
| Minorants de nœud / tests de disque / tests d'appartenance témoin | 128 294 / 9 755 / 8 936 |
| Sites testés à des centres témoins exacts | 2 536 138 |
| Formes évaluées au total, portes directes comprises | 6 842 462 |
| Nœuds/sites crédités et tous énumérés directement | 4 896 / 7 142 |
| Nœuds/sites exclus contrôlés par échantillon déterministe | 1 068 / 3 919 |
| Nœuds exclus / feuilles ambiguës / divisions | 39 400 / 32 216 / 56 678 |

Le [`pilot_shadow.tsv`](pilot_shadow.tsv) est joint par le
[`join_shadow.py K5`](../b_s2_trace_k5_20260924/join_shadow.py) : q3 est
prouvé sur 83 arêtes (`F=2 379`), q4 sur 105 (`F=3 077`), union `F=4 362`
et intersection `F=1 094`. Les classes `bloc∩core`, `bloc∖core`,
`core∖bloc` et aucun, par voie et avec masses `F_e`, figurent dans
[`PILOT_RESULT.json`](PILOT_RESULT.json). Dans cet échantillon, le bloc
prouve 30 voies q3 et 34 voies q4 que le core ne ferme pas ; aucun bit
prouvé par core ne lui échappe. Ces masses sont des éligibilités
conditionnelles, jamais des économies mesurées dans le moteur.

| Strate `F_e` | Arêtes | Fermetures | `F_e` éligible | Nœud-cellule | Formes totales | Mur arêtes (ms) |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| ≤8 | 117 | 30 | 222 | 30 937 | 1 990 752 | 27,515 |
| 9–32 | 74 | 53 | 946 | 50 160 | 2 645 443 | 35,627 |
| 33–128 | 60 | 45 | 2 240 | 50 452 | 2 144 647 | 29,308 |
| >128 | 5 | 5 | 690 | 1 641 | 61 620 | 0,786 |

Le détail **par masque, strate `F_e` et état core** est dans
`PILOT_RESULT.json` (`by_stratum`), avec coins, minorants, points témoins,
contrôles directs et temps séparés. Le regroupement spatial n'atteint que
**1,46 site par nœud crédité** (`7 142/4 896`) dans cet échantillon.
Les 6,84 millions de formes, dont 2,54 millions de tests ponctuels du
centre témoin, dépassent largement `ΣF_e=5 634` : cette version n'établit
aucun amortissement du travail pré-core. Les cinq seules arêtes `F_e>128`
ne suffisent pas à recommander un seuil de sélection.

Le passage local de la sonde, après lecture/validation et construction de
l'index, a pris 105,324 ms mur ; la somme des durées d'arêtes est
93,236 ms mur et 83,304 ms CPU, dont 2,761 ms de portes géométriques
directes incluses dans le mur. Ces chiffres dépendent de l'hôte partagé et
de l'instrumentation. Ils ne se comparent ni au run CPU produit K5 du reçu
de trace ni à la cible **100 ms G4 pour une tour K5 sur trame entière**.
La sélection stratifiée ne représente pas les 27 099 survivants : aucune
fréquence de fermeture, masse `F_e` ou durée n'est extrapolée.

## Rejeu et contrôle

Le code, ses sorties, les empreintes d'entrées et le binaire sont archivés
dans [`PROVENANCE.json`](PROVENANCE.json). Le petit
[`fixture géométrique`](fixtures/geometry_cases.json) et son
[`lecteur entier indépendant`](verify_geometry_fixture.py) vérifient le
contact de coquille, un intérieur strict, l'échec non-excluant d'un test
de maximum et un crédit uniforme. Il passe en Python normal et `-O`.
Le binaire `--selftest` passe ; ses deux modes de mutation
`--mutate-allow-shell` et `--mutate-exclusion-sign` échouent au code 1 sur
les portes géométriques réelles. Chaque nœud crédité de la capture est
contrôlé sur **tous** ses vrais sites aux quatre coins de cellule ; les
nœuds exclus sont contrôlés sur un sous-échantillon déterministe.

La première optimisation à auditer séparément serait le centre témoin
exact parcouru par BVH avec bornes de distance, car ses 2,54 millions
de tests ponctuels dominent ici. Elle exigerait une nouvelle capture et
ne modifierait pas ce reçu.
