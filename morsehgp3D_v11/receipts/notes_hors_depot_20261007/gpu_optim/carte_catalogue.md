# Carte GPU de l'étage DOMAIN (catalogue) de la v11

Rédigé le 6 octobre 2026 à 01 h 23 UTC (`date -u`), en lecture seule, sur `origin/main` = `df904711a`.

```text
phase=exploration_v11_hors_registre
backend=cpu_reference (référence) ; cuda_g4 pour la voie des feuilles déjà mesurée
profile=quantized_u21_input_only
public_status=not_claimed
GCP non utilisé ; aucun build, aucun commit, aucune branche.
```

Légende des chiffres : **M-G4** = mesure G4 relue dans un reçu ; **M-loc** = mesure locale ; **E** = estimation
raisonnée, jamais présentée comme mesure.

## 1. Ce que contient l'étage `domain`

`prepare_full_domain` (`src/tower/full_domain.cpp`) = `build_catalogue` en passe unique, puis la table de
recherche `S* → BallIdx` (CAS parallèle). Le catalogue se décompose ainsi (`src/catalogue/single_pass.cpp`,
`assemble.cpp`) :

| Sous-étage | Champ de minutage | Nature du travail | Arithmétique |
| --- | --- | --- | --- |
| Préambule (frontière adaptative, ≤ 27 rondes, ≤ 1 024 tâches) | `prefix_ns` | filtre G1 des gros nœuds du haut ; rondes parallèles, mais le premier nœud a 40 000 sites et est filtré par un seul fil | i64 |
| Parcours des sous-arbres | dans `single_pass_ns` | par nœud : réservoir des 3K plus proches du centre de boîte, filtre G1 (dominance sur la fermeture), enveloppe, coupe ; un `Buffer` alloué par nœud | **i64 seulement** (`2B+5 ≤ 63`) |
| Feuilles (≤ 16 sites à K = 5) | dans `single_pass_ns` | dominance, graphe de paires, préfixes G3, droites J2 (`center_line_meets`), candidats q2/q3/q4, `center_in_box`, census par masques (lemme R) et puissance, S* canonique | i128 certifié (`5B+6`, `5B+7 ≤ 127`) ; q3 en Wide à B21/B24 hors certificat |
| Matérialisation des Level | dans `single_pass_ns` | `materialize()` après admission (q3/q4 différés) | Wide |
| Compactage, `level_scan`, assemblage CSR, table de recherche | résidu | copies, comparaisons de niveaux, CAS | Wide pour les comparaisons |
| Tri des émissions | `sort_ns` | tri indirect par clés F3/F4, repli exact | Wide au repli (`n < 2^204`, `d < 2^152`) |

L'`index` (`src/index/`, construit en 0,3–0,4 ms) et son `census` servent l'étage `tree` (descentes, MEB), pas
l'étage `domain` : ils relèvent de la carte de l'étage `tree`.

## 2. Mesures relues

### 2.1 Mur par étage, qualification finale (M-G4, `claudefinmesure`, source `38b76701b`, CLI `--sortie=full`)

Médiane de trois prises, K = 5, feuilles de 16 (paramètres de `src/api/compute.cpp`).

| Trame (sites) | W1 domain | W48 domain | W48 tree | domain / (domain + tree) à W48 | pic domain |
| --- | ---: | ---: | ---: | ---: | ---: |
| ng00 (39 885) | 5 327 ms | 221,6 ms | 163,6 ms | 58 % | 347 Mo |
| ng01 (35 551) | 4 276 ms | 179,9 ms | 139,2 ms | 56 % | 294 Mo |
| ng02 (45 845) | 5 110 ms | 230,5 ms | 150,7 ms | 60 % | 371 Mo |
| ng00, K = 10 (une prise) | — | 1 739 ms | 1 654 ms | 51 % | 1 538 Mo |

Accélération W1 → W48 du domaine : ×23–24 sur une machine de 24 cœurs physiques (48 fils SMT).

### 2.2 Décomposition, voie CPU (M-G4, `gpu_g4/claudegpu6`, `22a6af6aa`, médianes à froid de sept prises, W48)

