# Banc CPU/GPU au commit publié 61da03749

Sources exactes du commit `61da03749344a6acc4fea2b9eee875606cc857e8`.
Cette capsule actualise les sources au pin 61da après la revue 77db.
Les quatre fonctions du protocole sont identiques en AST ; le changement
cherche nvcc et journalise sa version avant build. La télémétrie FULL gagne
des fins de tâches, sans changer la sérialisation de la dernière passe.
Par rapport au WIP de 14:50 : le banc contrôle désormais
le ledger du catalogue en plus du dump final. Les quatre contre-cas restent
acceptés avec un ledger final identique. Aucun sous-processus natif, CUDA,
outil Nsight, téléchargement, VM ou octet KITTI n'est utilisé.

**73 gardes Python standard**, normal et optimisé (`-B -S` / `-B -O -S`),
sorties identiques. L'AST du banc est exécuté avec `run` remplacé par une
fabrique de stdout/dumps synthétiques dans un répertoire temporaire :
le faux binaire n'est jamais exécuté.

| Témoin | Résultat du banc actuel |
|---|---|
| 1 prise froide, 3 passes chaudes complètes | conforme, mesures présentes |
| 0 prise froide, 3 passes chaudes | conforme, aucune médiane froide |
| 1 prise froide, 1 passe dite chaude | conforme, six médianes chaudes nulles |
| 1 prise froide, 3 passes demandées, lignes pass absentes | conforme, six médianes chaudes nulles |

Pour revendiquer les deux régimes, exiger `reps >= 1`, `warm_passes >= 2`,
les lignes de passes exactement 1..P et leur succès. Un mode de diagnostic
partiel reste possible avec une portée et un verdict explicites.
L'absence simulée de lignes ne démontre pas que le producteur natif les
omet : elle montre que le lecteur ne protège pas ce contrat.

`full_probe.cpp` sérialise seulement la dernière passe : le hash de cette
passe ne certifie pas l'identité des passes intermédiaires chronométrées.
Pour une qualification de chaque passe, comparer aussi leur sortie canonique
hors chrono FULL ; pour un diagnostic, annoncer la portée « dernier dump ».
Le périmètre FULL inclut construction index/domaine, initialisation CUDA,
transferts, comptage, écriture et retour. Préparation Cloud/Pool et dumps
restent hors FULL. Les temps sous Nsight doivent rester des diagnostics,
séparés des prises ordinaires du contrat de temps.

L'ordre CPU/GPU est correctement alterné pour deux modes. Le contrôle du
ledger final et le checkpoint après chaque prise sont favorables. La reprise
d'un binaire existant est annoncée et hachée ; son hash seul ne prouve pas
son profil ou sa configuration CUDA, à recouper lors de la qualification G4.
Aucun défaut de sortie native n'est établi.

Rejeu : `python3 -B -S check.py`, `python3 -B -O -S check.py`,
puis `sha256sum -c SHA256SUMS`.
