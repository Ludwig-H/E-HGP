# MMt : lignée médiane, seuil continu et transfert

Contrelecture indépendante du mémo privé `majorites_continues`, 30 septembre 2026.
Exploration mathématique, `public_status=not_claimed`. Aucun moteur, import natif,
GCP, EOM, benchmark ou fichier partagé modifié. Les cinq cas sont des arbres
abstraits rationnels, pas des nuages géométriques nouvellement qualifiés.

## Résultat utile au port

La réduction proposée par l'auditeur principal à une seule lignée médiane est
correcte. Elle conserve les dates et propriétaires MMt, y compris un plateau
exact à moitié et un maximum critique intérieur. Le coût ci-dessous est celui
du noyau une fois les atomes fournis, pas celui de leur extraction du nuage.

Un atome est `(v,c,e)` avec `c=c_x(v)`, `e=min(d_v,(1+eta)A)` et `w=e-c>0`.
Les nœuds d'un sous-arbre forment un intervalle de l'ordre DFS, y compris les
atomes des nœuds internes. Choisir un atome médian `m` dont l'intervalle de poids
contient `W/2`, avec une convention quelconque aux égalités. Tout sous-arbre
excluant m est entièrement à sa gauche ou à sa droite et pèse au plus W/2.

La masse actuelle d'un composant est au plus la somme des poids finaux de son
sous-arbre. Un composant strictement majoritaire doit donc contenir m et être
un ancêtre de m. Pour tout theta>1/2, le premier niveau où G atteint theta W
est exactement celui où la masse de la lignée de m l'atteint. Avant la naissance
de m, cette masse est zéro et aucune majorité stricte n'est possible.

Pour un atome v, poser `h=b(LCA(v,m))`, `a=max(c,h)`. Sa contribution sur la
lignée est exactement

`1_{s>=h} max(0,min(s,e)-c)`.

Elle donne un saut `min(a,e)-c` à a ; si a<e, elle donne aussi une pente +1 à a
et -1 à e. Au plus 2D événements avant agrégation. Si a>=e, tout son poids
arrive au LCA par un seul saut. Un atome hors lignée a h>=d_v et n'a donc pas
de rampe ; les atomes sur la lignée ont des vies disjointes. La pente totale
est bien 0 ou 1, pas le nombre des branches déjà fusionnées.

Avec LCA prétraité sur l'arbre global, tri DFS/médiane et tri des événements :
`O(D LCA + D log D)` temps, O(D) espace de travail ; puis balayage linéaire,
un seul point critique `W^2/(16 kappa^2 A)` et une recherche d'ancêtre finale.
Cela ne requiert ni union de tous les chemins d'ancêtres ni arbre virtuel.
Il reste à payer la préparation globale LCA et l'extraction de tous les atomes
de couverture : aucune borne sous-quadratique globale n'est acquise ici.

## Seuil strict : un détail indispensable

`T_half=inf{s:G(s)>W/2}` n'est pas toujours `T(1/2)`. Si G reste exactement à
W/2 pendant un intervalle, le premier seuil strict est à la fin de ce plateau.
Dans `half_plateau`, G=3 sur [4,9[, W=6 : T_half=9. À un franchissement continu,
la masse à T_half peut être seulement W/2 ; le propriétaire est celui de la
majorité juste à droite. Le balayage et le code de référence gèrent ces deux
situations correctement dans les cinq cas conservés.

## Lemme T : preuve réparée, même constante

Le compteur M doit être `sum max(kappa_f-1,0)` ou porter seulement sur les
fusions ayant au moins deux enfants déjà couvrants. `_compteurs` le fait déjà.
Ce n'est pas un défaut d'implémentation.

Pour intégrer le transfert, prendre la partie non vide de la bande source
`J=[alpha_X,min(r,E_X,E_Y-epsilon)]`, sinon J est vide. Pour rho dans J,
rho+epsilon est dans la bande cible, les composantes couvrantes se transfèrent
et leurs collisions sont imputables à des fusions source dans ]rho,rho+2eps].
On utilise le nombre d'images distinctes, donc `max(n_C-k,0)`, avant de remplacer
le jacobien 2(rho+eps) par 2rho. Chaque fusion fait perdre au plus
`4 eps E_X (kappa_f-1)`. La tranche supprimée a longueur au plus
`(lambda+1)eps` et multiplicité au plus n_X. Ceci redonne Delta_XY, y compris
quand J est vide. Les contacts isolés ont mesure nulle ; pas de dépendance à une
position générale. W se transfère en prenant un rayon assez grand pour la racine.

