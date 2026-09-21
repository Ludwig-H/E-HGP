# Index float32 natif : qualification et passage à l'échelle

21 septembre 2026. Exploration hors registre, CPU,
`lossless_float32_input_only`, `native_index_preparation`, `not_claimed`.
GCP non utilisé ; ce ne sont ni une WSPD, ni les voies q3/q4, ni FULL.
Voir le [contrat et la preuve de coût](../../docs/INDEX_FLOAT32_ET_SUITE_Q34_20260921.md).

## Captures

| Capture | Verdict | Commandes |
|---|---|---:|
| `sanitize/index_xrz3mgl3` | FAILED avant exécution native | 3 |
| `sanitize_r2/index_0kkoi932` | PASS Clang ASan/UBSan, fuites activées | 11 |
| `release_r2/index_d_t8ag95` | PASS GCC Release | 26 |

L'essai initial a rencontré un avertissement signé/non signé promu en
erreur dans la sonde de test : le résultat de `std::bit_width` est converti
explicitement en size_t dans R2. Aucun changement du propriétaire/index,
aucune désactivation d'avertissement ou de sanitizer. Sources et diagnostic
initiaux restent archivés. Builds épinglés :

- `build/v8_float32_index_sanitize_20260921` (échec) ;
- `build/v8_float32_index_sanitize_r2_20260921` ;
- `build/v8_float32_index_r2_20260921`.

La qualification reste autonome : CMake et le moteur u16 ne sont pas
modifiés. Aucun ancien CTest n'est revendiqué pour ce nouvel index.

## Résultats fonctionnels

Chaque qualification R2 exécute les gates en Python normal et `-O` :

- 27 fixtures, 1 125 sites, 2 223 nœuds et 404 requêtes ;
- 5 605 IDs émis comparés à l'oracle rationnel, 65 requêtes vides ;
- 35 entrées invalides refusées avec le code exact 1 ;
- 11 mutations de résultats rejetées par le lecteur géométrique ;
- 151 contrôles natifs, dont 67 refus, quatre modes d'arrondi et quatre
  combinaisons FTZ/DAZ sur cette machine SSE ;
- 64 requêtes concurrentes avec quatre lecteurs et compteurs privés.

La fixture des 278 exposants donne une profondeur 9 et non 277.
Les tests vérifient boîtes exactes, médianes, IDs/rangs/escape, bits signés,
alias de l'appelant, destruction, exceptions et réutilisation. Les tests
FTZ/DAZ portent sur l'index et ses requêtes de boîte, pas sur les futurs
filtres q3/q4 ni sur une qualification GPU. Pas de gate ThreadSanitizer
dans cette capture ; concurrence fonctionnelle sous ASan/UBSan seulement.

### Trois mutations du code compilé

La capture `mutants/` conserve 25 commandes GCC : référence inchangée puis
trois sources modifiées et recompilées séparément. La référence passe ses
151 contrôles. Chacune des trois variantes échoue avec le code exact1 et
son diagnostic géométrique attendu : zéros signés comparés distinctement,
boîtes ouvertes au lieu de fermées, lien escape interne raccourci d'un nœud.
Aucun échec de compilation, crash ou expiration n'est compté comme détection.
Il s'agit de trois mutations causales contre un même selftest, pas de trois
oracles indépendants supplémentaires. Sources, diffs, objets, binaires,
dépendances et flux bruts sont clos ; build épinglé
`build/v8_float32_index_mutants_20260921`.
Clôture : `572fcb3e9a2e15a60d30998464bbe5835df292caa2a94aae58936dbbdf1b8cd0`.
Les [relectures normal/−O avec contrôle des sources et builds vivants](MUTANTS_READBACK.json)
passent et sont identiques ; elles restent hors de la capture gelée.

## Construction : 54 mesures Release

Dix-huit configurations, trois répétitions chacune : neuf synthétiques
et neuf entrées LiDAR (les sept morceaux de la scène0, puis les deux autres
trames entières). Aucun sous-échantillonnage LiDAR. Les neuf essais
instrumentés à n=257 sont des smokes séparés, pas des chronos comparables.

