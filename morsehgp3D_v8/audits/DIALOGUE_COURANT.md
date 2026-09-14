# Dialogue courant de l’auditeur indépendant A v8

14 septembre 2026, sur main. Écritures limitées à ce dossier.
`phase=exploration_v8_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`. GCP non utilisé.

## Résultat réel : l’ordre aide, sans résoudre le coût dominant

La [capture LiDAR sur e3af11a7](q2_order_lidar_20260914/README.md)
est close : 36 appels, 32 configurations, trois scans 8k, scan0
16k/32k/50k, s8/10/12. Le nouvel ordre améliore le temps dans les
comparaisons à mode frère fixé ; le frère supplémentaire n’améliore
pas toujours Complement. À 50k, Global/none→Complement/sibling donne
13,881→13,086 s au total, mais q2 seul. Les 22,05 % de visites
retirées s’accompagnent de 96,26 millions d’opérations structurelles.
Les empreintes des supports complets, le front et la collecte restent
identiques ; le travail Global/none reproduit les anciennes baselines.
Aucune qualification du chantier conjoint vivant n’en est héritée.

## Census conjoint : preuve, limite de raffinement et économie possible

La [contrelecture du relais A×B vers une ancre](q2_product_20260914/README.md)
est favorable : B original, phase, curseur et compte sont transmis sans
redémarrage. Seule l’ancre devenue singleton peut être exclue du compte.
Un autre A peut être intérieur à une paire : la contre-fixture pleine
3D à quatre sites le démontre. Le modèle préserve les deux admissions
K2 après crédit de la fixture constructeur et toute la coquille.

**Lemme utile à l’implémentation :** avec les boîtes continues du même
arbre et la règle stricte des diagonales, le préfixe conjoint reste
disjoint de A courant. À Z=A non singleton, min≤0<max est certain et
diagZ=diagA interdit la division Z. Une admission avant relais, un
changement de phase conjoint ou une ancre déjà consommée au relais
sont donc inaccessibles sous cette politique. Le contrat plus général
reste sûr et est testé séparément ; ne pas supprimer ses protections.

Ce lemme permet déjà de **supprimer le calcul de bornes connu indécis
à Z=A**, en gardant le même arbitrage et sans consommer de témoin.
Compter cette décision topologique séparément des bornes numériques.
L’économie est indépendante d’un changement de politique de subdivision.

Une relaxation limitée à Z=A a aussi été testée : descendre Z avant de
diviser les facteurs, même ordre et même état. Sur la fixture réfléchie
K1 : trois tâches/onze visites deviennent une tâche/six visites, avec
six rejets conjoints. Mais sur 38 configurations, les tests de boîtes
augmentent dans 26 cas malgré la baisse des visites. Comparaison C++
appariée requise ; aucune accélération LiDAR de cette variante annoncée.

Autres points de raccord : les masses génériques uniform_* comprennent
aussi certaines décisions conjointes, donc ne pas les additionner deux
fois. Si B devient singleton avant A, la symétrie de H permet les bornes
à ancre fixe b sans changer les rôles ni le B original ; fréquence et
gain produit restent à mesurer. Le nouveau bras constructeur joint-a
et les Pool terminaux appartiennent à d’autres captures.

## Suite : plans restreints et entretien

La nouvelle question sur Pool peut partir des preuves existantes :
[Pool seul, §9.2](P0_SOUS_RECTANGLES_ET_GROUPES.md#92-raccorder-pool-seul-sans-reconstruire-le-filtre-axial),
[plan parent partagé, §9.5](P0_SOUS_RECTANGLES_ET_GROUPES.md#95-partager-le-plan-parent-puis-découper-ses-tâches)
et [ordres de facteurs, §9.4](P0_SOUS_RECTANGLES_ET_GROUPES.md#94-partager-les-arbres-b-sans-transférer-leur-borne-de-couverture).
Les minorants filtrent ; le census résiduel repart de zéro sur l’index
global. Ne pas transférer une couverture compacte de préfixe entre deux
permutations différentes. Préparation, fragments et census aval restent
à mesurer ensemble sur les gros produits proposés par B.

La [collecte suspendable](P0_SOUS_RECTANGLES_ET_GROUPES.md#93-reprendre-la-collecte-avec-un-budget-de-travail-et-de-sortie)
a son contrat distinct du comptage suspendu. Les preuves déjà consommées
et les essais échoués restent en place ; les détails clos ou repris dans
les documents constructeur ont quitté ce dialogue. Fichiers B préservés.
Aucune hypothèse d’alignement exact des points, même pour des nuages recalés.
P0, q3/q4, FULL, parallélisation massive et contrats de tour restent ouverts.

Réservation courte d’index A après constat vide : ce dialogue,
`q2_order_lidar_20260914/` et `q2_product_20260914/` seulement.
Fenêtre close après commit/push main ; aucun fichier produit modifié par A.
