# WARNING — le critère local d'auto-certification n'est pas complet

> [!CAUTION]
> Audit indépendant du 8 août 2026. Ce fichier ne modifie pas `DESIGN.md` et ne condamne pas l'idée de dualité inversive. Il signale une obstruction précise qui invalide, dans leur forme actuelle, le Théorème 4 du §3.1, son corollaire de croissance du §3.1 et les estimations GPU qui supposent cette terminaison complète.

## 1. Contre-exemple exact au Théorème 4

Prenons $s_{\max}=11$, $p=(0,0,0)$, les dix points $q_i=(-(100+i)/100,0,0)$ pour $i=0,\ldots,9$, puis le point lointain $z=(100,0,0)$.

Au point $p$, on a $d_{11}(p)=109/100$, donc la procédure du corollaire commence avec $\rho_0=109/50$. Le voisinage $W_{\rho_0}$ contient exactement $p$ et les dix points $q_i$. Comme ces points sont colinéaires, les sphères critiques locales incidentes à $p$ et de rang fermé au plus 11 ont des supports de taille au plus deux. Leur rayon maximal est celui de la paire $\{p,q_9\}$ :

$$R_{\rho_0}=\frac{109}{200},\qquad 2R_{\rho_0}=\frac{109}{100}\leq\frac{109}{50}=\rho_0.$$

Le critère annoncé arrête donc la croissance et déclare le catalogue local complet.

Pourtant, dans le nuage global, la paire $\{p,z\}$ porte la boule diamétrale de centre $(50,0,0)$ et de rayon 50. Tous les $q_i$ sont strictement à l'extérieur de cette boule, car leur première coordonnée est négative. Cette boule est bien centrée, son support minimal est $\{p,z\}$ et son rang fermé vaut 2, donc au plus $s_{\max}$. Elle est absente de $W_{\rho_0}$ et du catalogue déclaré complet.

Le contre-exemple vise exactement l'étape logique manquante : un plan dual lointain ne modifie aucun événement déjà situé dans $\left\lbrace \lVert\nu\rVert\leq2R_{\rho}\right\rbrace$, mais il peut créer un nouvel événement de norme supérieure. La maximalité de $R_{\rho}$ dans le catalogue tronqué ne borne donc pas les rayons du catalogue global; l'argument de maximalité employé dans `DESIGN.md` est circulaire.

Les séparations sont strictes. Une version en position générale s'obtient en perturbant rationnellement les dix points dans une petite boule autour de $(-1,0,0)$ tout en gardant $z$ très loin sur l'axe positif : Jung borne alors tous les rayons du catalogue local par une constante proche de $\sqrt{3/8}$, tandis que les points de coordonnée négative restent strictement hors de la boule diamétrale de $\{p,z\}$. L'obstruction n'est donc pas propre à la colinéarité.

## 2. Conséquences immédiates

- Le test $2R_{\rho}\leq\rho$ certifie au mieux un préfixe radial du catalogue; il ne certifie pas le catalogue global incident à $p$.
- Le doublement proposé peut s'arrêter avant un support de rang 2. Les portes P0 et P3 de `DESIGN.md` ne suffisent donc pas à établir la complétude.
- Les voisinages $W_p$ ne sont pas prouvés petits. Sans un autre certificat global, le peeling local peut devoir voir les $n-1$ autres points pour chaque $p$, soit déjà $\Theta(n^2)$ données avant le coût des niveaux d'arrangement.
- Les budgets 50 k du §7 ne peuvent pas être déduits de cette procédure tant que la source complète n'est pas remplacée ou réparée.

Une réparation doit apporter une borne globale indépendante du catalogue tronqué, ou un complément fail-open qui couvre tous les points/paires omis. Répéter la même condition après un voisinage local plus grand ne résout pas l'implication logique.

## 3. La réciproque du Théorème 3 oublie le coefficient de $p$

Une seconde obstruction exacte affecte la caractérisation duale. Dans $\mathbb{R}^{3}$, prenons $p=(0,0,0)$, $u=(1,0,0)$ et $v=(0,1,0)$, sans autre point. Les inversions de centre $p$ sont $\iota_p(u)=(1,0,0)$ et $\iota_p(v)=(0,1,0)$. Les deux plans duaux imposent $\nu_1=\nu_2=1$; le point de leur intersection le plus proche de l'origine est $\nu=(1,1,0)$ et

$$\frac{\nu}{\lVert\nu\rVert^{2}}=(1/2,1/2,0)\in\mathrm{relint}\,\mathrm{conv}\left\lbrace \iota_p(u),\iota_p(v)\right\rbrace.$$

