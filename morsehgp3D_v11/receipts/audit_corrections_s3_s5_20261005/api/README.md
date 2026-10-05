# S5 — contrelecture du correctif de provenance

Le défaut est corrigé dans les sources WIP de `build/v11-impl-l0`, contexte
Git `59743c21000bfa216a5301655a48e98b90aaa20a`. `fix.patch` contient seulement
le delta du correctif ; les deux portes nouvelles sont conservées à côté.
Les empreintes des sources consultées sont dans `source_manifest.json`.
Aucun changement pendant la capture, aucune compilation, exécution native,
session GCP ou répétition du précédent test de lecteur.

`src/api/manifest.cpp:229` transmet maintenant le poids du nuage à
`check_provenance`. Les lignes 286–288 exigent les tailles 12n/4n et refusent
un budget déclaré nul. Ce retour précède la référence au budget, la création
de `D.pending`, toute écriture de données et les affectations du rapport
(lignes 230, 238, 250–253). Le produit FULL est à poids un, et son poids est
inférieur à `kNone` : les multiplications par 12 et 4 ne débordent pas `u64`
(`src/api/compute.cpp:59–63`, contrat `src/cloud/cloud.hpp:91–92`).

La nouvelle porte `mhgp11_api_publish_reader` publie une provenance valide
avec quatre points à K=3, puis appelle réellement le lecteur officiel
(`publish_reader.py:40`). Elle contrôle les tailles 48/16 et le compte de
points. Pour chacun des quatre négatifs — tailles nulles, rapport faux,
compte faux, budget nul — elle exige code 2, `parameter_out_of_range`, état
`none`, absence de `D`, absence de `D.pending` et inventaire inchangé
(lignes 49–56). Son plancher 26 correspond aux contrôles écrits.

Le groupe C++ `provenance` protège également les refus avant création. Il
ne transmet pas de `RunReport` : la non-mutation du rapport est établie par
l'ordre du code, pas par cette porte. Une sentinelle de rapport passée aux
cas négatifs renforcerait la protection contre une future régression.

Réserve de qualification du mutant, pas défaut produit :
`provenance_tailles_ignorees` supprime la seule utilisation du paramètre
`points`. Avec `-Wextra -Werror` (`CMakeLists.txt:77`), la copie risque de
refuser la compilation pour paramètre inutilisé. Le lanceur ne transforme
pas ce refus en mutant tué : un échec de construction pour un mutant
attendu `porte` est `INVALIDE` (`tests/mutants/run_mutants.py`, fonction
`judge`). Remplacer le contrôle par `static_cast<void>(points);` permet une
mutation compilable puis une mise à mort par la porte.

Verdict : correction causale acquise à la lecture des sources, nouvelles
portes cohérentes avec le lecteur déjà éprouvé. Passage natif des portes et
mise à mort des deux nouveaux mutants restant à acquérir ; aucune sortie
native de `publish` n'a été exécutée par cet audit. `l1` à `83804634c` conserve
encore le défaut ; ce verdict ne concerne que le delta WIP capturé de `l0`.