Le catalogue n'a reçu depuis que deux retouches de la voie lot (`57dd21be1`) : ces chiffres valent pour la voie CPU.

| K, feuille | Trame | domain | passe unique | préambule | tri | résidu non attribué | feuilles (déduites) | parcours (borne haute) |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 5, 16 | ng00 | 210,4 | 153,5 | 20,2 | 11,1 | 25,6 | 47,6 | 105,9 |
| 5, 16 | ng01 | 179,5 | 129,4 | 17,3 | 9,3 | 23,5 | 45,4 | 84,0 |
| 5, 16 | ng02 | 215,6 | 154,0 | 20,6 | 13,1 | 27,9 | 54,1 | 99,9 |
| 10, 24 | ng00 | 852,4 | 673,4 | 29,7 | 49,9 | 99,4 | 535 | 138,4 |
| 10, 24 | ng01 | 678,2 | 529,4 | 25,3 | 37,5 | 86,0 | 415 | 114,4 |
| 10, 24 | ng02 | 802,6 | 623,9 | 32,7 | 53,0 | 93,0 | 485 | 138,9 |

(ms) Résidu = domain − passe unique − préambule − tri : il contient compactage, `level_scan`, assemblage,
allocations et table de recherche, qui ne sont pas publiés séparément dans ce reçu. **Feuilles déduites** = passe
unique CPU − passe unique de la voie GPU, qui retire les feuilles du parcours ; **parcours** = passe unique de la voie
GPU, qui contient encore la mise en file des feuilles (d'où « borne haute »). Ce sont des différences de médianes
entre modes, pas des chronos directs.

**Le parcours de l'arbre de boîtes, et non les feuilles, est le premier poste de l'étage à K = 5** : 84–106 ms sur
180–216 ms, contre 45–54 ms de feuilles. À K = 10 (feuilles de 24), les feuilles dominent (415–535 ms sur 678–852).

Travail logique (registre, M-G4, identique dans toutes les voies) :

| Compteur | ng00 K = 5 | ng01 K = 5 | ng00 K = 10, feuilles de 24 |
| --- | ---: | ---: | ---: |
| nœuds / feuilles | 783 071 / 353 456 | 637 505 / 284 835 | 1 137 395 / 530 259 |
| `filter_tests` (G1, parcours) | 379,4 M | 308,6 M | 1 229 M |
| `dominance_tests` (feuilles) | 32,5 M | 26,1 M | 119,2 M |
| `prefixes` | 120,4 M | 96,0 M | 511,0 M |
| `region_line_tests` (évaluations / cache) | 75,0 M (31,3 / 43,7) | 59,7 M | 498 M (146 / 352) |
| `census_tests` | 37,0 M | 30,2 M | 221,3 M |
| `q4_candidates` / `q4_levels` | 10,26 M / 158 k | 8,20 M / 121 k | 82,6 M / 1,73 M |
| boules émises / incidences | 1,307 M / 6,10 M | 1,096 M / 5,09 M | 5,51 M / 45,4 M |
| profondeur max | 36 | — | 38 |

### 2.3 Voie GPU des feuilles existante (M-G4, reçu `gpu_g4`, sessions 3 à 6, RTX PRO 6000 Blackwell Server, `sm_120`, 97 887 Mio, CUDA 12.9)

- Identité stricte : 372 prises à froid et 84 processus à chaud, dumps et registres égaux à la voie CPU, **zéro
  feuille `unresolved`** sur les trois trames à K = 5 et 10 (u21, `claudegpu6`).
- K = 5, session 6, médianes à froid : exécuteur 71–116 ms (comptage 31–38, écriture 13–20, contexte attendu
  13–56, retour 2 ms, Level hôte 4,6–5,6 ms) ; à chaud 52–59 ms. La voie CPU fait 45–54 ms de feuilles.
- K = 10 : exécuteur 330–360 ms (comptage 177–218, écriture 106–117, Level 20–22 ms) contre 415–535 ms CPU.
- **L'exécuteur ne démarre qu'après la fin du parcours** (`generate_single` : `parallel_for` du parcours, puis
  `batch_stage`). Les temps s'additionnent au lieu de se recouvrir : c'est la première raison de la perte à K = 5.
- Nsight Compute : 3,2–3,4 fils actifs sur 32 par warp ; pile locale 3 248 / 3 264 octets par fil, 2,2 octets utiles
  par secteur ; pipeline ALU entier à 24 % (**l'i128 n'est pas le facteur limitant**) ; `center_line_meets` environ
  20 % des échantillons d'attente ; barrière de réduction 19,6 % (session 3). Contexte CUDA : 78 ms (`cudaFree(0)`).
- L'exécuteur de lot sur l'hôte (`batch_leaves`) compte en 95–116 ms à K = 5 : plus lent que `leaf.cpp` en ligne.

### 2.4 Leviers déjà portés (M-G4 à un fil, reçu `mesures_g4_ab8_diag1`, une paire par trame, bruit A/A ±0,5 %)

| Levier | Mur W1 | Passe unique W1 |
| --- | --- | --- |
| q3 différé | 0,987–0,990 | 0,982–0,985 |
| lemme R (census par masques ; 43 % des tests de puissance évités sur ng00, M-loc) | 1,000–1,006 | 0,998–1,004 |
| compteurs locaux R1 | 0,999–1,003 | 0,999–1,002 |
| enveloppes M3/E4 | **1,003–1,014 (coût)** | **1,012–1,014 (coût)** |

À 48 fils, le bras A/A seul s'écarte jusqu'à 9 % sur la passe unique : **cinq paires ne tranchent rien sous
≈ 30–40 ms**. Les profils numériques coûtent peu sur CPU : u21/u18 = 1,054–1,061, u24/u21 = 0,998–1,003 (M-G4,
`catalogue_profiles_20261002/profiles1`, une prise).

### 2.5 Fait hors GPU relevé au passage

L'API (`src/api/compute.cpp`) fixe `leaf_size = 16` à **tout K**. Le domaine K = 10 mesuré par la CLI vaut
1 739 ms (ng00, `38b76701b`, feuilles de 16), contre 852 ms au banc avec des feuilles de 24 (`22a6af6aa`). Ces deux
prises ne sont pas appariées, mais `claudediag1` mesure, avec le même binaire, un mur FULL K = 10 de 3 285 → 2 506 ms
(ng00) quand on passe de 16 à 24 sites par feuille (−24 à −27 % sur les trois trames, M-G4). Ce levier V8 a été
mesuré au banc, mais il n'est pas appliqué dans le produit.

## 3. Ce que l'arithmétique exacte permet sur le GPU

| Famille | Budget prouvé | Sur le GPU |
| --- | --- | --- |
| Filtre G1, réservoir, dominance des feuilles, enveloppes M3/E4 | `2B+5 ≤ 63` (i64), coordonnées u32 | natif ; une multiplication 64 bits coûte quelques IMAD 32 bits. **Aucun i128 dans le parcours.** |
| `center_in_box`, puissance q1/q2/q4, `center_line_meets` | `5B+6`, `5B+7 ≤ 127` jusqu'à B24 | `__int128` de nvcc, déjà employé et qualifié (`leaf_device_predicates.hpp`) ; émulé par chaînes d'IMAD, mais ALU à 24 % |
| Puissance q3 | `6B+8` : i128 à B18 ; à B21/B24 selon le certificat homothétique | i128 sous certificat, sinon `unresolved` puis rejouée sur CPU avant admission (0 occurrence sur les trames) |
| Level et comparaison de niveaux | `n < 2^204`, `d < 2^152`, produits croisés ≈ 356 bits | **reste sur CPU** : aucune émulation Wide sur le GPU n'est justifiée |

Doctrine conservée : aucune décision flottante sur l'appareil (F1). Un chemin non certifié rend `unresolved`, et la
feuille, ou la tâche, est rejouée par le code CPU de référence avant admission. Les sorties restent identiques octet
pour octet, registre compris.

## 4. Opportunités chiffrées

Contexte d'Amdahl, à garder devant soi : même un domaine gratuit laisse l'étage `tree` à 139–171 ms à K = 5. Les
gains ci-dessous sont **nécessaires mais pas suffisants** pour tenir les 100 ms. Pour un FULL à 100 ms, il faudrait
un domaine d'environ 30–40 ms **et** un `tree` d'environ 60 ms (E).

### O1. Recouvrir l'exécuteur GPU des feuilles avec le parcours CPU, et partager la file entre CPU et GPU

- **Coût actuel (M-G4).** Voie GPU en série : parcours 84–106 ms **puis** exécuteur 71–116 ms à froid, 52–59 ms à
  chaud (K = 5) ; à K = 10, parcours 114–139 ms puis 330–360 ms. Voie CPU : 45–54 ms (K = 5) et 415–535 ms (K = 10)
  de feuilles.
- **Idée.** Dès qu'une tâche de la frontière (≤ 1 024) finit son DFS, sa file de feuilles part en morceau
  asynchrone sur un flux. Les ouvriers CPU libres en fin de parcours prennent des feuilles par le bout opposé, avec
  `leaf.cpp`, pas avec l'exécuteur de lot hôte. Les places de sortie sont fixées par (ordinal de tâche, rang local de
  feuille) ; les registres se somment.
