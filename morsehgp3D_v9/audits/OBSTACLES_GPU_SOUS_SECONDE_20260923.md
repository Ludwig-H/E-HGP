# Audit — ce qui sépare encore la tour FULL de 1 s sur GPU G4

23 septembre 2026. Reçu de référence :
[`g4_tower_r7b_20260923`](../receipts/g4_tower_r7b_20260923/README.md),
paquet `8e8b83a3`, G4 **CPU seul**, sans sol, grille 1 mm, `s=8`,
W48, tour FULL `complete_relative`. Deux répétitions ON/OFF du MEB proposé
sur trois trames de la seule séquence 08 ; aucune mesure GPU. La
[contrelecture indépendante](CONTRE_AUDIT_B_G4_R7B_20260923.md) vérifie le
reçu et borne ses comparaisons aux projections publiées. R7b précède le
Welzl move-to-front, les nouveaux séparateurs du tri FULL et la libération
précoce de l'index de clés : aucun de ces changements n'a encore un chrono
G4. [R6](../receipts/g4_tower_r6_20260923/README.md) reste historique ;
la [tentative R7](../receipts/g4_tower_r7_stockout_20260923/README.md) a
été refusée avant démarrage.

## Plancher de la chaîne actuelle, pas du futur algorithme

Les meilleurs cas R7b ON de la trame 08/000100 ont les postes suivants
(secondes, chronomètre interne de chaîne, hors digest) :

| Tour | Total | q3/q4 | FULL | Autres postes |
| --- | ---: | ---: | ---: | ---: |
| K=1..5 | 3,669 | 2,523 | 0,734 | 0,412 |
| K=1..10 | 9,579 | 5,226 | 3,199 | 1,154 |

Sources brutes : `vm/probe_8.stdout` et `vm/probe_12.stdout` du reçu
R7b. Sur cette chaîne **séquentielle par étages**, rendre q3/q4
gratuit laisserait donc 1,146 s à K5 et 4,353 s à K10 ; rendre en plus
FULL gratuit laisserait encore 1,154 s à K10. Ce ne sont pas des
bornes physiques du GPU : chevauchement et nouvel algorithme peuvent
modifier la somme. Elles réfutent seulement l'idée qu'un unique port
GPU de q3/q4, ou du tri final, garantisse le contrat de 1 s. Le digest
synchrone, compté séparément, ajoute 0,162/0,798 s ; le mur externe
est 3,925/10,555 s sur ces deux cas. Même sur cette meilleure trame,
le facteur total à gagner est donc environ ×3,7 à K5 et ×9,6 à K10 hors
digest, et davantage sur les deux autres trames.

R7b/K10/000100 développe 17 488 839 paires q3/q4, construit
1 656 978 covers et 4 383 302 boules distinctes (catalogue
981 859 648 octets). L'aval produit 5 954 045 nœuds FULL,
3 548 340 contributions, 13 665 670 représentants ; il effectue
8 574 527 appels MEB et
282 268 672 visites de nœuds pour intrus. Le compteur de formes
chargées était déjà supérieur à 1,12 milliard dans R6 sur la même
entrée ; les masses inchangées de candidats ne rendent pas ce coût nul.
Ces nombres décrivent un problème de **quantité de travail** et de
granularité, pas d'abord un manque de fils GPU. La fusion du catalogue
prend 0,093 s dans ce cas : le tri/fusion seul n'est plus le verrou.
Les autres trames R7b montent à 32,79 M paires développées et
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
3. **Porter aussi l'aval exact, et pas seulement son tri.** MEB,
   dédoublonnage/clés, calendrier des niveaux, multifusions, parents et
   émission FULL ne disparaissent pas avec q3/q4. Le tri est déjà
   parallélisé sur CPU. Les ordres K sont parallèles, mais chaque ordre
   ferme encore ses niveaux exacts successivement ; dans un niveau,
   `order_block` puis la fusion sont aujourd'hui séquentiels. Un état
   pré-niveau figé permettrait de calculer les blocs indépendamment,
   puis de résoudre leurs composantes connexes avant publication des
   ancres ; `order_root` comprime actuellement les chemins par écriture,
   donc son emploi concurrent direct est incorrect. Il faut mesurer la
   distribution des tailles de lots avant de planifier un kernel par
   niveau. Le MEB proposé donne un gain de tour K10 de 5–8,4 % dans
   l'ablation R7b ; son successeur move-to-front n'y est pas mesuré.
4. **Conserver l'exactitude de bout en bout.** Les comparaisons de
   niveau utilisent notamment des entiers fixes 192/320 bits ; les
   décisions au contact, les supports canoniques et les replis ne
   peuvent être remplacés par des décisions `float` GPU non certifiées.
   Chaque voie filtrée doit replier exactement et reproduire les
   sorties, indépendamment de l'ordonnancement CPU/GPU.
5. **Éviter les allers-retours et qualifier le vrai contrat.** Nuage,
   index, covers, catalogue et buffers de sortie doivent rester
   résidents autant que possible ; compter allocations, copies,
   synchronisations et reconstruction hôte dans le temps total. Les
   candidats actuels empruntent des spans de coquille et les buffers de
   présentations se recouvrent temporairement : le port asynchrone exige
   des offsets possédés, des lots bornés et une déduplication exacte
   déterministe. R7b ne mesure ni kernel GPU, ni autres séquences, ni
   trames brutes avec sol ; `complete_relative` ne prouve pas l'absence
   de clés entièrement omises dans le grand nuage.
   Le profil de référence est la tour complète, non un noyau ni un
   préfixe ; la croissance 8k/16k/32k et les ablations `s=8/10/12`
   restent à produire sur les régimes pertinents.

Le meilleur R7b exige déjà un gain total **×3,67** à K5 ou **×9,58**
à K10 sur `chain_total` hors digest ; les premiers essais des trois
trames culminent à **6,57 s** et **15,30 s**. Le GPU peut fournir une
grande partie de ce gain, mais aucune mesure présente ne le démontre.
Statut :
`not_claimed`.
