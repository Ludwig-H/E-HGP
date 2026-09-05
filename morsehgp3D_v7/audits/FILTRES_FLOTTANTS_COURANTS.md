# Filtres affine, Jung et corde : preuve des marges

Les marges suffisent sous binaire64, arrondi au plus proche, conversions et FMA correctement arrondies, sans réassociation. Cette preuve, citée par le [contrat constructeur](../docs/QUALIFICATION_S1_PRIMITIVES.md#5-binaire64-fma-et-compilation-effective), porte sur les [sources épinglées](receipts_20260904/float_sources.json) et leurs appels. Le [domaine CPU](DOMAINE_CPU_COURANT.md) porte les exécutions ; `public_status=not_claimed`.

Poser u=2⁻⁵³ et δ=2⁻⁴⁰=8192u. Pour les opérations normales concernées, RN(x)=x(1+θ), |θ|≤u ; les zéros restent exacts. Les entiers vᵢ=2zᵢ−aᵢ−bᵢ, q=|v|²−D² et leurs maxima sont sous 2^36, donc exactement représentables. G>0 et Nᵢ sont convertis une fois ; qmax/vmax proviennent du même cover que les appels. Les résultats affines arrondis restent entiers, E est multiple de 2⁻⁴⁸, les extrémités divisées par quatre de 2⁻⁵⁰ et les produits avec gardes de 2⁻¹⁴⁰. Les magnitudes u16 restent sous 2^230 : ni débordement ni sous-normal n'invalide ce modèle.

## Affine : inclure les arrondis des bornes finales

Noter g,nᵢ les conversions de G,Nᵢ et définir :

$$L=Gq-2\sum_iN_iv_i,\qquad\widehat S=|gq|+2\sum_i|n_iv_i|,\qquad S_{\max}=gq_{\max}+2\sum_i|n_i|v_{\max}.$$

Les conversions apportent au plus uŜ/(1−u). Une multiplication initiale, deux FMA de somme et une FMA finale donnent au plus γ4 Ŝ, avec γ4=4u/(1−4u) ; doubler la somme est exact. Donc |lh−L|<6uŜ≤6uSmax. L'inégalité suit de 1−22u+24u²>0.

Dans `affine_l_bound`, chaque terme positif de M traverse au plus quatre arrondis. Ainsi (1−u)⁴Smax≤M≤(1+u)⁴Smax, et la mise à l'échelle exacte E=32uM donne 31uSmax<E<33uSmax. Comme |lh|<2Smax, chaque arrondi final de lh±E coûte moins de 3uSmax. La réserve restante dépasse 22uSmax :

$$\mathrm{RN}(lh-E)<L<\mathrm{RN}(lh+E)\qquad(S_{\max}>0).$$

Si Smax=0, L=lh=E=0. Les bornes finales sont donc utilisables par Jung et la corde ; elles ne nécessitent pas des additions dirigées.

## Jung : protéger chaque côté

La branche suppose lh<−E. Les divisions exactes par quatre donnent pl≤P≤pu<0, donc 2pu²≤2P²≤2pl². Les gardes jlo=RN(RN(J)(1−δ)) et jhi=RN(RN(J)(1+δ)) encadrent J≥0. Pour 1≤m≤5 :

$$(1+u)^m(1-\delta)<1,\qquad(1-u)^m(1+\delta)>1.$$

En effet (1+u)^m≤1/(1−mu), (1−u)^m≥1−mu, δ>mu et δ>mu(1+δ). Ces relations absorbent tous les facteurs du code :

| Expression | Majorant ou minorant utile |
| --- | --- |
| `lhs_min` | ≤2pu²(1+u)²(1−δ)≤2P² |
| `lhs_max` | ≥2pl²(1−u)²(1+δ)≥2P² |
| `b2` | Entre B²(1−u)³ et B²(1+u)³ |
| `rhs_max` | ≥jhi B²(1−u)⁵(1+δ)≥JB² |
| `rhs_min` | ≤jlo B²(1+u)⁵(1−δ)≤JB² |

Les cinq facteurs droits comptent les conversions, le carré et les deux produits suivants. Les décisions strictes sont sûres ; 2P²=JB² force le repli. B=0 conserve exactement les produits nuls.

## Corde : un signe strict conservé

Pour T=c μ̂ B, c∈{−4,−2,0,2,4}, deux conversions et deux multiplications donnent conservativement |t−T|≤4u|t|/(1−8u)<5u|t|. Le produit |t|δ est exact ; chaque addition définissant tmin/tmax coûte moins de 2u|t|. Si T≠0 :

$$t_{\min}\leq T-(\delta-7u)|t|<T<T+(\delta-7u)|t|\leq t_{\max}.$$

Si T=0, c=0 ou B=0 et les bornes sont exactement nulles. Avec l'encadrement affine précédent, `lh+E<tmin` implique L<T strictement, et `lh−E>=tmax` implique L≥T. Une égalité ne devient jamais un témoin intérieur ; l'indécidable utilise L−c μ̂ B en entier.

Les [bornes et raccords de la corde](PREUVE_CHORD_SECTOR_COURANTE.md) justifient ces entrées et le repli i128. Une borne E infinie force les replis des appels examinés. Le localisateur possède sa [preuve séparée](CELLULES_COURANT.md) ; [S1](S1_COURANT.md#6-théorème-géométrique-conditionnel-et-rle) compose ces implications, sans certifier une autre toolchain ou le GPU. Aucun nouveau run dans cette condensation.