La profondeur est nulle et toutes les conditions annoncées par le Théorème 3 sont satisfaites pour $U=\{p,u,v\}$. Pourtant le centre original vaut $c=p+\nu/2=(1/2,1/2,0)$. Il appartient au segment $[u,v]$ et non à $\mathrm{relint}\,\mathrm{conv}\{p,u,v\}$. La miniboule a donc pour support minimal $\{u,v\}$, pas $U$.

Dans la preuve directe, les coefficients positifs des points de $U\setminus\{p\}$ donnent bien une combinaison convexe des points inversés. La réciproque ne garantit toutefois pas que le coefficient barycentrique restant de $p$ soit strictement positif. Il faut ajouter et démontrer cette condition; « projection dans l'intérieur relatif de l'enveloppe inversée » ne lui est pas équivalent. Le catalogue proposé peut sinon émettre des supports non minimaux et ses correspondances support--face deviennent fausses.

## 4. Deux autres portes de preuve à garder ouvertes

Le résultat de REANI--BOBROWSKI cité par `DESIGN.md` est formulé sous hypothèse de position générale, et son énoncé homologique suppose ensuite des valeurs critiques distinctes. Le papier publié est : [Morse Theory for the k-NN Distance Function, SoCG 2024](https://doi.org/10.4230/LIPIcs.SoCG.2024.75). Il ne justifie pas à lui seul l'affirmation du §6 selon laquelle aucune position générale n'est supposée et que toutes les dégénérescences sont résolues par lots. En particulier, la définition actuelle impose $X\cap\partial B=U$ avec $\lvert U\rvert\leq4$ et ne représente pas directement une coquille cosphérique contenant plus de quatre identifiants.

La descente du §4 échoue elle aussi dans ce cas. Pour $F=\{(-1,0,0),(1,0,0)\}$ et $X=F\cup\{(0,1,0)\}$, la miniboule fermée de $F$ rencontre bien $X\setminus F$, mais seulement sur son shell. Il n'existe aucun intrus dans la boule ouverte, contrairement à l'alternative utilisée par la preuve. Un lot de niveaux égaux ne fournit pas à lui seul la transition manquante. Il faut soit assumer explicitement la position générale pertinente, soit représenter la coquille complète, les supports minimaux multiples et leurs transitions de plateau.

Enfin, remplacer des coordonnées binary64 par une grille entière sur 21 bits est une quantification géométrique, pas un repli exact pour les données d'entrée originales. Sans hypothèse contractuelle de coordonnées déjà sur cette grille ou preuve qu'aucun prédicat ne change de signe, les revendications d'exactitude et les largeurs de mots du §6 restent à démontrer séparément.

La table du §6 sous-estime en outre le degré du prédicat in-sphere 3D. Après translation d'un point, le déterminant $4\times4$ contient trois colonnes linéaires et une colonne de normes carrées : chaque monôme est de degré total 5, pas 4. La borne $4b+6$ et les choix de limbs qui en dépendent doivent donc être recalculés avant toute revendication `__int128`.

Dans l'état observé le 8 août 2026, `include/mhgp/exact.hpp` ajoute deux risques arithmétiques indépendants. `big_cmp(a,b)` prend le signe de la soustraction en complément à deux; si la différence mathématique déborde la largeur signée, ce signe ne donne pas l'ordre. Par exemple, « maximum positif moins $-1$ » devient négatif modulo la largeur. De plus, former une magnitude par `-a` ou `-b` n'est pas défini pour la valeur signée minimale de `i128`. Une comparaison exacte doit d'abord comparer les signes puis les mots non signés, et la magnitude doit être construite en arithmétique `u128` sans négation signée de la valeur minimale.

Enfin, les assertions selon lesquelles les flèches verticales seraient immédiates, leur commutation vraie par construction et la couverture déjà exacte sont plus fortes que les résultats cités. La descente peut au mieux identifier une composante si toutes ses transitions de niveau sont déjà prouvées; elle ne rend pas canonique le minimum atteint. La caractérisation des minima de $\max(\lVert y-x\rVert,d_K(y))$, y compris le cas contraint annoncé puis omis par la passe proposée, demande une preuve et un oracle séparés.

## 5. Statut de cet avertissement

Le contre-exemple du §1 réfute le Théorème 4 tel qu'écrit et celui du §3 réfute l'équivalence annoncée au Théorème 3. Les remarques du §4 sont des obligations de preuve supplémentaires; elles ne prétendent pas réfuter toute approche par dualité inversive. Aucun benchmark, même favorable, ne peut fermer ces obligations de complétude.
