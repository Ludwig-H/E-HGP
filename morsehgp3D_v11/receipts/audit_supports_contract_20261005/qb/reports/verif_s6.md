# Contre-lecture de la tranche S6a : module `supports` ($\mathcal{Q}_b$, fermeture, comptes du lemme G)

4 octobre 2026. Rapport écrit à partir de 22 h 33 UTC ; l'heure vient de `date -u`. Rôle : contre-lecteur
indépendant de l'implémentation S6a, consignée dans `impl_s6.md`.

Conditions de la contre-lecture :

- **GCP non utilisé.**
- Rien n'a été écrit dans le worktree `build/v11-impl-s6`. Son état `git status` et les empreintes de ses fichiers sont
  inchangés depuis le début de la contre-lecture.
- Aucun `git add`, commit, stash ni branche.
- Les constructions et les essais sont dans `/tmp/v11-s6-verif/`. Les scripts et les comptes rendus de la
  contre-lecture sont dans `build/v11-persist/sortie_supports/verif_s6/`. Aucun de ces fichiers ne contient de
  coordonnée LiDAR.

```text
phase=exploration_v11_hors_registre
backend=cpu_reference
profile=quantized_u21_input_only   (u18, u24 et ASan+UBSan Debug rejoués localement sur core+supports)
public_status=not_claimed
```

## 0. Verdict

**Aucun point bloquant.** La tranche fait ce que demandent la spécification (§ 2.5, 2.6, 3.3, 4, 7.3, 7.4, 8.3, 8.5,
9.1) et `DECISIONS_UTILISATEUR.md`. Ce qui a été vérifié :

- le nom `kparties_reliees` ;
- aucun compte stocké, et `Support` sans champ `cofaces` ;
- $\mathcal{Q}_b$ énuméré sur toute la coquille ;
- le plafond 24 et son refus ;
- deux raisons ajoutées en fin de table ;
- le port explicite et épinglé de `mark_supports` et de `closure_counts` ;
- les supports écrits avant la fermeture.

Le rejeu est vert en u21, u24, u18 et ASan+UBSan :

- 121 portes `fast` sur 121 en u21 ;
- les 20 portes d'échelle et `lidar` hors `long` du module en u21, plus la sentinelle `lidar` ;
- les 6 mutants du manifeste tués par code.

Le contre-juge Fraction, écrit à neuf par la définition, n'a trouvé **aucun écart** sur plus de 12 000 jugements de
boules, les fixtures étant rejugées à chaque profil. Le différentiel de lecture contre l'oracle S1 de L0 n'en a trouvé
aucun non plus : 1 595 boules de $W_K$ en u21, 991 en u18.

Mutants de mon cru :

- 12 essayés, dont 10 tués par les portes enregistrées ;
- les **2 survivants** sont des gardes d'invariant qu'aucune porte ne peut atteindre sans un domaine corrompu (§ 5.2) ;
- ce trou est signalé ; il n'est pas bloquant.

Il reste trois corrections de documentation (§ 7) et des notes pour S6b, L0 et G4.

## 1. Ce qui a été lu

Tout le diff contre `f98aeed67` a été lu, ainsi que tous les fichiers nouveaux, contre les documents suivants :

- `DECISIONS_UTILISATEUR.md` ;
- `CRITIQUE_ET_PLAN_REVISE.md` ;
- `SPECIFICATION_FINALE.md` (§ 2.1 à 2.9, 3, 4, 7.3 à 7.5, 8.1 à 8.5, 9.1) ;
- le rapport `impl_s6.md`.

Les sources de la v11 lues pour situer le code sont :

- la source portée, `bench/catalogue_euler.hpp` ;
- `num`, pour les prédicats et `Sphere::through` ;
- `catalogue.hpp`, pour `CatalogueBall`, `shell()` et `interior()` ;
- `full_domain.hpp` ;
- `cloud/morton.hpp` ;
- `tests/support/test.hpp`, `mhgp11_gate.py` et `cmake/gates.cmake` ;
- `tools/check_style.py`, pour les règles `[inclusion]`, `[dependance]`, `[raison_morte]` et `[raison_sans_porte]` ;
- `tests/mutants/run_mutants.py`.

