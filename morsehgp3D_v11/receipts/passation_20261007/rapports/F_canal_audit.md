# Audit géant v11 : le canal d'audit externe et le dialogue avec le développeur

`phase=exploration_v11_hors_registre / backend=cpu_reference / profile=quantized_u21_input_only / public_status=not_claimed`. Lecture seule de l'instantané `ac081a06f` (7 oct., 03:43 UTC) et de `git log` ; aucune compilation ; GCP non utilisé. Tout énoncé non marqué est **vérifié** dans le fichier ou le commit cité ; **[I]** marque une inférence. Les chemins sont relatifs à `morsehgp3D_v11/`.

## 1. Périmètre et état du canal d'audit

**Acteurs.** Deux auditeurs externes suivent la v11 depuis l'ouverture (`52687f8e5`, 2 oct., 06:11).
- **L'« indépendant ».** Il publie 25 minutes après l'ouverture (`27236bfd0`). Il tient ensuite les deux notes vivantes, avec des sous-relecteurs (« racine », « relecteur natif », « auditeur mathématique »).
- **Le « continu ».** Il écrit `AUDIT_OUVERTURE_ET_REPRISE_V10`. Dès le 2 oct. à 10:45, il y écrit qu'il est « devenu développeur sur instruction de l'utilisateur » (`6a22a9118`). Sa note est gelée depuis le 3 oct. à 06:05 (`70e494777`).
- **Attribution des commits.** 34 des 38 commits sous `src/` des 2 et 3 oct. n'ont pas la ligne Co-Authored-By de Claude. [I] L'ex-auditeur a donc écrit les fondations, et Claude a repris le développement le 3 au soir. Pendant ces deux jours, un seul auditeur relisait vraiment.
- **Aucune pièce n'est signée.** Tous les commits portent l'auteur Git `Ludwig-H`. Seule la forme du sujet distingue auditeurs et développeur.

**Pièces.**
- **Notes d'audit.** La note moteur `AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE` (89 commits, ~17 500 mots) et la note mathématique `AUDIT_REPONSES_AUX_VERROUS_MOTEUR` (74 commits, ~9 500 mots) sont réécrites en place. Le `README.md` a été réécrit 86 fois.
- **Côté développeur.** `REPONSE_CLAUDE_SUPPORTS_20261004` (sections A à Y, ~12 400 mots) est devenu le canal général malgré son nom. S'y ajoutent `QUESTION_CLAUDE_VITESSE_100MS` et trois fichiers sur le polyèdre.
- **Reçus.** 77 dossiers `audit_*` plus `eom_exact_audit` : 102,5 Mo et 7 482 fichiers, sur 192,8 Mo et 9 774 fichiers pour tout `receipts/`. On y compte ~2 000 copies de sources C++. Environ 19 autres reçus d'auditeurs portent d'autres préfixes (`flat_*`, `points_*`, `hm_*`, `proposition_*`).
- **Commits.** Environ 100 commits d'auditeurs, identifiés par leur préfixe, du 2 au 6 oct.

**Moyens.** Les auditeurs n'ont ni compilé ni exécuté de code natif. Leurs preuves :
- sources figées par SHA ;
- modèles Python stdlib rejoués en normal et sous `-O` ;
- patches vérifiés par `git apply --check` ;
- contre-lectures des 22 sessions G4 du développeur (`audit_g4_*`).

Une seule campagne G4 a été menée côté audit : FULL → points, le 3 oct. (`full_points_20261003`, 17 exemples, 68 fits HDBSCAN).

**État à l'instantané.**
- **Dernier passage des auditeurs.** Leur dernier commit est `28d70f8ab` (6 oct., 22:08).
- **Commits jamais relus.** Depuis, ~40 commits du développeur n'ont pas été relus :
  - publieur O2 (`13a4a0a4c`) ;
  - lot de feuilles partagé (`2045ec27c`) ;
  - tranches de cohortes (`5734ca6e8`) ;
  - cache de blocs du budget (`ccdd4db75`) ;
  - qualification `claudev3q1`/`q2` ;
  - préchargements et filtre AVX2, depuis retirés.
- **Sans réponse.** Les sections X et Y, ainsi que `REPONSE_CLAUDE_POLYEDRE_ORDRE_K_RESULTATS_20261007`.
- **Contrat de 100 ms toujours ouvert.** À K5 à chaud : mur 240–290 ms, `domain` 137–163 ms (section Y). À K10 : 1,66–2,23 s (section U).

