# Classification des observations d'incidences — état courant

Vérification indépendante du 4 septembre 2026, terminée à 21:30 UTC. Cadre : `exploration_v7_hors_registre` / `cpu_reference` / `quantized_u16_input_only` / `audit_independant_math_and_architecture` / `not_claimed`. Sources, copie examinée et binaire sont stables avant/après. [Reçu reproductible, commandes et sorties brutes](receipts_20260904/campaign_current.json).

**La correction de classification est vérifiée.** La campagne reconnaît désormais sept motifs précis de manque de ressources et trois motifs précis de dégénérescence sous la forme `REFUS silent incidence K=…`, pour K=2 à 10. Le vocabulaire est fermé : un motif inconnu ou un invariant violé reste `invalid`. Le code doit être 2, stdout vide et l'étage `fold` ; les diagnostics d'ordres doivent être strictement croissants et ne pas dépasser l'ordre terminal (`bench/incidence_campaign.py:19–48`, `137–168`).

Le refus est enregistré comme `engine_refused`, avec `refusal_status` et `refusal_order`. Une tentative refusée peut terminer l'**observation**, tout en conservant `engine_successes=0`. Un timeout reste `censored`. Ce changement ne transforme aucun refus en résultat mathématique ni en forêt publiée.

La copie figée de `tests/incidence_campaign_gate.py` passe ses **7 tests en Python normal et sous `python3 -O`**, code 0 dans les deux cas. La porte couvre notamment les 20 combinaisons positives des dix motifs avec K=2/K=10, les motifs inconnus ou d'invariant, les lignes malformées, l'ordre des diagnostics, une sortie partielle, les mauvais codes et la conservation des tentatives interrompues ou invalides. Les assertions sont celles de `unittest`, donc restent actives sous `-O`.

Une seule exécution réelle supplémentaire a été effectuée, sur 11 points :

```text
mhgp7 --family=uniform --n=11 --coord=65536 --seed=3 --threads=2 --smax=11 --layout=csr --digest --complete-incidences --mem-budget=17179869184 --silent-meb-supports=1
```

Elle retourne **2**, stdout vide, avec `REFUS silent incidence K=2 : silent_meb_support_budget`. Les compteurs indiquent `core=24`, `query_nodes=5`, `meb_supports=1`. Le classifieur courant produit `engine_refused/resource_exhausted`, ordre 2, en mode normal et optimisé. Le même stderr rejoué contre la source initiale de l'itération 2 reproduit `invalid: unexpected refusal cause` : la différence vérifiée porte bien sur la classification, sans modification du calcul moteur.

**C1 est levé sur le CMake courant.** Une configuration Release indépendante puis les deux CTests `mhgp7_incidence_campaign` et `mhgp7_incidence_campaign_optimized` passent, code 0 ; ils portent le label `gate` et un timeout de 120 s. Le [reçu d’enregistrement courant](receipts_20260904/campaign_registration_current.json) conserve commandes, inventaire et hashes avant/après. Aucune campagne volumineuse n'a été relancée et aucune qualité géométrique supplémentaire n'est revendiquée. GCP non utilisé.

| Élément épinglé | SHA-256 |
| --- | --- |
| `bench/incidence_campaign.py` | `6ca21d8b1c89e6baea99ecc3dd414b35d06581df50833e76bac9bdc1d5d1c20a` |
| `tests/incidence_campaign_gate.py` | `d74bca5257bd307b5956a946ca188a217f519ec2dc49e6b0de3ebf33a0ccca0a` |
| `bench/compare_v6_v7.py` | `cfe40fd2b00508ec7887970b961482a87839afdf9a7e2539728855feec69ffcb` |
| Binaire copié depuis `audits/.work_iteration2/build/mhgp7` | `8a99b5cf5dc2c0622947f6b34880803e4ed347608ea61d56f3e080137bfc4a6b` |

Le reçu distingue les hashes des sources et celui du binaire. Le [raccord de compilation indépendant](receipts_20260904/interfaces_build_binding.json) relie ce même binaire à la reconstruction Release figée.

Le helper `compare_v6_v7.py` a ensuite évolué pour les campagnes K=5/K=10 et les séparations WSPD. Les deux portes d’incidences repassent sur cette dépendance courante : [qualification du delta de lanceur](receipts_20260904/paired_runner_delta_current.json). Le reçu de refus réel ci-dessus conserve les octets de sa propre exécution.
