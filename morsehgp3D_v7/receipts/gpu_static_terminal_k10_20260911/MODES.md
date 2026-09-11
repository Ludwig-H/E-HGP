# Incident de transport du mode exécutable

La tentative `cuda_trial/nvcc_r1` est fermée en échec : `nvcc_strict_host.py: Permission denied`, avant prétraitement du compilateur hôte. Le paquet portable K2..8 conserve les contenus mais pas les modes Unix. Son import mécanique a donc repris l’adaptateur en mode `0666`, contre `0700` dans l’ancien arbre d’exécution.

Seul le mode de `build/v7_gpu_terminal_k10_20260911/nvcc_strict_host.py` est restauré à `0700` avant `nvcc_r2`. Son contenu reste exactement SHA-256 `994d9e6970797594efa2d333275594ef2085cdd166b345b0000135e94e395fb7`. Le snapshot et les logs de r1 restent intacts. Aucun contenu source qualifié par O2/SAN n’est modifié.

Une extraction portable destinée à compiler avec NVCC doit restaurer explicitement le bit exécutable de cet adaptateur, après validation des hashes. Cette opération ne constitue ni un nouveau compilateur ni un relâchement des options strictes.

Le paquet privé de préparation `packet/`, manifeste `41c5fc7d1a7dd3db95a94fd1fdd275237529c6011571afe738172647259d1257`, est conservé : ses modes étaient déclarés dans le manifeste, sans observation `stat()` par le publieur. La révision `packet_r2/` ajoute cette observation explicite des deux snapshots immuables et vérifie les flags SAN/LSan dans le lecteur. Aucun calcul géométrique, aucune capture ni aucun contenu source ne change ; le premier manifeste est conservé dans `history/packaging_r1_manifest.json`.
