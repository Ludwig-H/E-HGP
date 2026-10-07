# Passation de la v11 vers la v12

7 octobre 2026. Chantier v11 clos sur demande de l'utilisateur.

```text
phase=exploration_v11_hors_registre (close le 7 octobre 2026)
backend=cpu_reference ; voie de banc cuda_g4 pour le lot de feuilles du catalogue
profile=quantized_u21_input_only
public_status=not_claimed
```

**Lire ensuite** : l'[audit final](docs/AUDIT_FINAL_V11.md), qui détaille tout ce qui suit, avec chiffres et sources.

**Complément du même jour** : l'[audit géant](docs/AUDIT_GEANT_V11.md) a vérifié cette passation et l'audit final
(huit lectures indépendantes, rejeu local des empreintes, réanalyse des mesures brutes). Il en corrige une trentaine
de points, listés à son § 8. Les principaux portent sur trois points :
- les mécanismes de la v10 à porter : la feuille J3 n'a jamais été dans la v10 et la partition T > 0 n'agit pas sur le
  LiDAR, tandis que la plus petite boule proposée puis certifiée manque à la liste ;
- l'état de la qualification au gel ;
- la portée des comparaisons à HDBSCAN.

Il établit aussi que la conception d'origine de la v11 n'a pas été implantée. Lire les deux documents ensemble.

## 0. Décision et portée

**Demande de l'utilisateur** (7 octobre 2026) : « On va organiser plutôt une passation et tout reconstruire à neuf
pour une v12 de Morse HGP 3D. Je veux que tu fasses un audit géant de cette v11 ; toutes les bonnes idées qui ont
marché, celles qui n'ont pas marché, etc. Puis que tu mettes à jour le dossier morsehgp3D_v11 avec cette
passation. »

**La v11 est gelée.**
- Dernier commit moteur : `ac081a06f`. Son moteur est identique à celui de `733912e65`, la base mesurée en dernier.
- La passation n'ajoute que des documents.
- Désormais, la v11 est une **source différentielle** et une source d'empreintes épinglées, comme la v10 l'a été pour
  elle. Tout port vers la v12 est explicite, épinglé et requalifié.

**Ce que cette passation ne fait pas.**
- Elle n'ouvre pas la v12 : `AGENTS.md` et `CLAUDE.md` ne changent pas avant l'accord de l'utilisateur.
- Elle ne touche pas `docs/implementation_status.toml`, car la v11 est hors registre.
- Elle ne fait aucune mesure nouvelle. **GCP non utilisé.**

## 1. En une page

**Ce que la v11 a réussi.**
- **L'objet.** Elle calcule exactement la tour FULL (K ≤ 12, entiers à budget de bits, aucune décision flottante),
  sur un contrat mathématique démontré sans position générale.
- **L'exactitude** :
  - aucun résultat FULL faux en six jours et au moins 126 sessions G4 ;
  - voie GPU identique au CPU à l'octet ;
  - empreintes FULL K5 des trois trames stables du 3 au 7 octobre.
- **Les sorties.** Quatre sorties transactionnelles : `full`, `supports` (arbre couvrant de Kruskal), `points`
  ($H^{r}_{K+1}$) et `plat` (EOM exacte).
- **L'outillage.** Un harnais de qualification sévère : portes à code exact, mutants causaux, session G4 gardée.

**Ce qu'elle n'a pas réussi.**
- Le contrat de 100 ms. Le jalon de 200 ms n'est approché que sur ng01, à chaud (212 ms).
- K10 en dessous de 1,3 s.
- Dépasser la v10 en vitesse. À K5, sa voie GPU égale la v10 CPU. Ses forêts restent ×1,3 plus lentes à K5, et
  ×2,6 à 2,8 à K10.
- Fermer le différentiel canonique v10/v11.
- Des points stables par insertion.
- Une sélection plate qui garde les séparations fugaces.
- Un polyèdre petit et robuste.

**Trois causes principales** (audit, § 14) :
1. Deux étages en série, `domain` puis forêts, chacun au-dessus de 90 ms à K5.
2. Des chemins critiques séquentiels par construction : le publieur de l'ordre K, les rondes de frontière, le join
   avant le lot GPU.
