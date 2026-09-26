# S2 — témoins partagés par tuiles indépendantes

26 septembre 2026. `exploration_v9_hors_registre`, `cpu_reference`,
`quantized_u18_input_only`, `audit_tile_cache`, `not_claimed`.
Prototype isolé ; moteur, CUDA, CMake et protocole inchangés. GCP non utilisé.

## Pourquoi cette piste

La voie CPU possède déjà un cache exact de nœuds témoins par ligne. La
voie GPU S2 repart de la racine pour chacune des paires. R24-B/00/K5 compte
1 110 657 775 visites de nœud par paire sur GPU, aucun cache. Le cache CPU
séquentiel se met à jour après chaque recherche : le porter littéralement
introduirait une dépendance entre les paires. Ce prototype remplace cette
dépendance par des **tuiles indépendantes** d'au plus 32 paires d'une même
ligne de rectangle.

Une recherche exacte sur la première paire fournit une trace de nœuds
admis. Les autres paires de la tuile retestent cette petite antichaîne en
parallèle potentiel. Une voie ayant assez de témoins stricts est rejetée ;
chaque voie restante repart de zéro dans la recherche historique. Aucune
mise à jour de la trace par les autres paires, aucun crédit partiel transmis.
Le résultat d'une tuile ne dépend d'aucune tuile précédente.

## Exactitude et coût

La trace provient d'UNE recherche du même index immuable. Pour chaque voie,
ses nœuds sont disjoints. L'admission est recalculée pour la nouvelle paire
avec les bornes exactes et strictes actuelles ; Hmin>0 exclut notamment les
nouveaux endpoints. Un rejet du cache implique donc celui de la recherche
globale. L'échec du cache ne conclut rien : repli obligatoire pour les bits
encore ouverts. Les fonctions produit `filter_q34_witnesses` et
`q34_cached_witness_rejections` sont appelées sans copie de géométrie.

Chaque nœud tracé apporte au moins un crédit effectif à l'une des deux
voies. Leur somme est au plus (K−1)+(K−2)=2K−3 : au plus sept nœuds à K5,
dix-sept à K10. Cette borne permet un paquet GPU court de nœuds/masques,
sans allocation par point ni histogramme O(|A|²+|B|²). Le cache ajoute au
plus O(K) tests géométriques par paire ; les recherches de repli restent
payées. Cela ne réduit pas l'exposant du nombre de paires résiduelles.

L'API publique CPU vérifie également la disjonction en O(K²) par requête.
**Ces contrôles structurels ne sont pas comptés dans `cache_node_tests`.**
Un port GPU interne pourrait utiliser l'antichaîne certifiée du producteur
sans refaire sa validation structurelle pour chaque paire ; il ne doit pas
accepter arbitrairement des nœuds publics comme s'ils avaient cette origine.

## Méthode et résultats bornés

Le nuage entier 1 mm sert à l'index et au front WSPD. Ensuite 4 096 positions
sont réparties déterministement dans la masse brute des rectangles ; chacune
désigne une tuile de ligne, puis les doublons de tuiles sont retirés. Seuls
les rectangles ainsi sélectionnés passent le filtre rectangle S2. Toutes
les paires de chaque tuile survivante sont jugées contre la recherche
globale. C'est un **échantillon diagnostique de requêtes**, pas une trame
sous-échantillonnée proposée comme contrat. La déduplication des strates
interdit de traiter les ratios comme un estimateur sans biais de la trame.

| Trame sans sol | K / s | Paires jugées | Visites témoin | Visites variante + tests cache | Rapport de travail |
| --- | --- | ---: | ---: | ---: | ---: |
| 08/000000 | 5 / 8 | 23 143 | 1 045 348 | 541 432 | 1,93 |
| 08/000100 | 5 / 8 | 13 479 | 623 474 | 400 019 | 1,56 |
| 08/000200 | 5 / 8 | 19 350 | 761 018 | 460 258 | 1,65 |
| 08/000000 | 10 / 8 | 19 364 | 1 175 745 | 780 115 | 1,51 |
| 08/000000 | 5 / 10 | 18 285 | 927 089 | 513 942 | 1,80 |
| 08/000000 | 5 / 12 | 13 638 | 717 145 | 413 047 | 1,74 |