## Propriétaires S_t : adaptation au seuil continu

Au rayon s>=max(t_X,t_Y), la lignée A du propriétaire X porte au moins W_X/2,
et strictement plus à droite si s=T_half est un franchissement continu.
Transporter la lignée B de Y et utiliser les mêmes pertes dirigées : si les
images restent distinctes, `m_X(A)<=W_X/2+3Delta/2`, tandis que l'image de B
après 2eps porte au moins `W_X/2-3Delta/2>0` si 3Delta<W_min.

Après T_half, G_X est la masse de A ; à T_half elle en est encore la masse,
éventuellement ex aequo à moitié. Tant que les deux lignées restent distinctes,
`mu_X<=3Delta/W_X`. Leur fusion f après admission de l'image de B donne un vrai
saut positif. La limite gauche du quantile/cône donne
`t_X>=f-3 kappa alpha_X Delta/W_X`. Ainsi elles ont fusionné au plus tard à
`s+max(2eps,3 kappa alpha_max Delta/W_min)` ; naturalité et composition des
entrelacements donnent alors l'égalité des propriétaires translatés. Le repli
indépendant d_K/2 reste valide. Remplacer les inégalités initiales strictes de
la preuve S par des inégalités larges suffit ; le S discret n'est pas invalidé.

Cette contrelecture est théorique : elle ne transforme pas les contrôles de
quelques arbres en qualification générale de S_t ou du moteur natif.

## Réserve réelle : constante finie simplifiée

La borne exacte codée est
`B_t=(1+kappa)eps+2 kappa alpha_max (Delta_XY+Delta_YX)/W_min`.
Déduire sans facteur d'échelles
`(1+kappa)eps+(8 kappa lambda/eta) C eps`, avec
`C=2 Mbar+(lambda+1)nbar`, est incorrect pour une paire finie. La déduction sûre
est

`B_t<=(1+kappa)eps+(4 kappa lambda/eta) alpha_max(alpha_X+alpha_Y)/alpha_min^2 C eps`.

On peut majorer le facteur par `2(alpha_max/alpha_min)^2` ; puisque alpha est
1-Lipschitz, une borne finie emploie `(1+eps/alpha_min)^2`. Le coefficient 8
sans ce facteur est une limite locale au premier ordre, pas cette majoration
finie de la borne exacte.

Deux sites X={-1,1}, Y={-2,2}, eps=1, eta=3, kappa=2 satisfont kappa=lambda.
M=0, n=1, W_X=3, W_Y=12, Delta_XY=12, Delta_YX=24. La borne exacte vaut 99 et
le simplifié annoncé 35. L'écart réel des dates vaut seulement sqrt(5/2) :
**ce n'est pas un contre-exemple à la stabilité de MMt**.

## Preuves conservées

`reference_functions.py` contient uniquement `_anc` et `mmt_point`, extraits
par AST de mmt.py lu intégralement. `check.py` les exécute dans un environnement
Fraction/QS minimal sans import mmc ni natif. Son comparateur de racines utilise
des intervalles rationnels certifiés, jamais des flottants pour décider.

Cinq arbres : branche seule (D=1), plateau moitié (D=2), fusion N-aire (D=3),
majorité continue+marge de fusion (D=4), point critique gagnant (D=12).
Toutes les masses de lignée sont recoupées par remontée explicite aux événements,
milieux, tiers et points voisins ; W,T_half,T1,date et owner coïncident avec
la fonction de référence. Le critique donne `s*=25/16`, `t=49/40`.

Deux captures finales normal/-O : 18:39:06.579548–18:39:06.805765 UTC,
codes 0, stderr vides, stdout identiques 997 octets. Un préflight réussi a
précédé la capture finale ; aucun échec de test. Une correction purement locale
de la fabrique d'atomes a été faite avant sa première exécution.
Les quatre sources partagées sont identiques avant/après la capture ; leurs
SHA et commandes figurent dans receipt.json. Le mémo est toujours au SHA
7ec56b4d07dc7728756662471efc9997cca3337e45db318b9132e5edcad03f02 ; mmt.py
93f6acd0de4146a20bc8078c7d339468a5f118a331e8ac8e39f4107021f52818.

Lecture autonome : `python3 -B verify.py`, puis `python3 -B -O verify.py`.
Le lecteur vérifie d'abord l'inventaire et tous les hashes, puis le reçu et
deux replays bornés. Aucune qualification FULL/GPU/100ms héritée.
