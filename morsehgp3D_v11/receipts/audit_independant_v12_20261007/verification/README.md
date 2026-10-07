# Vérification indépendante — instantané v11 du 7 octobre 2026

Source : `33c2ae3c8b66d12ef5b2405f1b0f807d7e57aeae`, moteur `ac081a06f`. Compilation CPU Release u21,
GCC 13.3.0, CMake 3.28.3, Python 3.12.1. Worktree détaché et build neuf dans `/tmp` ; aucun build épinglé réutilisé
ou écrasé. Les 644 fichiers de construction/contrôle de [source_manifest.json](source_manifest.json) sont restés
identiques à leurs empreintes après la campagne. GCP non utilisé.

## Résultat CTest

**921 portes sélectionnées, 920 passées, zéro échec, une sautée.** La sentinelle
`mhgp11_support_lidar_sentinel` est sautée en l'absence de `MHGP11_DATA_DIR`. Elle n'est pas une preuve LiDAR.
La ligne standard CTest « 100% tests passed … out of 921 » inclut cette absence : le
[résumé structuré](summary.json) la distingue, d'après le [JUnit brut](fast_junit.xml).

Le rejeu prend 656,50 s mur sur la machine locale partagée, avec au plus deux portes CTest simultanées.
Ce temps est un coût de vérification ; aucun chrono local de contrat HGP n'en est tiré.

Commandes exécutées :

```bash
cmake -S /tmp/ehgp-audit-v12-root-20261007/morsehgp3D_v11 -B /tmp/ehgp-v11-audit-build-20261007 -DCMAKE_BUILD_TYPE=Release -DMHGP11_COORD_BITS=21
cmake --build /tmp/ehgp-v11-audit-build-20261007 --parallel 2
ctest --test-dir /tmp/ehgp-v11-audit-build-20261007 -L fast -LE 'long|diff_v10' -j2 --no-tests=error --output-on-failure --output-junit /tmp/ehgp-audit-v12-root-20261007/morsehgp3D_v11/receipts/audit_independant_v12_20261007/verification/fast_junit.xml
```

[Configuration](configure.log), [compilation](build.log), [sortie CTest](fast_ctest.log) et [JUnit](fast_junit.xml)
sont conservés. Les SHA des journaux, du CLI, de la sonde FULL et de la bibliothèque sont dans `summary.json`.
Les tests compilent avec les avertissements stricts du dépôt. Aucun fichier source produit n'a été changé.

## Témoins supplémentaires

| Contrôle | Résultat et portée |
| --- | --- |
| Cache sous limite finie | Demande 262 145, allocation 286 720, limite inactive 1 ; restitution complète. Interception de taille Release, pas de RSS. |
| Prétraitement ASan | GCC appelle l'empoisonnement explicite du cache ; Clang 18.1.3 le supprime. Aucun binaire sanitizer exécuté. |
| Python normal/−O du témoin cache | Résultats identiques octet pour octet. |
| Cohorte géante W1/W8 | Refus mémoire supplémentaire en W8 dense ; tous les succès ont le même dump. Voir [reçu natif](../native/COHORT_MEMORY_RELEASE_U21.json). |
| Index radix | 22 sites, profondeur 22, trois permutations d'axes. Voir [reçu](../native/INDEX_RADIX_RELEASE_U21.json). |
| Prédicats narrow | 320 038 contrôles dans trois groupes, CPU u21 ; ces groupes recouvrent des portes CTest et ne doivent pas être comptés comme une couverture indépendante supplémentaire. |
| Oracles mathématiques | 1 080 coupes d'intervalles, six points dans Q(√3), sémantique du Kruskal ; voir [math](../math/README.md). |
| Juges et comptes historiques | 42 appels synthétiques de juges, contrôles du runner, 219 entrées de manifeste recalculées ; voir [preuve](../evidence/RAPPORT.md). |

Reproduction du témoin cache, depuis un checkout de l'instantané :

```bash
python3 morsehgp3D_v11/receipts/audit_independant_v12_20261007/verification/check_cache.py --source morsehgp3D_v11 --work /tmp/ehgp-cache-replay
python3 -O morsehgp3D_v11/receipts/audit_independant_v12_20261007/verification/check_cache.py --source morsehgp3D_v11 --work /tmp/ehgp-cache-replay
```

`check_cache.py` exige GCC et Clang et l'environnement ELF/GNU pour l'interception de `operator new`.
Son code 0 signifie que les deux limites décrites sont reproduites ; ce n'est pas un verdict de conformité du moteur.
Le [premier essai en échec](initial_attempt.json) omettait la macro de profil dans le script d'audit. Il a échoué
à la compilation, avant tout calcul. Les fichiers produit sont restés inchangés ; les résultats corrigés sont
[cache_normal.json](cache_normal.json) et [cache_opt.json](cache_opt.json).

## Ce qui n'a pas été rejoué

- Matrices u18/u24, ASan/UBSan/TSan, CUDA et campagne native des 530 mutants.
- Portes longues, stress et tests `diff_v10` ; les validations différentielles antérieures restent attachées à leurs reçus.
- Campagnes de trames LiDAR, contrat de latence, comparaison de clustering par de nouveaux calculs.

Les 26 portes de manifeste de mutants présentes dans cette sélection contrôlent les déclarations ; elles ne
signifient pas que les 530 mutants ont été construits et tués. Les portes de mutants de l'oracle Python ont leur
propre portée. Le vert CPU Release u21 de ce reçu ne se transfère pas aux configurations absentes.
