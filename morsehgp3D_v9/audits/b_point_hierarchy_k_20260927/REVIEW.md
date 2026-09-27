# Contrelectures indépendantes de la capture R1

27 septembre 2026. Aucune modification du moteur, de la capture ou des sources
gelées pendant ces relectures. GCP non utilisé.

Deux contrelectures séparées ont vérifié les résultats. La plus complète
est conservée dans [review_capture_r1.py](review_capture_r1.py), copie byte à
byte du script exécuté par l'auditeur :
SHA256 `f7140446b67be3e690d7e48de26d7d9f50c587e4bf3663db1de3dd75eea32a2f`.
Elle est **spécifique à cette capture locale** : chemins privés absolus et
versions épinglées, pas un lecteur autonome sans données. Ce script a été
ajouté après la capture initiale ; il n'est pas rétroactivement inclus dans
les hashes de ses quinze sources ni dans les neuf commandes de gates.

Exécution rapportée PASS, code0 :

- 504 tuples de paramètres uniques, grille complète, CSV concordant.
- 504 ensembles de métriques recalculés par formules indépendantes du runner :
  contingences entières pour ARI, entropies pour NMI, bruit/couverture.
- Pureté recalculée par paires de points et ancêtre commun, distincte de la
  récurrence de comptage utilisée par le benchmark.
- 144 projections et 432 sélections EOM rejouées à sorties JSON identiques.
- 72 nouveaux fits sklearn, arbres/sélections/labels/provenances identiques.
- 36 commandes natives reliées à leurs captures, stdout/stderr rehashés.
- Entrées communes complètes, 8 364 points, sources/binaire/sklearn stables.

Le reçu contrôlé est
`a299d72e8fd49e0e1bc243183a684c28fcff9c2d054f1794faeedc558adaebac`.
Les métriques indépendantes sont comparées avec une tolérance numérique
relative1e-11/absolue1e-12, pas certifiées comme nombres réels exacts.
Les replays de projections/sélections utilisent les mêmes implémentations ;
ils vérifient la fidélité de capture, pas leur justesse par un deuxième moteur.
Les oracles géométriques et de coupes sur petites fixtures sont ceux des gates.
Le présent fichier est un compte rendu de contrelecture, pas un faux reçu
de commandes dont les flux complets auraient été archivés.

Réserves expressément confirmées :

1. L'atomisation change16/72 résultats HDBSCAN ; le rejeu binaire concorde72/72.
2. La petite avance HGP agrégée masque un recul sur le sous-ensemble évaluation.
3. Atom est séparable dans l'arbre HGP mais EOM sur-segmente ; cela ne prouve
   pas qu'une seule coupe de rayon commun reconstruise les classes.
4. Ni performance G4, ni supériorité statistique générale, ni croissance LiDAR
   ne découle de ces benchmarks CPU de petite taille.

Les autres contrelectures ont validé l'emboîtement des greffes datées,
la borne d_K/2 <= alpha_K <= d_K et la distinction entre couverture dilatée
et appartenance des points aux régions de centres. La proposition C∩X demeure
non implémentée et n'a pas été incluse dans les scores.
