# G-L7 : empreinte partagee par cellule

Lecture seule du prototype `scratchpad/v12_tour_Gc/repo2`, annonce RAPPORT 23:15 ;
epingles et stabilite dans `capture.json`. Aucun natif, benchmark ou GCP execute.
Ce complement au recu `gc_index_borne` ne qualifie pas une livraison.

Pour une trace valide F = I union A, interieurs et coquille sont disjoints. Dans
l'anneau Z/(2^64), l'addition est associative et commutative : la somme des
`site_key` de F triee est exactement la somme de I, preparee une fois, plus
celle des sites de A selectionnes par le masque. `passes.cpp:108-122` conserve
des `u64` et applique le masque de l'index APRES la somme, comme
`populations.cpp:207-210`. Cela vaut pour tout masque, y compris zero et les
masques non contigus. Masquer prematurement les termes ne serait pas en general
equivalent. Le hash selectionne une recherche ; `find` exige aussi l'egalite
de toute la population. Aucune collision ne constitue un certificat.

Les masques internes de count/fill designent uniquement les m sites de U.
La garde m<=64 et les boucles sur bits non nuls rendent les indices 0..63
valides. Le stockage ajoute `u64[64]`, soit 512 octets de pile par callback,
sans allocation sur le tas. La file conserve chaque trace, sa cle, son
representant et sa cellule par valeur : changer de cellule ne change pas une
requete encore en vol. A l'entree, 0<=in-out<=15 ; le seizieme element est
consomme avant reutilisation de sa case. Les deux precharges ne decident rien,
la vidange reste FIFO. Les compteurs u64 ne peuvent boucler sur le domaine
declare d'au plus 2^32-1 representants par ordre.

Pour R representants d'une cellule, p=|I|, m=|U|, t=k-p, les appels au melange
`site_key` passent de R*k a p+m. Il reste R*t additions, la construction des
R traces et toutes les recherches exactes. Le nombre de melanges ne diminue
que si R*k>p+m : une cellule inerte a un representant peut en payer davantage.
Aucun gain de temps n'est deduit de cette reduction locale. L'essai local du
pilote annonce a 23:10 reste non decisif, avec bruit A/A declare ; sa reprise
statistique appartient au recu du pilote.

La porte `index_unit.cpp:147-189` construit d'abord la resolution produit, puis
rejoue chaque representant avec un index a masque nul, directement et via G-L5.
Elle compare toutes les cibles puis TOUS les compteurs (objet recopies dans
le compteur du rejeu, travail compare effectivement). Elle exige des premieres
sondes reussies, des succes apres pas et des arrets cellule. Le mutant declare
`empreinte_de_file_sans_interieur` touche seulement le hash de la file, pas
`key_of` du rejeu : il peut creer des sondes manquees que les compteurs voient,
meme quand la descente exacte conserve les cibles. L'egalite W1/W8 seule ne
refute pas une erreur deterministe commune aux deux regimes. Une empreinte
d'objet identique ne qualifie donc pas le travail. Manifestation causale lue
dans le source et le manifeste ; aucune nouvelle destruction native rejouee
ou attachee a ce recu.

`python model.py` et `python -O model.py` donnent `results.json` identique :
1 000 traces, 5 masques, debordements et bit63, collisions forcees departagees
par population complete, plus omission concrete de l'interieur. Ce modele
verifie l'algebre et le temoin de sondage, pas la descente geometrique du moteur.
