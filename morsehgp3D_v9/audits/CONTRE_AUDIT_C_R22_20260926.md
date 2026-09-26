# Contre-lecture C de R22 : sceau, recensement q2 précoce, bassin épinglé

26 septembre 2026, auditeur C.

- **Base :** `f44a8db03`, reçu [`g4_tower_r22_20260926`](../receipts/g4_tower_r22_20260926/README.md), paquet `43c5ad25`.
- **Cadre :** `exploration_v9_hors_registre`, `public_status=not_claimed`.
- **GCP :** non utilisé.
- **Méthode :** six pistes, 29 agents au total.
  - Les pistes : le reçu, l'attribution appariée R21 → R22, trois
    revues de code, le contrôle des corrections.
  - Les 22 constats des revues et du contrôle des corrections ont été
    soumis à un ou deux sceptiques. Le reçu et l'attribution ne l'ont
    pas été.
  - Deux vérificateurs ont ensuite recalculé chaque chiffre et contrôlé
    chaque affirmation de cette note.
- **Annexe :** sorties structurées et scripts dans
  [`c_r22_20260926/`](c_r22_20260926/README.md).

## Verdict

- **Le reçu tient.**
  - Intégrité : `SHA256SUMS` passe à 560/560.
  - Les 36 cas sont `complete_relative`.
  - Les douze épingles sont reproduites dans chaque cas, dont les 30
    cas scellés.
  - Les 24 comparaisons sont recalculées égales, jusqu'à `tower_work`.
  - L'échantillon scellé vaut exactement `ceil(balls/64)`.
  - Le recensement précoce couvre toutes les clés q2, sans repli.
  - Le bassin n'alloue rien pendant les appels.
- **Les trois séries ne changent pas l'objet.**
  - **Sceau (R-29).** Aucun chemin n'amène au sceau un catalogue que la
    passe 1 complète refuserait. Le prédicat de positivité de la chaîne
    concorde avec celui de la tour sur 5,99 M supports permutés.
  - **Recensement précoce.** Aucune course, et le même objet.
  - **Bassin épinglé.** Les octets sont identiques, levier actif ou non.
  - Les sceptiques confirment 17 constats, tous de gravité basse. Douze
    sont nouveaux et numérotés (§ 5) ; les autres complètent l'état de
    mes constats R21.
- **Deux de mes projections étaient fausses** (§ 2) :
  - le recensement précoce rapporte 2 à 3 fois moins que prévu ;
  - le bassin rapporte 2 fois plus dans la chaîne, mais déplace son coût
    hors de la chaîne.
- **Ma recommandation R21 « viser A(Kmax) » était erronée**, et ses
    propres données le montraient déjà (§ 3). Je retire mon conseil de
    « retirer "ordres bas" » du plan : il faut alléger la phase A de
    **tous** les ordres.
- **Avec sol, 1 s à K5** n'est atteint dans aucun modèle en cumulant
  les seules étapes du plan. Il ne l'est qu'au bout optimiste de **tous**
  les leviers réunis (§ 4) :
  - b01 : 0,90 à 1,17 s ;
  - b00 : 1,00 à 1,28 s ;
  - b02 : 1,06 à 1,34 s.

  Plusieurs de ces leviers sont hors du plan, et une phase A parallèle
  n'est pas prouvée.

## 1. Reçu : ce qui est à corriger

Les chiffres principaux du README et du canal sont conformes :
- K5 : 0,926 / 0,940, 0,760 et 0,983 s ;
- K10 : 2,96 / 2,27 / 2,89 s ;
- avec sol : 1,81 à 2,03 s à K5, 5,31 à 6,01 s à K10 ;
- sceau : validation 50 → 34 ms et 175 → 116 ms ;
- bassin : transfert 30,6 → 4,8 ms et 148,5 → 14,1 ms ;
- tour à 00/K5 : 289 ms, dont une fenêtre 38 + 148 ms.

