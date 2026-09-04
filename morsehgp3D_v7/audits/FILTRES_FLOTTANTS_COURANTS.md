# Filtres affine, Jung et corde : bornes explicites courantes

4 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

**Verdict conditionnel.** Les marges utilisées par les trois filtres suffisent dans le domaine binaire64, arrondi au plus proche, conversions correctement arrondies, FMA correctement arrondie et séquence sans réassociation. La réserve affine rend même `RN(lh ± E)` extérieure à la valeur exacte : il n'est pas nécessaire de supposer ces additions dirigées. Ce résultat est une preuve des expressions et de leurs appels examinés, pas une qualification de tout compilateur, ABI ou GPU. Aucun nouveau run n'a été effectué.

Les sources sont [float_filter.hpp](../src/pipeline/float_filter.hpp), [AffineSeed et ses appels](../src/pipeline/generate.hpp#L741), et [ChordPieces::certified_sign](../src/lanes/chord_kill.hpp#L62). Les [hashes épinglés](receipts_20260904/float_sources.json) identifient cette lecture.

## 1. Modèle et domaine

Posons $u=2^{-53}$ et $\delta=2^{-40}=8192u$. RN désigne l'arrondi binaire64 au plus proche. Pour une opération ou conversion concernée, sans débordement ni résultat sous-normal, $\mathrm{RN}(x)=x(1+\theta)$ avec $\lvert\theta\rvert\leq u$. Les zéros exacts sont traités exactement.

Les coordonnées affines $v_i=2z_i-a_i-b_i$ et $q=\lVert v\rVert^2-D^2$, ainsi que leurs maxima absolus, sont des entiers exactement représentables en binaire64. G et les Ni sont convertis une fois par seed. G est strictement positif. Les grandeurs qmax et vmax majorent les valeurs absolues concernées : elles sont les maxima effectivement calculés sur tous les sites auxquels ce seed applique le filtre, [generate.hpp:568](../src/pipeline/generate.hpp#L568).

Dans ce domaine u16, les coefficients entiers convertis restent des entiers, même lorsqu'ils sont arrondis. Les valeurs `lh` et M ci-dessous sont donc des entiers binaire64; E est un multiple de $2^{-48}$, et les extrémités P sont des multiples de $2^{-50}$. Les produits avec les gardes restent des multiples de $2^{-140}$. Les bornes u16 des expressions donnent des magnitudes inférieures à $2^{230}$. Ces marges excluent les débordements et sous-normaux des expressions étudiées. Le cas Smax=0 est trivial : L=lh=E=0.

## 2. Erreur affine et réserve effective de E

Écrivons g et ni pour les valeurs binaire64 obtenues par conversion de G et Ni, puis :

$$L=Gq-2\sum_iN_iv_i,\qquad\widehat S=\lvert gq\rvert+2\sum_i\lvert n_iv_i\rvert,\qquad S_{\max}=gq_{\max}+2\sum_i\lvert n_i\rvert v_{\max}.$$

La conversion d'un coefficient a vérifie $\lvert a-\widehat a\rvert\leq u\lvert\widehat a\rvert/(1-u)$. Son apport à l'erreur de L est donc au plus $u\widehat S/(1-u)$.

La séquence de [float_filter.hpp:43](../src/pipeline/float_filter.hpp#L43) contient une multiplication initiale, deux FMA pour la somme et une FMA finale. `t+t` est un changement d'échelle exact. Développer les quatre facteurs d'arrondi donne une erreur d'évaluation au plus $\gamma_4\widehat S$, où $\gamma_4=4u/(1-4u)$. Les conversions et l'évaluation réunies donnent :

$$\lvert lh-L\rvert\leq\left(\frac{4u}{1-4u}+\frac{u}{1-u}\right)\widehat S<6u\widehat S\leq6uS_{\max}.$$

L'inégalité stricte suit par multiplication des dénominateurs positifs de $1-22u+24u^2>0$; elle ne repose pas sur le commentaire approximatif « 8u ».

Soit M la valeur calculée par la FMA dans [affine_l_bound](../src/pipeline/float_filter.hpp#L47). Tous ses termes sont positifs. Chaque contribution traverse au plus deux additions d'absolus, une multiplication par vmax et la FMA finale; la multiplication par deux est exacte. Donc :

$$(1-u)^4S_{\max}\leq M\leq(1+u)^4S_{\max},\qquad E=32uM,\qquad31uS_{\max}<E<33uS_{\max}.$$

La mise à l'échelle $2^{-48}=32u$ est exacte dans le domaine. Par ailleurs $\lvert lh\rvert\leq(1+\gamma_4)\widehat S<2S_{\max}$. L'arrondi de chacune des deux additions `lh ± E` a donc une erreur absolue au plus $u(\lvert lh\rvert+E)<3uS_{\max}$. Il reste au moins la marge suivante :

$$E-\lvert lh-L\rvert-u(\lvert lh\rvert+E)>22uS_{\max}>0.$$

Ainsi $\mathrm{RN}(lh-E)<L<\mathrm{RN}(lh+E)$ lorsque Smax>0. Les décisions `lh < -E` et `lh > E` sont sûres, et les deux extrémités arrondies restent des bornes utilisables par Jung et la corde. Ce dernier point est plus fort que la seule borne de signe affine.

## 3. Jung : facteurs de chaque côté

Le site entre dans cette branche avec `lh < -E`. Les divisions par quatre sont exactes, donc `pl` et `pu` calculés à [float_filter.hpp:58](../src/pipeline/float_filter.hpp#L58) satisfont $pl\leq P\leq pu<0$. Par conséquent $2pu^2\leq2P^2\leq2pl^2$.

La construction d'appel $J_d=\mathrm{RN}(J)$, puis $jlo=\mathrm{RN}(J_d(1-\delta))$ et $jhi=\mathrm{RN}(J_d(1+\delta))$, [generate.hpp:936](../src/pipeline/generate.hpp#L936), donne :

$$jlo\leq J(1+u)^2(1-\delta)\leq J,\qquad jhi\geq J(1-u)^2(1+\delta)\geq J.$$

Pour $1\leq m\leq5$, les deux facteurs utiles vérifient exactement :

$$(1+u)^m(1-\delta)<1,\qquad(1-u)^m(1+\delta)>1.$$

Pour la première inégalité, $(1+u)^m\leq1/(1-mu)$ et $\delta>mu$. Pour la seconde, Bernoulli donne $(1-u)^m\geq1-mu$, puis $\delta>mu(1+\delta)$. Ces deux conditions sont largement satisfaites par δ=8192u et m≤5.

La table compte les arrondis dans les expressions réellement utilisées. Les multiplications par deux sont exactes; `1 ± δ` est exactement représentable.

| Expression | Encadrement avant emploi de la marge |
|---|---|
| `lhs_min` | Au plus $2pu^2(1+u)^2(1-\delta)$, donc au plus $2P^2$ |
| `lhs_max` | Au moins $2pl^2(1-u)^2(1+\delta)$, donc au moins $2P^2$ |
| `b2` | Entre $B^2(1-u)^3$ et $B^2(1+u)^3$ : conversion de B deux fois dans le carré, puis multiplication |
| `rhs_max` | Au moins $jhi B^2(1-u)^5(1+\delta)$, donc au moins $JB^2$ |
| `rhs_min` | Au plus $jlo B^2(1+u)^5(1-\delta)$, donc au plus $JB^2$ |

Les deux facteurs supplémentaires du côté droit sont la multiplication par la garde et la multiplication par jlo/jhi. Ces encadrements prouvent directement les retours +1 et −1 de `jung_interval_sign`. Si $2P^2=JB^2$, aucun des deux tests stricts ne peut réussir : l'égalité passe par le repli exact. Si B=0, les produits droits sont exactement nuls et le même raisonnement reste valable.

## 4. Produit et comparaisons de la corde

Posons $T=c\widehat\mu B$, avec $c\in\lbrace-4,-2,0,2,4\rbrace$. Dans [chord_kill.hpp:63](../src/lanes/chord_kill.hpp#L63), les deux conversions et les deux multiplications donnent conservativement $\lvert t/T-1\rvert\leq\gamma_4$. En réalité la multiplication par c est exacte pour ces valeurs, mais cette amélioration est inutile. Pour T non nul :

$$\lvert t-T\rvert\leq\frac{\gamma_4}{1-\gamma_4}\lvert t\rvert=\frac{4u}{1-8u}\lvert t\rvert<5u\lvert t\rvert.$$

Le produit $\lvert t\rvert\delta$ est exact, car δ est une puissance de deux. Chaque addition formant tmin/tmax a une erreur inférieure à $2u\lvert t\rvert$. On obtient donc :

$$tmin<T-(\delta-7u)\lvert t\rvert<T<tmax.$$

Plus précisément, les inégalités séparées utiles sont $tmin\leq T-(\delta-7u)\lvert t\rvert$ et $tmax\geq T+(\delta-7u)\lvert t\rvert$. Lorsque T=0, c=0 ou B=0 dans les appels considérés et les trois valeurs T,tmin,tmax sont exactement nulles.

Le § 2 donne déjà $\mathrm{RN}(lh-E)\leq L\leq\mathrm{RN}(lh+E)$. Ainsi `lh+E < tmin` implique strictement L<T, et `lh-E >= tmax` implique L≥T. Le résultat négatif est donc un témoin strict; une égalité exacte ne peut devenir un tel témoin. Le cas indécidable utilise la comparaison entière existante.

## 5. Portée exacte de cette fermeture

Les marges des expressions examinées sont justifiées avec leurs facteurs explicites. Elles ferment les décisions affine/Jung/corde **sous le domaine d'exécution annoncé**. `__FAST_MATH__` et le mode d'arrondi observé servent à désactiver le filtre dans [float_filter_runtime_enabled](../src/pipeline/float_filter.hpp#L70); une borne infinie entraîne alors les replis exacts des appels examinés. Cette garde ne prouve pas à elle seule toute conformité d'un compilateur à la séquence ou aux conversions demandées.

La [preuve des cellules](CELLULES_COURANT.md) établit séparément la surcouverture de leur localisateur et la marge de ses bornes finales, sous le même type de contrat binaire64. La [composition S1](S1_COURANT.md#6-théorème-géométrique-conditionnel-et-rle) réunit ces clauses en un théorème géométrique conditionnel. La qualification effective du domaine d'exécution et des primitives reste distincte de ces preuves. Aucun reçu d'une autre lignée, résultat GPU ou essai de compilation supplémentaire n'est hérité. **GCP non utilisé.**
