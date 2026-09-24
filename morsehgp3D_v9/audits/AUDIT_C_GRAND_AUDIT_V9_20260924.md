# Grand audit de la v9 : ce qui marche, ce qui peut changer, et les 100 ms

24 septembre 2026, auditeur C, à la demande de l'utilisateur. Cadre :
`phase=exploration_v9_hors_registre`, `public_status=not_claimed`.
GCP non utilisé.

**Méthode.** Six lecteurs indépendants, en lecture seule, ont couvert :
- la performance de R12 à R20 ;
- la confiance et les tests ;
- les trames avec sol ;
- l'architecture et le processus ;
- la tour FULL ;
- le générateur et le GPU.

Trois concepteurs ont ensuite traité la faisabilité des 100 ms, les
améliorations classées et le processus. Trois sceptiques ont contre-lu
leurs chiffres, leurs gains et leurs manques. Les lecteurs étaient ancrés
à `f02e29670`. Les sceptiques ont relevé cinq commits arrivés depuis :
- la contrelecture R20 de B et la **décision de l'utilisateur** : la tour
  explicite est matérialisée dans les 100 ms ;
- la v26 : session CUDA ouverte avant la chaîne, et plan R21 WIP avec les
  trames brutes ;
- la contre-vérification de B ;
- R-29.

Ce texte intègre ces corrections. Tout chiffre non tiré d'un reçu est
marqué **projection**. Les projections de conception passées ont été
optimistes de 1,3 à 2 fois pour les noyaux, et de 28 à 69 % pour les
étapes de la tour.

## 1. Réponse courte

**100 ms : non, pas avec les algorithmes connus, sur une G4, dans aucun
des quatre cas** (K5 ou K10, avec ou sans sol). Pour K5 sans sol,
l'objectif ne reste envisageable qu'au prix d'une réduction de travail
qui n'existe pas encore : c'est de la recherche, pas de l'ingénierie.

Faits mesurés en R20 sur les trames sans sol :
- la tour FULL seule prend **374 à 456 ms à K5** et **1,61 à 1,95 s à K10** ;
- le reste de la chaîne prend 636 à 804 ms à K5 ;
- plusieurs étapes dépassent chacune 100 ms à K5 : la phase A de l'ordre 5
  (148 ms), la phase 0 (199 ms), le front q3/q4 (106 ms), le recensement
  (99 ms), q2 (104 ms, aujourd'hui caché) et les noyaux GPU (155 à 233 ms
  au total).

La décision de l'utilisateur place la sortie explicite dans le budget :
1,54 M nœuds à K5 et 7,43 M à K10 sur 08/000000.

| cas | aujourd'hui (R20, projection pour le brut) | cible réaliste en ingénierie | fin de refonte GPU complète (projection) | 100 ms |
| --- | --- | --- | --- | --- |
| K5 sans sol | 1,01–1,26 s (médiane 000000 ≈ 1,20 s) | 0,9–1,1 s | 0,25–0,5 s | infaisable sans une réduction de travail de recherche |
| K10 sans sol | 3,15–3,87 s | 2,4–3,1 s | 0,7–1,4 s | infaisable |
| K5 avec sol | 2,0–2,4 s (projection R20) | 1,5–1,9 s | 0,4–0,8 s | infaisable |
| K10 avec sol | 6,9–8,3 s (projection R20) | 4,5–6,5 s | 1,2–2,8 s | infaisable |

La colonne « ingénierie » réunit la session résidente, les chevauchements
et les leviers CPU de la tour. Ses gains ne s'additionnent pas : plusieurs
puisent dans la même fenêtre de CPU inactif et la même fenêtre de la tour.
Une fois escomptée, **même 1 s à K5 sans sol n'est pas acquis sur la trame
la plus lente**. La colonne « refonte » suppose le générateur, le
recensement, la fusion et toute la tour sur l'appareil, en session chaude.
Elle suppose aussi la phase A parallèle, qui n'est ni prouvée ni
construite, et le traitement sur l'appareil des grosses arêtes des trames
brutes.

Je recommande donc de présenter à l'utilisateur des cibles étagées :
- **1 s à K5 sans sol**, presque atteint ;
- **1 s à K5 avec sol**, qui demande la tour sur GPU ;
- **250 à 500 ms à K5 et environ 1 s à K10 sans sol** comme horizon de
  refonte.

