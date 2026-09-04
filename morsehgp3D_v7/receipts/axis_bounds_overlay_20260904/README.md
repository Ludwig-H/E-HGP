# Reçu historique AxisBounds, 4 septembre 2026

`public_status=not_claimed`. Reçu de preuve locale et d'exécutions bornées,
pas de promotion globale ou de résultat de performance end-to-end.

L'overlay a été qualifié séparément, puis intégré sur GO explicite du
responsable de la campagne. `README.overlay.md` est conservé dans son état
antérieur à l'intégration : sa mention NON intégré est historique, pas le
statut courant du dépôt. Les patches et copies conservées permettent de
reconstituer ce delta sans dépendre des fichiers générés sous `build/`.

Le domaine final couvre les bornes conservatrices S1 : A<2^68, |B|<2^87,
|C|<2^105. Release et ASAN/UBSAN/leaks passent, avec quatre fixtures au-delà
des deux anciennes bornes B/C ; les cinq mutants sont tués par divergence
explicite. `qualification.json` scelle les sources/probes et hashes des
binaires locaux, sans embarquer ces binaires.

Le microbench a précédé la réconciliation des commentaires de domaine et
de la porte. Sa source exacte se reconstitue avec `census.before.hpp` et
`census.microbench.patch` ; son SHA est donné dans le reçu. La révision
de domaine n'a changé aucune instruction d'AxisBounds. Ne pas transformer
les observations de ce binaire en nouvelles mesures ou en gain global.

Après intégration, seules ces commandes ciblées ont été exécutées :

```bash
cmake -S morsehgp3D_v7 -B build/v7_axis_integrated -DCMAKE_BUILD_TYPE=Release
cmake --build build/v7_axis_integrated --target mhgp7_axis_bounds_gate --parallel 1
ctest --test-dir build/v7_axis_integrated --output-on-failure --no-tests=error -R '^mhgp7_axis_bounds($|_)'
```

Elles ont rendu 0 ; les six CTests ont passé (sortie brute conservée).
Aucun CLI ni build global n'a été lancé par l'agent de cet overlay.
La cible microbench est facultative et hors ALL, sans seuil temporel CTest.
La qualification globale et les paires B/C appartiennent à des reçus
distincts. GCP non utilisé pour ce travail.
