# Session A — admission indépendante des traces existantes

Capture du 8 octobre 2026, `phase=exploration_v12_hors_registre`, `backend=cpu_reference ; cuda_g4 pour le catalogue`, `objet=full_pi0`, `quantification=quantized_u21_input_only`, `public_status=not_claimed`.

**61 processus / 850 passes sont admis. La règle annoncée adopte le lot A, mais le contrat FULL de 100 ms sur plusieurs trames n'est pas atteint.** Aucun moteur, GPU, service distant ni jeu de coordonnées n'a été relancé ou lu par cet audit. Relecture de JSONL et métadonnées seulement.

L'avant réel est **27eca166b**, l'après **5f5c0c83f** ; la mention `902041f66` restée dans le pilote est obsolète pour cette campagne. Le plan exécutable et son archive avant SHA `929c3744…` font foi. La [provenance séparée](../session_a_provenance/README.md) ferme les sources, l'archive, les commandes et l'arrêt certifié. Le pilote A a fini avec code 0 ; le statut global `failed_remote` vient du CTest LiDAR distinct expiré (124), pas d'un échec de ce pilote. Le socle annonce 719 tests passés ; la qualification LiDAR complète reste absente.

## Admission et calcul

Le lecteur lie chaque prise à sa place dans le plan, puis vérifie SHA du journal, entier `code=0`, métadonnées exactes, profil u21, appareil, W48 (W1 seulement pour les deux identités ng00), K et noms attendus, schéma séquentiel/recouvert, alternance open/FULL/libération/exit, valeurs u64 et empreintes. Il exige exactement les cohortes, sans filtrer les observations étrangères :

- 25 processus d'identité, 106 passes : trois ng à K5/K10, ng00 W1, trois uniformes, les 37 trames, plus le chemin après séquentiel des trois ng. Les **46 objets** ont une empreinte FUL1 identique dans tous leurs bras/passes correspondants. Cela ne signifie pas 46 empreintes globalement distinctes.
- K5 décisif : trois ng × cinq tours appariés × deux bras × dix passes ; la première passe de chaque processus est retirée, soit **270 chaudes**.
- Sessions informatives : trois processus par bras, deux tours des 37 trames, ordre tournant de 0/1/2 positions ; seul le second tour compte, soit **222 chaudes**, trois par trame/bras.

Les deux observations GPU déclarent le même appareil et une liste de processus vide, explicitement renseignée. Les binaires hachés avant/après la campagne coïncident. Les codes 0 par processus sont archivés dans le rapport du pilote ; ce ne sont pas 61 reçus externes indépendants. Les options se déduisent du plan et du pilote épinglés, les JSONL ne contenant pas leurs argv. Les empreintes sont produites dans des processus d'identité distincts des chronos sans digest.

Le mur satisfait les inclusions source : P+C+G+raccord+TMVR ≤ mur ; en séquentiel T+M+V+R ≤ TMVR et tables+résolution ≤ G. En recouvert : fin = fin G + queue, queue = TMVR publié, P+C+tour ≤ mur. Les fenêtres de tâches se chevauchent : aucune somme n'est assimilée à une durée CPU ou à une partition exhaustive du mur. Les fins vérifient G≤T≤M, M≤R et M≤V pour k≥2 ; **V et R n'ont pas d'ordre mutuel imposé**. Les contrôles source vérifient aussi tables≤ouverture≤fin G, max G des ordres=fin G et toutes les fins≤fin.

Les relevés mémoire par étage sont complets : résident≤pic, pic suivant≥résident précédent, pic global=max des pics d'étage, entrée résidente≥16×Σsites distincts chargés. Le budget appareil est **partagé** ici, sans option de plafond distinct : `pic_octets` compte les réservations hôte et appareil du MemoryBudget ; `pic_appareil_octets=0` est conventionnel. Le RSS est un maximum cumulatif du processus, pas ce budget. Les 850 valeurs CPU·s et RSS sont présentes. Ni les diagnostics C ni les médianes d'étages ne sont sommés.

## Résultats admis

Mur FULL K5/W48, ms. Valeur centrale = médiane des cinq médianes de neuf passes chaudes ; maximum = plus grande de ces cinq médianes.

| Trame | Avant | Après | Maximum après | IC95 du rapport après/avant |
|---|---:|---:|---:|---:|
| ng00 | 147,466 | 99,744 | 101,678 | [0,669788 ; 0,683701] |
| ng01 | 117,213 | 81,029 | 82,602 | [0,687750 ; 0,699446] |
| ng02 | 150,413 | 97,800 | 99,449 | [0,636162 ; 0,658524] |

Les trois bornes hautes restent sous 1 : **adoption du lot** selon la règle figée. Recalcul indépendant des médianes et du bootstrap de 10 000 tirages (graine 20261008), plus rejeu du juge livré. La borne haute ng01 diffère d'un ULP (`1,11×10⁻¹⁶`) du rapport archivé ; le lecteur publie cet écart, tolère au plus deux ULP pour les seuls résultats flottants et conserve le même verdict. Les durées et compteurs entiers sont exacts.

Sur les 37 trames, chaque médiane porte sur trois passes chaudes :

| Statistique FULL K5 | Avant | Après |
|---|---:|---:|
| Médiane des 37 médianes, ms | 230,786 | 160,567 |
| Maximum des 37 médianes, ms | 440,291 | 319,740 |
| Maximum des 111 passes chaudes, ms | 448,211 | 327,273 |
| Trames avec médiane >100 ms | 35/37 | 26/37 |
| Passes >100 ms | 105/111 | 79/111 |

Les trois ng seules ne suffisent donc pas à fermer le contrat ; ng00 dépasse également 100 ms au maximum des médianes processus. K10 est joué pour l'identité, sans campagne chaude K10 dans ce lot. Aucun rapprochement causal avec la v11 ni isolement du seul recouvrement : l'objet comparé est le lot source avant/après entier, avec C et Pool communs.

Sur les 111 chaudes des 37 trames : CPU cumulé 453,809→475,097 CPU·s ; pic maximal du budget partagé 3 132 250 335→3 338 874 772 octets ; maximum RSS processus 1 375 133 696→2 836 582 400 octets. Ce sont trois mesures différentes. Les médianes d'étapes, ressources et valeurs par trame sont dans `results.json`.

## Reproduction et limites

```sh
python3 check.py DEPOT RETOUR_A PLAN_A > lecture.json
python3 -O check.py DEPOT RETOUR_A PLAN_A > lecture_O.json
cmp lecture.json lecture_O.json
```

`RETOUR_A` contient seulement les traces restituées ; `PLAN_A` est le plan JSON épinglé. Le lecteur contrôle l'inventaire assaini et les objets Git avant d'importer le pilote pour son seul parseur/juge pur. Il ne lance aucune fonction de campagne. Les sorties normal/−O sont identiques à `results.json`. Onze contre-flux dérivés d'une prise réelle conforme sont refusés : fermeture absente, raison d'ouverture incorrecte, types booléens, queue incohérente, mémoire absente ou invalide, mauvais schéma/trame et fins hors tour. Ces tests ne remplacent aucune porte moteur.

La source d'après, la sonde d'avant, les primitives mémoire et les chronos du pipeline sont épinglés dans `capture.json`. Le rejeu dépend des objets Git et des traces externes conservées : aucune copie des données ou des sources intégrales dans ce reçu. Les faiblesses génériques du pilote livré restent documentées dans les reçus antérieurs ; les prises présentes ne déclenchent aucun de ces trous. Aucun changement rétrospectif du critère d'adoption.
