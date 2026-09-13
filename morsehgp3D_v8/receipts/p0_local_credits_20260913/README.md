# Première brique P0 : résultats mono et limites

13 septembre 2026. `exploration_v8_hors_registre / cpu_reference /
quantized_u16_input_only / implementation_v8_p0 / not_claimed`.

**Une alternative sans histogramme quadratique systématique est implémentée
et testée. P0 n'est pas encore résolue : le résidu peut rester quadratique.**
Ces mesures portent sur un seul rectangle déjà séparé, sans expansion des
paires ni consommateur géométrique aval. Elles ne qualifient ni WSPD complète,
ni q3/q4 complets, ni tour FULL, ni contrat 50k/1s ou massif G4.

## Ce qui est livré

Une bibliothèque C++20 neuve, une propriété immuable des entrées validées,
des prédicats stricts entiers, trois méthodes de crédits — Pool, DualBlocks,
Tubes — et des sous-produits de classes représentant le résidu sans le
développer. Le [contrat](../../docs/P0_CREDITS_LOCAUX.md) distingue minorants,
comptes universels-boîte saturés et futur census exact. Le
[code de la sonde](../../bench/p0_probe.cpp) et sa
[description](../../bench/P0_PROBE.md) fixent les entrées et chronomètres.

Tubes réalise en C++ la
[proposition de l'auditeur](../../audits/P0_TUBES_ET_RANGS.md), requalifiée
par des juges indépendants. Les rails reprennent explicitement la
[contre-fixture complémentaire](../../../audits/morsehgp3D_v8_complementaire/P0_RAILS.md).
Aucun résultat ni moteur v7 n'est importé. Le tri des tubes est encore
refait par voie ; la validation est encore locale à un rectangle.

## Protocole et preuves

Machine locale exposant 8 CPU logiques, AMD EPYC 9V74 ; produit mono-thread
compilé par GCC 13.3, Release, C++20, avertissements stricts. Processus de
mesure successifs, sans échauffement caché, sans build lancé simultanément
par le constructeur. L'hôte n'est pas isolé des autres activités : ce sont
des mesures exploratoires, pas une garantie de latence de service.
La mémoire maximale et le coût aval ne sont pas mesurés dans cette campagne.

| Campagne | Domaine | Répétitions | Mesures |
| --- | --- | ---: | ---: |
| [mono_k10_s8](mono_k10_s8/MANIFEST.json) | n8k/16k/32k, grid/sheet/skew, trois voies et trois méthodes, Kmax10/s8 | 3 | 243 |
| [mono_k5_s8_10_12](mono_k5_s8_10_12/MANIFEST.json) | Même domaine, Kmax5, s8/10/12 | 1 | 243 |
| [mono_k10_s10_12](mono_k10_s10_12/MANIFEST.json) | Même domaine, Kmax10, s10/12 | 1 | 162 |
| [rails_k10_s8_10_12](rails_k10_s8_10_12/MANIFEST.json) | Rails n2718, Kmax10, s8/10/12, trois voies et trois méthodes | 3 | 81 |

Total : **729 mesures, 513 configurations distinctes**. Chaque sous-dossier
contient MANIFEST.json, MEASURES.jsonl et COMPLETION.json : sources et
exécutable hachés, commit de base et worktree déclaré, paramètres, commande,
compilateur, machine, entrée déterministe, chronomètres et compteurs.
Les sources sont inchangées pendant les captures. Les répétitions donnent
les mêmes compteurs et résidus ; les trois valeurs de s aussi, car **s
ne change pas les rectangles de cette sonde**. Ce n'est pas une comparaison
du nombre ou du coût de rectangles d'une WSPD effective.

Les huit CTests passent en [Release GCC](RELEASE_TESTS.xml) et en
[Debug Clang 18.1 avec ASan/UBSan](SANITIZER_TESTS.xml). Le premier juge
géométrique donne 148 587 contrôles, 390 plans, 32 rejets d'entrées et
trois contre-modèles de frontières. Le juge tubes donne 108 730 contrôles,
295 petits plans, 135 comparaisons de permutations et cinq contre-modèles.
Il conserve aussi les témoins réellement manqués et le repli de séparation.
Les grands rails sont jugés par formule ; l'exhaustif multiprécision y
reste limité aux petites instances. Aucun oracle n'est le chemin produit.
Les portes Python documentaire et CLI passent aussi avec `python3 -O`.
La gate des reçus exerce 41 scénarios en normal et −O, dont six tuples
falsifiés, rejeu, faux claims, compteurs, JSON tronqué, 1e999, octets UTF-8
invalides, changement de binaire et annulation. Ce n'est pas un oracle
géométrique supplémentaire : elle vérifie l'intégrité du protocole mesuré.

**Corrections avant livraison.** L'auditeur a montré qu'une copie mutable
du propriétaire pouvait être réaffectée derrière un plan, perdant trois
paires q2. Les quatre opérations de copie/déplacement/affectation sont
maintenant interdites. Un second canal passait par un pointeur mutable
conservé vers le tampon déplacé de l'entrée : la factory copie désormais
les coordonnées dans un stockage privé avant validation. Le coût de cette
copie est inclus dans la préparation r3. Le runner initial acceptait un JSON sans vérifier
son tuple et perdait les sorties tronquées : les reçus v2 ferment ces
défauts et épinglent aussi le binaire à la fin. La
[première passe](first_pass_pre_owner_fix/README.md) est conservée intacte,
et la [deuxième passe](second_pass_pre_alias_fix/README.md) aussi,
mais seules les nouvelles captures r3 des quatre campagnes font autorité.

Deux essais intermédiaires sont aussi conservés :
[modification du runner pendant une gate](RELEASE_RUNNER_EDIT_RACE.xml)
détectée par son contrôle de hash, et
[type de cache Clang non reconnu](SANITIZER_CACHE_TYPE_FAILURE.xml).
Le parseur accepte désormais FILEPATH comme UNINITIALIZED pour le
compilateur CMake ; ce cas a un test permanent. Le
[passage GCC avant ce dernier correctif](RELEASE_BEFORE_CACHE_TYPE_FIX.xml)
reste distinct des deux qualifications finales. Aucun défaut géométrique
supplémentaire n'est déduit de ces échecs de la chaîne de preuves.

Boost 1.83 est une dépendance de test en headers uniquement, déjà disponible
dans une extraction locale. La première configuration sans chemin Boost a
échoué ; les headers tiers ont ensuite été déclarés SYSTEM pour ne pas
appliquer les avertissements du projet à leurs extensions flottantes.
Un avertissement de copie dans un test a été corrigé. Les avertissements
stricts restent actifs sur tous les sources v8. Aucun package installé,
aucune VM créée ou démarrée.

## Comparaison à n32k

Kmax10, s8, médianes de trois exécutions. Chaque case donne **total du
composant en ms / nombre de paires restantes**. Ce total inclut génération
et hash, validation du nuage et construction du plan, pas l'aval absent.

| Géométrie | Voie | Pool | DualBlocks | Tubes |
| --- | --- | --- | --- | --- |
| Grilles équilibrées | q2 | 5,755 / 378 840 | 42,975 / 378 840 | 7,383 / 1 623 392 |
| Grilles équilibrées | q3 | 16,467 / 43 533 400 | 180,772 / 497 032 | 7,413 / 9 339 984 |
| Grilles équilibrées | q4 | 13,440 / 64 657 505 | 195,777 / 1 137 445 | 7,382 / 13 140 624 |
| Nappes parallèles | q2 | 5,484 / 256 000 000 | 32,625 / 256 000 000 | 6,332 / 256 000 000 |
| Nappes parallèles | q3 | 6,391 / 256 000 000 | 33,696 / 256 000 000 | 6,357 / 256 000 000 |
| Nappes parallèles | q4 | 6,132 / 256 000 000 | 33,691 / 256 000 000 | 6,451 / 256 000 000 |
| Grilles déséquilibrées 15:1 | q2 | 5,849 / 144 298 | 50,401 / 45 122 | 7,381 / 641 880 |
| Grilles déséquilibrées 15:1 | q3 | 15,789 / 10 448 447 | 239,023 / 205 896 | 7,577 / 3 615 214 |
| Grilles déséquilibrées 15:1 | q4 | 13,137 / 15 426 633 | 241,318 / 433 089 | 7,656 / 5 080 152 |

Les trois stratégies ne sont pas équivalentes en coût ou en résidu. q2
favorise souvent Pool ici. q3/q4 favorisent Tubes face à Pool, mais
DualBlocks réduit beaucoup plus le résidu, pour un coût de construction du plan
supérieur. Sur les nappes, toutes gardent **n²/4 paires**, n désignant
la taille totale : 16, 64, puis 256 millions. Le carré n'a pas disparu.

Le repli Kmax5 ne résout pas ce défaut de proposition : à n32k/grid/q4,
Pool garde 64 447 560 paires et Tubes 13 118 880, très proches de Kmax10.
Ces captures Kmax5 ont une répétition, pas trois médianes.

## Ce que montrent les trois tailles

Exemple grid/q4, Kmax10/s8, médianes de trois exécutions :

| Quantité | n8k | n16k | n32k |
| --- | ---: | ---: | ---: |
| Copie/validation/préparation du propriétaire, Tubes | 0,463 ms | 0,991 ms | 2,067 ms |
| Plan Tubes | 1,146 ms | 2,321 ms | 4,704 ms |
| Total composant Tubes | 1,761 ms | 3,619 ms | 7,382 ms |
| Tests du balayage Tubes | 13 024 | 27 200 | 56 756 |
| Comparaisons de tri Tubes | 120 267 | 266 825 | 546 260 |
| Paires restantes Tubes | 2 214 144 | 5 760 000 | 13 140 624 |
| Plan DualBlocks | 34,921 ms | 87,042 ms | 193,076 ms |
| Tâches DualBlocks | 183 030 | 453 650 | 1 016 700 |
| Paires restantes DualBlocks | 188 065 | 480 592 | 1 137 445 |

La construction du plan Tubes double approximativement à chaque doublement
de n sur cette série ; le résidu croît plus vite. Cela concorde avec le
tri O(m log m) et le balayage linéaire démontrés, sans prouver une borne
globale de pire cas. Les tâches de blocs sont irrégulières : grid/q2
en compte 73 964, 399 574, 495 712. Ne pas extrapoler un exposant universel
à partir de trois tailles ou additionner des compteurs d'unités différentes.

## Rails : pourquoi conserver plusieurs méthodes

q4, n2718, Kmax10/s8, médianes de trois exécutions :

| Méthode | Plan | Total composant | Paires restantes |
| --- | ---: | ---: | ---: |
| Pool | 0,452 ms | 0,629 ms | 1 846 881 |
| DualBlocks | 1,910 ms | 2,089 ms | 2 916 |
| Tubes | 0,177 ms | 0,355 ms | 2 916 |

Tubes retrouve ici le résidu de DualBlocks avec un composant environ
5,9 fois plus rapide. Ses 5 418 tests de balayage ne développent pas les
témoins. Le pool ne rejette aucune paire. Ce résultat discriminant ne
fait pas des rails une représentation des nuages usuels ni un contrat FULL.

## Décision de reprise

Ne pas adopter DualBlocks par défaut partout ; ne pas adopter non plus
Pool ou Tubes comme réponse universelle. Poursuivre la préparation partagée
des tubes entre voies, puis une réduction des sous-produits difficiles.
Les nappes exigent notamment un certificat plus local que l'universalité
sur toute la boîte opposée. Évaluer ce raffinement avec couverture et IDs
préservés, coût cumulé compté, puis un consommateur q2 exact minimal.
Les candidats ne sont pas encore des supports définitifs, et l'absence
de leur expansion dans cette sonde doit rester visible.

Le multi-CPU et le GPU viennent après ce choix mono et le raccord du
consommateur. GCP non utilisé ; aucune dépense de VM engagée. Tous les
contrats de tour 50k et de dizaines de millions restent ouverts.

## Relecture et reproduction

```bash
python3 -B morsehgp3D_v8/bench/check_p0_campaign.py morsehgp3D_v8/receipts/p0_local_credits_20260913
python3 -B -O morsehgp3D_v8/bench/check_p0_campaign.py morsehgp3D_v8/receipts/p0_local_credits_20260913
```

Ajouter `--summary` pour recalculer les médianes par configuration. Le
lecteur vérifie sources actuelles, matrice complète, déterminisme, comptes,
bornes des balayages, résidus des contre-fixtures et invariance à s dans
ce périmètre. S'il signale des sources modifiées lors d'une reprise, revenir
au commit de cette livraison pour relire ces reçus ; ne pas les réécrire.

Les commandes exactes des quatre campagnes sont dans leurs manifestes,
avec un nouveau répertoire de sortie requis pour chaque capture. Les
commandes de qualification sont consignées dans [QUALIFICATION](QUALIFICATION.json).
Les XML de tests ne sont pas des chronométrages de performance.

Un export neuf de l'index a également été recompilé :
[huit CTests réussis](INDEX_TESTS.xml), documentation PASS528 fichiers,
registre PASS20 phases et lecteur des campagnes sous −O PASS729 mesures.
Les fichiers v6/v7 en cours de travail et ceux des auditeurs indépendants
n'ont pas été ajoutés au commit constructeur. Les seuls ajouts après cet
export sont ces comptes rendus de vérification ; le code testé est inchangé.
