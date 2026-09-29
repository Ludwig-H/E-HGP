# Reçu : course de données dans le pool de fils, corrigée (29 septembre 2026)

`backend=reference_cpu`, `public_status=not_claimed`. GCP non utilisé.

## Symptôme

En rejouant les portes, l'oracle du catalogue a échoué une fois sur 161 contrôles
([`ctest_gate_echec_initial.txt`](ctest_gate_echec_initial.txt)). Le cas : K = 1, 18 points coplanaires sur une
grille ([`cas18_coplanaire_k1.u32le`](cas18_coplanaire_k1.u32le)), refusé avec `census_mismatch`, c'est-à-dire une
boule émise deux fois. Rejoué seul, le cas passait. L'échec est donc intermittent, et il est apparu sous charge.

## Reproduction et localisation

- **Stress** ([`stress18.sh`](stress18.sh)), 6 copies parallèles de 400 exécutions à 2 fils :
  - binaire à `7eee86c53` : 1 refus `census_mismatch` sur 2 400 ([`stress_7eee86c53_ancien_pool.txt`](stress_7eee86c53_ancien_pool.txt)) ;
  - binaire `568d45297`, d'avant J2 : 0 sur 2 400 ([`stress_568d45297.txt`](stress_568d45297.txt)), ce qui ne
    l'innocente pas à ce taux.
- **ThreadSanitizer** (`-DMHGP10_TSAN=ON`, lancé par `setarch -R`, sans quoi TSan bute sur la randomisation
  d'adresses de ce noyau) : course de données dans `src/sched/pool.cpp`, dès la deuxième exécution
  ([`tsan_rapport_avant.txt`](tsan_rapport_avant.txt)). Un ouvrier lit `n_` dans `run_chunks` sans verrou, alors que
  le `parallel_for` suivant l'a réécrit.

## Mécanisme

`parallel_for` n'attendait que les ouvriers déjà inscrits (`active_`). Le fil appelant exécute lui aussi des tranches.
S'il les avait toutes faites avant qu'un ouvrier lent ne s'inscrive, il rendait la main, puis publiait le travail
suivant. L'ouvrier, inscrit entre les deux, lisait alors sans synchronisation le compteur `next_`, `n_`, `grain_` et
`job_` d'un autre travail. Il pouvait prendre une tranche du travail suivant, par exemple avec son ancien compteur,
et l'exécuter, éventuellement avec la mauvaise fonction : une tranche faite deux fois, ou jamais. Dans le catalogue,
une tâche de boîtes exécutée deux fois émet ses boules deux fois : c'est le `census_mismatch` observé. Le défaut est
dans le pool d'origine, il précède J1 et J2. Des travaux plus courts rendent seulement la fenêtre plus fréquente.

## Correction

`src/sched/pool.{hpp,cpp}` : chaque appel de `parallel_for` a son propre descripteur de travail (fonction, taille,
grain, compteur de tranches, compteur d'utilisateurs).
- Un ouvrier ne le lit qu'après l'avoir capturé sous le verrou.
- L'appelant ferme la capture (`current_ = nullptr`) avant d'attendre que les ouvriers qui l'ont pris l'aient rendu.
- Un ouvrier en retard ne voit donc jamais les champs d'un autre travail.

La répartition des tranches, et donc le coût, ne changent pas.

## Contrôles

- **Nouvelle porte unitaire** `pool travaux courts enchaînés` (`tests/unit/unit_main.cpp`, dans `mhgp10_unit`,
  label `fast`). 50 000 travaux enchaînés à 4 puis 8 fils, alternant 1 et 64 indices, chacun avec sa propre fonction
  et ses propres compteurs ; chaque indice doit être exécuté exactement une fois, par la fonction de son travail.
  - Ancien pool : échec 6 fois sur 6, avec 2 à 6 cases fausses par exécution ([`unit_avant.txt`](unit_avant.txt)).
  - Pool corrigé : succès 6 fois sur 6.
- **ThreadSanitizer**, pool corrigé, aucun avertissement :
  - `mhgp10_unit`, porte de stress comprise ;
  - le cas de 18 points, 100 exécutions à 2, 3, 4 et 8 fils ;
  - la chaîne complète `mhgp10_cluster` (catalogue, tour, tête) sur le quart 01 de la trame LiDAR, à 4 et 8 fils.
- **Stress du cas de 18 points**, pool corrigé : 0 refus sur 2 400 ([`stress_pool_corrige.txt`](stress_pool_corrige.txt)).
- **Portes** : 9 sur 9 ([`ctest_gate.txt`](ctest_gate.txt)). Elles ont tourné sur l'arbre qui porte aussi
  l'accélération de la tête (`receipts/head_point_dendrogram_20260929`).
- **Différentiel du catalogue** contre `568d45297` sur les 10 entrées, à 1 et 4 fils : 10 sur 10 identiques
  ([`differentiel_catalogue.txt`](differentiel_catalogue.txt)).

## Portée sur les résultats antérieurs

- **Tests préenregistrés** (lots A et C, `bench/synthetic/run_test.py`), étude d'affectation
  (`bench/synthetic/alloc_dev.py`) et `methods.py` par défaut : les binaires y sont appelés avec `--threads=1`. Un
  pool à un fil n'a pas d'ouvrier : ces résultats ne peuvent pas avoir été touchés.
- **Campagnes dev lancées à plusieurs fils** (`run_campaign.py`, `adaptive_dev.py` avec `--threads 2`) : elles ont pu
  l'être rarement, de l'ordre d'une fois pour 10^4 à 10^5 appels de `parallel_for`. Un refus y compte pour ARI_s = 0
  (D8) ; une sortie fausse silencieuse n'est pas exclue. Elles ne servent qu'à choisir des configurations.
- **Mesures G4** : les temps restent valides. Les nombres d'amas des trames (66, 45, 46) sont identiques d'une session
  à l'autre.
- **Différentiels J1, J2 et assemblage** (1, 3 et 4 fils) : tous identiques. La course ne s'y est pas manifestée.
