# Clustering de points depuis un seul T_K — protocole du 27 septembre 2026

Cadre : `exploration_v9_hors_registre / cpu_reference /
quantized_u18_input_only / fixed_k_point_hierarchy_benchmark / not_claimed`.
Ce dossier est un prototype de recherche, pas une qualification FULL/G4/100 ms.
La demande explicite de benchmarks de clustering autorise ici des scores contre
les classes de référence. Ces classes ne servent ni à la géométrie ni au routage.
Pas de SemanticKITTI ni de labels KITTI dans cette campagne.

## Décision et question scientifique

On utilise **uniquement l'arbre d'ordre K**, pas les verticales ni une combinaison
des ordres. Le moteur natif calcule les ordres nécessaires à son fonctionnement,
mais seul T_K est exporté et consommé. Les couvertures de ses composantes peuvent
se chevaucher : leur simple union ne définit pas une partition des points.

Deux projections sont comparées, sans modifier le moteur :

1. `first_coverage`, proposition principale : rattacher chaque point à la
   composante dans laquelle il apparaît le plus tôt (au plus petit rayon exact).
   Si plusieurs composantes apparaissent simultanément, attendre leur plus proche
   ancêtre commun. Puis conserver ce trajet vers la racine, définitivement.
2. `entry_vote`, comparaison exploratoire avec l'idée historique de routage :
   conserver les premières entrées dans des branches incomparables, donner à
   chacune le poids r^-expZ, puis descendre irréversiblement dans le sous-arbre
   majoritaire. Une égalité, ou une comparaison numériquement trop proche, reste
   à l'ancêtre commun. Les entrées héritées d'un descendant ne sont pas recomptées.

Ce deuxième vote porte sur des **entrées de couverture ensembliste**, PAS sur les
incidences facette/coface pondérées du manuscrit. Ces dernières ne sont pas
reconstructibles depuis la seule topologie FULL et ne sont pas inventées ici.
Les deux projections sont indépendantes des répétitions de records de couverture.
`first_coverage` ne dépend pas d'expZ : l'exposant intervient ensuite dans EOM.
`entry_vote` utilise l'exposant dans le routage et dans EOM.

Les dates d'attachement sont conservées, y compris les continuations sans
multifusion. Avant son attachement, un point reste son propre singleton.
Les points écartés par EOM portent ensuite le label de bruit -1 ; ce label de
rendu ne représente pas un cluster unique de la hiérarchie emboîtée.

## Pourquoi la construction est cohérente

Une fois couvert par une composante, un point reste couvert par ses ancêtres.
Le plus petit rayon d'apparition maximise donc la précocité du rattachement à
une trajectoire admissible ; ce n'est pas une preuve de qualité statistique.
En cas d'ex-aequo, le plus proche ancêtre commun est le premier choix commun
aux trajectoires concurrentes, sans priorité arbitraire entre IDs.

Cette première apparition a aussi une interprétation géométrique simple.
Ici L_K(r) est l'ensemble des centres c tels que la boule fermée B(c,r)
contienne au moins K sites, et delta_r désigne la dilatation de rayon r.
Notons alpha_K(x) le plus petit rayon d'une boule contenant x et au moins K
sites (x compris), et d_K(x) la distance au K-ième voisin, x compris.
Alors **d_K(x)/2 <= alpha_K(x) <= d_K(x)** : toute boule de rayon r contenant
x et K sites les place à distance au plus 2r de x ; inversement la boule
centrée en x de rayon d_K(x) convient. Pour la couverture complète
X intersect delta_r(L_K(r)), alpha_K est précisément la première apparition.
Le rattachement proposé utilise donc une notion locale de densité comparable
à celle de HDBSCAN, mais avec centre libre. Cela ne rend pas les deux arbres
identiques : la connectivité et le traitement des ambiguïtés restent différents.
Le retard au LCA en cas d'ex-aequo peut dépasser alpha_K et doit être publié.

Chaque point est greffé une seule fois dans l'arbre source, à sa date effective.
Deux ensembles de feuilles descendant de nœuds de cet arbre sont disjoints ou
inclus l'un dans l'autre. Les coupes donnent donc de vraies partitions emboîtées,
en complétant les points non encore activés par des singletons individuels.
Le même argument vaut pour le vote dès lors que le trajet est irréversible.