Le contrat L0 en cours, dans `build/v11-impl-l0`, a été lu seulement : `docs/SORTIES.md` et
`reference/hgp11_ref/supports.py`.

- **Empreintes.** Les 17 fichiers livrés ont été copiés tels quels dans `/tmp/v11-s6-verif/src`. Leurs sha256 sont
  **identiques** à l'annexe de `impl_s6.md`.
- **Campagne de mutants.** Elle a jugé les sources de sha256 `4d232f7baa89…`, avec le manifeste de sha256
  `ab4ef9e7445e…`. Ce sont les valeurs annoncées par l'implémenteur.
- **Table des modules.** `cmake/modules.cmake` est identique à celui de L0 (vérifié par `diff`). La ligne `supports`
  de `docs/ARCHITECTURE.md` est identique, mot pour mot, à celle de L0.
- **Source épinglée.** `bench/catalogue_euler.hpp` a pour sha256 `f293df6e…`, et seul le commit `462dca187` l'a
  touché. Les plages de lignes citées par `PROVENANCE.md` et `source_pins.json` sont justes, à une ligne près pour
  `mark_supports` (note N7).

## 2. Rejeu indépendant

Toutes les constructions ci-dessous partent de la copie figée des sources. Chacune est faite à neuf avec `-j2` et
`MHGP11_MODULES="core;supports"`.

