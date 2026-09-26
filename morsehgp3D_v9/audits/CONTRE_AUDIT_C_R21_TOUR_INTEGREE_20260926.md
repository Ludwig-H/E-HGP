# Contre-lecture C de R21 et revue de la tour intégrée

26 septembre 2026, auditeur C.

- **Base :** `273c33f7c`, reçu [`g4_tower_r21_20260925`](../receipts/g4_tower_r21_20260925/README.md), paquet `d054c1c5`.
- **Cadre :** `exploration_v9_hors_registre`, `public_status=not_claimed`.
- **GCP :** non utilisé.
- **Méthode :** sept pistes indépendantes, puis chaque constat de revue
  soumis à un ou deux sceptiques chargés de le réfuter (26 agents au
  total). La note elle-même a été recalculée par deux vérificateurs, qui
  ont contrôlé chaque chiffre et chaque affirmation. Les pistes :
  - le reçu ;
  - l'attribution du temps avec sol ;
  - quatre revues de code ;
  - ThreadSanitizer.
- **Annexe :** scripts, sorties structurées et journaux TSan dans
  [`c_r21_20260926/`](c_r21_20260926/README.md).

## Verdict

- **Le reçu R21 tient.**
  - Intégrité : `SHA256SUMS` passe à 534/534.
  - Les 34 cas sont `complete_relative`.
  - Les **douze épingles** sont reproduites par chacun des cas de leur
    trame et de leur K.
  - Les 22 comparaisons, recalculées depuis les sorties brutes, sont
    égales. `tower_work` est identique entre le témoin et la tour
    intégrée.
  - Le mur externe tient sur les 34 cas, avec une marge de 0,076 à
    0,46 s.
  - `TERMINATED` est certifié par l'arrêt gardé.

  Les écarts relevés (§ 1) sont des arrondis, deux affirmations non
  étayées (Euler, reste de 08/000200) et une relecture de l'arrêt sans
  pièce jointe.
- **La tour intégrée n'a aucun défaut d'objet, de course ni de durée de
  vie trouvé.** Cela couvre le regroupement haché, le pool E2, la queue
  E4 et l'intégration par options nommées.
  - 17 constats ont été soumis aux sceptiques : 13 sont confirmés, 1
    reste plausible et 3 sont réfutés.
  - Les 13 constats confirmés sont tous de gravité basse (§ 5) :
    - onze touchent les portes, le lecteur ou la documentation ;
    - un est un défaut d'API (13) ;
    - un est un écart de **statut** du code produit sous double panne (2).
  - Plusieurs prolongent des constats antérieurs de B.
  - ThreadSanitizer ne donne **aucun rapport** sur la chaîne intégrée
    (coupe LiDAR 8k, K5, W4, trois leviers actifs), dont les condensés
    sont égaux à ceux de Release, ni sur les portes exécutées.
  - La couverture TSan reste partielle, car l'hôte était saturé.
- **Le temps avec sol.**
  - La chaîne double : ×2,22 à K5 et ×2,14 à K10. Elle ne triple pas :
    elle suit le catalogue (×2,28 et ×2,21), pas le nombre de sites
    (×3,11 en moyenne par trame).
  - À K5, la fenêtre de la tour est bornée par la **phase A de l'ordre
    du haut**, pas par celle des ordres bas.
  - Le plan annoncé mène le brut K5 vers 1,8 à 1,9 s (projection).
    Atteindre 1 s demande trois leviers absents du plan (§ 3).

## 1. Reçu : ce qui est à corriger

Les autres chiffres du README et des deux entrées du canal du
25 septembre (07 h 10 et 07 h 40) sont conformes à `SUMMARY.json`.
- **Conformes, notamment :**
  - les 42 temps de chaîne du tableau (34 cas de R21 et 8 de R20) ;
  - le tableau de la tour ;
  - la session d'appareil (122 à 172 ms) ;
  - les paires froid/chaud (36 à 102 ms) ;
  - le RSS de 8,9 Gio et les reports 22/57/8.
