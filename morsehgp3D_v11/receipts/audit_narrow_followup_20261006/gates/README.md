# Profil des deux mutants de frontière d’étendue

Pin : `5861c223f31b5d7d6f621522d0ca84d0064c9904`. Lecture source et modèle stdlib uniquement ; aucune compilation, CMake, exécution native ou action cloud. Les anciens reçus restent intacts.

La configuration `mutants` de `tools/g4_matrix.json:42–48` fixe u18. `CMakeLists.txt:300–314` transmet ce profil au lanceur. Les nouvelles entrées `etendue_seuil_double` et `etendue_sans_fermeture` n’ont pas d’option locale (`tests/mutants/catalogue.json:565–579`). Or `Leaf::prepare` ne calcule `wide` que sous `if constexpr(kBits>20)` (`leaf_device.hpp:54–55`), et `span_refusal` ne joue ses deux témoins hors étendue que sous `if constexpr(kCoordBits>20)` (`leaf_device_narrow_test.cpp:213–224`). Tout le cube légal u18, fermeture comprise, tient dans 2^18≤2^20. Ces mutations sont donc équivalentes au témoin en u18 et ne peuvent pas être tuées causalement par la porte choisie. Aucun résultat CTest observé n’est revendiqué.

`proposal.patch` ajoute seulement `options: ["-DMHGP11_COORD_BITS=21"]` à ces deux individus. Le plancher catalogue72, les autres 70 individus et toute la matrice restent inchangés. Le lanceur applique les options locales au témoin et au mutant ; elles sont ajoutées après le profil de base (`run_mutants.py:243,434–437`). u21 est le plus petit profil supporté où la branche et ses deux frontières existent.

`replay.py` charge les sources Git au pin et vérifie les empreintes principales. Il contrôle les 223 motifs principaux et les 2 motifs complémentaires des manifestes catalogue/tower, chacun une seule fois et dans l’ordre de remplacement du lanceur. Il vérifie la matrice u18, les guards source et l’exactitude de la proposition JSON. Il exécute la classe Python réelle `Builder` extraite par AST avec sa fonction `call` remplacée par une capture d’arguments : les définitions observées sont u18 puis u21, sans lancement de CMake. Enfin, il calcule la frontière réelle du témoin de fermeture : étendue 2^20+1 avec `hi`, 2^20 sans `hi`, dans les domaines u21/u24. Ce modèle prouve la différence de garde ; il ne simule ni les prédicats géométriques ni les verdicts natifs.

La lecture des changements narrow et du chemin chaud/froid du réservoir `79fa5e9f7` est favorable dans ce périmètre : la fermeture et tous les sites participent à la borne ; les feuilles larges suppriment tout préfixe et rendent `kUnresolved`, dont les émissions/compteurs sont jetés avant le repli exact ; le chemin chaud écrit les mêmes enregistrements et rangs, et le chemin froid préserve la chaîne, le comptage complet et le replay. Aucune nouvelle qualification CPU/GPU/performance n’est déduite de cette lecture. La session wfgpu1 couvre ses commandes GPU déclarées ; elle ne joue pas de CTest/campagne mutants. Les validations locales évoquées au § U restent distinctes de ce reçu.

Après intégration, rejouer les deux individus avec leur témoin u21 et conserver la cause du verdict. Les trois groupes `mhgp11_catalogue_leaf_narrow_{line_reference,acute_and_triple,span_refusal}` doivent garder leurs qualifications propres à chaque profil ; les portes existantes suffisent, sans baisse de plancher.

Rejeu depuis ce dossier dans un dépôt contenant le pin :

```sh
python3 -B replay.py
python3 -O -B replay.py
sha256sum -c SHA256SUMS
```

Les deux replays rendent `narrow_gate_source_verdict conforme profiles3 mutants2 anchors223 native0 cmake0 cloud0`. Le défaut de `replay.py` compare `proof.json` sans l’écraser ; `--emit` produit seulement le JSON sur stdout.
