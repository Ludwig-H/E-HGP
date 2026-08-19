# Addendum — préfiltre q4 par puissance équatoriale : exact, nécessaire, et mesuré

Date : 19 août 2026 UTC. Exécute la réponse d'audit `5b89bc6`
(« un préfiltre exact q4 en seules longueurs carrées existe »), dans
l'ordre qu'elle recommande.

Le verrou posé la veille était : *41 % des paires (seed, y) meurent au
prédicat le plus cher — existe-t-il une condition nécessaire bon marché,
en longueurs carrées, qui les écarte avant la forme de Cramer ?*
L'auditeur répond oui, et la caractérisation est exacte.

## 1. Le lemme, redéroulé

Face `(a,b,c)`, sommet opposé `d`. Soient `o_F` le circumcentre de la
face dans son plan, `R_F` son rayon, `n` la normale unitaire. Le
circumcentre `o` du tétraèdre vit sur `o_F + t n`, et
`d = pi_F(d) + h n` avec `h != 0`. De `|o-d|² = |o-a|² = R_F² + t²` :

```text
|o_F - pi_F(d)|² + (t-h)² = R_F² + t²
=> |d - o_F|² - 2 t h = R_F²
=> 2 t h = |d - o_F|² - R_F² = Pow_F(d).
```

`o` et `d` sont donc strictement du même côté de la face **ssi**
`Pow_F(d) > 0`. Le centre est strictement intérieur ssi c'est vrai pour
les quatre faces : la conjonction est une **caractérisation**, et chaque
face prise seule est une condition **nécessaire** — donc un préfiltre
valide.

Ce n'est pas l'acuité des faces. La distinction que les fixtures v3
imposent (« bien centré » et « à faces aiguës » se réfutent
mutuellement) est respectée : on teste la puissance du sommet opposé à
la boule équatoriale, pas des angles.

**Forme entière, sans centre ni division.** Avec `p = b-a`, `q = c-a`,
`r = d-a`, `A = |p|²`, `B = |q|²`, `C = p·q`, `D = p·r`, `E = q·r`,
`F = |r|²`, le Gram vaut `Delta = AB - C² > 0`, et

```text
o_F - a = alpha p + beta q,  alpha = B(A-C)/(2 Delta),
                             beta  = A(B-C)/(2 Delta),
Pow_F(d) = F - 2 r·(o_F-a) = F - (B(A-C)D + A(B-C)E)/Delta.
```

Le signe est celui de `Delta F - B(A-C)D - A(B-C)E`. En doublant les
produits scalaires (`C2 = A + B - |bc|²` par la loi des cosinus, etc.),
quatre fois ce nombre s'écrit

```text
(4AB - C2²) F - B(2A - C2) D2 - A(2B - C2) E2.
```

**Largeurs, vérifiées et non reprises.** Profil u16 : `|Δcoord| ≤ 65535`,
longueur carrée `≤ 3·65535² < 2^33,6`. Donc `|C2| < 2^34,6`,
`|4AB - C2²| < 2^70,2`, chaque produit `< 2^103,8`, la somme
`< 2^105,4` — **i128**, avec plus de vingt bits de marge. (L'audit
annonçait ~`2^102` ; le calcul détaillé donne `2^105,4`, ce qui ne
change pas la conclusion.)

## 2. Porte d'équivalence contre l'autorité Cramer

`mhgp4_q4_oracle` compare désormais, sur **tous** les tétraèdres des
petits nuages, la conjonction des quatre puissances au test des
orientations de Cramer :

```text
q4_oracle : 59 825 tetraedres, 4 880 supports, desaccords=0
puissances_equatoriales interieurs=4 880 exterieurs=52 750
                        frontiere=8 358 aigus_non_centres=718
                        obtus_centres=1 930 desaccords=0
```

