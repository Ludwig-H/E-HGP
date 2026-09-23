# Palettes de gardes par index spatial : exactitude, coût, déclenchement

23 septembre 2026. Shadow CPU **hors produit**, sur la trame brute complète
SemanticKITTI 08/000000 à 1 mm/u18, K5/s8. Les 120 arêtes sont les deux
échantillons stratifiés déjà épinglés dans
[`paired_guards_precore_20260923`](../paired_guards_precore_20260923/README.md),
20 par tranche de taille de cœur `F` et par graine. Elles ont donc été
**choisies après connaissance de `F`**. Ce reçu teste un sélecteur exact sur
l'index spatial immuable v9, puis un déclencheur calculable avant le cœur.
Il ne mesure ni exécution FULL, ni GPU, ni le temps gagné par le produit.

## Sélecteur et preuve d'exactitude

Pour l'arête `ab`, `s=a+b`, `d=b−a`, `D=|d|²`, un site `z` appartient au
cœur diamétral si `|2z−s|²≤D`. Deux vecteurs entiers indépendants `A,B`
perpendiculaires à `d` définissent quatre secteurs par les signes de
`A·(2z−s)` et `B·(2z−s)` ; zéro appartient au côté non négatif. `A,B`
ne sont pas nécessairement orthogonaux. La palette est constituée des
`B=4/8/16` plus proches du milieu **par secteur**, départagés par ID brut.

Pour chaque boîte fermée de l'index, les intervalles entiers de `2z−s`
donnent un minorant de `|2z−s|²` et les extrêmes exacts des deux formes
linéaires. Une boîte n'est écartée que si le cœur ou le secteur est
impossible. La file visite les boîtes par minorant croissant. Une fois
`B` sites trouvés, elle ne clôt la recherche que si le plus petit
minorant restant est **strictement supérieur** à la distance du pire
site retenu : les égalités restent parcourues pour respecter le départage
par ID brut, distinct du rang spatial. Les deux extrémités sont exclues.
Un parcours supplémentaire entretient les quatre palettes `B16` ensemble ;
leurs préfixes sont exactement les palettes `B4/B8`. L'appariement utilise
ensuite les inégalités entières strictes et les paires disjointes du reçu
précédent. Le budget de visites ne sert qu'à émettre des **preuves
positives** ; une recherche interrompue n'affirme jamais le top B exact
ni l'absence d'une preuve.

La [fixture](SELFTEST.stdout) de 47 sites vérifie les quatre frontières de
signes, les égalités de distance et le départage par ID brut. Sur les 120
arêtes LiDAR, les quatre palettes, à chacun des trois B, concordent **site
par site** avec un oracle qui balaie les 123 389 sites de la trame. Les
`F` recalculés concordent avec les traces S2 ; les nombres de paires et
les fermetures concordent avec les deux reçus antérieurs. Un passage
Clang ASan/UBSan de la fixture et des 120 arêtes est également passé.

| Palette | Fermées / 120 | `F` de ces arêtes / 520 631 | Nœuds dépilés | Boîtes évaluées | Sites testés |
| --- | ---: | ---: | ---: | ---: | ---: |
| 4 par secteur | 52 | 206 339 | 33 951 | 64 684 | 1 849 |
| 8 par secteur | 56 | 232 302 | 39 377 | 72 046 | 3 594 |
| 16 par secteur | **67** | **280 728** | 48 553 | 83 768 | 6 909 |
| Quatre secteurs en un parcours, 16 | **67** | **280 728** | 44 479 | 54 594 | 6 909 |

L'oracle a examiné **14 806 680 sites** (120 balayages complets). Le
parcours partagé conserve exactement la même palette en réduisant les
évaluations de boîtes par rapport aux quatre requêtes ; son coût de gestion
de quatre listes peut annuler ce gain en temps. À `B16`, les **53 arêtes
non fermées** ont tout de même payé 21 517 nœuds, 37 326 boîtes, 2 960
sites testés et tout l'appariement. La capture hôte unique
[`SUMMARY.json`](SUMMARY.json) mesure séparément préparation du nuage,
construction de l'index, sélection et matching ; les durées varient avec
la charge CPU partagée et ne sont pas des chronos de chaîne. L'index
occupe 19 244 996 octets en plus des 7 403 340 octets du nuage préparé ;
il est construit une fois, pas par arête. Un matching glouton qui trie
toutes les paires de jusqu'à 64 candidats reste un coût aval mesuré, pas
une solution industrielle arrêtée.

| Plafond par secteur | `B16` fermées / 120 | `F` correspondant | Nœuds | Boîtes | Secteurs interrompus / 480 |
| --- | ---: | ---: | ---: | ---: | ---: |
| 64 | 21 | 64 429 | 30 334 | 58 444 | 450 |
| 256 | 67 | 280 728 | 48 553 | 83 768 | 0 |

