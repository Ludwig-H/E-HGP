# Proposition bornée : répartir le fill séquentiel entre CTA

Patch proposé uniquement, basé sur `origin/main` publié `830473218e1b35fa725209df14b7855667f4c6d4` (L4 retiré). La copie source entière est figée par empreinte ; aucune source développeur n'a été éditée. Aucun build, natif, CUDA ou cloud n'a été exécuté. `git apply --check` accepte le patch sans l'appliquer.

Le `fill_kernel` actuel met une feuille par fil et lance ceil(fill_jobs/32) blocs : pour 14 positions, un bloc ; pour 4196, 132 blocs. L'essai proposé garde `leaf_device::run_leaf` séquentiel et place un seul fil actif par bloc. Pour une limite de grille au moins égale au nombre de positions, il lance 14 ou 4196 blocs distincts et lit `list[blockIdx.x]`. Cela permet de comparer ce placement avant toute coopération dans la géométrie J3. Il ne promet ni vitesse ni occupation matérielle : les ressources du noyau et son coût réel doivent être mesurés.

Le patch conserve `kThreads=32`, `count_kernel`, sa réduction et sa publication par thread0, ainsi que `copy_kernel`. Seuls l'indexation et le lancement de fill changent. Les entrées, `FillSink`, les préfixes fixes et les contrôles de fin de feuille restent. Tous les autres fils du CTA retournent au début de fill. Le compteur d'erreurs et la synchronisation CUDA gardent leur rôle de refus ; un statut invalide passe à la position suivante par `continue`, afin de ne pas omettre les feuilles suivantes quand la grille est bornée.

La capacité du device courant est interrogée par `cudaGetDevice` puis `cudaDeviceGetAttribute(cudaDevAttrMaxGridDimX)`. Une erreur de query ou une limite non positive rend le même refus explicite du backend, avant lancement fill. La documentation primaire NVIDIA définit le [device courant](https://docs.nvidia.com/cuda/cuda-runtime-api/group__CUDART__DEVICE.html) et l'[attribut de limite de grille X](https://docs.nvidia.com/cuda/cuda-runtime-api/group__CUDART__TYPES.html).

La grille est min(fill_jobs, limite), mais ne tronque pas la liste : le fil0 parcourt `position=blockIdx.x`, puis ajoute `gridDim.x` en u64. Les classes de positions modulo la grille sont disjointes et couvrent la liste entière. La garde existante autorise au plus 2^32 jobs et des identifiants u32 ; la grille positive est au plus INT_MAX d'après le type int du résultat query. Le cast unsigned est exact et même le dernier incrément reste inférieur à 2^33. Les cas au-delà de la limite peuvent donc conserver plusieurs positions par CTA. `fill_jobs=0` ne déclenche ni query ni lancement fill.

`replay.py` vérifie le patch exact, les empreintes, les kernels count/copy byte-identiques et 60 cas de partition, y compris un plafond forcé inférieur au nombre de positions. Douze cas aux limites 2^31/2^32 sont vérifiés par quotient/reste, sans allouer ou énumérer des milliards de positions. Le mutant gardant l'ancienne formule `blockIdx*32` avec un seul fil actif visiterait seulement 1/14 ou 132/4196 positions ; cette contre-épreuve évite ce faux raccord.

Pour adoption éventuelle : construire une référence830 et un candidat patché, puis comparer les dumps FULL et le registre, sur petites portes où les deux chemins copied/fill sont réellement exercés, et sur les prises K5/K10 retenues. Vérifier aussi le refus, les contrôles de fin sink, compute-sanitizer memcheck/racecheck et un plafond de grille réduit par une couture de test. Ensuite seulement relever fill_ns et domain_ns sur le protocole G4 épinglé. Ce helper ne qualifie aucune sortie ni aucun gain et ne nécessite aucune option publique pour l'essai.

Rejeu stdlib en lecture seule du JSON figé :

```sh
python3 -B replay.py --check proof.json
python3 -O -B replay.py --check proof.json
```

L'application du patch au produit appartient au développeur ; le fichier `fill_one_leaf_cta.patch` est reviewable et ne touche qu'un fichier CUDA.
