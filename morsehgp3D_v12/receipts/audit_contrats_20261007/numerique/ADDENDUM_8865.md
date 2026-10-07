# Delta numérique — 8865e32c1

Relecture limitée du 7 octobre 2026 : `morsehgp3D_v12/docs/CONTRAT_NUMERIQUE.md` au commit **8865e32c1**, §3 et §4, réponses au §9. Aucun moteur exécuté ni source modifiée. Cette note complète REPORT.md, sans remplacer ses témoins épinglés à e264de6f2.

**Réponse 1 — budgets mixtes : validés sous les hypothèses écrites.** Poser `M=2^s`, conserver l'origine `o` du support et ses coefficients exacts `D>0`, `D<24M^4`, `|N_j|<24M^5`.

- Site admis par NUM-GARDE : `|z_j−o_j|<3M`, donc `|z−o|²<27M²`. Le premier produit est `<648M^6` ; la somme des magnitudes des trois termes linéaires est `<2·3·24·3 M^6=432M^6`. **Chaque somme partielle**, et pas seulement le résultat, est donc `<1080M^6<2048M^6`. Budget **6s+11**, i64 jusqu'à s=8, i128 jusqu'à s=19.
- Trois sites réellement sur la coquille : chaque différence entre deux sites, ou entre un site de coquille et `o∈S`, est de magnitude par axe `<2R<4M`. Le produit vectoriel de deux différences a chaque composante `<2·(4M)²=32M²`. Chaque coordonnée `N+D(o−p)` a magnitude `<24M^5+24M^4·4M=120M^5`. Somme des magnitudes des trois produits `<3·32·120M^7=11520M^7<16384M^7`. Budget **7s+14**, i64 jusqu'à s=7, i128 jusqu'à s=16.

Les q1/q2/q4 sont couverts par ces majorants conservateurs pour M≥1. Ces preuves utilisent les coefficients du support d'origine, pas des coefficients refabriqués depuis un triplet quelconque de la grande coquille. La borne d'orientation **ne s'étend pas** à trois sites quelconques simplement admis par le pavé : ceux-ci ne sont pas nécessairement sur la coquille et leur écart mutuel peut approcher 5M. Les comparaisons/différences/conversions auxiliaires doivent employer les types signés élargis avant opération.

**Réponse 2 — certificats : blocage CST-0201 toujours ouvert.** Les lignes **134–136** réaffirment à tort que les certificats v11 ne lisent ni B ni s. Les fichiers et le témoin `certificate_guard` de REPORT.md restent applicables sans modification : support s20, site admis par la garde, certificat support seul accepté, premier produit à 128 bits. Les nouveaux budgets mixtes ne rendent pas ce certificat ancien valide. Il faut un certificat spécifique au domaine gardé ou le certificat v11 paramétré avec un domaine englobant (s+2 pour la puissance). Pour l'orientation de coquille, la preuve du certificat v11 emploie une normale de trois points dans un **même cube** de largeur `<2^t` ; elle doit être reparamétrée ou refaite pour le domaine élargi, pas simplement récupérer un booléen de support. Toute proposition de nouveau certificat doit borner séparément les intermédiaires.

**Réponse 3 — comparaison des centres en deux temps : validée pour les naissances critiques, avec deux précisions obligatoires.**

Pour chaque axe j, effectuer la division euclidienne exacte `N_j=q_j D+r_j`, `0≤r_j<D`, donc `floor(c_j)=o_j+q_j` et partie fractionnaire `r_j/D`. La division C++ signée tronque vers zéro : corriger quotient et reste quand le reste est négatif, comme le faisait v11.

Les centres de naissance comparés sont critiques, donc dans l'enveloppe convexe de sites u32. Leur partie entière appartient à `[0,2^32−1]`, et `q_j=floor(c_j)−o_j` tient dans i64. **D>0 seul ne garantit pas cette borne** pour une candidate non positive : conserver le type certifié/contrat des naissances ; ne pas étendre implicitement ce comparateur à toutes les sphères `through3/4` de la v11.

Pour deux boules, poser explicitement `s=max(s_a,s_b)`. Comme `0≤r_a<D_a<24·2^(4s_a)` et `0≤r_b<D_b<24·2^(4s_b)`, chacun des produits `r_a D_b`, `r_b D_a` est `<576·2^(4s_a+4s_b)≤576·2^(8s)<2^(8s+10)`. Comparer ces deux produits **sans les soustraire**. La largeur est indépendante de B après extraction des parties entières ; elle atteint 266 bits à s=32, donc la comparaison fractionnaire conserve une voie large (320 bits suffit).

L'ordre doit être **lexicographique axe par axe** : comparer entier x, puis fraction x ; uniquement si x est exactement égal, entier y puis fraction y, etc. Comparer d'abord le vecteur des trois parties entières, puis le vecteur des fractions changerait l'ordre (exemple centres `(1/4,10,0)` et `(3/4,0,0)`). Avec cette précision, l'ordre est exactement celui de `compare_centers` v11 et reste invariant par translation entière.

Enfin, le défaut **CST-0202 d'identité Cloud/Morton** et le budget du **réservoir CST-0208** de REPORT.md ne sont pas corrigés par cette révision : ils restent requis avant port. Aucun autre sujet n'est rouvert par cet addendum.
