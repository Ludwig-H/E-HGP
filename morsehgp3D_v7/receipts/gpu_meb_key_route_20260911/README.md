# Sélection MEB et clé primitive : couture locale qualifiée

Ce paquet porte une couture privée, pas le terminal GPU ni la tour FULL. Le chemin nominal produit la sélection MEB et sa clé primitive avec les helpers HD ; il ne reforme aucune clé côté hôte et ne rend aucun niveau exact.

O2 et ASan/UBSan/LSan ROOT confrontent 605 cas aux attentes Gram épinglées et au CPU : 22 245 contrôles, 6 050 mots de clé, 47 rejets, quatre flags de sélection et 17 mutations de clé. Les supports, coquilles et compteurs de sélection sont conservés. Le travail nominal déclaré comprend 605 matérialisations backend, zéro matérialisation et zéro puissance hôte. Le backend de ces exécutions est **HOST_STUB**.

NVCC compile et lie strictement pour sm120. **Aucun device n'a exécuté cette nouvelle couture** et GCP n'a pas été utilisé ici. Les exécutions G4 des deux primitives antérieures restent des preuves séparées ; elles ne qualifient pas leur raccord. Aucun temps de la tour FULL, gain RSS/VRAM ou contrat 50k n'est revendiqué.

L'acceptation runtime est uniquement celle du transport et des métadonnées, pas un certificat géométrique hôte ni une preuve du premier support. Des clés altérées ou non primitives franchissent cette acceptation et sont réfutées par le juge. L'ancien matérialiseur hôte est supprimé à la compilation de la gate. La forme acceptée q3/q4 est reformée une fois sur le backend pour la clé ; cette couture ne prétend pas encore réutiliser au mieux ce temporaire.

Voir [PROTOTYPE.md](PROTOTYPE.md) pour l'API, les limites et les commandes, et [PLAN_TERMINAL.md](PLAN_TERMINAL.md) pour le prochain raccord proposé. Les sources exactes et leurs originales/patches sont lisibles sous `sources/`, `upstream/`, `originals/` et `patches/`. `captures/` conserve toutes les tentatives de cette couture ; aucune preuve fermée antérieure n'est réécrite. `binary_pins.json` épingle les exécutables sans les distribuer. Aucun ELF ni vendor n'est contenu dans le paquet.

Le lecteur portable ne compile et n'exécute aucun binaire C++/CUDA :

```bash
python3 -B verify.py
python3 -B -O verify.py
```

Il vérifie l'inventaire physique, les hashes, la déduplication réversible, les snapshots réellement compilés, les logs/codes, les mêmes sources O2/SAN/NVCC et le contrat JSON exact. Le succès du lecteur vérifie cette preuve locale, jamais une exécution GPU ni la complétude FULL.
