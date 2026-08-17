# Addendum — le filtre de profondeur à la génération : le minorant par boule existait déjà

Date : 17 août 2026. Réponse constructive à ma propre question
`QUESTION_CLAUDE_MINORANT_PROFONDEUR_20260817.md` : le minorant de
profondeur par boule, calculable à la génération SANS descente d'arbre,
existe — et il n'a demandé ni nouvel index, ni pré-clé axiale.

## Le théorème d'implémentation (une ligne)

Le `cover` de l'ancre (coefficient 3, déjà collecté par les lanes q3/q4
pour énumérer les complétions) est un SOUS-ENSEMBLE du nuage ; donc

```text
#{ z ∈ cover : P_B(z) < 0 }  MINORE  |I_B|
```

pour toute boule candidate `B` de l'ancre. Un minorant qui atteint
`h_q` tue le candidat EXACTEMENT comme la passe count-only l'aurait
fait — mais avant l'émission : ni tri, ni descente par boule. La
complétude du cover n'est PAS requise (un sous-compte ne tue jamais à
tort) ; la sortie anticipée borne le scan à `h_q` succès (le cas des
98 % de boules profondes) ; les copies multi-lanes sont couvertes par
la complétude par lane et le label d'arité minimale du RLE (une boule
tuée dans la lane q4 à `h_4` mais de support réel plus petit est émise
par SA lane et survit avec SON seuil).

## Mesures (uniform, seed 3)

| n=400 | avant | après |
|---|---|---|
| candidats q3 / q4 émis | 728 347 / 6 858 491 | 48 980 / 44 051 |
| tués à la génération (q3/q4) | — | 679 367 / 6 814 440 |
| boules uniques | 7 597 781 | 105 076 |
| t_gen | 10 314 ms | 6 174 ms (le scan coûte moins que l'émission évitée) |
| t_tri | 6 463 ms | 27 ms |
| t_prefiltre | 27 060 ms | 470 ms |
| t_census | 562 ms | 500 ms |
| **pipeline sujet total** | **~46 s** | **~9,0 s** |
| clés au census / événements / fusions / nœuds | 103 942 / 104 802 / 639 099 / 67 029 | IDENTIQUES |

L'identité exacte des survivantes et des sorties (mêmes 103 942 clés,
mêmes événements, mêmes fusions, mêmes nœuds) confirme empiriquement
l'exactitude ; les portes jugées (n=120, deux familles, smax=11 et
smax=6) rendent 0 désaccord. La passe count-only d'arbre RESTE en place
en aval : elle re-tue ce que la génération aurait manqué (covers
incomplets par construction) et reste l'autorité jugée.

Mutant `genfilter-nonstrict` (P <= 0 : coquilles ET supports comptés —
des boules à plateau meurent à tort) : TUÉ par le juge (code 4), porte
`mhgp4_forest_probe_mutant_genfilter`.

## Conséquence

Le poste « nombre de candidats » est fermé à n=400 (105 k boules pour
105 k événements — le flux est désormais à ~1 candidat par événement).
**89 portes CTest vertes** (la suite entière passe de 200 s à 155 s).
Les campagnes n = 8000/16000/32000 deviennent abordables : c'est la
prochaine étape, sur les deux profils contractuels.

## Post-scriptum — le compte W₄ exact par ancre (réponse auditeur, § 1)

Exécuté dans la foulée : `in_spindle(kQ4)` compté sur le cover (arrêt à
`h_4`), l'ANCRE entière meurt avant les boucles seed × complétion —
W₄ est le plus grand cœur anchor-only (owner + Jung), tout point de W₄
est intérieur à TOUTE boule q4 de l'ancre, et un sous-compte ne tue
jamais à tort. n=400 : 7 486 ancres tuées, candidats q4 évalués
6,81 M → 2,43 M, `t_gen` 6 174 → 2 903 ms — pipeline sujet total
~5,7 s (~8× depuis la baseline de la veille). Sorties identiques,
0 désaccord jugé, 89 portes vertes. Compteur `ancres_w4` publié.
Restent, des réponses auditeurs : la boule intérieure candidate
`B(m, R−δ)` (test O(1), fixtures entières, mutant midball-nonstrict)
et le préfixe axial streaming (≤ 16 groupes par seed) — tâches
ouvertes, pentes n=400/800/1600 d'abord.
