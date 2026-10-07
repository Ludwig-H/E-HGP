# Table S* compacte : mémoire et raccord CPU/CUDA

Lecture avant fusion, exploration v12 CPU/u21/FULL, `public_status=not_claimed`. Aucun natif, CUDA ou GCP exécuté.
Deux corps distincts : G-c `repo2` pour la nouvelle table et GPU `repo99` au pin
`3b445b763057adc405141a6650efa14bfd4e276a` pour la finition partagée. Hashes dans [capture.json](capture.json).

**Conclusion :** la nouvelle table est exacte sur les supports valides et son allocation CPU est comptée. Son type
n'est pas encore raccordé à la sortie de la finition partagée CPU/CUDA. La fusion doit matérialiser les nouvelles
entrées, en comptant leur pic de coexistence ; ce n'est pas une modification interchangeable du seul en-tête.

## Mémoire effectivement payée

Dans G-c, `SupportEntry` contient trois SiteIdx et un BallIdx, tous fondés sur u32 : entrée de 16 octets au lieu de 4.
Pour E boules et n sites : table finale `16E + 8(n+1)`, contre `4E + 8(n+1)` ; delta brut **12E**. Le curseur temporaire
de construction reste `8n`. `table.cpp:28` alloue un `Buffer<SupportEntry>` : `Buffer::allocate` réserve
`E*sizeof(SupportEntry)`, pas une ancienne formule à 4E. Aucune formule globale du catalogue conservée à 4E trouvée
dans les sources lues. Les refus peuvent survenir pendant les allocations, comme auparavant : ce n'est pas une
preuve nouvelle d'admission de toute la finition avant son calcul.

Avec cache actif, `buffer_acquire` réserve la capacité physique de classe, ou la taille exacte si la classe ne tient
pas. `used/peak` voient le bloc vivant ; les blocs inactifs retenus restent dans `held`, sous la même limite.
Le delta réel du cache n'est donc pas exactement 12E. La sonde G-c lue crée `MemoryBudget(o.budget)` sans cache.
La table reste un contenu calculé propre au catalogue ; réutiliser une capacité n'amortit pas son remplissage entre trames.

## Raccord minimal à la finition partagée

GPU `finish_driver.hpp:50–57` rend encore `Buffer<BallIdx> table_values`. Ligne 299, `take_cast` copie des u32 vers ces
indices de même taille. `assemble.cpp:93–94` échange directement ce buffer avec la table du catalogue. Après passage
de celle-ci à `Csr<SupportEntry>`, cet échange entre buffers de types différents n'est plus possible ; remplacer
seulement le type de sortie ferait aussi échouer la condition `sizeof(T)==sizeof(U)` de `take_cast`.

Une option simple conserve le tri commun existant et ses sorties 4E :

1. Les boules canoniques et les indices triés sont déjà présents sur l'hôte après `finish_stage`.
2. Allouer dans le même budget un buffer 16E, remplir chaque entrée avec les trois mots de queue de
   `out.balls[idx(out.table_values[i])]` et son BallIdx ; conserver les décalages de ligne.
3. Vérifier les indices et la monotonie stricte des queues, terminer tout travail susceptible de refuser, puis
   publier par échange et libérer la liste d'indices.

Aucun transfert supplémentaire de queues n'est nécessaire pour cette option. Les sentinelles de tri GPU utilisent
`sites` et les queues stockées utilisent `kNone` : leur ordre est équivalent sur les IDs valides `<sites` ; copier
les queues depuis les boules, pas depuis les clés compactées du tri.

**Pic local de conversion :** 4E anciens indices +16E nouvelles entrées =20E simultanés, soit **+16E au pic** par
rapport à l'ancienne sortie hôte, pour +12E à l'état final. S'y ajoutent les tableaux du contexte GPU déjà retenus,
les autres sorties et les arrondis du cache. Admettre 16E avant cette allocation tient compte des 4E déjà vivants.
La materialisation doit rester dans C et dans ses diagnostics, avec une frontière disjointe : `adopt` est déjà
chronométré dans la publication appareil et dans l'assemblage CPU. Aucun coût ne disparaît en le déplaçant de table
vers publication. La mesure G seule ne peut juger cette variante ; sa préparation est payée en C.

## Exactitude et refus

Dans G-c, la ligne CSR fixe le premier SiteIdx ; les trois queues complétées par `kNone` distinguent exactement les
supports de cardinal 2/3/4 sur le domaine valide. Les supports répétés deviennent voisins et sont refusés après tri.
La recherche ne reconnaît que S* : une autre diagonale minimale du carré reste absente, comme exigé par LEM-T1.
`build_table` construit `made` et n'échange ses buffers qu'après succès ; un refus conserve donc la table passée et
détruit les allocations temporaires. `Assembly::finish` ne rend le catalogue qu'après cette construction complète.

Limite publique **préexistante** : les deux anciennes/nouvelles recherches bornent seulement le premier ID.
La requête invalide `[a,b,kNone]` a le même padding que `[a,b]` et peut l'aliaser. Cela ne montre pas de défaut dans
LEM-T1, qui exige un support inclus dans une partie de sites valides. Une garde de tous les IDs `<sites` rendrait
la frontière publique stricte sans changer les recherches valides ; ce n'est pas une régression du nouveau format.

Pas de requalification des tests du catalogue, du cache ni de la finition par cette lecture. Avant fusion, le test
commun de finition doit comparer les queues et les BallIdx, puis les identités catalogue et le refus mémoire sur
les deux exécuteurs. Les chiffres de mémoire ci-dessus sont des comptes de disposition, pas des mesures de RSS.
