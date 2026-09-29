# Reçu J2c : arbre des boîtes ajusté à l'enveloppe, coupe binaire au plus long côté, catalogue identique

29 septembre 2026. Lot J2c, demandé par le coordinateur : faire de la sonde de `AUDIT_FAIR_SPLIT.md` un patch
produit. Demandé sur `7eee86c53` (J2 `5565f94fb` + assemblage), puis rebasé, à sa demande, sur `b662673b2` (pool
corrigé `8e3b76245` + tête). Copies neuves dans `build/v10-j2/j2c/`. Aucune branche, aucun commit, aucune commande
GCP ; le worktree n'est pas touché.

```text
phase=exploration_v10_hors_registre (lot J2c)
backend=reference_cpu
profile=quantized_u18_input_only
mode=implementation_locale_mesuree (codespace EPYC 7763 Zen 3, partage, 1 a 4 fils)
public_status=not_claimed
GCP non utilise
```

## 1. En bref

- **Patch** : `build/v10-j2/j2c/J2c.patch`, 3 fichiers.
  - Il s'applique par `git apply` sur `b662673b2`, base finale demandée : l'arbre patché est identique à la copie de
    travail.
  - Il s'applique aussi sur `7eee86c53`, puisque les fichiers touchés y sont identiques.
- **Ce qui change** :
  - chaque nœud ajuste sa boîte à l'enveloppe de sa liste (S = Q ∩ [env.lo, env.hi + 1)) puis la coupe en deux au
    milieu de son plus long côté ;
  - D-loc et clé du réservoir par axe ;
  - pré-ignorance A6 retirée (elle ne se déclenche plus) ;
  - stagnation : 9 niveaux binaires sur le plus long côté ;
  - table M(K) : 12 / 16 / 24 / 28.
- **Exactitude** :
  - dumps identiques à `568d45297` sur les 10 entrées, à 1 et 4 fils, en build normal et empoisonné ;
  - niveaux exacts identiques ;
  - compteurs de l'arbre égaux à 1 et 4 fils, et identité de l'arbre binaire vérifiée ;
  - 9 portes sur 9 ; oracle T2 à 161 contrôles, 0 écart ;
  - ASan, UBSan et ThreadSanitizer propres ;
  - entrées dégénérées identiques, et même refus sur la sphère de 312 points ;
  - 3 mutants tués.
- **Gain local** :
  - t_boxes ×1,19 à ×1,30 à K = 5 et ×1,12 à ×1,19 à K = 10, à 1 et 4 fils ;
  - catalogue entier ×1,09 à ×1,24 ;
  - arbre : −33 à −40 % de nœuds et −54 % de feuilles sur les trames.
- **Point à surveiller sur G4** : `t_frontier` (+60 à +100 % en local, § 6 et § 8).

## 2. Ce qui change dans le code

