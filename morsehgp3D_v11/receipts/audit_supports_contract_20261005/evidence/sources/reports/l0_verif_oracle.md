# Contre-lecture de l'oracle borné des supports (livraison L0, tranche S1)

4 octobre 2026, 22 h 16 UTC (heure lue par `date -u`). Rôle : contre-lecteur indépendant, clé `v_oracle`.
Worktree relu : `build/v11-impl-l0`, détaché à `f98aeed67`, **sans aucune modification** (`git status` identique avant
et après, aucun `__pycache__`). Mes essais sont dans `/tmp/v11-l0-voracle/`. Mes scripts et sorties sont recopiés,
avec leur `SHA256SUMS`, dans [l0_verif_oracle/](l0_verif_oracle/). **GCP non utilisé.** Aucun build ni test natif,
ni `git add`, commit, stash ou branche.

```text
phase=exploration_v11_hors_registre
backend=cpu_reference
profile=quantized_u21_input_only
public_status=not_claimed
```

**Objet relu.** Les fichiers suivants, relus à ces empreintes (inchangées de 21 h 23 à 22 h 16) :

| Fichier (`morsehgp3D_v11/reference/`) | sha256 |
| --- | --- |
| `hgp11_ref/supports.py` (698 lignes) | `826f3f94ce9eeb66e6ed04edce8266dd70b52498fd929c2ec7f7dbc01538a541` |
| `test_supports.py` (564 lignes) | `4baad9aab31e9649b38e4be99a965fdc40e449c0ef0ee4355d4b5a57a8340c11` |
| `ref_mutants.py` | `3a9e028830bef618d1a42933cb286cb8dace09196cefb06d8036b8aab39d7fe8` |
| `tests.cmake` | `b5b832aebf0582c7f06e4d58530d4c9b56c4a952ad941989293159c8d2137528` |
| `README.md` | `cf476e55694808b6e93f0803083efc8d8a660a1e8b8131aa0a8ee5e6a31ca6a1` |
| `../docs/MATHEMATIQUES.md` (§ 10 de s0_math) | `b53315475600a1b193dfce5cce396cc317ddb7aaf33dcd61104a5f420430beca` |

Autorité suivie : `DECISIONS_UTILISATEUR.md`, puis `CRITIQUE_ET_PLAN_REVISE.md`, puis `SPECIFICATION_FINALE.md`
(§ 2, 8.2, 9.1, 11), puis `MATHEMATIQUES.md` et la thèse (Déf. 21, Prop. 5, Déf. 27–29, Th. 4, Prop. 6, relues dans
le PDF). J'ai aussi lu les deux audits poussés sur `origin/main` **après** la base du worktree : `de4ab58a8`
(20 h 51) et `aef7182b3` (21 h 34). Ils ajoutent le témoin D2 et la garde « q4 du cube à K1 », repris au § 4.

## 0. Verdict

**Aucun bloquant.**

L'oracle calcule l'objet décidé, sous les noms décidés. Mon calcul indépendant, écrit sans `supports.py` ni
`definition.py`, lui est égal champ par champ :
- 108 nuages et 479 ordres, $K$ de 1 à 11 ;
- 6 877 boules, dont 671 à $K\geq 6$ ;
- 139 725 champs comparés à $K\leq 5$ ;
- **zéro écart.**

Le reste de la vérification est conforme lui aussi :
- les portes passent en Python 3.12 et 3.10, en mode normal et sous `-O` ;
- les planchers sont gravés et appliqués ;
- les sept mutants sont tués par leur cause déclarée ;
- aucun attendu du § 2.9 de la spec n'est faux.

À corriger, sans bloquer (détail au § 6) :
1. **Robustesse de la porte.** Une `InvariantError` levée dans un fait supplémentaire ou dans le contrôle d'invariance
   n'est pas rattrapée. La porte sort alors sur une trace Python, au code 1 par coïncidence, sans ligne `ECART` ni
   ligne de synthèse.
2. **Contrôles jamais démontrés vivants.** Sept mutants de mon cru survivent à la porte complète, dont une confusion
   coupe ouverte / coupe fermée qui rend le contrôle W.4 tautologique. J'ai écrit et vérifié trois mutants de vivacité
   à ajouter, tous tués avec le code 4.
3. **Fixtures demandées par l'auditeur, absentes.** Le témoin D2 (`de4ab58a8`), et le fait explicite « q4 du cube
   gardés à $K=1$ ».
4. **Texte de s0_math sur E5 (lemme W.2).** Il ne vaut que pour l'une des deux lectures. Je confirme le constat de
   s1_oracle par un calcul indépendant, et la lecture du graphe de Gabriel de la thèse (Déf. 29) est l'autre.

## 1. Portes rejouées

Tous les temps sont pris sur le codespace partagé, de charge moyenne 5 à 10 sur 8 cœurs.

### 1.1 Porte principale