- **Gain attendu (E).** K = 5 : passe unique ≈ max(parcours, feuilles GPU) + queue, soit −30 à −50 ms à chaud. À
  froid, le gain est rogné par l'attente du contexte (13–56 ms mesurés). K = 10 : passe unique 529–673 → ≈ 250–350 ms,
  soit −250 à −350 ms ; le mur reste dominé par les forêts (1,2–1,7 s).
- **Exactitude.** Inchangée : les deux exécuteurs sont déjà prouvés identiques champ par champ ; `unresolved` →
  `leaf.cpp` avant admission ; aucune décision ne dépend de qui joue la feuille. Une porte doit exiger la même
  sortie pour toute répartition forcée (tout CPU, tout GPU, alternance).
- **Risque.** Moyen : ordonnancement asynchrone, réservations `MemoryBudget` par morceau (count–scan–emit), queue
  des feuilles lourdes (13–20 ms à K = 5), contexte à froid.
- **Effort.** Moyen (E : 3 à 5 jours-agent) ; réemploie `leaf_device.hpp`, `leaf_batch_cuda.cu` et `leaf_queue.hpp`.
- **Protocole G4.** `bench/gpu_ab.py`, nouveau mode `gpu_overlap` à côté de `cpu` et `gpu` ; ordre de Williams,
  sept prises à froid et huit passes à chaud ; W48 ; ng00/01/02 ; K = 5 feuilles 16 et K = 10 feuilles 24 ; bras A/A ;
  dump et registre identiques à la première prise CPU ; nouveaux champs (occupation GPU, fin du parcours, queue,
  part CPU/GPU des feuilles) ; Nsight Systems pour la chronologie du recouvrement. Décision : médiane des rapports
  appariés `domain` ≤ 0,85 sur les trois trames, au-delà du bras A/A, avec au moins dix paires (bruit de 9 % à W48).

