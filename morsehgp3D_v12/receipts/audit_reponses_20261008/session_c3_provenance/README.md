# MES-C3 : provenance et fermeture locales

La session `v12.20261008.mesc3` est fermée et récupérée : **completed, worker0, DONE0**, aucune erreur ; arrêt ciblé certifié code0 en une tentative, observation `RUNNING → TERMINATED`, garde intacte et réserve libérée. Relecture normale et `-O` identiques. Aucun moteur, compilateur, contrôleur ni appel distant lancé par l'audit.

| Preuve | Valeur vérifiée |
|---|---|
| Source effective | `72f622a556130e25afdadbef84984619417286e2` |
| Paquet | 10 232 498 octets ; SHA `9ec69b99d4a4fe4d4713c52e3fe8194c3af4ab5c643c54835d6887bb5ea54d9d` |
| Plan | SHA `61b2ed6feb8e9ac9e6ce1702c82efea2cbf9469c794f61baeef4604e685859dd` |
| Résultats | 2 158 037 octets ; SHA `99b2f6cfa6fa008fc008b7f0200b87a73340cf82bf81e949fb34b5818ca1a00e` |
| Manifeste intérieur | **504 fichiers couverts**, aucune absence ; SHA `49f032fafac98c40a145efe176a6f4dee8affa7a5c0827a1fe869fc4e17bd0fb` |

Les trois commandes ont un code0, un groupe fermé, aucun résidu tué et aucun flux tronqué : MES-C **839,383 s** sur limite2150 ; apparié appareil **40,535 s** sur900 ; apparié CPU **38,867 s** sur900. Ce sont des murs de pilotes entiers, pas des latences FULL. Leur fermeture ne suffit pas à adopter un bras ou à valider les contrats temps.

**357 sources du paquet sont identiques aux objets Git** sous `src/`, `bench/`, `tests/`, `cmake/` et `CMakeLists.txt` ; inventaire complet haché `5cfeb2cb…`. Sept sources clés et les scripts de mesure sont en outre épinglés. MES-C emploie le pilote `6b435ca4…`, les appariés `fe22a681…`, tous avec LF `c3e9e0f4…`. Les correctifs de lecteurs livrés plus tard en85 ne sont donc pas attribués à cette exécution. La sonde est `1068940a…` : **cache8 Gio par défaut**, CPU comme appareil, et chemin recouvert par défaut ; le correctif de terminaison0241 est présent (`pipeline_run f7e077d8…`), mais la porte native ajoutée enbdf est postérieure.

Les constructions archivées demandent Release/u21 ; CUDA ON pour MES-C et l'appariement appareil, aucune activation CUDA explicite pour l'appariement CPU. Les trois rapports déclarent des ELF distincts (`c99e855c…`, `cd640e73…`, `6e03dbb2…`). Aucun ELF final n'est archivé par l'ancien pilote apparié : la fermeture binaire complète ne se déduit pas du code0, et n'est pas reconstituée par ce reçu.

Le plan MES-C prévoit CPU/appareil, K5/K10, **W4/W48**, trois passes ; aucune W1. Chaque appariement prévoit les trois labels `p150,p1000,p5000`, K5/W48, dix tours et dix passes, avec `ref=` et `aa=` utilisant le défaut cache8 Gio, et `sans_cache=--cache=0`. La référence est donc **avec cache**. Leurs verdicts/statistiques sont relus séparément ; aucun seuil n'est modifié ici.

La même archive `g4_small.tar` de6 010 880 octets/SHA `1b11650d…` que MES-C2 est déclarée. Les trois fichiers supplémentaires donnent, par tailles XYZ/12 et IDs/4 concordantes, **156,1 013 et4 618 entrées** respectivement. Les labels ne sont pas des cardinalités exactes ; ces tailles ne prouvent pas l'unicité après déduplication. Les noms, tailles et hashes sont conservés dans `capture.json`. Le contrôleur déclare la vérification distante réussie ; **aucun tar de données, XYZ ou ID n'a été ouvert ou haché par cet audit**.

Le delta source complet depuis [MES-C2](../session_c2_provenance/README.md) est conservé par chemins dans la capture : il comprend A, B retenu, cache par défaut et terminaison, outre des changements du pool. La comparaison entre campagnes est descriptive et ne permet pas d'isoler un de ces changements. Le [raccord du défaut cache](../cache_defaut_raccord/README.md) et les limites du pic actif/RSS restent applicables.

Une capture persistante extérieure au dépôt conserve461 fichiers de sortie nécessaires aux lecteurs, dont254 JSONL (8 152 776 octets), et leur inventaire. Ce reçu ne duplique ni ces journaux ni les rapports complets, et n'archive aucune identité privée. L'admission MES-C et celle des deux appariements sont des preuves distinctes ; les lecteurs d'archives/sources déjà publiés sont réutilisés et hachés.

```sh
python3 -B check.py --repo DEPOT --session SESSION
python3 -B -O check.py --repo DEPOT --session SESSION
```

[SHA256SUMS](SHA256SUMS) couvre les quatre fichiers. La commande exige les objets Git et fichiers locaux fermés ; elle ne lance aucune sonde.
