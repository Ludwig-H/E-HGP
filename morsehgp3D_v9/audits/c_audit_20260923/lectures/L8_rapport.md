# Lentille 8 — État et hygiène du dossier `morsehgp3D_v9/audits/` et du canal v9

Cadre : `phase=exploration_v9_hors_registre`, `backend=reference_cpu`, `profile=quantized_u18_input_only`, `mode=audit_independant`, `public_status=not_claimed`. GCP non utilisé. Aucune écriture dans un dépôt ou un worktree. Aucune compilation. Seuls ont tourné de petits scripts Python en lecture seule (`nice -n 19`, quelques secondes CPU), placés dans `scratchpad/agents/lentille8/` : `links.py` (liens et ancres), `attrib.py` (regroupement des commits), `hashes.py` (commits cités), `lint_audits.py` (règles de `tools/check_docs.py`).

## 0. Ancrage et dérive pendant l'audit

- Base demandée : `0125dc18`. À cette date, le dossier compte **104 entrées de premier niveau** : 102 fichiers et 2 sous-dossiers, soit 116 fichiers en tout. On y trouve 67 notes `.md` de premier niveau, 21 sondes `check_*`, 13 fichiers `shadow_*`, `q34_block_shadow_20260923.cpp`, et les deux sous-dossiers `phase_a_20260923/` (10 fichiers) et `q4_global_12sites_20260923/` (4 fichiers).
- `origin/main` a avancé pendant l'audit : `c02d45ac`, puis `87ccf5fc`, `7f218872`, `cab281d8`, et enfin **`bbc41a9c`** (08:26 UTC). Ces commits ajoutent 7 notes, le dossier `c_euler_20260923/` de l'auditeur C, et modifient `ETAT_COURANT.md`, `LEDGER_VISITES_CACHEES_Q34_20260923.md`, `OBSTACLES_GPU_SOUS_SECONDE_20260923.md` et `Q4_INDUCTION_ATLAS_EVENEMENTS_20260923.md`. Le tableau du § 11 couvre les deux états.
- Le commit « non poussé » `4cde1502` cité dans la tâche est **obsolète**. Aucune référence ne l'atteint plus. Son contenu est arrivé sur `main` sous `4530644b`, à 07:44 UTC (sonde v12).

## 1. Auteurs : ce que Git permet et ne permet pas d'établir

- Les commits du dossier sont tous signés `Ludwig-H`. Aucun ne porte de ligne d'attribution (`git log --format=%B` sur `morsehgp3D_v9/audits` : 0 `Claude-Session`). Git ne distingue donc ni A, ni B, ni C, ni le développeur.
- L'attribution vient du nom de fichier (`*_A_*`, `*_B_*`, `PISTE_B_*`), du texte (« Contrelecture de l'auditeur B », `MEB_PROPOSITION_EXACTE_20260923.md:3`), du canal (B cite ses propres notes aux lignes 796, 821, puis 834, 1074 à `bbc41a9c`) et du regroupement des commits : un commit qui touche une note B déjà identifiée est attribué à B. Exemple : `1bbb38e6` retire trois notes B et amende `CONTRE_AUDIT_B_G4_R1_ET_SCHEMA_V2_20260922.md`. `be699a69` modifie `audits/morsehgp3D_v9/ETAT_COURANT.md`, le verdict propre de B.
- Environ 20 entrées restent **probables seulement** (marquées `A?` ou `B?` au § 11) : `AUDIT_DISTRIBUTION_SAMPLE_SORT`, `CACHE_TEMOINS_COUT_VALIDATION`, `CORE_LIDAR_LOCAL`, `OBSTACLES_GPU_SOUS_SECONDE`, `PHASE_A_GRAPHE_TEMPOREL`, `PROTOCOLE_TOUR_V10_V5_RUPTURE`, `Q34_BLOCS_LIDAR_SHADOW`, `Q4_INDUCTION_ATLAS_EVENEMENTS`, `SHADOW_HA_*` et leurs sorties, `DECOUPES_CAPTEUR_*`, `CROISSANCE_LIDAR_*`.
- `ETAT_COURANT.md` a été créé par le développeur (`3595725a`), puis tenu à plusieurs mains : 68 commits à `0125dc18`, de A (grappes `CONTRAT_COUTS`/`PREFETCH`) et de B (`e5319a93`, `5270f3df`).
- Le canal contient des **erreurs ou ambiguïtés d'attribution** :
  - Le filtre de ligne `a×B` est proposé par A (`CONTRAT_COUTS_ET_PARALLELISATION.md` § « Filtrer une ligne `a × B` », confirmé par `PISTE_B_Q34_RECTANGLES_AVANT_EXPANSION_20260923.md:8-9`). Le développeur l'appelle pourtant « la proposition B » (canal:377 ; `PASSATION.md:105`).
  - La palette par ancre vient de la note B `PISTE_B_…` (§ « Première porte shadow… », ligne 244, commit B `1140c176`). Le développeur l'appelle « piste A » (canal:626).

## 2. Le canal `audits/COORDINATION_MORSEHGP3D_V9.md`

