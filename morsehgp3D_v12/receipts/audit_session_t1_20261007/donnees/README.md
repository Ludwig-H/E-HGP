# Audit borné de la préparation des données

7 octobre 2026, pin `4147c5460`. Sources introduites par `c55c12871`.
`public_status=not_claimed`. Aucun téléchargement, aucune lecture de nuage réel, aucune campagne G4.
`check.py` vérifie les sources au pin avant/après ; les sorties normal et `-O` sont identiques.

## CST-0216 — chemin de rejeu non adapté au port

`docs/DONNEES.md` annonce le rejeu par `bench/data/replay_all.sh`. Ce script définit encore `S=$ROOT/scripts`.
Or les fichiers sont désormais ses **voisins** dans `bench/data/`. Sans `ROOT`, sa valeur par défaut désigne
`bench/`, qui n'a pas de sous-dossier `scripts/` ; avec `ROOT` désignant le dossier de données comme documenté,
les sources n'y existent pas davantage. Un Python disponible et un répertoire de sortie temporaire suffisent
à reproduire le code 2 : `verify_inputs.py` est recherché sous le mauvais chemin, avant toute vérification de données.

Corriger la racine des scripts d'après l'emplacement de `replay_all.sh`, indépendamment de la racine des sorties,
puis tester une étape légère depuis un autre répertoire courant. Le rejeu historique dans le dossier de
préparation peut avoir fonctionné ; il ne qualifie pas le pilote déplacé dans le dépôt.

## CST-0217 — validation de manifeste favorable sans preuve complète

Le véritable `verify_inputs.py`, lancé par processus séparé avec Python `-S`, donne :

| Entrée synthétique | Code | Sens |
| --- | ---: | --- |
| manifeste nominal, quatre points, hashes exacts | 0 | contrôle positif |
| `cases=[]` | **0** | aucun cas ni fichier vérifié |
| un cas, `sha256=null`, fichier de coordonnées présent | **0** | coordonnées non hachées |
| même cas, hash de coordonnées explicitement faux | 1 | contrôle négatif |

`files_of` transporte le hash nul puis `main` saute la comparaison quand `digest is None`. Aucun schéma strict
ni minimum de cas ne précède le verdict. Exiger un manifeste reconnu, les champs/types attendus, des hashes
obligatoires et une sélection non vide avant tout résultat conforme. Les variantes distinctes et leurs
multiplicités doivent également avoir leurs preuves obligatoires. Aucun cas réel non vérifié n'est imputé
aux manifestes publiés par ces injections ; le constat concerne le contrat du vérificateur.

## CST-0218 — un plafond exact de N sites ne garantit pas une coupe horizontale complète

Le rapport §3 promet des carrés horizontaux concentriques, « toute la hauteur ». `crop_scenes.py` trie par
distance de Tchebychev XY, puis distance carrée XY et rang ; il conserve `order[:N]`. Le dernier groupe d'ex æquo
peut donc être tronqué jusque dans une colonne de mêmes X/Y.

Le véritable pilote est exécuté sur `(0,0,z)`, z=0,1,2,3, avec `--sizes 2`. Il écrit les IDs 0 et 1 et annonce un
rayon horizontal nul. Pourtant le carré horizontal fermé de ce rayon contient **les quatre** sites. Aucune
sélection dépendant seulement de X/Y ne peut en garder deux : la hauteur entière n'est pas conservée.

Pour le régime promis, choisir un rayon puis garder toute sa frontière et publier l'effectif obtenu, même
différent de N ; les découpes peuvent rester emboîtées. Si une sélection avec frontière tronquée est voulue,
la nommer et la qualifier séparément. Le témoin ne démontre ni une erreur de clustering sur les scènes publiées,
ni une perte dans une trame SemanticKITTI : ces trames entières suivent un autre chemin.

## Points vérifiés et limites

La grille commune passe **28 030** comparaisons à l'arithmétique rationnelle : flottants binary32/binary64 et
échelles LAS usuelles, avec cas limites et tirages fixes. La translation et l'écriture lexicographique du petit
témoin sont cohérentes. Cela ne valide pas les lecteurs LAS/LAZ/PLY, les poses Boreas ni l'intégralité des
préparations réelles, non rejouées ici.

Observation complémentaire, sans nouveau numéro : `prepare_outputs` promet le plus petit ID source par site,
mais choisit le premier **rang d'entrée**. À positions `(0,0,0),(0,0,0),(1,0,0)` et IDs `[9,2,4]`, il écrit `[9,4]`,
attendu `[2,4]` pour cette promesse. Les producteurs dont les IDs sont croissants ne rencontrent pas ce cas ;
aucun écart des données actuelles n'est démontré. Déclarer cette précondition ou choisir effectivement le minimum.

```sh
python3 -B morsehgp3D_v12/receipts/audit_session_t1_20261007/donnees/check.py
python3 -B -O morsehgp3D_v12/receipts/audit_session_t1_20261007/donnees/check.py
```

Les fichiers synthétiques vivent dans un répertoire temporaire, supprimé en fin de contrôle. Seuls le script,
les comptes, les hashes et ce rapport sont conservés. Aucun octet de jeu sous licence n'est ajouté.
