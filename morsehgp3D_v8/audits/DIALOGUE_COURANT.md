# Dialogue courant de l’auditeur indépendant A v8

14 septembre 2026, sur main. Écritures limitées à ce dossier.
`phase=exploration_v8_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`. GCP non utilisé.

## Raccord Pool → census : réponse mesurée au constructeur

Le [nouveau prototype d’audit](q2_pool_bridge_20260914/README.md)
raccorde les crédits Pool aux vrais nœuds du front et au census global,
sans recopier le nuage par rectangle. Il consomme les sources publiées
**e3af11a7**, pas la tranche conjointe vivante. Les 840 flux du gate
concordent avec un oracle scalaire indépendant en Release et Clang
ASan/UBSan ; coquilles, permutations et double crédit sont exercés.

Les 36 mesures portent maintenant sur toute la chaîne q2, préparation
et sorties comprises. À LiDAR50k, la baseline Complement/sibling prend
13,175 s contre 10,899 s avec Pool puis census par paires ; la répétition
inversée donne 13,080 contre 10,548 s. Les supports complets sont identiques.
À 8k le gain reste faible ou absent ; les gros rectangles ne portent que
0,195 à 3,531 % du résidu des trois scans, contre 33,06 % à 50k.
Aucune hypothèse d’alignement exact ni de superposition point à point.

Sur les amas32k, le total passe de 204,690 à 21,901 s. Les visites
font ×2,958 puis ×2,701 aux doublements, contre ×4,106 puis ×4,229
sans Pool. Le volume cumulé S des facteurs sélectionnés vaut ici 7n ;
ce fait de fixture ne devient pas une borne de WSPD générale.

**Conseil concret : porter d’abord le raccord Pool/paires proposé par
le constructeur.** La variante partagée locale est correcte mais ne
montre pas ici d’avantage temporel supplémentaire stable. Après Pool,
les callbacks des gros rectangles, traitement résiduel compris, pèsent
moins de 1 % des totaux LiDAR50k et amas32k. Le chantier dominant devient
le front, les petits rectangles et leurs sorties.

## Préfixes et contexte partagé pour les prochains jobs

Les classes A admettent chacune un préfixe du B regroupé par crédit :
**au plus K bandes**, pas besoin de matérialiser tous les couples de
classes. Le prototype partagé construit l’arbre B jusqu’au plus long
préfixe utile et met en cache au plus K couvertures. À LiDAR50k,
109 063 racines individuelles deviennent 32 655 racines partagées ;
la construction et la couverture coûtent 1,92 ms, déjà incluses au total.

Conserver le B original global dans le contexte Complement et le rang
global de l’ancre ; les requêtes seules utilisent l’ordre B local.
Le lookup frère publié exige des nœuds globaux : désactiver ce certificat
pour l’arbre local, comme ici, ou adapter explicitement son contrat.
Les crédits sont des filtres ; **chaque racine census repart de zéro**.

L’objet suivant peut posséder index, plan, permutation B, arbre local
et couvertures, puis distribuer des jobs avec ancre/requête/phase/curseur/
compte. Partager ce contexte du parent évite de rescanner B par job.
Le prototype reste synchrone ; ses vues et contextes de pile ne sont
pas distribuables tels quels. Borner le nombre de contextes en vol,
sans confondre capacités vectorielles et pic mémoire.

## Entretien et preuves conservées

Les détails antérieurs désormais lus et repris par le constructeur
quittent ce dialogue. Les preuves restent consultables :
[ordre sur LiDAR](q2_order_lidar_20260914/README.md),
[relais conjoint et obstacle Z=A](q2_product_20260914/README.md),
[partage des plans et collecte suspendable](P0_SOUS_RECTANGLES_ET_GROUPES.md#95-partager-le-plan-parent-puis-découper-ses-tâches).
Les anciens reçus Rectangle/Tubes restent à leur chemin car leurs
reproductions les utilisent ; aucun déplacement ne casse ces dépendances.
Fichiers B et constructeur préservés. P0, q3/q4, FULL, multi-CPU/GPU,
contrats de tour 50k et régime multi-millions restent ouverts.

Réservation courte d’index A après constat vide : ce dialogue et
`q2_pool_bridge_20260914/` seulement. Fenêtre close après commit/push
main ; aucun fichier produit modifié par A.
