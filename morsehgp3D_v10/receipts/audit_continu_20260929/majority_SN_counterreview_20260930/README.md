# Contre-lecture S/N : dénominateurs et réserve exacte g=1

Audit mathématique indépendant du mémo majorites_continues, pas une campagne moteur. MEMO.md lu intégralement sous SHA c3611be90957bd76fa12b374a623f9576a38f260b3fa71698ca036579e69ab91 ; nouvelle observation à18:06:13UTC : SHA7ec56b4d07dc7728756662471efc9997cca3337e45db318b9132e5edcad03f02. Le passage N relu est inchangé. mmc.py SHA eba356d3c50b00fbc01b0c52bb2ece6928c3eee5dd90696ececc26e08aac9013 ; mmt.py SHA93f6acd0de4146a20bc8078c7d339468a5f118a331e8ac8e39f4107021f52818. Ces chemins sont une provenance, pas une dépendance LIVE du paquet.

## S : contre-lecture théorique, pas qualification nouvelle

Sur les univers étiquetés Γ_K et paires, les identités persistent ; les propriétaires sont transportés par les applications φ/ψ, et la perte de masse est au plus Δ. Pour K≥2, sites distincts et une famille finie non vide, a>0. Un vote minimisant r_F=a a exactement poids1, donc W≥1. Le dénominateur de MM n’est pas arbitrairement proche de zéro. K1/α0 demande un traitement séparé : les formules de MMg et MMt avec division par A ne couvrent pas ce cas.

Poser λ=√(1+η′). La fonction clip w(ρ)=max(0,1+(1−ρ²)/η′), restreinte àρ≥1, est globalement2λ/η′-Lipschitz (elle est nulle aprèsλ). Si les naissances et a sont d-Lipschitz, chaque vote positif dans au moins un nuage donne |Δρ|≤(1+λ)dε/a_min, en choisissant le côté positif. Ainsi Δ≤N_union·2λ(1+λ)dε/(η′a_min) EXACTEMENT, sans O(ε²). N_union compte l’union des votes positifs des deux configurations ; la limite locale doit également prendre en compte les votes au bord qui deviennent positifs.

Étape propriétaires de S : à s≥max(tX,tY), A est la lignée majoritaire X. Si son image diffère de B majoritaire Y, sa masse est <WX/2+3Δ/2 ; le retour ψ de B, disponible dans X après au plus2dε, porte >WX/2−3Δ/2. Si3Δ<W_min, cette masse est positive. Tant que les deux lignées restent séparées, G_X=m_A et μ_X≤3Δ/WX. Leur fusion f fait augmenter G ; la définition du cône impose f≤tX+3κa_maxΔ/W_min. Le maximum max(2dε,3κa_maxΔ/W_min) couvre l’admission du retour et cette fusion, puis naturalité et φψ=anc_{+2ε} donnent l’égalité des images au rayon s′+ε. Le repli d_K/2 est un argument indépendant via la couverture du même point dans Y ; prendre le minimum des deux délais est cohérent.

Cette relecture ne remplace pas des tests de S ni une qualification des objets natifs. Le retrait d’un témoin fort au contact n’est pas un petit changement de poids dans un univers fixe : le vote perdu peut avoir un poids strictement positif. Le théorème ne s’applique donc pas à MMc/MMf dans ces mécanismes ; le mémo le reconnaît. La bande souple soigne sa propre frontière, pas une disparition forte ailleurs.

MMt : W≥ηα² protège son domaine lorsque α>0, mais ce n’est PAS W≥1. Son code _compteurs ajoute len(kids)−1 seulement si len(kids)≥2 : la prose M doit écrire max(κ_f−1,0), ou limiter la somme aux fusions de plusieurs lignées couvrantes. Pas de défaut d’implémentation observé sur ce point. Le seuil T½ peut être un infimum de franchissement continu avec m=W/2 exactement ; l’argument des propriétaires doit utiliser ≥/limite droite au lieu de recopier littéralement > de MM discret. S_t n’est pas entièrement audité par ce paquet.

## N : contre-exemple exact à la membership annoncée avec g=1

K2, η′=1, α_x=1 ; x=0, a=(−2,0,0), deux b à2ρu±, oùρ=6/5 et u±=(99/101,±20/101,0). Les directions sont unitaires, distinctes et proches de l’axe. κ=25, γ=1/50, donc2κγ=1.

Les votes de x pèsent1,14/25,14/25 ; W=53/25. R a part28/53>1/2+γ et marge3/53. Mais les deux votes droits ne sont dans la même composante qu’au rayon T½=202/165=ρ·101/99>ρ. La fusion L/R est f=√(12101/2525)<1+ρ. Le cône de MM ne retarde pas T½ ici : f−25·3/53<T½. Ainsi t_x=T½>f−1, contrairement au délai imposé par D(γ,min(2κγ,1)).

check.py recalcule les SIX naissances de Γ2, les QUATRE cofaces par MEB exact, les masses et le premier seuil majoritaire ; aucune bibliothèque native importée. Les comparaisons avec f sont des comparaisons rationnelles de carrés positifs, pas des décisions flottantes.

Ceci réfute la phrase exacte « MMκ est dans D(γ,min(2κγ,1)) » au bord g=1. Cela ne réfute PAS la croissance Ω(N) du prix de comptage, ni la continuité locale S. Le mécanisme est arbitrairement proche de l’axe : remplacer10 par M dans u=((M²−1)/(M²+1),±2M/(M²+1)) puis M→∞ conserve t≥ρ>f−1 pour des directions toujours distinctes ; seul M10 est capturé ici.

Réparation possible : prendre un délai strict g<min(2κγ,1), contrôler le diamètre angulaire assez petit pour T½≤f−g et les propres entrées des sites b, puis passer à la limite angulaire dans la borne inférieure. Ou conserver un terme d’erreur angulaire explicite. Il faut une preuve de cette réparation avant de qualifier les constantes exactes ; le seul ajout −1 ne clôt pas cet écart géométrique.

## Capture portable

Deux petites exécutions Fraction normal/−O à18:09:01.348–18:09:01.510UTC ; source hachée avant/après, code0, stderr vide, stdout identiques1623octets. Aucun préflight ni run en échec, aucune source partagée modifiée. Lecteur hash-first : python3 -B verify.py puis python3 -B -O verify.py. Aucun moteur, benchmark industriel, EOM, GCP ou preuve nouvelle d’une constante uniforme.
