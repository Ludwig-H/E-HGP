# Audit J2/J2c : rejets, exactitude et témoins adverses

29 septembre 2026. Code examiné : `6206d1d11`, catalogue J2c `777406b82` ; le point d'entrée des audits ajouté ensuite ne change pas le code examiné. `phase=exploration_v10_hors_registre`, `backend=cpu_reference`, `profile=quantized_u18_input_only`, `mode=audit_independant`, `public_status=not_claimed`. Moteur intact, GCP non utilisé, aucune graine du banc test.

## Verdict

Pas de contre-exemple trouvé aux rejets J2/J2c. La campagne indépendante bornée clôt 432 appels natifs et 15 195 enregistrements contre des calculs rationnels exhaustifs. Elle vérifie aussi des propriétés que la porte actuelle ne compare pas directement : niveaux numériques exacts, support canonique minimal et poids.

Deux réserves industrielles demeurent : lecture binaire tronquée acceptée silencieusement ; conversions de tailles de catalogue en u32 sans refus. Une taille de feuille expérimentale trop petite peut également provoquer un travail gigantesque sur huit sites ; ce n'est ni le comportement du défaut ni une erreur de géométrie.

## 1. Ce qui tient dans les preuves

- `generator.cpp:481–558` : D-loc est exactement le maximum affine de la différence des distances carrées sur le pavé fermé. Les dominateurs sont des sites distincts du réservoir ; seuls leurs poids sont additionnés. Le site lui-même ne se compte pas, grâce au signe strict. Le mauvais choix d'un réservoir réduit la puissance du filtre, pas sa sûreté.
- `generator.cpp:561–619` : la liste contient la coquille complète dès que le poids intérieur est inférieur à K. Le centre critique appartient à l'enveloppe convexe de son support, donc à celle de la liste. L'intersection avec `[env.lo, env.hi + 1)` ne perd aucun tel centre. Le `+1`, la borne haute demi-ouverte et la partition en deux sont nécessaires.
- `generator.cpp:294–460` : les masques de dominance sont réunis avant comptage. Les parties du support canonique survivent aux seuils décroissants. Un triplet n'a pas besoin d'être aigu pour devenir vivant et engendrer un tétraèdre : l'aiguïté ne conditionne que l'émission q3. Cela évite une ancienne fausse piste de complétude q4.
- Les coquilles étendues sont recensées complètement ; leur support canonique identifie une sphère unique. Le mémo sur ce support est donc licite ici, bien que des conceptions historiques aient proposé un mémo sur toute la coquille.
- L'ordre commence en flottant, mais les comparaisons exactes de tous les voisins sont exécutées après réparation des bandes (`generator.cpp:779–836`). Un ordre erroné résiduel donne un refus, pas un catalogue publié dans le désordre.

Ces arguments supposent un `Cloud` préparé valide, immuable pendant l'appel, sur le domaine u18. Ils ne sont ni une preuve de coût global ni une qualification du catalogue pondéré pour la tour : cette dernière refuse encore les multiplicités.

## 2. Expérience neuve, bornée et reproductible localement

Sources de la sonde : `probe.cpp`, `check.py`, `check_r1_interrupted.py`. La sonde compile directement les sources courantes, sans mutation du moteur, avec GCC C++20, `-O2 -Wall -Wextra -Wpedantic -Werror -pthread`.

- 24 nuages : extrêmes u18, cube, carré, triangle rectangle, octaèdre avec intrus pondéré, 18 petits tirages géométriques de graine `2026092905`.
- K = 1, 2, 3, 5, 10, 12 ; feuilles par défaut, `max(8,K+1)`, 256 ; W1/W4.
- 432 appels ; 15 195 enregistrements, dont 6 309 pondérés et 2 031 à coquille étendue.
- Contrôles : égalité du catalogue, I/U complets, poids intérieur/coquille, admission, niveau rationnel numérique, rang exact dense, S* minimal dans l'ordre de Morton, indicateurs de dégénérescence.
- Aucun écart. Pas de nouveau sanitizer ni de mesure de croissance.

Le juge énumère les supports et utilise le Gauss `Fraction` de la référence Python, indépendante des prédicats natifs. Les multiplicitées sont comptées séparément des supports en positions.

`receipt.json` contient les hashes. Capture locale originale : `/tmp/mhgp10-catalogue-audit-20260929.35hiJi/`, commandes complètes dans `commands_r2.json`. Les scripts copiés ici sont les sources réellement exécutées ; leurs chemins absolus désignent cette capture locale, à adapter pour un rejeu sur une autre machine. Ce reçu d'audit n'est pas une qualification produit autonome transférable.

### Première tentative conservée

