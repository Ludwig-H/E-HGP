# Archive de qualification de l'overlay mono

Date de qualification et d'archivage : 4 septembre 2026. `GCP non utilisé`.
`public_status=not_claimed`. Aucun binaire n'est conservé dans ce dossier.

Cette archive conserve une preuve historique, obtenue avant l'intégration
du fold B inline. Le texte [OVERLAY_HISTORIQUE.md](OVERLAY_HISTORIQUE.md)
est une copie intacte du compte rendu initial : son « non intégré » et ses
commandes absolues décrivent cet instant, pas l'état courant du produit.
Ne pas rejouer son ancien mode de référence contre le `run.hpp` produit,
qui a depuis changé. L'intégration est décrite dans
[MODE_MONO.md](../../docs/MODE_MONO.md), et possède des portes maintenues
distinctes, sans mode de référence historique ni chemin absolu.

## Octets et autorité

| Fichier archivé | SHA-256 historique |
|---|---|
| `reference_run.hpp` | `885348a92f48658642e3783027cb7c4f239f1c8e1a0b91c66a698f3be6b29762` |
| `overlay_run.hpp` | `6b9526d896850b94e6455c040ca8bcd038c292c003b1138407e1d57f4fd9f441` |
| `historical_test_original.cpp.txt` | `cb427753568aafe7ec61f392a0b373b5ebe7c129818d27ce749bb21cebad7eee` |

`receipt.json` et `sanitizer_receipt.json` sont les reçus bruts initiaux.
Tous leurs stdout/stderr sont conservés. Les chemins d'exécution et hashes
de binaires qu'ils contiennent sont de la provenance, pas des binaires
distribués ni une instruction visant les sources actuelles. Le contre-test
`counterexample` retourne 1 lorsque l'original est soumis à zéro création
de thread ; ses diagnostics démontrent la non-vacuité de l'interposition.

`historical_gate.cpp` ne change que les deux chemins d'inclusion de la sonde
initiale : ils visent les deux headers archivés, jamais le header produit.
Les dépendances communes restent externes à cette archive ; celle-ci n'est
pas une distribution autonome. Un rejeu avec des dépendances actuelles
constitue une nouvelle qualification, pas la reproduction automatique du
reçu historique. Exemple de compilation depuis la racine, avec sorties
uniquement dans le dossier de build dédié :

```bash
mkdir -p build/v7_mono_archive_replay
g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -pthread -DMHGP7_TESTING=1 -DMHGP7_MONO_REFERENCE=1 -Imorsehgp3D_v7/src/pipeline morsehgp3D_v7/receipts/mono_inline_overlay_20260904/historical_gate.cpp -ldl -o build/v7_mono_archive_replay/reference
g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -pthread -DMHGP7_TESTING=1 -Imorsehgp3D_v7/src/pipeline morsehgp3D_v7/receipts/mono_inline_overlay_20260904/historical_gate.cpp -ldl -o build/v7_mono_archive_replay/inline
build/v7_mono_archive_replay/reference --require-zero
```

La dernière commande doit retourner exactement 1 et nommer le défaut de
création de threads. Les sorties originales et inline des quatre modes
étaient identiques octet pour octet ; leurs temps sur onze points ne
qualifient aucun objectif de latence ni de passage à l'échelle.

## Porte maintenue après intégration

Le sous-dossier [integrated_smoke](integrated_smoke/receipt.json) conserve
séparément les quatre CTests ciblés du code intégré : 4/4 passent. Il contient
le log complet, le XML CTest et les flags du compilateur. Le header, la sonde
et le binaire sont épinglés, mais pas la fermeture entière des dépendances :
ce contrôle ciblé n'est pas la qualification Release complète. Après ce
contrôle, seule la première ligne de commentaire CMake a été corrigée de
v6 en v7 ; les deux hashes CMake sont distingués dans le reçu. Aucun CLI ni
build global n'a été lancé par cette tâche d'intégration.