- Préambule (lignes 3-10) : chaque entrée doit porter titre daté, rôle, commit de base et questions. Toute recommandation d'audit doit recevoir une réponse « acceptée, refusée avec raison, ou différée avec échéance ». Un worktree par acteur.
- À `bbc41a9c` : **47 entrées** (33 du développeur, 12 de B, 2 de C, **0 de A**). **7** seulement déclarent un commit de base (lignes 12, 38, 58, 94, 148, 983, 1097). **5** seulement se terminent par une question.
- **Chronologie non fiable.**
  - Ordre : les entrées B de 05 h 35 et 05 h 48 UTC sont en tête de fichier (lignes 12-56), avant l'ouverture du 22. Les entrées développeur « 06 h 00 » et « 06 h 40 » précèdent « 05 h 05 ».
  - Heures d'en-tête du développeur, comparées au `git blame` : « 05 h 20 » est commité à 04:08 UTC, « 06 h 40 » à 04:28, « 05 h 05 » à 05:07, « 08 h 40 » à 06:20, « 09 h 10 » à 06:25. Il n'y a pas de fuseau cohérent.
  - En-têtes datés dans le futur : l'entrée « 08 h 40 UTC » est commitée à 08:16 UTC (`06f71037`), alors que l'horloge hôte indiquait 08:19 au moment de mon contrôle. **C a le même défaut** : « 08 h 05 UTC » commitée à 07:55 (`93066733`), « 08 h 45 UTC » commitée à 08:25 (`cab281d8`).
  - Les en-têtes de B, eux, suivent l'UTC à ±2 min près.
- **Entrées B commitées par le développeur.** Trois entrées B sont entrées dans `main` par des commits du développeur, qui portent son `Claude-Session` :
  - 05 h 35 UTC par `458fb0ed` ;
  - 05 h 48 UTC par `6420ff90` ;
  - 06 h 08 UTC par `8e8b83a3`.

  B a donc écrit dans le worktree du développeur. C'est contraire à la règle des lignes 8-10, et la paternité Git de ces entrées est perdue.

## 3. Réponses du développeur aux recommandations

Correspondance faite par nom de note, hash cité ou sujet explicite.

- **Acceptées, avec commit** : contre-audits A d'ouverture (tableau du canal, lignes 123-146) ; protocole R2/v4/v5 (189-246) ; préparation FULL parallèle (274-283, 347-363) ; chargement des formes (365-373) ; noyau et réception v6/v8 (472-539) ; tri de fusion et R6 (568-603) ; MEB proposé (633-660, 741-746) ; rupture de schéma (675-685) ; filtre d'absence couvert par l'index exact (699-702) ; grand-livre caché, sonde v12 (canal:891-907 à `bbc41a9c`).
- **Refusée ou écartée avec raison** : filtre de ligne `a×B` (375-385) ; budget de nœuds (779-789).
- **Différées sans échéance** — la règle du préambule exige une échéance :
  - notes B pré-atlas et grandes coquilles, « retenues pour V9-2 » (158-162) ;
  - seuil K−2 des arêtes q4 seules, « retenu pour la suite » (211-212) ;
  - palette par ancre, « en réserve » (626-631) ;
  - occupation des seaux, « pas de compteur publié… dans cette livraison » (776-777).
- **Engagement non tenu** : « La R5 publiera RSS et temps par phase » (355-356). R5, R6 et R7b ne publient qu'un `rss_kb` de pointe par cas (`receipts/g4_tower_r5_20260923/SUMMARY.json`, clés ; `bench/tower_probe.cpp:95,237` à `bbc41a9c`, `peak_rss_kb`). `ETAT_COURANT.md:362-364` le redemande.
- **Sans réponse écrite**, dont des questions explicites :
  - question de B sur C6/tri v6 (canal:35-36) ;
  - obligation d'induction globale q4 (canal:821-832 ; `COMPLETUDE_Q4_CLE_REMBOURREE`, `Q4_INDUCTION_ATLAS_EVENEMENTS`) ;
  - couture de la phase A (canal:796-808 ; `PHASE_A_FULL_LOTS`, `PHASE_A_GRAPHE_TEMPOREL`, `PHASE_A_MAX_ID`) ;
  - `OBSTACLES_GPU_SOUS_SECONDE` ;
  - ticket possédé du cache (`CACHE_TEMOINS_COUT_VALIDATION`) ;
  - tri unique par histogramme/scatter (`AUDIT_DISTRIBUTION_SAMPLE_SORT`, −31 077 584 comparaisons) ;
  - mesure des répétitions de BallKey (`INTRUS_FULL_PREFIXE_EXACT`, `CONTRE_AUDIT_B_PREFIXE_INTRUS`) ;
  - collisions et RSS de l'index (`CONTRE_AUDIT_B_INDEX_CLES_FULL`) ;
  - plages arrondies du README R6 (`ETAT_COURANT.md:158-159`) ;
  - commentaire « Exact depth » (`ETAT_COURANT.md:72-73`), toujours présent à `src/gen/lanes/q34_dead_lanes.cpp:168` sur `bbc41a9c`.
- **Désaccord non arbitré** : `ETAT_COURANT.md` (à `bbc41a9c`, lignes 280-283) recommande de « tester d'abord un ticket borné de nœuds témoins réemployés ». Le développeur déclare que « la piste reste fermée sans partage conjoint du préfixe » (canal:429-431). B prépare une note sur ce sujet (§ 8).

## 4. Liens, commits cités, couverture de `check_docs.py`

- **Liens relatifs** (script `links.py`, résolution sur l'arbre Git) :
  - à `0125dc18` : 385 liens dans 70 fichiers (notes et canal), **0 mort, 0 ancre morte** ;
  - à `bbc41a9c` : 423 liens dans 75 fichiers, 0 mort ;
  - aux commits de retrait `85d79753`, `128fb231` et `1bbb38e6` : 0 mort.
- **Une coupure transitoire** hors couverture : à `1bbb38e6`, `audits/morsehgp3D_v9/ETAT_COURANT.md:203` pointait vers la note supprimée `CONTRE_AUDIT_B_GCP_SESSION_20260922.md`. Elle a été réparée à la main 8 min plus tard (`be699a69`).
- **Commits cités** (script `hashes.py`) : 4 hashes pré-rebase sont inaccessibles depuis `main` : `23074987`, `028067a3`, `f599aed7`, `e9000f9c`. Les notes B donnent l'équivalent sur `main` (« `cc4664e5` sur main »), sauf le canal:520 (`028067a3`, dont l'équivalent est `e5688680`).
- **Couverture de `tools/check_docs.py`** (lignes 107-113) : seul `morsehgp3D_v9/audits/ETAT_COURANT.md` est couvert. Ne sont pas couverts : les autres notes du dossier, `q4_global_12sites_20260923/README.md`, `phase_a_20260923/MANIFEST.md`, le canal `audits/COORDINATION_MORSEHGP3D_V9.md` et `audits/morsehgp3D_v9/*.md`.
- Appliquées à l'ensemble, les règles de `validate()` donnent **0 erreur** : 70 fichiers à `0125dc18` ; 75 fichiers à `bbc41a9c`, hors liens externes à l'archive.
- Étendre la couverture ne coûte donc rien aujourd'hui. Pour respecter la politique v4-v8 (« les mots des auditeurs… un registre que l'on ne reformate pas », `check_docs.py:43-49`), je propose un contrôle **des liens seulement** pour les notes d'auditeurs.