3. Un GPU employé comme un CPU de plus : un fil par feuille, et un contexte ouvert à chaque processus.

**Trois leçons principales** (audit, § 15) :
1. Le contrat et l'oracle borné passent avant le natif.
2. Il faut un protocole statistique calibré : seuils tirés du bruit A/A et de l'effet attendu.
3. La v12 doit porter d'emblée les mécanismes qui faisaient la vitesse de la v10, mesurés en même session.

## 2. État exact au gel

### 2.1 Temps sur LiDAR réel

G4 W48, trames sans sol ng00 / ng01 / ng02 (SemanticKITTI 08/000000, 000100, 000200 ; grille 1 mm). Les mesures
viennent de la session `claudeg1` (variante de base, `733912e65`), reçu
`receipts/developpement_20261007/filtre_g1_avx2/`.

| Mode de banc | À froid (6 processus) | À chaud (passes 2–10) | À chaud : `domain` / forêts |
|---|---|---|---|
| K5 GPU `868347:400`, feuilles 24 | 335 / 301 / 345 ms | 251 / 212 / 255 ms | 138 / 114, 120 / 91, 141 / 114 |
| K5 CPU `802811`, feuilles 16 | 343 / 272 / 329 ms | 314 / 255 / 313 ms | 200 / 113, 163 / 92, 195 / 116 |
| K10 GPU `868347`, feuilles 24 | 1 824 / 1 395 / 1 602 ms | 1 782 / 1 336 / 1 536 ms | 489 / 1 293, 398 / 939, 471 / 1 065 |

**Référence v10** (`777406b82`, CPU W48, troisième passe chaude) :
- K5 : 252,0 / 204,2 / 253,6 ms, dont catalogue 163,5 / 136,9 / 164,3 et tour 88,5 / 67,3 / 89,3 ;
- K10 : 1 124,6 / 861,4 / 1 024,3 ms, dont tour 472,0 / 333,9 / 406,2.

C'est une comparaison descriptive entre sessions (audit, § 3.2).

### 2.2 Modes de banc et produit

Les modes du banc sont des masques d'options de la sonde `mhgp11_full_bench` :
- **16379** est la voie CPU de base : graphe de paires, table de populations, ordres concurrents, pipeline ;
- **+262144** ajoute le placement O1 ;
- **+524288** ajoute le cache de blocs ;
- **+65536** envoie le lot de feuilles sur le GPU ;
- le suffixe **`:400`** donne la part de l'hôte, en ‰, dans l'exécuteur partagé.

D'où les trois modes de référence : **802811** = 16379 + placement + cache (CPU) ; **868347** = 802811 + GPU.

**L'exécutable `mhgp11` et l'API ne jouent aucun mode de référence.** Ils prennent la voie CPU à feuilles de 16 à
tout K, sans GPU ni cache de blocs (`src/api/compute.cpp`). Le cache dans la `Session` attendait l'avis des
auditeurs (section Y) ; les feuilles de 24 à K10 attendaient une décision de l'utilisateur.

### 2.3 Qualification

**Dernière qualification complète** : `98a009550` (5 octobre ; `receipts/developpement_20261005/qualification_finale`).
- 3 695 portes et 485/485 mutants.
- Release u18, u21 et u24, plus le profil empoisonné.
- ASan/UBSan et TSan.
- Échelle et LiDAR.

**Depuis**, chaque session n'a joué que les mutants de son levier et des matrices ASan/TSan courtes (par exemple
807/807 pour `claudeg1`). Au HEAD :
- les 530 mutants déclarés n'ont pas tous été joués, et pas au profil u21 ;
- SPv2 (`supports` Kruskal) n'est qualifiée qu'en Release u21 (15/15) ;
- les différentiels S9 et S10 n'ont pas été rejoués après la garde de chronologie (`b0f2a0a9e`) ;
- les portes GPU numériques extrêmes ne sont couvertes que côté hôte.

`public_status=not_claimed` partout.

### 2.4 Empreintes de référence

