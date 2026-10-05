# Tranche S1 (livraison L0) : oracle borné des supports d'ordre K

4 octobre 2026, 21 h 27 UTC (heure lue par `date -u`). Rôle : implémentation de la tranche S1, clé `s1_oracle`.
Worktree `build/v11-impl-l0`, détaché à `f98aeed67`. Rien n'est commité : ni `git add`, ni commit, ni branche.
**GCP non utilisé.** Aucun build ni test natif C++.

```text
phase=exploration_v11_hors_registre
backend=cpu_reference
profile=quantized_u21_input_only
public_status=not_claimed
```

Autorité suivie : `DECISIONS_UTILISATEUR.md`, puis `CRITIQUE_ET_PLAN_REVISE.md`, puis `SPECIFICATION_FINALE.md`
(§ 2, 6, 8.2, 9.1 S1, 11), puis `docs/MATHEMATIQUES.md`, la thèse (Déf. 21, Prop. 5, Déf. 28, Th. 4, Prop. 6) et
les gardes de l'audit `1bf4be68f`. Pendant le travail, l'agent S0 a écrit dans le même worktree le § 10 de
`docs/MATHEMATIQUES.md` (lemmes A, P, W, B à H). Je l'ai lu sans le modifier, et l'oracle contrôle aussi ses
énoncés nouveaux (§ 3).

## 1. Résultat

Tout est vert. L'oracle est écrit et mis en porte. Les 13 fixtures du § 2.9 de la spécification et celles de l'audit
sont recalculées et gravées. **Aucun attendu du § 2.9 n'est faux.**

Un seul écart a été trouvé, et il porte sur le texte de S0, pas sur la spécification. C'est le témoin E5 du
lemme W, point 2 : sa phrase « elle n'en a que deux » ne vaut que pour l'une des deux lectures de « l'omettre »
(§ 4.3).

Ligne gravée de la porte :

```text
reference_supports_ok nuages=209 ordres=947 boules=15035 supports=16916 noeuds=12415 coupes=48018
```

Les compteurs dépassent tous les planchers de la spécification (§ 5). Il n'y a aucun écart. Les sorties sont
identiques sous Python 3.12 et 3.10, en mode normal et en `-O`, et sous deux graines de hachage. Les 7 mutants sont
tués, chacun par sa cause. Les 78 portes non `long` de l'unité `reference` passent sous CTest, dont les 18 nouvelles.

## 2. Fichiers

Tous les chemins sont relatifs à `build/v11-impl-l0/morsehgp3D_v11/reference/`.

| Fichier | Nature |
| --- | --- |
| `hgp11_ref/supports.py` | **Nouveau**, 698 lignes. C'est l'oracle (classe `Supports`). Il n'importe que `.definition` et `.model`. |
| `test_supports.py` | **Nouveau**, 564 lignes. Il contient la porte, les fixtures, les faits gravés, les empreintes, les compteurs exacts, les mutants et `--dump`. |
| `ref_mutants.py` | Ajouts : la table `SUPPORT_MUTANTS` (7 entrées), `load_private` et `load_supports`. `MUTANTS` et `DUMP_MUTANTS` sont inchangées. |
| `tests.cmake` | Enregistre `mhgp11_reference_supports`, `_refusal` et `_mutant_<nom>` (7), chacune avec sa jumelle `_opt`. |
| `README.md` | Ajoute la section « Oracle borné des supports d'ordre K », les lignes de la table des portes et l'exemple d'usage. |

Les fichiers Python sont en ASCII pur, comme l'exige `test_ref.py::test_python_310_syntax`, et sans `assert`.

Les deux contrôles de style restent tels quels :
- `check_style --root morsehgp3D_v11` donne `style_ok fichiers=424` ;
- la sortie de `python3 tools/check_docs.py` est identique, octet pour octet, à la ligne de base relevée avant toute
  modification (156 lignes, liens morts du checkout partiel).

## 3. Ce que l'oracle calcule et contrôle

La porte charge `model.py`, `definition.py`, `supports.py` et `families.py` **sans le paquet**. Elle n'utilise donc
ni `__init__`, ni `constructive`, ni `judge`, ni `dumps`. Elle passe par `ref_mutants.load_private`, sur le modèle
de `tests/tower/forest_oracle.py`.

