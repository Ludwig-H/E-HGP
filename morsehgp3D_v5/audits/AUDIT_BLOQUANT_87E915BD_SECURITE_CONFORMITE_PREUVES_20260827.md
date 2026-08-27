# Audit bloquant — sécurité API, conformité différentielle et preuves v5

- **Date :** 27 août 2026
- **Auditeur :** Codex
- **Pin de code jugé :** `87e915bd4596ca2db9bbf04ffb1373335529b379`
- **`origin/main` après fetch :** `87e915bd4596ca2db9bbf04ffb1373335529b379`
- **Capture du worktree :** `2026-08-27T08:37:24Z`
- **Cadre :** `phase=exploration_v5_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`
- **GCP :** non utilisé

Le pin et le worktree sont jugés séparément. Les résultats CTest ci-dessous
proviennent des exécutables Release compilés au pin. Les essais explicitement
nommés « correctif courant » proviennent d'une compilation directe des deux
sources suivies modifiées après ce pin. Les fichiers non suivis sont des
propositions : leur succès manuel ne les transforme ni en cibles CMake, ni en
preuves livrées.

## Empreinte du worktree jugé

Avant l'écriture de cet audit, le worktree contenait exactement deux fichiers
suivis modifiés :

```text
M  morsehgp3D_v5/src/core/mutants.hpp
M  morsehgp3D_v5/src/pipeline/generate.hpp
```

Le SHA-256 de `git diff --binary` était :

```text
74825df1471b983e10fd1ed9d5718e34dcded0d0b3a3ac2abc3a4db6b0c61fc8
```

Les propositions non suivies étaient :

```text
7556daaee1b5e027a514aa207cf720fe916e971b95ca0aa9083436dcffbdfd00  docs/MATHEMATIQUES.md
700f9fc8b229b5920916afa57d644dba6c0da6853294ecb4ce029494a181329b  tests/forest_judge.cpp
792cdbbe8c7753a4fa3d329c2264a94e7c525bdaf5a6cf3744bd095bfff73524  tests/q3_oracle.cpp
418a093e064203621bc7efaec1f573b623575eee3603bbdd30a649197e4ec5f1  tests/q4_oracle.cpp
ee762ff646e3bd4ff24afc1374d782251a5afecbc430efb5ef715c12649c3b6d  tests/q4_source_fixture.cpp
```

Toute modification de l'un de ces objets sort du périmètre exact de ce
verdict. Le commit qui contient l'audit n'absorbe volontairement aucun de ces
changements d'implémentation.

## Verdict exécutif

Le chantier reste une exploration utile, mais il est **refusé comme base de
claim**. Trois causes sont indépendamment bloquantes :

1. l'API bibliothèque possède deux chemins reproductibles de débordement
   mémoire sur des entrées non validées ;
2. le pin contredit ses propres reçus de conformité v4 sur les boules q4 ;
3. les preuves annoncées par CMake, le registre de mutants et la documentation
   ne correspondent pas au livré.

Le correctif q4 présent dans le worktree restaure exactement les deux cas
différentiels reproduits. Il est reçu comme une correction bornée et
mathématiquement sûre du filtre fail-open, pas comme une recertification de la
v5. Les autres blocages restent entiers.

## P0 — l'API bibliothèque peut déborder hors limites