Les SHA-256 des vidages FULL (`MHGP11FUL1`, u21) des trois trames à K5 et K10, et des nuages uniformes de 8 000,
16 000 et 32 000 sites à K5, sont dans l'[audit final, § 3.4](docs/AUDIT_FINAL_V11.md#34-empreintes-de-référence-pour-le-différentiel-de-la-v12).
Le premier jalon de la v12 doit les reproduire, ou expliquer chaque écart par un témoin exact.

## 3. À porter dans la v12

Chaque élément ci-dessous est un **port explicite**, épinglé à `ac081a06f` et requalifié.

| Élément | Où dans la v11 | Pourquoi |
|---|---|---|
| Contrat mathématique et galerie de témoins | `docs/MATHEMATIQUES.md` (§ 1–8 et § 10), sections V11 du registre racine, `tests/fixtures/` | démontré sans position générale, relu, contre-éprouvé |
| Oracle borné à deux étages et son juge | `reference/` (étages A et B, oracle d'intervalles, S1 et ses mutants de vivacité, familles SplitMix64) | établit la vérité au lieu de la re-vérifier |
| Doctrine numérique | `src/num/` (budgets `constexpr` par expression, voies native/contrôlée/`Wide`, certificats dans les fabriques, `Level` non réduit, F1–F6 avec F3 par expression) | aucun défaut numérique en sortie ; chaque défaut trouvé l'a été à une borne exacte |
| Index radix de Morton à bornes entières (V3), contrat de census, table de populations, journal des graines | `src/index/`, `src/tower/population_lookup.*` | gains mesurés et prouvés |
| Plateau atomique, descente datée, numérotation canonique, contrôles de naturalité | `src/tower/` | exactitude et déterminisme à tout W |
| Harnais | `cmake/run_expect.cmake`, `cmake/gates.cmake`, `tests/mutants/run_mutants.py`, `tests/support/` | aucun vert par vacuité ni par absence |
| Session G4 gardée | `gcp-migration/v11_session.py` | 124 arrêts certifiés sur 126 ; à compléter par un rapatriement depuis le disque de la VM après `--recover` |
| Budget mémoire transactionnel et cache de blocs | `src/core/buffer.*` | refus sans publication partielle ; restitution 14 → 0,8 ms |
| Dossier de sortie transactionnel, manifeste, codes 0/2/3 | `src/io/`, `src/api/` | doubles échecs joués, publication atomique |
| Exactitude du GPU : i128 certifié, sinon `unresolved` rejoué avant admission | `src/catalogue/leaf_device.hpp` | quatre jours sans un écart |
| Supports par Kruskal au plateau, S* seul | `src/supports/` | lemmes A–H, oracle S1 |
| Hiérarchie de points $H^{r}_{K+1}$ et tête plate exacte | `src/points/`, `src/head/` | fidèles, laminaires, exactes ; leurs limites sont connues (audit, § 8.3) |

**À porter de la v10** (`777406b82` et son raccord R2), à mesurer en même session contre la v11 :
- le choix des k plus proches dans les descentes ;
- le mémo cellulaire daté ;
- la partition T > 0 (sous-maille à 1/64) ;
- la feuille J3 complète ;
- la table M(K) des tailles de feuille (12/16/24/28).

## 4. À ne pas refaire sans élément nouveau

| Piste | Mesure | Ce qui la rouvrirait |
|---|---|---|
| Feuille coopérative par paires (un warp par feuille) | exécuteur ×1,15–1,38 à K5, divergence intacte | une forme data-parallèle (feuille cohérente par préfixe) prototypée hors moteur |
| L4, sous-lots GPU recouverts | ×1,6–3,5 plus lent | un `domain` en flux, sans join global |
| N1, arène de pile du parcours | +2,4 à 8,5 ms | une mesure par fil qui écarte le SMT |
| Pages de 2 Mio (THP) | ×1,064 sur G4, malgré −17 % en local | une cause établie par les compteurs du matériel |
| Mémos de lane ; lots ordre par ordre | 2,6 % de succès ; ~600 barrières | aucune |
| Supports v1 (tous les $\mathcal{Q}_b$) ; `kparties_reliees` | instables (Hausdorff ≥ 1/4) | aucune ; à garder dans l'oracle seulement |
| Seuil de condensation relatif au parent | 9 objets sur 19, aucun vélo contre un mur | une règle qui passe la fixture du vélo contre le mur |
| Euler comme certificat | compensations | aucune ; le garder comme filet |
| Marge en niveau carré (Q₁ v10, ER0h) | aucune constante uniforme | une preuve |
| Prédire le temps par le compte d'instructions, le SASS statique ou le local | V3 : instructions ×0,565, temps ×0,925 ; levier A ×1,11 ; THP | aucune ; le temps se mesure sur G4 |

**Leviers retirés de justesse par un seuil trop sévère.** Ils ont mesuré entre −3 et −17 % sur la phase visée :
G1 AVX2, frontière par tranches, préchargement des graines, préchargements combinés, annonces des publieurs. Ils ne
sont **pas** réfutés ; ils se rejugent sous le protocole statistique du § 6.4. Leurs patches sont dans leurs reçus
ou dans l'historique Git (`b6fd3796d`, `3e6f88c7f`, `1950c3727`, `e35db29c3`, `04b00810d`).

## 5. Leçons

Elles sont détaillées dans l'audit (§ 15). Les douze qui doivent changer la manière de travailler :

1. Écrire et faire relire le contrat (numérique, capacité, refus, compteurs) avant la première ligne native.
2. Graver d'abord les témoins de bord : s = 2^B−1, 2^127−1, 2^20±1, plateaux cosphériques, triangle équidistant.
3. Un seul profil produit (u21), u24 en matrice ; plus d'attendus gravés pour un seul profil.
4. Fermer le différentiel contre la version précédente dès la première tranche, sur trames entières.
5. Mesurer les leviers algorithmiques à W1 ou sur compteurs, et réserver W48 au parallélisme.
6. À W48, au moins 10 processus par bras et par trame, en ordre entrelacé, avec un bras A/A et un seuil tiré de la
   variance et de l'effet attendu.
7. Un témoin d'activation par prise pour chaque option mesurée.
8. Des modes nommés et versionnés ; ne jamais réutiliser un masque.
9. Une matrice en lots de 30 minutes au plus, aux durées mesurées ; ne jamais retirer une porte pour tenir une
   échéance ; tenir la dette de qualification commit par commit.
10. Construire et tester en local avec l'outillage de la VM (GCC 11.4, CMake 3.22.1, Python 3.10 nu) ; G4 n'est pas
    une boucle de compilation.
11. Concevoir les sorties comme des vues d'un même registre d'événements, et non une par une.
12. Un registre de constats d'audit lisible par une machine, des rôles stables, une relecture avant adoption de tout
    changement de budget, de concurrence ou de format.

## 6. Proposition pour la v12

### 6.1 Décisions à obtenir de l'utilisateur avant tout code

1. **Régime du contrat de 100 ms.**
   - Processus neuf (à froid), ou Session résidente qui reçoit des trames successives (à chaud, 10 Hz) ?
   - Ce choix décide si l'ouverture du contexte CUDA (75 à 150 ms) entre dans le budget.
2. **Matériel.** G4 seul, CPU et GPU ensemble ? Le GPU fait-il partie du chemin contractuel ?
3. **Périmètre du contrat.** Les 100 ms couvrent-ils FULL K = 1..5 avec verticales seulement, ou aussi `supports`,
   `points` ou `plat` ? K10 est-il un contrat ou un objectif ?
4. **Profil.** u21 seul, u24 en matrice et u18 abandonné, comme le recommande l'audit ?
5. **Seuil de condensation.** Absolu (thèse, p. 97) ou relatif (`CLAUDE.md`,
   `Zoltan/FoundationModel/SPECIFICATION.md`) ?
6. **Multiplicités** (retours fusionnés au millimètre) : refus, comme en v11, ou modèle par copies ?
7. **Polyèdres d'ordre k** : dans la v12, ou en recherche aval ?
8. **Ouverture.** Mettre à jour `AGENTS.md` et `CLAUDE.md` pour ouvrir la v12, avec son cadre.

### 6.2 Contrats à écrire et à faire relire d'abord

- **Numérique** : budget de bits par expression, conversions gardées, références gravées par profil.
- **Capacité** : hôte, mémoire épinglée et appareil, coexistences, arènes et caches comptés.
- **Refus** : une porte d'injection native de bout en bout pour chaque chemin de refus.
- **Compteurs** : compteurs logiques indépendants de l'ordre de visite, diagnostics physiques séparés. Ce contrat est
  préalable à toute forêt parallèle.
- **Qualification** :
  - lots dimensionnés ;
  - préflight du juge, des données et de l'outillage ;
  - « sans résultat » n'est jamais un PASS ;
  - provenance lue chez le worker.
- **Mesure** : protocole statistique (§ 6.4).
- **Différentiel** : empreintes de la v11 (§ 2.4) et table de vérité de la v10, sur les mêmes XYZ.

### 6.3 Architecture proposée

| Étage | v11 au gel (K5, à chaud) | Budget v12 (K5) | Moyens proposés |
|---|---|---|---|
| Préparation, index | quelques ms | ≤ 5 ms | port v11 (index radix V3), structures ouvertes une fois par Session |
| `domain` (catalogue) | 120–200 ms | ≤ 40–50 ms | résident sur le GPU et en flux (frontière → file → parcours, sans join global) ; une feuille data-parallèle par warp (forme cohérente ou J3), en source unique SIMD/CUDA ; filtres G1 des nœuds internes sur l'appareil ou vectorisés ; tri, balayage et assemblage sur l'appareil ; repli `unresolved` parallèle et budgété (U2) ; contexte et blocs ouverts une fois |
| Forêts | 91–116 ms | ≤ 40–50 ms | graines résolues en parallèle (descentes au coût v10 : k plus proches, mémo daté, histogramme des pas publié) ; forêt d'ordre k comme arbre couvrant minimal parallèle et déterministe sous l'ordre total (rang, ordinal de cellule), par Kruskal à réservations ou Borůvka canonique ; contraction des chaînes de plateau ; verticales par requêtes d'ancêtres sur forêts closes |
| Registre d'événements et sorties | non mesuré sur G4 | ≤ 10 ms | registre unique par ordre (nœuds, rangs exacts, hyper-arêtes de Kruskal, vies, verticales, masses § 9.1), dont toutes les sorties sont des vues ; `full` dans un format compact (l'écriture prend aujourd'hui 85 % du temps de la sortie) |

**Microbanc d'abord.** Avant tout port de la forêt, la jouer hors moteur sur graines vidées, avec pour seuils
≤ 10 ms à K5 et ≤ 50 ms à K10.

### 6.4 Méthode

- **Registre de constats** (audit et développement) : identifiant, classe, gravité, pin, témoin, état et preuve de
  clôture. Il est contrôlé par un `tools/check_*`.
- **Notes d'audit vivantes d'une page.** L'historique va dans les reçus.
- **Protocole statistique** :
  - leviers algorithmiques jugés à W1 ou sur compteurs déterministes ;
  - à W48, au moins 10 processus par bras et par trame, ordre entrelacé à froid comme à chaud, un processus
    d'échauffement jeté, un bras A/A par session ;
  - décision sur les log-rapports appariés, avec un intervalle par bootstrap ou un test des signes ;
  - seuil tiré de l'effet minimal détectable et de l'effet attendu (3 à 10 %) ;
  - confirmation sur prises neuves pour les petits gains cumulables ;
  - plusieurs séquences LiDAR, pas seulement la 08.
- **Un seul banc de décision.**
  - Il porte une règle structurée (statistique, régime, bras, seuil, conduite en cas d'échec).
  - Son juge unique, en bibliothèque, est couvert par CTest et refuse tout banc non conforme.
  - L'empreinte du juge est inscrite au reçu.
- **Modes nommés** (par exemple `ref_cpu_k5`, `ref_gpu_k5`) et une sonde à options nommées qui explique ses refus.
- **Qualification complète à chaque jalon** : matrice entière et tous les mutants au profil par défaut, plus une
  variante TSan pour les mutants de concurrence. `--check` vérifie que chaque porte citée existe.
- **Reçus** sans copies d'arbres sources ni identité de compte. Aucune porte ne lit `receipts/`.

### 6.5 Ordre de travail proposé

1. **T0 — contrats et socle.**
   - Ports du harnais, de `num`, de `reference/` et de la session G4.
   - Exportateur `MHGP11FUL1` ou empreinte canonique, et différentiel contre les empreintes de la v11.
2. **T1 — catalogue.**
   - Voie CPU de référence, puis voie GPU résidente au budget de 40–50 ms.
   - Différentiel contre la v11 dès la première prise.
3. **T2 — tour.**
   - Histogramme des pas de descente, v10 contre v11 contre v12.
   - Microbanc de la forêt comme arbre couvrant parallèle.
   - Intégration, puis verticales.
4. **T3 — registre d'événements et sorties** : `full` compact, `squelette` (SPv2), `points`, `condense`, `plat`.
5. **T4 — qualification et campagne de mesure** sur plusieurs séquences.
6. **T5 — comparaison à HDBSCAN.**
   - Protocole préenregistré tenu jusqu'au bout : P08, S3b, H_L1 et H_L2.
   - `sklearn` épinglé, sur la même machine.
   - Trois niveaux séparés : tour, hiérarchie, sélection.

## 7. Questions ouvertes transmises

| Sujet | Question | Origine |
|---|---|---|
| Descentes | Pourquoi les résolveurs v11 coûtent-ils environ 5 fois les descentes de la v10 à K10 (1 214 contre 243 ms sur ng00) ? | audit, § 3.2 |
| Catalogue | Partition T > 0 compatible avec u24 (comparer QE à Dr, budget 4B+5+T ≤ 127) ? | `QUESTION_CLAUDE_VITESSE_100MS_20261004.md` § B.6 |
| Forêt parallèle | Inscrire au registre le lemme MST/contraction et clore la `proof_obligation` sur la contraction des plateaux | rapport C |
| Points | Une règle stable par insertion, locale au profil, conforme à T0 et Q1–Q4 ? Critère d'existence d'un cluster ; synthèse multi-K | `docs/HIERARCHIE_POINTS.md` |
| Sélection plate | Une sélection sans vérité terrain qui garde les séparations fugaces | `docs/SORTIE_PLATE.md` |
| Identité des nœuds | Identité des nœuds de vie < 2δ (61 à 67 % des nœuds K5) | `REPONSE_CLAUDE_POLYEDRE_ORDRE_K_RESULTATS_20261007.md` |
| Polyèdre | Un représentant de A_k(r) petit, de topologie certifiée et robuste ; huit questions à l'auditeur | même fichier |
| Coquilles étendues | Un quotient polynomial au-delà de l'énumération bornée | `docs/MATHEMATIQUES.md` |
| FULL pondéré | Modèle par copies, ni relu ni implanté | `docs/MATHEMATIQUES.md` § 9 |

## 8. Arriéré du canal d'audit

- **Commits non relus.** Environ 40 commits du développeur, après le 6 octobre à 22 h 08, dont deux changements
  sensibles : le contrat mémoire du cache de blocs (`ccdd4db75`) et le publieur O2 (`13a4a0a4c`).
- **Questions sans réponse** dans `audits/REPONSE_CLAUDE_SUPPORTS_20261004.md` :
  - T1–T2 : prédicteur du travail d'une feuille ;
  - V1–V2 : invariant par nœud de l'arbre radix, contrat de budget ;
  - X : garde du préchargement d'O2 ;
  - Y1 : pourquoi THP perd sur 48 fils ;
  - Y2 : comptage honnête de la réutilisation des blocs.
- **Polyèdre** : les huit questions de `audits/REPONSE_CLAUDE_POLYEDRE_ORDRE_K_RESULTATS_20261007.md`.
- **Constats ouverts des auditeurs** :
  - le runner de matrice sans `--output-on-failure` (`tools/g4_matrix.py`, l. 735) ;
  - `bench/sorties_g4.py` (l. 251) qui recopie `--commit` au lieu de lire `V11_SOURCE_PIN` ;
  - l'isolation des processus descendants, non certifiée ;
  - Clang jamais qualifié.

La note [`audits/NOTE_CLAUDE_CLOTURE_V11_20261007.md`](audits/NOTE_CLAUDE_CLOTURE_V11_20261007.md) informe les
auditeurs de la clôture.

## 9. Errata connus des documents v11

Ces documents ne sont pas réécrits : ce sont des sources. Les errata sont relevés ici.

| Document | Erratum |
|---|---|
| `docs/SORTIES.md` (§ 2) | annonce encore `MHGP11SP` v1 ; le format publié est v2 (Kruskal, S* seul) |
| `docs/MATHEMATIQUES.md` | deux sections numérotées 10.10 (l. 866 et 918) |
| `docs/CATALOGUE.md` (l. 158) | workspace de feuille en 8C⌈C/64⌉ ; l'auditeur demande 16C⌈C/64⌉ (non revérifié ici) |
| `docs/DEVELOPPEMENT.md` | figé au 3 octobre (bandeau ajouté par cette passation) |
| `receipts/developpement_20261003/pipeline_g4` | titre « Qualification claudeab5 », mais les résultats viennent de claudeab7 |
| `receipts/developpement_20261005/qualification_sorties` | attribue claudequalA à `b319efc84` au lieu de `00bd979ac` |
| `receipts/audit_dialogues_20261004/README.md` | lien mort vers `audits/REPONSE_CLAUDE_POINTS_20261003.md` |
| Préenregistrement E1 (`plans/e1_prereg_*`) | corrigendum manquant (rapport F) |
| Enveloppes M3/E4 | retrait de la voie CPU approuvé par l'auditeur, jamais fait (`src/catalogue/leaf.cpp`) |

## 10. Hors dépôt, hygiène et données

- **Notes de travail jusqu'ici hors dépôt.** Elles sont versées, après une relecture intégrale, dans le reçu
  [`receipts/notes_hors_depot_20261007/`](receipts/notes_hors_depot_20261007/README.md) :
  - `gpu_optim/` : cartes du catalogue, des forêts, de l'aval, de l'infrastructure, du profil des étages et du GPU
    existant ; plans GPU ;
  - `conception/PISTES_DE_RUPTURE.md` ;
  - `audit_transpositions/` : audit des transpositions v2–v10 vers la v11 ;
  - `polyedres_reconnaissables/SYNTHESE.md`.

  Ce sont des notes de travail, avec des estimations antérieures aux mesures. Le README du reçu liste les points
  dépassés et les erreurs connues. Les images et les données réelles restent hors dépôt.
- **Paquets de sessions G4** : `/workspaces/.ehgp-sessions/` et le bloc-notes de la session de développement. Ils
  sont volatils ; les reçus du dépôt en conservent l'essentiel.
- **Identité du compte GCP.** L'adresse électronique du compte figure dans 143 fichiers de reçus (champs `user`,
  `recovery_command` et `gcloud_account`). Les reçus sont immuables : purger ou non est une décision de
  l'utilisateur. La v12 ne doit plus l'écrire.
- **Volume.** `receipts/` pèse environ 190 Mo (9 774 fichiers, dont environ 2 000 copies de sources C++). La moitié
  vient des reçus d'audit. La v12 cite des SHA au lieu de copier des arbres.
- **Données LiDAR.** Aucune donnée ni coordonnée SemanticKITTI dans le dépôt ; les trames sont reconstruites par
  script, empreintes vérifiées.

## 11. Carte de la v11

**Code** (`src/`, environ 21 900 lignes C++20) :

| Module | Rôle |
|---|---|
| `core` | statuts, tampons comptés, budget mémoire et cache de blocs |
| `sched` | Pool synchrone |
| `num` | entiers à budget de bits, prédicats, certificats, racines |
| `cloud` | entrée u21 |
| `index` | arbre radix de Morton |
| `catalogue` | frontière, passe unique, feuilles CPU et GPU, assemblage |
| `tower` | domaine, naissances, résolution, publieurs, verticales, pipeline |
| `supports` | arbre couvrant de Kruskal |
| `points` | $H^{r}_{K+1}$ |
| `head` | condensation et EOM exacte |
| `io` | formats et dossier transactionnel |
| `api` | `Session` |

**Autres répertoires** :
- `cli/` : l'exécutable `mhgp11` ;
- `bench/` : la sonde `full_probe.cpp`, les bancs `gpu_ab.py` et `ab_g4.py`, les bancs Python `points_*` ;
- `tools/` : `g4_matrix.py`, `check_style.py` ;
- `reference/` : l'oracle borné ;
- `tests/` : portes, fixtures, mutants.

**Documents** (`docs/`) :

| Thème | Documents |
|---|---|
| Objet et cadre | [MATHEMATIQUES.md](docs/MATHEMATIQUES.md), [ARCHITECTURE.md](docs/ARCHITECTURE.md), [PROVENANCE.md](docs/PROVENANCE.md), [AUDIT_V10_SYNTHESE.md](docs/AUDIT_V10_SYNTHESE.md), [CONCEPTION_MOTEUR.md](docs/CONCEPTION_MOTEUR.md) |
| Catalogue | [CATALOGUE.md](docs/CATALOGUE.md), [CATALOGUE_SINGLE_PASS.md](docs/CATALOGUE_SINGLE_PASS.md), [CATALOGUE_FRONTIERE_ADAPTATIVE.md](docs/CATALOGUE_FRONTIERE_ADAPTATIVE.md), [CATALOGUE_OPTIMISATIONS.md](docs/CATALOGUE_OPTIMISATIONS.md), [INDEX.md](docs/INDEX.md), [MEB.md](docs/MEB.md) |
| Tour | [FULL_FORESTS.md](docs/FULL_FORESTS.md), [FULL_DOMAIN.md](docs/FULL_DOMAIN.md), [PERFORMANCE_FULL.md](docs/PERFORMANCE_FULL.md), [FULL_PARALLEL.md](docs/FULL_PARALLEL.md) |
| Sorties | [SORTIES.md](docs/SORTIES.md), [SORTIE_PLATE.md](docs/SORTIE_PLATE.md), [HIERARCHIE_POINTS.md](docs/HIERARCHIE_POINTS.md) |

**Reçus** : voir les références de l'audit (§ 17). **Canal d'audit** : `audits/`.

## 12. Rejouer la v11

**Construction et portes rapides.** En local, Release et portes ciblées seulement, comme depuis le 5 octobre ; la
matrice complète passe sur G4.

```bash
cmake -S morsehgp3D_v11 -B build/v11 -DCMAKE_BUILD_TYPE=Release        # MHGP11_COORD_BITS=21 par défaut
cmake --build build/v11 --parallel
ctest --test-dir build/v11 -LE long --no-tests=error --output-on-failure
```

**Exécutable.**

```bash
build/v11/mhgp11 --sortie=full --points=<x.u32le> --ids=<ids.u32le> --dossier=<D> --k=5 [--fils=48]
```

**Banc G4 de référence** : `bench/gpu_ab.py`, dans une session gardée `gcp-migration/v11_session.py`, avec la cible
`us-central1-c` / `ehgp-v7-3b1d496aed430749ea7e049f` et `--max-run-seconds 4200`. Certifier `TERMINATED` après chaque
session. Exemple du mode GPU K5 de référence :

```bash
python3 morsehgp3D_v11/bench/gpu_ab.py --src <src> --work <w> --data <d> --out <o> \
  --modes gpu=868347:400 --kmax 5 --leaf 24 --reps 6 --warm-passes 10
```

**Matrice** : `tools/g4_matrix.py`, configurée par `tools/g4_matrix.json`.

**Contraintes du lanceur** :
- il n'accepte que `./mhgp11*`, `ctest` et `python3 {src}/morsehgp3D_v11/<script>.py` ;
- il exige environ 1,15 Go libres sur `/workspaces`, plus deux fois le plafond des résultats ;
- TSan exige `setarch -R`.
