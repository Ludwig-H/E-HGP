# Route privée de lots MEB hôte/CUDA

Snapshot `ce842a3f`, `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`. Aucune source active, audit, reçu figé ou index Git modifié. Aucune commande GCP. Les sources CUDA ici ne sont pas un résultat de device exécuté localement.

## Contrat worker

Source autonome : `source/morsehgp3D_v7/tests/anchor_meb_route_device_gate.cu`. Seul `--selftest` est accepté ; les autres formes rendent 2 avant tout accès CUDA. Le succès produit une unique ligne JSON, sans autre sortie stdout. En mode NVCC : `backend=CUDA`, `sm=12.0`, `device_executed=true` ; en mode g++ : `backend=HOST_STUB`, `sm=0.0`, `device_executed=false`. Le programme exige SM120 lorsqu'il utilise CUDA. `gcp_used=false` signifie seulement qu'il n'appelle aucune API cloud ; un worker G4 doit conserver sa propre provenance GCP, sans reprendre ce champ comme lieu d'exécution.

Le juge porte sur 605 cas et 350 positions géométriques distinctes. Les attentes sont embarquées dans `tests/anchor_meb_fixtures.inc` (SHA256 `1ea6d0f74a256baf88e4c31cc001af13548f1bc9978c5a9daa9e70359767995d`). Elles proviennent d'un générateur local dont chaque cas est confronté au Gram rationnel indépendant puis à `anchor_meb` : 11 752 checks, 300 permutations, 197 extra-shells, supports q1/q2/q3/q4 = 82/393/110/20. Le worker n'a besoin ni de Boost ni d'autre fichier de données extérieur.

Le JSON expose aussi les checks/rejets/flags, les tailles ABI, les octets de positions résidentes, les octets de transfert et le nombre de lancements du lot nominal, les matérialisations/puissances hôte supplémentaires et les puissances déclarées par la sélection. Ces compteurs décrivent le lot de 605 cas, pas toute la gate ni une tour FULL. Aucun temps de composant n'est un contrat 50k points/1 seconde.

## Propriétaire et transaction

`IndexSnapshot::capture` copie un `CloudIndex` dans un propriétaire immutable, vérifie son domaine géométrique, ses clés Morton et leur unicité, et lui attribue une identité opaque avec serial non réutilisable. Un contexte garde ce propriétaire vivant. Une autre capture aux mêmes positions ne peut pas lui être substituée. Les requêtes contiennent des GeometryIndex de ce snapshot, jamais des PointId. La validation ne certifie pas le BVH, les buckets ou les autres données du CloudIndex, qui ne sont pas consommés par la MEB. Cette copie de tout CloudIndex est un choix privé de simplicité, pas l'architecture massive finale.

Seules les positions sont copiées une fois sur device. Les buffers de requêtes et réponses restent possédés par le contexte et croissent géométriquement. L'identité de snapshot et un serial de lot sont présents dans chaque ligne wire. Les types de 72 et 112 octets ont leurs tailles, alignements et offsets vérifiés statiquement et par une petite sonde réellement lancée en mode CUDA. Le contrat est un ABI de processus 64 bits little-endian, pas un format d'archive portable.

Le wrapper valide toute l'entrée avant travail device, préinitialise toutes les sorties par des sentinelles fraîches, lance un thread par facette, synchronise, recopie les sorties, puis valide et matérialise tout le lot dans des temporaires. Le vecteur de géométrie n'est publié que si tout réussit. Les sorties brutes éventuellement revenues sont des diagnostics sans autorité géométrique, jamais un préfixe publié. Un lot vide n'accède à aucun pointeur ni ne lance de kernel.

Les erreurs d'entrée/propriétaire/thread antérieures au travail sont des refus sans empoisonner le contexte. Les erreurs de transport, de résultat ou de publication après engagement du travail l'empoisonnent. Le contexte est mono-thread propriétaire, lié au device CUDA 0 ; un autre thread ou une autre sélection de device ne peut l'utiliser silencieusement.

