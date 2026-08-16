# Question bloquante — la partition des paires est quadratique, WSPD compris

Date : 16 août 2026 UTC.
Dossier : `morsehgp3D_v3/`.
Fait suite à `NOTE_CLAUDE_Q2_BOUT_EN_BOUT_20260816.md` et à la mise en mesure de
q2 aux tailles d'intérêt.

Cadre :

```text
phase=exploration_v3_hors_registre
backend=cpu_reference
profile=quantized_u16_input_only
mode=diagnostic_counter_only
public_status=not_claimed
```

Reçus **développeur**, aucun workflow attaché.

---

## 0. Ce qui a changé de statut

Les tailles de nuage d'intérêt sont désormais inscrites normativement :
**`n = 8000`, `16000`, `32000`** (`docs/TEST_PLAN_MORSEHGP3D.md` § 3.1), avec
une seconde règle : **aucune vérification exhaustive** (§ 3.2). Ce qu'un
théorème garantit est invoqué, pas re-parcouru ; ce qui reste à tester est la
faute d'implémentation, et elle se voit sur un invariant global, un juge
d'échantillon ou un mutant.

C'est en appliquant la première que la seconde est devenue nécessaire, et que le
problème ci-dessous est apparu. Il n'était pas visible à `n = 120`.

---

## 1. Le fait

q2 tourne aux trois tailles. La **correction tient** : masse de partition
exactement `C(n,2)` — `511 984 000` à `n = 32000` —, zéro écart au juge
d'échantillon, zéro violation de l'invariant `lower <= N <= upper`, zéro
dépassement de cap ponctuel.

Le **coût**, lui, est quadratique :

| `n` | états traités | temps | classifications |
|---|---|---|---|
| `8 000` | `530 752` | `24 s` | `85,7 M` |
| `16 000` | `2 110 080` | `72 s` | `272,7 M` |
| `32 000` | `8 414 464` | `254 s` | `973,5 M` |

Les rapports d'états sont `×3,98` puis `×3,99` pour un doublement de `n`. C'est
`n^2` exactement, pas approximativement.

**La cause immédiate est ma faute, et elle tient en une ligne.** Ma condition
d'acceptation d'un rectangle est un cap de MASSE :

```text
|A| * |B| <= cap_rect
```

Elle force la descente jusqu'à des rectangles de quelques dizaines de paires,
donc elle en produit `C(n,2)/cap` — quadratique par construction. J'ai écrit
exactement le catalogue `∝ C(n,k)` que `AGENTS.md` interdit, dans un probe dont
le reste respecte l'invariant. La condition correcte est la **séparation**, pas
la masse, et le dossier a déjà `wspd_wavefront.hpp`.

---

## 2. RETRACTATION — le WSPD n'est PAS quadratique

**J'ai poussé une version de cette note qui affirmait que le WSPD du dossier
croît en `n^1,44`. C'est faux, et je le corrige avant que vous ne répondiez
dessus.**

L'exposant `1,44` était mesuré sur `n` allant de `1 000` à `8 000`, c'est-à-dire
entièrement dans le **régime transitoire** où le WSPD est encore plafonné par
`C(n,2)`. Confronté à la théorie `O(s^d n)` de Callahan-Kosaraju, sur `uniform`
à `s = 8` — donc `s^3 = 512` :

| `n` | mesuré | `s^3 n` | mesuré / théorie | `C(n,2)` | part des paires |
|---|---|---|---|---|---|
| `1 000` | `202 773` | `512 000` | `0,40` | `499 500` | `40,6 %` |
| `2 000` | `552 075` | `1 024 000` | `0,54` | `1 999 000` | `27,6 %` |
| `4 000` | `1 456 727` | `2 048 000` | `0,71` | `7 998 000` | `18,2 %` |
| `8 000` | `3 957 383` | `4 096 000` | **`0,97`** | `31 996 000` | `12,4 %` |

Le rapport à la théorie **monte vers `1`** au lieu de diverger. À `n = 1000` le
WSPD représente `40,6 %` de toutes les paires du nuage : il ne *peut pas*
croître à sa pente asymptotique, il est borné par le nombre total de paires. Ce
que j'ai pris pour un exposant superlinéaire était la sortie de ce plafond.

La signature est encore plus nette sur la dimension effective implicite,
`d = log(rectangles / n) / log(s)` à `n = 8000`, `s = 8` :

