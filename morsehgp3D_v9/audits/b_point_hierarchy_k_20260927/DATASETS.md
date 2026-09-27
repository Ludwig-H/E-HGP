# Corpus de points — plan fixé le 27 septembre 2026

Le plan comprend douze scènes entières : quatre FCPS 3D, six synthétiques
3D et deux SIPU **2D plongées dans z=0**. Aucun sous-échantillonnage, aucun
ajustement des paramètres selon les scores de vérité terrain. Les divisions
development/evaluation portent sur des scènes entières ; elles ne rendent
pas ces jeux publics « aveugles » et ne constituent pas un résultat statistique
sur douze scènes indépendantes. Les deux graines d'une famille restent liées.

| Cas | n | Dimension d'origine | Split |
|---|---:|---:|---|
| FCPS Hepta | 212 | 3 | development |
| FCPS Tetra | 400 | 3 | development |
| FCPS Atom | 800 | 3 | evaluation |
| FCPS Chainlink | 1000 | 3 | evaluation |
| SIPU Flame | 240 | 2, z=0 | development |
| SIPU Spiral | 312 | 2, z=0 | evaluation |
| Synthétique varied_density | 900 × 2 | 3 | development / evaluation |
| Synthétique linked_rings | 900 × 2 | 3 | development / evaluation |
| Synthétique bridge_noise | 900 × 2 | 3 | development / evaluation |

K={2,5,10}, min_cluster_size={20,50}, z={1,2} sont préfixés dans
`datasets.PLAN`. K=1 ne sert qu'à une vérification sur petits cas. Ces valeurs
ne sont pas choisies pour reproduire le nombre de classes. L'évaluation doit
publier chaque combinaison, pas sélectionner son meilleur score de labels.

## Sources et transport

