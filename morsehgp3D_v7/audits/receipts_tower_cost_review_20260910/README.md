# Tour retenue : trois réductions de résidence à distinguer

Lecture de **d188e3de**, 10 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

La [tour retenue du constructeur](../../docs/TOUR_FULL_PAR_BOULES.md) livre désormais les dix forêts avec leurs verticales. Ce complément examine ses durées de vie et sa représentation ; la qualification mathématique du raccord reste dans le périmètre du second auditeur. Les [mesures constructeur](../../docs/RESULTATS_TOUR_BOULES_20260910.md) restent des observations non appariées ; aucun moteur ni GCP n’a été exécuté par cet audit.

## Résultat exploitable

| n, K1..10 régulier | Tableaux de travail libérables avant encodage | Métadonnées des brouillons, minimum | Records de naissance redondants, format futur |
| ---: | ---: | ---: | ---: |
| 8 000 | 147 535 504 octets | 508 310 576 octets | 192 371 680 octets |
| 16 000 | 309 882 152 octets | 1 062 206 992 octets | 402 112 160 octets |
| 32 000 | **642 249 936 octets** | **2 193 190 400 octets** | **830 477 120 octets** |

Ces colonnes concernent des objets et des phases différents : **ne pas les additionner pour annoncer un gain de pic RSS**. Elles comptent des éléments logiques avec l’ABI locale, sans capacités excédentaires, métadonnées d’allocateur ni fragmentation. Le pic publié de 10 559 316 KiB à 32k n’est pas localisé par phase. La borne des brouillons n’est pas une quantité intégralement supprimable : un encodage de remplacement a son propre coût.

## 1. Fermer les états de construction avant la banque

Après la boucle des ordres, dans `Builder::run`, le dernier `MonotoneHistory` a déjà été détruit. La création de banque et l’encodage ne lisent plus que `domain`, `populations` et `drafts`. On peut alors détruire `by_key`, `programs`, `population_ids`, `lower_anchors`, `compressed` et `lower_history`. Les brouillons possèdent leurs niveaux et identifiants par valeur : aucune référence vers ces tableaux n’y survit. Une portée dédiée est préférable à un simple `clear()`, qui conserve la capacité. `identity`, `stack` et `extra` sont également morts, mais restent hors chiffrage.

Noter B le nombre de census, A le nombre d’ancres programmées, N le nombre total de nœuds. Toutes les neuf histoires inférieures sont entièrement activées dans ces captures : leurs arêtes E donnent N10=N−E−9. Le volume des six états ci-dessus est donc :

$$V_{mathrm{mort}}=(4+8+8)B+4A+(48+8+8)N_{10}.$$

Le calcul retrouve N10=987 934 / 2 081 240 / 4 324 933. Cette libération ne modifie ni le contrat du journal ni les décisions géométriques. À vérifier lors de son implémentation : mêmes sorties et mêmes compteurs sémantiques, absence de référence survivante, résidence observée avant copie de banque et durant encodage. Elle ne promet pas une accélération temporelle.

## 2. Aplatir les brouillons avant d’optimiser leur stockage final

Le constructeur signale déjà que les brouillons restent simultanément en mémoire. Les captures permettent maintenant de borner leur seul coût d’en-têtes. Sur ce triplet sans extra-shell, une action publiée crée exactement un nœud : les connexions régulières n’apportent aucune contribution et leurs continuations inertes ne sont pas journalisées. Il y a donc N actions. Le lot initial K1 groupe n actions ; chaque autre lot de m boules peut économiser au plus m−1 en-têtes de batch. Avec L=`lot_dsu_slots` et G=`grouped_lots` :

$$B_{mathrm{draft}}geq N-n-(L-G)+1,\qquad V_{mathrm{entetes}}geq80B_{mathrm{draft}}+48N.$$

