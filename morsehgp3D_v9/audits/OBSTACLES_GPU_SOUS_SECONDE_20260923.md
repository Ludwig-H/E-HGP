# Audit — ce qui sépare encore la tour FULL de 1 s sur GPU G4

23 septembre 2026. Reçu de référence :
[`g4_tower_r6_20260923`](../receipts/g4_tower_r6_20260923/README.md),
snapshot `78ce9fd4`, G4 **CPU seul**, sans sol, grille 1 mm, `s=8`,
W48, tour FULL `complete_relative`. La [tentative
R7](../receipts/g4_tower_r7_stockout_20260923/README.md) a été refusée
par GCE avant démarrage : elle n'apporte aucun temps ni essai GPU.

## Plancher de la chaîne actuelle, pas du futur algorithme

Les meilleurs cas R6 ON de la trame 08/000100 ont les postes suivants
(secondes, chronomètre interne de chaîne) :

| Tour | Total | q3/q4 | FULL | Autres postes |
| --- | ---: | ---: | ---: | ---: |
| K=1..5 | 4,151 | 2,634 | 0,829 | 0,688 |
| K=1..10 | 11,726 | 5,321 | 3,997 | 2,408 |

Sources brutes : `vm/probe_8.stdout` et `vm/probe_12.stdout` du reçu
R6. Sur cette chaîne **séquentielle par étages**, rendre q3/q4
gratuit laisserait donc 1,517 s à K5 et 6,405 s à K10 ; rendre en plus
FULL gratuit laisserait encore 2,408 s à K10. Ce ne sont pas des
bornes physiques du GPU : chevauchement et nouvel algorithme peuvent
modifier la somme. Elles réfutent seulement l'idée qu'un unique port
GPU de q3/q4, ou du tri final, garantisse le contrat de 1 s.

R6/K10/000100 développe 17 488 839 paires q3/q4, construit
1 656 978 covers et 4 383 302 boules distinctes (catalogue
981 859 648 octets). L'aval produit 5 954 045 nœuds FULL,
3 548 340 contributions, 13 665 670 représentants ; il effectue
8 574 527 appels MEB, 260 646 771 tests exacts de puissance et
282 268 672 visites de nœuds pour intrus. Le compteur de formes
chargées `dead_form_sites + dead_core_form_sites` dépasse 1,12 milliard.
Ces nombres décrivent un problème de **quantité de travail** et de
granularité, pas d'abord un manque de fils GPU. La fusion du catalogue
prend 0,616 s dans ce cas : supprimer son coût seul laisserait plus de
11 s. Les autres trames R6 montent à 32,79 M paires développées et
5,51 M boules à K10 ; ne pas prendre le meilleur cas pour la garantie.

## Verrous techniques à lever

1. **Éliminer avant l'expansion.** Les certificats cœur/cover réduisent
   fortement les formes, mais le front développe encore toutes les
   paires résiduelles. Un certificat exact de bloc (nœuds WSPD × cellule
   de centres ou équivalent), avec coût et rejets comptés avant `A×B`,
   est prioritaire. Un filtre après expansion déplace le coût et ne
   prouve aucune pente sous-quadratique.
2. **Aplatir le travail réellement survivant.** Covers, atlas, graines
   et census ont des tailles et durées très variables. Un GPU a besoin
   de tâches compactées par étapes, d'offsets possédés et de longues
   rafales de calcul ; une arête entière affectée à un fil/warp reproduit
   le déséquilibre et les accès irréguliers. Mesurer chemin critique,
   dispersion par tâche, trafic et compactions, pas seulement le nombre
   de threads lancés.
3. **Porter aussi l'aval exact.** MEB, dédoublonnage/clés, calendrier
   des niveaux, multifusions, parents et émission FULL ne disparaissent
   pas avec q3/q4. Le MEB proposé par `458fb0ed` a un gain local
   encourageant, mais R7 n'a pu le mesurer sur G4. Réduire les appels
   et isoler les étapes parallèles avant d'introduire des kernels.
4. **Conserver l'exactitude de bout en bout.** Les comparaisons de
   niveau utilisent notamment des entiers fixes 192/320 bits ; les
   décisions au contact, les supports canoniques et les replis ne
   peuvent être remplacés par des décisions `float` GPU non certifiées.
   Chaque voie filtrée doit replier exactement et reproduire les
   sorties, indépendamment de l'ordonnancement CPU/GPU.
5. **Éviter les allers-retours et qualifier le vrai contrat.** Nuage,
   index, covers, catalogue et buffers de sortie doivent rester
   résidents autant que possible ; compter allocations, copies,
   synchronisations et reconstruction hôte dans le temps total. R6 ne
   mesure ni kernel GPU, ni autres séquences, ni trames brutes avec sol.
   Le profil de référence est la tour complète, non un noyau ni un
   préfixe ; la croissance 8k/16k/32k et les ablations `s=8/10/12`
   restent à produire sur les régimes pertinents.

Le meilleur R6 exige déjà un gain total **×4,15** à K5 ou **×11,73**
à K10 ; pour les trois trames R6, le cas lent exige respectivement
**×6,87** et **×18,06**. Le GPU peut fournir une grande partie de ce
gain, mais aucune mesure présente ne le démontre. Statut :
`not_claimed`.
