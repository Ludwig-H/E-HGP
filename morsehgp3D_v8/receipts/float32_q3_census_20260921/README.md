# Census q3 natif float32 : qualification et matrice

Statut borné : `exploration_v8_hors_registre`, `cpu_reference`,
`lossless_float32_input_only`, `not_claimed`.
Une arête fournie et un sous-arbre de graines du même index ; ni générateur
global, ni catalogue, ni tour FULL, ni GPU. GCP non utilisé.

## Captures closes

| Capture | Commandes | Résultat | SHA256 de COMPLETION.json |
|---|---:|---|---|
| [Release](release/q3_census_4d38rabq/COMPLETION.json) | 14 | PASS | `97d12e389f165c783bf7cc523d158ee3b0063e1dc279fc5d8ab54d71599e221d` |
| [Clang18 ASan/UBSan](sanitize/q3_census_1e7x60d4/COMPLETION.json) | 14 | PASS | `f530d91941aaab66cc40fb8c3d2e12d617dc846e2c35a5ea134ed991b2bad211` |
| [Matrice Release](matrix/matrix_wnpily8o/COMPLETION.json) | 36 | PASS | `706d96cbe1e328f5a9f39ec4cad6e6c0a629571546ac640943c0f40875b519fb` |

La capture instrumentée impose explicitement
`ASAN_OPTIONS=detect_leaks=1:halt_on_error=1` et
`UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`.
Les builds `build/v8_float32_q3_census_20260921` et
`build/v8_float32_q3_census_sanitize_20260921` sont désormais épinglés.
Ces trois captures officielles n'ont rencontré aucun échec ; cela ne
réinterprète pas les préflights ou mutations conservés séparément.

## Portée des portes

Chaque qualification compile cinq unités et un binaire dans un build neuf,
puis exécute la porte Python normalement et sous `-O` :

- 18 scènes, 612 requêtes contre un census global indépendant en `Fraction` ;
- 1 798 émissions et 8 610 IDs de coquille cumulés, coquille maximale 30 ;
- 37 entrées CLI invalides refusées et 16 corruptions de résultats détectées ;
- 458 contrôles natifs, dont 224 contrôles de bornes et 96 appels sous les
  environnements d'arrondi/FTZ/DAZ ;
- quatre lecteurs concurrents, 32 appels, contrôle de durée de vie du
  propriétaire et propagation d'une exception du callback. Pas de TSan.

Les 1 798 émissions ne sont pas un catalogue de boules distinctes.
Le compte porte sur tous les sites, y compris les graines invalides comme
témoins ; toute coquille acceptée est recherchée globalement et contient
aussi les points du support. Les qualifications ne testent pas un raccord
WSPD natif ni une propriété de plus longue arête.

