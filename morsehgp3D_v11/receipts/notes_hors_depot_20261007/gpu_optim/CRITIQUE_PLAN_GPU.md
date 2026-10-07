# Critique adverse de `PLAN_GPU.md`

Rédigée le 6 octobre 2026, de 01 h 37 à 01 h 50 UTC (`date -u`). Lecture seule au commit `cf5da0e91` (origin/main,
moteur identique à `df904711a`). Aucun build, aucun commit. **GCP non utilisé.**

```text
phase=exploration_v11_hors_registre
backend=cpu_reference (référence) ; cuda_g4 (voie de banc relue)
profile=quantized_u21_input_only
public_status=not_claimed
```

Étiquettes : **M-G4** = mesure lue dans un reçu G4 ; **M-loc** = mesure sur le codespace ; **E** = estimation. Toute
valeur ci-dessous a été relue dans sa source, citée.

## 0. Verdict

Le plan est honnête sur sa conclusion : 100 ms n'est pas promis. Mais **son classement repose sur une
décomposition du domaine fausse, et sur deux gains non étayés**. Une fois ces trois points corrigés, le chemin
vers 100 ms n'atteint plus « 75 à 125 ms ». Il donne plutôt 150 à 220 ms (E), sauf si l'on découvre une cause
encore inconnue au coût par pas.

Les cinq défauts majeurs :

1. **Le levier n°1 (L2) attaque probablement le mauvais coût.** Le « parcours » de 84 à 106 ms n'est pas du
   filtre G1 : deux mesures le contredisent (§ 1.1).
2. **Le budget du domaine compte deux fois 33 à 40 ms** : le préambule est le `prefix`, et le résidu contient
   déjà `compact`, `level_scan` et `assembly` (§ 1.2).
3. **L1 est gonflé d'un facteur 2 à 3.** Les sous-leviers sourcés donnent −10 à −37 ms, pas −50 à −80. Et la
   cible de 188 ns par pas s'appuie sur un census en flottant que l'audit déclare non transposable (§ 1.3).
4. **Le gain SHA-NI de −1,4 s ignore le `fsync`**, qui est mesuré dans `write` (§ 2.4).
5. **Le protocole de 7 prises à W48 ne peut pas trancher** la plupart des gains annoncés (−3 à −30 ms) : ses
   propres sources l'interdisent (§ 5).

Deux leviers CPU mesurés ou sourcés manquent au classement :

- la feuille J3 **sur CPU** (×1,37 mesuré sur la v10) ;
- le surcoût par nœud du parcours, que la v10 n'avait pas.

Un levier « gratuit » est mal classé : les feuilles de 24 à K5 font tomber le parcours de 94–110 à 29–38 ms
(§ 4).

## 1. Points bloquants

### 1.1 L2 vise le filtre G1, mais le coût du « parcours » ne suit pas les tests G1

Le plan, comme `carte_catalogue.md` § 2.2, appelle « parcours » la passe unique en mode GPU, qui vaut 84 à 106 ms
(M-G4, `claudegpu6`). Il en déduit les feuilles CPU par différence : 45 à 54 ms. Ce chiffre est ensuite l'appui
de L2 (−100 à −150 ms) et de l'argument d'Amdahl contre la voie des feuilles. Trois mesures le contredisent.

1. **Profil CPU à W1** (M, AB7 `perf_new_self.stdout`, relu dans
   `audit_transpositions/plans/PLAN_VITESSE_100MS.md` l. 117-119 et `verif/v10_moteur.md` l. 90-94).
   - La fonction `filter` (G1) pèse 8,0 % du CPU du processus.
   - La feuille (`extend`, `enumerate_leaf`, `center_line_meets`, `num::side`, etc.) en pèse environ 45 %.
   - La feuille vaut donc **80 à 85 % de la passe unique**, et le filtre 15 à 20 %.
   - À W1, la passe unique de ng00 vaut 4 673 ms (AB). Le filtre G1 représente donc environ 0,7 s de CPU, soit
     environ 25 à 30 ms à W48 avec l'accélération ×28,7 de l'étage (E).
