# L0, consolidation (clé `consolidation`) : contrat S0 et oracle S1 après les trois contre-lectures

4 octobre 2026, 22 h 55 UTC (heure lue par `date -u`). Rôle : agent de consolidation de la livraison L0. Worktree
`build/v11-impl-l0`, détaché à `f98aeed67` (checkout partiel, sans `receipts/`). Écritures : ce worktree, ce
rapport et `l0_consolidation_verif/`. **GCP non utilisé.** Aucun `git add`, commit, stash, branche ni push ; aucun
build ni test natif ; configuration CMake limitée à l'unité `reference` dans `/tmp/v11-l0-consolidation/`, 3 cœurs.

```text
phase=exploration_v11_hors_registre
backend=cpu_reference
profile=quantized_u21_input_only
public_status=not_claimed
```

**Autorité appliquée.** `DECISIONS_UTILISATEUR.md`, puis `CRITIQUE_ET_PLAN_REVISE.md`, puis `SPECIFICATION_FINALE.md`
(§ 2, 6, 8, 9.1, 11), puis `MATHEMATIQUES.md` et la thèse (Déf. 21, 27 à 29, Prop. 5 et 6, Th. 4, relues par
`pdftotext`). Contraintes : les audits `1bf4be68f`, `de4ab58a8` et `aef7182b3` (réponses D.1 à D.4), lus sur
`origin/main` par `git show`. Entrées : les rapports des rédacteurs (`l0_s0_math.md`, `l0_s0_docs.md`,
`l1_s1_oracle.md`) et des contre-lecteurs (`l0_verif_math.md`, `l0_verif_docs.md`, `l0_verif_oracle.md`).

## 0. Résultat

- **Les trois bloquants de `v_docs` sont levés** : la promesse inexacte de la réponse à l'audit est corrigée et les
  réponses D.1 à D.4 y sont intégrées ; l'invariance par translation est rectifiée ; `tree_k_sha256` devient la
  signature version 2 de l'auditeur, et l'état publié distinct `published_complete` est écrit (D.3).
- **Toutes les corrections demandées sont appliquées** : 13 sur 13 pour `v_math`, 24 sur 24 pour `v_docs`
  (facultatives comprises), 5 sur 5 pour `v_oracle`.
- J'ai ajouté trois mutants de vivacité, en plus des trois proposés : règle du parent, vie d'une boule interne et
  M1, dont `v_oracle` avait montré que les contrôles pouvaient disparaître sans bruit.
- **Refusés, avec leur raison (§ 4)** : trois contrôles facultatifs de l'oracle. Deux trous de couverture restent
  ouverts : la suite `long` à $K\geq 6$ et les coquilles de 13 à 24 sites.
- **Aucune décision de l'utilisateur n'est changée.** Deux constats touchent à leur portée : la stabilité de
  `kparties_reliees` et la règle « auditeurs avant toute ligne native ». Ils sont posés en questions (§ 10).
- **Portes finales vertes**, à commencer par l'oracle :

  ```text
  reference_supports_ok nuages=210 ordres=951 boules=15062 supports=16943 noeuds=12441 coupes=48074
  ```

  Ce résultat tient sous Python 3.12 et 3.10, en normal et sous `-O`, avec des sorties identiques. Les 13 mutants
  sont tués, avec le code 4. CTest passe 90 portes sur 90 hors `long`. `check_style` et son auto-test sont verts.
  `check_docs` est identique, à l'octet, à la base `origin/main`.

## 1. Bloquants (`v_docs`)