Quinze sources locales sont épinglées : cinq unités C++, six headers,
deux portes Python (dont l'aide rationnelle explicitement réutilisée),
le runner et le collecteur. Chaque unité a ses propres dépendances `-M`
capturées avant toute compilation, puis ses fichiers `.d` et `.o` distincts.
Sources, snapshots, compilateur, dépendances et binaires sont rehachés à la
fermeture. Les commandes, environnements, codes et flux bruts sont conservés.
Le lecteur est **LIVE** : ce n'est pas une archive portable autonome.
Le champ `git_commit` désigne le HEAD au lancement (e2b09f94), avant
publication des nouveaux fichiers. Les snapshots et hashes de sources,
pas ce seul commit, identifient donc le contenu effectivement testé.

## Préflight et mutations compilées

Le [préflight incomplet](preflight/README.md) conserve une erreur manuelle
d'attendus de puissance dans le selftest : −1/2 et5/2 au lieu de−3/2
et3/2. Sa correction n'a pas changé le produit. Cette trace n'est pas
une fausse qualification close, ni une copie rétrospective de ses sources.

La [capture de mutations](mutations/COMPLETION.json) est close PASS,
37commandes, dans `build/v8_float32_q3_census_mutants_20260921` désormais
épinglé. SHA256 de COMPLETION :
`4520d1e628cb4e5debc7a002ff9bed5ecc28edc81e8ec6dc07582aa75aaf664b`.
Trois builds sur copies fraîches (référence et deux erreurs délibérées),
sources/dépendances/binaires hachés avant/après. Une colonne de9sites,
K5, deux modes, est jugée par Fraction ; les mutants doivent sortir
normalement avec code0, puis être réfutés par leurs sorties géométriques.

- Recompter à la racine avec le crédit transmis : profondeur2 devenue3,
  perte du quatrième support. Individual reste inchangé.
- Commencer la coquille au curseur de compte : les deux endpoints0/1
  disparaissent de toutes les coquilles partagées. Individual reste inchangé.

Les deux relectures normal/−O avec `--check-live` concordent :
[MUTATION_READBACKS.json](MUTATION_READBACKS.json). Aucun natif réexécuté
par ces lectures, aucun ancien header arithmétique/index modifié dans le lot.
Les sources communes d'index, de boules et d'entiers ainsi que leurs anciens
runners/portes n'ont pas été modifiés par ce lot. CMake et le moteur u16
restent inchangés ; la compilation autonome ci-dessus ne requalifie pas
implicitement toutes les anciennes portes ni leurs contrats.

## Matrice et relectures

Deux régimes synthétiques `column`/`slab`, n=8 000/16 000/32 000, Kmax=5/10,
Individual et SharedPrefix de grains 1/8 : 36 observations, une chacune.
Chaque observation reconstruit et paie son index ; celui-ci est ensuite
partagé par les graines de l'arête. Les 24 comparaisons de sorties appariées
passent. Pour `column`, le lecteur juge aussi le payload par une formule
analytique indépendante ; sur `slab`, le grand payload est différentiel,
pas un nouvel oracle exhaustif à 32k.

Les huit relectures explicites sont dans [READBACK.json](READBACK.json) :
Release et instrumenté normal/`-O`, matrice `read` et `selftest` normal/`-O`.
Chaque paire donne exactement le même résumé. Le lecteur matrice rejoue
douze corruptions, dont un bit XOR1 du digest, une suppression/duplication,
des paramètres erronés, des comptes négatif/booléen/chaîne et une masse
de graines fausse. Il ne lance aucun natif lors de ces lectures.

Les coûts, contre-limites et temps uniques non isolés sont détaillés dans
[ANALYSE_CROISSANCE.md](ANALYSE_CROISSANCE.md). Sur les 24 doublements observés,
aucun ratio défini parmi les 122 postes publiés n'atteint le seuil quadratique,
sans preuve générale ni extrapolation à toutes les arêtes ou aux trames
SemanticKITTI.

## Reproduction des lectures

Depuis la racine, après conservation du build et des sources épinglées :

```bash
python3 morsehgp3D_v8/bench/run_float32_q3_census_checks.py read --path morsehgp3D_v8/receipts/float32_q3_census_20260921/release/q3_census_4d38rabq
python3 morsehgp3D_v8/bench/run_float32_q3_census_checks.py read --path morsehgp3D_v8/receipts/float32_q3_census_20260921/sanitize/q3_census_1e7x60d4
python3 morsehgp3D_v8/bench/run_float32_q3_census_matrix.py read --path morsehgp3D_v8/receipts/float32_q3_census_20260921/matrix/matrix_wnpily8o --compact
python3 -O morsehgp3D_v8/bench/run_float32_q3_census_matrix.py selftest --path morsehgp3D_v8/receipts/float32_q3_census_20260921/matrix/matrix_wnpily8o --compact
python3 morsehgp3D_v8/tests/float32_q3_census_mutations.py read --path morsehgp3D_v8/receipts/float32_q3_census_20260921/mutations --check-live
```

Supprimer `--compact` restitue les 36 observations et les 24 comparaisons
de croissance pour chaque poste, avec valeurs avant/après et ratios.