| Construction | Portes jouées | Issue |
| --- | --- | --- |
| u21 Release, `build-u21` | toute la suite `fast` de core et supports, avec `MHGP11_DATA_DIR` posé pour que la sentinelle `lidar` soit jouée | **121/121** (49 s) |
| u21 Release | les 20 portes `scale8000`, `scale16000`, `scale32000` et `lidar` hors `long` du module : juges d'échantillon (uniforme 8000, grille 8000, ng00 K5) et registres (uniformes 8000, 16000 et 32000, grille 8000, ng00 à ng02 K5), chacune avec sa jumelle `_opt`, plus la sentinelle `mhgp11_support_lidar_sentinel` | **21/21**, lignes gravées exactes (22 h 21 à 22 h 28 UTC) |
| ASan+UBSan Debug, `build-asan` | 13 portes unitaires `supports`, `shell_capacity`, `judge_small` et `_opt`, manifeste et `_opt`, `mhgp11_style`, `mhgp11_support_check_style` (et leurs `_opt`), `mhgp11_core_unit_reasons` | **23/23**, aucun `runtime error` ni rapport ASan |
| u24 Release, `build-u24` | 19 portes `fast` : `supports` et `core_unit_reasons` | **19/19** |
| u18 Release, `build-u18` (non joué par l'implémenteur) | les mêmes 19 portes | **19/19** |

Trois autres vérifications ont été faites à la main :

- **Python nu.** `sample_judge.py --small=20261004,40 --k=3` donne, sous `python3 -S -B` comme sous `python3 -S -B -O`,
  la ligne `supports_sample_judge_couverture boules=9067 etendues=4520 multiples=1545 tetraedres=1554 brutes=9066`
  et `supports_sample_judge_ok controles=154294`. Seul Python 3.12 est présent dans le Codespace : la vérification
  sous 3.10 reste à faire sur G4. Aucune construction propre à 3.11 ou plus n'a été vue à la lecture. Les fonctions
  de `random` employées sont `getrandbits`, `randint`, `choice` et `sample` ; à ma connaissance, leur algorithme
  n'a pas changé sur des entiers entre 3.10 et 3.12. L'identité des nuages tirés reste pourtant à constater sur G4,
  par les lignes gravées.
- **Sphere50.**
  - À K = 2, la sonde rend `{"phase":"domain","sites":84,"balls":435,…}` puis
    `supports_probe_verdict refus support_shell_capacity`, avec le code 2.
  - À K = 1, elle rend le même refus sur un $\mathrm{Cat}_1$ de 231 boules.
  - Aucune ligne de boule n'est publiée.
- **Recoupement avec un reçu G4.** Les nombres suivants viennent des portes de registres de la sonde, voie
  séquentielle `build_forest`. Ils sont égaux aux registres de l'ordre 5 du reçu
  `audit_deep_20261004/performance/selected/c40_paired`, lu par `git show origin/main:…`, qui a été produit par la
  voie pipeline sur G4.

  | Trame | $\lvert W_5\rvert$ (`classified_cells`, égal à `work.cells` du reçu) | cellules (`replayed_cells`, égal au reçu) |
  | --- | ---: | ---: |
  | ng00 | 789 886 | 448 805 |
  | ng01 | 652 958 | 369 751 |
  | ng02 | 832 386 | 471 060 |

  La contre-épreuve sommée a donc aussi un témoin produit par une autre voie de la tour.

## 3. Contre-juge Fraction écrit à neuf : `verif_s6/myjudge.py`

Le contre-juge n'emploie aucun code du produit, ni du juge de l'implémenteur, ni de l'oracle L0. Sa méthode :

- **Les boules critiques** sont toutes les parties $S$ de 2 à 4 sites du nuage qui sont affinement indépendantes et
  dont le centre circonscrit est dans l'intérieur relatif de $\mathrm{conv}\,S$.
  - Le centre est calculé dans $\mathrm{aff}\,S$ par les formules fermées des produits vectoriels.
  - Les poids barycentriques viennent de Cramer, par aires et volumes signés. La formulation de Gram n'est pas
    employée.
  - Les boules sont dédoublonnées par (centre, rayon carré).
  - $I_b$ et $U_b$ sont calculés par force brute.
  - $\mathrm{Cat}_K$ est l'ensemble des boules avec $p+q\leq K+1$.
- **$\mathcal{Q}_b$ est calculé par deux voies, dont l'égalité est exigée.** La voie (a) prend les parties
  ci-dessus. La voie (b) prend les parties **minimales** de $U_b$ dont l'enveloppe contient $c_b$ : test faible pour
  au plus 4 sites, puis réduction de Carathéodory. Cette égalité contrôle le lemme F à chaque boule.
- **$N_j$** est le nombre de parties de $U_b$ à $j$ sites dont l'enveloppe contient $c_b$.
- **Les comptes par la MEB exacte** se calculent en force brute sur toutes les parties de $P_b$, sans passer par
  $\mathcal{Q}_b$ :
  - `strict_traces` : les $K$-parties contenant $I_b$ dont la MEB a un rayon strictement inférieur à $r_b$ ;
  - `cofaces` : les $(K+1)$-parties de MEB égale à $b$ ;
  - `gabriel_cofaces` : celles qui contiennent aussi $I_b$ ;
  - incidences par support, avec contrôle de M2.

  Ils sont comparés à la sonde (`--all`) et aux formules du lemme G.
- **Ce qui est comparé à la sonde**, pour chaque boule :
  - l'ensemble des boules de $\mathrm{Cat}_K$, ni plus ni moins ;
  - le niveau ;
  - $(p,m,q)$ ;
  - $\mathcal{Q}_b$ dans l'ordre publié, (arité, rangs de Morton recalculés) ;
  - les `SiteIdx` ;
  - $N_0..N_m$ ;
  - les cinq comptes, les incidences et les incidences de Gabriel.

| Sonde | Entrées | Boules jugées | Coquilles étendues | Plusieurs supports | Comptes par MEB brute | Écarts |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| u21 | 17 fixtures, dont 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 (trois immersions), 12 et la coquille mixte, à tous leurs K | 499 | 65 | 48 | 499 | 0 |
| u21 | 60 nuages tirés : grilles 3 ou 4, pas 1 à 3, 8 à 13 points, K de 1 à 5, un quart sans grille | 3 121 | 1 077 | 189 | 3 121 | 0 |
| ASan+UBSan | fixtures et 80 nuages | 5 252 | 1 624 | 336 | 5 252 | 0 |
| u21 | coquille mixte à l'échelle `kCoordMax/10` (u21), K de 1 à 3 | 109 | 11 | 4 | 109 | 0 |
| u24 | fixtures et 30 nuages, puis coquille mixte à $(2^{24}-1)/10$ et cercle à $B_t$ multiplié par 64 | 1 957 + 115 | 520 + 12 | 116 + 5 | tout | 0 |
| u18 | fixtures et 20 nuages | 1 622 | 428 | 117 | 1 622 | 0 |

Deux contrôles complètent ce tableau :

- **Le contre-juge détecte bien les fautes.** Une sonde truquée a été passée au juge :
  - deux supports permutés ;
  - un $N_j$ augmenté de 1 ;
  - une coface ajoutée ;
  - une boule étendue retirée ;
  - une incidence augmentée de 1.

  Le juge rend `ECHEC` dans les cinq cas, avec 22, 29, 74, 58 et 44 écarts.
- **La fixture `cercle24`** a une coquille de 24 sites, au-delà de la voie (b), limitée à 14 sites. Elle a été
  recomptée à part : 6 diamètres et 220 triangles strictement aigus, donc 226 supports, comme la porte. À 25 sites on
  compte 7 diamètres et 301 triangles, et la porte refuse ce cas.

Ces résultats confirment par la définition les attendus gravés des portes unitaires, à la fois pour $\mathcal{Q}_b$,
pour $N_j$ et pour les comptes : la sonde égale la définition, et les portes unitaires égalent la sonde. Ils
confirment aussi les formules du lemme G par dénombrement brut des MEB, sans passer par $\mathcal{Q}_b$.

## 4. Différentiel de lecture contre l'oracle S1 de L0 : `verif_s6/l0diff.py`

L'oracle `reference/hgp11_ref/supports.py` du worktree L0 a été chargé en lecture seule, comme le fait `ref_mutants`,
sans bytecode : aucun `__pycache__` n'a été créé dans L0. On compare sa sortie `canonical(k)` à la sonde native
`--all`, filtrée à $W_K=\lbrace p+m\geq K\rbrace$. Les boules sont appariées par (niveau, ensemble des supports en
coordonnées). Sont comparés : $(p,m,q)$, les cinq comptes, et les incidences par support réordonnées, puisque L0 range
les supports par coordonnées et le natif par `SiteIdx`.

| Sonde | Entrées | Boules de $W_K$ comparées | Écarts |
| --- | --- | ---: | ---: |
| u21 | fixtures et 40 nuages (n de 7 à 11) | 1 595 | 0 |
| u18 | fixtures et 20 nuages | 991 | 0 |

C'est un aperçu du futur `mhgp11_supports_fraction`. Les noms (`kparties_reliees`, `cofaces_support`,
`gabriel_cofaces_support`) et les définitions concordent déjà.