| affirmation | constat |
| --- | --- |
| les trois leviers : −62 à −82 ms à K5, −224 à −266 ms à K10, contre le bras `gpu_r21` « apparié » | seule la répétition 1 est entrelacée ; les paires entrelacées donnent −62 à −68 ms et −224 à −232 ms ; les bornes hautes viennent de la répétition 0, jouée 60 à 62 s plus tôt ; et 265,3, pas 266 |
| recensement précoce : « 0 + 505 » à K10, recensement restant −4 ms à K5 et −17 ms à K10 | 505 ne correspond à aucune ligne (499,6 et 509,4 ; moyenne 504,5). Le −4 vient de la répétition 0, le −17 de la moyenne. Sur une même base : −1,2 à −4,6 ms à K5, dans le bruit des répétitions (3,4 ms), et −12 à −22 ms à K10. Le « 116 » du sceau à K10 mélange lui aussi les bases (115,7 et 117,5 ; moyenne 116,6) |
| ablations d'un seul levier « en paires » | un seul passage chacune, ni répété ni entrelacé. Sceau et bassin tiennent quand même, car leurs minuteries internes bougent bien plus que l'écart des répétitions |
| côté q2 : « 66 à 78 ms » et « 99 à 122 ms » sous « à 08/000000 » | les extrêmes viennent de 000100 et 000200 ; à 08/000000, c'est 70,5–74,8 ms et 110,2–115,3 ms |
| réservation du bassin : « 38 à 40 ms » à K5 | 38,4 à 40,6 ms |
| « TERMINATED relu en lecture seule » | aucune pièce du reçu ne contient cette relecture (même remarque que pour R21) |
| « K5 sous la seconde sur les trois trames » | 08/000200 : un seul passage à 0,983 s, 17 ms sous le seuil, pour un écart de 14 ms entre les deux répétitions de 00 |
| « reste à 08/000200/K5 » | 31 ms manquent au total (`gen_index` 13,7, préparation 1, reste 16,7), et 91 ms de q3/q4 ne sont pas ventilés (glu non chronométrée 86, queue 4,5) |
| `gpu_r21` présenté comme le bras R21 | les contrôles R-29 du recensement sont inconditionnels (`tower_chain.cpp:867`, `:901`) ; d'une session à l'autre (indicatif), le recensement coûte plus dans les bras sans recensement précoce, jumeaux moteur compris : +5 à +9 ms à K5 sans sol, +18 à +28 ms à K10, +11 à +14 ms au brut K5, +31 à +47 ms au brut K10, soit 3 à 9 % ; la projection était d'environ 1 ms ; sans bras qui coupe ces contrôles, on ne peut pas les séparer de la dérive |

## 2. Leviers mesurés et correction de mes projections

À 08/000000, on peut écrire −51,7 ms à K5 comme −72,2 ms pour les trois
leviers plus +20,5 ms de dérive et de code. À K10, −210,4 ms fait
−244,7 + 34,3 ms. C'est une identité arithmétique, et ces moyennes
incluent la répétition 0 non entrelacée. La répétition 1, seule
entrelacée, donne −65 ms pour les leviers à K5 et −228 ms à K10.

| levier (phases visées) | 00 K5 | 00 K10 | brut K5, R21 → R22 |
| --- | ---: | ---: | ---: |
| bassin épinglé (transfert et hôte des voies) | −28 ms | −149 ms | −52 à −63 ms |
| sceau (validation : passe 1 ramenée à 1/64) | −16 ms | −59 ms | −20 à −28 ms |
| recensement précoce (index de la tour, recensement) | −13 ms (index −10, recensement −3) | −27 ms (index −10, recensement −17) | −31 à −34 ms (index −31, recensement −0 à −4, dérive comprise) |

### 2.1 Le recensement précoce

- **Ma projection :** −95 à −130 ms au brut K5, en supposant que le coût
  des clés q2 suit leur part des clés (43 à 46 %).
- **Mesuré :**
  - R21 → R22 sur le bras GPU, dérive comprise : −31 à −34 ms ;
  - contre le jumeau moteur de R22 : −45 à −48 ms, dont l'index −31 et
    le recensement −14 à −17. Cette seconde base n'est pas une ablation
    propre, car le jumeau diffère aussi par d'autres leviers.
