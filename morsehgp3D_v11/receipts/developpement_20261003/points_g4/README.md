# Hiérarchie de points depuis FULL contre HDBSCAN : sessions G4 du 3 octobre 2026

Reçu de [HIERARCHIE_POINTS.md](../../../docs/HIERARCHIE_POINTS.md) (§ 7). Cadre :
`exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`.
**GCP utilisé** : sessions gardées `gcp-migration/v11_session.py`, VM `g4-standard-48` (AMD EPYC 9B45), Python
épinglé (`numpy` 2.2.6, `scipy` 1.15.3, `scikit-learn` 1.7.2) ; arrêt ciblé certifié et état `TERMINATED` relu
après chaque session. Les sessions A à D sont des instantanés de l'arbre de travail (`dev_snapshot`) : essais de
développement, jamais une preuve publiable. E et F partent de commits poussés (`pushed_commit`).

```sh
python3 -B morsehgp3D_v11/receipts/developpement_20261003/points_g4/check.py
```

Le lecteur vérifie les empreintes, les arrêts, le verdict de chaque porte et l'état de chaque scène ; il recalcule
les tableaux **session par session** à partir des archives par [`bench/points_summary.py`](../../../bench/points_summary.py),
sépare le synthétique par taille de nuage et vérifie que C et D, puis D et F, publient des scènes identiques hors
chronométrage. Une étape coupée par l'échéance de la VM est déclarée, pas refusée ; l'échec de E doit être
exactement celui qui est déclaré (codes de sortie et trace).

## Protocole

| Pièce | Rôle |
| --- | --- |
| [`bench/points_export.cpp`](../../../bench/points_export.cpp) | FULL 1..kmax (voie qualifiée 16379), niveaux exacts, forêts des ordres demandés, entrées core, incidences fortes P3 par site |
| [`bench/points_radius.py`](../../../bench/points_radius.py) | règle retenue $H^{r}_{k+1}$ (`margin_r`), dates $\sqrt{t}+\sqrt{m}-\sqrt{q}$ décidées exactement |
| [`bench/points_hierarchy.py`](../../../bench/points_hierarchy.py) | règles témoins core, cover, first, $Q_1\circ\Pi_1$ et $Q_1\circ\Pi_{k+1}$ (marge en niveau carré) en rationnels exacts ; évaluateur commun aux plateaux fermés ; arbre `sklearn` |
| [`bench/points_reference.py`](../../../bench/points_reference.py) | oracle des règles par force brute sur les coupes de $\Gamma_k$ (étage A de `reference/hgp11_ref`) |
| [`bench/points_gate.py`](../../../bench/points_gate.py) | porte : export natif + règles contre l'oracle, PointId permutés, fixtures gravées, mutants causaux |
| [`bench/points_campaign.py`](../../../bench/points_campaign.py) | campagne ; refuse de tourner sans porte conforme du même exportateur |
| [`bench/points_lidar_prepare.py`](../../../bench/points_lidar_prepare.py) | préparation sur la VM des trames voisines du criblage (sonde de sol v8 recompilée, garde de rejeu) |

Domaine : séquence 08, grille 1 mm, sol retiré sauf dans la démo 04, de 32 462 à 126 267 sites (voisines de
36 752 à 81 688). La voisine 000882 est la démo 02 : le lecteur la compte comme démo.
Données : coordonnées dérivées de SemanticKITTI (CC BY-NC-SA), jamais versionnées ; seuls les manifestes
(`data_manifest_*.json` : noms, rôles, effectifs, empreintes) sont gardés ici. Les archives de résultats ne
contiennent que des scores, les étiquettes `sem | inst << 16` des objets et, pour les blocs rattrapés, des indices de
sites ; aucune coordonnée. Scènes synthétiques régénérées sur la VM par le générateur v10 épinglé
(`receipts/full_points_20261003/experiment/vendor_scenes.py`).

## Sessions

| Session | Source | Règle de la tour mesurée | Données | Étapes |
| --- | --- | --- | --- | --- |
| A `claudepts1` | instantané e26b48055 + arbre de travail | marge en niveau carré (consommateur 5df63b60, relevé par l'auditeur) | 128 synthétiques ; 5 démos et 39 trames du criblage (`data_manifest_pts1.json`) | porte 4 602 nuages, 261 335 ultramétriques exactes, 0 désaccord, fixtures 4/4 ; tout « ok » |
| B `claudepts2` | idem | idem | 72 trames voisines préparées sur la VM, 20 témoins (`data_manifest_pts2*.json`) | préparation (sonde de sol conforme, 0 écart de rejeu), porte 2 889 nuages ; tout « ok » |
| C `claudepts3` | idem | `margin_r` f023f6d0 (filtre flottant défectueux, relevé par l'auditeur) | 128 synthétiques, 64 scènes, 72 voisines (`data_manifest_pts3.json`) | porte 2 495 nuages ; `lidar-b` coupée par l'échéance (66 voisines persistées sur 72) |
| D `claudepts4` | idem | `margin_r` 58952a8b (arithmétique corrigée) | mêmes données que C | porte 2 478 nuages, 168 882 comparaisons, fixtures 7/7 ; `lidar-b` coupée (68 voisines sur 72) |
| E `claudepts5` | commit 6c88fe0ed | `margin_r` publiée, porte stricte | mêmes données que C | porte arrêtée après 300 s : `FileNotFoundError`, dossier d'export des mutants non créé ; campagnes refusées (code 2) |
| F `claudepts6` | commit f02f91c7e (correctif de E) | idem | démos, 72 voisines, synthétique | **porte stricte conforme** : 2 854 nuages, 194 520 comparaisons, 215 974 sites comparés exactement, fixtures 12/12, mutants 4/4 ; tout « ok »

**C contre D.** Les 258 scènes communes sont identiques hors chronométrage : le correctif arithmétique (classes de
carrés, refus, filtres en erreur absolue) ne change aucun observable publié (meilleurs IoU à six décimales, nombres
de blocs, de nœuds et d'incidences, membres des blocs rattrapés) ; aucun dump canonique des dates et propriétaires
internes ne l'établit pour toutes les décisions. La version publiée de
`points_radius.py` ne diffère de 58952a8b que par sa docstring
([`points_radius_58952a8b_vers_publie.diff`](points_radius_58952a8b_vers_publie.diff)).

**D contre F.** Les 201 scènes communes (128 synthétiques, démos, voisines) sont identiques hors chronométrage : le
commit poussé reproduit l'instantané de développement, et les mesures de D sont celles du code publié.

**Portes.** Les portes de A à D comparaient des ultramétriques en flottant pour la règle en rayon et sommaient
l'oracle décimal à 28 chiffres : insuffisant aux égalités exactes (auditeur). La porte publiée compare dates et
propriétaires exactement, site par site, avec un oracle entièrement exact, des nuages à égalités exactes (dont les
témoins de l'auditeur) et quatre mutants causaux ; elle est qualifiée sur G4 par la session F.

Tableaux : § 7 de la note, recalculés par le lecteur.
