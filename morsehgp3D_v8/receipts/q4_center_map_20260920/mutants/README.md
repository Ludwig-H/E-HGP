# Trois mutations compilées de la carte q4

20 septembre 2026. Capture [compiled_yonakzn0](compiled_yonakzn0/MANIFEST.json),
avec les 170 sources de la tranche27 épinglées par leurs propres hashes.
La [fermeture](compiled_yonakzn0/COMPLETION.json) est `passed`.

La gate non modifiée passe d'abord ses 1 956 contrôles. Chaque mutation
est ensuite appliquée à une copie temporaire de la source concernée,
compilée et liée devant la bibliothèque originale. Ni le produit, ni la
gate, ni les artefacts de référence ne sont modifiés. Les dix commandes
sont conservées avec leurs sorties, codes de retour et hashes.

| Mutation | Défaut introduit | Réponse causale de la gate |
|---|---|---|
| `line_miss_cached_as_outside` | Transformer une cellule manquée par une seule droite en exclusion globale persistante | Une racine positive propriétaire est rejetée malgré une profondeur rationnelle du pool insuffisante |
| `boundary_witness_credited` | Créditer un témoin dont la puissance maximale est nulle | Même contradiction avec la profondeur rationnelle stricte du pool |
| `obtuse_completion_omitted` | Supprimer de la lentille les complétions non aiguës | Population du domaine différente du recensement rationnel de la lentille fermée |

Les trois mutations sont tuées au premier essai par une comparaison
mathématique de la gate, avec code de sortie 1. Ce ne sont ni des erreurs
de compilation ni de simples échecs de planchers de non-vacuité.
Les différences sources, objets/binaires temporaires et leurs hashes
sont référencés dans la fermeture.

Les quatre lectures Python normal et `-O`, avec et sans `--check-live`,
passent. Le [reçu de relecture](MUTANTS_READBACK.json) conserve les quatre
commandes, sorties et codes de retour ainsi que les hashes avant/après
de 203 entrées : sources, référence compilée, compilateur, helper,
preuves enregistrées et objets/binaires mutants. Ces entrées sont inchangées.

```sh
python3 -B morsehgp3D_v8/receipts/q4_center_map_20260920/mutants/run_mutants.py read morsehgp3D_v8/receipts/q4_center_map_20260920/mutants/compiled_yonakzn0 --check-live
python3 -B -O morsehgp3D_v8/receipts/q4_center_map_20260920/mutants/run_mutants.py read morsehgp3D_v8/receipts/q4_center_map_20260920/mutants/compiled_yonakzn0 --check-live
```

Il s'agit de trois altérations causales jugées par une même gate, pas de
trois oracles indépendants ni d'une preuve exhaustive d'absence de défaut.
Aucun temps de performance ni contrat FULL/G4 n'en découle ; GCP non utilisé,
`public_status=not_claimed`. Les reçus différentiels sont distincts.
