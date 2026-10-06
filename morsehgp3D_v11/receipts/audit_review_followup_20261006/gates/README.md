# Supports v2 : suivi de deux raccords, mutant clos en source

**Publication 0cc9cbec4 : le mutant est corrigé en source.** Il emploie désormais `role()[i] <= BallRole::internal`, donc utilise le paramètre tout en gardant tous les rôles. Ne plus appliquer `mutant_parameter_use.patch`. Les douze préfixes u18 restent identiques à SPv1 ; ce point demeure ouvert. Voir `publication.json`. Le rejeu ci-dessous conserve le constat WIP antérieur, pas un défaut encore ouvert du mutant publié.

WIP développeur capturé deux fois avec octets identiques au-dessus de `09f20aca1455c0a288d1701a9a4d2c798fb666e6`. Empreintes et dépendances Git dans `sources.json`. Les trois dépendances de la base locale sont identiques octet pour octet au commit publié `d4228f5e5`, utilisé par le rejeu ; l’objet Git local `09f20aca1` n’est donc pas requis. Aucun source développeur modifié, aucun compilateur, calcul natif, GPU ou GCP exécuté.

Le nouveau mutant **`sp_internes_gardees`** (`tests/mutants/cli.json:233–238`) remplace le corps de `kept(u64 i)` par `return true;` (`src/supports/hierarchy.cpp:45–48`). Son seul usage de `i` disparaît. Les options C++ du pin sont `-Wall -Wextra -Wpedantic -Werror` (`CMakeLists.txt:77,95–97`) : refus de compilation attendu pour paramètre inutilisé, **non observé par compilation dans cet audit**. Le runner classe un refus de construction comme `INVALIDE` pour un mutant attendu par porte (`tests/mutants/run_mutants.py:296–298`) ; il ne s'agit pas d'une mise à mort par l'oracle.

`mutant_parameter_use.patch` remplace seulement la mutation par `static_cast<void>(i); return true;`. Le rejeu vérifie que le motif est unique, le diff exact et le paramètre utilisé après mutation. **Patch conditionnel au `kept` capturé** : le futur sélecteur Kruskal peut déplacer ou supprimer ce corps ; rebaser alors le motif et la mutation sur sa source finale. Ce patch ne corrige ni ne qualifie le sélecteur lui-même. Les contrôles Python `--check` des manifestes actuels CLI31/31 et supports15/15 ont passé pendant la revue ; ils ne compilent pas les mutants et ne détectent donc pas ce défaut.

Les **six paires de références API u18** (`tests/api/tests.cmake:68–80`) sont exactement celles du pin `98a00955083d483306c4f92b9031e382e81b0e59`, soit douze préfixes SHA256 historiques SPv1, alors que `write_supports.cpp:144` publie maintenant l'en-tête2 et `manifest.cpp:444` la version2. Les tables u21/u24 ont été modifiées. Le rejeu compare les douze valeurs u18 aux objets Git98a, sans prétendre connaître les nouvelles valeurs. Aucun échec natif SPv2 ni collision/non-collision de préfixes n'a été mesuré ici : ces attentes n'ont pas été régénérées pour le nouveau format.

Après intégration du sélecteur corrigé et gel/push de sa source, collecter **sur une session G4 gardée u18**, avec `MHGP11_DATA_DIR` complet, les six sorties directes de `mhgp11_api_supports_route_probe` à K5/W1,W4. Les six argv exacts figurent dans `summary.json`, pour uniform18=8k/16k/32k et ng00/ng01/ng02. Ils utilisent un plancher diagnostic1, afin de ne pas bloquer la collecte sur un compte provisoire de sélection ; ils ne sont pas un plan de qualification. Exemple :

```sh
{build}/mhgp11_api_supports_route_probe --work={out}/u18_spv2_scale8000 --k=5 --workers=1,4 --uniform18=8000,20261002 --min-balls=1 --min-cells=1
```

Remplacer ensuite les **six paires** u18 depuis les champs `fichier`/`manifeste` effectivement émis, recouper comptes et registre, puis jouer les six portes API exactes au même commit/profil. Ne pas recopier les préfixes u21/u24 : `coord_bits` appartient au fichier, au manifeste et à la signature. Ne pas figer ces références avant la vraie sélection Kruskal. L'auditeur ne fournit aucun nouveau hash produit, délai de session garanti ou qualification anticipée ; dimensionner la session selon les gardes existantes.

```sh
python3 -B replay.py --repo /workspaces/E-HGP
python3 -O -B replay.py --repo /workspaces/E-HGP
sha256sum -c SHA256SUMS
```

Normal/−O identiques, stdlib uniquement. Le dépôt et les objets Git épinglés sont nécessaires. Les snapshots ne contiennent que du code/configuration, aucun payload LiDAR ni sortie native. Les constats cycle/sélecteur/lecteur déjà publiés restent distincts.