2. **Feuilles de 24 à K5** (M-G4, `claudegpu4`, `carte_gpu_existant.md` § 2.7).
   - Le « parcours » en mode GPU tombe de 94–110 à **29–38 ms** (÷3).
   - Pendant ce temps, `filter_tests` ne baisse que de 379 M à 248 M (÷1,5), alors que les nœuds passent de 783 k
     à 272 k (÷2,9).
   - Le temps suit donc le nombre de nœuds et de feuilles (allocation d'un `Buffer` par nœud avec CAS partagé,
     construction et mise en file des `LeafJob`), et non les tests G1.
3. **v10, même travail** (M-G4, protocoles différents, `AUDIT_TRANSPOSITIONS_V11.md` § 2.1-2.2).
   - Les boîtes de la v10, feuilles comprises, coûtaient 106,7 / 84,8 / 101,5 ms à W48, autant que le seul
     « parcours » de la v11.
   - La v10 n'avait « aucune allocation ni opération atomique partagée par nœud ».
   - Elle avait un grain de 19 482 tâches, contre au plus 1 024 en v11.

**Conséquence.**

- Les feuilles CPU pèsent vraisemblablement 100 à 120 ms à W48, et non 45 à 54 (E, à mesurer).
- Le vrai filtre G1 pèse 25 à 40 ms.
- L2 ne peut retirer au mieux que le filtre et l'enveloppe, soit environ −25 à −40 ms, et non −100 à −150. Le reste
  du gain imputé à L2 vient en fait de L5 (feuilles plus rapides).
- Le « chrono interne du parcours » de T1 doit donc précéder tout classement. Le seuil c(64) ≤ 20 % ne tranche
  rien : il mesure la répartition des tests, pas leur coût.

Le plan ne cite pas cette contradiction entre deux sources mesurées. **Elle doit être levée avant de garder L2 au
rang 1.**

### 1.2 Le budget du domaine compte deux fois 33 à 40 ms

Le tableau du § 2 du plan additionne cinq postes :

- parcours : 84 à 106 ms ;
- feuilles : 45 à 54 ms ;
- « préambule + prefix » : 20 + 20 ms ;
- `sort` + `level_scan` + `assembly` + `compact` : 25 à 27 ms ;
- « résidu non attribué » : 23 à 28 ms.

Ces postes totalisent **217 à 255 ms, pour un domaine mesuré à 180–231 ms.** Deux postes sont comptés deux fois :

- le « préambule » **est** `prefix_ns` (`carte_catalogue.md` l. 24, « Préambule … `prefix_ns` »). Il compte une
  seule fois, pour 17 à 22 ms ;
- le résidu de `claudegpu6` est calculé comme `domain − passe unique − préambule − tri`, et il **contient**
  `compact`, `level_scan` et `assembly` (`carte_catalogue.md` l. 62). Ces trois postes (12 à 15 ms) sont donc
  comptés une fois dans la ligne `sort+…` et une seconde fois dans le résidu.

Le résidu réel se recalcule à partir de l'AB (`carte_profil_etages.md` § 1.3) : 221 − (163 + 20,3 + 11,6 + 4,5 +
4,4 + 4,6) = **12,6 ms** (ng00), 12,3 ms (ng01) et 14 ms (ng02).

Les petits étages du domaine pèsent donc en tout **52 à 63 ms**, et non 88 à 95. Conséquences :

- **L6 passe de −25/−50 à environ −15/−35 ms (E).** La carte O5 donne elle-même −10 à −15 ms pour le préambule.
- Le plan annonce aussi tantôt « 85–120 ms » (§ 0.4), tantôt « 75–125 ms » (§ 2 et résumé) pour la même
  estimation.

### 1.3 L1 : le gain de −50 à −80 ms ne repose sur aucun mécanisme identifié

La fiche O3 de `carte_forets.md` (l. 288-293) chiffre les deux sous-leviers connus : **V3 à −5/−25 ms, V7 à
−5/−12 ms**, soit −10 à −37 ms en tout. Le plan retient −50 à −80 ms en supposant que le coût par pas « revient
vers la v10 ». Cette hypothèse a trois faiblesses.

- La cause de l'écart (188 contre 596–704 ns par pas) est **non attribuée**. Le plan le dit lui-même (« cause de
  l'écart inconnue ») ; la carte aussi (§ 6, question 2).
