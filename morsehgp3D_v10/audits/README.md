# Audits de la v10 : point d'entrée des auditeurs

`public_status=not_claimed`. Les audits motivent des corrections ; ils ne certifient rien.

## Où lire

- État courant : [`../PASSATION.md`](../PASSATION.md) (fait foi), puis [`../README.md`](../README.md).
- Conception : [`../docs/SPEC_V10.md`](../docs/SPEC_V10.md), [`../docs/conception/`](../docs/conception/),
  [`../docs/CLUSTERING_DEPUIS_LA_TOUR_20260929.md`](../docs/CLUSTERING_DEPUIS_LA_TOUR_20260929.md).
- Preuves de mesure : [`../receipts/`](../receipts/), un dossier par livraison, immuable. Les corrections après coup
  sont dans [`../receipts/ERRATA.md`](../receipts/ERRATA.md).
- Audits déjà présents :
  - [`audit_v9_20260928/`](audit_v9_20260928/) : audit critique de la v9, qui a fondé la v10 ;
  - [`audit_hierarchie_knn_20260929/`](audit_hierarchie_knn_20260929/) : pertinence de la hiérarchie de la tour pour
    les niveaux de densité K-NN, comparaison avec HGP-old ;
  - [`tete_multik_20260929/`](tete_multik_20260929/) : trois têtes multi-K et leur juge.

## Conventions

- Un fichier ou un dossier par audit, daté `_AAAAMMJJ`, ancré au hash court du commit audité.
- **Auditeurs** : `AUDIT_*`, `CONTRE_AUDIT_*`, `ADDENDUM_*`, `QUESTION_AUDITEUR_*`.
- **Claude**, développeur : `REPONSE_CLAUDE_*`, `NOTE_CLAUDE_*`, `QUESTION_CLAUDE_*`.
- Personne ne modifie le fichier d'un autre ; on répond par un nouveau fichier qui cite le précédent.
- Toute correction produit un reçu dans `../receipts/`. Une contradiction mathématique devient une fixture
  permanente.
- Je relis ce dossier à chaque poussée : une veille me signale tout nouveau fichier ici. Je réponds par
  `REPONSE_CLAUDE_*` et j'exécute ce qui doit l'être avant toute nouvelle dépense.

## Règles partagées

- **Git** : commits sur `main` seulement, sans branche. `git add` fichier par fichier, jamais `-A`. Vérifier que
  `git diff --cached` est vide avant d'ajouter, et faire `git pull --rebase` avant de pousser. Les fichiers d'autres
  acteurs (`morsehgp3D_v6/`, `morsehgp3D_v9/experiments/`) ne sont pas à nous.
- **G4** : une seule VM. Je lance les sessions par les scripts gardés (`gcp-migration/v10_session.py`), avec un arrêt
  certifié TERMINATED. Pour une mesure sur G4, demandez une fenêtre par une `QUESTION_AUDITEUR_*`.
- **Graines** : les graines `test` et `test_v10b` ne s'utilisent que dans une exécution préenregistrée
  (`../bench/synthetic/prereg/`). Tout le reste se fait en `dev`.
- HGP-old est une source critique, jamais un oracle ni une base de code, et sous licence en lecture seule. La thèse
  est une source, pas une autorité.

## Ce qui mérite le plus un regard critique (29 septembre 2026)

1. **Exactitude des changements de performance du jour.** Chacun est annoncé identique octet pour octet :
   - J2, filtre D-loc (`5565f94fb`) ;
   - assemblage non initialisé (`7eee86c53`) ;
   - course du pool (`8e3b76245`) ;
   - tête par dénombrement (`b662673b2`) ;
   - J2c, boîtes ajustées (`777406b82`).

   Les lemmes et contrôles sont dans les reçus correspondants. Une faille de preuve, un cas que les différentiels ne
   couvrent pas ou un mutant non tué est ce qui compte le plus.
2. **Le banc et ses conclusions** :
   - lot C préenregistré (`../receipts/test_cover_C_20260929/`) ;
   - affectation sous le col et décomposition Bayes/Morse (`../receipts/bench_dev_alloc_20260929/`) ;
   - sélection (`../receipts/bench_dev_shrink_20260929/`) ;
   - ordres K plus grands (`../bench/synthetic/bigk_dev.py`, reçu à venir).
3. **L'échelle** (`../receipts/g4_session5_scale_20260929/`) : temps linéaire en boules, mur mémoire d'environ 280
   octets par boule. La conclusion, un catalogue qui ne réside pas tout entier au-delà d'environ 1,4 M sites LiDAR,
   est-elle la bonne ?
4. **En cours** : conception de six leviers de performance (frontière, feuilles, tour, ordre et tête, GPU,
   sélection par scène), avec vérificateurs adverses. Le plan ordonné sera versé ici quand il sera prêt.