- **L'écart :** la projection était environ 2 à 3 fois trop optimiste,
  et 5,6 à 6,8 fois sur le recensement seul.

**Pourquoi.** Deux causes plausibles se combinent. Les données ne
permettent pas de les séparer : octets du catalogue et nœuds parcourus
sont colinéaires.

1. **Les clés q2 sont les moins chères à parcourir.** Ce sont de petites
   boules diamétrales, à coquille régulière de 2 sites et à positivité
   triviale. C'est l'explication du développeur (README R22).
2. **Le travail par clé à l'intérieur de l'horloge.** Il ne dépend pas
   du levier.
   - `std::vector<tower::BallData> balls(unique)` (`tower_chain.cpp:1805`)
     initialise 224 octets par clé, en série sur le fil principal.
   - La voie précoce ajoute une recopie **parallèle** des boules q2
     (`:1837–1844`), puis une libération **en série** (`:1846`).
   - La libération des présentations (`:1894`) est elle aussi dans
     l'horloge.

Le coût par clé du recensement vaut 79 à 82 ns à K5 et 89 à 95 ns à K10
sans le levier, contre 74 à 80 ns et 87 à 93 ns avec. Chaque clé q2
déplacée ne fait gagner que 7 à 16 ns.

Ce point d'initialisation n'est pas nouveau. Je l'avais relevé en L4-04
et au constat 10 de mon audit du 23 septembre, et B l'avait noté le 22
([résidence de la chaîne](CONTRE_AUDIT_B_RESIDENCE_CHAINE_20260922.md)).
Il manque toujours la mesure : la sonde v29 doit publier des sous-chronos
du recensement (allocation et initialisation, parcours, recopie q2,
libérations) et les nœuds parcourus par arité. Le levier qui s'en déduit
consiste à initialiser le tableau final hors de l'horloge, sur les fils
inactifs pendant les appels de l'appareil, ou à recenser les clés q2
directement dans le tableau final.

### 2.2 Le bassin épinglé

- **Ma projection :** −20 à −40 ms au brut K5.
- **Mesuré dans la chaîne :** −52 à −63 ms.

La réservation, déclarée au README, est payée hors de `chain_total`, dans
la session d'appareil : 38 à 41 ms à K5 (256 Mo), 190 à 201 ms à K10
(1,28 Go). Le temps hors de toute minuterie augmente aussi avec le
bassin : environ +80 ms à K10 et +20 ms à K5. Ce surplus est mesuré ;
sa cause probable, la libération du bassin à la sortie, n'est pas
prouvée.

Par processus d'une seule trame, à 08/000000 :

| K | mur externe, avec le bassin | sans le bassin | écart |
| ---: | ---: | ---: | --- |
| 5 | 1,718 / 1,772 s | 1,725 s | neutre |
| 10 | 6,029 / 6,036 s | 5,885 s | +144 à +151 ms ; +104 à +151 ms imputables au bassin, une fois corrigé le contexte d'appareil, bimodal (122–128 ou 160–166 ms) |

Les trois leviers ensemble allongent le mur externe de 37 à 49 ms à
00/K10, contre `gpu_r21`. Le gain du bassin n'existe donc que dans un
processus résident qui sert plusieurs trames par réservation. C'est un
**amendement de frontière** au sens de R-31 : publier le mur externe par
bras, et mesurer la boucle multi-trames persistante que B demande depuis
R21.

## 3. Tour : correction de ma recommandation R21

Le modèle de fenêtre `max_K (prêt(K) + A(K))`, où la phase 0 va de
l'ordre du haut vers le bas, tient toujours sur R22 : à 0,2 ms près à
K5, à 1,2 ms près à K10.

| cas | A(Kmax) | avance de A(Kmax) sur la fin de K4 | fenêtre | A(Kmax) → 0 | toutes les A ÷2 | toutes les A ÷4 | toutes les A ÷4 et phase 0 ÷2 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 00 K5 | 147,6 | 17,7 | 185,8 | 167,9 | 115,8 | 96,7 | 57,9 |
| 02 K5 | 155,9 | 19,7 | 190,6 | 170,7 | 114,0 | 93,2 | 57,0 |
| b00 K5 | 342,7 | 53,0 | 414,7 | 361,4 | 243,1 | 192,4 | 121,6 |
| b02 K5 | 363,0 | 49,8 | 431,8 | 381,9 | 250,1 | 196,5 | 125,1 |

