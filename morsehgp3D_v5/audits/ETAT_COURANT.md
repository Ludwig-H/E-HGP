# État courant — audit de MorseHGP3D v5

- **Date :** 27 août 2026
- **Pin audité :** `8600c53b9fca58fb40148f8ecb4e37cedf820abe`
- **Cadre :** `phase=exploration_v5_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`
- **GCP :** non utilisé

Le `HEAD` et `origin/main` pointaient sur ce pin à la clôture des essais. Claude avait déjà repris le développement dans le worktree ; ces ajouts postérieurs, non suivis ou non commités, sont hors périmètre. La v4 n'est ici qu'une référence différentielle : aucun code, théorème ni statut n'est transféré automatiquement.

## Verdict exécutif

Le commit est **reçu comme un bon jalon de fondations** : organisation neuve, index spatial déterministe, familles appariées à la v4, ledger WSPD, juges de fuseaux fail-open et harness de codes de sortie exacts. La construction Release est propre et les 21 portes déclarées passent.

Il n'est pas reçu comme une v5 fonctionnelle complète. Un même défaut de contrat racine casse les parcours sur les singletons et rend certains appels dangereux sur le vide. Le census perd en outre les multiplicités pourtant acceptées par l'index. Ces deux points doivent être fermés avant de brancher la génération et la forêt. Il n'est pas nécessaire de reprendre le socle : les correctifs sont locaux et doivent devenir des fixtures permanentes.

## Ce qui est établi au pin

- Les quatre exécutables CMake compilent en C++20 avec `-Wall -Wextra -Wpedantic -Werror`.
- `ctest` passe `21/21` ; toutes les portes portent uniquement le label `gate`.
- Tous les en-têtes suivis sous `src/` compilent isolément avec les mêmes avertissements stricts. Cela prouve leur santé syntaxique, pas leur comportement.
- Les 12 cas de familles gravés concordent point par point avec la référence v4. La portée est strictement la génération de ces familles, pas une équivalence HGP v4/v5.
- Les invariants d'identité (`PointId`, rang géométrique, bucket de positions dupliquées) sont mieux séparés que dans la v4. Le ledger WSPD en masse 128 bits et les deux mutants de fuseau tués sont de bonnes portes de régression.

## P0 — fermer le contrat vide/singleton

[`CloudIndex::root()`](../src/tree/cloud_index.hpp) représente un singleton valide par `leaf_ref(0)`, alors que `nodes.empty()` est vrai à la fois pour ce singleton et pour un nuage vide. [`ball_depth_at_least()` et `ball_census()`](../src/pipeline/census.hpp) interprètent actuellement `nodes.empty()` comme « aucun point ». Le même motif existe dans les parcours q3 et cover. À l'inverse, les parcours de [`witness_count.hpp`](../src/spindle/witness_count.hpp) partent directement de `ix.root()` et peuvent déréférencer `upos[0]` sur le vide.

Reproducteur minimal : un point `(1,1,1)` et la forme `P(z)=|z|²-4`. Résultat observé :

```text
valid=1 unique=1 nodes=0 at_least=0 count=0 status=0 interior=0 shell=0
```

Le point est strictement intérieur : `at_least` doit valoir 1 et le census contenir un intérieur.

Correction conseillée :

1. décider le vide avec `unique_count() == 0`, jamais avec `nodes.empty()` ;
2. initialiser tout parcours non vide avec `ix.root()` ;
3. imposer une garde explicite `invalid_input`/vide aux API qui reçoivent des ancres ;
4. ajouter des fixtures communes pour vide, singleton intérieur, singleton sur coquille et `h=0`, puis les exercer sur census, q3, cover et témoins.

## P0 — préserver la multiplicité ou la refuser

L'index accepte plusieurs `PointId` à une même position et le WSPD les pondère. Pourtant [`census.hpp`](../src/pipeline/census.hpp) ajoute le nombre de positions uniques dans une plage, ajoute 1 à une feuille et matérialise un seul index géométrique par bucket. Deux supports et dix identités co-positionnées strictement dans leur boule donnent donc une profondeur calculée de 1 au lieu de 10.

