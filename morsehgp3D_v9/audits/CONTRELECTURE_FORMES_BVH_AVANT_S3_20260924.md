# Contrelecture mathématique des formes par nœuds avant S3

24 septembre 2026. Audit indépendant, sans modification du moteur ni essai G4.
Cadre : `exploration_v9_hors_registre`, entrée entière u18, générateur
q3/q4 exact, `public_status=not_claimed`. Sources lues dans le worktree
`build/v9-open-worktree` au commit `61cfba666` :
`src/gen/lanes/q34_dead_lanes.{cpp,hpp}`,
`src/gen/lanes/edge_cover.cpp`,
`src/gen/pipeline/q2_census.{cpp,hpp}`,
`src/gen/pipeline/prepared_cloud.cpp`,
`src/gen/pipeline/wspd_q34.cpp` et `src/gpu/certificate.hpp`.
Cette note contre-vérifie la [piste des nœuds avant cover](PISTE_B_Q34_NOEUDS_AVANT_COVER_20260923.md)
et la [contrelecture du cœur](CERTIFICAT_NOEUDS_CORE_LIDAR_20260923.md) ;
elle ne qualifie pas un port CPU ou CUDA.

## Verdict et preuve des 32 coins

Le certificat proposé est **exact comme condition suffisante**. Fixer une
arête `ab` de deux sites distincts, `D=|b-a|²`, les vecteurs entiers
`A,B` du prouveur, `Q=2²⁰`, `w=2z-a-b`, et une cellule de paramètres
fermée `C=[α₋,α₊]×[β₋,β₊]`. La forme entière effectivement évaluée est

`f_Q(z,α,β)=Q(|w|²-D)-2w·A α-2w·B β`.

Le centre est `c=(a+b+u₁A+u₂B)/2`, avec `α=Q u₁`, `β=Q u₂`.
L'orthogonalité de `A,B` à `b-a` donne
`f_Q=4Q(|z-c|²-|a-c|²)` : `f_Q<0` signifie intérieur strict,
`f_Q=0` contact. Pour `α,β` fixés, `f_Q` est une fonction **convexe de
`z`**, malgré son terme mixte `z·(αA+βB)`. Tout point d'une boîte
spatiale fermée `Z` est une combinaison convexe de ses huit coins ; sa
valeur est donc au plus le maximum de ces coins. Pour chaque coin spatial
fixé, la forme est **affine** en `α,β`, et son maximum sur `C` est à un
des quatre coins de `C`. Par conséquent

`max_(z∈Z, u∈C) f_Q(z,u) = max_(s∈corners(Z), t∈corners(C)) f_Q(s,t)`.

La convexité *conjointe* de `(z,u)` n'est pas requise. Si les 32 valeurs
sont toutes `strictement <0`, chaque vrai site de la plage du nœud est
intérieur pour **tous** les centres de la cellule. Les coins spatiaux
fictifs peuvent rendre le test pessimiste ; ils ne le rendent pas faux.
Pour un coin spatial donné, les quatre tests de centre se réduisent
exactement à **un** maximum de forme, comme dans `cell()` :

`k+x·(x<0 ? α₋ : α₊)+y·(y<0 ? β₋ : β₊)`,

où `k=Q(|w|²-D)`, `x=-2w·A`, `y=-2w·B`. Huit maxima spatiaux suffisent
donc arithmétiquement. Ce n'est pas un gain de temps mesuré.

## Domaine et identité des crédits