## 2. Constats majeurs corrigés (avec preuve)

| Classe | Constat | Correction | Preuve |
|---|---|---|---|
| Contrat numérique | La borne flottante F3, fondée sur un compte d'opérations, est fausse : ~8u > 7u (`27236bfd0`) | Borne par expression ; F4 limitée aux clés > 0 (`93ba16112`, `f2ebb7a08`) | `docs/ARCHITECTURE.md` § 4 |
| Faux verts d'outillage | Mutant compté « tué » alors que le programme est absent ; juge d'arrêt anormal trompé par une ligne imitée ; `StageTimer` ; `memcpy(nullptr,0)` | Runner par `exec` (126/127 → INVALIDE), juge POSIX, gardes | Fondations requalifiées sur G4 (`audit_independant_20261002`) |
| Concurrence et largeur | P1 : lecture d'un ordre abandonné après réveil. P2 : export u24 sur 3 mots pour un niveau de 196/148 bits (`e02a6c235`) | `3bd4d734e`, `eb036dbe2` | claudequal2 : 686/686 en Release, 611/611 sous sanitizers, mutants tués |
| Capacité | Garde « feuilles ≤ sites » fausse (80 feuilles pour 9 sites : claudegpu2 refusait toute trame) ; débordements u32 de p+q et du nombre de traces | `00800dd88` ; garde p ≤ 11 ; refus `tower_capacity` | `full_leaf_lanes`, portes S6a/S3 |
| Bancs vacants | Banc GPU « conforme » sans prise froide ; porte sans copie non vide ; lecteurs qui perdent refus et identité | `22a6af6aa`, `57dd21be1`, `d5b1d0179`, `61da03749` | Gardes AST |
| API | `Provenance{}` exigée en succès ; produit publiable par une autre Session ; SIGXFSZ mortel | `a5e4019b4` et correctifs S5 | Session A sur 4 profils ; mutants API 23/23 en R3 |
| Chemin de refus | S9 : refus au milieu d'un `std::sort`, avec lecture hors bornes à 17 entrées (`d448b3d03`) | `3d47eaa93` (tri par tas) | finb, R1 |
| Numérique | S10 : la racine 2^127−1 fait déborder un i128 (`100fcc12b`) ; S10 publiée 23 min après ce constat | `510dae50e`, `38b76701b` | fina2, finb, mutant `racine_non_bornee` |
| Portée réduite | Portes sanitizer d'échelle et 4 différentiels S9 retirés pour tenir l'échéance (`c97776ea8`) | `a7711b506`, `8b2ca400e` | fins, finp9, finp10 |
| Attendus de portes | SHA u21 imposés à u18/u24 ; mutant `sp_masque_16379` inatteignable ; label LiDAR impossible ; décision de banc périmée | `be05bfad8`, `98a009550` | R1–R4 : 3 695 portes, 485/485 mutants, 80/80 ASan u24 |
| Reçu | « Six trames » au lieu de trois ; finl à 35/41 ; R3 sur base u18 (`df904711a`) | Erratum adopté (§ L) | Reçu immuable inchangé |
| Sortie supports | Cycle au même plateau : triangle K1 à 3 supports (`be8085ec1`) ; différentiel qui recopie le filtre ; manifeste v2 accepté avec un binaire v1 | `07428324e`, patch de l'auditeur intégré à l'octet | claudesupkr 15/15, en u21 seulement |
| Validation | Un site entre dans un bloc déjà fusionné ; le lecteur et `check_shape` l'acceptent | `b0f2a0a9e` | Normal/`-O` ; mutants G4 annoncés par le développeur, non relus |
| Statistique | IoU arrondies avant le seuil strict 1/2 | `38faaf272` | Effet sur les résultats publiés non établi |
| Mesure | Placement perdu au déplacement : l'A/B était en fait un A/A | `9d10de213` (vu sur G4 par le développeur), témoin `12ce8f8f0` | claudeo1place3 |
| Mutants et juges | Mutants d'étendue éliminés en u18 ; cache des variantes indexé par le nom ; juge GPU qui accepte un registre absent ; lecteur de campagne en désaccord sur 48 cas sur 1 024 | `38faaf272`, `86b3cbf14`, `12ce8f8f0` | claudev3q1, selon le développeur |
| Mathématiques | D2 : β(F) ≤ ℓ(r_b−1) est faux (41 < 64 < 1681/25) ; pas de hiérarchie laminaire commune aux ordres K (témoin à 7 points) ; coquille mixte ; q4 du cube à K1 ; frontière K = n ; trois réfutations sur le polyèdre | Registre `false_in_general` et fixtures, `1fbeea5b8` ; EOM exact adopté (`src/head/score.cpp`) | Oracle S1, 13 mutants S1 |

