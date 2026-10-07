# CUDA — réponse sur les feuilles réécrites

**Le prototype actif corrige le compteur : `leaves_rewritten` reçoit désormais les réécritures sur appareil et sur
hôte.** Lecture statique du commit local `66c41ede461dcc7a235a2414079c916841c4fb46`, encore non publié sur main,
observé à `2d3f91983ef37f2a9f6667a50cda962288f22a5c`. Le rapport du développeur indique la nouvelle copie active ;
l'ancienne copie au corps CUDA `5e215fe2…` n'était plus celle du chantier. Aucun patch concurrent n'est proposé.
Capture stable par double lecture, 16 épingles dans `capture.json`.

Cadre : exploration v12 hors registre, catalogue appareil hybride / CPU de référence, FULL pi0, u21 ;
`public_status=not_claimed`. Aucun test natif, modèle supplémentaire, compilation CUDA, mesure ou opération GCP.

## Compteur maintenant raccordé

- `ReduceView` reçoit `a.balls`, unique initialisation positionnelle recensée dans `src`, `tests`, `bench` et
  `microbancs`. Aucun initialiseur positionnel de `BatchStats` ; ses trois constructions utilisent `{}`.
- `ReduceTileKernel` compte les feuilles `cls.cause == 0 && balls > kCase`. C'est exactement la condition d'entrée
  de `ReplayKernel`. `CountKernel` met les boules à zéro en cas de refus ; une faute de feuille ou d'écriture empêche
  la publication. `ReduceTopKernel` additionne `BatchStats.rewritten` sur toutes les tuiles.
- Le pilote cumule ce total dans `rewritten_device`, puis ajoute **`stage.totals().rewritten`** dans
  `rewritten_host`. Il ne déduit pas les réécritures du nombre de feuilles non résolues. La somme est publiée dans
  `leaves_rewritten`, avec les deux composantes séparées dans la sonde.
- `replayed_leaves` reste le nombre de feuilles reprises sur CPU. `replayed_wide` et `replayed_span` décrivent les
  deux causes, éventuellement simultanées : leur somme peut dépasser `replayed_leaves`. Les feuilles reprises
  mais sans émission ne comptent pas comme réécrites.

Les agrégats restent propres à l'appel et les additions aux lots successifs. La capacité des tableaux de statistiques
utilise `sizeof(BatchStats)` via l'allocateur typé, donc intègre le nouveau champ. Par lot, le nouveau total ne peut
dépasser le nombre de feuilles (au plus `2^17`) ; au succès complet, chaque réécriture produit au moins une boule et
le catalogue exige un total de boules sous la sentinelle u32. La somme publiée n'introduit pas de débordement u64
dans ce domaine. Ces compteurs physiques ne réinjectent aucun calcul de Replay dans les compteurs logiques J3.

**Précision documentaire à faire :** les commentaires de `device_driver.hpp`, `catalogue.hpp` et
`device_pipeline_test.cpp` assimilent encore toutes les réécritures à « plus de 64 émissions ». Cela décrit
l'appareil et les feuilles hôte de largeur 32. Dans `LeafStage::fill_body`, une feuille hôte de **plus de 32 sites**
est réécrite dès qu'elle émet une boule, même si elle en émet au plus 64 ; son comptage initial ne garde pas les cases
d'émission. Le code nouveau agrège correctement ce compteur existant. La définition commune est « seconde
exécution d'émission de la feuille », avec ces conditions propres aux voies, et non « reprise CPU ».

## Mémoire et raccord au socle

`device_cuda.cu` passe de `5e215fe2…` à `7a8cfe35…`. Les corps `grow`, `ensure`, `ensure_keep` et `stage` sont
**identiques** entre ces deux captures : vérification textuelle exacte, empreinte du bloc dans `capture.json`.
Le nouveau delta porte sur les synchronisations et la mesure des copies. Les deux nettoyages déjà contre-lus restent
donc présents : libération de `fresh` sur échec de copie/synchronisation, restitution de la réservation épinglée sur
échec d'allocation. Les libérations CUDA restent best effort ; aucune injection de panne ni qualification mémoire
générale n'est acquise par cette lecture. Les nouveaux chronomètres de transfert ont une portée d'audit distincte.

Le `buffer.cpp` du prototype correspond au corps qualifié `5c885dbf…`. Son en-tête `8e4d8466…` est exactement celui
de main `3c7fffc6…`, augmenté de `BudgetReservation::swap`, qui échange ensemble le compte et les octets. Il n'ajoute
ni acquisition ni libération durant l'échange. L'intégration doit conserver le correctif du cache publié et ajouter
ce déplacement explicitement, puis fusionner les fichiers communs avec les autres chantiers, notamment T/M/V.
La qualification du prototype ne se transfère pas automatiquement au résultat de cette fusion.

## Portes disponibles et portée restante

`pipeline_witnesses` vérifie les nouveaux diagnostics contre ceux de la vraie voie CPU sur ses neuf témoins, avec
égalité du total, de sa décomposition, des paliers et des causes. Deux planchers exigent des réécritures non nulles
sur appareil simulé et sur hôte ; `coquille48` exige une contribution hôte. Deux mutants annulent respectivement
l'accumulation appareil et hôte. Ces sources ont été lues, **pas exécutées par cet audit**.

Le rapport développeur revendique 146 contrôles et des reprises locales avec l'exécuteur Pool, dont plusieurs lots.
Ce sont des résultats de ce rapport, pas une contre-épreuve GPU. La porte `device_open` actuelle compare encore les
catalogues et les répétitions, sans demander les diagnostics : étendre cette comparaison aux nouveaux champs sur
appareil réel, puis rejouer les portes concernées sur le corps intégré avant une mesure qualifiante. Les tests doivent
couvrir les deux chemins et la remise à zéro entre appels ; conserver aussi les cas de refus. Aucun modèle mimant
la seule formule du compteur n'a été ajouté.

Principaux pins : feuilles `4131c38c…`, pilote `e87f947f…`, pipeline `4a752fb7…`, CUDA `7a8cfe35…`.
Ce reçu actualise l'état de la correction ; il ne ferme ni le contrat G4/FULL, ni la conformité des transferts,
ni un constat global de mémoire. `sha256sum -c SHA256SUMS` vérifie les fichiers du reçu.