La v5 doit choisir et documenter un seul contrat. La voie cohérente avec l'index actuel est d'utiliser `range_weight()` pour la profondeur et de définir l'expansion CSR vers tous les `PointId` lorsque les ensembles intérieur/coquille sont rendus. Si les doublons sont hors modèle, les refuser dès l'entrée. Une fixture de co-positionnement doit verrouiller la décision.

## P1 — portes à ajouter avant l'échelle

- Les fonctions annoncées « écrêtées » dans q3 et dans le juge exact de fuseau peuvent dépasser `cap` après un crédit de sous-arbre ou de bucket. Employer une addition saturée et tester un gros sous-arbre entièrement intérieur.
- Le ledger WSPD vérifie la masse totale, pas que chaque paire apparaît exactement une fois. Ajouter un oracle indépendant exhaustif à petit `n`, avec vérification de chaque certificat. Le juge fuseau actuel, partagé avec le sujet et limité à quelques ancres, reste une bonne régression mais pas un oracle complet.
- Le registre annonce 48 mutants. Au pin, 19 mutants déclarés ont un site, 29 n'en ont pas ; `cover-rect-dmin` a un site mais n'est pas enregistré. Seuls quatre mutants ont une porte directe attendue en code 4. Séparer `actifs` et `planifiés`, puis faire échouer une porte de cohérence si un mutant actif n'a pas exactement un site et au moins un CTest qui le tue.
- Les budgets `200 * n` et `400 * n` des générateurs sont calculés en `int` et débordent avant la cible de dizaines de millions. Valider `n`, employer un entier large et une multiplication vérifiée. Borner aussi les conversions `size_t` vers `u32`/`int` de l'index.
- Les sorties parallèles doivent être indexées par item ou tranche. L'affectation dynamique actuelle ne suffit pas, à elle seule, à garantir la fusion en ordre d'index annoncée par [`pool.hpp`](../src/parallel/pool.hpp).

## P1 — aligner la documentation sur le livré

[`README.md`](../README.md) présente au présent la forêt `K=1..10`, le streaming et les portes d'échelle, alors que le pin livre les fondations, le WSPD et les fuseaux. Ajouter une table courte « livré / prochain / non revendiqué ». Dix forêts horizontales ne sont pas encore la tour HGP complète : les applications verticales doivent rester un objet explicite.

[`docs/PROVENANCE.md`](../docs/PROVENANCE.md) référence des documents, sources et cibles encore absents au pin et intitule trop largement une section « Conformité v4 ≡ v5 ». La renommer en conformité différentielle bornée et marquer chaque preuve comme livrée ou planifiée. Le reçu de familles doit conserver le programme ou la commande, la toolchain, les hashes complets et l'état du worktree ; ses constantes C++ ne doivent pas devenir une seconde autorité.

Enfin, `ctest -L scale8000` sélectionne zéro test, `tools/check_docs.py` ne couvre aucun Markdown v5 et le guide racine affirme encore que la v5 n'a pas de CMake. Corriger ces annonces avant de demander une conclusion de coût ou de fraîcheur documentaire.

## Ordre de fermeture conseillé à Claude

1. Contrat racine vide/singleton et fixtures transverses.
2. Contrat de multiplicité et census pondéré ou rejet explicite.
3. Saturation des compteurs et limites d'entrée larges.
4. Oracle WSPD/fuseau à petit `n`, puis cohérence automatique des mutants.
5. Seulement ensuite, raccordement génération–forêt, différentiel d'objets v4/v5 et portes `scale8000` réelles.

## Vérifications exécutées

```bash
cmake -S morsehgp3D_v5 -B build/v5 -DCMAKE_BUILD_TYPE=Release
cmake --build build/v5 --parallel
ctest --test-dir build/v5 --output-on-failure
ctest --test-dir build/v5 --print-labels
ctest --test-dir build/v5 -N -L scale8000
python tools/check_docs.py
python tools/check_implementation_status.py
```

Résultats : configuration et build réussis ; `21/21` tests réussis en environ deux minutes ; seul label `gate` ; zéro test `scale8000` ; vérificateur documentaire vert sur 203 fichiers mais sans la v5 ; registre vert sur 20 phases, la v5 restant correctement hors registre.

Ce verdict n'établit ni produit complet, ni exactitude HGP, ni complexité asymptotique, ni résultat GPU. `public_status=not_claimed` reste la seule lecture autorisée.