| Synthétique, médiane CPU | 8k | 16k | 32k |
|---|---:|---:|---:|
| uniforme | 7,071 ms | 15,355 ms | 33,173 ms |
| terrain | 7,097 ms | 15,375 ms | 33,259 ms |
| amas | 7,241 ms | 15,376 ms | 33,433 ms |

Les comparaisons de tri font ×2,113 à ×2,214 aux doublements ; lectures
de partition ×2,154 puis ×2,143. Les copies et le stockage sont linéaires.
Tous les coûts publiés sont sous leur seuil quadratique dans les six
relations synthétiques ; le résultat général O(n log n) vient de la
preuve des tris/partitions, pas de cette seule observation.

| Morceau float32 | Sites | Construction médiane |
|---|---:|---:|
| scène0 entière | 123389 | 126,906 ms |
| moitié x négatif | 61045 | 59,259 ms |
| moitié x non négatif | 62344 | 58,480 ms |
| quart x−/y− | 30265 | 26,897 ms |
| quart x−/y+ | 30780 | 27,398 ms |
| quart x+/y− | 31391 | 27,737 ms |
| quart x+/y+ | 30953 | 27,573 ms |
| scène100 entière | 124479 | 129,596 ms |
| scène200 entière | 125526 | 95,983 ms |

Les six relations spatiales emploient les ratios réels de cardinalités,
pas un facteur2 imposé. Exposants des comparaisons de tri :1,093 à1,125.
Les douze relations et leurs quinze postes sont publiés sans sélection
dans [GROWTH.json](GROWTH.json). Ne pas comparer deux scènes différentes
pour inventer un exposant de croissance.

Sur ce build : points12octets, nœuds64octets, vecteurs conservés
`156n−64` octets et pic de construction `180n−64` octets. Pour les trois
trames, environ19,25/19,42/19,58Mo conservés ; le pic inclut les permutations
temporaires, pas RSS, objet fixe, allocator ou buffers de l'appelant.

Ces temps proviennent du CPU local partagé. Construction = copie privée,
validation, tris, arbre et rangs inverses. Lecture/génération et checksums
sont séparés ; les 16 requêtes de contrôle sont aussi chronométrées à part
avec leur consommation d'IDs. Ces requêtes choisies sur l'arbre ne servent
pas à prétendre un census sous-quadratique. Ni 95,983ms de préparation ni
la preuve de l'index ne constituent le contrat FULL 100ms/1s sur G4.

## Fermeture et reproduction

Les [lectures de fermeture](READBACK.json) Release/Sanitize en normal/−O
concordent par paire, de même que les deux analyses de croissance.
Le lecteur reconstruit les entrées float32 depuis le brut, rejoue l'oracle
sur les transcriptions natives, vérifie les 11 mutations et contrôle
sources, artefacts, objets, binaires et dépendances avant **et après** lecture.
Les checksums des points mesurés sont recalculés indépendamment depuis les
fichiers ou le générateur déclaré. Neuf sources sont instantanées ; les
compilations ont un fichier de dépendances distinct par unité.

Clôtures :

- Release : `7419b09fe4183b09ba37029831205b57ec20b8858e65a0eed141d05c51a9f349` ;
- ASan/UBSan : `da79e4a93115c8ca81e477a796ed84710257fe4a5d93f5f2c21ccf6eba2d31ad`.

```bash
python -B morsehgp3D_v8/bench/run_float32_index_checks.py read --path morsehgp3D_v8/receipts/float32_index_20260921/release_r2/index_d_t8ag95
python -B -O morsehgp3D_v8/bench/run_float32_index_checks.py read --path morsehgp3D_v8/receipts/float32_index_20260921/release_r2/index_d_t8ag95
python -B morsehgp3D_v8/bench/analyze_float32_index.py --path morsehgp3D_v8/receipts/float32_index_20260921/release_r2/index_d_t8ag95
```

Pour reconstruire, employer `run ... --build NEUF --compiler /usr/bin/g++
--output NEUF --prepared DIR_FLOAT32_0 DIR_FLOAT32_100 DIR_FLOAT32_200` ;
les trois destinations préparées sont enregistrées dans le manifeste.
L'option `--sanitize --compiler /usr/bin/clang++` sélectionne la qualification
instrumentée, sans grands chronométrages. Ne pas écraser ces builds.