### O2. Parcours G1 sur le GPU, en largeur et niveau par niveau (route V12 « sous-arbres »)

- **Coût actuel (M-G4).** Parcours 84–106 ms (K = 5) et 114–139 ms (K = 10), plus un préambule de 17–21 et
  25–33 ms. Au total, environ 100–125 ms (K = 5), soit 55–60 % du domaine. À un fil, environ 380 M tests G1
  (K = 5) et 1,23 G (K = 10).
- **Idée.** Une file de nœuds par niveau (au plus 38 niveaux), avec cinq noyaux par niveau : réservoir des 3K plus
  proches par nœud (warp par gros nœud, fil par petit), filtre G1 à un fil par couple (nœud, site), compactage stable
  par sommes préfixes, enveloppe par réduction segmentée, coupe. Les feuilles naissent sur l'appareil et vont
  directement à l'exécuteur (O3) ou, par morceaux, au CPU (O1). Aucune liste ne transite.
- **Gain attendu (E).** Débit : environ 50 M couples (nœud, site) par trame, quelques centaines de Mo de lectures
  aléatoires, environ 200 lancements regroupables en graphe CUDA, soit 5–15 ms sur l'appareil. Le domaine K = 5
  passerait de 180–231 ms à environ 60–110 ms selon le sort des feuilles. À relativiser : la conception v10
  mesurait c(16) = 52 % (part du travail des boîtes restant hors des sous-arbres ≤ 16), c(32) = 24 %, c(64) = 15 %
  (M-loc v10). Cette quantité n'a **jamais été mesurée sur l'arbre de la v11**.