| famille | `d` mesuré | géométrie |
|---|---|---|
| `uniform` | `2,98` | volumique 3D |
| `eight_clusters` | `2,64` | amas |
| `terrain` | `2,30` | relief 2,5D |
| `scanline_single_pass` | `2,06` | balayage 2D anisotrope |

Ce sont exactement les dimensions attendues. **Le WSPD du dossier est conforme
à sa théorie** : linéaire en `n`, avec la constante `s^d`.

Ma lecture (b) de la version précédente — « l'exposant serait une propriété du
profil » — était doublement fausse : Callahan-Kosaraju est inconditionnel en
dimension fixe, donc aucun profil ne peut produire un exposant `> 1`, et
l'exposant n'existait pas.

J'ai aussi testé mon autre hypothèse, celle du *fair split tree*. Mon arbre
coupe à la médiane du **rang** de Morton, ce qui ne garantit aucune décroissance
de diamètre. Mesuré à `n = 8000`, part des nœuds internes dont le diamètre ne
décroît pas (`ratio > 0,99`) :

| famille | médiane du rang | fair split (côté long) |
|---|---|---|
| `uniform` | `2,3 %` | `0,0 %` |
| `terrain` | `3,6 %` | `0,3 %` |
| `eight_clusters` | `2,8 %` | `0,0 %` |
| `scanline_single_pass` | `8,2 %` | `2,4 %` |

L'arbre est bien moins bon qu'un fair split, mais **la corrélation est
inverse** : `scanline` a le plus de nœuds stagnants (`8,2 %`) et le *moins* de
rectangles (`580 421`) ; `uniform` a le moins de stagnants (`2,3 %`) et le plus
de rectangles (`3 957 383`). Les stagnants n'expliquent donc pas le compte. Cette
hypothèse tombe aussi.

---

## 3. Le vrai problème, reformulé

Ce n'est pas un exposant, c'est une **constante**, et un **désalignement entre
le travail et la sortie**.

À `n = 32 000` :

```text
paires du nuage                     511 984 000
rectangles WSPD s=8 (extrapolation)  ~16 400 000
etats de ma partition (cap de masse)   8 414 464
aretes q2 VIVANTES                        656 652
```

Ma partition à cap de masse est quadratique — `×3,98` puis `×3,99` par
doublement — mais elle est encore *plus petite* que le WSPD à cette taille ; le
croisement se situe entre `32 000` et `64 000`. Aux tailles d'intérêt, les deux
structures produisent le même ordre de grandeur, environ `10^7` rectangles.

Et **aucune des deux n'est guidée par la sortie** : `10^7` rectangles pour
`6,6 \cdot 10^5` arêtes vivantes, soit un facteur `15` à `25` de sur-travail qui
ne diminue pas avec `n`. Remplacer mon cap de masse par le WSPD corrigerait
l'asymptotique sans rien changer au régime qui nous intéresse.

---

## 4. Ce qui pousse vers (c), et pourquoi je n'ose pas le poser seul

Une observation de ma campagne, à `h_2 = 10` :

| `n` | arêtes mortes | arêtes vivantes | fraction vivante |
|---|---|---|---|
| `8 000` | `31 834 742` | `161 258` | `0,50 %` |
| `16 000` | `127 666 419` | `325 581` | `0,25 %` |
| `32 000` | `511 327 348` | `656 652` | `0,13 %` |

Le nombre d'arêtes **vivantes** croît en `×2,02` puis `×2,02` pour un doublement
de `n` : il est **linéaire**. La sortie est linéaire, l'entrée traitée est
quadratique. Nous payons `n^2` pour produire `Θ(n)`.

Cela ressemble beaucoup à un problème où la structure d'énumération devrait être
guidée par la sortie et non par la couverture. Mais je ne sais pas énoncer le
théorème qui garantirait qu'un voisinage borné contient **toutes** les arêtes
vivantes, et sans ce théorème une telle structure serait exactement le genre de
raccourci que le dossier refuse : une heuristique qui marche sur les familles
testées.

---

## 5. Questions

**Q1 — le choix de `s`.** La séparation ne sert PAS à la correction. Le prédicat
de bloc q2 utilise les extrema exacts de `Phi` sur les trois boîtes ; il est juste
quelle que soit la séparation. `s` ne gouverne donc qu'un **arbitrage de coût** :

```text
s petit  -> moins de rectangles, boîtes moins separees, plus de MIXTE, plus de raffinement
s grand  -> plus de rectangles, decision plus souvent immediate
```