[`run_pipeline`](../src/pipeline/run.hpp#L63) valide l'index, mais ne valide ni
la cardinalité vide, ni `s`, ni `smax`. Pour une entrée vide,
`smax_eff == 0`, puis [`kmax_eff = smax_eff - 1`](../src/pipeline/run.hpp#L79)
sous-déborde. Pour `smax = 12` et au moins douze points, `kmax_eff == 11` alors
que [`expand_events`](../src/pipeline/expand.hpp#L152) dimensionne plusieurs
structures à onze cases, donc aux indices valides `0..10`.

Deux harnesses minimaux compilés avec
`-fsanitize=address,undefined -fno-omit-frame-pointer` ont échoué :

```text
run_pipeline({})
  runtime error: index 11 out of bounds
  heap-buffer-overflow dans expand.hpp:189

RunOptions.smax = 12, n = 12
  heap-buffer-overflow dans expand.hpp:180
```

Le second diagnostic atteint l'écriture d'un événement d'ordre 11. Le code
contient les indexations fixes de
[`lev.assign(..., 11)`](../src/pipeline/expand.hpp#L160),
[`ev_k->assign(11, {})`](../src/pipeline/expand.hpp#L185) et
[`events_by_k[K]`](../src/pipeline/expand.hpp#L189).

Le pilote protège partiellement ces cas à
[`mhgp5.cpp:42`](../cli/mhgp5.cpp#L42), mais l'API annoncée dans `run.hpp` reste
directement appelable. Une garde CLI ne sécurise pas une bibliothèque.

**Condition de fermeture :** refuser avant tout calcul l'entrée vide et toute
option hors profil, avec le statut et le code de sortie contractuels ; ajouter
des fixtures ASan/UBSan permanentes pour vide, singleton, `smax=0`, `smax=1`,
`smax=11`, `smax=12`, `s<1`, `threads<=0` et les conversions de capacité.

## P0 — le pin falsifie son claim de conformité v4

Le pin utilisait un cover q4 de coefficient 4 dans la génération, alors que la
v4 utilise le coefficient 3 aux deux étages. Le cover sert ici à des minorants
de profondeur avant émission. Le coefficient 4 voit davantage d'intérieurs et
retire donc certaines boules déjà profondes. Le préfiltre global exact les
aurait retirées ensuite : la forêt finale ne change pas, mais
[`digest_balls`](../src/pipeline/run.hpp#L119) signe les candidats post-RLE
avant cette élimination.

Résultats au pin :

| Cas | v4 attendu | v5 au pin | Écart | `digest_all` |
|---|---:|---:|---:|---|
| `eight_clusters n=1200` | 285 949 boules | 285 948 | 1 q4 | égal |
| `uniform n=8000` | 3 134 427 boules | 3 134 404 | 23 q4 | égal |

La boule manquante du petit cas a la clé
`(2712,-198919,-939434,-201167,88336155)`. Sa profondeur globale vaut 8,
mais le cover coefficient 3 n'en voit que 7. Le point manquant vérifie
`3D2=1215 < dist2q=1237 <= 4D2=1620`. Les 23 cas `uniform` ont le même profil :
profondeur globale 8, coquille 4, profondeur vue par le cover 3 égale à 7.

Le claim de conformité de
[`ETAT_COURANT.md`](ETAT_COURANT.md), du
[`README` v5](../README.md#L7), du
[`README` racine](../../README.md#L12), de
[`ARCHITECTURE.md`](../docs/ARCHITECTURE.md#L10) et de la
[`QUESTION_CLAUDE`](QUESTION_CLAUDE_VERROUS_OUVERTURE_20260827.md#L3) était
donc faux au pin.

### Évaluation du correctif courant

Le worktree fixe le coefficient 3 pour la découverte et les minorants q4. Ce
choix restaure exactement les reçus :

```text
eight_clusters n=1200 : digest_balls égal, digest_all égal, code 0
uniform n=8000        : digest_balls égal, digest_all égal, code 0
```

La correction est sûre pour l'objet : les sommets d'un tétraèdre possédé par
son arête maximale vivent dans la lentille, donc dans le cover 3 ; les autres
usages du cover ne sont que des minorants fail-open ; le census global reste
l'autorité de profondeur. Le coût est l'émission de quelques candidats
profonds supplémentaires, pas une perte d'événement.

Elle ne peut toutefois pas être livrée avec la fixture proposée en l'état.
La proposition non suivie `tests/q4_source_fixture.cpp` décrit encore le
coefficient 4 comme production et son « plancher de mécanisme » teste seulement
le compteur global `depth_killed[2] >= 1`, pas la boule ciblée. Ce plancher reste
vert sous le coefficient 3 et ne prouve donc pas son commentaire. Les variantes
nominales ne sont pas toutes valides : `--fixture=13` sort 3 sur un contrat
contradictoire de l'ancre `xy`, et `--fixture=23` sort 3 avec neuf violations
d'intériorité ; seules les variantes 22 et `13+8` essayées sortent 0.

**Condition de fermeture :** graver la clé différentielle minimale, vérifier
son sort précis avant et après le préfiltre, nettoyer les références au mutant
retiré, puis rejouer toutes les portes `gate` et les quatre cas `scale8000` sur
un build frais.

## P0 — les entrées documentaires n'étaient pas fraîches

Avant cet audit, [`ETAT_COURANT.md`](ETAT_COURANT.md) était ancré au commit
`4bcd29af`, trois commits fonctionnels derrière le pin jugé. Il disait encore
que le rendu et le relabeling n'étaient pas livrés. Cette obsolescence suffit à
interdire tout claim selon [`audits/README.md`](README.md#L22), indépendamment
des défauts techniques ci-dessus.

Le [`README` v5](../README.md) a été touché pour la dernière fois au commit
`8600c53b`, six commits derrière le pin fonctionnel. Il porte encore la
conformité falsifiée, les surclaims de complétude et la référence à
`docs/PISTES_FERMEES.md`, absent. Comme le présent commit reste volontairement
borné à `audits/`, ce README demeurera non frais après publication. L'exigence
de fraîcheur conjointe du README v5 et de l'état courant interdit donc encore
tout claim, même si le verdict d'audit lui-même devient frais.

Le document mathématique annoncé comme autorité par le README n'existe pas au
pin : il est seulement non suivi. `docs/PISTES_FERMEES.md`, également annoncé,
n'existe ni au pin ni dans le worktree.

## P1 — le contrat vide/singleton reste cassé dans le census

Un singleton valide possède `nodes.empty() == true` et
`root() == leaf_ref(0)`. Pourtant
[`ball_depth_at_least`](../src/pipeline/census.hpp#L67) et
[`ball_census`](../src/pipeline/census.hpp#L103) interprètent encore
`nodes.empty()` comme un nuage vide.

Reproduction avec le point `(1,1,1)` et la forme
`P(z) = x² + y² + z² - 4` :

```text
valid=1 unique=1 nodes=0 at_least=0 count=0 status=0 interior=0 shell=0
```

Le point est strictement intérieur. Pour un seuil de 1, le résultat contractuel
attendu est le booléen `true` et un intérieur matérialisé par `ball_census` ; le
paramètre `count` de `ball_depth_at_least` n'est garanti exact que lorsque le
booléen retourné est `false`. Avec un seuil de 2, le résultat attendu est
`false` et `count=1`. Le court-circuit actuel renvoie à tort `false`, `count=0`
et un census vide dans les deux lectures.

Le pipeline refuse actuellement les positions dupliquées, ce qui est un choix
contractuel cohérent tant que le HGP pondéré n'est pas défini. Les API basses
doivent néanmoins soit imposer explicitement la même précondition, soit compter
les multiplicités par `range_weight()` ; elles ne doivent pas accepter un index
bucketisé puis compter silencieusement les seules positions uniques.

## P1 — les mutants ne sont ni exhaustivement tués ni isolés du produit

Au worktree capturé, 40 mutants sont déclarés, mais CTest n'enregistre que 30
portes attendues en code 4 pour 29 noms distincts : `dense-pointid` est doublé.
Onze noms n'ont aucune porte CTest à code 4 :

```text
attach-detector-disabled
cover-rect-dmin
level-trunc-hi
obig-carry-lost
q3-prune-ge
q4-eq-nonstrict
q4-no-canonical
q4-seed-core-nonstrict
q4-seeds-from-q3-live
wspd-cap-terminal
wspd-split-heaviest
```

Le pin ajoute `q4-cover-coef3` à cette liste. Les deux portes WSPD appariées
sont attendues en code 0, pas en code 4. Les neuf mutants fail-open enregistrés
au pin, dont `q4-cover-coef3`, survivent tous à la porte générique sur
`eight_clusters n=400` avec code 3 ; le worktree, qui retire ce mutant, en
laisse huit. Les deux digests restent inchangés dans chacun de ces essais.

[`mutants_gate.py`](../tests/mutants_gate.py#L18) compare seulement deux
ensembles de chaînes. Il ne vérifie ni la présence d'une cible CMake, ni son
code attendu, ni sa capacité à tuer le mutant. Il accepte comme sites les
commentaires, les blocs non compilés et les fichiers non suivis. La preuve est
directe : le worktree le rend rouge à cause de `q3-cramer-swap` et
`q3-sign-p`, présents seulement dans l'oracle q3 non suivi. Il vérifie en outre
« au moins un site », alors que
[`ARCHITECTURE.md`](../docs/ARCHITECTURE.md#L105) annonce exactement un site par
mutant ; `level-trunc-hi` possède deux sites dans `src/core/wide.hpp`.

La porte différentielle a en outre un faux positif structurel. Avec
`--inject`, [`conformity_v4.cpp`](../tests/conformity_v4.cpp#L68) classe toute
divergence préexistante du reçu ou tout statut changé comme « mutant tué »,
sans exécuter un bras nominal apparié. Au pin et avec le vrai reçu,
`eight_clusters n=1200` diverge déjà sur `digest_balls` ; l'injection de
`q3-prune-ge` conserve exactement les deux digests du nominal défaillant, mais
la porte sort tout de même en code 4 et annonce le mutant tué. Le même faux
positif a été reproduit en altérant volontairement un reçu pour
`attach-detector-disabled`, qui laisse lui aussi les digests nominaux
inchangés.

Enfin, le pilote produit expose
[`--inject`](../cli/mhgp5.cpp#L35). Un run avec
`attach-detector-disabled` peut sortir en code 0 et imprimer des digests sans
marqueur de mutation. Cela contredit la doctrine « jamais une option produit »
et permet de confondre un reçu mutant avec un run nominal. Le macro conserve
également une garde statique dynamique et un test dans les boucles chaudes ; le
« coût nul » annoncé n'est pas établi.

**Condition de fermeture :** compiler les mutants uniquement dans des cibles
de test, apparier chaque porte à son bras nominal dans la même invocation,
contrôler mécaniquement registre, site compilé, cible CMake et code 4, puis
ajouter les quatre fixtures encore réellement absentes : `cover-rect-dmin`,
`q4-no-canonical`, `q4-seed-core-nonstrict` et
`attach-detector-disabled`.

## P1 — les oracles annoncés ne sont pas des portes livrées

CTest expose 74 tests, mais aucun label `oracle`. Deux tests suivis sont les
seuls fichiers `.cpp` suivis absents de CMake :

- `tests/obig_selftest.cpp` : compilation stricte réussie, nominal code 0,
  `obig-carry-lost` code 4 ; Boost absent, donc la troisième autorité
  `cpp_int` n'a pas été exercée ;
- `tests/level_cmp.cpp` : compilation stricte réussie, nominal code 0,
  `level-trunc-hi` code 4.

Les propositions non suivies donnent en exécution manuelle :

| Proposition | Résultat |
|---|---|
| `q3_oracle.cpp` | nominal code 0, aucun désaccord |
| `q4_oracle.cpp` | nominal code 0, aucun désaccord |
| `q4_source_fixture.cpp` | variantes 22 et `13+8` code 0 ; variantes 13 et 23 code 3 ; plancher non spécifique |
| `forest_judge.cpp` | ne compile pas sous `-Werror` |

`forest_judge.cpp` a un paramètre `K` inutilisé et accède au membre inexistant
`ForestResult::events`. Il ne vérifie pas non plus les compteurs supprimés par
`attach-detector-disabled`.

Un succès manuel hors CMake est une information de chantier, pas une preuve de
régression. Les lignes q3/q4/niveaux/forêt de
[`PLAN_DE_TESTS.md`](../docs/PLAN_DE_TESTS.md#L49) décrivent donc une cible,
pas le livré.

## P1 — les contrats de sortie et de résidence ne décrivent pas le code

### Publication transactionnelle

[`ARCHITECTURE.md`](../docs/ARCHITECTURE.md#L23) promet qu'aucun préfixe de
payload n'est publié sur un refus. Or `run_pipeline` appelle le consommateur à
[`run.hpp:148`](../src/pipeline/run.hpp#L148) après chaque ordre, avant de
savoir si un ordre ultérieur refusera à
[`run.hpp:128`](../src/pipeline/run.hpp#L128). La promesse transactionnelle ne
peut donc pas être déduite de cette API.

### Résidence

La table mémoire annonce seulement les événements du K courant. En réalité,
[`expand_events`](../src/pipeline/expand.hpp#L151) construit les onze vecteurs
d'événements, puis [`run_pipeline`](../src/pipeline/run.hpp#L113) les conserve
tous pendant les folds et les libère progressivement. La v5 évite bien dix
`ForestResult` résidents, mais ne streame pas encore l'expansion par K.

### Complétude de la tour

Le document mathématique non suivi précise que la sortie complète requiert les
applications verticales et que la v5 ne les construit pas. `RunResult` expose
des forêts horizontales indépendantes, leurs cardinalités et leurs digests.
Les expressions « forêt HGP complète K=1..10 » et « même objet » sont donc trop
larges tant que l'objet reconstructible et les applications verticales ne sont
pas livrés et jugés.

## P1 — provenance et reçus insuffisants

[`PROVENANCE.md`](../docs/PROVENANCE.md#L18) dit couvrir les modules un par un,
mais s'arrête au census. Il omet notamment `generate`, `expand`, `digest`,
`run`, `fold`, `plateau`, `render`, `device` et la CLI. Plusieurs portes citées
n'existent pas dans CMake.

Le reçu `receipts/conformite_v4/digests_v4.txt` conserve des digests v4 et une
commande, mais pas les sorties et codes appariés v5, le hash du binaire, la
toolchain complète, l'état du worktree ou un manifeste immuable par campagne.
La porte lit seulement `(famille, n, digest_balls, digest_all)` ; elle ne
valide ni le pin déclaré dans l'en-tête ni la provenance du fichier.

Le digest différentiel reste une bonne porte de régression. Il prouve « même
sérialisation que la v4 sur cette entrée », pas l'exactitude mathématique, la
complétude de la tour ni la capacité produit.

## P2 — les contrôleurs documentaires donnent un vert hors périmètre

`python tools/check_docs.py` réussit sur 203 fichiers, mais
[`active_markdown()`](../../tools/check_docs.py#L30) n'inclut aucun Markdown
v5. Son succès ne vérifie donc ni les liens, ni les règles KaTeX, ni la
fraîcheur des documents de ce chantier.

`python tools/check_implementation_status.py` réussit sur 20 phases. C'est le
résultat attendu : la v5 reste correctement hors registre et aucune promotion
n'a été tentée.

## Campagnes exécutées

| Commande ou contrôle | Résultat | Portée |
|---|---|---|
| configuration et build Release canoniques | réussi | pin compilé |
| `ctest --test-dir build/v5 -L '^gate$' -j1` | 59/61, 351,02 s | binaires du pin ; scan mutant sur le worktree |
| conformité `eight_clusters n=1200` | code 1 au pin | une boule q4 manque, forêt égale |
| conformité `uniform n=8000` | code 1 au pin | 23 boules q4 manquent, forêt égale |
| mêmes deux conformités, correctif courant | codes 0 | correction bornée du worktree |
| `ctest -N -L '^oracle$'` | zéro test | aucun oracle enregistré |
| `obig_selftest` manuel | nominal 0, mutant 4 | test suivi mais orphelin |
| `level_cmp` manuel | nominal 0, mutant 4 | test suivi mais orphelin |
| oracles q3/q4 manuels | nominaux 0 | propositions non suivies |
| fixture source q4 manuelle | 22 et `13+8` : 0 ; 13 et 23 : 3 | proposition non suivie |
| `forest_judge.cpp` sous flags stricts | échec de compilation | proposition non suivie |
| harnesses ASan vide et `smax=12` | deux débordements | API bibliothèque |
| fixture census singleton | résultat faux reproduit | API basse |
| `python tools/check_docs.py` | 203 fichiers validés | aucun Markdown v5 |
| validation directe par `tools.check_docs.validate` | cinq fichiers, zéro erreur | dossier d'audit v5 |
| `python tools/check_implementation_status.py` | 20 phases validées | v5 hors registre |
| `git diff --check` | réussi | worktree capturé |

La CI GitHub ne construit pas la v5. Tous les résultats ci-dessus sont locaux,
CPU seulement. Aucun résultat GPU n'est déclaré.

## Protocole exact de reproduction

Toutes les commandes partent de la racine du dépôt. La campagne utilisait
`c++ 13.3.0`, `cmake 3.28.3` et `ctest 3.28.3`. Le répertoire `build/v5`
avait été construit au pin fonctionnel avant l'apparition des deux
modifications suivies ; le binaire `/tmp/mhgp5_conformity_current_audit` a été
compilé directement depuis le worktree capturé.

### Build et portes enregistrées

```sh
cmake -S morsehgp3D_v5 -B build/v5 -DCMAKE_BUILD_TYPE=Release
cmake --build build/v5 --parallel
ctest --test-dir build/v5 --output-on-failure -L '^gate$' -j1
ctest --test-dir build/v5 -N -L '^oracle$'
```

### Débordement sur l'entrée vide

```sh
g++ -std=c++20 -O1 -g -fsanitize=address,undefined \
  -fno-omit-frame-pointer -Wall -Wextra -Wpedantic -Werror -pthread \
  -x c++ -o /tmp/mhgp5_empty_audit - <<'CPP'
#include "morsehgp3D_v5/src/pipeline/run.hpp"
int main() {
  const std::vector<mhgp5::InputPoint> input;
  const mhgp5::RunOptions options;
  const mhgp5::RunResult result = mhgp5::run_pipeline(input, options);
  return mhgp5::status_exit_code(result.status);
}
CPP
ASAN_OPTIONS=abort_on_error=1:halt_on_error=1 /tmp/mhgp5_empty_audit
```

Résultat : diagnostic UBSan d'indice 11 hors bornes, puis
heap-buffer-overflow à `expand.hpp:189` ; arrêt par `SIGABRT`.

### Débordement sur `smax=12`

```sh
g++ -std=c++20 -O1 -g -fsanitize=address,undefined \
  -fno-omit-frame-pointer -Wall -Wextra -Wpedantic -Werror -pthread \
  -x c++ -o /tmp/mhgp5_smax12_audit - <<'CPP'
#include "morsehgp3D_v5/src/cloud/families.hpp"
#include "morsehgp3D_v5/src/pipeline/run.hpp"
int main() {
  mhgp5::RunOptions options;
  options.smax = 12;
  const auto input =
      mhgp5::make_family_input(mhgp5::CloudFamily::kUniform, 12, 12, 3);
  const mhgp5::RunResult result = mhgp5::run_pipeline(input, options);
  return mhgp5::status_exit_code(result.status);
}
CPP
ASAN_OPTIONS=abort_on_error=1:halt_on_error=1 /tmp/mhgp5_smax12_audit
```

Résultat : heap-buffer-overflow sur `lev[c][K].push_back` à
`expand.hpp:180` ; arrêt par `SIGABRT`.

### Census singleton

```sh
g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror \
  -x c++ -o /tmp/mhgp5_singleton_audit - <<'CPP'
#include <cstdio>
#include <vector>
#include "morsehgp3D_v5/src/pipeline/census.hpp"
int main() {
  const mhgp5::CloudIndex index =
      mhgp5::build_cloud_index(std::vector<mhgp5::P3>{{1, 1, 1}});
  const mhgp5::BallKey key{1, {0, 0, 0}, -4};
  mhgp5::u64 count = 0;
  mhgp5::DepthStats stats;
  const bool at_least =
      mhgp5::ball_depth_at_least(index, key, 1, &count, &stats);
  std::vector<mhgp5::i32> interior, shell;
  const auto status =
      mhgp5::ball_census(index, key, 1, 4, &interior, &shell);
  std::printf("valid=%d unique=%d nodes=%zu at_least=%d count=%llu "
              "status=%d interior=%zu shell=%zu\n",
              index.valid, index.unique_count(), index.nodes.size(), at_least,
              (unsigned long long)count, (int)status, interior.size(),
              shell.size());
}
CPP
/tmp/mhgp5_singleton_audit
```

La sortie exacte est celle reproduite dans la section singleton.

### Conformité différentielle du pin et du correctif capturé

```sh
/usr/bin/c++ -O3 -DNDEBUG -std=c++20 -Wall -Wextra -Wpedantic -Werror \
  morsehgp3D_v5/tests/conformity_v4.cpp \
  -o /tmp/mhgp5_conformity_current_audit
build/v5/mhgp5_conformity_v4 \
  --receipt=morsehgp3D_v5/receipts/conformite_v4/digests_v4.txt \
  --family=eight_clusters --n=1200 --threads=8
/tmp/mhgp5_conformity_current_audit \
  --receipt=morsehgp3D_v5/receipts/conformite_v4/digests_v4.txt \
  --family=eight_clusters --n=1200 --threads=8
build/v5/mhgp5_conformity_v4 \
  --receipt=morsehgp3D_v5/receipts/conformite_v4/digests_v4.txt \
  --family=uniform --n=8000 --threads=8
/tmp/mhgp5_conformity_current_audit \
  --receipt=morsehgp3D_v5/receipts/conformite_v4/digests_v4.txt \
  --family=uniform --n=8000 --threads=8
```

Les codes successifs sont `1`, `0`, `1`, `0`. La comparaison pin/correctif
n'est reproductible à l'identique que tant que le patch fonctionnel possède
l'empreinte déclarée en tête de ce rapport ; ce patch n'est volontairement
pas inclus dans le commit d'audit.

### Tests suivis orphelins et propositions non suivies

```sh
/usr/bin/c++ -O3 -DNDEBUG -std=c++20 -Wall -Wextra -Wpedantic -Werror \
  morsehgp3D_v5/tests/obig_selftest.cpp -o /tmp/mhgp5_obig_selftest_audit
/tmp/mhgp5_obig_selftest_audit
/tmp/mhgp5_obig_selftest_audit --inject=obig-carry-lost
/usr/bin/c++ -O3 -DNDEBUG -std=c++20 -Wall -Wextra -Wpedantic -Werror \
  morsehgp3D_v5/tests/level_cmp.cpp -o /tmp/mhgp5_level_cmp_audit
/tmp/mhgp5_level_cmp_audit
/tmp/mhgp5_level_cmp_audit --inject=level-trunc-hi
```

Les codes sont respectivement `0`, `4`, `0`, `4`. Les commandes suivantes ne
sont rejouables que sur les propositions non suivies possédant les hashes de
la capture ; leur absence du pin fait précisément partie du verdict :

```sh
/usr/bin/c++ -O3 -DNDEBUG -std=c++20 -Wall -Wextra -Wpedantic -Werror \
  morsehgp3D_v5/tests/q3_oracle.cpp -o /tmp/mhgp5_q3_audit
/tmp/mhgp5_q3_audit
/usr/bin/c++ -O3 -DNDEBUG -std=c++20 -Wall -Wextra -Wpedantic -Werror \
  morsehgp3D_v5/tests/q4_oracle.cpp -o /tmp/mhgp5_q4_audit
/tmp/mhgp5_q4_audit
/usr/bin/c++ -O3 -DNDEBUG -std=c++20 -Wall -Wextra -Wpedantic -Werror \
  morsehgp3D_v5/tests/q4_source_fixture.cpp -o /tmp/mhgp5_q4_source_audit
for fixture in 13 22 23 13+8; do
  /tmp/mhgp5_q4_source_audit --fixture="$fixture"
done
/usr/bin/c++ -O3 -DNDEBUG -std=c++20 -Wall -Wextra -Wpedantic -Werror \
  -fsyntax-only morsehgp3D_v5/tests/forest_judge.cpp
```

Les deux oracles nominaux sortent 0. Les quatre variantes de la fixture q4
sortent `3`, `0`, `3`, `0`. La dernière compilation sort 1.

### Contrôleurs documentaires

```sh
python3 tools/check_docs.py
python3 - <<'PY'
import sys
from pathlib import Path
sys.path.insert(0, "tools")
import check_docs
errors = []
paths = sorted(Path("morsehgp3D_v5/audits").glob("*.md"))
for path in paths:
    errors.extend(check_docs.validate(path.resolve()))
if errors:
    print("\n".join(errors))
    raise SystemExit(1)
print(f"Validated {len(paths)} v5 audit Markdown files.")
PY
python3 tools/check_implementation_status.py
git diff --check
```

## Ordre impératif de fermeture

1. sécuriser l'entrée de `run_pipeline`, le vide/singleton et les bornes
   d'index/capacité, sous sanitizers ;
2. livrer le correctif q4 avec une fixture ciblée, puis rejouer `gate` et les
   quatre `scale8000` sur un build frais ;
3. isoler les mutants des cibles produit, renforcer la porte mécanique et
   fermer chaque mutant par code 4 apparié ;
4. câbler `obig_selftest`, `level_cmp`, les oracles q3/q4 et seulement ensuite
   le juge de forêt corrigé ;
5. choisir et implémenter le contrat transactionnel, la vraie résidence par K
   et les applications verticales, ou réduire explicitement l'objet revendiqué ;
6. compléter provenance et reçus, inclure la v5 dans le contrôleur documentaire
   et refaire auditer un pin propre.

Ce verdict n'établit ni exactitude HGP, ni forêt complète, ni complexité, ni
capacité, ni résultat GPU. Il ne promeut aucun statut.