- Le principal mécanisme de la v10 connu pour le census est l'arbre k-d serré : environ 1,06 µs par requête contre
  environ 3,1 µs (E). L'audit le déclare **« non transposable tel quel : parcours en flottant avec une marge
  figée pour u18 (`kMargin = 0,02`) »** (`AUDIT_TRANSPOSITIONS_V11.md` § 2.3). Le mémo partagé ne retirait que 7 %
  des pas.
- Le census ne pèse qu'environ 9 % du CPU du processus à W1 (`power_bound_signs` 5,0 + `bound_terms` 2,0 + `query`
  2,1, AB7). Même un census gratuit ne divise pas R par 3.

**Conséquences.**

- R réaliste après V3 et V7 : environ 80 à 107 ms sur ng00 (E), contre 30 à 40 dans le plan.
- `tree` réaliste après L1, L3 et L8 : environ 95 à 130 ms (E). **L'étage `tree` seul reste au-dessus de 100 ms.**
- Le chemin du § 2 suppose R ≤ 40 ms. Tant qu'aucune attribution par route n'existe, ce n'est pas une estimation :
  c'est une **condition**. Le plan doit l'écrire ainsi.

Repère utile : la v10 n'a jamais tenu 100 ms (FULL K5 : 252 / 204 / 254 ms, M-G4, troisième passe chaude). Le
plan vise donc un facteur 2,5 au-delà du meilleur moteur jamais mesuré. Ce n'est pas un retour à un état connu.

## 2. Chiffres faux, mal lus ou mal étiquetés

| # | Plan | Source relue | Correction |
| --- | --- | --- | --- |
| 2.1 | « préambule + prefix ≈ 20 + 20 » ; résidu 23–28 en plus de `compact`, `level_scan` et `assembly` | `carte_catalogue.md` l. 24 et l. 62 | double compte (§ 1.2) ; résidu vrai 12 à 14 ms (AB) |
| 2.2 | « feuilles CPU 45–54 ms (dérivé M-G4) » | `gpu_g4/README.md` : « 40 à 63 ms de feuilles à 48 fils » ; s6 ng01 : 122 − 95 = 27 ms ; profil W1 : feuille ≈ 80–85 % de la passe unique | différence de médianes entre modes, d'un autre commit (`22a6af6aa`), contredite par le profil (§ 1.1) ; à étiqueter « dérivé, contesté » |
| 2.3 | « voie GPU : +30 à +50 ms de mur à K5 » | `gpu_g4/README.md` | vrai à chaud (meilleure passe GPU contre meilleure passe CPU) ; **à froid, la perte est de +49 à +86 ms** (ng01 : 380,4 contre 294,1) |
| 2.4 | « SHA-NI : −1,4 s sur 2,16 s de write `full` » | `writer.cpp` l. 130 : `fsync` avant `fclose`, compté dans `write` ; disque à 290 Mo/s (`deploy.sh`) | voir § 2.4 ci-dessous |
| 2.5 | « K10 : domain 1,74 s → 0,83–0,90 s (M-G4 sonde) » | `carte_catalogue.md` O7 : « les prises ne sont pas appariées » | 1,74 s vient du CLI (`claudefinmesure`) et 0,83–0,90 s de la sonde (`claudegpu6`), deux commits et deux processus différents ; seul le −24 à −27 % de mur (`claudediag1`) est apparié |
| 2.6 | « médianes chaudes » de FIN, prises comme référence d'un régime résident | `sorties_g4.json` : `"cold": "premier appel de la configuration, cache non vide"` ; chaque prise est un **processus CLI neuf** | la référence est un processus neuf ; la cible (résident) n'est pas le même régime (§ 3.1) |
| 2.7 | décomposition du `tree` (R, queue) appliquée aux prises de FIN | phases de `claudediag1` (binaire `e49ea4690`, sonde, `forest_ns`) | quantité et binaire différents : la médiane FIN ng01 (139,2 ms) dépasse le maximum de `forest_ns` de diag1 (121,9 ms) ; celle de ng02 (150,7 ms) est sous sa médiane (173,5 ms). La queue par trame ne se transporte pas |
| 2.8 | « R stable à ±3 ms » | carte : R = 83,4 à 120,4 ms sur 60 prises, par trame 115,7–120,4 / 84,2–86,9 / 94,5–97,6 | exact par trame, mais pour 15 prises d'un seul binaire de diag1 |

