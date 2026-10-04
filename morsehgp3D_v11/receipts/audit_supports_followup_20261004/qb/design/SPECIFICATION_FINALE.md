# Spécification finale — sortie paramétrée de `mhgp11` et hiérarchie des supports d'ordre K

4 octobre 2026, 19 h 10 UTC (heure lue par `date -u`). Rôle : arbitre du workflow de conception. Lecture seule :
rien construit, modifié ni commité ; seule écriture, ce fichier. **GCP non utilisé.**

```text
phase=exploration_v11_hors_registre
backend=cpu_reference
profile=quantized_u21_input_only   (défaut de compilation, README.md:60 ; u18 et u24 qualifiés à part)
public_status=not_claimed
```

**Sources.** Moteur : worktree `build/v11-claude-20261003/morsehgp3D_v11`, HEAD `57dd21be1`. Audits et reçus :
`main` à `1bf4be68f` (« audit planned support hierarchy… », qui juge déjà les lectures de ce workflow). Sauf
mention, les chemins `src/…`, `docs/…`, `bench/…`, `tests/…`, `reference/…`, `cmake/…`, `tools/…`, `cli/…`,
`CMakeLists.txt` et `README.md` sont relatifs à `morsehgp3D_v11/`. Les chemins `Zoltan/…`, `morsehgp3d/…`,
`docs/references/…`, `docs/math/…` et `build/…` sont relatifs à la racine du dépôt. Les numéros de ligne sont ceux de
`57dd21be1` ; ils corrigent ceux des lectures, prises à `7df199f73`, là où ils diffèrent (§ 1.4). La thèse est citée
par ses numéros de page imprimés (`docs/references/MANUSCRIT_THESE_HAUSEUX.pdf`).

---

## 0. En bref

1. **Un exécutable, un paramètre.** `mhgp11 --sortie=full|supports|points|plat --points=… --ids=… --dossier=…
   --k=K [--fils=W] [--budget=…]`, au-dessus d'une façade C++ `src/api/api.hpp` (`Session`, `compute`, `publish`).
   La cible CMake s'appelle `mhgp11_cli`, avec `OUTPUT_NAME mhgp11`, car `mhgp11` est déjà la bibliothèque
   (`CMakeLists.txt:245`). Une valeur de `--sortie` n'est acceptée qu'une fois livrée avec ses portes
   (`docs/ARCHITECTURE.md:21-22`) : `full` d'abord (tranche S5), `supports` ensuite (S7), puis `points` (S9) et
   `plat` (S10).
2. **L'objet « supports ».** Il s'agit de l'arbre $T_K$, c'est-à-dire la forêt d'ordre K de FULL, muni de la
   **partition** des boules d'événement d'ordre K, $W_K=\lbrace b\in\mathrm{Cat}_K : p+m\geq K\rbrace$, sur ses nœuds.
   - Chaque boule porte son niveau $\lambda_b$, son rôle (naissance, fusion ou interne), ses branches réunies, **tous**
     ses supports positifs minimaux $\mathcal{Q}_b$ et des comptes exacts, nommés selon l'audit de conception.
   - Le polyèdre $P_v$ est l'union sur le sous-arbre de $v$, donc $P_{\mathrm{enfant}}\subseteq P_{\mathrm{parent}}$.
   - Les lemmes A à G (§ 2) le démontrent à partir de M1–M2 et T1–T5 (`docs/MATHEMATIQUES.md`).
3. **Arbre K seul, dès la première livraison.** Cela suit le souhait de l'utilisateur. `build_order` construit
   l'ordre K seul, sans ordres 1..K−1 ni verticales, par la machinerie **existante et qualifiée** :
   `ForestBuilder` sur la voie par lots, la même que la boucle non concurrente de `build_full`
   (`src/tower/forest_vertical.cpp:298-327,334-352`). Aucun code chaud n'est réécrit ; l'identité avec
   `build_full(...).order(K)` est gardée par une porte. La voie pipeline à un seul ordre, plus rapide, est une
   tranche de performance mesurée (S11).
4. **Rattachement exact sans aucune descente supplémentaire (E1).**
   - Un **journal des graines** est tenu aux deux seuls points d'application des cellules,
     `ForestBuilder::cell` et `ForestBuilder::regular_cell` (`src/tower/forest_plateau.cpp:39-67,100-120`). Les
     trois voies les appellent dans l'ordre croissant des `BallIdx`.
   - **Un seul balayage** `ClosedAncestorSweep` à la coupe ouverte $r_b-1$ donne les branches touchées
     $\mathrm{ant}(b)$, dédupliquées à la coupe stricte comme l'exige l'audit. Il donne aussi $\mathrm{att}(b)$ par la
     règle du parent (lemme D).
   - La descente-puis-ancêtre de `ball_nodes` (E2, `bench/points_export.cpp:178-214`) reste un **juge de test**.
5. **$\mathcal{Q}_b$ exact.** On compose les trois prédicats stricts publics de `num`, sans test de minimalité
   (Carathéodory strict). L'énumération se fait sur **toute** la coquille $U_b$, jamais limitée à $q_{\min}$.
   - Une coquille régulière donne $\lbrace S^*\rbrace$ sans calcul.
   - Le plafond de coquille étendue est 24, constante de compilation, avec refus explicite au-delà.
   - Contre-épreuve gratuite par boule : $S_{\mathrm{journal}}(b)=\binom{m}{t}-N_t(b)$.
