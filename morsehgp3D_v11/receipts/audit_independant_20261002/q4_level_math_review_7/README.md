# Niveau q4 différé et minorant commun d'une famille d'extensions

Les copies initiales viennent du checkout développeur à
`9df77494732b03ddf11dbcf1dcb11d96bef54a3b` : douze fichiers publiés et
`docs/CATALOGUE.md` WIP, identifiés par empreinte. Le code copié calcule
encore Level pendant la factory q4. Ce reçu vérifie **les conditions d'une
modification future**, sans attribuer à ce snapshot un port lazy déjà fait.
Aucun produit modifié, compilé, importé ou exécuté, aucun appel cloud.

## Level après admission du support canonique

Pour un candidat défini par une ancre de coquille a et un centre
`c=a+N/D`, D>0, le rayon carré est exactement `sum(N_j²)/D²`.
Propriété, signes de puissance, orientations, stricte positivité et tests
de support canonique dépendent seulement de a,N,D et des sites ; ils ne
requièrent pas ce quotient de degré huit. Voir les appels dans
[leaf.cpp](source/morsehgp3D_v11/src/catalogue/leaf.cpp),
[support.cpp](source/morsehgp3D_v11/src/catalogue/support.cpp) et
[catalogue.cpp](source/morsehgp3D_v11/src/catalogue/catalogue.cpp).

Il est donc mathématiquement sûr de matérialiser Level après le census
exact, la canonicalisation, `support == generated` et l'admission
`p+q_min<=K+1`, juste avant Collector. Les branches q2/q3 restent
exhaustives, y compris lorsque la boule possède une présentation q4
positive mais un support canonique de cardinal inférieur. Un triplet
obtus reste disponible comme préfixe q4. La factory q4 fermée ne certifie
pas que le centre est intérieur au tétraèdre : ce contrôle demeure séparé.
Les seuls candidats publiés portent un Level réel ; jamais un faux zéro.

**L'ancre doit rester un site de coquille.** Le centre seul ne détermine
pas le rayon. Rebaser le même centre sur un site intérieur puis appliquer
`sum(N_j²)/D²` construirait une autre boule. L'interface fermée doit
préserver le certificat d'ancre, les formes numériques et l'arité de
présentation ; ce reçu ne propose pas un tuple forgeable.

Un q4 strict appartient au hull de ses quatre sommets. Si tous sont dans
la fermeture de la boîte Q, pleine dimension et poids stricts placent
chaque coordonnée du centre strictement entre min/max : il appartient
alors à Q demi-ouverte. Ce certificat optionnel d'owner ne détermine **ni
p ni I/U**. Ajouter le site central à un tétraèdre régulier conserve son
support strict et augmente p de zéro à un ; l'admission à K3 disparaît.

## Nouveau minorant commun aux extensions q4

Soit T un triplet non collinéaire, aigu ou obtus, avec cercle circonscrit
de centre c_T dans son plan et rayon r_T. Tous les centres des
circumsphères des quadruplets indépendants T∪{s} sont sur la droite
`c_T+t n`, normale au plan. Leur rayon carré est `r_T²+t²||n||²`.
Pour tout site z **coplanaire** avec T, la différence
`||z-(c_T+t n)||²-(r_T²+t²||n||²)` vaut exactement
`||z-c_T||²-r_T²`, indépendamment de t.

Ainsi tout z coplanaire strictement intérieur au disque de T est un
intérieur commun à toutes ses extensions q4. Il est distinct des sommets
de T ; le quatrième sommet s, non coplanaire, ne peut l'absorber. Compter
une fois ces témoins dans un sous-ensemble sûr de L⊂X fournit un minorant
commun de p, sans exiger une population complète ou un centre c_T
propriétaire. La frontière du disque ne compte pas.

Pour K>=3, saturer le compte à **K−2** suffit à constater
`p > theta_4 = K−3` et couper les **présentations/extensions q4** de T
avant construction. Ce n'est pas nécessairement un rejet de la boule
géométrique : elle peut avoir qmin2/qmin3 via d'autres sites, que leurs
branches exhaustives doivent encore traiter. La décision q3 utilise
séparément `theta_3=K−2`, avec rejet strict au-delà ; appliquer theta_4
à q3 perdrait des événements admis. Pour K<3, l'arité4 est déjà hors seuil.

Ce compte spécial sert seulement au rejet. Il ne devient ni un seed ni
une addition au census final et n'autorise aucune acceptation précoce.
Le census q3 ordinaire contient aussi des sites hors du plan, dont la
puissance varie avec t : **son compte ne peut pas être réutilisé en bloc**.
Un parcours q3 déjà payé peut relever séparément ses intérieurs
coplanaires. Ajouter un scan Theta(|L|) par triplet sans mesurer son coût
total déplacerait potentiellement le travail évité ; aucune baisse de
complexité ou de temps n'est déduite de ce certificat.

## Témoins exacts et gardes FULL

T={(10,5,5),(2,9,5),(2,1,5)}, z=(5,5,5), s=(5,5,11) donne :
le cercle de T a beta25, p1 et reste admis à K3 ; le q4 strict a centre
(5,5,71/12), beta3721/144, p1, rejeté à K3 et admis à K4. Le témoin z a
puissance −25 sur toute la droite des centres. Ce cas garde le bon
événement q3 quand ses extensions q4 sont coupées.

La perturbation hors plan est réelle : placer T au plan z50, prendre
z=(5,5,51) et s=(5,5,9). z est intérieur au cercle de T (puissance −24),
mais extérieur au q4 strict (puissance 672/41). Il ne doit pas entrer
dans le minorant commun. Un autre témoin conserve un préfixe obtus et
un site médian coplanaire, puis l'admission q4 à K4.

[check.py](check.py) est indépendant du produit et des oracles développeur.
Il compare les petits catalogues exhaustifs aux seules présentations q4
filtrées, avec mêmes supports canoniques et I/U, sur quatre nuages de cinq
sites, 24 ordres de sites, K1..5. Les lectures normal/−O sont identiques.
Il vérifie aussi la matérialisation exacte du Level et le contre-cas
d'une ancre intérieure. [RUN.json](RUN.json) conserve les premières sorties.

Pour le futur raccord FULL, conserver les rangs exacts et plateaux après
tri, tous I/U, les préfixes obtus, et les relations boule forte→nœud vivant
des continuations étendues. Une liste de feuille ne certifie pas un census
au centre d'une MEB descendue hors de sa boîte. Différer Level ne permet
pas de supprimer les comparaisons exactes des descentes ou leurs dates.
Les champs géométriques du catalogue restent la porte de non-régression ;
ce reçu ne qualifie ni FULL ni core/cover ni une projection de points.

Les copies et observations sont fermées par `SOURCE_BEFORE.json`,
`SOURCE_AFTER.json` et `SHA256SUMS`, vérifié par le lecteur lorsqu'il existe.
