# MES-C2 : provenance et fermeture locales

La session `v12.20261008.mesc2` est **fermée et récupérée** : `completed`, commande et worker code0, `DONE=0`, aucun incident, archive vérifiée ; arrêt ciblé certifié, code0 en une tentative, observation `RUNNING → TERMINATED`, garde intacte et réserve libérée. Python normal et `-O` donnent la même vérification. Aucun contrôleur, moteur, compilateur ou appel distant n'a été lancé par l'audit.

| Preuve | Valeur vérifiée |
|---|---|
| Source effective | `27eca166b9b678bd97c9469e7071823097fc0c0b` |
| Paquet | SHA `b6dc7950fe0f808a0f42658dfed568df499729c09986012cf1e2e0e144b16716` |
| Plan | SHA `bb93c86bba3fad8394b51e37470e949601624dfa3db94417d547d33256d61eb5` |
| Résultats | 916 858 octets, SHA `c88472e0a5eb64f15ad3dc09855f18bcec66159648eed549956efa7bd4e089d9` |
| Manifeste intérieur | **92 fichiers couverts**, aucune entrée manquante ; SHA `495f02ee3e2c951312f1ba40a5cbb66a7dd885b7bd37690daa0c8752fc95b864` |
| Processus pilote | commande0 `mes_c`, code0, **889,011 s**, limite2150 s ; groupe fermé, aucun résidu tué, aucun flux tronqué |

Les 889 s sont le mur du **pilote entier**, jamais une latence FULL. Le lancement local est daté07:06:34 UTC ; les heures de VM du rapport publié ne sont pas substituées à ce mur. Le certificat d'arrêt est relu depuis la capture locale, sans nouvelle interrogation distante.

**351 sources du paquet sont identiques aux objets Git**, sous `src/`, `bench/`, `tests/`, `cmake/` et `CMakeLists.txt`, avec inventaire complet par chemin et SHA. Le pilote MES-C et son lecteur, hors de ces répertoires, sont vérifiés séparément. La construction archivée demande Release, u21 et CUDA ON. Le rapport déclare le binaire `4787a332…` ; aucun ELF n'est reconstruit ou qualifié rétroactivement par cette lecture.

Depuis [MES-C1](../session_mes_c_provenance/README.md), le delta produit exact contient **douze fichiers du catalogue T2-d-C et trois du pool**, sans autre fichier natif. `full_probe.cpp` reste `201119a7…`, le lecteur FULL `3594c5d3…`. Le pilote devient `a0b3e9b9…`. Le pool est celui de5b, et **A n'est pas dans cette campagne**. Une différence C1/C2 ne permet donc pas d'attribuer un gain au pool seul ; les Sessions sont distinctes.

Le plan C2 et les paramètres effectivement rapportés concordent : voies CPU/appareil, K5/K10, **W4/W48**, trois passes par Session ; cas difficiles W48 et deux passes. Il prévoit donc huit Sessions régulières, **aucune W1**. L'archive contient56 JSONL. Leur admission et les statistiques sont séparées dans [session_c2_admission](../session_c2_admission/README.md) ; la fermeture du processus pilote ne vaut pas succès de toutes ses sondes.

Les métadonnées déclarent exactement la même archive `g4_small.tar` que C1 : 6 010 880 octets, SHA `1b11650d…`, manifeste déclaré `379dc49f…` vérifié comme fichier de métadonnées. La vérification distante des données est annoncée réussie par le contrôleur. **Aucun tar de données, XYZ ou ID n'a été ouvert ou haché ici** ; cette comparaison porte sur les déclarations et leur provenance, pas sur une nouvelle inspection des scènes.

Hors Git, une capture persistante conserve seulement rapport,56 JSONL, construction et tableaux :59 fichiers,3 686 465 octets. Aucun brut volumineux, compte ou commande privée n'est dupliqué dans ce reçu. [capture.json](capture.json) garde les hashes, options publiques, issue de commande et delta de sources. Les lecteurs d'archive et de sources déjà publiés pour MES-C sont réutilisés et épinglés, sans copie.

```sh
python -B check.py --repo DEPOT --session SESSION > /tmp/c2-normal.txt
python -B -O check.py --repo DEPOT --session SESSION > /tmp/c2-opt.txt
cmp /tmp/c2-normal.txt /tmp/c2-opt.txt
```

Les octets Git et les fichiers locaux fermés sont requis. [SHA256SUMS](SHA256SUMS) couvre ce reçu ; aucune modification de source ou de qualification produit.