Elle est lancée depuis `reference/` :

| Interprète | Mode | Code | Réel | CPU |
| --- | --- | ---: | ---: | ---: |
| Python 3.12.1 `-S -B` | normal | 0 | 28,5 s | 28,0 s |
| Python 3.12.1 `-S -B` | `-O` | 0 | 28,5 s | 28,2 s |
| Python 3.10.21 `-S -B` | normal | 0 | 32,1 s | 31,6 s |
| Python 3.10.21 `-S -B` | `-O` | 0 | 27,0 s | 26,2 s |

L'interprète 3.10 est celui de G4 ; je l'ai trouvé dans `/opt/conda/pkgs/python-3.10.21-h267e890_0_cpython/`.

Les quatre sorties sont identiques, octet pour octet (md5 `daae1933c91c23ca97e6199de83afb53`, celui du rapport
s1_oracle), et chaque processus tirait sa propre graine de hachage. Ligne finale :

```text
reference_supports_ok nuages=209 ordres=947 boules=15035 supports=16916 noeuds=12415 coupes=48018
```

### 1.2 Planchers

Ils sont **gravés et appliqués**, pas seulement affichés. Trois copies mutées du harnais (dans `/tmp`) le montrent :

| Mutation du harnais | Code | Sortie |
| --- | ---: | --- |
| famille `cospherical` retirée de la suite | 3 | `PLANCHER : arity2 9224 != 10138, ...` (compteurs exacts) |
| plancher `clouds` porté à 250 | 3 | `PLANCHER : clouds 209 < 250` |
| empreinte de la suite modifiée d'un caractère | 3 | `PLANCHER : empreinte de la suite ... au lieu de ...` |

Ce qui est gravé :
- les huit planchers du § 8.2 de la spec, dans `FLOORS` ;
- 26 compteurs exacts, dans `SUITE_EXACT` ;
- l'empreinte de la suite ;
- les 49 empreintes de fixtures ;
- le nombre de faits jugés, 46.

### 1.3 Mutants de l'oracle

Les sept mutants sortent tous avec le code 4 et la ligne `mutant_killed <nom>`, en 3.12 normal et `-O`, et en 3.10.
Chacun prend moins d'une seconde. Le premier écart porte la cause déclarée :

| Mutant | Premier écart | Cause déclarée |
| --- | --- | --- |
| `att_coupe_ouverte` | `triangle_aigu@2` : lemme A (T3), les 3 $K$-parties donnent {0, 1, 2} | lemme A |
| `ant_coupe_fermee` | lemme C.1 : {0, 1, 2} contre {3} | lemme C |
| `fenetre_forte` | périmètre : liaison de Gabriel (0, 1, 2) de boule hors de $W_K$ | perimetre |
| `premier_support_seul` | `cube@2` : lemme F, [(0, 3)] contre [(0, 3), (1, 2)] | lemme F |
| `triangle_droit_admis` | `triangle_droit@1` : lemme F, [(0, 1, 2), (1, 2)] contre [(1, 2)] | lemme F |
| `cofaces_ordre_k` | `carre@1` : lemme G, `cofaces` 0 par énumération, 1 par formule | lemme G |
| `populations_naissances_seules` | `growth_abcz@3` : lemme H, polyèdre [0, 1, 2, 3], union [0, 1, 2] | lemme H |

La porte de refus `--inject=absent` sort avec le code 2.

Chaque raison est la bonne. `fenetre_forte` perd la boule faible du triangle équilatéral, et sa liaison de Gabriel
tombe hors de $W_K$ : c'est exactement la perte que la spec vise.

### 1.4 CTest

Configuration limitée à l'unité `reference`, sans aucune compilation :

```text
cmake -S morsehgp3D_v11 -B /tmp/v11-l0-voracle/build -DMHGP11_MODULES=reference -DCMAKE_BUILD_TYPE=Release
ctest --test-dir /tmp/v11-l0-voracle/build -LE long -j3
```

Elle enregistre 95 portes. Les 78 portes hors `long` passent, **78 sur 78**, en 135 s réelles sur 3 cœurs.
- `mhgp11_reference_supports` : 49,3 s ; sa jumelle `_opt` : 43,4 s (sous charge).
- Les 16 portes de mutants et de refus, avec leurs jumelles, prennent moins d'une seconde chacune.

Autres contrôles :
- `check_style --root morsehgp3D_v11` donne `style_ok fichiers=424`.
- `check_docs.validate` appliquée à `reference/README.md` donne 0 remarque.

### 1.5 Hygiène

Les fichiers `supports.py`, `test_supports.py` et `ref_mutants.py` sont en ASCII et ne contiennent aucun `assert`.
Seule la bibliothèque standard est importée : `fractions`, `itertools`, `math`, `hashlib`, `json`, `os`, `sys`,
`importlib`, `shutil`, `tempfile`, `types`. `math.comb` existe depuis Python 3.8. `supports.py` n'importe que
`.definition` et `.model`.

