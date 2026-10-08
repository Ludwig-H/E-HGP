# Ce qu'établit le refus `wide_leaf`

Précision de **CST-0237**, sans nouvelle implantation ni fermeture du constat.
Sources L2 **a2c2fccfd**, identiques à **83ed7620d** sur les douze fichiers
épinglés dans [capture.json](capture.json). La [session L2](../session_l2_admission/README.md)
publie `unsupported_degeneracy/wide_leaf` pour ETH3D courtyard, 16 828 368 sites,
CPU/u21/K5/W48 ; aucune passe FULL n'est publiée. Ce reçu ne relit pas les points
et ne déduit ni localisation, ni densité, ni cause liée au scanner.

| Grandeur | Domaine dans ces sources |
|---|---|
| Feuille de `GlobalIndex` | plage de sites Morton, taille par défaut **8** (`index.hpp:18`) |
| Feuille du parcours C | **boîte de centres possibles + liste de candidats filtrée** ; seuil souhaité **24**, plafond **256** |
| Coquille U d'une boule | tous ses sites de contact ; plafond d'émission **64**, refus distinct `shell_capacity` |
| Support canonique S* | **2 à 4** sites pour une boule positive ; ce n'est pas la coquille |

Les valeurs 24, 32/256 du warp, 64 de la coquille et l'étendue du repère
arithmétique sont des notions différentes. `wide_leaf` n'est pas un refus
d'arithmétique large et ne signifie pas qu'une coquille contient 257 points.

**Chaîne causale exacte.** `traversal_kernels.hpp:348` marque une boîte non vide
comme feuille si `count <= leaf_size` **ou** si sa largeur maximale est au plus
1. `traversal_scan.hpp:13` ne compte dans `max_leaf` que les enfants ainsi
marqués. L'unique émission produit de `Reason::wide_leaf` est
`traversal_driver.hpp:173`, lorsque ce maximum dépasse `max_leaf`.
`full_probe.cpp` laisse `max_leaf=256` et le plan L2 conserve `leaf_size=24`.
Sous ces paramètres et cette source, le refus atteste donc **au moins une liste
survivante de 257 candidats ou plus, dans une boîte de centres de largeur au
plus une unité de grille par axe**.

Ici les boîtes sont demi-ouvertes à bornes entières ; le contrôle `lo>=hi`
(`traversal_kernels.hpp:337–344`) élimine les boîtes vides. Les trois largeurs
survivantes sont donc strictement positives et, dans ce cas précis, égales à 1.
Ce raisonnement ne vaut pas pour un intervalle fermé générique de largeur nulle.
La liste candidate n'est pas l'ensemble des points physiquement situés dans
cette petite boîte : elle accompagne une région de **centres** et peut conserver
des sites éloignés. Le refus ne révèle ni son cardinal exact, ni le nombre de
boîtes concernées, ni une multiplicité de contact ou une borne de temps.

Le contrôle précède `traversal_emit` et la consommation des feuilles de ce
niveau. Des niveaux antérieurs peuvent avoir calculé des temporaires. Mais
`catalogue.cpp:46` propage le refus avant assemblage/publication ; aucun
catalogue partiel ne sort. Dans `full_probe.cpp`, C précède G puis TMVR : ces
derniers ne sont pas atteints pour cette prise. C'est une limite déclarée de
complétude du domaine calculable, pas une sortie géométrique incorrecte admise.

**Pourquoi découper les candidats ne suffit pas.** Le carré exact
`A=(1,1,1), B=(1,3,1), C=(3,1,1), D=(3,3,1)` possède la boule centrale
`c=(2,2,1), r²=2`, avec `p=0`, `U={A,B,C,D}`, `qmin=2`, `S*=AD` par positions
lexicographiques. Elle appartient à Cat₁. Découper les candidats en les colonnes
`{A,B}` et `{C,D}`, calculer chaque catalogue puis réunir les sorties perd cette
boule : ses deux supports minimaux, AD et BC, traversent les blocs. Tronquer le
census ou canoniser par bloc peut aussi fausser p, U et S*.
[check.py](check.py) vérifie ce contre-exemple avec `Fraction`, sans moteur.

Subdiviser **l'espace des centres** peut préserver Cat_K si chaque enfant garde
une liste K-certifiée complète, une couverture sans trou, une propriété unique
des frontières et des prédicats exacts. C'est différent de partager les points.
La subdivision entière actuelle est déjà épuisée à largeur 1 : supprimer
simplement l'arrêt ne crée pas deux enfants strictement plus petits. Passer à
des coupes rationnelles changerait les domaines numériques et la preuve de
profondeur ; surtout, cela ne garantit pas une liste sous 256. À un centre commun,
des sites cosphériques sont à égalité de distance : aucun ne peut être éliminé
par une dominance **stricte sur toute boîte contenant ce centre**.

Une future voie T1-c peut partager le travail sur les supports de 2–4 sites,
mais doit couvrir aussi les supports traversant les blocs, recenser sur toute
la liste K-certifiée (ou l'index global), et conserver la canonisation globale,
la propriété d'émission et l'admission transactionnelle. La certification K
explique pourquoi le census local complet suffit : pour une boule admissible,
`p <= K−1`, donc aucun site intérieur ou de coquille ne peut avoir K sites
strictement plus proches au centre. Leur suppression par G1 serait impossible.
Ce lemme ne justifie aucune troncature supplémentaire. Ni coût amélioré ni borne
globale ne sont promis ; lever `wide_leaf` ne lève pas le plafond indépendant
de coquille de 64, ni les budgets d'émission et de la tour.

Rejeu léger des pins et du carré, normal/−O identiques :

```sh
python3 -B check.py --repo /workspaces/E-HGP
python3 -B -O check.py --repo /workspaces/E-HGP
```

Aucun natif, banc, contrôleur ou accès distant. Les qualifications antérieures
ne sont ni rejouées ni transférées à une voie large future.