## 5. Doublons, grappes et contradictions

- **Deux verdicts v9** :
  - `morsehgp3D_v9/audits/ETAT_COURANT.md` : 578 lignes à `0125dc18`, 637 à `bbc41a9c`.
  - `audits/morsehgp3D_v9/ETAT_COURANT.md` (B) : 312 lignes, dernier contenu de fond `74aa8172` à 00:27. Il écrit encore que « Le protocole G4 v3 reste **bloqué** » (paragraphe `e6405952`). Il se déclare subordonné à la synthèse produit (ligne 47), mais son titre reste « État courant ».
- **Grappes à indexer** (ce ne sont pas des doublons stricts) :
  - phase A : `CONTRE_AUDIT_B_PARALLELISME_Q34_FULL`, `PHASE_A_FULL_LOTS`, `PHASE_A_GRAPHE_TEMPOREL`, `PHASE_A_MAX_ID`, `phase_a_20260923/` ;
  - complétude q4 : `Q4_STRUCTURE`, `CONTRE_AUDIT_B_PREATLAS`, `COMPLETUDE_Q4_CLE_REMBOURREE`, `Q4_INDUCTION`, `q4_global_12sites`, `Q34_PROPRIETAIRE_PASSAGE_AMONT`, `NOTE_C_INVARIANT_EULER` ;
  - coût q3/q4 avant expansion : `PISTE_B_*`, `DOMINATION`, `SHADOW_HA_*`, `Q34_BLOCS`, `CORE_LIDAR`, `CERTIFICAT_NOEUDS_CORE_LIDAR` ;
  - croissance LiDAR : `CONTRE_AUDIT_PENTE_LIDAR_LOCALE_PARTIELLE`, remplacée de fait par `CONTRE_AUDIT_LIDAR_SCALING_V12_LOCAL` sans mention explicite, puis `DECOUPES_CAPTEUR`, `CROISSANCE_LIDAR`.
- **Phrase obsolète sur le profil d'entrée** :
  - `CONTRAT_COUTS_ET_PARALLELISATION.md:7-9` (réécrit à `8175c752`, 03:37 le 23) : « Le float32 original reste le défaut d'entrée fixé en v8 ». Formulations proches : `Q3_STRUCTURE_ET_BORNES.md:3`, `Q4_STRUCTURE_ET_BORNES.md:11`.
  - Ces lignes contredisent la précision du développeur (canal:142), `README.md:8` (`profile=quantized_u18_input_only`) et `ETAT_COURANT.md:28-30`.
- **Politique de retrait incohérente** :
  - 9 notes B ont été supprimées (`85d79753`, `128fb231`, `1bbb38e6`), leur substance reportée dans les survivantes ;
  - 6 notes gardent `_WIP` dans leur nom alors que le code est publié (`CHARGEMENT_FORMES`, `FULL_PARALLELE`, `NOYAU_DIAMETRAL`, `Q34_PREUVE_CONJOINTE`, `Q3_FEUILLE`, `RECEPTION_V8_GARDES`) ;
  - la règle énoncée par B à C (canal:1008-1015 à `bbc41a9c`) est pourtant « Une note remplacée reste accessible et marquée historique, pas supprimée implicitement ».
- **Entrées sans lien entrant** (rien ne les cite, ni `ETAT`, ni les notes, ni le canal, ni `README`/`PASSATION`) :
  - `OBSTACLES_GPU_SOUS_SECONDE_20260923.md`, à `0125dc18` comme à `bbc41a9c` : c'est pourtant la note la plus directement liée au contrat GPU sous la seconde ;
  - `CONTRE_AUDIT_A_MESURES_PLAN_20260922.md` : répondue seulement par hash ;
  - sonde orpheline `check_q34_dead_owner_aba_20260923.cpp` : portée comme porte produit `mhgp9_gen_q34_dead_lanes_owner` (canal:422-428), mais aucune note ne la nomme.

## 6. `ETAT_COURANT.md` face au code et aux reçus

- **Vérifié exact** :
  - produit `ec6d1b74` à `0125dc18` : `src/` inchangé depuis ; seul `bench/run_lidar_scaling.py` a été ajouté par `6fe8ec7a` ;
  - échantillonnage `16×4W` par slot (`src/chain/tower_chain.cpp:96-100`) ;
  - séparateurs splitmix64 (`src/tower/parallel/pool.hpp:168,188`) ;
  - libération de `key_slots` (`full_ball_tower.hpp:390`) ;
  - R7b, recalculé depuis `SUMMARY.json` : 3,67 s à K5 et 9,58 s à K10, digest 0,16 et 0,80 s, q3/q4 5,23 s, tour 3,20 s ;
  - `CPU·s/48` : 91/48 = 1,90 et 279/48 = 5,81.