## 2. Calcul indépendant

### 2.1 Ce qui est différent de l'oracle

Le module [indep_supports.py](l0_verif_oracle/indep_supports.py) n'importe que la bibliothèque standard. Il ne lit ni
`supports.py`, ni `definition.py`, ni `model.py`. Ses choix diffèrent volontairement de ceux de l'oracle :
- **sphère circonscrite** par les formules fermées du § 2 de `MATHEMATIQUES.md` (produits vectoriels), au lieu du
  système de Gram ;
- **boule minimale** par le critère M1 (centre dans l'enveloppe convexe du support, sphère englobante), recoupée par
  le plus petit rayon englobant ;
- **$\mathcal{Q}_b$** par les prédicats stricts du lemme F.2 (milieu ; triangle strictement aigu et coplanaire au
  centre ; centre strictement intérieur au tétraèdre), au lieu de poids barycentriques ;
- **$\Gamma_K$** balayé par union-recherche, chaque racine portant l'ensemble de ses anciens nœuds. Les coupes
  fermée et ouverte sont lues par remontée directe : niveau du parent $\leq a$, ou $<\lambda_b$.
- **numérotation canonique**, rôles, `ant`, `att` et comptes recalculés depuis les définitions de la spec et du
  § 10.

Le lemme H est vérifié **dans mon propre calcul**, à chaque niveau de sommet ou d'arête :
- le $K$-polyèdre (Déf. 21) égale l'union des $P_b$ ;
- il égale aussi l'union restreinte aux boules fortes ;
- à $K=1$, il égale l'ensemble des feuilles.

La porte de l'oracle juge en outre les niveaux d'entrée $D_k$ ; son compteur `coupes` n'est donc pas comparable au
mien.

### 2.2 Comparaison

[compare.py](l0_verif_oracle/compare.py) sérialise les deux sorties au schéma de `Supports.canonical`, puis les
compare. Pour chaque nœud : niveau, parent, enfants, `kind`, postordre, boules propres, centre de naissance. Pour
chaque boule : nœud, niveau, centre, rôle, $p$, $m$, $q_{\min}$, `components`, `prior`, supports, et les huit comptes.
$\mathrm{att}$ et $\mathrm{ant}$ sont ainsi comparés entièrement : `node` donne $\mathrm{att}$, et `prior` avec
`components` donnent $\mathrm{ant}$.

Les nuages comparés :
- **27 nuages nommés.** Leurs coordonnées sont recopiées de la spec et de l'audit, pas de `test_supports.py` :
  - ceux qu'impose la tâche : carré, ligne, triangle aigu, plateau K5 (tétraèdre de l'audit), cube $\lbrace 0,2\rbrace^3$
    et passagère ;
  - les autres fixtures de la spec : triangle droit, `growth_ABCZ`, triangle équilatéral, octaèdre, cercles $n=4$ et
    $n=1023$ perturbé, ligne 0–4–6–8–12, losanges, $\lbrace 0,2,4\rbrace$ ;
  - E5 et **D2** ;
  - les nuages d'Euler `sphere12`, `cercle12` et `sphere12_centre` ;
  - six familles dégénérées de mon cru : cuboctaèdre et son centre, hexagone et son centre, prisme, deux coquilles
    concentriques, carré et son centre, pentagone doublé.
- **63 nuages tirés** de 5 à 9 points (graine 20261004) : grilles $\lbrace 0,1,2\rbrace^3$ et
  $\lbrace 0,\dots,3\rbrace^3$, plan, sphères entières (avec et sans intérieurs), droite, amas.

| Passage | Nuages | Ordres | Boules | Supports | Nœuds | Champs | Lemme H (couples) | Écarts | Durée |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| $K\leq\min(5,n-1)$, plus les ordres de la spec | 90 | 411 | 6 206 | 7 080 | 4 889 | 139 725 | 13 421 | **0** | 41 s |
| $6\leq K\leq n-1$, $n$ de 9 à 12 ([compare_highk.py](l0_verif_oracle/compare_highk.py)) | 18 | 68 | 671 | — | — | — | — | **0** | 99 s |

Le second passage monte jusqu'à $K=11$, $p=8$ et $m=12$. La variante rapide (33 nuages, 135 ordres) rend le même
résultat sous Python 3.10 `-O`.

Rôles des 6 206 boules du premier passage : 2 682 naissances, 2 487 fusions, 1 037 internes. On y compte aussi
1 341 coquilles étendues et 269 boules à plusieurs supports.

**Non-vacuité.** [sanity.py](l0_verif_oracle/sanity.py) injecte trois fautes dans **mon** calcul :

| Faute injectée | Ordres en écart sur 54 |
| --- | ---: |
| `ant` à la coupe fermée | 45 |
| triangle droit admis | 39 |
| liaisons comptées sur les $K$-parties | 54 |