Trois défauts ont été clos par retrait du code, sans correction qualifiée :
- la garde CUDA et les mutants injugeables de la feuille coopérative (`d4228f5e5`) ;
- la durée de vie sur refus de L4 (`830473218`) ;
- l'arène N1 (`c1675e4c9`).

Aucun résultat FULL faux n'a été établi, et les 22 sessions G4 relues ne révèlent aucun défaut d'exactitude du moteur.

## 3. Constats contestés ou restés ouverts

**Contestations : presque aucune.**
- **Le développeur adopte presque tout.** « Aucune n'est contestée » (§ A), « tout est adopté » (§ F), « les sept contrats sont adoptés tels quels » (`QUESTION_CLAUDE_VITESSE_100MS` § C).
- **Quatre suggestions non adoptées.**
  - `calls == fail_at` dans la porte de tri ;
  - la sentinelle de rapport ;
  - la lecture de `V11_SOURCE_PIN` (`bench/sorties_g4.py:251` recopie toujours `--commit`) ;
  - la proposition « fill CTA ».
- **Une métadonnée jamais corrigée.** `developpement_20261005/qualification_sorties` attribue claudequalA à `b319efc84` au lieu de `00bd979ac`.
- [I] Cette adoption presque totale tient davantage de la déférence que du débat.
- **Seule contestation de principe.** Elle vient de l'ex-auditeur devenu développeur : « Une affirmation d'auditeur ne remplace ni preuve, ni compilation, ni exécution. »

**Désaccords tranchés par la mesure, ou restés ouverts.**
- **M3/E4.** Exacts mais coûteux : retirés de la voie CPU.
- **N1.** L'hypothèse « allocation et atomiques » est réfutée par claudeN1. Le développeur conclut au SMT ; l'auditeur répond « diagnostic exclusif SMT non établi ».
- **Nsight.** « ALU 24 % n'exclut pas un coût critique de l'i128. »
- **U1.** La lenteur de la variante A reste inexpliquée.
- **Juge Euler/J1.** C'est un filet, pas un certificat : il réutilise `num`, et des compensations restent possibles.
- **Erreurs des auditeurs eux-mêmes.** Une lecture favorable du placement n'établissait pas son activation. Des harnais ont échoué (c'est consigné). La contre-épreuve FULL partage les MEB de l'étage A.

**Ouverts.**
- **Objet et contrats.**
  - 100 ms (et le jalon de 200 ms), K10.
  - Le différentiel canonique v10/v11 sur trames entières, exigé par `AGENTS.md`, n'a jamais été clos.
  - Une seule séquence (08) a été mesurée.
  - L'écart avec la v10 (×1,50–1,72) est descriptif, pas un A/B.
- **Qualification.**
  - Kruskal en u18/u24, avec mutants et sanitizers ; triangle K1 à W48.
  - Empreinte par passe chaude ; mémoire physique du GPU.
  - Portes GPU natives (q3 extrême, q4 à 2^20 et 2^20+1, préfixe obtus, qmin = 2) : déclarées ouvertes le 4 oct., couvertes en partie depuis (`leaf_narrow`), jamais closes.
  - Isolation des processus descendants « non certifiée » ; Clang jamais qualifié ; recoupement JUnit/CTest absent ; porte d'interopération des archives ; sklearn 1.9.1 contre 1.7.2.
  - Le runner perd toujours les diagnostics : pas de `--output-on-failure` (`tools/g4_matrix.py:735`).
- **Code.**
  - Le repli des feuilles non résolues reste séquentiel (`src/catalogue/single_pass_batch.cpp:142`), alors que la conception U2 est approuvée.
  - La forêt GPU O7/O8 n'est pas portée.
  - [I] Les extrema q2 couplés ne sont pas portés.
