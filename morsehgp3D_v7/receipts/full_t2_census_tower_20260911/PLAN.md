# Oracle T2 borné pour le vrai raccord census → tour

Travail privé sur c03f6be8. Aucun changement produit, GPU ou GCP.
But : confronter les catalogues réellement générés à un inventaire Gram
indépendant, puis leurs coupes/parents/verticales à Gamma, jusqu'à K10.

L'oracle historique reste inchangé. Le nouveau modèle réemploie uniquement
ses rationnels et son système de Gram (jamais les prédicats produit).
Il calcule une fois chaque support positif de taille au plus quatre.
Un tel support Q et sa boule B certifient exactement la MEB de chaque F
tel que Q ⊆ F ⊆ X∩B : Q impose le rayon et B contient F. Deux témoins
attribués au même F doivent donner exactement la même boule. Toutes les
facettes sont vérifiées présentes, sans repli dans un moteur produit.

Gamma est encore explicite et exhaustif, mais ses voisins sont énumérés
par ajout d'un point puis retrait d'un point, au lieu de comparer toutes
les paires de facettes. C'est la même adjacence |F∪G|=K+1, pas un quotient.
Les nouveaux raccourcis de juge doivent être comparés littéralement au
modèle historique pour n≤8 avant l'emploi à n12/14. Ils n'ont aucune
vocation à devenir le chemin produit. La borne 14 est celle de cet oracle
exponentiel de test, pas une limite du nombre de points du constructeur.

Le constructeur n'est alimenté que par generate/prefilter/census réels.
L'inventaire rationnel reste le juge, pas son entrée de substitution.
Comparer s=8/10/12, les modes temporel/statique 1/4, des coupes ouvertes
et fermées et des permutations explicites. Commencer par une ligne n12
puis une coquille de douze points avec centre et extérieur n14.
Tout refus ou toute divergence reste conservé ; ni complétude universelle
WSPD ni contrat de vitesse ne sera déduit de cette campagne bornée.
