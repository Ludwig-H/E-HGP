# Critique indépendante : périmètre « arbre d'ordre K seulement », sondes de banc, tranches, G4

4 octobre 2026, 19 h 21 UTC (heure lue par `date -u`). Rôle : avocat indépendant de la question du périmètre.
Lecture seule : rien construit, modifié ni commité ; seule écriture, ce fichier. **GCP non utilisé.**

```text
phase=exploration_v11_hors_registre
backend=cpu_reference
profile=quantized_u21_input_only   (défaut de compilation ; u18 et u24 qualifiés à part)
public_status=not_claimed
```

Sources : worktree `build/v11-claude-20261003/morsehgp3D_v11` à `57dd21be1` (code), `main` à `1bf4be68f` (reçus,
audits). Chemins relatifs à `morsehgp3D_v11/` sauf mention. Dossier lu en entier : cinq lectures, deux conceptions,
`SPECIFICATION_FINALE.md` (dite « la spécification »).

---

## 0. Verdict en six lignes

1. **Objet : oui à l'arbre K seul.** Les sorties `supports`, `points` et `plat` ne publient que $T_K$ et ce qui s'y
   attache. C'est juste mathématiquement et dans le code : $H^{r}_{K+1}$ ne lit que l'ordre K.
2. **Calcul : non à « ordre K seul » dès la première livraison**, du moins par la voie que la spécification retient
   (voie par lots, `concurrent_orders` refusé jusqu'en S11). Les reçus G4 montrent que cette voie est **2,4 à 3,3 fois
   plus lente** à W48 que le pipeline FULL qualifié, pour toutes les forêts. Ne garder que l'ordre K n'enlève qu'environ
   30 % du travail de descente ; la voie par lots perdrait donc vraisemblablement à W48, même sur un seul ordre.
3. **Meilleure option : calculer $T_K$ par `build_full` (pipeline 16379, inchangé), tenir le journal des graines sur le
   seul constructeur d'ordre K, et ne publier que $T_K$.** Il y a un seul chemin moteur dans le produit, et l'empreinte
   `tree_k_sha256` est commune par construction. La porte d'identité I10 n'a plus d'objet en v1. La voie « ordre K
   seul » devient une optimisation mesurée (S11), menée **dans** le chantier 100 ms.
4. **FULL reste complet** (1..K et verticales) : `MHGP11FUL1` est identique à l'octet. Aucune discussion là-dessus.
5. **Sondes de banc : figées pour de bon**, et promues témoins différentiels. `mhgp11_full_bench` reste l'instrument
   de performance du chantier 100 ms, puisque le CLI n'a pas d'options moteur. On ne leur ajoute aucune sortie.
6. **Tranches : quatre livraisons G4 au lieu d'environ neuf.** `--sortie=supports` arrive dans la même livraison que
   `--sortie=full`, et non une tranche plus tard. L'oracle borné (Python bibliothèque standard) livre de la valeur
   vérifiable dès le premier jour, sans G4.

---

## 1. Ce que l'utilisateur a demandé, et ce que la spécification en a fait

- Demande : « travailler sur l'arbre d'ordre K plutôt que toute la tour ». La phrase peut viser **l'objet** (ne
  publier que $T_K$, y attacher les supports) ou **le calcul** (ne construire que la forêt K).
- La spécification honore les deux, et dès la première livraison (§ 0.3, § 3.2, § 7.1).
  - `build_order` reprend la mise en place de la boucle **non concurrente** de `build_full`
    (`src/tower/forest_vertical.cpp:298-327,334-352`), avec `concurrent_orders` « refusé jusqu'à S11 ».
  - Elle reconnaît elle-même que cette voie « peut perdre contre le pipeline FULL à W48 » (§ 7.1). Elle renvoie la
    mesure à S7 et la correction à S11.
- La conception « exactitude » faisait l'inverse (§ 0.6, § 6.1) : objet $T_K$ dès la première livraison, calcul
  par `build_full`, ordre K seul en dernière tranche. L'arbitrage a tranché en faveur de l'utilisateur, sans chiffrer
  ce que cela coûte.
- L'utilisateur a précisé ensuite que ses réponses n'étaient pas intangibles. Je chiffre donc ce coût (§ 3).

---

