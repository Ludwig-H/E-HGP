# Preuves verticales, masses et qualification F

5 septembre 2026. `public_status=not_claimed`. Cette reprise lit les résultats moteur déjà clos ; elle ne compile ni n'exécute le produit pendant la fenêtre de mesure E/F. GCP non utilisé.

## Verticale : contrat et rejeu indépendant

Le [contrat vertical](../CONTRAT_VERTICAL_COURANT.md) définit les applications entre ordres et leur naturalité. Le [juge](../vertical_replay.py) relit les seuls deltas des sorties E scellées, reconstruit leurs composantes et utilise Gamma rationnel comme référence bornée à sept points. Ses cartes sont des objets d'audit : aucun export vertical ni resolver v7 n'est qualifié par ce rejeu.

Les reçus [normal](vertical/normal.json) et [Python optimisé](vertical/optimized.json) retrouvent, pour chacun des deux builds sources O2/UBSan : 16 appels historiques, 432 coupes globales, 1 608 couples coupe/ordres adjacents, 764 images de composantes, 720 carrés de naturalité et 400 carrés de composition sur deux niveaux. Les coupes comprennent chaque côté de chaque niveau, y compris quand seul l'ordre cible change.

Sur 15 688 requêtes de faces, 2 296 occurrences désignent une face absente de la table inférieure compressée. Le rejet `vertical.lower_label_unmaterialized` détecte le raccourci consistant à assimiler cette absence à une réponse géométrique. Ce n'est pas un mutant du produit. Sur les corpus essayés, chaque label conserve au moins une face présente ; aucune garantie générale de terminaison du scan n'est déduite de cette observation.

Les 248 inclusions strictes de points empêchent de remplacer l'image verticale par une égalité de couvertures. Les 144 changements de composante cible imposent de suivre ses successeurs horizontaux, même quand la clé canonique reste la même. Les exemples complets et hashes des entrées sont conservés dans les reçus.

```bash
PYTHONDONTWRITEBYTECODE=1 python3 morsehgp3D_v7/audits/vertical_replay.py
PYTHONDONTWRITEBYTECODE=1 python3 -O morsehgp3D_v7/audits/vertical_replay.py
```

## Masses et vote

Le [contrat de masses](../CONTRAT_MASSES_VOTE_COURANT.md) distingue l'univers des facettes, celui des cofaces incidentes et les niveaux au carré du moteur. Ses fixtures bornées se rejouent ainsi :

```bash
PYTHONDONTWRITEBYTECODE=1 python3 morsehgp3D_v7/audits/masses_vote_probe.py --receipt normal
PYTHONDONTWRITEBYTECODE=1 python3 -O morsehgp3D_v7/audits/masses_vote_probe.py --receipt optimized
```

Le rapport en précise les préconditions et les résultats. La conservation de H0 ne dispense pas de conserver les contributions de cofaces redondantes pour le rendu pondéré.

## Qualification F

Le [verdict F](f_qualification/review.json) conserve la contrelecture des campagnes propres à F et les pièces nécessaires à leur reproduction. Leurs exécutions appartiennent au constructeur ; les cartes verticales ci-dessus restent calculées depuis les sorties E. La revue statique de pile antérieure conserve son [reçu historique](../receipts_horizontal_20260905/f_delta/review.json).

Le [reçu s=8](f_qualification/pair_s8_review.json) confirme séparément la seule paire E/F close à 09:46:51 UTC : dix ordres, onze digests et les cardinalités brutes concordent. Les autres séparations encore en cours à cette inspection ne sont pas anticipées. Cette lecture n’attribue aucun gain de performance.

Pour relire les pièces F capturées sans dépendre des binaires privés :

```bash
PYTHONDONTWRITEBYTECODE=1 python3 -O morsehgp3D_v7/audits/receipts_vertical_20260905/f_qualification/verify_receipts.py
PYTHONDONTWRITEBYTECODE=1 python3 -O morsehgp3D_v7/audits/receipts_vertical_20260905/f_qualification/verify_receipts.py --self-test
PYTHONDONTWRITEBYTECODE=1 python3 -O morsehgp3D_v7/audits/receipts_vertical_20260905/math/combinatorial_anchor_counterexample.py
```