Les libérations ont un statut permanent : tentatives, échecs, octets alloués et octets de libération certifiés. `close()` reste faux dès qu'une libération n'a pas été certifiée, y compris un ancien buffer remplacé dans `grow`. Une construction échouée avec nettoyage non certifié remonte explicitement cette limite. Aucun `cudaDeviceReset` global ni récupération supposée d'un échec `cudaFree`. Le flag de test de libération réalise physiquement la libération du tampon de test, puis injecte un statut non certifié afin de tester ce caractère permanent sans créer une fuite intentionnelle sous SAN ; ce n'est pas une panne réelle du pilote observée.

## Limites géométriques et compteurs

La sélection reprend le prototype publié, avec sa seule annotation HD q4. Une option privée de falsification de positivité a été ajoutée au chemin de test ; la voie nominale est inchangée. Positivité, confinement, première sélection et coquille sont falsifiés causalement. La validation hôte certifie la MEB du support accepté, mais ne prouve pas que le préfixe lexicographique a été exécuté : le carré avec un support ultérieur valide teste explicitement cette limite. Les positions du support et tous les compteurs sont comparés à la référence et aux attentes dans la gate.

Les puissances et matérialisations hôte sont séparées du travail de sélection rapporté. Les agrégations des compteurs déclarés sont vérifiées contre le débordement, mais une sortie forgée n'est pas une preuve du travail exécuté. Les compteurs de chaque requête nominale sont bornés par K au plus 10. Il n'existe ici ni recherche d'intrus, ni catalogue, ni terminal, ni assemblage FULL, ni tour GPU.

## Commandes privées

Depuis la racine, chaque dossier de capture doit être nouveau :

```bash
python3 -B build/v7_gpu_meb_device_20260911/record.py --out stub_replay --mode stub
python3 -B build/v7_gpu_meb_device_20260911/record.py --out san_replay --mode san
python3 -B build/v7_gpu_meb_device_20260911/record.py --out nvcc_replay --mode nvcc
```

`fixture_r1` conserve la génération indépendante réussie. `stub_r1` conserve une compilation échouée par appel ambigu `build_cloud_index({})`, corrigé avec un type de vecteur explicite. `stub_r2` passe 21 432 checks et 44 rejets ; son JSON ne contenait pas encore le champ de provenance `gcp_used` attendu par le worker. `stub_r3` (O2 final), `san_root_r1` (SAN exécuté depuis le contexte ROOT, avec LSan actif) et `nvcc_r1` (compilation/lien, sans exécution device) passent sur des sources identiques. Les rapports de la gate O2 et SAN sont identiques : 21 432 checks, 44 rejets, 4 flags causaux. Les erreurs d'arguments rendent exactement 2. Aucune capture ni source figée antérieure n'est réécrite. Les refus LSan/ptrace des expériences précédentes ne sont pas transformés en réussites ; cette nouvelle exécution SAN ROOT constitue un reçu distinct.

`binary_pins.json` conserve les hashes/tailles des exécutables locaux, sans les distribuer. `manifest.json` scelle les sources, originaux, patches et captures textuelles. `verify.py` les relit sans compiler ni exécuter de binaire, en mode normal comme sous `-O`. La première tentative de snapshot, refusée pour un chemin inexistant de l'adaptateur NVCC, est conservée dans `setup_failure.json`.

Le TU peut être compilé directement sur le worker, sans Boost :

```bash
g++ -x c++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror morsehgp3D_v7/tests/anchor_meb_route_device_gate.cu -o anchor_meb_host
```

SAN remplace `-O2` par `-O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined`, puis utilise `ASAN_OPTIONS=detect_leaks=1:halt_on_error=1` et `UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`. `-DMHGP7_FAKE_DEVICE` est inutile sous g++ : l'absence de `__CUDACC__` sélectionne le backend hôte. Un binaire g++ ne revendique jamais une exécution GPU.

La compilation CUDA utilise C++20, `-O2 --gpu-architecture=sm_120 -fmad=false --expt-relaxed-constexpr -Xcompiler=-Wall,-Wextra,-Wpedantic,-Werror`, l'adaptateur de phase hôte strict déjà qualifié et le toolkit 12.9 existant. `record.py --mode nvcc` ne lance jamais le binaire lié ; seul le worker gardé autorisé peut exécuter sa gate device. Les anciens échecs LSan/ptrace du prototype publié restent conservés ; une nouvelle qualification SAN n'efface pas ces tentatives.
