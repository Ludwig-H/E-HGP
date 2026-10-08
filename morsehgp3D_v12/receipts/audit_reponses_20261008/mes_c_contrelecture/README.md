# MES-C : contrelecteur préparé avant les résultats

Lecteur indépendant des cohortes, températures et statistiques MES-C, préparé sur le plan de la session `v12.20261008.mesc` et le Git **83ed7620d0b243343688a45473aa479240129725**. Aucune campagne lancée, aucune commande distante, aucun moteur exécuté. Ce lot contient des essais Python inventés ; il ne qualifie encore aucun résultat MES-C réel.

Le [plan](plan.json) est épinglé à `71ef953a…`, le paquet de sources à `86eb5522…`. Les dix sources utilisées ont été comparées octet pour octet entre Git et ce paquet ; son SHA-256 a été revérifié. Le [relevé](capture.json) contient seulement noms, étiquettes, nombres de sites et groupes du manifeste local `g4_small/bundle_manifest.json` (`bed8fa90…`). Aucun XYZ, ID, label sémantique ou tar de données n'a été lu, extrait ou copié. Le hash de l'archive `1b11650d…` est une déclaration du préflight : la liaison de ce manifeste local au contenu de cette archive n'est pas reconstituée ici par lecture du tar. La provenance extérieure et l'arrêt de session sont une vérification distincte.

## Cohortes et températures

Le plan demande 159 nuages : 132 réels, 15 synthétiques sains (5 par famille uniform/clusters8/slab), 12 difficiles (5 lattice, 5 sphere, 2 line). Les 147 nuages réels ou sains forment chaque Session. Les étiquettes `c000`…`c158` sont celles de leur position **dans le manifeste entier**, sans renumérotation après séparation des familles ; ce sont des noms de trames, pas des classes sémantiques.

- 12 Sessions : CPU/appareil × K5/K10 × 1/4/48 fils, chacune 147 trames × 3 tours, soit 5 292 passes.
- 48 processus difficiles : 12 cas × CPU/appareil × K5/K10, à 48 fils, deux passes chacun, soit 96 passes.
- Campagne complète : **60 processus, 5 388 passes dont 3 576 chaudes**. Chaque première visite d'une trame est écartée comme premier tour ; seules ses visites suivantes entrent dans la médiane. Cela ne signifie pas un processus froid pour chaque trame de la tournée. Retirer uniquement la première passe du processus serait faux.

Les noms de fichiers bruts, les clés des configurations et les lignes difficiles sont reconstruits depuis ces métadonnées et le plan. Une duplication, une ligne inconnue, des paramètres différents ou une ligne planifiée absente sont refusés. Une ligne explicitement `non_joue` reste une absence de mesure. K10 est informatif pour les critères C1/C2/C3 : son absence déclarée réduit la couverture, sans rendre artificiellement faux un critère K5. `cohorte_complete` distingue cette situation d'une campagne entièrement jouée.

La même distinction vaut pour une Session K5 non jouée hors de la configuration CPU/K5/W48 exigée par C1/C2 : les critères peuvent être tenus avec une couverture partielle. En revanche, **toute Session effectivement tentée mais non réussie**, y compris K10, ajoute un contrôle manquant conformément à `play_session` et rend le verdict global refusé. Un cas difficile K10 refusé ou expiré reste informatif ; une sortie illisible reste toujours un contrôle manquant.

C3 requiert les 24 paires (nuage difficile, voie) K5/W48, chacune complètement jouée. Un cas manquant, illisible ou non joué rend C3 **non évalué** et le verdict **refusé**. Une expiration, un échec ou un refus effectivement observé dans une cohorte renseignée rend C3 **non tenu**. C'est la correction proposée dans [la prélecture MES-C](../mes_c_prelecture/README.md), encore absente du pilote empaqueté ; une divergence de son verdict est donc publiée explicitement, sans altérer les temps bruts.

## Lecture des processus et statistiques

Les primitives `read_rows`, `check_full` et `parse_output` du [lecteur FULL commun contre-jugé](../lecteur_full_commun/README.md) sont chargées depuis leurs objets Git et hashes exacts. Ce réemploi est explicite ; les fonctions de lancement et de déballage du pilote ne sont jamais appelées. Le lecteur impose open/full/libération/exit, types u64 sans booléens, JSON ASCII sans doublon ni NaN, trame/sites/K/voie/fils/profil attendus, empreinte et inclusions de durées. Les gardes supplémentaires déjà justifiées ferment les types de codes, les couples statut/raison de `reasons.def`, les trois cohérences mémoire manquantes et le plancher d'entrées.

