# Contre-audit B — G4 R5, tour K parallèle

23 septembre 2026. Reçu produit :
[`g4_tower_r5_20260923`](../receipts/g4_tower_r5_20260923/README.md),
commit d'exécution `aae9da0e`, publication `19de1b7f`. Cadre :
`exploration_v9_hors_registre`, CPU G4 SPOT, grille u18/1 mm, sans sol,
`complete_relative`, `not_claimed`. Ce document contre-vérifie le reçu ;
il n'élargit pas le contrat qualifié.

## Intégrité et portée

- `sha256sum -c` sur le reçu versionné : **245/245 OK**. Sur la capture
  hôte vivante, contrelecture des **152/152** entrées du manifeste,
  **147/147** sources du commit, trois blobs d'entrée, **14/14** commandes
  hôte et **47/47** commandes invitées avec sorties et hashes cohérents.
  Les cartes source avant/après sont égales. Le snapshot, le manifeste
  et la capture coïncident avec le reçu. Le préflight natif de 1 500 sites
  est non vacant, mais n'est pas un cas contractuel.
- **13/13** cas FULL CPU aboutissent, sept comparaisons internes sont
  égales. Arrêt ciblé relu `TERMINATED` ; aucun autre VM du projet n'est
  indiqué active par le journal de fermeture. La copie `lifecycle.txt`
  garde `targeted_running`, état intermédiaire historique ; le reçu hôte
  `completed`, `guarded_stop` et sa sortie font autorité pour la clôture.
  Le validateur de réception conserve la faiblesse de rebinding des
  champs de garde déjà décrite dans
  [l'audit v5](CONTRE_AUDIT_B_G4_RECEPTION_V5_20260923.md) ; ici, la
  marque et le schedule bruts ont aussi été examinés séparément.
- Seules trois trames **08/000000, 000100, 000200** d'une **même séquence**
  SemanticKITTI, préalablement privées du sol et mises sur grille 1 mm,
  sont mesurées. K=1..5 et 1..10, s8, 48 vCPU/48 fils statiques, deux
  répétitions par cas ; un cas supplémentaire K10 à 24 fils. Ce n'est
  ni la trame brute entière, ni plusieurs séquences, ni float32 original,
  ni s10/s12, ni GPU. L'exactitude FULL est **relative au catalogue
  croisé**, pas une preuve indépendante de sa complétude globale ; un
  condensé de tour n'est pas une sérialisation exhaustive.

## Résultats vérifiés

Première répétition à 48 fils ; `total` inclut tout le chemin sonde
depuis la lecture jusqu'à la tour. Les temps R4b sont des comparaisons
inter-sessions sur la même cible, mêmes entrées/options, mais **pas une
ablation appariée** du seul parallélisme FULL.

| Trame 08 | K | Total R4b → R5 (s) | q3/q4 R5 (s) | Tour R4b → R5 (s) | RSS R5 (Gio) |
| --- | ---: | ---: | ---: | ---: | ---: |
| 000000 | 5 | 7,388 → 5,984 | 3,973 | 2,474 → 1,023 | 1,090 |
| 000000 | 10 | 26,688 → 17,369 | 8,572 | 14,890 → 5,368 | 4,475 |
| 000100 | 5 | 5,439 → 4,332 | 2,794 | 2,005 → 0,857 | 1,045 |
| 000100 | 10 | 19,398 → 12,088 | 5,497 | 11,416 → 4,147 | 3,755 |
| 000200 | 5 | 9,182 → 7,379 | 5,188 | 2,653 → 1,093 | 1,219 |
| 000200 | 10 | 29,500 → 19,566 | 11,197 | 14,582 → 4,849 | 4,721 |

Les six cas R4b cache ON correspondants ont **mêmes objets complets**
`generator`, `catalogue`, `orders`, `tower_work` et même `tower_digest`
que R5. Dans `ledger`, seules `dead_uniform_tests` et les trois compteurs
de cache de témoins changent légèrement avec l'ordonnancement ; covers,
sites, graines et émissions ne changent pas. Les deux répétitions R5 ont
également les mêmes sorties logiques. Le total baisse de **19–38 %**, la
phase tour de **57–67 %** ; q3/q4 reste globalement du même ordre et est
maintenant le premier poste. La comparaison ne sépare pas les effets des
commits `684d8fc7`, `133c8653`, `47f8a5da` et `aae9da0e`.

Sur 000000/K10, 24 fils donnent 21,586 s contre 17,369 s à 48 fils,
avec mêmes objets/digest et 4,217 contre 4,475 Gio de RSS ; le gain total
n'est que ×1,24 pour 48 fils, avec environ 42 % de CPU·s en plus. La
machine expose 24 cœurs/48 fils matériels. Ces résultats demandent de
mesurer le travail q3/q4 et les pics co-résidents, pas seulement le
nombre de workers.

Le verrou massif est aussi matériel : à K10, ces seules trames de
35–46 k sites produisent **4,38–5,51 millions de boules** et le
catalogue annonce **0,982–1,235 Go décimaux**, tandis que le RSS du
processus monte à 3,76–4,72 Gio. Une simple extrapolation au même
rapport boules/site vers 30 M points impliquerait des milliards de
boules et une résidence hors de portée ; ce n'est **pas** une borne
sur le LiDAR à cette taille, mais elle interdit de prétendre au contrat
massif sans sorties/runs/catalogue et tour adressables par fenêtres,
avec coûts d'export mesurés.

## Verdict et suite d'audit

Le meilleur temps R5 est **4,263 s** pour K1..5 et **12,067 s** pour
K1..10 ; ni 1 s ni 100 ms ne sont atteints. Il n'existe toujours aucune
mesure GPU de toute la tour, ni preuve de sous-quadraticité sur les
régimes LiDAR visés. À K10, q3/q4 prend 5,5–11,2 s et compte
4,15–9,28 milliards de formes **hors extrémités** selon la trame ;
`load()` écrit aussi deux formes nulles par cover, soit exactement
`dead_form_sites+2·dead_loads` formes physiques. Accélérer seulement le tri
ou la phase tour ne peut fermer le contrat. Priorité : certificat exact
avant expansion des produits q3/q4, puis réduction des covers et tests
de voies mortes, avec compteurs et repli résiduel complet. Le
[sidecar de lignes](CONTRAT_COUTS_ET_PARALLELISATION.md) ne prouve pas encore
un gain net. Qualifier ensuite sur coupes spatiales 8k/16k/32k appariées
et plusieurs séquences, avec et sans sol, s8/s10/s12, sorties et RSS.

Il existe aussi un **plancher mesuré hors q3/q4 et hors tour** : sur la
meilleure répétition K10 de chaque scène, retrancher intégralement ces
deux postes laisse encore **2,50 / 3,39 / 3,44 s** (000100 / 000000 /
000200). q2+fusion+recensus en représente déjà **1,43 / 2,02 /
2,06 s**. Il faudra donc réduire la préparation/fusion/catalogue et
les autres frais de chaîne en même temps que q3/q4 ; cette soustraction
est un diagnostic de postes, pas une prédiction de nouvel algorithme.

Prochaine mesure de croissance à coût contrôlé : les **sept fichiers
1 mm sans sol déjà figés** dans
`morsehgp3D_v8/receipts/lidar_ground_20260921/release/ground_fq64xq_6/scene_00_grid/`
(`full`, `half_x_neg`, `half_x_nonneg`, puis les quatre `quarter_*`, tous
en `.u32le`). Ils contiennent respectivement 39 885, 24 591, 15 294,
11 536, 13 055, 8 225 et 7 069 sites ; l'origine de grille, le masque
et les coupes capteur ont été figés avant cette v9. Le SHA-256 du
`full.u32le` est exactement celui du fichier d'entrée R5/000000
(`0baa4de1…`). Exécuter **chaque
morceau comme nuage autonome** sur un même snapshot FULL/K5/s8/W et
répéter la trame entière dans la même campagne. Publier les six
relations parent→enfant pré-déclarées, même défavorables, pour les
relations parent→enfant pré-déclarées, même défavorables, pour les
sorties, les compteurs dominants et les temps, avec les vrais rapports
de tailles `r=N_parent/N_enfant` ; pour un travail positif,
`p=ln(W_parent/W_enfant)/ln(r)`. Les cas nuls/égaux ne donnent pas
d'exposant. Étendre ensuite K10, s10/s12, scans bruts et autres
séquences. Les essais nominaux 8k/16k/32k complètent ces coupes mais
ne remplacent pas leur géométrie. Le reçu v8 spatial à 2 cm/q3-q4
seul n'est **pas** un exposant v9 à 1 mm/FULL.

Sur le snapshot R5, une réserve indépendante subsiste dans FULL : le correctif
`133c8653` rend le statut de banque publique sûr, mais la priorité du
plus petit K en échec n'est garantie que **par phase** ; voir
[l'audit des ordres](PREFETCH_GEOMETRIE_FULL_PAR_K_20260923.md) et la
[contrelecture B](CONTRE_AUDIT_B_FULL_PARALLELE_WIP_20260923.md).
Le commit produit ultérieur `84c74a5e` corrige ce point pour les
`Failure` des phases lots/images et ses trois tests ciblés passent,
mais il **n'appartient pas** au snapshot G4 R5 et ne qualifie pas encore
la priorité globale des erreurs de préparation/banque.
