# Secteurs et corde : suffisance et réalisation bornée

Pour une boule positive de support q∈{2,3,4}, possédée par une arête maximale AB, chaque rejet ci-dessous fournit hq=smax−q+1 identités strictement intérieures. Il implique donc p+q>smax. Les [sources géométriques](receipts_20260904/s1_sources.json), le [reçu arithmétique](receipts_front_20260905/secteur_corde_arithmetic.json) et les [exécutions compilées](receipts_front_compiled_20260905/secteur_corde/receipt.json) sont des autorités distinctes. `public_status=not_claimed`.

## Secteurs : couvrir les centres, compter sans doublon

Les [bornes de rayon](S1_COURANT.md) placent le centre c dans le plan bissecteur, à distance du milieu m au plus D/√12 en q3 ou D/√8 en q4. Le code choisit les deux vecteurs d×axe de normes maximales, d=B−A. Cela omet l'axe k de plus grande |dₖ|>0. Avec u=d×eᵢ, v=d×eⱼ :

$$|u\times v|^2=d_k^2D^2\geq D^4/3,\qquad\max(|u-v|^2,|u+v|^2)=D^2+d_k^2+2|d_id_j|\leq2D^2.$$

Les distances carrées aux droites d'arêtes du losange ±u,±v sont les quotients de ces quantités, donc au moins D²/6. Les tests de contenance passent dès A=B=1 pour les dénominateurs 8/12 réellement appelés ; les accroissements de la boucle sont inatteignables. Le losange contient le disque requis et l'octogone des ±u,±v,±(u+v),±(u−v) le contient à son tour. Ce résultat ne s'étend pas à un choix d'axes arbitraire ni à un dénominateur inférieur à 6.

Pour w′=2z−A−B, l'intérieur est donné par 4w′·(c−m)>|w′|²−D². Cette inégalité affine stricte aux trois sommets d'un secteur triangulaire est stricte sur tout son triangle fermé. Le compte du secteur contenant le centre réel minore donc la profondeur ; les huit comptes ne s'additionnent pas.

Le crédit `base` porte sur A sans a et B sans b, dont la [convexité aux coins](FRONT_ET_TEMOINS_COURANT.md) certifie les témoins sur les centres admissibles. Les sites extérieurs à A∪B sont disjoints de cette population. Ainsi `min_k max(cnt[k],cnt_out[k]+base)` reste un minorant uniforme, sans exiger que le crédit soit valable sur tout l'octogone artificiel. Les sites sectoriels sont diamétralement intérieurs ; les filtres radiaux ne peuvent ajouter de témoin. Le `break` n'est utilisé qu'avec cover trié : les témoins diamétraux q4 sont dans les classes 0..7, avant la coupure au coefficient 3. Le mode non trié ne fait qu'un `continue` sur un minorant de distance.

## Corde : le futur sommet n'est pas un témoin

Pour un vrai seed aigu ABX possédé par AB, poser G=|d×(X−A)|²>0, n=d×(X−A), E=AX² et X2=BX². Les centres équidistants sont cμ=c3+μn/(2G), de rayon carré R3²+μ²/(4G). Jung impose 2μ²≤J=D²(3G−2EX2) ; l'acuité et l'arête maximale donnent R3²≤D²/3, donc J≥GD²/3>0.

Avec W de q3 et N=W−Gd, d·W=GD² implique exactement L=G(|w′|²−D²)−2w′·N=4P. La puissance au centre cμ est proportionnelle à P−μB, B=n·(z−A). Les divisions par quatre sont donc exactes, y compris pour L négatif.

La racine corrigée r=floor(sqrt(floor(J/2))) vérifie r²≤floor(J/2)≤J/2<(r+1)². Ainsi μ̂=r+1 est strictement extérieure, même sur carré parfait. Les quatre intervalles de sommets μj=(2j−4)μ̂/4 couvrent la corde admissible. Négativité de L−(2j−4)μ̂B aux deux extrémités entraîne négativité sur tout l'intervalle fermé.

Au centre de sa propre boule, le futur sommet Y a une puissance nulle : il ne peut être compté dans l'intervalle contenant ce centre. Aucune soustraction supplémentaire d'arité n'est nécessaire. Les exclusions explicites A/B/X sont cohérentes. Les comptes cœur et corde sont réunis par OU, car leurs témoins peuvent se recouvrir ; la branche P>0 est elle aussi traitée avant son saut.

## Chaque opération active reste représentable

Avec M=65535 et la base non dilatée, les produits de Gram sont promus en i128 : cross²≤12M⁴, comparaisons de contenance≤144M⁴<2^72. Les sommets sectoriels ont des coordonnées ≤2M ; les tests `4*dot` sont sous 48M²<2^38. Les comptes distincts sont ≤n<2^30 et `cnt_out+base<2^30+9` ; les lanes inexistantes n'emploient pas leur seuil nul.

Pour la corde, W−Gd est formé sans dépassement sous 45M⁵ avant d'utiliser N=2G(c3−m), qui affine |Nᵢ|≤9M⁵. Les opérations écrites donnent |L|≤216M⁶<2^104, |B|≤6M³<2^51 et |J|≤81M⁶<2^103. La racine μ̂≤8M³<2^51 rend chaque produit cμ̂B≤192M⁶, puis |L−cμ̂B|≤408M⁶<2^105. Les promotions précèdent les multiplications.

`isqrt128_floor` reçoit ici J/2<2^102 ; son domaine déclaré v<2^120 donne une proposition finie convertible ≤2^60 sous les opérations conformes. Le premier carré est ≤2^120, le carré suivant <2^121, avant correction. Les décréments puis incréments terminent à r²≤v<(r+1)² sans sous-débordement. La correction répare l'arrondi initial, pas un NaN ou un cast déjà invalide. Les [marges flottantes](FILTRES_FLOTTANTS_COURANTS.md) conservent ensuite exactement le signe strict, repli entier inclus.

## Témoins exécutés, sans nouvelle qualification

Le [pont](secteur_corde_compiled_probe.cpp) appelle les vrais helpers ; le [pilote](secteur_corde_compiled_runner.py) les confronte aux entiers Python et centres rationnels par élimination de Gram. Par build O2/UBSan : 736 bases non dilatées, 212 racines sous quatre arrondis, 96 cas affines géométriques et 156 cas de corde. Les trois classes de signes sont non vides, avec 72 replis sous modes dirigés. Les commandes ajoutent `-frounding-math -ffp-contract=off` et restent distinctes du pipeline Release.

Le mutant produit `sector-kill-nonstrict` crédite indûment un shell ; la faute d'audit `chord-nonstrict-parameter` conserve séparément son statut de modèle. Les [contre-fixtures de base et largeurs](secteur_corde_arithmetic_20260905.py) préservent aussi la borne 1/6 atteinte et le mauvais choix d'axes dépendants. Aucun résultat de ces portes locales ne démontre un gain de tour. Les menues corrections de commentaires appartiennent aux [questions secondaires](QUESTIONS_SECONDAIRES.md).
