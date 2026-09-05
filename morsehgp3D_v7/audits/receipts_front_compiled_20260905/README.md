# Raccord compilé des preuves du front

5 septembre 2026. Audits CPU indépendants, `public_status=not_claimed`, GCP non utilisé. Les calculs légers antérieurs restent dans [leurs reçus](../receipts_front_20260905/README.md) ; les présentes pièces exécutent les helpers C++ sur des fixtures bornées.

| Sonde et juge | Pièces exécutées | Portée par build |
| --- | --- | --- |
| [Fuseaux](../spindle_compiled_probe.py) | [Résumé, commandes et hashes](spindle/summary.json) | 4 116 racines, 5 184 fuseaux, 432 cœurs, 560 compteurs ; deux vrais mutants rejetés code 4 |
| [Secteurs et cordes](../secteur_corde_compiled_runner.py) | [Reçu](secteur_corde/receipt.json), [O2](secteur_corde/o2.json), [UBSan](secteur_corde/ubsan.json) | 736 bases, 212 racines, 96 cas affines, 156 cordes ; un mutant produit et une faute de paramètre d'audit détectés |
| [Cellules](../cell_compiled_oracle.py) | [Reçu, commandes et hashes](cell/summary.json), [entrées](cell/input.txt) | 38 400 cellules, 32 centres/cordes, 98 bits atteints ; trois vrais mutants détectés par le juge |

Chaque sonde possède deux builds C++20 stricts, O2 et O1 UBSan, sans diagnostic. Les conditions numériques exactes sont celles des commandes conservées, détaillées dans le [domaine CPU](../DOMAINE_CPU_COURANT.md). Ces six builds ne sont pas six suites CTest complètes.

Le [rejeu léger](../replay_compiled_front.py) contrôle les sorties conservées et les juges Python, sans recompiler. Les reçus [normal](replay_normal.json) et [optimisé](replay_optimized.json) distinguent les codes des ponts, ceux du juge et les divergences causales attendues.

La [contrelecture de qualification](../AUDIT_QUALIFICATION_20260905.md) utilise séparément le dossier `qualification/` pour les campagnes et CTests du constructeur. Les preuves brutes nécessaires y sont préservées avec leurs hashes ; elles ne sont pas présentées comme des exécutions indépendantes de l'auditeur.