- **Non vérifiables depuis le reçu :** les comptes locaux (28/28,
  286/290, 41/41, 58 et 108).

| affirmation | constat |
| --- | --- |
| canal 07 h 40 : « K10 : 3,17 » à 08/000000 | 3 164,6 ms, donc 3,16 (double arrondi) ; la répétition fait 3,212 |
| README : « q34 0,55 s » à 08/000200/K5 | 544,5 ms, donc 0,54 |
| « tour : −120 ms à K5 » | −107 à −124 ms selon la paire (moyenne −116) ; à K10, −620 à −663 (et non −664) |
| brut : « 2,0 à 2,4 fois pour 3,1 fois plus de sites » | 3,1 est le rapport global ; par trame, les sites sont multipliés par 3,09 / 3,50 / 2,74 et la chaîne par 2,02–2,16 / 2,34–2,38 / 2,02–2,13 (K5 et K10) |
| « Euler holds partout » | vrai, mais contrôlé seulement jusqu'à K−2 (ordres 1 à 3 à K5, 1 à 8 à K10), ce que le README ne dit pas |
| reste de 08/000200/K5 : tour 0,30, q34 0,55, recensement 0,10 | la liste omet 73 ms (fusion 33, `gen_index` 14, `tower_index` 11, préparation 1, reste 14) ; dans q34, elle omet 87 ms de glu hôte non chronométrée et 4 ms de queue |
| « la fenêtre est bornée par la phase A des ordres bas » | inexact à K5 : c'est l'ordre du haut (§ 2.3) |
| « TERMINATED relu en lecture seule (dernier arrêt 00:11:07) » | aucune pièce du reçu ne contient cette relecture ; l'arrêt est certifié par `guarded_stop.redacted.stdout` et `targeted_shutdown_certified=true` ; `lifecycle.txt` reste à `targeted_running` (fichier de démarrage) |

## 2. Où va le temps avec sol

Toutes les valeurs viennent des sorties `vm/probe_N.stdout`, identiques
aux lignes de `SUMMARY.json`. Script : `c_r21_20260926/attribution/extract.py`.

- **Bras GPU :**
  - sans sol : 00/01/02 = `probe_0/4/8` à K5 et `probe_2/6/10` à K10 ;
  - brut : b00/b01/b02 = `probe_22/26/30` et `probe_24/28/32`.
- **Précision :** les cas bruts n'ont tourné qu'une fois. Un écart de
  rapport inférieur à 3 % entre phases relève du bruit (la répétition de
  00 varie de 2,7 % à K5).

### 2.1 K5, bras GPU (ms)

| phase | 00 | 01 | 02 | b00 | b01 | b02 | brut / sans sol (moyenne) |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| chaîne | 972 | 805 | 1 026 | 2 094 | 1 914 | 2 180 | 2,22 |
| q3/q4 total | 509 | 404 | 545 | 1 064 | 911 | 1 059 | 2,10 |
| · front (CPU) | 101 | 74 | 96 | 285 | 237 | 282 | **2,99** |
| · appels filtre / certificats / voies | 95 / 118 / 109 | 66 / 98 / 95 | 98 / 131 / 127 | 148 / 209 / 242 | 135 / 177 / 194 | 162 / 201 / 230 | 1,75 / 1,70 / 2,02 |
| · hôte dans les appels + glu non chronométrée | 72 + 82 | 67 + 68 | 79 + 87 | 137 + 172 | 131 + 161 | 138 + 176 | 1,86 / 2,16 |
| recensement | 99 | 83 | 104 | 214 | 207 | 232 | 2,29 |
| fusion + index | 39 | 42 | 45 | 84 | 89 | 97 | — |
| tour | 300 | 251 | 304 | 661 | 638 | 714 | 2,36 |
| · validation | 50 | 42 | 51 | 96 | 94 | 107 | 2,08 |
| · phase 0 | 83 | 68 | 78 | 164 | 155 | 164 | 2,12 |
| · A(5), fil unique | 149 | 125 | 157 | 340 | 331 | 367 | 2,43 |
| · populations / images / encodage | 11 / 20 / 31 | 10 / 13 / 29 | 12 / 18 / 29 | 24 / 45 / 77 | 22 / 47 / 69 | 24 / 57 / 81 | 2,19 / 3,03 / 2,53 |

