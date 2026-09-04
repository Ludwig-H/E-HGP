# Préparation de la qualification C fraîche

État initial : **préparé, non exécuté**. Aucun résultat de build ni de test C n'est revendiqué ici.

Le runner est inerte sans `--execute`. Root doit d'abord intégrer et revoir le mono strict et l'argmin du census, terminer la campagne mono B/C, puis donner explicitement le GO. Les six noms de portes `axis_bounds` ont été confirmés par l'agent math lors de l'intégration ; un inventaire différent échouera fermé.

```bash
python3 -B morsehgp3D_v7/receipts/release_c_20260904/run_qualification.py
```

Après coordination seulement, avec le chemin réel d'une campagne terminale scellée :

```bash
python3 -B morsehgp3D_v7/receipts/release_c_20260904/run_qualification.py --execute --after-campaign morsehgp3D_v7/receipts/REMPLACER_PAR_CAMPAGNE_BC_TERMINEE
```

Le build `build/v7_c_qualification` doit être absent. Le runner ne le supprime, ne l'écrase et ne le réutilise jamais. Il configure Release C++20 avec CUDA explicitement désactivé, construit toutes les cibles CPU, inventorie puis exécute toutes les portes `gate` avec JUnit. Bornes externes : configure120s, build3600s, CTest7200s ; deux tâches parallèles par défaut. Les durées de qualification ne sont pas des mesures de débit.

Avant de commencer et entre étapes, il contrôle les processus de mesure visibles, les sources et les SHA des binaires C mesuré et B conservé. Le manifeste terminal de campagne doit être valide, même si la campagne comporte des censures conservées comme échecs ; ses sources et son candidat doivent correspondre à C. Le contrôle des processus ne prétend pas prouver l'inactivité globale de l'hôte et ne remplace pas la coordination root.

Le lien source→construction est nouveau : build inexistant auparavant, sources épinglées aux frontières, commandes et sorties complètes, `compile_commands.json`, dépendances compilateur, flags/liens et SHA binaires. Il ne s'agit pas d'un build hermétique immuable. Le SHA du CLI frais est comparé à celui du CLI mesuré ; s'ils diffèrent, aucune qualification des mesures ne peut être transférée automatiquement entre les deux binaires.

Le `source_snapshot` du banc apparié ne lit que `src/`, `cli/`, `oracle/` et `CMakeLists.txt` de v6/v7, puis `v7/bench/compare_v6_v7.py`. Ce runner, ce README et les reçus dans `receipts/release_c_20260904` sont hors de ce périmètre : leur préparation ne change pas le snapshot gelé. Le banc lui-même reste inchangé.

`public_status=not_claimed`. Aucun résultat antérieur A ou B n'est réutilisé comme exécution fraîche C. GCP non utilisé.