L'entrée est un ensemble de positions distinctes ; un doublon est refusé par `ValueError`.

### Les objets

- **$W_K$.**
  - Ce sont les MEB (`Definition.meb`) des $K$- et des $(K+1)$-parties, de niveau positif, dédoublonnées par
    (centre, niveau) exacts et retenues si $p+q\leq K+1$.
  - $I_b$ et $U_b$ sont calculés par distances entières exactes, puis recoupés avec le masque fermé de l'étage A.
- **$\mathcal{Q}_b$.**
  - On parcourt toutes les parties de $U_b$ de 2 à 4 sites. Une partie est retenue si son système de Gram n'est pas
    singulier, si $c_b$ se reconstruit exactement et si ses poids sont tous strictement positifs (`Fraction`).
  - $q$ est l'arité du premier support, dans l'ordre (arité, coordonnées).
  - La géométrie et $\mathcal{Q}_b$ sont mis en cache par boule, pour tous les ordres.
- **$\mathrm{att}(b)$.** On applique `Definition.node_at` à la coupe fermée $\lambda_b$ sur **toutes** les
  $K$-parties de $P_b$.
- **$\mathrm{ant}(b)$.** On applique `node_at` au niveau d'événement précédent (la coupe ouverte), sur toutes les
  $K$-parties strictes.
- **Rôle.** Une boule sans $K$-partie stricte est une naissance. Sinon, c'est une fusion si
  $a_{\mathrm{att}(b)}=\lambda_b$, et une boule interne dans le cas contraire.
- **Comptes.** Ils sont tous obtenus par énumération brute des parties de $P_b$ :
  - `kparties_reliees` et `compressed_parts` ;
  - `strict_traces`, à partir des traces $I_b\cup A$ et du test strict de l'étage A ;
  - `cofaces` par boule et par support, en contrôlant $B(G)=b$ sur chaque $(K+1)$-partie ;
  - `components` ;
  - `gabriel_cofaces`, par boule et par support.

### Les contrôles

Un contrôle violé lève `InvariantError`, et la porte compte un écart.

| Contrôle | Contenu |
| --- | --- |
| Lemme A (T3) | Toutes les $K$-parties de $P_b$ donnent le même nœud. |
| Lemme B | Une naissance a même niveau et même centre que son nœud. Une naissance n'existe jamais à $K=1$. Une cellule vérifie $\lvert P_b\rvert\geq K+1$. Une fusion est rattachée à une fusion. Une boule interne est rattachée dans $[a_v,a_{\mathrm{parent}(v)})$. |
| Lemme C.1 | Les branches des traces comprimées $I_b\cup A$ sont égales à $\mathrm{ant}(b)$. |
| Lemme C.2 | Interne : $\mathrm{ant}=\lbrace\mathrm{att}\rbrace$. Fusion : $\mathrm{ant}$ est contenu dans les enfants. **Règle du parent** (lemme D, ajout de S0) : $\mathrm{att}(b)=\mathrm{parent}(u)$ si $a_{\mathrm{parent}(u)}=\lambda_b$, et $u$ sinon, pour tout $u$. |
| Lemme C.3 | Toute fusion porte une boule de rôle fusion, et la réunion des branches de ces boules égale ses enfants. |
| Naissances | À $K\geq 2$, chaque naissance porte exactement une boule de naissance, de même centre. À $K=1$, une feuille ne porte aucune boule. |
| Listes propres | La première boule a le niveau du nœud. Au-delà, il n'y a que des boules internes. |
| Lemme F | $\mathcal{Q}_b$ (Gram) est égal aux parties **non séparables minimales** de $U_b$, trouvées par force brute sans borne d'arité. La séparabilité se lit sur $\beta(A)<\lambda_b$ (M1, T2). En outre, chaque support redonne sa boule (M1), $2\leq q\leq 4$, et une coquille régulière donne $\lbrace U_b\rbrace$. |
| Lemme G | Toutes les formules du tableau. Contrôle M2 : une $(K+1)$-partie qui contient un support a pour boule $b$. On vérifie aussi $\max_Q\leq$ `cofaces` $\leq\sum_Q$, avec égalité à droite si et seulement si aucune liaison ne contient deux supports. Le rôle naissance équivaut à `strict_traces` nul, et les cas réguliers (jonction, naissance) sont contrôlés. |
| Lemme W.3 | Toute liaison de Gabriel a sa boule dans $W_K$. Leur nombre égale $\sum$ `gabriel_cofaces`. |
| Lemme W.4 | Une liaison non Gabriel n'est jamais séparante (Th. 4 sans position générale). La coupe ouverte est lue par une remontée propre, indépendante de `node_at`. |
| Lemme H | Voir le détail ci-dessous. |
| Instantanés | Les boules du sous-arbre forment une tranche contiguë dans l'ordre canonique. Celles du sous-arbre strict sont de niveau $<a_v$. L'instantané daté est un préfixe de la tranche, et ses supports sont dans le $K$-polyèdre. |

