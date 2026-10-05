# Contre-lecture L0/S0 : contrat produit, documents et réponse à l'audit (clé v_docs)

4 octobre 2026, 22 h 05 UTC (heure lue par `date -u`). Contre-lecteur indépendant `v_docs` de la livraison L0.
Lecture seule du worktree `build/v11-impl-l0` (détaché à `f98aeed67`) ; essais dans `/tmp/v11-l0-vdocs/`. Rien n'est
modifié ni commité ; aucun build ni test natif ; **GCP non utilisé**.

```text
phase=exploration_v11_hors_registre
backend=cpu_reference
profile=quantized_u21_input_only
public_status=not_claimed
```

**Objet relu** (chemins relatifs à `morsehgp3D_v11/`) : `docs/SORTIES.md` (sha256 `f4ed497e…`),
`docs/ARCHITECTURE.md` (`864c85cb…`), `cmake/modules.cmake` (`c60dd9c3…`), `README.md` (`39f7a2ae…`),
`docs/PROVENANCE.md` (`3783ddbd…`), `docs/INDEX.md` (inchangé), `audits/REPONSE_CLAUDE_SUPPORTS_20261004.md`
(`ac17f514…`) et `audits/README.md`. Ce sont exactement les versions que l'auditeur a capturées dans `aef7182b3`
(empreintes identiques).

**Autorités appliquées.** DECISIONS_UTILISATEUR (§ 2 normatif), CRITIQUE_ET_PLAN_REVISE, SPECIFICATION_FINALE (§ 2, 6,
8, 9.1, 11), `docs/MATHEMATIQUES.md` § 10 (worktree, s0_math), code `src/io/` et `src/core/` à `f98aeed67`, audit
`1bf4be68f` (section « Sortie supports », reçu `audit_supports_20261004` et ses sous-revues qb, incidences, plateau,
carrier, lues par `git show`).

**Fait nouveau, décisif pour cette relecture.** `origin/main` a avancé à `aef7182b3` (21 h 34 UTC), après
`de4ab58a8` (20 h 51 UTC). L'auditeur mathématique y relit le contrat S0 **dans les versions ci-dessus** et les WIP
S3/S5/S6, et **répond à D.1–D.4** (section « Sortie supports : revue du contrat et du raccord », reçus
`audit_supports_followup_20261004` et `audit_supports_implementation_20261004`). Ces réponses sont traitées ici comme
contraintes. Le second auditeur (`AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md`) n'a rien publié depuis
`f98aeed67`.

## 0. Verdict

**Bloquant (3).**
1. La réponse à l'audit promet ce que le workflow ne tient pas, et elle est dépassée. § D, lignes 77-78 : « Aucune
   ligne native ne sera écrite avant votre réponse : ni `build_order`, ni le journal des graines, ni le
   rattachement, ni le module `supports` ». Or `build/v11-impl-s3` (`order_tree.hpp` né à 21 h 00 min 02,
   `seed_log.hpp` 21 h 00 min 36, `attachment.cpp` 21 h 01 min 42), `build/v11-impl-s6` (`supports.hpp` 21 h 01,
   `enumerate.cpp` 21 h 03) et `build/v11-impl-s5` (`api.hpp` 21 h 01) contiennent ces lignes natives, écrites avant
   la réponse de l'auditeur (21 h 34), qui les a d'ailleurs relues. Les quatre questions du § D ont reçu réponse dans
   `aef7182b3` ; la note doit en prendre acte avant d'être commitée.
2. `SORTIES.md:476-478` contredit `MATHEMATIQUES.md` § 10.10 et `SORTIES.md` § 4 : une translation entière **ne
   change pas** la numérotation canonique. Elle trie les naissances par (niveau, centre lexicographique) et les
   fusions par (niveau, plus petite naissance) ; `FULL_FORESTS.md:17-23` dit aussi que « l'ordre de Morton … ne
   remplace pas le centre ». Témoin (`translation_check.py`, oracle S1) : 40 nuages de 5 à 8 sites, K = 1, 2, 3,
   translation entière aléatoire et permutation de l'entrée. Numérotation (parent, kind, niveau, enfants) identique
   dans **120 cas sur 120** ; une réflexion la change dans 78 cas sur 120.
