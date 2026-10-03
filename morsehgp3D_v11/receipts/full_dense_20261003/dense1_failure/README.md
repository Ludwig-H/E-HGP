# dense1 — échec conservé de la sonde d’orientation

Source poussée `768070ddb549e16f1914f05ef36b5ae3ccbf42cf`, session
`v11.20261003.dense1`, fermée `failed_remote` (worker 1, DONE 3).
Arrêt ciblé, génération, retrait des clés et libération de la réserve sont
certifiés ; `errors` et `warnings` sont vides. Le lecteur rend
`coherent=true`, **`conforming=false`**.

| Configuration | Portes réussies / sélectionnées |
|---|---:|
| GCC Release | 601 / 603 |
| Mutants | 21 / 21 |
| GCC ASan/UBSan | 526 / 528 |
| GCC TSan | 526 / 528 |
| Profil u21 | 526 / 528 |
| Profil u24 | 526 / 528 |
| Poison | 527 / 529 |
| Style | 2 / 2 |
| Total matrice | **3 255 / 3 267** |
| Supplément ASan18 | **285 / 287** |

Clang Release facultatif est absent. Toutes les constructions de base
aboutissent. Les deux portes échouées dans chaque configuration concernée sont
`mhgp11_num_orientation_certificate_fraction` et sa variante `_opt` :

- dix refus `REFUS orientation_certificate_oracle: anchor` dans Release,
  TSan, u21, u24 et poison ;
- quatre refus `REFUS orientation_certificate_oracle: native process` dans
  ASan/UBSan et le supplément ASan18.

Les tests unitaires d’orientation sont passés ; ces succès ne ferment pas la
qualification Fraction en échec. Les 288 mutants sont vérifiés contre leurs
manifestes et les sorties complètes LastTest reliées à JUnit : **286 morts par
code/ligne et deux constructions attendues**. Aucune campagne mutants n’est
invalide dans ce reçu, contrairement à l’échec historique census1.

## Défaut établi et limite des diagnostics

La lecture de la source exécutée établit un défaut de durée de vie dans la
**sonde**, `tests/num/orientation_certificate_probe.cpp:24` :

```cpp
for (const auto x : ball.anchor().coordinates()) std::cout << ' ' << x;
```

Pour Sphere et Q4Candidate, `anchor()` retourne un `Point` par valeur ;
`Point::coordinates()` retourne une référence à son tableau membre. En C++20,
ce passage par un appel membre ne prolonge pas la vie du `Point` temporaire :
la boucle emprunte donc une référence pendante. Le lecteur vérifie ces
signatures et cette ligne dans les objets Git de la source épinglée.
Conserver le `Point` dans une variable locale corrige cette durée de vie ;
ce correctif ultérieur n’est pas appliqué à la source de la capture.

**Aucun rapport ASan enfant complet n’a été conservé.** L’oracle exécuté
capturait `stdout` et `stderr` du processus natif, puis refusait avec le seul
message `native process` si le code était non nul ou si `stderr` n’était pas
vide. Il ne publiait ni le code enfant ni son `stderr`. Les archives JUnit et
LastTest ne permettent donc pas de reconstruire un diagnostic
`stack-use-after-scope`, un signal ou une pile d’appels. Le code de sortie 1
conservé est celui de l’oracle. Le champ
`asan_child_diagnostic_preserved=false` rend cette perte explicite.

Le défaut statique de la sonde et les refus conservés ne prouvent pas un défaut
du prédicat numérique. Ils empêchent sa qualification complète sur cette
source. La future correction de la sonde et l’amélioration de la conservation
des flux devront être rejouées séparément ; aucun résultat futur n’est hérité.

## Banc et fermeture

`002_dense_full` termine code 2 avec `full_parallel_refused: ValueError`,
sans rapport ni essai FULL. Les **20 unités déclarées** restent non démarrées :
six paires LiDAR u21/u24 en modes 511/1023 à W48, mode 1023 W1/W8 sur ng00 u21,
et trois paires uniformes u21. Aucun temps FULL ni gain du lookup dense n’est
mesuré dans cette capture. Le diagnostic du collecteur ne conserve pas de
traceback ; aucun flux natif enfant n’est inventé.

Préflight original : 850 + 180 + 570 secondes de commandes et 120 secondes de
préparation, soit **1 720 ≤ 1 737 secondes** utiles. Les trois commandes sont
fermées, sans troncature ou groupe résiduel tué. L’extrait initial reste dans
[first_failure.txt](first_failure.txt).

## Relecture

Depuis la racine du dépôt :

```sh
python -B morsehgp3D_v11/receipts/full_dense_20261003/dense1_failure/check.py
python -B -O morsehgp3D_v11/receipts/full_dense_20261003/dense1_failure/check.py
python -B morsehgp3D_v11/receipts/full_dense_20261003/dense1_failure/check_selftest.py
python -B -O morsehgp3D_v11/receipts/full_dense_20261003/dense1_failure/check_selftest.py
```

Le lecteur exige le reçu brut LIVE, l’archive originale, le paquet source local
(relocalisé par symlink bit-identique) et les objets Git. Il rehash ces pièces,
vérifie le manifeste tar, les copies compactes, les options/cache/provenances,
l’inventaire des portes et les raisons exactes d’échec dans JUnit et LastTest.
Les sept helpers historiques sont copiés et hachés à la source exécutée avant
import ; aucun WIP n’est utilisé. Le calendrier seul est évalué depuis sa
fonction AST épinglée.

[reader_proof.json](reader_proof.json) conserve les lectures normal/−O
identiques : **11 témoins, 66 corruptions rejetées**, zéro appel natif ou cloud.
Les corruptions changent notamment les deux logs de concert, les raisons
`anchor`/`native process`, fabriquent un diagnostic absent, convertissent une
mort de mutant en signal ou altèrent les gardes de fermeture. Aucun paquet
source, binaire natif ou payload LiDAR n’est copié dans la capsule ; une seule
archive de résultats est conservée. Ce lecteur n’est pas un audit exhaustif
des helpers historiques ni un nouveau test du produit.