- **Incomplet** : `ETAT_COURANT.md:158-159` relève que le README R6 omet 0,51 %, 8,66 % et 72,85 %. Mais son « CPU −10 à −20 % » exclut aussi les extrêmes 8,09 % et 21,18 % que l'`ETAT` publie lui-même (ligne 149). Mon recalcul exact sur `SUMMARY.json`, dont les champs sont arrondis, donne : chaîne 0,50–8,64 %, CPU 8,17–21,20 %, formes 72,85–81,87 %. Aucun erratum n'a suivi (`ERRATUM.md` ne traite que « générateur »).
- **Recommandation non exécutée** : commentaire « Exact depth » (§ 3).
- **Forme** : l'`ETAT` croît d'environ 6 lignes par heure (578 lignes, puis 637). Il mêle verdict et récit reçu par reçu, alors qu'il se présente comme « verdict mutable ».

## 7. État non commis comparé à l'état publié

- **Worktree partagé** `/workspaces/E-HGP`, `HEAD` `a74e90f2` : `morsehgp3D_v9/audits/` y est **entièrement non suivi** (`?? morsehgp3D_v9/audits/`, 108 entrées).
  - À 08:19 UTC, le seul brouillon était `CONTRE_AUDIT_LIDAR_SCALING_V12_LOCAL_20260923.md` (sha `b19f168f…`), publié à l'identique par `87ccf5fc` (08:21), puis révisé par `bbc41a9c`. La copie locale égale désormais `bbc41a9c`.
  - `QUESTION_CLAUDE_CONTRE_AUDIT_OUVERTURE_20260922.md` y est absent.
  - Le canal racine n'existe pas dans ce worktree.
  - Risque : un `git add -A` y indexerait tout le dossier.
- **Worktree développeur** `build/v9-open-worktree`, `HEAD` `06f71037` :
  - `M audits/COORDINATION_MORSEHGP3D_V9.md` : ajoute les deux entrées B « Clôture du contre-audit B… » et « Nœuds d'index… », **déjà publiées** par `87ccf5fc` et `7f218872`. Committer ou rebaser ce fichier provoquerait un conflit ou des doublons.
  - `?? CERTIFICAT_NOEUDS_CORE_LIDAR_20260923.md` : identique à `7f218872`.
  - `?? CONTRE_AUDIT_RECTANGLES_Q34_TICKET_BORNE_20260923.md` : mtime 08:32, **non publié**.
  - Des brouillons d'auditeur vivent donc dans le worktree du développeur. C'est le mécanisme qui a déjà fait entrer trois entrées B dans des commits développeur (§ 2).

## 8. Plan de rangement non destructif

Il respecte la propriété de chacun : A et B restent maîtres de leurs fichiers ; C n'écrit que ses fichiers `AUDIT_C_*`/`NOTE_C_*`, `README.md` et le canal (canal:983-1006). B et le développeur ont accepté l'index (canal:1008-1015, 1045-1047) ; **A n'a pas répondu**.

1. **`morsehgp3D_v9/audits/README.md`**, tenu par C, sans rien déplacer. Il comporte :
   - une ligne par entrée, avec un identifiant stable (`A-nn`, `B-nn`, `C-nn`, `D-nn`) ;
   - les colonnes : lien, auteur déclaré ou probable, commit et heure UTC de création, thème, **portée de preuve** (vocabulaire proposé par B : démontré localement, conditionnel, shadow, refusé, historique, mesure), **cycle de vie** (vivant, clos par `<hash>`, remplacé par `<fichier>`, retiré à `<hash>`), **réponse du développeur** (acceptée `<hash|ligne>`, refusée, différée `<échéance>`, sans réponse, sans objet), sondes attachées ;
   - des sections par thème : cadre, u18, q3, q4 et complétude, coût q3/q4, FULL (MEB, index, tri, phase A, résidence), chaîne, G4, LiDAR, GPU, hors v9 ;
   - une section « Notes retirées », avec le commit où chaque note reste lisible ;
   - une section « Hors dossier » (`audits/morsehgp3D_v9/`, reçus).
   - L'`ETAT_COURANT.md` y reste la synthèse.
2. **Registre des recommandations ouvertes**, dans ce README : une ligne par demande (§ 3), avec propriétaire et échéance. Le développeur y répond **dans le canal** en citant l'identifiant.
3. **Pas de déplacement des sondes existantes.** Au moins 23 notes les lient par chemin relatif. Quatre sondes contiennent leur propre chemin dans leur ligne de compilation (`check_plateau_u18_wide`, `check_q34_dead_owner_aba`, `check_q4_k5_interior_chain`, `shadow_q34_rows_u18`). S'y ajoutent `docs/HERITAGE_V7_V8.md:69` et `audits/morsehgp3D_v9/ETAT_COURANT.md:277`. Pour les **nouveaux** livrables : un sous-dossier `<thème>_<AAAAMMJJ>/` avec `README`/`MANIFEST`, sur le modèle de `phase_a_20260923/`, `q4_global_12sites_20260923/` et `c_euler_20260923/`. Un regroupement ultérieur en `sondes/` ne se ferait que par propriétaire, en un commit `git mv` plus réécriture des liens, avec son accord.
4. **En-tête normalisé** pour les nouvelles notes (5 lignes) : Auteur, Date UTC, Base ou snapshot, Statut, Réponse attendue. Rien de rétroactif sans le propriétaire.
5. **Canal** : ajout seulement en fin de fichier ; heure prise par `date -u` au moment du commit ; ligne `Base :` obligatoire ; rôle entre parenthèses ; questions numérotées. Chaque acteur commite ses propres entrées depuis son worktree ; le développeur vérifie `git diff --cached` avant tout `git add` du canal.
6. **Retraits** : ne plus supprimer. Marquer « historique, remplacé par… » en tête de note, et tracer la ligne dans le README.
7. **Couverture documentaire** (décision du développeur, propriétaire de `tools/`) : nommer `morsehgp3D_v9/audits/README.md` à côté d'`ETAT_COURANT.md`, et ajouter un contrôle des liens seuls pour `morsehgp3D_v9/audits/**/*.md`, `audits/COORDINATION_MORSEHGP3D_V9.md` et `audits/morsehgp3D_v9/*.md` (0 erreur aujourd'hui).
8. **Racine** : B requalifie le titre de `audits/morsehgp3D_v9/ETAT_COURANT.md` en « photographie historique au `74aa8172` ».