3. SORTIES garde deux points que l'auditeur a tranchés autrement, sans les contester :
   - § 8, `tree_k_sha256` repose sur le `BallIdx`, non publié (D.2 : signature versionnée recalculable depuis SP) ;
   - § 9 (et § 3), deux points de la transaction :
     - « Dans les deux cas, l'appel rend le code 2 », sans état publié distinct ;
     - `manifest_sha256()` reste nul alors que le dossier publié persiste.

     D.3 demande un état distinct et la conservation de l'empreinte du manifeste fermé.

   La note annonce « Toutes ses gardes sont adoptées » : le contrat normatif doit les intégrer, ou les contester par
   écrit, avant le commit.

**À corriger** : 24 points, au § 11, avec fichier, ligne et correction attendue.

**Conforme.**
- Le format `MHGP11SP` v1 suit exactement le § 2 de DECISIONS : en-tête, colonnes, aucune colonne de compte, aucune
  population, ordres canoniques, dérivations, contrôles du lecteur, agrégats du manifeste, déclarations obligatoires.
- Les noms des comptes, les rôles et la fenêtre $W_K$ concordent avec MATHEMATIQUES § 10 et avec l'oracle S1.
- La transaction de dossier est décrite fidèlement par rapport au code de `src/io/`, `retract()` compris.
- Les raisons, existantes et annoncées, sont cohérentes avec `reasons.def`.
- La règle de L2 est présente et précise ; la clause de report de `plat` est présente.
- La table des modules et sa copie CMake concordent : `check_style` est vert.
- La réponse couvre tous les points de `1bf4be68f`.
- `check_docs` ne produit aucune ligne nouvelle.
- `INDEX.md` est laissé intact à juste titre : c'est la note de l'index spatial, pas un index de documents.

## 1. Portes et essais rejoués

| Commande | Résultat |
| --- | --- |
| `python3 -S -B morsehgp3D_v11/tools/check_style.py --root morsehgp3D_v11`, puis sous `-O` | `style_ok fichiers=424`, code 0 dans les deux modes |
| `tests/support/test_check_style.py tools/check_style.py` sous `python3 -S -B`, puis sous `-O` | `check_style_ok controles=133`, code 0 |
| `cmake -S morsehgp3D_v11 -B /tmp/v11-l0-vdocs/cmake-ref -DMHGP11_MODULES=reference`, puis `ctest -R '^mhgp11_style' -j2` | configuration réussie, 95 portes enregistrées ; 2 sur 2 passent |
| Témoins négatifs de `[table]` sur une copie : ordre `points supports`, `supports` retiré d'`api`, `supports` dépendant de `points` | chaque écart est vu, code 1 ; la copie restaurée est verte |
| `python3 -B tools/check_docs.py`, base contre worktree (§ 9) | 156 lignes de chaque côté, `diff` vide |
| `check_docs.validate()` sur neuf Markdown v11, liens résolus contre `origin/main` plus le worktree | 0 écart : SORTIES, ARCHITECTURE, PROVENANCE, INDEX, README, REPONSE, audits/README, MATHEMATIQUES, HIERARCHIE_POINTS |
| `translation_check.py` (oracle S1, `python3 -B`) | 120 numérotations sur 120 identiques après translation ; 78 sur 120 changées par réflexion |
| Empreintes des six sources de PROVENANCE | identiques à `f98aeed67`, à `57dd21be1` et dans le worktree ; les noms cités (`ball_nodes`, `mark_supports`, `closure_counts`, `serialize`, `birth_sphere`, `sqrt_bounds`, `RadSum`, `PointTree`, `Hanging`, etc.) existent |

Après tous les essais, `git status` du worktree est inchangé (15 entrées) et aucun fichier n'y est plus récent que la
ligne de base.

## 2. `MHGP11SP` v1 contre le § 2 de DECISIONS

