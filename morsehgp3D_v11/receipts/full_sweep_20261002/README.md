# FULL : classification et balayage, capture sweep2

Source exécutée `12f49d0cae6488f0076ef295a9f107920d7567db`, plan
`bench/plans/full_sweep_g4.json`. **Preuves cohérentes ; campagne en échec de
calendrier** : 13 succès K5, 11 omissions de budget, aucun essai K10.
Le code 0 du lecteur atteste cette cohérence, jamais une campagne conforme.

Le contrôleur a conservé `failed_remote`, `worker_exit_code=1`, `DONE=3`.
L'arrêt ciblé est certifié, les deux clés sont retirées, le verrou est libéré,
les résultats sont vérifiés et `errors=[]`. Génération exacte
`2026-10-02T15:44:39.382-07:00`, observation arrêtée à
`2026-10-02T16:05:22.893-07:00`. Trois commandes closes : matrice 600 s,
ASan18 180 s, FULL 680 s ; aucun groupe résiduel tué ni flux tronqué.

La matrice a passé **2 229/2 229** portes : Release 430, ASan/UBSan B24,
TSan B21, B21 et B24 chacun 355, poison 356, mutants 21, style 2.
Clang est absent selon le contrat optionnel. Le supplément ASan/UBSan B18
`num;index;tower` a passé **158/158** portes. Les 221 mutations sont recoupées
avec les manifests de cette source et les sorties intégrales archivées :
219 morts par code/ligne, deux échecs de construction explicitement attendus,
aucune mort par signal ou délai. Les portes classification, absence d'allocation,
balayage, publications transactionnelles et codecs sont exigées explicitement.

Le calendrier demandait 24 unités : trois processus frais K5 et un K10 pour
chaque trame/profil, W48, feuille16, mode3 (cache J2 et tri indirect), réserve
native 8 Gio, délai enfant 60 s. Il termine sa boucle de collecte après
530,229 s ; ses 11 omissions satisfont la garde de budget 600 s qui réserve
80 s avant lancement. Les 13 processus payés cumulent 221,275 s et le
décodage Python 308,462 s. Ces sommes expliquent le calendrier, sans être
des chronos FULL. Il reste trois répétitions pour ng00/B21 et deux pour
chaque autre couple ; aucun groupe demandé n'a ses six succès K5.

Temps FULL API, en secondes, sur les mêmes entrées entières sans sol à 1 mm :

| Trame de la séquence08 | Sites | Profil | Essais | Médiane FULL3 | Médiane sweep2 | Pic Buffer (octets) |
|---|---:|---:|---:|---:|---:|---:|
| 000000 | 39 885 | B21 | 3 | 21,264385 | 18,973686 | 313 429 756 |
| 000000 | 39 885 | B24 | 2 | 21,684407 | 19,131355 | 332 569 532 |
| 000100 | 35 551 | B21 | 2 | 15,249491 | 13,541093 | 264 666 124 |
| 000100 | 35 551 | B24 | 2 | 15,410777 | 13,693912 | 280 963 268 |
| 000200 | 45 845 | B21 | 2 | 18,556377 | 15,972769 | 332 796 800 |
| 000200 | 45 845 | B24 | 2 | 18,904410 | 16,130066 | 352 856 536 |

FULL inclut index, catalogue/lookup et forêts/verticales ; lecture, Cloud,
Pool, préparation de la grille et segmentation sont séparés. Les réservations
incluent Cloud, sans mesurer RSS ni le décodeur Python. B21/B24 emploient les
mêmes coordonnées du domaine18, sans gain de précision d'entrée. Ce sont
trois trames d'une même séquence, pas trois séquences ; aucune revendication
sur trames brutes, poids multiples, projection des points, clustering ou GPU.
Le jalon FULL 200 ms reste ouvert.

Les 13 enregistrements de sortie sont identiques à ceux de
[FULL3](../full_20261002/README.md) : SHA brut apparié au même profil,
SHA sémantique, tailles et topologie complète déclarée par le codec.
Le lecteur rejoue d'abord le lecteur LIVE FULL3, puis compare neuf compteurs
conservés : cellules, cellules rejouées, plateaux, traces, unions,
continuations, pas de descente, descentes verticales et vérifications des
enfants. Les nouveaux compteurs de classification/balayage sont validés
séparément ; les anciennes remontées d'ancêtres ne sont pas comparées à zéro.
FULL3 était en mode catalogue0 ; sweep2 ajoute aussi cache/tri. Cette
comparaison ne constitue donc pas une ablation isolée du classificateur ou
du balayage.

Pour ng00/B21, première répétition : classification 0,065509 s,
naissances 0,265058 s, plateaux 13,398348 s et verticales 1,974770 s.
Ces phases disjointes non exhaustives sont internes à la forêt. La
classification examine 223 combinaisons sur 4 480 327 déclarées ; les
descentes exécutent encore 6 016 334 pas et 63 994 685 présentations MEB
de parties. Ce diagnostic ne prédit aucun gain d'une optimisation future.

Le mode **LIVE** exige les reçus bruts locaux de sweep2 et FULL3 sous
`/workspaces/.ehgp-sessions/`. La seule archive originale sweep2, son reçu
filtré et les copies compactes `matrix.json`, `asan18.json`, `full.json`,
`inputs.json` sont conservés ici. Le manifest d'entrée a été recopié depuis
le staging après vérification exacte de son SHA et de sa taille uploadés.
Aucun payload KITTI n'est versionné. Les cinq helpers `source_12f49/`
sont épinglés depuis Git ; le WIP `full_campaign.v4` n'est jamais importé.
`source_contract.json` ancre archive, source, plans et comparaison FULL3.

L'archive est lue par `extractfile`, sans extraction. Tous ses membres et
leurs SHA sont recoupés. Les 13 gros payloads FULL, totalisant 3 912 988 962
octets, ont été supprimés sur la VM : **leurs SHA sont enregistrés, pas
recalculés ici**. Un payload qui serait présent est réellement rehaché et
décodé ; ce chemin est couvert par une minuscule fixture binaire autonome.
Le codec contrôle la structure et les rationnels, sans prétendre refaire
un oracle géométrique exhaustif sur les grands nuages.

```sh
python3 morsehgp3D_v11/receipts/full_sweep_20261002/check.py
python3 -O morsehgp3D_v11/receipts/full_sweep_20261002/check.py
python3 morsehgp3D_v11/receipts/full_sweep_20261002/check_selftest.py
python3 -O morsehgp3D_v11/receipts/full_sweep_20261002/check_selftest.py
```

Lectures normal/−O identiques ; auto-test : **6 témoins, 72 corruptions
refusées, zéro exécution native**. Les contrôles incluent calendrier,
profil/mode, compteurs/timings avec flux JSON concordant, comparaison
historique, fermeture du brut/compact altérés ensemble, source épinglée,
mutant tué par signal et véritables octets du petit payload. Les métadonnées
de ces lectures sont dans `check_selftest.json`. Ce lecteur est borné à la
capture sweep2 ; il ne qualifie pas un schéma ou une campagne futurs.
