# Contre-audit B — cover commun par arêtes survivantes

23 septembre 2026. Lecture indépendante de
[`CONTRAT_COUTS_ET_PARALLELISATION.md`](CONTRAT_COUTS_ET_PARALLELISATION.md),
§ « Covers communs », de l'[oracle entier](check_cover_batch_u18_20260922.py)
au commit `5cdeec30` et du [cover v8](../../morsehgp3D_v8/src/lanes/edge_cover.cpp).
**Certificat mathématique sûr, gain et raccord non qualifiés.** Aucun moteur
ni fichier A modifié.

## Verdict sur les bornes

Pour `E⊂A×B` non vide et une boîte spatiale `Z` non vide, le prédicat fermé
du cover est `F(a,b,z)=|2z−a−b|²−4|a−b|²≤0`. Les extrema **réels sur E**
des sommes `S_i=a_i+b_i` et du rayon `R=4|a−b|²` donnent les intervalles
`T_i=[2Z_i^-−S_i^+,2Z_i^+−S_i^-]`, puis
`L_E=Σ min_{t∈T_i}t²−R^+` et `U_E=Σ max_{t∈T_i}t²−R^-`.
Chaque `T_i` est inclus dans l'intervalle construit depuis les boîtes
`A×B` ; `R^-` est au moins son minorant et `R^+` au plus son majorant.
Donc **`L≤L_E≤F(e,z)≤U_E≤U`** pour tout `e∈E,z∈Z` : admission si
`U_E≤0`, rejet si `L_E>0`, raffinement sinon. L'égalité est admise.
Les « vrais extrema » sont ceux de `S_i` et `R` sur E, **pas** les extrema
de F sur `E×Z` ; les corrélations entre coordonnées, arête et sites de Z
restent relâchées. Le domaine u18 `[0,M]^3`, `M=262143`, donne
`|L|,|U|,|L_E|,|U_E|≤12M²=824627429388<2^40` : i64 signé suffit avec
promotion *avant* carré et soustraction.

« Resserre toujours » signifie **jamais moins serré**, non strictement
meilleur. Même un résidu propre peut garder les quatre extrema :
`A={0,1,2}`, `B={4,5,6}` sur l'axe x, `Z={(0,0,0)}` et
`E={(0,4),(2,6),(2,4),(0,6)}⊊A×B` ont les mêmes extrema de somme
`[4,8]` et de rayon `[16,144]` que les boîtes ; `L_E=L=−128` et
`U_E=U=48`. Même avec une seule arête, une boîte Z peut rester ambiguë
alors que tous ses sites sont dehors : `a=(4,5,0)`, `b=(6,5,0)`,
`Z={(0,10,0),(10,0,0)}` donnent `F=184` aux deux sites, mais
`L_E=−16,U_E=184` sur leur boîte. Aucun de ces cas n'est une erreur
de sûreté ; ils interdisent une promesse de gain systématique.

## Oracle, coût caché et portée

Le script actuel passe en Python normal et `-O` avec **43 386 triplets
ponctuels**, **1 000 familles survivantes**, `sharper_lower=916`,
`sharper_upper=943`, deux fixtures de décision et trois tangences.
[`CONTRAT_COUTS_ET_PARALLELISATION.md:97`](CONTRAT_COUTS_ET_PARALLELISATION.md)
cite encore l'ancien état **45 871 triplets / 1 001 familles** : mettre
ce reçu en cohérence côté A. L'oracle vérifie les inégalités des bornes
sur E et le partitionnement exact de l'ancien test groupé `A×B` ; dans
`verify_grouped(edges,zz)` ([lignes 86–131](check_cover_batch_u18_20260922.py)),
il n'emploie **pas** `surviving_edge_bounds` et ne teste donc ni sa
subdivision `E×Z`, ni les ranges fusionnées du raccord produit.

Réduire les extrema de E coûte `Ω(|E|)` pour cette méthode : E doit déjà
être matérialisé ou énuméré après filtre de paires. Si E reste un produit
WSPD implicite, cette étape peut réintroduire son développement. Pour
raffiner à la fois E et Z, conserver les agrégats associatifs de chaque
sous-bloc E **une fois**, puis les réutiliser pour tous ses Z ; les
recalculer à chaque tuile `(E,Z)` peut payer
`Σ_{tuiles}|E_t|`, jusqu'à un scan d'E par nœud Z visité. Les handles,
buffers bornés, copies et ranges spatiaux triés/disjoints **par arête**
restent obligatoires. Le certificat ne transfère aucun compte de
profondeur, ne décide ni graines q3 ni centres q4, et la boîte q4
`completion_box` exige en plus d'exclure les endpoints propres à chaque
arête. En mode q3 `GlobalBoxes`, le census utilise l'index global : le
cover q3-seul ne sert qu'à fournir l'arête, donc supprimer sa construction
est plus direct qu'en accélérer les tests.

Cette piste vise les **440 194 038 visites d'index** des covers v8 sans
sol 1 mm, poste réel mais distinct des milliards de tests de l'atlas q4.
Comparer sur les mêmes rectangles WSPD et masques q3/q4 le coût total
filtre→agrégats→tuiles→ranges→atlas/census→sorties, avec mémoire de pointe
et sorties exactes, avant toute affirmation sous-quadratique ou G4.

L'interface actuelle paie aussi une **seconde traversée** : après les
440,194 M visites de `Q34EdgeCover::build`, la décomposition de ce cover
par l'atlas q4 visite 315,737 M nœuds et copie 62,914 M IDs de nœuds sur
la même ligne mesurée. Un lot `E×Z` qui accélère seulement le premier
passage ne supprime pas ce travail ni les ranges par arête ; l'ablation
doit compter les deux passages et leur raccord. Sur cette ligne, les
171 444 arêtes q3-seules représentent au plus 8,4 % des 2 043 612
covers : les élider en mode q3 `GlobalBoxes` est une simplification sûre
à vérifier, mais ne peut fermer le verrou q4 partagé. L'atlas q4 rejette
153,036 M graines q3 sur les arêtes communes ; le remplacer exige un
certificat q3 de rechange, sans quoi le gain q4 peut devenir une dépense
q3.
