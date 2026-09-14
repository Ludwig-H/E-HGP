# Dialogue courant de l’auditeur indépendant A v8

14 septembre 2026, sources produit auditées **da366f7f**, sur main.
Écritures limitées à ce dossier. `phase=exploration_v8_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

## Priorité utilisateur : aucune hypothèse d’alignement des points

Des nuages correctement recalés gardent un échantillonnage irrégulier.
Les colonnes ou lignes exactes ne peuvent donc pas conditionner le
chemin général ni ses performances. La précision de l’utilisateur ne
demandait pas de faire de SemanticKITTI un benchmark de recalage.

L’[audit LiDAR réel](lidar08_20260914/README.md) fournit trois scans isolés
primaires de KITTI 08 et 36 mesures closes du front général. À 50k sites
uniques quantifiés, Kmax=10 et s=8, MidpointSamples élimine 96,93–98,67 %
des paires q2 sans filtre axial. Le front avec callback coûte cependant
7,810–10,476 s en mono, contre 0,777–1,136 s en Pure ; il paie
177–197 millions de pas de recherche. Aucun gain de chaîne complète
n’est acquis sans census. Le contrôle de cinq scans voisins reste
secondaire et ne qualifie aucun recalage de captures indépendantes.

Suite utile au constructeur : mesurer le raccord direct des nœuds WSPD
au census sur ces mêmes fichiers, coût front + census + collecte compris,
avec s8 en référence appariée et s10/12 conservés. Les données locales,
matrices, collisions de quantification, correspondances et hashes sont
disponibles ; le dépôt ne contient pas les scans bruts. La grille isotrope
2 cm définit un ensemble de sites distinct de celui des retours bruts.
Cette première capture ne couvre ni toute la diversité LiDAR ni le massif.
La capture actuelle active q2/q3/q4 ; le raccord q2 seul annoncé ensuite
appelle une comparaison des modes à masque q2 identique, avec de nouveaux
reçus. Le coût des branches Xi retirées ne doit pas être attribué à un
meilleur partage du census.

Le [snapshot et son gate](lidar08_20260914/BUILD.json) sont clos ; ses seize
fichiers produit correspondent au commit publié. Les travaux constructeur
ultérieurs sur le raccord restent à auditer sur leurs propres sources.
Les 36 lignes passent les lecteurs normal/−O ; les 18 contrôles de
préparation passent également. Les limites et erreurs d’invocation sont
conservées dans les reçus. GCP non utilisé.

## Propositions encore distinctes du produit

La [répartition du plan parent, §9.5](P0_SOUS_RECTANGLES_ET_GROUPES.md#95-partager-le-plan-parent-puis-découper-ses-tâches)
reste disponible pour une future API de jobs : rangs A disjoints, même
ordre B emprunté, témoins du parent conservés. Le probe et son reçu
Release/UBSan couvrent 108 plans, 288 répartitions et 756 jobs. Cette preuve
ne regroupe pas une WSPD entière en parents admissibles ; elle interdit
aussi d’additionner sans exclusion crédit parental et nouveau cœur enfant.
La [collecte suspendable, §9.3](P0_SOUS_RECTANGLES_ET_GROUPES.md#93-reprendre-la-collecte-avec-un-budget-de-travail-et-de-sortie)
reste une proposition pour les continuations.

Contrelecture à terminer par B dans sa note des verrous, §2 :
`h_q ≤ h_qmin` rend le rejet sûr **pour les supports de la lane q**,
sans rendre nécessairement la boule inerte. Son exemple qmin=2,
p=Kmax−1 élimine q3 et conserve q2. La complétude globale passe par
la lane minimale et la rétention d’une clé dès qu’une présentation
pertinente la conserve. Le constructeur respecte déjà cette distinction ;
les fichiers de B restent à leur propriétaire.

Les défauts de propriétaire, de lien/IPO, le risque d’ordre B et la
spécialisation de `terrain` sont désormais documentés par le constructeur ;
leurs développements ont quitté ce dialogue. Le modèle axial préliminaire
et son reçu redondants ont été supprimés au commit précédent ; §8 conserve
l’ancre et la fixture citées par le produit. Les propositions de fenêtres
A/B et les preuves encore épinglées restent en place. Le
[dialogue de B](DIALOGUE_AUDITEUR_B.md) garde son étude propre du front.

P0, q3/q4, FULL, parallélisation massive et contrat de tour 50k/G4 restent
ouverts. La publication présente reste limitée à ce dialogue et au dossier
`lidar08_20260914/` ; les fichiers des autres intervenants sont exclus.

Réservation d’index A après da366f7f, index constaté vide : ce dialogue
et les sources, reçus et métadonnées de `lidar08_20260914/` uniquement.
Les répertoires ignorés data/prepared/.build/.snapshot restent locaux.
Fenêtre close au commit/push main ; aucun fichier constructeur ou B inclus.
