# Qualification G4 du levier V3, des leviers de constante et des correctifs des auditeurs

6 octobre 2026. Cadre : `exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`.
Deux sessions gardées, cible `us-central1-c / ehgp-v7-3b1d496aed430749ea7e049f`, arrêt `TERMINATED` certifié après
chacune (`receipt.json`). Source : `38faaf272`. Elle contient V3 (`a841d8c4b`), les leviers de constante du pas de
descente (`751686868`), le placement adopté dans l'API (`12ce8f8f0`) et quatre correctifs d'auditeurs (mutants
d'étendue au profil u21, cache des variantes de `gpu_ab` indexé par l'archive, juge GPU sans registre absent, IoU non
arrondie avant le seuil strict). Données : `data_complet`, soit les trois trames LiDAR réelles et les trois nuages
uniformes des portes d'échelle, hors dépôt. Aucune qualification ne promeut un statut public.

## Résultat

| Session | Contenu | Résultat |
| --- | --- | --- |
| `claudev3q1` | mutants de la garde des arbres de points (`b0f2a0a9e`), mutants d'étendue du levier C en u21 | **4 tués sur 4** : `vie_du_bloc_sans_borne`, `vie_du_bloc_egalite`, `etendue_seuil_double`, `etendue_sans_fermeture` |
| `claudev3q1` | portes ordinaires : Release u18, u21 et u24, ASan+UBSan u24 | **conformes** : 890/890, 800/800, 800/800, 800/800 |
| `claudev3q2` | portes d'échelle (8 000, 16 000, 32 000) et LiDAR, Release aux trois profils, deux lots chacun | **conformes** : 52/52 et 66/66 dans chacun des trois profils |

Les portes LiDAR de `claudev3q2` comparent les sorties des trois trames réelles à leurs empreintes ; elles passent
aux trois profils avec l'arbre radix et la borne entière. Cela qualifie l'invariance des sorties, que les compteurs
déterministes et les bancs A/B établissaient déjà pour u21.

**Hors périmètre.** TSan u21 n'est pas rejoué ici ; il l'est dans la session `claudeo2a`, avec le publieur. Il en
va de même des portes `long`, de la campagne complète des mutants et des différentiels Python S9 et S10. Les reçus de
la qualification finale du 5 octobre restent valables pour le code qu'ils couvrent et ne se transfèrent pas à ces
changements au-delà de ce qui est rejoué ici.

## Pièces

Par session : `plan.json`, `launch.json`, `receipt.json`, `matrix_summary.json` et `result_<configuration>.json`
(sortie de `tools/g4_matrix.py`) ; pour `claudev3q1`, `mut_head.*` et `mut_catalogue.*`. Aucune coordonnée LiDAR :
seuls les noms et les empreintes des fichiers d'entrée. `SHA256SUMS` couvre tous les fichiers sauf lui-même.