Les autres chiffres relus sont exacts :

- 386 / 319 / 382 ms : somme des médianes de `domain` et de `tree` (221,6 + 163,6 ; 179,9 + 139,2 ; 230,5 +
  150,7) ;
- contexte CUDA de 78 ms, et 136 à 237 ms dans le fil ;
- 3,29 fils actifs par warp, 168 registres, pile de 3,3 Kio ;
- 75,0 M droites contre 31,3 M ;
- CPU de P5 : 81 à 151 ms en pipeline, 35 à 46 ms à W1, 36 à 52 ms en étage séparé ;
- queue de 2,0 à 58,7 ms ;
- 4,80 M pas, et 596 à 704 ns par pas ;
- erreurs de documentation : `kScratchRecords = 128` et `kScratchPopulation = 1024` dans `leaf_batch.hpp`
  l. 64, contre « 32 enregistrements, 256 incidences » dans `CATALOGUE.md` l. 277 ; `setvbuf(nullptr, 64 Kio)`.

### 2.4 SHA-NI : le gain de −1,4 s ignore le `fsync`

- Le profil local qui donne « SHA = 78 % du write » est un profil **`SIGPROF` en temps CPU**. Il ne voit pas le
  temps bloqué dans `fsync`.
- Sur G4, le débit apparent de `write` est de 134 à 150 Mo/s, quelle que soit la taille. C'est exactement ce que
  donnent un SHA scalaire et un `fsync` à pleine bande passante mis bout à bout : 1/(1/250 + 1/290) ≈ 134 Mo/s
  (E sur des M-loc et un débit provisionné).
- La VM a beaucoup de mémoire, et la vidange de fond par défaut (30 s, 10 % de la RAM) ne démarre pas en 2 s. Le
  `fsync` vide donc vraisemblablement les 300 Mo, soit environ 1,04 s (plancher déjà écrit dans `carte_aval.md`).
- **Gain probable du SHA-NI : −0,8 à −1,0 s** (write `full` vers 1,1 à 1,3 s), et non −1,4 s, ce qui donnerait
  0,76 s, sous le plancher.
- `sync_file_range` (carte O7) en devient le complément nécessaire.
- T3 doit chronométrer `fsync` **avant** d'annoncer un gain.

## 3. Gains surestimés et sous-estimés

### 3.1 Le régime résident profite aussi au CPU, et le plan ne le compte pas

Le plan exige un processus résident pour le GPU, mais le compare à une référence CPU mesurée en **processus
neufs**. `gpu_g4/README.md` montre que le CPU gagne aussi à chaud, d'une passe à l'autre du même processus :

| Trame | CPU, médiane à froid | CPU, meilleure passe à chaud |
| --- | ---: | ---: |
| ng00 | 368,6 ms | 363,3 / 342,7 ms |
| ng01 | 294,1 ms | 274,6 ms |
| ng02 | 374,7 ms | 330,0 ms |

Le gain va de −5 à −45 ms (M-G4). Le biais « meilleure passe contre médiane » n'est pas retiré.

Il faut comparer à régime égal : CPU résident contre GPU résident. Sinon, le régime résident est compté au crédit
du GPU. Les arènes pré-touchées et les pages huge relèvent aussi du CPU : à W48, les fautes de page valent 3,1 % du
CPU et le verrou noyau 1,55 % (PROF1).

### 3.2 L2 et L5 dépendent de J3, et J3 n'existe pas

- Sans J3, les feuilles sur GPU coûtent 52 à 59 ms à chaud, plus l'assemblage des Level (5,6 ms) et la queue
  `fill` d'un seul warp (17,2 ms).
- Le domaine GPU de L2 resterait alors vers 110 à 140 ms (E), et non 30 à 60.
- Le plan l'écrit (« J3 de ×3 à ×6 = E non étayée »). Il classe pourtant L2 premier avec −100 à −150 ms.

Deux contraintes manquent au plan :

- **les bornes J3 sont écrites pour u18 et une sous-maille T = 6** (`|lo|, |hi| < 2^25`). Elles sont à refaire
  en u21 et u24 (`AUDIT_TRANSPOSITIONS_V11.md` l. 195, `PLAN_VITESSE_100MS.md` l. 191) ;