| Élément | DECISIONS § 2 | SORTIES § 6 | Verdict |
| --- | --- | --- | --- |
| En-tête | magie, `version`, `coord_bits`, `k`, `n`, `N`, `root`, `B`, `S`, `Z`, `A`, décalages, taille | mots 0 à 15 dans cet ordre, 136 octets, 5 décalages | conforme |
| `SITES` | `x`, `y`, `z`, `point_id` en `u32[n]`, ordre `SiteIdx` | identique | conforme |
| `NODES` | `parent`, `rank`, `kind u8`, `ball_count` | identique, numérotation canonique | conforme |
| `BALLS` | `rank`, `prior_count`, `support_count u16`, `role`, `p`, `m` ; tri (postordre, rang, $S^*$) | identique, avec preuve que (rang, $S^*$) coïncide avec l'ordre `BallIdx` dans un nœud (vérifiée sur `catalogue/assemble.cpp:12-16` et `catalogue.hpp:46`) | conforme |
| `SUPPORTS` | `arity u8[S]` puis `sites u32[Z]`, ordre (arité, lexicographique) | identique | conforme |
| `PRIOR` | `node u32[A]`, rôle fusion seulement | identique | conforme |
| Colonnes de compte, populations | aucune | aucune ; « Aucun compte n'est stocké », « Aucune population » | conforme |
| Dérivations | enfants, postordre, taille, décalages, $q_{\min}$, rattachement, feuille de K = 1, niveaux, comptes, `has_support_geometry` | table complète ; feuille $i$ = rang lexicographique $(x,y,z)$, vérifié sur `point_births` (`forest_build.cpp:59-69`) | conforme |
| Contrôles du lecteur | arbre, rangs, tri, première boule au rang du nœud, supports positifs en `Fraction`, ordre, lemme B, `strict_traces` nul à une naissance ; complétude non vérifiable | identique | conforme |
| Manifeste | agrégats par arité, coquilles étendues, boules à plusieurs supports, somme et maximum de `kparties_reliees` et `cofaces` | `arities`, `extended_shells`, `multi_support_balls`, `kparties_reliees{sum,max}`, `cofaces{sum,max}` | conforme ; sens des sommes à préciser (D.1, § 11) |
| Déclarations | non stable (cercle), omissions, pas le $K$-polyèdre (lemme H) | présentes | conforme ; nuance de stabilité de `kparties_reliees` à reprendre (D.1) |

Les formules dérivées sont celles du lemme G (MATHEMATIQUES § 10.7) ; la remarque « $N_j$ ne dépend que de
$\mathcal{Q}_b$ et de $m$ » est juste, puisque les sites orphelins sont interchangeables.

## 3. Cohérence avec MATHEMATIQUES § 10

- **Noms.** `kparties_reliees`, `compressed_parts`, `strict_traces`, `components` (`strict_global_components`),
  `cofaces` par boule et par support, `gabriel_cofaces` : mêmes noms, mêmes sens et mêmes formules dans SORTIES § 6,
  MATHEMATIQUES § 10.7 et `reference/hgp11_ref/supports.py`.
- **Rôles.** Naissance 0, fusion 1, interne 2, décidés par les rangs (lemme B) ; `components` vaut `prior_count`,
  1 ou 0 (lemme C).
- **Fenêtre.** $p+q_{\min}-1\leq K\leq p+m$, notée $p+q-1$ dans MATHEMATIQUES.
- **Titre et lettres.** Le titre « Hiérarchie des supports d'ordre K » est bien celui de la section 10 ; les lettres
  B (rôles) et H (forme datée de P3) concordent.

**Écarts.**
1. Translation (bloquant 2) : SORTIES § 10 contre MATHEMATIQUES § 10.10.
2. Stabilité de `kparties_reliees` :
   - SORTIES, lignes 307-308 : « ne dépend que de $(p,m,K)$ : il échappe à cette instabilité » ;
   - MATHEMATIQUES § 10.9 : « Il n'est pas stable pour autant : une perturbation qui brise une cosphéricité change $m$ » ;
   - auditeur, D.1 : sortir un site de coquille d'une boule diamétrale fait passer le compte de 3 à 1.

   SORTIES doit reprendre la réserve.
