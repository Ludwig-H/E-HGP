# Couture privée : sélection MEB et clé primitive sur device

Cadre : `phase=exploration_v7_hors_registre`, profil `quantized_u16_input_only`, `public_status=not_claimed`. Ce prototype ne porte ni le terminal, ni la recherche d'intrus, ni le lookup, ni la tour FULL. GCP non utilisé par ce travail. Les résultats des deux primitives antérieures sur G4 ne sont pas transférés à cette couture nouvelle.

## Source et contrat

Le socle est `c03f6be8488453486b112811071827a96303ec86`. `pins.json` décrit les annotations et sources explicitement reprises des paquets fermés `gpu_meb_device_route_20260911` et `gpu_ball_key_device_20260911`. Les originales restent dans `originals/`; aucun fichier actif ou paquet fermé n'est modifié. Les 605 attentes Gram autonomes sont reprises byte-identiques, SHA-256 `1ea6d0f74a256baf88e4c31cc001af13548f1bc9978c5a9daa9e70359767995d`. Aucun Boost n'est requis pour cette gate.

`source/morsehgp3D_v7/src/gpu/anchor_meb_key.cuh` ajoute un helper pur HD. Un thread traite une facette, conserve l'ordre q2/q3/q4 et le premier support du sélecteur précédent, puis calcule la clé primitive de ce support avec les formes et le PGCD128 existants. Le singleton q1 passe par la forme diamètre de deux points égaux. La forme acceptée q3/q4 est reconstruite une fois après la sélection : travail backend supplémentaire borné, explicitement distinct des compteurs de supports et de puissances de sélection. Ce n'est pas encore la réutilisation optimale de la forme temporaire acceptée.

La sortie commune est `PrimitiveKeyWords`, 10 mots u64 dans l'ordre a, bx, by, bz, c, chaque coefficient low64/high64 en complément à deux. Aucun i128 natif ne traverse l'ABI hôte/device. Le décodage éventuel ne fait ni forme ni PGCD. Les indices de requêtes sont des indices géométriques, pas des PointId. Leur ordre est préservé pour les slots du premier support ; un futur consommateur intrus pourra trier une copie des indices d'exclusion.

`anchor_meb_key_route.cuh` conserve le propriétaire de snapshot et les buffers réutilisables du prototype précédent, avec ABI version 2 vérifiée sur le backend : requête 72 octets, réponse 208 octets, dont clé 80 octets. La publication est transactionnelle pour le lot entier, jamais par préfixe. Les omissions, mauvais identifiants, statuts ou métadonnées entraînent un refus ; les erreurs après travail empoisonnent le contexte. Une erreur de libération reste non certifiée jusqu'à la fermeture, sans reset global ni promesse de récupérer un cudaFree échoué. La faute de test de libération libère réellement la mémoire puis déclare l'opération non certifiée, pour ne pas créer une fuite volontaire sous SAN.

## Suppression du chemin hôte de matérialisation

Le chemin nominal `Context::run` ne produit plus d'`AnchorMebResult`. Il rend des mots de clé, les slots, q et la cardinalité de coquille. Le niveau exact est **absent**, pas remplacé par une valeur nulle. L'acceptation hôte contrôle seulement l'identité du lot/snapshot, les champs, slots, rangs combinatoires et bornes de compteurs. Elle ne refait ni puissances, ni formes, ni réduction PGCD, ni niveau.

Cette acceptation de transport/métadonnées n'est pas une validation géométrique : des mots altérés ou une clé non primitive peuvent la franchir et sont détectés par le juge. La preuve du résultat est l'obligation du producteur, confronté au CPU et aux attentes Gram. Le rang déclaré des compteurs n'est pas une preuve d'exécution de tous les supports antérieurs. La contre-fixture carré conserve donc un support positif ultérieur valide pour la même MEB, mais détecté comme différent par les slots et le travail.

