# Arithmétique des secteurs et de la corde

5 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

**Les opérations entières actives des secteurs et de la corde tiennent dans leurs types. Le changement d'échelle de la corde conserve le signe strict et sa racine corrigée fournit un sur-intervalle exact.** Un verrou supplémentaire est levé : pour les deux dénominateurs réellement appelés, 8 et 12, `bisector_basis` réussit dès la première itération, avec A=B=1. Sa garde à 128 itérations n'est pas nécessaire à la terminaison dans ce domaine. Le constructeur peut exploiter ce fait dans une future simplification qualifiée ; aucun changement produit n'est effectué ici.

Les chemins réels sont [src/lanes/sector_kill.hpp](../src/lanes/sector_kill.hpp), [src/lanes/chord_kill.hpp](../src/lanes/chord_kill.hpp), leurs appels dans [generate.hpp](../src/pipeline/generate.hpp) et la borne du [filtre affine](../src/pipeline/float_filter.hpp). La [preuve géométrique corde/secteurs](PREUVE_CHORD_SECTOR_COURANTE.md) justifie la suffisance des témoins ; la présente note ferme ses bornes d'opérations et précise la base entière. Le [front et ses témoins](FRONT_ET_TEMOINS_COURANT.md), les [marges flottantes explicites](FILTRES_FLOTTANTS_COURANTS.md) et [S1](S1_COURANT.md) portent leurs autres contrats.

## 1. Domaine réellement appelé et seuils

Posons M=65535 et d=b−a. Les positions appartiennent au cube u16, les extrémités d'une ancre sont distinctes et `D2=|d|²` est réellement calculé depuis elles. Donc `1 <= D2 <= 3*M²`. Les secteurs reçoivent `rho2_den=12` en q3 et 8 en q4. `CellGrid::build`, autre consommateur de `bisector_basis`, reçoit les mêmes valeurs. La corde reçoit une vraie graine q3 aiguë possédée par son arête maximale, avec G>0.

La porte du pipeline borne l'entrée par `kMaxTreePositions=2^30-1` avant construction de l'index. Les covers visités ici contiennent des indices de positions uniques, sans doublon de handles ; cette propriété appartient au contrat déjà justifié du front. Chaque `cnt` u32 reçoit donc au plus `2^30-1` incréments par ancre ou graine. Les compteurs de secteur et de corde ne peuvent pas boucler. Les seuils actifs valent `h_q=smax-q+1`, avec `smax<=11` : au plus 9 en q3, 8 en q4. Les lanes inexistantes sont sorties du front, sans interpréter le seuil nul comme un témoin.

Le crédit d'extrémité n'est utilisé que si `base<h_q`. Les témoins hors A∪B sont disjoints des témoins crédités dans A∪B. L'addition u64 `cnt_out+base` est inférieure à `2^30+9`; `++n_out+base` a la même sûreté. Le maximum est pris par secteur avant le minimum : chaque terme est un minorant de la profondeur au centre réel. Les comptes de secteurs ne sont pas additionnés entre eux, et les comptes du cœur et de la corde sont réunis par un OU.

## 2. La base choisie contient déjà le disque avec A=B=1

Les candidats sont les trois produits d×eᵢ. Leur norme carrée vaut `D2-d_i²`. Choisir les deux plus grandes normes revient donc à choisir les axes i,j des deux plus petites valeurs |dᵢ|. Notons k l'axe omis : |dₖ| est maximal et strictement positif. En posant u=d×eᵢ et v=d×eⱼ, les identités suivantes valent, quel que soit le départage des égalités :

$$\lVert u\times v\rVert^2=d_k^2D^2\geq\frac{D^4}{3},\qquad\max\bigl(\lVert u-v\rVert^2,\lVert u+v\rVert^2\bigr)=D^2+d_k^2+2\lvert d_id_j\rvert\leq2D^2.$$