- **Exactitude.** Arithmétique identique en i64 ; ordre du réservoir reproduit par la clé (distance, position dans
  la liste parente) ; le compactage stable conserve l'ordre SiteIdx des listes ; registre = sommes (`nodes`,
  `filter_tests`) et maxima (`max_depth`, `max_leaf`). Le refus `wide_leaf` (> 256 sites) et le quota `max_nodes`
  doivent être reproduits, ou rendre la voie GPU inapplicable avant calcul. Une feuille de plus de 32 sites reste
  `unresolved` et va au CPU.
- **Risque.** Élevé : code neuf, mémoire par niveau à réserver après comptage, divergence de l'arrêt « K
  dominateurs ». L'historique du dépôt est défavorable (cinq ports GPU, jamais plus de ≈ 10 % de bout en bout), mais
  ces échecs portaient sur des étages à i128/Wide ou à proposition flottante, pas sur un filtre i64.
- **Effort.** Élevé (estimation v10 : 6–8 jours-agent pour les sous-arbres).
- **Protocole G4.** Étape 0 sans GPU : un compteur c(L) sur l'arbre v11 (tests G1 par taille de liste) et un
  chrono par nœud selon la taille, à W1, sur les trames et sur `uniform` 8 000/16 000/32 000. Poursuivre si
  c(64) ≤ 20 %. Puis une sonde G4 isolée du noyau (`ncu`) et le même banc qu'O1, avec en plus la porte
  `mhgp11_catalogue_euler_lidar_ng0{0,1,2}_k5` en mode GPU (`--production`), des mutants GPU (une feuille omise,
  un témoin du réservoir faux, une égalité G1 retournée) tués par l'identité, par J1 ou par Euler, et un échantillon
  de nœuds comparé au CPU (jamais un juge exhaustif).

### O3. Feuille coopérative par warp (forme J3), pour la voie GPU

- **Coût actuel (M-G4).** Noyau de comptage 31–38 ms (K = 5) et 177–218 ms (K = 10) ; écriture des feuilles qui
  débordent 13–20 et 106–117 ms ; 3,2–3,4 fils actifs sur 32.
- **Idée.** Sites et masques de la feuille en mémoire partagée ; un fil par site, puis par candidat, en phases
  paires/triplets/quadruplets ; rangs locaux produits par la feuille elle-même (la recherche linéaire coûtait
  environ 30 ms à K = 10).
- **Gain attendu (E).** ×2 à ×4 sur l'exécuteur si la divergence tombe. Il ne sert qu'avec O1 ou O2 : seul, le plafond
  des feuilles est d'environ ×1,3 sur le domaine à K = 5 (les feuilles y pèsent environ 25 %).
- **Exactitude.** Mêmes prédicats i128 certifiés ; les compteurs J2 (« cache simulé ») doivent rester égaux au
  registre CPU, selon le contrat de compteurs J3 accepté par l'auditeur.
- **Risque et effort.** Élevés (E : 4 à 6 jours-agent).
- **Protocole G4.** `bench/gpu_profile.py` (Nsight Compute : fils actifs par warp, mémoire locale, secteurs), puis le
  banc d'O1.

### O4. Tri des émissions : base de tri sur les clés F3, sur le CPU

- **Coût actuel (M-G4).** 9–13 ms (K = 5), 37–53 ms (K = 10), tri par fusion indirect avec repli exact.
- **Idée.** Tri LSD parallèle des clés double positives, dont l'ordre IEEE est celui des entiers u64. Puis, sur la
  suite triée, on regroupe en chaînes les voisins non certainement ordonnés (`level_key_order == 0`) et on trie
  chaque chaîne par `num::compare` puis par support. C'est exact : entre deux chaînes successives, le dernier de
  l'une est certainement avant le premier de l'autre, et la monotonie des clés étend cet ordre à toute paire.