La gate définit `MHGP7_MEB_KEY_FORBID_LEGACY_HOST_MATERIALIZE` : appeler l'ancien matérialiseur hôte y est une erreur de compilation. Les compteurs nominaux `host_materializations` et `host_validation_powers` restent nuls. Le CPU `anchor_meb` s'exécute séparément dans le juge, après la publication du lot ; son calcul de clé/niveau ne fait pas partie de la route nominale. Le compteur `reported_key_materializations` compte le travail backend déclaré ; dans le stub c'est du travail hôte simulant le device, jamais une exécution CUDA.

## Qualification bornée

Captures `stub_r1` O2 et `san_root_r1` SAN fermées PASS sur les mêmes sources : compilation 0, selftest 0, argument inconnu et absent 2 ; 22 245 contrôles, 605 cas, 6 050 mots, 1 384 mots hauts non nuls, 47 rejets, quatre flags de sélection et 17 mutations de clé. Le travail nominal déclaré contient 9 697 puissances de sélection, 605 matérialisations backend, zéro matérialisation et zéro puissance hôte. SAN a été exécuté par ROOT avec ASan/UBSan/LSan activés. `nvcc_r1` est fermé PASS compilation/lien sm120, stderr vide. Les trois modes ont compilé leurs snapshots propres des mêmes sources. Les tentatives sont create-only et toutes conservées ; aucun échec n'a eu lieu dans ces trois captures. Aucun résultat CUDA exécuté n'est acquis pour cette couture.

La gate couvre les 605 cas (q1=82, q2=393, q3=110, q4=20), 197 coquilles supplémentaires et les extrêmes u16 déjà présents dans ces attentes. Elle compare les 10 mots de chaque clé, les slots, q, coquilles, supports tentés et puissances au CPU et aux attentes Gram. Le niveau est confronté uniquement sur le CPU, car la nouvelle API n'en rend aucun.

Des fautes ciblent le transport, les entrées invalides de fin de lot, les allocations/copies/lancements/synchronisations, la publication et la libération. Quatre flags altèrent support positif, inclusion, coquille ou premier support. Dix-sept mutations de clé ciblent a, b (mot haut), c et la non-primitivité sur q1 à q4, plus le centre du singleton. Ces mutations sont privées, désactivées sur le chemin nominal.

Depuis la racine du dépôt, avec un nom de capture encore absent :

```bash
python3 -B build/v7_gpu_meb_key_20260911/record.py --out stub_r1 --mode stub
python3 -B build/v7_gpu_meb_key_20260911/record.py --out san_root_r1 --mode san
python3 -B build/v7_gpu_meb_key_20260911/record.py --out nvcc_r1 --mode nvcc
```

O2 et SAN compilent `anchor_meb_key_route_gate.cu` avec g++ C++20 et `-Wall -Wextra -Wpedantic -Werror`, puis exécutent `--selftest` (attendu 0), `--unknown` et l'absence d'argument (attendus 2). SAN conserve ASan/UBSan/LSan activés. NVCC compile et lie strictement pour sm120 avec l'adaptateur déjà qualifié, sans exécuter l'ELF CUDA localement. Les commandes exactes, logs, hashes et instantanés avant/après sont dans chaque capture.

Sur un futur device explicitement autorisé et gardé, l'ELF CUDA s'exécutera avec `--selftest` et rendra une unique ligne JSON (`scope=local_meb_device_key_only`). Cette commande n'est pas exécutée dans cette sous-tâche. Aucun chiffre de performance, RSS/VRAM ni contrat 50k ou multi-millions n'est acquis ici.

## Limites d'architecture inchangées

Le propriétaire copie encore tout CloudIndex alors que cette primitive ne consomme et ne transfère que `upos`. La requête et les sentinelles de réponse sont transférées à chaque lot, et la réponse entière revient à l'hôte. Ce n'est pas l'architecture finale de résidence GPU d'un terminal complet ni des nuages massifs. La sortie reste diagnostique hors de la publication complète. L'absence de reconstruction de clé côté hôte ne vaut ni absence de transferts ni mesure de gain.
