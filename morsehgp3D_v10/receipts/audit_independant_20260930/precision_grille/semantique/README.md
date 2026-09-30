# Précision de grille : stabilité géométrique et unités

30 septembre 2026. Petit reçu mathématique indépendant. Aucun moteur natif
ou oracle HGP importé, aucune campagne, aucun usage GCP. `check.py` utilise
uniquement `Fraction` et des graphes Γ exhaustifs colinéaires de trois ou
quatre observations. Les sorties normal et `-O` sont capturées séparément ;
`receipt.json` et `SHA256SUMS` ferment les sources et les sorties.

## Précondition : conserver l'univers étiqueté

Écrire X=(x_i) et Y=(y_i) sur les **mêmes n identifiants**, avec
`||x_i−y_i||≤ε`. Un arrondi au plus proche sur une grille isotrope de pas h,
sans écrêtage, donne `ε=√3 h/2`, dans une unité physique commune. L'origine,
le pas et la règle d'arrondi doivent être publiés.

La preuve compte les observations étiquetées. Des collisions peuvent être
agrégées en sites avec leurs multiplicités et tous les IDs, car cela garde
le même multiensemble mathématique. Les supprimer en gardant un site de
poids 1 change l'objet : ce n'est plus cette comparaison. La tour native
actuelle refuse les multiplicités ; aucune qualification pondérée du
produit ne découle de ce reçu.

## Preuve générale en rayon

Pour une partie étiquetée F, poser
`ρ_X(F)=min_c max_{i∈F} ||c−x_i||`, le rayon de sa MEB. La MEB de X
contient les points déplacés dans son rayon augmenté de ε. En échangeant
X et Y, on obtient `|ρ_X(F)−ρ_Y(F)|≤ε`. Cela vaut pour toutes les parties,
pas seulement les supports restés critiques.

Γ_k(r) a pour sommets les k-parties de rayon MEB au plus r et pour liens
les (k+1)-parties présentes. L'identité des parties donne
`Γ_k^X(r)⊆Γ_k^Y(r+ε)` et l'inclusion inverse avec le même décalage.
Les composés sont les inclusions au rayon `r+2ε`. Les images de composantes
sont naturelles en rayon et en k : prendre une face (k−1)-partie commute
avec le transport, et les autres faces sont dans la même composante.
Par le modèle Γ, le foncteur FULL π0 possède donc un **ε-interleaving en
rayon**. On peut aussi le lire directement sur les inclusions de sous-niveaux
`L_k^X(r)⊆L_k^Y(r+ε)` obtenues par l'inégalité triangulaire.

Cette borne ne fournit **aucun appariement inchangé des supports critiques**,
des coquilles I/U, des nombres de naissances, des plateaux ou des projections
dures sur les points. Une petite branche peut apparaître ou disparaître.
Les nouveaux cas de contact et de continuation des sections 10–11 de
[l'audit frontière](../../../../audits/audit_continu_20260929/AUDIT_LAMINARITE_POINTS_20260929.md)
restent des réserves nécessaires pour les masses et la projection.

La relation complète de couverture admet néanmoins un transport théorique :
une observation i couvre une composante C à r si et seulement s'il existe
une k-partie F contenant i, active à r, dans cette composante de Γ_k.
La direction directe choisit F dans la boule de rayon r centrée en un point
de C couvrant x_i ; le centre MEB rejoint ce point dans l'intersection
convexe des boules centrées aux sites de F. La réciproque utilise ce centre
MEB comme témoin. F reste actif à `r+ε` après déplacement. C'est une relation
à valeurs multiples transportée vers les images des composantes, pas une
stabilité des incidences aux boules du catalogue ni du choix d'un propriétaire.

En niveau carré β, le décalage est `(√β+ε)²`. Pour une partie dont le rayon
initial est au plus R, `|β_Y−β_X|≤2Rε+ε²`. **h² n'est pas une erreur uniforme
sur β**. Pour l'entrée core au site déplacé lui-même, les distances entre
deux observations changent d'au plus `2ε`, d'où une borne `2ε` sur cette
date d'entrée en rayon ; ce n'est pas la date de première couverture.

## Unités et EOM

Sur une même géométrie de grille, `r_phys=h r_grid` et
`β_phys=h² β_grid`. À z fixé,
`λ_phys=r_phys^(-z)=h^(-z) λ_grid`. Une stabilité
`Σ m_i(λ_sortie−λ_naissance)` est donc multipliée par le même facteur
`h^(-z)`. Les comparaisons EOM exactes restent identiques si l'arbre
condensé, les masses, z, la convention de zéro et les autres seuils sont
inchangés et si les unités sont transformées ensemble.

Ce résultat concerne un changement d'unité d'un **même arbre**. Modifier h
et arrondir de nouveau les données peut changer arbres, masses, contacts et
condensation. Il n'existe ici aucune garantie EOM ou statistique sous cette
modification. Une convention de plancher en rayon doit également préciser
si elle est fixée dans l'unité physique ou dans l'unité de grille.

La garde numérique R2 `M×λ_max<2^1000`, stricte, dépend de l'unité : après
conversion elle devient `M h^(-z) λ_max_grid<2^1000`. À z=2, le script
montre deux unités usuelles acceptées et h=`2^-498` refusé par ce seul
plafond, bien que la comparaison EOM rationnelle reste inchangée. Cela ne
juge pas le domaine binary64 complet : normalité, finitude, arrondis et
marges EOM doivent être contrôlés séparément. Une normalisation commune
déclarée peut conserver les comparaisons mathématiques ; elle ne doit pas
servir à revendiquer des valeurs physiques non représentables.

## Portée des petits contrôles

Le script vérifie les rayons de toutes les parties, les inclusions et images
de composantes Γ, les composés du transport et la naturalité verticale sur
deux fixtures colinéaires, dont une collision qui conserve les copies. Il
montre qu'une déduplication sans poids fait disparaître l'univers K3.
Un exemple 3D atteint exactement l'erreur carrée `3h²/4`. Trois valeurs de h
vérifient les unités β, λ et le facteur commun des stabilités à z=2, avec
une comparaison EOM favorable au parent et une autre favorable aux enfants.
Ces calculs bornés illustrent les preuves ; ils ne qualifient ni un nouveau
profil d'entrée, ni la tête, ni les points frontière, ni les performances.