- **Gain attendu (E).** −5 à −8 ms (K = 5) et −25 à −40 ms (K = 10).
- **Pourquoi pas le GPU.** Les Emission vivent sur l'hôte ; le repli Wide ne se fait pas sur l'appareil ; un tri
  radix CPU de 1,3 à 5,5 M clés tient en quelques ms.
- **Risque et effort.** Faibles ; une porte « permutation identique » sur des fixtures de niveaux égaux ou
  quasi égaux, aux bornes de la bande `1 − 2^-40`.
- **Protocole G4.** `ab_g4.py`, A/B du binaire, sept prises, W48 et W1, `sort_ns` et dumps identiques.

### O5. Préambule de la frontière adaptative

- **Coût actuel (M-G4).** 17–21 ms (K = 5), 25–33 ms (K = 10), dans le squelette séquentiel : la racine et les
  premiers nœuds (40 000 sites) sont filtrés par un seul fil par nœud.
- **Idée.** Côté CPU, filtre intra-nœud découpé en blocs sur le Pool, avec concaténation stable par sommes préfixes,
  pour les nœuds de plus de quelques milliers de sites. La racine se passe du filtre (lemme V6). Côté GPU, cela fait
  partie d'O2.
- **Gain attendu (E).** −10 à −15 ms (K = 5).
- **Exactitude.** Mêmes tests, même ordre de liste, même registre.
- **Risque et effort.** Faibles à moyens.
- **Protocole G4.** Banc d'O4, champ `prefix_ns`.

### O6. Résidu non attribué du domaine : mesurer avant d'agir

- **Coût actuel (M-G4).** 23,5–27,9 ms (K = 5) et 86–99 ms (K = 10). Avant L2b, le 3 octobre (`c40`, autre commit),
  compactage, `level_scan` et assemblage valaient chacun 4–7 ms à K = 5 (M-G4).
- **Idée.** Publier `compact_ns`, `level_scan_ns`, `assembly_ns`, `allocation_ns` et un chrono de la table de
  recherche dans les rapports `gpu_ab`/`ab_g4` (la sonde les imprime déjà). Ensuite seulement : pré-toucher les
  pages, fusionner `level_scan` dans le tri (O4), compter les fautes de page.
- **GPU.** Non : copies de CSR et table de recherche liées à la mémoire hôte ; le catalogue doit être sur l'hôte pour
  `tree`.
- **Protocole.** Une session de mesure sans changement de code (champs déjà émis par `bench/full_probe.cpp`).

### O7. Hors GPU, mais mesuré : feuilles de 24 à K = 10 dans le produit

- **Coût actuel (M-G4).** Domaine K = 10 par la CLI : 1 739 ms (feuilles de 16), contre 852 ms au banc (feuilles de
  24). Les prises ne sont pas appariées, mais l'A/B appariée `claudediag1` donne −24 à −27 % de mur FULL.
- **Exactitude.** Le choix de la feuille ne change pas l'objet (G1–G4). Les sorties sont identiques ; le registre
  change (compteurs de feuille), ce qui exige une décision explicite et des portes de route regravées.
- **Effort.** Une ligne dans `catalogue_params` plus les portes. **Protocole :** `sorties_g4.py` à K = 10, trois
  trames.

## 5. Pistes GPU écartées

- **Feuilles seules sur le GPU comme route finale** : les feuilles valent environ 25 % du domaine à K = 5. Plafond
  d'Amdahl d'environ ×1,3, cohérent avec la perte mesurée (+30 à +50 ms de mur). Elles ne valent qu'avec O1 et O2.
- **Exécuteur de lot sur l'hôte (`batch_leaves`)** : comptage en 95–116 ms contre 45–54 ms de feuilles en ligne
  (M-G4).
- **Tri ou comparaisons de niveaux sur le GPU** : produits croisés d'environ 356 bits (Wide) et données sur l'hôte.
  O4 sur le CPU suffit.
- **Assemblage CSR, populations, table `S* → BallIdx` sur le GPU** : liés à la mémoire, quelques ms, résultat requis
  sur l'hôte ; un aller-retour PCIe de 24–180 Mo coûterait autant que le travail.
- **Émulation Wide (q3 à B21/B24 hors certificat) sur l'appareil** : zéro `unresolved` mesuré sur les trames ; le
  repli CPU suffit.
