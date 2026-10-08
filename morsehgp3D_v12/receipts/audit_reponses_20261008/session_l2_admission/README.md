# L2 : cinq prises CPU, deux réussites froides

Le [lecteur indépendant L2](../session_l2_contrelecture/README.md), préparé sur
JSON synthétiques avant ce rejeu, admet le rapport et les dix fichiers bruts du
lot **a2c2fccfd / commande 0**, sans condition résiduelle. Lectures normales et
`−O` identiques. La [provenance et l'arrêt](../session_l2_provenance/README.md)
sont contrôlés séparément : ce reçu n'exécute ni moteur, ni contrôleur, ni GCP,
et ne lit aucune coordonnée.

Plan `1f9e4599…`, archive **13 110 octets**, SHA-256 `52f1dded…`, rapport
`c1e55eae…` : hashes entiers dans [capture.json](capture.json). Le pilote
`e10a9cd4…` dépend du lecteur produit `3594c5d3…` ; notre validation indépendante
porte le schéma FULL 902 émis par `201119a7…`. Les codes individuels restent ceux
déclarés par le rapport épinglé. Aucun binaire n'est requalifié par ce rejeu.

Les cinq processus demandent **CPU, u21, K5, W48, une passe** : deux réussites,
trois refus, aucun échec illisible ni cas absent. Budgets commandés : 160 Gio
hôte, 88 Gio appareil ; les deux réussites CPU publient zéro capacité/pic
appareil et zéro capacité épinglée. La compilation avec CUDA et la capture de
l'environnement GPU ne sont pas une exécution FULL GPU.

| Cas | Sites déclarés | Mur froid (s) | CPU (s cumulées) | Issue |
|---|---:|---:|---:|---|
| Paris sans sol | 9 111 422 | 112,865790359 | 4 020,513234 | succès |
| Paris brut | 14 551 520 | 165,789026728 | 5 674,656369 | succès |
| Courtyard | 16 828 368 | — | — | unsupported_degeneracy / wide_leaf |
| Lyon sans sol | 24 016 862 | — | — | resource_exhausted / memory_budget |
| Lyon brut | 32 412 887 | — | — | resource_exhausted / memory_budget |

Aucune statistique chaude : médiane et maximum chauds restent `null`. Aucun
digest FULL demandé, les cinq cas dépassant le seuil de 1,6 M sites. L'admission
est structurelle et chronométrique ; elle n'établit aucune nouvelle identité
géométrique différentielle. **B1, B2, B3 et B4 sont non évalués** (voie appareil
pour les trois premiers, K10 pour le dernier). Le verdict automatique « non
tenu » faute de critères évalués ne mesure donc pas ces objectifs GPU.

Les étapes des deux réussites sont conservées exactement dans
[mesures.json](mesures.json), en ns. Pour Paris sans sol puis brut : C =
67,681448314 / 94,208502471 s ; G = 14,903743646 / 20,971437969 s ; TMVR =
29,635574971 / 49,577178674 s, dont T = 20,302690137 / 35,082908883 s.
Ces prises uniques décrivent deux scènes ; elles ne prouvent aucune croissance
asymptotique ni aucun gain entre versions. Les durées sont emboîtées :
P+C+G+raccord+TMVR laisse 940 / 1 040 ns dans le mur ; T+M+V+R laisse
872 492 367 / 1 378 922 949 ns dans TMVR. Ne pas additionner les sous-étapes C.

Le pic du budget hôte est atteint pendant C : **99 987 683 104 /
147 151 303 928 octets**, contre des usages du budget en fin C de
26 819 742 820 / 38 327 850 820 octets. Ce ne sont pas des tailles de catalogue
isolées. RSS maximal séparé : **100 025 561 088 / 148 233 908 224 octets**.
Les trois processus refusés n'émettent aucune passe FULL ni pic par étape ;
aucun étage responsable, profil mémoire ou durée FULL n'est inventé pour eux.

Rejeu, avec la whitelist de métadonnées extraite par l'audit de provenance :

```sh
python3 -B replay.py --repo /workspaces/E-HGP --plan PLAN_L2 --results DOSSIER_B --archive ARCHIVE_L2
python3 -B -O replay.py --repo /workspaces/E-HGP --plan PLAN_L2 --results DOSSIER_B --archive ARCHIVE_L2
```

Le rejeu contrôle les sources Git du lecteur, les hashes du plan, du rapport,
de l'archive et de l'inventaire des bruts, puis recalcule le résumé entier. Les
preuves historiques L1/L1r et leurs lecteurs restent inchangés. Aucun nouveau
constat ni changement d'état du registre dans ce reçu.
