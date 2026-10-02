# Ouverture v11 : trois contrats utiles avant les fondations

2 octobre 2026, commit **52687f8e532db49c9d7334b49111763a72246e1c**. Quatre documents d'ouverture lus et figés ; aucun code Session/Buffer/IO ou moteur présent lors de cette revue. Il s'agit de **décisions d'architecture à préciser**, pas de défauts d'implémentation reproduits. Aucun build, calcul massif, GCP/GPU, source ou document développeur modifié.

## 1. Session unique : portée du budget et durée de vie

[ARCHITECTURE, lignes 15–16 et 24–25](sources/morsehgp3D_v11/docs/ARCHITECTURE.md#L15) impose l'unique budget/Pool et tous les grands tableaux en Buffer. C'est la bonne correction au budget partiel de v10. **L'unicité seule ne ferme pas la résidence.**

Le contrat devrait couvrir capacités des petits états locaux cumulées par worker, agrandissement d'un Buffer (ancien+nouveau pendant copie), scratch de tri/fusion, staging IO et résultats simultanés. Définir « petit » par une borne ou réserver ces capacités ; `vector` hors boucle chaude ne limite pas sa taille. Les candidats K-NN restent dépendants des ambiguïtés/ex æquo, pas seulement de K : la sonde v10 séparée du 2 octobre collecte 144 candidats pour K=1 puis conserve les capacités. Ce résultat borné n'est ni une panne mémoire ni une extrapolation LiDAR/v11.

**Décision nécessaire pour l'API :** préciser si un résultat peut vivre après sa Session. Un Buffer qui rend ses octets au MemoryBudget emprunté à sa destruction exige que ce budget soit encore vivant. Soit le résultat garde le propriétaire comptable en vie, soit la Session doit survivre à tous ses résultats/vues avec une précondition publique vérifiée. Une seconde opération conserve-t-elle le premier résultat ? Si oui, son coût reste réservé. Le contrat transactionnel doit conserver ce premier résultat sur refus, tout en réservant le nouveau pic avant travail.

Recommandation : état de budget propriétaire commun, réservations par phase et scratch compté par worker, avec exceptions d'allocation traduites à la frontière après fermeture/join des tâches. Mesurer les capacités simultanées, pas seulement le payload final ou un pic par worker pris isolément. La somme des pics de phases différents ne représente pas un pic simultané.

## 2. IO transactionnelle : résultat public, staging privé et reprise

[ARCHITECTURE, lignes 17–19](sources/morsehgp3D_v11/docs/ARCHITECTURE.md#L17) demande résultat entier ou refus ; [table des modules, ligne 41](sources/morsehgp3D_v11/docs/ARCHITECTURE.md#L41) annonce des sorties transactionnelles. Cela ne choisit pas encore **la portée de la transaction** : opération en mémoire, ensemble de fichiers d'une tour, ou Session entière.

Pour le massif, proposer des segments/checkpoints privés durables puis **un manifeste de commit unique** qui rend public l'ensemble validé. Chaque fichier final appartient à une génération immuable ; le lecteur n'ouvre que la génération référencée par le commit. Un refus ne remplace pas le résultat public précédent et ne rend pas visible un plateau partiel. Poser le modèle de crash : atomicité de visibilité seule ou durabilité après panne ; rename seul ne garantit pas toute la durabilité ni la cohérence de plusieurs fichiers.

La frontière des segments est celle des boîtes de centres certifiées et des tâches à émission unique, avec recherche de voisins globale et I/U complets. La fusion de niveaux et la clôture des plateaux restent globales. Un halo spatial fixe n'est pas une alternative exacte. Une sortie FULL disque exige en outre l'accès aux descentes/unions, atlas et verticales ; produire le spool catalogue ne clôt pas ces couches.

Recommandation : réserver RAM **et disque**, bornes de fichiers ouverts et volumes de lecture/écriture par phase ; déclarer le contenu validé d'un checkpoint (hashes, profil/repère, ordres, tâches/segments couverts) et ce qui sera refait à la reprise. Ce contrat autorise des checkpoints privés sans publier un préfixe FULL. Définir si 100 ms comprend préparation, fusion/flush/export durable et projection/tête : même objet, périmètres chronométrés distincts.

## 3. Multi-profils : format commun et domaines d'indices

[ARCHITECTURE, lignes 57–68](sources/morsehgp3D_v11/docs/ARCHITECTURE.md#L57) distingue B=18 par défaut, compilations 21/24 et qualifications par profil ; PointId u32 arbitraire unique, multiplicités et niveaux exacts non réduits. Ces distinctions sont correctes. Compiler les mêmes sources n'établit pas un profil géométrique qualifié.

**Deux contrats à relier tôt :** (a) le format de nuage/retours et son repère ; (b) la représentation canonique des événements/sorties. Le manifeste doit fixer h rationnel, unité, origine commune exacte, domaine B déclaré, règle d'arrondi, hashes, retours→sites et fusions. B ne fixe pas h. Garder β en cellules² et sa conversion physique h² ; ne pas changer le pas ou l'origine par segment pour faire entrer les données dans un profil.

Coordonnées u32, PointId et indices/offsets de boules, incidences, niveaux, cellules et nœuds sont des domaines séparés. 10–50 M points rentrent dans un rang dense u32 ; cela ne dimensionne pas B/P ni leurs offsets. Fixer les types forts **et leurs maxima/sentinelles**, contrôles avant casts et encodage de fichier ; conserver les gardes mathématiques amont de la forêt, sans prétendre une corruption géométrique ExtCell déjà prouvée. Pour agréger des scans, préciser l'identité externe (par exemple source+ordinal) et sa carte vers PointId global unique.

Un Level non réduit est exact, mais n'est pas à lui seul un encodage canonique indépendant du constructeur ou du profil : des couples N/D différents peuvent représenter la même date. Choisir un encodage exact déterministe, une définition d'égalité par produits croisés et un rang global. La vue double doit rester séparée des plateaux FULL, de la condensation par cohortes et des incidences de frontière. Déclarer l'objet différentiel byte-identical comparé à v10 et le périmètre des futures sorties communes entre profils.

Recommandation : conserver u18 comme seule voie moteur qualifiée tant que 21/24 n'ont pas leurs portes conjointes identité, centre/support positif, prédicats, niveaux, filtres/K-NN, attaches et IO. L'objectif plein u32 demandé auparavant doit rester une étape explicite distincte de u24 ; l'ouverture ne le promet pas encore. Déclarer aussi le traitement FULL des multiplicités : leur conservation par cloud n'établit pas la sémantique de tour, que v10 refuse actuellement. Un retour/site dédupliqué puis rediffusé ne récupère pas la densité perdue.

## Portée et preuve

Les trois recommandations complètent des règles déjà favorables, sans imposer une autre géométrie ou une réécriture du moteur. Aucune qualification v10 n'est héritée. La provenance R2 annoncée doit rester un port vérifié fichier par fichier ; cette revue ne valide ni sa série privée ni les chiffres de portes annoncés.

[SOURCE_BEFORE.json](SOURCE_BEFORE.json) / [SOURCE_AFTER.json](SOURCE_AFTER.json) donnent les hashes des quatre documents et leurs blobs au commit d'ouverture. [SHA256SUMS](SHA256SUMS) ferme la capture ; le lecteur [judge.py](judge.py) vérifie les quatre textes et le manifeste, normal/−O, sans compiler ni charger de moteur. La fermeture documentaire n'est pas une preuve fonctionnelle v11.