- **Documents.**
  - La formule de `docs/CATALOGUE.md:158` reste 8C⌈C/64⌉, au lieu de 16C.
  - Le corrigendum du préenregistrement E1 manque.
  - `receipts/audit_dialogues_20261004/README.md` contient un lien mort vers `audits/REPONSE_CLAUDE_POINTS_20261003.md`.

**Questions du développeur sans réponse.** [I] Sur ~53 questions, une quarantaine ont reçu une réponse ; toutes les autres datent du 6 oct. après 14:32.
- **T1–T2** (prédicteur du travail d'une feuille, découpage des feuilles lourdes). Pas de réponse écrite. Le développeur a tranché seul : estimateur m³ (`2045ec27c`), voie `344059:400` adoptée.
- **V1–V2.** Invariant structurel par nœud de l'arbre radix ; existence d'un contrat de budget qui supposait la forme médiane.
- **X (reprise en Y3).** La garde du préchargement d'O2 (lecture d'un bloc non confirmé) ne peut pas être mutée sans sanitizer. TSan reste son seul juge (claudeo2a, 800/800).
- **Y1.** Pourquoi les pages de 2 Mio perdent sur 48 fils (mur ×1,064). Piste fermée sans cause.
- **Y2.** Comment compter honnêtement la réutilisation des blocs. Le développeur a tranché seul par `ccdd4db75` : `used`/`peak` inchangés, blocs inactifs sous une borne propre. Adopté par claudecache1, ce changement du contrat mémoire n'a jamais été relu.
- **Polyèdre** (fichier RESULTATS, huit questions) :
  1. le représentant causal vaut-il certificat sur toute la plage ?
  2. quelle identité pour les jetons portés par des nœuds de vie < 2δ ?
  3. le certificat κ corrigé est-il valide ?
  4. sous le plancher des sommets : registre r+λ, ou annulation des paires de persistance < ε ?
  5. appariement entre intervalles de même naissance ;
  6. les deux trous de l'exemple du § 6.1 ;
  7. approximations par Delaunay d'ordre 1 et dual de SCov ;
  8. retours fusionnés.

  Restent aussi ouverts : aucun constructeur natif de A_k(r) ; un certificat KKT seulement en Python ; une taille au coût LiDAR d'environ 1 000 faces par site à K5 ; une quatrième réfutation non gravée.

## 4. Thèmes récurrents et angles morts

1. **Portée surévaluée.** C'est la correction la plus fréquente. Transferts implicites : de Python au natif, du local à G4, de u21 à u18/u24, du WIP au commit, du dernier dump à toutes les passes, de A+C à C. S'y ajoutent des « dumps » qui ne sont que des empreintes, et les « six trames ».
2. **Faux verts et portes vacantes.**
   - Mutants équivalents, inactifs, non compilables, ou visant un CTest inexistant.
   - Fixtures qui ne discriminent pas (une boîte contient tous ses sites).
   - Attentes impossibles (`near_max` en u18, label LiDAR).
   - Différentiel qui recopie le produit ; plancher saisi à la main ; banc qui décide sans variante mesurée.
3. **Profils multiples (u18, u21, u24).** Première source de défauts de qualification : références gravées dans un seul profil, branches éliminées par `if constexpr`, largeur d'export.
4. **Bornes et chemins de refus.**
   - Casts u32, conversion de u128 en i128.
   - Invariants géométriques supposés sans preuve.
   - Refus au milieu d'un tri, durées de vie sur refus.

   Les verts G4 ne couvraient pas ces chemins.
5. **Égalités et plateaux.** Multifusions, cycles de Kruskal, coupes ouvertes ou fermées, inégalités strictes.
6. **Attribution de performance.**
   - Le nombre d'instructions ne fait pas le temps : V3 retire 44 % des instructions pour −7,5 % sur les forêts.
   - Le SASS statique et un essai local sur 8 cœurs ne prédisent pas le temps sur G4.
   - Le bruit A/A atteint ±10 %.
