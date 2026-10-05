# Contre-lecture L, mesure et publication 484fb98ee

Sources exécutées des sessions L et mesure :
`b319efc8477fec234afc0b31e86f8a43e3023641`. Leurs reçus finaux, archives,
paquets et plans concordent par SHA ; arrêt ciblé certifié, génération et
cible concordantes, clé supprimée et réservation libérée.

L donne **34 Passed sur 38**, dont les 27 portes sans label de campagne
mutants. Les deux identités CLI FULL K10 (scale32000 et trame ng00), Euler
K10 sur ng00/ng01/ng02, les registres supports K10 et la hiérarchie ng00 K10
ont des résultats Passed explicites. Le résumé global reste non conforme :
timeout et quatre campagnes mutants tower/supports/api/cli sans résultat.
Le reçu dit `failed_remote`, worker 1, DONE contrôleur 3. Noms exacts dans
`summary.json` ; aucun échec individuel consigné.

La mesure est complète : **52 appels uniques, tous ok**, soit 48 à K5
(ng00/ng01/ng02, FULL/supports, W1/W48, un froid et trois chauds) et quatre
descriptifs K10 ng00/W48. Les empreintes relevées de fichier/manifeste restent
identiques entre prises et W pour chaque sortie ; `tree_k_sha256` est commune
à FULL et supports. Tous les étages requis sont relevés, dont attach/output/write.
Les dossiers mesurés ont été retirés par le banc : le rejeu vérifie les valeurs
et la couverture consignées dans l'archive, sans reconstruire leurs fichiers.

Réapplication indépendante de la règle prédéclarée : à K5/W48 sur l'étage
`tree`, ng02 seule respecte le seuil. **La décision `livrer_L2b` concorde** avec
les appels et le document publié. Les appels comparent FULL au masque 16379
et supports au masque 7035 ; cette décision prescrit la suite L2b.
Les compléments supports W48 ng02 et ng00 sont clos : verdict conforme,
14 appels et 52 contrôles chacun à W1/W4/W48.

Le reçu Git `484fb98ee7f3e64ab9965b1b3beb180608129beb` a 41/41 empreintes
conformes ; ses reçus et résumés L/mesure sont les copies exactes des archives.
Un décalage de provenance doit être corrigé dans son README : **A exécutait
00bd979ac**, comme le dit son reçu publié exact, alors que les lignes 4 et 13
la rattachent à b319efc84. Le résumé L doit garder la distinction entre succès
des portes achevées et non-conformité de la matrice entière.

B reste explicitement partielle dans la publication : **50 portes ASan/UBSan
et 65 TSan sans résultat**. Les nouvelles preuves L/mesure ne ferment pas ce
périmètre, ni les changements ultérieurs de source, dont S8. Aucun contrat
de temps FULL n'est acquis par cette contre-lecture.

```sh
python3 /chemin/de/cette/capsule/replay.py --repo /chemin/du/depot
python3 -O /chemin/de/cette/capsule/replay.py --repo /chemin/du/depot
```

Rejeu borné en lecture seule : stdlib et `git show` local, aucun build, test du
produit ou accès cloud. Dépend des deux sessions locales sous
`/workspaces/.ehgp-sessions/`, du reçu A local et de l'objet Git publié.
`manifest.json` conserve seulement les champs sûrs, SHA et plans consommés ;
aucun compte, clé, log brut ou nuage recopié. Capsule non autonome ; captures
antérieures inchangées. `SHA256SUMS` ferme les quatre fichiers.