## 9. Inventaire complet

Légende.
- Auteur : `A`, `B`, `C`, `DEV` ; `A?` ou `B?` = probable.
- Statut : `V` vivant ; `C` clos ; `H` historique ; `N` négatif ou écarté ; `S` shadow ou diagnostic ; `R` retiré.
- Réponse : `Acc` acceptée ; `Acc~` partielle ; `Ref` refusée ; `Dif` différée sans échéance ; `SR` sans réponse ; `SO` sans objet.
- `Lnnn` = ligne du canal (au-delà de 832 : à `bbc41a9c`).
- Liens morts : 0 pour toutes les lignes.
- Heures UTC du 23 septembre, sauf mention « 22/ ».

| # | Entrée | Aut. | Créé | Thème | Statut | Rép. dév. | Remarque |
| ---: | --- | --- | --- | --- | --- | --- | --- |
| 1 | AUDIT_A_ARCHITECTURE_K_GABRIEL_20260922.md | A | 0674dc02 22/21:44 | cadre, route k-Gabriel | H | Acc L139 | 4 commits cités |
| 2 | AUDIT_DISTRIBUTION_SAMPLE_SORT_20260923.md | A? | cd0ce60c 05:30 | chaîne, tri | V | SR | tri unique −31,08 M comparaisons |
| 3 | CACHE_TEMOINS_COUT_VALIDATION_20260923.md | B? | 128fb231 06:26 | q34, cache | V | SR | ticket possédé |
| 4 | COMPLETUDE_Q4_CLE_REMBOURREE_20260923.md | B | 0a6efac3 06:58 | q4, complétude | V | SR | canal L821 |
| 5 | CONTRAT_COUTS_ET_PARALLELISATION.md | A | 0674dc02 22/21:44 | cadre, coûts | V | Acc~ L140, L429 | 13 commits ; phrase float32 |
| 6 | CONTRELECTURE_B_CELLULES_ET_CATALOGUE_20260922.md | B | 3cf72097 22/22:04 | q3/q4, sortie | H | Dif L160 | |
| 7 | CONTRE_AUDIT_A_MATH_MOTEUR_20260922.md | A | 0674dc02 | u18, FULL | C | Acc L129-135 | |
| 8 | CONTRE_AUDIT_A_MESURES_PLAN_20260922.md | A | 0674dc02 | plan, mesures | C | Acc L136-144 | 0 lien entrant |
| 9 | CONTRE_AUDIT_B_ANCHOR_MEB_DIAMETRE_20260922.md | B | ae88ff1f 22/23:36 | FULL, MEB | H | SR | voir #49 |
| 10 | CONTRE_AUDIT_B_CHARGEMENT_FORMES_Q34_WIP_20260923.md | B | faa76965 03:15 | q34 | C aae9da0e | Acc L365 | nom WIP |
| 11 | CONTRE_AUDIT_B_COVER_BATCH_20260923.md | B | bee0609f 00:09 | q34, cover | V | SR | piste secondaire |
| 12 | CONTRE_AUDIT_B_DOMINATION_Q4_20260923.md | B | 60ef2a95 00:20 | q4, gardes | H | SR | qualifie #45 |
| 13 | CONTRE_AUDIT_B_FULL_COUTS_ET_INTERFACES_20260922.md | B | b6bd8cdc 22/22:33 | FULL, coûts | H | SR | |
| 14 | CONTRE_AUDIT_B_FULL_PARALLELE_WIP_20260923.md | B | c6a9d18a 02:00 | FULL, parallèle | C | Acc L274, L347 | nom WIP |
| 15 | CONTRE_AUDIT_B_G4_R1_ET_SCHEMA_V2_20260922.md | B | 177fffbd 22/23:58 | G4 R1 | H | Acc L191 | |
| 16 | CONTRE_AUDIT_B_G4_R2_PREFLIGHT_20260923.md | B | 6de74c75 00:38 | G4 R2 | H | Acc L189 | R2 refusé |
| 17 | CONTRE_AUDIT_B_G4_R3_20260923.md | B | 2c40035b 02:06 | G4 R3 | H | SO | |
| 18 | CONTRE_AUDIT_B_G4_R4B_CACHE_20260923.md | B | f3d671af 02:59 | G4 R4b | H | SO | |
| 19 | CONTRE_AUDIT_B_G4_R5_20260923.md | B | 9d76d157 03:34 | G4 R5 | H | Acc L457 | |
| 20 | CONTRE_AUDIT_B_G4_R6_20260923.md | B | c17db454 05:11 | G4 R6 | H | Acc~ ERRATUM | plages non corrigées |
| 21 | CONTRE_AUDIT_B_G4_R7B_20260923.md | B | b46826e2 06:24 | G4 R7b | H | SO | |
| 22 | CONTRE_AUDIT_B_G4_RECEPTION_V5_20260923.md | B | f0929ea6 01:54 | G4, protocole | C a1d7a9bc | Acc L305 | cite 23074987 (hors main) |
| 23 | CONTRE_AUDIT_B_INDEX_CLES_FULL_20260923.md | B | 95e073a5 06:05 | FULL, index | V | Acc~ L741 | mesures SR |
| 24 | CONTRE_AUDIT_B_NOYAU_DIAMETRAL_WIP_20260923.md | B | aed84902 04:07 | q34, cœur | C | Acc L472-539 | nom WIP ; 2 hashes hors main |
| 25 | CONTRE_AUDIT_B_PARALLELISME_Q34_FULL_20260922.md | B | d4942b4e 22/23:33 | FULL, q34 | H, remplacé par #51 | SR | |
| 26 | CONTRE_AUDIT_B_PORTE_PUBLIQUE_T2_20260922.md | B | 174fb8db 22/23:18 | oracle T2 | H | SR | porte hors dossier (X3) |
| 27 | CONTRE_AUDIT_B_PORT_V3_SATURATION_TRI_20260923.md | B | 495edc0c 00:27 | G4 v3 | H | Acc L191 | |
| 28 | CONTRE_AUDIT_B_PREATLAS_ET_Q3_20260922.md | B | 2c6d806e 22/22:21 | q3/q4, pré-atlas | V | Dif L160 | fixture q4 sans faces |
| 29 | CONTRE_AUDIT_B_PREFETCH_FULL_20260923.md | B | bee0609f 00:09 | FULL, préfetch | H | SR | |
| 30 | CONTRE_AUDIT_B_PREFIXE_INTRUS_20260923.md | B | d6fc9fe0 00:22 | FULL, intrus | V | SR | |
| 31 | CONTRE_AUDIT_B_PREMIERE_CAMPAGNE_20260922.md | B | 174fb8db 22/23:18 | reçu local | H | SO | |
| 32 | CONTRE_AUDIT_B_Q34_PREUVE_CONJOINTE_WIP_20260923.md | B | f2301d91 02:25 | q34 | C 7f64a279 | Acc L289 | nom WIP |
| 33 | CONTRE_AUDIT_B_Q3_FEUILLE_WIP_20260923.md | B | bb3c696e 00:42 | q3, feuille | C | Acc L203 | nom WIP |
| 34 | CONTRE_AUDIT_B_Q4_K5_DEUX_INTERIEURS_20260922.md | B | d4942b4e 22/23:33 | q4 | H | SO | |
| 35 | CONTRE_AUDIT_B_Q4_NIVEAUX_ORIENTES_20260922.md | B | 177fffbd 22/23:58 | q4 | V | SR | |
| 36 | CONTRE_AUDIT_B_Q4_SHALLOW_20260922.md | B | 174fb8db 22/23:18 | q4 | H | SR | |
| 37 | CONTRE_AUDIT_B_QUOTIENT_COQUILLE_20260922.md | B | d4942b4e 22/23:33 | FULL, coquilles | V | Dif L160 | |
| 38 | CONTRE_AUDIT_B_RECEPTION_V8_GARDES_WIP_20260923.md | B | 39e37e9d 04:18 | G4, protocole | C cc4664e5 | Acc L514 | nom WIP ; 3 hashes hors main |
| 39 | CONTRE_AUDIT_B_RECU_VOIES_MORTES_20260923.md | B | f0929ea6 01:54 | q34, reçu | C | Acc L262 | |
| 40 | CONTRE_AUDIT_B_RESIDENCE_CHAINE_20260922.md | B | 0786d6c1 22/22:59 | FULL, résidence | V | Acc~ L352 | RSS par phase non livré |
| 41 | CONTRE_AUDIT_B_SAMPLE_SORT_PUBLIE_20260923.md | B | 610264da 05:17 | chaîne, tri | V | Acc~ L772 | compteurs Dif |
| 42 | CONTRE_AUDIT_B_U18_ET_SONDE_20260922.md | B | 0786d6c1 22/22:59 | u18 | H | Acc L158 | |
| 43 | CONTRE_AUDIT_B_WIP_V6_C6_TRI_20260923.md | B | 73d1d0a6 05:31 | v6, hors v9 | V | SR (question L35) | |
| 44 | CORE_LIDAR_LOCAL_20260923.md | A?/B? | 3a18c863 04:43 | q34, cœur | S | SO | cite f599aed7 (hors main) |
| 45 | DOMINATION_Q4_PARESSEUSE_PAR_BLOCS_20260923.md | A | 573c6869 00:17 | q4, gardes | V | SR | 11 commits |
| 46 | ETAT_COURANT.md | DEV→A+B | 3595725a 22/21:18 | verdict | V | n/a | 68 commits ; 578 lignes |
| 47 | INTRUS_FULL_PREFIXE_EXACT_20260923.md | A | 573c6869 00:17 | FULL, intrus | V | SR | |
| 48 | LEDGER_VISITES_CACHEES_Q34_20260923.md | B | 5270f3df 07:43 | q34, grand-livre | C 4530644b | Acc L891 | |
| 49 | MEB_PROPOSITION_EXACTE_20260923.md | B | 85d79753 06:03 | FULL, MEB | C 8e8b83a3 | Acc L741 | |
| 50 | OBSTACLES_GPU_SOUS_SECONDE_20260923.md | B? | c717a1f2 05:56 | GPU | V | SR | 0 lien entrant |
| 51 | PHASE_A_FULL_LOTS_ET_PARALLELISME_20260923.md | B | cb4add26 06:43 | FULL, phase A | V | SR | canal L796 |
| 52 | PHASE_A_GRAPHE_TEMPOREL_20260923.md | B? | 1bbb38e6 06:59 | FULL, phase A | V | SR | |
| 53 | PHASE_A_MAX_ID_COMPOSANTE_20260923.md | B | 5270f3df 07:43 | FULL, phase A | V | SR | |
| 54 | PISTE_B_Q34_NOEUDS_AVANT_COVER_20260923.md | B | f36c140c 01:59 | q34 | V | Dif (revue 06f71037) | |
| 55 | PISTE_B_Q34_RECTANGLES_AVANT_EXPANSION_20260923.md | B | de26dd7a 03:03 | q34 | V | Ref~ L375 ; Dif L626 | 16 commits, 554 lignes |
| 56 | PLATEAUX_GRANDES_COQUILLES_B_20260922.md | B | 3cf72097 22/22:04 | FULL, coquilles | V | Dif L160 | |
| 57 | PREFETCH_GEOMETRIE_FULL_PAR_K_20260923.md | A | 5cdeec30 00:03 | FULL, ordres K | C 684d8fc7 | Acc L331 | |
| 58 | PROTOCOLE_TOUR_V10_V5_RUPTURE_20260923.md | B? | b6b81ba4 05:47 | G4, protocole | C f55ea40c | Acc L675 | |
| 59 | Q34_BLOCS_LIDAR_SHADOW_20260923.md | A? | 867e68b3 07:25 | q34 | N | SO | |
| 60 | Q3_STRUCTURE_ET_BORNES.md | A | 0674dc02 22/21:44 | q3 | V | Acc~ L133 | 10 commits |
| 61 | Q4_INDUCTION_ATLAS_EVENEMENTS_20260923.md | B? | 28f0c284 07:15 | q4, complétude | V | SR | |
| 62 | Q4_STRUCTURE_ET_BORNES.md | A | 0674dc02 22/21:44 | q4 | V | Acc~ L130 | 12 commits |
| 63 | QUESTION_CLAUDE_CONTRE_AUDIT_OUVERTURE_20260922.md | DEV | 3595725a 22/21:18 | cadre | C | n/a | répondue 0674dc02 |
| 64 | RECEPTION_V6_IDENTITES_MANQUANTES_20260923.md | B | 8d6b2e41 02:48 | G4, protocole | C e5688680 | Acc L494 | |
| 65 | SEUIL_SATURATION_ATLAS_PAR_VOIE_20260923.md | A | 763d2b42 00:40 | q4, atlas | V | Dif L211 | |
| 66 | SHADOW_HA_OCTANT_Q34_LIDAR_20260923.md | B? | 128fb231 06:26 | q34, palette | S | SR | |
| 67 | SHADOW_HA_Q34_LIDAR_20260923.md | B? | 34c3164f 05:44 | q34, palette | S | Dif L626 | |
| 68 | check_cover_batch_u18_20260922.py | A | ce949e2e 22/23:09 | q34 | sonde | n/a | cité par #5, #11 |
| 69 | check_full_intruder_prefix_20260923.py | A | 573c6869 | FULL | sonde | n/a | #47 |
| 70 | check_meb_proposed_fenv_20260923.cpp | B? | 34c3164f | FULL, MEB | sonde | n/a | #46, #49 |
| 71 | check_phase_a_temporal_max_id_20260923.py | B | 5270f3df | FULL, phase A | sonde | n/a | #53 |
| 72 | check_plateau_u18_wide_20260922.cpp | B | 0786d6c1 22/22:59 | u18 | sonde | n/a | #42 ; chemin propre intégré |
| 73 | check_q34_core_stream_local_20260923.cpp | A?/B? | 3a18c863 | q34 | sonde | n/a | #44 |
| 74 | check_q34_dead_owner_aba_20260923.cpp | A | a4c7007b 03:25 | q34 | sonde **orpheline** | portée L422 | chemin propre intégré |
| 75 | check_q3_atlas_rational_location_20260923.py | A | ae406a4e 01:06 | q3 | sonde | n/a | #60 |
| 76 | check_q3_leaf_palette_20260923.py | A | 6c3629ae 01:27 | q3 | sonde | n/a | #60 |
| 77 | check_q3_shared_u18_20260922.py | A | ab34417c 22/23:34 | q3 | sonde | n/a | #46, #60 |
| 78 | check_q4_block_dominance_20260923.py | A | 573c6869 | q4 | sonde | n/a | #45 |
| 79 | check_q4_false_vertex_inside_disc_20260922.py | B | 174fb8db | q4 | sonde | n/a | #36 |
| 80 | check_q4_global_padding_20260923.cpp | B | 0a6efac3 | q4 | sonde | n/a | #4 |
| 81 | check_q4_k5_interior_chain_20260922.cpp | B | d4942b4e | q4 | sonde | n/a | #34 ; chemin propre intégré |
| 82 | check_q4_lane_threshold_20260923.py | A | 763d2b42 | q4 | sonde | n/a | #65 |
| 83 | check_q4_outward_levels_20260922.py | A | fa27bf31 22/23:55 | q4 | sonde | n/a | #35, #62 |
| 84 | check_q4_owner_dominance_20260923.py | B | 94c145ad 01:34 | q4 | sonde | n/a | #36 |
| 85 | check_q4_shallow_lines_20260922.py | A | efc14c99 22/22:47 | q4 | sonde | n/a | #62 |
| 86 | check_q4_without_q3_faces_20260922.py | B | 0f3d077a 22/22:41 | q4 | sonde | n/a | #4, #28, #61, #92 |
| 87 | check_qmin_planes_u18_20260922.py | A | 8054540c 22/22:19 | u18 | sonde | n/a | #7, #56, #62, X2 |
| 88 | check_shell_region_quotient_small_20260922.py | B | d4942b4e | FULL | sonde | n/a | #37 |
| 89 | check_u18_bounds_20260922.py | A | 0674dc02 | u18 | sonde | n/a | #7 ; docs/HERITAGE:69 |
| 90 | phase_a_20260923/ (10 fichiers) | B? | 1bbb38e6 06:59 | FULL, phase A | S | n/a | MANIFEST ; cité par #52 |
| 91 | q34_block_shadow_20260923.cpp | A? | 867e68b3 | q34 | sonde | n/a | #59 |
| 92 | q4_global_12sites_20260923/ (4 fichiers) | B? | 28f0c284 07:15 | q4 | porte ciblée | n/a | README et receipt ; cité par #46, #61 |
| 93 | shadow_ha_octant_00_k10_s8_replay_20260923.txt | B? | 128fb231 | q34 | sortie | n/a | #66 |
| 94 | shadow_ha_octant_00_k5_s8_replay_20260923.txt | B? | 128fb231 | q34 | sortie | n/a | #66 |
| 95 | shadow_ha_octant_01_k5_s8_replay_20260923.txt | B? | 128fb231 | q34 | sortie | n/a | #66 |
| 96 | shadow_ha_octant_q34_u18_20260923.cpp | B? | 128fb231 | q34 | sonde | n/a | #66 |
| 97 | shadow_ha_octant_resources_20260923.txt | B? | 128fb231 | q34 | sortie | n/a | #66 |
| 98 | shadow_ha_octant_rows_20260923.txt | B? | 128fb231 | q34 | sortie | n/a | #66 |
| 99 | shadow_ha_q34_u18_20260923.cpp | B? | 34c3164f | q34 | sonde | n/a | #67 |
| 100 | shadow_ha_scene00_k10_s8_replay_20260923.time.txt | B? | 34c3164f | q34 | sortie | n/a | #67 |
| 101 | shadow_ha_scene00_k10_s8_replay_20260923.txt | B? | 34c3164f | q34 | sortie | n/a | #67 |
| 102 | shadow_ha_scene00_k5_s8_context_20260923.txt | B? | 34c3164f | q34 | sortie | n/a | #67 |
| 103 | shadow_ha_scene01_k5_s8_context_20260923.txt | B? | 34c3164f | q34 | sortie | n/a | #67 |
| 104 | shadow_q34_rows_u18_20260923.cpp | A | a4c7007b 03:25 | q34 | sonde | n/a | #5 ; chemin propre intégré |
| R1 | CONTRE_AUDIT_B_GCP_SESSION_20260922.md | B | 4cd29558 22/23:07 | G4, protocole | R 1bbb38e6 (lisible à b0afa8c9) | Acc L171 | |
| R2 | CONTRE_AUDIT_B_PREMIER_G4_20260922.md | B | 177fffbd | G4 R1 | R 1bbb38e6, remplacé par #15 | SO | |
| R3 | CONTRE_AUDIT_B_G4_R4_PREVOL_20260923.md | B | b357ab4d 02:46 | G4 R4 | R 1bbb38e6 | SO | |
| R4 | CONTRE_AUDIT_B_PROTOCOLE_V4_WIP_20260923.md | B | fca6b807 00:50 | G4 v4 | R 128fb231 (lisible à 4d91329f) | Acc L236 | résumé dans #16 |
| R5 | CONTRE_AUDIT_B_VOIES_MORTES_WIP_20260923.md | B | 94c145ad | q34 | R 128fb231 | Acc L228 | |
| R6 | CONTRE_AUDIT_B_CACHE_TEMOINS_WIP_20260923.md | B | 50b5ae0d 02:29 | q34, cache | R 128fb231, remplacé par #3 | Acc L294 | |
| R7 | CONTRE_AUDIT_B_TRI_FUSION_WIP_20260923.md | B | 93b268e0 05:01 | chaîne, tri | R 128fb231, remplacé par #41 | Acc L570 | |
| R8 | CONTRE_AUDIT_B_MEB_PROPOSE_WIP_20260923.md | B | 4776b8f1 05:38 | FULL, MEB | R 85d79753 (lisible à 02d55856), remplacé par #49 | Acc L635 | |
| R9 | FULL_FILTRE_ABSENCE_CLE_20260923.md | B? | 34c3164f | FULL, index | R 85d79753 | Acc L699 | |
| D1 | CONTRE_AUDIT_PENTE_LIDAR_LOCALE_PARTIELLE_20260923.md | B | 4c3344ea 07:48 | LiDAR | H de fait, remplacé par D4 | Acc L1017 | |
| D2 | Q34_PROPRIETAIRE_PASSAGE_AMONT_20260923.md | B? | 4291c538 07:56 | q4/q34, amont | V | SR | |
| D3 | DECOUPES_CAPTEUR_LIDAR_BRUT_ET_GRILLE_20260923.md | A?/B? | c02d45ac 08:17 | LiDAR | V | SR | |
| D4 | CONTRE_AUDIT_LIDAR_SCALING_V12_LOCAL_20260923.md | B | 87ccf5fc 08:21 | LiDAR | C (« close ») | SO | brouillon vu à 08:19 |
| D5 | CERTIFICAT_NOEUDS_CORE_LIDAR_20260923.md | B | 7f218872 08:24 | q34 | V | SR | canal L1074 |
| D6 | NOTE_C_INVARIANT_EULER_20260923.md et c_euler_20260923/ | C | cab281d8 08:25 | q4/FULL, juge global | V | SR | |
| D7 | CROISSANCE_LIDAR_PLANS_ET_DENSITE_20260923.md | A?/B? | bbc41a9c 08:26 | LiDAR | V | SR | |
| D8 | CONTRE_AUDIT_RECTANGLES_Q34_TICKET_BORNE_20260923.md | B? | non publié (worktree dév., 08:32) | q34 | brouillon | n/a | à publier depuis le worktree de B |
| X1 | audits/morsehgp3D_v9/AUDIT_INITIAL_V8_20260922.md | B | 3cf72097 22/22:04 | cadre | H | n/a | 537 lignes |
| X2 | audits/morsehgp3D_v9/ETAT_COURANT.md | B | 3cf72097 | verdict B | H de fait (74aa8172) | n/a | second « état courant » |
| X3 | audits/morsehgp3D_v9/public_chain_t2_gate.cpp | B | 174fb8db 22/23:18 | oracle T2 | sonde | n/a | cité par #26 |