Les 100 ms deviennent un horizon de recherche, conditionné à une nouvelle
réduction de travail d'au moins 2,3 fois dans le générateur et à une tour
fondamentalement moins chère. La recherche restreinte à certains ordres ou
la sortie compacte hors du chrono ne sont plus des échappatoires : la
première change l'objet, et l'utilisateur a refusé la seconde.

**Sol.** Aucune trame brute n'a encore tourné sur le chemin actuel (S3, S4,
tour allégée, G4). Les seules mesures sont locales, sur CPU, avec d'anciens
binaires. Le WIP R21 (`61cfba666`) les ajoute : trois trames brutes de
123 à 126 k sites, versionnées dans le reçu v8 `float32_precision`, à K5 et
K10, avec jumeaux moteur. Je calcule leurs épingles CPU en local avant la
session (§ 5).

## 2. Ce qui marche

**Trajectoire** (meilleure chaîne, trames 000100/000000/000200, W48) :

| session | levier principal | K5 (s) | K10 (s) |
| --- | --- | --- | --- |
| R12 | S2, filtre sur GPU | 2,011 / 2,473 / 2,694 | 6,633 / 8,324 / 8,701 |
| R14 | S3 à 16 warps, tour allégée | 1,563 / 1,976 / 2,053 | 5,488 / 6,995 / 7,051 |
| R16 | S4b, voies q4 sur GPU | 1,214 / 1,532 / 1,651 | 3,919 / 5,159 / 5,007 |
| R18 | tour E3/E2, voies en tâches | 1,046 / 1,251 / 1,355 | 3,258 / 4,268 / 4,181 |
| R20 | voies étape 3, préparation en deux étapes | 1,010 / 1,109 / 1,260 | 3,147 / 3,873 / 3,860 |

**Gains appariés dans la même session** (08/000000) :

| levier | K5 | K10 |
| --- | --- | --- |
| S2 face au moteur | −0,76 s | −2,12 s |
| S3 | −0,22 s | −0,79 s |
| S4a | −0,17 s | −0,39 s |
| S4b | −0,27 s | −1,58 s |
| q2 recouvert | −0,06 à −0,10 s | −0,13 à −0,23 s |

L15 est négatif (+4 % sur le noyau des voies) ; le développeur l'a coupé
en v26. Le rapport GPU/moteur passe de 1,31 à 2,55 à K5, et de 1,25 à 2,36
à K10.

**Exactitude**, sur le régime mesuré :
- chaque cas G4 a un jumeau moteur, et les 12 comparaisons de R20 sont
  égales ;
- les **90 cas sur 90** de R16 à R20 retrouvent les six épingles de tour et
  de catalogue, alors que les chemins ont beaucoup changé ;
- le recensement v7 recompte chaque clé du générateur v8 ;
- Euler est vérifié pour K ≤ Kmax−2, et T2 à n ≤ 14 ;
- la validation FULL tourne à chaque exécution ;
- 258 CTest sont enregistrés, dont 223 `gate` et 88 mutants, et les juges
  q2/q3 (R-20) sont adoptés.

**Discipline des reçus** : commit, arbre, snapshot, binaire, entrées et
`SHA256SUMS` sont épinglés. Le contrat sonde/worker échoue fermé, et le
`TERMINATED` de chaque session est certifié.

## 3. Où va le temps (R20, 08/000000)

| poste | K5 (ms) | K10 (ms) |
| --- | ---: | ---: |
| chaîne | 1 109 | 3 873 |
| tour FULL | 422 : validation 71, phase 0 199, phase A 57, queue 96 | 1 955 : validation 248, phase 0 1 226, queue 408 |
| q3/q4 (front, filtre, certificats, voies) | 526, dont 80 hors de tout chrono | 1 049, dont 176 hors chrono |
| recensement | 99 | 495 |
| fusion et deux index | 50 | 120 |
| q2 (recouvert) | 104 | — |

Constats principaux :
1. **L'appareil n'occupe que 19 à 22 % du mur.** Noyaux : 210 ms à K5,
   571 ms à K10. K10 est lié au CPU : 1 s à K10 demande de diviser par plus
   de 3 le travail CPU, pas des noyaux plus rapides.
2. **Rendements décroissants** : les écarts de K5 d'une session à l'autre
   passent de −271 à −66 ms. Le CPU de la chaîne est plat depuis R16, la
   tour depuis R18. Front, recensement, fusion et noyau du filtre n'ont
   **pas bougé depuis R12**.
3. **Fenêtres inexploitées** :
   - le CPU attend l'appareil environ 180 ms à K5 et 590 ms à K10 ;
   - l'appareil attend pendant le front, la fusion, le recensement et la
     tour ;
   - en moyenne, 16 à 20 fils sur 48 sont occupés.
