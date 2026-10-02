# Cloud immuable : contre-lecture après qualification G4

2 octobre2026. Sources extraites de **6a22a9118a9dc692d3b5a34b8fd97673746697c4** avant lecture, puis dépendances/documents/preuves G4 supplémentaires capturés depuis ce même commit. Audit statique et lecture d'archives : aucun produit compilé ou exécuté localement, aucune nouvelle session GCP. Le premier reçu cloud_contract_review_2 reste intact.

## Réserves précédentes fermées

[Cloud](sources/morsehgp3D_v11/src/cloud/cloud.hpp#L56) possède maintenant un stockage privé, ne donne que des spans const, interdit copie et affectations, et autorise un déplacement sans allocation qui vide l'ancien propriétaire. Les entrées doivent explicitement rester stables pendant l'appel synchrone (104–105) ; elles ne sont plus empruntées après retour. Le refus restitue les réservations propres de l'appel (101), le pic ajoute les octets déjà vivants et conserve l'historique (111–112). [Record](sources/morsehgp3D_v11/src/cloud/cloud.cpp#L29) garde aussi son sizeof par static_assert. Aucune anomalie nouvelle atteinte n'est trouvée dans ces contrats.

Les IDs arbitraires et uniques, dont UINT32_MAX, restent distincts des indices denses. La préparation conserve tous les IDs et les multiplicités en CSR u64 ; ni permutation d'entrée ni renumérotation injective des IDs ne modifie les positions/sites/poids. Les clés Morton portent54/63/72bits aux profils18/21/24. Le helper32 n'est pas une qualification d'un profil natif u32.

## Preuves G4 réellement relues

[G4_CLOUD_EVIDENCE.json](G4_CLOUD_EVIDENCE.json) compare les **19 fichiers src/core, src/cloud et tests/cloud** à la source exécutée `a971806679a1c68519249bb28c5dac9533a43f59` : tous sont identiques. Les helpers de harnais ne changent pas non plus dans le diff ciblé ; leur audit général relève de l'auditeur principal. Cette recoupe ne transfère pas les succès à de nouvelles modifications.

L'archive reprise3 a le SHA256 **dd6b96811a7d389833b5e720a372f08508eb81ff1b4a1f157fb676c67c3b4260**, exactement celui de receipt.json. Les JUnit archivés contiennent **20 portes cloud conformes** dans chacune des configurations Release, ASan/UBSan, TSan, B21, B24 et poison, sans échec ni saut cloud. Les sorties conservées montrent notamment ownership27 contrôles, shared_budget11, starvation9, et cinq refus à la compilation pour largeur/API. La campagne de mutants cloud et ses deux lecteurs figurent également dans la configuration mutants. Clang est absent, facultatif et non qualifié.

Ces constats relisent des tests CPU exécutés sur G4 : aucun LiDAR, GPU, catalogue ou FULL, et aucun chrono de performance isolé. Le lecteur développeur check.py demande en plus des bruts locaux LIVE ; nous ne prétendons pas requalifier toute sa chaîne de collecte. La capsule conserve l'archive publiée, ses JUnit extraits et la source identique, pas une nouvelle exécution. Une première erreur de notre lecteur supposait un JUnit pour Clang absent ; sa version et son échec sont conservés. Cette erreur n'est pas un échec produit.

## Mémoire : formule et dernier contrôle utile

Avec n retours, s sites, R=16/16/32 et H=65 536/73 728/81 920 selon B18/21/24, le pic propre est **F=max(2Rn+H, Rn+4n+24s+8)**. Pour U stable déjà réservé : **peak_final=max(peak_initial,U+F)**. Quatre buffers d'entrée vivants ajoutent16n ; pour s=n : B18/21 donnent max(48n+H,60n+8), B24 donne80n+81920. Ce sont des octets réservés, pas le RSS ; le régime massif reste hors contrat v11 actuel.

Le test [shared_budget](sources/morsehgp3D_v11/tests/cloud/cloud_test.cpp#L590) prouve maintenant qu'un refus préserve les allocations préexistantes et le premier Cloud, puis que deux Cloud peuvent coexister. Il ne compare pas le pic absolu à max(ancien_pic,U+F). Une porte complémentaire utile sur G4 utiliserait les quatre buffers d'entrée dans le même budget et vérifierait cette égalité, puis un historique où ancien_pic>U+F. Cela ferme la liaison entre le modèle et l'entrée réelle sans invalider les tests actuels.

## Avant le port index/catalogue

Deux obligations concrètes méritent d'être posées au contrat du futur code :

1. **Identité du propriétaire emprunté.** Les spans restent valides après move, mais un `const Cloud*` vers l'objet source ne suit pas le transfert : ce Cloud devient vide. Un index doit emprunter un propriétaire à adresse stable, ou interdire son déplacement pendant l'emprunt, ou transporter un handle propriétaire immuable. Les durées de vie explicites exigées par CONCEPTION:57–59 vont dans le bon sens ; ce n'est pas un défaut d'un index déjà livré.
2. **Deux passes de catalogue.** CONCEPTION:100–102/120 impose count puis fill déterministes. Fixer le même propriétaire, les paramètres et la règle d'émission aux deux passes ; borner additions et préfixes B/I/U/L en u64, vérifier les plafonds d'IDs avant conversion/allocation, et contrôler les fins de remplissage contre les capacités comptées. Le pic doit comprendre comptes, résultat, permutation/tri et construction des rangs lorsqu'ils coexistent. Deux passes suppriment la croissance extensible, pas ces coexistences ni leur travail géométrique. Aucun catalogue v11 n'est jugé ici.

Le futur index global ne peut réutiliser une liste certifiée seulement pour les centres d'une feuille quand une descente MEB sort de cette boîte ; CONCEPTION:150–154 distingue correctement témoin saturant et vrais K plus proches. Les coordonnées seules de Cloud ne portent pas h/origine/hash : le futur raccord IO doit conserver ce descripteur avec la correspondance des retours, dans le même repère physique.

Les snapshots, lectures de preuves, premières erreurs, normal/−O du lecteur de capsule et SHA256SUMS ferment ce reçu. Aucun ancien reçu, produit ou note active n'est modifié.
