# Porte de flux complet q4 sur douze sites

Le [cas exact](../check_q4_without_q3_faces_20260922.py) fournit une boule q4 positive de clé `(1,−40,−40,−40,900)` et profondeur 0 à K3. Ses **quatre** faces q3 ont profondeur 2 et sont rejetées à K3. La [sonde dense](../COMPLETUDE_Q4_CLE_REMBOURREE_20260923.md) vérifiait seulement la présence de cette clé ; cette porte compare le **flux complet** du petit nuage à l'oracle rationnel indépendant du gate produit.

[`check.cpp`](check.cpp) inclut une copie temporaire non modifiée de `tests/gen/wspd_q34_gate.cpp`, sauf le renommage de son `main`, pour réutiliser `global_oracle`, `eligible`, `check_global` et leurs comparateurs de clé, support, profondeur et coquille. Le [lanceur](run.py) prépare cette copie dans un répertoire temporaire et lie contre la bibliothèque générateur du build indiqué. Aucun fichier produit ou test n'est modifié.

Deux ordres d'IDs (initial et inverse) sont chacun comparés pour `s=8,10,12`. Six configurations par séparation : Local28 avec les quatre couples clipping/saturation, Window30, et Local28 avec les options de filtrage/atlas de la sonde dense. Chaque configuration exécute le flux mono et le flux parallèle à W1 et W4 : **36 configurations géométriques, 108 flux complets**. Les branches Local28 forcent un atlas non trivial (`leaf_sites=0`, profondeur 3, 85 cellules, budget Z 32). Les options de clipping/saturation ne s'appliquent pas au backend Window30. La porte vérifie séparément que les quatre faces q3 ont profondeur 2 et que l'unique clé cible n'a aucune présentation d'arité inférieure à 4 ; sa présentation q4 a support et coquille exactement égaux aux quatre sites attendus.

Commande utilisée depuis `/workspaces/E-HGP` :

```sh
python3 morsehgp3D_v9/audits/q4_global_12sites_20260923/run.py \
  --source-root /workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v9 \
  --build /workspaces/E-HGP/build/v9-open-worktree/build/v9-dev \
  --receipt morsehgp3D_v9/audits/q4_global_12sites_20260923/receipt.json
```

Le [reçu](receipt.json) est **PASS** : 990 quadruplets inspectés par l'oracle, 338 boules rationnelles distinctes, 3 580 assertions et 108 flux complets égaux. Son `source_commit`, les SHA-256 du gate produit, de la bibliothèque, du sidecar et du binaire compilé, ainsi que les états `src/gen` et `tests/gen` vides, fixent la provenance locale. La bibliothèque et les en-têtes du build sont des dépendances **LIVE** ; ce reçu n'est pas un paquet autonome. Ce résultat ferme le trou de fixture des quatre faces rejetées dans le petit domaine testé, sans démontrer l'induction générale de l'atlas ni la complétude de toutes les clés d'une grande trame LiDAR.

Un rejeu du même lanceur sous `python3 -O`, sans réécrire le reçu, rend aussi `PASS` avec les mêmes 108 flux, 3 580 assertions et SHA-256 du binaire.

Pour en faire une régression produit permanente, ajouter ces douze sites et l'assertion `q_min=4` au gate global existant, en gardant sa comparaison exhaustive des payloads ; le sidecar sert de spécification exécutable de ce cas.
