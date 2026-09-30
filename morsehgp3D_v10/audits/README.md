# Audits de la v10 : point d'entrée des auditeurs

`public_status=not_claimed`. Les audits motivent des corrections ; ils ne certifient rien.

## Où lire

- Produit : [`../PASSATION.md`](../PASSATION.md) (fait foi), puis [`../README.md`](../README.md).
- Audits actifs : [état courant de l'audit continu](AUDIT_ETAT_COURANT.md) et [suivi des constats de l'audit indépendant](SUIVI_AUDIT_INDEPENDANT.md). Ces vues sont mises à jour en place ; les rapports de base ne ferment pas les correctifs ultérieurs.
- Conception : [`../docs/SPEC_V10.md`](../docs/SPEC_V10.md), [`../docs/conception/`](../docs/conception/),
  [`../docs/CLUSTERING_DEPUIS_LA_TOUR_20260929.md`](../docs/CLUSTERING_DEPUIS_LA_TOUR_20260929.md).
- Preuves de mesure : [`../receipts/`](../receipts/), un dossier par livraison, immuable. Les corrections après coup
  sont dans [`../receipts/ERRATA.md`](../receipts/ERRATA.md).
- Audits déjà présents :
  - [archive de l'audit v9](../receipts/audit_v9_20260928/) : audit critique historique qui a fondé la v10 ; conclusions et preuves conservées dans `receipts/` ;
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
- Je relis ce dossier (`git fetch`) à chaque étape de mon travail et avant chaque poussée. Je réponds par
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

## Priorités courantes

1. **FULL → points** : conserver la couverture et les masses des points frontière avant condensation ; utiliser core comme comparateur de stabilité. [La note courante](audit_independant_20260929/ANCRAGE_AMBIGUITES.md) relit les parties I et II de la thèse, précise le bras K2 et répond au choix du panel de paires.
2. **Correctifs et juges** : fermer les défauts reproduits d'entrée, pool, tête et banc par contre-vérification, puis vérifier leur intégration. [La réponse du développeur sur le raccord](REPONSE_CLAUDE_CONTRE_AUDITS_ET_RACCORD_20260929.md) fixe l'extraction commune et les décisions par constat ; les suivis actifs ci-dessus portent leur statut.
3. **Contrats et portée des preuves** : utiliser [la passation](../PASSATION.md) et [les errata](../receipts/ERRATA.md). Les ratios mémoire et temps mesurés ne sont pas des bornes générales ; aucun contrat FULL GPU ou capacité LiDAR générale n'est acquis.

Les archives, captures, scripts de reproduction et notes intermédiaires remplacées vont dans `../receipts/`, avec leurs empreintes et leurs liens. L'audit v9 y a été déplacé intégralement ; ses conclusions restent accessibles par le lien historique ci-dessus. Les fichiers encore utilisés par un autre auditeur ne sont pas réorganisés pendant ses travaux.
