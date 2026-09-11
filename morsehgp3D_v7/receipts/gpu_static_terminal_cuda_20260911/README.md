# Terminal géométrique CUDA préparé — export exact et qualification locale

Jalon privé v7, profil u16, base c03f6be8 sans raccourci CPU post-seed, `public_status=not_claimed`. Export Gram autonome, gate hôte O2/SAN et compilation **avec liaison** NVCC sm120 sont fermés PASS. **Aucun kernel CUDA de ce terminal n’a été exécuté**, aucune session GCP n’a été utilisée, aucun gain de vitesse ni contrat FULL n’est annoncé.

## Résultats et domaine effectivement éprouvé

L’export contient les 949 facettes K≥2 des 14 géométries historiques de taille ≤8, donc **K2..8 seulement**. Il conserve toutes les facettes déclarées nominales avant calcul ; aucun refus inattendu n’a été filtré. Les 1 428 lignes de trace de la référence statique CPU c03 sont vérifiées indépendamment par Gram pour la clé et le niveau : q2/q3/q4 = 1 041/362/25, 206 coquilles supplémentaires, 478 descentes strictes et une à rayon égal, 479 requêtes d’intrus. Les 949 ordinaux dépassent 2^32.

La gate portable passe O2 et ASan/UBSan avec détection des fuites activée, 9 162 contrôles chacun et sorties déterministes identiques. Douze rejets d’entrée/vue/coupe et huit omissions/corruptions de transport sont causaux. Les deux lectures normales/optimisées des prédicats de provenance font 44 contrôles et 40 rejets chacune. NVCC compile et lie réellement les kernels avec `-std=c++20 -O2 --gpu-architecture=sm_120 -fmad=false --expt-relaxed-constexpr` et les diagnostics hôte stricts, sans exécuter le binaire ; stderr de compilation est vide.

La qualification du raccord FULL/T2 **hôte** sur K1..10 est séparée et explicitement épinglée ; elle ne qualifie pas K9/K10 sur ce transport/kernel. Avant la prochaine exécution G4, des requêtes valides K9/K10 devront être ajoutées et qualifiées sur la source cible choisie. Une exécution de ces seules 949 facettes ne serait pas une preuve du contrat de tour K1..10.

## Ce que vérifie exactement la gate

Un thread est prévu par facette. Le kernel réunit sélection MEB, clé primitive, niveau brut, lookup du catalogue et descente par intrus ; il n’appelle pas la référence CPU ni un matérialiseur géométrique hôte. La référence et l’oracle appartiennent au juge/exporteur. Les clés transportent dix mots u64, les niveaux cinq, sans i128 natif dans l’ABI.

Les **48 descripteurs ABI** comparent tailles, alignements et offsets de structures, pas une trajectoire ni « 48 mots de résultat géométrique ». Au premier passage, toutes les lignes de trace sont confrontées : sites, support/slots/coquille, clé, intrus, terminal et compteurs. Les niveaux sont comparés **en valeur rationnelle**, pas en représentation binaire. Au second passage, la capacité de trace vaut zéro : résultat, statut, ordinal, travail, longueur annoncée et overflow sont confrontés, **aucune ligne n’est comparée**. Les 949 résultats restent complets ; aucune limite de descente n’est ajoutée.

Les douze rejets contrôlent la **dernière requête explicitement fautive**, pas l’annulation transactionnelle du lot entier. Les buffers device sont exposés `const` dans les vues mais ne sont pas relus après le kernel : aucune immutabilité effectivement constatée n’est revendiquée. L’`Arena` synchrone réutilise le backend d’allocation/copie/libération de la primitive MEB/clé déjà figée ; elle reste une infrastructure de gate, pas le futur propriétaire CUDA industriel ni un wrapper FULL transactionnel. La requête vide ne lance rien et ne lit/écrit rien.

Les compteurs de gate incluent ses replays et fautes : 49 lancements simulés hôte (dont ABI), H2D 1 037 526 octets, D2H 773 776, toutes les allocations libérées avec certification. Ce ne sont ni des mesures GPU ni une estimation de performances. Le binaire laisse l’autorité infrastructure au contrôleur externe et ne contient pas de `gcp_used:false` intrinsèque ; seuls les reçus de cette session locale établissent l’absence d’usage GCP.

## Sources, provenance et limites

Les quatre captures compilent leurs snapshots exacts. L’export exige les six clôtures O2/SAN core/T2/guards du propriétaire corrigé, avec la même fermeture **complète** des sources communes, et non le seul owner. Le consommateur exige les mêmes helpers et les mêmes exporteur/référence/types de fixtures ; les snapshots et la fixture effectivement consommée sont vérifiés. La fixture SHA-256 est `5a1d332d80d2dabc7d2a9ac699cf520bef3705dd6e62b8d03740503e19ce696f`.

L’autorité hôte est le paquet terminal de manifeste `3ac9ca12fc22433fcedff2d7130f2a78031799fff81687c4b87c2d7945ca5ef1` ; ses manifests et les six reçus requis sont conservés ici. Leurs snapshots de helpers sont liés à la copie identique présente dans ce paquet. L’histoire complète du propriétaire fautif et sa réfutation causale restent dans ce paquet hôte distinct, non réinterprétées comme une exécution GPU.

Les limites de représentation héritées sont maintenues : index sans doublons géométriques, catalogue c03 avec coquille ≤12 et intérieur ≤9, API terminale K2..10 mais corpus CUDA préparé limité à K2..8. Le niveau `before` de l’export est `3*65535²+1` ; les requêtes sont toutes les facettes bornées, pas seulement les occurrences chronologiques effectivement demandées par un lot FULL. Catalogue exhaustif et référence CPU ne sont pas une architecture produit. Aucun raccord à la tour active, aucun lot device industriel et aucun objectif 50k/multi-millions ne sont acquis.

## Lecture et reproduction

```bash
python3 -B verify.py
python3 -B -O verify.py
python3 -B verify.py --extract /tmp/mhgp7_terminal_cuda_neuf
```

Le lecteur est portable, effectif sous `-O`, et ne relance aucune géométrie. Il vérifie sources, attentes, clôtures hôte, commandes, résultats et pins ELF. `sources/current/` contient une arborescence directement compilable ; `storage_map.json` restitue sans perte les quatre captures et leurs snapshots dédupliqués. Quatre ELF sont seulement épinglés, jamais distribués ; aucun vendor n’est embarqué. Le binaire CUDA compilé n’a jamais été exécuté, même sur CLI invalide.

Les commandes historiques absolues et flags sont conservés dans `captures/*/receipt.json`. Pour recompiler la gate hôte sans Boost, utiliser `g++ -x c++ -DMHGP7_FAKE_DEVICE -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -pthread sources/current/cuda_trial/device_gate.cu -o /tmp/mhgp7_terminal_stub`, puis `--selftest` ; CLI inconnue/absente→2. L’exporteur seul nécessite Boost. Les chemins NVCC/toolkit/adaptateur doivent être adaptés à l’environnement. Une future exécution CUDA exige une session gardée pilotée par ROOT, hors de cette qualification.
