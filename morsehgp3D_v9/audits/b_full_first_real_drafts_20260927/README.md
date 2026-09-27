# Première occurrence : mesure sur de vrais drafts FULL

27 septembre 2026, base moteur `b9fcc3d63`, audit CPU local uniquement.
Cadre `exploration_v9_hors_registre`, `cpu_reference`,
`quantized_u18_input_only`, `mode=first_parent_real_drafts`,
`public_status=not_claimed`. Aucun moteur modifié, aucun GCP utilisé.

## Résultat à retenir

Le [chemin sans continuation](../b_full_first_parent_20260927/README.md)
reconstruit exactement les forêts natives sur une vraie trame LiDAR sans
sol et sur les trois tailles uniformes 8k/16k/32k. Le tri a disparu ; le
scratch demandé est presque divisé par quatre par rapport au prototype
général. En revanche, **aucun gain CPU net n'est démontré sur le LiDAR**.
Ce résultat soutient l'architecture linéaire à paralléliser, pas un
remplacement immédiat de l'encodeur scalaire du moteur.

| entrée K1..5 | sites | natif, somme médianes ms | première occurrence, somme médianes ms | scratch demandé, octets |
| --- | ---: | ---: | ---: | ---: |
| 08/000000 sans sol, entière | 39 885 | 168,118 | 171,082 | 24 668 000 |
| uniforme synthétique | 8 000 | 63,779 | 62,383 | 10 070 464 |
| uniforme synthétique | 16 000 | 133,109 | 131,788 | 20 828 704 |
| uniforme synthétique | 32 000 | 308,247 | 282,465 | 42 564 992 |

Ce sont **des sommes de médianes d'encodages isolés par K**, trois essais
par ordre, pas des murs de tour ni des sommes de temps GPU. La machine
était partagée ; une campagne locale d'arène q34 W1/W4 ABBA du développeur
était en cours pendant cette fenêtre. La forte variabilité est conservée
dans les reçus. Aucun rejeu sélectif n'a remplacé ces mesures.

Sur LiDAR, détail natif/première occurrence en ms : K1 3,077/2,748 ;
K2 9,545/12,754 ; K3 32,836/31,187 ; K4 54,422/49,100 ; K5 68,239/75,293.
Il n'est pas justifié d'en déduire un gain stable, encore moins le contrat
100 ms pour une tour explicite G4.

## Attribution et comparaison

`probe.cpp`, `run.py` et `CMakeLists.txt` sont des ports explicites de
`b_full_real_drafts_20260927` publié en `b9fcc3d63`. La géométrie, l'accroche
aux deux overloads, les copies et les chronos sont inchangés. Les seules
adaptations fonctionnelles sont l'appel de `audit_first_parent::encode`,
la vérification explicite d'absence de continuation dans ces mesures,
le scratch `2*A*sizeof(size_t)` et les noms de build/schéma/capture.

Le constructeur natif reste appelé par la chaîne. Après capture, natif et
prototype réencodent le même draft dans l'ordre AB/BA/AB. Tous les champs
des nœuds, parents, successeurs et contributions, les mots exacts des niveaux,
l'ordre et l'identité de banque sont comparés ; le prototype est aussi
comparé à la forêt réellement publiée. Vingt ordres, soixante paires de
réencodages au total. La génération des drafts est hors de ces chronos
d'encodage, mais son coût est publié ci-dessous.

L'accroche garde une seule unité de traduction incluant `tower_chain.cpp`,
sans lien `libchain` ni seconde définition alternative du Builder. Un
écrivain par case K préallouée ; aucune modification de la banque privée
du moteur. Le générateur et le stub GPU sont ceux des sources/bibliothèque
épinglées, pas une copie réécrite. Tous les leviers GPU restent OFF.

Les petites portes comparent aussi le pass-through sans capture, avec les
trois digests égaux, et exercent les formes imbriquée et plate du draft.
Les grandes mesures utilisent la forme plate réelle, W4/static4/s8/K5.
Les verticales restent celles du produit : **le prototype ne les produit
pas**, et cette capture ne qualifie pas un autre constructeur FULL complet.

`compare.py`, lecteur post-capture distinct et non inclus dans les pins de
compilation, vérifie les identités d'entrée, paramètres, trois digests,
tailles et capacités des nouveaux drafts contre les mesures générales
précédentes. Il ne remplace pas les lecteurs LIVE de chaque génération.
Les deux schémas restent différents ; les anciens reçus sont inchangés.
Les temps historiques du prototype à tri ne sont pas une comparaison
appariée inter-processus et ne servent pas à annoncer un facteur de gain.