4. **La tour est ordonnée de façon optimale** (règle de Jackson). Sa
   fenêtre égale la somme des phases 0 plus la phase A d'un ordre bas, à
   0,5 ms près sur 36 sondes. **Seul le débit de la phase 0 déplace la
   fenêtre.** Ensuite, la phase A mono-fil de l'ordre le plus haut devient
   le plancher : 148 ms à K5, 544 ms à K10.
5. **Glu hôte des trois appels GPU**, soit tout ce qui n'est pas noyau :
   210,6 ms à K5. Elle comprend :
   - 80 ms de contrôles non chronométrés ;
   - les parties hôte des appels ;
   - des transferts.

   Le « transfert des voies » (30 ms à K5, 151 ms à K10) mélange
   téléversement, téléchargement, petites copies synchrones et
   allocations hôte. **Ce n'est pas un débit PCIe**, et on ne peut pas
   l'attribuer en entier à un tampon épinglé.

## 4. Les trames avec sol

**Mesuré**, uniquement sur 08/000000 brut, en local, avec les binaires v12
et v17. Rapports brut / sans sol :

| grandeur | K5 | K10 |
| --- | --- | --- |
| catalogue | ×2,16 | ×2,07 |
| CPU·s | ×1,90 | ×1,79 |
| survivants | ×1,95 | — |
| sites de cœur | ×1,56 | ×1,38 |
| paires | ×0,93 | ×1,23 |
| RSS | ×1,96 | 8,2 Gio |

Le sol agit comme une population de témoins : le travail croît de 1,2 à
2,2 fois pour 3,1 fois plus de sites. Exception : la masse des cœurs sur
les arêtes longues, avec des pentes finies de 1,9 à 2,1 dans les quarts
chauds. D'après B, il y a à K10 37,87 M paires et 2,33 Md incidences cœur
+ cover extrémités comprises, dont 2,30 Md formes chargées.

**Projection** sur le code R20, pour 08/000000 brut : K5 2,0 à 2,4 s, K10
6,9 à 8,3 s. La tour, le recensement et la fusion suffisent à dépasser 1 s
à K5, même si q3/q4 ne coûtait rien.

**Ce qui cassera d'abord** :
1. le **temps** ;
2. les capacités par arête que les trames sans sol n'atteignent pas :
   ardoises de 2^16 sites (un cover de 72 833 sites a été mesuré en v8 sur
   ce brut à K5) et 2^12 événements q4 par graine (déjà 93 % sur 000200
   sans sol à K10). Leur dépassement renvoie l'arête au CPU, avec une
   traîne de taille inconnue ;
3. l'**arène de cover des voies**, dont le dépassement refuse **tout le
   cas** (`resource_exhausted`, sans repli).

La mémoire n'est pas un verrou : RSS environ 9 Gio sur 177, plafonds 2^31
utilisés à moins de 2 %.

**À publier par R21** :
- tailles maximales de cœur et de cover ;
- histogrammes log2 ;
- reports par cause et temps de traîne CPU ;
- usage de l'arène face à sa capacité ;
- pic mémoire de l'appareil ;
- RSS ;
- temps des phases de la tour ;
- Euler à K10.

## 5. Épingles des trames brutes (en cours)

Entrées du WIP R21 : `b00`, `b01` et `b02` = 08/000000, 000100, 000200
bruts, SHA-256 `233cc4ea…`, `de45e8dc…` et `37a7be39…` (reçu v8
`float32_precision`). Je calcule sur `main` les condensés de tour et de
catalogue à K5 et K10 :
- bras moteur ;
- bras par lots CPU, c'est-à-dire les jumeaux hôte du chemin GPU : filtre,
  certificats, voies q3 et q4.

Les valeurs suivront dans le canal et dans
[`c_raw_pins_20260924/`](c_raw_pins_20260924/README.md). Tout cas R21
brut devra les reproduire.

## 6. Ce qui peut changer : classement après contre-lecture

Les gains sont des **projections**, déjà escomptées d'après l'historique.
Chaque changement doit garder l'objet, prouvé par les portes, les jumeaux
et les épingles, et se chiffrer par des paires entrelacées dans la même
session.