**107 259 masques égaux**, aucun désaccord. Chaque cas exerce de vrais
rejets complets et partiels par cache ; le repli après rejet partiel est
donc réellement payé. Les représentants sont inclus dans le travail de la
variante. Sur 00/K5/s8, 944 représentants suffisent pour 23 143 paires ;
14 728 autres paires sont entièrement rejetées par cache, 3 206 partiellement.
Les trois trames viennent d'une seule séquence, pas de plusieurs séquences.

Les durées `sample_with_judge_ms` incluent les DEUX bras et leur comparaison,
sur hôte partagé avec d'autres audits. Aucun gain temporel CPU/GPU ne s'en
déduit. Aucun nouveau test de croissance 8k/16k/32k de la chaîne ici.
Les résultats n'autorisent ni « S2 divisé par deux sur G4 », ni un nouveau
chrono FULL ; ils justifient un port expérimental bien borné.

## Portes causales et provenance

[`gate.cpp`](gate.cpp) vérifie cache vide ⇒ mêmes masques et tous les
compteurs de recherche, auto-relecture de la trace, et refus de nœuds
chevauchants. La fixture collinéaire x={0,10,1,5,6}, K3, compare les paires
(0,10) et (0,1). La première est rejetée, la seconde doit garder les deux
voies. Le mutant compilé « réutiliser le rejet du représentant sans retest »
est tué par `cause=tile_cache.endpoint_retest`, code 1, pas par crash.
Cette porte est Release ; les sanitizers des autres prototypes ne lui sont
pas attribués. Le refus K/s est relu avec son message exact, pas seulement
le code 2 qu'un fichier absent pourrait aussi produire.

[`results/MANIFEST.json`](results/MANIFEST.json) conserve commandes, sources
avant/après, bibliothèque, binaire et sorties hachées. Les entrées sont
hachées après appel puis revérifiées par le lecteur LIVE ; cette capture
ne prétend pas avoir un hash d'entrée avant chaque appel. Leur provenance
est celle des fichiers v8 1 mm référencés, sans copie de données ici. La
bibliothèque vient du build neuf
`/tmp/mhgp9-b-100ms-20260926-build`, construction documentée et bibliothèque
hachée dans [le lot S2 numérique](../b_s2_narrow_20260926/README.md), au même
état produit ; le reçu n'archive pas la sortie complète de sa compilation.
Une première compilation du harnais a refusé l'affectation à `Point3::operator[]`
(accès par valeur) ; remplacée par les champs x/y/z avant la capture, sans
changement géométrique. Aucun build historique écrasé.

```sh
python3 -B morsehgp3D_v9/audits/b_s2_tile_cache_20260926/readback.py
python3 -B -O morsehgp3D_v9/audits/b_s2_tile_cache_20260926/readback.py
```

`run.py` et `gates.py` refusent d'écraser leurs captures. Le lecteur dépend
des sources, entrées et binaires locaux : ce n'est pas une archive autonome.

## Port GPU proposé

Deux passes : produire en masse les traces courtes des représentants, puis
tester les autres paires avec une trace possédée et immutable par tuile.
Compacter les survivants dans l'ordre historique. Mesurer les petits
rectangles, les tuiles partiellement remplies, la divergence, le stockage
et les relances : économiser des visites ne garantit pas un gain GPU.
Le juge compare chaque masque et les objets finaux, mais **n'exige plus
l'égalité des visites** avec S2 sans cache, puisque leur baisse est l'objet
même de ce changement. Les visites témoin, du représentant, du cache et du
repli doivent être séparées. Protocole G4 et sceau à adapter explicitement
avant d'attribuer un gain à une tour entière.
