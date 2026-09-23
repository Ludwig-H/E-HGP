# Addendum : revalidation du reçu de croissance LiDAR locale

23 septembre 2026. **GCP non utilisé**, aucun nouveau calcul HGP. Cet addendum
rejuge le reçu [`lidar_scaling_local_20260923`](../lidar_scaling_local_20260923/README.md)
(inchangé) avec le lecteur durci du runner, en réponse au contre-audit B (points
ouverts du runner v2 : identité d'entrée non liée aux octets, IDs des morceaux
v8 recopiés sans relecture, champs lus hors du bloc de refus).

## Lecteur durci (`run_lidar_scaling.py`)

- `validate_probe` exige maintenant le jeu exact de clés d'entrée, `format =
  u32le`, la grille attendue, le **FNV-1a 64 recalculé sur les octets
  fournis** (même définition que la sonde et le worker G4), les fils de la
  tour statique attendus, puis le condensé, les temps, le CPU et le RSS avant
  toute écriture d'un succès. Une sortie de code 0 mal formée devient un
  `.failure.json` typé.
- Les nouveaux cas passent explicitement `--static=W --grid=1mm` à la sonde.
- Chaque morceau v8 est contrôlé sur ses points **et** son fichier d'IDs.
- `--selftest CASE.json` : un cas archivé sur un morceau v8 versionné est
  accepté et 25 altérations sont refusées. Porte CTest
  `mhgp9_lidar_scaling_reader_{normal,optimized}` (préfixe exact
  `lidar_scaling_selftest mutants_killed=25/25`), verte localement.
- `--revalidate OUT_DIR` : reconstruit chaque entrée (emboîtés depuis la
  trame v8, morceaux relus contre le MANIFEST), compare la provenance
  archivée, recalcule le FNV, rejuge la sortie et la ligne du résumé.

## Résultat

`REVALIDATION.json` : **60 cas sur 6 campagnes, aucun échec** (provenance,
FNV, sortie de sonde et lignes de résumé identiques). Les cas archivés ont
été lancés sans `--grid` ni `--static` : ils sont jugés contre les défauts
de la sonde (`unspecified`, fils statiques = W), tirés de leur ligne de
commande.

**Erratum du reçu d'origine** : il compte « 66 cas » ; ce sont **60 cas**
(dix par campagne) et 66 JSON (plus six résumés). Les tableaux et
exposants sont inchangés.