## 5. Mutants

### 5.1 Campagne du manifeste

Commande : `run_mutants.py --jobs 2 --build-jobs 1`, sur la copie figée, de 22 h 05 à 22 h 13 UTC. Le témoin est
vert, et chaque mutant est tué par code par la porte annoncée :

| Mutant | Porte qui le tue |
| --- | --- |
| `triangle_droit_accepte` | `unit_right_triangle` |
| `drapeau_q4_presentation` | `unit_cube` |
| `arret_premier_support` | `unit_square` |
| `fermeture_omise` | `unit_square` |
| `cofaces_ordre_k` | `unit_square` |
| `coquille_sans_plafond` | `shell_capacity` |

Ligne rendue : `mutants_ok module=supports mutants=6 tues=6 dont_signal=0 dont_delai=0 dont_construction=0 plancher=6`.
Le compte rendu est dans `verif_s6/mutants_supports_report.json`.

La table des raisons a changé. Les deux mutants de `tests/mutants/core.json` qui la visent ont donc été rejoués
(`statut_d_une_raison` et `ordre_de_la_table`, par `--only`) : ils sont **tués** par
`mhgp11_core_unit_reasons` avec la table à 27 raisons (`verif_s6/mutants_core_reasons_report.json`).

### 5.2 Mutants de mon cru

Chaque mutant est appliqué à une copie, `mysrc`, puis reconstruit (u21, `-j2`). On joue ensuite les portes `fast` du
module, sans les jumelles `-O`, et mon contre-juge sur les fixtures et 12 nuages. Pour finir, la copie a été restaurée
et retrouvée identique par `diff -r`.

