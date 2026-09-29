# Reçu J2, suite : assemblage du catalogue sans remplissage en série (V9)

29 septembre 2026. Suite de `RECU_J2.md` : partie « assemblage » de l'étape J2 du plan du juge (V9 : grands tableaux
initialisés en série, premier contact des pages sur un seul fil). Aucune branche, aucun commit, aucune commande GCP.

```text
phase=exploration_v10_hors_registre (etape J2, assemblage)
backend=reference_cpu
profile=quantized_u18_input_only
mode=implementation_locale_mesuree (codespace EPYC 7763, 1 a 4 fils)
public_status=not_claimed
GCP non utilise
```

## 1. En bref

- **Patch** : `build/v10-j2/J2_assemblage.patch`, 3 fichiers (+49 / −16). Il s'applique :
  - sur `82fc2a6b5`, HEAD actuel, qui contient déjà J2 (commit `5565f94fb`) ;
  - ou après `J2.patch` sur `568d45297`.

  Les deux applications ont été vérifiées sur des extractions propres.
- **Ce qui change** :
  - les grands tableaux du catalogue ne sont plus remplis de zéros en série par `resize` (`rank`, `support`, `qmin`,
    `p`, `u`, `flags`, `pop_off`, `pop`, `n_interior`, `level`) ;
  - les tableaux de travail de l'ordre non plus (`refs`, `cmp`, tampon de sortie du tri parallèle) ;
  - chaque case est écrite par la boucle parallèle qui remplit déjà le tableau, et les pages sont touchées pour la
    première fois par les fils de ces boucles.
