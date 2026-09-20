# Quatre mutations produit compilées — 20 septembre 2026

Résultat : **4/4 tuées par une comparaison géométrique**, après réussite des deux
portes non modifiées. Aucun fichier produit ni artefact du build de référence
n'a été modifié. Ce complément hors inventaire mesure la sensibilité des portes,
pas la performance, la complétude globale ni un contrat de tour/G4.

## Expérience et preuve

- Build : `build/v8_q34_candidates_20260920`, Release sans sanitizers.
- Source : HEAD `9ae4e28beb86b9066b86628007fa05ad9547d165` plus la tranche en cours,
  identifiée par **144 hashes de sources identiques avant/après**, et non par le
  seul commit de base. Archive, objets des portes, exécutables, cache CMake,
  compilateur et harnais également épinglés.
- Baselines : ExactBall **1 807 cas / 34 460 contrôles** ; candidats q34 **635
  appels / 24 590 contrôles**, toutes deux PASS.
- **14 commandes** conservées : deux baselines, puis compilation, lien et
  exécution pour chacune des quatre mutations. Chaque binaire muté sort avec le
  code **1**, après compilation/lien réussis ; ni signal ni échec de compilation
  ni simple défaut de non-vacuité ne vaut mutation tuée.
- Sources copiées et objets/binaires mutés dans
  `/tmp/mhgp8_q34_mutants_x2jq9kx8`, laissé disponible ; l'objet modifié est lié
  avant l'archive inchangée, dont le membre original correspondant n'est donc
  pas extrait. Copies originales/modifiées et patches sont archivés au dépôt.

| Mutation unique | Échec observé dans la porte |
| --- | --- |
| Orientation q4 : ne plus changer le signe du numérateur linéaire quand le déterminant est négatif | `primitive key differs from independently rationalised centre/radius` |
| Positivité q4 : accepter le poids barycentrique nul de l'origine (`remaining < 0` au lieu de `<= 0`) | `strict positivity/rank differs from rational Gram solve` |
| Census q3 : compter la coquille parmi les intérieurs (`power <= 0`) | `seed candidates differ from rational brute-force oracle` |
| Groupe q4 : abandonner au premier support non propriétaire au lieu d'essayer le suivant | `seed candidates differ from rational brute-force oracle` |

Les deux portes utilisent le même auxiliaire de résolution rationnelle
indépendant du produit. **Ce sont quatre perturbations causales, pas quatre
oracles indépendants.** L'expérience n'ajoute aucune famille de nuages aux tests
existants et ne prouve pas l'absence d'autres défauts.

## Reçus et relecture

Capture : [mutants/compiled_4cqzufx_](mutants/compiled_4cqzufx_/).
Les `record_*.json` contiennent chaque commande, le code de sortie et les sorties
brutes en texte et Base64. `COMPLETION.json` conserve les hashes de fermeture et
ceux des patches, objets et binaires modifiés.

- SHA256 manifeste : `532e59026cdf589699e3744f18b29facfca3495662fe9bb5fcc3eff0007ba87e`.
- SHA256 clôture : `8c0f681fb92cafc9c84ebb6b33afc67637dfe9d27778b8e3167cd83bc91414b4`.
- SHA256 harnais : `152a4678f124dddf14145d90b870225edbe531a630fa4bec89e8d97d6ba25211`.

Les quatre relectures **normal / `-O`, historique / `--check-live`** passent ;
voir [mutants/READBACK.json](mutants/READBACK.json). Le lecteur historique
n'exige pas la survie des fichiers temporaires : il vérifie la capture, les
patches, les commandes, les sorties et les égalités avant/après archivées.
`--check-live` ajoute le contrôle des sources, artefacts, compilateur et harnais
actuels, ainsi que des objets/binaires temporaires.

Depuis la racine du dépôt :

```sh
python -B morsehgp3D_v8/receipts/q34_seed_20260920/mutants/run_mutants.py read morsehgp3D_v8/receipts/q34_seed_20260920/mutants/compiled_4cqzufx_
python -B -O morsehgp3D_v8/receipts/q34_seed_20260920/mutants/run_mutants.py read morsehgp3D_v8/receipts/q34_seed_20260920/mutants/compiled_4cqzufx_ --check-live
```
