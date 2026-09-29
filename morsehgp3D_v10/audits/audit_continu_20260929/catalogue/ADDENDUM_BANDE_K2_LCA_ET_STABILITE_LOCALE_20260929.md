# Bande relative K2 et rattachement au LCA : propriétés et limites

29 septembre 2026. Addendum mathématique à l'audit catalogue de `6206d1d11`. Proposition de recherche, aucun port moteur, aucune mesure de qualité ni campagne de performance. `public_status=not_claimed`, GCP non utilisé. Les rayons ci-dessous ne sont pas des rayons carrés.

## 1. Définir précisément le prototype

On suppose des points distincts, n ≥ 2, la tranche FULL K2 exacte, ses plateaux atomiques et η ≥ 0 fixé. Pour un point x :

$$\alpha_x=\frac{1}{2}\min_{y\ne x}\lVert x-y\rVert,\qquad A_\eta(x)=\lbrace y\ne x:\rho_{xy}=\frac{1}{2}\lVert x-y\rVert\le(1+\eta)\alpha_x\rbrace.$$

On retient toutes les égalités, sans choix par ID. À chaque paire incidente {x,y}, on associe son milieu et la composante de la multicouverture L2 au rayon ρ_xy qui contient ce milieu. Cette composante existe toujours. La boule diamétrale de la paire n'est pas nécessairement un enregistrement du catalogue : elle peut contenir beaucoup de points intérieurs. Une simple recherche de sa clé dans le catalogue n'est donc pas une implémentation complète de cette association.

Le nœud v_x est le LCA de ces composantes, évaluées chacune à son propre rayon. Convention de date auditée ici :

$$t_x=\max\bigl(b(v_x),\max_{y\in A_\eta(x)}\rho_{xy}\bigr).$$

Le point est absent avant t_x, puis suit les ancêtres de v_x. Si l'API fournit un représentant ancestral ancien plutôt que la composante active au rayon de la paire, il faut d'abord le remonter à ce rayon. Au besoin, on remonte aussi le LCA à t_x. La convention de coupe est fermée.

Cette date exige que toutes les paires retenues aient atteint leur rayon propre. Une variante qui attend seulement que leurs milieux soient naturellement présents et connectés peut utiliser des dates plus précoces ; c'est un autre objet. D2(milieu) peut être strictement inférieur à ρ_xy. Il ne faut pas identifier ces deux variantes implicitement.

## 2. Propriétés effectivement prouvables

### Laminarité

Chaque point suit une seule branche, sans réaffectation ultérieure. Toute coupe partage les points déjà entrés entre les composantes de l'arbre ; quand le rayon augmente, un bloc ne peut que croître ou fusionner. On obtient donc des partitions emboîtées du sous-ensemble actif. Pour obtenir des partitions de tout X, on peut déclarer chaque point inactif singleton : cette convention est explicite, pas une assertion d'appartenance précoce à L2.

La réduction des branches vides et la contraction des nœuds unaires doivent préserver les dates d'entrée des points. Le résultat peut nécessiter des événements d'entrée au-dessus du dernier nœud FULL : ce n'est pas toujours une simple suppression de nœuds de l'export existant (§6).

### Équivariance

Les distances, les milieux, les composantes et le LCA sont intrinsèques. La règle est équivariante par permutation des points et par isométrie, à isomorphisme des arbres près, si toutes les égalités sont conservées et si les plateaux sont atomiques. Elle est aussi équivariante par homothétie, avec multiplication des dates. Cela n'affirme pas l'identité binaire de dumps utilisant l'ordre de Morton. Sur le moteur u18, l'isométrie doit garder les coordonnées dans le profil, sans requantification intermédiaire.

### Monotonie en η — attention au sens

Sur un nuage et un arbre FULL fixés, augmenter η ajoute des paires. Le LCA ne peut que remonter et t_x ne peut que croître. À rayon fixé, les partitions obtenues avec les points inactifs singletons se **raffinent**, elles ne deviennent pas plus grossières : les points attendent plus longtemps avant de fusionner.

Une formulation sans ambiguïté donne, pour x ≠ z, la hauteur de fusion u_η(x,z) comme le premier rayon r tel que :

1. r est au moins le rayon propre de chaque paire retenue par x ou z ;
2. tous les milieux de ces paires appartiennent à une seule composante de L2(r).

Ajouter des paires ne peut satisfaire plus tôt ces deux conditions. Cette formulation est exactement celle du LCA daté ci-dessus. Elle prouve aussi l'ultramétricité, avec u(x,x)=0 comme convention pour les singletons.

## 3. La bande déplace la discontinuité ; elle ne la supprime pas

Témoin rationnel pour η = 1/10, trois points colinéaires dans R3 :

$$X_-=(0,10-\delta,21),\qquad X_+=(0,10+\delta,21),\qquad 0<\delta<\frac{1}{2}.$$

Les deux nuages sont appariés avec déplacement maximal 2δ. Pour le point médian, le voisin gauche est le plus proche. Le voisin droit est exclu dans X_- et inclus dans X_+ :

