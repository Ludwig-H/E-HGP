# Contrelecture du correctif GPU et du banc — 4 octobre 2026

Complément borné au [paquet Euler/GPU précédent](../audit_gpu_euler_20261004/README.md), conservé intact. Sources produit `00800dd88d5d1368b1987e399a05e87a17f208ae` ; banc `b74f9ea3a0b986f659d388b8140067b0b35be0c2`. Aucune exécution native, CUDA ou GCP dans cet audit.

- **Plafond feuilles≤sites corrigé en source** : plafonds de ressources explicites, sommes bornées, réservations et préchauffage dans le chrono FULL. [198 contrôles scalaires et raccords](gpu_update008/README.md).
- **Ordre GPU favorable au pin008** : permutation stable par taille ; statuts, préfixes et sorties gardent l'ordinal original. [6 852 contrôles de transport bornés](gpu_order_008/README.md). Ils ne certifient ni matériel ni gain.
- **Non-vacuité du banc encore ouverte au pinb74** : « conforme » sans prise froide ou sans passes chaudes mesurées ; simulation des lignes absentes également acceptée. [75 contrôles AST](gpu_bench_b74/README.md). Exiger des prises suffisantes et les passes attendues, ou annoncer un verdict partiel ; le dernier dump ne prouve pas les passes intermédiaires.

**Le nouveau transport scratch/pool b74 n'est pas audité ici.** Les conclusions de mémoire et de count/fill au pin008 ne lui sont pas transférées. Ses exécutions G4 et le contrat 100 ms restent distincts de ces preuves portables.

Les trois capsules sont closes, sources et sorties incluses. `check.py` vérifie les inventaires exhaustifs et rejoue chaque modèle en Python standard normal et optimisé. Les 7 125 contrôles se recoupent : leur somme ne constitue pas autant de preuves indépendantes.

```sh
python3 -B -S check.py
python3 -B -O -S check.py
```
