# Architecture de la v11

Document normatif pour le code de `morsehgp3D_v11/`. Il fixe ce que « propre » veut dire ici, la carte des modules,
le profil numérique et les conventions de construction et de test. Les énoncés mathématiques vivent dans
`MATHEMATIQUES.md` (à venir avec le moteur) ; ce document n'en recopie aucun.

## 1. Règles de propreté

Chaque règle est vérifiable ; `tools/check_style.py` contrôle celles qui se lisent dans le texte du code.

1. **Un module = un dossier = un en-tête public** `src/<module>/<module>.hpp`. Les autres fichiers du dossier sont
   internes. Un module ne dépend que des modules placés avant lui dans la table du § 2 ; aucun cycle.
2. **Taille** : un fichier de `src/` fait au plus 500 lignes, une fonction au plus 100. Un fichier qui approche la
   limite se découpe par responsabilité, jamais par numérotation (`part1`, `part2`).
3. **Aucun état global modifiable.** Une `Session` porte l'unique `MemoryBudget` et l'unique `Pool` ; ils sont
   passés explicitement.
4. **Erreurs** : `Outcome` et `Result<T>` ; aucune exception ne traverse une frontière de module ; jamais `assert`
   (une précondition interne violée rend `invariant_violated`).
5. **Transactions** : une opération rend son résultat entier ou un refus ; jamais un préfixe publié.
6. **Aucun mutant, crochet de test ni option morte dans le produit.** Les mutants sont des correctifs appliqués à une
   copie des sources (`tests/mutants/`). Une option n'existe que si une porte l'exerce et qu'une ablation la justifie.
7. **Déterminisme** : toute sortie publiée est identique octet pour octet quel que soit le nombre de fils. Une tâche
   parallèle écrit à des positions fixées par son ordinal ; les concaténations passent par des préfixes.
8. **Mémoire** : tout grand tableau est un `Buffer<T>` réservé dans le budget avant l'allocation. `std::vector` est
   permis pour les petits états locaux hors boucle chaude.
9. **Chaque décision est entière et exacte.** Le flottant suit la doctrine du § 4.
10. **Chaque prédicat cite son budget de bits** (§ 3) et le garde par `static_assert` ; chaque élagage cite son lemme.
11. **Style** : C++20 sans extension (hors `__int128`, déclaré par `__extension__`), deux espaces, `snake_case` pour
    fonctions et variables, `PascalCase` pour les types, namespace `mhgp11`, macros `MHGP11_*`, cibles et tests
    `mhgp11_*`. Commentaires en français sans accents. Compilation `-Wall -Wextra -Wpedantic -Werror`, GCC et Clang.
    Python : PEP 8, aucune porte ne repose sur `assert`.

## 2. Modules

| Module | Rôle | Dépend de |
| --- | --- | --- |
| `core` | entiers, identifiants forts, statuts et raisons, `Result`, budget mémoire, `Buffer`, `Csr`, compteurs | — |
| `num` | entiers à budget de bits, entiers larges, niveaux rationnels, prédicats géométriques exacts, clés approchées à borne prouvée | `core` |
| `sched` | `Pool`, `parallel_for`, tri parallèle, sommes préfixes | `core` |
| `cloud` | contrôle du domaine, sites en ordre de Morton, multiplicités, table site → `PointId` | `core`, `sched` |
| `io` | lecture des nuages, sorties transactionnelles, formats canoniques, empreintes | `core`, `cloud` |
| `index` | requêtes exactes sur les sites (plus proches voisins, boules fermées) | `num`, `cloud` |
| `catalogue` | catalogue critique (boîtes de centres) | `num`, `sched`, `cloud` |
| `tower` | tour FULL (cellules, descentes, Kruskal par plateaux, verticales) | `catalogue`, `index` |
| `points` | hiérarchies de points tirées de la tour | `tower` |
| `head` | condensation, sélection, étiquettes | `points` |
| `api` | façade publique `mhgp11.hpp` et `Session` | tous |

`cli/` contient un seul exécutable, `mhgp11`, à sous-commandes ; `reference/` l'oracle exact borné en Python ;
`bench/` les bancs (synthétique, LiDAR, G4) ; `tests/` les portes, par module.

Les fondations (`core`, `num`, `sched`, `cloud`, `io`, `reference`) sont des **ports explicites** des composants de
la v10 durcis par son raccord R2 ; voir `PROVENANCE.md`.

## 3. Profil numérique

