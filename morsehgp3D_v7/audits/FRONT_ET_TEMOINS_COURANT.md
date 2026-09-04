# S1 : front WSPD, fuseaux et comptages universels

4 septembre 2026. Cadre exploratoire v7 : CPU de référence, entrée u16, audit mathématique et architectural indépendant, statut public `not_claimed`.

Cette contre-lecture justifie les raccords combinatoires du front et la sûreté géométrique des crédits de fuseau. Elle complète la [matrice S1](S1_COURANT.md) ; les certificats aval, l'arithmétique de tous les prédicats et la composition finale gardent leurs obligations. Aucun nouveau test lourd n'est lancé pour cette lecture.

## Partition et terminaison du front

Pour deux feuilles de positions distinctes, leur plus petit ancêtre commun possède un fils contenant la première et l'autre fils contenant la seconde. La paire appartient à exactement une graine `(left(v), right(v))`. Chaque scission de A en enfants disjoints remplace son rectangle par deux rectangles disjoints dont la réunion est exactement A×B ; même argument pour B. Cela justifie la partition indépendamment de la masse des rectangles.

Dans [alive_rectangles_fused](../src/pipeline/generate.hpp#L326), une lane sort une seule fois du masque d'un rectangle mort. Les enfants reçoivent uniquement les lanes restantes. Pour chacune, les rectangles émis et tués partitionnent donc les graines initiales. La fusion ordonnée des tranches conserve ces décisions disjointes. Le ledger de masse vérifié dans [run.hpp](../src/pipeline/run.hpp#L498) contrôle cet invariant ; son égalité seule ne prouverait pas la partition, car un doublon et une perte de même masse pourraient s'annuler.

Chaque scission remplace un facteur interne par un enfant strict. La somme des hauteurs restantes des deux facteurs diminue strictement le long d'une branche. Deux feuilles distinctes ont un diamètre de boîte nul et sont séparées. Le front termine donc sur tout arbre radix fini valide. Une limite de capacité interrompt par `cap_refus`, repris en refus par le pipeline ; elle ne transforme pas un front incomplet en catalogue complet.

La séparation entière de [wavefront.hpp](../src/wspd/wavefront.hpp#L68) est suffisante : avec D la distance des centres des boîtes et M le plus grand rayon de leurs boules circonscrites, le test donne $D\geq(s+2)M$, donc $D-r_A-r_B\geq sM$. Aucune borne industrielle de temps ou de nombre de rectangles n'est déduite ici du seul choix du plus grand diamètre.

## Témoins stricts des fuseaux

Fixons un support positif de cardinal q, une arête maximale AB de longueur D, et sa miniball de centre c et rayon R. Ses poids barycentriques positifs, de somme un, donnent :

$$R^2=\frac{1}{2}\sum_{i,j}\lambda_i\lambda_j\lVert u_i-u_j\rVert^2\leq\frac{q-1}{2q}D^2.$$

Avec $m=(A+B)/2$, le vecteur $v=c-m$ est orthogonal à AB et $R^2=D^2/4+\lVert v\rVert^2$. Ainsi $\lVert v\rVert\leq D/(2\sqrt{3})$ pour q3 et $\lVert v\rVert\leq D/(2\sqrt{2})$ pour q4.

Pour un site z, posons $H=(z-A)\cdot(B-z)$ et $\Xi=\lVert(B-A)\times(z-A)\rVert^2$. Avec $t=3$ pour q3 et $t=2$ pour q4 :

$$\lVert z-c\rVert^2-R^2=-H-2v\cdot(z-m)\leq-H+\sqrt{\frac{\Xi}{t}}.$$

Les conditions `H>0` et `t H²>Xi` de [in_spindle](../src/spindle/spindle.hpp#L58) rendent donc la puissance strictement négative pour tout support de cette arité ancré sur AB. Pour q2, v est nul et `H>0` décrit exactement l'intérieur diamétral. Un point du shell du support ne peut être crédité comme intérieur.

Un support pertinent possède $p+q\leq s_{\max}$, donc $p<h_q=s_{\max}-q+1$. Au moins $h_q$ témoins distincts excluent ce support. Cette preuve requiert une arête maximale pour q3/q4, conformément au choix canonique du support ; elle ne vaut pas pour une arête arbitraire.

## Coins et boules de cœur

À z fixé, écrire $u=z-A$ et $w=B-z$. Pour une extrémité fixe, $u\cdot w>0$ et $\sqrt{t}(u\cdot w)>\lVert u\times w\rVert$ définissent un cône convexe ouvert dans l'autre extrémité. Vérifier les coins de A pour chaque coin de B étend d'abord la condition à toute A, puis à toute B. Cette convexité séparée justifie les 64 coins, ainsi que les huit coins avec une extrémité fixée. Elle ne demande pas une convexité conjointe en (A,B).

Une boule ouverte de centre m et rayon $\kappa_qD$ est contenue dans le fuseau, où $\kappa_2=1/2$, $\kappa_3=1/(2\sqrt{3})$ et $\kappa_4=\sin(15^\circ)$. À distance $\rho$ du milieu, $H=D^2/4-\rho^2$ et la composante perpendiculaire est au plus $\rho$. La racine positive de $\rho^2+D\rho/\sqrt{t}-D^2/4=0$ fournit ces constantes pour q3/q4.

Pour des boîtes de rayons $r_A,r_B$, dont les centres sont séparés de $D_0$, deux minorants de rayon commun autour du milieu de ces centres sont :

$$R_{\mathrm{dec}}=\kappa_q(D_0-r_A-r_B)-\frac{r_A+r_B}{2},\qquad R_{\mathrm{coup}}=\kappa_qD_0-\sqrt{\frac{4\kappa_q^2+1}{2}(r_A^2+r_B^2)}.$$

Le premier suit de l'inégalité triangulaire. Pour le second, les déplacements a et b des extrémités vérifient :

$$\kappa_q\lVert a-b\rVert+\frac{1}{2}\lVert a+b\rVert\leq\sqrt{(2\kappa_q^2+1/2)(\lVert a\rVert^2+\lVert b\rVert^2)}.$$

Cauchy–Schwarz et l'identité du parallélogramme bornent ainsi simultanément la perte de longueur et le déplacement du milieu. Le maximum des deux minorants reste valable puisque leur centre est commun. Dans [core_ball](../src/spindle/spindle.hpp#L140), les distances sont arrondies par défaut, les rayons de boîtes par excès, les coefficients positifs par défaut et le terme soustrait par excès. Les constantes sont contrôlées par les inégalités entières `static_assert`. Une valeur non positive ne crédite rien. Les comparaisons avec les boîtes et sites sont strictes.

## Crédits disjoints et sens de Hmax

[count_universal_witnesses](../src/spindle/witness_count.hpp#L54) retire A et B de chaque crédit de sous-arbre. Ces intervalles sont disjoints par construction du front. Quand une lane crédite un sous-arbre, son bit est retiré avant de visiter les enfants ; les mêmes PointId ne sont pas recomptés dans cette lane, y compris aux feuilles.

`hmin_boxes` est un minimum exact : l'expression est séparable par axe, affine séparément aux extrémités et concave en z, donc les minima sont atteints aux coins. `hmax4_boxes` a un autre sens. Pour chaque axe, il choisit les extrémités minimisant le maximum en z. La somme construit un couple d'extrémités pour lequel H est partout au plus cette borne. Si elle est non positive, aucun z ne peut être témoin universel de toutes les paires. Cela ne signifie pas l'absence de témoins pour chacune des autres paires. Ce sens suffit au prune utilisé ; avec des extrémités ponctuelles, la distinction disparaît.

Les histogrammes `ha` et `hb` de [generate.hpp](../src/pipeline/generate.hpp#L488) portent respectivement sur A et B, hors l'extrémité elle-même. Ils sont disjoints du cœur hors A∪B. Leur somme avec le crédit de cœur reste un minorant des témoins stricts pour l'ancre. Ils incrémentent par position unique : en présence de multiplicités, cela minore encore le nombre de PointId. Les tests de ligne et de seuil ne peuvent donc retirer un support pertinent.

Ces raccords supposent la validité de l'index et des opérations entières. Ils ne prouvent pas seuls les secteurs, cellules, cordes, décisions flottantes certifiées ou la conservation finale du représentant minimal après RLE. L'identité des sources relues est conservée dans [le reçu statique](receipts_20260904/front_sources.json). GCP non utilisé.
