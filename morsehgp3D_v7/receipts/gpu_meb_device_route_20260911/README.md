# Route privée de lots MEB : prête pour une gate device

`phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

Ce reçu conserve un wrapper privé CUDA/hôte depuis le snapshot `ce842a3f1c0d55786250b1deba85e7bd92c8807a`. Il est compilable et lié pour SM120, mais aucune exécution CUDA n'a eu lieu dans ces captures. O2 et SAN hôte passent sur les mêmes sources. Il ne s'agit ni d'un terminal statique, ni d'une hiérarchie ou tour FULL GPU, ni d'une mesure de performance. GCP non utilisé par cette qualification locale.

## Résultats et tentatives conservées

| Capture | Résultat borné |
| --- | --- |
| `fixture_r1` | Génération de 605 attentes confrontées au Gram rationnel indépendant et au CPU exact ; 11 752 checks, 300 permutations |
| `stub_r1` | Compilation refusée pour un appel ambigu au constructeur d'index vide ; source et diagnostic conservés |
| `stub_r2` | O2 passe ; JSON antérieur à l'ajout du champ de provenance `gcp_used` |
| `stub_r3` | O2 final passe, 21 432 checks, 44 rejets, 4 flags causaux |
| `san_root_r1` | ASan/UBSan/LSan actifs, exécution depuis ROOT : même rapport que O2, stderr vide |
| `nvcc_r1` | CUDA 12.9.86 compile et lie SM120 strictement, stderr vide ; aucun lancement du binaire |

Les 605 comparaisons donnent q1/q2/q3/q4 = 82/393/110/20 et 197 extra-shells, sur 350 positions distinctes. Le corps nominal conserve les mêmes slots, boules, représentations exactes des niveaux et compteurs que le CPU et les attentes indépendantes. Les attentes sont embarquées : aucune installation de Boost n'est nécessaire au worker. Les erreurs d'arguments rendent exactement 2 avant tout accès CUDA.

Le succès SAN est un nouveau reçu, exécuté dans le contexte ROOT. Les échecs LSan/ptrace du [prototype antérieur](../gpu_meb_selection_prototype_20260911/README.md) restent des échecs conservés, jamais réécrits en succès. Une première tentative de snapshot refusée pour un chemin inexistant est aussi documentée ici.

## Ce que le wrapper garantit, et ce qu'il ne garantit pas

Le contexte possède une copie immutable du snapshot d'index et garde ses positions résidentes. Identité opaque de propriétaire, serial de snapshot et serial de lot empêchent une substitution silencieuse d'index ou de réponse. Les buffers de requêtes/réponses croissent géométriquement et sont réutilisés. L'ABI POD explicite de 72/112 octets est contrôlé statiquement ; une sonde de comparaison est incluse dans la future exécution device.

Tout le lot est validé avant publication de géométrie : entrée complète, sentinelles de réponse fraîches, transfert, sélection, synchronisation, retour, contrôle et matérialisation dans des temporaires. Un défaut final ne publie pas de préfixe. Les diagnostics bruts restent non autoritaires. Les erreurs antérieures au travail — entrée, propriétaire ou thread — ne condamnent pas le contexte ; les défauts de transport ou de résultat après travail l'empoisonnent.

Les échecs de libération restent permanents et rendent `close()` faux, même si l'erreur concernait un ancien buffer remplacé. La fixture `kRelease` teste ce statut après libération physique du tampon de test, sans fuite intentionnelle ; ce n'est pas une panne réelle `cudaFree` observée. Aucun `cudaDeviceReset` ni récupération supposée d'une libération échouée.

La matérialisation hôte certifie la MEB du support sélectionné, pas l'exécution du préfixe de recherche. Le carré avec un support ultérieur mais la même boule teste explicitement cette limite ; la gate compare donc aussi les slots et tous les compteurs. Les puissances supplémentaires de validation et les matérialisations hôte sont séparées du travail déclaré par la sélection. Les compteurs exposés concernent le lot nominal de 605 cas, pas toute la gate ni une tour.

Le snapshot copie encore tout `CloudIndex` alors que seules ses positions sont consommées : ce choix privé n'est pas l'architecture massive finale. La validation ne certifie pas le BVH ou les buckets non consommés. Aucun catalogue, intrus, descente terminale ou raccord FULL n'est implémenté. Le document [PROTOTYPE.md](PROTOTYPE.md) précise le contrat complet et les commandes.

## Sources et reproduction

La [gate autonome](sources/morsehgp3D_v7/tests/anchor_meb_route_device_gate.cu), le [wrapper propriétaire](sources/morsehgp3D_v7/src/gpu/anchor_meb_route.cuh) et les [attentes](sources/morsehgp3D_v7/tests/anchor_meb_fixtures.inc) sont lisibles directement. Les sources finales, originaux et patches sont présents. Les copies historiques répétées sont dédupliquées dans `objects/`, avec tous leurs chemins logiques dans `storage_map.json`. `binary_pins.json` conserve les hashes et tailles des ELF locaux, sans aucun ELF distribué ni vendor.

Depuis la racine du dépôt :

```bash
python3 -B morsehgp3D_v7/receipts/gpu_meb_device_route_20260911/verify.py
python3 -B -O morsehgp3D_v7/receipts/gpu_meb_device_route_20260911/verify.py
```

Le lecteur vérifie le manifeste public, restaure temporairement les 527 fichiers textuels du manifeste privé inchangé, puis relit les sources, commandes, logs et résultats, sans compilation, ELF ou API GCP. Les deux modes doivent réussir. Les commandes de qualification historiques et leurs chemins de dépendances ne constituent pas un toolchain hermétique.

Pour la future gate device, le worker doit compiler les sources dans son propre environnement CUDA puis exécuter `--selftest`. Le programme émet une seule ligne JSON : `backend=CUDA`, `sm=12.0`, `device_executed=true` sur device ; `backend=HOST_STUB`, `sm=0.0`, `device_executed=false` sous g++. Son champ `gcp_used=false` indique seulement l'absence d'API cloud dans le programme ; le worker G4 doit conserver séparément sa provenance GCP et les preuves d'arrêt de sa session. Aucun résultat GPU futur n'est anticipé par ce reçu.
