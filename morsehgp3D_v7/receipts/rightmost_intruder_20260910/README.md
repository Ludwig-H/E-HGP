# Intrus de rang Morton maximal — prototype NON INTÉGRÉ

10 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`. **La variante est NON INTÉGRÉE à la source active `910f45ba…` qualifiée ici. Aucun gain contractuel n'est revendiqué.**

Le [patch privé](rightmost.patch) change seulement deux lignes de `intruder()` : DFS droite d'abord et plages descendantes, avec retour immédiat. Il trouve exactement le dernier intrus du DFS gauche exhaustif de l'auditeur, sans balayer tous les intrus. Le [rapport expérimental original](report.md.source) donne la preuve par ordre Morton, les limites et les captures ; la [synthèse chiffrée](summary.json) est recalculée par le lecteur.

Qualification réellement exécutée : porte FULL à 28 nuages O2/ASan/UBSan (170 320 contrôles, 45 948 verticales), oracle rationnel de choix sur 72 840 requêtes en O2 et sous sanitizers, et deux mutants directionnels compilés puis rejetés avec leur raison causale. Les sources actives n'ont pas été modifiées.

À n=1 000, s=8, K=1..10, un thread, trois répétitions par bras donnent le même payload complet. Les nœuds visités pour les intrus passent de 5 452 553 à 4 430 457 (−18,75 %), les appels MEB de 583 337 à 565 406 (−3,07 %). La médiane du temps tour passe de 5,641 à 5,469 s **en environnement partagé** : ce faible écart n'est pas une preuve de gain de latence. Aucune extrapolation à 50k, aucun contrat 1 s/100 ms et aucune qualification massive/GPU. GCP non utilisé.

## Vérification portable

```bash
python3 -B morsehgp3D_v7/receipts/rightmost_intruder_20260910/verify.py
python3 -B -O morsehgp3D_v7/receipts/rightmost_intruder_20260910/verify.py
```

Le [manifeste logique d'origine](logical_manifest.json), inchangé, est épinglé à `27bcde2d6a58cd8c3eb892c165f4455f3c0ab920ea8921afb3a308d2a685aa5d`. Chaque nom logique désigne `objects/<sha256>.source` : ce stockage par contenu évite de dupliquer les arbres et les logs identiques, tout en conservant les sources, résultats bruts, scripts et hashes exacts. Les documents Markdown historiques sont des artefacts, pas une navigation active. Le lecteur vérifie tous les objets, reconstitue temporairement les noms logiques puis exécute le lecteur original inchangé ; il ne compile ni n'exécute aucun moteur et ne contacte pas GCP. Aucun ELF n'est publié.

Baseline : `910f45baea1750b11d2b34f40c893c9d1a34f950705cdb127ffa226de60f7b2e`. Variante privée : `7bb2ac1fab0a1cac8c516b01a6114ffa42ae608f46793cde325550a62bee3245`. Cette publication ne vaut pas autorisation d'intégration et n'établit pas une borne sous-quadratique globale.