- **la décision de l'utilisateur du 30 septembre** (grille u32 par paliers, u24 puis u32) borne la durée de vie
  des deux conceptions GPU :
  - le G1 en i64 exige `2B+5 ≤ 63`, soit B ≤ 29 ;
  - les prédicats de feuille en i128 exigent `5B+7 ≤ 127`, soit B ≤ 24 ;
  - à u32, ni L2 ni L5 ne tiennent sans voie Wide sur l'appareil.

### 3.3 L4 est plutôt sous-estimé à K5, à chaud

Recalcul sur les chiffres que le plan cite (`claudegpu6`, ng00) :

- passe unique CPU : 154 ms ;
- parcours en mode GPU : 106 ms ;
- exécuteur à chaud : 52 à 59 ms, donc inférieur au parcours.

Une fois recouvert, le coût est d'environ 106 ms + Level (5,6) + rassemblement (2,5) + dernière tranche et queue
`fill` (≤ 17), soit **−20 à −40 ms par rapport au CPU** (E), contre −10 à −20 dans le plan.

Il faut en revanche :

- réserver un fil matériel à l'hôte GPU : l'ouverture du contexte est déjà ralentie d'un facteur d'environ 2 en
  concurrence avec les 48 fils du parcours (136–183 contre 69–78 ms, `carte_gpu_existant.md` § 2.6) ;
- garder le parcours à 47 fils.

Le plan n'en parle pas. L4 reste soumis au § 1.1 : si le « parcours » mesuré en mode GPU contient le coût des
`LeafJob`, une partie de ce coût suit les feuilles.

### 3.4 L3 surestimé sur deux trames sur trois

Les médianes de queue dans diag1 valent 23,2 (ng00), 18,9 (ng01) et 51,1 ms (ng02). Le placement ne peut retirer
que la queue, moins un plancher. **−15 à −50 ms ne vaut que pour ng02.** Sur ng00 et ng01, on peut attendre au
plus −10 à −20 ms (E).

Deux points sur le placement lui-même :

- il ne doit pas regarder seulement les frères SMT : l'EPYC 9B45 (Zen 5) partage son L3 par CCD. Le consommateur P5
  et ses producteurs gagnent à partager un L3, et le plan ne parle que de SMT ;
- dans `claudediag1`, W48 libre bat W24 épinglé (`taskset 0-23`) d'un facteur 1,4 à 1,5. Retirer des fils SMT aux
  résolveurs coûte cher à R. Le bras « résolveurs −2 » doit publier R et pas seulement la queue.

## 4. Leviers CPU oubliés ou mal classés

| Levier | Source | Gain (E sauf mention) | Pourquoi il prime |
| --- | --- | --- | --- |
| **Surcoût par nœud du parcours** : `Buffer` sans CAS partagé (contrat R3 déjà accepté par les auditeurs), grain fin, `LeafJob` sans copie | `AUDIT_TRANSPOSITIONS_V11.md` § 2.3 ; v10 boîtes + feuilles = 85 à 107 ms à W48 (M-G4) ; § 1.1 ci-dessus | −30 à −60 ms de domaine | CPU, sans contexte ni transfert ; il attaque le coût dont le temps du « parcours » suit la variation (nœuds) |
| **J3 sur CPU** (feuille par étages, census par masques déjà porté, M3 non strict, E4, droite i64) | `PLAN_VITESSE_100MS.md` l. 191 : ×1,371 sur le CPU des boîtes v10 (M-loc, ABBA) ; incrément propre −20 à −40 ms | −20 à −40 ms | le plan ne cite J3 que comme forme GPU (L5). La version CPU est mesurée (v10) et ne dépend d'aucun contexte |
| **Feuilles de 24 à K5**, couplées à des feuilles plus rapides (J3 CPU, ou L4 avec J3 GPU) | `carte_gpu_existant.md` § 2.7 : parcours 29–38 ms au lieu de 94–110 (M-G4) | parcours −60 ms ; net à mesurer | le plan range « feuilles de 24 » dans « à ne pas faire » sur la seule mesure à un fil par feuille. L'effet sur le parcours est mesuré et gratuit. Le même dump à 16 et 24 n'est établi qu'à K10 : il est **à vérifier à K5** |
| **Processus résident côté CPU** : arènes réutilisées, pages pré-touchées ou huge | § 3.1 | −5 à −45 ms (M-G4, biais compris) | condition que le plan impose déjà au GPU |
| **Débit contre latence** | contrat « 100 ms » et capteur à 10 Hz | décision | si le contrat est un débit de trames, plusieurs trames concurrentes effacent les queues d'Amdahl (26 % du mur) ; L3, L6, L8 et L9 ne valent alors plus rien, et seuls la réduction du travail et le délestage comptent. Le plafond de travail reste d'environ 290 ms par trame sur CPU seul (9,05 s à W1 pour environ 31 cœurs-équivalents, E) |

