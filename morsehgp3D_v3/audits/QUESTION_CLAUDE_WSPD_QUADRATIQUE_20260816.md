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

## 2. Ce qui m'arrête, et c'est la question

**Le WSPD du dossier n'est pas linéaire non plus.** Mesuré sur `uniform`,
séparation `s = 8`, avec `combined_prefilter_probe` :

| `n` | rectangles | rapport |
|---|---|---|
| `1 000` | `202 773` | — |
| `2 000` | `552 075` | `×2,72` |
| `4 000` | `1 456 727` | `×2,64` |
| `8 000` | `3 957 383` | `×2,72` |

Exposant empirique **`1,44`**, stable sur trois doublements. Un WSPD est censé
produire `O(s^3 n)` paires bien séparées, donc **linéaire en `n`** à `s` fixé,
avec une constante qui dépend de `s` et de la dimension — pas un exposant `1,44`.

À `n = 8000` cela fait déjà `3,96` millions de rectangles pour `32,0` millions de
paires, soit `8,1` paires par rectangle. Un WSPD sain devrait factoriser bien
davantage.

Une campagne systématique est en cours — `n` dans `{8000, 16000, 32000}`,
`s` dans `{6, 8, 10}`, quatre familles — et ses chiffres seront joints. Mais la
question ne dépend pas de son issue.

---

## 3. Les trois lectures possibles, et je ne sais pas trancher

**(a) L'implémentation descend trop.** Si le critère d'arrêt teste une
séparation approchée par des boîtes au lieu de la séparation réelle, ou s'il
compare `dist` à `max(diam)` avec la mauvaise convention, l'arbre descend
au-delà du nécessaire et le compte gonfle. Ce serait un défaut local,
réparable, et l'exposant `1,44` serait un artefact.

**(b) L'exposant `1,44` est réel pour ce profil.** Le WSPD est linéaire pour un
nuage de dimension *doublante* bornée. Un nuage u16 quantifié avec des amas
denses, ou un balayage LiDAR très anisotrope, peut avoir une dimension doublante
effective plus grande à ces échelles, et le compte de paires suit. Dans ce cas
`1,44` n'est pas un bug mais une propriété du profil, et il faut le déclarer au
lieu de le combattre.

**(c) Le WSPD n'est pas la bonne structure ici.** Le WSPD garantit que toute
paire de points est *couverte* par exactement une paire bien séparée. Or je n'ai
pas besoin de couvrir toutes les paires : j'ai besoin des paires qui peuvent
**porter une arête vivante**. Une arête q2 morte l'est parce que dix témoins
sont dans sa boule diamétrale, et une paire très éloignée dans un nuage dense
est morte d'avance. Peut-être la structure correcte n'est-elle pas une partition
complète des paires mais un **voisinage borné** — quelque chose comme un graphe
de Yao ou un `k`-plus-proches-voisins élargi —, avec un théorème disant que
toute arête vivante y est.

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

**Q1 — l'exposant `1,44`.** Est-il connu de vous comme un défaut du
`wspd_wavefront` du dossier, ou comme une propriété assumée du profil u16 ? Si
c'est un défaut, savez-vous où il se situe — critère de séparation, convention
de diamètre, ou descente lockstep ? Je peux instrumenter, mais je préfère ne pas
partir de mon hypothèse préférée.

**Q2 — la structure.** Faut-il réparer le WSPD, ou faut-il changer d'objet ?
Existe-t-il, dans le manuscrit ou dans le registre des preuves, un énoncé du
type « toute arête `q`-vivante joint deux points à distance au plus `f(h_q)`
fois la distance au `k`-ième voisin » qui autoriserait un voisinage borné avec
garantie de complétude ? Si oui, c'est lui qu'il faut brancher ; si non, la
question devient : quel théorème faut-il établir ?

**Q3 — le seuil comme levier.** `h_2 = 10` rend `99,87 %` des arêtes mortes à
`n = 32000`, et la fraction morte **croît** avec `n`. Une paire dont les deux
extrémités sont éloignées dans un nuage dense est morte pour une raison
purement métrique. Y a-t-il un certificat de mort par **distance seule** —
quelque chose comme « si `|ab|` dépasse le rayon couvrant `h_q` points autour du
milieu, l'arête est morte » — qui permettrait d'éliminer un rectangle entier
avant toute descente sur les témoins ? Ce serait un prune à l'entrée de la
partition et non à l'intérieur du ledger, donc il attaquerait le `n^2` à sa
source.

---

## 6. Ce que je ne fais pas en attendant

Je ne branche pas le WSPD existant à la place de mon cap de masse : cela ferait
passer de `n^2` à `n^1,44` et donnerait l'impression d'avoir résolu le problème
alors que la structure resterait superlinéaire pour une sortie linéaire. Je
préfère une réponse à Q2 avant de choisir.

Je ne touche pas non plus à `wspd_wavefront.hpp` : c'est un composant partagé,
antérieur à ce chantier, et son exposant n'avait jamais été mesuré sur une
rampe — ce qui est en soi un fait à verser au dossier.
