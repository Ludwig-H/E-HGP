# Distance de mesures et dates en norme SUP

## Hypothèses et portée

On fixe `k=2`, qualification `m=3`, sites distincts de poids unitaires et dates en **rayon**. La règle de
pendaison satisfait A5 : si le profil qualifié d'un site est une unique remontée sans rival qualifié à aucun
rayon, le site entre à sa première couverture qualifiée. On compare les dates des identifiants communs en norme
SUP. Les probabilités empiriques normalisées servent **seulement à définir la distance d'entrée** : elles ne
remplacent pas les comptes unitaires utilisés par la qualification. Ce résultat ne porte pas sur une règle
utilisant réellement des masses normalisées avec des seuils de masse fixes.

## Lemme de lentille, valable dans R³

Soient des sites collinéaires `0<s₁<s₂<…` sur le premier axe et
`L₂(r²)={c : au moins deux sites sont à distance ≤r de c}`. Si `c∈B(0,r)∩L₂(r²)`, le site 0 est dans la boule
centrée en c, donc un site positif sⱼ y est aussi. Puisque s₁ est une combinaison convexe de 0 et sⱼ, il y est
aussi. Ainsi, exactement,

`B(0,r)∩L₂(r²) = B(0,r)∩B(s₁,r)`.

Le membre droit est une lentille convexe, entièrement dans L₂. Tous les témoins de couverture de 0 appartiennent
donc à une même composante, à chaque rayon. Ces composantes se suivent par inclusion : le profil de couverture
de 0, brut ou qualifié, est une unique remontée, même après les fusions. Cet argument ne réduit pas les centres
à l'axe ; il utilise la convexité des boules de R³.

Pour `s₁/2≤r<s₂/2`, cette composante est précisément la lentille de 0 et s₁. Une lentille de 0 et sⱼ, j≥2, est
vide. Toute autre lentille de paire a une extrémité sⱼ≥s₂ et ne peut rencontrer la première : une telle rencontre
donnerait un point dans `B(0,r)∩B(sⱼ,r)`, impossible puisque `sⱼ>2r`. La réunion comporte un nombre fini de
lentilles compactes, donc aucun chemin ne permet de franchir ces séparations. Aucun troisième site n'est couvert
par cette composante pour la même raison.

À `r=s₂/2`, le centre `s₂/2` contient 0, s₁ et s₂ dans sa boule **fermée**, les deux extrêmes sur la coquille.
La composante qui couvre 0 est alors qualifiée. Sa première qualification est donc exactement `s₂/2`, sans rival
qualifié à aucun rayon. A5 impose la date d'entrée `e₀=s₂/2`.

## Famille et saut d'entrée

Fixons R>0, N≥4 et 0<η<2R. Avec les mêmes identifiants pour tous les anciens sites, posons

`X_N={0,2R,4R} ∪ {6R+jR/N : j=0,…,N−4}`, puis `Y_N=X_N∪{η}`.

X_N contient N sites ; Y_N en contient N+1. Tous sont dans `[0,7R)`. Dans X_N, les deux premiers sites positifs
sont 2R et 4R ; dans Y_N, ils sont η et 2R. Le lemme et A5 donnent donc, pour **tout** N≥4,

`e₀(X_N)=2R`, `e₀(Y_N)=R`, `||e(X_N)−e(Y_N)||SUP,IDs communs ≥ R`.

## Couplage explicite pour tout p fini

Notons `μ_N=(1/N)Σ_{x∈X_N}δ_x` et `ν_N=(1/(N+1))(Σ_{x∈X_N}δ_x+δ_η)`. Le couplage laisse une masse
`1/(N+1)` de chaque ancien site sur lui-même, puis transporte son surplus `1/[N(N+1)]` vers η. Les deux
marginales sont exactement μ_N et ν_N. Puisque toutes les distances vers η sont inférieures à 7R,

`W_p(μ_N,ν_N)^p ≤ (7R)^p/(N+1)`, donc `W_p ≤ 7R/(N+1)^(1/p)` pour tout `1≤p<∞`.

Par conséquent,

`||Δe||SUP,IDs communs / W_p ≥ (N+1)^(1/p)/7 → ∞`.

Il n'existe pas de constante lipschitzienne uniforme en N entre ce W_p et cette erreur SUP, pour toute règle
satisfaisant A5, même à R fixé et sur un support compact fixé. Cela contredit aussi une continuité **uniforme**
ayant un module commun à tous ces nuages ; cela n'exclut pas la continuité séparée à chaque cardinalité ni toutes
les autres règles de projection.

## Grille u21 et limites

La variante scalaire `R=N`, `η=1` donne des coordonnées entières : arrière-plan `6N,…,7N−4`. Elle respecte u21
pour `4≤N≤floor((2²¹−1+4)/7)=299593`. Aucune allocation de cette taille ni exécution FULL n'est faite ici.
À pas physique `h=R_physique/N`, ces coordonnées représentent la même famille physique compacte, avec insertion
à h ; la borne finie sur N subsiste pour le profil u21 fixé.

Le théorème asymptotique concerne N et précision non bornés ; il ne prétend pas exclure une constante dépendant
d'un profil u21 fini. Le couplage ne donne aucun no-go pour W∞. Un saut porté par un seul ancien site a une masse
1/N : il ne prouve pas une instabilité de la sortie en L¹ ou Lᵖ pondérée. Aucun comportement de tous les autres
sites, aucune panne mémoire ni performance FULL massive ne sont affirmés.

Enfin, pour une distance non équilibrée, une conclusion analogue exige une convention explicite : **si** ajouter
une masse unitaire coûte au plus un plafond c_λ indépendant de R, alors la distance entre les mesures de comptage
est ≤c_λ tandis que le saut de date vaut R. Une constante uniforme en échelle est alors impossible lorsque
R→∞. Le présent reçu ne suppose pas qu'un coût p-ième puissance soit lui-même la distance.