La dernière inégalité utilise `2|d_i*d_j| <= d_i²+d_j² = D2-d_k²`. Les distances carrées de l'origine aux quatre arêtes du losange sont les quotients de la norme carrée du produit vectoriel par l'une de ces normes carrées d'arêtes. Il en résulte :

$$r_{\mathrm{inscrit}}^2\geq\frac{D^2}{6}>\frac{D^2}{8}\geq\frac{D^2}{12}.$$

La base est donc indépendante, et les deux tests entiers `cross2*rho2_den >= D2*dm` et `cross2*rho2_den >= D2*dp` passent dès A=B=1. Les calculs de cette première itération sont bornés ci-dessous **avant** d'invoquer cette sortie ; le raisonnement ne suppose pas qu'un débordement empêcherait ou provoquerait la sortie. Il est ainsi légitime de conclure que les opérations d'accroissement de A ou B sont inatteignables dans les appels de production examinés.

La constante 1/6 est atteinte pour d=(1,1,1). Un dénominateur 6 suffirait déjà, avec une égalité admissible de contenance. Ce résultat ne s'étend pas à un dénominateur arbitraire inférieur à 6 ni à une paire d'axes arbitraire.

Le commentaire historique `|e| >= D*sqrt(2/3)` est faux pour le **second** vecteur : d=(1,1,0) donne les normes carrées 2 et 1, alors que `2*D2/3=4/3`. La borne universelle de cette seconde norme est `D2/2`. Cette erreur documentaire ne remet pas en cause le test ; la preuve par le produit vectoriel donne directement le résultat plus utile ci-dessus. La petite fixture permanente conserve également le mauvais choix d'axes d=(1,1,0), qui rend les deux vecteurs `(0,0,-1)` et `(0,0,1)` dépendants : la règle effective de sélection compte.

## 3. Chaque opération entière des secteurs

La première itération fournit `|u_i|,|v_i|<=M`. Les huit sommets sont u, u+v, v, −u+v, −u, −u−v, −v, u−v ; leurs composantes ont module au plus 2M. Leurs triangles avec l'origine recouvrent l'octogone, qui contient le losange puis le disque des centres.

| Expression réellement écrite | Borne des intermédiaires |
| --- | --- |
| Différences d, candidats, opposés | Module au plus M ; négation i64 sûre |
| `nn[k]` | Deux carrés non nuls au plus M² ; somme au plus 2M², formée en i128 |
| `A*e1[i]`, `B*e2[i]` à l'itération atteignable | A=B=1 ; module au plus M en i64 |
| Une composante de `u×v` | Chaque produit i64 au plus M² ; différence au plus 2M², sous `2^33` |
| `cross2` | Chaque carré est promu avant multiplication ; somme au plus `12*M^4 < 2^68` |
| `dm`, `dp` | Les sommes ou différences i64 ont module au plus 2M ; trois carrés totalisent au plus 12M² |
| Comparaisons de contenance | `cross2*rho2_den <= 144*M^4 < 2^72`; `D2*dm` et `D2*dp <= 36*M^4 < 2^70`, en i128 |
| `sx=a.x+b.x`, `2*z.x-sx` et autres axes | Sommes/produits i64 au plus 2M ; différences de module au plus 2M |
| `n2w` et `rhs=n2w-D2` | Carrés promus ; somme au plus 12M² ; différence dans `[−3*M²,12*M²]` |
| `dot=w2·P[k]` | Trois produits promus de module au plus 4M² ; sommes partielles au plus 4, 8 puis 12M² |
| `4*dot` | Module au plus `48*M² < 2^38`, en i128 |
| Filtre radial `32*dist2q/(3*D2+1)` | `dist2q<=12*M²`; numérateur au plus `384*M² < 2^41`; dénominateur strictement positif, au plus `9*M²+1 < 2^36`; promotions i128 avant les produits |

La condition au sommet origine est strictement `n2w<D2`. Après ce filtre, rhs est même négatif. Les deux autres sommets demandent strictement `4*dot>rhs`. Par affinité, ces trois inégalités donnent la stricte intériorité sur tout le triangle, y compris son bord ; une égalité au centre réel ne peut être comptée comme témoin. Les `>=` des **tests de contenance du disque** ont un rôle différent et correct : le disque et les cellules doivent être couverts avec leur bord.