**Lemme H.** Il est vérifié pour chaque nœud et chaque coupe d'événement où ce nœud est vivant.
- On calcule le $K$-polyèdre (Déf. 21), c'est-à-dire les sites des $K$-parties de la composante.
- À $K\geq 2$, il doit égaler l'union des $P_b$ prise sur les boules de $W_K$ :
  $\mathrm{pts}(C_v(a))=\bigcup\lbrace P_b : b\in W_K,\ \mathrm{att}(b)\preceq v,\ \lambda_b\leq a\rbrace$.
- Il doit égaler aussi la même union restreinte aux seules boules fortes.
- À $K=1$, il égale les feuilles du sous-arbre. L'union des $P_b$ lui est égale dès qu'il compte deux sites ; elle est
  vide sur une feuille.

### La sortie canonique

`Supports.canonical(k, ids)` produit un dict JSON, que la porte sérialise trié et sans espaces. Son schéma est
décrit dans l'en-tête du module.

Contenu :
- les sites en coordonnées, triés lexicographiquement ;
- les nœuds dans la numérotation de l'étage A, la même que celle de la forêt du moteur ;
- les boules triées par (postordre du nœud, niveau, centre) ;
- les supports en listes de coordonnées ;
- `prior` pour le seul rôle fusion, comme la section `PRIOR` ;
- tous les comptes.

Aucune population n'est publiée (décision 4) ; seuls `p` et `m` figurent.

## 4. Fixtures gravées et confrontation à la spécification

### 4.1 Fixtures du § 2.9 et de l'audit

Les 22 faits gravés figurent dans `CLAIMS`. S'y ajoutent les empreintes sha256 des sorties canoniques des 49 fixtures.

