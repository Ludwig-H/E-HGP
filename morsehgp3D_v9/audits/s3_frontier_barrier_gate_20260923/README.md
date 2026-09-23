# Gate ciblé du frontier S3 (23 septembre 2026)

Le header figé du commit local `50dabc0fa` (SHA-256 `7a2ba5e148cace26d9f008714f7e43102a9dae799ee61e7a330beb54919f2ce8`) est comparé au diff mutable du constructeur, archivé dans `mutable_certificate.patch` (SHA-256 `cfb0faa5693d1d4777f1d8b4813fd1a47018ce32c0f89accfca9d2e0e70a3a98`). Le header corrigé obtenu par ce patch a le SHA-256 `f816480502fb01853ae8c5d71f3765cc48ce511a659c064f5e3998fdd5a6894c`, identique **octet pour octet** à `morsehgp3D_v9/src/gpu/certificate.hpp` publié dans `545c71799`. Sa dépendance `witness_filter.hpp` a aussi le même SHA dans le commit publié. `HEADERS.sha256` scelle les trois variantes et leur dépendance inchangée ; `SHA256SUMS` scelle ce reçu.

`gate.cpp` appelle directement le template `enter_cell` avec un groupe hôte instrumenté et trois cellules au même niveau. C0 écrit 16 sites partiels puis retourne tôt sur le site uniforme 32 ; C1 réécrit les mêmes cases avec d'autres sites, lit les 17 sites de son frontier au test ponctuel, puis termine ; C2 réécrit les mêmes cases. Le groupe enregistre les appels à `sync` entre deux accès au frontier. C'est un gate **structurel d'ordre**, pas une simulation des délais mémoire CUDA ni une preuve globale d'exactitude S3.

Rejouer depuis la racine du dépôt :

```bash
morsehgp3D_v9/audits/s3_frontier_barrier_gate_20260923/run.sh
```

La recette extrait les deux headers de `50dabc0fa`, applique le patch en `/tmp`, compile les trois variantes avec `g++ -std=c++20 -O2 -Wall -Wextra -Werror`, vérifie les SHA et compare les sorties archivées. Chaque exécutable retourne `0` pour son résultat attendu. Le header figé et le mutant qui ôte **seulement** la nouvelle barrière donnent `waw_unordered=1`, `war_unordered=1` ; le header corrigé donne zéro pour les deux. Voir `frozen.stdout`, `mutable.stdout`, `mutant.stdout` et `run.stdout`.

La barrière au début du scan protège aussi les **lectures** faites après l'ancienne barrière de fin de scan : une barrière ajoutée uniquement au retour anticipé de C0 ne traiterait pas le réemploi après la lecture de C1. La [documentation CUDA](https://docs.nvidia.com/cuda/cuda-programming-guide/05-appendices/cpp-language-extensions.html) garantit l'ordre mémoire de `__syncwarp` et précise que `__ballot_sync` ne le garantit pas. Il reste à compiler et comparer le noyau sur G4, avec un test mémoire device et un différentiel par arête ; ce reçu ne les remplace pas.