## Travail, mémoire et génération réellement payés

Toutes les actions de ces quatre vrais drafts ont zéro ou au moins deux
parents : aucune continuation, donc le chemin spécialisé est effectivement
utilisé. Actions/nœuds : 1 541 750 pour LiDAR, puis 629 404 / 1 301 794 /
2 660 312 pour uniforme. Les sorties et leurs tailles sont exactement
celles de la capture générale précédente, sans nouvelle compression.

Le travail de l'encodeur spécialisé est O(B+A+P+C) en taille du draft ;
sur les entrées uniformes, A fait ×2,068 puis ×2,044 et les contributions
×2,066 puis ×2,045. Cela n'est ni une borne générale en nombre de points,
ni une mesure de croissance du générateur sur plusieurs scènes LiDAR.

Le scratch demandé est `batch[A]+first[A]`, soit 16A octets sur ce build.
Sur LiDAR, 24 668 000 octets contre 98 671 880 pour le précédent prototype
à tri. Ces sommes par K ne sont pas des pics d'exécution concurrente,
excluent sorties, pile et métadonnées d'allocateur ; le prototype réencode
un seul K à la fois. Les allocations et écritures de sortie sont dans le
chrono ; comparaisons et destruction des objets retournés sont en dehors.
La destruction des temporaires internes est incluse.

| entrée | chaîne instrumentée ms | copie des drafts, somme par K ms | pic processus KiB |
| --- | ---: | ---: | ---: |
| LiDAR00 sans sol | 28 312,692 | 53,642 | 960 348 |
| uniforme 8k | 6 212,132 | 21,930 | 423 808 |
| uniforme 16k | 12 856,766 | 47,216 | 865 264 |
| uniforme 32k | 27 979,463 | 91,120 | 1 701 260 |

LiDAR : q2 1 535,479 ms, q34 24 061,093 ms, tour instrumentée 1 443,262 ms,
mur externe de l'appel de chaîne 29 031,880 ms. Capacités : drafts copiés
127 383 080 octets, sorties natives 195 162 040, banque partagée 59 075 492.
La copie contamine la fenêtre d'encodage de la chaîne (87,744 ms), et la
somme des copies ne doit **pas** être soustraite à un mur parallèle.
Le pic RSS comprend génération/capture/réencodages ; il ne mesure ni la
mémoire du seul nouvel encodeur ni la VRAM.

L'entrée LiDAR reste 08/000000 entière après masque sans sol figé, grille
1 mm, 39 885 sites, préparation historique v8 lue sans copie dans v9.
Le runner vérifie les sept partitions mais ne chronomètre que le plein
nuage retenu. Pas de nouvelle segmentation, de vérité terrain ni de coût
de préparation dans ces temps. Les autres entrées sont bien synthétiques.

## Qualification et suite

Première capture réussie : six commandes de qualification GCC Release /
Clang ASan/UBSan/LSan, puis quatre mesures Release, aucune campagne GCP.
Les petites portes font quatre chaînes, dix ordres et trente paires par
binaire. Aucun nouveau test TSan et aucun thread interne à l'encodeur.
Les cinq lecteurs LIVE passent en modes normal et `-O` ; le comparateur
post-capture aussi. Sources, bibliothèque, configurations, binaires et
entrées sont épinglés ; les sorties sont reliées à leurs commandes.

Build clos, ne pas reconstruire :
`/workspaces/E-HGP/build/v9-audit-full-first-real-drafts-20260927-r1`.
Autorité : [receipts/full_first_real_drafts_20260927/r1](../../receipts/full_first_real_drafts_20260927/README.md).

```bash
python3 -B morsehgp3D_v9/audits/b_full_first_real_drafts_20260927/run.py check qualification
python3 -B -O morsehgp3D_v9/audits/b_full_first_real_drafts_20260927/run.py check ng00
python3 -B morsehgp3D_v9/audits/b_full_first_real_drafts_20260927/compare.py
```

La suite justifiée est la réduction minimum et la dispersion réellement
parallèles, avec allocation/écriture de la sortie explicite payées, puis
mesure G4 de la chaîne entière. Le tri supprimé est un obstacle retiré,
pas la promesse que la génération des drafts ou le FULL passent en 100 ms.
