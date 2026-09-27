# Référence Gabriel pondérée à K fixé

27 septembre 2026. Développement expérimental après la
[relecture du manuscrit et de HGP-old](../../audits/RELECTURE_THESE_ET_HGP_OLD_20260927.md).
Cadre : `exploration_v9_hors_registre / cpu_reference /
quantized_u18_input_only / weighted_fixed_k_reference / not_claimed`.

## Objet et limites

Cette tranche remet les opérations du §9.1 dans leur ordre : toutes les
cofaces contributives, scores et masses de facettes, condensation/EOM,
puis vote vers les points. `expZ` modifie les masses ET les niveaux de
densité. Le seuil minimal porte sur une masse fractionnaire, pas sur un
nombre de facettes ni sur le cardinal final des clusters après vote.

La source géométrique est le catalogue canonique complet du moteur v9,
pas les seules populations mentionnées dans les couvertures FULL. Pour
chaque boule, on inclut tous les intérieurs et énumère les sous-ensembles
de coquille de la bonne taille dont l'enveloppe convexe contient le centre.
Il n'y a pas d'énumération des parties de tout le nuage dans ce chemin.
L'oracle exhaustif est réservé à de petits cas de qualification.

Il s'agit d'une **référence Gabriel pondérée à K fixé, confrontée à T_K**.
Ce n'est pas encore un lecteur du seul T_K nu : son supplément d'incidences
ne découle pas des parents/couvertures. Aucune verticale ni autre ordre
n'intervient dans le choix des labels ; l'exporteur natif calcule néanmoins
la chaîne amont 1..K. Les sorties ponctuelles votées ne constituent pas
encore une hiérarchie de partitions emboîtées à toutes les coupes.

La borne prouvée dans [AUDIT_MATH.md](AUDIT_MATH.md), `m_facette <= 1`,
justifie le profil `min_cluster_size >= 2` : les dates des facettes isolées
ne changent pas les composantes admissibles. Ce n'est pas un plafond de
recherche ni une suppression des petits objets géométriques.

Géométrie et niveaux source entiers/rationnels ; post-traitement statistique
binary64, avec diagnostics des décisions proches du seuil. Les masses à
expZ2 ont aussi une référence rationnelle pour les tests. Aucune nouvelle
certification numérique globale, mesure G4 ou promesse de 100 ms.

## Implémentation

- `native_weighted_export.cpp` : adaptateur natif séparé, catalogue complet
  et cofaces dédupliquées, export T_K original conservé pour confrontation.
- `weighted_model.py` : sommes avant réduction, normalisation par point,
  fusions simultanées par niveaux rationnels, vote après sélection.
- `weighted_eom.py` : port explicite du sélecteur commun déjà testé,
  désormais à masses positives, sans modification du module épinglé.
- `test_weighted_model.py` et `test_weighted_eom.py` : cas positifs,
  invariants, refus et références indépendantes.
- `qualify_geometry.py` : oracle rationnel sur petits nuages seulement.
- `benchmark_weighted.py` : pilote apparié décrit ci-dessous.

Pour C cofaces et F facettes, la préparation Python coûte, à K fixé,
O(C log C) en temps et O(C+F+n) en mémoire. La formation explicite des
tuples stocke O(K²C+KF+n) identifiants lorsque K varie. La condensation
est linéaire hors tris/départages documentés ; le vote paie les KF
incidences. Ces bornes **ne bornent pas C ni F en fonction de n**.
Les coquilles non régulières peuvent donner plusieurs cofaces par boule.
Le refus natif des coquilles de plus de 12 sites est conservé, pas masqué.
Cette matérialisation est une référence de validation, pas la nouvelle
architecture industrielle par défaut.

## Pilote gaussien fixé avant les nouveaux scores

Même corpus de 48 scènes que la campagne précédente ; sous-ensemble
diagnostique annoncé de 13 scènes complètes de 1 200 points :

- trois graines chacune : sphériques (G2, δ8), (G8, δ4), (G16, δ2) ;
- deux graines chacune : allongées (G8, δ4), déséquilibrées (G8, δ4).

Ces régimes sont connus après la campagne précédente : **ce n'est pas une
évaluation tenue à l'écart**. K5/10, seuils20/50, expZ1/2, référence K5/20/1.
26 exports natifs, 104 sélections pondérées prévues. Comparateurs déjà
capturés réutilisés par hash : première couverture, HDBSCAN EOM commun,
HDBSCAN standard à expZ1. Total attendu : 364 lignes, aucun réglage choisi
sur les nouveaux scores. La racine est exclue pour les deux EOM ; c'est
un profil commun explicite, pas le défaut historique autorisant la racine.
Pas de remplissage 1-NN, même entrée, bruit et égalités déclarés.

ARI/NMI, couverture, nombre de clusters et appariement des classes restent
les métriques précédentes. Les cardinalités finales après vote sont publiées
sans leur imposer artificiellement la borne du seuil massique. La vérité
terrain ne participe ni aux poids ni à EOM. Le pilote ne qualifie pas une
supériorité générale, un gain de vitesse ou une croissance sous-quadratique.

Les captures utilisent des répertoires neufs, enregistrent les échecs et
ferment les sources/binaires/entrées par hashes. Données et grands tableaux
intermédiaires restent privés ; les sorties gzip sont sans date variable.
Ni HGP-old ni ses dépendances sous licence distincte ne sont copiés dans
la ligne produit. GCP non utilisé pour cette tranche.
