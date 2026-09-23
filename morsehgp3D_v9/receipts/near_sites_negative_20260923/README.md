# Reçu négatif : certificat de voie morte sur les voisins proches (q3/q4)

23 septembre 2026. **GCP non utilisé.** Cadre : `exploration_v9_hors_registre`,
`backend=reference_cpu`, `profile=quantized_u18_input_only`,
`public_status=not_claimed`.

## Ce qui a été construit (hors produit, deux patchs sur `ed11c6c3`)

Le reçu `knn_core_probe_20260923` montrait que les voisins proches ferment
96 % (K5) et 86 % (K10) des arêtes arrivées au cœur. Ce patch construit
donc, avant les jobs q3/q4, les **16 plus proches autres sites** de chaque
site (`Q34NearSites`).

- **Construction** : kNN exact, un parcours en profondeur sur l'index
  immuable, borne de boîte, égalités départagées par ID. La table fait
  `n × 16` IDs `u32`.
- **Certificat** : le prouveur de voie morte existant (`Q34DeadLanes`)
  est appliqué à l'union dédoublonnée des listes des deux extrémités de
  l'arête, nouvelle entrée `load_sites`.
- **Correction** : tout sous-ensemble de vrais sites distincts est un
  témoin sain (note de C, `CERTIFICAT_Q34_SOUS_ENSEMBLES_LOCAUX_20260923`).
  Un échec retombe sur la voie exacte actuelle.
- **Levier et sonde** : levier `q34_near_prover`, sonde v17 locale.

Deux placements ont été mesurés.

- `near_sites_cache.patch` : après le cache de témoins, seulement quand le
  cache appartient à la même extrémité `a`, et avant la recherche
  ponctuelle de paire.
- `near_sites_survivors.patch` : sur les seules arêtes qui survivent à la
  recherche ponctuelle, avant le noyau diamétral.

Une première version du placement « cache » perdait les voies du cache au
retour anticipé : `validate_completion` refusait l'appel. Le placement
mesuré compte chaque voie une seule fois ; c'est le croisement cache/voisins
que B a signalé à 11 h 45.

## Mesure

Trame 08/000000 sans sol, entière : 39 885 sites, fichier
`morsehgp3D_v8/receipts/lidar_ground_20260921/release/ground_fq64xq_6/scene_00_grid/full.u32le`,
sha256 `0baa4de1…`. Réglages : s = 8, W8, `--no-tower`, tous les autres
leviers ON. Hôte local partagé à 8 cœurs, sonde v17 construite depuis le
patch. Les six sorties sont dans `out/`.

| placement | K | voisins | catalogue | q34 (ms) | CPU q34 (s) | requêtes de paire | cœurs | chargements voisins | tests uniformes voisins | fermées par voisins |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| survivants | 5 | ON | 1 306 696 | 22 657 | 128,56 | 7 161 944 | 1 135 450 | 2 043 612 | 308,8 M | 908 162 |
| survivants | 5 | OFF | 1 306 696 | 23 231 | 130,23 | 7 162 036 | 2 043 612 | 0 | 0 | 0 |
| cache | 5 | ON | 1 306 696 | 21 772 | 127,63 | 2 703 956 | 1 485 145 | 8 857 295 | 763,9 M | 8 146 050 |
| cache | 5 | OFF | 1 306 696 | 22 310 | 130,31 | 7 162 112 | 2 043 612 | 0 | 0 | 0 |
| cache | 10 | ON | 5 512 670 | 64 230 | 407,09 | 7 528 938 | 3 924 730 | 11 328 743 | 1 851,1 M | 7 224 742 |
| cache | 10 | OFF | 5 512 670 | 60 833 | 392,73 | 11 799 888 | 4 507 278 | 0 | 0 | 0 |

Coût des listes : 39 885 requêtes, 4 598 937 visites de nœuds, table de
39 885 × 16 IDs (2,6 Mo), soit un pic RSS inchangé à ±1 Mo.

## Lecture

- **Exactitude (portée limitée)** : les résumés `catalogue` des paires
  ON/OFF sont identiques champ pour champ (nombre de clés, présentations par
  arité, histogrammes, Euler) et les statuts sont `complete_relative`. Ces
  sorties n'énumèrent aucune clé et `--no-tower` ne produit ni ordres ni
  condensé FULL : l'égalité clé par clé et celle de la tour ne sont **pas**
  vérifiées ici (remarque de B, 12 h 04). Le reçu conclut seulement à
  l'absence de gain.
- **Gain** : le placement « cache » à K5 retire 62 % des requêtes de paire
  et 27 % des cœurs, mais ajoute 764 M tests de formes. Le CPU q34 ne baisse
  que de 2 %, le mur de 2 à 3 %, dans le bruit d'un hôte partagé.
- **Perte à K10** : le seuil passe à K−1 ou K−2 intérieurs uniformes, les
  cellules se résolvent plus profondément et 36 % des tentatives échouent.
  Le CPU q34 **augmente de 3,7 %** et le mur de 5,6 %.
- **Placement « survivants »** : les voies des seules arêtes qui atteignent
  le noyau ne sont fermées qu'à 44 %, pour −1,3 % de CPU à K5.
- **Écart avec le reçu `knn_core_probe`** : ses 96 %/86 % portaient sur des
  voisins **choisis dans le cœur déjà construit**, pas sur une liste
  globale, et ne comptaient pas le coût des formes. B l'avait signalé
  (11 h 40) : la proportion de fermetures ne prédisait pas le coût total.
- **Suite** : la recherche de paire et le noyau coûtent à peu près ce que
  coûte leur remplaçant par voisins. Le verrou q34 n'est pas dans le choix
  des témoins mais dans le nombre d'arêtes et de formes.
- Code retiré du produit ; la sonde reste en v16.

## Contenu

`near_sites_cache.patch`, `near_sites_survivors.patch`, `out/*.json` (sonde
v17 locale), `SHA256SUMS`.