**A. Mesurer d'abord (aucun gain direct ; conditionne tout le reste).**
1. Sous-chronos des 80 à 176 ms de q3/q4 hors chrono. Il faut aussi :
   - le CPU de fil par phase de la tour ;
   - le pic mémoire de l'appareil ;
   - les niveaux maximaux des capacités ;
   - les fautes de page par phase ;
   - la séparation téléversement / téléchargement.

   Les compteurs exploratoires vont dans une table `diagnostics` typée,
   hors du jeu exact de clés, ce qui arrête le va-et-vient du schéma
   (v1 à v26 en 44 h).
2. En titre, la **configuration par défaut** et la **médiane d'au moins 3
   répétitions entrelacées**. Aujourd'hui, R20 égale R19 à la médiane de
   000000/K5.
3. **Geler le contrat** (R-18) : début et fin du chrono, froid ou chaud,
   contrôles obligatoires dans le chrono. Tout déplacement de frontière,
   par exemple le contexte CUDA hors de `chain_total` en v26, est un
   **amendement** publié en froid et en chaud, pas un gain.

**B. Ingénierie, par gain attendu décroissant, K10 d'abord.**

| n° | changement | gain K5 | gain K10 | risque pour l'objet |
| --- | --- | --- | --- | --- |
| 1 | Tour, phase 0 : grouper les requêtes par empreinte et radix au lieu de trier des enregistrements de 56 o (28–42 % de la phase 0) ; radix des niveaux de la validation | −30 à −60 ms | −200 à −300 ms | faible, les cibles ne dépendent que de la clé ; identités du registre à garder |
| 2 | Tour, E6 : résolution de la phase 0 sur l'appareil inactif, par la couture `FullBallBatchResolver` | −20 à −40 ms (plancher : phase A de l'ordre 5) | −250 à −350 ms de plus | moyen-élevé : prédicats U320 exacts sur l'appareil ; chaque cible jugée dans le préflight |
| 3 | Glu q3/q4 : contrôles gardés, mais en parallèle et recouverts par l'appel suivant ; fin de la session résidente (index validé une fois, tampons résidents) | −40 à −100 ms (plafond : réserve de 210 ms) | −100 à −250 ms | nul si les sorties restent identiques ; risque de durée de vie, cf. la revue pré-R20 |
| 4 | Recensement et index de la tour dans la fenêtre de l'appareil (clés q2 d'abord, puis voies fenêtrées) | −30 à −50 ms | −60 à −100 ms, puis −200 à −400 ms fenêtré | nul : même recensement une fois par clé |
| 5 | Queue de la tour : E4 (populations en pipeline, IDs par décalage statique) et E5 (images et encodage scindés) | −40 à −65 ms | −150 à −330 ms | modéré : identité des IDs à prouver et à garder par porte |
| 6 | Catalogue scellé (R-29) : positivité dans la chaîne, puis passe 1 sautée | −15 à −22 ms | −60 à −80 ms | frontière de confiance ; voir la proposition |
| 7 | Fil de travail persistant unique, avec budget global de concurrence | −5 à −30 ms | −35 à −90 ms | très faible |
| 8 | Phase A de l'ordre haut : allègement maintenant (u32, pas de pool imbriqué), **parallélisation** (lemme max-ID, Borůvka) après E6 | −8 à −20 ms, puis décisif | idem | élevé pour la parallélisation : preuve à enregistrer d'abord |
| 9 | Front WSPD : émission SoA directe, puis front sur l'appareil fusionné au filtre des rectangles | −10 à −30 ms, puis −70 à −100 ms | −120 à −160 ms | moyen : même multiensemble de rectangles et de voies |
| 10 | Noyau du filtre, inchangé depuis S1 : parcours coopératif par warp, tuiles de paires, suppression des plafonds 2^31 | −10 à −22 ms | −15 à −35 ms | nul : même prédicat |

**C. Pour le brut, avant toute revendication.**
- Arène de cover et gros cœurs : report **par arête** ou appel fenêtré par
  plages d'arêtes à ordinaux stables, au lieu du refus de tout le cas.
- Chemin appareil pour les arêtes au-delà de 2^16 sites (un bloc ou
  plusieurs warps par arête), pour qu'aucune grosse arête ne tombe sur le
  chemin critique CPU.