| # | Constat | Décision | Où | Vérification |
| ---: | --- | --- | --- | --- |
| B1 | La réponse promettait « aucune ligne native avant votre réponse », ce qui est faux : les brouillons S3, S5 et S6 existent. D.1 à D.4 sont répondus dans `aef7182b3` | **Appliqué.** Le § D est récrit : correction explicite de la promesse (brouillons commencés vers 21 h 00, relus dans une capture de l'auditeur, ni commités ni qualifiés) ; nouvelle règle « aucun commit ni qualification natifs avant l'intégration des réponses et la réponse de l'auditeur moteur » ; tableau D.1 à D.4 et D2 ; gardes nouvelles (cast `u32` des traces, `make_shape`, $\mathcal{Q}_b$ avant la fermeture zêta, q4 à K1, identité des octets) | `audits/REPONSE_CLAUDE_SUPPORTS_20261004.md`, en-tête et § D | horodatages relus par `stat` (le brouillon S6 évolue encore à 22 h 51) |
| B2 | `SORTIES.md` affirmait qu'une translation entière change la numérotation canonique | **Appliqué.** La translation laisse invariants la numérotation (colonnes `NODES`), les rangs, les rôles, les listes propres comme ensembles et les $\mathcal{Q}_b$, à la translation près. Seuls changent les ordres par `SiteIdx` : lignes de `SITES`, $S^*$, boules de même rang, supports. Une réflexion ou un échange d'axes peut changer la numérotation | `SORTIES.md` § 10 | 14 cas recalculés (`verif_faits.py`). L'oracle grave aussi désormais la translation $(5,11,17)$ sur ses 70 contre-épreuves d'invariance |
| B3 | `tree_k_sha256` reposait sur le `BallIdx`, non publié (D.2) ; un double échec rendait le code 2 sans état distinct, avec `manifest_sha256()` nul (D.3) | **Appliqué.** Signature **version 2**, octet pour octet celle du modèle `check_d2_signature.py` de l'auditeur. Elle se recalcule depuis le seul `MHGP11SP`, pas depuis `MHGP11FUL1` seul, dont les octets ne changent pas. Pour D.3 : état `published_complete` dans le résultat de l'API et dans la ligne de refus (sortie standard, sinon sortie d'erreur) ; empreinte du manifeste conservée dès sa fermeture (correction de `commit_steps` attendue en S5) ; troisième chemin : fermeture de `Session` en échec après la publication, puis retrait, code 3 | `SORTIES.md` § 3, § 8 et § 9 ; `ARCHITECTURE.md` § 7.2 | encodage relu dans `receipts/audit_supports_implementation_20261004/evidence/check_d2_signature.py` (`origin/main`) |

## 2. Contre-lecture mathématique (`v_math`)

| # | Correction demandée | Décision | Où |
| ---: | --- | --- | --- |
| 1 | Rangs : seuls les ensembles de nœuds coïncident entre coupe ouverte $\lambda_b$ et coupe fermée de rang $r_b-1$ ; aucune garde $\beta(F)\leq\ell(r_b-1)$ ; témoin D2 | **Appliqué** : paragraphe neuf sous « Rangs » (notation $\ell(r)$), remarque dans la réalisation E1, ligne D2 au § 10.11 | MATHEMATIQUES § 10.2, § 10.5, § 10.11 |
| 2 | Registre : entrée `false_in_general` pour $\beta(F)\leq\ell(r_b-1)$, avec D2 ; fixture permanente | **Appliqué** : entrée ajoutée (avec la non-coïncidence des composantes) ; fixture `d2_audit` gravée dans l'oracle (arbre, boule $ABC$, fait `d2_witness`) | registre ; `reference/test_supports.py` |
| 3 | Lemme W.2 : « sans elle », « l'omettre » ambigus ; nommer les deux lectures | **Appliqué** : sommet $AC$ gardé (8 naissances au lieu de 7, fusion à trois enfants $\lbrace AB\rbrace$, $\lbrace AC\rbrace$, $\lbrace BC\rbrace$), comme dans le $K$-graphe de Gabriel (Déf. 29, p. 89, relue) ; sommet retiré (deux enfants) ; dans les deux cas, la fusion (24, 2). D2 est cité comme second témoin plan (145/2) | MATHEMATIQUES § 10.3 et § 10.11 ; registre |
| 4 | Lemmes D et E à $K=1$ | **Appliqué** : la descente s'arrête sur le site, $g$ (ou $g_F$) est la feuille ; « naissance de $W_K$ » réservé à $K\geq 2$ | MATHEMATIQUES § 10.5 ; registre (D, E) |
| 5 | Équivalence $c_b\in W_F(\lambda_b)$ si et seulement si $F\subseteq P_b$ | **Appliqué**, avec sa preuve d'une ligne | MATHEMATIQUES § 10.1 et lemme A ; registre (A) |
| 6 | La somme de `kparties_reliees` compte des incidences $(b,F)$ (ligne : 5 pour 3 paires) | **Appliqué**, avec la remarque symétrique (la somme des `cofaces` des boules compte des liaisons distinctes, limitées à $W_K$) | MATHEMATIQUES § 10.7 ; SORTIES § 6 et § 8 ; registre (G) |
| 7 | « plafond du format » | **Appliqué** : plafond du produit, `support_shell_capacity` ; le format porte $m$ en `u8` | MATHEMATIQUES § 10.7 |
| 8 | `gabriel_cofaces` d'un support : « nul si $t+1<\lvert Q\rvert$ » | **Appliqué** dans les deux tableaux | MATHEMATIQUES § 10.7 ; SORTIES § 6 |
| 9 | « change $m$ et scinde la boule » : témoin exact | **Appliqué** : diamètre $(0,0)$, $(20,0)$, site $(10,10)$ puis $(10,11)$ ; 3 puis 1, même boule, même $\mathcal{Q}_b$ (recalculé) | MATHEMATIQUES § 10.9 |
| 10 | Registre, entrée P fausse à la lettre | **Appliqué** : « parmi les boules de niveau $\lambda$, un sommet neuf n'est contenu que par sa boule minimale » ; contre-exemple du carré cité ; « hors de $W_K$ avec $p+m\geq K$ » | registre (P) |
| 11 | Registre, entrée D : « donne $\mathrm{ant}(b)$ » | **Appliqué** : le nœud de la trace, élément de $\mathrm{ant}(b)$ ; la réunion est $\mathrm{ant}(b)$ | registre (D) |
| 12 | Registre, non-stabilité : nuancer `kparties_reliees` | **Appliqué** : « sans être stable pour autant : 3 à 1 à $\mathcal{Q}_b$ fixé » | registre |
| 13 | `HIERARCHIE_POINTS.md` : compléter la divergence $m(1)$ | **Appliqué** : `points_flat_dump.py:67` (et `:71`, métadonnées), `points_campaign.py:135-138` ; ces deux-là seulement si `--orders` contient 1 | HIERARCHIE_POINTS § 9 |
| note | « § 9 » ambigu aux lignes 96 et 110 | **Appliqué** : « § 9 de ce document » | HIERARCHIE_POINTS |
| note | Rapport `s0_math` : le § 10 finissait à la ligne 865 et non 863 | Sans objet : le rapport d'un rédacteur n'est pas réécrit ; le § 10 finit maintenant plus bas | — |

## 3. Contre-lecture documentaire (`v_docs`), corrections non bloquantes

| # | Correction | Décision | Où |
| ---: | --- | --- | --- |
| 1, 2 | Promesse de la réponse ; suite de D.1 à D.4 ; gardes nouvelles | **Appliqué** (B1) | REPONSE § D |
| 3 | Translation | **Appliqué** (B2) | SORTIES § 10 |
| 4 | D.3 aux lignes 81, 152-155 et 451-459 ; troisième chemin | **Appliqué** (B3) : intro du § 3, étape 9 neuve, ligne d'état, § 9 | SORTIES § 3 et § 9 |
| 5 | `tree_k_sha256` recalculable depuis SP | **Appliqué** (B3) | SORTIES § 8 |
| 6 | Indépendance de `kparties_reliees` à $(p,m,K)$ fixés seulement | **Appliqué** | SORTIES § 6 |
| 7 | Somme de `kparties_reliees` = incidences | **Appliqué** | SORTIES § 6 et § 8 |
| 8 | Même réserve dans la réponse | **Appliqué** | REPONSE § B.2 |
| 9 | « La $K$-partie descendue y est stricte » ne vaut que pour le sélecteur comprimé | **Appliqué** (ligne D.4 du tableau) ; aussi au lemme E | REPONSE § D ; MATHEMATIQUES § 10.5 |
| 10 | « précède toute ligne native de S3 » démenti | **Appliqué** : « tout commit et toute qualification natifs de S3, S5 et S6 » ; voir la question 1 du § 10 | SORTIES § 11 ; README |
| 11 | Dépendances d'`api` (« tous ») incompatibles avec `CMakeLists.txt:202-228` | **Appliqué** : `core` à `tower`, ligne identique, à l'octet, à celle du brouillon S5 ; elles croissent avec les livraisons (S7 `supports`, S9 `points`, S10 `head`) ; « fixées d'avance » retiré | ARCHITECTURE § 2 ; `cmake/modules.cmake` |
| 12 | D.3 dans le § 7.2 | **Appliqué** | ARCHITECTURE § 7.2 |
| 13 | `audits/README.md` réécrit sur `origin/main` | **Appliqué** : le fichier du worktree est la version `origin/main` (`aef7182b3`) plus **une** ligne (§ 9) | `audits/README.md` |
| 14 | PROVENANCE, ligne E2 : D.4 et D2 | **Appliqué** | PROVENANCE |
| 15 | PROVENANCE, ligne `enumerate.cpp` : avant la fermeture zêta, aucun filtre | **Appliqué** | PROVENANCE |
| 16 | `budget_not_released` après la publication | **Appliqué** : étape 9 | SORTIES § 3 |
| 17 | Ordre réel de `read_u32le` | **Appliqué**, relu dans `input.cpp:118-135` | SORTIES § 3, étape 4 |
| 18 | `index_overflow_u32` (catalogue), `task_exception`, « notamment » | **Appliqué**, relu dans `reasons.def` et les sources | SORTIES § 3, étape 7 |
| 19 | Lemme H : $K\geq 2$ ; feuilles à $K=1$ | **Appliqué** | SORTIES § 6 |
| 20 | Clause de report : condition vérifiable, auteur, E1-bis | **Appliqué** : « tant que le § 3.4 de SORTIE_PLATE.md porte la mention Ouvert » ; décision écrite **de l'utilisateur** (question 3 du § 10) ; l'expérience E1-bis n'est pas la condition | SORTIES § 11 ; README ; REPONSE § C |
| 21 | Colonne « Où » de l'admission en deux passes | **Appliqué** : paragraphe ajouté à SORTIES § 6 ; renvoi à ARCHITECTURE § 7.1 | SORTIES § 6 ; REPONSE § A |
| 22 | « hors les doubles échecs » | **Appliqué** (paragraphe récrit) | SORTIES § 3 |
| 23 | « au plus tard par `support_shell_capacity` » (facultatif) | **Appliqué** | SORTIES § 6 |
| 24 | Cofaces de boules distinctes disjointes (facultatif) | **Appliqué** | SORTIES § 6 et § 8 ; MATHEMATIQUES § 10.7 |
| note | Ordre des raisons (S6 avant S5) garanti seulement par l'ordre d'intégration | **Appliqué** : « dans l'ordre d'intégration ; `reasons.def` fait foi, tableau et `status_test.cpp` mis à jour au même commit » | SORTIES § 3 |
| note | Règle de L2 : prise aux sorties différentes | **Appliqué** : elle invalide la mesure, défaut à corriger avant décision | SORTIES § 11 |
| note | Garde de la sortie standard du brouillon S5 | **Appliqué**, annoncée « prévue en S5 » à l'étape 2 | SORTIES § 3 |
| note | Trois omissions mineures de la réponse (traces au plateau, `cut_side` et univers, représentants) | **Appliqué** : trois lignes au § A | REPONSE § A |
| note | Second auditeur sans réponse | Consigné : condition de commit dans la réponse et le README | REPONSE § D ; README |

## 4. Contre-lecture de l'oracle (`v_oracle`)

| # | Correction | Décision | Vérification |
| ---: | --- | --- | --- |
| 1 | Faits et invariance non protégés (trace Python, code 1 par coïncidence) | **Appliqué** : `try/except` autour de chaque fait et de la contre-épreuve d'invariance ; l'exception devient une ligne `ECART` | Copie mutée par `w4_gabriel_juge` dans le source : code 1, 0 trace, lignes `ECART` et synthèse |
| 2 | Trois mutants de vivacité (`w4_gabriel_juge`, `h_fortes_etroites`, `c3_une_fusion`) | **Appliqué**, plus trois autres : `regle_parent_inversee`, `interne_vie_inversee`, `m1_support_inverse`. Il y a maintenant 13 mutants et 26 portes, jumelles `-O` comprises | Code 4 et cause présente, pour chacun. Sur une copie où le contrôle est rendu tautologique (`w4_coupe_fermee`, `h_fortes_omises`, `c3_union_omise`, `regle_parent_omise`), le mutant correspondant **survit**. Si le contrôle est retiré (`f_m1_omis`), il devient inapplicable (code 3). Dans les deux cas, sa porte échoue |
| 3 | Permutation identité admise ; compteur | **Appliqué** : une rotation remplace l'identité ; compteur `permuted` gravé (70) et plancher `permuted == invariance` | Gate verte |
| 4 | Fixture `d2_audit` et fait `cube@1` ; regravure | **Appliqué** : fait d'arbre et de boule, `d2_witness`, et `cube@1` (tétraèdres à 0 coface). `SUITE_EXACT`, `SUITE_DIGEST` (`9bddd8aa…`), l'empreinte `d2_audit` (`671b0cb7…`), la ligne de `tests.cmake` et le README sont regravés. **Aucune autre empreinte de fixture n'a changé**, et `supports.py` est inchangé (sha256 `826f3f94…`) | Gate verte |
| 5 | Lecture de E5 | **Appliqué** (`v_math` 3) | — |
| fac. | Invariance par translation entière | **Appliqué**, sans calcul supplémentaire : le nuage de la contre-épreuve est permuté, réétiqueté **et** translaté | 70 nuages |
| fac. | Contrôles directs de P.1 et P.2 | **Refusé.** Ce sont des étapes de preuve. Leurs conséquences sont contrôlées (B, C.3, H, W.3, W.4), et il n'existe aucune faute causale de l'oracle qu'ils tueraient seuls. Un balayage supplémentaire de $\Gamma_K$ par niveau coûterait sans mutant à tuer | — |
| fac. | « Une naissance est toujours forte » | **Refusé.** La preuve est immédiate ($t\geq q$ pour une naissance), et le mutant correspondant est équivalent (`compteur_fortes_faux`). Placé avant le lemme H, le contrôle tuerait `h_fortes_etroites` avant la restriction aux fortes, dont il masquerait la vivacité | Essai sur copie : `h_fortes_etroites` est alors tué sans sa cause, code 3 |
| fac. | « `components` ne dépasse pas `strict_traces` » | **Refusé** : tautologique une fois C.1 contrôlé, car $\mathrm{ant}(b)$ est l'ensemble des nœuds des traces comprimées strictes, au plus `strict_traces` éléments | — |
| trou | Suite `long` à $K\geq 6$ ; coquilles de 13 à 24 sites | **Non fait** : il faudrait récrire $N_j$ par combinaisons (aujourd'hui $2^{m}$ masques), puis graver une suite `long` jouée sur G4. Hors du périmètre de L0, et le budget du label `fast` ne le permet pas. Écrit dans le README de la référence (« ce que cet oracle n'établit pas ») et posé en question 4 | — |

## 5. Questions ouvertes des rédacteurs : suite donnée

| Rédacteur | Question | Suite |
| --- | --- | --- |
| s0_math 1 | E5 fixture permanente de l'oracle | Oui : empreinte `e5` et fait `e5_window` (deux lectures) ; D2 s'y ajoute |
| s0_math 2 | Aucune résolution de trace ne filtre $\Gamma_K$ à $W_K$ (S3, S11) | Confirmé par `v_math`. C'est écrit au lemme W.2 et au registre. Le mutant « graine résolue dans le seul $W_K$ » est à écrire en S3 et S11 ; E5 et D2 le tuent (§ 9) |
| s0_math 3 | Marquer les résultats Python à $k=1$ ($m=2$) | Marqué dans HIERARCHIE_POINTS § 9 ; question 5 du § 10 |
| s0_math 4 | Longueur du § 10 | Gardé : aucun lecteur ne demande de migration |
| s0_math 5 | Stabilité de `kparties_reliees` non revendiquée | Conforme selon `v_math` et l'auditeur (D.1) ; question 2 du § 10 |
| s0_docs 1 à 4 | Agrégat `cofaces`, `tree_k_sha256`, doubles échecs, E2 | Répondus par l'auditeur (D.1 à D.4) et intégrés (§ 1) |
| s0_docs 5 | Commentaire `io.hpp:167-171` (omet `output_conflict` de `retract()`) | **Non corrigé** : c'est du code S4 livré, et L0 n'écrit que documents et oracle. À aligner en S5, qui modifie `io` pour D.3 (§ 9) |
| s0_docs 6 | Détails S5 (ligne de refus, décimaux, octets du manifeste, `counts` de `full`) | Restent à S5, comme écrit au § 3 et au § 8 de SORTIES ; seule la garde de la sortie standard est annoncée |
| s0_docs 7 | Cohérence avec le § 10 | Vérifiée (§ 6) |
| s0_docs 8 | Contrôles du lecteur : numérotation canonique, `ball_count` nul aux feuilles de K1 | **Non ajouté** à L0 : la liste du lecteur vient du § 2 de DECISIONS. Je recommande de les ajouter avec le lecteur, en S7 |
| s1_oracle 1 | Lecture de E5 | Les deux lectures sont nommées |
| s1_oracle 2 | Morton dans l'oracle | Non, par conception : les différentiels retrient par (postordre, niveau, centre) ; $S^*$ et l'ordre des supports se jugent côté natif. C'est écrit dans le README de la référence |
| s1_oracle 3 | Suite `long` | Voir le trou du § 4 et la question 4 |

## 6. Cohérence croisée

| Objet | MATHEMATIQUES § 10 | SORTIES | Réponse à l'audit | Registre | Oracle |
| --- | --- | --- | --- | --- | --- |
| Noms des comptes | `kparties_reliees` (en tête), `cofaces` (boule, support), `strict_traces`, `compressed_parts`, `components`, `gabriel_cofaces` | mêmes, § 6 | mêmes | mêmes (G) | mêmes clés (`_support` pour la portée support) |
| `kparties_reliees` | $\binom{p+m}{K}$ ; indépendant de $\mathcal{Q}_b$ à $(p,m,K)$ fixés, pas stable (3 puis 1) ; somme = incidences | idem, § 6 et § 8 | idem, § B.2 | idem | somme 54 659 décrite comme incidences |
| D2 | $41<64<1681/25$, $ABC$ faible, $AB$ hors $\mathrm{Cat}_2$, descente vers $ZW$, fusion (145/2, 2) sans la boule de $AB$ | — | ligne D2 | entrée `false_in_general` | fixture `d2_audit`, fait `d2_witness` |
| E5 | deux lectures, (24, 2) dans les deux | — | § D | entrée `false_in_general` | `e5_window` |
| Cube à K1 | ligne du § 10.11 | — | garde S6 | — | fait `cube@1` |
| Translation | numérotation invariante, ordres par `SiteIdx` variables | idem, § 10 | idem | — | contre-épreuve gravée |
| `tree_k_sha256` | — | version 2, § 8 | ligne D.2 | — | — |
| D.3 | — | `published_complete`, § 3 et § 9 | ligne D.3 | — | — |
| Fenêtre | $p+q-1\leq K\leq p+m$ | $p+q_{\min}-1\leq K\leq p+m$ | idem | $W_K$ | `p + q <= K + 1` sur $\mathrm{Cat}_K$ |

## 7. Portes finales

| Commande | Résultat exact |
| --- | --- |
| `python3 -S -B test_supports.py` (dans `reference/`, Python 3.12.1) | code 0 ; `reference_supports_ok nuages=210 ordres=951 boules=15062 supports=16943 noeuds=12441 coupes=48074` ; 30,7 s de CPU |
| `python3 -O -S -B test_supports.py` | code 0 ; même sortie (md5 `55c4a1b6a4d2a67a6b47724eaec868b2`) |
| Python 3.10.21, `-S -B` puis `-O -S -B` | code 0 et 0 ; même md5 ; 35,7 et 35,2 s de CPU |
| `PYTHONHASHSEED=4242 python3 -S -B test_supports.py` | code 0 ; même md5 |
| Ligne de synthèse | `reference_supports faits=50 ecarts=0 clouds=210 orders=951 balls=15062 supports=16943 nodes=12441 cuts=48074 births=6558 merge_balls=5768 internal=2736 passing=271 merges=4428 nary_merges=1890 arity2=10158 arity3=5640 arity4=1145 extended=2551 multi_support=530 extended_higher_arity=226 strong=7584 weak=7478 kparties=54659 cofaces=20463 shared_cofaces=277 gabriel=17058 non_gabriel=40053 invariance=70 permuted=70` |
| `test_supports.py --inject=<nom>`, 13 mutants, en 3.12 normal et `-O`, en 3.10 normal et `-O` | code 4 partout, ligne `mutant_killed <nom>`, cause présente |
| `test_supports.py --inject=absent` | code 2 |
| `python3 -S -B test_ref.py --suite=fast` | code 0 ; `reference_fast_ok nuages=342 ordres=1362 coupes=48234 noeuds=13029` |
| `cmake -S morsehgp3D_v11 -B /tmp/v11-l0-consolidation/build -DMHGP11_MODULES=reference -DCMAKE_BUILD_TYPE=Release` | code 0 ; 107 portes enregistrées, dont 30 pour les supports |
| `ctest --test-dir /tmp/v11-l0-consolidation/build -LE long -j3` | `100% tests passed, 0 tests failed out of 90` ; 114 s réelles ; `mhgp11_reference_supports` 40,4 s, `_opt` 38,5 s |
| `python3 -S -B morsehgp3D_v11/tools/check_style.py --root morsehgp3D_v11`, puis sous `-O` | `style_ok fichiers=424`, code 0, dans les deux modes |
| `python3 -S -B tests/support/test_check_style.py tools/check_style.py`, puis sous `-O` | `check_style_ok controles=133`, code 0 |
| `python3 -B tools/check_docs.py`, à la racine du worktree | code 1, 156 lignes, sha256 `8910a36a857153903d5ea61037382757d2c7d47ac01bc89b66b45e941bec7c4b`, **identique à l'octet** à la base `git archive origin/main`, restreinte au checkout partiel et extraite dans `/tmp/v11-l0-consol/base` (même code, même sha256) : aucune ligne d'erreur nouvelle |
| `validate_md.py` : règles de `check_docs.validate` sur les dix Markdown touchés, liens résolus contre `origin/main` | `validate_md ok remarques=0` |
| `verif_faits.py`, sous `-S -B` puis `-O` | `verif_faits ok ecarts=0`, sorties identiques |

Toutes les portes Python tournent en bibliothèque standard, sans `assert`.

## 8. Fichiers

`git -C /workspaces/E-HGP/build/v11-impl-l0 status --short` :

```text
 M docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md
 M morsehgp3D_v11/README.md
 M morsehgp3D_v11/audits/README.md
 M morsehgp3D_v11/cmake/modules.cmake
 M morsehgp3D_v11/docs/ARCHITECTURE.md
 M morsehgp3D_v11/docs/HIERARCHIE_POINTS.md
 M morsehgp3D_v11/docs/MATHEMATIQUES.md
 M morsehgp3D_v11/docs/PROVENANCE.md
 M morsehgp3D_v11/reference/README.md
 M morsehgp3D_v11/reference/ref_mutants.py
 M morsehgp3D_v11/reference/tests.cmake
?? morsehgp3D_v11/audits/REPONSE_CLAUDE_SUPPORTS_20261004.md
?? morsehgp3D_v11/docs/SORTIES.md
?? morsehgp3D_v11/reference/hgp11_ref/supports.py
?? morsehgp3D_v11/reference/test_supports.py
```

**Modifiés par la consolidation** : tous ces fichiers, sauf `reference/hgp11_ref/supports.py`, que je n'ai pas touché.
L'index est vide. `docs/implementation_status.toml` n'est pas touché.

**Pièces de vérification**, dans `l0_consolidation_verif/`, avec `SHA256SUMS` (28 fichiers) :
- `verif_faits.py` et sa sortie : D2, E5 dans ses deux lectures, diamètre 3 puis 1, somme 5, cube à K1, carré,
  translation ;
- `d2_enfants.py` : enfants de la fusion de niveau $1681/25$ dans les trois graphes ;
- `validate_md.py` ;
- les sorties de la porte, des 13 mutants, de CTest et de `check_docs` (base et final).

## 9. Notes pour l'intégration

- **`audits/README.md`.** Le fichier du worktree diffère de `f98aeed67` sur environ 160 lignes : c'est la version
  `aef7182b3` plus une ligne. Le commit L0 doit seulement insérer, sur `origin/main`, après la ligne « Question
  courante du développeur sur 100 ms », la ligne suivante :

  ```text
  - [Réponse courante du développeur sur les supports](REPONSE_CLAUDE_SUPPORTS_20261004.md) : gardes adoptées, réponses D.1 à D.4 intégrées au contrat S0.
  ```

  Si `origin/main` a bougé sur ce fichier, rejouer l'insertion ; ne jamais recopier. Ce README dit « Six Markdown
  actifs », et notre réponse en fait sept : ce texte est celui de l'auditeur, je ne l'ai pas changé.
- **Brouillon S5.**
  - La ligne `api` de la table et de `modules.cmake` est identique, à l'octet, à celle de L0.
  - Il doit passer à `tree_k_sha256` version 2.
  - Il doit conserver l'empreinte du manifeste après sa fermeture (`commit_steps`) et rendre `published_complete`.
  - Il doit aligner le commentaire de `io.hpp:167-171`.
  - L'ordre des raisons est à trancher avec S6 (`reasons.def`, `tests/core/status_test.cpp`).
- **Brouillon S6.** Son `modules.cmake` porte `api` jusqu'à `head`. À l'intégration, il ne doit garder que l'insertion
  de `supports`, déjà présente dans L0 à l'identique ; la ligne `api` reste celle de L0. Sa ligne `supports` de la
  table est identique à celle de L0.
- **Brouillon S3.** Garder le cast `u32` des traces par `tower_capacity`, sans plafond 24 dans FULL. Le témoin D2
  entre dans les fixtures du juge E2. Ajouter le mutant « graine résolue dans le seul $W_K$ », que E5 et D2 tuent.
- **PROVENANCE.** Les lignes « annoncées » de L0 (E2, `enumerate.cpp`, `write_full.cpp`) seront remplacées par les
  sections de ports réels de S3, S5 et S6, ou y renverront.

## 10. Questions pour l'utilisateur

1. **Règle « réponse des auditeurs avant toute ligne native ».** Le plan révisé la fixait comme sortie de L0. Les
   brouillons natifs S3, S5 et S6 ont pourtant été commencés en parallèle, vers 21 h 00. J'ai reformulé la règle en
   « avant tout commit et toute qualification natifs », et la réponse à l'audit le reconnaît. L'auditeur mathématique
   a répondu (`aef7182b3`) ; l'auditeur moteur, pas encore. Confirmez-vous cette reformulation, ou faut-il geler ces
   brouillons jusqu'à sa réponse ?
2. **Portée de la décision 5.** `kparties_reliees` ne dépend pas de $\mathcal{Q}_b$ et échappe bien à l'instabilité des
   supports (cercle à quatre points). Il n'est pourtant pas stable en général : sortir un site de la coquille d'une
   boule diamétrale le fait passer de 3 à 1. Les documents l'écrivent comme une réserve, sans rien changer à la
   décision. Voulez-vous en plus un compte stable, ou la réserve suffit-elle ?
3. **Auteur du report de `plat`.** J'ai écrit « décision écrite de l'utilisateur », consignée par le développeur.
   Voulez-vous plutôt que le développeur puisse décider seul, sur avis écrit ?
4. **Couverture bornée.** Faut-il programmer, avant la qualification de S6, une suite `long` de l'oracle ($n$ de 11 à
   14, $K\leq 10$) et une coquille de 24 sites (par exemple $x^{2}+y^{2}+z^{2}=5$ à $K=2$) ? Elle demande de récrire
   $N_j$ par combinaisons et une session G4. Aujourd'hui, $K\geq 6$ et les coquilles de 13 à 24 sites n'ont aucune
   porte bornée.
5. **Résultats Python à $k=1$.** La chaîne plate Python calcule encore avec $m=2$ à $k=1$, jusqu'à son alignement en
   L3. Faut-il signaler explicitement, dans les démos ou reçus déjà publiés, les résultats à $k=1$ ?
