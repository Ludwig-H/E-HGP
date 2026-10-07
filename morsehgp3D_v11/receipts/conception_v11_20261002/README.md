# Conception de la v11, brouillon mathématique et audit de la v10, jusqu'ici hors dépôt

7 octobre 2026. Cadre : `exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`.
Versés par l'[audit géant de la v11](../../docs/AUDIT_GEANT_V11.md) (§ 7.5), pour que ses constats reposent sur des
pièces du dépôt et non sur `build/`. **GCP non utilisé.**

## Pourquoi ces pièces

Ce sont les seules copies :
- de la **conception d'origine du moteur v11** (2 octobre 2026, avant toute ligne de moteur), dont l'implantation s'est
  ensuite écartée sans que l'écart soit documenté ;
- du **brouillon mathématique** que condense `docs/MATHEMATIQUES.md` (dont la proposition du modèle par copies des
  multiplicités) ;
- de l'**audit de la v10** en lentilles (rapports L01 à L08 et L10, preuves L01 à L14), dont `docs/AUDIT_V10_SYNTHESE.md`
  ne donne que l'inventaire ; il contient les seules mesures par segment des descentes de la v10 (L06) et les
  ablations du catalogue v10 (L05 : partition T et taille de feuille M).

## Avertissement

Ce sont des **notes de travail d'agents**, écrites le 2 octobre 2026 (et le 3 pour quelques pièces mathématiques).
- Leurs estimations de temps **n'ont jamais été mesurées** : notamment 18 à 30 ms de catalogue et 22 à 32 ms de tour à
  K5 sur G4 (`CONCEPTION_GENERATEUR.md` § 0, `CONCEPTION_TOUR.md` § 0). Elles valent comme hypothèses à mesurer par
  microbancs, pas comme résultats.
- « Prouvé » y signifie qu'un argument est écrit par l'auteur, à contre-lire ; cela ne qualifie aucun code.
- Les chiffres de la v10 qu'elles citent sont soumis à `morsehgp3D_v10/receipts/ERRATA.md`.
- Les scripts ne se relancent pas tels quels : ils codent en dur des chemins de travail (`build/v11-persist/`,
  `/workspaces/...`) et certains lisent des données hors dépôt.
- L'état final fait foi ailleurs : [passation](../../PASSATION.md), [audit final](../../docs/AUDIT_FINAL_V11.md),
  [audit géant](../../docs/AUDIT_GEANT_V11.md).

## Contenu

| Dossier | Pièces | Empreinte des pièces épinglées par `docs/AUDIT_V10_SYNTHESE.md` |
| --- | --- | --- |
| `conception/` | `CONCEPTION_TOUR.md`, `CONCEPTION_GENERATEUR.md`, `A_CONTRE_LIRE_TOUR_20261002.md`, `A_LIRE_AUDITEUR_VERROUS_20261002.md` ; contrôles `preuves_tour/` (noyau de forêt sans lots, contraction, historique d'attache, quotient des coquilles), `preuves_generateur/`, `preuves_pistes_de_rupture/` | `CONCEPTION_TOUR.md` `01e217f0…`, `CONCEPTION_GENERATEUR.md` `0fc30de4…` |
| `mathematiques/` | brouillon assemblé `brouillon/MATHEMATIQUES.assemble.md` et ses six parties ; pièces de vérification `pieces/` | `MATHEMATIQUES.assemble.md` `f829827d…` |
| `audit_v10/` | rapports `L01`–`L08` et `L10` ; preuves `preuves_l01`–`preuves_l14` (les rapports L09 et L11–L16 n'ont jamais été écrits) | `L02_MATH_TOUR.md` `ecb3231a…`, `L03_MATH_POINTS.md` `b84c5088…` |

Le troisième document de conception, `PISTES_DE_RUPTURE.md` (`e093f292…`), est déjà versé dans
[`../notes_hors_depot_20261007/conception/`](../notes_hors_depot_20261007/README.md) ; il n'est pas recopié ici.

## Revue avant versement

- **Exclus** : trois binaires ELF reconstructibles depuis leurs sources (`conception/preuves_generateur/proto/cat_sonde`,
  `replis_filtres`, `generator_sonde.o`).
- **Données** : aucune coordonnée LiDAR. Les deux fichiers de points sont des fixtures synthétiques de 13 et 14 points
  (`audit_v10/preuves_l13_raccord_r2/sphere13_u18.u32le`, `audit_v10/preuves_l01_math_catalogue/amas_coins_14.u32le`) ;
  l'archive `audit_v10/preuves_l09_perf_lidar/sorties/sorties_brutes.tar.gz` ne contient que des sorties de chronométrage
  et de compteurs ; les deux `jsonl.gz` de L07 ne contiennent que des métriques de scènes synthétiques.
- **Identité et secrets** : aucun chemin personnel (seulement `/home/codespace`), aucune clé ; les seules adresses
  électroniques sont celles, déjà publiques dans l'historique, de deux en-têtes de patchs (`users.noreply.github.com`,
  `noreply@anthropic.com`).
- Aucun fichier de plus de 1 Mio ; 576 fichiers versés (5,6 Mo), plus ce README et `SHA256SUMS`. Les journaux
  `*.log`, exclus par le `.gitignore` racine, sont versés explicitement, comme dans les autres reçus.

## Intégrité

`SHA256SUMS` couvre tous les fichiers de ce dossier, sauf lui-même :

```sh
cd morsehgp3D_v11/receipts/conception_v11_20261002 && sha256sum -c SHA256SUMS
```
