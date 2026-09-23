# Contre-audit B — domination q4 par gardes de blocs

23 septembre 2026. Lecture indépendante de
[`DOMINATION_Q4_PARESSEUSE_PAR_BLOCS_20260923.md`](DOMINATION_Q4_PARESSEUSE_PAR_BLOCS_20260923.md)
au commit `573c6869`. Le certificat `(⋆)` par coins de cellule est **sûr** ;
ni port moteur ni gain LiDAR ne sont qualifiés ici.

## Préservation exacte

Pour chaque paire fixe de sites `g∈G,z∈Z`, la différence
`|z−c|²−|g−c|²` est affine en `c`. Si la borne par boîtes est strictement
positive à chaque sommet d'une cellule convexe fermée, elle l'est pour
tous les centres de cette cellule et tous les sites de G et Z. Des nœuds
gardes disjoints fournissant `Kmax−2` sites excluent Z de chaque support,
intérieur et contact d'une présentation q4 survivante centrée dans la
cellule. Un contact ne peut pas être écarté sur une inégalité large. Le
seuil q4 ne se réutilise **pas** pour l'atlas partagé q3 ; la fixture K5
de la note A le démontre. La boîte des centres q4 positifs de l'arête
propriétaire est aussi un surensemble sûr via
`|c−(a+b)/2|²≤|a−b|²/8`. Le nombre de cellules et le travail de recherche
des gardes restent entièrement ouverts.

## Correction : deux tests suffisants incomparables

La note A qualifie `(⋆)` de « strictement plus fort » que le test ancien
`gap²(C,B_Z)>max_{g∈G,c∈C}|g−c|²`. C'est faux lorsque `B_G` contient
des coins sans site : le maximum sur la **boîte des gardes** peut dépasser
celui sur les gardes réelles. Contre-exemple entier à `Kmax=5` :

`C={(0,0,0)}`, `G={(1,0,0),(0,1,0),(0,0,1)}` et
`Z={(1,1,0)}`. Les trois gardes sont distinctes et disjointes de Z.
L'ancien majorant est `U_C=1`, le gap² est `2` : ancien rejet sûr.
Pour `(⋆)`, `B_G=[0,1]^3` a un coin fictif `(1,1,1)` à distance² `3`,
alors que `m_Z=2` : le nouveau test reste indécis. La fixture de la note
A montre la direction inverse. Les tests sont donc **incomparables** :
garder leur disjonction logique si le coût des deux est justifié, sans
attribuer une domination uniforme au nouveau test. Cette correction ne
réfute ni sa sûreté, ni l'économie possible sur les régimes LiDAR.

Une ablation honnête mesure séparément gardes cherchées, coins testés,
nœuds Z rejetés par ancien/seul nouveau/union, cellules et incidences
`E×C`, formes vraiment évitées, temps q3/q4/FULL et sorties identiques.
Les 1 200 boîtes de l'oracle A ne mesurent pas cette fréquence sur
SemanticKITTI.