q2 est entièrement caché : `q2_wait` vaut 0 dans les douze cas GPU.

### 2.2 Chemin critique du brut K5 (moyenne des trois trames : 2 063 ms)

| segment | ms | part | exécution |
| --- | ---: | ---: | --- |
| temps d'appareil des trois appels (noyaux 357, transferts 74) | 431 | 20,9 % | GPU ; les 48 fils CPU attendent, hors q2 |
| fenêtre de la tour : static(5) + A(5) | 415 | 20,1 % | A(5) sur un seul fil |
| front q3/q4 | 268 | 13,0 % | 48 fils, borné par une tâche traînarde |
| recensement | 218 | 10,6 % | 48 fils |
| glu hôte non chronométrée de q3/q4 | 169 | 8,2 % | séquentielle, fil principal |
| queue de la tour (populations, images, banque, encodage) | 151 | 7,3 % | coureurs et aides |
| hôte dans les appels | 135 | 6,6 % | boucles surtout séquentielles |
| validation de la tour | 99 | 4,8 % | — |
| fusion, index, préparation, reste | 176 | 8,6 % | en partie séquentiel |

Ce qu'on en tire :

- **Glu autour de l'appareil.** Ce qui n'est pas un noyau (glu 169 +
  hôte dans les appels 135 + transferts 74 = environ 378 ms) coûte plus
  que les noyaux (357 ms).
- **Appareil inactif.** L'appareil, noyaux et transferts compris, est
  occupé 20 à 22 % de la chaîne brute. Les 48 fils CPU restent inactifs
  366 à 433 ms pendant les appels.
- **Front.** Sa croissance ×3 est un effet d'ordonnancement, pas du
  travail en plus : la somme CPU de ses tâches ne croît que de ×2,28. Une
  seule tâche prend 218 / 172 / 192 ms, alors que la somme divisée par
  48 donnerait 150 / 137 / 157 ms (`q34_occupancy.max_job_ms`).
- **Accélération GPU.** Le GPU aide moins sur le brut. L'accélération de
  la chaîne (moteur contre GPU) vaut 2,47 à 2,69 au lieu de 2,64 à 2,92.
  Les appels GPU croissent le moins : ×1,70 à ×2,02 en moyenne à K5,
  1,53 à 2,22 par trame. Le travail ajouté par le sol tombe là où le GPU
  est déjà.
- **Capacités.** Elles sont atteintes sans conséquence visible :
  - 22 et 57 certificats reportés à b00, 8 voies à b02/K10 ;
  - le tampon d'événements q4 monte à 4 091 sur 4 096 à b02/K10 ;
  - le rattrapage CPU reste caché sous les voies ;
  - aucun refus d'arène.
- **Diagnostics manquants.** R21 ne publie toujours pas ceux que
  demandait le § 4 du grand audit : tailles maximales de cœur et de
  cover, histogrammes, reports par cause, occupation de l'arène, pic de
  mémoire de l'appareil.

### 2.3 Quelle phase A borne la tour

La phase 0 traite les ordres **du haut vers le bas** : à K5, dans
l'ordre K5, K4, K3, K2. A(K) commence dès que la phase 0 de son ordre
est prête. La fin de la fenêtre est donc `max_K (prêt(K) + A(K))`. Cela
se vérifie à 0,21 ms près à K5 et à 0,8 ms près à K10.

