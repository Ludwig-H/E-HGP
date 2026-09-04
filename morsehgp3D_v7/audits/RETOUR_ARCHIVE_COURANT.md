# Retour constructeur — nettoyage de l'archive sous épuisement mémoire

Cadre : `exploration_v7_hors_registre`, `cpu_reference`, `quantized_u16_input_only`, `audit_independant_math_and_architecture`, `public_status=not_claimed`.

**Défaut reproduit : le destructeur d'une archive provisoire peut appeler `std::terminate` lorsque le nettoyage rencontre `std::bad_alloc`.** La sortie finale n'est pas publiée, mais le processus perd son chemin de refus contrôlé et le provisoire contenant l'entrée reste présent.

La cause est `src/io/archive.hpp:154` : le destructeur, implicitement `noexcept`, appelle `std::filesystem::remove_all(staging_, ec)`. Le paramètre `error_code` n'interdit pas les allocations internes ; sur GCC/libstdc++ 13.3.0, cette opération alloue effectivement dans la scène exercée.

## Reproduction minimale confirmée

Le programme [destructor_probe.cpp](.work_boundary2/destructor_probe.cpp) construit une archive, y écrit deux points valides, puis laisse son destructeur s'exécuter. Le bras adverse remplace les allocations suivantes par `throw std::bad_alloc()`, sans épuiser physiquement la RAM. Un gestionnaire `std::terminate` appelle `_Exit(97)` pour rendre l'issue observable sans produire de core dump ; **97 est le code témoin de l'instrumentation**, pas un code revendiqué par le produit.

```bash
TMPDIR=/workspaces/E-HGP/morsehgp3D_v7/audits/.work_boundary2/tmp g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -pthread morsehgp3D_v7/audits/.work_boundary2/destructor_probe.cpp -o morsehgp3D_v7/audits/.work_boundary2/destructor_probe
morsehgp3D_v7/audits/.work_boundary2/destructor_probe morsehgp3D_v7/audits/.work_boundary2/normal/output normal
morsehgp3D_v7/audits/.work_boundary2/destructor_probe morsehgp3D_v7/audits/.work_boundary2/fail/output fail
```

Les deux répertoires parents doivent exister avant l'appel. [Le résultat brut](.work_boundary2/destructor_probe_result.json) donne :

| Bras | Code | Résidu après le processus |
| --- | --- | --- |
| Allocations disponibles | `0`, `destructor_returned` | Aucun |
| Allocations refusées au nettoyage | `97`, gestionnaire de terminaison atteint | `.work_boundary2/fail/.mhgp7-provisional-xgwgPF/input.u16` |

Source `src/io/archive.hpp` examinée : SHA-256 `e7f056ce909527f668f0239e416bb22175cf3676900f2cd459d36a25e895c9c5`. Probe : source `fbf1424c5d385e656d791f028a7e5026d4bbe70ed2e80648bb483921d1f8b21a`, binaire `00bffdfde054974847fd233383d26df80209fc87ffbc694f1c61071c6c2d80c6`. La reproduction minimale ne prétend pas encore injecter la panne après un callback du pipeline.

## Correction proposée et fermeture

Préparer pendant la construction les descripteurs et noms nécessaires au nettoyage, puis supprimer sans allocation les seuls fichiers de ce format : `input.u16`, `forest_K1.bin` à `forest_K10.bin`, `manifest.json`, puis le répertoire provisoire. Des descripteurs conservés et `unlinkat`/suppression du répertoire permettent un chemin POSIX borné, sans reconstruction de chemins au destructeur. Les noms de forêts peuvent être formés dans un tampon de pile. Ne pas remplacer le problème par une capture silencieuse laissant systématiquement les gros provisoires après un refus mémoire.

Critères de fermeture : témoin nominal conservé ; branche d'allocation impossible réellement atteinte ; aucune terminaison ; aucun fichier final publié ; suppression des fichiers et du provisoire dans la scène injectée ; et maintien du refus mémoire contrôlé quand la panne survient après plusieurs callbacks. Une panne OS empêchant la suppression doit être signalée sans nouvelle allocation et sans prétendre que le nettoyage a réussi.

Toutes les écritures de cette reproduction sont dans `morsehgp3D_v7/audits/`. Aucun code produit modifié. GCP non utilisé.
