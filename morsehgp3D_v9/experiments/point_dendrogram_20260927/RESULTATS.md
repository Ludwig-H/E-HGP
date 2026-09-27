# Bilan : arbre ponctuel explicite et comparaison gaussienne

27 septembre 2026. **Le routage exclusif améliore plusieurs résultats et
fournit désormais un véritable dendrogramme de points, mais ne domine pas
HDBSCAN sur tous les critères.** Le vote pondéré précédent est conservé
comme comparateur, pas rebaptisé en dendrogramme ponctuel.

Le pilote préannoncé est clos : 13 scènes complètes connues de 1 200 points,
K5, seuils20/50 et expZ1/2, **52 nouvelles sélections +182 lignes héritées**.
Aucun nouveau calcul géométrique ni fit HDBSCAN. Le plan n'est pas ajusté
aux résultats. C'est un diagnostic de développement, pas une validation
tenue à l'écart. Les moyennes reposent sur trois graines sphériques et deux
graines dans chaque régime de stress, pas sur un grand échantillon.

Publication après contrelecture : [toutes les tables](../../receipts/point_dendrogram_20260927/r1/TABLES.md),
[234 lignes](../../receipts/point_dendrogram_20260927/r1/rows.csv),
[90 agrégats](../../receipts/point_dendrogram_20260927/r1/aggregates.csv),
[provenances](../../receipts/point_dendrogram_20260927/r1/receipt.json).

## Profil principal : K5, seuil20, expZ1

ARI tous points, bruit inclus ; F1 macro après appariement des classes.
Le comparateur ci-dessous est HDBSCAN avec **la même condensation/EOM**.
Sa variante standard et la première couverture restent dans les tables.

| Régime | ARI routage | ARI HDBSCAN | F1 routage | F1 HDBSCAN |
|---|---:|---:|---:|---:|
| Sphérique, 2 amas bien séparés | 1,0000 | 1,0000 | 1,0000 | 1,0000 |
| Sphérique, 8 amas, δ4 | 0,5061 | 0,2239 | 0,6384 | 0,4286 |
| Sphérique, 16 amas, δ2 | 0,0200 | 0,0197 | 0,0799 | 0,0539 |
| Anisotrope, 8 amas, δ4 | 0,1977 | 0,1856 | 0,1938 | 0,4410 |
| Déséquilibré, 8 amas, δ4 | 0,5406 | 0,2686 | 0,6952 | 0,3231 |

Les progrès utiles sont les huit amas sphériques et déséquilibrés. Le
gain par rapport à la première couverture reste modeste : ARI0,5061 contre
0,4948 et0,5406 contre0,5359. Le routage est donc une alternative désormais
testée, pas un motif suffisant pour abandonner cette baseline préservée.
Sur les13 scènes individuellement, ce profil donne **sept ARI supérieurs,
trois égaux et trois inférieurs** à HDBSCAN commun : les moyennes favorables
ne constituent pas une victoire sur chaque scène.

Sur les amas anisotropes, le meilleur ARI moyen masque une récupération
des classes bien inférieure : 2,5 groupes contre5 pour huit classes vraies,
malgré95,54% de couverture contre70,25%. **Ce cas n'est pas gagné.**
Sur les seize amas très recouvrants, les faibles scores des deux méthodes
ne correspondent pas à une récupération satisfaisante des communautés.

## expZ2, toujours au seuil20

| Régime | ARI routage | ARI HDBSCAN commun | F1 routage | F1 HDBSCAN commun |
|---|---:|---:|---:|---:|
| Sphérique, 2 amas | 1,0000 | 1,0000 | 1,0000 | 1,0000 |
| Sphérique, 8 amas, δ4 | 0,5064 | 0,2239 | 0,6385 | 0,4286 |
| Sphérique, 16 amas, δ2 | 0,0407 | 0,0197 | 0,2005 | 0,0539 |
| Anisotrope, 8 amas, δ4 | 0,4558 | 0,2523 | 0,7154 | 0,5943 |
| Déséquilibré, 8 amas, δ4 | 0,5412 | 0,3473 | 0,6966 | 0,4813 |

