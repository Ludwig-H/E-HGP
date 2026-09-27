# Clustering hiérarchique de points après FULL, à K fixé

27 septembre 2026. Consommateur expérimental CPU, moteur v9 inchangé.
Le [plan figé](PLAN.md) distingue la correction mathématique, la qualité
statistique et les performances de production. Aucun GCP dans ce travail.

## Ce qui est maintenant construit

Chaque point choisit une seule branche, de haut en bas, par les scores de
ses facettes incidentes. Cette affectation est fixée avant toute coupe et
avant le seuil de taille. Les égalités restent au parent. On obtient ainsi
de vraies partitions emboîtées, sans permettre à un point de changer de
branche quand le rayon change.

Le nouvel [arbre explicite](POINT_TREE.md) possède une feuille par point,
des parents et des dates rationnelles. Les événements simultanés sont
traités ensemble ; les nœuds vides ou unaires sont supprimés. Les points
extérieurs restent des singletons, sans fusion inventée. Le squelette a
moins de 2n nœuds ; les métadonnées de provenance peuvent encore avoir
la taille de l'arbre de facettes source.

`point_eom.cluster_point_tree(tree, min_cluster_size=20, exp_z=1)`
condense ensuite cet arbre en comptant **les points**, puis applique EOM.
C'est la même implémentation de condensation/EOM que le comparateur HDBSCAN
commun. Les groupes sélectionnés ont donc au moins le nombre de points
demandé. Le bruit n'est pas rempli artificiellement. Ce premier adaptateur
accepte une seule vraie racine ; le constructeur d'arbre et ses coupes
supportent séparément les forêts et le nuage vide.

Les dates restent exactes dans l'arbre. La sélection statistique utilise
des rayons, densités et stabilités binary64 ; toute collision de deux
dates rationnelles distinctes lors de la conversion est refusée. Le
routage du pilote est exact sur les scores Sτ **déjà arrondis** et figés,
pas une certification des poids géométriques réels.

## Preuves et diagnostic

[Qualification close](QUALIFICATION.md) : quatre commandes normal/−O,
729 empreintes inchangées. Par mode : 181 fixtures, 3 124 coupes, 27 refus,
trois intégrations FULL ; neuf tests EOM, dont 228 comparaisons à un oracle
rationnel. Les sources et captures antérieures ne sont pas modifiées.

Le [pilote préannoncé est clos](RESULTATS.md) :13 scènes gaussiennes déjà
connues, K5, seuils20/50 et expZ1/2,52sélections nouvelles et182comparateurs
inchangés. La [contrelecture normal/−O](POST_AUDIT.md) passe :26arbres,
52condensations,364coupes,1 292pins, ARI et scores par classe vérifiés.
Les cas défavorables restent publiés. Ce pilote ne qualifie pas K10,
SIPU, la croissance LiDAR ni une durée GPU.

## Pourquoi relire les anciens clusterers ?

La [lecture de HGP-old et HGP-Clusterer3D](../../audits/LECTURE_HGP_OLD_CLUSTERER3D_20260927.md)
confirme que leur chemin habituel est différent : masses sur les facettes,
EOM, puis vote vers les points. Le helper `whole_tree` old constitue une
inspiration de routage, pas un dendrogramme global exclusif déjà acquis.

Surtout, leurs catalogues contributifs incluent des simplexes non Gabriel.
Préserver la connexité FULL en retirant des événements redondants ne prouve
pas que les sommes de poids restent identiques. C'est une différence à
isoler expérimentalement, pas une raison de changer silencieusement la
signification des scores déjà publiés. La nouvelle projection doit rester
comparée au vote plat et à la première couverture, pas les remplacer par
décret.

La [lecture du protocole SIPU historique](../../audits/PROTOCOLE_POINTS_THESE_SIPU_20260927.md)
distingue encore une troisième voie publique K2 par première incidence ;
elle n'est ni le `fit` pondéré old/3D ni une reproduction acquise ici.