Mesuré à `n = 8000`, `s = 6 / 8 / 10` : `uniform` passe de `2 440 769` à
`3 957 383` ; `terrain` de `621 946` à `948 005` puis `1 312 284` ; `scanline`
de `377 007` à `580 421` puis `813 373`. La dépendance mesurée est `s^1,5`, ce
qui est *moins* que `s^d` — cohérent avec le fait que le régime `s^d` n'est
atteint qu'asymptotiquement.

Existe-t-il une raison de fixer `s = 8` autre que l'héritage ? Le dossier a-t-il
un `s` minimal admissible, ou faut-il le choisir par mesure du produit
`rectangles x raffinement` ? Je peux mesurer cet optimum, mais je ne veux pas
inventer un critère si le dossier en a déjà un.

**Q2 — la structure, et c'est la vraie question.** Ni le cap de masse ni le WSPD
ne sont guidés par la sortie : `10^7` rectangles pour `6,6 \cdot 10^5` arêtes
vivantes à `n = 32 000`, et le rapport ne s'améliore pas avec `n`. Existe-t-il,
dans le manuscrit ou dans le registre des preuves, un énoncé du type

```text
toute arete q-vivante joint deux points a distance au plus f(h_q) fois
la distance au k-ieme voisin
```

qui autoriserait un voisinage borné avec **garantie de complétude** ? Sans un
tel théorème, une structure guidée par la sortie serait exactement le raccourci
que le dossier refuse. Avec lui, elle devient la structure correcte.

Je note un obstacle que j'ai trouvé en essayant de l'établir seul, et qui montre
que l'énoncé naïf est faux. Sur une droite, `a = 0`, `b = 10`, aucun point dans
`(0,10)`, et mille points dans `[-10, 0)` : l'arête `(a,b)` est vivante, et `b`
est le `1001`-ième plus proche voisin de `a`. **Le rang de voisinage n'est donc
pas borné**, et une structure de type `k`-plus-proches-voisins ne peut pas être
complète telle quelle. Ce qui reste possible est une caractérisation par la
géométrie du milieu, cf. Q3.

**Q3 — le certificat de mort par distance, exact.** Il existe et il est simple.
Avec `m` le milieu de `[a,b]` et `r_k(x)` la distance de `x` à son `k`-ième plus
proche voisin dans `P` :

```text
(a,b) est q2-MORTE  <=>  r_{h_2}(m) <= |ab| / 2
```

C'est une équivalence, pas une borne : `W_2(a,b)` est exactement `B(m, |ab|/2)`.
Le problème est que `m` n'est pas un point du nuage, donc `r_{h_2}(m)` n'est pas
indexable par un pré-calcul par point.

La question est de savoir si l'on peut relâcher vers quelque chose d'indexable
et **conservateur du bon côté** — un certificat qui ne tue jamais une arête
vivante, quitte à en manquer des mortes. Je n'ai pas trouvé : toute boule
centrée en `a` intersecte `W_2(a,b)` sans y être incluse, puisque `a` est
exactement sur la sphère. Voyez-vous un relâchement admissible, ou faut-il
accepter que le certificat exige une requête au milieu — donc une structure de
requête sur `m`, ce qui change la nature du problème ?

## 6. Ce que je ne fais pas en attendant

Je ne branche pas le WSPD existant à la place de mon cap de masse. Non parce
qu'il serait mauvais — il est conforme à sa théorie —, mais parce qu'aux tailles
d'intérêt les deux produisent le même ordre de grandeur, `10^7` rectangles, et
que le remplacement corrigerait une asymptotique sans changer le régime qui nous
occupe. Ce serait du travail qui ressemble à une solution.

Je ne touche pas non plus à `wspd_wavefront.hpp` : il n'a rien à réparer.

Ce que je verse au dossier en revanche, parce que personne ne l'avait mesuré :
le WSPD n'atteint son régime `s^d n` qu'à partir de `n` de l'ordre de `8 000`.
En dessous il est plafonné par `C(n,2)` — à `n = 1000` il couvre `40,6 %` de
toutes les paires. **Toute mesure de pente faite sous cette taille est fausse**,
et c'est exactement l'erreur que je viens de commettre.

---

## 7. La campagne, en entier

