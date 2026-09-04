# Census : argmin entier par axe

Cadre : `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

Le delta `AxisBounds` est correct par lecture sous les préconditions ci-dessous. **Aucune compilation ni exécution indépendante de ce delta n'a été réalisée.** Le [reçu](receipts_20260904/census_axis_review_current.json) est `reviewed_not_executed` ; il conserve les hashes avant/après, stables, et le [diff](receipts_20260904/census_axis_review_current.diff) contre la copie mono figée. Aucun résultat antérieur n'est transféré au produit modifié.

## Preuve du calcul

Dans [census.hpp](../src/pipeline/census.hpp), lignes 61–101, poser $f(t)=At^2+Bt$, avec $A>0$. La division plancher de [intmath.hpp](../src/core/intmath.hpp), lignes 76–81, corrige la division tronquée lorsque le numérateur est négatif et le reste non nul. Ainsi $q=\lfloor-B/(2A)\rfloor$ et $r=-B-2Aq$ vérifient $0\leq r<2A$.

L'identité $f(q+1)-f(q)=A-r$ donne un minimum entier en $m=q+\mathbf{1}_{r>A}$. Les différences successives $f(t+1)-f(t)=A(2t+1)+B$ croissent strictement : aucun autre entier extérieur à ces deux voisins ne peut donner moins. Lorsque $r=A$, les deux voisins sont minimaux et le code choisit q.

Pour tout intervalle entier $[l,h]$, ramener m dans cet intervalle donne un minimum de f sur celui-ci. Si le minimum global est extérieur, la monotonie impose l'extrémité la plus proche ; sinon il est conservé. Lorsque $[l,h]\subseteq[0,65535]$, ramener d'abord m dans le domaine u16 puis dans la boîte donne le même résultat que le ramener directement dans la boîte. La conversion finale vers i64 porte donc exclusivement sur 0..65535. Le maximum de cette parabole convexe est atteint à une extrémité.

La somme des trois minima, respectivement maxima, plus C donne les extrema exacts du polynôme sur la boîte entière : les coordonnées sont indépendantes et les trois extremiseurs peuvent être choisis ensemble. Les points présents dans un nœud sont un sous-ensemble de cette boîte ; ces extrema restent donc des bornes sûres pour le census. Les distinctions strictes sont préservées : profondeur, `mn>=0` et `mx<0` ; census complet, élagage seulement si `mn>0` (lignes 145–158 et 188–199).

## Domaine arithmétique

Les préconditions consommées sont $0<A<2^{68}$, $|B_i|<2^{87}$, $|C|<2^{105}$ et $0\leq l_i\leq h_i\leq65535$, conformément au domaine conservateur de la qualification S1. Les mutants doivent être désactivés et la clé référencée rester inchangée pendant les requêtes.

On a $2A<2^{69}$ et $|2Aq|\leq|B|+2A<2^{88}$ ; division, reste et ajout de 1 tiennent en i128. Pour une coordonnée u16, $At^2<2^{100}$ et $|Bt|<2^{103}$, donc $|f(t)|<2^{104}$. Le module de chaque somme partielle utilisée dans `bounds` est inférieur à $2^{105}+3(2^{100}+2^{103})=59\cdot2^{100}<2^{106}$. Aucun calcul du polynôme au centre rationnel potentiellement éloigné n'est nécessaire. Les boîtes et coefficients arbitraires hors de ce domaine ne sont pas couverts ; `AxisBounds` conserve des préconditions internes, sans ajouter une API de validation d'entrée.

## Porte prête à exécuter

[axis_bounds_gate.cpp](../tests/axis_bounds_gate.cpp) énumère chaque axe en OBig512 sans réutiliser la division ni l'argmin du produit, et recoupe certaines petites boîtes par volume entier. La source prévoit égalités, centres éloignés, frontières u16, coefficients larges, 1 200 boîtes déterministes et 45 requêtes de profondeur. Les dix rejets concernent le validateur de fixtures, pas une nouvelle garde produit.

CMake enregistre un nominal attendu en code 0 et cinq mutants attendus en code 4 **avec** préfixe `DIVERGENCE axis_bounds` : plancher seul, plafond systématique, absence de clip par boîte, coefficient réduit à i64 et minimum substitué au maximum. Cette combinaison code/préfixe impose une divergence observable plutôt qu'un échec quelconque. Il s'agit du protocole lu, pas de résultats exécutés ici.

La qualification suivante doit compiler et exécuter ces six portes sur les sources épinglées, puis requalifier le binaire produit consommant ce census. Le microbenchmark facultatif est synthétique, exclu de `ALL`, et ne qualifie ni l'exactitude ni le coût de bout en bout. Aucun gain de performance n'est déduit de la preuve.

GCP non utilisé.
