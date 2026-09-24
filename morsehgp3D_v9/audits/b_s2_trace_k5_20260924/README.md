# Trace S2→S3 K5 autonome, quartier physique 1 288 sites (audit B)

Capture **audit-only** sur le quartier `x≥0,y≥0` sans sol de SemanticKITTI
`08/000200`, densité globale `1/4`, grille commune 1 mm/u18 : exactement
1 288 sites, K5/s8/W8. Ce répertoire est séparé de la
[`trace K10`](../b_s2_trace_20260924/README.md) ; aucun ordinal K5 ne doit
être joint à un ordinal K10. Aucune trame entière, tour FULL G4 ni borne de
100 ms n'est qualifiée ici.

## Entrée et méthode

Les deux entrées sont celles du panneau physique S4a `quarter_quarter_x_nonneg_y_nonneg` :
XYZ SHA-256 `33630aea9492d3b059001e10c74ba30bec143dd7a1a0be6b25f8701b2f2a5e8f`,
IDs de retours bruts SHA-256
`d473e6314cf322c9213998906d3c43f59ee5044b809a8545fe4febe9113e053a`.
Le sidecar [`trace.cpp`](trace.cpp) reprend le protocole public de la trace
K10 : index du nuage une fois, front mass-first avec 512 jobs puis concaténation
en ordre de job, filtre S2 CPU, certificats S3 CPU et rejeu du core diamétral
par survivant pour attacher `F_e` et le masque post-core. Il refuse un
effectif différent et épingle R/P/S/ΣF/core_closed dans le code. Il ne
modifie aucune source du moteur ; la recertification `F_e` est du travail
d'audit ajouté, pas un coût proposé pour le produit.

Construire depuis la racine du clone isolé :

```sh
cmake -S morsehgp3D_v9/audits/b_s2_trace_k5_20260924 \
  -B /tmp/mhgp9-s2k5-build.J6RCbX -DCMAKE_BUILD_TYPE=Release \
  -DMHGP9_BOOST_INCLUDE_DIR=/workspaces/E-HGP/build/v7_boost_gate/extracted/usr/include
cmake --build /tmp/mhgp9-s2k5-build.J6RCbX --target mhgp9_b_s2_trace_k5 -j2
```

L'appel archivé prend XYZ, IDs bruts, puis deux **nouvelles** sorties
`trace.tsv` et `segments.tsv` ; il refuse d'écraser des sorties existantes.
La source est SHA-256
`012e645d4d5bd6667a72c94c3a45b8a09601a011e7f0b5f7bde855e3f3b8326a`,
le binaire sidecar `be594cbefe537e72509d4356479568f5003b18d007f977840c4776e86c5ba3b7`
et la bibliothèque `mhgp9_gen`
`e17932e499905ae2ae389b243b1fdf973303091719d55514379abce978f395b0`.
Le `RESULT.json` garde les chemins du build temporaire pour le contrôle LIVE
optionnel ; la lecture archivée ne les exige pas.

## Sortie et comparaison

Le stdout [TRACE.stdout](TRACE.stdout) et le lecteur trouvent R=37 459,
P=46 218, S=27 099, `core_builds=dead_core_loads=S`, Σ(F_e)=298 205,
7 020 arêtes complètement fermées par le core et 9 447 masques nuls après
S3. Le core prouve q3 sur 5 734 arêtes (masse F 108 979), q4 sur 6 811
(masse F 131 727) ; l'union pèse 168 506 et l'intersection 72 200.
Les masses des deux voies ne s'additionnent pas. Les 7 020 fermetures
complètes pèsent 146 394 F. Ce sont des expositions du travail du core,
pas des économies réalisées.

[`quarter_1288_k5.trace.tsv`](quarter_1288_k5.trace.tsv) porte l'ordinal
S2, l'ordinal du rectangle, les IDs locaux et bruts, le masque S2,
`F_e`, puis les masques post-core et post-S3 pour chaque survivant.
[`quarter_1288_k5.segments.tsv`](quarter_1288_k5.segments.tsv) conserve
`[begin,end)` pour chaque rectangle, y compris vide, avec une dernière borne
à 27 099. Leurs SHA-256 sont respectivement
`7d5823ea50c45ef0dd7a95ddd17bf09cf2819f13ae11355d92fc9100e2e40297`
et `fd0fb60a17ee0dceab3ab90f29a9b4b321c64844f6af19d5bbd286feee53e4aa`.

Il n'existait aucun stdout produit K5 pour ces 1 288 sites dans les audits
disponibles. Un **nouveau** run CPU local du produit, archivé avec
[sa commande](LOCAL_PRODUCT.command.txt), [son stdout](LOCAL_PRODUCT.stdout)
et ses hashes dans [`RESULT.json`](RESULT.json), concorde sur R/P/S,
ΣF, les deux preuves core et les 7 020 fermetures. Son binaire a été
construit dans le même clone que le sidecar : c'est un contrôle de ledgers
de deux chemins publics de la même base de code, **pas** un oracle
géométrique ou un reçu historique indépendant. Son unique mesure locale
(`chain_total` 869,186 ms, q34 548,362 ms, certificats 117,706 ms)
dépend de l'hôte ; elle ne compare pas des vitesses K5/K10 et n'établit
aucun seuil G4.

## Lecture reproductible

[`verify.py`](verify.py) épingle les SHA d'entrée, source, trace et
segments, vérifie tous les IDs bruts, l'unicité des arêtes, les masques,
les `F_e`, les seuils de comptes et la partition complète des 37 459
segments. Il exige le stdout produit local archivé et vérifie ses levers
CPU exacts. Le lecteur ne redéduit pas la géométrie du moteur.

```sh
python3 -B morsehgp3D_v9/audits/b_s2_trace_k5_20260924/verify.py
python3 -B -O morsehgp3D_v9/audits/b_s2_trace_k5_20260924/verify.py
python3 -B morsehgp3D_v9/audits/b_s2_trace_k5_20260924/verify.py --check-local-binary
```

Les deux premiers lecteurs portent sur les artefacts archivés ; le dernier
exige les binaires locaux aux chemins de `RESULT.json`. Le
[`join_shadow.py`](join_shadow.py) accepte des décisions pré-core
clairsemées `s2_ordinal<TAB>proved_mask` avec bits q3=2/q4=4, refuse les
bits non demandés par S2 et somme séparément les masses par voie,
union/intersection et arêtes entièrement fermées. Une arête absente a
masque zéro. La fixture vide n'infère aucun gain ; une fixture q4 interdite
sur une arête S2 q3 seule doit échouer. Les sorties
[`READ_normal.json`](READ_normal.json), [`READ_optimized.json`](READ_optimized.json)
et [`READ_live.json`](READ_live.json) sont identiques hors du statut LIVE ;
[`MUTANT_CHECKS.json`](MUTANT_CHECKS.json) conserve les refus observés sur
une copie temporaire privée de `LOCAL_PRODUCT.stdout` et sur l'ordinal 2
qui ne demande que q3. La capture originale n'a pas été déplacée.