6. **Comptes publiés** (lemme G, noms de l'audit `1bf4be68f`).
   - Par boule : `k_parts` $=\binom{p+m}{K}$, `strict_traces`, `components` (branches) et `cofaces` (liaisons
     distinctes).
   - Par support : `cofaces` $=\binom{p+m-\lvert Q\rvert}{K+1-\lvert Q\rvert}$, les liaisons au sens de la thèse
     (Prop. 5, p. 86).
   - Les parties comprimées $\binom{m}{t}$ et les comptes de Gabriel se dérivent dans le lecteur.
7. **Formats.**
   - `MHGP11FUL1` est inchangé à l'octet (`bench/full_probe.cpp:40-75`).
   - `MHGP11SP` v1 :
     - nœuds dans la **numérotation canonique** de la forêt, la même que FULL et que points ;
     - boules triées par postordre de leur nœud, si bien que $P_v$ et chacun de ses instantanés datés sont des
       **tranches contiguës** ;
     - sites des supports publiés comme **lignes** de la table `SITES` (rang de Morton), pas comme `PointId` ;
     - aucune table de niveaux : chaque rang a un témoin géométrique dans le fichier.
   - `MHGP11PT` et `MHGP11ET` pour points et plat.
   - Un manifeste JSON écrit en dernier, une empreinte `tree_k_sha256` commune aux quatre sorties.
8. **Transaction de dossier.**
   - Tout est écrit dans `DOSSIER.pending/`, manifeste en dernier.
   - Publication par **un seul renommage sans remplacement** (`renameat2(RENAME_NOREPLACE)`) : un lecteur voit tout ou
     rien, ce qui est plus fort que `docs/ARCHITECTURE.md:176-180`.
   - `io` est un **port explicite** du lecteur `u32le` et de l'`OutputSet` durcis par le raccord R2 de la v10, port
     déjà annoncé par `docs/PROVENANCE.md:42`.
9. **Points et plat natifs, aucun flottant décisionnel (F1).**
   - Une **table d'encadrements entiers des racines par rang**, $\lfloor 2^{64}\sqrt{\ell_r}\rfloor$ en `u128`,
     décide presque tout.
   - En cas de chevauchement, on bascule sur le repli exact, port de `bench/points_radius.py` et
     `bench/points_flat.py` : classes de carrés, Besicovitch, encadrements jusqu'à 8 192 bits, puis refus
     `radical_sign_budget`.
   - Aucun amendement F7 n'est nécessaire.
   - Convention $m=1$ à $K=1$ (`docs/HIERARCHIE_POINTS.md:95-96`) et $\kappa=1$.
10. **Douze tranches.** S0 (contrat) précède tout. Ensuite :
    - l'oracle borné (S1) précède tout code natif de supports ;
    - `io` (S4) et `num` (S8) avancent en parallèle ;
    - le CLI arrive tôt, avec la sortie `full` (S5).
    Les sondes de banc et leurs portes ne changent pas. Aucun benchmark ne promeut un statut.

---

## 1. Arbitrage des deux conceptions

### 1.1 Base retenue et greffes

**Base retenue.** C'est la conception « exactitude » : contrat d'abord, oracle borné avant le natif, aucun code chaud
modifié au départ, arithmétique entière seule. On y greffe les meilleures idées de la conception « produit » :
journal des graines, antécédents, postordre, CLI nommé.

| Axe | Exactitude | Produit | Retenu, et pourquoi |
| --- | --- | --- | --- |
| Objet $W_K$, coupe fermée | lemmes S1–S3 | lemmes A–B | Identiques. Vérifiés dans `forest_plateau.cpp:39-98`. |
| Rôles | naissance, fusion, interne, par les rangs | idem, plus les antécédents | **Produit.** Le nombre de branches est intrinsèque, dédupliqué à la coupe stricte. Les unions DSU, elles, dépendent de l'ordre (2, 2, 1, 0 sur le témoin K5, `receipts/audit_supports_20261004/plateau/README.md`). |
| Calcul de l'arbre K | `build_full` d'abord, ordre K seul en S10 | pipeline à un ordre dès T2 | **Hybride.** L'ordre K seul sort dès S3, par la voie non concurrente existante. Le pipeline à un ordre (S11) est mesuré. |
| Rattachement | E2 (descentes) dans le produit, E1 plus tard | E1 (journal) dans le produit | **E1 dans le produit, E2 comme juge.** Zéro descente, $\mathrm{ant}(b)$ exact, « toutes les traces disponibles à la fermeture du plateau » (`qb/README.md`). |
| Balayage | `advance(r)` | `advance(r-1)` puis `advance(r)` | **Une passe** : `advance(r-1)` puis règle du parent (lemme D). L'avance reste monotone (`forest_ancestor_sweep.hpp:40-42`). |
| $\mathcal{Q}_b$ | trois prédicats, plafond 24, `is_positive_support` dans `num` | idem, `--coquille-max` | **Composition dans `supports`** : `num` reste intact. **Plafond constant** : une option sans ablation est interdite (`ARCHITECTURE.md:21-22`). |
| Comptes | kp, S, L(b), L(Q) | ℓ, g, composantes | **Union des deux**, sous les noms fixés par l'audit (`AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md:34-45`). |
| Module | `supports` après `tower` | idem | Commun. |
| `io` | primitives neuves | `OutputSet` multi-fichiers | **Port du raccord R2** (`u32le_input.hpp`, `cli_output.hpp`, `PROVENANCE.md:42`), plus la transaction de dossier. |
| Transaction | renommage du dossier | fichiers puis manifeste | **Dossier**, avec `RENAME_NOREPLACE` (§ 6.7). |
| CLI | positionnel | nommé | **Nommé** (`--clé=valeur`) : options en français, clés JSON en anglais comme les bancs. |
| Format SUPPORTS | `MHGP11SH`, NodeIdx, `LEVELS`, PointId | `MHGP11SP`, postordre, sans `LEVELS`, PointId | **`MHGP11SP`** : NodeIdx canonique, boules en postordre (tranches contiguës), sites en lignes, sans `LEVELS`. |
| Arithmétique points et plat | `Big` fixe de 8 192 + 256 bits, encadrements entiers | `Big` et règle flottante F7 | **Entiers seuls**, avec table d'encadrements par rang (`u128`). Capacité de `Big` corrigée (§ 1.4). |
| Convention $K=1$ (points) | question | $m=2$ | **$m=1$** : décision documentée (`HIERARCHIE_POINTS.md:95-96`, commit `6c88fe0ed`). |
| Défauts de la sortie plate | question | z = 1, mcs 20, EOM | **z = 1, mcs 20, EOM**, ligne LiDAR publiée (`SORTIE_PLATE.md:147-150`). |

### 1.2 Apports de l'arbitrage, absents des deux conceptions

- **Audit de conception `1bf4be68f`.** Ses gardes deviennent des exigences et des fixtures :
  - $\mathcal{Q}_b$ s'énumère depuis toute $U_b$ (cube) ;
  - les événements faibles $p+q-1\leq K$ sont gardés (triangle équilatéral, K2) ;
  - `compressed_parts` $\neq$ K-parties ;
  - les cofaces sont des incidences ;
  - les unions effectuées ne sont pas une multiplicité ;
  - le support porteur n'est pas stable ;
  - dates conservées (`receipts/audit_supports_20261004/*/README.md`).
- **Port `io` depuis le raccord R2.**
  - `build/v10-integration-r2/src/morsehgp3D_v10/src/core/cli_output.hpp` (448 lignes) : conflits par chemin résolu
    et par inode, écritures contrôlées, temporaires, refus sans publication.
  - `src/cloud/u32le_input.hpp` (95 lignes).
  - Ni l'une ni l'autre conception ne les citait.
- **Sites des supports en lignes de `SITES`.** Un réétiquetage des `PointId` ne change plus qu'**une colonne** du
  fichier (`SITES.point_id`) ; tout le reste est identique à l'octet.
- **Numérotation canonique et décalages en postordre.** Les tranches contiguës de la conception « produit » sont
  conservées, sans perdre l'identité des nœuds avec FULL et points.
- **Table d'encadrements des racines.** Un filtre entier `u128` qui respecte F1 remplace le filtre flottant F7.
- **Fixture « cellule passagère » explicite** (§ 2.9, n° 4). La conception « exactitude » n'en trouvait pas à la main.
- **Six contre-épreuves de registres.** Elles comparent le rattachement aux compteurs que la forêt calcule par une autre
  voie : `trace_resolutions`, `cells.combinations`, `replayed_cells`, `plateaus`, `touched_components`,
  `continuations` (§ 8.4, I4).

### 1.3 Affirmations décisives revérifiées dans les sources

| Affirmation | Vérification |
| --- | --- |
| Fenêtre et `kinds` | `src/tower/forest_build.cpp:150` (fenêtre), `:151-166` ; codage dans `src/tower/forest_internal.hpp:146`. |
| Seuls `cell` et `regular_cell` appliquent une cellule | Voie sérielle : `forest_plateau.cpp:122-138`, toutes les cellules par `cell`. Voie par lots : `forest_parallel.cpp:287-309`, `flush` en `:252-285`. Pipeline : `forest_concurrent.cpp:27-56`. Dans les trois, l'ordre est celui des `BallIdx` croissants. |
| Graines = naissances de rang $<r_b$ | Contrôle en `forest_plateau.cpp:56-57` et `:109`. |
| Fils d'une fusion = anciens `top` touchés | `forest_plateau.cpp:69-98`. Le comptage par `touch` (`:17-24`) vaut une fois par ancienne composante et par plateau. |
| `representative` est perdu dans le pipeline | Conditionné à `vertical_seeds` (`forest_plateau.cpp:58`) ; le pipeline le met à nul (`forest_pipeline.cpp:235`). |
| `RegularVerticalSeeds` ne sert pas | Les jonctions de l'ordre K = kmax ne sont pas retenues (`regular_vertical_seeds.hpp:32`). |
| Balayage monotone | `forest_ancestor_sweep.hpp:40-42`. La requête exige rang(graine) ≤ niveau (`:60-62`). |
| `build_forest` sur un seul ordre existe et est testé | `forest.hpp:131-137`, `forest_build.cpp:414-421`. Comparé à `build_full` dans `full_tables` (`tests/tower/regular_classification_test.cpp:118-138`). |
| Restrictions du pipeline | `forest_pipeline.cpp:169` (K<2), `:178` ; `forest_build.cpp:372` et `:352` ; `population_lookup.hpp:55-59`. |
| `build_cell` matérialise **toutes** les traces strictes | `cells.hpp:1` et `:83-91` ; `cells.cpp:88-145`. Donc $\sum S=$ `trace_resolutions`. |
| Ordre des graines régulières = ordre de `build_cell` | `forest_parallel.cpp:47-58` et `cells.cpp:117-128`. |
| Masque des sondes | 16 379 = paramètres de `points_export.cpp:376-383` ; masque décodé en `full_probe.cpp:365-388`. |
| Cycle d'inclusion | `descent_memo.hpp:3`, `descent.hpp:4`, `cells.hpp:4`. Quatre inclusions internes de `tower/tower.hpp` : `cells.hpp:4`, `locate.hpp:4`, `census_slots.hpp:3`, `meb.cpp:2`. |
| Règle `[inclusion]` | `tools/check_style.py:391-404`. |
| Raisons d'`io` | `src/core/reasons.def:24,33,35,37`. |
| Prédicats | `src/num/geometry.hpp:155-169` ; `src/num/predicates.cpp:183-240,305-309,322-338`. |
| Coquille complète | G2 (`MATHEMATIQUES.md:102-106`) ; `catalogue.hpp:161-167`. |

### 1.4 Errata

| Où | Écrit | Correct |
| --- | --- | --- |
| Lecture `foret_k` | `kparts = C(m,K-p)`, « nouveaux sommets = C(m,K-p) − S » | $\binom{m}{t}$ compte les parties **comprimées** (qui contiennent tout $I_b$). Toutes les K-parties : $\binom{p+m}{K}$. Les nouveaux sommets ne s'en déduisent pas (témoin K5 : $C-S=0$, mais trois nouveaux sommets ; `plateau/README.md`). |
| Conception « exactitude », § 6.9 | `Big` à 8 192 + 256 bits (132 mots) | L'encadrement à $P$ bits fait $\mathrm{isqrt}(nd\cdot 2^{2P})$ (`bench/points_radius.py:52-57`). À $P=8192$, il faut une capacité d'au moins $2P+\mathrm{bits}(nd)$, d'où le choix de $16384+1024$ bits (§ 7.8). |
| Conception « produit », § 3.4 et § 5 | option `--coquille-max` | Retirée : une option sans ablation est interdite (`ARCHITECTURE.md:21-22`). |
| Conception « produit », § 6.6 | `moteur.source` dans le manifeste | Retiré : aucune identité de source n'existe dans le binaire. Le reçu épingle binaires et sources. |
| Les deux conceptions | supports publiés en `PointId` | Publiés en lignes de `SITES` (§ 6.3). |
| Conception « produit », T2 | pipeline à un ordre dès la première livraison | Reporté en S11. La première livraison passe par la voie non concurrente existante. |
| Lecture `plat` | `bench/points_flat.py:881` | `:882` (`out[pt.ids] = native` suppose des `PointId` denses). |
| Lecture `points` | « m = k+1 figé » | `points_flat_gate.py:117` et `points_flat_campaign.py:164` passent bien $k+1$ **même à k = 1**. Mais le contrat (`HIERARCHIE_POINTS.md:95-96`) et la décision `6c88fe0ed` fixent $m=1$ à $k=1$. Divergence à aligner en S9. |
| Les deux conceptions | « cellule passagère : aucun exemple » | Fixture n° 4 du § 2.9. |

---

## 2. Objet mathématique

### 2.1 L'arbre d'ordre K

**Définition.** $T_K$ est la forêt d'ordre $K$ de FULL, c'est-à-dire l'arbre de fusion des composantes de
$\Gamma_K(a)$, en bijection avec celles de $L_K(a)$ par T1 (`MATHEMATIQUES.md:137-163`). Dans le code, c'est
`OrderForest` (`src/tower/forest.hpp:78-129`).

**Numérotation des nœuds.**
- Les naissances $[0,b)$ viennent d'abord, triées par (rang, centre exact) (`forest_build.cpp:72-112`). À $K=1$, ce
  sont les sites, en ordre $xyz$ (`:59-69`).
- Les fusions N-aires suivent, créées plateau par plateau (`forest_plateau.cpp:69-98`).
- La racine est unique (`forest_build.cpp:405-412`).
- Un enfant a toujours un indice inférieur à celui de son parent (`forest_ancestor_sweep.hpp:47-49`), et un rang
  strictement inférieur (`forest_plateau.cpp:85-86`).

**Vie d'un nœud.** Le nœud $v$ est vivant à la coupe fermée $a$ si et seulement si
$a_v\leq a<a_{\mathrm{parent}(v)}$ (`MATHEMATIQUES.md:240-241`).

**Niveaux.** Ce sont des rayons carrés rationnels exacts, désignés par des rangs denses `LevelRank`
(`ARCHITECTURE.md:74-75,186-191`).

### 2.2 Boules d'événement

$$W_K=\lbrace b\in\mathrm{Cat}_K : p+q-1\leq K\leq p+m\rbrace=\lbrace b\in\mathrm{Cat}_K : p+m\geq K\rbrace$$

La seconde égalité vient de $\mathrm{Cat}_K=\lbrace p+q\leq K+1\rbrace$ (`MATHEMATIQUES.md:80-86`). La fenêtre
découle de T3 (`:188-198`) et elle est codée en `forest_build.cpp:150`.

$W_K$ se partage en deux :
- les **naissances** (`kinds` = 1) : $\lvert P_b\rvert=K$, ou bien aucune $K$-partie stricte ;
- les **cellules** (`kinds` = 2) : au moins une trace stricte. Dans ce cas $\lvert P_b\rvert\geq K+1$, car $t=m$
  donne une naissance (`cells.cpp:116`).

**Périmètre.** Une boule hors de $W_K$ ne change pas $\Gamma_K$.
- Si $p+m<K$, elle n'a aucune $K$-partie.
- Si $p+q\geq K+2$, on a $t\leq q-2$, donc un seul morceau et aucun événement (T3, `MATHEMATIQUES.md:194-196`) ; si
  $p\geq K$, les parties strictes sont déjà reliées strictement (T2, `:175-176`). Elle ne change pas non plus la
  couverture (P3, `:263-278`).
- Tout $K$-simplexe de Gabriel (Déf. 28, p. 87) contient $I_b$ et un support. Il vérifie donc $p+q\leq K+1$ et
  $p+m\geq K+1$ : **toutes les liaisons de Gabriel sont portées par $W_K$**. Ce sont les seules qui puissent fusionner
  (Th. 4, p. 88 ; Prop. 6, p. 90).

Énumérer les autres boules exigerait un catalogue d'ordre supérieur, que l'invariant d'architecture interdit
(`CLAUDE.md`). Ce périmètre est soumis à l'utilisateur (question 2, non bloquante).

### 2.3 Rattachement, branches et rôles

Pour $b\in W_K$, on note $\lambda_b$ son niveau et $r_b$ son rang.

**Lemme A (rattachement).** Toutes les $K$-parties de $P_b$ sont dans une même composante $C_b$ de $\Gamma_K$ à la
coupe fermée $\lambda_b$. On définit $\mathrm{att}(b)$, l'unique nœud vivant à cette coupe qui représente $C_b$.

*Preuve.* Si $\lvert P_b\rvert=K$, il n'y a qu'une partie. Sinon, T3 relie toutes les $K$-parties de $P_b$ au seuil
fermé (`MATHEMATIQUES.md:188-192`). Nœuds vivants et composantes se correspondent par T1. ∎

**Lemme B (rôles).**
- Si $b$ est une naissance, $\mathrm{att}(b)$ est le nœud de naissance de $b$, de même rang.
- Si $b$ est une cellule, alors :
  - soit $\mathrm{rang}(\mathrm{att}(b))=r_b$, et $\mathrm{att}(b)$ est une fusion créée au plateau $\lambda_b$ : rôle
    **fusion** ;
  - soit $\mathrm{rang}(\mathrm{att}(b))<r_b<\mathrm{rang}(\mathrm{parent}(\mathrm{att}(b)))$, ou bien
    $\mathrm{att}(b)$ est la racine : rôle **interne** (continuation, qui ferme des cycles ou ajoute des sommets sans
    fusionner).

*Preuve.*
- *Naissance.* Ses parties sont toutes nouvelles et isolées à leur niveau (T4, `MATHEMATIQUES.md:202-206`). Le
  moteur ne fait jamais d'une naissance de rang $r$ l'enfant d'une fusion de même rang (`forest_plateau.cpp:85-86`).
- *Cellule.* Elle a une trace stricte $F$, avec $\beta(F)<\lambda_b$ et $F\in C_b$. Le nœud $\mathrm{att}(b)$ n'est
  donc pas une naissance de niveau $\lambda_b$. Si le plateau réunit au moins deux anciennes composantes dans $C_b$, T4
  crée exactement une fusion à ce niveau. Sinon, $C_b$ prolonge l'unique composante ancienne. ∎

**Définition (branches).** $\mathrm{ant}(b)$ est l'ensemble des nœuds vivants à la **coupe ouverte**, c'est-à-dire à
la coupe fermée au rang $r_b-1$, dont la composante contient une $K$-partie **stricte** de $P_b$. Le champ publié
`components` vaut $\lvert\mathrm{ant}(b)\rvert$ (nom de l'audit : `strict_global_components`). Il vaut 0 pour une
naissance.

**Lemme C (branches).**
1. $\mathrm{ant}(b)$ est aussi l'ensemble des nœuds de coupe ouverte des seules traces strictes $I_b\cup A$, avec
   $\lvert A\rvert=t=K-p$.
2. Rôle interne : $\mathrm{ant}(b)=\lbrace\mathrm{att}(b)\rbrace$. Rôle fusion :
   $\mathrm{ant}(b)\subseteq\mathrm{enfants}(\mathrm{att}(b))$.
3. Pour toute fusion $v$, $\bigcup\lbrace\mathrm{ant}(b) : \mathrm{att}(b)=v,\ b\ \text{de rôle fusion}\rbrace=\mathrm{enfants}(v)$.

*Preuve.*
1. T2 comprime toute $K$-partie stricte de $P_b$ en une trace stricte $I_b\cup A$, par des échanges stricts : ajouter
   un intérieur laisse la trace $G\cap U_b$ inchangée, donc séparable (`MATHEMATIQUES.md:175-180`).
2. Soit $u$ le nœud de coupe ouverte d'une trace stricte. Son parent a un rang $\geq r_b$. S'il vaut $r_b$, ce parent
   est l'ancêtre fermé, donc $\mathrm{att}(b)$. Sinon, $u$ est vivant à $\lambda_b$, donc $u=\mathrm{att}(b)$ par le
   lemme A.
3. Les enfants d'une multifusion sont exactement les anciennes composantes touchées par les événements de sa
   composante de plateau (T4). Le code le réalise ainsi : `forest_plateau.cpp:59-62` (touch et unions) et `:83-89`
   (enfants = `top` des racines de la chaîne). ∎

**Conséquences.**
- Les $\mathrm{att}$ **partitionnent** $W_K$ sur les nœuds.
- Toute fusion possède au moins une boule de rôle fusion.
- À $K\geq 2$, toute naissance possède exactement une boule de rôle naissance, la sienne, plus d'éventuelles
  liaisons internes.
- À $K=1$, les naissances sont des sites et ne possèdent aucune boule.
- Une cellule **passagère** (rôle fusion avec $\lvert\mathrm{ant}(b)\rvert=1$) se lit sur `components` ; elle n'appelle
  aucune descente supplémentaire.
- Les unions DSU effectuées par une cellule dépendent de l'ordre de traitement. Elles ne sont **jamais** publiées
  (`plateau/README.md` : 2, 2, 1, 0 pour quatre boules qui touchent chacune trois composantes).

### 2.4 Calcul exact sans descente (E1), et juge par descente (E2)

**Lemme D (le journal suffit).** On considère la graine $g$ qu'a rendue le constructeur pour une trace stricte $F$ de
$b$, en voie complète par `resolve_descent` (`forest_plateau.cpp:50-57`) ou en voie régulière par `resolve_job`
(`forest_parallel.cpp:48-113`).
1. $\mathrm{rang}(g)<r_b$.
2. Le nœud $u$ vivant à la coupe fermée $r_b-1$ qui contient $g$ est le nœud de coupe ouverte de $F$.
3. $\mathrm{att}(b)=\mathrm{parent}(u)$ si $\mathrm{rang}(\mathrm{parent}(u))=r_b$, et $\mathrm{att}(b)=u$ sinon.
   Cette valeur ne dépend pas de la trace ; l'égalité pour toutes les traces est un **contrôle de T3**.

*Preuve.*
1. C'est contrôlé par le code (`:57`, `:109`).
2. T5 place $g$ dans la composante de $F$ à la coupe fermée $\beta(F)\leq\ell(r_b-1)$ (`MATHEMATIQUES.md:212-215`).
3. C'est la preuve du point 2 du lemme C. ∎

**Lemme E (juge).** Pour **toute** $K$-partie $F\subseteq P_b$, l'ancêtre fermé au rang $r_b$ de la naissance
rendue par `descend(F)` est $\mathrm{att}(b)$ (T5 et lemme A). C'est `ball_nodes` (`bench/points_export.cpp:178-214`)
avec la fenêtre au lieu du prédicat fort (`:131-133`). Ce juge **de test** ne figure jamais dans le produit.

### 2.5 Supports positifs minimaux

Définition, option 2 de `Zoltan/FoundationModel/JETON.md:67-71` :

$$\mathcal{Q}_b=\lbrace Q\subseteq U_b : c_b\in\mathrm{relint}\,\mathrm{conv}\,Q,\ Q\ \text{affinement indépendant},\ 2\leq\lvert Q\rvert\leq 4\rbrace$$

**Lemme F.**
1. **Carathéodory strict.** Pour $Q\subseteq U_b$, deux énoncés sont équivalents :
   - $Q$ est affinement indépendant et $c_b\in\mathrm{relint}\,\mathrm{conv}\,Q$ ;
   - $c_b\in\mathrm{conv}\,Q$ et aucune partie propre de $Q$ ne vérifie cette inclusion.

   $\mathcal{Q}_b$ est donc la famille des parties non séparables **minimales** de $U_b$.
2. **Prédicats exacts.** Tous les points de $Q$ sont sur la sphère.

   | $\lvert Q\rvert$ | Test |
   | --- | --- |
   | 2 | `is_midpoint` (`predicates.cpp:232-240`) |
   | 3 | `strictly_acute` puis `orientation(a,b,c,sphère) == 0` (`:305-309`, `:183-214`) |
   | 4 | `strictly_inside` (`:216-230`) |

   - Aucun test d'indépendance ni de minimalité n'est nécessaire.
   - Un triangle droit n'est pas un support ; son hypoténuse l'est.
   - Le drapeau `q4_presentation_strictly_inside()` ne vaut que pour le tétraèdre générateur
     (`geometry.hpp:59-60`) : il n'est jamais utilisé ici.
3. **Propriétés.**
   - $\mathcal{Q}_b\neq\varnothing$, et son plus petit cardinal est $q$.
   - Son premier élément pour l'ordre (cardinal, ordre lexicographique des `SiteIdx`) est $S^*$
     (`MATHEMATIQUES.md:24-28`).
   - Si $m=q$, $\mathcal{Q}_b=\lbrace S^*\rbrace$.
   - Les $\mathcal{Q}_b$ sont deux à deux disjoints, puisque $Q$ détermine sa boule par M1 (`:30-40`). Chaque support
     appartient donc à la liste propre d'un seul nœud.
4. **Énumération sur toute $U_b$.** Le canoniseur de $S^*$ (`src/catalogue/support.cpp:89-111`) et les seuils
   d'admission du générateur ne sont pas des énumérateurs. Le cube $\lbrace 0,2\rbrace^3$, de $q_{\min}=2$, porte
   quatre diamètres et deux tétraèdres (`receipts/audit_supports_20261004/qb/README.md`).

*Preuve.*
- Point 1 : coordonnées barycentriques uniques et strictement positives dans un sens ; translation des poids le long
  d'une dépendance affine dans l'autre.
- Point 2 : deux points diamétraux ; un triangle coplanaire au centre a ce centre pour centre circonscrit, intérieur
  si et seulement si le triangle est strictement aigu ; signes stricts sur les quatre faces.
- Point 3 : il découle des définitions de $q$ et de $S^*$, et de M1. ∎

### 2.6 Comptes publiés

**Notations.** $t=K-p$, avec $1\leq t\leq m$. $N_j=\lvert\lbrace A\subseteq U_b : \lvert A\rvert=j,\ \exists Q\in\mathcal{Q}_b,\ Q\subseteq A\rbrace\rvert$
(fermeture vers le haut des supports).

**Lemme G.**

| Champ | Sens exact | Formule | Jonction régulière ($m=q$, $K=p+q-1$) | Naissance régulière ($K=p+q$) |
| --- | --- | --- | --- | --- |
| `k_parts` (boule) | $K$-parties de $P_b$, toutes reliées à $\lambda_b$ (T3) | $\binom{p+m}{K}$ | $p+q$ | 1 |
| `compressed_parts` (lecteur seulement) | $K$-parties qui contiennent tout $I_b$ (traces $I\cup A$) | $\binom{m}{t}$ | $q$ | 1 |
| `strict_traces` (boule) | traces strictes, déjà présentes avant $\lambda_b$ (T2) | $\binom{m}{t}-N_t$ | $q$ | 0 |
| `components` (boule) | $\lvert\mathrm{ant}(b)\rvert$, branches réunies ou prolongées | tour (lemme D) | $\geq 1$ | 0 |
| `cofaces` (boule) | $(K+1)$-parties $G\subseteq P_b$ avec $B(G)=b$ : **liaisons distinctes** ($K$-simplexes, Prop. 5, p. 86) | $\sum_j\binom{p}{K+1-j}N_j$ | 1 | 0 |
| `cofaces` (support $Q$) | $(K+1)$-parties de $P_b$ qui contiennent $Q$ : **incidences** $(Q,G)$ | $\binom{p+m-\lvert Q\rvert}{K+1-\lvert Q\rvert}$ (0 si $K+1<\lvert Q\rvert$) | 1 | 0 |
| `gabriel_cofaces` (lecteur seulement) | celles qui contiennent $I_b$ (Déf. 28, p. 87) | support : $\binom{m-\lvert Q\rvert}{t+1-\lvert Q\rvert}$ ; boule : $N_{t+1}$ | 1 | 0 |

*Preuve.*
- `strict_traces` : par T2, $I_b\cup A$ est stricte si et seulement si $A$ ne contient aucun support (lemme F).
- `cofaces` : $B(G)=b$ si et seulement si $G\subseteq P_b$ et $G\cap U_b$ n'est pas séparable (M1–M2,
  `MATHEMATIQUES.md:30-45`) ; on compte selon $j=\lvert G\cap U_b\rvert$.
- `cofaces` par support : $Q\subseteq G\subseteq P_b$ entraîne $B(G)=b$ (M2) ; on choisit ensuite les autres sites.
- Gabriel : on impose de plus $I_b\subseteq G$. ∎

**Remarques.**
- $\max_Q\mathrm{cofaces}(Q)\leq\mathrm{cofaces}(b)\leq\sum_Q\mathrm{cofaces}(Q)$. La dernière inégalité est une
  égalité si et seulement si aucune $(K+1)$-partie ne contient deux supports.
- La **somme sur $Q$ compte des incidences**, pas des cofaces uniques (audit,
  `AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md:39-41`).
- En position générale, `cofaces` vaut 1 à une jonction et 0 à une naissance. Les « nombreuses liaisons »
  n'apparaissent qu'avec des coquilles cosphériques (question 1).

**Bornes.** On a $p\leq K-1\leq 11$ et $m\leq 24$. Toutes les valeurs tiennent en `u32` : $\binom{35}{13}<1{,}5\cdot 10^{9}$
et $\binom{24}{12}=2\,704\,156$. Une table `constexpr` de binômes, sous `static_assert`, suffit.

**Contre-épreuve par boule.** `strict_traces` vient du **journal du constructeur** : le test de stricte séparabilité
y passe par `bounded_meb` (`cells.cpp:76-86`). $N_t$ vient de **$\mathcal{Q}_b$** (prédicats de `num`). Les deux
voies arithmétiques sont distinctes. L'égalité $S_{\mathrm{journal}}=\binom{m}{t}-N_t$ est exigée pour chaque boule,
naissances comprises ($S=0$, $N_t=\binom{m}{t}$).

### 2.7 Polyèdres, instantanés, ordre un

**Polyèdre.** On note $w\preceq v$ quand $w$ est dans le sous-arbre de $v$.

$$P_v=\bigcup_{w\preceq v}\ \bigcup_{\mathrm{att}(b)=w}\ \bigcup_{Q\in\mathcal{Q}_b}\mathrm{conv}\,Q$$

Il en découle $P_w\subseteq P_v$ pour $w\preceq v$.

**Instantané à $a\in[a_v,a_{\mathrm{parent}(v)})$.**
- Les boules propres de $v$ de niveau $\leq a$. Ce sont un **préfixe** de la liste propre, triée par rang.
- Plus toutes les boules du sous-arbre strict, toutes de niveau $<a_v$ (lemme B).
- Seules des liaisons internes de $v$ peuvent dépasser $a_v$. C'est le cas de la fixture `growth_ABCZ`
  (`Zoltan/FoundationModel/CONTRAT_COUPES_ET_MASSES_20260926.md:27-38`) : un état est **daté**.

**Masque de géométrie.** `has_support_geometry` n'est faux que pour les feuilles de $K=1$ (des sites,
`JETON.md:82-85`). Tout autre nœud possède au moins une boule.

