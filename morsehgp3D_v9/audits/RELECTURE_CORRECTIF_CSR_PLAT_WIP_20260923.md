# Relecture du correctif CSR plat FULL (WIP)

**Addendum du 24 septembre 2026.** La relecture ci-dessous porte sur des
sources mutables non commises au moment des essais. Le correctif de la
surcharge CSR publique a ensuite été publié sur `main` dans `3765080cf` ;
les tests locaux décrits ici ne sont pas transférés à ce commit.

23 septembre 2026. Relecture de la surcharge publique de
`build_full_coverage_certificate` après le contre-audit B
`CONTRE_AUDIT_B_BROUILLON_PLAT_FULL_WIP_20260923.md` et son addendum exécuté
publié au commit `3fd9f155a`. Travail local **non commis** sur le worktree
`/workspaces/E-HGP/build/v9-open-worktree`, base `aad7416a5` ; aucun push.

Sources mutables lues et compilées (SHA-256) :

| Fichier | SHA-256 |
| --- | --- |
| `src/tower/forest/full_coverage_certificate.hpp` | `3e9cb6a8c416533ad14c8757edda50f8596cb2e76b799b26931d20b9e5053661` |
| `tests/tower/full_coverage_certificate_gate.cpp` | `65a7f512bf99afe0054114e9c973065f21241631368bb37a43f48f61143c2c96` |

## Verdict de source

Le contrôle `full_coverage_flat_draft_shaped` précède désormais le premier
`FlatDraftSource::actions`. Il vérifie pour les trois CSR la longueur exacte,
le zéro initial, la monotonie et la fin égale à la taille du tableau associé.
Il lie en outre le nombre d'actions des offsets batch aux deux tableaux
d'offsets par action. Ainsi chaque offset lu est au plus la taille du vecteur
visé ; `batch_begin[b] + a` reste au plus le nombre d'actions. Les conversions
`u64` vers `size_t` sont donc bornées sur les cibles 32/64 bits du projet.
Les tailles viennent de `std::vector` : le `rows + 1` du contrôle ne peut pas
déborder sur ces cibles pour ces types d'éléments. Une forme invalide renvoie
`kInvalidInput` / `coverage_flat_draft_shape`, avec certificat vide, avant
toute allocation du constructeur. Une forme valide atteint le constructeur
commun, qui traite `bad_alloc` et `length_error` en `kResourceExhausted`.
Je n'ai trouvé aucun contre-exemple CSR atteignable à cette frontière.

## Exécution ciblée

Compilateur : `g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0`. Commandes
lancées depuis le worktree ci-dessus ; les seuls binaires produits sont dans
`/tmp`.

```sh
git show 3fd9f155a:morsehgp3D_v9/audits/flat_draft_invalid_probe.cpp |
  g++ -std=c++20 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
    -x c++ - -I /workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9/src \
    -pthread -o /tmp/mhgp9_flat_csr_review_probe
ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 /tmp/mhgp9_flat_csr_review_probe
```

Sortie : `status=1 reason=coverage_flat_draft_shape`, code 0, sans rapport
ASan/UBSan. C'est le micro-test exact qui déclenchait le heap-buffer-overflow
sur la surcharge publiée avant le correctif.

```sh
g++ -std=c++20 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
  -pthread -isystem /workspaces/E-HGP/build/v7_boost_gate/extracted/usr/include \
  morsehgp3D_v9/tests/tower/full_coverage_certificate_gate.cpp \
  -o /tmp/mhgp9_flat_csr_review_gate
ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 /tmp/mhgp9_flat_csr_review_gate --selftest
```

Sortie : `full_coverage_certificate checks=1362 rejects=38 replay_cuts=60
gamma_cuts=40 allocation_rejects=20 authority=structural_only`, code 0,
sans rapport ASan/UBSan. Un premier essai de compilation de cette porte sans
le chemin Boost `-isystem` a échoué sur l'en-tête manquant
`boost/multiprecision/cpp_int.hpp` ; il n'a exécuté aucun test.

## Limites du gate et portée

La porte couvre le cas causal de B (`level.size()==1`, `batch_begin.size()==1`),
plus plusieurs offsets tronqués, décroissants ou terminaux incohérents. Elle
ne cible pas explicitement le brouillon vide, `UINT64_MAX`, les actions
orphelines ni toutes les tailles de `contribution_begin`. Sa boucle de pannes
d'allocation exerce la surcharge vectorielle, pas la surcharge plate.
`flat.same_forest` compare les tailles des arènes puis les lectures : il ne
compare pas directement les valeurs de `nodes`, `parents`, `successors` et des
contributions datées entre les deux constructions. Ces ajouts renforceraient
le gate sans changer le verdict sur la validation CSR. Les méthodes publiques
`parents_of` et `contributions_of` restent des accès directs qui supposent un
brouillon bien formé ; cette relecture porte sur la surcharge de construction.

Ce contrôle local sur sources mutables n'est pas un reçu de qualification
FULL, un gate TSan, une mesure de coût ni une exécution G4. GCP non utilisé.