| # | Fixture | K | Attendu de la spec | Oracle |
| ---: | --- | --- | --- | --- |
| 1 | carré | 1–4 | K1 : côtés fusion, `components` 2 ; diagonale interne, `cofaces` 2, 1 par support. K2 : quatre naissances ; diagonale fusion, `components` 4, `k_parts` 6, `strict_traces` 4, `cofaces` 4, 2 par diagonale. K3 : naissance étendue, 4, `cofaces` 1, 1 par diagonale. K4 : naissance, `cofaces` 0. | **conforme**, et `kparties_reliees` vaut 2 par côté et 4 pour la diagonale à K1 |
| 2 | triangle droit | 1, 2 | hypoténuse 25/4, $m=3$, $q=2$, $\mathcal{Q}_b$ réduit à l'hypoténuse ; K1 : fusions à 9/4 puis 4, interne ; K2 : naissances 9/4 et 4, fusion 25/4, `strict_traces` 2, `cofaces` 1 | **conforme** |
| 3 | `growth_ABCZ` | 3 | naissance ABC à 16 (boule AC, B intérieur) ; interne à 25, $m=4$, $\mathcal{Q}_b=\lbrace BZ,ACZ\rbrace$, `cofaces` 1, 1 et 1 | **conforme** ; `kparties_reliees` vaut 4 |
| 4 | passagère | 1 | fusion de $\lbrace a,b,c\rbrace$ à 2 ; à 4, $(c,d)$ a `components` 2 ; la boule $\lbrace a,b,c\rbrace$ est de rôle fusion avec `components` 1 | **conforme** ; `prior` vaut [4] pour la passagère et [3, 4] pour $(c,d)$ |
| 5 | triangle aigu | 2 | 25/16, trois composantes, fusion à trois enfants, `strict_traces` 3, `cofaces` 1 | **conforme** |
| 6 | ligne 0, 1, 2 | 2 | β = 1, I = {1}, `k_parts` 3, `strict_traces` 2, `components` 2, `cofaces` 1 | **conforme** ; `compressed_parts` vaut 2 |
| 7 | triangle équilatéral (audit `qb`) | 2 | événement faible : trois naissances à 2, fusion à trois enfants à 8/3 | **conforme** ; p = 0, m = q = 3 |
| 8 | tétraèdre K5 (audit `plateau`, +10) | 5 | six composantes à 200 ; quatre faces à 800/3, `components` 3, `k_parts` 6, `strict_traces` 3, `cofaces` 1 ; une fusion à six enfants | **conforme** ; `compressed_parts` vaut 3 |
| 9 | cube $\lbrace 0,2\rbrace^3$ (audit `qb`) | 2 | quatre diamètres et deux tétraèdres, aucun triangle ; premier support = $S^*$ | **conforme** : la boule interne au niveau 3 a $m=8$ et $q=2$ ; le premier support est d'arité $q$ |
| 10 | octaèdre | 2 | trois paires seulement | **conforme** |
| 11 | cercle $B_t$ en u21 (audit `carrier`) | 2 | $\mathcal{Q}_b$ passe de $\lbrace AC,BD\rbrace$ à $\lbrace AC,B_tCD\rbrace$ | **conforme** pour $n=3,4,5,1023$ (§ 4.2) |
| 12 | ligne 0, 4, 6, 8, 12 ; losanges ; $\lbrace 0,2,4\rbrace$ | 2, 3 | non-naissance au niveau 4, fusion à trois enfants, deux graines menant au même nœud | **conforme** : (a) la boule {4, 8}, avec $I=\lbrace 6\rbrace$, est une fusion à 4, et la racine à 9 a trois enfants ; (b) losanges : naissances étendues 4, 4 et 34, de centres (2,12), (12,2) et (7,7), puis quatre boules de fusion à 50 et une fusion à trois enfants ; (c) {0, 2, 4} : `prior` vaut [0, 1], le nœud 2 est unique |
| 13 | `sphere50` | 2 | $m=84>24$ : refus natif | $m=84$, $p=0$ et niveau 50 ; la paire (15,15,10), (5,5,10) est un support, donc $q=2$ et la boule est dans $W_2$ ; $m$ dépasse le plafond de 24. Le refus lui-même est natif. |

### 4.2 Fixtures ajoutées

**Cercles pour $n=3$ et $n=5$.** Ils viennent de la table du § 10.11 de S0, qui prévoit $n\in\lbrace 3,5,1023\rbrace$ ;
l'audit utilisait $n=4,\dots,1023$.

**Témoin de Hausdorff**, pour $n=3,4,5,1023$, à l'échelle $d=n^{2}+1$ :
- le témoin $w=(3d/4,5d/4)$ a des poids strictement positifs dans $B_tCD$ ;
- sa distance carrée aux deux diamètres $AC$ et $BD$ vaut exactement $d^{2}/16$ ;
- le déplacement carré de $B$ vaut $4d$ ;
- `kparties_reliees` vaut 6 avant et après la perturbation.

**Effet de la perturbation sur la boule du cercle.**
- Avant : rôle fusion, `components` 4, `strict_traces` 4, `cofaces` 4, avec (2, 2) par support.
- Après : `components` 3, `strict_traces` 5, `cofaces` 3, avec (2, 1) par support.
- L'arbre change aussi : une fusion à deux enfants apparaît juste avant le cercle, par exemple au niveau 272 pour
  $n=4$.
- `kparties_reliees` est le seul compte de la boule qui ne bouge pas, ce qui confirme la décision 5.