Fichiers : `src/catalogue/generator.cpp`, `src/catalogue/catalogue.hpp` (grand livre, commentaire d'en-tête),
`cli/mhgp10_catalogue.cpp` (JSON du grand livre).

- **Boîte ajustée et coupe binaire (`process`).**
  - Après le filtre, S = Q ∩ [env.lo, env.hi + 1), où env est l'enveloppe fermée de la liste, en coordonnées
    entières.
  - S vide sur un axe, ou liste vide : c'est le lemme K, et le nœud est ignoré (`skipped_bbox`). Le test est
    identique à celui de J2 : S est vide sur l'axe a si et seulement si env.hi < lo ou env.lo ≥ hi.
  - Sinon, S est coupée en deux au milieu de son plus long côté, le premier axe en cas d'égalité. L'enfant bas vient
    avant l'enfant haut. La feuille énumère sur S.
- **D-loc par axe (`filter_node`).** Le membre droit devient Σᵢ max(0, 2hᵢ·x′ᵢ − 2hᵢ·y′ᵢ), avec hᵢ = hiᵢ − loᵢ.
- **Clé du réservoir par axe.** dd = Σᵢ (2x′ᵢ − hᵢ)² = |2X − (lo + hi)|², soit quatre fois le carré de la distance au
  centre exact de la boîte. La forme J2, avec un seul h, supposait un cube.
- **Pré-ignorance A6 retirée.**
  - Une boîte fille est incluse dans S, elle-même incluse dans [env.lo, env.hi + 1), où env est l'enveloppe de la
    liste parente. Sur chaque axe, fille.lo ≤ env.hi et fille.hi > env.lo : le test env.hi < fille.lo ou
    env.lo ≥ fille.hi n'est jamais vrai.
  - Confirmé par comptage : la sonde, qui gardait A6, a compté 0 pré-ignorance sur les 10 entrées
    (`build/v10-j2/fair_split/differentiel_fs.txt`).
  - Disparaissent avec A6 : le champ `Task::env`, le paramètre `penv`, `struct Env`, `misses()` et le compteur
    `preskipped_bbox`.
- **Stagnation** : `kStagnationLimit = 9` niveaux binaires, comparés au plus long côté de S (§ 3.6).
- **Commentaires.** Ceux sur les « boîtes cubiques » sont réécrits (en-tête de `generator.cpp`, filtre, nœud,
  `catalogue.hpp`).
- **Bornes i64.** Le `static_assert(2 (B + T) + 5 <= 63)` reste valide tel quel : chaque hᵢ est au plus le côté de la
  racine (§ 3.2).
- **Frontière J1 et table M(K).**
  - La frontière pilotée par la charge ne change pas : `inside()` compte les sites de la liste parente dans la boîte
    de la tâche, quelle que soit sa forme.
  - La table M(K) devient 12 / 16 / 24 / 28 (§ 4).

## 3. Lemmes, énoncés et preuves

**Notations.**

- Q = [lo, hi) est la boîte d'un nœud (pavé, côtés hᵢ = hiᵢ − loᵢ ≥ 1), et L = L(Q) sa liste certifiée.
- Théorème C : L contient la boule K-NN fermée de tout point de Q.
- Une boule admise B a un centre c, un rayon r, un intérieur I de poids p ≤ K − 1 (dans les deux cas d'admission,
  p + q_min ≤ K + 1 avec q_min ≥ 2, ou p ≤ K − 1), une coquille U et un support S* ⊆ U.
- Coordonnées mises à l'échelle : X = x·2^T, entiers ; c est rationnel.

### 3.1 Lemme de l'ajustement

Si c ∈ Q, alors c ∈ S = Q ∩ [env.lo, env.hi + 1), où env est l'enveloppe fermée de L.

*Preuve.*

1. La boule ouverte de rayon r autour de c pèse p < K. La boule K-NN fermée de c a donc un rayon ≥ r et contient
   I ∪ U. Par le théorème C, I ∪ U ⊆ L, donc S* ⊆ L.
2. c est dans l'intérieur relatif de conv(S*), donc dans conv(L), inclus dans le pavé fermé [env.lo, env.hi].
3. Les bornes de env sont entières : cᵢ ≤ env.hiᵢ entraîne cᵢ < env.hiᵢ + 1. Avec c ∈ Q, on a c ∈ S.

∎

- La borne haute doit être env.hi + 1. Des centres tombent exactement sur env.hi : grilles de l'oracle, sphères
  entières. Le mutant M1 le confirme (§ 5.4).
- S ⊆ Q : la liste de Q vaut aussi pour S et pour ses sous-boîtes. Le théorème C vaut pour toute boîte incluse dans
  Q, puisque les boules K-NN des points de S sont celles de points de Q.

### 3.2 D-loc et réservoir par axe

Avec x′ = X − lo, y′ = Y − lo et A = |x′|², et C = lo + c′, c′ ∈ Π [0, hᵢ] :

$$\max_{C} (|Y - C|^2 - |X - C|^2) = A(y) - A(x) + \sum_{i} \max(0, 2h_i x'_i - 2h_i y'_i)$$

Chaque terme 2c′ᵢ(x′ᵢ − y′ᵢ) est affine en c′ᵢ, donc maximal à c′ᵢ = hᵢ ou à 0. C'est le même entier que la forme
par coins, donc la même décision, sur un pavé quelconque.

Bornes (B = 18, T = 6) :
- Xᵢ ∈ [0, 2^24) et lo est dans la racine, de côté ≤ 2^24 : |x′ᵢ| < 2^24 et hᵢ ≤ 2^24.
- |2hᵢx′ᵢ| < 2^49 ; 2hᵢ(x′ᵢ − y′ᵢ) = 2hᵢ(Xᵢ − Yᵢ), de module < 2^49 ; membre droit < 3·2^49 ; A < 3·2^48.
- |2x′ᵢ − hᵢ| < 3·2^24, donc dd < 27·2^48.
- Mêmes bornes que J2 : le `static_assert(2 (B + T) + 5 <= 63)` reste la garde.

La clé dd = Σᵢ(2x′ᵢ − hᵢ)² vaut quatre fois le carré de la distance au centre exact (lo + hi)/2. Le choix du
réservoir n'affecte pas l'exactitude, car toute exclusion est certifiée par D-loc. Il affecte la qualité de Y, donc
les listes, l'arbre et le coût. Le mutant M3 (centre cubique) est donc attendu au grand livre, pas au dump.

### 3.3 Partition, unicité, déterminisme

- Les deux moitiés [S.lo, mid) et [mid, S.hi) de l'axe le plus long pavent S (demi-ouvertes ; mid = ⌊(lo + hi)/2⌋,
  les deux côtés ≥ 1 puisque le côté est ≥ 2).
