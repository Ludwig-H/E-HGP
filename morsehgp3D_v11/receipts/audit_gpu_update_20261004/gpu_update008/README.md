# Correctif GPU 00800dd88 : recoupe de source

Source figée : `00800dd88d5d1368b1987e399a05e87a17f208ae`. Cette annexe indépendante complète les captures privées `batch_transport` et `batch_transport_live`, demeurées intactes. Elle ne qualifie aucune exécution native, CUDA, G4 ou aucun contrat de temps.

| Point revu | Conclusion et ancrage dans les copies |
| --- | --- |
| Nombre de feuilles | L'ancien refus `count > cloud_sites` est supprimé. `leaf_batch.cpp:71–72` admet jusqu'à `2^40` travaux ; `leaf_batch_cuda.cu:245–248` ajoute le plafond `2^32`. Le témoin antérieur de 80 feuilles pour 9 sites n'est plus refusé par cette garde. |
| Réductions | `leaf_device_predicates.hpp:29–38` : P = 41 448 ; C = 3 979 008 < 2^22. Pour chaque champ, C × 2^40 < 2^62 ; sur CUDA, C × 2^32 < 2^54. Ceci remplace l'ancienne hypothèse nombre de feuilles ≤ nombre de sites. |
| Ordre CUDA | `leaf_batch_cuda.cu:150–163,265–288` : permutation stable des travaux par taille décroissante, tableau hôte et tableau device réservés dans le budget commun. Les deux noyaux récupèrent l'indice original j, conservant statuts, préfixes et places d'écriture par feuille (:54–61,80–88). Le modèle scalaire vérifie cet ordre et ses sorties sur 41 listes bornées. |
| Mémoire et durée de vie | Réservation avant `cudaMalloc`, `cudaFree` dans le corps du destructeur avant destruction de la réservation (:97–117). Les tableaux d'ordre et tous les tableaux device coexistent avec les buffers hôte téléchargés, eux aussi réservés (:255–321). Résultat favorable pour les payloads explicites ; contexte/piles internes du runtime et échecs du pilote ne sont pas qualifiés ici. |
| Préchauffage et chrono | `bench/full_probe.cpp:200–204` démarre le chrono FULL puis appelle le préchauffage avant l'index. `single_pass.cpp:155–163` le demande avant la frontière. Le préchauffage n'est pas retiré du chrono FULL. |
| Concurrence et sortie | État statique, lancement sous mutex, écriture de ns par un seul thread, lecture après join sous mutex, join au destructeur si nécessaire (:126–148,221–235). L'exécuteur attend ce thread puis contrôle son propre appel CUDA (:252–254). Pas de référence à un objet détruit constatée sur ce chemin ; aucun test de course ni arrêt forcé exécuté. |

`prefetch_ns` est la durée complète du travail asynchrone, recouverte par l'index/catalogue : elle ne s'additionne pas aux temps mur. Elle reste mémorisée dans l'état du processus et peut être reportée dans une passe chaude sans nouvelle ouverture. `device_init_ns` comprend l'attente résiduelle du join et l'appel de contrôle ; `device_bytes` ne représente pas un pic global simultané hôte + device.

Les six fichiers complets sont accompagnés de petits extraits de raccord de `catalogue.hpp` et `single_pass_batch.cpp`, avec leurs empreintes Git. Les plafonds ci-dessus établissent les bornes scalaires ; l'admission réelle CUB, la grille matérielle, la compilation et les résultats CUDA restent à qualifier. La permutation ne transforme pas les deux énumérations count/fill en une unique énumération physique.

Rejeu portable, depuis cette capsule :

```sh
python3 -B -S check_update008.py
python3 -B -O -S check_update008.py
```

Les sorties `stdout_normal.json` et `stdout_opt.json` portent le domaine exact de ces contrôles. Les inventaires clos incluent tous les payloads et leurs sous-inventaires ; seul `SHA256SUMS` à la racine est exclu de son propre inventaire.
