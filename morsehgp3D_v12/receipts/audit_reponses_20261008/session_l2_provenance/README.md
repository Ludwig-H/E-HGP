# Session L2 : provenance et arrêt vérifiés localement

8 octobre 2026. Archive locale `52f1dded…`, **13 110 octets** ; manifeste
46/46 fichiers vérifié, seul le manifeste exclu de lui-même. Source
`a2c2fccfd65f89a4bb874b8bf5e1bd2c737856b1`, paquet `e81527e1…`, plan
`1f9e4599…`. Aucun moteur, téléchargement ou appel cloud lancé par l'audit.
Les champs privés du contrôleur ne sont ni imprimés ni archivés ici.

Le paquet contient exactement les objets Git de `bench/full_probe.cpp`
(`201119a7…`), `microbancs/mes_b_scenes/pilote_b.py` (`e10a9cd4…`) et du
lecteur commun `microbancs/outils/lecteur_full.py` (`3594c5d3…`). Le rapport
`c1e55eae…`, 7940 octets, déclare le même pilote et le binaire `c95c1e1e…`.
Cette empreinte de sonde provient du rapport : aucun ELF distant n'est
rehaché ici. Construction déclarée Release/u21/CUDA ON, exécutions CPU.

La commande `mes_b` termine code 0, sans délai dépassé ; ses 747,634 s
englobent la commande et ne sont pas un chrono FULL. Le plan demande
cinq cas CPU K5, W48, **une seule passe** chacun, budgets hôte160 Gio /
appareil88 Gio, délai global1880 s. Deux cas émettent une passe FULL
complète (Paris sans sol9 111 422 sites, Paris brut14 551 520). ETH3D
16 828 368 refuse `wide_leaf` ; Lyon sans sol24 016 862 et brut32 412 887
refusent `memory_budget`. Zéro passe chaude, zéro essai GPU. La limite
d'empreinte1,6 million exclut les empreintes FULL de ces succès : cette
archive ne constitue pas une nouvelle preuve différentielle de l'objet.
Les critères B1–B4 sont « non évalués », verdict du rapport « non tenu ».
Le label tronqué du premier cas est conservé, à relier à son étiquette
prévue et à la commande ; l'admission indépendante est un reçu distinct.

Contrôleur : `completed`, `DONE=0`, worker0, zéro erreur, récupération
`downloaded`, résultats vérifiés. Une tentative d'arrêt, code0, états
observés **RUNNING → TERMINATED**, arrêt ciblé certifié, garde intacte et
réservation rendue. Ce sont les certificats locaux épinglés, pas une
nouvelle interrogation de la VM.

Relecture métadonnées uniquement ; réutilise les fonctions de manifeste
du [reçu L1/récupération](../session_l1_recuperation/README.md), épinglées :

```sh
python check.py --session /chemin/session-L2
python -O check.py --session /chemin/session-L2
```

Normal/−O donnent le même résultat. Le rapport, dix fichiers JSONL/err,
log de construction, plan et trois sources sont aussi sauvegardés hors Git
dans la capture persistante transmise au coordinateur ; aucune coordonnée
du jeu de données ou identité de compte n'est ajoutée au dépôt.
