# MorseHGP3D — cœur certifié

Ce répertoire porte la nouvelle implémentation décrite par la roadmap. `HGP-old/` reste une référence historique séparée et n'est pas importée par ce cœur.

État actuel : Phase 2A, backend `reference_cpu`, couche numérique commune aux profils, mode `certified`.

Le premier lot fournit :

- l'interprétation exacte des coordonnées binary64 comme dyadiques;
- `ExactRational3` homogène et canonique;
- `ExactLevel` non négatif, canonique et compatible avec le contrat v2;
- le fallback `boost::multiprecision::cpp_int` qui sert d'oracle aux filtres CPU;
- la comparaison de distances carrées et l'orientation 3D exactes;
- la forme affine exacte `H_RQ` pour deux labels canoniques de même cardinalité;
- des entrées `decision-only` séparées des témoins rationnels de replay;
- des décisions séparant signe scientifique, étage de certification et compteurs;
- un replay diagnostique versionné à partir des mots binary64 d'entrée.

Ce lot ne ferme ni la Phase 2A ni G1. Les trois premières familles de prédicats utilisent honnêtement l'étage `cpu_multiprecision`; les filtres bornés et expansions, puis les autres prédicats géométriques, seront ajoutés par lots testables ultérieurs.

La convention d'orientation est exactement `det([b-a, c-a, d-a])` : le quadruplet `(0, e1, e2, e3)` est positif. Cette définition explicite prévaut sur les conventions de signe parfois opposées d'autres bibliothèques.

## Construction locale

```bash
cmake -S morsehgp3d -B build/morsehgp3d -DMORSEHGP3D_BUILD_TESTS=ON
cmake --build build/morsehgp3d --parallel
ctest --test-dir build/morsehgp3d --output-on-failure
```

Le target exporté est `morsehgp3d::exact`. Après `cmake --install`, un consommateur peut utiliser `find_package(MorseHGP3D CONFIG REQUIRED)` sans dépendre de chemins d'en-têtes propres à la machine de construction.

## Replay d'un prédicat

Le wrapper Python valide un artefact d'entrée séparé du schéma scientifique public, appelle le binaire C++ et produit un résultat JSON canonique muni d'un identifiant SHA-256 stable de l'entrée normalisée :

```bash
python morsehgp3d/tools/replay_predicate.py \
  morsehgp3d/tests/fixtures/predicates/distance_one_ulp.json \
  --executable build/morsehgp3d/morsehgp3d_replay_predicate
```

Cet identifiant ne couvre ni le résultat ni le binaire et n'est donc pas un certificat public. Le replay conserve les bits de zéro signé à des fins diagnostiques, tout en canonisant leur valeur géométrique. Une coordonnée non finie, un prédicat inconnu, une sortie native non canonique ou incohérente, et un binaire explicitement demandé mais introuvable échouent fermés.

Le schéma diagnostique v1 reste actif pour les prédicats à nombre fixe de points. Le schéma v2 ajoute `power_bisector_side` avec table de points, labels de 1 à 10 identifiants triés et uniques, et témoin `ExactRational3`; les deux labels doivent avoir le même cardinal afin que le terme quadratique s'annule exactement. Un signe négatif signifie que le coût de `R` est inférieur à celui de `Q`.

Le binaire accepte aussi un flux batch, une commande par ligne, afin que les différentiels n'ouvrent pas un processus par décision :

```bash
python morsehgp3d/tests/differential/check_predicate_random.py \
  build/morsehgp3d/morsehgp3d_replay_predicate
```

La suite courte traite 2 048 cas déterministes et publie le hash du corpus. Les paramètres `--distance-cases`, `--orientation-cases` et `--power-cases` permettent d'augmenter ce volume; cette infrastructure ne constitue pas encore la campagne de fermeture à dix millions de signes.

Le mode batch est un flux : une erreur tardive peut suivre des lignes déjà écrites. Un consommateur ne valide donc jamais ces lignes avant d'avoir observé la fin du processus avec un code nul. Toute erreur de lecture ou d'écriture du flux produit un échec fermé.
