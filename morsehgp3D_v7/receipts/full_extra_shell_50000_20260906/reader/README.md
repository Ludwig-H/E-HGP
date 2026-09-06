# Vérification rationnelle des quatre coquilles à 50 000 points

Lecteur indépendant, sans import produit. `diagnostic_only=true`, `public_status=not_claimed`.
Deux lectures réelles, Python normal et `-O`, ont rendu 0 et des sorties identiques
(SHA256 `c4a066e620b7850b6b3f1937f5b6d92b027f763012a554f9d1fbbf5512cc3c81`).
Chaque lecture reconstruit l'entrée complète uniforme/graine 3/u16, vérifie son
digest, son ordre Morton et les identités, puis confronte **les 50 000 points à
chacune des quatre clés**. Les listes intérieur/coquille sont complètes pour
ces clés ; aucune complétude du catalogue de boules n'est revendiquée.

Toutes les clés sont primitives, de coefficient quadratique 1. Chaque coquille
contient trois points et possède exactement un support positif minimal : un
diamètre, avec les deux poids rationnels 1/2. Il n'existe aucun autre support
positif minimal q≤4 pour ces coquilles. Les rayons exacts sont respectivement
14352441/4, 2904043/2, 6675549/2 et 12622643/2.

| Indice de boule | q_min | Intérieur p | Coquille u | Quotient local dans K=1..10 |
| ---: | ---: | ---: | ---: | --- |
| 174406 | 2 | 3 | 3 | K≤4 : un groupe couvrant S ; K5 : deux groupes stricts vers un fermé ; K6 : aucun strict, un fermé ; K≥7 : vide |
| 254569 | 2 | 0 | 3 | K1 : un groupe couvrant S ; K2 : deux stricts vers un fermé ; K3 : aucun strict, un fermé ; K≥4 : vide |
| 996863 | 2 | 4 | 3 | K≤5 : un groupe couvrant S ; K6 : deux stricts vers un fermé ; K7 : aucun strict, un fermé ; K≥8 : vide |
| 1251653 | 2 | 9 | 3 | Un groupe strict couvrant S pour tous les K≤10 |

S désigne l'union de l'intérieur et de la coquille. Les deux composantes
strictes locales ont chacune pour couverture I augmenté du troisième point
de coquille et de l'une des extrémités du diamètre. Leurs identités restent
distinctes malgré leur recouvrement. Leurs représentants et toutes les
couvertures figurent intégralement dans `checks/normal.stdout`.

Ce tableau **ne donne pas les parents globaux** : des chemins extérieurs
peuvent déjà relier les composantes locales. Une naissance sans facette
stricte locale n'est pas renommée naissance globale. De même, l'inertie locale
de la boule p=9 pour K≤10 n'autorise pas à la supprimer : son ancre K=10 reste
nécessaire pour les cofaces faibles de cardinal 11, conformément au contrat
BALL_ANCHORS.

La comparaison avec l'ancienne capture GCP est conservée dans `comparison.json`.
L'entrée complète est identique (digest
`3f7c6dd47bcba4222e511c94f90aaeeeb80198b0d5ac8a6721e4ff55feedab3f`).
Pour Kmax=10, les 84 champs de configuration et 67 champs terminaux communs
hors mesures/threads/métadonnées nouvelles sont exactement égaux, y compris
le travail imbriqué : 21 685 604 candidats, 21 468 368 boules census, quatre
coquilles signalées, mêmes visites et mêmes pics logiques. Les différences
sont les mesures, les 8 contre 48 workers et les diagnostics ajoutés.
**L'identité individuelle des boules GCP n'est pas prouvée**, car ses anciens
bruts ne contiennent ni leurs clés ni un digest du catalogue. L'ancien Kmax=5
signale trois coquilles, compatible avec p+q_min≤6 pour les trois premières
clés locales, sans preuve supplémentaire d'identité.

Le processus local et les anciens processus GCP sont des refus explicites
`unsupported_degeneracy`, code 2, avant tout ordre FULL. Ce ne sont ni des
tours achevées ni des délais censurés par timeout. Le succès du lecteur
rationnel ne réétiquette aucun de ces refus.

Les captures synthétiques sont conservées séparément dans `synthetic/` :
quatre géométries, 80 masques, 165 facettes strictes, 34 mutants rejetés,
normal/`-O` identiques. Le premier échec de décompte des tests est conservé.
`preparation_README.md` décrit l'état historique avant lecture réelle, pas
une absence actuelle de cette lecture. Les chemins absolus des commandes
restent historiques. Depuis ce dossier, le lecteur lui-même est portable :

```bash
python3 -B read_extra_shell.py ../run_r3/n50000_k10.stderr --expected-n 50000 --mixed-stderr
python3 -B -O read_extra_shell.py ../run_r3/n50000_k10.stderr --expected-n 50000 --mixed-stderr
python3 -B selftest_reader.py
python3 -B -O selftest_reader.py
```

Le supplément ne contient aucun ELF ni nouvelle copie des bruts C++.
Son manifeste propre est `FILES.json` ; le manifeste global est laissé à ROOT.