`n` dans `{8000, 16000, 32000}`, `s` dans `{6, 8, 10}`, quatre familles,
`seed=3`, `combined_prefilter_probe`. Toutes les exécutions rendent
`ecart=0` sur le ledger de masse : la partition couvre exactement `C(n,2)`.

| famille | `s` | `n=8000` | `n=16000` | `n=32000` | exp. `8k->16k` | exp. `16k->32k` |
|---|---:|---:|---:|---:|---:|---:|
| `eight_clusters` | `6` | `1 318 319` | `3 399 832` | `7 947 899` | `1.37` | `1.23` |
| `eight_clusters` | `8` | `1 944 388` | `5 216 527` | `12 583 384` | `1.42` | `1.27` |
| `eight_clusters` | `10` | `2 624 373` | `7 258 640` | — | `1.47` | — |
| `scanline_single_pass` | `6` | `377 007` | `801 363` | `1 659 389` | `1.09` | `1.05` |
| `scanline_single_pass` | `8` | `580 421` | `1 241 999` | `2 595 906` | `1.10` | `1.06` |
| `scanline_single_pass` | `10` | `813 373` | `1 756 144` | `3 698 899` | `1.11` | `1.07` |
| `terrain` | `6` | `621 946` | `1 506 898` | `3 591 439` | `1.28` | `1.25` |
| `terrain` | `8` | `948 005` | `2 291 578` | `5 535 416` | `1.27` | `1.27` |
| `terrain` | `10` | `1 312 284` | `3 163 428` | `7 720 807` | `1.27` | `1.29` |
| `uniform` | `6` | `2 440 769` | `5 545 073` | `12 616 686` | `1.18` | `1.19` |
| `uniform` | `8` | `3 957 383` | `9 473 939` | `22 247 725` | `1.26` | `1.23` |
| `uniform` | `10` | `5 669 146` | `14 198 380` | — | `1.32` | — |

Rectangles **par point** — constant si et seulement si le régime est linéaire :

| famille | `s` | `8k` | `16k` | `32k` |
|---|---:|---:|---:|---:|
| `eight_clusters` | `6` | `164.8` | `212.5` | `248.4` |
| `eight_clusters` | `8` | `243.0` | `326.0` | `393.2` |
| `eight_clusters` | `10` | `328.0` | `453.7` | — |
| `scanline_single_pass` | `6` | `47.1` | `50.1` | `51.9` |
| `scanline_single_pass` | `8` | `72.6` | `77.6` | `81.1` |
| `scanline_single_pass` | `10` | `101.7` | `109.8` | `115.6` |
| `terrain` | `6` | `77.7` | `94.2` | `112.2` |
| `terrain` | `8` | `118.5` | `143.2` | `173.0` |
| `terrain` | `10` | `164.0` | `197.7` | `241.3` |
| `uniform` | `6` | `305.1` | `346.6` | `394.3` |
| `uniform` | `8` | `494.7` | `592.1` | `695.2` |
| `uniform` | `10` | `708.6` | `887.4` | — |

### Lecture

`scanline_single_pass` est **linéaire et convergé** : `72,6 -> 77,6 -> 81,1`
rectangles par point à `s = 8`, exposant `1,05` sur le dernier doublement.
Conforme à Callahan-Kosaraju, régime atteint.

Les trois autres ne le sont pas encore à ces tailles. `terrain` tient un
exposant **`1,27` stable sur deux doublements**, ce qui ressemble à une pente et
non à une convergence en cours. `uniform` dépasse la théorie : rapport à
`s^{3} n` de `0,97` à `n = 8000`, puis `1,16` à `n = 16000`.

### Un facteur confondant que je signale avant de conclure

`cloud_family_default_coord` fait varier l'emprise avec `n` pour tenir une
densité cible. **Les familles ne sont donc pas auto-similaires sous `n`** :
l'exposant mesuré mélange le scaling de `n` et un changement de géométrie du
nuage. Un contrôle à emprise FIXE est en cours ; je ne conclus pas sur
l'exposant avant de l'avoir.

### Le fait qui ne dépend pas de l'exposant

Il faut `495` à `887` rectangles par point sur `uniform`, contre `73` à `116`
sur `scanline`. À `n = 32 000` cela fait de l'ordre de `10^{7}` rectangles pour
`6,6 \cdot 10^{5}` arêtes vivantes. C'est le coût réel, linéaire ou non, et ni
mon cap de masse ni le WSPD ne le réduisent — c'est pourquoi Q2 est la question
qui compte.
