# Lots FULL groupés — scellement des essais historiques du 10 septembre 2026

`phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

Ce reçu rassemble les captures existantes `run_r1`, `run_r2` et `run_r3_vertical` sans refaire les essais ni consommer les sources actives. Les statuts originaux sont conservés : **failed, failed, passed**. Résultat borné : six exécutions nominales O2/SAN réussies, trois arguments invalides au code 2 et six mutants effectivement compilés puis rejetés au code 1. Les deux échecs de compilation ne sont jamais comptés comme des mutants tués.

## Historique exact

| Capture | Acquis réel | Échec conservé |
| --- | --- | --- |
| r1 | Trois portes, chacune O2 et ASan/UBSan ; sorties appariées identiques | Le premier mutant ne lie pas : son chemin de binaire était déjà le répertoire de ses sources. Aucune exécution de ce mutant. |
| r2, mutants seuls | Les cinq mutants croissance/inerte groupés, croissance/inerte singleton et rayon strict compilent puis sont tués | Le mutant vertical utilise `ExactLevel{U192{}, 1}`, de type invalide. Il ne compile pas et ne s'exécute pas. |
| r3, reprise verticale seule | Le même remplacement corrigé en `ExactLevel{{0, 0, 0}, 1}` compile puis échoue sur `pair/variant0`, `vertical.all_subfacets_at_same_open_or_closed_cut` | Aucun échec de harnais dans cette reprise. |

Les deux nouveaux cas doublés activent réellement la branche groupée : le retrait de croissance est détecté par le multiensemble de couvertures de `growth_ABCZ_doubled_lot`, le retrait d'ancre inerte par l'absence d'ancre verticale. Les autres raisons exactes sont conservées dans les stderr et exigées par le lecteur. La porte tour compare toutes les coupes ouvertes/fermées à l'oracle indépendant, avec multiplicités, histoires et verticales ; la porte travail vérifie les comptes d'opérations des lots.

Le nominal historique donne 28 nuages, 170 320 contrôles et 45 948 vérifications verticales. La porte travail couvre 28 nuages, 176 lots groupés, 306 lots singletons et 520 cases DSU de lots. L'ancien front, conservé seulement comme résultat historique, donne 429 268 contrôles et 13 rejets ; **ce résultat n'est pas transféré au front actif**. Sa qualification fraîche est le [reçu distinct du front hôte](../witness_front_20260910/README.md).

## Identité et limites de preuve

Les inventaires avant exécution de r1 et r2 sont identiques (60 fichiers projet). Une seule copie est publiée dans `source_snapshot/`. FULL est épinglé à `0b72b4e9cb3858f7026d6b5d2b55f8a7b191903fc37aa24c140dfb8b557657e8`, avant l'optimisation cache/résidence ; la porte 28 nuages est `bf1a28242dd2d6897f3dc02edff8c6077dbec1caa3f9b9a67813dcc748e3d75a`. Les variantes ne stockent que leur en-tête modifié et le patch causal. Aucun ELF ni arbre source mutant complet n'est dupliqué.

Les sources archivées ont été re-hachées avant/après **publication**, ce qui est distinct d'une capture après les essais historiques : r1/r2 s'étant interrompus, leur script n'a pas écrit `sources_after.json`. Ce reçu n'invente pas cette preuve manquante. Les `.d` de r1/r2 décrivent les dépendances projet réellement nommées par le compilateur ; leur contenu est confronté aux archives épinglées. La reprise r3 copie r2 puis change un seul en-tête, mais n'a pas capturé de `.d` : son inventaire copié n'est pas présenté comme une trace indépendante du compilateur. Les bibliothèques système/Boost ne sont pas archivées ; leur chemin historique et la version du compilateur sont capturés.

## Lecteur portable

```bash
python3 -B morsehgp3D_v7/receipts/grouped_lot_gate_20260910/verify.py
python3 -B -O morsehgp3D_v7/receipts/grouped_lot_gate_20260910/verify.py
```

Le lecteur n'exécute aucun moteur, n'accède pas aux chemins absolus historiques, vérifie les hashes/codes/diagnostics et distingue les deux échecs de compilation des six refus causaux. Les scripts originaux, intents, commandes closes, stdout/stderr et reçus failed restent disponibles dans `capture/`. Aucune mesure de performance ou de complétude globale S1 n'est acquise ici. GCP non utilisé ; aucun contrat 50k/1s, 100ms ou massif revendiqué.
