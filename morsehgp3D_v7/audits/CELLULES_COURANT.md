# Cellules : comptage strict et surcouverture

Cette preuve ferme le certificat entier, les coordonnées de seed et la marge du localisateur sous le [domaine CPU](DOMAINE_CPU_COURANT.md). Le rejet d'ancre `all_dead` ne dépend pas du localisateur flottant. Les [sources géométriques épinglées](receipts_20260904/cell_sources.json) et le [certificat de largeurs](receipts_front_20260905/cell_width.json) gardent leurs autorités propres ; `public_status=not_claimed`.

## Compter sur chaque cellule fermée

Pour une ancre AB de milieu m, longueur D et centre c=m+p dans son plan bissecteur, le rayon carré est D²/4+|p|². Avec w′=2z−A−B :

$$\lVert z-c\rVert^2-R^2<0\quad\Longleftrightarrow\quad4w'\cdot p>\lVert w'\rVert^2-D^2.$$

Aux sommets p=(i′u+j′v)/G, cela devient `4*i′*du+4*j′*dv>rhs`. Cette condition affine, stricte aux quatre sommets, l'est sur toute la cellule fermée. Pour du>0, les témoins d'une ligne forment un suffixe ; le maximum des débuts de deux lignes donne leurs cellules communes. Pour du<0, ce sont des préfixes dont le minimum fixe la borne stricte ; du=0 donne une ligne entière ou vide. Les tableaux de différences encodent exactement ces intervalles.

Un site contribue au plus une fois par cellule ; cover sans doublon et exclusions A/B donnent des identités distinctes. Les comptes de cellules ne s'additionnent pas : chacune doit atteindre hq=smax−q+1. Aucun crédit d'extrémité ne s'y ajoute. Une boule dont le centre appartient à une cellule morte possède alors trop d'intérieurs stricts pour être pertinente.

La [base sectorielle](PREUVE_CHORD_SECTOR_COURANTE.md) contient le disque des centres de rayon carré D²/12 ou D²/8 dans son losange |α|+|β|≤1. `cell_needed` calcule exactement si une cellule fermée rencontre ce losange. Les cellules nécessaires couvrent donc tous les centres admissibles, frontière comprise. Toutes mortes implique l'impossibilité de tout centre, sans localisation flottante.

## Entrées entières et centres réellement localisés

La base atteint A=B=1, donc |uᵢ|,|vᵢ|≤M=65535. Pour G∈{8,16}, du/dv≤6M² en module, |rhs|≤192M², `mag=4G(|du|+|dv|)≤768M²` ; `rhs−4(j−G)dv` reste sous 2^47. Le chemin i64 est sûr. Même hors géométrie, ses gardes |rhs|,mag<2^62 rendent la différence strictement inférieure à 2^63. Gram est promu en i128. Les compteurs sont des sous-ensembles disjoints du cover, de taille n<2^30 ; les indices restent dans les tableaux de G≤16. La politique `acute_seeds*ratio`, ratio≤8, tient en size_t64 et ne certifie aucune mort.

Avec le vrai Gram G3>0 du seed et N=W−G3(B−A), le centre relatif est N/(2G3). Les bornes q3 donnent |Nᵢ|≤45M⁵ et |N·u|,|N·v|≤135M⁶<2^104. Les centres q4 sont (N+μn)/(2G3), avec 2μ²≤J=D²(3G3−2AX²BX²). La valeur μ̂=floor(sqrt(floor(J/2)))+1 est strictement extérieure à √(J/2), même sur carré exact. J≥0 et J≤81M⁶ donnent μ̂<2^51 ; les coordonnées d'extrémité restent sous 2^105 en i128. Une boîte contenant les deux extrémités contient toute la corde.

## Erreur du localisateur, bornes finales incluses

Hypothèses : RN binaire64, conversions conformes, séquence sans réassociation et environnement stable. Poser e=2⁻⁵³. Les entrées sont i128, den et le déterminant entier Δ sont positifs, G≤16. Produits avant division <2^257, échelle positive >2⁻²⁵⁵ et ≤16, résultats suivants <2^263. Les différences de produits d'entiers arrondis valent zéro ou ont module au moins un : les coordonnées non nulles restent normales. Le terme absolu 2⁻⁴⁰ protège les bornes près de zéro ; aucun bon conditionnement de Gram n'est requis.

Définir T1=pu(v·v), T2=pv(u·v), T3=pv(u·u), T4=pu(u·v), s=G/(den Δ)>0 et S=sΣ|Tj|. Les coordonnées exactes sont a=(T1−T2)s et b=(T3−T4)s. Δ est calculé en entier avant conversion.

Deux conversions et un produit donnent |T̂j−Tj|≤4e|Tj|. La soustraction suivante coûte au total ≤6e(|T1|+|T2|). Conversions, produit du dénominateur et division donnent |ŝ/s−1|≤5e ; le produit final porte donc l'erreur de chaque coordonnée à ≤16eS. Les termes d'ordre e² restent dans ces marges.

Pour l'epsilon calculé, les opérations positives donnent maĝ≥(1−e)⁶Σ|Tj| et ŝ≥(1−e)⁴s. Le produit, la mise à l'échelle exacte 2⁻⁴⁶ et l'addition finale impliquent :

$$\widehat\epsilon\geq128e(1-e)^{12}S+8192e(1-e)>120eS+8000e.$$

Il reste à compter RN(â±ε̂). Comme |â|≤(1+16e)S, la réserve de chaque côté reste strictement positive :

$$(1-e)\widehat\epsilon-17eS-16e^2S>(103e-136e^2)S+8000e(1-e)>0.$$

Ainsi `floor(lo)..floor(hi)` contient toutes les cellules fermées incidentes, des deux côtés d'une coordonnée entière exacte. Pour une corde, le maximum des deux epsilons protège chaque extrémité ; min/max et RN sont monotones. Les cellules extérieures au losange sont sans centre admissible, mais une cellule intérieure incidente à sa frontière doit toujours être consultée.

Les gardes `range_in_domain` imposent des intervalles ordonnés dans [−4G,4G] avant casts ; NaN et infinis échouent. Les conversions et indices restent donc bornés par 64. Une politique refusée, un environnement incompatible ou une base échouée laisse `built=false` et conserve l'ancre.

## Raccord exécuté conservé

Le [pont réel](cell_compiled_bridge.cpp) et le [juge rationnel](cell_compiled_oracle.py) comparent distances aux sommets, centres par élimination de Gram et cordes depuis les rayons. Les [reçus O2/UBSan](receipts_front_compiled_20260905/cell/summary.json) portent 32 grilles, 38 400 cellules, 32 centres et cordes, 96 refus d'environnement et 192 refus de domaine par build. Les trois mutants produit de stricteté, seuil et epsilon sont détectés ; le [rejeu renforcé](replay_compiled_front.py) exige de vrais changements de mort et les deux cellules au contact entier. Les cas i128 synthétiques sont hors profil géométrique et annoncés comme tels. Les preuves de domaine ne sont pas déduites de ces seuls tests. Aucun nouveau run dans cette condensation.