ExpZ2 aide ici l'anisotropie, mais produit12,5 groupes en moyenne pour huit
classes : la sur-segmentation reste visible. La première couverture y a
un ARI plus élevé0,4755, mais un F1 plus faible0,6386. Aucun seul critère
ne résume toute la qualité. Les seuils50 sont tous publiés, sans remplacer
le profil principal par le meilleur réglage de chaque scène.

Changer z change les scores de routage et la densité λ de cette variante ;
pour HDBSCAN commun, seule λ change. Les scores Sτ sont les valeurs
binary64 déjà archivées, relevées en rationnels exacts, y compris en z2.
Les dates restent rationnelles ; les calculs EOM sont approximatifs.

## Structure, coût et portée

Les26 arbres ont chacun1 200 feuilles,1 024 à1 063 multifusions, une racine,
aucun point extérieur. Les364 coupes diagnostiques strictes/fermées sont
identiques à celles du routage source. La qualification indépendante sur
petits cas est décrite dans [QUALIFICATION.md](QUALIFICATION.md).

La référence Python paie encore112 927 à153 681 nœuds source et427 520
à582 185 incidences par unité. Le routage prend **7,91 à11,22 secondes**,
la construction/validation de l'arbre ponctuel1,02 à1,36 seconde, en CPU
local partagé, sans répétitions de performance. Ce n'est **pas un moteur
industriel rapide**, ni une mesure FULL ou G4. Les359,51 secondes de campagne
incluent lectures, contrôles et écritures. Aucune borne de croissance
8k/16k/32k ni qualification100ms n'en découle.

Le squelette final est O(n) ; sa provenance et l'entrée facettes ne le
sont pas nécessairement. Éviter les tables point×nœud supprime un coût
inutile, mais ne borne pas le nombre de facettes géométriques.

## Ce que change la lecture des anciennes implémentations

[HGP-old et HGP-Clusterer3D](../../audits/LECTURE_HGP_OLD_CLUSTERER3D_20260927.md)
utilisent habituellement masses de facettes → EOM → vote plat. Leur catalogue
contributif inclut des cofaces non Gabriel ; leur politique des racines et
leurs régularisations numériques diffèrent aussi du profil commun actuel.
Le chemin public SIPU K2 est encore une autre variante, à distinguer.

Le [contre-exemple exact à quatre points](../../audits/CATALOGUE_ET_POIDS_NON_GABRIEL_20260927.md)
montre que la restriction au catalogue Gabriel modifie les poids et même
l'univers des facettes. Une preuve de connexité obtenue par ailleurs ne
suffirait pas à justifier la préservation de cette mesure. Le petit test
ne compare pas lui-même les partitions des graphes. Certaines boules contributives sont
hors de la fenêtre FULL du K fixé. Cela interdit une simple agrégation
des seuls événements conservés ; avec les coordonnées complètes, une
régénération géométrique reste possible, mais constitue un calcul nouveau.

La priorité suivante est une **ablation du catalogue sur de petits oracles
exacts**, avec géométrie FULL et règles de projection fixées, avant tout
grand port rapide ; les facettes supplémentaires doivent être réattachées.
Comparer aussi le vote massique et le routage unitaire sur chacun des deux
catalogues. Il faudra ensuite une évaluation tenue à l'écart et une
reproduction SIPU précisément nommée. On ne peut transférer les bons scores
du vieux programme à une projection ou à un catalogue différent.

## Capture privée

`/tmp/mhgp9-point-dendrogram-pilot-20260927-F4NFQA/capture/receipt.json`,
SHA256 `89a206c4305d26a98b945952f155b5bf0661208a9b00fb744fe900f250472fb9`.
Code0,26 unités complètes, sources avant/après identiques ; les182 lignes
héritées restent inchangées. Les arbres, condensations et labels sont
conservés par unité. Les scripts/tableaux publiés ne remplacent pas ces
payloads privés pour une contrelecture LIVE. Aucun GPU ni GCP utilisé.

[Contrelecture close](POST_AUDIT.md), normal/−O identiques :26arbres,
52condensations/labels,364coupes et1 292pins. ARI et scores appariés
recalculés ; solveur hongrois partagé, NMI non recalculée. Onze tests du
runner et dix du lecteur passent dans chaque mode. La publication est
aussi contre-relue :234lignes,90agrégats,1 620contrôles mean/min/max et
40valeurs des deux tableaux ci-dessus concordent.
