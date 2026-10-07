# Cache : contre-épreuve causale de l'empoisonnement ASan

7 octobre 2026. Audit indépendant, **correctif développeur non commis** lors de la capture.
Portée : partie empoisonnement de `CST-0019`, pas clôture du budget/concurrence `CST-0007/0019`.
`phase=exploration_v12_hors_registre`, `backend=cpu_reference`, `public_status=not_claimed`.

**La correction Clang/GCC est confirmée sur les trois modes de la sonde.** Revenir à la seule détection GCC
supprime effectivement les deux diagnostics sous Clang, à source et sonde autrement identiques.

| Construction locale ciblée | `garde` | `lecture_apres_restitution` | `lecture_hors_taille` |
| --- | --- | --- | --- |
| Clang 18.1.3, garde corrigée, ASan partagé + UBSan | code 0 | SIGABRT, `use-after-poison` | SIGABRT, `use-after-poison` |
| Clang 18.1.3, détection GCC-only rétablie dans une copie | code 0 | code 0, aucun diagnostic | code 0, aucun diagnostic |
| GCC 13.3.0, garde corrigée, ASan + UBSan | code 0 | SIGABRT, `use-after-poison` | SIGABRT, `use-after-poison` |

Le programme Python observe `returncode=-6` pour SIGABRT (un shell l'exprime par 134). Il exige aussi le diagnostic
précis `ERROR: AddressSanitizer: use-after-poison`, pas seulement un arrêt anormal. Les chemins sains et la variante
ancienne doivent imprimer `sonde_cache_ok` et n'émettre aucun stderr. [Résultats](result.json).

## Sources et méthode

[capture.json](capture.json) épingle trois fichiers :

- `buffer.cpp` : `fb3c52393cef9b31fe064995a94958b24edcbe3aeda0925dfa30ebf54f37664d` ;
- `buffer.hpp` : `3c7fffc6ed97b6456f666ef849ddd6f4f3a1b3f2427c0fc875b7d5edcd14130b` ;
- `cache_poison_probe.cpp` : `3848d1865fec74b05dd0a457bc1f0eeb654089bfcf5ce3293a9c3602d308aad6`.

La revendication développeur relue est épinglée par SHA-256
`18676fe738df5e3e43c82609aa6f6dc932d2fed5b309b1356c4b9f95f9d8f1ab`.
Elle ne remplace pas les exécutions indépendantes ci-dessus.

[check.py](check.py) exporte uniquement `core` depuis `f601b36ac` dans `/tmp`, applique le patch mémoire déjà
publié au commit `c9356b5b8` dans le [reçu précédent](../audit_reprise_20261007/buffer/README.md), puis ajoute la
sonde capturée par [différentiel](probe.patch). Les trois empreintes sont vérifiées **avant compilation**.
La variante causale ne change que la détection de `MHGP12_ASAN_POISON` dans une copie de `buffer.cpp` :
`__SANITIZE_ADDRESS__` seul, au lieu de la disjonction avec `__has_feature(address_sanitizer)`.
Le corps du cache, les tailles et la sonde restent identiques.

Trois binaires seulement, neuf processus, un tampon de 300 Kio par sonde ; `-O1`, C++20, u21,
`-fsanitize=address,undefined`, Clang avec `-shared-libasan`. Détection des fuites et fichiers core désactivés.
Les binaires et sources temporaires sont supprimés ; aucune compilation du worktree vivant, matrice CTest,
campagne de mutants, charge LiDAR, nouveau temps de performance ou utilisation GCP.

## Limites

Ce résultat qualifie seulement les deux protections testées et leur témoin sain, pour ces sources et chaînes.
Il ne qualifie pas toutes les tailles, tous les accès concurrents, les fuites ou la chaîne produit.
Le [refus d'allocation après admission pendant une éviction](../audit_reprise_20261007/buffer/README.md)
reste présent dans le même correctif ; l'ASan confirmé ne le clôt pas.

Rejeu depuis la racine :

```sh
python3 -B -S morsehgp3D_v12/receipts/audit_cache_poison_20261007/check.py
```

Clang 18 avec son runtime ASan partagé et GCC sont nécessaires. Le lecteur limite chaque processus de sonde à dix
secondes et chaque compilation à 45 secondes. `SHA256SUMS` ferme ce nouveau reçu ; aucun ancien reçu réécrit.
