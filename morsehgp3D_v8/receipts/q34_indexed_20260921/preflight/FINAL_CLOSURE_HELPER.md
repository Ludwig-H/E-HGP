# Préflight du helper de clôture finale

Premier appel de `python3 -B morsehgp3D_v8/receipts/q34_indexed_20260921/close_final_reads.py` : code1 avant lancement de tout lecteur, benchmark ou création de `FINAL_READBACK.json`.

Erreur du helper neuf, hors206sources : il testait la présence de `xml_sha256` dans une clôture LiDAR, alors que cette clé existe et vaut `null` hors régression. Il tentait donc de lire le fichier inexistant `lidar/lidar_2x8pm0nw/result.xml` et levait `FileNotFoundError`.

Correction du helper seul : contrôler `completion.get("xml_sha256") is not None` avant de lire le JUnit. Aucun reçu existant, source206, binaire ou mesure native n’a été modifié. La clôture suivante est la première exécution effective de ses lecteurs.