**Nuages d'Euler** de `tests/catalogue/euler_oracle.py` : `sphere12`, `cercle12` et `sphere12_centre`. Ce sont des
coquilles de 12 sites qui portent de nombreux supports, triangles compris.

**Fixtures historiques de `families.py`.** Les 25 fixtures à positions distinctes, hors ensembles déjà présents,
sont gravées par empreinte.

### 4.3 Écart relevé dans le texte de S0 (lemme W, point 2)

S0 écrit, au § 10.3 de `MATHEMATIQUES.md` : sur E5 à $K=2$, la boule de $AC$ est hors fenêtre (niveau 33/2,
$p=2$, $q=m=2$), puis « Avec elle, la fusion de niveau 83886/3563 a trois enfants ; sans elle, elle n'en a que deux,
et une fusion supplémentaire apparaît au niveau 24 ».

L'oracle (`Supports.window_tree`) donne :

| Graphe | Fusions (niveau, enfants) | Enfants de la fusion de niveau 83886/3563 |
| --- | --- | --- |
| $\Gamma_2$ vrai ($T_2$) | (162/25, 3), (189/17, 3), (83886/3563, 3) | {AB}, {AC, AD, AE, CD, CE, DE}, {BC} |
| liaisons de $W_2$ seules, tous les sommets gardés | (162/25, 3), (189/17, 3), (83886/3563, **3**), (24, 2) | {AB}, {AC}, {BC} |
| liaisons de $W_2$ seules, sommets hors fenêtre retirés (AC n'entre qu'avec la liaison ABC) | (162/25, 3), (189/17, 3), (83886/3563, **2**), (24, 2) | {AB}, {BC} |

La conclusion du lemme W.2 tient dans les deux lectures : $T_K$ change. La fusion supplémentaire à 24 apparaît
aussi dans les deux. En revanche, « elle n'en a que deux » n'est vrai que si l'on retire aussi le sommet $AC$.
Si l'on garde ce sommet, la fusion garde trois enfants, mais ce ne sont plus les mêmes.

Les deux lectures sont gravées (fait `e5_window`). Je propose à S0 de préciser la phrase (§ 9, question 1).

### 4.4 Ce qui n'a pas été trouvé

L'oracle n'a trouvé aucune contradiction mathématique. Aucun lemme n'a été mis en défaut, que ce soit sur les
fixtures ou sur les 209 nuages. Aucune fixture minimale nouvelle n'est donc à porter au registre des preuves.

## 5. Compteurs et durées

### 5.1 Suite de la porte

La suite comprend 49 fixtures et 160 nuages des familles :
- les dix familles de `families.py` à positions distinctes, `duplicates` exclue ;
- 16 nuages par famille, de 5 à 10 points, graine 31 ;
- tous les ordres $K\leq\min(5,n-1)$ ;
- pour les fixtures, les ordres de la spécification en plus.

Compteurs exacts, gravés dans `SUITE_EXACT` (toute dérive fait sortir la porte avec le code 3) :

| Compteur | Valeur | Plancher de la spec (§ 8.2) |
| --- | ---: | ---: |
| nuages | 209 | 150 |
| ordres | 947 | — |
| boules de $W_K$ | 15 035 | 5 000 |
| supports (arités 2, 3 et 4 : 10 138, 5 633 et 1 145) | 16 916 | — |
| nœuds | 12 415 | — |
| couples (nœud, coupe) du lemme H | 48 018 | — |
| naissances, fusions, internes (rôles) | 6 545, 5 756, 2 734 | 100 internes |
| cellules passagères | 271 | 10 |
| fusions d'au moins trois enfants | 1 885 | 20 |
| coquilles étendues ($m>q$) | 2 551 | 200 |
| boules à plusieurs supports | 530 | 50 |
| coquilles étendues avec un support d'arité $>q_{\min}$ | 226 | 1 |
| boules fortes, faibles ($p+q=K+1$) | 7 571, 7 464 | — |
| `kparties_reliees` (somme) | 54 604 | — |
| `cofaces` (somme) ; boules où la somme par support dépasse | 20 449 ; 277 | — |
| liaisons de Gabriel (W.3) ; non Gabriel (W.4) | 17 044 ; 40 041 | — |
| nuages rejoués, permutés et réétiquetés | 69 | — |