À 64, les 21 fermetures sont des preuves positives et appartiennent
également aux 67 fermetures du reçu exact précédent ; les 99 autres
arêtes reviennent au chemin existant. À 256, aucune recherche n'est
interrompue dans ce panel, donc le résultat est réellement top B exact.
Ces budgets sont des **nœuds dépilés par secteur**, et le coût total inclut
quatre secteurs. Une petite palette bornée ne remplace pas le prouveur
par groupes de nœuds : celui-ci peut créditer plusieurs paires sans
énumérer les feuilles, ce que notre compteur de sites ne reflète pas.

## Verrou de déclenchement avant `make_diametral`

Dans la v9 auditée, le filtre ponctuel S2 précède `make_diametral`.
`D=|ab|²` se calcule donc avec les seules extrémités avant le cœur. Le
diagnostic [`TRIGGER_D.json`](TRIGGER_D.json) passe les huit parties de
trace S2 pleine aux SHA épinglés et compte les seuils `D`, sans utiliser
`F` pour prendre la décision. `F`, connu **après** construction du cœur,
ne sert ici qu'à décrire les populations.

| Déclencheur pré-cœur | Sur 3 986 433 arêtes survivant à S2 | Fermables `B16` conservées dans le panel |
| --- | ---: | ---: |
| `D≥2²²` | 909 278 (22,8 %) | 67 / 67 |
| `D≥2²³` | 572 621 (14,4 %) | 63 / 67 |
| `D≥2²⁴` | 327 900 (8,2 %) | 57 / 67 |

**La longueur seule ne constitue pas un déclencheur assez sélectif** :
pour conserver tous les exemples, elle lancerait près d'un million de
tentatives sur cette seule trame. Même le seuil `2²⁴` en lancerait
327 900 et manquerait dix arêtes fermables du panel. Le flux complet
d'arêtes avant S2 n'est pas mesuré ici. Les 280 728 `F` des 67 arêtes
fermables représentent un potentiel conditionnel de cœur évité **si et
seulement si** un déclencheur peu coûteux les reconnaît avant ce cœur ;
ce ne sont pas des sites de cœur économisés dans le produit. La prochaine
expérience utile est un prouveur borné par groupes de nœuds, déclenché
par des descripteurs déjà disponibles avant le cœur et mesuré sur **tout**
le flux, succès et replis inclus. Il faut ensuite tester plusieurs
trames, sans sol puis avec sol, coupes physiques et densités.

## Rejouer et vérifier

Depuis la racine du dépôt, avec l'archive produit CPU dont le SHA est
`208aabb30a764bad25ab1c99d74885bd405e84a13dcb8375622d66aa6203f70a` :

```sh
python3 -B morsehgp3D_v9/audits/lidar_raw_physical_scaling_20260923/generate.py \
  --repo "$PWD" --out /tmp/mhgp9-index-inputs
python3 -B morsehgp3D_v9/audits/paired_guard_index_shadow_20260923/prepare.py
g++ -std=c++20 -O2 -Wall -Wextra -Werror \
  -I build/v9-open-worktree/morsehgp3D_v9/src \
  -I build/v9-open-worktree/morsehgp3D_v9/src/gen \
  morsehgp3D_v9/audits/paired_guard_index_shadow_20260923/index_shadow.cpp \
  build/v9-open-worktree/build/v9-dev/libmhgp9_gen.a -pthread \
  -o /tmp/mhgp9-index-shadow
/tmp/mhgp9-index-shadow --selftest
/tmp/mhgp9-index-shadow \
  /tmp/mhgp9-index-inputs/s00_full_full.u32le \
  /tmp/mhgp9-index-inputs/s00_full_full.raw_return_ids.u32le \
  morsehgp3D_v9/audits/paired_guard_index_shadow_20260923/SAMPLES.tsv \
  > /tmp/mhgp9-index-replay.stdout
python3 -B morsehgp3D_v9/audits/paired_guard_index_shadow_20260923/verify.py \
  /tmp/mhgp9-index-replay.stdout > /tmp/mhgp9-index-replay-summary.json
```

Les temps du rejeu peuvent différer ; le lecteur compare les identités,
`F`, palettes par l'assertion native, paires et fermetures aux reçus
publiés. Ajouter `--budget64` ou `--budget256` à la commande native pour
les captures bornées. Pour rejouer [`TRIGGER_D.json`](TRIGGER_D.json),
utiliser les huit parties S2 épinglées du premier reçu de cœur et
`trigger_d.py --inputs /tmp/mhgp9-index-inputs
--trace /tmp/mhgp9-edge-core-audit-20260923/full/trace
--output /tmp/mhgp9-trigger-replay.json` ; les huit parties originales
peuvent être reconstruites via
[`edge_matched_core_20260923/replay.sh`](../edge_matched_core_20260923/replay.sh).
Le script exige leurs SHA épinglés ; si une nouvelle répartition entre
workers change les parties, il refuse de confondre ce rejeu avec la
capture d'origine. Le lecteur statique ne prouve pas à lui seul
l'identité des fichiers `/tmp` ni les échecs des recherches interrompues.
`SHA256SUMS` épingle tous les petits fichiers de ce dossier.
