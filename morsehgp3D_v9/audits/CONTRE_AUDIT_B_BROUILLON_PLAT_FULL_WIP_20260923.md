# Contre-audit B — brouillon plat FULL, préflight du port local

23 septembre 2026. Lecture de source du commit local développeur
`f93dc1659` (`tower: flat draft on the static path`), observé avant sa
publication sur `main`. Ce document n'est **pas** une qualification du port,
ni une reproduction d'un échec en exécution. Aucun fichier produit n'a été
modifié par B.

## Verdict immédiat

La voie statique remplace le journal d'actions individuellement allouées
par six vecteurs CSR dans `FullCoverageFlatDraft`. L'ordre batch/action
et le chemin de validation métier du certificat semblent conservés à la
lecture. Cependant le **nouvel overload public** accepte un brouillon
modifiable et lit ses offsets **avant toute vérification de forme**.
Un brouillon mal formé peut donc quitter le contrat `kInvalidInput` et
entraîner un accès hors limites, une longueur de span invalide ou un
parcours démesuré. Le port ne doit pas être annoncé comme qualifié tant
que cette frontière n'est pas fermée et testée.

## Mécanisme de source, reproductible sans hypothèse géométrique

Dans `src/tower/forest/full_coverage_certificate.hpp`, vers les lignes
154–176, les champs `level`, `batch_begin`, `parent_begin`, `parent`,
`contribution_begin` et `contribution` sont publics. Vers 188–195,
`FlatDraftSource::actions(b)` lit `batch_begin[b+1]` et soustrait
`batch_begin[b]`. Le nouvel overload vers 384–386 appelle directement
`build_from`; celui-ci commence son premier passage à compter par
`batches.actions(b)` vers 302, sans contrôle préalable des tailles ni de
la monotonie du CSR. Par exemple, après construction d'une banque de
populations valide, un appel avec `draft.level.size()==1` et
`draft.batch_begin.size()==1` atteint la lecture `batch_begin[1]`.
Des offsets de parents/contributions hors plage ou décroissants ont le
même problème dans `parents_of`/`contributions_of`; un dernier offset
incohérent peut également laisser des actions ignorées. La validation
ultérieure des niveaux et références ne peut pas réparer cette lecture.

La porte attendue pour **l'overload public**, avant le premier parcours :

- `batch_begin.size()==level.size()+1`, `batch_begin.front()==0`,
  offsets non décroissants, `batch_begin.back()==parent_begin.size()-1`
  et `==contribution_begin.size()-1` ;
- tableaux d'offsets non vides, chacun commençant à zéro, non
  décroissant et finissant exactement à `parent.size()` ou
  `contribution.size()` ;
- conversion sûre de chaque `u64` en `size_t` avant toute indexation ou
  arithmétique de pointeur, et réponse typée `kInvalidInput` aux formes
  invalides.

La factory interne peut maintenir des invariants plus forts, mais elle
ne dispense pas de valider cet overload public. Tester un brouillon vide,
les longueurs manquantes, les offsets décroissants, trop grands et les
actions orphelines sous ASan/UBSan ; comparer la sortie vectorielle et
plate sur les mêmes lots valides, y compris lots groupés et naissances.

## Performance et autres limites de portée

Le chemin singleton évite effectivement une allocation **par action
publiée** ; les lots groupés construisent encore un `FullCoverageAction`
temporaire et ses deux vecteurs dans `full_ball_tower.hpp` vers 781–805.
Il faut mesurer sur plusieurs trames le nombre de lots/actions groupés,
les allocations, le RSS maximum, phase A et durée FULL, avec l'ancien et
le nouveau chemin sur le même paquet. Les portes statiques W1/W4
existantes comparent au chemin séquentiel, mais le commit n'ajoute pas de
porte directe de l'overload plat ou de formes invalides. Un rejeu
Release/ASan/UBSan/TSan et sous échec d'allocation reste nécessaire.

Aucune nouvelle course n'est apparue à la lecture du producteur statique :
les brouillons sont par ordre, puis l'encodeur lit une banque immuable.
Ce constat de source n'est ni un gate de concurrence ni une preuve du
payload FULL. Aucun chrono G4 et aucun gain contractuel ne sont attribués
à ce commit.

## Addendum exécuté sur la surcharge publiée

Le [micro-test causal](flat_draft_invalid_probe.cpp) prépare une banque
valide à un site, puis passe à la surcharge publique un brouillon avec
`level.size()==1` et `batch_begin.size()==1` (le zéro initial par défaut).
L'appel devrait rendre `kInvalidInput`, sans lecture hors limites.
Compilation locale sur la source v9 publiée :

```sh
g++ -std=c++20 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
  -I morsehgp3D_v9/src morsehgp3D_v9/audits/flat_draft_invalid_probe.cpp \
  -pthread -o /tmp/mhgp9_flat_draft_invalid_probe_b
ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 \
  /tmp/mhgp9_flat_draft_invalid_probe_b
```

Résultat observé : code **1**, `AddressSanitizer: heap-buffer-overflow`,
lecture de huit octets immédiatement après le vecteur `batch_begin`.
Pile : `FlatDraftSource::actions` à
`full_coverage_certificate.hpp:192`, appelé par `build_from:286`, puis
par la surcharge publique `:386` et le micro-test `:23`.
L'adresse correspond au vecteur initial d'un seul `u64` ; ce n'est pas un
échec de la banque de populations. Le défaut source est donc confirmé
**en exécution** sur entrée publique invalide. Il ne prouve aucune erreur
géométrique ni aucune corruption sur le producteur interne qui construit
un CSR valide. Correctif : valider toute la forme CSR et ses conversions
avant le premier `batches.actions`, puis garder ce programme comme gate
ASan/UBSan et un cas Release à statut typé. Aucun GCP utilisé.