Le plancher de travail est le cadre qui manque au plan. À W1, ng00 coûte 9,05 s de CPU (AB). Avec 24 cœurs
physiques et un gain SMT d'environ ×1,3, la machine fournit au mieux environ 31 cœurs-équivalents. **Aucune voie
CPU seule ne descend sous environ 290 ms sans réduire le travail d'un facteur d'au moins 2,9** (E). Les leviers à
classer en premier sont donc ceux qui retirent du travail (surcoût par nœud, J3 CPU, L1) ou qui le délestent
(GPU). Ceux qui ne font que paralléliser (L3, L6, L8, L9) viennent ensuite.

## 5. Mesurabilité : protocole et tranches

- **7 prises à W48 ne tranchent pas sous environ 30 ms.**
  - Le reçu `mesures_g4_ab8_diag1` dit : « le bras A/A seul s'écarte jusqu'à 9 % sur la passe unique ; à cinq
    paires (p bilatérale minimale 0,0625), aucun de ces changements n'est mesurable ».
  - `carte_forets.md` § 7 : « cinq paires ne tranchent pas sous environ 30–40 ms ».
  - `carte_catalogue.md` O1 demande **au moins dix paires**.
  - Le plan retient 7 prises et un repère A/A de 6 %. L6 (−15/−35), L8 (−5/−10), L9 (−3/−9), L4 à K5 et une
    partie de L3 tombent dans le bruit. Ils ne se jugent qu'à W1 apparié (A/A ±0,5 %), sur un chrono de
    sous-étage, ou avec au moins 10 à 15 paires.
  - La règle « trois trames dans le même sens » a une p de 0,125 au test des signes : c'est un repère, pas une
    décision.
- **T1 n'a pas besoin de G4 pour ce qui le décide.**
  - c(L), la part de chaque route par pas, les nœuds, les tests et les `LeafJob` sont des compteurs déterministes.
    Ils se mesurent en local (règle « exposants en local, temps sur la VM »).
  - Seul le placement L3 exige G4.
  - La sonde publie déjà `compact_ns`, `level_scan_ns`, `allocation_ns` et `assembly_ns`
    (`catalogue.hpp`, `CatalogueTimings`) : le résidu se mesure sans code.
- **L'instrumentation de T1 doit sortir du chemin chronométré.** 4,8 M pas avec deux lectures d'horloge chacun
  coûtent environ 0,1 à 0,2 s de CPU à W1 (E). Elle doit passer derrière un bit inactif dans les bras A/B, ou dans
  une passe séparée, comme Nsight.
- **T2 groupe L0 (sans gain) et L4.** Ce n'est mesurable que si L4 a son propre bras contre un L0 seul, ce qui
  ajoute un bras à la session.
- **Tailles d'intérêt.** Le plan ajoute uniform 8000, 16000 et 32000 : c'est bien. Aucune décision de pente ne doit
  pourtant s'appuyer sur les trois trames seules.

## 6. Exactitude

Les points relus dans le code tiennent.

- **Réservoir.** `boxes.cpp` l. 16-37 fait une insertion stricte (`distances[at-1] > distance`). Les ex aequo
  gardent donc l'ordre de la liste parente, et la clé (distance, position) du plan reproduit l'ordre.
- **Ordre d'émission et registre.** Le `CatalogueLedger` n'a que des sommes et des maxima (`catalogue.hpp`
  l. 55-68). `sort_comparisons`, qui dépend de l'ordre d'entrée du tri en tas, est dans `CatalogueTimings`, hors
  registre.