- **Exactitude** :
  - dumps identiques à `568d45297` sur les 10 entrées, à 1 et 4 fils, en build normal et en build empoisonné
    `MHGP10_POISON` (octets 0xA5 à l'allocation) : aucune case lue avant d'être écrite n'atteint la sortie ;
  - niveaux exacts identiques (empreinte de `cat.level`), build normal et empoisonné ;
  - 9 portes `gate` sur 9 vertes au HEAD (la neuvième, `multiplicity_refusal`, est apparue avec `24ac5fc51`).
- **Gain** (médianes de 3, exécutions alternées, t_order + t_assemble) :
  - à 4 fils : −29 à −32 %. Trame 02 : 0,327 → 0,232 s à K = 5, 1,350 → 0,941 s à K = 10. Catalogue entier −3 à
    −5,5 % ;
  - à 1 fil : neutre (0,662 → 0,657 s ; 2,796 → 2,756 s).

## 2. Ce qui change dans le code

- **`src/catalogue/catalogue.hpp`** :
  - allocateur `UninitAlloc<T>` : dérivé de `std::allocator<T>`, avec `construct` sans argument qui ne fait rien,
    empoisonnement 0xA5 sous `MHGP10_POISON`, et `static_assert` restreint aux types triviaux ;
  - alias `UninitVector<T>`, que prennent les 10 champs du `Catalogue`.

  Les consommateurs (tour, tête, points, CLI) n'utilisent que l'indexation, `size()` et `data()` : ils compilent sans
  changement. Le `Catalogue` reste copiable.
- **`src/catalogue/generator.cpp`** : `refs` et `cmp` passent en `UninitVector`. `cmp[0]` n'est jamais lu.
- **`src/sched/sort.hpp`** : `parallel_sort` devient générique en allocateur. Son tampon de sortie prend
  l'allocateur du vecteur trié, donc n'est plus initialisé, et il est écrit par les seaux en parallèle. Pour les
  `std::vector` ordinaires, le comportement ne change pas ; le catalogue en est le seul appelant.

**Écart au plan.** Le plan demandait `Buffer<T>`, non initialisé et compté au budget. J'ai pris un allocateur
compatible avec `std::vector` pour ne toucher aucun consommateur : avec `Buffer<T>`, le `Catalogue` deviendrait non
copiable et une dizaine de sites seraient à adapter. Conséquence : ces tableaux ne sont toujours pas comptés au
budget, exactement comme avant (`std::vector`). Le passage à `Buffer<T>` reste possible et relève d'une décision
séparée.

## 3. Pourquoi c'est exact : chaque case est écrite avant d'être lue

| tableau | écriture |
| --- | --- |
| `refs` | les intervalles [first[li], first[li] + \|recs\|) pavent [0, nb) |
| tampon du tri | les seaux [off[j], off[j + 1]) pavent [0, n) |
| `cmp` | cases 1 à nb − 1 ; toute lecture est gardée par `b > 0`, ou court-circuitée par `b == 0` |
| `rank`, `pop_off` | boucle des tranches, positions 0 à nb − 1 ; `pop_off[nb]` écrit explicitement |
| `level` | rangs 0 à inc[chunks] : un rang nouveau n'apparaît qu'en `b == 0` ou `cmp[b] < 0`, là où le niveau est écrit ; le rang d'une tranche part de celui de la fin de la tranche précédente |
| `support`, `qmin`, `p`, `u`, `flags`, `n_interior` | boucle de copie, toutes les positions |
| `pop` | les intervalles [pop_off[b], pop_off[b + 1]) pavent [0, total) |

Les types sont triviaux (`static_assert`), donc à durée de vie implicite en C++20 : l'allocation crée les objets
sans construction explicite. Le chemin n < 2 (`pop_off.push_back(0)`) construit avec argument, comme avant.

**Preuve expérimentale.** Build `MHGP10_POISON` :

- dumps identiques sur les 10 entrées à 1 et 4 fils (`assemblage/differentiel_asm_poison.txt`) ;
- niveaux identiques (`assemblage/niveaux_asm.txt`).

Une case non écrite porterait 0xA5… au lieu d'une valeur calculée et changerait la sortie.

## 4. Mesures

`assemblage/mesure_asm.sh` : 3 répétitions, alternance « J2 au HEAD » (`mhgp10_catalogue.head`) contre
« J2 + assemblage » (`mhgp10_catalogue.asm`). Données brutes dans `assemblage/mesure_asm.tsv`.

| trame | K | fils | order + assemble J2 (s) | J2 + assemblage (s) | écart | détail J2 → assemblage (compare, ranks, copy) | catalogue entier (s) |
| --- | ---: | ---: | ---: | ---: | ---: | --- | --- |
| 02 | 5 | 4 | 0,327 | 0,232 | −29 % | 0,074 / 0,089 / 0,034 → 0,043 / 0,047 / 0,043 | 1,945 → 1,847 |
| 02 | 10 | 4 | 1,350 | 0,941 | −30 % | 0,279 / 0,414 / 0,142 → 0,176 / 0,190 / 0,192 | 7,936 → 7,500 |
| 00 | 5 | 4 | 0,307 | 0,211 | −31 % | 0,069 / 0,085 / 0,032 → 0,041 / 0,043 / 0,040 | 1,964 → 1,904 |
| 00 | 10 | 4 | 1,366 | 0,923 | −32 % | 0,294 / 0,421 / 0,144 → 0,172 / 0,197 / 0,196 | 8,294 → 7,872 |
| 02 | 5 | 1 | 0,662 | 0,657 | neutre | 0,193 / 0,132 / 0,128 → 0,160 / 0,123 / 0,168 | 6,979 → 6,979 |
| 02 | 10 | 1 | 2,796 | 2,756 | neutre | 0,781 / 0,608 / 0,550 → 0,685 / 0,497 / 0,736 | 28,344 → 28,348 |

Lecture :

- **À 1 fil**, la remise à zéro disparaît, mais les défauts de page passent dans la boucle de copie : bilan nul.
- **À 4 fils**, cette part est répartie sur les fils. Sur G4 à 48 fils, le remplissage en série ne se parallélisait
  pas du tout : le gain relatif y sera plus grand. Il est à mesurer en session.
- **Parts restantes en série :** le repérage des bandes (`t_bands`, 0,03 à 0,07 s à 4 fils) et le rassemblement des
  enregistrements par fil. Le plan ne les traite que « si le chronomètre le justifie ».

## 5. Risques

- Les tableaux du catalogue ne sont pas comptés au budget, comme avant (voir l'écart au plan, § 2).
- L'empoisonnement ne couvre que cet allocateur. Ce qui vit dans les `Local` (enregistrements par fil) reste en
  `std::vector` ordinaire.
- Les tableaux du catalogue exposés aux consommateurs ne sont plus mis à zéro. Un futur remplissage partiel lirait de
  l'indéterminé. Parade : le build `MHGP10_POISON` joué dans le différentiel, comme ici.

## 6. Fichiers

Tous dans `build/v10-j2/` :

- `J2_assemblage.patch` ;
- `assemblage/` :
  - `base/` : HEAD `82fc2a6b5` vierge ;
  - `morsehgp3D_v10/` : HEAD + assemblage ;
  - `build/`, `build-head/` ;
  - binaires `mhgp10_catalogue.{asm,asm_poison,head}` ;
  - sorties : `differentiel_asm.txt`, `differentiel_asm_poison.txt`, `niveaux_asm.txt`, `ctest_gate_asm.txt` ;
  - mesures : `mesure_asm.sh`, `mesure_asm.tsv`, `mesure_asm.txt`.

Le différentiel a été joué avec la référence passée explicitement, `build-base/mhgp10_catalogue` (`433b4b97…`). En
effet, `build/v10-wt/mhgp10_catalogue` a été reconstruit au HEAD à 13 h 11 et n'est plus `568d45297`.