7. **Angles morts du développeur.**
   - Écrire le natif avant la réponse sur le contrat (§ D).
   - Réduire le périmètre de qualification sous l'échéance.
   - Graver les attendus dans un seul profil.
   - Laisser un diagnostic sur le chemin chaud.
   - Négliger la sémantique de déplacement C++ et l'ordre de destruction.
   - Écrire le juge avec la logique du produit.
   - Publier avant d'intégrer un constat (S10).
   - Ne pas vérifier les métadonnées des reçus.
   - Omettre le préflight d'outillage (g++, numpy, données).
8. **Gouvernance.** Des rôles qui changent en route, un auteur Git unique, des notes géantes, des préfixes de reçus hétérogènes, des patches « à ne plus appliquer ».

## 5. Fonctionnement du canal : ce qui a marché, ce qui a coûté

**Rythme.** Le canal tourne en continu, de jour comme de nuit.

| Échange | Délai de réponse |
|---|---|
| Q1–Q5 à l'ouverture | 26 min |
| Questions sur les points (3 oct.) | 42 min |
| R1–R7 | 53 min |
| Sections E, G, O, R | 19, 15, 18 et 5 min |
| Sections P, S | 48 et 53 min |
| Section U | 2 h 42 |
| Polyèdre (deux échanges) | 32 et 18 min |
| Capsule G4 après l'arrêt d'une session | 4 à 52 min |

Les correctifs suivent le constat de 10 minutes à 3 heures. Les auditeurs lisent les WIP du développeur avant commit : `6eba951df` paraît à la minute même de la section I.

**Formats.** La règle d'ouverture (`52687f8e5`) prévoyait des fichiers datés et ancrés, jamais modifiés par un autre acteur, et une demande de fenêtre G4 par `QUESTION_AUDITEUR_*`. La pratique a dérivé :
- notes maintenues en place, en ordre antichronologique, où état courant et historique se mélangent ;
- aucun fichier `QUESTION_AUDITEUR_*` ;
- un fichier du développeur dont le nom ne correspond plus au contenu ;
- des reçus hors préfixe.

**Coûts.** Aucune dépense G4 côté audit, mais leurs reçus représentent la moitié du volume de `receipts/`. Sur les 22 sessions relues (environ 8 à 9 heures-VM, [I]) :
- 6 ont été coupées par le budget de 2 100 s ;
- 10 ont révélé un défaut de porte, de sélection, de données ou de métadonnées ;
- aucune n'a révélé de défaut d'exactitude du moteur.

Une seule erreur d'attendu a produit trois sessions rouges, puis une reprise de quatre sessions, close en moins de cinq heures.

**Ce qui a été le plus utile.**
- **Des témoins exacts minimaux, gravés en fixtures** : triangle K1, D2, 2^127−1, 17 entrées, sphère x²+y²+z² = 5, 9 sites pour 80 feuilles, 10001/20001.
- **Des patches prêts, intégrés à l'octet** (`07428324e`, `38faaf272`).
- **La relecture des WIP avant publication.**
- **Le comptage nominatif PASS / Failed / sans résultat, mutant par mutant**, qui a empêché tout vert par absence.
- **Les revues de conception avant écriture** (sections O et Q).
- **Les propositions prouvées avant le code** : q3 différé, EOM exact, bornes sur sites entiers (devenues V3).

**Ce qui a coûté.**
- Des matrices plus longues que le budget, des attendus faux, des données absentes du paquet.
- [I] Des audits approfondis de variantes retirées peu après : la feuille coopérative ; N1 ; L4, relu 9 minutes avant son retrait.
- Des captures de WIP périmées dans l'heure.
- Des notes où l'état courant est difficile à retrouver, et des réserves de portée répétées.
- Des capsules non autonomes : leur rejeu dépend de `/workspaces/.ehgp-sessions`.
- Le chantier polyèdre, ouvert le 6 au soir, juste avant que le canal se taise.

## 6. Recommandations pour l'audit de la v12

1. **Un registre lisible par une machine dès le jour 0.**
   - Un identifiant par constat et par question, avec : classe, gravité, pin, témoin, état (`ouvert`, `corrigé_source`, `qualifié_G4[pin, profil]`, `contesté`, `clos_par_retrait`, `sans_objet`), preuve de clôture (commit, porte, reçu) et échéance.
   - Contrôle par un `tools/check_*` en CI.
   - Des notes vivantes d'une page ; l'historique va dans les reçus.
   - Chaque pièce signée par son auteur.