Les disques **fermés** du prouveur contiennent tous les centres possibles
d'une présentation q3 strictement aiguë ou q4 positive possédée par sa
plus longue arête `ab` : respectivement
`3|u₁A+u₂B|²≤D` et `2|u₁A+u₂B|²≤D`. La variance barycentrique donne
`R²≤D/3` (trois supports) et `R²≤3D/8` (quatre supports), donc
`|c-(a+b)/2|²≤D/12` et `≤D/8`. La plus petite valeur singulière de
la base `A,B` vaut au moins `h=max_i|b_i-a_i|≥sqrt(D/3)` : les deux
disques sont dans la racine `[-2,2]²` de `q34_dead_lanes`.
Une cellule qui touche le disque, **même seulement à sa frontière**, ne
peut être omise sans une borne d'exclusion stricte ; la condition actuelle
`disk_factor·min_norm > D Q²` est conservative. Si chaque cellule de ce
recouvrement détient au moins `T₃=K−1` sites strictement intérieurs, la
voie q3 est vide ; pour q4 le seuil est `T₄=K−2`. Un seuil nul ne ferme
aucune voie (`K<2` pour q3, `K<3` pour q4). Les deux masques sont décidés
séparément. Aucune profondeur ainsi acquise ne devient un census exact,
une coquille ou un crédit à additionner au filtre S2.

`PreparedCloud` refuse les coordonnées dupliquées et possède la copie
immuable ; `Q2CensusIndex` donne à chaque nœud une plage de rangs de sites
réels distincts, dont les enfants partitionnent celle du parent. Le crédit
`node.range.size()` est donc valide **si** les nœuds crédités d'une même
cellule forment une antichaîne de plages disjointes. Un parent crédité
sort de la frontière avant toute subdivision spatiale ; ses enfants ne
s'ajoutent jamais à lui. Au passage vers une sous-cellule de centres, son
crédit est hérité, et seuls les nœuds encore ambigus sont raffinés. Les
quatre sous-cellules sont des preuves alternatives sur des régions
différentes : leurs comptes ne s'additionnent pas. Les extrémités
`a,b` ont identiquement `f_Q=0`, donc aucun nœud qui les contient ne peut
être crédité par le test strict, même si sa boîte est lâche. Les retours
LiDAR fusionnés sur une même coordonnée restent un seul **site** pour le
seuil ; leur correspondance d'IDs d'origine relève du propriétaire.

Le test de maximum **ne dit rien sur l'extérieur** d'un nœud qui échoue.
Contre-exemple : `a=(0,0,0)`, `b=(4,0,0)`, centre médian `u=0`, boîte
`Z=[0,4]×[0,4]×{0}`. Tous les coins de `Z` ont `f_Q≥0`, mais le vrai
site `(2,0,0)` donne `f_Q=-16Q`. Ce nœud doit rester dans la frontière,
être subdivisé, ou être exclu par un **minorant** distinct. En particulier,
un simple minorant de profondeur `<T` ne permet pas l'arrêt anticipé
`center_inside` de l'ancien prouveur : les nœuds ambigus peuvent encore
contenir des intérieurs. Une preuve inachevée laisse le bit ouvert.

Une exclusion conservative de toute la cellule est possible avec
`r_i=αA_i+βB_i` et les intervalles exacts `w_i∈[w_i^-,w_i^+]`,
`r_i∈[r_i^-,r_i^+]` :

`LB=Q(Σ_i min_(w_i∈[w_i^-,w_i^+]) w_i²-D)
    -2Σ_i max_(w_i,r_i dans leurs intervalles)(w_i r_i)`.

Chaque véritable forme est `≥LB`, même si les intervalles oublient les
corrélations entre coordonnées. Ainsi `LB≥0` exclut tout intérieur
strict du nœud pour la cellule ; `LB<0` laisse le nœud ambigu.

Au **seul centre témoin** `(α,β)` situé dans le disque de la voie, un
minorant plus serré du minimum continu s'écrit exactement

`Q·min_(z∈Z) f_Q(z,α,β)
 = Σ_i dist(r_i,[Qw_i^-,Qw_i^+])² - |r|² - Q²D`.

