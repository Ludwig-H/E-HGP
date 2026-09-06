# Contrelecture indépendante du filtre privé R2

6 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`. GCP non utilisé.

Le [paquet constructeur](../../receipts/meb_filtered_20260906/README.md) est cohérent avec ses captures : 504 fichiers, 503 entrées du sceau `c8268e85`, 502 copies contrôlées, 63 sources capturées, 41 commandes closes et onze identités de binaires non distribués. Les onze compilations enregistrées emploient C++20, les quatre avertissements stricts et aucune macro `MHGP7_TESTING`. Les builds SAN ajoutent ASan/UBSan sans récupération. Les 224 entrées de dépendances issues des onze fichiers `.d` correspondent aux sources capturées ; `-MMD` ne décrit pas les en-têtes système et ce n'est pas un build hermétique. Le contrôleur capturé définit les options d'arrêt SAN et LeakSanitizer ; ce n'est pas une capture indépendante de l'environnement des processus.

Le [lecteur de l'audit](constructor_review.py) n'importe aucun juge constructeur et ne lance aucun C++. Il réutilise uniquement la primitive géométrique rationnelle de l'audit, épinglée `ad6c0d6c`, pour recalculer les attendus depuis les stdin. Ses [résultats normaux](constructor_review_normal.json) et [optimisés](constructor_review_optimized.json) portent les mêmes octets et les mêmes vérifications.

Par build capturé, les 3 430 appels sur 178 nuages ordonnés retrouvent les supports entiers, clés, rayons, représentations brutes q4, treize statistiques, coquilles et sentinelles attendus. Les 1 507 ordinaux couvrent exactement tous les tuples lexicographiques de cardinal 2–4 sur n=2–11, jusqu'au rang 550. Les 59 lignes budgétaires sont recalculées séparément, notamment huit états près de MAX et quatre appels persistants : 25 succès, 28 refus L, six refus de coquille, 50 formes proposées. Les formes respectent la borne filtrée de 146 et la charge prospective, y compris après certification suivie d'un refus L.

Les trois mutations privées ont chacune un seul changement causal vérifié. Les sorties conservées du shell omis divergent sur 28 appels, celles de l'ordinal +1 sur 990 et celles du niveau q4 multiplié par deux sur 64. Dans ce dernier cas, les 64 divergences portent sur la représentation brute alors que les rayons rationnels restent identiques. Ce sont des rejugements des moteurs O2 du constructeur, pas trois nouvelles exécutions mutantes de l'audit.

Les autres gates sont relues comme des tests compilés avec résumés, sans inventer une télémétrie par cas : 9 344 comparaisons géométriques, 62 permutations locales, 654 préfixes et 180 raccords natifs. Les mutants prospectifs ont le code 4 et les causes attendues : 50 violations budgétaires, 22 661 géométriques. Les stdout nominaux O2/SAN sont identiques pour les cinq portes examinées.

Deux points de code ferment des limites auparavant ouvertes. `geometry_gate.cpp` contrôle explicitement la vraie scène q4 de rang 550 depuis c=MAX−550 avec L=MAX, certificat rapide et aucun repli. `trajectory_gate.cpp`, fonction `order_budget_admissible`, confronte le tétraèdre régulier au calendrier q3 puis q4 : les huit appels locaux, six appels natifs et le rejeu global P3 vérifient trois égalités de supports et trois différences d'admission. Le rejet `order_budget.calendar_changed` survient **après** la comparaison des supports ; la contre-fixture à plusieurs bases reste explicitement hors Q positif. Les bases intermédiaires sont celles du rejeu côté test, raccordé aux terminaux, Work et arités du natif.

Ce contrôle ferme la lecture des captures privées R2. Il ne qualifie pas une intégration dans FULL, la persistance du Work du Builder produit, les exceptions d'observateurs ou un gain de latence. Les derniers moteurs du paquet sont clos à `2026-09-06T08:14:41.633637+00:00` ; l'audit n'en a relancé aucun.

Rejeu depuis la racine, sans compilation :

```bash
python3 -B morsehgp3D_v7/audits/receipts_filtered_review_20260906/constructor_review.py
python3 -B -O morsehgp3D_v7/audits/receipts_filtered_review_20260906/constructor_review.py
```
