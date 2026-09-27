# Complément : portée exacte des fichiers de paramètres CUDA R1

27 septembre 2026. Relecture seule après clôture de la
[note initiale](README.md) et du
[reçu R1](../b_q34_resident_survivors_20260927/checks/r1/capture.json).
Aucun ancien fichier, build ou reçu modifié. Aucun nouveau test, build ou
GCP lancé pour ce complément.

## Constat vérifié sur les trois builds réels

Les dictionnaires R1 `pins_before`, `pins_after` et `build_pins` ne
contiennent aucun fichier `.rsp`. Pour la cible réellement construite
`mhgp9_resident_survivors` :

| Profil | Compilation | Lien |
| --- | --- | --- |
| Release | Arguments C++ directs, aucun `.rsp` | Objets et bibliothèque gen directement dans `link.txt` |
| Sanitize | Arguments Clang directs, aucun `.rsp` | Objets, instrumentation et gen directement dans `link.txt` |
| CUDA | Deux TU CUDA utilisent le même `includes_CUDA.rsp` | Objets, gen et archives CUDA directement dans `link.txt` |

Les chemins inspectés sont
`/workspaces/E-HGP/build/v9-resident-survivors-20260927-r1_{release,sanitize,cuda}/`.
Dans le build CUDA, les deux unités sont le nouveau `device_cuda.cu` et
celui de la référence résident filtré. Leurs commandes enregistrées et
`flags.make` contiennent littéralement :

```text
--options-file CMakeFiles/mhgp9_resident_survivors.dir/includes_CUDA.rsp
```

Le fichier présent aujourd'hui ne contient que les trois chemins
d'inclusion du dépôt et le chemin système CUDA local. Cette observation
tardive **n'est pas** un hash pris avant/après la compilation historique.
Un deuxième `includes_CUDA.rsp` existe dans le sous-répertoire CMake
`baseline/`, mais n'est pas celui consommé par les cinq unités de la cible
qualifiée.

En particulier, **aucun des trois `link.txt` R1 n'utilise `objects1.rsp`
ou `linkLibs.rsp`**. Ces fichiers apparaissent dans la nouvelle porte CUDA
directe, pas dans ce lien historique. Ne pas étendre la lacune constatée
au lien R1 ou aux compilations Release/San qui n'emploient pas cette
indirection.

## Ce qui reste établi et ce qui ne l'est pas

Restent les sources, outils et archives listés, les inventaires de headers
`-M` pris avant build, l'inclusion des `.o.d` compilés dans ces inventaires,
les commandes, `compile_commands.json`, `flags.make`, `link.txt`, les
binaires et les sorties observées, avec leurs pins historiques. Les tests
Release/San et les mêmes réponses portables du binaire lié CUDA ne sont
pas annulés par cette lacune. Aucun résultat géométrique faux, crash ou
mutation réelle des paramètres n'a été observé.

En revanche, le hash de la commande qui **nomme** un fichier de paramètres
n'épingle pas son **contenu**. Un changement de macros/options dans ce
fichier peut laisser les mêmes noms de headers dans `-M` et `.o.d`.
La fermeture des headers n'est donc pas une fermeture exhaustive des
arguments indirects consommés par NVCC. Le lecteur R1 ne rejugerait pas
ce contenu. Sa preuve des options exactes de compilation CUDA est partielle.

Les `.o` individuels ne figurent pas non plus dans `build_pins` R1 ; les
binaires finaux y figurent. Cela ne transforme pas les fichiers d'objets
présents aujourd'hui en artefacts historiquement épinglés. La nouvelle
porte les ferme explicitement, mais cette amélioration n'est pas héritée
par R1.

R1 n'exécutait de toute façon aucun kernel du nouveau collecteur. Il
conserve son statut de tests portables et de compilation CUDA observée,
avec cette restriction de traçabilité ; il n'est pas une qualification
GPU à rétrograder en résultat géométrique contraire.

## Décision de reprise proportionnée

Une réexécution massive Release/San ne réparerait pas les arguments
indirects historiques de NVCC. Il n'est pas nécessaire de recommencer ces
portes portables pour cette seule lacune. Ne jamais repinner R1 après coup
ni renommer les fichiers courants en preuve « avant compilation ».

La prochaine compilation CUDA du **comparatif réellement utilisé sur G4**
doit prendre ses propres pins avant/après pour tous les fichiers de
paramètres consommés, y compris les éventuels `@...` de lien, et vérifier
les archives effectivement résolues. Elle doit conserver son propre
binaire, ses dépendances et ses résultats candidat/référence, sans hériter
la fermeture manquante de R1.

La [nouvelle porte des grandes clés](../b_q34_survivors_device_gate_20260927/README.md)
ferme ses fichiers `.rsp`, objets et archives dans une capture autonome.
Elle compile directement le collecteur candidat inclus ; ce n'est ni une
réparation rétroactive de R1 ni une requalification de l'intégralité du
comparatif et de sa référence. Les essais device et les chronos appariés
restent une étape distincte.

Verdict : conserver les observations R1 avec cette portée corrigée ;
utiliser la prochaine capture CUDA complète et fermée comme nouvelle
autorité pour le comparatif G4. Aucun échec géométrique n'est inféré d'une
lacune de traçabilité. Complément clos.