Le raccourci radial n'ajoute pas de témoin. Avec `radially_sorted=false`, une distance de boîte minorante peut seulement faire évaluer des sites supplémentaires, puis les tests exacts décident. Avec le cover trié, les classes précédant la coupure contiennent tous les témoins diamétraux pertinents, comme justifié dans la preuve géométrique. Aucun flottant ni racine n'intervient dans ce certificat sectoriel.

## 4. Coefficients de la corde et changement d'échelle

Écrivons E=|x−a|², X=|x−b|² et G=|d×(x−a)|². Les bornes simples sont `D2,E,X<=3*M²`, `G<=9*M^4`. La graine étant aiguë avec AB maximal, son centre c₃ vérifie `|c3-m|²<=D2/12`. Le vecteur réellement calculé `N=W-G*d` vérifie donc :

$$N=2G(c_3-m),\qquad\lvert N_i\rvert\leq\frac{G D}{\sqrt{3}}\leq G M\leq9M^5.$$

Il faut distinguer cette borne du **résultat N** des opérations qui le forment : les termes de W restent bornés individuellement par le grand-livre q3, soit `|W_i|<=36*M^5`, puis `|G*d_i|<=9*M^5`; leur soustraction est sûre sous `45*M^5` avant d'appliquer l'identité géométrique plus fine. Ce resserrement n'est valable que pour les vraies graines positives possédées de cet appel.

Pour `w2=2z-a-b` et `qz=|w2|²-D2`, `AnchorScratch::fill_affine_sites` calcule les trois produits et leur somme en i64, avec `|w2_i|<=2M`, `|qz|<=12*M²`. Les négations pour les maxima et leurs conversions en double sont exactes : ces entiers sont sous `2^36`, donc sous `2^53`.

| Expression de la corde | Borne opérationnelle |
| --- | --- |
| `N=W-G*d` | Termes et soustraction sous `45*M^5`; résultat affiné sous `9*M^5` par composante |
| `G*qz` | Module au plus `108*M^6` |
| Somme `w2_i*N_i` | Chaque terme au plus `18*M^6`; sommes partielles au plus 18, 36 puis 54 M⁶ |
| `L=G*qz-2*sum` | Le double est au plus `108*M^6`; soustraction sous `216*M^6 < 2^104` |
| `nrm=d×(x-a)` | Chaque produit i64 au plus M² ; différence au plus 2M² |
| `Bz=nrm·(z-a)` | Chaque produit i64 au plus `2*M^3`; somme au plus `6*M^3 < 2^51`, donc le `p3_dot` i64 est sûr malgré une entrée issue d'un produit vectoriel |
| `3*G-2*E*X` | Multiplications déjà i128, bornées par `27*M^4` et `18*M^4`; différence de deux non-négatifs de module au plus `27*M^4` |
| `J=D2*(3*G-2*E*X)` | Module au plus `81*M^6 < 2^103`; la géométrie affine J à une valeur strictement positive |

En effet `R3²=D2*E*X/(4G)<=D2/3`, donc `E*X<=4G/3` et `J>=D2*G/3>0`. Le garde J<0 nomme un invariant violé ; il ne justifie pas de supprimer silencieusement une graine valide.

Le développement exact utilise `d·W=G*D2`, conséquence de la puissance nulle de b. Il donne :

$$L=G(\lVert2(z-a)-d\rVert^2-D^2)-2(2(z-a)-d)\cdot(W-Gd)=4\bigl(G\lVert z-a\rVert^2-W\cdot(z-a)\bigr)=4P.$$

La division `l_exact/4` des chemins de cœur est donc exacte, y compris pour L négatif. Pour le centre paramétré `c_mu=c3+mu*nrm/(2G)`, la puissance est P−μB. Avec les cinq abscisses `mu_j=(2*j-4)*mu_hat/4`, les valeurs comparées par la corde sont exactement `4*(P-mu_j*B)`. Ni le signe ni les zéros ne changent avec ce facteur positif.