**D. Recherche, seule piste vers 100 ms ou 1 s à K10 avec sol.**
- Réduction du travail avant les cœurs : certificats de moments A×B (sûrs
  selon B, mais le bloc uniforme naïf est **négatif** sur LiDAR), chargement
  paresseux du cœur (jusqu'à 43 % des formes sur le brut), rejet des
  graines q4 face aux cellules échouées de S3 (**lemme nouveau** à
  enregistrer).
- Une tour qui résout en masse les facettes, leurs cibles et leurs
  incidences en gardant les plateaux exacts. Elle fait 17,4 M représentants
  de facettes et 11,3 M MEB à K10 : c'est la quantité de travail qu'il
  faut réduire, pas seulement l'ordonnancement.

**E. Arrêter.**
- Dépenser des sessions G4 sur des micro-leviers K5 des trames sans sol.
- Compter un déplacement de frontière comme un gain.
- La prolifération des leviers : 19 leviers, 14 règles de refus, 32 512
  combinaisons valides, dont 3 exécutées. À remplacer par des préréglages
  nommés et des énumérations ; les bras OFF des leviers toujours ON passent
  en build de test ; L15 va aux fausses pistes.
- Rouvrir une piste fermée sans théorème de complétude ni fixture.

## 7. Confiance et tests à ajouter, par risque

1. **Épingles des trames brutes et d'un jeu réservé**, avant tout temps
   G4 brut. En cours pour les trois trames (§ 5).
2. **Angle mort commun d'Euler et de FULL** (q2 à p = Kmax−1, q3 à
   p = Kmax−2) : 26,9 % des boules régulières à K5 et 11,0 % à K10 sur les
   coupes 8k ; les juges n'en couvrent que 0,2 à 1 %. Deux actions :
   - le protocole Kmax+2 à K5 sur les **trames entières** (invariant global,
     pas une vérification exhaustive) ;
   - les juges stratifiés sur les trames entières, avec la population par
     strate publiée.

   À K10, déclarer explicitement le résidu de K9 et K10.
3. **Épingle sans leviers** (leviers capables d'omission OFF), au niveau
   catalogue et présentations : les épingles actuelles sont prises leviers
   ON.
4. **GPU jugé au-delà du préflight synthétique** : préflight jugé sur une
   coupe LiDAR, à K10, et sur les chemins de capacité forcés ; juge
   échantillonné sur les trames entières. Aujourd'hui, `judged_edges = 0`
   sur LiDAR.
5. **Positivité** des supports réguliers dans la chaîne, et portes de refus
   (R-29). **Bornes des IDs de support** dans `check_lanes_batch` avant leur
   déréférencement (B).
6. TSan sur la chaîne en émulation CPU (q2 recouvert, chevauchement,
   validation parallèle) et compute-sanitizer sur les préflights de la VM.
7. Un bras T2 avec les leviers G4 en émulation CPU et des capacités
   réduites, pour exercer les reports.
8. Journaux CTest archivés (junit) et condensé du préflight épinglé.

## 8. Processus

- **Revue indépendante des diffs v22 à v26**, surtout la propriété des fils
  et les durées de vie : fil q2, fil de préparation de l'appareil,
  chevauchement, validation parallèle. B a repris le 24 au soir.
- **Découper les monolithes** : `Builder` fait environ 1 760 lignes, en
  en-tête, inclus par 14 fichiers ; `run_tower_chain` environ 710 lignes.
- **Réécrire la PASSATION** en une page d'état courant (contrat, meilleurs
  reçus, configuration par défaut, points ouverts) et déplacer la
  chronologie dans un journal.
- **Mécaniser les gardes** : un crochet pré-commit dans les worktrees
  d'audit refuse les chemins hors `audits/` et le canal, ce qui évite une
  perte comme `293aa6d7`. Ajouter une identité ou un pied `Actor:` par
  acteur. Le préflight d'espace disque est fait (R-27).

## 9. Corrections apportées par la contre-lecture

- La décision de l'utilisateur (sortie explicite dans les 100 ms) retire
  les gains de sortie compacte et « d'expansion hors chrono ».
- Les gains de la session résidente sont bornés par la réserve de 210 ms,
  qui contient aussi les contrôles gardés. Les projections qui annonçaient
  −150 à −220 ms à K5 sont ramenées à −40 à −100 ms.
- Les projections cumulées ne sont pas additives : sous 1 s à K5 n'est pas
  assuré.
- Le « transfert des voies » n'est pas un débit (§ 3).
- Mon libellé « 2,33 Md formes » compte des incidences extrémités
  comprises. Les formes chargées valent 2,30 Md (B).
- Les trames brutes sont bien versionnées (reçu v8 `float32_precision`).
  Leur transport vers G4 n'est pas bloqué.

Pièces brutes du workflow (cartes, conceptions, critiques) : conservées
hors dépôt ; ce texte en est la synthèse.