3. Lemme H, à $K=1$ :
   - SORTIES, lignes 311-313, dit seulement « union des $P_b$ des boules … rattachées au sous-arbre » ;
   - à $K=1$, le $K$-polyèdre est l'ensemble des feuilles du sous-arbre, et une feuille ne possède aucune boule
     (MATHEMATIQUES § 10.8).

## 4. Transaction de dossier contre `src/io/`

| Étape (SORTIES § 9) | Code | Verdict |
| --- | --- | --- |
| Plan : formes refusées `parameter_out_of_range` (vide, au moins `PATH_MAX`, `.`, `..`, `D.pending` au-delà de `NAME_MAX`) ; « `D/` désigne `D` » | `split_directory`, `directory.cpp:53-71` | conforme |
| Parent absent ou non dossier : `output_unwritable` | `open(... O_DIRECTORY)` et `realpath`, `:129-132` | conforme (EACCES à l'ouverture tombe aussi ici) |
| Entrée résolue sous `D` ou `D.pending` : `output_conflict` ; entrée non résolue non comparée | `inputs_outside`, `:84-92`, avant `absent` | conforme, ordre compris |
| `D`, puis `D.pending`, existe (lien pendant compris) : `output_conflict` ; autre erreur : `output_unwritable` | `absent`, `:95-99` | conforme |
| Parent non inscriptible ou non traversable | `faccessat(W_OK X_OK)`, `:136` | conforme |
| `create` : `D.pending` créé à la première demande ou au commit ; `O_EXCL`, `O_NOFOLLOW` ; nom `[a-z0-9_.]+`, 64 octets au plus, sans `.` initial, différent du manifeste ; 8 fichiers au plus ; doublon ou `D.pending` apparu : `output_conflict` ; erreur d'entrée-sortie définitive | `:180-197`, `writer.cpp:51-74` | conforme |
| Écriture petit-boutiste, `pad8`, SHA-256 au fil de l'eau, tampon de 64 Kio, erreur définitive | `writer.cpp:76-120` | conforme |
| Commit : `fflush`, `ferror`, `fsync`, `fclose`, puis manifeste, puis `fsync` de `D.pending`, puis `renameat2(NOREPLACE)`, puis `fsync` du parent ; `EEXIST`/`ENOTEMPTY` : `output_conflict`, sinon `output_unwritable` ; jamais de `rename` POSIX | `:200-247`, `:111-121` | conforme |
| Destructeur : retire les fichiers créés (manifeste compris, `created_`), puis `D.pending` s'il l'a créé ; ne touche jamais `D` | `discard`, `:158-169` | conforme |
| `retract()` : `D` vers `D.pending` sans remplacement, puis retrait ; refus `output_unwritable` si rien n'est publié, `output_conflict` ou `output_unwritable` si le renommage échoue | `:234-240` | conforme ; le commentaire `io.hpp:167-171` omet `output_conflict` (constat de s0_docs confirmé, à aligner en S5) |
| Double échec 1 : `fsync` du parent en échec, puis retour arrière en échec ; `committed()` vrai, `manifest_sha256()` nul | `publish`, `:216-223`, puis `commit_steps`, `:225-232` | conforme au code, mais contraire à D.3 (bloquant 3) |

Une seule imprécision d'ordre, au § 3, étape 4. Dans `read_u32le`, les refus `input_unreadable` de lecture
(lecture incomplète, octet de trop) viennent **après** `memory_budget` (`input.cpp:118-135`). SORTIES les liste
avant.

## 5. Raisons de refus

- Toutes les raisons existantes que cite SORTIES figurent dans `reasons.def` à `f98aeed67`, avec les statuts
  implicites du texte. Les codes 0, 2 et 3 sont ceux d'`exit_code`, et la fusion `merge` est décrite exactement.
- Les six raisons annoncées ont le statut et le module de la spécification (§ 3.3, 3.5, 3.7). Leur ordre, avec S6
  avant S5, est justifié par le plan révisé.
- Les WIP ajoutent chacun leurs raisons en fin de la table de `f98aeed67` : S6 `support_shell_capacity` et
  `supports_invariant`, S5 `environment_selftest`. L'ordre annoncé ne tient donc que si S6 est intégrée avant S5.
- Liste du calcul (§ 3, étape 7) incomplète :
  - `index_overflow_u32` est aussi émis par le catalogue (`catalogue.cpp:75`, `sort_indices.cpp:145`,
    `assembly_parallel.cpp:14,70`) ;
  - `task_exception` (sched) manque ;
  - `budget_not_released` se constate à la fermeture de la `Session`, donc **après** la publication. C'est le cas du
    WIP S5 (`cli/mhgp11.cpp:212-239,306-310`) : un troisième chemin où un refus peut laisser un dossier publié, avec
    le code 3.

## 6. Règle de décision de L2 et clause de report

- **Règle de L2** (SORTIES § 11). Elle est précise et ne laisse pas d'ambiguïté qui change la décision. Elle reprend
  fidèlement la critique :
  - étage `tree` de la ligne standard, `supports` contre `full` ;
  - trames ng00 à ng02 à K5 ;
  - médiane de trois prises à W48 ;
  - seuil $T_{\mathrm{supports}}\leq 1{,}1\,T_{\mathrm{full}}$ sur au moins deux trames ;
  - W1 et K10 descriptifs ;
  - repli L2b décrit.

  Le même exécutable sert aux deux mesures, donc la configuration de compilation est commune. Seul cas non prévu :
  une prise dont les sorties diffèrent. Elle invalide la mesure, mais la règle ne le dit pas.
- **Clause de report** (SORTIES § 11, README, REPONSE § C). Elle est présente, le refus `parameter_out_of_range` y
  est explicite, et les exigences de S10 restent intactes. Il reste deux flous :
  - la condition nomme une « règle E1-bis ». Or SORTIE_PLATE.md, § 2, point 2, reporte à E1-bis une **expérience**
    (V1, V2, κ = 2, `first`, `cover`), et c'est le § 3.4 qui dit « Ouvert » pour une **règle**. La critique faisait
    déjà ce raccourci. Il faut nommer une condition vérifiable ;
  - l'auteur de la décision écrite n'est pas nommé.

## 7. Table des modules et copie CMake

- `check_style` est vert : la table et sa copie concordent, et les témoins négatifs sont tués (§ 1). Un module
  planifié sans dossier est admis par `[module]`.
- **Incompatibilité latente avec le plan de livraison.** `ARCHITECTURE.md:49` donne à `api` les dépendances
  « tous », c'est-à-dire avec `supports`, `points` et `head`, et `modules.cmake:18` les recopie.
  - `CMakeLists.txt:202-228` exige que toute la fermeture d'un module demandé soit présente, sinon la configuration
    échoue (`FATAL_ERROR`).
  - Or L2 livre `src/api/` avant `points` (L3) et `head` (L4). La configuration par défaut échouera donc dès S5.
  - Le WIP S5 l'a vu : il réduit `api` à `core … tower` dans les deux fichiers. Il entre ainsi en conflit avec L0.
  - Le paragraphe des lignes 55-58 (« leurs dépendances sont fixées d'avance ») est donc inexact pour `api`.
  - Ce défaut existait déjà à `f98aeed67`, mais L0 l'affirme désormais.

## 8. Réponse à l'audit : couverture

**Audit `1bf4be68f`, section « Sortie supports ».** Les 18 points de la section sont couverts par les lignes A1 à A15
de la note :
- arbre K seul ;
- propriétaire fermé après le plateau, continuations et niveaux, listes propres ;
- recouvrements, $K$-parties abstraites et supports distincts, carré à K2 ;
- $\mathcal{Q}_b$ sur toute $U$, canoniseur et seuils, événements faibles, coût publié ;
- `compressed_parts`, nouveaux sommets, nommage, incidences ;
- unions 2, 2, 1, 0, carrier et u21.

**Sous-revues.**

| Sous-revue | Points | Couverture |
| --- | --- | --- |
| qb | cube, canoniseur non énumérateur, une identité de boule, événement faible à K2 (et paire q2 à K1, couverte par la fenêtre), hypothèses de `ball_nodes` (question D.4), admission count/fill sans export partiel | couverte ; non reprise : « toutes les traces et incidences doivent demeurer disponibles à la fermeture du plateau » |
| incidences | table triangle/ligne, `compressed_part_count`, cofaces et incidences, filtrage $\lambda_b\leq a$ | couverte ; non reprise : « univers explicites » du contrat Zoltan |
| plateau | six $K$-parties par boule de face, trois sommets neufs, 2, 2, 1, 0, coupes stricte et fermée, $\lambda_b$ dans chaque charge, pas le $K$-polyèdre, noms `nerve_edges` et `performed_unions` | couverte ; non reprise : « le nombre de représentants d'un raffinement n'est pas un nombre intrinsèque de liaisons » |
| carrier | cercle, saut de Hausdorff, neuf immersions u21, carré K2 et K1 datés, triangle aigu à K2, mutation à poids nuls | couverte |

Les trois omissions sont mineures. Les chiffres cités sont exacts :
- 3 pour le triangle et pour la ligne, 6 par boule de face ;
- fixtures 8 et 11 gravées par l'oracle S1.

Aucune promesse ne dépasse le plan, sauf celle du § D (bloquant 1). Une référence est inexacte : la colonne « Où » de
A9 cite SORTIES § 3 et § 8 pour l'admission du brouillon en deux passes, qui figure à la spécification § 7.3 et à
ARCHITECTURE § 7.1.

**Après `1bf4be68f` : ce que la note ne couvre pas encore** (`de4ab58a8`, `aef7182b3`).

| Réponse ou garde de l'auditeur | Où l'intégrer |
| --- | --- |
| D.1 : les cofaces par boule suffisent ; l'incidence par support est facultative. La somme de `kparties_reliees` compte des incidences $(b,F)$ (ligne 0/1/2 à K2 : 1 + 1 + 3 = 5 pour 3 paires). L'indépendance vis-à-vis de $\mathcal{Q}_b$ ne vaut qu'à $(p,m,K)$ fixés | SORTIES 307-308 et 386 ; REPONSE 57 |
| D.2 : signature versionnée recalculable depuis SP : contexte, géométrie `SITES` sans `PointId`, parent, rang, kind, site à K1 ou $S^*$ à K ≥ 2, enfants. FUL1 seul ne contient pas $S^*$ | SORTIES 392-401 |
| D.3 : état publié distinct (`published_complete` proposé) dans le résultat API et la ligne de refus ; empreinte du manifeste fermé conservée (S4 l'affecte après `publish`) ; portes de faute sur G4 | SORTIES 81, 152-155, 451-459 ; ARCHITECTURE 186-188 |
| D.4 : le sélecteur comprimé est strict pour une boule faible, pas une $K$-partie quelconque (AC de la ligne, niveau 1 = $\lambda_b$). Le juge général garde `initial ≤ λ` et l'ancêtre fermé ; le journal garde `initial < λ`. Ajouter le témoin D2 aux fixtures E2 | REPONSE 101-104 ; PROVENANCE 230 |
| S3 : garde avant le cast `u32` des traces strictes (`attachment.cpp`), sans imposer le plafond 24 à FULL | REPONSE (prise d'acte) |
| S6 : extraire $\mathcal{Q}_b$ avant la fermeture zêta (6 supports du cube deviennent 177 parties) ; garder les q4 à K1, aucun filtre par $\lvert Q\rvert\leq K+1$ ni par cofaces positives | PROVENANCE 231 |
| Preuve D2 corrigée ; exceptions d'invariance | déjà reprises dans L0 selon l'auditeur ; rien à faire dans ces fichiers |

## 9. `check_docs`

La base est extraite par `git archive origin/main` dans `/tmp/v11-l0-vdocs/base`, **restreinte aux motifs du
checkout partiel** : fichiers de premier niveau, `tools/`, `docs/`, `morsehgp3D_v11/` sans `receipts/`.
- Une archive complète ne serait pas comparable, puisque le checkout partiel produit à lui seul les liens morts et les
  documents manquants.
- `docs/` et `tools/` sont identiques entre `f98aeed67` et `origin/main`.
- Les sorties sont identiques octet pour octet, 156 lignes de chaque côté (empreintes égales), code 1 des deux côtés.
- Le registre racine modifié, `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md`, est parcouru par `check_docs` et
  n'ajoute rien.
- `check_docs` ne parcourt pas `morsehgp3D_v11/`. Sa fonction `validate()`, appliquée aux neuf Markdown v11 avec les
  reçus d'`origin/main`, donne 0 écart.

## 10. Intégration avec les WIP parallèles (notes)

- `audits/README.md`. La ligne modifiée par s0_docs (ligne 24) **n'existe plus** sur `origin/main` : `de4ab58a8` et
  `aef7182b3` ont réécrit ce README. Un `cherry-pick` entrera en conflit, et une recopie effacerait le texte de
  l'auditeur. Il faut rejouer l'ajout sur la version d'`origin/main`.
- `PROVENANCE.md`. S3, S5 et S6 ajoutent chacun leur section de ports réels. Les lignes « annoncées » de L0 (E2,
  `enumerate.cpp`, `write_full.cpp`) devront y renvoyer ou être retirées à l'intégration.
- `cmake/modules.cmake` et `ARCHITECTURE.md`. Les WIP S5 et S6 les modifient aussi (§ 7) ; il faudra réconcilier
  les trois versions.
- Contrat du CLI. Le WIP S5 ajoute un refus à l'étape 2 : sortie standard fermée (`output_unwritable`), ou égale à une
  entrée (`output_conflict`). C'était la « garde éventuelle » que SORTIES laissait à S5. Il retire aussi le dossier
  publié si la fermeture de session échoue (§ 5). SORTIES § 3 et § 9 devront le reprendre.
- Un seul des deux auditeurs a répondu. La condition « réponse écrite des deux auditeurs » (SORTIES 495) n'est donc pas
  remplie.

## 11. Corrections attendues

Bloquantes d'abord, puis par importance. Les chemins sont relatifs à `morsehgp3D_v11/`.

1. `audits/REPONSE_CLAUDE_SUPPORTS_20261004.md:77-78`. Retirer « Aucune ligne native ne sera écrite avant votre
   réponse ». Écrire à la place :
   - S3, S5 et S6 sont écrits en parallèle (WIP non commités, relus dans `aef7182b3`) ;
   - aucun n'est commité ni qualifié avant l'intégration de vos réponses.
2. `audits/REPONSE_CLAUDE_SUPPORTS_20261004.md:90-107`. Prendre acte de D.1 à D.4 et dire ce qui est adopté (§ 8 :
   somme en incidences, empreinte V2, état publié distinct, juge E2 à `initial ≤ λ`). Ajouter les gardes nouvelles :
   - cast `u32` des traces avec `tower_capacity`, sans plafond 24 dans FULL ;
   - $\mathcal{Q}_b$ avant la fermeture zêta ;
   - q4 conservés à K1.
3. `docs/SORTIES.md:476-478`. Une translation entière laisse invariants la numérotation canonique (colonnes `NODES`),
   les rangs et les listes propres. Elle ne change que les ordres par `SiteIdx` :
   - lignes de `SITES` ;
   - ordre des boules de même rang d'un nœud ;
   - ordre des supports d'une boule.

   Une réflexion ou un échange d'axes peut changer la numérotation (MATHEMATIQUES § 10.10).
4. `docs/SORTIES.md:451-459`, `:81` et `:152-155`. Reprendre D.3. Un refus qui laisse $D$ publié porte un état
   distinct :
   - dans la ligne de refus si elle peut être écrite, sinon sur la sortie d'erreur ;
   - dans le résultat de l'API.

   Le code 2 ne signifie pas « rien publié ». L'empreinte du manifeste fermé est conservée (S5 corrige `commit_steps`,
   qui l'affecte après `publish`). Ajouter le chemin de la fermeture de session après publication : retrait, puis code
   3 si le retrait échoue.
5. `docs/SORTIES.md:392-401`. Redéfinir `tree_k_sha256` en signature versionnée recalculable depuis `MHGP11SP` (D.2).
   Elle ne doit être promise ni depuis FUL1 seul, ni par un changement de ses octets.
6. `docs/SORTIES.md:307-308`. Indépendance vis-à-vis de $\mathcal{Q}_b$ à $(p,m,K)$ fixés seulement ; pas de stabilité
   (MATHEMATIQUES § 10.9, D.1).
7. `docs/SORTIES.md:386`. La somme de `kparties_reliees` compte des incidences $(b,F)$, pas des $K$-parties
   distinctes.
8. `audits/REPONSE_CLAUDE_SUPPORTS_20261004.md:57`. Même réserve que le point 6.
9. `audits/REPONSE_CLAUDE_SUPPORTS_20261004.md:101-104`. « La $K$-partie descendue y est stricte » ne vaut que pour le
   sélecteur comprimé (D.4).
10. `docs/SORTIES.md:495` et `README.md:131-132`. Écrire « précède tout commit et toute qualification natifs de S3 » :
    les lignes natives existent déjà.
11. `docs/ARCHITECTURE.md:49`, `:55-58` et `cmake/modules.cmake:18`. Les dépendances d'`api` doivent croître avec les
    livraisons :
    - S5 : `core` à `tower`, comme le WIP S5 ;
    - S7 : avec `supports` ;
    - S9 : avec `points` ;
    - S10 : avec `head`.

    Sinon, retirer « dépendances fixées d'avance ».
12. `docs/ARCHITECTURE.md:186-188`. État publié distinct (D.3) dans le § 7.2.
13. `audits/README.md:24`. Rejouer l'ajout sur la version d'`origin/main`, jamais par copie.
14. `docs/PROVENANCE.md:230`. Ligne E2 :
    - garde `initial ≤ λ_b` et ancêtre fermé pour une $K$-partie quelconque ;
    - témoin D2 aux fixtures : A = (2, 10, 0), B = (18, 10, 0), C = (10, 20, 0), Z = (9, 3, 0), W = (11, 3, 0), K = 2.
15. `docs/PROVENANCE.md:231`. Ligne `enumerate.cpp` :
    - $\mathcal{Q}_b$ extrait des marques avant la fermeture zêta ;
    - aucun filtre par arité ni par cofaces.
16. `docs/SORTIES.md:104-107`. `budget_not_released` relève de l'étape 8 (fermeture de session après publication),
    pas de l'étape 7.
17. `docs/SORTIES.md:92-96`. Ordre réel : `input_unreadable` (ouverture, nature, tailles), puis `index_overflow_u32`,
    puis `memory_budget`, puis `input_unreadable` (lecture incomplète, octet de trop).
18. `docs/SORTIES.md:100-105`. Ajouter `index_overflow_u32` (catalogue) et `task_exception`, ou écrire « notamment ».
19. `docs/SORTIES.md:311-313`. Lemme H : préciser $K\geq 2$, et qu'à $K=1$ ce sont les feuilles du sous-arbre.
20. `docs/SORTIES.md:519-522`, `README.md:134-137` et `audits/REPONSE_CLAUDE_SUPPORTS_20261004.md:66-69`. Clause de
    report :
    - condition vérifiable, par exemple « tant que le § 3.4 de SORTIE_PLATE.md porte la mention Ouvert » ;
    - auteur de la décision nommé ;
    - E1-bis désigne une expérience, pas une règle.
21. `audits/REPONSE_CLAUDE_SUPPORTS_20261004.md:36`. Colonne « Où » : spécification § 7.3 et ARCHITECTURE § 7.1, au
    lieu de SORTIES § 3 et § 8.
22. `docs/SORTIES.md:155`. « hors le double échec du § 9 » devient « hors les doubles échecs du § 9 ».
23. Facultatif, `docs/SORTIES.md:206-207`. Sur une très grande coquille, la tour peut refuser avant les supports
    (`tower_capacity`, `memory_budget`). Écrire « refusé, au plus tard par `support_shell_capacity` ».
24. Facultatif, `docs/SORTIES.md:387`. Les cofaces de deux boules sont disjointes (boule minimale unique) : la somme
    compte des liaisons distinctes, limitées aux boules de $W_K$.

Artefacts de cette relecture, dans `/tmp/v11-l0-vdocs/` :
- `check_docs_base.txt` et `check_docs_cur.txt` ;
- `translation_check.py` et `validate_v11.py` ;
- `base/`, `full/`, `style/` et `cmake-ref/`.