$$\frac{11+\delta}{2}>\frac{11}{10}\frac{10-\delta}{2},\qquad\frac{11-\delta}{2}<\frac{11}{10}\frac{10+\delta}{2}.$$

Le point gauche ne retient que le médian dans les deux nuages. Les deux composantes K2 des paires adjacentes fusionnent exactement au rayon 21/2 : avant, les boules des deux extrémités sont disjointes ; à ce rayon, leur point de contact appartient également à la boule du médian. Ainsi :

$$u_-(\mathrm{gauche},\mathrm{milieu})=5-\frac{\delta}{2},\qquad u_+(\mathrm{gauche},\mathrm{milieu})=\frac{21}{2}.$$

Le saut tend vers 5,5 quand le déplacement tend vers zéro. En u18, les témoins `(0,9999,21000)` et `(0,10001,21000)`, sur un axe, donnent le même mécanisme avec un déplacement de 2 unités et un saut de rayon de 5 500,5. C'est une preuve géométrique, pas une nouvelle exécution native rapportée.

La bande améliore le comportement autour de l'ancienne égalité des deux plus proches voisins ; elle crée une nouvelle frontière à leur rapport 1+η. Aucun η fini fixé ne donne par ce mécanisme une stabilité globale sous petits déplacements.

## 4. Une garantie locale, conditionnelle à une marge

Apparier deux nuages de même cardinal, avec déplacement de chaque point au plus ε. Chaque rayon de paire et chaque α_min changent d'au plus ε. Pour une paire incidente :

$$g_{xy}=\rho_{xy}-(1+\eta)\alpha_x,\qquad |g'_{xy}-g_{xy}|\le(2+\eta)\varepsilon.$$

Si toutes les paires ont une marge stricte `|g_xy| > (2+η)ε`, les identités des paires retenues sont inchangées. À η=0, cette condition suffisante est vacue pour toute paire minimale ; ne pas la présenter comme certificat universel. Pour η>0, elle est vérifiable en principe, mais son coût est à mesurer. Il suffit de contrôler les distances immédiatement de part et d'autre de chaque seuil, pas de matérialiser toutes les paires exclues.

Sous cette invariance des identités, dates d'entrée et hauteurs de fusion du prototype changent d'au plus 2ε.

*Preuve.* Chaque milieu retenu se déplace d'au plus ε. Pour tout centre fixe c, la distance au deuxième voisin dans le nouveau nuage est au plus l'ancienne plus ε. Une composante ancienne à r est donc un sous-ensemble connecté de L2 nouveau à r+ε. Pour rejoindre chaque milieu déplacé, on lui ajoute le segment de longueur au plus ε depuis l'ancien milieu : tout ce segment appartient à L2 nouveau à r+2ε. Les rayons propres des paires ont, eux, augmenté d'au plus ε. Les deux conditions de §2 sont donc satisfaites au plus tard à r+2ε. On échange les deux nuages pour l'autre inégalité. Le même argument appliqué aux seules ancres d'un point traite sa date d'entrée.

Cette garantie est en rayon. Elle n'est ni une borne uniforme identique en rayon carré, ni une stabilité aux suppressions, ni un résultat de consistance statistique, ni une garantie pour EOM ou ARI.

## 5. Paires non incidentes et supports q3/q4

Une paire diamétrale {a,b} non incidente dont la boule de centre c et de rayon r contient x satisfait :

$$\lVert x-a\rVert^2+\lVert x-b\rVert^2=2r^2+2\lVert x-c\rVert^2\le4r^2.$$

Donc d_nn(x) ≤ √2 r, soit r ≥ √2 α_min(x). Pour η < √2−1, notamment η=0,1, aucune de ces paires non incidentes ne peut entrer dans la bande.

Le même argument vaut pour une boule critique à support positif S ne contenant pas x : si les coefficients barycentriques du centre sont λ_s, leur moyenne donne `Σ λ_s ||x−s||² = r² + ||x−c||²`. Cela exclut aussi les boules q3/q4 **dont x n'est pas un site de support**.

Ce n'est pas une exclusion de toutes les boules q3. Un triangle aigu très fin, de sommets x=(0,0), a=(R,h), b=(R,−h), a un rayon `(R²+h²)/(2R)` dont le rapport à α_min(x) tend vers 1 quand h/R tend vers zéro. À K2 non pondéré, q_min=3 décrit une fusion ; q_min=4 n'est pas admis. Les présentations dégénérées d'arité 4 doivent être réduites à leur q_min. Aucune conclusion analogue de suffisance n'est ici prouvée pour K≥3.

## 6. Une entrée peut dépasser la dernière fusion FULL

Exemple K2, η=0,1, coordonnées u18 : x=(0,1,0), a=(100,2,0), b=(100,0,0), w=(109,1,0).

- α_min(x)=√10001/2, environ 50,0025.
- La paire xw, de rayon 54,5, est dans la bande.
- La dernière fusion FULL est celle du triangle xab, au rayon 10001/200 = 50,005. Les points a,b,w sont déjà dans la même composante auparavant.
- La date choisie en §1 rattache donc x à la racine, mais à 54,5, après sa naissance.