## 2. L'objet : l'arbre K suffit (vérifié)

| Affirmation | Vérification |
| --- | --- |
| $H^{r}_{K+1}$ ne lit que l'ordre K | `bench/points_radius.py:208-216` : `hang_margin_radius(order, m)` appelle `ph.qualify(order, m)`, `qualified_starts`, `first_points`, `order.lca` sur **un seul** ordre. La qualification $m=K+1$ est un seuil de cardinal, pas une lecture de l'ordre K+1. |
| La forêt d'ordre 1 de la sonde ne sert qu'au cœur et à K = 1 | `bench/points_export.cpp:306-313` (singletons), utilisés en `:162` (départ du cœur) et `:252` (incidence à K = 1, où $T_1$ est l'arbre lui-même). Ni le cœur ni le k-NN ne sont lus par $H^{r}$. |
| Une forêt d'ordre K ne lit aucune forêt inférieure | `ForestBuilder::cell` (`src/tower/forest_plateau.cpp:39-67`) résout ses traces dans `result`, la forêt en construction. Seules les verticales lisent l'ordre K−1. |
| `build_forest` sur un seul ordre existe | `src/tower/forest.hpp:131-137`, `src/tower/forest_build.cpp:414-421`. |
| Le catalogue ne diminue pas | Les jonctions d'ordre K exigent $p+q=K+1$, donc tout $\mathrm{Cat}_K$ (lecture `foret_k`, § 4). Le domaine reste l'étage dominant, 181 à 255 ms à W48. |
| Sortie `full` complète | `build_full` construit 1..K et les verticales (`forest_vertical.cpp:281-356`). `MHGP11FUL1` les sérialise (`bench/full_probe.cpp:40-75`). |

**Conclusion sur l'objet.** L'utilisateur a raison. Publier toute la tour pour `supports` alourdirait le format sans
rien apporter au jeton : P_v vit sur $T_K$. L'audit `1bf4be68f` le confirme : la sortie est « cohérente avec la forêt
existante » (`receipts/audit_supports_20261004/README.md`).

---

## 3. Le calcul : les faits vont contre « ordre K seul » maintenant

### 3.1 Ce que mesurent les reçus

Reçu `receipts/qualification_performance_20261003/review/metrics.optimized.json`, médianes de trois prises, K = 5,
u21, temps de l'étage `forest` (verticales comprises), en ms :

| Trame | Voie par lots `current2047`, W48 | Ordres concurrents `current16379`, W48 | Rapport | Après pipeline (tranche 3), W48 |
| --- | ---: | ---: | ---: | ---: |
| ng00 | 571,0 | 240,8 | 2,37 | 171,0 |
| ng01 | 460,0 | 163,6 | 2,81 | 133,0 |
| ng02 | 645,0 | 197,2 | 3,27 | 157,7 |

- Dernière colonne : `receipts/developpement_20261003/pipeline_g4/README.md`, tableau `claudeab5`.
- À W1, l'écart est faible : 5 600 contre 4 882 ms sur ng00. C'est la parallélisation qui sépare les deux voies, pas
  le travail. Le commentaire du code le dit aussi : « La voie historique cumulait ~600 barrières et ~330 ms de pilote
  seul sur G4 W48 » (`src/tower/forest_concurrent.cpp:8-9`).
- Réserve : les masques 2047 et 16379 diffèrent sur plusieurs bits (mémo, table de populations, graphe de paires,
  ordres concurrents). Le rapport n'isole pas le seul effet de la voie.
- La configuration exacte de `build_order` (16379 sans 8192, donc voie par lots sans mémo, table de populations non
  liée) **n'a aucun reçu**. Je n'ai trouvé le masque 8187 dans aucun reçu de performance (recherche dans `receipts/`).

### 3.2 Projection (ordre de grandeur, non mesurée)

- À K = 5, l'ordre K porte 69 à 70 % des présentations MEB des descentes (lecture `foret_k`, § 4, même reçu).
- La voie par lots réduite à l'ordre K coûterait donc grossièrement 0,6 à 0,8 fois 460–645 ms, soit **270 à 520 ms**.
  Le pipeline FULL de la tranche 3, lui, fait **133 à 171 ms pour les cinq ordres et leurs verticales**.
- Le total `supports` serait alors d'environ 220 ms (domaine) + 270–520 ms, au lieu d'environ 350 à 410 ms par
  `build_full`.
- Le gain plafond de l'ordre K seul **dans le pipeline** (S11) serait de l'ordre de 30 à 45 % de l'étage forêt, soit
  40 à 75 ms. Le domaine, inchangé, reste le mur (`README.md`, tranche 3 : « le domaine (200–255 ms) borne désormais
  le total »).
- À W1, l'ordre K seul gagnerait environ 30 % du travail forêt. Mais la cible du contrat est G4 W48.

**Lecture.** Calculer l'ordre K seul par la voie par lots rendrait le produit **plus lent** sur la machine cible, sans
rien changer à l'objet. La mémoire économisée (une forêt au lieu de K, sans verticales) se compte en dizaines de Mo à
30 000–60 000 sites. Elle ne lie rien aujourd'hui ; elle compterait à $10^{7}$ points, ce qui n'est pas ce chantier.

### 3.3 La meilleure option : `build_full` qualifié, journal sur l'ordre K

**Pourquoi le journal tient dans le pipeline.**
- Les deux seuls points d'application des cellules sont `ForestBuilder::cell` et `ForestBuilder::regular_cell`
  (`forest_plateau.cpp:39-67,100-120`).
- Le pipeline les appelle depuis `ForestBuilder::publish`, une tâche par ordre, dans l'ordre des `BallIdx`
  (`src/tower/forest_concurrent.cpp:27-56`).
- La voie par lots les appelle aux lignes `src/tower/forest_parallel.cpp:275` et `:301`.
- Le pipeline promet « mêmes graines, forêts, verticales et compteurs que la voie par étages »
  (`src/tower/forest_pipeline.cpp:1-7`).
- Un `SeedLog*` posé sur `s.builders[kmax-1]` dans `build_concurrent` (`forest_concurrent.cpp:239-244`), et sur
  `builder` dans la boucle non concurrente (`forest_vertical.cpp:334-339`), a donc un écrivain unique et un contenu
  déterminé par les graines.

**Coût en code.**
- Les deux lignes gardées que la spécification ajoute déjà dans `forest_plateau.cpp`.
- Plus un paramètre facultatif (pointeur nul par défaut) qui descend jusqu'au constructeur d'ordre K, dans
  `build_full` et `build_concurrent`.
- Aucune ligne n'est à recopier de `build_full` : la spécification, elle, duplique la mise en place des lignes
  298–327 dans `order_tree.cpp`.

**Ce qu'on gagne face à la spécification.**
1. **Un seul chemin moteur dans le produit.** `full` et `supports` passent par le même appel. L'égalité de
   `tree_k_sha256` (I11) est vraie par construction, et la porte d'identité I10 (`build_order` = `build_full`) n'a plus
   d'objet en v1 : elle passe en S11.
2. **Le moteur qualifié à l'échelle.** C'est le masque 16379 de 81/81 prises et de la qualification `claudeab5`, et
   non une combinaison 8187 jamais mesurée.
3. **Le temps mural le plus bas connu à W48** pour l'objet demandé.
4. **Pas de mise en place recopiée.** Le CLI fixe le masque 16379 (spécification § 5) ; avec un `build_order` qui
   refuse `concurrent_orders`, `supports` tournerait de toute façon sur une autre configuration que `full`.

**Ce que cela coûte.**
- On calcule des ordres 1..K−1 et des verticales qu'on jette : du CPU gaspillé, mais pas du temps mural à W48.
- On touche `build_concurrent`, le code chaud du chantier 100 ms, mais d'un seul pointeur facultatif. La
  spécification le touche de toute façon en S11, et bien plus (naissances par blocs à $K\geq2$,
  `forest_build.cpp:372` ; liaison clairsemée, `population_lookup.hpp:55-59` ; un ordre dans `pipeline_orders`,
  `forest_pipeline.cpp:169,178`).
- Il faut un TSan ciblé (`mhgp11_tower_pipeline`) pour l'écriture du journal par le publieur. Ce TSan existe déjà
  (7/7 dans `claudeab5`).

**Garde-fou doctrinal.**
- Le journal n'est **jamais** publié ni haché. Seuls $\mathrm{att}(b)$, $\mathrm{ant}(b)$ et $S(b)$ le sont.
- Ces trois valeurs ne dépendent ni de la graine ni du chemin (lemmes D et E de la spécification ; T5,
  `docs/MATHEMATIQUES.md:212-222`).
- Les portes W1/W4/W48 et la porte de permutation jugent les sorties, pas le journal.

### 3.4 Alternative écartée : E2 (descentes) comme produit

- La conception « exactitude » livrait E2 d'abord, ce qui ne touche aucun constructeur.
- Mais $\mathrm{ant}(b)$, demandé par l'utilisateur (« liaisons internes, fusions »), exige une descente **par
  trace stricte**. Cela rejoue toute la résolution régulière de l'ordre K, soit environ 70 % du travail de descente.
- E2 reste le juge de test, comme le veut la spécification. Mettre un juge dans le produit est d'ailleurs contraire
  à `docs/ARCHITECTURE.md:21-22`.

### 3.5 Quand « ordre K seul » redevient la bonne réponse

- S11 (pipeline à un ordre) est une **tranche de performance du chantier 100 ms**. Elle n'appartient pas au chantier
  des sorties.
- On l'adopte comme défaut **seulement** si un reçu G4 apparié la montre plus rapide à W48 sur ng00–02, à K5 et K10.
  La porte I10 garde alors l'identité de la forêt, octet pour octet.
- Pour un futur profil W1 ou petite machine, la voie par lots à un ordre peut gagner. Ce serait un choix de défaut
  par mesure, jamais une option sans ablation (`docs/ARCHITECTURE.md:21-22`).

---

## 4. Les sondes de banc

| Sonde | Ce qui la fige | Rôle recommandé |
| --- | --- | --- |
| `mhgp11_full_bench` (`bench/full_probe.cpp`, déclarée `tests/tower/tests.cmake:64-65`) | porte à ligne exacte `attempts427 successes413 refusals14` (`tests/tower/tests.cmake:77-78`), masque 0..131071 et passes à chaud (`full_probe.cpp:348-388`), pilotes `bench/ab_g4.py`, `march_variants.py`, plans de session épinglés | **Instrument de performance permanent** du chantier 100 ms : le CLI n'a aucune option moteur (règle 6), seule la sonde peut faire des A/B de masques. **Témoin d'identité** : `mhgp11 --sortie=full` doit rendre le même `raw_sha256` (spécification § 8.6). |
| `mhgp11_points_export` (`bench/points_export.cpp`, `tests/tower/tests.cmake:67-68`) | porte `mhgp11_tower_points_export_width` (`:74-76`), lecteur `bench/points_hierarchy.py`, sessions E1, démos (mémoire « HGP : toujours le moteur actif ») | **Témoin des incidences fortes** : le sous-ensemble fort de `WindowAttachment` doit égaler le bloc d'incidences `MHGP11PH` à l'octet (spécification § 8.3). **Source de l'oracle Python** pour `mhgp11_points_vs_python` (S9). Chaîne des démos jusqu'à ce que `--sortie=points` soit reçu. |

**Règles.**
1. Aucune modification d'octet, d'argv ni de ligne JSON. Aucune sortie `supports` ajoutée aux sondes : ce serait
   recréer un second exécutable produit, contre `docs/ARCHITECTURE.md:50`.
2. Les démos migrent vers `mhgp11 --sortie=points` seulement après le reçu d'identité site par site contre
   `points_export` + `points_radius.py`. Ce reçu est déjà prévu en S9 (`mhgp11_points_vs_python`). On gagne alors les
   3,1 à 8,4 s de Python `margin_r` par scène (lecture `points`, § b).
3. Aucun retrait des sondes dans ce chantier. Les reçus immuables épinglent leur binaire et leur argv
   (`receipts/developpement_20261004/*/sessions/*/plan.json`, lecture `infra`, § d.1).

---

## 5. Découpage : la valeur vérifiable au plus vite

### 5.1 Le coût réel est le nombre de sessions G4

- Aucune porte native ne tourne dans le Codespace : `README.md`, section « Construction », dit « aucun build ou test
  natif dans le Codespace ».
- Une session dure au plus 4 200 s (mémoire « Chantier 100 ms v11 »).
- La spécification termine **chaque** tranche native par une session gardée avec la matrice complète (§ 8.8). Pour
  S2–S11, cela fait environ neuf sessions. La chaîne critique de `supports` passe par S2 → S3 → S5 (après S4) → S6
  → S7, soit **quatre à cinq sessions** avant qu'un seul `supports.mhgp11sp` existe sur une trame.
- `--sortie=full` livrée seule (S5) n'apporte rien de neuf à l'utilisateur : le banc produit déjà ce fichier. Sa vraie
  valeur est de valider `io` et `api`.

### 5.2 Découpage recommandé (mêmes contenus que S0–S11, regroupés)

| Livraison | Contenu (tranches de la spécification) | Session G4 | Valeur vérifiable pour l'utilisateur |
| --- | --- | --- | --- |
| **L0** | S0 (contrat) + S1 (oracle borné des supports, Python bibliothèque standard) | **aucune** : `python3 -S` et `-O` en local, comme `receipts/audit_supports_20261004/check.py` | Le jour même : $\mathcal{Q}_b$, rôles, $\mathrm{att}$, $\mathrm{ant}$ et comptes sur les 13 fixtures (carré, cube, `growth_ABCZ`, passagère, équilatéral K2). L'utilisateur les vérifie à la main. |
| **L1** | S2 (en-tête public) + S3' (journal sur `build_full`, `WindowAttachment`, juge E2) + S6 (module `supports`) + sondes de test JSON (`attach_probe`, `supports_probe`, spécification § 8.3) | **#1** | Différentiel natif contre oracle sur les petits nuages. E1 = E2 sur `scale8000`. Comptes de $W_K$, rôles et $\mathcal{Q}_b$ sur ng00–02 à K5. Identité forte contre `points_export`. |
| **L2** | S4 (`io`) + S5 (`api`, F5) + S7, avec **`--sortie=full` et `--sortie=supports` ensemble** ; lecteur `bench/mhgp11_formats.py` | **#2** | Un exécutable `mhgp11` qui écrit `supports.mhgp11sp` sur les trames LiDAR. Identité `full` contre le banc. Déterminisme W1/W4/W48. Échantillon 32 000. K10 LiDAR une fois. |
| **L3** | S8 (`num`) + S9 (`points`, alignement $m(1)=1$) | **#3** (pip épinglé pour les différentiels numpy) | `--sortie=points` natif, identique site par site à la chaîne Python. Les démos migrent. |
| **L4** | S10 (`plat`) | **#4** (pip épinglé) | `--sortie=plat` natif, `lot2` (144 arbres), F14a–e. |
| **L5** (facultative, chantier 100 ms) | S11 (pipeline à un ordre) | A/B dédié | Seulement si la mesure le justifie (§ 3.5). |

- S8 (`num`) peut se développer pendant L1 et L2, mais n'a besoin d'être qualifié qu'en L3 : rien en amont n'en
  dépend.
- Si une session échoue, on rejoue la livraison entière, pas une tranche isolée. Le risque est borné par le petit
  nombre de fichiers touchés dans `tower`.

### 5.3 Pourquoi `supports` avant `points`

- L'utilisateur l'a demandé : c'est la sortie qui **n'existe pas encore**. `points` existe déjà par la sonde et Python.
- `points` natif réutilise le rattachement de L1 : les incidences fortes sont un sous-ensemble de $W_K$
  (spécification § 7.6). L'ordre supports → points ne coûte donc rien.
- Réserve honnête : pour l'objectif principal de la v11, la comparaison à HDBSCAN, `points` natif a plus de valeur
  pratique (vitesse des démos). Si l'utilisateur veut d'abord des démos plus rapides, on fait L3 avant L2, sans
  autre changement.

---

## 6. Qualifications G4

### 6.1 Indispensables à chaque livraison native (sans elles, pas de verdict « conforme »)

1. **GCC Release u21 et suite `fast` complète.** `tower` est touché, toute la suite le juge. Pas de native en
   Codespace.
2. **Rejeu du manifeste des mutants de `tower`**, avec des motifs uniques (`tests/mutants/tower.json`), plus les
   mutants causaux de la livraison (spécification § 8.5).
3. **TSan ciblé** quand un écrivain concurrent est ajouté : en L1, `mhgp11_tower_pipeline` avec le journal ; en L3,
   le parallélisme par site de `points`.
4. **ASan/UBSan sur les unités neuves** : `supports` (L1), `io`/`api` (L2), `num`/`points` (L3), `head` (L4).
5. **Correction à l'échelle, sans juge $O(n^3)$** : `scale8000` avec E1 = E2 sur toutes les boules, ng00–02 à K5 avec
   I1–I6, sorties identiques à W1/W4/W48 et sous permutation (I8, I9).
6. **Construction u24 et sous-ensemble `fast`, une fois par livraison.** Les chemins i128 sous certificat et
   `Wide` des prédicats, et la largeur des niveaux, changent avec le profil (lecture `supports_qb`, budgets).

### 6.2 Ce qui peut attendre ou se regrouper

| Qualification | Quand | Pourquoi elle peut attendre |
| --- | --- | --- |
| K10 sur LiDAR (`long`) | une fois, en fin de L2 | Mêmes chemins de code qu'à K5. Le risque est de taille (format, `MHGP11SP`), mesuré une fois. |
| Échelle 16 000 | L2 | 8 000 (tout) + 32 000 (échantillon de 2 000 boules) encadrent déjà la pente. |
| Mesures de temps dédiées | jamais en L1 ; gratuites en L2 (ligne JSON par étage) | Aucune décision de défaut ne dépend du temps avant S11. Aucun benchmark ne promeut un statut. |
| A/B ordre K seul contre FULL | L5 seulement | C'est la seule décision qui dépend d'une mesure (§ 3.5). |
| u18 | après L4 | Profil distinct, non demandé par le contrat LiDAR (grille 1 mm, u21 par défaut). |
| Différentiels numpy (`points_vs_python`, `head_vs_python`) | L3 et L4, session `python_packages: pinned` | Python 3.10 sans numpy sur G4 (mémoire « portes Python nues ») : les portes `fast` ne peuvent pas les porter. |
| Clang, GPU | hors chantier | Clang absent de la VM (`README.md`) ; aucune sortie n'a de voie GPU. |

---

## 7. Ce que je change par rapport à la spécification

| Point | Spécification | Recommandé | Raison factuelle |
| --- | --- | --- | --- |
| Calcul de $T_K$ en v1 | `build_order`, voie par lots, ordre K seul (S3) | `build_full` 16379 et journal sur l'ordre K | Voie par lots 2,4 à 3,3 fois plus lente à W48 (§ 3.1). Masque 8187 jamais mesuré. Un seul chemin moteur. |
| I10 (identité `build_order` / `build_full`) | porte de S3 | reportée en S11 | Sans objet tant qu'il n'y a qu'un chemin. |
| Ordre `full` puis `supports` | S5, puis S7 | ensemble en L2 | `full` seul n'apporte rien de neuf ; on économise une session. |
| Une session G4 par tranche | environ 9 | 4 (+1 facultative) | Sessions de 70 min au plus, aucune native en Codespace. |
| Oracle S1 | précède le natif | inchangé, mais livré **seul et tout de suite** (L0) | Valeur vérifiable sans G4. |
| Sondes | inchangées | inchangées, avec un rôle nommé (§ 4) | `full_bench` = instrument du chantier 100 ms. |

Le reste de la spécification n'est pas contesté ici (objet $W_K$, lemmes A–G, $\mathcal{Q}_b$, formats, transaction,
`num`).

---

## 8. Question à l'utilisateur

Votre « arbre d'ordre K plutôt que toute la tour » visait-il **l'objet publié** ou **le calcul** ?

- Je recommande l'objet $T_K$ seul dès la première livraison, calculé par la tour FULL qualifiée en ne gardant que
  l'ordre K.
- Les reçus G4 indiquent qu'un calcul par l'ordre K seul, avec la machinerie disponible sans toucher au pipeline,
  serait environ deux fois plus lent à W48.
- La voie à un seul ordre dans le pipeline (gain plafond estimé à 40–75 ms, le domaine restant le mur) relèverait du
  chantier 100 ms, adoptée seulement sur reçu apparié.

Cela vous convient-il ? Et préférez-vous `supports` (L2) ou `points` natif (L3) en premier ?
