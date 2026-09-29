# Contre-audit de la sonde CUDA corrigée

29 septembre 2026. `public_status=not_claimed`. Aucun GCP, aucune mesure de
débit, aucune modification du moteur ou des reçus historiques.

## Conclusion

La correction désignée `695934464` supprime le défaut de débordement signé
des boucles de débit. Un petit contrôle hôte indépendant confirme maintenant
leurs résultats modulaires, sans diagnostic UBSan. **Il ne qualifie ni les
durées GPU de cette version corrigée, ni les prédicats ou la tour du moteur.**

La source relue est [cuda_probe.cu](../../../bench/g4/cuda_probe.cu), SHA256
`3ea56187e2e406c45ac24aca997bbc99f76c0391f6d1a195820777ad42daeb11`, identique
avant et après le contrôle. L'[addendum historique](AUDIT_ADDENDUM_GPU_GRANDK_20260929.md)
reste valable pour l'ancienne sonde signée ; il n'est pas réécrit.

## Domaine et preuve hôte

Aux lignes 42–63, `x`, les produits et les accumulateurs sont non signés.
Les débordements des récurrences sont donc modulaires ; les décalages portent
sur des valeurs non signées. Le stockage final dans `int64_t` n'est pas
réinjecté dans une récurrence. Il n'y a plus le produit ni l'addition signée
hors domaine reproduits dans l'ancienne version.

Le comparateur des lignes 32–36 et 106–110 reste inchangé : chacun des produits
i64 × i64 appartient à `[−2^126 + 2^63, 2^126]`, contenu dans i128 signé.
Cette propriété ne s'étend pas aux sommes, déterminants ou types I192 du moteur.

Le contrôle indépendant a utilisé GCC 13.3, C++17, `-O1`,
`-fsanitize=undefined -fno-sanitize-recover=undefined` :

- 8 entrées : `INT64_MIN`, `INT64_MIN+1`, −2, −1, 0, 1, 2, `INT64_MAX` ;
- 6 longueurs de récurrence : 0, 1, 2, 16, 64, 4096, soit 48 cas pour chacune
  des deux largeurs, confrontés à un oracle Python entier modulo 2^64/2^128 ;
- les 4096 quadruplets de ces 8 valeurs, comparateur confronté aux entiers
  Python sans débordement ;
- corps des récurrences comparés à la source CUDA avant compilation ;
  code de sortie 0, stderr vide, aucun écart.

Artefacts persistants de cette nouvelle vérification locale, copiés octet pour
octet depuis `/tmp/mhgp10-cuda-corrected-audit-20260929.0MGxSWlo/` :

| Fichier | SHA256 |
| --- | --- |
| [host.cpp](../../../receipts/audit_continu_20260929/timeout/cuda_corrected/host.cpp) | `1436846a7c2b3875d267531f3605cbcb4974912169fb451350cd9062ac5e4a27` |
| [run.py](../../../receipts/audit_continu_20260929/timeout/cuda_corrected/run.py) | `711eb674a19afce637c887729471df2cf808154d133c55cef3807812ad9a8344` |
| [receipt.json](../../../receipts/audit_continu_20260929/timeout/cuda_corrected/receipt.json) | `3f64a8a1dedafc644bb263880eb6ec50f5549561fa0fd7c64f9796b2bef96bec` |

Les chemins `/tmp` contenus dans le reçu restent ceux de l'exécution historique.
Ne pas lancer `run.py` dans l'archive : pour rejouer, copier `host.cpp` et `run.py`
dans un nouveau répertoire temporaire. Le script refuse une source CUDA dont le
hash a changé ; il compile et écrit le nouveau reçu à côté de sa propre copie.

Ce n'est pas le reçu UBSan initial évoqué dans la
[réponse du développeur](../../REPONSE_CLAUDE_ADDENDA_ET_RAPPORT_INDEPENDANT_20260929.md),
lignes 21–27 : son artefact précis n'a pas été retrouvé dans les reçus v10 ni
dans `build/v10-persist`. Il s'agit d'un contrôle indépendant nouvellement
exécuté, exclusivement hôte, pas d'une instrumentation NVCC/device.

## Mesures et contrôles restant à distinguer

Le [reçu G4 disponible](../../../receipts/g4_session7_cuda_probe_20260929/README.md)
porte toujours `3b3ea7241`, donc l'ancienne sonde. Aucun reçu GPU de la source
corrigée n'est présent dans le périmètre relu. Les anciens ×2,32 ne sont pas
réhabilités par le contrôle hôte. Les nouvelles récurrences restent deux charges
différentes et leur `gops` compte des itérations, pas les prédicats exacts du moteur.

Les appels d'allocation, de copie et d'événements sont désormais vérifiés.
Les lignes 146–167 séparent bien les 1000 lancements synchronisés individuellement
des 1000 lancements asynchrones avec synchronisation finale. Aucun nouveau temps
n'est déduit ici de la seule lecture de ce code.

Deux petites précisions de robustesse, sans défaillance observée : les erreurs
d'initialisation des lignes 72–74 deviennent `no_cuda`/code 2, pas `cuda_error`/3 ;
et si le dernier `cudaGetLastError` échouait avec zéro écart géométrique, les
lignes 157–168 produiraient `status="ok"` mais un code de sortie 1 et un champ
`cuda_error` non nul. Le lanceur Python propage ce code : respecter ensemble le
code processus et le contenu, jamais `status` seul. Les durées flottantes ne sont
pas explicitement contrôlées positives/finies avant division ; ajouter ce contrôle
au futur lecteur éviterait d'accepter un chiffre non fini. Ce sont des réserves
prospectives, pas des défauts constatés du reçu G4 historique.

## Rangement : contrôle ciblé des index

Dans le worktree partagé, tous les liens locaux relus existent : 12/12 dans
`audits/AUDIT_ETAT_COURANT.md`, 16/16 dans `audits/README.md`, 2/2 dans
`receipts/audit_v9_20260928/README.md`. Le nouveau lien vers l'archive v9 n'est donc
pas mort. La relocalisation de nos anciens artefacts publiée dans `c5015a570`
n'est pas encore intégrée dans cette copie locale ; cette divergence ne prouve
aucun lien cassé dans la version publiée. Aucun fichier d'un autre acteur modifié.
