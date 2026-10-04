# Bouts de scène SemanticKITTI : hiérarchie de points HGP contre hiérarchie HDBSCAN, session G4 du 4 octobre 2026

Reçu du dossier [`Zoltan/demos/bouts_hgp/`](../../../../Zoltan/demos/bouts_hgp/README.md). Cadre :
`exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`. **GCP utilisé** : session
gardée `claudebouts1` (`gcp-migration/v11_session.py`), commit poussé f1a53fe1c (`pushed_commit`), VM `g4-standard-48`,
Python épinglé (scikit-learn 1.7.2) ; arrêt ciblé certifié, état `TERMINATED` relu.

```sh
python3 -B morsehgp3D_v11/receipts/developpement_20261004/bouts_g4/check.py
```

Demande de l'utilisateur : des bouts de scène SemanticKITTI difficiles, réduits à deux ou trois voitures, deux ou trois
vélos, ou des vélos et des piétons, sans sol et sans autre point que ceux des objets, où la hiérarchie HDBSCAN échoue
et la hiérarchie HGP réussit ; les hiérarchies ne sont calculées que sur ces bouts.

| Étape | Commande du plan | Résultat |
| --- | --- | --- |
| déballage | `bench/points_unpack.py` (archive unique : la session limite les fichiers de données à 512) | 721 fichiers, 360 scènes, 0 écart d'empreinte |
| porte | `bench/points_gate.py --seconds 300` | conforme : 2 835 nuages, 12 fixtures sur 12, 4 mutants tués sur 4 |
| bouts | `bench/points_campaign.py --mode lidar --roles bout` | 360 scènes « ok », 112 s |

Entrées : [`bouts_lot1.json`](bouts_lot1.json) décrit les 360 bouts (trame, empreintes de la trame et des étiquettes,
instances, écarts, empreintes des fichiers du bout) ; aucune coordonnée. Ils viennent de deux passes de
`Zoltan/demos/tools/chercher_bouts.py` : séquences 00 à 10, une trame sur 10 (304 bouts), puis vélos et piétons, une
trame sur 2, écart jusqu'à 1 m (68 bouts, dont 12 déjà présents).

Comptes (critères écrits dans `Zoltan/demos/tools/choisir_bouts.py` avant la lecture des résultats ; recalculés par le
lecteur) :

| Famille | Bouts | HDBSCAN échoue | HGP réussit où HDBSCAN échoue | dont à tous les ordres | HGP échoue où HDBSCAN réussit |
| --- | --- | --- | --- | --- | --- |
| voitures | 263 | 0 | 0 | 0 | 0 |
| vélos | 81 | 23 | 11 | 2 | 5 |
| vélos et piétons | 16 | 4 | 2 | 1 | 0 |

Ces mesures portent sur le meilleur groupe de chaque hiérarchie pour chaque objet, pas sur un découpage automatique.
