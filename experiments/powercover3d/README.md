# Expérience historique `PowerCover3D`

> [!WARNING]
> Ce dossier conserve un protocole expérimental du prototype cubique `PowerCover3D`. Il ne valide ni `MorseHGP3D`, ni `HomogeneousLensTower`, et ne doit pas servir à annoncer leurs performances.

## Contenu conservé

- [`blackwell_30m.ipynb`](blackwell_30m.ipynb) : protocole Colab/Blackwell pour le chemin CUDA, les audits de voisinage, les diagnostics mémoire et les exécutions jusqu'à 30 millions de points;
- le package exécutable associé se trouve dans [`perg_hgp/`](../../perg_hgp/).

Le notebook versionné définit le protocole mais ne contient pas de sortie d'exécution. Les résultats bruts, manifests et notebooks intermédiaires ont été retirés de l'arbre courant. L'outillage GCP utile aux campagnes actuelles reste maintenu dans [`gcp-migration/`](../../gcp-migration/); les artefacts supprimés restent accessibles dans le [snapshot historique `0b8a1a1`](https://github.com/Ludwig-H/E-HGP/tree/0b8a1a11750b931f486ce666265eed4b6e95e2b1).

## Ce que la campagne historique a établi

La campagne G4 du 12 juillet 2026 a surtout montré que le coût du prototype provenait du voisinage, de la régularisation et de l'évaluation du champ, davantage que de la forêt cubique elle-même. Elle a également confirmé que la latence et la topologie dépendaient directement de la résolution de grille.

Ces observations restent utiles pour concevoir les futurs noyaux, mais les temps mesurés portent sur un autre objet mathématique. Toute nouvelle campagne devra repartir d'un manifeste reproductible et publier séparément :

- le matériel, les versions et le temps de compilation;
- le temps froid et le temps chaud;
- les coûts de voisinage, de fermeture et de réduction hiérarchique;
- le statut de certification et son périmètre;
- la comparaison événement par événement à un oracle de petite taille.

## Statut

Le notebook est **gelé comme référence**. Les corrections du prototype doivent rester limitées à la reproductibilité ou à la sécurité numérique; les nouveaux algorithmes doivent être développés dans des backends distincts après stabilisation du corpus A–E.