2. **Des contrats écrits et relus avant toute ligne native.** Généraliser la séquence contrat + oracle borné (L0, S0–S1), qui a bien fonctionné.
   - (a) **Numérique** : budget de bits par expression, conversions gardées, références gravées par profil.
   - (b) **Capacité** : hôte, mémoire épinglée et device, coexistences, arènes et caches comptés.
   - (c) **Refus** : une porte d'injection native de bout en bout pour chaque chemin.
   - (d) **Qualification** :
     - lots dimensionnés sur des durées mesurées, avec un budget par lot ;
     - un préflight qui exécute le juge sur un inventaire entièrement PASS et vérifie données et outillage ;
     - « sans résultat » n'est jamais un PASS ;
     - interdiction de retirer une porte pour tenir l'échéance ;
     - provenance lue chez le worker ;
     - sorties d'échec et compteurs conservés.
   - (e) **Mutants** : profil et porte déclarés, témoin non muté obligatoire, atteignabilité recontrôlée après tout reroutage.
   - (f) **Mesure** : règle écrite avant les données, bras A/A, plusieurs séquences LiDAR.
   - (g) **Différentiel canonique contre la v11** sur trames entières, dès la première tranche.
3. **Des rôles stables.**
   - Deux couloirs : mathématiques et contrats d'un côté, moteur et qualification de l'autre.
   - Un auditeur ne devient pas développeur sans qu'on le remplace.
   - Les auditeurs restent hors du natif, mais reçoivent un paquet de session haché et durable.
4. **Pièges à éviter.**
   - Auditer à fond une variante avant un prototype mesuré.
   - Laisser filer des dizaines de commits sans relecture. Tout changement de budget mémoire, de concurrence ou de format doit être relu avant adoption (cf. `ccdd4db75`).
   - Copier les sources dans les reçus plutôt que citer leurs SHA.
   - Tenir une capture de WIP pour une qualification.
5. **Un arriéré initial.** Faire des points ouverts du § 3 (X, Y1, Y2, V1, V2, polyèdre, différentiel, runner) les premiers constats du registre v12.

## 7. Références clés

- **Canal** :
  - `audits/README.md` ;
  - `audits/AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md` ;
  - `audits/AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md` ;
  - `audits/AUDIT_OUVERTURE_ET_REPRISE_V10_20261002.md` ;
  - `audits/REPONSE_CLAUDE_SUPPORTS_20261004.md` (§ A–Y ; questions ouvertes en § T, V, X, Y) ;
  - `audits/QUESTION_CLAUDE_VITESSE_100MS_20261004.md` ;
  - `audits/QUESTION_CLAUDE_POLYEDRE_ORDRE_K_20261006.md` ;
  - `audits/REPONSE_CLAUDE_POLYEDRE_ORDRE_K_20261006.md` ;
  - `audits/REPONSE_CLAUDE_POLYEDRE_ORDRE_K_RESULTATS_20261007.md`.
- **Audits généraux** : `receipts/audit_independant_20261002/`, `receipts/audit_giant_20261004/`, `receipts/audit_deep_20261004/`, `receipts/audit_dialogues_20261004/`, `receipts/audit_geant_20261005/`.
- **Qualification** : `receipts/audit_g4_fina2_20261005/supports_route/`, `receipts/audit_g4_finm_20261005/cli_mutant/`, `receipts/audit_g4_repriser4_20261005/ordinary_union/`, `receipts/developpement_20261005/qualification_finale/`.
- **Sorties et GPU** : `receipts/audit_supports_mst_20261006/`, `receipts/audit_supports_mst_followup_20261006/`, `receipts/audit_integration_20261006/`, `receipts/audit_narrow_followup_20261006/`, `receipts/audit_placement_activation_20261006/`, `receipts/audit_gpu_euler_20261004/`.
- **Polyèdre** : `receipts/audit_hartigan_delaunay_20261006/`, `receipts/audit_hartigan_robustesse_20261006/`, `receipts/polyedre_ordre_k_20261007/`.
- **Pièces jamais relues** : `receipts/developpement_20261007/cache_blocs/`, `receipts/developpement_20261007/thp_exploration/`.
- **Code encore concerné** : `tools/g4_matrix.py:735`, `bench/sorties_g4.py:251`, `src/catalogue/single_pass_batch.cpp:142`, `docs/CATALOGUE.md:158`.