| Mutant | Faute injectée | Portes en échec | Contre-juge |
| --- | --- | --- | --- |
| `zeta_cinq_dimensions_basses` | fermeture intra-mot sur 5 dimensions au lieu de 6 | 5 : `cube`, `octahedron`, `mixed`, `shell_bound`, `judge_small` | 40 écarts |
| `poids_six_oublie` | décompte sans le poids 6 (`r < 6`) | 5 : idem | 15 écarts |
| `gabriel_a_t` | `gabriel_cofaces` $=N_t$ au lieu de $N_{t+1}$ | 9 | 1 564 écarts |
| `capacite_decalee` | garde de capacité `count > out.size()` | 1 : `unit_refusals` | 0 (tampons larges) |
| `centre_d_un_cote` | triangle admis si le centre est d'un côté (`<= 0`) | 4 | 231 écarts |
| `raccourci_regulier_large` | coquilles $m=q+1$ traitées comme régulières | 3 : `mixed`, `right_triangle`, `judge_small` | 20 écarts |
| `triplets_k_decroissant` | triplets énumérés à $k$ décroissant (ordre publié faux) | 1 : `unit_shell_bound` | 0 |
| `strict_traces_sans_fermeture` | $N_t$ retiré sauf si $t=q$ | 5 | 68 écarts |
| `plafond_vingt_cinq` | plafond décalé à 25 | 2 : `shell_bound`, `refusals` | sans objet |
| `cofaces_sans_interieurs` | $\binom{p}{K+1-j}$ remplacé par un indicateur | 2 : `unit_lines`, `judge_small` | 12 écarts |
| **`premier_support_non_controle`** | contrôle « premier support $=S^*$ » retiré (`enumerate.cpp:190`) | **0** | 0 |
| **`niveau_non_controle`** | contrôle « niveau de la sphère de $S^*$ = niveau du catalogue » retiré (`enumerate.cpp:143`) | **0** | 0 |

Deux mutants appellent une remarque :

- `triplets_k_decroissant` n'est tué que par la fixture `cercle24` : ni les 40 petits nuages de `judge_small`, ni mes
  nuages n'ont deux triangles de même préfixe $(i,j)$. Ce mutant est tué, mais par une seule porte.
- Les **deux survivants sont un trou réel mais inévitable**, et c'est le point de ce paragraphe. Ce sont deux gardes
  d'invariant du catalogue, que la spécification exige au § 7.3, points 2 et 4.
  - Ces gardes ne peuvent se déclencher que sur un domaine corrompu.
  - `FullDomain` n'est constructible que par `prepare_full_domain`.
  - La règle 6 d'`ARCHITECTURE.md` interdit tout crochet d'injection dans le produit.
  - Aucune porte ne peut donc les atteindre. Il en va de même de `shell.size() != m` (`enumerate.cpp:175`), de
    `site_point` hors du nuage (`:125`), de $S^*$ dégénéré (`:142`) et des champs `qmin` et `m` (`:212`).

  Le sens inverse est bien jugé : aucun refus parasite. Ces gardes ne se déclenchent jamais sur toute $W_5$ des trois
  trames, sur la grille cosphérique (125 064 coquilles étendues) ni sur les petits nuages.

  Ce qu'il faut faire : les déclarer comme gardes sans porte (§ 7, F3) et ne pas les compter dans les mutants.

## 6. Revue du code : constats

**Exactitude.** Ce qui a été vérifié :

- **`ball_supports`.**
  - Ordre des refus : `parameter_out_of_range`, puis champs incohérents, puis le raccourci régulier, puis
    `check_shell`. Viennent ensuite les tampons : le brouillon et la taille de la coquille avant toute écriture, la
    capacité de `out` au fil de l'énumération, comme le documente l'en-tête.
  - Coquille régulière : $\lbrace S^*\rbrace$ et $N_j=[j=q]$. C'est juste : $m=q$ et $S^*\subseteq U_b$ donnent
    $U_b=S^*$.
  - Coquille étendue : sphère `Sphere::through` de l'arité $q$, puis égalité exacte du niveau.
  - Les trois boucles et leurs prédicats sont identiques à la source.
  - Les masques sont écrits au fil de l'énumération, et la zêta en OU est faite sur exactement `closure_words(m)`
    mots.
  - Le décompte par poids est juste pour $m\leq 6$ : aucun bit au-delà de $2^m$.