- Par récurrence depuis la racine, qui contient tous les centres possibles, chaque centre admis est dans exactement
  une feuille. Il y est énuméré une fois (mémo des coquilles étendues inchangé).
- La feuille énumère sur S ⊆ Q, qui contient tous les centres admis de Q.
- L'arbre (boîtes, listes, feuilles) est une fonction de l'entrée seule : ordre des enfants fixé, égalités de côté
  départagées par le premier axe. La frontière J1 n'en est qu'une coupe, déterministe à nombre de fils donné.
- Les compteurs de l'arbre sont vérifiés égaux à 1 et 4 fils sur les 10 entrées (§ 5.1).

### 3.4 Lemme K et A6

- S est vide sur l'axe a si et seulement si max(lo, env.lo) ≥ min(hi, env.hi + 1), c'est-à-dire env.hi < lo ou
  env.lo ≥ hi. C'est exactement le test de J2, donc le même classement « ignoré ».
- A6 ne se déclenche jamais (§ 2).
- Nouvelle identité de l'arbre binaire : chaque nœud interne a deux enfants, tous traités, donc
  nœuds = 1 + 2 × internes et ignorés = nœuds − feuilles − (nœuds − 1)/2. Elle est vérifiée sur toutes les entrées.

### 3.5 Terminaison et profondeur

- Chaque coupe divise par deux le plus long côté de S, et l'ajustement ne fait que réduire.
- La somme des log₂ des côtés, au plus 3 × 24, baisse d'environ 1 par niveau : profondeur ≤ ~75 niveaux binaires,
  contre ≤ 25 pour l'octree.
- La récursion reste légère : le cadre de `process` est petit, et `filter_node` est hors ligne.

### 3.6 Règle de stagnation (décision)

**Rôle.**
- La stagnation ne touche pas l'exactitude : toute feuille à liste certifiée est exacte.
- Elle décide où s'arrête la descente au-dessus d'une dégénérescence. Elle influe sur le coût et sur le refus
  `wide_leaf` (liste de plus de 256 sites).

**Pourquoi une liste qui ne décroît plus signale une dégénérescence.**
- Soient m sites cosphériques de centre c, avec c dans la boîte fermée. Aucun ne domine l'autre, puisque leurs
  distances sont égales en c. La liste garde les m sites à toute profondeur.
- À l'inverse, pour une configuration générique, la liste d'une boîte qui rétrécit converge vers les K plus proches
  voisins de ses points, et K < M.
- Une liste de plus de M sites qui ne décroît plus sous l'échelle de la grille est donc la signature d'une
  dégénérescence cosphérique, exacte ou quasi exacte.

**Quel côté comparer à 2^T.**
- Le plus long côté de S : la boîte est alors sous l'échelle de la grille dans toutes les directions.
- Avec le plus court, une dalle d'un millimètre d'épaisseur mais longue de plusieurs mètres serait déclarée
  stagnante alors qu'elle contient encore beaucoup de géométrie.
- Pour un cube, les deux coïncident avec la règle de l'octree.

**Combien de niveaux.**
- 9 niveaux binaires sans décroissance. Autour de c, le volume est alors divisé par 2^9 : chaque côté par 8, puisque
  la coupe au plus long côté parcourt les axes tour à tour.
- C'est exactement le critère de l'octree de J2 : 3 niveaux, chacun divisant les trois côtés par 2.
- La décision « abandonner et faire une feuille bloquée » se prend donc après le même rétrécissement géométrique
  qu'avant. Ce n'est pas un réglage.

**Contrôle sur des dégénérescences réelles** (`degen_j2c.sh`, § 5.5) :
- grilles entières 10³ (pas 1 et pas 7) et 16³ ;
- sphères de points entiers (168 et 312 points cosphériques) ;
- à K = 5 et 10 : dumps identiques à `568d45297`, et même refus `wide_leaf` pour la sphère de 312 points, comme la
  référence ;