- Entrée : coordonnées entières $0 \leq x < 2^{B}$ par axe. $B$ = `MHGP11_COORD_BITS`, constante de compilation,
  **18 par défaut** (grille de 1 mm sur une trame LiDAR). Les profils $B = 21$ et $B = 24$ doivent compiler depuis les
  mêmes sources ; seul $B = 18$ est qualifié tant qu'un profil n'a pas ses propres portes.
- Les identifiants de points sont des `u32` arbitraires et uniques ; les positions égales forment un site de
  multiplicité $w \geq 1$.
- **Entiers à budget** : `num::Int<bits>` désigne le plus petit type exact capable de porter tout entier de valeur
  absolue $< 2^{\text{bits}}$ (`i64`, `i128`, puis entiers larges à mots de 64 bits). Le budget de chaque expression
  est calculé en `constexpr` à partir de $B$ ; aucun type n'est choisi à la main. Un débordement est donc une erreur
  de compilation, pas une garde à l'exécution.
- Les différences de coordonnées sont de valeur absolue $< 2^{B}$ (et non $2^{B+1}$).
- Un niveau est un rationnel exact `num / den`, `den > 0`, jamais réduit ; l'ordre des niveaux se décide par produits
  croisés exacts.

## 4. Doctrine flottante

La v10 a dû refuser une à une les options de compilation qui affaiblissent IEEE-754 (raccord R2, constat B2), sans
pouvoir fermer tous les canaux. La v11 ne s'appuie pas sur un contrat de compilation : ses usages du flottant restent
justes sous tout mode d'arrondi, avec ou sans contraction, avec ou sans réassociation.

- **F1.** Aucune décision n'est prise en flottant.
- **F2.** Noyaux entiers portés en binaire64 : toutes les valeurs, y compris toute somme partielle dans un ordre
  quelconque, sont des entiers de valeur absolue $< 2^{53}$. Le calcul est alors exact, quel que soit le mode.
- **F3.** Clés approchées : obtenues à partir d'entiers exacts par conversions, produits, quotients et sommes de
  termes de même signe seulement. Sous la seule hypothèse qu'une opération élémentaire a une erreur relative
  $\leq 2^{-52}$, une clé issue de $m$ opérations a une erreur relative $\leq (1 + 2^{-52})^{m} - 1$. Aucune
  soustraction entre approximations.
- **F4.** Deux clés approchées ne sont déclarées ordonnées que si leur écart dépasse la marge prouvée ; sinon la
  comparaison est rejouée en exact.
- **F5.** Défense en profondeur, sans rôle dans les preuves : refus de `__FAST_MATH__` à la compilation, et auto-test
  des hypothèses F2 et F3 au démarrage d'une `Session`.

## 5. Construction et portes

- `CMakeLists.txt` inclut `src/<module>/module.cmake` (sources de la bibliothèque `mhgp11`) et
  `tests/<module>/tests.cmake` (portes) pour chaque module présent. Un module n'édite jamais un fichier partagé.
- Options : `MHGP11_COORD_BITS` (18), `MHGP11_SANITIZE` (ASan + UBSan), `MHGP11_TSAN`, `MHGP11_POISON` (tampons
  empoisonnés), `MHGP11_MARCH` (jeu d'instructions, vide par défaut).
- Codes de sortie exacts (`cmake/run_expect.cmake`) : 0 conforme, 1 désaccord d'un juge, 2 refus avant calcul,
  3 plancher ou invariant violé, 4 mutant tué. Un arrêt par signal est toujours un échec.
- Toute porte porte un plancher de couverture contre le vert par vacuité.
- Labels CTest : `unit`, `oracle`, `diff_v10`, `scale8000`, `scale16000`, `scale32000`, `lidar`, `mutant`, `fast`
  (les portes `fast` n'exigent ni NumPy ni scikit-learn).
- Les petites tailles sont des oracles de correction ; toute conclusion de coût se mesure à 8 000, 16 000 et 32 000
  points et sur les trames LiDAR du contrat.
- Compilations et calculs hors du dépôt (`/tmp`), binaires de campagne figés depuis `git archive`.

## 6. Conformité

La v11 calcule le même objet que la v10. La conformité se prouve par : (a) l'oracle exact borné de `reference/` ;
(b) des campagnes appariées contre le binaire figé de la v10 (sorties canoniques identiques octet pour octet sur les
mêmes entrées, trames LiDAR du contrat comprises) ; (c) des invariants globaux à l'échelle ; (d) des mutants tués.
Aucun benchmark ni accord moyen ne promeut un statut public.