**Ce que $P_v$ n'est pas.** C'est un **squelette de témoins** (`JETON.md:78-88`) :
- il n'est ni $\mathrm{conv}(U_b)$, ni la population, ni le $K$-polyèdre de la Déf. 21 (p. 58) ;
- il peut omettre des intérieurs et des sites de coquille : environ 97 % des coquilles étendues LiDAR ont $m=3$ et
  $q=2$, avec un site orphelin (lecture `supports_qb`, § e.1 ; mesure locale non reçue, à requalifier) ;
- les géométries de deux branches peuvent se recouvrir.

**Stabilité.** La stabilité de FULL en rayon (P5) **ne se transfère pas** à ce support porteur : un saut de Hausdorff
d'au moins $1/4$ peut survenir sous une perturbation qui tend vers 0 (`receipts/audit_supports_20261004/carrier/README.md`).
La représentation est déclarée ; aucune stabilité n'est revendiquée.

### 2.8 Invariance et déterminisme

La sortie ne dépend que des positions et de $K$. Les raisons :
- `SiteIdx` est le rang de Morton des positions (`src/cloud/cloud.hpp:4-10`).
- `BallIdx` suit l'ordre (niveau, $S^*$).
- La numérotation des nœuds est géométrique (§ 2.1).
- $\mathrm{att}$ ne dépend ni de la trace ni de la politique de descente (lemmes D et E).
- $\mathcal{Q}_b$ est un ensemble, énuméré dans un ordre fixe.
- Les écritures parallèles se font à des positions fixées (`ARCHITECTURE.md:23-24`).

Conséquences :
- une permutation de l'entrée donne des fichiers identiques à l'octet ;
- un réétiquetage injectif des `PointId` ne change que la colonne `SITES.point_id` (et les étiquettes de la sortie
  plate).

Portée : permutations, réétiquetages et symétries exactes de la grille. **Pas les rotations** : la quantification au
millimètre casse l'équivariance (`Zoltan/FoundationModel/SPECIFICATION.md:21-28`).

### 2.9 Fixtures gravées

Les attendus sont **recalculés par l'oracle S1** avant d'être gravés. Les valeurs ci-dessous ont été vérifiées à la
main ou viennent des modèles de l'audit `1bf4be68f`. Coordonnées entières, avec $z=0$ si rien n'est précisé ;
translation positive si besoin.

| # | Fixture | K | Attendus principaux |
| ---: | --- | --- | --- |
| 1 | carré `(0,0),(2,0),(2,2),(0,2)` | 1–4 | K1 : quatre côtés de rôle fusion (`components` 2), puis la diagonale (niveau 2) interne sur la racine, `cofaces` 2, 1 par support. K2 : quatre naissances au niveau 1, la diagonale de rôle fusion, `components` 4, `k_parts` 6, `strict_traces` 4, `cofaces` 4, 2 par diagonale. K3 : naissance étendue, `k_parts` 4, `cofaces` 1, 1 par diagonale (double compte). K4 : naissance, `cofaces` 0. |
| 2 | triangle droit `(0,0),(4,0),(0,3)` | 1, 2 | Hypoténuse de niveau 25/4, $m=3$, $q=2$, $\mathcal{Q}_b$ réduit à l'hypoténuse (l'origine est un site orphelin). K1 : fusions à 9/4 puis 4, hypoténuse interne. K2 : naissances à 9/4 et 4, fusion à 25/4, `strict_traces` 2, `cofaces` 1. |
| 3 | `growth_ABCZ` `(1,8),(5,10),(9,8),(5,0)` | 3 | Naissance ABC à 16. À 25, boule interne sur cette naissance ($m=4$), $\mathcal{Q}_b=\lbrace BZ, ACZ\rbrace$, `cofaces` 1 pour chacun, `cofaces` 1 pour la boule (`morsehgp3D_v9/tests/tower/full_ball_tower_gate.cpp:131,509-512`). |
| 4 | **passagère** `(0,0),(2,2),(4,0),(8,0)` | 1 | Au niveau 2, une fusion de $\lbrace a,b,c\rbrace$. Au niveau 4, la boule de $(c,d)$ réunit cette composante et $d$ (`components` 2). La boule $\lbrace a,b,c\rbrace$, de même niveau 4 (diamètre $ac$, $b$ sur le cercle), est de rôle **fusion** avec `components` 1. |
| 5 | triangle aigu `(0,0),(2,0),(1,2)` | 2 | $\beta=25/16$, trois composantes avant, fusion à trois enfants, `strict_traces` 3, `cofaces` 1 (`incidences/README.md`). |
| 6 | ligne `(0,0),(1,0),(2,0)` | 2 | $\beta=1$, $I=\lbrace 1\rbrace$, `k_parts` 3, `strict_traces` 2, `components` 2, `cofaces` 1 (même reçu). |
| 7 | triangle équilatéral `(0,0,0),(2,2,0),(2,0,2)` | 2 | Événement **faible** : trois naissances de paires au niveau 2, fusion à trois enfants au niveau 8/3 (`qb/README.md`). |
| 8 | tétraèdre de l'audit, translaté de +10 : sommets `(20,20,20),(20,0,0),(0,20,0),(0,0,20)`, intérieurs `(10,10,10),(11,10,10),(10,11,10)` | 5 | Six composantes au niveau 200. Quatre boules de face au niveau 800/3, chacune avec `components` 3, `k_parts` 6, `strict_traces` 3, `cofaces` 1. Une fusion à six enfants. Aucune donnée d'unions effectuées (`plateau/README.md`). |
| 9 | cube $\lbrace 0,2\rbrace^3$ | 2 | $\mathcal{Q}_b$ : quatre diamètres et deux tétraèdres, aucun triangle ; premier support $=S^*$. |
| 10 | octaèdre `(0,1,1),(2,1,1),(1,0,1),(1,2,1),(1,1,0),(1,1,2)` | 2 | $\mathcal{Q}_b$ : trois paires, ni triangle ni tétraèdre (tiroirs). |
| 11 | cercle à $B_t$, en u21 (`carrier/README.md`) | 2 | $\mathcal{Q}_b$ passe de $\lbrace AC,BD\rbrace$ à $\lbrace AC,B_tCD\rbrace$ : documente la non-stabilité. |
| 12 | ligne `0,4,6,8,12` à K2 ; deux losanges à K3 (`docs/FULL_BIRTH_RUNS.md:72-81`) ; $\lbrace 0,2,4\rbrace$ à K2 (`MATHEMATIQUES.md:219-222`) | 2, 3 | Non-naissance au niveau 4, fusion à trois enfants, deux graines menant au même nœud. |
| 13 | `sphere50` : les 84 points entiers de $x^2+y^2+z^2=50$, translatés de +10 | 2 | $m=84>24$ : refus `support_shell_capacity`, aucun dossier publié. |

---

## 3. Architecture

### 3.1 Modules

On ajoute le module `supports` à la table normative (`docs/ARCHITECTURE.md:36-48`) et à sa copie
(`cmake/modules.cmake:6-17`). La règle `[table]` les compare (`tools/check_style.py:96-115`).

| Module | Rôle | Dépend de |
| --- | --- | --- |
| `io` | lecture `u32le`, SHA-256, écrivains petit-boutistes, transaction de dossier, JSON déterministe | `core`, `cloud` |
| `tower` | inchangé, plus : en-tête public parapluie, `build_order`, journal des graines, `WindowAttachment` | `catalogue`, `index` |
| `supports` (nouveau) | $\mathcal{Q}_b$, comptes, postordre, `SupportHierarchy` | `tower` |
| `points` | $H^{r}_{K+1}$ : pendaisons et arbre de points | `tower` |
| `head` | condensation, scores exacts, sélection, étiquettes | `points` |
| `api` | `Session`, requêtes, produits, sérialiseurs des quatre formats, manifeste | tous |

- Nouvel ordre : `core num sched cloud io index catalogue tower supports points head api`.
- `docs/ARCHITECTURE.md:48` devient « façade publique `api/api.hpp` et `Session` » (la règle `[module]` exige
  `<m>.hpp`).
- `:50` devient « `cli/` contient un seul exécutable, `mhgp11`, à paramètre de sortie obligatoire `--sortie` ».

### 3.2 `tower`

**En-tête public, sans cycle (tranche S2, aucun octet de sortie ne change).**
1. `src/tower/meb.hpp` reçoit sans modification le contenu actuel de `tower.hpp` : `kMaxMebSites`, `MebLedger`,
   `BoundedMeb`, `bounded_meb`, `MebCensus`, `meb_census` (`src/tower/tower.hpp:13-72`).
