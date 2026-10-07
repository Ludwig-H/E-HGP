# MES-G1 / MES-M7 : preuves relues et limites du juge

Pin `274592a30f6961cb7702125dcd2f031ff22b7df2`. Aucun rejeu LiDAR ni temps G4.
Le [lecteur](check_receipts.py) confronte les JSON publics aux **six journaux complets**
retrouvés par leurs SHA-256, vérifie leurs partitions et rapproche les routes G1/M7.
Les deux exécutables historiques disponibles portent les hashes publiés. Cela ne
reconstitue pas une attestation source→build antérieure à leur exécution ; les
payloads réels et leurs hashes déclarés ne sont pas relus ici.

## Comptes confirmés, adoption toujours en attente

- K5, ng00–02 : **456 534 / 549 923 = 83,0178 %** des anciens census saturés sont
  certifiables par les voisins ; par trame : 82,37 / 83,07 / 83,88 %.
- K10, ng00 : **1 585 555 / 1 656 079 = 95,7415 %**.
- Parmi les succès K5, la cible change sur **28,7 à 30,4 %** des parties. Ces taux
  mesurent la possibilité locale sur les traces de la v11 ; une descente effectuant
  ces sauts suivra d'autres traces. Elle doit mesurer son propre census, son coût
  total et la préparation des voisins avant adoption. Le reçu déclare correctement
  G-L3 non adopté. Trois trames de la même séquence ne qualifient pas D7.
- Les census complets de sphères déjà au catalogue sont bien isolés : 3 / 3 / 13
  à K5 et 18 à K10. L'absence de contre-exemple publiée concerne uniquement
  les sphères hors catalogue et les ordres 2..K.

Le profil M7 retrouve **41,596 %** des cycles dans les sondes, **23,300 %** dans
proposition/T1 et **20,828 %** dans le census à K5 ; à K10, respectivement
32,911 / 31,316 / 22,883 %. Les occurrences égalent celles des routes et les
graines sont déclarées identiques dans chaque journal. Ce sont les parts de la
**passe locale instrumentée**, biais inclus, pas une attribution des temps G4 ni
une preuve du bénéfice de G-L5. La plage de surcoût « 18 à 28 % » du README vaut
pour K5 ; à K10, les ratios par ordre vont de **0,967 à 1,462**, signe supplémentaire
qu'une prise locale partagée ne décide pas le contrat de performance.

## Résidu de CST-0018 : le juge G1 fait confiance à la route annoncée

[g1_probe.py](g1_probe.py) appelle le vrai exécutable compilé depuis le pin,
une seule unité C++20 Release liée à l'archive v11 existante, dont le SHA-256 est
dans [g1_build.json](g1_build.json). Les 29 dépendances locales de compilation
ont été confrontées au pin. Le petit `--porte` livré (17 sites, quatre témoins)
réussit également ; aucun mutant natif n'est rejoué.

Témoin indépendant : quatre sites `(0,0,0)` à `(3,0,0)`, K=2. Le catalogue exact
contient les trois paires adjacentes et les deux paires de distance 2. Le lecteur
de transition admet ce catalogue. La partie F={0,3} a deux intérieurs, {1,2}, et
sa boule **n'appartient pas** au catalogue (`p+q=4 > K+1=3`). Une ligne de route 2
est donc correcte : le vrai G1 recense, certifie et publie un census saturé.

Une mutation d'**un seul octet**, route 2 → route 1, laisse `ball=kNone` et tous
les autres champs inchangés. `mes_g1.cpp:583–586` incrémente route1 puis saute
immédiatement la ligne, sans contrôler la boule ni le certificat de table.
Résultat : **code 0, zéro écart**, census saturé devenu zéro. C'est une admission
incohérente de la preuve d'entrée, pas un défaut du test géométrique de côté.
Il faut valider la route catalogue (boule, support et inclusions) ou exiger une
validation stricte du vidage avant calcul des proportions.

Autre refus manquant : sur le même catalogue K2, `--ordres 9` rend code 0 avec
aucun ordre joué. Refuser une sélection absente de 2..K ; un bilan vide ne doit
pas devenir une preuve. Aucun de ces défauts n'est allégué dans les six journaux
réels relus, et aucun résultat d'adoption n'est fabriqué par cette sonde.

## Conservation et rejeu

[result.json](result.json) garde les rapprochements et hashes, [g1_result.json](g1_result.json)
les trois appels synthétiques. Normal et `-O` sont identiques pour les deux scripts.
Les journaux, binaires et données ne sont pas copiés ; les chemins privés ne sont
pas publiés. Le script de lecture exige `--external-dir DOSSIER_HISTORIQUE_G1`.

Pour reproduire la petite sonde native, compiler `mes_g1.cpp` avec les flags de
`g1_build.json`, les en-têtes v11 du pin et l'archive v11 de même empreinte ; fournir
ce binaire et son manifeste à `g1_probe.py --binary BINAIRE --build MANIFESTE`.
Le manifeste doit associer l'exécutable à ses dépendances du pin ; une source
différente est refusée. Aucun gain de vitesse, catalogue intégré ou FULL qualifié.