La première matrice utilisait `leaf_size=2`. Elle a été interrompue (code 130) pendant le cube extrême ; elle ne compte pas comme campagne close. Le script est conservé sous `check_r1_interrupted.py`. R2 remplace explicitement cette configuration, ne masque pas l'échec et ne reprend aucun résultat partiel.

Reproduction bornée (`leaf_stress.py`, `leaf_stress.json`) : huit coins de `[0,262143]^3`, K5. Le défaut termine en environ 0,008 s ; M2 dépasse 2 s, puis le fils est tué et joint. K1/M2 termine en environ 0,017 s. Une boîte peut conserver au moins K sites, donc demander une feuille plus petite que K force une subdivision qui ne peut satisfaire la taille visée. La stagnation sous le pas de grille n'empêche pas cette explosion en amont. Ne pas interpréter l'option `--leaf` comme un paramètre de coût toujours sûr ; aucune perte de boule n'est déduite de ce témoin interrompu.

## 3. Points à corriger ou compléter

### P1 — Lecture binaire tronquée acceptée

`cli/mhgp10_catalogue.cpp:40` arrête la lecture dès que `fread(...,4,3)` rend moins de trois mots, sans distinguer EOF propre, dernier point incomplet et erreur d'E/S. Le fichier de deux points suivi de 1, 4, 8 ou 11 octets est accepté avec `status=ok`, code 0, n=2. Reçu : `truncated_input.json`. Le même motif est présent dans la CLI tour.

Action : vérifier les octets restants et `ferror`, refuser avant calcul toute taille qui n'est pas un multiple de 12 ; ajouter les quatre refus de cette fixture. Ce défaut d'entrée ne contredit pas la géométrie des points effectivement lus, mais contredit une promesse de traiter le fichier complet.

### P1 différé — Débordement des indices de boules

`generator.cpp:767` parcourt les enregistrements locaux avec un u32 ; `generator.cpp:803` convertit `refs.size()` en u32 sans garde. Dès 2^32 boules au total, la taille publiée est tronquée ; dès cette limite dans un worker, la boucle peut reboucler. L'entrée limite le nombre de sites, pas le nombre de boules. La barrière mémoire actuelle arrive avant sur les campagnes publiées : ce n'est pas un échec observé sur LiDAR, mais il faut un refus `index_overflow_u32` avant assemblage. Vérifiable par un test de cardinal artificiel sans allouer des milliards de boules.

### P2 — Porte catalogue partielle

`tests/oracle/test_catalogue_oracle.py:54–80` valide l'ensemble `(q,p,I,U)`, la validité d'un support et les comparaisons de rangs. Elle ne demande pas S* minimal, ne lit pas les valeurs `cat.level` du dump et ne vérifie pas le rang dense à partir de zéro. Elle ne traite pas les entrées pondérées. Les reçus J2c ajoutent des différentiels de niveaux contre l'ancien binaire ; cela prouve la non-régression, pas indépendamment la valeur exacte.

La campagne §2 ferme ces angles morts pour ses seuls petits cas. Porter ces vérifications et les fixtures dans les portes permanentes avant de revendiquer une certification complète.

### P3 — Formulation de stagnation trop forte

Dans `RECU_AGENT_J2C.md:171–174`, neuf coupes binaires ne divisent pas nécessairement chacun des trois côtés par huit : un pavé anisotrope peut ne couper que son côté long, et les longueurs impaires ne donnent pas exactement un facteur deux. La règle reste un choix de coût sûr pour l'exactitude ; l'équivalence géométrique exacte avec trois niveaux d'octree doit être retirée. La terminaison se démontre avec la somme des plafonds des logarithmes des côtés, pas avec un volume supposé divisé exactement par 512.

## 4. Complément à la nouvelle priorité de projection

Le témoin `cover_discontinuity.json` porte six exécutions natives K2, trois points `(0,0,0)`, `(m,0,0)`, `(2000,0,0)` pour m=999,1000,1001, entrée `cover` et `core`. Binaire J2c hashé ; sources tour/CLI comparées identiques au code examiné.

- Pour m=999, cover fusionne gauche/milieu au rayon 499,5 et milieu/droite à 1000.
- Pour m=1001, ces deux valeurs sont inversées. Un déplacement de 2 fait donc sauter une hauteur de fusion de 500,5.
- Pour m=1000, le point du milieu est départagé vers la gauche.
- Core donne respectivement les rayons 999 et 1001, inversés après perturbation : pas de saut macroscopique.

Ce témoin vise la projection exclusive par première couverture, pas la stabilité de la multicouverture continue. Les sources et dumps complets sont joints. Le coordinateur traite la conséquence mathématique et le choix de projection dans un audit séparé.