Empreinte de la suite : `ff68fa273b100e69d50b43abbcd200ea70f120ab085a67cbf17a769cc53d9d51`.

### 5.2 Durées

Temps CPU sur un cœur, en secondes, sous `-S -B`. Le codespace était partagé avec d'autres agents.

| Python | normal | `-O` |
| --- | ---: | ---: |
| 3.12.1 | 21,9 | 22,0 |
| 3.10.21 (celui de G4) | 29,4 | 31,8 |

Les quatre sorties sont identiques (md5 `daae1933c91c`), et le restent avec `PYTHONHASHSEED` à 1 puis à 4242, sur la
version finale de la porte.

Le label `fast` tient donc sans découpage : on vise moins de 90 s, et l'on en est à environ 30 s sous 3.10.

## 6. Mutants

On compte une porte par mutant, avec le code 4 et la ligne `mutant_killed <nom>` exacte, plus sa jumelle `-O`.

Chaque mutant est d'abord joué sur les sources intactes, ses fixtures servant de témoin : celles-ci doivent être
conformes. Il est ensuite joué sur une copie mutée, chargée sans le paquet. Il doit être tué, et l'un des écarts
doit porter sa **cause** déclarée ; sinon la porte sort avec le code 3.

Aucun mutant n'est déclaré équivalent.

| Mutant | Faute injectée | Fixtures | Tué par (premier écart) |
| --- | --- | --- | --- |
| `att_coupe_ouverte` | att lu à la coupe ouverte, sur les traces strictes | triangle aigu K2, passagère K1 | lemme A (T3) : les 3 $K$-parties donnent les nœuds {0, 1, 2} |
| `ant_coupe_fermee` | branches lues à la coupe fermée | triangle aigu K2, passagère K1 | lemme C.1 : {0, 1, 2} contre {3} |
| `fenetre_forte` | $p+q\leq K$ au lieu de $p+q-1\leq K$ | triangle équilatéral K2, carré K1 | **périmètre** (lemme W.3) : liaison de Gabriel (0, 1, 2) de boule hors de $W_K$ |
| `premier_support_seul` | $\mathcal{Q}_b$ réduit au premier support | cube K2, `growth_ABCZ` K3 | lemme F : [(0, 3)] contre [(0, 3), (1, 2)] |
| `triangle_droit_admis` | poids nuls admis ($w\geq 0$) | triangle droit K1, carré K2 | lemme F : [(0, 1, 2), (1, 2)] contre [(1, 2)] |
| `cofaces_ordre_k` | liaisons comptées sur les $K$-parties | carré K1, ligne K2 | lemme G : `cofaces` vaut 0 par énumération et 1 par formule |
| `populations_naissances_seules` | lemme H calculé sur les seules boules de naissance | `growth_ABCZ` K3 | lemme H : nœud 0, coupe 25, polyèdre [0, 1, 2, 3] contre union [0, 1, 2] |

Remarque : `fenetre_forte` était déclaré « tué par le lemme C ». Il est en fait tué plus tôt et plus directement par
le contrôle de périmètre (W.3), et sa cause déclarée est `perimetre`. Le lemme C.3 le tuerait aussi : une fusion
perdrait ses boules de rôle fusion.

## 7. Portes jouées

Dans le dossier `reference/` :

```text
python3 -S -B test_supports.py                              code 0, ligne gravée, 21,9 s CPU
python3 -O -S -B test_supports.py                           code 0, même sortie
python3.10 -S -B test_supports.py ; python3.10 -O -S -B     code 0, même sortie (29,4 s et 31,8 s)
python3 -S -B test_supports.py --inject=<nom>  (x7)         code 4, en normal et en -O
python3 -S -B test_supports.py --inject=absent               code 2
python3 -S -B test_ref.py --suite=fast                       code 0 : faits et compteurs de la référence inchangés
```

Configuration CMake limitée à l'unité `reference`, sans aucune compilation :

```text
cmake -S morsehgp3D_v11 -B /tmp/v11-l0-s1_oracle -DMHGP11_MODULES=reference -DCMAKE_BUILD_TYPE=Release
ctest --test-dir /tmp/v11-l0-s1_oracle -LE long -j3
```