Un format de dendrogramme qui impose que le niveau du parent final soit au moins toute date d'entrée doit prolonger la branche de racine et ajouter l'événement correspondant. Afficher simplement le dernier niveau FULL comme hauteur de la racine du dendrogramme de points serait incorrect pour cette convention.

Vérification rationnelle courte, effectuée en lecture seule avec `reference/hgp10_ref.py` : le dernier lot K2 de cet exemple est bien `100020001/40000` en rayon carré ; le rayon carré de xw vaut `11881/4`. Aucun nouveau test natif de performance n'est invoqué.

## 7. Coût et mémoire : ne pas confondre les profils

Soit c=1+η. Le nombre de paires retenues par une seule ancre peut être linéaire en n, même pour η=0,1 en u18. Prendre x=(0,0,0) et y_j=(100000,j,0), pour j=0,...,45000. Tous les 45 001 y_j sont retenus par x ; chaque y_j a pourtant un voisin à distance 1. Aucun grand calcul n'est nécessaire pour établir ce témoin.

Sur des coordonnées réelles de dynamique non bornée, le nombre total peut être quadratique : sur la droite x_i=12^i, à η=0,1, chaque point sauf le premier retient tous ses prédécesseurs. Cette famille exige une étendue exponentielle ; **elle ne prouve pas une croissance quadratique dans le profil u18 fixé**.

Au contraire, une borne grossière existe pour la seule taille des listes sur cette grille. Les distances aux voisins les plus proches sont entre 1 et √3(2^18−1), donc dans 19 bandes dyadiques. Dans une bande `[2^j,2^(j+1))`, les ancres sont distantes deux à deux d'au moins 2^j. Celles qui retiennent un site y sont dans sa boule de rayon `2c·2^j`. Des boules ouvertes disjointes de rayon `2^(j−1)` autour de ces ancres donnent, par comparaison des volumes, au plus `(4c+1)^3` ancres par bande. Ainsi :

$$\sum_x |A_\eta(x)|\le19\bigl(4(1+\eta)+1\bigr)^3 n.$$

C'est une borne linéaire à largeur entière et η fixés, avec une constante très grossière. Elle ne borne pas les visites d'index ni le coût d'associer chaque paire à une composante FULL. Une recherche naïve de toutes les distances reste quadratique, malgré la borne de sortie.

Pour l'implémentation future : accumuler le LCA et le maximum de rayon au fil des voisins retenus, sans tableau global de paires. La mémoire additionnelle des attaches peut rester O(n), hors index et prétraitement LCA. Un LCA par binary lifting coûte O(V log V) mémoire ; ce coût peut être rédhibitoire sur les grandes tours. Un index de LCA linéaire ou une procédure hors ligne doit être évalué séparément. L'association paire→composante nécessite elle aussi un algorithme explicite et qualifié : elle n'est pas gratuite parce que le catalogue FULL existe.

### Extension de la borne d'incidences aux voisinages k-cœur

Pour des sites distincts, 2 ≤ k ≤ n, k comptant le point lui-même, considérer les voisins y vérifiant `||x−y|| ≤ c d_k(x)`. Dans une bande `s ≤ d_k(x) < 2s`, les sources qui visent y sont dans B(y,2cs). Prendre parmi elles un sous-ensemble maximal à séparation au moins s/2. Ses boules ouvertes de rayon s/4 sont disjointes et incluses dans B(y,(2c+1/4)s) : leur nombre est au plus `(8c+1)^3`.

Chaque source restante est à moins de s/2 d'un centre de ce sous-ensemble. Chaque boule de couverture contient au plus k−1 sites : son rayon est strictement inférieur à s, alors que le k-ième voisin de son centre est à distance au moins s. Par conséquent, le nombre de sources visant y dans la bande est au plus `(k−1)(8c+1)^3`. Sur u18 :

$$\#\lbrace(x,y):\lVert x-y\rVert\le c\,d_k(x)\rbrace\le19(k-1)(8c+1)^3 n.$$

Le cas k=1, de rayon nul, et les doublons doivent être traités séparément. Cette borne porte sur les incidences des voisinages, pas sur leurs sous-ensembles. L'étoile du début de §7 a une liste de taille Θ(n), bien que le total des listes soit linéaire. Énumérer toutes les paires dans cette seule liste coûte déjà Θ(n²). Ni les candidats q3/q4, ni les boules critiques, ni les tests d'index ne sont bornés par ce lemme.

## Conclusion bornée

La règle fournit bien une projection déterministe, laminaire, sans départage géométrique arbitraire entre paires retenues. Elle possède une stabilité locale avec marge, mais pas une stabilité globale au seuil. Ses coûts peuvent être contrôlés en mémoire par streaming ; aucune latence ni qualité statistique n'est acquise. Elle reste un prototype à comparer aux entrées core et cover, en publiant la fraction de points différés et la marge des seuils.
