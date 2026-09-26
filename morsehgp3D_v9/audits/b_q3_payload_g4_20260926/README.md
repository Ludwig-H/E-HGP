# Publication des reçus FULL G4 avec intérieurs q3

Outils hors ligne, aucun appel GCP. Reprise du découpage `host/` + `vm/`
de R24B et des contrôles de publication du cache S2 ; les anciens outils
et reçus ne sont pas modifiés. Aucun résultat nouveau tant que la capture
n'est pas terminée, arrêtée et rejugée.

Capture suivante close et publiée dans `receipts/g4_q3_payload_20260926` :
26 cas rejugés, 18 GPU/8 engine CPU, 19 comparaisons exactes, neuf paires
ON/OFF. Lecteur normal/−O identique, SHA256 de sa sortie :
`946da1f39949e3184e4ed1ae6b033e2f7d3aafbd63e174543a31b2e4141b4283`.
Tous les processus utilisent `frames=1` ; aucun chrono chaud. Le census
diminue mais le bénéfice FULL n'est pas stable ; défaut OFF conservé.
La capture consomme 306,768 s d'allocation, soit 737,423 s avec les échecs
clos ci-dessous. Aucun nouveau contrat 100 ms ou de croissance acquis.

Échecs clos conservés dans `receipts/g4_q3_payload_failed_capture_20260926` :
91 fichiers hachés plus l'inventaire, aucun nuage KITTI ni archive. Le worker
initial a rendu code 0 mais sa capture a échoué ; aucun de ses 26 cas
planifiés n'est qualifié localement. R1 est refusée avant création de
génération (expiration de clé OS Login). R2 démarre puis échoue au pack sans
sortie expliquant le test bloquant : aucune disparition de `/tmp` n'en est
déduite. Les deux générations allouées ont été certifiées arrêtées.
Durées GCE : 309,542 s + 121,113 s = **430,655 s**, R1 ajoutant zéro
génération. Ce sont des durées d'allocation, pas des montants facturés.
`read_failed.py <reçu> --snapshot <snapshot privé>` rejoue cette preuve sans
prétendre rejouer des probes absents. Lectures normal/−O identiques :
SHA256 `fdf6336be6277738fb6b5b6ecad2049d559762f0b00313196064d285f3cfd8f4`.

`publish_closed.py` exige un reçu hôte `completed`, un arrêt ciblé certifié,
les hashes du paquet et la confirmation GCE `TERMINATED` de la même
génération. `after_stop.json` doit être la réponse JSON complète de describe
(identité, zone, machine, labels, scheduling et deux timestamps).
Il rejoue le protocole et toutes les sorties avant de créer le répertoire.
Il ne copie que les fichiers VM explicitement autorisés et les preuves hôte
sélectionnées. Le seul binaire autorisé est le préflight **synthétique**,
comparé octet pour octet à sa recette. Jamais de nouveau nuage KITTI,
snapshot, archive, clé SSH ou sortie brute OS Login. Les logs de garde sont
expurgés ; leurs hashes originaux restent référencés. Le parent de session
n'est jamais parcouru.

Un chemin de récupération séparée a été préparé avec `--recovery-dir` mais
n'a pas obtenu de capture rejugée. Il reste documenté, non qualifié. L'hôte initial
reste strictement `capture_failed`, avec ses champs GPU/FULL faux et son
incident inchangés. Le répertoire `recovery/` conserve le reçu distinct,
les commandes de copie seule et les deux observations GCE expurgées.
Le lecteur exige que le premier arrêt ait été **lu avant le redémarrage**,
deux générations différentes et deux arrêts ciblés certifiés. Les probes
récupérés sont rejugés contre la génération, les gardes et les sources de
la première session, jamais contre le démarrage de récupération. Le hash
du reçu worker est post-récupération : aucun épinglage antérieur à l'arrêt
n'est inventé. Le statut publié est explicitement `recovered_capture`.
Les deux durées d'allocation sont additionnées ; la deuxième ne contient
ni compilation ni nouvelle mesure.

`--failed-recovery-dir` conserve également la première tentative refusée
avant démarrage (clé OS Login existante insuffisamment rafraîchie). Son reçu
et ses commandes sont inchangés, ses logs sélectionnés sont expurgés, et la
lecture LIVE suivante doit encore confirmer la génération originale arrêtée.
Aucune durée de VM supplémentaire n'est attribuée à ce refus avant start.

Après feu vert du responsable, depuis la racine du dépôt :

```sh
python3 -B morsehgp3D_v9/audits/b_q3_payload_g4_20260926/publish_closed.py \
  --host /workspaces/E-HGP/build/v9-q3-payload-rerun-20260926.Is4XsY/tower_v9_host \
  --package /workspaces/E-HGP/build/v9-q3-payload-snapshot-f9f273bb0/PACKAGE.json \
  --after-stop /workspaces/E-HGP/build/v9-q3-payload-rerun-20260926.Is4XsY/closure/after_stop.json \
  --output morsehgp3D_v9/receipts/g4_q3_payload_20260926
python3 -B morsehgp3D_v9/audits/b_q3_payload_g4_20260926/readback.py \
  morsehgp3D_v9/receipts/g4_q3_payload_20260926 \
  --snapshot /workspaces/E-HGP/build/v9-q3-payload-snapshot-f9f273bb0/snapshot.tar.gz
python3 -O -B morsehgp3D_v9/audits/b_q3_payload_g4_20260926/readback.py \
  morsehgp3D_v9/receipts/g4_q3_payload_20260926 \
  --snapshot /workspaces/E-HGP/build/v9-q3-payload-snapshot-f9f273bb0/snapshot.tar.gz
```

Le lecteur est LIVE : il exige le snapshot local privé et le protocole
épinglé. En cas de perte du snapshot, le reconstruire à partir du commit
`source_commit` et de `plan.json` avec `tower_snapshot_v9.py --commit ...
--plan ... --output <répertoire neuf hors v9>`. Les données déjà présentes
dans les objets Git historiques servent à cette reconstruction ; elles ne
sont pas republiées.

`SUMMARY.json` est entièrement recalculé depuis les probes bruts puis
comparé au fichier publié. L'unité statistique est le processus : premier
passage, médiane par processus et passages chauds après le premier sont
distincts. Les paires ON/OFF doivent garder géométrie, travail producteur et
trois digests identiques ; seuls les censuses consommateurs diminuent.
Les sous-100 ms/sous-1 s sont des observations de ces cas, jamais une
qualification automatique multis-scènes ou sous-quadratique. La durée
d'allocation SPOT est calculée depuis les timestamps GCE ; aucun montant
facturé n'est inventé sans export de facturation ou tarif vérifié.

Tests hors ligne : `python3 -B .../test_tools.py`, puis `python3 -O -B .../test_tools.py`.

Les 17 tests couvrent notamment le refus d'une récupération encore ouverte,
d'une reprise de benchmark, de générations/hashes divergents, d'un premier
arrêt observé trop tard, d'un mauvais arrêt ciblé et de toute promotion du
reçu hôte initial. Ils sont synthétiques et ne qualifient pas la G4.