La boîte étant axiale, les minima par coordonnée se séparent. Si ce
nombre est `≥0`, aucun vrai site du nœud n'est strictement intérieur au
centre témoin, égalité de coquille comprise. En partitionnant les nœuds,
la somme des populations encore *possiblement* intérieures est une borne
**supérieure** de profondeur, avec les populations déjà créditées incluses
une seule fois. Si cette somme vaut `<T`, ce centre réfute
le certificat de profondeur uniforme : le bit reste ouvert pour le chemin
exact. Elle ne démontre pas qu'une présentation q3/q4 existe. Le centre
témoin peut être un coin dyadique rationnel ; il doit appartenir au disque
fermé de la voie testée.

## Largeurs entières

Avec `M=2¹⁸−1`, chaque coordonnée de site et de coin de boîte est dans
`[0,M]`, `|w_i|≤2M`, `|A_i|,|B_i|≤M` et `|α|,|β|≤2Q`. Les coefficients
vérifient `|k|≤12QM²`, `|x|,|y|≤8M²`. Une évaluation de coin et ses
sommes partielles sont bornées par `44QM² =
3 170 509 948 459 155 456 < 2⁶²`, donc tiennent dans `i64` signé.
Pour les minorants, calculer en `i128` après promotion des opérandes.
En effet `|r_i|≤4QM`, `|Qw_i-r_i|≤6QM`, et la somme de valeurs absolues
dans la formule de distance est `<159Q²M²<2⁸⁴`, très en deçà de
`i128`. Ces bornes dépendent du profil u18 et de la racine actuelle ;
elles ne se transfèrent pas au float32 exact ni à un domaine de cellules
élargi. Pour atteindre un seuil `T≤9`, plafonner tout crédit à
`T−count` **avant** une conversion de `size_t` en `u32` ; le format
d'index GPU plat doit refuser explicitement les populations hors de sa
capacité. Aucun signe ne doit être décidé par flottant ou arrondi.

## Ce que cela remplace dans S3

Une preuve pré-cœur sur l'index **entier** peut éviter `make_diametral`,
`load()` et même le cover complet lorsqu'elle ferme **tous** les bits
actifs d'une arête. Elle peut compter un site situé hors du cœur
diamétral : cela reste un vrai intérieur au centre considéré, mais ne
reproduit plus la métrique historique `core_sites`. Si seul un bit ferme,
le S3 GPU/CPU actuel charge encore toutes les formes de son cœur partagé
pour le bit restant ; supprimer ces formes exige aussi un prouveur
paresseux pour les voies ouvertes. Si la preuve échoue, ou si une voie
reste ouverte, conserver le repli exact avec le cover **complet** pour
q3/q4, leurs coquilles d'IDs, le catalogue et la tour.

Le filtre ponctuel S2 certifie déjà les sites uniformément intérieurs à
**tout** le disque. Pour une voie réellement survivante de ce filtre
exact, une seule cellule englobant ce disque ne peut atteindre son seuil
par des sites individuellement uniformes ; ce serait le rejet S2 déjà
obtenu. Le ressort propre de cette proposition est la subdivision des
centres, avec des nœuds témoins pouvant changer selon la cellule, ou une
restriction du domaine de centres prouvée. Le pré-cœur reste **par
arête après expansion S2** : aucune baisse du nombre de paires, aucun
gain de chaîne ni borne sous-quadratique ne s'ensuit formellement.

Le code S3 GPU du snapshot matérialise dans `CertificateSlab` les plages,
les trois tableaux de formes et les frontières, puis compare ses comptes
avec ceux du prouveur CPU. Un port par nœuds change ce ledger : séparer
populations logiques du cœur/cover, formes réellement calculées, tests de
coins, bornes inférieures, copies de frontières, voies et arêtes fermées,
replis, RSS/HBM, CPU et mur. Juger la sortie complète et tous les IDs de
coquille, ainsi que les masques par arête, sur les mêmes entrées. Les boîtes
et plages aplaties transférées au GPU doivent être certifiées comme
englobant tous les sites d'un même propriétaire ; une boîte qui
omet un site ou deux plages créditées qui se recouvrent détruisent la
preuve. Ce texte n'est ni un port ni une mesure de temps S3/G4.
