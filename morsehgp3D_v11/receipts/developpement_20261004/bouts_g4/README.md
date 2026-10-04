# Bouts de scène SemanticKITTI : hiérarchie de points HGP contre hiérarchie HDBSCAN, sessions G4 du 4 octobre 2026

Reçu de [`Zoltan/demos/`](../../../../Zoltan/demos/README.md), rangé par issue (HGP réussit ou échoue, HDBSCAN réussit
ou échoue) sur demande de l'utilisateur ; le premier dossier `Zoltan/demos/bouts_hgp/` (commit 0af635a71) est remplacé
par cette organisation, et ce reçu a été complété en conséquence. Cadre :
`exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`. **GCP utilisé** : sessions
gardées `claudebouts1` (commit poussé f1a53fe1c) et `claudebouts2` (commit poussé b72fe8771), `gcp-migration/v11_session.py`,
VM `g4-standard-48`, Python épinglé (scikit-learn 1.7.2) ; arrêts ciblés certifiés, état `TERMINATED` relu.

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

Session `claudebouts2` (lot 2 : les 31 bouts qui ont un sous-dossier dans `Zoltan/demos/` et les 5 démos de scène
entière, [`selection_lot2.json`](selection_lot2.json)) : déballage de 73 fichiers, 36 scènes, 0 écart ; porte conforme
(2 863 nuages, 12 fixtures sur 12, 4 mutants tués sur 4) ; campagne `--members-all` (meilleur bloc de chaque objet
publié pour HGP et pour HDBSCAN, qui sert aux images), 36 scènes « ok », 679 s.

Entrées : [`bouts_lot1.json`](bouts_lot1.json) décrit les 360 bouts (trame, empreintes de la trame et des étiquettes,
instances, écarts, empreintes des fichiers du bout) ; aucune coordonnée. Ils viennent de deux passes de
`Zoltan/demos/tools/chercher_bouts.py` : séquences 00 à 10, une trame sur 10 (304 bouts), puis vélos et piétons, une
trame sur 2, écart jusqu'à 1 m (68 bouts, dont 12 déjà présents).

Catégories (critères écrits dans `Zoltan/demos/tools/choisir_bouts.py` avant la lecture des résultats ; recalculées
par le lecteur depuis `claudebouts1` et comparées à `Zoltan/demos/bouts_evalues.json`) :

| Catégorie | Voitures | Vélos | Vélos et piétons |
| --- | --- | --- | --- |
| HGP réussit, HDBSCAN échoue | 0 | 11 | 2 |
| HGP échoue, HDBSCAN réussit | 0 | 3 | 0 |
| les deux échouent | 0 | 9 | 2 |
| les deux réussissent | 263 | 58 | 12 |

Ces mesures portent sur le meilleur groupe de chaque hiérarchie pour chaque objet, pas sur un découpage automatique.
