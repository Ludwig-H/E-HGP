# Précision physique : B, pas de grille et identité des retours

Source publiée `9d639e146`, copies Git figées ; le manifeste LiDAR vient du
reçu leaf16 clos. Calculs rationnels autonomes normal/−O, sans import de
produit, donnée LiDAR, build, natif ou GCP. Aucun profil plus fin n'est
qualifié ici. Le défaut u21 élargit la capacité ; ARCHITECTURE dit déjà
explicitement que B ne change ni h ni les coordonnées.

## Comparaison des profils et vrai changement de précision

Le nouveau banc 18/21/24 utilise exactement les mêmes coordonnées entières,
IDs, masque et pas 1 mm. C'est une comparaison de représentations et de coûts
arithmétiques. Elle ne mesure aucun effet géométrique d'une grille plus fine.
Le format normalisé du banc garde ces coordonnées et réduit seulement les
niveaux rationnels ; ce périmètre est correct pour ses entrées communes.

Dans un repère donné, la position physique vaut o+hq et le niveau physique
vaut h²β. Avec q'=8q,h'=h/8 ou q'=64q,h'=h/64, positions et niveaux physiques
restent IDENTIQUES : étirer les anciens entiers ne retrouve aucune précision.
Une vraie grille fine doit repartir des coordonnées d'origine, avec IDs et
repère/masque communs. Les niveaux physiques, égalités et incidences sont
alors jugés dans leurs unités déclarées ; on ne demande pas le même catalogue
si la géométrie a effectivement changé.

| B | Portée par axe à h=1 mm | h maintenant la portée de u18/1 mm environ |
| --- | ---: | ---: |
| 18 | 262,143 m | 1 mm |
| 21 | 2 097,151 m | 0,125 mm |
| 24 | 16 777,215 m | 0,015625 mm |

Ces portées sont h(2^B−1) pour des coordonnées normalisées, pas une promesse
de précision réelle du capteur. Les extrêmes entiers après quantification,
translation commune comprise, doivent être contrôlés ; ne pas clipper.
Aucun réglage de h ni préparateur LiDAR nouveau n'est livré par cette note.

## Collisions et frontières

Témoin exact, en millimètres : x=(0,3/8,1), IDs=(10,20,30). À h=1 mm,
q=(0,0,1) et deux retours fusionnent ; à h=1/8 mm, q=(0,3,8).
Le Cloud garde leurs deux IDs et le poids deux ; le catalogue actuel refuse
ce poids. Ne pas retirer un retour pour transformer cette entrée en poids un.
Multiplier q grossier par huit donne (0,0,8), pas les coordonnées plus fines.

Sans clipping, arrondi isotrope au plus proche : chaque retour se déplace
d'au plus sqrt(3)h/2. Entre deux pas h,h', avec appariement des mêmes retours,
le déplacement est au plus sqrt(3)(h+h')/2 ; la preuve de stabilité FULL en
rayon s'applique à la spécification avec toutes les multiplicités. Le moteur
pondéré reste absent : cette preuve ne le qualifie pas. L'attache figée
premier-cover/LCA n'hérite pas de cette stabilité, même après affinement.
Le cas (0,2,33/8) mm est (0,2,4) à 1 mm mais (0,16,33) à 1/8 mm : une grille
plus fine révèle l'asymétrie du témoin frontière déjà établi. Ce changement
n'est pas une erreur du prédicat exact et ne résout pas le croisement inter-K.

Prochain diagnostic de précision utile : déclarer h exact et origine, garder
masque/IDs communs, publier collisions et correspondances, puis séparer qualité
géométrique, correction native et coût de chaîne. Pour les millions de retours,
choisir B d'après l'étendue ET h ; augmenter B seul ne ferme aucun contrat
mémoire ou temps. [Calculs et métadonnées](check.py), [sources](SOURCE_BEFORE.json).
