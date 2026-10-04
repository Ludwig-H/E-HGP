# Compatibilité préparation / décompression — 4 octobre 2026

Constat de lecture du commit `f1a53fe1ccc121bc80736473753ca156b0efee46`, sans modification du produit et sans
extraction d'archive réelle. Les deux sources et le manifeste historique pts3 sont des blobs Git figés,
copiés avant lecture puis rehachés après. Aucun octet LiDAR ne figure dans ce reçu.

`points_lidar_prepare.py:169–170` n'écrit que `sites_sha256` pour ses nouvelles scènes voisines, alors que
`points_unpack.py:42–45` exige aussi `labels_sha256`. Une archive de ces scènes échoue donc systématiquement
à la vérification (code2), même si ses fichiers sont corrects. Les 64 scènes du manifeste historique pts3 ont
les deux champs : ce constat n'invalide pas leurs archives historiques et ne rejoue pas leurs données.

La garde AST exécute uniquement l'expression réelle de production du manifeste et la boucle réelle de
vérification des hashes, sur deux fichiers synthétiques totalisant 16 octets. Elle constate le refus du champ
absent, l'acceptation après ajout de son hash réel, et le maintien du refus pour un hash incorrect. Elle ne
construit ni n'ouvre de tar et ne lance aucune préparation native. Les gardes explicites passent de façon
identique en Python normal et −O.

Conseil : ajouter `labels_sha256` au producteur, conserver le consommateur strict, puis ajouter une porte
interopération qui déballe une petite archive synthétique issue de la préparation et une archive conforme au
schéma historique. Cette dernière étape n'est pas exécutée ici. Le mapping XYZ/étiquettes reste préservé par
les octets et le réordonnancement explicite par IDs du consommateur de campagne ; aucun défaut de mapping
n'est établi par cette lecture.

Reproduction depuis ce répertoire :

```
python -B check_interop.py > checks.json 2> checks.stderr.txt
python -B -O check_interop.py > checks_optimized.json 2> checks_optimized.stderr.txt
cmp checks.json checks_optimized.json
```

SHA256SUMS couvre exhaustivement les fichiers réguliers, en excluant seulement lui-même. Aucun produit,
ancienne capsule, note active, build, fit ou GCP n'est modifié/exécuté.