- **`ball_counts`.** La validation de la fermeture et les cinq formules sont vérifiées : pas de soustraction négative
  (`strict_traces` : $N_t\leq\binom{m}{t}$, et $t>m$ donne 0 des deux côtés), garde `u64` des cofaces.
- **Binômes.** `binomial` n'indexe jamais en négatif ($y<0$, $y>x$ ou $x>35$ donnent 0). Toutes les lectures restent
  sous les `static_assert` : bas $\leq 13$, ou haut $\leq 24$, où le maximum est $\binom{24}{12}$.
- **Les deux cas hors domaine.**
  - Une boule de $\mathrm{Cat}_K$ hors de $W_K$ rend des comptes nuls, sauf `compressed_parts` et `kparties_reliees`,
    eux aussi nuls quand $t>m$.
  - Une boule hors de $\mathrm{Cat}_K$ est refusée par `make_shape`.

**Budget et ressources.**

- Le module n'alloue rien : `out` et `scratch` appartiennent à l'appelant.
- La pile est bornée : 24 points, plus 4 pour $S^*$.
- Sans `check_shell`, `closure_words` (fonction totale) et la garde `words == 0` empêchent encore tout débordement du
  tableau de 24 points. C'est une défense en profondeur, et c'est ce qui rend le mutant `coquille_sans_plafond`
  causal.
- Le brouillon atteint 2 Mio à $m=24$ ; un `static_assert` le garantit.

**Concurrence et déterminisme.**

- Le module n'a pas d'état global mutable. `num` n'en a pas non plus : pas de `static` mutable, de `thread_local`
  ni d'atomique.
- La porte `concurrency` lance 4 fils de 50 appels sur la coquille mixte, et ses contrôles sont faits après la
  jointure. Elle est verte en u21 et sous ASan.
- La sortie ne dépend que du domaine : la permutation et le réétiquetage du cube jusqu'à `0xFFFFFFF7` sont gravés.
- TSan n'a pas été joué : c'est pour G4.

**Portes, planchers, vacuité.**

- Les planchers unitaires égalent les contrôles mesurés.
- Les juges ont des planchers, plus une ligne de couverture exacte.
- Les registres ont une ligne de verdict exacte, qui fixe $\lvert\mathrm{Cat}_K\rvert$, $\lvert W_K\rvert$, les
  coquilles étendues, les supports, les boules multiples et l'empreinte de l'entrée.