**À K5.**
- Tout levier portant sur A(Kmax) seule, allègement et parallélisation
  compris, est plafonné à son avance sur la fin de K4 : 14 à 21 ms sans
  sol, 37 à 53 ms avec sol. Mon tableau R21 le montrait déjà : K4
  finissait à 390 ms sur 436 à b02.
- Il faut alléger la phase A de **tous** les ordres. Divisée par 2, elle
  rapporte −157 à −182 ms au brut ; divisée par 4, −204 à −235 ms.
- Le plancher devient alors la phase 0 (153 à 163 ms au brut), qu'il
  faut accélérer à son tour. E6 vaut au plus la résolution de static(5),
  soit 41 à 46 ms au brut.

**À K10.**
- A(10) ne borne pas aujourd'hui. Les ordres qui bornent sont K7, K6 et
  K7 sans sol (à 02, K6 et K7 sont à égalité : 0,1 ms), et K8, K8 et K9
  avec sol.
- La phase 0, séquentielle d'un ordre à l'autre, fait 71 à 82 % de la
  fenêtre.
- **Sans sol**, diviser la phase 0 par 2 rapporte le plus : −141 à
  −221 ms, contre −82 à −141 ms pour les phases A.
- **Avec sol**, diviser toutes les phases A par 2 rapporte davantage :
  −257 à −321 ms, contre −189 à −229 ms pour la phase 0.
- Dès que la phase 0 est divisée par 2, A(10) borne dans 6 cas K10 sur
  7. Les deux leviers vont donc ensemble.

## 4. Budget d'une seconde au brut K5, révisé (projection)

| poste | R22 b02 | moyenne b00–b02 | ce que R22 a changé (moyenne) | levier restant (projection) |
| --- | ---: | ---: | --- | --- |
| fenêtre de la tour | 432 | 411 | −4 | phase A de tous les ordres, puis phase 0 : −157 à −235 |
| glu hôte de q3/q4 | 308 | 301 | −4 | parallèle et recouverte : −81 à −199 |
| front q3/q4 | 256 | 261 | −8 (dérive) | tâche traînarde scindée : −61 à −102 ; au-delà, front sur l'appareil |
| recensement | 229 | 215 | −2 | travail par clé hors de l'horloge : −60 à −130, si sa part est de 30 à 60 % (non mesurée) |
| appels d'appareil | 397 | 380 | −50 (bassin) | noyaux et appels en pipeline : −50 à −100 |
| queue de la tour | 178 | 154 | +3 | E5 : −70 à −95 |
| validation | 80 | 74 | −25 (sceau) | tri radix des niveaux : −15 à −25 |
| fusion, index, préparation, reste | 148 | 146 | −31 (index) | `gen_index` parallèle, libérations différées : −30 à −48 |
| **chaîne** | **2 028** | **1 943** | **−120** | — |

**Modèles :**
- phases A : fenêtre du § 3 ;
- front : chaque ouvrier finit à la somme CPU divisée par 48 ;
- glu : la partie non chronométrée divisée par 2, jusqu'à ÷4 avec la
  moitié de l'hôte des appels recouverte ;
- postes 4 à 8 : les fourchettes du tableau.

**Paliers, trame par trame (s) :**

| paliers cumulés | b00 | b01 | b02 |
| --- | ---: | ---: | ---: |
| R22 | 1,99 | 1,81 | 2,03 |
| les trois étapes annoncées (A(Kmax), glu, front) | 1,64–1,75 | 1,53–1,63 | 1,71–1,82 |
| en remplaçant A(Kmax) par toutes les phases A ÷2 | 1,52–1,63 | 1,41–1,51 | 1,58–1,69 |
| plus les postes 4 à 8 | 1,12–1,40 | 1,01–1,29 | 1,18–1,47 |
| plus les phases A ÷4 et la phase 0 ÷2 | 1,00–1,28 | 0,90–1,17 | 1,06–1,34 |

