# Front et témoins : preuve géométrique conservée

Sous positions u16 distinctes et opérations conformes, les crédits suivants sont des minorants d'intérieurs stricts. Ils complètent la [composition constructeur](../docs/PREUVE_HORIZONTALE_COMPOSITION.md#21-ce-que-la-wspd-permet-de-prouver-et-ce-quelle-ne-suffit-pas-à-prouver), qui cite cette preuve. La partition des piles et covers est démontrée dans l'[audit d'index](AUDIT_INDEX_20260905.md) ; leurs identités ne sont pas déduites d'une seule égalité de masse. `public_status=not_claimed`.

Pour un support positif de cardinal q, une arête maximale AB de longueur D et des poids barycentriques λᵢ>0 de somme un, l'identité de variance donne :

$$R^2=\frac{1}{2}\sum_{i,j}\lambda_i\lambda_j\lVert u_i-u_j\rVert^2\leq\frac{q-1}{2q}D^2.$$

Avec m=(A+B)/2, v=c−m est orthogonal à AB et R²=D²/4+|v|². Pour un site z, H=(z−A)·(B−z) et Ξ=|(B−A)×(z−A)|² donnent, avec t=3 en q3 et t=2 en q4 :

$$\lVert z-c\rVert^2-R^2=-H-2v\cdot(z-m)\leq-H+\sqrt{\frac{\Xi}{t}}.$$

Ainsi H>0 et tH²>Ξ impliquent l'intérieur strict pour toute boule de cette arité possédée par AB. En q2, H>0 décrit exactement l'intérieur diamétral. Un support pertinent vérifie p+q≤smax, donc p<hq=smax−q+1 : hq témoins distincts suffisent à l'exclure. Un point du shell ne peut être crédité.

À z et une extrémité fixés, les conditions H>0 et √t H>|u×w|, avec u=z−A et w=B−z, définissent un cône convexe ouvert dans l'autre extrémité. Vérifier les coins de A pour chaque coin de B étend successivement le prédicat à toutes les extrémités. Cette convexité séparée justifie les 64 coins, sans supposer une convexité conjointe.

Une boule ouverte centrée en m de rayon κq D appartient au fuseau, avec κ2=1/2, κ3=1/(2√3), κ4=sin(15°). À distance ρ de m, H=D²/4−ρ² et la composante perpendiculaire est au plus ρ ; résoudre ρ²+Dρ/√t−D²/4=0 donne ces constantes. Pour deux boîtes de rayons rA,rB et centres séparés de D0, deux rayons minorants de centre commun sont :

$$R_{\mathrm{dec}}=\kappa_q(D_0-r_A-r_B)-\frac{r_A+r_B}{2},\qquad R_{\mathrm{coup}}=\kappa_qD_0-\sqrt{\frac{4\kappa_q^2+1}{2}(r_A^2+r_B^2)}.$$

Le premier suit de l'inégalité triangulaire. Pour le second, Cauchy–Schwarz et le parallélogramme donnent κq|a−b|+|a+b|/2≤√((2κq²+1/2)(|a|²+|b|²)). Le maximum des minorants est sûr puisque leur centre est commun. Les distances et coefficients positifs sont arrondis par défaut, les rayons de boîtes et le terme soustrait par excès ; les valeurs non positives ne créditent rien. Leur réalisation entière est justifiée dans l'[arithmétique des témoins](ARITHMETIQUE_SPINDLE_COURANTE.md).

`hmin_boxes` est un minimum exact : l'expression est séparable par axe, affine aux extrémités et concave en z, donc minimale aux coins. `hmax4_boxes` choisit, par axe, des extrémités minimisant le maximum en z. Une borne non positive exclut un témoin universel de toutes les ancres du rectangle ; elle n'exclut pas les témoins de chacune des autres ancres individuellement.

Les crédits de sous-arbre retirent A∪B, puis perdent leur bit de lane avant toute descente. Ils sont donc disjoints. Les histogrammes d'extrémités portent sur A sans a et B sans b : leurs populations sont disjointes entre elles et du cœur extérieur. Leur somme est sûre ; les certificats aval partageant des témoins se combinent par OU. Le test WSPD entier impose Dcentres≥(s+2) max(rA,rB), donc séparation ≥s max(rA,rB), sans fournir une borne de coût industriel.

Les [certificats d’histogrammes par blocs](receipts_block_histograms_20260906/README.md) sont désormais repris par le constructeur. La [preuve du terminal en un passage](receipts_terminal_count_20260906/README.md) ajoute l’indépendance des comptes vis-à-vis du masque, nécessaire en plus de leur domination. L’égalité positive de cœur q2 demandée dans le gate permanent est maintenant ajoutée et contre-lue, en complément du différentiel clos. Aucun gain de temps du nouveau terminal n’est acquis par cette preuve.

La [sélection par suppressions stables](receipts_phase_selection_20260906/README.md) réalise la borne en taille d’entrée et de sortie sans trier l’émission par crédit. Ses contre-fixtures distinguent le cap global need d’un cap local réutilisé à tort, et le crédit écrêté de la population positive d’un bloc. Le coût des histogrammes reste séparé de celui de cette sélection.

Les [hashes de lecture](receipts_20260904/front_sources.json) identifient les expressions examinées. [S1](S1_COURANT.md) compose ce résultat avec secteurs, corde, cellules et filtres ; cette condensation n'ajoute ni exécution ni qualification.