Il ne faut surtout pas revoter indépendamment à chaque coupe : une fusion
de branches concurrentes pourrait faire changer un point de camp et détruire
l'emboîtement. Il ne faut pas non plus raccorder toutes les composantes qui
partagent un point : cela détruirait la séparation recherchée à l'ordre K.

## Protocole fixé avant lecture des scores

- 4 jeux FCPS réellement 3D complets : Hepta, Tetra, Atom, Chainlink.
- 3 familles synthétiques 3D (densités variables, formes non convexes,
  pont/bruit), chacune avec une réalisation développement et une évaluation.
- 2 jeux SIPU complets : Flame et Spiral, annoncés **2D, z=0**, séparés du bilan 3D.
- Pas de sous-échantillonnage. Une même quantification isotrope u18 des trois
  coordonnées pour les deux méthodes ; publier pas et erreurs, refuser les
  collisions plutôt que fusionner silencieusement points/classes.
- K=2,5,10 ; `min_cluster_size`=20,50 ; expZ=1,2. Configuration principale
  préannoncée : **K=5, taille minimale=20, expZ=1**. Toutes les lignes publiées,
  pas le meilleur paramètre par jeu. K=1 sert au contrôle single linkage.
- HDBSCAN sklearn 1.9.1, `min_samples=K` inclut le point lui-même,
  métrique euclidienne, alpha=1, epsilon=0, EOM, pas de sélection de la racine.
- Même condensation et même programme EOM sur les deux arbres de points,
  masses unitaires, multifusions simultanées. Densité lambda=1/r^expZ.
  À K fixé, K/(n omega_3) est constant et ne change aucun choix EOM.
- Publier également HDBSCAN standard (expZ=1) et les écarts éventuels dus
  aux plateaux binaires ; expZ=2 est une ablation commune, pas HDBSCAN standard.
- ARI tous points, NMI, nombres de clusters et couverture. En présence de bruit
  connu : ARI hors bruit vrai, précision/rappel/F1 du bruit. Un bon score après
  élimination massive de points ne suffit pas : garder le score tous points.
  Un second ARI retire le bruit vérité et traite chaque point rejeté comme un
  singleton distinct : rejeter une classe entière n'est ainsi pas récompensé.
- Pureté du dendrogramme sur les vrais points non bruités, après atomisation des
  niveaux égaux : moyenne, pour les paires d'une même classe, de sa proportion
  parmi les feuilles non bruitées sous leur ancêtre commun. Ce diagnostic
  évalue l'arbre indépendamment d'EOM ; il ne pénalise pas le bruit absorbé.
  Pour ARI/NMI tous points, -1 est traité comme une classe, convention publiée.
- Chronos séparés géométrie native, export/lecture, projection, condensation/EOM,
  HDBSCAN. Ce prototype Python/CPU n'est pas un comparatif GPU optimisé.

## Complexité et limites annoncées

Avec H nœuds source et M incidences de couverture effectivement développées,
le prétraitement de l'ancêtre commun utilise O(H log H) temps/mémoire ; la
projection première apparition utilise O(M log H) dans le pire cas avec égalités.
La greffe trie au plus n dates et construit au plus n-1 nœuds internes :
O(H+n log n), hors coût binaire des rationnels. Le vote traite pour chaque point
un arbre virtuel des entrées/ancêtres communs : pas de parcours de tous les
ancêtres pour chaque incidence. La condensation/EOM est linéaire dans l'arbre
de points, sous réserve des conventions numériques déclarées.

**Ces bornes ne prouvent pas que H ou M sont sous-quadratiques en n.** Le format
JSON développé et Python sont des outils de mesure, pas l'architecture massive
finale. Une version industrielle devrait consommer les populations compactes
et paralléliser première apparition, ancêtres communs et greffes ; son coût doit
être mesuré séparément de celui de la production de T_K.

La première projection utilise des niveaux rationnels exacts. Le vote utilise
float64 avec arrêt conservateur en cas de proximité, explicitement non certifié.
EOM utilise float64 pour les deux méthodes ; aucun certificat numérique exact
de stabilité n'est revendiqué. Les niveaux exacts et dates des greffes restent
archivés ; une collision de deux niveaux exacts distincts lors du rendu float
est refusée. Voir FAIRNESS.md pour le protocole numérique commun et DATASETS.md
pour les provenances. Les résultats effectifs seront consignés séparément.
