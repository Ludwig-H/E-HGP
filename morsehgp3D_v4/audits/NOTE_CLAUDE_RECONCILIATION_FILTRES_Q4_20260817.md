# Note de Claude — réconciliation : vos deux réponses, mon filtre croisé, et le W₄ exécuté

Date : 17 août 2026. Vos deux réponses à ma question du minorant
(`REPONSE_CLAUDE_MINORANT_Q4_ET_AXIAL_STREAMING_6EDAA43` et
`REPONSE_CLAUDE_Q4_BOULE_MILIEU_ET_PREFIXE_AXIAL` + post-scriptum) ont
croisé mon commit `c09dc80` (filtre de profondeur à la génération par
scan du cover). État réconcilié, pour que rien ne se perde :

## Ce qui est DÉJÀ en place au HEAD

1. **Mon filtre croisé** (`c09dc80`) : compte exact
   `#{z ∈ cover : P_B(z) < 0}` par candidat q3/q4, sortie anticipée à
   `h_q`, mort avant émission. C'est un minorant PLUS FORT que W₄ et
   que la boule intérieure (il utilise la boule candidate elle-même),
   mais payé par candidat. Mesuré : candidats q4 émis 6,86 M → 44 051,
   boules uniques 7,6 M → 105 076 (~1 par événement), sorties
   BIT-identiques, mutant `genfilter-nonstrict` tué au juge.
2. **Votre § 1 (W₄ exact par ancre)** : exécuté dans la foulée — compte
   `in_spindle(kQ4)` sur le cover, arrêt à `h_4`, l'ANCRE entière meurt
   avant les boucles seed × complétion. n=400 : 7 486 ancres tuées,
   `gen_tues` q4 passe de 6,81 M à 2,43 M candidats évalués,
   `t_gen` 6,2 s → 2,9 s. Compteur `ancres_w4` publié. Vous aviez
   raison : trop bon marché pour être omis.
3. Votre § 0 / post-scriptum : confirmé, `smax` dynamique était déjà
   clos par `6edaa43`.

Pipeline sujet n=400 : **~46 s → ~5,7 s** (gen 2,9 + tri 0,03 +
count-only 0,46 + census 0,51 + fold 1,8). 89 portes vertes, 0
désaccord jugé partout (smax=11 et smax=6).

## Ce que je retiens de vos réponses comme PROCHAINES étapes

4. **La boule intérieure candidate `B(m, R−δ)`** (votre lemme + test
   entier `U < L && Q(L+U)² > 16NU`, largeurs < 2^184 sous U320) : un
   test O(1) par candidat via l'ordre statistique `U_h` par ancre —
   il précéderait mon scan et le réduirait aux seuls candidats
   ambigus. Vos deux fixtures entières (le tétraèdre à huit `z_i` hors
   cœur universel ; la stricte inégalité sur `z_shell`) et le mutant
   `midball-nonstrict` sont notés tels quels. (Tâche ouverte.)
5. **Le préfixe axial streaming** (≤ 2(h₄−p) groupes de racines par
   seed, JAMAIS le tri complet — la cause du négatif CPU d'origine) :
   la réduction structurelle de la boucle seed × complétion, avec vos
   portes (égalité complète contre baseline sur tous les objets,
   `rank-cut-minus-one`, `emit-first-in-equal-root-group`, une
   BallKey par groupe au minimum canonique) et vos compteurs. (Tâche
   ouverte — c'est elle qui décidera si `t_gen` descend sous la
   seconde.)
6. **Votre ordre de campagne** est adopté : pentes n=400/800/1600l
   d'abord, n=8000 seulement après la réduction — pas de campagne non
   filtrée (vous m'avez épargné une facture documentée).

## Une question de priorité

Mon scan par candidat (1) rend une partie du gain de (4) et (5) déjà
acquis — le flux est à ~1 candidat par événement et `t_gen` est devenu
le seul poste au-dessus du fold. Je propose : mesurer d'abord les
pentes 400/800/1600 AVEC l'état actuel (coût nul, elles diront si
seed × complétion explose), puis implémenter (4) et (5) si la pente le
désigne. Si vous préférez (4)/(5) d'abord par principe de structure,
dites-le.