## 5. Racine proposée en flottant puis corrigée en entier

`isqrt128_floor` annonce `0<=v<2^120`; les appels de corde utilisent `v=J/2<2^102`. Sous les opérations binaire64 conformes utilisées par le projet, convertir un tel v puis prendre sa racine produit un nombre fini non négatif inférieur ou égal à `2^60` sur le domaine générique. Le cast vers i128 est donc représentable. Cette conclusion vaut aussi pour les modes d'arrondi dirigés usuels : l'extrémité supérieure `2^120`, puis `2^60`, est exactement représentable.

Le premier `r*r` est au plus `2^120`; le test suivant `(r+1)*(r+1)` est sous `2^121`. Ils tiennent en i128 signé **avant** toute correction. La première boucle décrémente jusqu'à `r²<=v`, sans passer sous zéro ; la seconde incrémente jusqu'à `(r+1)²>v`. Elle ne peut dépasser la racine plancher et toutes ses opérations restent dans les mêmes bornes. Le retour satisfait donc exactement `r²<=v<(r+1)²`, indépendamment de l'arrondi initial. La correction ne serait pas une protection contre une libm produisant NaN ou une valeur arbitrairement hors domaine ; ce n'est pas le domaine d'exécution déclaré.

Posons `r=floor(sqrt(floor(J/2)))`. Comme les carrés entiers successifs encadrent l'entier `floor(J/2)`, on obtient dans les deux cas de parité de J :

$$r^2\leq\lfloor J/2\rfloor\leq J/2<(r+1)^2.$$

Ainsi `mu_hat=r+1` est **strictement** supérieur à `sqrt(J/2)`, même quand J/2 est un carré parfait. Ce n'est pas un plafond exact déguisé : c'est un sur-intervalle volontaire, qui peut réduire l'efficacité du certificat mais conserve tous les centres admissibles.

La borne J ci-dessus donne `mu_hat <= 8*M^3 < 2^51`. Pour `c=2*j-4` dans −4,−2,0,2,4, le produit partiel `c*mu_hat` est sous `32*M^3` et le produit suivant par B sous `192*M^6`. La soustraction exacte finale est donc bornée par `408*M^6 < 2^105`, sensiblement plus petit que la borne historique `2^110` et toujours représentable en i128. L'indice j va de 0 à 4, chaque cellule de `neg[5]` est écrite avant lecture ; les quatre compteurs reçoivent uniquement la conjonction des signes stricts de deux sommets voisins.

## 6. Le filtre flottant conserve ce même signe strict

La [preuve existante des marges](FILTRES_FLOTTANTS_COURANTS.md) établit une réserve d'erreur pour l'affine plus forte que `|lh-L|<=E` : les additions arrondies `lh−E` et `lh+E` restent extérieures à L. Le raccord arithmétique présent vérifie que tous ses entiers d'entrée proviennent effectivement des grandeurs bornées ci-dessus. Les maxima qmax/umax sont relevés sur le même cover que celui utilisé par le filtre.

Pour `T=c*mu_hat*B`, la valeur `t` de la corde contient les conversions et multiplications annoncées. Avec l'unité d'arrondi ε=2⁻⁵³, la borne conservative existante donne `|t-T|<5*epsilon*|t|`. La multiplication par `delta=2^-40` est un changement d'échelle exact ; chacun des arrondis finaux de `t±|t|*delta` coûte moins de `2*epsilon*|t|`. Il reste donc la marge strictement positive `delta-7*epsilon` lorsque T est non nul. Si c ou B est nul, T et ses deux bornes sont exactement nuls.

Par conséquent `lh+E<tmin` implique L<T strictement ; `lh-E>=tmax` implique L>=T. Le premier retour seul crédite un témoin, et une égalité exacte ne peut devenir un témoin strict. Une comparaison indécidable évalue une seule fois L exact et compare `L-c*mu_hat*B` dans le domaine i128 établi plus haut. Quand le filtre est désactivé, E=+∞ rend les deux branches flottantes fausses pour les autres valeurs finies et force ce repli.

