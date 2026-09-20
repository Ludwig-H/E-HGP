# Trois mutations compilées du certificat collectif q3/q4

20 septembre 2026. Capture [compiled_n8onzuz3](compiled_n8onzuz3/MANIFEST.json),
HEAD observé `8d0a0f0f7c6ab4319cf64d2b0113fe4733e2f4eb`, avec les
163 sources de la tranche26 épinglées par leurs hashes propres.
La [fermeture](compiled_n8onzuz3/COMPLETION.json) est `passed`.

La gate non modifiée passe d'abord ses 8 549 contrôles. Chaque mutation
est ensuite appliquée à une copie temporaire de `q34_collective.cpp`,
compilée et liée devant la bibliothèque originale. Ni le produit, ni la
gate, ni les artefacts de référence ne sont modifiés. Les dix commandes
prévues sont conservées avec leurs sorties, codes de retour et hashes.

| Mutation | Défaut introduit | Réponse causale de la gate |
|---|---|---|
| `closed_right_exit_dropped` | Supprimer la racine d'une sortie exactement à la borne droite | Rejet différent du minimum rationnel strict du pool |
| `mixed_group_shell_credited` | Mesurer le groupe avant de retirer ses sorties | Rejet différent du minimum rationnel strict du pool |
| `variance_quotient_truncated` | Tronquer le quotient de la borne Variance au lieu de l'arrondir vers l'extérieur | Borne différente de l'arrondi rationnel indépendant |

Les trois mutations sont tuées au premier essai par une comparaison
mathématique de la gate, avec code de sortie 1. Ce ne sont ni des erreurs
de compilation ni de simples échecs de planchers de non-vacuité.
Les différences sources, objets/binaires temporaires et leurs hashes
sont référencés par la fermeture.

Les lecteurs Python normal et `-O` passent tous deux, avec et sans
`--check-live` : quatre lectures concordantes. Le contrôle vivant vérifie
aussi les 163 sources, le compilateur, les artefacts de référence et les
objets/binaires mutants temporaires. Exemple de relecture depuis la racine :

```sh
python3 -B morsehgp3D_v8/receipts/q34_collective_20260920/mutants/run_mutants.py read morsehgp3D_v8/receipts/q34_collective_20260920/mutants/compiled_n8onzuz3 --check-live
python3 -B -O morsehgp3D_v8/receipts/q34_collective_20260920/mutants/run_mutants.py read morsehgp3D_v8/receipts/q34_collective_20260920/mutants/compiled_n8onzuz3 --check-live
```

Il s'agit de trois altérations causales jugées par une même gate, pas de
trois oracles indépendants ni d'une preuve exhaustive d'absence de défaut.
Aucun temps de performance ni contrat FULL/G4 n'en découle ; GCP non utilisé,
`public_status=not_claimed`.