- **Census global par l'index sur le GPU** : relève de l'étage `tree`. Échecs v6/v7 mesurés : noyaux de 29,8 ms pour
  un étage de 846 ms, ×1,03–1,12 de bout en bout.
- **Proposition flottante sur l'appareil puis recertification CPU** : contraire à F1. La bibliothèque produit a
  mesuré 4,8–5,4 s de recertification pour un étage de 7,0–7,5 s.
- **Catalogue entier matérialisé sur l'appareil (v5)** : parité au mieux, jamais un gain mesuré.
- **Juge d'Euler à K+2 sur le GPU** : hors du chemin produit (3,5–8 % de son propre coût, M-loc).
- **Enveloppes M3/E4 transposées au GPU pour elles-mêmes** : elles coûtent 1,2–1,4 % de passe unique sur le CPU
  (M-G4) ; leur intérêt sur l'appareil ne serait que la réduction de divergence, à juger dans O3.

## 6. Questions ouvertes

1. Le contrat des 100 ms est-il **à froid** (processus neuf : contexte CUDA de 78 ms, dont 13–56 ms attendus à
   K = 5 malgré l'ouverture anticipée) ou **en flux résident** ? Sans réponse, tout GPU à K = 5 reste pénalisé à froid.
2. Les 100 ms portent-ils sur `domain + tree` seulement (environ 320–385 ms aujourd'hui à W48), hors écriture ?
3. L'identité stricte du **registre** (compteurs logiques) est-elle exigée d'une voie GPU du parcours, ou seulement
   celle du catalogue publié ? La première impose de reproduire l'ordre du réservoir et l'arrêt du filtre.
4. c(L) sur l'arbre v11 : non mesuré. C'est la condition d'O2.
5. Le parcours n'a pas de chrono interne : on ignore la part du filtre G1, du réservoir, de l'enveloppe, et celle de
   l'allocation d'un `Buffer` par nœud (783 k à 1,14 M réservations, avec CAS sur le compte partagé du budget). Cette
   dernière n'est qu'une hypothèse, non mesurée : l'accélération W1 → W48 de ×23–28 ne montre pas de contention
   massive.
6. Résidu de 23–28 ms (K = 5) et 86–99 ms (K = 10) non attribué (O6).
7. Aucune mesure des coûts du domaine aux tailles `uniform` 8 000/16 000/32 000 sur G4 : seules les trames sont
   relues ici. Ces trois tailles sont dans le protocole d'O2.
8. L'étage `tree` (139–171 ms) dépasse seul le contrat : faut-il prioriser O1 (quelques jours, gain sûr à K = 10)
   ou attendre la décision sur `tree` avant d'ouvrir O2 ?

## 7. Sources relues

- `morsehgp3D_v11/receipts/developpement_20261005/qualification_finale/claudefinmesure/sorties_g4.json` (`origin/main`)
- `morsehgp3D_v11/receipts/developpement_20261004/gpu_g4/README.md` et `sessions/claudegpu{5,6}/gpu_k{5,10}_report.json`
- `morsehgp3D_v11/receipts/developpement_20261004/mesures_g4_ab8_diag1/README.md`
- `morsehgp3D_v11/receipts/audit_deep_20261004/performance/README.md` et `selected/c40_paired/.../full_paired.json`
- `morsehgp3D_v11/receipts/catalogue_profiles_20261002/README.md`
- `morsehgp3D_v11/docs/CATALOGUE.md`, `docs/ARCHITECTURE.md`
- `morsehgp3D_v11/src/catalogue/{single_pass.cpp,boxes.cpp,internal.hpp,frontier.cpp,adaptive_frontier.hpp,assemble.cpp,sort_indices.cpp,sort_level_key.hpp,leaf.cpp,leaf_device_predicates.hpp}`, `src/tower/full_domain.cpp`, `src/api/compute.cpp`, `src/core/buffer.hpp`
- `build/v11-persist/audit_transpositions/AUDIT_TRANSPOSITIONS_V11.md` (V6, V8, V12, § 5.4)
- `morsehgp3D_v11/audits/AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md` (R1, R7)
