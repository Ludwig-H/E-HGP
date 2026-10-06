# Relecture des deux reçus G4 intégrés — 6 octobre 2026

Cadre : `exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`. Cette capsule vérifie les pièces publiées au commit `d8c015dbd5059fc06d18c0991923b51c26b18561` contre les deux sessions locales closes. Elle conserve seulement des métadonnées, empreintes et verdicts ; aucune donnée LiDAR, sortie binaire native ou journal brut.

| Session | Source réellement paquetée | Arrêt ciblé certifié UTC | Résultats directement lus |
| --- | --- | --- | --- |
| `v11.20261006.claudesupkr` | `07428324eaa5cbf26446362bceadd77722c1a5b7` | 12:57:30.806 | 15/15 CTests PASS, 0 échec, 0 absent |
| `v11.20261006.claudej2memo` | `34a8a561d0c6c7f346608c49ac9113e3830b216f` | 12:16:35.723 | 14/14 CTests PASS ; deux rapports GPU K5/K10 conformes, refus vides |

Les deux chaînes présentent `DONE=0`, `completed`, worker code 0, `closure=stopped`, même génération de démarrage à la fermeture et arrêt ciblé certifié. Les empreintes des archives, paquets, plans et fichiers de résultats sont rejugées. Les pièces publiées couvertes par leurs `SHA256SUMS` (5 + 7) sont vérifiées ; plans/reçus/lancements/rapports sont identiques aux pièces locales et les extraits CTest correspondent exactement aux verdicts archivés. Une archive Git sélective vérifie l'égalité exacte de 636 et 635 fichiers livrés utiles, sans inclure docs ni reçus historiques. Les empreintes de ces inventaires de source sont conservées dans `summary.json`.

Le résultat Kruskal porte sur le sélecteur final SPv2 de `07428324e`, en Release u21, Python normal : le lecteur spanning, les deux oracles et les douze portes d'échelle/LiDAR API/CLI ont effectivement PASS. La session J2memo utilise le filtre de rôle antérieur et ne qualifie pas ce sélecteur Kruskal. Ses deux modes FULL CPU `16379` et GPU `81915` sont joués sur les trois trames déclarées, W48, K5/feuille16 et K10/feuille24. Les 60 processus représentent 168 passes FULL ; 60 empreintes canoniques sont conservées et égales aux six références. Les 108 passes chaudes intermédiaires ont un statut réussi mais aucun dump/registre individuel archivé : l'identité et le registre du dernier passage chaud sont contrôlés par le juge épinglé, dont les ancres sont conservées.

Aucune qualification `_opt`, mutants, ASan/UBSan, TSan ou u18/u24 n'est acquise par ces sessions. Aucun contrat FULL de 100 ms, ni sortie points/plat, ni qualification de l'instrumentation chrono ultérieure n'en découle. Cette relecture ne reprend pas de statistiques de temps et ne réexécute aucun calcul natif.

Rejeu depuis ce dossier :

```sh
python3 -B replay.py
python3 -B -O replay.py
sha256sum -c SHA256SUMS
```

Le rejeu dépend des objets Git locaux et des fichiers `package/package.tar.gz`, `package/plan.json`, `receipt.json`, `launch.json`, `DONE` et `results/results.tar.gz` des deux sessions sous `/workspaces/.ehgp-sessions`. Les options `--repo` et `--sessions` permettent de déplacer ces dépendances. La capsule est portable avec ces archives ; elle n'est pas un reçu autonome contenant leurs octets. `--capture` sert seulement à la création initiale : un rejeu ordinaire refuse toute divergence avec la synthèse figée.