- les feuilles bloquées y existent (grille 16³ à K = 10 : 6 431, contre 25 688 dans l'octree).

## 4. Table M(K) (décision)

Taille de feuille M(K) = 12 / 16 / 24 / 28 pour K ≤ 3 / ≤ 6 / ≤ 10 / > 10. Par rapport à J2, seule la dernière case
change (32 → 28). C'est une table fixe, sans réglage par entrée.

**Protocole de mesure.**
- `calibre_m.sh`, `--leaf=M`, t_boxes à 1 fil, 2 répétitions, les valeurs de M alternées dans chaque répétition,
  binaire J2c rebasé.
- Charge de 1,1 à 1,8 pendant la série, relevée avant et après (`calibre_charge.txt`).
- Une première série a chevauché la fenêtre des portes du coordinateur (14 h 47 – 14 h 55). Elle est écartée
  (`calibre_m_fenetre_portes.tsv`) et ses conclusions ont été rejouées.

**Médianes de t_boxes, en secondes (`calibre_m.txt`, `calibre_m_bornes.txt`)** :

| K | entrée | valeurs de M | meilleur |
| ---: | --- | --- | ---: |
| 5 | trame 02 | 12 : 6,580 · **16 : 5,197** · 20 : 5,343 · 24 : 5,883 · 28 : 6,739 | 16 |
| 5 | trame 00 | 12 : 7,096 · **16 : 5,593** · 20 : 5,650 · 24 : 6,232 · 28 : 7,097 | 16 |
| 5 | quart 01 | 12 : 1,030 · **16 : 0,785** · 20 : 0,808 · 24 : 0,862 · 28 : 0,993 | 16 |
| 5 | syn_shells_x2 | 12 : 1,818 · **16 : 1,585** · 20 : 1,628 · 24 : 1,812 · 28 : 2,119 | 16 |
| 10 | trame 02 | 16 : 41,578 · 20 : 24,849 · **24 : 22,311** · 28 : 22,665 · 32 : 24,435 · 40 : 29,732 | 24 |
| 10 | trame 00 | 16 : 45,695 · 20 : 27,209 · **24 : 24,003** · 28 : 24,355 · 32 : 26,158 · 40 : 31,915 | 24 |
| 10 | quart 01 | 16 : 7,302 · 20 : 3,945 · **24 : 3,258** · 28 : 3,261 · 32 : 3,444 · 40 : 4,172 | 24 |
| 10 | syn_shells_x2 | 16 : 9,440 · 20 : 6,204 · **24 : 5,819** · 28 : 6,029 · 32 : 6,502 · 40 : 8,079 | 24 |
| 3 | quart 01, syn_shells_x2 | 8 : 0,484 / 0,896 · **12 : 0,304 / 0,665** · 16 : 0,312 / 0,694 | 12 |
| 12 | trame 02 | 24 : 34,777 · **28 : 32,840** · 32 : 33,812 | 28 |
| 12 | trame 00 | 24 : 38,057 · **28 : 35,728** · 32 : 36,929 | 28 |
| 12 | quart 01, syn_shells_x2 | 24 : 5,479 / 8,631 · **28 : 4,883 / 8,425** · 32 : 4,903 / 8,822 · 40 : 5,676 / 10,466 | 28 |

**Lecture.**
- À K = 5, M = 16 est le meilleur sur les quatre entrées ; à K = 10, M = 24 aussi. Le plus mauvais écart au meilleur,
  sur toutes les entrées, vaut +3,0 % pour M = 20 à K = 5 et +3,6 % pour M = 28 à K = 10 : ces minimums sont plats.
- À K = 12, qui sert au juge d'Euler de la tour à K = 10 (kmax + 2) et à l'entrée « cover » du clustering,
  M = 28 bat 32 de 3 % sur les deux trames et de 0,4 à 4,7 % sur les deux autres entrées : la case K > 10 passe à 28.
- M ne change pas le catalogue (théorème C, pour toute taille de feuille). Le différentiel des 10 entrées tourne avec
  la table finale.

## 5. Exactitude

Code final : `b662673b2` (pool corrigé, `8e3b76245`, puis tête, `b662673b2`) + `J2c.patch`. Binaire
`mhgp10_catalogue.j2c`. Référence : `build/v10-j2/build-base/mhgp10_catalogue`, soit `568d45297`, sha256
`433b4b97…`.

### 5.1 Différentiel des 10 entrées (`differentiel_j2c.sh`, `differentiel_j2c.txt`)

Référence à 3 fils, J2c à 1 et 4 fils, dumps canoniques complets (dans `/tmp`, sha256 puis suppression).

| entrée | K | dump (16 premiers) | ref / J2c 1 fil / J2c 4 fils | nœuds ref → J2c | feuilles ref → J2c |
| --- | ---: | --- | --- | --- | --- |
| trame 02 | 5 | `8a850649ff103c1c` | identiques | 1 225 705 → 734 083 | 710 429 → 323 224 |
| trame 02 | 10 | `d6abe0dba4d9be33` | identiques | 1 639 729 → 1 065 585 | 1 061 120 → 485 833 |
| trame 00 | 5 | `3982c3ab79c4a542` | identiques | 1 250 977 → 781 865 | 767 906 → 352 967 |
| trame 00 | 10 | `7b2d4055f4cc7004` | identiques | 1 684 689 → 1 137 143 | 1 150 322 → 530 147 |
| quart 01 | 5 | `414aa4d47ffe1c55` | identiques | 224 977 → 123 159 | 118 570 → 52 445 |
| quart 01 | 10 | `7c46e50a72c08087` | identiques | 301 745 → 188 013 | 184 136 → 84 052 |
| syn_shells_x2 | 5 | `fcd0425f8101b278` | identiques | 387 289 → 206 543 | 187 132 → 81 707 |
| syn_shells_x2 | 10 | `8c145ca4ed47628a` | identiques | 490 649 → 278 917 | 256 969 → 111 792 |
| syn_filaments_x4 | 5 | `e9408470b9317629` | identiques | 938 281 → 612 875 | 635 279 → 286 370 |
| syn_filaments_x4 | 10 | `b9719e2166bb7c92` | identiques | 1 283 065 → 863 123 | 883 310 → 403 280 |

Sur chaque ligne, trois contrôles :

- **Compteurs du catalogue** (`balls`, `levels`, `by_q_p`, `extended`, `weighted`, `max_shell`) : égaux à la
  référence.
- **Compteurs de l'arbre** (nœuds, feuilles, ignorés, Σm, m max, bloquées, `filter_tests` et compteurs de feuille) :
  égaux à 1 et 4 fils. Leurs valeurs à 1 fil sont dans `ledger_j2c.jsonl`.
- **Identité de l'arbre binaire** : ignorés = nœuds − feuilles − (nœuds − 1)/2.

### 5.2 Build empoisonné, niveaux exacts

- **Build `MHGP10_POISON`** (octets 0xA5 à l'allocation) : même différentiel, mêmes 10 identités
  (`differentiel_j2c_poison.txt`). Aucune case non écrite n'atteint la sortie.
- **Niveaux exacts publiés** (`cat.level`, absents du dump). La sonde `levelhash.cpp` est compilée contre chaque
  bibliothèque. Les empreintes sont identiques sur les 10 entrées entre `568d45297` (4 fils) et J2c (4 fils, 1 fil,
  et build empoisonné à 4 fils) (`niveaux_j2c.txt`).

### 5.3 Portes, oracle T2

- `ctest -L gate` : **9 sur 9** au `b662673b2` patché (`ctest_gate_j2c.txt`). Cela inclut la porte de stress du pool
  dans `mhgp10_unit` et `multiplicity_refusal`.
- Oracle T2 : **161 contrôles, 0 écart**, 14 132 boules. Oracle de la tour : 73 contrôles, 0 écart.
- **Course du pool.** Aucun échec intermittent n'a été vu dans ce travail. Les oracles des mutants ont été joués deux
  fois, et leurs échecs se reproduisent à l'identique. La course, antérieure à J2, est corrigée dans la base
  (`8e3b76245`).

### 5.4 Mutants (hors patch ; `mutants_j2c.py`, `mutants_j2c.sh`, `mutants_j2c.txt`)

Chaque mutant est jugé de trois façons :
- dumps contre la référence : quart 01, syn_shells_x2 et grille 10³, à K = 5 et 10 ;
- grand livre de l'arbre contre J2c correct ;
- oracle T2, joué deux fois.

| mutant | faute | dumps | grand livre | oracle T2 | tué par |
| --- | --- | --- | --- | --- | --- |
| M1 | borne haute de la boîte ajustée sans + 1 | **différents** partout : quart K = 5, 210 424 → 210 224 boules ; grille 24 995 → 23 543 | différent | **code 1**, 120 écarts sur 161, reproduit | dumps (LiDAR compris), oracle, grand livre |
| M2 | D-loc avec le côté de l'axe 0 pour les trois axes | **différents** partout : quart K = 5, 210 424 → 38 188 | différent | **code 1**, 88 écarts sur 161, reproduit | dumps, oracle, grand livre |
| M3 | clé du réservoir sur le centre cubique (demi-côté de l'axe 0) | identiques | **différent** : quart K = 5, nœuds 123 159 → 123 735, `filter_tests` +1,3 % | code 0 | **grand livre de l'arbre seul** |

M3 est attendu tel quel (§ 3.2) : le réservoir ne décide d'aucune exclusion, il ne fait que choisir les candidats
dominateurs, et un mauvais choix élargit les listes. La porte qui le tue est la comparaison du grand livre de l'arbre
à la valeur J2c enregistrée (`ledger_j2c.jsonl`). C'est une porte de performance, pas d'exactitude.

### 5.5 Entrées dégénérées (`degen_j2c.sh`, `degen_j2c.txt`, `degen_j2c_poison.txt`)

Contre `568d45297`, à 4 fils, en build normal et en build empoisonné :

- **Dumps identiques** :
  - grilles entières 10³ (pas 1, et pas 7 décalé) et 16³, à K = 5 et 10 ;
  - sphère entière de 168 points cosphériques (x² + y² + z² = 101), à K = 5 et 10.
- **Même refus** `resource_exhausted / wide_leaf` pour la sphère de 312 points (x² + y² + z² = 314), à K = 5 et 10.
- Feuilles bloquées sur ces entrées :
  - grille 10³ à K = 10 : 5 096 dans l'octree contre 1 157 dans J2c ;
  - grille 16³ à K = 10 : 25 688 contre 6 431 ;
  - sphère de 168 points : 8 des deux côtés, avec la sphère entière dans une feuille (m max 168).

### 5.6 Sanitizers

- **ASan et UBSan** (`-DMHGP10_SANITIZE=ON`, 2 fils) sur `lidar02_quarter_x_nonneg_y_nonneg`,
  `syn_shells_density_x1`, sphère de 168 points et grille 10³, à K = 5 et 10 : code 0, aucun message, dumps
  identiques à la référence (`sanitizers_j2c.txt`).
- **ThreadSanitizer** (`-DMHGP10_TSAN=ON`, binaire lancé par `setarch $(uname -m) -R`), `tsan_j2c.txt` :
  - quart 01 à 4 fils, K = 5 et 10 : code 0, aucun avertissement, dumps identiques ;
  - oracle T2 lancé sous `setarch -R` avec le binaire TSan (12 nuages et la translation, 2 fils par appel) :
    49 contrôles, 0 écart. Un avertissement TSan rendrait un code non nul, que l'oracle compte comme écart.

## 6. Mesures locales (`mesure_j2c.sh`, `mesure_j2c.tsv`, `mesure_j2c.txt`)

Protocole :
- « avant » : `b662673b2` vierge ; « après » : `b662673b2` + J2c. Même compilateur, Release, sans `-march`.
- Exécutions alternées avant et après pour chaque (entrée, K, fils), 3 répétitions, médianes avec [min – max] pour
  t_boxes.
- Charge de 0,8 à 2,5 (`mesure_charge.txt`), hors des fenêtres du coordinateur.

| entrée | K | fils | t_boxes avant → après (s) | gain | catalogue_s avant → après (s) | gain | t_frontier avant → après (s) |
| --- | ---: | ---: | --- | ---: | --- | ---: | --- |
| trame 02 | 5 | 1 | 6,265 [6,260–6,286] → 5,164 [5,160–5,188] | ×1,21 | 6,982 → 5,922 | ×1,18 | 0,050 → 0,086 |
| trame 02 | 10 | 1 | 25,521 [25,492–25,548] → 22,123 [22,058–22,151] | ×1,15 | 28,345 → 25,064 | ×1,13 | 0,077 → 0,145 |
| trame 00 | 5 | 1 | 6,475 [6,433–6,482] → 5,460 [5,418–5,465] | ×1,19 | 7,128 → 6,171 | ×1,16 | 0,045 → 0,081 |
| trame 00 | 10 | 1 | 26,885 [26,828–26,901] → 23,996 [23,949–24,021] | ×1,12 | 29,764 → 26,905 | ×1,11 | 0,071 → 0,136 |
| quart 01 | 5 | 1 | 0,976 [0,974–0,981] → 0,770 [0,763–0,789] | ×1,27 | 1,074 → 0,866 | ×1,24 | 0,008 → 0,014 |
| quart 01 | 10 | 1 | 3,883 [3,879–3,906] → 3,268 [3,231–3,271] | ×1,19 | 4,243 → 3,630 | ×1,17 | 0,012 → 0,025 |
| trame 02 | 5 | 4 | 1,601 [1,596–1,601] → 1,277 [1,276–1,327] | ×1,25 | 1,871 → 1,553 | ×1,20 | 0,024 → 0,037 |
| trame 02 | 10 | 4 | 6,474 [6,434–6,511] → 5,637 [5,598–5,649] | ×1,15 | 7,483 → 6,690 | ×1,12 | 0,037 → 0,060 |
| trame 00 | 5 | 4 | 1,646 [1,614–1,695] → 1,373 [1,356–1,376] | ×1,20 | 1,887 → 1,636 | ×1,15 | 0,022 → 0,037 |
| trame 00 | 10 | 4 | 6,830 [6,790–6,859] → 6,114 [6,061–6,116] | ×1,12 | 7,838 → 7,160 | ×1,09 | 0,034 → 0,061 |
| quart 01 | 5 | 4 | 0,250 [0,244–0,263] → 0,192 [0,191–0,193] | ×1,30 | 0,287 → 0,231 | ×1,24 | 0,006 → 0,009 |
| quart 01 | 10 | 4 | 0,982 [0,977–0,985] → 0,832 [0,824–0,841] | ×1,18 | 1,118 → 0,969 | ×1,15 | 0,008 → 0,014 |

Lecture :

- **Gains.** L'étage des boîtes gagne ×1,19 à ×1,30 à K = 5 et ×1,12 à ×1,19 à K = 10, à 1 comme à 4 fils. C'est ce
  qu'annonçait la sonde (−10 à −20 %). Le catalogue entier gagne ×1,09 à ×1,24.
- **Coût de la frontière.** `t_frontier` augmente de 60 à 100 %. Avec un éventail de 2, il faut environ trois fois plus
  de tours pour atteindre la cible de 64 × P tâches, et les premiers tours sont peu parallèles. En valeur absolue,
  cela reste de 0,01 à 0,07 s, mais à 48 fils c'est la partie qui peut peser : à surveiller sur G4 (§ 8).

## 7. Grand livre

| compteur | statut J2c |
| --- | --- |
| `balls`, `levels`, `by_q_p`, `extended`, `weighted`, `max_shell` | propriétés du catalogue : valeurs identiques à `568d45297` sur les 10 entrées |
| `nodes`, `leaves`, `skipped_bbox`, `sum_m`, `max_m`, `stalled_leaves`, `filter_tests` | même sens, valeurs différentes (arbre binaire ajusté) ; égales à 1 et 4 fils ; identité ignorés = nœuds − feuilles − (nœuds − 1)/2 |
| `leaf_dominance_tests`, `pair_tests`, `triple_tests`, `line_hits`, `quad_tests`, `judged` | même sens, valeurs différentes (autres feuilles) |
| `preskipped_bbox` | **retiré** avec A6, qui ne se déclenche plus jamais : clé absente du JSON plutôt qu'un zéro permanent |

- `bench/scaling/scale_run.py` lit `triple_tests` : les exposants d'échelle mesurés avant et après J2c ne se
  comparent pas, puisque les feuilles changent.
- Toute comparaison de grands livres d'arbre avec J2 ou `568d45297` est sans objet ; seuls le catalogue et les
  niveaux restent comparables.

## 8. Risques

1. **Conceptions GPU « arbre » (J5, J6).** La voie complète supposait un octree.
   - Ce qui ne vaut plus :
     - nœuds repérés implicitement par (niveau, clé de Morton), sans stockage de boîte ;
     - huit enfants par nœud, donc un éventail fixe et un indexage direct ;
     - boîtes dyadiques cubiques ;
     - « niveaux de tête » de GEN_v2 § 10.2 (193 nœuds) décrits pour l'octree.
   - Ce que J2c impose à une voie GPU :
     - des boîtes explicites (6 entiers par nœud ; en relatif, 32 bits suffisent sous la racine) ;
     - une réduction min/max de la liste par nœud avant la coupe (l'ajustement), soit un passage de plus par niveau ;
     - un éventail de 2, avec une profondeur environ triple et 33 à 40 % de nœuds en moins.
   - La récursion en profondeur du CPU ne change pas de nature.
   - Les noyaux de feuille reçoivent des pavés quelconques. La feuille CPU le fait déjà (tests par axe). Le prototype
     `leaf_fast.cuh` de la voie partielle (J3a, J5) est à vérifier sur ce point : toute hypothèse d'un h unique serait
     fausse.
   - Les feuilles sont moitié moins nombreuses et un peu plus longues (m moyen 11,4 → 14 à K = 5 ; 18,4 → 21,6 à
     K = 10). La charge par lancement de feuille augmente donc, et le nombre de lancements baisse.
   - La décision D-GPU (règle des 30 % par étage) se prend sur les nouveaux profils.
2. **Frontière et tâches (J1).**
   - Avec un éventail de 2, il faut environ trois fois plus de tours pour atteindre 64 × P tâches.
   - Les premiers tours ont peu de parallélisme : racine seule, puis 2, puis 4 tâches. Les nœuds du haut ont les plus
     longues listes, mais leur filtre reste court (au plus 3K tests par site).
   - `t_frontier` est relevé dans les mesures (§ 6) : +60 à +100 % en local, de 0,01 à 0,07 s en valeur absolue.
     À 48 fils, la cible passe à 3 072 tâches, soit environ 12 tours binaires au lieu de 4. À mesurer sur G4.
   - Si le coût y est notable, la parade serait de développer plusieurs niveaux par tour (profondeur 3, soit 8
     tâches). La frontière reste une coupe de l'arbre, donc le catalogue et l'arbre n'en dépendent pas.
3. **Stagnation.**
   - Le critère garde le rétrécissement géométrique de l'octree : volume divisé par 2^9 sans décroissance. Il est
     contrôlé sur des grilles et des sphères entières.
   - Une entrée quasi dégénérée pourrait arrêter la descente un peu plus tôt ou plus tard qu'avant. Cela ne change
     jamais le catalogue. Cela ne peut changer qu'un refus `wide_leaf`, identique sur la sphère de 312 points testée.
4. **Profondeur de récursion** : environ 75 niveaux au plus, contre 25, avec des cadres petits. Rien d'observé sous
   ASan.
5. **Mesures locales.** Elles viennent d'un Zen 3 partagé. La mesure à 48 fils sur G4 reste à faire par le
   coordinateur.

## 9. Fichiers et reproduction

Tous dans `build/v10-j2/j2c/`, avec leurs empreintes dans `SHA256SUMS` :

- **Patch** : `J2c.patch`.
- **Arbres** :
  - `base/` : `7eee86c53` vierge ;
  - `base_b662/` : `b662673b2` vierge ;
  - `morsehgp3D_v10/` : `b662673b2` + J2c ;
  - `build/`, `build-avant/` (7eee86c53), `build-avant-b662/`.
- **Binaires** :
  - `mhgp10_catalogue.j2c` (final) ;
  - `mhgp10_catalogue.j2c_poison` ;
  - `mhgp10_catalogue.avant_b662` ;
  - `mhgp10_catalogue.avant_7eee86c53` ;
  - `mhgp10_catalogue.j2c_b662` (binaire de la calibration, table M d'avant le passage 32 → 28) ;
  - `mhgp10_catalogue.j2c_m0` (même code, base `7eee86c53`).
- **Scripts** :
  - `differentiel_j2c.sh`, `degen_j2c.sh`, `mutants_j2c.py`, `mutants_j2c.sh` ;
  - `calibre_m.sh`, `mesure_j2c.sh`, `controles_j2c.sh` (enchaînement des contrôles).
- **Sorties** :
  - exactitude : `differentiel_j2c.txt`, `differentiel_j2c_poison.txt`, `ledger_j2c.jsonl`, `niveaux_j2c.txt`,
    `degen_j2c.txt`, `degen_j2c_poison.txt` ;
  - sanitizers : `sanitizers_j2c.txt`, `tsan_j2c.txt` ;
  - portes et mutants : `ctest_gate_j2c.txt`, `mutants_j2c.txt` ;
  - calibration : `calibre_m.tsv`, `calibre_m.txt`, `calibre_m_bornes.tsv`, `calibre_m_bornes.txt`,
    `calibre_charge.txt`, `calibre_m_fenetre_portes.tsv` (écartée) ;
  - mesures : `mesure_j2c.tsv`, `mesure_j2c.txt`, `mesure_charge.txt` ;
  - journal : `controles_j2c.log`.
- **Entrées dégénérées** : `/tmp/j2work/degen/*.u32le`, régénérées par `gen_degen.py`.

```bash
cd /workspaces/E-HGP/build/v9-open-worktree && git archive b662673b2 morsehgp3D_v10 | tar -x -C <dir>
cd <dir> && git apply /workspaces/E-HGP/build/v10-j2/j2c/J2c.patch
cmake -S morsehgp3D_v10 -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build --parallel 4
PYTHONDONTWRITEBYTECODE=1 ctest --test-dir build -L gate
/workspaces/E-HGP/build/v10-j2/j2c/differentiel_j2c.sh build/mhgp10_catalogue \
    /workspaces/E-HGP/build/v10-j2/build-base/mhgp10_catalogue
```