La comparaison voit donc bien une différence quand il y en a une.

### 2.3 Faits de la spec recalculés par mon seul calcul

Les faits sont dans [facts.out](l0_verif_oracle/facts.out). Ce script ne charge aucun module de l'oracle.

- **Carré.**
  - K1 : quatre côtés de rôle fusion, avec `components` 2 et `kparties_reliees` 2. La diagonale, au niveau 2, est
    interne, avec `kparties_reliees` 4, `cofaces` 2 et (1, 1) par support.
  - K2 : quatre naissances. La diagonale est de rôle fusion, avec `components` 4, `kparties_reliees` 6,
    `strict_traces` 4, `cofaces` 4 et (2, 2) par support.
  - K3 : naissance étendue, avec `kparties_reliees` 4, `cofaces` 1 et (1, 1) par support.
  - K4 : `kparties_reliees` 1 et `cofaces` 0.
- **Triangle droit.** L'hypoténuse est au niveau 25/4, avec $m=3$, $q=2$ et $\mathcal{Q}_b$ réduit à l'hypoténuse.
  - K1 : elle est interne.
  - K2 : elle est de rôle fusion, avec `strict_traces` 2 et `cofaces` 1.
- **`growth_ABCZ` à K3.**
  - Naissance au niveau 16, avec $p=1$ et $m=2$.
  - Au niveau 25 : boule interne, avec $m=4$, `kparties_reliees` 4, `strict_traces` 1, $\mathcal{Q}_b=\lbrace BZ,ACZ\rbrace$,
    `cofaces` 1 et (1, 1) par support.
- **Passagère à K1.** Au niveau 4, la boule de $ac$ a $m=3$, `components` 1 et `prior` [4]. La boule de $cd$ a
  `prior` [3, 4].
- **Triangle aigu à K2.** Fusion à trois enfants au niveau 25/16, avec `strict_traces` 3 et `cofaces` 1.
- **Ligne 0, 1, 2 à K2.** $p=1$, `kparties_reliees` 3, `compressed_parts` 2, `strict_traces` 2, `components` 2 et
  `cofaces` 1.
- **Triangle équilatéral à K2.** Boule faible, fusion à trois enfants au niveau 8/3.
- **Tétraèdre à K5.**
  - Six naissances au niveau 200.
  - Quatre boules de face au niveau 800/3, chacune avec $p=3$, $m=3$, `components` 3, `kparties_reliees` 6,
    `strict_traces` 3 et `cofaces` 1.
  - Une fusion à six enfants.
- **Cube.**
  - K2 : quatre diamètres et deux tétraèdres. La boule est interne, avec `cofaces` 24 et
    (6, 6, 6, 6, 0, 0) par support.
  - K1 : les deux tétraèdres restent dans $\mathcal{Q}_b$ avec 0 coface, ce qui est la garde de l'auditeur.
- **Octaèdre.** Trois diamètres seulement.
- **Cercle $n=4$.**
  - Avant la perturbation : `components` 4, `strict_traces` 4, `cofaces` 4 et (2, 2) par support.
  - Après : `components` 3, `strict_traces` 5, `cofaces` 3 et (2, 1) par support.
  - `kparties_reliees` vaut 6 avant comme après.

**Tous conformes à la spec, et identiques aux attendus gravés dans `CLAIMS`.**

## 3. Cohérence avec la spec, DECISIONS et le § 10 de s0_math

### 3.1 Noms

Les noms de la sortie canonique sont ceux des décisions 4 et 5, de l'audit `1bf4be68f` et du § 10.7 :
- `kparties_reliees`, `compressed_parts`, `strict_traces`, `components` ;
- `cofaces`, et sa forme par support `cofaces_support` ;
- `gabriel_cofaces`, et sa forme par support `gabriel_cofaces_support`.

Le suffixe `_support` est un nom de clé JSON pour la portée « support » du tableau du § 10.7 ; il n'y a pas de
divergence de sens. Les rôles sont `naissance`, `fusion` et `interne`. `prior` n'est publié que pour le rôle fusion,
comme la section `PRIOR`. Aucune population n'est publiée (décision 4) : seuls $p$ et $m$ figurent.

### 3.2 `kparties_reliees`

Il vaut $\binom{p+m}{K}$ dans les deux calculs (décision 5). Son contrôle dans l'oracle (`supports.py:522`) est une
identité combinatoire : il ne peut pas échouer. Ce qui porte le sens, c'est le lemme A, contrôlé à la ligne 421 :
toutes les $K$-parties de $P_b$ ont un même nœud à la coupe fermée.

### 3.3 Définitions

Mon calcul les recode depuis la spec et le § 10, et il retrouve exactement l'oracle :
- $W_K$, des deux côtés de la fenêtre ;
- $\mathrm{att}$ à la coupe fermée, après le plateau ;
- $\mathrm{ant}$ à la coupe ouverte ;
- les rôles lus sur les niveaux ;
- $\mathcal{Q}_b$ énuméré sur toute la coquille ;
- les comptes du lemme G ;
- la numérotation du § 10.2.