À 32k, cela impose au moins **17 114 695 batches**, en plus des 17 166 975 actions. Parents, références de contribution, images inférieures et allocations s’y ajoutent. Une représentation temporaire plate par lots, avec arènes de parents/contributions et offsets, retire l’imbrication de millions de vecteurs ; elle doit garder les frontières exactes des lots et leurs parents pré-lot. Son coût et ses copies doivent être mesurés, sans supposer que tout en-tête peut disparaître. La borne donnée ici n’est pas transposée telle quelle aux plateaux non réguliers avec continuations.

## 3. Une naissance peut porter directement sa population

Le contrat v2 impose une unique population entière pour une naissance. Son record daté répète donc des informations déjà connues : segment égal au nouveau nœud, date égale à son niveau, coquille entière et inclusion de tous les intérieurs présents. Une représentation future peut stocker l’identifiant de population dans le champ propre aux feuilles, et réserver les records datés aux contributions des continuations ou fusions. Le champ `first` pourrait avoir cette union de sens selon `parent_count`, comme dans d’autres formats de forêt ; **ce serait un nouveau contrat**, puisque v2 lui impose toujours un offset CSR.

Preuve de conservation : pour chaque racine et coupe, le lecteur développe les populations des feuilles ancestrales admises à cette coupe, puis les contributions ultérieures admises. Ce sont exactement les mêmes ensembles et les mêmes dates que dans v2. La projection en points reste une union par composante ; aucune identité n’est fusionnée par égalité de couverture. L’argument admet aussi une naissance de plateau couvrant plus de K points. Les contributions ultérieures doivent conserver leur date : les antidater à la naissance détruirait les coupes historiques.

Dans les captures régulières, toutes les contributions sont des naissances : supprimer leur record de 80 octets représente la troisième colonne. Ce n’est ni une modification livrée ni un gain mesuré. Qualifier ce futur format contre les coupes ouvertes/fermées v2, les plateaux et lots mixtes, et comparer un objet décodé canonique ; le digest dense actuel dépend de la représentation.

## Preuve et clôture du contrôle des parents

La sonde [layout.cpp](layout.cpp) ne construit aucune forêt. Sa compilation stricte O2 puis son exécution donnent : `ExactLevel=48`, `FullNode=64`, `FullDatedContribution=80`, `FullCoverageBatch=80`, `FullCoverageAction=48`. Les [captures](layout_capture/receipt.json) épinglent les dépendances avant/après, le compilateur, les commandes et le hash du binaire local non distribué. Les headers système ne sont pas archivés. [record_layout.py](record_layout.py) refuse d’écraser un essai existant.

Le [recalcul](verify.py) vérifie les deux manifestes constructeur, leurs octets, l’archive source et la fermeture des dépendances de la sonde ABI, puis produit [review.json](review.json). Il rejette quatre corruptions ciblées de compteurs après décodage ; ce ne sont pas des mutants du moteur ni des falsifications de sceaux. Les sources historiques sont relues dans les paquets constructeur, sans exiger que le worktree futur leur reste identique.

La [régression constructeur des parents](../../receipts/coverage_parent_array_20260910/README.md) ferme la demande de l’audit 514038ed : **837 contrôles O2/SAN**, carré K2 à quatre parents, comparaison des arènes depuis les actions, mutant `parent→0` refusé par `FAIL arena.parent_value` (code 1). Les vérificateurs constructeur normal/-O passent ; onze dépendances archivées correspondent aux trois inventaires de compilation. Notre [preuve historique](../receipts_coverage_cpp_20260910/README.md) reste intacte. Aucun défaut nominal du journal n’était établi.

```bash
python3 -B morsehgp3D_v7/audits/receipts_tower_cost_review_20260910/verify.py
python3 -B -O morsehgp3D_v7/audits/receipts_tower_cost_review_20260910/verify.py
```

Ce lecteur ne qualifie ni la complétude WSPD ni la géométrie de la grande tour. Les contrats 50k/1s, 100ms et le régime massif restent ouverts.
