# Note de Claude — le raccord est fait : WSPD → SpherePlateau → forêt

Date : 17 août 2026. Le point final de vos deux ordres de forêt est
exécuté : les flux WSPD réels alimentent le fold, avec l'ABI
`SpherePlateau` d'échelle. Reçu :
`receipts/forest_20260817/ADDENDUM_FLUX_REELS_20260817.md`.

## Ce que vous voudrez vérifier

1. **Le lemme de complétude sous les seuils** (en tête de
   `src/pipeline/ball_stream.hpp`, `derive_v4`) : un plateau pertinent de
   support minimal d'arité q a au plus `h_q − 1` témoins de fuseau — les
   seuils du profil sont exactement calibrés pour ne perdre aucun plateau.
   C'est le chaînon entre vos filtres h et votre quotient ; un
   contre-exemple le tuerait net, je n'en ai pas trouvé et le juge non
   plus (0 désaccord sur deux familles jugées).
2. **Le census uniforme par clé primitive** : `P(z) = A|z|² + B·z + C`
   (< 2^106, i128) — les trois lanes partagent désormais UN prédicat de
   census, une descente, une passe par boule unique. Les 505 564 points
   de coquille recensés à n=400 confirment votre diagnostic : le quotient
   n'était pas une annexe.
3. **Les compteurs qui désignent la suite** : 98 % des boules uniques
   meurent en profondeur (`|I_B| > 9`) APRÈS avoir payé leur census — un
   pré-filtre de profondeur à la génération (les témoins de fuseau de
   l'ancre minorent `|I_B|`… mais par le mauvais côté ; il faudra un
   minorant de profondeur par boule, pas par ancre) est le prochain gain
   évident ; et le générateur q4 domine le flux (6,86 M candidats sur
   7,6 M à n=400) — la sélection axiale ou sa pré-clé certifiée
   redeviendront pertinentes ICI, comme filtre de candidats sans census.
4. Une leçon d'échelle payée : la première version instantanéisait les
   partitions par lot (`O(lots × facettes)`) et s'est fait tuer par l'OOM
   à n=120 — le probe compare désormais la partition FINALE plus les
   multiensembles de nœuds par lot, et les instantanés restent aux
   petits n du selftest.

## Où en est l'objet, globalement

Les trois lanes produisent ; les boules sont dédupliquées inter-lanes ;
les plateaux hors position générale sont quotientés exactement ; la
forêt à macro-lots sort nœuds, niveaux, rôles et partitions — jugée à
trois étages (Déf. 28 pure sur sous-ensembles ; brut par prédicats de
production ; flux WSPD). 71 portes vertes.

Prochaines étapes proposées : (a) rendu § 9.1 (`F_K^render`, poids
`S_τ/T_x/m_τ`, votre question Q4) — il ferme l'objet côté sortie ;
(b) pré-filtre de profondeur guidé par les compteurs ci-dessus ;
(c) l'échelle n = 8000/16000/32000. Je pars sur (a).
