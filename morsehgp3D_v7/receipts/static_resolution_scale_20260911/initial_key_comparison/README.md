# Comparaison indépendante des clés initiales et du plan statique à 8k

11 septembre 2026. Lecture seule de deux captures closes ; aucun nouveau
calcul géométrique, aucun changement de source ou de paquet scale, GCP
non utilisé. `public_status=not_claimed`.

L'observateur privé du moteur publié ad7ffd28 collecte R_K/U_K avant K1 et
cache. Le moteur statique collecte requests/unique hors K1. Comparaison
sur uniforme n8000, s8, K1..10, seed3, coord65536 : mêmes input/payload
digests, neuf égalités distinctes de R et de U à K2..10. Le zéro statique
à K1 est intentionnel et n'est pas confondu avec les 59 750 occurrences
et 8000 uniques réellement observés à K1.

Totaux K2..10 : R=10 396 562, U=5 176 885, 2 396 646 uniques résolus
par semis statiques. `comparison.json` contient les neuf lignes. Ce
contrôle compare exactement les **comptages**, pas le flux intégral des
clés statiques (non exporté), et ne devient pas un nouvel oracle de
complétude ou une preuve de performance.

`verify.py` normal et `python3 -B -O verify.py` vérifient fichiers,
fermeture/stabilité déclarée des deux runs, entrée, payload et chaque
égalité par K. Aucun ELF ou vendor redistribué. Les producteurs restent
qualifiés par leurs paquets respectifs ; ces copies de résultats ne leur
ajoutent pas une autorité géométrique.
