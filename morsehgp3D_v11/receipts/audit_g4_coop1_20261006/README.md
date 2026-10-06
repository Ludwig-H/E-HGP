# Coop1 — CUDA conforme sur le périmètre joué, critère de vitesse refusé

Session `v11.20261006.claudecoop1` close : `completed`, worker 0, `DONE=0`, arrêt ciblé certifié le **6 octobre 2026 à 09:26:34.814 UTC**. Source exécutée `c3df818052494d62837834d257dbb28b89b4b074`, paquet de 638 fichiers utiles identique aux objets Git. Le reçu développeur publié dans `ee3eabe5e` correspond exactement aux pièces closes ; ses SHA256 sont valides. Ce commit publie aussi un correctif produit postérieur, qui reste hors de la qualification coop1.

Cadre : `exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`. Lecture locale de métadonnées uniquement ; aucun build, test natif ni accès cloud par l'auditeur.

- **Six portes hôte PASS**, noms et verdicts directement lus dans CTest : les quatre groupes `leaf_coop` et leur inventaire, puis `mhgp11_tower_full_leaf_lanes`. Aucun échec ni résultat manquant.
- **Seize prises synthétiques conformes**, sur A (3 000 sites 16 bits) et B (400 sites 21 bits), K5/feuille16 et K10/feuille24, CPU, GPU un fil, GPU coopératif et lot hôte coopératif. Dumps et registres égaux au CPU, lots égaux sur jobs/records/population/unresolved. B/K10 exerce **45 feuilles non résolues** et le repli ; A/K10 exerce 555 feuilles rejouées et 34 576 copiées.
- **Six Compute Sanitizer conformes** : memcheck A/K5 et B/K10, racecheck B/K5 et B/K10, synccheck B/K5 et B/K10. Codes 0, marqueurs zéro erreur/zéro hazard et même dump CPU directement contrôlés dans les résultats conservés. Portée synthétique u21 ; aucun transfert aux trames LiDAR ni à u24.
- **Trois trames entières sans sol**, 39 885 / 35 551 / 45 845 sites, K5/feuille16 et K10/feuille24, W48, modes CPU16379 / GPU81915 / coop212987 : **90 processus, 252 passes construites et 90 dumps canoniques contrôlés**. Les 162 passes chaudes intermédiaires sont toutes annoncées réussies ; leur dump et registre individuels ne sont pas conservés. Chaque processus froid et la dernière passe de chaque processus chaud ont le dump attendu. L'égalité de leurs registres complets est déduite du contrôle du juge épinglé et de l'absence de refus ; le rapport conserve les références, pas chaque ligne native de registre.

Les critères écrits dans le plan échouent sur **toutes les prises résidentes après la première**, par comparaison des intervalles bruts conservés, sans calcul de médiane :

| Critère | ng00 | ng01 | ng02 |
| --- | --- | --- | --- |
| K5, exécuteur coop/GPU un fil ≤0,5 | ratio toujours ≥1,008 | ≥0,846 | ≥1,291 |
| K10, domain coop/CPU ≤0,85 | ratio toujours ≥0,917 | ≥0,927 | ≥0,920 |

Les bornes sont `min(coop)/max(référence)` sur les passes résidentes 2..P ; elles suffisent à refuser le critère quelle que soit la réduction de ces observations. Ce résultat ne qualifie ni le contrat FULL 100 ms ni une cause exclusive de la régression. Aucune campagne mutants n'a été jouée dans coop1. Le correctif des tables/J2 publié après la session et la session coop2 conservent leurs propres preuves.

`summary.json` contient seulement hashes, inventaires, compteurs et bornes. Aucun octet LiDAR, dump natif, journal complet ou identité de compte n'est recopié. `replay.py` dépend de la session/archive locale close et des objets Git : `python3 -B replay.py` et `python3 -O -B replay.py` revalident hashes, fermeture, source, pièces publiées et résultats ; ils ne recalculent aucun HGP et n'appellent aucun service cloud. Ce reçu n'est pas une archive autonome.