**Ce qu'on en tire :**
- 1 s au brut K5 n'apparaît qu'au dernier palier, et seulement au bout
  optimiste : b01 passe dessous, b00 l'atteint, b02 reste au-dessus.
- Faire travailler recensement et tour pendant les appels de l'appareil
  serait une autre voie. Mais c'est une **hypothèse** qui demande une
  analyse de dépendances : la tour consomme le catalogue scellé complet,
  et dans la chaîne les appels (`tower_chain.cpp:1497`) précèdent la
  fusion (`:1737`) et le recensement (`:1804`).
- Pendant les appels, ce sont les fils hôte qui attendent (380 à 397 ms
  au brut) : c'est la ressource inactive à exploiter.
- Front et tour sur l'appareil restent deux leviers non chiffrés.

## 5. Revue des trois séries et des corrections

**Sceau (R-29), ce qui tient :**
- Seul le recensement écrit les boules.
- Chaque contrôle de la passe 1 a son équivalent exact dans la chaîne.
- Le type de sceau a un constructeur privé, sans copie ni déplacement.
- L'échantillon (indice 0 modulo 64 de l'ordre des clés) ne dépend pas
  du nombre de fils.
- L'ordre des refus suit la plus petite clé fautive.
- Le résidu déclaré, une écriture en mémoire après le recensement, n'est
  pas atteignable par le code actuel.

**Recensement précoce, ce qui tient :**
- Aucun état partagé modifiable pendant le recouvrement.
- Le même représentant en arité 2.
- L'erreur rendue est toujours celle de la plus petite clé.

**Bassin épinglé, ce qui tient :**
- Le bail vit au-delà de la conversion.
- La copie est synchrone.
- Un bloc prêté n'est jamais agrandi.
- `check_lanes_batch` refuserait une copie courte ou périmée.

### Constats confirmés, gravité basse

