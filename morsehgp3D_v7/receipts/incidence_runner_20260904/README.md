# Transition du lecteur de refus, 4 septembre 2026

Ce dossier conserve les octets du lecteur précédent et une nouvelle observation
du lecteur corrigé. Aucun ancien reçu, notamment `incidence_mini_20260904` et
`incidence_local_20260904`, n'est modifié ni reclassé rétroactivement.

| Fichier d'origine | SHA-256 avant | SHA-256 après |
| --- | --- | --- |
| `bench/incidence_campaign.py` | `0b154161e9178fb7f279977fb300bc60b4e2478c99cfbfef531f5a74722b5d89` | `6ca21d8b1c89e6baea99ecc3dd414b35d06581df50833e76bac9bdc1d5d1c20a` |
| `tests/incidence_campaign_gate.py` | `8cf34a3ae30c3b403ff0bb4624de447fd68cec98578551d870b7b34e5179ab2a` | `d74bca5257bd307b5956a946ca188a217f519ec2dc49e6b0de3ebf33a0ccca0a` |

Les copies `incidence_campaign.before.py` et
`incidence_campaign_gate.before.py` ont exactement les hashes « avant ».
Ce sont des artefacts de source historiques, pas de nouveaux points d'entrée :
les chemins relatifs de leur implantation originale doivent être restaurés
pour rejouer l'ancien programme. Leur dépendance `bench/compare_v6_v7.py` est
inchangée, SHA-256
`cfe40fd2b00508ec7887970b961482a87839afdf9a7e2539728855feec69ffcb`.

Le produit, le CMake, les oracles et le banc comparatif restent inchangés par
cette transition. Aucun modèle mathématique ni limite produit n'est modifié.

## Delta du lecteur

Le moteur émet réellement `REFUS silent incidence K=N : silent_...`, sans
préfixe de statut. Le précédent lecteur local ne reconnaissait que les refus
commençant par `unsupported_degeneracy` ou `resource_exhausted` ; un cap normal
de complétion devenait donc une observation `invalid`, conservée comme telle.

Le nouveau lecteur reconnaît exactement ces sept motifs de ressource :

- `silent_core_record_budget` ;
- `silent_chain_step_budget` ;
- `silent_added_coface_budget` ;
- `silent_query_node_budget` ;
- `silent_meb_support_budget` ;
- `silent_direct_catalogue_budget` ;
- `silent_allocation_failure`.

Il reconnaît exactement trois motifs `unsupported_degeneracy` :
`silent_local_nonessential_shell`, `silent_external_shell` et
`silent_nonregular_direct_catalogue`. Ces correspondances proviennent des
branches explicites de `src/forest/silent_incidence.hpp`, relayées par
`src/pipeline/run.hpp` à l'étage fold. Elles ne sont pas inférées d'un suffixe
`budget` ou du seul code de sortie 2.

Les ordres acceptés sont K2 à K10. L'étage doit être `fold` pour cette forme de
refus, les diagnostics par ordre doivent être strictement croissants et ne
peuvent annoncer un ordre postérieur à celui de l'échec. Le stdout doit rester
vide ; les lignes d'étage et de travail ainsi que tous les diagnostics restent
obligatoires et contrôlés. Un refus précoce peut légitimement ne pas avoir de
ligne de compteurs par ordre. Les invariants, entrées invalides, motifs inconnus
et sorties mal formées restent invalides ; aucune règle générique `silent_*`
n'est ajoutée. Les messages déjà typés du pipeline conservent leur catégorie
historique et doivent désormais respecter la syntaxe complète du préfixe.

Les enregistrements reconnus ajoutent `refusal_status` et `refusal_order` tout
en conservant intégralement `reason`. `engine_refused` n'est pas
`engine_completed`. Les censures restent séparées, même si une sortie partielle
ressemble à un refus. `parse_completion`, les critères de réussite et les
compteurs de la synthèse ne changent pas.

## Portes et observation réelle

```bash
python3 -B morsehgp3D_v7/tests/incidence_campaign_gate.py
python3 -B -O morsehgp3D_v7/tests/incidence_campaign_gate.py
```

Résultat : 7 tests réussis dans chaque mode. Les nouvelles fixtures couvrent
les 20 couples motif/ordre pour K2 et K10, 37 variantes négatives du nouveau
chemin, le refus précoce sans compteurs, la séparation censure/refus et la
conservation des tentatives invalides par le runner complet. L'intégration
CMake est préparée séparément dans le correctif d'archive, non appliquée ici.

Une exécution réelle du binaire gelé est conservée dans `real_cap/` :

```bash
python3 -B morsehgp3D_v7/bench/incidence_campaign.py --binary build/v7/mhgp7 --output morsehgp3D_v7/receipts/incidence_runner_20260904/real_cap --sizes 11 --families uniform --coords 65536 --seed 3 --threads 2 --timeout 10 --meb-supports 1
```

Résultat moteur : code 2, `silent_meb_support_budget` à K2. Résultat du runner :
code 0, `observations_completed`, **zéro succès moteur**, un refus reconnu,
zéro censure, `source_stable=true`, `public_status=not_claimed`. Cette observation
teste la classification d'un cap ; elle ne valide ni une sortie exacte ni une
capacité de traitement. GCP non utilisé par cette sous-tâche.
