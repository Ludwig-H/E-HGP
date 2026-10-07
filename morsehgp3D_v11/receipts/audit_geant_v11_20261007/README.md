# Audit géant de la v11 : rapports bruts, scripts et vérification locale

7 octobre 2026. Cadre : `exploration_v11_hors_registre (close) / cpu_reference / quantized_u21_input_only /
not_claimed`. Reçu de l'[audit géant](../../docs/AUDIT_GEANT_V11.md). **GCP non utilisé.**

## Ce que contient ce reçu

| Dossier | Contenu |
| --- | --- |
| `rapports/` | les huit rapports bruts, tels que rendus par les lecteurs indépendants : A (thèse et objet), B (contrat et preuves), C (catalogue, index, GPU), D (tour et comparaison à la v10), E (sorties, HDBSCAN, applications), F (lignée v2 → v10), G (vérification par exécution locale), H (chiffres, qualification, mesure, canal d'audit) |
| `notes_F/` | les notes de lecture vérifiées par version (v2–v4, v5–v6, v7–v8, v9–v10) sur lesquelles s'appuie le rapport F |
| `scripts/A` | calculs exacts en `Fraction` sur au plus six sites : MEB, E5, Gabriel, deux triangles du § 6.1, masses du § 9.1 |
| `scripts/B` | rejeu de l'oracle borné et de ses mutants, témoins de la sortie SPv2 (translation, saut de $S^{*}$ sur le cercle), sorties |
| `scripts/D` | lecture des compteurs de l'ordre 10 et du chemin critique du pipeline |
| `scripts/E` | recomptes des sessions `points_g4` et des bouts de l'étude E1 (lecture seule) |
| `scripts/H` | statistiques en bibliothèque standard sur les rapports bruts des bancs : bruit, A/A, permutation, bootstrap, puissance, rejugement des leviers retirés ; sorties `*_out.*` |
| `verification_locale/` | journaux de configuration, de construction et de CTest ; enregistrements des empreintes (`empreintes/*.jsonl`) ; outils de la sonde ; manifestes et lignes d'état du CLI ; rejeu Python de l'oracle |

## Avertissements

- **Rapports bruts.** Ils sont versés sans réécriture. La synthèse, qui fait foi, est le document
  [`docs/AUDIT_GEANT_V11.md`](../../docs/AUDIT_GEANT_V11.md). Quelques comptes y sont arrondis ou reformulés après
  recoupement (par exemple le nombre d'affirmations confirmées par le lecteur H, qui diffère entre son fichier et son
  résumé final).
- **Chemins.** Les rapports citent le bloc-notes de la session (`/tmp/claude-1000/…/scratchpad/agent_*`), volatil : les
  pièces utiles en sont copiées ici. Les scripts codent ces chemins ; ils lisent le dépôt, les paquets de sessions G4
  hors dépôt (`/workspaces/.ehgp-sessions/`, pour H) ou les données hors dépôt (`build/v11-full-data-20261002/`,
  pour G).
- **Aucune donnée KITTI.** Les vidages FULL et les fichiers de sortie du CLI, qui contiennent des coordonnées dérivées
  des trames, ont été effacés ou laissés hors du dépôt ; seuls leurs SHA-256, leurs manifestes et leurs lignes d'état
  sont versés. Aucune identité de compte n'est recopiée.
- **Démos postérieures.** Le commit `050989d60` (7 octobre, 08 h 51 UTC, démos Zoltan) est postérieur à la lecture E :
  les vidéos de duel comptent désormais les classes 52 et 99 comme fond ; aucun gain ni aucune perte ne change, mais
  cinq doubles réussites deviennent des doubles échecs. Les comptes de vidéos du rapport E sont donc antérieurs à ce
  commit ; la synthèse ne les reprend pas.
- **Temps locaux.** Les durées de `verification_locale/` ont été prises sur le codespace, sous la charge des autres
  lecteurs (4 cœurs physiques) : elles ne décident rien.

## Rejouer la vérification locale

```bash
cmake -S morsehgp3D_v11 -B <b> -DCMAKE_BUILD_TYPE=Release
cmake --build <b> -j6
ctest --test-dir <b> -LE long --no-tests=error --output-on-failure -j4
MHGP11_DATA_DIR=build/v11-full-data-20261002 ctest --test-dir <b> -L lidar -LE long --no-tests=error --output-on-failure -j4
<b>/mhgp11_full_bench <cas>.u32le <cas>.ids.u32le <vidage> <K> <feuille> 256 0 4294967295 8589934592 8 <masque>
```

Les empreintes attendues sont au § 3.4 de [`docs/AUDIT_FINAL_V11.md`](../../docs/AUDIT_FINAL_V11.md) ; le
différentiel v10 exige `-DMHGP11_MODULES=reference -DMHGP11_V10_FROZEN_DIR=<binaires v10 figés>`.

## Intégrité

`SHA256SUMS` couvre tous les fichiers de ce dossier, sauf lui-même :

```sh
cd morsehgp3D_v11/receipts/audit_geant_v11_20261007 && sha256sum -c SHA256SUMS
```