Résultat : 95 portes enregistrées, 78 jouées hors `long`, **78 sur 78 passent** en 84 s réelles sur 3 cœurs. Parmi
elles :
- `mhgp11_reference_supports` et sa jumelle `_opt` ;
- `_refusal` et sa jumelle ;
- les 7 mutants et leurs 7 jumelles ;
- les portes existantes de la référence : `fast`, `fast_split`, les 25 mutants, `projection_contracts` ;
- `mhgp11_style` (`--units reference`).

## 8. Écarts à la spécification et choix d'implantation

1. **Ordres de la sortie canonique.**
   - Les sites sont triés lexicographiquement, et non par Morton.
   - Les boules d'un même nœud et d'un même niveau sont départagées par leur centre, et non par $S^*$ en `SiteIdx`.
   - Ce sont deux ordres géométriques, qui rendent la sortie indépendante de l'ordre d'entrée. L'oracle ne connaît
     pas Morton, comme l'étage A.
   - Conséquence : les différentiels S3 et S6 devront traduire les lignes de `SITES` en coordonnées, puis retrier par
     les mêmes clés. Pour $S^*$, l'oracle ne contrôle que l'arité $q$ du premier support.
2. **Noms des portes de mutants.** La famille `mhgp11_reference_supports_mutants` de la spécification devient une
   porte par mutant, `mhgp11_reference_supports_mutant_<nom>`, sur le modèle de `mhgp11_reference_mutant_<nom>`.
3. **Contrôles ajoutés au-delà du § 8.2.**
   - Le lemme W, points 3 et 4, et le témoin E5 du point 2.
   - La règle du parent.
   - Les tranches contiguës et le préfixe daté.
   - La restriction du lemme H aux boules fortes.
   - Le témoin de Hausdorff.
   Les points W et la règle du parent viennent du § 10 de S0, écrit en parallèle.
4. **Nuages des familles.** J'ai pris 16 nuages par famille, de 5 à 10 points, graine 31. Les nuages de la suite
   rapide de `test_ref.py` (4 à 8 points, $K\leq 4$) étaient trop petits pour $K\leq 5$.
5. **Cause des mutants.** Les mutants sont tués par les lemmes de l'oracle. Les faits gravés et les empreintes les
   tuent aussi, et la cause déclarée est exigée.
6. **Fixture 11.** J'ai ajouté $n=3$ et $n=5$ (table de S0) à $n=4$ et $n=1023$ (audit).

## 9. Questions ouvertes

1. **Pour S0 (lemme W.2).** Quelle lecture de « l'omettre » est voulue sur E5 ? Je propose : « sans ses liaisons,
   sommet AC gardé, la fusion de niveau 83886/3563 garde trois enfants, mais ce ne sont plus les mêmes, et une fusion
   à deux enfants apparaît au niveau 24 ; si l'on retire aussi le sommet AC, elle n'a plus que deux enfants ». Les
   deux faits sont gravés.
2. **Pour S3 et S6.** Faut-il que l'oracle calcule l'ordre de Morton, pour comparer l'identité de $S^*$ et l'ordre
   natif des boules à rang égal ? Ou bien le différentiel retrie-t-il par (postordre, niveau, centre) ? Je recommande
   la seconde option, qui garde l'oracle géométrique.
3. **Suite longue.** Une variante `long` plus large ($n$ de 12 à 14, $K\leq 10$) serait utile sur G4, comme la suite
   complète de `test_ref.py`. Elle n'est pas écrite ; ses planchers seraient gravés au premier passage.

## 10. Ce que ce travail n'établit pas

- Il n'établit aucune propriété du moteur natif, ni coût, ni échelle. La règle des tailles d'intérêt (8 000 à
  32 000 points) relève des juges natifs.
- Le refus `support_shell_capacity` n'est pas joué : seul $m=84$ est constaté.
- L'identification de $\pi_{0}(L_K)$ à $\pi_{0}(\Gamma_K)$ (théorème 2) est invoquée, pas re-vérifiée.
- Aucun statut public n'est promu : `public_status=not_claimed`.