Gabriel (Déf. 28) et K-séparant (Déf. 27) suivent bien la thèse :
- un simplexe est de Gabriel si son intérieur ne contient aucun point extérieur, soit $I_{B(G)}\subseteq G$ ;
- un simplexe est K-séparant s'il a des facettes actives dans deux composantes de $\Gamma_K^{<}(\beta(G))$.

### 3.4 Fixtures du § 2.9

Les treize lignes sont conformes, et le § 2.3 les recalcule de façon indépendante. Il n'y a aucun écart à justifier.

Deux limites sont documentées par s1_oracle et me paraissent justes :
- **Fixture 9.** « Premier support = $S^*$ » n'est contrôlé que par l'arité $q$, car l'oracle ignore Morton.
- **Fixture 13.** Seul $m=84$ est constaté ; le refus `support_shell_capacity` est natif.

### 3.5 Lemmes du § 10 contrôlés par l'oracle

**Contrôlés directement :**
- A ;
- B (naissance ; cellule avec $\lvert P_b\rvert\geq K+1$ ; fusion ; vie de l'interne) ;
- C.1, C.2 (dont la règle du parent) et C.3 ;
- F (contre une force brute des parties non séparables minimales, avec M1) ;
- G ;
- H (et sa restriction aux boules fortes) ;
- W.3 et W.4 ;
- le témoin E5 de W.2.

**Pas contrôlés directement :**
- P.1 et P.2 ;
- W.1, qui n'est couvert qu'indirectement par B, C.3 et H ;
- D, sauf la règle du parent ;
- E, qui demande des descentes et revient au juge natif E2 ;
- « une naissance est toujours forte » (§ 10.3) ;
- « `components` ne dépasse pas `strict_traces` » (§ 10.7).

Le mutant équivalent `compteur_fortes_faux` (§ 4) montre toutefois que « naissance implique forte » tient sur toute la
suite.

### 3.6 E5 et le lemme W.2

Je confirme, par un calcul indépendant ([facts.out](l0_verif_oracle/facts.out)), le constat de s1_oracle :

| Graphe | Fusions (niveau, enfants) |
| --- | --- |
| vrai $T_2$ | (162/25, 3), (189/17, 3), (83886/3563, 3) |
| liaisons de $W_2$ seules, **sommet $AC$ gardé** | (162/25, 3), (189/17, 3), (83886/3563, **3**), (24, 2) |
| liaisons de $W_2$ seules, **sommet $AC$ retiré** | (162/25, 3), (189/17, 3), (83886/3563, **2**), (24, 2) |

Le sommet $AC$ est gardé dans la lecture du $K$-graphe de Gabriel de la thèse (Déf. 29). $AC$ y est un sommet, comme
facette du triangle de Gabriel $ABC$ : il est présent dès son niveau propre 33/2 et reste isolé jusqu'à 83886/3563.

Le paragraphe historique du registre (`STATUT_PREUVES_ET_HEURISTIQUES.md:747-749`, « deux unions recouvrantes (0,1,2)
et (0,2,3,4) ») raisonne sur les unions de points, et celles-ci sont identiques dans les deux lectures. Seul le nombre
d'enfants de la fusion les distingue.

La phrase de `MATHEMATIQUES.md:530-531` (« sans elle, elle n'en a que deux ») ne vaut que pour la seconde lecture.
L'entrée nouvelle du registre (l. 1362) dit bien « les sommets et liaisons », mais le lemme W.2 dit « ces
rattachements… les omettre ».

## 4. Trous

### 4.1 Mutants de mon cru

Chaque mutant est appliqué à une copie de `reference/` dans `/tmp`, puis la **porte principale complète** est jouée
([run_mutants.py](l0_verif_oracle/run_mutants.py), [mutants_voracle.out](l0_verif_oracle/mutants_voracle.out)).

| Mutant | Faute | Code | Verdict |
| --- | --- | ---: | --- |
| `w4_coupe_fermee` | dans `_theorem_4`, remontée `<= level` au lieu de `< level` (`supports.py:406`) : la coupe ouverte devient fermée | 0 | **survit** ; W.4 devient une tautologie |
| `h_fortes_omises` | volet « boules fortes » du lemme H retiré (`supports.py:622`) | 0 | **survit** |
| `c3_union_omise` | union des branches d'une fusion non comparée à ses enfants (`supports.py:563`) | 0 | **survit** |
| `regle_parent_omise` | règle du parent jamais évaluée (`supports.py:464`) | 0 | **survit** |
| `b_interne_omis` | vie $[a_v,a_{\mathrm{parent}})$ de l'interne non contrôlée (`supports.py:443`) | 0 | **survit** |
| `f_m1_omis` | « chaque support redonne sa boule » (M1) non contrôlé (`supports.py:479`) | 0 | **survit** |
| `permutation_identite` | permutation identité dans `invariance()` (`test_supports.py:430`) | 0 | **survit** ; le compteur `invariance=69` ne bouge pas |
| `compteur_fortes_faux` | `strong` compté vrai aussi pour toute naissance | 0 | équivalent : une naissance est toujours forte (§ 10.3) |
| `w4_gabriel_juge` | W.4 juge aussi les liaisons de Gabriel | 1 | tué, par **trace Python** (§ 4.2) |
| `h_fortes_etroites` | fortes trop étroites, $p+q\leq K-1$ | 1 | tué, par **trace Python** (§ 4.2) |

**Les mutants qui changent l'objet calculé sont bien gardés.** Je n'en ai trouvé aucun qui survive : les empreintes
des 49 fixtures, l'empreinte de la suite et 26 compteurs exacts couvrent toute la sortie canonique de la suite.

**Le trou est ailleurs : seuls sept contrôles ont une démonstration de vivacité.**

- Un contrôle qui devient tautologique, ou qu'on retire, laisse la porte verte.
- Le cas le plus sérieux est `w4_coupe_fermee`. La confusion entre coupe ouverte et coupe fermée est la faute
  classique du chantier. Ici, elle rend muet le seul contrôle borné de l'énoncé nouveau W.4 (Th. 4 sans position
  générale, `proved_here` au registre).
- `permutation_identite` vide le contrôle d'invariance sans changer son compteur.

**Liveness proposée.** J'ai écrit trois mutants et je les ai joués par le harnais officiel (`--inject`) sur une copie
de `ref_mutants.py`. Tous trois sortent avec le code 4, tués par leur cause :

| Mutant | Fixture | Cause | Écart obtenu |
| --- | --- | --- | --- |
| `w4_gabriel_juge` | `triangle_aigu@2` | lemme W.4 | liaison (0, 1, 2) séparante, branches [0, 1, 2] |
| `h_fortes_etroites` | `growth_abcz@3` | lemme H | polyèdre [0, 1, 2], fortes [] |
| `c3_une_fusion` | `passagere@1` | lemme C.3 | fusion 4, branches [0, 1] contre enfants (0, 1, 2) |

Pour `c3_une_fusion`, la liste `merges` est tronquée à sa première boule (`supports.py:559`). Les motifs exacts sont
au § 6.

### 4.2 Robustesse de la porte

**Les faits supplémentaires ne sont pas protégés.** `main_gate` appelle les trois faits supplémentaires sans
`try` (`test_supports.py:483-485`). `circle_witness` et `e5_window` calculent des ordres complets, avec tous les
contrôles de lemmes.

Une `InvariantError` levée là sort donc par une **trace non rattrapée**. Sur les deux mutants tués ci-dessus, la trace
vient de `circle_witness` (`test_supports.py:388`). Il en résulte :
- un code 1 dû à l'interprète, et non au verdict de la porte ;
- aucune ligne `ECART`, aucune ligne de synthèse ;
- les écarts déjà collectés par `judge_fixtures` ne sont pas imprimés.

**L'invariance non plus.** `invariance()` est appelée hors du `try` de `run_suite` (`test_supports.py:465`) : une
exception lors du calcul permuté donnerait la même trace.

**Gravité.** Le verdict reste un échec, donc rien n'est masqué ; c'est la lisibilité du code de sortie et de l'écart
qui est en jeu.

### 4.3 Familles et domaines absents

**$K\geq 6$ jamais exercé par la porte.**
- La suite s'arrête à $K\leq 5$, alors que le produit vise $K=10$ et que le format borne $K$ à 12.
- Mon passage à $K$ de 6 à 11 (§ 2.2) ne trouve aucun écart, mais ce n'est pas une porte.
- Je soutiens la suite `long` proposée par s1_oracle (question 3) : $n$ de 11 à 12, $K\leq 10$.

**Coquilles de 13 à 24 sites jamais exercées.**
- La suite plafonne à $m=12$ (nuages d'Euler), et `sphere50` ne fait que constater $m=84$.
- La fermeture $N_j$ parcourt $2^{m}$ masques par boule (`supports.py:495`). Elle met donc hors de portée la frontière
  native $m=24$ : $2^{24}$ itérations par boule et par ordre.
- La frontière du plafond n'a donc aucune vérité bornée. Le juge d'échantillon natif (§ 8.3 de la spec) reste seul.
- Un dénombrement par combinaisons jusqu'à $j\leq t+1$ suffirait pour les comptes publiés ; à envisager pour une
  fixture à 24 sites, par exemple les 24 points entiers de $x^{2}+y^{2}+z^{2}=5$, à $K=2$.

**Témoin D2 de l'auditeur absent.** Il est publié à `de4ab58a8` et redemandé à `aef7182b3` (« Ajouter le témoin D2 à
ses fixtures »). Les cinq sites sont $A=(2,10)$, $B=(18,10)$, $C=(10,20)$, $Z=(9,3)$, $W=(11,3)$, à $K=2$.

Mon calcul et l'oracle donnent :
- la boule $ABC$ au niveau 1681/25, de rôle fusion, `components` 3 ;
- ses branches : la naissance $AC$ (41), la naissance $CB$ (41) et la fusion de niveau 65/2 ;
- cette dernière branche n'est atteinte que par la trace stricte $AB$, de niveau 64, née **après** le niveau
  précédent de $\mathrm{Cat}_2$, qui vaut 41.

D2 est aussi un **second témoin minimal du lemme W.2**. Sans la boule de $AB$ ($p=2$, hors fenêtre), le graphe
restreint ajoute une fusion (145/2, 2), et la fusion de 1681/25 n'a plus les mêmes enfants. Comme E5, c'est donc un
candidat minimal pour tuer le mutant « graine résolue dans le seul $W_K$ » que suggère s0_math. Je ne l'ai pas joué.

Le phénomène existe déjà dans la suite : 49 cellules à $K\leq 3$, dans neuf familles
([d2_frequency.py](l0_verif_oracle/d2_frequency.py)). Il n'y est porté que par des empreintes. Il manque le témoin
nommé.

**Garde « q4 du cube à $K=1$ » de l'auditeur** (`de4ab58a8`, revue `qb`). Elle n'est gravée que dans l'empreinte de
`cube`, sans fait explicite. Mon calcul la confirme : deux tétraèdres, `cofaces_support` 0.

**Ce qui est déjà bien couvert.**
- Grilles, plan, droite, amas, cocycliques et cosphériques avec intérieurs.
- Centre de boule égal à un site (`sphere12_centre`, `tetra_center`).
- $K=n$ (carré à $K=4$).
- Profil u21 (cercle $n=1023$).

Mes six familles dégénérées supplémentaires n'ont révélé aucun écart.

### 4.4 Assert et dépendances

Aucun `assert`. Aucune dépendance hors de la bibliothèque standard. Les fichiers sont en ASCII. Les sorties sont
identiques sous `-O`.

## 5. Réponses aux questions ouvertes

**À s1_oracle.**
1. **E5.** Les deux lectures sont réelles, et je les reproduis. La phrase de s0_math doit nommer la sienne (§ 6,
   point 5).
2. **Morton.** Je recommande comme vous de retrier le différentiel par (postordre, niveau, centre). En plus,
   l'identité de $S^*$ et l'ordre des supports doivent être jugés côté natif : trier les supports de l'oracle,
   traduits en lignes de `SITES`, par (arité, `SiteIdx` lexicographique), puis comparer. L'oracle ne contrôle que
   l'arité du premier support.
3. **Suite longue.** Oui, voir le § 4.3.

**À s0_math.**
1. **E5 comme fixture permanente.** Oui. L'oracle grave l'empreinte `e5`, à tous les ordres de 1 à 4, et le fait
   `e5_window` dans ses deux lectures.
2. **Pas de filtrage de $\Gamma_K$ à $W_K$.** L'oracle lit bien le vrai $\Gamma_K$ (étage A). D2 tue un tel filtrage
   en cinq points, comme E5.
3. Sur la stabilité de `kparties_reliees`, je suis d'accord avec le § 10.9 et avec l'auditeur (D.1) : il est
   indépendant de $\mathcal{Q}_b$ **à $(p,m,K)$ fixés** seulement.

   Remarque connexe : le compteur `kparties` de la suite (54 604) est une somme d'incidences $(b,F)$, pas un nombre de
   $K$-parties distinctes (D.1). Il serait utile de le dire dans le README.

## 6. À corriger (non bloquant), avec la correction attendue

1. **`test_supports.py:483-485`.**
   - Entourer chaque `fact(STAGE)` d'un `try/except Exception`, qui ajoute `'<nom du fait> : <type> : <message>'` à
     `facts`.
   - Faire de même pour `invariance(...)` à la ligne 465, ou l'appeler dans le `try` de la ligne 452.
   - Effet attendu : une exception de lemme devient un `ECART` compté, et la ligne de synthèse est imprimée.
2. **`ref_mutants.py:207-242`.** Ajouter trois mutants de vivacité, vérifiés sur copie (code 4, cause présente) :
   - `w4_gabriel_juge` : `' ' * 12 + 'if mask_of(every[(center, level)].inner) & ~mask_of(g) == 0:\n' + ' ' * 16 + 'continue\n'` devient `' ' * 12 + 'if False:\n' + ' ' * 16 + 'continue\n'` ; fixtures `['triangle_aigu@2', 'carre@1']` ; cause `'lemme W.4'`.
   - `h_fortes_etroites` : `S8 + 'ball.strong = ball.p + ball.q <= k <= ball.p + ball.m\n'` devient `S8 + 'ball.strong = ball.p + ball.q <= k - 1 <= ball.p + ball.m\n'` ; fixtures `['growth_abcz@3', 'carre@3']` ; cause `'lemme H'`.
   - `c3_une_fusion` : `' ' * 16 + 'merges = [b for b in mine if b.role == ROLE_MERGE]\n'` devient `' ' * 16 + 'merges = [b for b in mine if b.role == ROLE_MERGE][:1]\n'` ; fixture `['passagere@1']` ; cause `'lemme C.3'`.

   Mettre à jour en conséquence la table du README : ses « (7) » portes de mutants deviennent 10. `tests.cmake` n'est
   pas à toucher, puisqu'il lit la liste à la configuration.
3. **`test_supports.py:426-440`.** Garantir une permutation non triviale : retirer tant que `order` est l'identité,
   ou rejeter la permutation identité. Puis graver un compteur des nuages réellement permutés.
4. **`test_supports.py:57-90` et `CLAIMS`.**
   - Ajouter la fixture `d2_audit`, `_plane([(2, 10), (18, 10), (10, 20), (9, 3), (11, 3)])`, aux ordres (2,).
   - Fait à graver à $K=2$ :
     - l'arbre `[('1', 0), ('49/2', 0), ('49/2', 0), ('41', 0), ('41', 0), ('65/2', 3), ('1681/25', 3)]` ;
     - la boule `1681/25` de rôle fusion, `components` 3, `prior` [3, 4, 5].
   - Ajouter aussi le fait `cube@1` : boule de niveau 3, supports `DIAMETERS + TETRAHEDRA`, `cofaces_support`
     [1, 1, 1, 1, 0, 0].
   - Les compteurs exacts, les empreintes et la ligne gravée (`tests.cmake:78-79`) sont à regraver au passage.
5. **`docs/MATHEMATIQUES.md:526-531` et `:855`, registre `STATUT_PREUVES_ET_HEURISTIQUES.md:1362`.** Nommer la lecture
   de E5. Formulation proposée :

   > Sans cette boule (son sommet $AC$ et ses liaisons $ACD$, $ACE$), la fusion de niveau $83886/3563$ n'a plus que
   > deux enfants. Si l'on garde le sommet $AC$ isolé, comme le $K$-graphe de Gabriel de la thèse (Déf. 29), elle en
   > garde trois, mais $\lbrace AB\rbrace$, $\lbrace AC\rbrace$, $\lbrace BC\rbrace$ au lieu de
   > $\lbrace AB\rbrace$, $\lbrace AC,AD,AE,CD,CE,DE\rbrace$, $\lbrace BC\rbrace$. Dans les deux lectures, une fusion
   > à deux enfants apparaît au niveau 24.

   Fichiers de s0_math, à corriger par lui ou à la consolidation.

### Facultatif

Contrôles directs bon marché des énoncés nouveaux de s0_math qui ne le sont pas encore :
- P.1 : une arête de même niveau qu'un sommet neuf a la même boule ;
- P.2 : les $K$-parties strictes d'une boule hors fenêtre forment une seule composante ouverte et couvrent ses sites ;
- « naissance implique forte » ;
- « `components` ne dépasse pas `strict_traces` » ;
- l'invariance par translation entière du § 10.10.

## 7. Ce que cette contre-lecture n'établit pas

- Aucune propriété du moteur natif, ni coût, ni échelle. Les tailles d'intérêt (8 000 à 32 000 points) relèvent des
  juges natifs.
- Mon calcul suit les mêmes définitions écrites (spec, § 10) que l'oracle. Un accord prouve que les deux codes
  réalisent ces définitions ; il ne prouve pas les définitions. Pour celles-ci, je n'ai relu que Déf. 21, Prop. 5,
  Déf. 27–29, Th. 4 et Prop. 6 dans le PDF.
- Ni la frontière $m=24$, ni $K\geq 12$, ni les entrées pondérées.
- Aucun statut public n'est promu : `public_status=not_claimed`.

## 8. Rejeu

Depuis [l0_verif_oracle/](l0_verif_oracle/). L'inventaire est dans `SHA256SUMS` (25 fichiers). Les scripts lisent le
worktree en lecture seule et écrivent seulement dans `/tmp/v11-l0-voracle/` :

```text
python3 -S -B compare.py          # 90 nuages, K <= 5 : compare ... mismatches=0
python3 -S -B compare_highk.py    # 18 nuages, 6 <= K <= n - 1 : mismatches=0
python3 -S -B facts.py            # faits de la spec, calcul independant seul
python3 -S -B sanity.py           # trois fautes injectees dans le calcul independant : ecarts detectes
python3 -S -B d2_frequency.py     # 49 cellules de type D2 dans la suite (K <= 3)
python3 -S -B run_mutants.py      # dix mutants de mon cru, porte complete sur copie (environ 5 min)
```