2. Les quatre inclusions internes passent à `"tower/meb.hpp"` : `cells.hpp:4`, `locate.hpp:4`, `census_slots.hpp:3`,
   `meb.cpp:2`.
3. `tower.hpp` devient le parapluie public. Il inclut `meb.hpp`, `forest.hpp` puis, en S3, `order_tree.hpp`, et
   déclare dans `namespace mhgp11` des `using` vers `tower_detail` : `ForestNode`, `OrderForest`, `FullTower`,
   `FullParams`, `FullTimings`, `OrderTimings`, `build_full`, puis `OrderTree`, `WindowAttachment`, `BallRole`,
   `build_order`.
4. Aucun code ne quitte `tower_detail`.
5. Les bancs, qui incluent `tower/forest.hpp` directement (`bench/full_probe.cpp:8`, `bench/points_export.cpp:20`),
   ne changent pas.

**Arbre K et rattachement (tranche S3).** Trois nouveaux fichiers : `order_tree.hpp`, `order_tree.cpp`,
`attachment.cpp`.
- **`build_order`** reprend la mise en place de la boucle non concurrente de `build_full`
  (`forest_vertical.cpp:298-327,334-352`) : table de populations, espaces census, mémo, `ForestParallel`. Il ne garde que
  l'ordre K, avec `vertical_seeds = nullptr`, et ajoute `builder.seed_log`. `build_full` n'est pas modifié.
- **Journal.**
  - Un champ `SeedLog* seed_log = nullptr` s'ajoute à `ForestBuilder` (`forest_internal.hpp:134-194`).
  - Il est distinct de `representative`, que le pipeline désactive.
  - Quelques lignes gardées par `if (seed_log != nullptr)`, dans `cell` et dans `regular_cell`, ouvrent la cellule
    et consignent chaque graine de naissance (jamais une racine ni un `top`). Sans journal, le comportement est
    inchangé à l'octet.
  - Le journal est écrit par le seul fil qui applique les cellules, en ordre `BallIdx`.
  - Les motifs de mutants de `forest_plateau.cpp` doivent rester uniques : `active = true; MHGP11_TRY(cell(BallIdx{b}));`,
    `states[states[a].tail].next = states[b].head;`, `const u32 root = find(idx(seed));`, `root = a; …`
    (`tests/mutants/tower.json`).
- **Rattachement.** Après `finish()`, le balayage du lemme D produit `WindowAttachment`, et les contrôles I1 à I4 sont
  faits dans le produit. Tout écart rend `tower_invariant`, sans aucun résultat.

### 3.3 `supports`

- `src/supports/supports.hpp`, l'en-tête public.
- `enumerate.cpp` : port explicite de `mark_supports` et de `closure_counts` (`bench/catalogue_euler.hpp:108-165`),
  avec reconstruction de la sphère depuis $S^*$ et contrôle du niveau (`:236-241`).
- `counts.hpp` : binômes `constexpr`.
- `hierarchy.cpp` : postordre, tri des boules par postordre de leur nœud, CSR des supports et des branches.

Deux raisons nouvelles, ajoutées en fin de table (`reasons.def:6-8`) :
- `support_shell_capacity` (`unsupported_degeneracy`, `supports`) ;
- `supports_invariant` (`invariant_violated`, `supports`).

### 3.4 `io` (port du raccord R2)

Sources épinglées par sha256 dans `docs/PROVENANCE.md` :

| Source R2 | Ce qui est porté |
| --- | --- |
| `build/v10-integration-r2/src/morsehgp3D_v10/src/cloud/u32le_input.hpp` | Lecture entière ou refus, par `stat()`, sans allocation avant le contrôle de taille. |
| `src/core/cli_output.hpp` | Déclaration préalable des entrées et sorties ; conflit par chemin résolu et par inode ; écritures contrôlées (`fflush`, `ferror`, `fclose`) ; temporaires dans le dossier cible ; destructeur sans publication. |
| `morsehgp3d/src/cpu/contract/canonical_id.cpp:49-188` | SHA-256 (`CanonicalSha256Builder`). |

**Adaptations.**
- `Outcome`, aucune exception, `MemoryBudget`.
- Fichier `ids.u32le` séparé, comme `bench/whole_input.hpp:27-56`.
- Transaction **de dossier**, sans sauvegarde `.bak` : la cible ne doit jamais exister, ce qui supprime la
  restauration.

**Raisons.** Les quatre raisons d'`io` (`reasons.def:24,33,35,37`) sont émises dans `src/io/` et citées dans
`tests/io/` (règles `[raison_morte]` et `[raison_sans_porte]`, `check_style.py:449-476`).
`tests/io/full_march_collector.py` est déjà enregistré par `tests/tower/tests.cmake:206-208` : ne pas le
réenregistrer.

### 3.5 `api` et 3.6 `cli`

**`api`.**
- `session.cpp` : budget unique et `Pool` unique, plus l'auto-test F5 (`ARCHITECTURE.md:107-108`) avec la raison
  `environment_selftest` (`invariant_violated`, `api`). `close` appelle `released()` (`src/core/buffer.hpp:96-98`).
- `compute.cpp`, `write_full.cpp` (port octet pour octet de `bench/full_probe.cpp:14-75`), `write_supports.cpp`, puis
  `write_points.cpp` et `write_flat.cpp`, et `manifest.cpp`.

**`cli`.**
- `cli/cli.cmake` ne déclare que la cible `mhgp11_cli` (`OUTPUT_NAME mhgp11`, liée à `mhgp11`) ; il est inclus avant
  CTest (`CMakeLists.txt:265-270`).
- Ses portes vont dans `tests/cli/tests.cmake`.
- `cli/mhgp11.cpp` n'inclut que `"api/api.hpp"`.

### 3.7 `points`, `head`, `num`

- `points` : port de `bench/points_hierarchy.py:215-312` et de `bench/points_radius.py:184-256`, plus l'arbre de
  points de `bench/points_flat.py:462-539`.
