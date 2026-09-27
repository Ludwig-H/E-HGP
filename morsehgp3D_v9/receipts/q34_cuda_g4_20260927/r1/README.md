# G4 — consommateur q34 par vagues, S2 uniquement

État : **completed**. Sources : `33c1d28d7f1d6a210061407d9282ca0ae125110c`.
Autorité : journaux VM bruts, reçus hôte, `SUMMARY.json` recalculé et preuve
d'arrêt de la génération exacte. Aucune tour FULL n'est calculée ici.

Trame entière sans sol ng00, grille 1 mm/u18, 39885 sites, K5/s8.
Sortie comparée intégralement à la référence native : P=23686751, E=9122704, S=2043612.
Une seule mesure de cette trame ; aucune borne de croissance ni qualification multi-scènes.

| Étape | ms |
|---|---:|
| Front CPU | 2097.075689 |
| Préparation CPU | 9898.347799 |
| Snapshot | 49.732965 |
| Runner GPU + tri CPU + libérations internes | 267.479866 |
| Dont vagues + petits compteurs D2H | 34.966614 |
| Dont transfert des survivantes + croissance hôte | 22.568056 |
| Dont tri/conversion CPU | 39.170853 |
| Construction S2 → sortie | 10215.560630 |
| Référence native de contrôle | 10080.259684 |
| Destructions finales groupées | 8.303474 |
| Processus complet mesuré | 22437.350192 |

Les lignes « dont » sont incluses dans le runner, elles ne sont pas à additionner.
Construction→sortie additionne préparation + snapshot + runner. La destruction
tardive de Prepared/snapshot survient avec celle de la référence, des sorties,
de l’index et des entrées : elle est publiée séparément, pas isolée par objet.
Le total paie aussi lecture/index/front, référence, comparaison et destruction.
Ce ne sont ni des temps FULL ni une preuve du contrat 100 ms.

Le gate compte 85 cas et 340 boucles **portables**.
CUDA exécute Q7/Q257 sur chacun, plus Q1 pour E≤4 ; le compteur runs
n'est pas un nombre d'appels device. Les brut restent inchangés.

Allocation observée : 188.416 s ; aucun montant facturé estimé.

Relecture LIVE, normale puis avec `-O` :

```sh
python3 -B morsehgp3D_v9/audits/b_q34_cuda_g4_readback_20260927/readback.py --readback CHEMIN_DU_RECU
```

Le snapshot et les originaux privés liés dans `PRIVATE_LINKS.json` restent nécessaires.
`SHA256SUMS` couvre toutes les pièces sauf lui-même. Ni archive, clé SSH,
nouvelle donnée KITTI, réponse OS Login brute ou état GCE non expurgé ne sont publiés.