Les planchers sont ceux que l'audit demande, et ils sont tous non
vides : **8 358 configurations de frontière** (une puissance nulle =
centre dans le plan d'une face, que le contrat strict doit refuser),
**718 tétraèdres à faces toutes aiguës mais non bien centrés**, et
**1 930 bien centrés avec une face obtuse** — les deux configurations
que les fixtures v3 opposent. Sans elles la porte ne dirait rien.

Trois mutants tués (code 4) : `q4-equatorial-nonstrict` (`>= 0` au lieu
de `> 0` — la frontière passerait), `q4-equatorial-drop-cross` (`C2²`
retiré du Gram), `q4-equatorial-opposite-sign`.

## 3. Rendement, mesuré avant de brancher

L'audit recommande d'instrumenter **une seule** face — `abx`, sommet
opposé `y` — parce que `a,b,x` est fixe pendant toute la boucle des
complétions d'un seed : ses coefficients `A, B, C2, H, U, V`
s'amortissent une fois par seed, et il ne reste que trois produits i128
par paire.

Fraction des rejets du centre capturée **avant** `q4_form`, à n=800 :

| famille | paires testées | rejets | part des rejets du centre | faux rejets |
|---|---|---|---|---|
| uniform | 5 900 166 | 3 919 513 | **81,2 %** | 0 |
| eight_clusters | 8 255 924 | 4 156 349 | **63,7 %** | 0 |
| terrain | 302 719 | 244 290 | **85,9 %** | 0 |

À n=8000 (uniform) : 87 499 759 paires testées, **57 274 981 rejets**,
soit **80,7 %** des 70 989 328 rejets du centre — le rapport tient à
l'échelle.

Une seule face suffit donc à capturer les quatre cinquièmes. Les trois
autres n'ont pas été branchées : leur rendement marginal porterait sur
le cinquième restant, pour un coût par paire non amorti.

## 4. Le câblage est vérifié sur le flux réel, pas seulement la primitive

L'oracle reçoit la **primitive**. Il ne peut rien dire du **câblage** —
quelle face, quel sommet opposé, quelles six longueurs. Un compteur de
**faux rejets** le fait : dans un run où Cramer tranche chaque paire,
toute paire rejetée par la puissance mais gardée par Cramer est comptée.

Porte `--q4-eq-gate`, trois familles, deux runs dans le même processus :

```text
q4_eq_gate violations=0 rejets=1 742 517 faux_rejets=0
```

flux identique après tri et RLE, zéro faux rejet, plancher de rejets.
Mutant `q4-eq-wrong-length` (deux des six longueurs échangées — le
prédicat n'est plus la puissance d'aucune face, donc plus une condition
nécessaire) : **91 282 faux rejets**, tué en code 4.

Premier mutant écrit pour cette porte : une injection directe du
compteur dans la porte elle-même. Il passait, mais il n'exerçait pas le
vrai chemin — remplacé, et c'est le remplaçant qui est gravé.

## 5. Coût, banc apparié contrebalancé (n=8000, taille d'intérêt)

Même discipline que le banc d'internement — échauffement non
chronométré, ordre ABBA, plan refusé si `--bench-repeat` est impair ou
`< 4`, signature du flux vérifiée à chaque exécution, médiane des
rapports **appariés** comme estimateur.

Rapports appariés (préfiltre / sans préfiltre), dix paires :

```text
0,9694  0,9457  0,9496  0,9968  0,9362
0,9996  0,9167  0,9897  0,9854  0,9397

mediane_appariee    = 0,9595  -> x1,042   (ESTIMATEUR)
mediane_log         = -0,0414 -> x1,042   (coherent)
rapport_de_medianes = 0,9639
victoires_prefiltre = 10/10   P(X>=10 | Bin(10;1/2)) = 1/1024
ordre_sans_premier  = 5/5     flux_identique = oui
```

**×1,042 sur la génération entière**, dix victoires sur dix. Le gain est
modeste parce que la complétion n'est qu'une part de `t_gen` et que
`q4_form` n'est pas tout le coût d'une paire rejetée ; il est en
revanche **certain** — le plan le supporte, et l'objet ne bouge pas d'un
bit.

## 6. Ce qui reste ouvert

- Les **trois autres faces** : à ne brancher que si le cinquième restant
  le justifie, mesure appariée à l'appui.
- Le préfiltre ne touche pas le poste amont (`t_anchor_cover`,
  12,6 s CPU) ni la descente (déjà parallèle).

## 7. Reproduction

```bash
cmake -S morsehgp3D_v4 -B build/v4 -DCMAKE_BUILD_TYPE=Release
cmake --build build/v4 -j
ctest --test-dir build/v4 --output-on-failure          # 141 tests
./build/v4/mhgp4_q4_oracle                             # equivalence + planchers
./build/v4/mhgp4_q4_oracle --inject=q4-equatorial-nonstrict     # code 4
./build/v4/mhgp4_forest_probe --q4-eq-gate                      # cablage
./build/v4/mhgp4_forest_probe --q4-eq-gate --inject=q4-eq-wrong-length  # code 4
./build/v4/mhgp4_forest_probe --q4-prefilter-bench --family=uniform \
    --n=8000 --s=8 --smax=11 --seed=3 --threads=4 --bench-repeat=10
```
