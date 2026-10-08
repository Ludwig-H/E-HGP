# Population des masques CPU : quatre compilations vers assembleur

Le 8 octobre 2026 à 04:37 UTC, GCC 13.3 confirme le mécanisme proposé dans
[cpu_popcount](../cpu_popcount/README.md) : **cinq appels logiciels statiques deviennent zéro**
avec le patch SWAR dans les deux wrappers x86-64 de base. Avec `-mpopcnt`, les bras original et
corrigé produisent exactement le même assembleur : un `popcntl` et quatre `popcntq`.

| Options de cible | En-tête | Appels `__popcountdi2` | Instructions POPCNT |
| --- | --- | ---: | ---: |
| x86-64 de base | original | 5 | 0 |
| x86-64 de base | proposition SWAR | 0 | 0 |
| `-mpopcnt` | original | 0 | 5 |
| `-mpopcnt` | proposition SWAR | 0 | 5 |

Les cinq sites sont un dans `audit_popc32` et quatre dans `audit_popc256`, qui compte les quatre
mots u64 de `Bits<4>`. Ce sont des **sites d'instructions dans ce témoin**, pas un comptage dynamique
sur LiDAR. Dans le bras SWAR de base, GCC vectorise les quatre mots avec SSE ; aucun appel ne reste
dans ces deux fonctions. Cela ne quantifie ni un gain de temps ni le coût de cette vectorisation.

## Épingles et fraîcheur

Ce reçu est une nouvelle capture après la coupure du codespace, pas une reconstitution des journaux
perdus. Les trois lignes du wrapper historique ont été récupérées ; leur SHA est identique
(`77de1442…`). Les captures, les dépendances et le compilateur sont datés de la présente exécution.
Le reçu de proposition antérieur reste inchangé.

- Source Git du worktree : `8da450ab751017a77928cb5c33e284b0fd0371e8`.
- `simt.hpp` original : `acda01afa5e114234be2e22b17815d5b7fe32c5fe5a0ae184675ea6ef5f57122`.
- Patch publié : `d2168db65cd280b4f987c127914dd2b295b7ef13d6ef4ec3e2d94b0618d4c325`.
- En-tête après application : `d32fa18d22a23fc1e56ffe5451265eacf326b32630562bab1f22b68dcd243cf2`.
- GCC : Ubuntu `13.3.0-6ubuntu2~24.04.1`, cible `x86_64-linux-gnu` ; pilote et `cc1plus` hachés dans la capture.

Une copie privée des dépendances a été utilisée, sans toucher au produit. `-MMD` a relevé les sept
fichiers du dépôt effectivement inclus ; leurs octets correspondent au commit indiqué et restent
identiques avant/après. Les en-têtes système ne sont pas épinglés : ce n'est pas une compilation
hermétique. Le premier essai a échoué au prétraitement parce que la copie temporaire omettait
`core/reasons.def` ; cet échec de préparation est conservé dans `capture.json`. Après correction de
la copie, les quatre compilations ferment avec code 0, stdout/stderr vides. Aucun objet ni exécutable
n'a été produit ou exécuté. Aucun moteur, CUDA, GCP, chrono ou donnée LiDAR n'est utilisé.

## Relecture et reproduction

Les quatre bras n'ont que trois sorties distinctes : les deux sorties POPCNT sont stockées une seule
fois dans `popcnt.s`. La capture conserve leur association et leurs empreintes séparément.

```sh
python3 -B CHEMIN_DU_RECU/check.py --repo DEPOT
python3 -B -O CHEMIN_DU_RECU/check.py --repo DEPOT
```

Ces commandes ne recompilent rien. Elles extraient les sources Git épinglées, contrôlent les hashes,
réappliquent le patch sur une copie temporaire et relisent les instructions des assembleurs conservés.
Les sorties normal/−O doivent être identiques au champ `result` de `capture.json`. Elles ne prouvent
pas à nouveau que le compilateur a produit ces fichiers ; les quatre invocations observées et leurs
codes figurent dans cette nouvelle capture.

Pour reproduire la compilation, préparer deux copies de la liste `source_pins` à partir du commit
indiqué et appliquer `proposition.patch` dans la seconde. Depuis le dossier contenant `wrappers.cpp`,
utiliser successivement chaque copie comme `SNAPSHOT`, avec puis sans `-mpopcnt` :

```sh
g++ -O3 -DNDEBUG -std=c++20 -DMHGP12_COORD_BITS=21 -S \
  -MMD -MF dependencies.d -I SNAPSHOT/morsehgp3D_v12/src wrappers.cpp -o result.s
```

Les flags de chaque invocation sont enregistrés. Aucun `-march=native`, PGO ou LTO n'est utilisé.
L'assembleur exact dépend du compilateur et des en-têtes ; une autre machine doit publier ses
propres résultats. La macro `__POPCNT__` désigne ici l'autorisation donnée au compilateur.

## Limites pour le développeur

Ce résultat répond à la réserve « le compilateur pourrait réintroduire un appel logiciel » de la
proposition, pour ces wrappers et ce compilateur seulement. Il ne requalifie pas les instanciations
de `leaves.cpp`, l'inlining dans G3, les compteurs, les sorties géométriques ni le binaire de la
session K. Il ne fournit pas de gain CPU mesuré et ne clôt pas CST-0234.

Le port est toujours explicite depuis la v11 gelée `ac081a06fd684fdcd3b11c2b1faa4679f2ad287c`,
`src/catalogue/leaf.cpp` (`popcount_word`) : u64 repris, u32 adapté. La preuve arithmétique et le
pin complet de ce fichier restent dans le reçu de proposition. La prochaine qualification utile
reste celle du produit : instructions de l'objet corrigé, digests/compteurs N32 et N256, puis
ablation CPU à options identiques, feuilles et catalogue entier, à un fil et 48 fils.