La négativité aux deux extrémités conserve la négativité d'une fonction affine sur chaque intervalle fermé ; il s'agit du **maximum** des deux valeurs qui doit être négatif. Le mot « minimum » présent dans le commentaire de `chord_kill.hpp` est une imprécision documentaire, déjà identifiée, sans effet sur les deux tests réellement exécutés. Les quatre intervalles couvrent la corde admissible. Le futur sommet y, de puissance nulle à son propre centre, ne peut être compté dans l'intervalle contenant ce centre : il n'est pas nécessaire de soustraire artificiellement un témoin supplémentaire.

## 7. Vérifications légères et portée

La [fixture arithmétique](secteur_corde_arithmetic_20260905.py) conserve les deux contre-fixtures documentaires de choix de base, les constantes de largeur, la marge du produit flottant et les corrections de racine. Son exécution sous Python optimisé a rendu 0 :

```bash
python3 -O morsehgp3D_v7/audits/secteur_corde_arithmetic_20260905.py
```

Le [reçu](receipts_front_20260905/secteur_corde_arithmetic.json) rapporte 368 directions, la borne 1/6 effectivement atteinte, 53 valeurs de racine dont 13 requièrent une correction, et toutes les inégalités de largeur satisfaites. Le calcul utilise les entiers Python et des fractions exactes. Il contrôle un modèle arithmétique et les constantes de la preuve ; **il n'exécute aucun binaire C++ produit**. Aucune compilation ni charge lourde n'a été lancée pendant la fenêtre de chronométrage du constructeur. Le raccord compilé réalisé après sa clôture est décrit séparément ci-dessous.

## 8. Raccord effectif aux helpers C++

Après la clôture de la fenêtre constructeur à 06:51:49 UTC, le [pont C++ permanent](secteur_corde_compiled_probe.cpp) appelle directement `bisector_basis`, `isqrt128_floor`, `anchor_sector_kill`, `AnchorScratch::fill_affine_sites`, `AffineSeed::l_exact/l_hat` et `ChordPieces`. Il inclut les vrais headers, dont `generate.hpp`, sans recopier leurs implémentations. Le [pilote](secteur_corde_compiled_runner.py) compile puis juge leurs résultats avec des entiers arbitraires et des fractions Python :

```bash
python3 -O morsehgp3D_v7/audits/secteur_corde_compiled_runner.py
```

La commande a rendu **0**. Les deux builds C++20 utilisent `-Wall -Wextra -Wpedantic -Werror -frounding-math -ffp-contract=off -DMHGP7_TESTING -pthread`, respectivement avec `-O2` et `-O1 -g -fsanitize=undefined -fno-sanitize-recover=all`. Les compilations et les exécutions ont rendu 0, sans diagnostic. Ces options nomment le domaine des helpers effectivement exécutés ; elles ne sont pas réattribuées à un autre binaire du pipeline.

| Contrat exercé dans chaque build | Résultat |
| --- | --- |
| Base entière réelle, 368 directions × dénominateurs 8 et 12 | 736 bases ; vecteurs non dilatés A=B=1, indépendants et de normes maximales |
| Racine réelle, 53 entrées × quatre modes d'arrondi | 212 racines identiques à `math.isqrt`, jusqu'à `v=2^120-1` |
| Affine réel et raccord de corde depuis de vraies graines | 96 cas jugés par centres rationnels issus d'élimination de Gram |
| Corde, frontières littérales et grandes valeurs entières | 156 cas, dont valeurs exactement nulles aux extrémités |
| Secteurs, site intérieur / shell / extérieur | 3 cas contre distances exactes aux boules des sommets |
| Mises à jour de corde avec appel exact / entièrement certifiées | 197 / 55 ; au plus un appel exact par mise à jour |
| Extrémités strictement négatives / nulles / positives | 448 / 300 / 512 |
| Décisions flottantes négatives / non négatives / indécidables | 197 / 214 / 849 ; toutes conformes au signe exact |
| Mises à jour sous modes dirigés avec repli affine vérifié | 72 ; filtre coupé, cinq signes indécidables, un appel exact |

