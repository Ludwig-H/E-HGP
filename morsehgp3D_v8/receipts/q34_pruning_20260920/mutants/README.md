# Qualifications auxiliaires de la tranche 25

20 septembre 2026 : **trois mutations compilées tuées** et **vingt
différentiels du chemin par défaut concordants**. Ces expériences restent hors
de l'inventaire moteur de 158 sources. Aucun fichier produit, ancien reçu ou
artefact des builds n'a été modifié ; GCP non utilisé.

Le commit de base est `77f659e4db6213c729eb648362f3cebbba67dcd7`. Les captures
identifient la tranche non encore commitée par les hashes des **158 sources**,
pas par ce seul commit. Sources, exécutables, archives, caches et auxiliaires
pertinents sont épinglés avant/après chaque expérience.

## Mutations du produit

Capture [compiled_15yykks6](compiled_15yykks6/) : baseline
`mhgp8_q34_family_pruning_gate --selftest` PASS, **5 176 contrôles**, 178 appels
de primitive, 129 appels par seed et 63 par arête. Les trois variantes sont
compilées hors dépôt puis liées avant l'archive inchangée de
`build/v8_q34_pruning_20260920`, en réutilisant l'objet de la porte.

| Mutation unique | Échec sémantique observé, code 1 |
| --- | --- |
| Accepter `maximum_power <= 0` comme témoin q4 | `witness flags differ from rational strict certificate` |
| Publier la racine entière inférieure au lieu de supérieure pour un non-carré | `integer Jung bound differs from multiprecision ceil sqrt` |
| Supprimer q3 dès que les témoins ont rejeté q4 | `pruned seed differs from rational complete census, depth or shell` |

**Dix commandes** sont conservées : une baseline puis compilation, lien et
exécution de chacune des trois variantes. Toutes les compilations et tous les
liens réussissent ; aucun signal, défaut de compilation ou simple manque de
non-vacuité n'est compté comme une mutation tuée. Les sources originales,
modifiées, patches, hashes d'objets/binaires et sorties brutes sont archivés.
Les artefacts temporaires restent dans `/tmp/mhgp8_q34_pruning_mutants_ewuk08xf`.

Le [harnais](run_mutants.py) adapte explicitement celui de la tranche 24, sans
le modifier. Ce sont trois perturbations causales d'une porte existante,
**pas trois oracles indépendants** ni une preuve d'absence de tout défaut.

## Compatibilité du chemin par défaut

Capture [differential_syl9zcwk](differential_syl9zcwk/) : **40 commandes,
20 paires** de `mhgp8_q34_cover_probe`, ancien puis nouveau build :

- Référence épinglée : `build/v8_q34_cover_20260920`.
- Nouveau : `build/v8_q34_pruning_20260920`.
- Fonds `far` et `cap` : n = 8 000, 16 000, 32 000, K = 5 et 10, soit 12 paires.
- Adversaire : n = 32, 64, 128, 256, K = 5 et 10, soit huit paires.

Le [lecteur différentiel](run_default_differential.py) valide séparément chaque
sortie avec le validateur qualifié de la sonde cover, puis exige l'égalité de
**tous les champs JSON sauf `timings`** : entrée, travail géométrique, préparation,
mémoire, validation et digests de sortie. Les quarante commandes concordent
avec leur reçu et leur jeu de paramètres. Les anciennes sorties restent
physiquement vérifiées par la sonde ; la comparaison entre binaires utilise
les champs publiés et leurs digests, pas un nouvel oracle exhaustif.

Le binaire/cache anciens sont authentifiés contre la capture épinglée
`q34_cover_20260920/scale_7ganupv_`. Ses manifeste et clôture sont copiés
intégralement dans `OLD_MANIFEST.json` et `OLD_COMPLETION.json`, avec hashes
vérifiés et égalité des 151 sources historiques avant/après. Cela ne prétend
pas que ces anciennes sources correspondent au worktree actuel.

Les chronos bruts sont conservés et validés mais **aucun gain de performance
n'est déduit de ce différentiel**, exécuté pendant d'autres qualifications.
Il concerne une arête et le chemin sans préfiltre, pas la tour complète, la
complexité globale ou les contrats G4.

## Fermeture et lectures

Les huit lectures **normal/`-O`, historique/`--check-live`**, pour les deux
captures, passent. Leurs commandes et sorties exactes figurent dans
[READBACK.json](READBACK.json). Le lecteur historique n'exige pas la survie
des binaires mutés dans `/tmp` ; le mode live vérifie aussi les fichiers actuels.

| Capture | SHA256 manifeste | SHA256 clôture |
| --- | --- | --- |
| `compiled_15yykks6` | `c1ba5545cf16eca3539f3963045ad5cded71abd988d009f8a381b60444123dcb` | `4bbefc7fb1a504c78438e3cca38f462786a95799693ac51aecb53e92d6a5d25e` |
| `differential_syl9zcwk` | `613151a929133b4ffcc4aaf54e1c0ac7a03892f9b35be3f385a2cfa871c4f9d6` | `ca860f7b865fb9d9685ffda1c812355483a230a38ee20a0625bde35d4137f75d` |

Depuis la racine du dépôt :

```sh
python -B morsehgp3D_v8/receipts/q34_pruning_20260920/mutants/run_mutants.py read morsehgp3D_v8/receipts/q34_pruning_20260920/mutants/compiled_15yykks6
python -B -O morsehgp3D_v8/receipts/q34_pruning_20260920/mutants/run_default_differential.py read morsehgp3D_v8/receipts/q34_pruning_20260920/mutants/differential_syl9zcwk --check-live
```
