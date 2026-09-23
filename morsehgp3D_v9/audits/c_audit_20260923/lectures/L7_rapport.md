# Lentille 7 — Mesures, reçus et écart au contrat

Auditeur C, 23 septembre 2026. Cadre : `phase=exploration_v9_hors_registre`, `backend=reference_cpu`, `profile=quantized_u18_input_only`, `mode=audit_independant`, `public_status=not_claimed`. **GCP non utilisé.** Aucune compilation, aucune exécution HGP, aucune écriture dans un dépôt.

**Base lue.** Reçus G4, sonde et protocole dans le worktree `build/v9-audit-c-worktree` (0125dc18). `origin/main` a depuis avancé de 15 commits, jusqu'à `c02d45ac`. Les reçus `g4_tower_r*` y sont inchangés (vérifié par `git diff --stat 0125dc18 origin/main`). En revanche, le reçu de croissance complet `receipts/lidar_scaling_local_20260923/` (commit `1f73b40d`) n'existe que sur `origin/main`. Je l'ai lu par `git show origin/main:…`, en lecture seule, parce qu'il remplace le reçu partiel pour les lois d'échelle.

**Méthode.** Tous les chiffres ci-dessous sont recalculés par script à partir des sorties **brutes** : `vm/probe_N.stdout` (JSON de la sonde v1 à v11), `vm/probe_N.stderr` (GNU time -v) et `vm/probe_N.summary.json`. Ni les README ni les `SUMMARY.json` ne servent de source. Il y a 109 exécutions G4 : R1 8, R2 13, R3 14, R4b 13, R5 13, R6 24, R7b 24. Toutes sont `complete_relative`, mais R2 a été refusée par son validateur ; ses chiffres restent exploratoires. Scripts (scratchpad de l'agent `lentille7/`, valides sous `python3 -O`) :

- `table_g4.py` (sha256 `59242b56…`) ;
- `analyse.py` (`74ad456d…`) ;
- `scaling_laws.py` (`0779a131…`) ;
- enveloppe `lentille7_mesures_g4.py` (`d6b05bc9…`) ;
- sortie complète dans `all_out.txt`, lignes brutes dans `rows.json`.

Statut des affirmations : **mesuré** = recopié ou recalculé d'un reçu ; **calculé** = arithmétique sur ces mesures ; **supposé** = hypothèse signalée comme telle.

## 1. Chronologie R1 → R7b (mesuré)

Meilleur cas par session, configuration par défaut de chaque session, W48, en secondes. Le périmètre est l'**ancien** (condensé inclus) pour rester comparable : jusqu'à R6, le digest était dans `chain_total` ; depuis R7b, il est à part (`tower_chain.cpp:584-601`).

| Session | Paquet | 000100 K5 | 000000 K5 | 000200 K5 | 000100 K10 | 000000 K10 | 000200 K10 |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| R1 (tour mono) | e28296bb | 15,05 | 18,81 | 29,25 | 82,31 | 111,68 | 125,44 |
| R2 (refusé) | 0b29b6c3 | 10,45 | 13,23 | 19,73 | 37,14 | 49,63 | 64,16 |
| R3 | b4e480fc | 6,19 | 8,89 | 10,93 | 25,96 | 35,46 | 37,75 |
| R4b | a1d7a9bc | 5,44 | 7,39 | 9,18 | 19,40 | 26,69 | 29,50 |
| R5 | aae9da0e | 4,26 | 5,96 | 7,38 | 12,07 | 17,00 | 19,28 |
| R6 | 78ce9fd4 | 4,15 | 5,76 | 6,87 | 11,73 | 16,67 | 18,06 |
| R7b (+digest) | 8e8b83a3 | 3,83 | 5,75 | 6,65 | 10,38 | 14,91 | 16,28 |
| R7b hors digest | 8e8b83a3 | 3,67 | 5,56 | 6,44 | 9,58 | 13,91 | 15,28 |

De R1 à R7b, en sept heures, le gain est de ×3,3 à ×4,4 à K5 et de ×7,5 à ×7,9 à K10 (calculé, périmètre ancien). La tour K10 est passée de 55,7–75,9 s (mono) à 3,2–4,0 s. q3/q4 a été divisé par 2 à 3 par le certificat de voies mortes (R3). En revanche, **q2 et le recensus n'ont pas bougé depuis R1** (q2 0,23–0,85 s, census 0,08–0,48 s selon trame et K). De R5 à R7b, les gains sont faibles : −3 à −10 % à K5, −14 à −16 % à K10.

Le plancher CPU·s/48 est le temps d'une répartition parfaite du même travail sur les 48 fils logiques (calculé, minimum par session) :

| Session | 000100 K5 | 000000 K5 | 000200 K5 | 000100 K10 | 000000 K10 | 000200 K10 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| R1 | 9,15 | 11,36 | 20,00 | 25,78 | 34,65 | 58,08 |
| R3 | 2,42 | 3,44 | 3,96 | 7,28 | 10,36 | 11,34 |
| R6 | 1,89 | 2,34 | 2,55 | 5,81 | 7,67 | 7,99 |
| R7b (hors digest) | 1,81 | 2,41 | 2,58 | 5,58 | 7,30 | 7,74 |

## 2. Ventilation R7b par phase (mesuré, moyenne des deux répétitions MEB ON)

Secondes ; part dans `chain_total` entre parenthèses. « Autres » regroupe prepare, gen_index, tower_index et le résidu non attribué, sans le digest. « Hors chrono » = mur externe GNU time − (lecture + chaîne + digest).

| Trame, K | Chaîne | q2 | q3/q4 | Fusion | Census | Tour | Autres | Digest | Hors chrono | CPU·s | Fils occupés |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 000100 K5 | 3,741 | 0,231 (6,2 %) | 2,595 (69,4 %) | 0,035 | 0,082 | 0,731 (19,5 %) | 0,068 | 0,162 | 0,072 | 87,4 | 23,4 |
| 000000 K5 | 5,627 | 0,453 (8,0 %) | 4,066 (72,3 %) | 0,041 | 0,098 | 0,906 (16,1 %) | 0,064 | 0,191 | 0,060 | 115,8 | 20,6 |
| 000200 K5 | 6,505 | 0,499 (7,7 %) | 4,792 (73,7 %) | 0,043 | 0,105 | 0,966 (14,9 %) | 0,099 | 0,209 | 0,091 | 124,7 | 19,2 |
| 000100 K10 | 9,582 | 0,421 (4,4 %) | 5,266 (55,0 %) | 0,090 | 0,362 | 3,175 (33,1 %) | 0,267 | 0,798 | 0,219 | 268,0 | 28,0 |
| 000000 K10 | 13,922 | 0,755 (5,4 %) | 8,198 (58,9 %) | 0,106 | 0,465 | 4,016 (28,8 %) | 0,382 | 0,998 | 0,210 | 351,2 | 25,2 |
| 000200 K10 | 15,286 | 0,821 (5,4 %) | 9,824 (64,3 %) | 0,118 | 0,447 | 3,845 (25,2 %) | 0,231 | 1,003 | 0,263 | 371,9 | 24,3 |

« Fils occupés » = `chain_cpu_s / chain_total`. **Plus de la moitié de la machine est inactive en moyenne** : 19–23 fils sur 48 à K5, 24–28 à K10. À K10, q2 + census font déjà 0,77–1,27 s, soit ≥ 1 s sur 000000 et 000200 à elles seules.

## 3. Écart au contrat (calculé)

Quatre essais par trame et K (deux répétitions × MEB ON/OFF). « Min » est le chiffre publié ; « moy. ON » est la moyenne de la configuration par défaut.

| Trame, K | Min / moy. ON / max (s) | ×1 s | ×100 ms | CPU·s/48 | CPU requis ×(1 s) / ×(100 ms) | CPU par nœud de sortie | Budget par nœud 1 s / 100 ms |
| --- | --- | ---: | ---: | ---: | --- | ---: | --- |
| 000100 K5 | 3,669 / 3,741 / 3,813 | 3,7 | 37 | 1,82 | ×1,8 / ×18 | 66,9 µs | 36,7 / 3,67 µs |
| 000000 K5 | 5,314 / 5,627 / 5,697 | 5,3 | 53 | 2,41 | ×2,4 / ×24 | 75,1 µs | 31,1 / 3,11 µs |
| 000200 K5 | 6,403 / 6,505 / 6,569 | 6,4 | 64 | 2,60 | ×2,6 / ×26 | 74,1 µs | 28,5 / 2,85 µs |
| 000100 K10 | 9,579 / 9,582 / 10,053 | 9,6 | 96 | 5,58 | ×5,6 / ×56 | 45,0 µs | 8,06 / 0,81 µs |
| 000000 K10 | 13,913 / 13,922 / 14,295 | 13,9 | 139 | 7,32 | ×7,3 / ×73 | 47,3 µs | 6,46 / 0,65 µs |
| 000200 K10 | 15,275 / 15,286 / 15,647 | 15,3 | 153 | 7,75 | ×7,7 / ×77 | 49,8 µs | 6,43 / 0,64 µs |

Les chiffres publiés à K5, 5,31 et 6,40 s (PASSATION.md:147-148, COORDINATION:769), sont des essais **MEB OFF** (probe_3, probe_19). Ce sont des minima sur quatre essais ; la configuration par défaut donne en moyenne 5,63 et 6,51 s.

Conclusion arithmétique : même avec une occupation parfaite des 48 fils, le code R7b doit réduire son **travail CPU** d'un facteur ×1,8–2,6 à K5 et ×5,6–7,7 à K10 pour tenir 1 s. Pour 100 ms, il faut ×18–26 et ×56–77. Aucun réglage d'ordonnancement ne ferme cet écart ; il faut un autre algorithme, ou le GPU, sur le travail lui-même.

## 4. Parallélisme (mesuré)

La machine compte 24 cœurs physiques × 2 fils (`vm/lscpu.stdout`). Toutes les paires W24/W48 disponibles portent sur la même trame :

| Session | Cas 000000 | Chaîne W24 → W48 | q3/q4 | Tour | CPU·s |
| --- | --- | --- | ---: | ---: | --- |
| R1 | K5, tour mono | 21,92 → 18,81 (×1,165) | ×1,248 | ×1,044 | 344 → 545 (+58 %) |
| R2 (refusé) | K10, statique 24→48 | 56,47 → 50,07 (×1,128) | ×1,236 | ×1,020 | 792 → 1 239 (+56 %) |
| R3 | K10 | 38,92 → 35,51 (×1,096) | ×1,275 | ×1,019 | 345 → 498 (+44 %) |
| R5 | K10 | 21,59 → 17,19 (×1,256) | ×1,435 | ×1,108 | 300 → 425 (+42 %) |

Il n'existe **aucune** mesure W24 depuis R5, aucune W1 sur G4 et aucun balayage de W. L'efficacité parallèle et la fraction séquentielle par phase restent donc inconnues. Le gain de q3/q4 de 24 à 48 fils (×1,24–1,44) correspond à un rendement SMT. La tour n'en tire presque rien. Le CPU·s mesuré à W48 inclut aussi l'inflation due au SMT : CPU·s/48 est un plancher **optimiste**.

**Temps système : un signal nouveau.** GNU time montre 9,4–15,5 s de CPU système à K5 R7b, soit 10–13 % du CPU total, et 4,6–11,2 s à K10 (1,7–2,9 %). En R1 et R2, ce temps était de 1,1–3,6 s (0,1–0,5 %). L'ablation appariée R3 attribue le saut au certificat de voies mortes :

- temps système ON/OFF : 6,74/1,68 s sur 000000 K5, 11,80/4,50 s sur 000200 K5, 6,13/2,68 s sur 000000 K10 ;
- commutations volontaires : 0,69/0,12 M, 1,11/0,63 M, 0,90/0,09 M ;
- défauts mineurs quasi inchangés (0,46/0,45 M sur 000000 K5).

La cause est donc des blocages ou appels système, pas des défauts de page. En R7b, on compte 0,44 à 1,97 M commutations volontaires par exécution. Le code montre une file q3/q4 unique protégée par un seul mutex : `wspd_q34.cpp:846-853`, verrou à chaque retrait (884-906), publication (867) et mise à jour de compteur après chaque tâche (918-919). Les fils de travail sont recréés à chaque région parallèle, sans pool persistant (`tower_chain.cpp:196-231`, `tower/parallel/pool.hpp:56-93`). L'attribution précise reste **supposée**.

Le générateur calcule déjà `worker_timings` (mur, CPU et attente par fil) et `tasks` (`wspd_q34.hpp:237-243`). Mais `tower_chain.cpp:375-417` ne recopie que `r34.pipeline.work`, et la sonde ne publie aucun temps par fil ni aucun CPU par phase. L'auditeur A l'avait demandé dès `d2700314` (CONTRE_AUDIT_A_MESURES_PLAN:162-183). C'est toujours absent au commit `c02d45ac`, sonde v12.

## 5. Honnêteté du chronomètre

**Inclus dans `chain_total`** (`tower_chain.cpp:287-288` à `584`) :

- validation des options, `prepare_cloud` (≈0,9 ms), index du générateur (≈12 ms) ;
- q2, q3/q4, fusion, index de la tour (≈10 ms), census, tour ;
- le résumé des ordres, qui parcourt tous les nœuds (`543-555`) ;
- la destruction des temporaires déclarés dans le `try` : catalogue `balls` de 0,25–1,23 Go, index, slots ;
- la création des fils à chaque région ;
- les défauts de page de premier contact : 0,37–0,52 M à K5 et 1,74–2,25 M à K10, soit 1,4–2,0 Gio et 6,6–8,6 Gio touchés en pages de 4 Kio, pour un pic RSS de 1,08–1,32 et 3,78–4,96 Gio. Il y a donc un brassage d'environ ×1,5–1,8 (calculé, pages de 4 Kio supposées).

Le CPU vient de `CLOCK_PROCESS_CPUTIME_ID`, tous fils compris (`tower_chain.cpp:44-47, 585-586`).

**Exclu** :

- la lecture du fichier et son empreinte (1,1 ms, publiée à part, `tower_probe.cpp:158-166`) ;
- la quantification à 1 mm, le dédoublonnage et le masque sans sol. Ce sont des préparations hors ligne ; seul le pilote v8 local donne un temps, 29,1–31,8 ms de la lecture brute au masque (`morsehgp3D_v8/docs/PILOTE_LIDAR_SANS_SOL_20260921.md:107-117`). Les reçus G4 ne publient **ni grille ni masque**, contrairement à l'hypothèse de travail « mesurés à part et publiés dans le même reçu » (`docs/AUDIT_V8_SYNTHESE.md:311-315`) ;
- le digest, depuis R7b : 0,16–0,21 s à K5 et 0,80–1,00 s à K10, synchrone ;
- la destruction du résultat, l'impression JSON et la sortie du processus : 0,06–0,09 s à K5 et 0,21–0,26 s à K10 ;
- la conversion en `CertifiedTowerInput`, absente de `src/` (hypothèse § 9.2 de la synthèse, `AUDIT_V8_SYNTHESE.md:316-319`) ;
- tout fonctionnement à chaud : chaque cas est un processus neuf.

Le lecteur borne `read + chain_total + digest` par le mur externe à 0,05 s près (`tower_worker_v9.py:145, 554-563`).

**Jugement.** Le chronomètre est honnête **dans son périmètre écrit** (`tower_probe.cpp:14-17`) et contrôlé par le mur externe. Ce n'est pas le chronomètre d'une trame en production. À 100 ms, les postes exclus (masque ≈30 ms en local, fin de processus 0,2 s à K10, digest s'il est gardé) sont de l'ordre du budget. À l'inverse, le démarrage à froid et le brassage de pages pénalisent la mesure actuelle ; une boucle à chaud avec arènes réutilisées pourrait les amortir (supposé, non mesuré).

**Protocole.** Le plan exige au moins trois répétitions sur hôte calme, avec refus automatique au-delà d'un seuil (PLAN_V9.md:257-258), puis échauffements et p95 (220). Les reçus G4 n'ont qu'une ou deux répétitions, sans échauffement ni p95. La campagne locale de croissance a tourné sous une charge moyenne de 9 à 13 (son README:6-8). Les dispersions entre répétitions restent faibles sur G4 (≤ 4 % à K5, ≤ 2,1 % à K10), sauf pour R3, qui n'a qu'une répétition. Enfin, `SUMMARY.json` garde le même schéma `mhgp9_g4_tower_summary_v1` alors que ses champs changent d'une session à l'autre et que `cpu_s` a changé de sens : somme utilisateur+système de GNU time en R1 (8/8), `chain_cpu_s` à partir de R2 (13/13 … 24/24). Aucun générateur de ce fichier n'est versionné. Les autres valeurs concordent avec les bruts.

## 6. Représentativité

La mesure porte sur trois trames d'**une seule séquence** (08 : 000000, 000100, 000200), sans sol (masque Patchwork++ approximatif), à 35 551–45 845 sites. Rien n'est mesuré entre 46 000 et 60 000 sites, ni sous 35 000, alors que le contrat vise 30 000–60 000. La dépendance à la scène est forte : à K5, q3/q4 prend 2,52 s sur 000100 et 4,74 s sur 000200, soit ×1,9 pour ×1,29 sites. Une extrapolation à 60 000 sites avec un exposant de 1 à 1,5 donnerait 8,5–9,7 s à K5 et 20–23 s à K10 sur le code R7b (**supposé**). Il n'y a ni trame brute avec sol, ni s = 10/12, ni GPU, ni p95 entre trames.

## 7. Lois d'échelle disponibles

Seule existe la campagne locale v12 (`origin/main` 1f73b40d) : W8, une répétition, hôte chargé, disques emboîtés 8k/16k/32k, trame entière et six morceaux, trois trames, K5 et K10. Les contre-audits CONTRE_AUDIT_LIDAR_SCALING_V12_LOCAL et CROISSANCE_LIDAR_PLANS_ET_DENSITE l'ont déjà analysée pour les compteurs lourds (`core_sites` p ≤ 3,05, paires développées p ≤ 2,27). J'ajoute trois points.

**(a) La densité de sortie change avec la taille emboîtée** (mesuré). Nœuds de sortie par site à 8k / 16k / 32k / trame entière :

| Trame | K5 | K10 |
| --- | --- | --- |
| s00 | 48,6 / 45,9 / 40,8 / 38,7 | 259,3 / 235,4 / 200,0 / 186,2 |
| s01 | 38,3 / 38,1 / 37,1 / 36,8 | 168,5 / 166,9 / 169,3 / 167,5 |
| s02 | 41,2 / 39,6 / 37,3 / 36,7 | 184,3 / 175,0 / 162,5 / 162,9 |

Les boules suivent le même profil. Leurs exposants inférieurs à 1 (0,75–1,03) viennent donc du recadrage radial, qui ajoute une périphérie moins dense. **Ce n'est pas une propriété de l'algorithme.**

**(b) Les temps locaux ne se prêtent pas à un exposant.** Sur la relation 32k → trame entière (rapport de tailles 1,11–1,43), les exposants de temps vont de −1,29 (q3/q4, s01 K5) à 4,16 (tour, s01 K10). Sur la même relation, les compteurs déterministes (MEB, intrus, boules, nœuds) restent entre 0,66 et 1,5.

**(c) q2 croît vite au premier doublement.** De 8k à 16k, les exposants de temps de q2 valent 1,20–1,84 dans les six cas (temps bruités).

Ce qui manque encore :

- toute croissance mesurée sur G4 ;
- W1, pour séparer croissance du travail et efficacité parallèle ;
- des répétitions ;
- des densités à emprise fixe (la proposition de CROISSANCE_LIDAR_PLANS_ET_DENSITE) ;
- les trames brutes et d'autres séquences ;
- 60k/64k, s = 10/12 ;
- CPU système et CPU par phase dans le runner.

## 8. Bornes pour tout algorithme alternatif (R7b, mesuré ou calculé)

| Trame, K | Boules/site | Nœuds/site (ordre K seul) | Contributions/site | Sortie ABI actuelle | Catalogue |
| --- | ---: | --- | ---: | ---: | ---: |
| 000100 K5 | 30,83 | 36,76 (13,45) | 21,43 | 168 Mio | 0,245 Go |
| 000000 K5 | 32,76 | 38,65 (14,45) | 22,51 | 198 Mio | 0,293 Go |
| 000200 K5 | 30,71 | 36,71 (13,29) | 21,43 | 216 Mio | 0,315 Go |
| 000100 K10 | 123,30 | 167,48 (35,58) | 99,81 | 770 Mio | 0,982 Go |
| 000000 K10 | 138,21 | 186,19 (41,08) | 110,67 | 960 Mio | 1,235 Go |
| 000200 K10 | 119,61 | 162,90 (33,94) | 97,43 | 968 Mio | 1,228 Go |

La sortie suit la formule de A : `80·nœuds + 8·parents + 80·contributions`. Il y a 1 à 13 présentations en doublon par catalogue. Entre K5 et K10, l'exposant apparent en K vaut 1,96–2,08 pour les boules et 2,15–2,27 pour les nœuds (deux valeurs de K seulement) ; la chaîne coûte ×2,35–2,56 et le CPU ×2,98–3,07.

À 100 ms, K10 exige 60–75 M nœuds/s, et 18–22 Go/s d'écriture (sortie + catalogue) avec l'ABI actuelle. Une représentation compacte est donc nécessaire. La taille de sortie n'est pas, en soi, le verrou.

Le verrou est l'**amplification de travail** (compteurs v12 locaux, trames entières) :

- 313–387 tests ponctuels d'atlas par candidat q3/q4 émis ;
- 4,8–27,9 paires développées par candidat émis ;
- 15–55 graines q4 par q4 émise ;
- 14,5–20 candidats q2 par paire acceptée (R7b) ;
- 16–73 nœuds d'intrus et 0,8–2,05 appels MEB par boule.

Un algorithme sensible à la sortie doit ramener ces rapports vers une constante petite ; c'est l'ordre de grandeur des facteurs ×18–77 du § 3.

## 9. Ce que j'ajoute aux notes existantes

- **Déjà dit par d'autres** : plancher CPU·s/48 de R6 (ETAT_COURANT:183-189), postes restants de R7b (OBSTACLES_GPU:16-36), digest hors chrono et comparaisons R6/R7b (B R6, B R7b), octets de sortie à K10 et boules/site (CONTRAT_COUTS:85-112), limites de la pente locale (contre-audits v12). Je ne les re-signale pas comme neufs.
- **Ce que j'ajoute** :
  1. la table unifiée R1 → R7b sur un périmètre constant ;
  2. les facteurs 100 ms et les budgets par nœud ;
  3. l'occupation (19–28 fils sur 48) ;
  4. le temps système et les commutations, causés par le certificat de voies mortes selon l'ablation R3 ;
  5. la stagnation de q2 et du census ;
  6. le chiffrage « hors chrono » de R7b ;
  7. l'écart au protocole écrit (grille et masque absents du reçu, répétitions, p95, hôte chargé) ;
  8. le mélange ON/OFF dans les meilleurs chiffres ;
  9. la sémantique mouvante de `SUMMARY.json` ;
  10. l'artefact de densité des disques emboîtés et le bruit des temps locaux.

## 10. Mesures recommandées, par ordre de priorité

1. Faire publier par la sonde, pour chaque phase, les écarts `getrusage` (utilisateur, système, commutations volontaires et involontaires, défauts mineurs), plus `worker_timings` et `tasks` de q3/q4. C'est quasi gratuit et c'est la seule façon d'expliquer les 20–29 fils inactifs et les 10–15 s de CPU système.
2. Une session G4 dédiée, à K5 sur 000100 : W1 / W6 / W12 / W24 / W48, et W24 avec un fil par cœur physique, trois répétitions. Budget CPU estimé à moins de 1 500 s (supposé). Elle ajusterait la fraction séquentielle par phase.
3. Un chronomètre « boucle de trames » à chaud : même processus, N trames, arènes réutilisées, grille, dédoublonnage et masque inclus en option, p50 et p95 publiés.
4. Un échantillon de trames déclaré à l'avance, sur plusieurs séquences, allant jusqu'à ≈60 000 sites sans sol, trois répétitions et p95.
5. Un générateur de `SUMMARY.json` versionné, avec un schéma par version.