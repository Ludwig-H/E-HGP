# Addendum — sélection axiale : reçue exacte, non rentable CPU à ces tailles

Date : 17 août 2026. Exécute le point 4 de l'audit « lemme préfixe et
niveau » : l'accélérateur axial reçu CONTRE la baseline énumérée et
l'oracle q4, sans changer ni les records ni les plateaux. Dossier :
`docs/MATHEMATIQUES.md` § 4.6 (re-dérivation v4 de la piste v3).

## La mathématique reçue

Faisceau de sphères par le cercle du seed : `Phi(z; mu) = P_3(z) −
mu·pi(z)` (`P_3` = forme de Gram q3, tête `G > 0` ; `pi = n·(z−a)`,
`n` = normale de la face). Test intérieur exact d'un site dans la boule
d'une complétion `y` : `sign(P_3(z)·pi(y) − P_3(y)·pi(z))·sign(pi(y)) <
0` — produits `< 2^161`, comparaison U192 signée (`q4_axial.hpp`).
**Borne de rang** : `depth(y) >= p + prédécesseurs_stricts(y)` dans son
côté du plan — les candidats vivent dans les premiers groupes du côté
positif et les derniers du côté négatif, `p + preds <= h_4 − 1`, soit au
plus `2(h_4 − p)` groupes par seed. Minorant fail-open : le census, owner,
arité et exact-once restent inchangés sur chaque candidat.

## Validation (portes)

| porte | résultat |
|---|---|
| `--axial --judge` : fixture23, uniform n=120, clusters n=120 | records IDENTIQUES à la baseline, juge brut 0/0 |
| fixture de justesse `fixture_tight20` (événement de profondeur EXACTEMENT `h_4 − 1 = 7`, sept prédécesseurs du même côté que la complétion) | gardé par la coupe saine, baseline et axial d'accord (152 événements, 0/0) |
| mutant `axial-rank-cut` (un groupe de moins) | perd l'événement limite, code 4 — la borne est serrée AU GROUPE PRÈS |

## Mesures de coût (uniform, s=8, seed 3, CPU conteneur) — HONNÊTES

| n=400 | baseline | axial |
|---|---|---|
| complétions essayées | 58 530 854 | 13 803 658 (dont 14,5 M candidats) |
| tests de puissance (census) | 22 526 088 | 2 861 054 (**7,9× de moins**) |
| événements | 43 989 | 43 989 (identiques) |
| `t_instruction` | **6,8 s** | **31,7 s** |

**La sélection axiale est une perte nette CPU à ces tailles.** Cause
identifiée : la classification par seed coûte `|cover|` évaluations de
`P_3` + un TRI par comparateur exact U192 signé (~`c·log c` par seed, soit
~3·10^8 comparaisons exactes à n=400) — plus cher que tout ce que la
coupe économise, alors même qu'elle atteint son maximum théorique
(~15,8 candidats/seed à n=120, borne 16). Le « 59× moins de propositions »
de v3 était un compte de propositions, pas un temps ; la proposition seule
ne paie pas le tri exact qui la sélectionne.

## Ce qui en reste (et la question aux auditeurs)

1. La borne est VRAIE, jugée, et serrée au groupe près — elle reste la
   voie du **budget statique par seed** (≤ 16 candidats) dont un port GPU
   a besoin (allocation fixe par warp, pas de listes dynamiques).
2. Pour la rendre rentable CPU, il faudrait une PRÉ-CLÉ approchée
   (flottante ou entière tronquée) avec arbitre exact aux frontières —
   une analyse d'erreur certifiée est nécessaire pour rester fail-open
   (le profil FP certifié du dépôt l'exige).
3. Alternative : garder `--axial` comme chemin reçu mais optionnel,
   baseline énumérée par défaut, et passer à la forêt.

Sauf avis contraire des auditeurs, je retiens (3) : le poste dominant
mesuré n'est PAS la sélection des candidats mais leur simple énumération
bon marché — le prochain gain de complexité est ailleurs (forêt d'abord,
optimisations guidées par compteurs ensuite). 57 portes CTest vertes.