Sources primaires : [SIPU, University of Eastern Finland](https://cs.uef.fi/sipu/datasets/)
et [FCPS, dépôt du mainteneur Michael Christoph Thrun](https://github.com/Mthrun/FCPS).
Hepta, Tetra, Atom et Chainlink sont **FCPS**, pas SIPU. EngyTime est 2D et
n'est pas inclus. Le lecteur vérifie réellement les dimensions des tableaux.
Référence FCPS : Thrun et Ultsch, *Clustering Benchmark Datasets Exploiting
the Fundamental Clustering Problems*, Data in Brief 30, 105501 (2020),
[DOI](https://doi.org/10.1016/j.dib.2020.105501).

FCPS est épinglé au commit `c9c55f4a6162b32d9d34de8b39011a8c254dda6a` :
fichiers `data/{Hepta,Tetra,Atom,Chainlink}.rda` téléchargés depuis
`https://raw.githubusercontent.com/Mthrun/FCPS/COMMIT/data/NOM.rda`.
Les champs sont `NOM$Data` et `NOM$Cls`, convertis avec `rdata==1.0.0`.
Leur SHA256 est obligatoire dans `datasets.PUBLIC`, avant décodage.

SIPU : [Flame officiel](https://cs.uef.fi/sipu/datasets/flame.txt) et
[Spiral officiel](https://cs.uef.fi/sipu/datasets/spiral.txt). Chaque ligne
est `x y label` : la troisième colonne **n'est pas z**. Références données
par SIPU : Fu et Medico, *FLAME*, BMC Bioinformatics 8:3 (2007) ; Chang et
Yeung, *Robust path-based spectral clustering*, Pattern Recognition 41(1),
191–203 (2008).

Les téléchargements directs SIPU ont expiré. Root a récupéré les 240/312
lignes par le navigateur sur ces URL officielles, vérifié les indices
consécutifs puis extrait les lignes numériques et normalisé LF. Fichiers
privés `raw/sipu_{flame,spiral}_browser.txt` et leurs
`_browser_provenance.txt`. Les hashes épinglés sont ceux de **ce texte
normalisé**, pas d'une archive HTTP byte-identique. Aucun miroir n'entre
dans les entrées mesurées. Les essais miroir privés éventuels sont inutilisés.

| Entrée préparatoire | SHA256 |
|---|---|
| Hepta.rda | dc1eb7a1da3de0a28003cae69355be112758d050ec44c3d76aaa2b340c765456 |
| Tetra.rda | 496ec6a0a43c21ef5283729120660bc8bd46c7f6e1409f47a5e754c9b7206c1a |
| Atom.rda | 8270f8a7f20ab7d4794d05471661d7519945c65c2fd50ded4b928c731f2131e9 |
| Chainlink.rda | a180d1e0b087308bf46e04d2c16372ee52852c9757de5e13e79ae59fe1e95564 |
| Flame, texte navigateur LF | 2523942f59388e428580e98974189fcdc29ecac71bb861b41ec76d422669ed93 |
| Spiral, texte navigateur LF | 5f0ae012e6c25d469e9b4485fb5cb0bc3f5c1c29f3163be2bc66e2a562a0bbe6 |

Précaution licence : FCPS annonce GPL-3 pour le paquet ; cela ne prouve pas
une licence uniforme de redistribution pour toutes ses données historiques.
La page SIPU demande de citer les articles originaux et aucune licence
générale de redistribution n'a été identifiée ici. Les données brutes et
préparées restent donc **privées et non versionnées** ; les URL, hashes,
transformations et scripts sont publiables. Aucun nouveau droit n'est présumé.

## Synthétiques 3D

Générateur NumPy `Generator(PCG64(seed))`. Graines fixes 2026092701 pour
development, 2026092702 pour evaluation, pour chacune des trois familles.
Les versions NumPy et hashes des réalisations sont conservés ; les fonctions
transcendantes et les lois normales ne sont pas promises byte-identiques
entre toutes versions/architectures. Les fichiers préparés épinglés restent
les entrées communes des méthodes, sans régénération pendant les mesures.

- `varied_density` : trois normales 3D de 300 points, centres (-2,0,0),
  (0,2,1), (2,0,-1), écarts-types isotropes 0.08, 0.22, 0.5. Labels 1/2/3.
- `linked_rings` : deux cercles entrelacés, 450 points chacun, angles
  uniformes ; (cos a,sin a,0) et (1+cos b,0,sin b), perturbation normale
  isotrope sigma=0.04. Supports non convexes 3D, labels 1/2.
- `bridge_noise` : deux normales de 350 points, centres (-2,0,0), (2,0,0),
  sigma=0.25 ; 80 points de pont x uniforme [-1.5,1.5], y/z normaux sigma
  0.06 ; 120 points uniformes dans [-3,3]×[-1.5,1.5]². Pont et fond ont le
  label brut 0, converti en -1. C'est une convention générative déclarée,
  pas une preuve que tout point ainsi étiqueté doit être géométriquement
  rejeté par toute méthode de densité.

## Une seule géométrie quantifiée pour les deux méthodes

Pour chaque scène, origine = minimum par axe, et **un seul** pas
`h = max_j(max X_j - min X_j) / (2^18 - 1)` pour les trois axes. Une
translation n'est pas une normalisation par axe. Les coordonnées d'entrée
binary64 sont converties en rationnels exacts ; l'arrondi de `(x-o)/h`
est au plus proche, égalités vers l'entier pair. Cela définit les entrées
u18 de ce benchmark, pas une qualification du moteur float32 sans perte.
Erreur ≤ h/2 par coordonnée, ≤ sqrt(3) h/2 en norme euclidienne ; erreur
réelle maximale et borne, pas/origine rationnels, doublons bruts et collisions
de grille sont publiés par cas. Le chiffre flottant du maximum euclidien
est descriptif ; la borne coordonnée est vérifiée rationnellement.

Le préparateur refuse **tout doublon quantifié**, y compris même label,
et rapporte les contradictions de labels. Il ne fusionne ni ne supprime
de point, ne perturbe pas les égalités, ne remplace pas la géométrie par
les labels. Les IDs restent les indices de ligne originaux. Labels 0 de
bruit deviennent -1, jamais une classe ordinaire ; -1 reste -1, les labels
positifs ne sont pas renumérotés.

`points.u32le` : XYZ entrelacés, uint32 little-endian sans en-tête,
exactement 12n octets. `points.npy` : les **mêmes entiers**, convertis
exactement en float64 pour HDBSCAN, ni coordonnées originales ni autre
standardisation. `labels.json` : liste séparée d'entiers, aucun label dans
les fichiers géométriques. L'uniforme changement d'unité modifie les valeurs
de densité absolues ; ne comparer ces valeurs qu'avec l'échelle déclarée.

## Interface et reproduction locale

`prepare(root) -> dict` écrit `root/plan.json` avant de charger les jeux,
puis `root/manifest.json` et `root/prepared/CASE/`. Un sous-ensemble demandé
explicitement écrit un autre manifeste avec `complete=false` ; le premier
lot dix cas reste conservé. Réexécuter à octets identiques est permis,
écraser une différence est refusé. Le manifeste expose pour chaque cas :
`id,name,dimension,n,split,points_u32le,points_npy,labels_json,source_sha256`,
`quantization`, les hashes des trois fichiers et les métadonnées de source.
Pas de réseau implicite dans `prepare`.

Racine privée : `/workspaces/E-HGP/build/v9-point-clustering-data-20260927`.
Lecteur R privé, sans installation globale : `tools/venv/bin/python` sous
cette racine, rdata 1.0.0, pandas 3.0.6, xarray 2026.7.0, NumPy 2.5.3.
L'environnement ne sert qu'à préparer les données. Les tests de ce module
ne demandent que NumPy et n'accèdent ni au réseau ni aux sources brutes.

```sh
python -B datasets.py /CHEMIN/PRIVE
python -B test_datasets.py
python -B -O test_datasets.py
```

Aucun benchmark de qualité, résultat HDBSCAN/HGP, coût FULL ou résultat
GPU ne découle de cette seule préparation. Les tests des algorithmes et
le gel du protocole de comparaison appartiennent aux autres fichiers du lot.