La sonde charge les trois tableaux u32 et les IDs u32 de toutes les trames avant les passes et les garde dans `frames` : le pic hôte compté doit être au moins `16 × somme(sites chargés)`. Les budgets séparés sont de 64 Gio chacun dans ce plan. Le pic appareil domine sa capacité finale ; l'épinglé est compté dans le pic hôte ; le pic de chaque étage suivant domine l'usage final précédent. On n'impose pas des usages croissants. Le pic est celui de `MemoryBudget`, **pas le RSS** ; `rss_max_octets` est cumulatif sur le processus. Les capacités CUDA résidentes et leur réemploi empêchent d'interpréter ces champs comme le coût isolé d'une trame. Aucune somme des sous-diagnostics C n'est ajoutée au mur.

Les hashes FUL1 sont comparés sur toutes les passes admises d'un même nuage, entre voies et nombres de fils, **à K égal**. Les tailles et paramètres restent contrôlés avant les statistiques. Les valeurs chaudes, les médianes CPU et les régressions sont recalculées depuis les bruts ; des tableaux publiés différents sont signalés, sans remplacer les observations par le tableau erroné.

L'OLS `t = a + b n` est recalculée en fractions séparément pour chaque configuration et chaque groupe. C1/C2 conservent les règles déclarées CPU/K5/W48/groupe réel : `a ≤ 2 000 000 ns` et `b ≤ 241 300 000 / 64 740 ns/site`. Le seuil est le ratio de deux médianes de la session K, avec le temps arrondi à 241,3 ms ; ce n'est ni une pente K ni nécessairement le temps/site d'une même trame. Un intercept ajusté n'est pas une mesure directe du coût fixe. Le maximum des résidus et des rapports individuels temps/site est publié comme diagnostic, **sans nouvelle règle d'adoption**. La comparaison des coefficients publiés tolère seulement l'arrondi flottant (`1e-12` relatif, `1e-6 ns` absolu).

## Codes, preuves et utilisation

`banc_full.run()` archive stdout, mais pas les codes de chaque sous-processus. Sans `--codes`, nous les **inférons conditionnellement du rapport et du pilote épinglé** : `ok ⇒ 0`, `refus ⇒ 2`, expiration et signal selon leur raison publiée. Un code non reconstructible rend le contrôle incomplet. Un fichier optionnel de véritables codes externes peut les remplacer ; son ensemble de noms doit être exactement celui des bruts. Aucun code inféré n'est présenté comme archivé extérieurement. Une expiration ne fournit pas de nouvelles mesures admises à partir d'un préfixe interrompu.

L'émetteur peut publier `cpu_ns=null` quand la métrique du système manque. Le lecteur commun épinglé le classe illisible avant les statistiques ; notre contrelecture conserve ce refus propre, sans convertir cette absence en zéro ni appeler une médiane de valeurs nulles.

Le lecteur vérifie aussi les hashes pilote/lecteur/archive déclarés, la forme du hash de sonde, les options exactes du plan, les champs Release/u21/CUDA du cache publié et un GPU connu vide avant/après. Cela ne certifie ni l'identité du binaire historique par recompilation, ni une isolation GPU continue. Fermeture du contrôleur, hashes de l'archive de résultats et authenticité des observations restent à joindre pour une qualification réelle.

```sh
python -B test_reader.py --repo /chemin/du/depot
python -B -O test_reader.py --repo /chemin/du/depot
python -B mutants.py --repo /chemin/du/depot
python -B -O mutants.py --repo /chemin/du/depot
python -B reader.py --repo /chemin/du/depot --rapport rapport_c.json --bruts brut
```

Le nominal synthétique réemploie explicitement le schéma de la fixture officielle, avec mémoires corrigées, puis les fonctions statistiques du pilote extraites par AST pour produire son rapport. Six nuages inventés donnent 160 passes/104 chaudes ; l'OLS indépendante retrouve exactement `a=1 015 000 ns`, `b=10 ns/site`. Les contre-JSON couvrent cohortes, températures, paramètres, mémoire, hashes, format, refus réels et métadonnées ; les six mutants de **notre lecteur** sont syntaxiquement valides et tués par ces témoins (aucune erreur d'import ou de syntaxe). Ce ne sont pas des mutants du moteur. [Résultats](results.json) identiques en Python normal et `-O`.
