# Contre-fixture de budget du prototype MEB par pivots

5 septembre 2026. `phase=exploration_v7_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

**Résultat négatif confirmé, prototype non intégré.** Sur le triangle aigu
entier (0,0,0), (2,2,0), (2,0,2), D examine quatre supports, tandis que la
proposition en examine cinq. Au plafond 1, les deux routes rendent le même
refus et la même boule sentinelle, mais le prototype a déjà tenté cinq
supports : l'égalité des compteurs finaux ne préserve pas le plafond de
travail physique. Le delta E q2 ne contient pas ce prototype.

La [note de conception](../../docs/PROPOSITION_MEB_ET_BUDGETS.md) donne la
preuve du certificat régulier et de l'ordinal, puis cette obstruction.
Sa variante avec un budget prospectif de proposition distinct est
**future, non implémentée** ; la borne L_D+L_P n'est pas attribuée au
prototype archivé.

## Contenu et résultat exécuté

- `fixture.json` décrit les trois positions, caps et attentes du test.
- `counter_budget.historical.cpp`, `pivot.historical.hpp` et
  `run_counter_budget.historical.py` sont des copies exactes, non adaptées,
  des sources qui ont produit ce reçu.
- `receipt.json` est le reçu brut original : argv, codes de sortie, temps,
  47 pins sources avant/après, hashes des sorties et du binaire privé.
- `compiler.*`, `compile.*`, `execute.*` conservent intégralement stdout
  et stderr, fichiers vides compris ; `dependencies.historical.d` est le
  fichier de dépendances exact de cette compilation.
- `provenance.json` donne les chemins privés d'origine, les correspondances
  publiques, hashes et tailles ; `SHA256SUMS` couvre les fichiers publics
  de ce dossier, sauf lui-même.

Compilation C++20 -O0 avec -Wall -Wextra -Wpedantic -Werror, sans
MHGP7_TESTING : code 0 en 2,277 s. Exécution : code 0 en 0,004 s, deux
contre-cas confirmés, aucun stderr. Ces temps ne sont pas un benchmark.
Tous les pins sont stables. Le reçu n'est pas une qualification industrielle,
un test Gamma, une nouvelle suite Release ou un résultat GPU.

Le binaire privé n'est **pas publié**. Son hash reste dans le reçu historique,
avec son chemin d'origine, sans prétendre qu'il peut être trouvé dans cette
archive. Les 44 headers produit référencés ne sont pas recopiés : leurs
pins sont conservés dans le reçu. Celui-ci ajoute les trois sources
historiques pour un total de 47. Le contrôle d'export vérifie aussi la
note privée complète, soit 48 sources épinglées ; ce n'est pas un nouveau
run moteur ni un transfert de qualification à des sources ultérieures.

## Vérification et reproduction

Depuis ce dossier, vérifier l'intégrité par `sha256sum --check SHA256SUMS`.
Les chemins de cette liste sont relatifs à l'archive et n'utilisent ni
traversée de répertoire ni fichiers privés.

Les noms `historical` sont explicites : l'include du test pointe vers
`../v7_meb_pivot_prototype/pivot.hpp`, le runner déduit la racine depuis
son ancien emplacement, et le reçu contient les argv absolus réellement
exécutés. Les lancer directement depuis ce dossier serait incorrect.
Pour reproduire, restaurer dans un environnement de travail distinct les
44 headers exactement épinglés et les trois sources historiques à leurs
chemins relatifs d'origine indiqués dans `provenance.json`. Vérifier
leurs hashes avant compilation. Lancer alors le runner depuis cet arbre,
avec un dossier `build/v7_meb_fast_design/counter_budget_run_20260905`
inexistant : il est create-only et refuse tout résultat préexistant.

Un tel rejeu doit produire un nouveau reçu ; il ne peut pas réécrire ou
remplacer celui-ci. Des paths absolus et le hash du binaire peuvent différer
dans un autre environnement. Le critère est la contre-fixture et ses pins,
pas l'identité d'une latence. Aucun nouveau rejeu n'a été lancé pour l'export.

GCP non utilisé. Aucun fichier produit, tests ou CMake n'a été modifié.
