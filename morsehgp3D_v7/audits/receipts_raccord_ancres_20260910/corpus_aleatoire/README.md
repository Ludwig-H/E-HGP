# Corpus différentiel de nuages entiers aléatoires — raccord FULL par ancres de boule (WIP 13:04 UTC)

10 septembre 2026, sous-auditeur indépendant. Dépôt `/workspaces/E-HGP` à HEAD `514038ed`, **aucune écriture dans le dépôt**, aucun GCP, aucun agent imbriqué. Cadre : `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

Objet jugé : le producteur `build_full_ball_tower` de l'overlay hors dépôt `../../overlay_wip_130402/` (copie figée du WIP, `src/forest/full_ball_tower.hpp` sha256 `4929a542…`, conforme à `../snapshots/SHA256SUMS`, vérifié par `review.json:overlay_matches_snapshot_sha256sums`). Le juge est un modèle rationnel Python qui **n'inclut aucun en-tête produit** ; le seul lien est le pont `tower_bridge.cpp`, compilé contre l'overlay et piloté par stdin/stdout.

## Fichiers

| Fichier | Rôle |
| --- | --- |
| `tower_bridge.cpp` | Pont : lit `point`/`ball`/`cut`/`kmax`/`run`, construit l'index et la tour, imprime statut/raison puis, par (K, coupe, côté), les couvertures triées des racines vivantes (`full_coverage_at`) et la couverture de leur image verticale à K−1 à la même coupe (`full_ball_vertical_root_at`). Codes : 0 tour exécutée (refus produit inclus dans le JSON), 2 entrée invalide, 3 construction impossible. |
| `tower_corpus.py` | Juge et générateur : géométrie Gram rationnelle (logique copiée en lecture de `audits/meb_rational_oracle_20260905.py:circumball`), toutes les boules candidates (sous-ensembles de 2 à 4 points, centre dans l'enveloppe convexe fermée), dédoublonnage par clé primitive, I/U/q_min, fenêtre `p+q_min ≤ min(kmax+1,n)`, référence Γ par balayage incrémental, images verticales certifiées à chaque activation et fusion, comparaison des multi-ensembles de couvertures et des images. Mutants `drop_extra_ball`, `wrong_arity`, `ref_open_as_closed`. Familles `random` et `cocircular`. |
| `silent_mutant_analysis.py`, `redundant_merge_check.py` | Explication des nuages où `drop_extra_ball` reste silencieux. |
| `shell_distribution.py` | Distributions u, p, q_min, u−q_min et tailles de lots égaux dans les catalogues. |
| `make_review.py` → `review.json` | sha256 des sources lues et des artefacts, commandes, temps, comptes. |
| `corpus_*.json`, `mutants/*.json`, `distributions_*.json`, `logs/` | Sorties. `logs/*.stderr` = progression, `logs/*.time` = temps réels, `logs/git_status_after.txt` = état du dépôt après la session (aucun de mes fichiers). |

## Reproduction

```bash
cd /tmp/claude-1000/-workspaces-E-HGP/6300a9ba-5bd7-42ea-91b3-d2c07c163916/scratchpad/receipts_raccord_ancres_20260910/corpus_aleatoire
g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -I ../../overlay_wip_130402 tower_bridge.cpp -o bin/tower_bridge
python3 -B    tower_corpus.py --bridge bin/tower_bridge --clouds 200 --seed 20260910 > corpus_normal.json
python3 -B -O tower_corpus.py --bridge bin/tower_bridge --clouds 200 --seed 20260910 > corpus_optimized.json
cmp corpus_normal.json corpus_optimized.json
python3 -B    tower_corpus.py --bridge bin/tower_bridge --clouds 2000 --seed 20260911 --no-fixtures > corpus_ext_normal.json
python3 -B -O tower_corpus.py --bridge bin/tower_bridge --clouds 2000 --seed 20260911 --no-fixtures > corpus_ext_optimized.json
python3 -B    tower_corpus.py --bridge bin/tower_bridge --clouds 300 --seed 20260912 --no-fixtures --family cocircular > corpus_cocircular_normal.json
python3 -B -O tower_corpus.py --bridge bin/tower_bridge --clouds 300 --seed 20260912 --no-fixtures --family cocircular > corpus_cocircular_optimized.json
python3 -B tower_corpus.py --bridge bin/tower_bridge --clouds 200 --seed 20260910 --mutant drop_extra_ball   > mutants/drop_extra_ball.json    # attendu : exit 1
python3 -B tower_corpus.py --bridge bin/tower_bridge --clouds 200 --seed 20260910 --mutant wrong_arity       > mutants/wrong_arity.json        # attendu : exit 1
python3 -B tower_corpus.py --bridge bin/tower_bridge --clouds 200 --seed 20260910 --mutant ref_open_as_closed > mutants/ref_open_as_closed.json # attendu : exit 1
python3 -B silent_mutant_analysis.py mutants/drop_extra_ball.json > mutants/drop_extra_ball_silent_analysis.json
python3 -B redundant_merge_check.py mutants/drop_extra_ball_silent_analysis.json > mutants/drop_extra_ball_redundant_merge.json
for f in corpus_normal corpus_ext_normal corpus_cocircular_normal; do python3 -B shell_distribution.py $f.json > distributions_$f.json; done
python3 -B make_review.py > review.json
```

Le code de sortie de `tower_corpus.py` vaut 0 si `status=passed` (aucun refus produit, aucune divergence), 1 sinon ; un défaut du juge ou du pont donne 2/3 avec `REFUSAL …` sur stderr.

## Ce qui est comparé

Pour chaque nuage (n = 5..12 points entiers distincts, PointId u32 arbitraires non consécutifs, kmax ∈ [2, min(n, 6)]), pour chaque ordre K = 1..kmax, chaque coupe t ∈ {0} ∪ {r² de toutes les boules candidates} ∪ {max+1} et chaque côté ouvert (r² < t) / fermé (r² ≤ t) :

1. le multi-ensemble des couvertures des racines produit vivantes (`full_coverage_root_at`/`full_coverage_at`) contre celui des composantes Γ de référence (K-facettes reliées par (K+1)-cofaces de même critère, couverture = union des points) ;
2. pour chaque racine produit dont la couverture est unique à cette coupe : l'égalité de la couverture de l'image verticale (`full_ball_vertical_root_at` puis `full_coverage_at` à K−1, même coupe et côté) avec celle de la composante de K−1 contenant toutes les (K−1)-sous-facettes ; à K = 1, l'image doit être absente ;
3. les couvertures dupliquées à une même coupe sont comptées comme ambiguïtés et ne sont pas jugées.

L'unicité de l'image de référence est vérifiée dans le balayage à chaque activation de K-facette (toutes ses sous-facettes dans une même composante de K−1) et à chaque fusion (images des deux parties égales), ce qui couvre toutes les coupes par induction. Le modèle vérifie aussi l'unicité de la MEB de chaque sous-ensemble (aucune égalité de rayon entre deux boules contenant le même sous-ensemble), la présence d'un support positif dans le sous-ensemble, la monotonie des niveaux, et l'égalité du q_min fermé et du q_min positif.

## Résultats (chiffres tirés de `review.json`)

| Campagne | Nuages | Ordres | Coupes | Racines | Images | Divergences | Refus produit | Octets `-B` = `-B -O` | Temps réel |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- | --- |
| principale (seed 20260910, 200 aléatoires + 7 fixtures) | 207 | 795 | 69 694 | 119 751 | 93 038 | 0 | 0 | oui | 14,5 s / 16,6 s |
| étendue (seed 20260911, 2000 aléatoires) | 2000 | 7 773 | 633 958 | 1 059 397 | 811 601 | 0 | 0 | oui | 2 min 30 s / 2 min 16 s |
| cocirculaire (seed 20260912, 300 sur x²+y²=25 + intrus) | 300 | 1 177 | 73 976 | 134 179 | 100 459 | 0 | 0 | oui | 17,0 s / 17,0 s |

Coquilles supplémentaires (u > q_min) réellement présentes dans les catalogues : 2 120 (principale), 22 050 (étendue), 1 053 (cocirculaire) ; u jusqu'à 9 en aléatoire et jusqu'à 12 en cocirculaire (`distributions_*.json`). Lots à niveau égal (≥ 2 boules planifiées au même ordre et au même rayon) : 3 303 / 32 805 / 3 548, avec des lots jusqu'à 16 boules. Côté produit (sommes des compteurs) : `extra_blocks` 5 363 / 56 015 / 2 756, `intruder_queries` 95 / 1 119 / 483, `anchor_hits` 17 358 / 166 249 / 18 613. Ambiguïtés (couvertures dupliquées) : 0 partout, donc toutes les images des racines vivantes ont été jugées.

Branches faiblement exercées : `same_radius_steps` n'apparaît que sur la fixture `actual_equal_radius_descent` de la gate v2 (1 pas, 0 divergence), jamais sur un nuage aléatoire ; aucune couverture dupliquée n'a été rencontrée.

## Mutants (non-vacuité du juge)

| Mutant | Effet | Résultat |
| --- | --- | --- |
| `drop_extra_ball` (retire du catalogue une boule à u > q_min) | 199 nuages mutés | 148 refus produit (`full_ball_missing_weak_terminal` 125, `full_ball_final_component_count` 23), 12 nuages divergents (128 coupes), 39 nuages silencieux |
| `wrong_arity` (arité := u sur une telle boule) | 199 nuages mutés | 199 refus (`full_ball_census_geometry` 189, `full_ball_census_shape` 10 quand u > 4) |
| `ref_open_as_closed` (la référence lit l'ouvert comme le fermé) | référence seule | 8 458 divergences sur 207/207 nuages |

Les 39 nuages silencieux de `drop_extra_ball` sont tous expliqués sans mettre en cause le produit (`mutants/drop_extra_ball_silent_analysis.json`, `mutants/drop_extra_ball_redundant_merge.json`) : 27 boules inertes à tous leurs rangs planifiés (une seule composante stricte locale couvrant S), 11 boules dont les composantes strictes locales ont un seul parent global à la coupe ouverte et couvrent S (pont extérieur, `README.md` de l'auditeur § 3) ou dont le niveau est globalement muet à ces rangs, 1 boule dont la fusion de deux parents est portée par une autre boule du même lot égal. Dans ces cas la boule ne porte qu'une ancre jamais demandée par le resolver : sa suppression n'a pas d'effet observable sur les couvertures ni sur les images, et la sortie produit reste conforme à la référence.

## Limites

Nuages bornés (n ≤ 12, coordonnées ≤ 10) : aucune conclusion d'échelle, de coût, de mémoire ni de complétude WSPD. Seules les couvertures et les images verticales sont comparées ; les identités de nœuds, les dates de toutes les facettes et les poids du manuscrit ne le sont pas. `public_status=not_claimed`.