| cas | fin de A(1) à A(K) (ms, depuis l'ouverture de la fenêtre) | fenêtre | ordre qui borne |
| --- | --- | ---: | --- |
| 00 K5 | 78, 133, 150, 167, **186** | 186,2 | K5 |
| 02 K5 | 81, 135, 154, 174, **191** | 191,1 | K5 |
| b00 K5 | 189, 282, 342, 372, **412** | 412,4 | K5 |
| b02 K5 | 188, 295, 347, 390, **436** | 435,8 | K5 |
| 00 K10 | 99, 814, 832, 836, 857, 866, **903**, 878, 852, 789 | 903,9 | K7 |
| b00 K10 | 301, 1 406, 1 472, 1 505, 1 565, 1 625, **1 648**, 1 641, 1 636, 1 627 | 1 647,9 | K7 |

Conséquences :

- **À K5**, c'est A(5), sur un seul fil, qui borne la tour : 125 à
  157 ms sans sol, 331 à 367 ms avec sol. Un levier sur la phase A des
  « ordres bas » ne rapporte **rien** à K5.
- **E6** (résolution de la phase 0 sur l'appareil) ne peut cacher que la
  partie résolution de static(5) : 19 à 24 ms sans sol, 41 à 46 ms avec
  sol. static(5) entier (30 à 37 ms, 66 à 73 ms) en est l'enveloppe.
- **À K10**, K6 à K10 finissent dans une plage de 23 à 114 ms. Le poste
  dominant est la phase 0, séquentielle d'un ordre à l'autre : 737 ms
  sans sol à 00, 1 260 ms avec sol à b00. Dès que la phase 0 accélère,
  les planchers A(K) apparaissent : A(10) vaut 580 ms à 00 et 1 292 ms à
  b00.
- **Populations à K10 brut.** Elles croissent de ×4,5 : 203 / 109 /
  141 ms, contre 30 / 34 / 39 ms sans sol. Les pas des ordres 7 à 9
  varient beaucoup d'une trame à l'autre. On suppose une contention
  (912 fils d'aide), mais elle n'est pas mesurée.

## 3. Ce qu'il faudrait pour 1 s à K5 avec sol (projection)

Il faut retirer environ 1,06 s en moyenne et 1,18 s à b02. Aucune phase
ne dépasse 21 % de la chaîne : toutes doivent baisser en même temps.

| poste (b02) | aujourd'hui | cible | levier | dans le plan ? |
| --- | ---: | ---: | --- | --- |
| fenêtre de la tour, static(5) + A(5) | 436 | ≈ 170 | A(Kmax) allégée puis parallèle (lemme max-ID, Borůvka), **preuve d'abord** | non (E6 : au plus −41 à −46) |
| glu hôte de q3/q4 | 314 | ≈ 50 | contrôles et conversions en parallèle, recouverts par l'appel suivant | non |
| front q3/q4 | 282 | ≈ 110 | scinder la tâche traînarde, puis front sur l'appareil | non |
| recensement | 232 | ≈ 60 exposés | clés q2 dans la fenêtre de l'appareil (43 à 46 % des clés), puis voies fenêtrées | q2 seulement |
| appels d'appareil | 454 | ≈ 300 | D2H épinglé (−20 à −40), noyaux filtre et certificats, appels en pipeline | D2H seulement |
| queue de la tour | 165 | ≈ 80 | E5 (images et encodage scindés) | non |
| validation | 107 | ≈ 60 | R-29 (passe 1, 33 ms) et tri radix des niveaux | R-29 |
| fusion, index, préparation, désallocation | 190 | ≈ 100 | `tower_index` pendant l'appareil, `gen_index` parallèle, désallocation différée | en partie (`tower_index` avec le recensement q2) |

**Ce que rapporte le plan annoncé, sur le brut K5 :**

| étape | gain | remarque |
| --- | --- | --- |
| recensement q2 pendant l'appareil | −95 à −130 ms | seule étape dont le poids grandit avec le sol |
| E6 | −41 à −46 ms | résolution de static(5) |
| D2H épinglé | −20 à −40 ms | mais −100 à −150 ms au brut K10 |
| R-29 | environ −30 ms | — |
| phase A des ordres bas | 0 ms | voir § 2.3 |

Au total, environ −180 à −246 ms, soit 1,8 à 1,9 s. Trois leviers
manquent au plan : la phase A de l'ordre du haut, la glu hôte et le
front. Ce budget ne suppose **pas** de tour sur l'appareil.

## 4. Potentiel d'une tour sur l'appareil (projection)

Réponse à une question de l'utilisateur, recalée sur les phases
mesurées de R21.

Hypothèses, plutôt favorables :
- la validation devient quasi nulle ;
- la phase 0 est divisée par 3 à 6 ;
- la queue passe à 25–35 % de son temps, rapatriement épinglé de la
  sortie explicite compris ;
- la phase A reste sur CPU, ou elle est divisée par 3 à 5 si elle est
  parallélisée.

La fenêtre suit le modèle du § 2.3 : `max_K (prêt(K) + A(K))`.

| cas | tour R21 | chaîne R21 | chaîne, tour GPU et A sur CPU | chaîne, tour GPU et A parallèle |
| --- | ---: | ---: | ---: | ---: |
| K5 sans sol | 0,25–0,30 s | 0,81–1,03 s | 0,70–0,92 s | 0,60–0,81 s |
| K10 sans sol | 0,97–1,27 s | 2,45–3,16 s | 1,98–2,63 s | 1,65–2,32 s |
| K5 avec sol | 0,64–0,71 s | 1,91–2,18 s | 1,66–1,92 s | 1,40–1,68 s |
| K10 avec sol | 2,28–2,57 s | 5,71–6,49 s | 4,70–5,53 s | 3,84–4,68 s |

Lecture :

- **Phase A sur CPU.** Si elle y reste, la tour sur l'appareil ne
  rapporte qu'environ :
  - −0,1 s à K5 sans sol ;
  - −0,5 s à K10 sans sol ;
  - −0,25 s à K5 avec sol ;
  - −1,0 s à K10 avec sol.

  Les planchers A(K) apparaissent dès que la phase 0 accélère.
- **Phase A parallèle.** Elle ajoute −0,1 à −0,25 s à K5 et −0,3 à
  −0,85 s à K10. Le gros du gain à K10 en dépend.
- **Avec sol.** La tour sur l'appareil n'est **pas suffisante** pour
  1 s : la chaîne hors tour prend déjà 1,28 à 1,47 s. Sa nécessité
  n'est pas établie non plus : le budget du § 3 s'en passe, au prix
  d'une phase A de l'ordre du haut parallèle.
- **Sans porter toute la tour.** R-29 et E6 captent une part du gain,
  avec un risque bien moindre.
- **Vérification.** Toute version sur l'appareil devra reproduire les
  douze épingles.

## 5. Revue de la tour intégrée

### 5.1 Ce qui a été vérifié

**Regroupement haché.** Chaque correspondance d'étiquette est confirmée
par la clé entière, pour les classes comme pour les graines
(`full_ball_tower.hpp:2297`, `2210`, `2229`). Le sondage linéaire ne
remplit un emplacement qu'une fois, et seul le min-CAS de la même classe
le réécrit ensuite : une clé a exactement un emplacement. Les premiers
représentants et les cibles ne dépendent que de la clé et du niveau.

Sur 17,1 M requêtes de la trame à K5, `tag_rejects` vaut 0.

**Pool E2.** Les champs d'un travail sont publiés avant la génération ;
l'entrée se fait par CAS gardé par `kOpen`. Un fil en retard ne peut pas
entrer dans un travail fermé, et aucun réveil n'est perdu. Des tests de
contrainte indépendants le confirment.

**Queue E4.** `population_offset` est calculé avant tout coureur.
L'image d'un ordre ne lit l'ordre inférieur qu'après publication de ses
lots. Les objets touchés vivent au-delà de la jointure. IDs et condensés
ne dépendent pas du temps.

Un harnais d'audit a rejoué 12 114 pannes `Failure` simples et
appariées sur six chemins, avec la même raison partout.

**Lecteur R21.**
- `tower_detail` est exigé partout.
- Sur 64 combinaisons de leviers et de fils statiques, avec la vraie
  sonde sur le nuage de contrat de 360 sites (K5), le lecteur et la
  sonde concordent, avec le même condensé.
- La borne des IDs de support refuse bien `== n` (`wspd_q34.cpp:1311`).
- Les six épingles brutes sont identiques à `PINS_RAW.json`.

**ThreadSanitizer**, sur une construction CPU de `273c33f7c`. Un témoin
positif (une course plantée) est bien signalé, donc « 0 rapport » veut
dire « aucun rapport trouvé ».

| exécution | code | rapports | commentaire |
| --- | ---: | ---: | --- |
| chaîne, coupe LiDAR 8k (`cf848663…`), K5, W4, leviers par défaut | 0 | 0 | trois leviers pris ; condensés tour `3c9748474fb49e83`, catalogue `48404414ff089c6b`, présentation `b7b0bb0cffd2f459` = Release |
| porte du regroupement `--selftest` | 0 | 0 | 18 passages, dont 15 à plusieurs ouvriers ; `clusters_1500` = préflight R21 |
| pool `--unit`, `--priority` | 0 | 0 | — |
| pool `--fixtures` | arrêtée | 0 | arrêtée après 688 s pour libérer l'hôte ; déjà passée sous TSan par le développeur |
| banque des populations | 0 | 0 | — |
| `full_ball_tower_gate --pipelined-4` (piste E4) | 0 | 0 | références différées 14, ordres en pipeline 168 |
| harnais E4 : 576 déroulés sur panne, 51 + 260 pannes hors `Failure` | 0 | 0 | copie d'audit de l'en-tête, avec points d'échec |
| queue de la tour `--selftest` (8 combinaisons) | tuée | 0 | inachevée : hôte saturé (charge 30 à 80) |
| priorité d'échec `--selftest` | tuée | 0 | inachevée ; `--unwind` non lancé |
| `chain_static_paths`, trame entière 00 à K5, coupe 8k à K10 | — | — | non lancées |

Aucun rapport n'a été émis sur la coupe 8k, mais
`population_deferred_refs` y vaut 0 : les références différées
concurrentes ne sont pas exercées sur données réelles.

### 5.2 Constats confirmés (gravité basse)

Aucun ne change l'objet.
- **Onze** ferment des portes, durcissent le lecteur ou corrigent des
  commentaires.
- **Le 2** corrige un statut rapporté par le code produit sous double
  panne.
- **Le 13** est un défaut d'API.

| n° | constat | lieu | correction proposée |
| --- | --- | --- | --- |
| 1 | aucune porte ne force une collision sur les entrées 6 à 9 de la clé : le test à hachage faible ne dépasse pas K6, et une comparaison partielle (24 octets) survivrait | `static_grouping_gate.cpp:245` | fixture à hachage faible à K ≥ 7, ou mutant de comparaison partielle |
| 2 | `pipelined_tail` change le statut rapporté : une exception hors `Failure` (`bad_alloc` ou échec de lancement) passe devant une `Failure` d'ordre bas que le témoin rapporte. Elle peut venir de B ou C à un ordre plus haut, ou du dimensionnement des rows sur le coureur 1. Prolonge le point 2 de [la reprise E4](CONTRE_AUDIT_E4_TAIL_REPRISE_20260924.md) | `full_ball_tower.hpp:981` | relancer les erreurs hors `Failure` après la boucle des `Failure`, ou les ordonner comme le témoin |
| 3 | le chemin anti-blocage de la queue n'est pas testé : un mutant qui retire `publish(lots_state[0], 2)` après un échec des rows bloque et survit. Prolonge la ligne 19 de la même reprise | `full_ball_tower.hpp:890` | point d'échec hors `Failure` dans `prepare_population_rows`, avec délai de garde |
| 4 | commentaire faux, antérieur à E4 : la priorité « échec de phase 0 d'abord » n'est pas celle de la boucle séquentielle, et la porte ne compare jamais la phase 0 à un fil statique | `full_ball_tower.hpp:798` | corriger le commentaire, puis ajouter le scénario {static K4, lots K2} à un fil |
| 5 | `pipelined_orders` est déclaratif (kmax à chaque sortie) mais sert de plancher ; les fils d'aide E4 ne sont comptés nulle part. **Constat de B** ([queue E4](CONTRE_AUDIT_B_QUEUE_E4_WIP_20260924.md)), resté sans suite. Les planchers sont étiquetés « chemin pris » : aucun verdict de porte n'est faux | `full_ball_tower.hpp:825` | compter les ordres dont C a réellement tourné ; compter les fils d'aide |
| 6 | le commentaire de `same_work` annonce les compteurs du curseur vertical, qui ne sont pas comparés | `tower_tail_gate.cpp:45` | comparer les `lower_*`, ou corriger le commentaire |
| 7 | contrat de `run_threads` : `fn(t)` n'est pas appelé pour tout `t` sur le chemin du pool, et rien n'impose un seul soumetteur | `pool.hpp:324` | documenter « au plus » ; vérifier le propriétaire (`owned_by_this_thread`) |
| 8 | la fixture de frontière de la borne d'ID (`n+7`), réponse du développeur au [préflight R21 de B](CONTRE_AUDIT_B_PREFLIGHT_R21_V26_20260924.md), laisse survivre un `>` au lieu de `>=` | `lanes_port_gate.cpp:1082` | fixture de support égal à `n` |
| 9 | `PINNED_DIGESTS` n'est indexé que par `s=8`, alors que le plan admet `s=10/12` : aucun cas à `s≠8` ne serait épinglé | `tower_worker_v9.py:1136` | indexer par (trame, K), l'objet ne dépendant pas de `s`, et refuser un cas sans épingle |
| 10 | les minuteries de `tower_detail` ne sont pas bornées (`pool_ms` = 10^6 accepté) ; `pool_ms` manque dans la borne de somme des phases | `tower_worker_v9.py:843` | borner par `tower_ms` ; ajouter `pool_ms` à la somme |
| 11 | les compteurs de chemin sont jugés trop lâchement : un repli partiel du hachage (`hashed_orders` < K−1), `runner_threads=0` et des références différées énormes sont acceptés | `tower_worker_v9.py:859` | égalité `hashed_orders = K−1` sous le levier ; bornes sur les autres |
| 12 | le lecteur n'est testé qu'avec les trois leviers de la tour tous actifs ou tous coupés | `tower_selftest_v9.py:1111` (et 1082) | cas mixtes dans l'autotest |
| 13 | le défaut de la bibliothèque (`overlap_static=false`) diffère de celui de la chaîne (`true`) : `FullBallTowerOptions{}` prend un chemin hors produit où `pipelined_tail` est sans effet ; seul le commentaire de la ligne 151 le signale, et ce défaut est antérieur à l'intégration ; commentaire périmé à `tower_chain.hpp:113` | `full_ball_tower.hpp:150` | aligner le défaut ou nommer le préréglage produit |

**Plausible, informatif.** Le plancher de `chain_static_paths` porte
sur un compte d'ouvriers engagés qui dépend de l'ordonnancement, et
l'en-tête parle encore d'« ouvrier créé ». Un faux rouge est possible en
principe, mais n'a jamais été observé.

**Réfutés.**
- Doublon de graine : code défensif inatteignable après la validation.
- Réveil de tous les fils : la fermeture n'attend que les fils déjà
  entrés.
- Mutant `parallel-one-worker` : il n'est pas injecté, mais la
  conséquence annoncée est fausse. Il reste un point d'hygiène de preuve,
  déjà ouvert au point 8 de mon audit du 23 septembre.

## 6. Corrections et confirmations du grand audit C

**Corrections :**
- **q3/q4 de R20 à K10.** Il vaut 1 253 ms (1 047 chronométrés + 29 de
  queue + 176 hors chrono), et non 1 049 (§ 3).
- **Projection brute de R20 à K10.** Elle portait sur b00 seul, avec le
  code R20 : 6,9 à 8,3 s. Elle n'est pas contredite une fois rendu le
  gain de la tour intégrée : b00 fait 6,49 s en R21 avec la nouvelle
  tour, dont le gain est estimé à environ 1,3 s au brut.
- **Cibles d'ingénierie avec sol.** La cible brute à K10 (4,5 à 6,5 s)
  est atteinte de justesse. La cible brute à K5 (1,5 à 1,9 s) ne l'est
  pas : 1,91 à 2,18 s.
- **« 1 s à K5 avec sol demande la tour sur GPU ».** C'est faux dans
  les deux sens : la tour sur l'appareil n'est pas suffisante, et sa
  nécessité n'est pas établie (§ 3 et § 4).

**Confirmations :**
- Le plancher de la phase A de l'ordre du haut, 148 ms à K5, est
  retrouvé exactement : A(5) = 148,7 ms à 00. À K10, le plancher
  projeté (544 ms) est proche de la mesure (A(10) = 580 ms, +7 %), sans
  borner encore la fenêtre.
- Les gains des leviers de la tour tombent dans la fourchette projetée.
- Le front, le recensement, la fusion et le noyau du filtre n'ont pas
  bougé depuis R20.
- L'appareil est occupé 20 à 22 % de la chaîne au brut K5, noyaux
  seuls 16 à 19 %.
- 16 à 18 fils sur 48 sont actifs à K5.
- **Confirmation partielle :** tour, recensement et fusion devaient
  dépasser 1 s au brut K5. Ils prennent 0,93 / 0,90 / 1,01 s : le seuil
  n'est dépassé que sur une trame sur trois.

## 7. Recommandations

**Temps, avec sol d'abord :**

1. **A(Kmax).** C'est le levier de la fenêtre de la tour à K5. D'abord
   l'allègement, puis la parallélisation une fois la preuve enregistrée.
   Retirer « ordres bas » de la formulation du plan.
2. **Glu hôte de q3/q4**, environ 300 ms au brut K5 : contrôles et
   conversions en parallèle, recouverts par l'appel suivant.
3. **Front** : scinder la tâche traînarde, puis mesurer un front sur
   l'appareil.
4. **Recensement q2 pendant l'appareil**, annoncé comme deuxième étape
   du plan : c'est la bonne priorité pour le brut.
5. **D2H épinglé**, avec son ablation. Le gain est surtout à K10 :
   −100 à −150 ms au brut.
6. **Populations au brut K10** : mesurer la contention entre fils
   d'aide avant tout levier.
7. **Diagnostics du § 4 du grand audit** : les publier dans R22.

**Portes.** Les constats 2, 3, 8 et 9 d'abord :
- le 2 touche le contrat « mêmes statuts » ;
- le 3 transformerait une régression en blocage ;
- le 8 est une frontière de mémoire ;
- le 9 laisserait un cas non épinglé.

Viennent ensuite 1, 10, 11 et 12, puis les commentaires. Enfin, TSan
doit être passé sur la porte des huit combinaisons de la queue et sur la
priorité d'échec (`--unwind` compris), à un moment où l'hôte est calme.