- Le manifeste a un plancher de 6.
- Le juge rejette une sonde corrompue (essai de l'implémenteur et le mien, § 3).
- Points faibles, non bloquants :
  - les portes K10 n'ont ni ligne ni plancher (N2) ;
  - la porte `shell_capacity` ne vérifie pas l'absence de ligne de boule (N3).

**Sorties existantes.**

- Aucune source de `tower`, du catalogue, des bancs ni d'`io` ne change.
- Les deux raisons sont ajoutées **en fin** de `reasons.def`. Les valeurs des raisons existantes et leur priorité
  (`Outcome::precedes`) sont donc inchangées.
- Aucun format ne sérialise le numéro d'une raison (vérifié par `grep` dans `src`, `bench` et `cli`).
- Pour cette tranche, je n'ai pas refait l'identité à l'octet de `mhgp11_full_bench` : rien de ce qu'il exécute n'a
  changé.

**Style.**

- `mhgp11_style` et `mhgp11_support_check_style` sont verts.
- `[dependance]` admet l'inclusion transitive de `num/num.hpp` par `supports.hpp`, puisque `supports` dépend de
  `tower`, qui dépend de `catalogue`, qui dépend de `num`.
- `tools/check_docs.py`, joué en lecture seule à la racine du worktree, ne relève **rien** sur les fichiers de la
  tranche. Ses autres écarts (liens morts vers `receipts/`, absent du checkout partiel) sont étrangers à S6a.

## 7. À corriger (non bloquant, documentation)

- **F1 — `src/supports/supports.hpp:61-72`.** Le contrat de `ball_supports` dit « appels concurrents permis sur des
  tampons distincts », mais ne dit rien du registre facultatif. Un `SupportLedger*` partagé entre fils serait une
  course : `add()` n'est pas atomique.
  - À ajouter : « un registre par fil, sommé après la jointure ».
  - L'assemblage S6b en aura besoin.
- **F2 — `src/supports/supports.hpp:74-76`.** La documentation de `ball_shape` ne cite que `parameter_out_of_range`.
  Or la fonction rend aussi `support_shell_capacity` ($m>24$) et `supports_invariant` (forme hors de
  $\mathrm{Cat}_K$), tous deux par `make_shape`. La porte `unit_shell_bound` joue d'ailleurs le premier de ces refus.
  Il faut compléter la liste.
- **F3 — `src/supports/enumerate.cpp:125,142-143,175,190,212`, avec `PROVENANCE.md`, section S6a.** Il faut déclarer
  ces contrôles comme gardes d'invariant du catalogue **sans porte possible**, au même titre que les gardes du
  catalogue lui-même. Le commentaire de tête ou la ligne `PROVENANCE` doivent dire que les mutants qui les retirent
  survivent par construction (§ 5.2), pour que le trou soit écrit et non découvert.

## 8. Notes

- **N1 (S6b) : I6 à l'échelle.** La sonde `--window` ne contrôle pas I6 boule par boule. I6 demande
  $\max_Q\mathrm{cofaces}(Q)\leq\mathrm{cofaces}(b)\leq\sum_Q\mathrm{cofaces}(Q)$, plus les cas réguliers du lemme G.
  Ce contrôle est gratuit sur toute $W_K$, et la spécification le range dans `mhgp11_supports_scale*` (§ 8.4 et 9.1).
  Il est à ajouter avec l'assemblage, en même temps que la contre-épreuve $S_{\mathrm{journal}}=\binom{m}{t}-N_t$ par
  boule, qui attend le journal de S3.
- **N2 : portes K10.** Les portes `mhgp11_supports_registers_lidar_ng0{0,1,2}_k10` (`tests.cmake:80-85`) n'ont ni
  ligne ni plancher.
  - Une vacuité réelle est presque impossible : les six registres de `build_forest` sont calculés indépendamment et
    ne peuvent valoir zéro que si le catalogue est vide.
  - Il faut toutefois graver la ligne de verdict après G4, comme l'implémenteur le prévoit.
  - À défaut, la porte devrait transmettre un `--min-balls` à la sonde en mode `--registers`, ce qu'elle ne fait pas
    aujourd'hui.
- **N3 : porte `shell_capacity`.** Elle exige le code 2 et la ligne de refus, mais pas l'absence de ligne de boule
  (« aucun résultat » du § 9.1). La sonde la garantit par sa mise en tampon, et je l'ai constatée à la main. La porte
  de transaction du CLI (S7) devra l'exiger sur le dossier.
- **N4 : coquille régulière.** Ce chemin ne valide pas les `SiteIdx` de $S^*$ ; le chemin étendu les valide par
  `site_point`. Cette asymétrie est voulue : le catalogue fait foi (G2), et le coût est de 16 ns par boule. Elle est à
  mentionner dans `SORTIES.md` (§ 9).
- **N5 : `Closure`.** Son constructeur par défaut, public, construit une fermeture vide ($m=0$), que `ball_counts`
  refuse (porte `refusals`). La phrase « Closure ne se construit que contrôlée » (rapport, `PROVENANCE.md`) est donc
  un peu forte : il faudrait écrire « vide ou contrôlée ».
- **N6 : ordre des raisons.** L'ordre final de `support_shell_capacity`, `supports_invariant` et
  `environment_selftest` (S5) se décide à l'intégration, dans l'ordre des livraisons. Il faudra alors mettre à jour
  `status_test.cpp`, qui passe à 28 raisons.
- **N7 : `PROVENANCE.md:229`.** `mark_supports (l. 135–164)` : la fonction se ferme ligne 165. C'est un détail sans
  effet sur l'épingle.
- **N8 : fichiers différés.** `supports_oracle.py` et `scale.py`, prévus au § 9.1, sont reportés à S6b, comme
  `hierarchy.cpp`, `Ball` et les mutants `boules_propres_decroissantes` et `supports_tri_pointid`. Cela respecte la
  consigne.

## 9. Ce qu'il faudra ajouter aux documents de L0

Je reprends la liste de l'implémenteur et j'y souscris, avec ces ajouts.

**`docs/SORTIES.md`, comptes dérivés.**

- Les noms C++ : `ball_counts` (rend `BallCounts`), `support_cofaces`, `support_gabriel_cofaces`.
- $N_j$ est rendu par l'API comme `Closure`, et n'est jamais stocké.

**`docs/SORTIES.md`, raisons.**

- `support_shell_capacity` vient de `check_shell`, seul contrôle du plafond. Il est appelé par `ball_supports`,
  `make_shape` et la pré-passe de l'appel entier.
- `supports_invariant` couvre ce qu'énumère l'implémenteur, plus le cas « plus de `out.size()` supports ».
- `ball_supports` rend aussi `parameter_out_of_range` pour un `BallIdx` hors du catalogue.
- Le registre `SupportLedger` est un registre par fil (F1).

**Ordre des supports.** L'ordre natif est (arité, `SiteIdx`, donc rang de Morton). La sortie canonique de l'oracle S1
range par (arité, coordonnées). Le différentiel `mhgp11_supports_fraction` doit réordonner ; c'est fait dans
`verif_s6/l0diff.py`, à reprendre.

**`MATHEMATIQUES.md` § 10.6–10.7.** Rien à changer : les lemmes F et G sont confirmés par la définition (§ 3). On peut
citer la contre-épreuve sommée par les six registres, et son recoupement avec `c40_paired` (§ 2).

## 10. Pour la session G4

- La matrice de l'implémenteur, à laquelle s'ajoute u18, déjà vert en local sur les 19 portes `fast`.
- **TSan** sur `mhgp11_supports_unit_concurrency` et sur la sonde ; puis la campagne `mhgp11_mutants_supports` et le
  rejeu complet de `mhgp11_mutants_core`.
- **Graver les lignes K10** (N2), puis relever les registres $W_{10}$ des trames.
- Vérifier `sample_judge.py` sous Python 3.10 nu : les lignes doivent être identiques à celles rendues localement sous
  3.12.
- Aucune mesure de temps locale n'est une revendication. Les temps de ce rapport ne sont que des durées de portes.

## Annexe : fichiers de la contre-lecture

Dossier `build/v11-persist/sortie_supports/verif_s6/` ; sha256 au moment de l'écriture.

| Fichier | Contenu | sha256 |
| --- | --- | --- |
| `myjudge.py` | contre-juge Fraction par la définition et la MEB brute | `c31ac502c46bad64d71f78d555e028ee16a8df9563207f65187807e51b28afe3` |
| `l0diff.py` | différentiel de lecture contre l'oracle S1 de L0 | `9fc2c0c106f09d62351c633da8c2a8cc0f115374ca66db3182e7e82b45f35b77` |
| `mymutants.json`, `mymutants_run.py` | mes 12 mutants et leur lanceur | `bbf51d45…`, `a80cc8f9…` |
| `mymutants_results.json` | portes en échec et contre-juge, par mutant | `09c94571…` |
| `mutants_supports_report.json` | compte rendu de `run_mutants.py` (manifeste `supports`) | `7ed1c017…` |
| `mutants_core_reasons_report.json` | les deux mutants de la table des raisons | `f758c356…` |

Constructions et journaux, susceptibles d'être effacés au redémarrage : `/tmp/v11-s6-verif/`. On y trouve
`build-u21`, `build-asan`, `build-u24`, `build-u18`, `build-my` et les journaux `fast-*.log`, `scale-u21.log`,
`mutants-campaign.log` et `mymutants.log`.

Commandes du contre-juge :

```text
python3 -S -B myjudge.py <sonde> <dossier> <graine> <nuages>
PYTHONDONTWRITEBYTECODE=1 python3 -S -B l0diff.py <sonde> <dossier> <nuages>
```