- `head` : port de `bench/points_flat.py:647-888`.
- `num` reçoit `big`, `rational`, `roots` (table d'encadrements) et `radical`, avec la raison `radical_sign_budget`
  (`resource_exhausted`, `num`).
- Raisons de `points` et `head` : `points_invariant` et `head_invariant`.

### 3.8 Ce qui ne bouge pas

- Les sondes `mhgp11_full_bench` et `mhgp11_points_export`, leurs déclarations (`tests/tower/tests.cmake:64-68`) et
  leurs portes à ligne exacte.
- Les formats `MHGP11FUL1` et `MHGP11PH`.
- Le catalogue, les cellules, les descentes et `build_full`.
- Le registre `docs/implementation_status.toml` : l'exploration v11 est hors registre.

---

## 4. API C++ (esquisses normatives)

Toutes les fonctions sont `noexcept`. Elles rendent un `Result` ou un `Outcome` ; aucun résultat partiel n'est publié.
Tout tampon est admis dans le budget avant son allocation (`ARCHITECTURE.md:159-172`).

```cpp
// src/tower/order_tree.hpp (expose par tower/tower.hpp)
namespace mhgp11::tower_detail {
enum class BallRole : u8 { birth = 0, merge = 1, internal = 2 };
class WindowAttachment {  // W_K, BallIdx croissants (donc rangs croissants)
 public:
  u32 size() const noexcept;
  std::span<const BallIdx> balls() const noexcept;
  std::span<const NodeIdx> node() const noexcept;        // att(b) : coupe FERMEE lambda_b
  std::span<const BallRole> role() const noexcept;
  std::span<const u32> strict_traces() const noexcept;   // S(b) du journal ; 0 pour une naissance
  std::span<const u32> components() const noexcept;      // |ant(b)| ; 0 pour une naissance
  std::span<const u64> prior_offsets() const noexcept;   // size()+1 ; non vide pour le role merge seulement
  std::span<const NodeIdx> prior() const noexcept;       // ant(b) a la coupe OUVERTE, croissants
};
class OrderTree {  // possede le domaine, la foret d'ordre k et le rattachement
 public:
  const FullDomain& domain() const noexcept;
  Order order() const noexcept;
  const OrderForest& forest() const noexcept;
  const WindowAttachment& attachment() const noexcept;
};
// Ordre k seul, sans verticales ni autres ordres. FullParams comme build_full ; parallel_verticals et
// reuse_regular_verticals exiges faux ; concurrent_orders refuse jusqu'a S11 (parameter_out_of_range).
// Refus : parameter_out_of_range, memory_budget, tower_capacity, tower_invariant ; domaine intact sur refus.
[[nodiscard]] Result<OrderTree> build_order(FullDomain&&, Order k, MemoryBudget&, FullParams = {},
                                           sched::Pool* = nullptr, OrderTimings* = nullptr) noexcept;
}

// src/supports/supports.hpp
namespace mhgp11::supports {
inline constexpr u32 kMaxShell = 24;  // coquille etendue ; au-dela : support_shell_capacity
struct Support { std::array<SiteIdx, 4> sites; u8 arity; u32 cofaces; };  // sites croissants, kNone au-dela
struct Ball {
  BallIdx key; NodeIdx node; LevelRank rank; BallRole role; u8 p, m, qmin;
  u32 k_parts, strict_traces, cofaces, components;
};
class SupportHierarchy {
 public:
  Order order() const noexcept;
  std::span<const u32> post() const noexcept;            // NodeIdx -> rang de postordre
  std::span<const u32> subtree_size() const noexcept;    // NodeIdx -> noeuds du sous-arbre
  std::span<const u64> ball_offsets() const noexcept;    // N+1, indexe par rang de postordre
  std::span<const Ball> balls() const noexcept;          // ordre (postordre du noeud, rang, BallIdx)
  std::span<const u64> support_offsets() const noexcept; // B+1
  std::span<const Support> supports() const noexcept;    // par boule : (arite, lexicographique des SiteIdx)
  std::span<const u64> prior_offsets() const noexcept;   // B+1
  std::span<const NodeIdx> prior() const noexcept;
};
[[nodiscard]] Result<SupportHierarchy> build_support_hierarchy(const OrderTree&, MemoryBudget&,
                                                               sched::Pool* = nullptr) noexcept;
// Q_b d'une boule de Cat_K (portes, juges) : nombre ecrit ; scratch de 2^(m-6) mots.
[[nodiscard]] Result<u32> ball_supports(const FullDomain&, BallIdx, std::span<Support> out,
                                       std::span<u64> scratch) noexcept;
}

// src/io/io.hpp
namespace mhgp11::io {
using Digest = std::array<u8, 32>;
class Sha256 { public: void update(std::span<const u8>) noexcept; Digest finish() noexcept; };
struct InputFiles { Buffer<u32> x, y, z; Buffer<PointId> ids; Digest points_sha256, ids_sha256; u64 points_bytes, ids_bytes; };
[[nodiscard]] Result<InputFiles> read_u32le(const char* points, const char* ids, MemoryBudget&) noexcept;
class FileWriter {  // petit-boutiste, taille et sha256 au fil de l'ecriture, erreurs controlees
 public:
  [[nodiscard]] Outcome bytes(std::span<const u8>) noexcept;
  [[nodiscard]] Outcome u32s(std::span<const u32>) noexcept;
  [[nodiscard]] Outcome u64s(std::span<const u64>) noexcept;
  [[nodiscard]] Outcome pad8() noexcept;
  u64 size() const noexcept; Digest digest() const noexcept;
};
class OutputDirectory {  // paragraphe 6.7
 public:
  [[nodiscard]] static Result<OutputDirectory> plan(const char* directory,
                                                    std::span<const char* const> inputs) noexcept;  // rien n'est cree
  [[nodiscard]] Result<FileWriter*> create(std::string_view name) noexcept;  // [a-z0-9_.]+, distincts
  [[nodiscard]] Outcome commit(std::string_view manifest_json) noexcept;     // manifeste en dernier, NOREPLACE
  ~OutputDirectory();                                                        // sans commit : .pending retire
};
}

// src/api/api.hpp (seul en-tete inclus par cli/)
namespace mhgp11::api {
enum class OutputKind : u8 { full, supports, points, flat };  // --sortie=full|supports|points|plat
struct SessionParams { u64 budget_bytes = MemoryBudget::kUnlimited; u32 workers = 1; };
class Session {  // regle 3 : unique budget, unique Pool ; survit a tout Product
 public:
  [[nodiscard]] static Result<Session> make(const SessionParams&) noexcept;  // auto-test F5
  MemoryBudget& budget() noexcept; sched::Pool& pool() noexcept;
  [[nodiscard]] Outcome close() noexcept;  // budget_not_released si un Product vit encore
};
struct CloudView { std::span<const u32> x, y, z; std::span<const PointId> ids; };
struct FullRequest { Order k; };
struct SupportsRequest { Order k; };
struct PointsRequest { Order k; };
struct FlatRequest { Order k; u32 min_cluster_size = 20; u8 z = 1; bool leaves = false; };
using Request = std::variant<FullRequest, SupportsRequest, PointsRequest, FlatRequest>;
struct StageReport { u64 nanoseconds = 0, peak_bytes = 0; };
struct RunReport { std::array<StageReport, 8> stages{}; };  // cloud index domain tree attach output write total
class Product;  // resultat entier possede, compte dans le budget de la Session
[[nodiscard]] Result<Product> compute(Session&, const CloudView&, const Request&, RunReport* = nullptr) noexcept;
struct Provenance { io::Digest points_sha256, ids_sha256; u64 points_bytes, ids_bytes;
                    std::string_view grid_step, origin; };
[[nodiscard]] Result<io::Digest> publish(Session&, const Product&, const Request&, io::OutputDirectory&,
                                         const Provenance&, RunReport* = nullptr) noexcept;  // sha256 du manifeste
}
```

**`points` (S9) et `head` (S10).**
- `points::hang(const OrderTree&, MemoryBudget&, Pool*)` rend `PointHierarchy`, qui contient :
  - par site, une date en rangs $(t,M,Q)$, avec $M=Q=0$ sans rival ($\ell_0=0$) ;
  - le propriétaire, le rang plancher et le drapeau strict ;
  - un `PointTree` : plateaux exacts, au plus $2n-1$ blocs, `site_block`, `site_plateau`.
- `head::flat(const PointHierarchy&, std::span<const PointId> input_ids, FlatParams, MemoryBudget&)` rend des
  étiquettes `i64` dans l'**ordre d'entrée** et des compteurs.

---

## 5. CLI `mhgp11`

```text
mhgp11 --sortie=<full|supports|points|plat> --points=<x.u32le> --ids=<ids.u32le> --dossier=<D> --k=<K>
       [--fils=<W>] [--budget=<octets>] [--pas=<decimal>] [--origine=<x,y,z>]
       [--mcs=<M>] [--z=<1|2|3>] [--selection=<eom|feuilles>]          (plat seulement)
```

| Option | Domaine | Défaut |
| --- | --- | --- |
| `--sortie` | une seule valeur, parmi celles déjà livrées | obligatoire |
| `--points`, `--ids` | fichiers réguliers ; 12 et 4 octets par point | obligatoires |
| `--dossier` | ne doit pas exister, ni `D.pending` | obligatoire |
| `--k` | $1\leq K\leq 12$ (`catalogue.hpp:26`, `tower.hpp:13`), puis $K\leq n$ ; pour `full`, c'est kmax | obligatoire |
| `--fils` | 1..256, aucune détection implicite (`sched.hpp:17-20`) | 1 |
| `--budget` | octets, $>0$ | illimité |
| `--pas`, `--origine` | décimaux exacts, recopiés dans le manifeste (§ 7.3 de `ARCHITECTURE.md:190`) | non déclarés |
| `--mcs` | $\geq 2$ | 20 |
| `--z` | 1, 2 ou 3 (valeurs exercées, `bench/points_flat_gate.py:38`) | 1 |
| `--selection` | `eom` ou `feuilles` | `eom` |

**Moteur.** Les paramètres sont fixes, ceux du masque qualifié 16 379 : `leaf_size` 16, `max_leaf` 256 et les
options de `bench/points_export.cpp:376-383`. Il n'existe aucune option de moteur (règle 6).

**Ordre déterministe des refus.** Aucun refus ne publie de dossier.
1. Options : inconnue, répétée, absente, hors domaine ou propre à une autre sortie → `parameter_out_of_range`, avant
   tout effet.
2. Plan de sortie, **avant toute lecture** :
   - `D` ou `D.pending` existe, ou une entrée se trouve sous $D$ (chemin résolu ou inode) → `output_conflict` ;
   - parent non inscriptible → `output_unwritable`.
3. Session : `session_overhead`, `memory_budget`, `environment_selftest`.
4. Lecture : `input_unreadable`, `index_overflow_u32`.
5. Nuage : `empty_input`, `coordinate_out_of_domain`, `duplicate_point_id` (`cloud.hpp:90-100`). Positions répétées :
   `multiplicity_unsupported`.
6. $K>n$ : `parameter_out_of_range`.
7. Calcul : `memory_budget`, `tower_capacity`, `support_shell_capacity`, `radical_sign_budget`, invariants.
8. Écriture et publication : `output_unwritable`.

**Codes de sortie** (`status.hpp:111-116`) : 0 conforme ; 2 refus ; 3 invariant (`budget_not_released` compris). Un
signal est toujours un échec.

**Sortie standard.** Exactement une ligne JSON, à clés anglaises comme les bancs :

```json
{"phase":"mhgp11","output":"supports","status":"ok","reason":"none","coord_bits":21,"k":5,"workers":48,"sites":39885,"stages_ns":{"cloud":0,"index":0,"domain":0,"tree":0,"attach":0,"output":0,"write":0},"peaks_bytes":{"domain":0,"tree":0,"output":0},"counts":{"nodes":0,"balls":0,"supports":0}}
```

Temps et nombre de fils vont dans cette ligne, **jamais dans le manifeste**.

---

## 6. Formats et manifeste

### 6.1 Conventions communes

- Petit-boutiste. Chaque colonne commence sur une frontière de 8 octets ; les octets de bourrage sont nuls. Les
  sections sont stockées colonne par colonne, ce qui permet `numpy.frombuffer`.
- `kNone = 0xFFFFFFFF` ne sert de sentinelle que pour les rangs denses. Un `PointId` peut valoir `0xFFFFFFFF`
  (`src/core/types.hpp:61-64`) : un tableau de `PointId` est toujours borné par un compte.
- Les rangs publiés sont des `LevelRank` du catalogue $\mathrm{Cat}_K$ de l'appel. L'égalité de rang équivaut à
  l'égalité de niveau.
- **Ordres canoniques**, tous géométriques :
  - sites : `SiteIdx` (Morton) ;
  - nœuds : numérotation de la forêt ;
  - boules : (postordre du nœud, rang, `BallIdx`) ;
  - supports : (arité, ordre lexicographique des `SiteIdx`).

### 6.2 `full.mhgp11ful1` : `MHGP11FUL1`, inchangé

Ses octets sont ceux de `serialize` (`bench/full_probe.cpp:40-75`, lecteur `bench/full_semantic.py`). Seule
l'écriture change : elle devient transactionnelle. Aucune nouvelle version n'est créée, pour garder la comparabilité
avec les reçus immuables.

### 6.3 `supports.mhgp11sp` : `MHGP11SP` version 1

| Section | Colonnes, dans cet ordre |
| --- | --- |
| En-tête (136 octets) | magie `MHGP11SP` ; 16 × `u64` : `version=1`, `coord_bits`, `k`, `n` (sites), `N` (nœuds), `births`, `root`, `B` (boules), `S` (supports), `A` (branches publiées), décalages de `SITES`, `NODES`, `BALLS`, `SUPPORTS`, `PRIOR`, taille totale |
| `SITES` | `x u32[n]`, `y u32[n]`, `z u32[n]`, `point_id u32[n]` |
| `NODES` (ordre canonique) | `parent u32[N]` (`kNone` à la racine) ; `rank u32[N]` ; `kind u8[N]` (0 feuille-site à $K=1$, 1 naissance de boule, 2 fusion) ; `birth u32[N]` (ligne de site si `kind` 0, `BallIdx` si 1, `kNone` si 2) ; `child_off u64[N+1]` ; `children u32[N-1]` ; `post u32[N]` ; `size u32[N]` ; `ball_off u64[N+1]` (indexé par rang de postordre) |
| `BALLS` (`B`) | `key u32` (`BallIdx`, propre à l'appel) ; `node u32` ; `rank u32` ($\lambda_b$) ; `role u8` (0 naissance, 1 fusion, 2 interne) ; `p u8` ; `m u8` ; `qmin u8` ; `k_parts u32` ; `strict_traces u32` ; `cofaces u32` ; `components u32` ; `support_off u64[B+1]` ; `prior_off u64[B+1]` |
| `SUPPORTS` (`S`) | `arity u8` ; `sites u32[4S]` (lignes de `SITES`, croissantes, `kNone` au-delà de l'arité) ; `cofaces u32` |
| `PRIOR` (`A`) | `node u32` : $\mathrm{ant}(b)$ en numérotation canonique, seulement pour le rôle fusion. Interne : $\lbrace$`node`$\rbrace$ par le lemme C. Naissance : vide. |

**Lectures directes.** On note $j=$`post[v]` et $s=$`size[v]`.
- Boules propres de $v$ : `[ball_off[j], ball_off[j+1])`.
- **$P_v$** : boules `[ball_off[j+1-s], ball_off[j+1])`, puis leurs supports par `support_off`.
- **Instantané** à un rang $r\in[$`rank[v]`$,$ `rank[parent[v]]`$)$ : les boules du sous-arbre strict, plus le
  préfixe des boules propres de rang $\leq r$.
- `has_support_geometry` vaut `kind != 0`.

**Niveaux sans table.** Chaque rang publié a un témoin dans le fichier :
- une boule a pour niveau le rayon carré circonscrit de son premier support $S^*$ (`MATHEMATIQUES.md:49-71`) ;
- un nœud de `kind` 1 ou 2 a le niveau de sa première boule propre (I11) ;
- une feuille de $K=1$ a le niveau 0.

Le lecteur en bibliothèque standard (`bench/mhgp11_formats.py`) fournit `exact_level(rank)` en `Fraction`, en
reprenant la routine `circumsphere` de `reference/hgp11_ref/definition.py:47-61`. Ce choix évite environ
$8+16W$ octets par rang, soit des dizaines de Mo à $K=10$.

**Taille estimée** : environ 41 octets par nœud, 48 par boule, 21 par support et 4 par branche. À mesurer en S7.

### 6.4 `points.mhgp11pt` : `MHGP11PT` version 1 (tranche S9)

| Section | Contenu |
| --- | --- |
| En-tête | magie `MHGP11PT` ; `version=1`, `coord_bits`, `k`, `m` (qualification : 1 si $K=1$, $K+1$ sinon), `kappa=1`, `n`, `L` (niveaux référencés), `W` (mots par niveau : 3 en u18/u21, 4 en u24, comme `bench/points_export.cpp:28-34`), `N`, `P` (plateaux), `Bk` (blocs), décalages |
| `SITES` | comme `MHGP11SP` |
| `LEVELS` | `rank u32[L]`, `num u64[W·L]`, `den u64[W·L]` : valeurs exactes non réduites des rangs référencés, triés |
| `NODES` | `parent u32[N]`, `rank u32[N]` : même numérotation que `MHGP11FUL1` et `MHGP11SP` |
| `HANGING` (par `SiteIdx`) | `t u32[n]`, `M u32[n]`, `Q u32[n]` (date $\sqrt{t}+\sqrt{M}-\sqrt{Q}$ ; $M=Q=0$ sans rival) ; `owner u32[n]` ; `floor u32[n]` ; `strict u8[n]` |
| `TREE` | `plateau_t u32[P]`, `plateau_M u32[P]`, `plateau_Q u32[P]` (strictement croissants en valeur exacte ; un plateau de niveau du catalogue vaut $(r,0,0)$) ; `block_plateau u32[Bk]` ; `block_parent u32[Bk]` ; `site_block u32[n]` ; `site_plateau u32[n]` |

Le schéma est celui de `PointTree` (`bench/points_flat.py:352-403`) et de `Hanging`
(`bench/points_hierarchy.py:315-336`).

### 6.5 `etiquettes.mhgp11et` : `MHGP11ET` version 1 (tranche S10)

- En-tête : magie, `version=1`, `n_points`, `k`, `mcs`, `z`, `selection` (0 pour EOM, 1 pour feuilles).
- Puis `labels i64[n_points]` **dans l'ordre du fichier d'entrée** : le plus petit `PointId` du cluster retenu, ou −1
  pour le bruit.
- Les compteurs vont dans le manifeste.

Le port n'utilise pas `out[pt.ids]` (`bench/points_flat.py:882`), qui suppose des `PointId` denses.

### 6.6 `manifeste.json`, écrit en dernier

Clés en ordre fixe, entiers en décimal, aucun temps ni nombre de fils : le manifeste est identique quel que soit W.

```json
{"schema":"ehgp.v11.output.v1","output":"supports","status":"complete","public_status":"not_claimed",
 "coord_bits":21,"k":5,"parameters":{"budget_bytes":0,"grid_step":null,"origin":null},
 "inputs":[{"name":"points","bytes":0,"sha256":"…"},{"name":"ids","bytes":0,"sha256":"…"}],
 "files":[{"name":"supports.mhgp11sp","format":"MHGP11SP","version":1,"bytes":0,"sha256":"…"}],
 "tree_k_sha256":"…",
 "counts":{"sites":0,"nodes":0,"births":0,"merges":0,"balls":0,"roles":{"birth":0,"merge":0,"internal":0},
           "supports":0,"arities":{"2":0,"3":0,"4":0},"extended_shells":0,"max_supports_per_ball":0,"prior":0}}
```

**`tree_k_sha256`.** C'est l'empreinte SHA-256 de : `"MHGP11TK"`, `u64 K`, `u64 N`, puis pour chaque nœud en ordre
canonique (`parent`, `rank`, `birth_key`, `child_count`) en `u32`, puis la liste concaténée des enfants. Elle est
commune à `full` (pour l'ordre kmax), `supports`, `points` et `plat` sur une même entrée et un même K. C'est la
traçabilité demandée (`Zoltan/FoundationModel/SPECIFICATION.md:27-28`).

### 6.7 Transaction de dossier

1. `plan` contrôle les conflits **avant tout calcul**. Rien n'est créé à cette étape.
2. Après le calcul :
   - `mkdir D.pending` ;
   - fichiers écrits et hachés au fil de l'écriture, chacun suivi de `fflush`, `ferror`, `fsync`, `fclose`
     contrôlés ;
   - manifeste écrit en dernier, puis `fsync` ;
   - `fsync` du dossier.
3. `renameat2(D.pending, D, RENAME_NOREPLACE)`, puis `fsync` du parent. Si `RENAME_NOREPLACE` est indisponible ou que
   `D` existe : refus, et **jamais** de renommage qui écrase (sur POSIX, `rename` remplace un dossier vide).
4. En cas de refus ou d'exception mémoire (`guarded`, `status.hpp:160-170`), le destructeur retire `D.pending`.
5. Un `D.pending` orphelin, laissé par un arrêt brutal, fait refuser l'appel suivant (`output_conflict`) ; il n'est
   jamais retiré automatiquement.

Le jeu est visible tout entier ou pas du tout. Le manifeste reste le seul témoin d'achèvement
(`ARCHITECTURE.md:174-182`, à amender en S0).

---

## 7. Algorithmes, exactitude, coûts

### 7.1 `build_order`

1. Validation : `k` est dans $[1,\min(\mathrm{kmax},n)]$ (`forest_build.cpp:416`) et les `FullParams` sont valides
   (`ForestParallel::validate`, `forest_parallel.cpp:9-18`).
2. Mise en place, comme `forest_vertical.cpp:298-327`, sans `RegularVerticalSeeds` : table de populations
   (non liée, `hit`), espaces census, mémo, `ForestParallel`.
3. Après la classification de l'ordre K, on admet le journal :
   - `cell_ball u32[C]` et `seed_off u64[C+1]`, où $C$ est le nombre de `kinds` = 2 ;
   - `seeds u32[Σ]`, avec $\Sigma=\sum_{\mathrm{régulières}}q+\sum_{\mathrm{étendues}}\binom{m}{t}$, majorant exact
     de $\sum S$, calculé par `cell_binomial` (`cells.cpp:16-27`).
4. `ForestBuilder::run()` sur la voie par lots, puis le rattachement (§ 7.2), puis la libération du journal.

**Exactitude.** C'est la même construction que l'ordre K de `build_full` en voie non concurrente. La forêt est
identique par construction, et une porte d'identité la contrôle (I10).

**Coût.** Il n'est pas mesuré. La voie par lots avance par barrières : une vidange avant chaque cellule étendue, et une
tous les 4 096 travaux (`forest_parallel.cpp:287-309`). Elle peut perdre contre le pipeline FULL à W48. La mesure est
faite en S7, et S11 traite l'écart.

### 7.2 Journal et balayage (lemme D)

```text
naissances (K >= 2) : pour v < births : b = birth_key(v) ; exiger b dans W_K, rang(v) = rang(b), b absent du journal ;
                      att(b) = v, role birth, S = 0, components = 0
cellules            : groupes du journal de meme rang r (strictement croissants) :
                      sweep.advance(r - 1)
                      pour chaque cellule b du groupe :
                        U_b = { sweep.query(g) : g graine de b }, deduplique et trie
                        pour u dans U_b : a(u) = parent(u) si parent(u) != kNone et rang(parent(u)) = r, sinon u
                        exiger a(u) unique (T3) ; att(b) = a ; role = merge si rang(att) = r, sinon internal
                        exiger internal => U_b = {att} ; merge => U_b inclus dans enfants(att)
                        S(b) = nombre de graines ; components = |U_b| ; prior = U_b si merge
controles           : chaque b de W_K une fois (I1) ; union des U_b sur les boules merge d'une fusion = ses enfants (I3)
```

**Correction.** Les conditions d'appel du balayage sont satisfaites :
- les rangs demandés ne décroissent jamais, car $r-1\geq r_{\mathrm{précédent}}$ (`forest_ancestor_sweep.hpp:40-42`) ;
- toute graine a un rang $<r$, donc $\leq r-1$ (`:60-62`).

**Coût.**
- Temps : $O((\lvert W_K\rvert+N)\,\alpha+\sum S)$.
- Mémoire : $12N$ octets pour le balayage (`:25-30`), plus le journal, plus le résultat (environ 17 octets par boule).
- Le travail est séquentiel. Il est mesuré en S7.

### 7.3 Supports et fermeture

Le calcul se fait en parallèle par tranches de boules, à positions fixes, en deux passes (compter, puis remplir).

- **Coquille régulière** ($m=q$) : $\mathcal{Q}_b=\lbrace S^*\rbrace$ et $N_j=[j=q]$, sans sphère ni prédicat.
- **Coquille étendue** :
  1. si $m>24$, refus `support_shell_capacity` avant tout calcul, pour l'appel entier ;
  2. sinon, sphère `Sphere::through(S*)` de l'arité $q$, avec égalité exigée au niveau du catalogue (comme
     `bench/catalogue_euler.hpp:236-241`) ; écart : `supports_invariant` ;
  3. paires, puis triplets, puis quadruplets de positions de $U_b$ (`Catalogue::shell`, `catalogue.hpp:165-167`) ;
  4. le premier support doit être $S^*$ (contrôle de `catalogue_euler.hpp:274-276`) ;
  5. fermeture zêta en OU, puis comptes $N_j$ (`:108-128`).
- Brouillon : $2^{m-6}$ mots par fil, au plus 2 Mio, admis avant les tâches.
- **Coût.** Sur les trames, $m\leq 5$ (`docs/CATALOGUE.md:207-208`) : au plus 25 prédicats par coquille étendue. Il y
  a 204 à 1 559 coquilles étendues par trame et par ordre ; ce sont des majorants mesurés sur $\mathrm{Cat}_{K+2}$
  (`docs/CATALOGUE.md:238-243`). Le coût est négligeable.

### 7.4 Comptes

Par la table de binômes : `k_parts`, `cofaces` de la boule et de chaque support, selon le lemme G. `strict_traces`
vient du journal, avec la contre-épreuve du § 2.6.

### 7.5 Assemblage

1. Postordre itératif depuis la racine, enfants visités par `NodeIdx` croissant (`forest_plateau.cpp:90-91`). On en
   tire `post` et `size`, en $O(N)$.
2. Tri par seaux des boules selon `post[node]`, stable dans l'ordre des `BallIdx`. On en tire `ball_off`.
3. CSR des supports et des branches par sommes préfixes.

Le tout coûte $O(N+B+S)$ et la sortie est déterministe.

### 7.6 Points : $H^{r}_{K+1}$ (tranche S9)

**Port explicite**, décision par décision.

1. **Incidences.** Ce sont les populations $I_b\cup U_b$ des boules **fortes** ($p+q\leq K$) de `WindowAttachment`,
   triées par site selon (rang, nœud). Elles remplacent `ball_nodes`, sans aucune descente. À $K=1$, l'incidence est la
   feuille de chaque site (`bench/points_export.cpp:233-234`).
2. **Qualification** (`bench/points_hierarchy.py:215-246`) : $m(K)=1$ si $K=1$, $K+1$ sinon.
3. **Départs qualifiés** par pointeurs (`:286-302`), puis $t_i$ et $p_1$ : premier départ de rang $t$ dans l'ordre
   (rang, nœud) (`:305-312`).
4. **LCA** par pointeurs de saut, un mot plus une profondeur par nœud, nouveau composant interne
   `tower/ancestor_index`.
5. **Rival maximal.**
   - Élagage par dominance de rangs : si $M_1\geq M_2$ et $Q_1\leq Q_2$, alors $\sqrt{M_1}-\sqrt{Q_1}\geq\sqrt{M_2}-\sqrt{Q_2}$.
   - Puis comparaison de deux racines contre deux (`sqrt_cmp2`, `bench/points_radius.py:48-49`).
6. **Plancher** par dichotomie sur les rangs (`floor_rank_radius`, `:184-193`).
7. **Propriétaire** : le plus haut ancêtre de $p_1$ de rang $\leq$ plancher, en rangs entiers.
8. **Arbre de points** (`bench/points_flat.py:462-539`). Les plateaux sont atomiques : fusions de rang $r$, puis
   entrées de date exactement $\sqrt{\ell_r}$, puis entrées strictement entre deux rangs. Les dates de même plancher
   sont ordonnées par comparaison exacte de trois racines contre trois (`sign_of_radicals`, `:93-117`).

**Exactitude.** Toute comparaison de sommes de racines passe d'abord par la **table d'encadrements** (§ 7.8). Si les
intervalles se chevauchent, on bascule sur le repli exact, avec les mêmes décisions que Python : élévations au carré
contrôlées, ou classes de carrés puis encadrements jusqu'à 8 192 bits. Au-delà, refus `radical_sign_budget`, jamais
une égalité supposée. Garanties : H1 à H7 (`docs/HIERARCHIE_POINTS.md:126-134`).

**Coût.**
- CSR : $8I$ octets.
- Session F, à $k=10$ : 7 547 225 incidences, 97 % de sites retardés (lecture `points`, § b).
- Les comparaisons se font en `u128` dans le cas courant.

### 7.7 Tête plate (tranche S10)

**Port explicite de `bench/points_flat.py:647-888`.**
- Condensation au critère A : la masse est le nombre de sites engagés.
- Scores : $S(C)=\sum c\,\varphi(\text{jonction})-\lvert C\rvert\,\varphi(\text{haut})$, avec $\varphi(r)=r^{-z}$.
- EOM N-aire : le parent l'emporte sur une égalité **certifiée**.
- Variante feuilles.
- Racine exclue, $\varepsilon=0$, racine virtuelle pour une forêt.
- Étiquettes : plus petit `PointId`, dans l'ordre d'entrée.

**Exactitude.**
- $\varphi$ s'encadre en virgule fixe à partir de la table des racines : $r\in[r_-,r_+]$, donc
  $\varphi\in[r_+^{-z},r_-^{-z}]$, avec une division entière `Big`.
- Les sommes et la décision EOM se font par intervalles entiers.
- En cas de chevauchement, on bascule sur le repli exact : port de `RadSum.sign` et `phi_exact`
  (`bench/points_flat.py:86-307`), c'est-à-dire forme close $1/(\sqrt{t}+\sqrt{m}-\sqrt{q})$ par conjugués, classes de
  carrés et Besicovitch.
- Un $\varphi(0)$ demandé est un `head_invariant` : aucune jonction n'a lieu au niveau 0.
- Mesure : 0 repli exact sur 272 358 décisions EOM LiDAR (lecture `plat`). Le filtre entier doit suffire presque
  toujours.

### 7.8 `num` : entiers, table de racines, sommes de radicaux (tranche S8)

**`num::Big`.**
- Entier signe-magnitude à **capacité fixe** de $16384+1024$ bits, soit 272 mots, sur la pile ou dans un brouillon par
  fil compté au budget.
- Opérations : addition, soustraction, multiplication d'école, division (Knuth D), PGCD binaire, racine entière
  (Newton), test de carré parfait, résidu modulo un petit premier.
- Tout dépassement rend `radical_sign_budget`.
- Au-dessus : `num::Rational` réduit et `num::RadicalSum` à radicandes entiers $N=nd$. Deux classes sont égales si et
  seulement si $N_1N_2$ est un carré parfait ; l'égalité est certifiée si et seulement si tous les coefficients sont
  nuls.

**`num::RootTable`.**
- Pour chaque rang référencé, $R_r=\lfloor 2^{64}\sqrt{\ell_r}\rfloor=\mathrm{isqrt}(\lfloor N\cdot 2^{128}/D\rfloor)$,
  pour $\ell_r=N/D$ non réduit. On a $\sqrt{\ell_r}<2^{B+1}$, puisque le centre est dans l'enveloppe du support, donc
  $R_r<2^{89}$ en u24 : un `u128` suffit.
- Une somme de $j$ racines signées est encadrée à $j\cdot 2^{-64}$ près.
- La décision est prise si 0 est hors de l'intervalle ; sinon, on passe au repli exact.

**Doctrine.**
- Aucune décision flottante (F1, `ARCHITECTURE.md:83`) : les filtres sont des intervalles **entiers**, donc aucune
  règle F7 n'est nécessaire.
- Ajout à `ARCHITECTURE.md` § 3, en S8 : « une expression de longueur variable (somme de radicaux) a un budget déclaré
  avec refus explicite, en plus des `static_assert` de degré ».

### 7.9 Coûts : ce qui est connu et ce qui reste à mesurer

| Étage | Connu | À mesurer sur G4 (trames ng00–02, K5 et K10 ; synthétique 8 000, 16 000, 32 000) |
| --- | --- | --- |
| Domaine (index + $\mathrm{Cat}_K$) | 181–239 ms à W48 (`receipts/qualification_performance_20261003/review/metrics.optimized.json`) | inchangé |
| Forêts FULL 1..5 | 164–241 ms à W48 (même reçu) ; total 412 / 352 / 381 ms après la tranche 3 (`README.md:86-91`) | arbre K seul par lots, contre FULL pipeline (S7), puis pipeline à un ordre (S11) |
| Rattachement | $O((\lvert W_K\rvert+N)\alpha)$ | temps et part du total |
| $\mathcal{Q}_b$ et comptes | négligeable attendu (§ 7.3) | idem |
| Écriture | — | octets par section, $\lvert W_K\rvert$, $S$ |
| Points | Python `margin_r` 3,1–8,4 s | natif |
| Tête | Python environ 60 ms par arbre | natif |

Aucune de ces hypothèses n'est une revendication. Les conclusions de coût se mesurent aux tailles d'intérêt
(`ARCHITECTURE.md:144-145`).

---

## 8. Portes et oracles

### 8.1 Principes

- **La vérité d'abord.** Un oracle **de définition**, exhaustif et borné ($n\leq 12$–$14$), écrit en Python
  bibliothèque standard sur l'étage A (`reference/README.md`), précède tout code natif.
- **À l'échelle**, jamais de juge $O(n^3)$ ni de tableau par paire. On s'appuie sur des invariants globaux, des
  contre-épreuves de registres, des juges d'échantillon, des identités entre voies et des mutants causaux.
- **Python nu.** Toute porte enregistrée tourne en Python 3.10 sans paquet sur G4 : il faut la vérifier sous
  `python3 -S`. Les différentiels qui tirent numpy sont en label `long`, ou en session `python_packages: pinned`.
- **Aucune porte native dans le Codespace** (`README.md:51-52`). La matrice G4 (`tools/g4_matrix.py` : GCC Release
  u21/u24, ASan/UBSan, TSan, mutants) tourne dans une session gardée `gcp-migration/v11_session.py`, cible certifiée
  `TERMINATED`. Chaque tranche a son reçu immuable.
- Seules les aides de `cmake/gates.cmake` sont permises. Codes exacts 0 à 4 (`ARCHITECTURE.md:137-138`). Planchers
  contre le vert par vacuité.

### 8.2 Oracle borné des supports (S1)

`reference/hgp11_ref/supports.py`, sur `definition.py` seul (comme `tests/tower/forest_oracle.py:21-33`), calcule :
- $W_K$ : les MEB des $K$- et $(K+1)$-parties, dédoublonnées par (centre, niveau) exacts, avec $p+q\leq K+1$ ;
- $\mathrm{att}(b)$ par `Definition.node_at` sur **toutes** les $K$-parties de $P_b$ (contrôle direct de T3) ;
- $\mathrm{ant}(b)$ sur **toutes** les $K$-parties strictes à la coupe ouverte, c'est-à-dire au niveau d'événement
  précédent ;
- le rôle, lu sur l'arbre canonique ;
- $\mathcal{Q}_b$ par centre circonscrit égal à $c_b$ et poids barycentriques strictement positifs (formulation de
  Gram, `tests/catalogue/euler_oracle.py:75-114`) ;
- `strict_traces`, `cofaces` et `cofaces` par support **par dénombrement brut** des parties, avec contrôle
  $B(G)=b$ ;
- `components`.

Portes et critères :
- **`mhgp11_reference_supports`** (labels `oracle` et `fast`, avec jumelle `-O`). Toutes les fixtures du § 2.9 et les
  familles de `reference/hgp11_ref/families.py`, pour $K\leq\min(5,n-1)$. Planchers gravés au premier passage : au
  moins 150 nuages, 5 000 boules, 200 coquilles étendues, 50 boules à plusieurs supports, 100 internes, 10
  passagères, 20 fusions d'au moins trois enfants, une coquille étendue avec un support d'arité supérieure à
  $q_{\min}$ ; zéro écart.
- **`mhgp11_reference_supports_mutants`**, code 4 chacun : coupe ouverte pour $\mathrm{att}$, branches à la coupe
  fermée, fenêtre forte, premier support seulement, triangle droit admis, cofaces à $K$ au lieu de $K+1$. Un mutant
  qui limiterait $\mathrm{ant}$ aux traces $I\cup A$ ne changerait rien (lemme C, point 1) : il n'est pas causal et
  n'est pas retenu.

### 8.3 Juges natifs

- **Différentiel borné.** `tests/tower/attach_probe.cpp` et `tests/supports/supports_probe.cpp` produisent un JSON
  canonique (identité des boules par $S^*$ en indices d'entrée et niveau exact), jugé par l'oracle S1. Profils u18,
  u21 et u24.
- **E2** (`tests/tower/attach_judge.cpp`) : descente d'une $K$-partie, puis ancêtre fermé (lemme E). E1 = E2 sur
  toutes les boules à 8 000 points, sur 2 000 boules tirées à graine fixe à 32 000 points et sur les trames.
- **Export validé** (`tests/tower/attach_export_gate.py`, lecteur bibliothèque standard). Les incidences fortes tirées
  de `WindowAttachment` sont **identiques à l'octet** au bloc d'incidences `MHGP11PH` de `mhgp11_points_export`
  (kmax = K, ordres = K) : petits nuages et trames.
- **Juge d'échantillon** (`tests/supports/sample_judge.py`, `Fraction`). 200 boules : $U_b$ recalculé par force brute
  sur les $n$ sites, en $O(n)$ par boule ; $\mathcal{Q}_b$ et comptes ; dénombrement brut si
  $\binom{p+m}{K+1}\leq 20\,000$.

### 8.4 Invariants à l'échelle

Portes `_scale8000`, `_scale16000`, `_scale32000` et `_lidar_ng0{0,1,2}_k5`, plus `_k10` en label `long`.

| # | Invariant |
| --- | --- |
| I1 | Chaque $b\in W_K$, recalculée depuis le catalogue par la fenêtre, apparaît une et une seule fois. Nombre de naissances = `births()` à $K\geq 2$, 0 à $K=1$. |
| I2 | Rangs et rôles (lemme B). |
| I3 | Toute fusion porte au moins une boule de rôle fusion, et l'union de leurs branches égale ses enfants. Rôle interne ⇒ `components` = 1. |
| I4 | Registres de forêt : $\sum S=$ `trace_resolutions` ; $\sum\binom{m}{t}=$ `cells.combinations` ; nombre de cellules = `replayed_cells` ; nombre de rangs distincts des cellules = `plateaus` ; $\sum_r\lvert\bigcup_{\mathrm{rang}\,r}\mathrm{ant}\rvert=$ `touched_components` ; $\sum_r$ (nœuds internes distincts au rang $r$) = `continuations` (`forest.hpp:17-28`, `forest_plateau.cpp:17-24,43,54,78,104-106,134`). |
| I5 | $\mathcal{Q}_b$ non vide ; premier support = $S^*$ ; coquille régulière ⇒ $\lbrace S^*\rbrace$ ; sites dans $U_b$, triés, arité 2 à 4 ; $S_{\mathrm{journal}}=\binom{m}{t}-N_t$ pour toute boule. |
| I6 | `k_parts` $=\binom{p+m}{K}$ ; $\max_Q$ `cofaces`(Q) $\leq$ `cofaces`(b) $\leq\sum_Q$ `cofaces`(Q) ; cas réguliers du lemme G. |
| I7 | T3 sur échantillon : une seconde $K$-partie donne le même $\mathrm{att}$ (E2). |
| I8 | W1, W4 et W48 : fichiers et manifeste identiques à l'octet. |
| I9 | Permutation de l'entrée : fichiers identiques. Réétiquetage non dense (`0xFFFFFFFF` compris) : seule la colonne `point_id` change. |
| I10 | Forêt de `build_order` = `build_full(...).order(K)` (nœuds, enfants, rangs, `birth_key`, champs logiques du registre). E1 = E2. |
| I11 | Listes propres triées par rang ; la première boule propre d'un nœud de `kind` 1 ou 2 a le rang du nœud ; intervalles de sous-arbre emboîtés ; même `tree_k_sha256` que `full` à même K. |

### 8.5 Mutants (manifestes `tests/mutants/<unité>.json`)

Avant toute modification de `tower`, il faut rejouer `mhgp11_mutants_tower_manifest`, pour garder les motifs
présents une seule fois.

| Unité | Mutants | Tués par |
| --- | --- | --- |
| `tower` | `rattachement_coupe_ouverte` (att := u) ; `branches_coupe_fermee` (requête au rang $r$) ; `journal_racine_dsu` (consigne une racine) ; `fenetre_forte` ; `journal_premiere_graine` ; `role_rang_egal_interne` | I1, I3, I4, oracle, passagère, fixture 8 |
| `supports` | `triangle_droit_accepte` ; `drapeau_q4_presentation` ; `arret_premier_support` ; `fermeture_omise` ; `cofaces_ordre_k` ; `boules_propres_decroissantes` ; `coquille_sans_plafond` ; `supports_tri_pointid` | fixtures 2, 9, 1, 13 ; I5, I6, I9, I11 ; oracle |
| `io` | `manifeste_avant_donnees` ; `pending_non_retire` ; `conflit_ignore` ; `renommage_ecrasant` ; `lecture_tronquee_acceptee` ; `sha_tronque` | portes de transaction et de faute |
| `api`, `cli` | `session_sans_liberation` ; `option_hors_sortie_admise` ; `ecriture_full_permutee` | contrat du CLI, identité `full` |
| `num` | `carre_parfait_hors_classe` (défaut `b4632db51`) ; `egalite_non_certifiee` ; `budget_ignore` ; `encadrement_decale` | témoins F5, F6, F14a, $5\sqrt{2}$ |
| `points` | `qualification_decalee` ; `marge_carree` ; `sans_marge` ; `coupe_ouverte` (`bench/points_gate.py:254-300`) ; `m_k1_deux` | fixtures portées, fixture à K1 |
| `head` | les sept mutants de la tête et les deux de points (lecture `plat`, § 5.3) | F5, F6, F8, F14a–e |

### 8.6 `io`, `api`, `cli`

**`io`.**
- `mhgp11_io_unit_*` : quatre raisons.
- `mhgp11_io_sha256` : différentiel contre `hashlib` (tailles 0, 55, 56, 63, 64, 65, puis multiblocs).
- `mhgp11_io_transaction` : refus à chaque étape ; `.pending` orphelin ; entrée sous la sortie, par lien symbolique
  et par lien physique ; `/dev/full` ; `NOREPLACE`.
- `mhgp11_io_fault` : `operator new` remplacé ; aucun dossier publié.

**`api`.**
- `budget_not_released` si un produit survit à la Session.
- F5 sous les quatre modes d'arrondi, et faute injectée qui donne `environment_selftest`.

**`cli`.**
- `mhgp11_cli_contract` : au moins 20 cas de refus, codes 2 et 3, ni dossier ni `.pending`.
- `mhgp11_cli_full_identity` : `sha256` brut égal au dump de `mhgp11_full_bench`. Au moins 400 tentatives, comme
  `attempts427` (`tests/tower/tests.cmake:77-78`) ; $K=1..5$ ; permutations ; W1 et W4 ; u21 et u24.
- Déterminisme, réétiquetage, `--pas` et `--origine` recopiés.

### 8.7 `points` et `plat`

**`points`.**
- Les 12 fixtures de `bench/points_gate.py:140-251`, rejouées en bibliothèque standard.
- Oracle extrait de `bench/points_reference.py` sans numpy, recoupé une fois en `long`. Planchers : 150 nuages, 4 000
  comparaisons, 500 retardés.
- `mhgp11_points_vs_python` (`long`) : identité exacte site par site contre `hang_margin_radius(order, m(K))` sur le
  même `MHGP11PH`, avec les nombres de la session F.
- Fixture à $K=1$ : entrées à 0, liaison simple (H2).
- Invariants : laminarité, propriétaire vivant, $t\leq Q\leq M$.

**`plat`.**
- F14a–e, F5, F6 et F8 en arbres abstraits, avec les égalités certifiées $1/4$, $\sqrt{2}/8$, $7/6$, $7/12$ et l'écart
  $2^{-70}$ recalculés par `bench/points_flat_oracle.py`.
- `mhgp11_head_vs_python` (`long`) : `lot2` (144 arbres) et scènes synthétiques ; z = 1, 2, 3, feuilles ; mcs 10 et 20.
- Antichaîne et raffinement du théorème 9.
- F13 ($T=A$ à $K=1$) rejouée sous $m(1)=1$.

### 8.8 G4 et reçus

Chaque tranche native se termine par :
- une session gardée, matrice complète, mutants ;
- un reçu `receipts/developpement_<AAAAMMJJ>/<tranche>/` (lu en mode normal et sous `-O`) ;
- une note dans `audits/` pour les deux auditeurs.

Sessions de 4 200 s au plus, cible épinglée, arrêt `TERMINATED` certifié (`CLAUDE.md`).

---

## 9. Tranches

| Tranche | But | Dépend de | En parallèle |
| --- | --- | --- | --- |
| **S0** contrat | Documents seulement : § 10 de `MATHEMATIQUES.md` (lemmes A–G), `docs/SORTIES.md`, table des modules et sa copie CMake, `ARCHITECTURE.md:48,50` et § 7.2, `PROVENANCE.md`, registre des preuves, réponse à l'audit `1bf4be68f` | — | non |
| **S1** oracle | Vérité bornée des supports, fixtures, mutants de l'oracle | S0 | oui (S2, S4, S8) |
| **S2** tour publique | `meb.hpp`, `tower.hpp` parapluie ; aucun octet changé | S0 | oui (S1, S4, S8) |
| **S3** arbre K + rattachement | `build_order` (voie par lots), journal, `WindowAttachment`, juge E2, différentiel d'export | S1, S2 | oui (S4, S5, S8) |
| **S4** io | Port R2, SHA-256, transaction de dossier, quatre raisons | S0 | oui |
| **S5** api + cli `full` | Session (F5), `compute(full)`, `MHGP11FUL1` transactionnel, CLI à `--sortie=full` | S2, S4 | oui (S3, S6, S8) |
| **S6** module supports | $\mathcal{Q}_b$, comptes, assemblage ; deux raisons | S1, S3 | oui (S5, S8) |
| **S7** sortie supports | `MHGP11SP`, lecteur, `--sortie=supports`, échelle, LiDAR, **mesure G4** (arbre K par lots contre FULL) | S5, S6 | non |
| **S8** num | `Big`, `RootTable`, `RadicalSum` ; `radical_sign_budget` | S0 | oui |
| **S9** points | $H^{r}_{K+1}$ natif, `MHGP11PT`, `--sortie=points` ; alignement de $m(1)$ | S3, S7, S8 | non (fichiers `api` et `cli` partagés avec S7) |
| **S10** plat | Tête native, `MHGP11ET`, `--sortie=plat` | S8, S9 | non |
| **S11** pipeline à un ordre | Ensemble d'ordres sans verticales dans `build_concurrent` et `pipeline_orders` ; naissances par blocs à $K\geq 2$ ; table de populations filtrée à $p+m=K$ avec liaison clairsemée ; identité et mesure G4 ; changement de défaut seulement sur reçu | S3, S7 | oui (S8–S10), en coordination avec le chantier 100 ms |

### 9.1 Détail : fichiers, portes, critères d'acceptation

Chemins relatifs à `morsehgp3D_v11/`, sauf mention. Une porte est acceptée si son code de sortie est exact (0, ou 4
pour un mutant) et si ses planchers sont gravés et atteints. Toute tranche native se clôt par la matrice G4 et un
reçu immuable.

**S0 — contrat.**
- Fichiers :
  - `docs/MATHEMATIQUES.md` (§ 10 : $W_K$, $\mathrm{att}$, $\mathrm{ant}$, rôles, $\mathcal{Q}_b$, comptes,
    $P_v$, instantanés, invariance, lemmes A–G avec leurs preuves) ;
  - `docs/SORTIES.md` (nouveau : CLI, refus, formats, manifeste, transaction) ;
  - `docs/ARCHITECTURE.md` (table, `:48`, `:50`, § 7.2) ;
  - `cmake/modules.cmake` ;
  - `docs/PROVENANCE.md` (ports annoncés) ;
  - `docs/HIERARCHIE_POINTS.md` ($m(K)$) ;
  - `README.md` ;
  - `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md` (racine) ;
  - `audits/REPONSE_CLAUDE_SUPPORTS_20261004.md`.
- Portes :
  - `python3 tools/check_style.py --root morsehgp3D_v11` (`[table]`, `[module]`) ;
  - `python tools/check_docs.py` (racine) ;
  - réponse des auditeurs avant S3.

**S1 — oracle.**
- Fichiers : `reference/hgp11_ref/supports.py`, `reference/test_supports.py`, `reference/ref_mutants.py`,
  `reference/tests.cmake`, `reference/README.md`.
- Portes :
  - `mhgp11_reference_supports`, avec sa jumelle `-O` (planchers du § 8.2, zéro écart) ;
  - `mhgp11_reference_supports_mutants` (six mutants, code 4).
- Exécution locale sous `python3 -S -B`.

**S2 — tour publique.**
- Fichiers : `src/tower/meb.hpp` (nouveau), `src/tower/tower.hpp`, `src/tower/cells.hpp`, `src/tower/locate.hpp`,
  `src/tower/census_slots.hpp`, `src/tower/meb.cpp`, `tests/tower/public_header_test.cpp`, `tests/tower/tests.cmake`.
- Portes :
  - toutes les portes `tower` existantes, inchangées ;
  - ligne `attempts427 successes413 refusals14` de `mhgp11_tower_full_bench_io`, inchangée ;
  - dumps `MHGP11FUL1` identiques par leur sha256 brut ;
  - `mhgp11_mutants_tower_manifest` ;
  - `mhgp11_style` (`[inclusion]`, `[dependance]`) ;
  - `mhgp11_tower_public_header`.

**S3 — arbre K et rattachement.**
- Fichiers :
  - `src/tower/order_tree.hpp`, `src/tower/order_tree.cpp`, `src/tower/attachment.cpp` ;
  - `src/tower/forest_internal.hpp`, `src/tower/forest_plateau.cpp`, `src/tower/tower.hpp`, `src/tower/module.cmake` ;
  - `tests/tower/order_tree_test.cpp`, `order_tree_fault.cpp`, `attach_probe.cpp`, `attach_judge.cpp`,
    `attach_oracle.py`, `attach_export_gate.py`, `attach_scale.py`, `tests.cmake` ;
  - `tests/mutants/tower.json`, `docs/PROVENANCE.md`, `docs/FULL_FORESTS.md`.
- Portes :
  - `mhgp11_tower_order_identity` (K = 1..5 ; W1, W2, W4 ; u18, u21, u24), plus `_scale8000/16000/32000` et
    `_lidar_ng0{0,1,2}_k5` ;
  - `mhgp11_tower_attach_fraction`, contre S1 ;
  - `mhgp11_tower_attach_e1e2` ;
  - `mhgp11_tower_attach_export` : incidences fortes égales à l'octet au bloc `MHGP11PH` ;
  - `mhgp11_tower_attach_scale*` (I1 à I4, I10) ;
  - `mhgp11_tower_order_fault` ;
  - six mutants `tower`.

**S4 — io.**
- Fichiers : `src/io/io.hpp`, `module.cmake`, `input.cpp`, `sha256.cpp`, `writer.cpp`, `output_dir.cpp`, `json.cpp` ;
  `tests/io/tests.cmake`, `io_test.cpp`, `io_fault.cpp`, `io_probe.cpp`, `sha256_gate.py`, `transaction_gate.py` ;
  `tests/mutants/io.json` ; `docs/PROVENANCE.md`.
- Portes :
  - `mhgp11_io_unit_*` (quatre raisons) ;
  - `mhgp11_io_sha256` ;
  - `mhgp11_io_transaction` ;
  - `mhgp11_io_fault` ;
  - six mutants ;
  - `mhgp11_style` (`[raison_morte]`, `[raison_sans_porte]`).

**S5 — api et cli `full`.**
- Fichiers :
  - `src/api/api.hpp`, `module.cmake`, `session.cpp`, `selftest.cpp`, `compute.cpp`, `write_full.cpp`,
    `manifest.cpp` ;
  - `src/core/reasons.def` (`environment_selftest`) ;
  - `cli/cli.cmake`, `cli/mhgp11.cpp`, `bench/mhgp11_formats.py` ;
  - `tests/api/tests.cmake`, `session_test.cpp`, `selftest_fault.cpp` ;
  - `tests/cli/tests.cmake`, `cli_contract.py`, `cli_full_identity.py` ;
  - `tests/mutants/api.json`, `tests/mutants/cli.json` ;
  - `docs/SORTIES.md`, `docs/PROVENANCE.md`, `README.md`.
- Portes :
  - `mhgp11_api_session_*` ;
  - `mhgp11_api_selftest` (quatre modes d'arrondi, faute → code 3) ;
  - `mhgp11_cli_contract` (au moins 20 refus, ni dossier ni `.pending`) ;
  - `mhgp11_cli_full_identity` (au moins 400 tentatives, sha256 brut égal à `mhgp11_full_bench`) ;
  - `_determinism` et `_relabel` ;
  - mutants `api` et `cli`.

**S6 — module supports.**
- Fichiers :
  - `src/supports/supports.hpp`, `module.cmake`, `enumerate.cpp`, `counts.hpp`, `hierarchy.cpp` ;
  - `src/core/reasons.def` (`support_shell_capacity`, `supports_invariant`) ;
  - `tests/supports/tests.cmake`, `supports_test.cpp`, `supports_probe.cpp`, `supports_oracle.py`,
    `sample_judge.py`, `scale.py` ;
  - `tests/mutants/supports.json`, `docs/PROVENANCE.md`.
- Portes :
  - `mhgp11_supports_unit_*` ;
  - `mhgp11_supports_fraction`, contre S1 ;
  - `mhgp11_supports_shell_capacity` (`sphere50` : code 2, aucun résultat) ;
  - `mhgp11_supports_sample_judge` ;
  - `mhgp11_supports_scale*` et `_lidar_*` (I1–I6, I11, contre-épreuve $S$) ;
  - huit mutants.

**S7 — sortie supports.**
- Fichiers :
  - `src/api/write_supports.cpp`, `src/api/compute.cpp`, `src/api/api.hpp`, `cli/mhgp11.cpp` ;
  - `bench/mhgp11_formats.py`, `bench/sorties_g4.py` ;
  - `tests/api/supports_format.py`, `tests/cli/cli_supports_oracle.py`, `tests/cli/cli_supports_scale.py`,
    `tests/cli/tests.cmake`, `tests/mutants/cli.json` ;
  - `docs/SORTIES.md`, `README.md`, `receipts/developpement_<date>/sortie_supports/`.
- Portes :
  - `mhgp11_cli_supports_oracle` (au moins 150 nuages) ;
  - `mhgp11_api_supports_format` (arbre égal à l'ordre K de `FUL1`, `tree_k_sha256` égal) ;
  - `_determinism` (W1, W4, W48) et `_relabel` ;
  - `_scale*` et `_lidar_*` ;
  - session G4 de mesure (au moins trois prises, sorties identiques entre prises) et reçu.

**S8 — num.**
- Fichiers :
  - `src/num/big.hpp`, `big.cpp`, `rational.hpp`, `roots.hpp`, `roots.cpp`, `radical.hpp`, `radical.cpp`,
    `num.hpp`, `module.cmake` ;
  - `src/core/reasons.def` (`radical_sign_budget`) ;
  - `tests/num/big_test.cpp`, `big_probe.cpp`, `big_gate.py`, `radical_gate.py`, `tests.cmake` ;
  - `tests/mutants/num.json`, `docs/ARCHITECTURE.md` (§ 3), `docs/PROVENANCE.md`.
- Portes :
  - `mhgp11_num_big` (au moins 20 000 opérations contre l'`int` de Python, refus à la capacité) ;
  - `mhgp11_num_roots` ;
  - `mhgp11_num_radical` (témoins du § 8.7) ;
  - quatre mutants.

**S9 — points.**
- Fichiers :
  - `src/tower/ancestor_index.hpp`, `src/tower/ancestor_index.cpp` ;
  - `src/points/points.hpp`, `module.cmake`, `incidences.cpp`, `qualify.cpp`, `hang.cpp`, `point_tree.cpp` ;
  - `src/core/reasons.def` (`points_invariant`) ;
  - `src/api/write_points.cpp`, `src/api/compute.cpp`, `cli/mhgp11.cpp`, `bench/mhgp11_formats.py` ;
  - `bench/points_flat_gate.py`, `bench/points_flat_campaign.py` (alignement de $m(1)$) ;
  - `tests/points/tests.cmake`, `points_test.cpp`, `points_probe.cpp`, `points_oracle_stdlib.py`,
    `points_vs_python.py` ;
  - `tests/mutants/points.json`, `docs/HIERARCHIE_POINTS.md`, `docs/PROVENANCE.md`.
- Portes :
  - `mhgp11_points_fixtures` (12 fixtures, plus $K=1$) ;
  - `mhgp11_points_oracle` (150 nuages, 4 000 comparaisons, 500 retardés) ;
  - `mhgp11_points_vs_python` (`long`) ;
  - `mhgp11_points_scale*` ;
  - `mhgp11_cli_points` ;
  - cinq mutants.

**S10 — plat.**
- Fichiers :
  - `src/head/head.hpp`, `module.cmake`, `condense.cpp`, `score.cpp`, `select.cpp` ;
  - `src/core/reasons.def` (`head_invariant`) ;
  - `src/api/write_flat.cpp`, `src/api/compute.cpp`, `cli/mhgp11.cpp`, `bench/mhgp11_formats.py` ;
  - `tests/head/tests.cmake`, `head_fixtures_test.cpp`, `head_vs_python.py` ;
  - `tests/mutants/head.json`, `docs/SORTIE_PLATE.md`, `docs/PROVENANCE.md`.
- Portes :
  - `mhgp11_head_fixtures` ;
  - `mhgp11_head_vs_python` (`long`) ;
  - `mhgp11_head_f13` ;
  - `mhgp11_cli_plat` ;
  - `mhgp11_head_scale*` ;
  - neuf mutants.

**S11 — pipeline à un ordre.**
- Fichiers : `src/tower/forest_concurrent.cpp`, `forest_pipeline.cpp`, `forest_build.cpp`, `forest_internal.hpp`,
  `population_lookup.hpp`, `population_lookup.cpp`, `order_tree.cpp` ; `tests/tower/order_tree_test.cpp`,
  `tests/tower/tests.cmake`, `tests/mutants/tower.json` ; `bench/sorties_g4.py` ;
  `receipts/developpement_<date>/ordre_k_seul/`.
- Portes :
  - identité étendue à `concurrent_orders` (fixtures, 8 000, 16 000, 32 000, trames K5 et K10) ;
  - E1 = E2 sur le pipeline ;
  - portes pipeline existantes vertes, TSan ciblé ;
  - trois nouveaux mutants tués ;
  - mesure G4 appariée et reçu. Le défaut ne change que sur reçu.

**Règles communes.**
- Commits sur `main`, jamais de branche.
- Index partagé avec les auditeurs : vérifier que `git diff --cached` est vide avant `git add`, et jamais
  `git add -A`.
- `reasons.def` : ajout en fin de table, dans l'ordre des livraisons (`reasons.def:6-8`).

---

## 10. Risques et parades

| Risque | Parade |
| --- | --- |
| Conflit avec le chantier 100 ms (`tower`, `catalogue`) | S2 ne change aucun octet ; S3 ne touche que quelques lignes gardées dans `forest_plateau.cpp` et un champ de `ForestBuilder` ; S11 est coordonnée par une note aux auditeurs ; manifeste des mutants rejoué à chaque tranche de la tour. |
| Voie par lots plus lente que le pipeline FULL | Mesure en S7 ; S11 ; aucune revendication de temps. |
| Lire `top` ou une racine pendant un plateau ; fermeture différée | Journal de graines de naissance seulement, balayage après `finish()` ; mutants `journal_racine_dsu` et `rattachement_coupe_ouverte`. |
| Rôle dérivé d'unions effectuées | Interdit : `components` est dédupliqué à la coupe stricte (fixture 8). |
| Explosion en $\binom{m}{4}$ (réseaux) | Plafond 24, refus explicite (fixture 13). |
| Taille de `MHGP11SP` à K10 | Mesurée en S7 ; une version 2 compacte si besoin, sans toucher à la version 1. |
| `PointId = 0xFFFFFFFF` ou non denses | Sites en lignes ; étiquettes `i64` dans l'ordre d'entrée ; porte de réétiquetage. |
| Oracles numpy absents sur G4 (incident `claudequal1`) | Extractions en bibliothèque standard pour les portes `fast`. |
| Défaut de `Big` ou des radicaux | Différentiel contre l'`int` de Python, témoins gravés, capacité fixe, refus. |
| Instabilité du support porteur | Déclarée (§ 2.7) ; aucune garantie de robustesse revendiquée pour le jeton. |
| Arrêt brutal pendant la publication | `.pending` orphelin refusé ; jamais d'écrasement. |
| Données perdues par redémarrage du Codespace | Corpus sous `build/v11-persist/`. |

---

## 11. Décisions prises

**Sortie et CLI**

1. **Exécutable.** Un seul exécutable, `mhgp11` (cible `mhgp11_cli`, `OUTPUT_NAME mhgp11`).
   - Paramètre obligatoire `--sortie=full|supports|points|plat`, une seule valeur par appel.
   - Options nommées en français.
   - Une valeur n'est admise qu'une fois livrée avec ses portes.
2. **Ordre de livraison.** `full` (S5), puis `supports` (S7), puis `points` (S9), puis `plat` (S10). `io` et `num`
   avancent en parallèle.

**Objet « supports »**

3. **Objet.** L'arbre $T_K$, plus la partition de $W_K=\lbrace b\in\mathrm{Cat}_K : p+m\geq K\rbrace$ (naissances,
   fusions, liaisons internes).
   - $P_v$ est l'union sur le sous-arbre.
   - Les boules hors fenêtre sont exclues (question 2).
4. **Rattachement et rôles.**
   - Rattachement à la coupe fermée, après fermeture du plateau.
   - Rôle décidé par les rangs.
   - Branches dédupliquées à la coupe stricte.
   - Les unions DSU effectuées ne sont jamais publiées.
5. **Arbre K seul dès S3.** `build_order` passe par la voie non concurrente existante, sous l'identité avec
   `build_full(...).order(K)`. Le pipeline à un ordre (S11) ne devient le défaut que sur reçu G4.
6. **Rattachement E1 dans le produit.**
   - Journal des graines dans `cell` et `regular_cell`.
   - Un balayage au rang $r_b-1$, puis la règle du parent.
   - E2 (port de `ball_nodes`) reste un juge de test.
7. **Contrôles dans le produit.** Ils rendent `tower_invariant` : unicité de $\mathrm{att}$ (T3), lemme C, I1 et I2.

**Supports et comptes**

8. **$\mathcal{Q}_b$.**
   - Tous les supports positifs minimaux, énumérés sur toute $U_b$.
   - Prédicats stricts de `num`, composés dans `supports` ; `num` reste inchangé.
   - Coquille régulière : $\lbrace S^*\rbrace$ sans calcul.
   - Premier support égal à $S^*$, contrôlé.
9. **Plafond de coquille étendue.** `kMaxShell` = 24 est une constante. Au-delà, l'appel entier est refusé
   (`support_shell_capacity`). Aucune option au CLI.
10. **Comptes publiés.**
    - Par boule : `k_parts`, `strict_traces` (tiré du journal), `cofaces`, `components`.
    - Par support : `cofaces`.
    - Dérivés par le lecteur : `compressed_parts` et les comptes de Gabriel.
    - Les noms sont ceux de l'audit `1bf4be68f`.
11. **Nombre de liaisons par défaut.** `cofaces`, au sens de la thèse (Prop. 5), sous réserve de la question 1.

**Architecture**

12. **Module `supports`.** Il dépend de `tower` et se place entre `tower` et `points`.
    - `tower/tower.hpp` devient l'en-tête parapluie.
    - Le contenu MEB passe dans `tower/meb.hpp`.
    - Aucun code ne quitte `tower_detail`.
13. **Façade `api/api.hpp`.**
    - La `Session` porte un budget unique et un `Pool` unique, avec l'auto-test F5 et la raison `environment_selftest`.
    - Corrections de `ARCHITECTURE.md:48` et `:50`.
14. **`io`.**
    - Port explicite du raccord R2 : lecture `u32le` et `OutputSet`.
    - SHA-256 porté de `morsehgp3d`.
    - Transaction de dossier, renommage `NOREPLACE`, manifeste en dernier, `.pending` orphelin refusé.

**Formats**

15. **`MHGP11FUL1`.** Inchangé à l'octet ; identité avec `mhgp11_full_bench`.
16. **`MHGP11SP` v1.**
    - Nœuds en numérotation canonique.
    - Boules triées par postordre du nœud : $P_v$ et ses instantanés sont des tranches contiguës.
    - Sites donnés comme lignes de `SITES`.
    - Aucune table de niveaux (témoins géométriques).
    - Colonnes alignées.
17. **`MHGP11PT` v1.** Avec `LEVELS` exacts.
18. **`MHGP11ET` v1.** Étiquettes `i64` dans l'ordre d'entrée.
19. **Manifeste.**
    - `ehgp.v11.output.v1`, clés anglaises, sans temps ni nombre de fils.
    - `tree_k_sha256` commun aux sorties.
    - Une ligne JSON sur la sortie standard.

**Moteur et paramètres**

20. **Moteur.**
    - Paramètres qualifiés des sondes : masque 16 379, `leaf_size` 16, `max_leaf` 256.
    - Aucune option de moteur.
    - `--fils` vaut 1 par défaut, `--budget` est illimité par défaut.
    - `--pas` et `--origine` sont recopiés dans le manifeste.

**Points et plat**

21. **Points.**
    - $H^{r}_{K+1}$ porté à l'identique, avec $\kappa=1$.
    - $m=1$ à $K=1$, $K+1$ sinon (`HIERARCHIE_POINTS.md:95-96`) ; la chaîne Python plate est alignée à $K=1$.
    - Incidences fortes tirées du rattachement.
22. **Plat.** Défauts EOM, z = 1, mcs 20. z = 2 est explicite pour le synthétique. Le bras sklearn reste en Python.
23. **Arithmétique.**
    - Aucune décision flottante.
    - Table d'encadrements entiers `u128` des racines par rang, puis repli exact jusqu'à 8 192 bits.
    - `num::Big` à capacité fixe de $16384+1024$ bits.
    - Refus `radical_sign_budget`.
    - Ni règle F7, ni GMP, ni Boost.

**Raisons, portes, périmètre**

24. **Raisons nouvelles**, ajoutées en fin de table dans l'ordre des livraisons : `environment_selftest`,
    `support_shell_capacity`, `supports_invariant`, `radical_sign_budget`, `points_invariant`, `head_invariant`.
25. **Portes.**
    - Oracle de définition borné avant tout code natif.
    - Fixtures de l'audit et de l'arbitrage gravées.
    - Portes en Python nu.
    - Aucun natif dans le Codespace.
    - Matrice G4 et reçu immuable par tranche.
26. **Ce qui ne bouge pas.** Sondes de banc, `MHGP11PH`, `build_full`, catalogue et registre
    `docs/implementation_status.toml`.

## 12. Questions pour l'utilisateur

1. **Sens du « nombre de liaisons ».**
   - La spécification retient celui de la thèse : une liaison est un $K$-simplexe ($K+1$ sites) qui relie ses
     $K$-faces (Prop. 5, p. 86).
   - Avec chaque support $Q$, elle publie `cofaces` $=\binom{p+m-\lvert Q\rvert}{K+1-\lvert Q\rvert}$. Par boule,
     elle publie aussi les liaisons distinctes, les $K$-parties reliées ($\binom{p+m}{K}$, soit $K+1$ à une jonction
     régulière), les traces strictes et les branches réunies.
   - En position générale, `cofaces` vaut 1 à une jonction et 0 à une naissance ; il ne dépasse 1 que sur les coquilles
     cosphériques.

   Est-ce le sens voulu, ou bien « liaison » désigne-t-il les $K$-parties reliées par la boule ? La question n'est pas
   bloquante : tout est publié ou dérivable. Seuls le nom du champ et le poids $a_Q$ par défaut en dépendent.
2. **Périmètre de $P_v$.**
   - $P_v$ ne retient que les boules d'événement de l'ordre K, celles de la fenêtre $p+q-1\leq K\leq p+m$. Elles
     portent toutes les liaisons de Gabriel, les seules qui puissent fusionner (Th. 4, p. 88).
   - Les boules à $p+q>K+1$ n'ajoutent que des liaisons non-Gabriel entre parties déjà connexes. Les inclure exigerait
     un catalogue d'ordre supérieur, contraire à l'invariant d'architecture.

   Confirmez-vous ce périmètre ? Question non bloquante.
