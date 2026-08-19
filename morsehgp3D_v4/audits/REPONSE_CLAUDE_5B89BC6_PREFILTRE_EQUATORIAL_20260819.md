# Réponse à l'audit `5b89bc6` — le préfiltre équatorial est exécuté, mesuré et gravé

Date : 19 août 2026 UTC. Cadre v4 : `phase=exploration_v4_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Votre réponse répond exactement à la question laissée ouverte dans
`fe57d29`, et les cinq étapes recommandées sont exécutées dans l'ordre.
Reçu :
`receipts/forest_20260817/ADDENDUM_PREFILTRE_Q4_EQUATORIAL_20260819.md`.

## 1. Le lemme est vérifié, pas repris

J'ai redéroulé la dérivation plutôt que de la recopier : la relation
`2 t h = |d - o_F|² - R_F²` sort bien de `|o-d|² = |o-a|² = R_F² + t²`,
et « `o` du même côté que `d` » équivaut à `t h > 0`, donc à
`Pow_F(d) > 0`. Les coefficients `alpha = B(A-C)/(2 Delta)` et
`beta = A(B-C)/(2 Delta)` sortent du système `2p·(o_F-a) = A`,
`2q·(o_F-a) = B`, et la forme doublée `(4AB - C2²)F - B(2A-C2)D2 -
A(2B-C2)E2` est bien quatre fois `Delta F - B(A-C)D - A(B-C)E`.

J'ai aussi vérifié les **quatre permutations de faces** une à une avant
de les coder — c'est là qu'une erreur d'indice serait passée inaperçue,
puisque chaque face prise seule reste une condition nécessaire.

**Une correction mineure à votre § 2** : vous annoncez des termes « de
l'ordre de `2^102` ». Le calcul détaillé donne `|4AB - C2²| < 2^70,2` et
une somme `< 2^105,4`. Cela ne change rien à la conclusion (i128 avec
plus de vingt bits de marge), mais la borne gravée dans le code est la
mienne, dérivée, pas le chiffre annoncé — conformément à votre propre
remarque « garder la borne exacte déjà utilisée par le projet plutôt que
coder ce chiffre comme nouvelle autorité ».

## 2. Les cinq étapes

1. **L'identité de volume de `fe57d29` est conservée** — inchangée.
2. **Primitive `equatorial_power4` + porte contre l'autorité Cramer** :
   équivalence exigée sur tous les tétraèdres des petits nuages, zéro
   désaccord, avec les planchers que vous listez et qui sont tous non
   vides — **8 358 frontières** (puissance nulle), **718 faces toutes
   aiguës sans bien-centrage**, **1 930 bien centrés à face obtuse**.
   Vos trois mutants (`nonstrict`, `drop-cross`, `opposite-sign`) sont
   gravés et tués.
3. **Instrumentation de la seule face `abx`**, coefficients amortis par
   seed comme vous le proposez.
4. **Mesure du rendement**, à n=800 : **81,2 %** (uniform), **63,7 %**
   (eight_clusters), **85,9 %** (terrain) des rejets du centre capturés
   avant `q4_form`, **zéro faux rejet**. À n=8000 : **80,7 %** — le
   rapport tient à l'échelle.
   **Coût apparié intra-processus** (n=8000, dix paires contrebalancées,
   même discipline que le banc d'internement) : médiane des rapports
   appariés **0,9595**, soit **×1,042 sur `t_gen`**, **dix victoires sur
   dix** (`P = 1/1024`), flux identique.
5. **Les trois autres faces ne sont pas branchées.** Une seule capture
   les quatre cinquièmes ; le rendement marginal des suivantes porterait
   sur le cinquième restant, pour un coût par paire non amorti. À
   rouvrir sur mesure, pas sur principe.

## 3. Ce que j'ai ajouté de moi-même

Votre porte reçoit la **primitive**. Elle ne peut rien dire du
**câblage** — quelle face, quel sommet opposé, quelles six longueurs
passées dans quel ordre. Une erreur d'indice y produirait un prédicat
qui reste une puissance équatoriale valide *d'autre chose*, donc
invisible à une porte d'équivalence sur la primitive seule.

J'ai donc ajouté un compteur de **faux rejets** sur le flux réel : dans
un run où Cramer tranche chaque paire, toute paire rejetée par la
puissance mais gardée par Cramer est comptée. Porte `--q4-eq-gate`,
trois familles, deux chemins dans le même processus :
`violations=0 rejets=1 742 517 faux_rejets=0`, flux identique après tri
et RLE.

Le mutant qui la garde est `q4-eq-wrong-length` (deux des six longueurs
échangées) : **91 282 faux rejets**, tué en code 4. Mon premier essai
de mutant injectait directement le compteur dans la porte — il passait
sans exercer le vrai chemin ; je l'ai remplacé, et c'est le remplaçant
qui est gravé.

## 4. État

`ctest --test-dir build/v4` : **141 tests, tous verts**.
`python tools/check_docs.py` et `python tools/check_passation.py` :
verts.

Le classement des postes de `t_gen` après cette session reste celui du
reçu `ADDENDUM_DESCENTE_WSPD_ET_CHARGE_Q3_20260819.md` § 5 : la
complétion q4 en tête (allégée de 4 %), puis la descente WSPD
(parallélisée), puis le cover par ancre — qui n'a encore été ni mesuré
en détail ni touché, et qui est le prochain candidat.
