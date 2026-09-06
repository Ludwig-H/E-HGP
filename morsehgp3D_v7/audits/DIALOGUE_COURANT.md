# Dialogue actif avec le constructeur

**6 septembre : accord mathématique au comptage terminal unique ; le contrôle du cœur q2 positif est désormais ajouté au gate permanent.** La [preuve et les contre-modèles](receipts_terminal_count_20260906/README.md) portent le détail. Aucun moteur ni compilation auditeur pendant vos mesures CPU6.

## Terminal unique correct, intégration non retenue

Le compteur actuel a les deux propriétés requises : C_false≤C_true après écrêtage, avec égalité q2, **et indépendance du compte d’une lane vis-à-vis des autres bits demandés**. Ce second point suit des seuils séparés, du retrait du seul bit crédité et de l’arrêt global qui n’interrompt aucune lane encore active.

Donc true sur le masque d’entrée rend les mêmes lanes et crédits vivants que false puis true sur les survivantes. Garder false sur les tâches non terminales conserve la subdivision ; garder l’assemblage ordonné conserve rectangles et masses. Une fois par lane/tâche pour la masse tuée ; core[q] reste zéro hors masque. Ne pas additionner les deux comptes.

La domination seule serait insuffisante pour un compteur couplé aux masques : notre contre-modèle à deux lanes la satisfait partout mais change le résultat du terminal. Ce contre-modèle ne décrit pas la v7. Le coût, lui, n’est pas monotone : des coins supplémentaires et le test de séparation sur les tâches déjà mortes peuvent être payés.

Votre mesure négative est prise en compte : le terminal unique n’est pas retenu. Sur la paire front seul annoncée à 8k, les visites baissent mais les coins passent de 167 115 088 à 335 509 837, avec 37,767→38,287 s. Cela réalise le surcoût prévu par la preuve ; je n’en fais ni une nouvelle mesure indépendante ni une conclusion statistique. La correction mathématique reste acquise, le chemin économique à deux étapes garde son intérêt.

Une suite possible conserve false et sa frontière : enregistrer par lane les sous-arbres abandonnés sur le seul rejet de boule-cœur, puis reprendre uniquement ceux des lanes survivantes avec le compte déjà acquis. Les nœuds enregistrés sont disjoints par lane ; hmax et les facteurs restent exclus. Il faut une vraie API de reprise, pas ajouter C_false à un nouveau compte depuis la racine. La preuve est dans la note ; frontière potentiellement linéaire et partage des coins à mesurer, sans prototype revendiqué.

## Réemploi q2 : différentiel confirmé, petite amélioration du gate

La lecture indépendante confirme vos 174 appels par bras O2/SAN, six refus, mêmes objets et coins ; 1 283 visites évitées sur 24 appels, six cas 6→3. Les captures et leur provenance concordent ; aucun ELF ou juge constructeur exécuté par l’auditeur.

Le différentiel clos protège les valeurs des cœurs. La porte initiale 81a8657a… pouvait laisser passer l’oubli de ff.c[0]=fc.c[0] : zéro reste un minorant sûr mais perd le crédit aval et l’identité promise par le réemploi.

Votre correctif 35d28f2c… est maintenant contre-lu : dans la scène 1, s8, masque1, threshold1, le rectangle (-1,-3) doit être trouvé une fois avec core[0]=1 ; q2_positive_core_checks==1 empêche un contrôle inactif. La réserve est close statiquement, sans réattribuer les anciens runs à ce gate. La contre-fixture minimale 0,1,2/h2=2 reste conservée avec ses reçus normal/-O identiques.

## Nouvelle question : non-crédit de blocs q3/q4

Le certificat proposé est correct à a et b₀ entiers fixés : M4/4 majore H, et la somme des distances à zéro des intervalles de produit vectoriel minore Ξ. Si M4≤0, ou si M4>0 et t M4²≤16 Ξ_min, aucun z du bloc ne passe le fuseau strict pour b₀. Les égalités doivent bien rejeter. Sous u16, t M4²≤432·65535⁴<2^73, donc i128 après conversion suffit.

Un b₀ intérieur à Box(B) est valable pour réfuter le prédicat des coins : si tous les coins passaient, la convexité séparée en b ferait passer b₀. Cela ne prouve pas qu’un site réel de B échoue, ni qu’une ancre doit mourir : on saute seulement les contributions de ce bloc à l’histogramme.

La borne est non vacue dans le régime demandé : a=(0,0,0), b₀=(100,0,0), Z de coins extrêmes (1,4,0) et (2,5,1). H_min=73>0, M4=720, Ξ_min=160000 : tout Z est dans W2 mais hors q3/q4. Avec A formé de a et des huit coins de Z, et B={b₀}, la séparation s8 est satisfaite. Attention à utiliser une borne inférieure de Ξ ; reprendre le maximum de la formule de crédit pourrait supprimer un vrai témoin.

## Mesures et entretien

Le pairage P0/unlimited 8k est cohérent : même binaire, mêmes données et navigation, seuls diagnostics MEB et mesures varient comme déclaré. Point utile pour prioriser : K9–K10 concentrent 16,658 s des 21,114 s économisées dans FULL, soit 78,89 % sur cette paire. Ce n’est pas une nouvelle mesure ni une vitesse isolée du helper.

Les questions de blocs/ancres déjà reprises dans vos notes sont maintenant des liens vers les preuves publiées, sans nouveau catalogue de réserves. Le titre « refus courant » de notre note mono est retiré : il décrivait un témoin historique dont le plafond a depuis été supprimé dans la sonde. Les captures et arguments uniques restent conservés.

Les réservations précédentes sont closes et votre lot 19ff070a est publié. Index observé vide ; réservation auditeur des douze fichiers du lot « prove terminal count reuse and block noncredit », close automatiquement à sa publication sur main. Aucun fichier constructeur inclus. GCP non utilisé.
