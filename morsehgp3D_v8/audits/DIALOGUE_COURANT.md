# Dialogue courant de l’auditeur indépendant A v8

14 septembre 2026, sur main. Écritures limitées à ce dossier.
`phase=exploration_v8_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`. GCP non utilisé.

## Résultat utile : le filtre paie son coût avec le census LiDAR

Le [raccord q2 sur LiDAR](q2_front_20260914/README.md) est mesuré sur un
snapshot figé, dont les seize sources produit/test correspondent au commit
constructeur f7edd646. Quinze appels
clos, dont le pilote à 8k dans les deux ordres d’exécution : Samples/Shared
1,668–2,128 s contre Pure/Shared 12,233–16,463 s, soit ×7,3–7,7 dans
les deux campagnes. Front, census, collecte et callback sont payés ;
les quatre modes rendent le même digest et les mêmes compteurs de sortie.
Cette comparaison garde le masque q2 identique, sans utiliser les temps
du précédent front trois voies comme référence.

La montée Samples/Shared donne 3,733 s à 16k, 7,582 s à 32k et
13,778 s à 50k sur le même scan. À 50k, 1 040 133 supports sont émis,
avec 579,8 millions de visites de comptage, 3,83 millions de démarrages
racine et 29,92 millions de tâches. La collecte/callback vaut 8,6 % du
temps intégré observé. Priorité mesurée : partager davantage le comptage
et réduire les recherches du proposeur, en suivant aussi le nombre de
tâches. Le résidu légèrement inférieur à s10/12 ne diminue pas ici
nettement le travail total. Aucune borne générale ni qualification G4.

La revue de code est favorable : nœuds B partagés, compte initial nul,
reprise conjointe compte/curseur Z, IDs originaux préservés, coquille
complète. Le gate du snapshot passe 1 255 appels intégrés ; l’adaptateur
concorde avec un calcul indépendant sur 496 paires. Les erreurs et leurs
correctifs de harnais sont conservés, sans rejouer ni réécrire les reçus
antérieurs. Les six campagnes passent les lecteurs normal/−O.

## Prochain objet à confronter aux mesures

Les preuves de [§9–9.1](P0_SOUS_RECTANGLES_ET_GROUPES.md#9-census-q2--des-extrema-exacts-pour-partager-les-recherches)
couvrent déjà A×B×Z et les divisions de A/B. Le complément de port C++
est précisé dans la nouvelle note : 96 octets de constantes par activation,
file portant trois IDs et un compte avec index/seuil au niveau du batch,
et repli vers le parcours actuel dès qu’un facteur devient singleton.
Le coût de préparation après suspension doit rester explicite.

L’ordre Z peut provoquer un partage de B avant un témoin commun utile :
une fixture à quatre points en pleine dimension, conservée dans ce dossier,
le vérifie par modèle après réflexion. Tester d’abord le certificat
autonome saturant du frère proposé par le constructeur. Pour conserver
un crédit partiel, le modèle vérifie un état avec un bloc E explicitement
exclu du comptage ultérieur ; son mutant sans exclusion est rejeté.
Ces mécanismes ne sont pas encore mesurés sur amas ou scans réels.

Les [jobs d’un plan parent, §9.5](P0_SOUS_RECTANGLES_ET_GROUPES.md#95-partager-le-plan-parent-puis-découper-ses-tâches)
et la [collecte suspendable, §9.3](P0_SOUS_RECTANGLES_ET_GROUPES.md#93-reprendre-la-collecte-avec-un-budget-de-travail-et-de-sortie)
restent des propositions distinctes du produit. La correction I1 de B
à 1ca8f62d clôt la réserve sur l’inertie globale des boules ; son ancien
développement a été retiré du dialogue.

## Entrées et entretien

Aucune hypothèse d’alignement des points, même pour des nuages recalés.
Les [entrées réelles et leur provenance](lidar08_20260914/README.md)
restent disponibles : trois scans isolés primaires, grille isotrope fixe
2 cm, sites uniques et correspondances avec les retours bruts. Le contrôle
d’accumulation voisin reste secondaire. Les mesures anciennes du front
restent épinglées et distinctes de celles du raccord.

Les points corrigés et documentés par le constructeur ont quitté ce
dialogue. Les preuves encore consommées sont conservées ; les fichiers
et chantiers des autres auditeurs restent à leurs propriétaires.
P0, q3/q4, FULL, parallélisation massive et contrats de tour restent ouverts.
Réservation d’index A après publication constructeur f7edd646, index
constaté vide : ce dialogue et `q2_front_20260914/` uniquement.
`.build/` et `.snapshot/` restent ignorés. Fenêtre close au commit/push
main ; aucun fichier des autres intervenants inclus.
