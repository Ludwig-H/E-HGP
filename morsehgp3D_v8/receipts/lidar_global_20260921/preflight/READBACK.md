# Lectures du premier pilote — hors qualification finale

`edge_pilot12_12dv8ive` est clos PASS : 12 mesures, six paires d'ordre, trois arêtes originales sur les scans 0/100/200 à n8000, K5/10. Les 195 sources et les artefacts sont restés identiques pendant la capture. SHA-256 de sa clôture : `13878002cad1a63ebc6971968e994d3ae44ec09dc8751e5da4436f1766b74649`.

Les commandes suivantes ont ensuite été exécutées avec le runner 195 sources, chacune en Python normal puis avec `-O` :

- `read …/edge_pilot12_12dv8ive --check-live --compact` : **exit 1**, `InvalidReceipt: live source/build changed` ;
- `selftest …/edge_pilot12_12dv8ive` : **exit 0**, 26 corruptions refusées, 12 mesures réelles ;
- `read …/edge_pilot12_12dv8ive --compact` : **exit 0**, six paires d'ordre concordantes, 24 000 tests scalaires du census constructeur de cibles, sept fichiers d'entrée épinglés ; les 12 cibles sont conservées.

Le diagnostic après les lectures identifie uniquement `tests/wspd_q2_parallel_receipts_gate.py` comme changé depuis cette capture, aucun artefact. Ce changement est postérieur à sa fermeture. Le lecteur live a donc correctement refusé un ancien instantané ; les lectures historiques ne le promeuvent pas en qualification du nouveau.

Le runner exact utilisé est [archivé](run_q34_lidar_checks_195.py), SHA-256 `8902d1c7dab874c6508f656ba705895ca2aae97a43936803223a97d57d959fca`. L'ajout ultérieur de `tests/wspd_q34_mutations.py` porte l'inventaire du runner courant à 196. Pour relire ce préflight avec son ancien lecteur, utiliser `PYTHONPATH=morsehgp3D_v8/bench python3 -B …/preflight/run_q34_lidar_checks_195.py read …/edge_pilot12_12dv8ive --compact`. Les chemins abrégés désignent `morsehgp3D_v8/receipts/lidar_global_20260921/`.

Ces six premières lectures sont consignées ici depuis leurs sorties d'exécution, pas prétendues recapturées par un harnais de clôture. Aucun test GCP ou générateur global n'est couvert par ce pilote.
