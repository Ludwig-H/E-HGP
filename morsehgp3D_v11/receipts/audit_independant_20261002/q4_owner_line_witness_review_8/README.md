# Témoins communs sur la ligne des centres propriétaires

Proposition mathématique, sources figées `ffc2ff95f` avant lecture.
Le produit porte le niveau q4 différé, **aucun filtre de famille** ci-dessous.
Aucun code produit importé, compilé ou exécuté, aucune GCP ou mesure de débit.
Ce certificat étend le minorant coplanaire à des sites hors du plan, en
restreignant les centres à la boîte propriétaire ; il ne suppose aucune
tolérance de plan ni approximation de la géométrie.

## Certificat exact

Soit T=(a,b,c) non collinéaire, aigu ou obtus. Poser n=(b−a)×(c−a),
D=2||n||²>0, et N le numérateur relatif du cercle circonscrit :
c_T=a+N/D, avec n·N=0. Toutes les circumsphères des extensions indépendantes
T∪{s} ont leur centre sur c(u)=a+(N+u n)/D, u réel.

Dans une boîte **fermée** Q=[lo,hi], l'intersection J de cette droite et
de Q est décrite, axe par axe, par :

`D(lo_i−a_i)−N_i <= u n_i <= D(hi_i−a_i)−N_i`.

Pour n_i=0, vérifier cette contrainte constante ; sinon normaliser les deux
fractions et intersecter **toutes** les contraintes. J est vide ou un
intervalle rationnel fermé borné, car n≠0 et Q est bornée. J vide suffit à
couper les présentations q4 propriétaires de Q. Employer Q fermé est
conservateur pour la propriété demi-ouverte du produit ; conserver les contacts.

Pour un site z, poser delta=z−a, F=D||delta||²−2N·delta et S=n·delta.
La puissance géométrique de z par rapport à la sphère de centre c(u),
passant par a,b,c, est exactement `(F−2uS)/D`.
Ainsi `max(F−2u_min S,F−2u_max S)<0` certifie un intérieur strict commun
à **toutes les extensions dont le centre appartient à Q**, même hors plan.
Pour S=0, on retrouve le certificat coplanaire. Une puissance nulle à un
seul endpoint interdit ce crédit ; aucun epsilon n'est introduit.

Compter les SiteIdx distincts certifiés dans un sous-ensemble sûr L⊂X.
Pour K>=3, atteindre K−2 prouve p>K−3 et coupe seulement ces présentations
q4. Garder les branches q≤3 exhaustives : la boule peut avoir qmin moindre.
Aucune acceptation précoce, aucun seed ni addition au census final.
Un quatrième site d'une extension réellement propriétaire est sur sa
coquille ; il ne peut donc être compté comme ce témoin strict commun.
Si l'on combine ce compte avec les dominateurs existants, conserver les
IDs et l'union sans doublons, ou prendre le maximum des comptes ; ne pas
les additionner en supposant les ensembles disjoints.

Un centre c_T hors Q ou un triplet obtus ne permettent pas de supprimer
ses extensions. Le clipping de la ligne et la positivité q4 restent les
certificats distincts appropriés.

## Domaine numérique d'un éventuel port

M=2^B, points dans [0,M), extrémités de Q dans [0,M]. Chaque composante
de n est une aire orientée de trois points du même carré : |n_i|<M².
La forme est multiaffine en leurs coordonnées ; ses extrema sur le carré
fermé sont aux sommets et ont magnitude au plus M². Les bornes q3 déjà
certifiées D<24M⁴, |N_i|<24M⁵, |F|<216M⁶ donnent :

- A=D(edge−a_i)−N_i : |A|<48M⁵ ; i128 suffit jusqu'à B24 ;
- S=n·delta : |S|<3M³ ;
- comparaison de deux endpoints A_i/n_i : produits de degré sept ;
- signe à l'endpoint normalisé A/d : Fd−2AS, somme des magnitudes
  <216M⁸+288M⁸=504M⁸<2^(8B+9).

Le signe demande donc jusqu'à 153/177/201 bits en B18/21/24, soit
3/3/4 mots larges. **Il ne bénéficie pas de la voie q4 i128**, même en
B18. Tous les intermédiaires signés, signes de dénominateurs et conversions
devront être portés et qualifiés ; les tests Fraction ne le font pas.
Cette formule évite le carré générique de N3 de degré dix ; elle ne
matérialise aucun faux Level et ne change pas les Level q3 existants.

## Cas utile et limite du certificat

T={(10,5,50),(2,9,50),(2,1,50)}, z=(5,5,51),
Q=[4,6]×[4,6]×[50,52]. z est hors du plan et strictement intérieur à toutes
les sphères de T dont le centre appartient à Q. L'extension s=(5,5,56)
est propriétaire et contient z. L'extension s=(5,5,9) a son centre hors Q
et laisse z dehors : puissance 672/41. Oublier la restriction d'owner
rendrait le certificat faux. Le site (5,5,55) touche une sphère à un
endpoint de J : il ne reçoit aucun crédit.

[check.py](check.py) confronte les coefficients du cercle à une résolution
Gram indépendante, les puissances affines à 1 260 sphères q4 résolues
directement, les trois axes/permutations, les contacts et trois profils
aux limites. 5 807 contrôles normal/−O byte-identiques ; 65 branches de
témoins stricts réellement exercées dans les cas aléatoires figés.
Les sept sources sont dans [SOURCE_BEFORE.json](SOURCE_BEFORE.json).

Pour le coût, préparer J une fois par triplet puis réutiliser un parcours
de sites déjà payé quand il existe. Le census q3 actuel n'est joué que pour
un triplet aigu propriétaire et peut s'interrompre : il ne fournit pas une
population complète réutilisable en bloc. Toute préparation, nouvelle
visite/orientation et comparaison large doit être mesurée avec le résidu
et les sorties. Aucun régime LiDAR atteignable, gain ou borne globale
n'est établi ici. Garder ce levier séparé de l'ablation du niveau q4 différé.