| n° | constat | lieu | correction proposée |
| --- | --- | --- | --- |
| R22-1 | le prédicat q3 de la chaîne est une copie de celui de la tour, et la porte n'exerce qu'une position de sommet obtus (la troisième) | `tower_chain.cpp:721` | appeler le prédicat de la tour, ou forger l'angle obtus aux trois positions |
| R22-2 | le lecteur accepte `declared_support_checks = 0` sous le sceau et ne vérifie pas la raison `…_sealed_in_process_census` | `tower_worker_v9.py:917` | borne basse ; raison exacte selon le levier |
| R22-3 | la phrase « une faute systématique est détectée », issue de ma proposition et que B avait déjà nuancée, dépend de la fréquence et de la place de la faute : l'échantillon est un résidu fixe, non stratifié ; une faute uniforme sur m boules échappe avec probabilité (63/64)^m, et une faute périodique qui évite 0 modulo 64 échappe toujours | `full_ball_tower.hpp:201` | le dire ; stratifier par classe de coquille ou décaler le résidu si l'on veut davantage |
| R22-4 | nouvelle instance de mon L4-03 : pas de point d'essai du côté q2, donc une faute plantée sur un groupe q2 est ignorée sous le levier ; le refus `keys_differ` n'a pas de fixture négative ; le levier change alors la raison du refus, sans taire le refus lui-même | `tower_chain.cpp:1833` | point d'essai dans `run_early_census`, fixture négative |
| R22-5 | même classe que mon constat R21 11 : un repli par trame (`early_census_keys = 0`) est accepté par le lecteur et n'apparaît pas dans `SUMMARY.json` (écrit par l'assembleur hors dépôt), bien qu'il soit lisible dans chaque sortie hachée | `tower_worker_v9.py:995` | exiger l'absence de repli sur les cas de production, ou publier le champ |
| R22-6 | le gain du bassin est déplacé hors de `chain_total` : neutre à K5, perte à K10 par processus d'une trame (§ 2.2) | README R22 | publier le mur externe par bras ; boucle multi-trames |
| R22-7 | le README crédite déjà le bassin (« copie divisée par 14 »), alors que la provenance exige « avant tout crédit » de lancer sur G4 l'étape B qui l'alloue, la croissance en cours d'appel et les portes `--device` ; rien de cela n'a tourné | `PROVENANCE.md:1534` | lancer ces portes avant de maintenir le crédit |
| R22-8 | `lanes_pinned_reserve_records` publie le nombre demandé, pas le nombre réservé (échecs compris) | `tower_chain.cpp:187` | publier le résultat réel de `warm_up_lanes` |
| R22-9 | aucune porte ne voit un bail rendu avant la conversion | `tower_chain.cpp:464` | empoisonner le bloc rendu (build d'essai) et mutant de retour anticipé |
| R22-10 | correction du constat 2 sans le cas « erreur d'image sous une `Failure` d'ordre plus haut » ; le mutant qui survit à `--unwind` est exactement ma première proposition de R21 (relancer après la boucle des `Failure`), que le témoin ne suit pas ; le cas {lots K4, images_alloc K2} le tue, et le code corrigé passe (70 contrôles) | `order_failure_priority_gate.cpp:156` | ajouter ce septième cas |
| R22-11 | déjà signalé dans l'annexe R21 : le chemin d'échec de lancement des aides n'a pas de point d'échec ; sous double panne, l'erreur de dimensionnement des rows peut être écrasée (depuis `a70ad6c7a`) | `full_ball_tower.hpp:1003` | point d'échec au lancement des aides |
| R22-12 | la clause de préflight « recensement précoce sans repli » n'a pas de mutation d'autotest (les clauses sceau et bassin sont impliquées par d'autres contrôles) ; la clause C++ `steps.pinned = … && !where.empty()` n'est exercée par aucune porte | `tower_worker_v9.py:1113`, `tower_chain.cpp:535` | une mutation `early_census_keys=0` sur le préflight ; un appel vide sous le levier |

**État de mes constats R21 (le 5 est un constat de B) :**
- **Fermés :** 2 (hors le cas R22-10), 3 (garde par délai CTest
  seulement), 8 et 12.
- **En partie fermés :**
  - 9 : indexé par (trame, K), mais un cas sans épingle est encore
    accepté (`tower_worker_v9.py:1229`), et le commentaire est périmé ;
  - 10 : `populations_by_k` et `images_own_by_k` ne sont toujours pas
    bornés ;
  - 11 : `runner_threads` n'est pas contrôlé hors recouvrement, et
    `helper_threads` et `pool_jobs` ne sont pas bornés. L'égalité
    `hashed_orders = K−1` était ma propre règle ; le repli à 2^31
    qu'elle ignore n'est pas atteignable en production.
- **Ouverts, sources inchangées :** 1, 4, 5, 6, 7 et 13. Ce n'étaient
  pas les priorités demandées.

Constats réfutés, pour mémoire :
- sécurité mémoire du résidu déclaré ;
- `friend` redéfinissable ;
- représentant q2 ;
- repli sur `bad_alloc` ;
- porte du recensement précoce sans sceau (couverte par le contrat de
  sonde).

## 6. Recommandations au développeur

1. **Tour.**
   - À K5 : alléger la phase A de **tous les ordres**, avec une mesure
     par ordre. Ne pas investir dans A(Kmax) seule.
   - À K10 : phase 0 d'abord sans sol, phases A d'abord avec sol ; les
     deux ensemble ensuite.
2. **Recensement.** Sous-chronos d'abord (sonde v29). Puis sortir de
   l'horloge l'initialisation par clé, et la recopie et la libération
   de la voie précoce.
3. **Frontière.** Publier le mur externe par bras, et mesurer la boucle
   multi-trames avant de créditer le bassin. Ajouter un bras sans les
   contrôles R-29 du recensement, pour chiffrer leur coût.
4. **Brut K5.** Avec les leviers chiffrés, 1 s n'est atteint qu'au bout
   optimiste et pas sur b02. Deux pistes à instruire : l'analyse de
   dépendances d'un recouvrement recensement/tour avec les appels de
   l'appareil, et le front sur l'appareil.
5. **Portes.** R22-10, R22-2 et le reste du constat 9 d'abord, puis
   R22-4, R22-5 et R22-1.