- **Tri.** Il départage par niveau, puis par support, puis par indice d'émission (`sort_indices.cpp` l. 22-31). Un
  parcours GPU en largeur ne change donc l'ordre qu'entre émissions de même support.
  - **À vérifier** avant L2 : les doublons de même boule issus de feuilles qui se recouvrent sont-ils identiques à
    l'octet, population comprise ? Si oui, l'ordre en largeur ne change rien. Sinon, il faut reproduire l'ordre du
    parcours en profondeur par une clé de chemin.
- **Exécuteur partagé CPU/GPU (L4).** Le GPU simule le cache J2 pour rendre les mêmes compteurs logiques que
  `leaf.cpp`. Une répartition dynamique selon l'horloge reste donc sans effet sur le registre. La porte de
  « répartition forcée » du plan est la bonne.
- **L7.** Les compteurs `ancestor_*` dépendent du chemin. Le plan le pose en décision, et le préalable d'inscription
  au registre des preuves est juste.

Ce qui manque au plan :

- une porte `--inject` qui force `unresolved` sur toutes les feuilles GPU (repli complet au même dump), exécutée
  **en natif sur G4**, et pas seulement en local sans GPU ;
- une porte de profil : refus explicite de la voie GPU au-delà de B = 24, avant calcul (§ 3.2).

## 7. Pièges de la VM ignorés

- **CMake 3.22.1** ne connaît pas le dialecte CUDA20 (`claudegpu1` en `failed_remote`). Les portes P1-P10 de L0
  doivent se construire avec cette version, et non avec celle du codespace.
- **Portes CTest en Python nu** : Python 3.10, sans numpy, sous `python3 -S`. Les portes GPU nouvelles doivent
  tenir dans ce cadre.
- **Première prise après démarrage de la VM** : `device_init` de 237 ms et prefetch de 370 ms. Elle doit sortir de
  l'échantillon froid, ou être déclarée.
- **Mode persistance et horloges du GPU** : `nvidia-smi -pm 1` et `-lgc` exigent root. Sans mode persistance, un
  « résident » qui reste inactif entre deux trames peut faire redescendre les horloges. À consigner.
- **Sessions de 4200 s, VM SPOT, `/tmp` vidé au redémarrage.** `sorties_g4.py` écrit sous `mkdtemp`, donc sur le
  disque de démarrage. Une préemption perd les résultats non rapatriés (`--recover`). La construction CUDA et le
  plan A/B doivent tenir dans une session, avec un rapatriement intermédiaire.
- **TSan exige `setarch -R`** sur cette lignée de noyaux (mémoire du projet). L1 et L3 demandent TSan.

## 8. Ordre proposé à la place

1. **T0 local, sans G4 (compteurs).**
   - Chrono interne du parcours : réservoir, filtre, enveloppe, allocation, `LeafJob`.
   - c(L), part de chaque route par pas, résidu du domaine.
   - Cette tranche décide entre le § 1.1 et le plan.
2. **T1 sur G4, A/B à 10 à 15 paires** :
   - placement L3, qui publie R et la queue ;
   - régime résident CPU contre processus neuf ;
   - feuilles de 24 à K5, identité vérifiée.
3. **Surcoût par nœud (R3) et J3 sur CPU** : retrait de travail, sans GPU.
4. **L0 + L4 avec un fil hôte réservé**, à régime résident égal des deux côtés.
5. **L1 (V3, V7)**, avec une cible déclarée −10 à −37 ms. La cible de 30 à 40 ms reste une condition tant que la
   répartition par route n'existe pas.
6. **L2 et J3 sur GPU** seulement si T0 confirme que le filtre G1 dépasse 50 ms à W48.
7. **SHA-NI avec `sync_file_range`**, hors contrat, annoncé à −0,8/−1,0 s.

## 9. Décisions à ajouter à celles du plan

1. **100 ms en latence par trame ou en débit à 10 Hz.** Le choix change l'ordre des leviers (§ 4).
2. **Profil numérique du contrat** (u21 aujourd'hui, u24 puis u32 selon la décision du 30 septembre). Il borne L2
   et L5 à B ≤ 24.
3. **Référence de comparaison** : résident contre résident.
