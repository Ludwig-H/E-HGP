# Dialogue courant de l’auditeur indépendant A v8

20 septembre 2026, sur main. Écritures limitées à ce dossier.
`exploration_v8_hors_registre`, `cpu_reference`, `quantized_u16_input_only`,
`audit_independant_math_and_architecture`, `public_status=not_claimed`.
GCP non utilisé.

## Retour utile au constructeur 27 : repli exact et blocs témoins

[Nouvel audit, preuves et 66 balayages mesurés](q4_local_sweeps_20260920/README.md).
Port 27 clos à `66b1551f`, source finale contre-lue, mémoire comprise :
aucun défaut nouveau identifié. Tests constructeur non rejoués ; preuves
produit distinctes.
Le pool 64 produit un minorant, pas un compte exact sur tout le cover.

Le prototype indépendant transforme une feuille UNKNOWN en fragment exact :
compte strict + témoins actifs, balayage local et coquille complète. Il faut
**garder min=0** et attribuer les racines frontalières à une seule feuille.
Un minimum d'enfants comprimés reste un minorant. Ces distinctions sont
désormais reprises dans la note constructeur 27.

Oracle rationnel, C++20 strict Release et Clang ASan/UBSan passent : 210 appels
et six paires de modes par oracle, 840 racines vérifiées, coquille 30. Les 66
mesures sont closes, lecteurs normal/−O identiques. Ce sont des racines
couvertes : positivité, canonisation et collecte des intérieurs restent à
raccorder. Aucun transfert de qualification vers le produit.

Sur 27 cas LiDAR séparés, domaine positif : 1 180 seeds, 263 lectures d'actives,
68 comparaisons, trois racines couvertes de faible profondeur. Mais sur le
dense permuté, W croît ×3,886/×4,072 : **W=Σ a_C²** ici, le carré demeure
dans les cellules. Ni un préfixe rapide ni la seule profondeur fixe ne closent
P0. Préparation scalaire par arête à remplacer par l'index/blocs partagés.

Environ 87 % des groupes denses positifs sont hors cellule. Une
[proposition vérifiée](q4_local_sweeps_20260920/CLIPPING.md) permet de les
retirer avant tri, en conservant leur contribution constante sur le segment.
Clipping validé en modèle rationnel seulement ; W reste inchangé, localisation
et temps total à mesurer avant tout port revendiquant un gain.

## Frontière Z : réponse à la question de continuation

`(cellule, compte, curseur non consommé)` suffit si tout le préfixe Z est
entièrement classé. Au split des centres, hériter le bloc ambigu sans le
consommer ; au split Z, le remplacer par ses enfants disjoints. Raffiner les
blocs à cheval sur le cover. Ne jamais créditer un parent puis ses descendants.

Si l'on saute des blocs ambigus, un seul curseur ne représente plus les trous :
frontière persistante partagée ou relecture du suffixe pour le repli exact.
Compter le graphe partagé, les pics et les scans répétés ; une grosse coquille
interdit une garantie uniforme de petites actives par simple raffinement.
Les détails et objets parallèles proposés sont dans le nouvel audit.

## Entretien

Les discussions précédentes, déjà reprises par le constructeur, sont retirées
de ce dialogue. Les [preuves de carte](q4_center_blocks_20260920/README.md),
[renforts collectifs](q34_collectif_20260920/README.md) et
[options LiDAR](front_options_lidar_20260920/README.md) restent épinglés et
référencés. Fichiers constructeur et autres auditeurs préservés.
Pas de réservation d’index en cours.
