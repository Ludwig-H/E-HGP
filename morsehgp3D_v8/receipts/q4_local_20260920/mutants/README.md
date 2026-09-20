# Mutations compilées de la partition et du balayage local q4

20 septembre 2026. L'autorité corrigée est la capture
[compiled__sv8d6az](compiled__sv8d6az/MANIFEST.json), liée au build neuf
`build/v8_q4_local_r2_20260920` et à ses 177 sources épinglées.
Sa [fermeture](compiled__sv8d6az/COMPLETION.json) est `passed`.

## Premier essai conservé : une lacune du test

La capture [compiled_0_udmiyg](compiled_0_udmiyg/MANIFEST.json) reste en
échec. Sa gate r1 passait 11 954 contrôles et tuait le retrait des tangences,
mais la mutation supprimant la contribution intérieure d'un événement
clippé survivait. Le troisième mutant n'avait pas encore été exécuté.
Les événements extérieurs étaient exercés, sans contre-fixture garantissant
qu'une contribution intérieure oubliée modifie une profondeur publiée.

Le moteur n'a pas été corrigé pour faire passer ce test : seule la gate et
son lecteur ont été renforcés, puis reconstruits dans de nouveaux builds.
La [gate r1 archivée](../historical_r1/q4_local_gate.cpp) conserve exactement
le hash `d76e0d04e31c043d5759fa7aeddb682a852331bd74da48d56bcf014a07a4dd6f`,
identique au manifeste du premier essai et à `smoke_k74j91m0`.

La nouvelle contre-fixture a les sites `(100,100,100)`, `(120,120,100)`,
`(100,120,80)`, `(120,100,80)` et `(105,110,96)`. La boule régulière
portée par les quatre premiers a pour centre `(110,110,90)`, rayon carré300,
et contient strictement le dernier site : profondeur1. Pour la seed2,
la racine de ce dernier est à ξ=−239/20, hors du carré racine, tandis
que sa forme change de signe sur le carré. Elle demeure donc active dans
la partition, puis devient une constante intérieure lors du clipping.
Oublier cette constante donne une profondeur publiée erronée.

## Capture corrigée

La gate r2 non modifiée passe d'abord 11 981 contrôles, dont75 contributions
intérieures clippées. Chaque altération est ensuite appliquée à une copie
temporaire de la source et compilée devant la bibliothèque originale.
Les produits, gate et artefacts de référence ne sont jamais remplacés.

| Mutation | Défaut introduit | Réponse de la gate r2 |
|---|---|---|
| `tangent_active_site_removed` | Remplacer le retrait strict min>0 par min≥0 | Candidat, profondeur ou coquille différent de l'oracle rationnel |
| `clipped_inside_contribution_lost` | Supprimer les deux incréments inside/base d'un événement extérieur intérieur | Même échec de l'oracle rationnel, sans provoquer artificiellement le garde final de population |
| `closed_cell_emits_without_ownership` | Émettre dans toutes les cellules fermées au lieu de la seule cellule propriétaire | Même échec de l'oracle rationnel, par émission frontalière incorrecte |

Les trois mutants r2 sortent avec code1 sur une comparaison des résultats,
ni une erreur de compilation ni un plancher de non-vacuité. Les dix commandes,
leurs sorties brutes, différences sources et hashes sont conservés.

Les quatre lecteurs normal et `-O`, avec et sans `--check-live`, passent.
[MUTANTS_READBACK.json](MUTANTS_READBACK.json) conserve les commandes,
sorties et codes ainsi que les 210 hashes avant/après identiques : sources,
référence compilée, compilateur, helper, reçus et objets/binaires mutants.
Ces relectures qualifient r2 seulement, jamais le premier essai en échec.

```sh
python3 -B morsehgp3D_v8/receipts/q4_local_20260920/mutants/run_mutants.py read morsehgp3D_v8/receipts/q4_local_20260920/mutants/compiled__sv8d6az --check-live
python3 -B -O morsehgp3D_v8/receipts/q4_local_20260920/mutants/run_mutants.py read morsehgp3D_v8/receipts/q4_local_20260920/mutants/compiled__sv8d6az --check-live
```

Il s'agit de trois altérations causales jugées par une même gate, pas de
trois oracles indépendants ni d'une preuve exhaustive d'absence de défaut.
Aucun gain de performance ni contrat FULL/G4 n'en découle ; GCP non utilisé,
`public_status=not_claimed`. Les preuves différentielles restent distinctes.

Le [différentiel final27→28 r2](DEFAULT_DIFFERENTIAL_R2.md) est documenté
séparément ; ses captures et relectures ne sont pas celles des mutants.
