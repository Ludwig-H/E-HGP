# Contre-épreuve minimale du lecteur compare_scale

`original/` est la copie exacte des 40 fichiers du paquet I620fdij : 37 fichiers manifestés, le manifest SHA `f1755305ea07e21b9017b6eeddd6613c3bc0735361a3d0c4f826e01fe4947bbc` et deux anciens journaux. Les anciens PASS sont **archive-only**, pas une validation LIVE.

`strictread.py` est conservé à l'identique comme source de lecture. Ne pas appeler son ancienne option `--replay`, qui écrit : le point d'entrée de cette annexe est **verify.py**, strictement sans écriture. Il vérifie l'empreinte externe du manifest, tous les 45 fichiers déclarés, l'absence d'extras, les pins internes, puis rejoue les huit commandes sur le snapshot. Un témoin valide et trois sorties invalides, en normal et `-O`, reproduisent six acceptations erronées. Timeout : 10 secondes par commande ; horaires UTC réels dans CAPTURE_LIVE.json et dans la sortie du lecteur. Les empreintes avant/après doivent rester identiques.

MUTATIONS.json garde uniquement les résultats épinglés des trois refus : manifest vide, cas omis et code faux ; les trois arbres jetables ne sont pas conservés ici. Les gardes sémantiques ont également été isolées pour les deux derniers cas.

Usage : `python3 -B verify.py SHA256_DU_MANIFEST_FINAL` ; puis `python3 -O -B verify.py SHA256_DU_MANIFEST_FINAL`.

Ce rejeu qualifie la contre-épreuve d'un lecteur sur une source figée, pas le produit actuel. redecide_refusion.py est seulement archivé, jamais exécuté. Aucun moteur natif, GCP ni fichier partagé modifié.