Les modes exécutés sont `FE_TONEAREST`, `FE_DOWNWARD`, `FE_UPWARD` et `FE_TOWARDZERO`. Les retours de `fesetround` sont contrôlés. Les frontières de racine comprennent des carrés et leurs deux voisins, le haut du domaine générique et la borne de J/2 utilisée par la preuve. Les sorties sont identiques entre O2 et UBSan, hash d'objets comparé par le pilote.

Le juge des bases calcule la distance de l'origine à chaque **droite d'arête** en minimisant la quadratique `|u+t*(±v-u)|²` sur le rationnel t. Il ne reprend donc pas le test de produit vectoriel du helper comme unique autorité. La positivité du déterminant de Gram donne indépendance et aire non nulle. Les vecteurs retournés sont des candidats d×axe non dilatés ; comme les facteurs de la boucle ne peuvent que croître, ce constat vérifie aussi le raccord A=B=1 au code réel. Les égalités d'axes, d=(1,1,0), d=(1,1,1), les axes purs et les extrêmes u16 sont présents.

Pour l'affine, la référence calcule un centre et un rayon rationnels par élimination de Gram, puis juge `L=4G*(distance²-rayon²)`. Elle retrouve J depuis la différence entre la borne de Jung et ce rayon, et B par déterminant à élimination. Elle ne prend pas l'égalité entre deux formules produit comme seule preuve. Les cas géométriques transmettent le vrai `AffineSeed::bound` à `ChordPieces`. Les cas littéraux de corde déclarent séparément leur intervalle d'entrée valide, ou le forcent à l'infini pour exercer le repli ; leurs paramètres arithmétiques larges ne sont pas présentés comme une nouvelle fixture géométrique u16.

Deux défauts sont distingués explicitement :

- Le mutant **produit** `sector-kill-nonstrict` est activé par le registre réel et exécuté au site existant de `anchor_sector_kill`. Sur a=(0,0,0), b=(2,0,0), z=(1,1,0), les huit comptes nominaux sont nuls puisque z est sur la boule diamétrale. Le mutant crée des crédits sectoriels indus. La divergence est jugée sur ces comptes exacts, sans prétendre observer une suppression d'ancre entière sur cette fixture.
- La faute d'audit **`chord-nonstrict-parameter`** injecte explicitement `true` dans le paramètre `nonstrict` du vrai `ChordPieces::init`. Elle n'est pas annoncée comme une traversée du site mutant de `generate.hpp`. Pour J=8, B=1, L=−12, la valeur nominale des comptes est `[0,1,1,1]`; la faute produit `[1,1,1,1]` et transforme `dead(1)` en vrai. Cette égalité au premier sommet donne une dent minimale du contrat strict du helper.

Le pilote rend 0 quand il constate ces divergences attendues ; aucun code processus 4 n'est inventé pour le pont. Le [reçu commun](receipts_front_compiled_20260905/secteur_corde/receipt.json) épingle les sources et vérifie leur stabilité pendant l'exécution. Les reçus [O2](receipts_front_compiled_20260905/secteur_corde/o2.json) et [UBSan](receipts_front_compiled_20260905/secteur_corde/ubsan.json) conservent les commandes, entrées, sorties brutes, codes et hashes des binaires. Les gardes du pilote restent actives sous `python -O`.

Le raccord compilé local demandé est ainsi satisfait, sans ajouter de structure globale au moteur : huit secteurs et quatre morceaux de corde restent des filtres locaux de témoins. Le localisateur de cellules et les compteurs globaux gardent leurs reçus propres ; aucune suite complète ni mesure de performance n'est déduite de ces sondes. Aucun code produit modifié. **GCP non utilisé.**
