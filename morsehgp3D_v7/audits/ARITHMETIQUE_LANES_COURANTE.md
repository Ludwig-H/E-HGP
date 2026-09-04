# Arithmétique des lanes q2/q3/q4

Cadre : `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

La contrelecture des §§ 2–4 de [ARITHMETIQUE_PRIMITIVES.md](../docs/ARITHMETIQUE_PRIMITIVES.md) confirme les bornes annoncées et les identités des formes sur positions u16. Aucun intermédiaire débordant ni signe incorrect n'a été trouvé dans ce périmètre. Le [reçu statique](receipts_iteration3/lanes_static_current.json) épingle les octets consommés et leur fraîcheur. Aucune lane, compilation ni porte produit n'a été exécutée pour cette note ; les résultats antérieurs ne sont pas réattribués à cette lecture.

## 1. Domaine et largeurs effectivement fermés

Poser $M=65535$. Les positions sont dans $[0,M]^3$, les deltas dans $[-M,M]^3$, les formes proviennent des mêmes points que les prédicats consommateurs et les mutants sont désactivés. Les types sont ceux de `core/types.hpp:20–28`. Ces préconditions n'autorisent pas des structures `P3`, `Q3Form` ou `Q4Form` arbitraires.

Les opérations i64 de `types.hpp:43–49` sont sûres : produits scalaires et normes de deltas ont leurs sommes partielles bornées par $3M^2<2^{34}$ ; une composante de croix est bornée par $2M^2<2^{33}$. La norme de cette croix n'est pas une application de la même borne.

| Calcul relu | Borne avant réduction | Source |
| --- | --- | --- |
| q2 : somme des positions, C, puissance générique | $2M$, $3M^2$, $12M^2<2^{36}$ | `q2.hpp:16–26`, `keys.hpp:94–95` |
| q3 : produits DE/F², coefficients c1/c2, W | $9M^4$, $18M^4$, $36M^5$ | `q3.hpp:38–50` |
| q3 : puissance relative, B, C | $135M^6<2^{104}$, $54M^5<2^{86}$, $135M^6<2^{104}$ | `q3.hpp:54–68` |
| q3 : puissance générique, numérateur, dénominateur | $324M^6<2^{105}$, $27M^6<2^{101}$, $36M^4<2^{70}$ | `keys.hpp:95`, `q3.hpp:75–82` |
| q4 : cofacteurs, déterminant, numérateurs de Cramer | $8M^2$, $48M^3<2^{54}$, $72M^4<2^{71}$ | `q4.hpp:50–70` |
| q4 : puissance relative, B, C | $576M^5<2^{90}$, $240M^4<2^{72}$, $576M^5<2^{90}$ | `q4.hpp:75–77,135–144` |
| q4 : somme des carrés des numérateurs, carré du déterminant | $15552M^8<2^{142}$, $2304M^6<2^{108}$ | `q4.hpp:149–154` |

Les produits susceptibles de dépasser i64 sont promus avant multiplication. Les différences D−F et E−F se font encore en i64, mais leur module est au plus $6M^2<2^{35}$. Pour X, les intermédiaires D+E et 2F sont bornés par $6M^2$ et leur différence par $12M^2$ ; l'identité $X=|b-x|^2$ donne ensuite $X\leq3M^2$. Ces annulations ne cachent donc pas un débordement.

**Complément au grand-livre : puissance q4 développée.** Dans `BallKey::power`, le terme quadratique est borné par $144M^5$, chaque terme linéaire par $240M^5$, et C par $576M^5$. Les modules des sommes partielles de gauche à droite sont donc bornés successivement par $144M^5$, $384M^5$, $624M^5$, $864M^5$ et $1440M^5<2^{91}$. Cela ferme aussi la route générique utilisée après création de la clé q4, sans supposer le bien-centrage. Une réduction exacte par un entier positif n'augmente aucune de ces bornes ; sa réalisation est auditée séparément.

## 2. Identités, signes et conversions

La clé q2 développe exactement $|2z-a-b|^2-|b-a|^2=4(|z|^2-(a+b)\cdot z+a\cdot b)$. A vaut 1 et le niveau est $|b-a|^2/4$.

Pour q3, écrire $d=b-a$, $u=x-a$, $D=|d|^2$, $E=|u|^2$, $F=d\cdot u$, $G=DE-F^2$. Avec W construit par `q3_form`, on vérifie $d\cdot W=DG$, $u\cdot W=EG$ et $W\in\mathrm{span}(d,u)$. Si G>0, $v=W/(2G)$ est donc le circumcentre relatif dans le plan du triangle. L'identité $|W|^2=GDE(D+E-2F)$ prouve le niveau et la borne de centre, indépendamment d'un calcul flottant.

En particulier $|v|^2=DEX/(4G)\leq27M^6/4$, puisque G est un entier positif. Ainsi $|v_i|<3M^3<2^{50}$ pour **tout vrai triangle non colinéaire u16**, même obtus. Le plancher de chaque composante et son successeur tiennent en i64. La conversion avant clip dans `q3_detail::axis_min` (`q3.hpp:117–129`) est donc justifiée sur ce domaine. Elle ne l'est pas pour des coefficients artificiels satisfaisant seulement des bornes indépendantes sur G et W. Aucun appel à `q3_ball_depth` n'a été trouvé dans les sources `src/` et `cli/` relues, hors sa définition ; la preuve du helper reste distincte de la route census active.

Pour q4, les cofacteurs écrits forment bien l'adjugée de la matrice de lignes $2(b-a),2(x-a),2(y-a)$. Les lignes 65–67 multiplient ses **colonnes de cofacteurs** par le second membre, conformément à Cramer. La négation conjointe du déterminant et des trois numérateurs conserve $c-a=N'/\mathrm{det}$. Les modules sont loin du minimum signé i128. Avec det>0, $P_4(z)=\mathrm{det}(|z-c|^2-|c-a|^2)$ donne le signe de puissance attendu.

Dans `q4_center_strictly_inside`, le volume signé V se calcule en i64 avec des sommes partielles au plus $6M^3<2^{51}$. Pour chaque face, $rc=N'-\mathrm{det}(\mathrm{face}_0-a)$ a ses composantes bornées par $120M^4<2^{71}$. Les trois termes du déterminant de face sont chacun bornés par $240M^6$, et leur somme par $720M^6<2^{106}$. Les orientations des sommets opposés sont bien $(-V,+V,-V,+V)$ pour l'ordre de faces écrit ; le code compare le centre au bon côté et rejette chaque zéro.

Les gardes nécessaires sont présentes aux appels examinés : seed strict avant q3 dans `generate.hpp:795–797,909–914`, produits scalaires stricts puis G>0 dans `silent_incidence.hpp:175–180`, arête maximale et test strict dans `render.hpp:135–148`. Pour q4, det=0 est rejeté avant niveau/prédicat dans ces trois routes (`generate.hpp:1153–1166`, `silent_incidence.hpp:192–194`, `render.hpp:154–156`). Le rendu n'exige pas le bien-centrage ; les bornes de Cramer et de puissance n'en dépendent pas.

## 3. Critères concrets des petites portes futures

Les valeurs attendues G1–G6 du [plan](../docs/PLAN_PORTES_ARITHMETIQUES.md) sont cohérentes par calcul algébrique : q2 aux coins du cube, triangle équilatéral entier, rangs et centre obtus, tétraèdre régulier, petit déterminant et zéro de face non coplanaire. En G4, la clé commune q2/q4 et l'égalité des niveaux doivent être vérifiées malgré leurs représentations différentes. En G6, le centre $(2,5/6,0)$ a quatre distances carrées $169/36$ ; le rejet doit provenir du zéro de face, avec det=192 non nul.

Ajouter une fixture dédiée à la conversion q3 : $a=(0,0,0)$, $b=(46368,28657,0)$, $x=(28657,17711,0)$. L'identité de Cassini donne le déterminant plan −1, donc G=1. Les équations de centre donnent exactement $2(c-a)=(-20100270015213,32522920160401,0)$. Ce centre dépasse $2^{40}$ en module et reste dans le domaine démontré. Le helper doit rendre les extrema corrects après clip sur de petites boîtes u16 décalées par a ; le seed q3 strict doit être refusé. Cette fixture éprouve une vraie forme géométrique, avec un critère de non-vacuité de conversion, sans appeler le helper sur G=0.

La porte formes doit résoudre q3 par les équations linéaires indépendantes proposées et q4 par déterminants à six permutations, comparer les coefficients par produits croisés, puis contrôler puissances et niveaux. Publier séparément les compteurs de rang nul/non nul, det brut positif/négatif, centre intérieur/extérieur/sur face et puissances négatives/nulles/positives ; parcourir les permutations du support et les trois axes. Un juge qui déborde ne rend aucun verdict. Un défaut de cofacteur, une canonisation d'un seul numérateur, une erreur de parité ou de facteur du niveau doit produire une divergence identifiée, avec code exact fixé par le futur protocole.

Ces portes restent à exécuter. La preuve ci-dessus ferme les clauses arithmétiques locales et les raccords d'appel examinés ; elle ne qualifie ni les limbs/PGCD examinés séparément, ni le front complet, ni l'ABI compilée, ni l'exactitude horizontale/verticale ou les coûts du produit.

GCP non utilisé.
