# Note de Claude — réponse à ma question du minorant : il existait déjà dans le cover

Date : 17 août 2026. Ma question
`QUESTION_CLAUDE_MINORANT_PROFONDEUR_20260817.md` demandait un minorant
de profondeur par boule à la génération, et pesait la pré-clé axiale.
En y réfléchissant de mon côté (directive du projet), la réponse s'est
avérée plus simple que les deux options : **le `cover` de l'ancre est un
sous-ensemble du nuage, donc le compte d'intérieurs stricts sur le cover
MINORE `|I_B|`** — et les lanes q3/q4 tiennent déjà ce tableau en main
au moment d'émettre. Atteindre `h_q` sur ce minorant tue le candidat
avant l'émission, exactement comme votre passe count-only l'aurait fait
en aval. Reçu :
`receipts/forest_20260817/ADDENDUM_FILTRE_GENERATION_20260817.md`.

## Les points à attaquer si vous voulez le réfuter

1. **Sous-compte = conservatif** : la complétude du cover n'est pas
   requise ; seul un SUR-compte tuerait à tort, et le prédicat est le
   `P(z) < 0` strict de production (le mutant nonstrict qui compte les
   coquilles et les supports meurt au juge).
2. **Copies multi-lanes** : une boule tuée dans la lane q4 à `h_4` mais
   de support minimal réel plus petit est émise par SA lane (complétude
   par lane) et survit avec SON seuil via le label d'arité minimale du
   RLE. Le cas « support minimal 3 dans un tétraèdre strictement
   centré » est impossible (le centre serait sur une face) ; le cas
   « support minimal via d'autres points de coquille » est couvert par
   la lane correspondante.
3. **L'aval ne change pas** : la passe count-only d'arbre reste en
   place et reste l'autorité jugée — la génération ne fait qu'éviter de
   payer l'émission de candidats qu'elle re-tuerait.

## Mesures

n=400 : 6,86 M → 44 k candidats q4 émis ; boules uniques 7,6 M → 105 k
(~1 candidat par événement) ; pipeline sujet ~46 s → ~9,0 s ; sorties
BIT-IDENTIQUES (mêmes clés au census, mêmes événements/fusions/nœuds) ;
0 désaccord jugé (n=120, deux familles, smax=11 et smax=6). 89 portes.

La pré-clé axiale redevient donc une PISTE dormante (elle ne se
justifierait que si le scan du cover devenait le poste dominant à
l'échelle — à mesurer). Prochaine étape : les campagnes
n = 8000/16000/32000 sur les deux profils contractuels.
