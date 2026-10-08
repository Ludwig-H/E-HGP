# MES-B1p — FULL massif après C, A, B et cache

Audit Codex, 8 octobre 2026. **18 processus, 15 scènes, 10 succès et 8 refus
`memory_budget` ; 18 passes complètes dont 8 chaudes.** Relectures normal/−O
identiques, bruts admis sans condition résiduelle et rapport concordant.
B1/B2/B4 restent **non tenus**, B3 **non évalué**. L'admission de ces mesures
ne qualifie pas les objectifs massifs ni le contrat SemanticKITTI à 100 ms.

Source `c648b3857c83ec1ed174139eef36106b187d2cf0`, pilote `9eb04408…`, lecteur
FULL renforcé `15437e5f…`, commande 0 du plan `782c6e6a…`. L'[audit de provenance](../session_b1p_provenance/README.md)
porte l'archive, les 359 sources, les codes extérieurs et l'arrêt certifié.
Aucun moteur, build, contrôleur, GCP, XYZ ou IDs lu/exécuté par ce reçu.

## Mesures et régime

u21, W48, tour **recouverte**, catalogue appareil sauf la prise CPU explicite ;
budgets séparés 160 Gio hôte et 88 Gio appareil ; cache hôte par défaut **8 Gio**.
Les 45 déclarations d'entrées sont identiques à L1r selon la provenance ; la
cohorte, les étiquettes uniques et les effectifs attendus restent fermés par le plan.
L'empreinte FUL1 est demandée seulement jusqu'à 1,6 M sites. Ses trois scènes
réussies concordent entre passes **et avec L1r** ; Boreas n10 sans sol concorde
aussi entre catalogue CPU et appareil. Au-dessus du seuil, l'admission est
structurelle/chronométrique : aucune nouvelle identité FULL n'est inventée.

| K5, catalogue appareil sauf mention CPU | Sites | Dernier FULL (s) | Régime | L1r antérieure (s) |
|---|---:|---:|---|---:|
| Boreas n1 sans sol | 146 316 | 0,319914 | chaude | 0,519050 |
| Boreas n1 brut | 215 665 | 0,450011 | chaude | 0,699520 |
| Boreas n10 sans sol | 1 513 483 | 7,298151 | chaude | 10,024965 |
| Même scène, catalogue CPU | 1 513 483 | 20,056203 | chaude | 22,344450 |
| Boreas n10 brut | 2 153 342 | 9,392870 | chaude | 13,001688 |
| Marseille sans sol | 2 465 285 | 8,348700 | chaude | 10,955437 |
| Scion sans sol | 3 439 371 | 30,981949 | chaude | 41,951244 |
| Meadow scan1 | 6 181 091 | 30,564533 | chaude | 35,324820 |
| Scion brut | 3 589 247 | 31,818153 | froide | 42,829632 |
| Marseille brut | 6 709 045 | 26,602018 | froide | 31,548426 |

Chaque cas chaud n'a **qu'une** seconde passe, dans un seul processus ; médiane
et maximum chauds y sont la même observation. Les deux dernières lignes n'ont
qu'une passe froide. Les comparaisons L1r/B1p sont descriptives : 39 fichiers
natifs ont changé, plusieurs leviers et le cache sont réunis, les campagnes
sont distinctes. Elles n'attribuent aucun gain au seul recouvrement, au census,
au catalogue ou au cache, et ne sont pas un essai statistique apparié.

Les six refus K5 et deux refus K10 sont les mêmes qu'en L1r, tous sans passe
FULL ; aucun cas non joué, délai expiré ou sortie illisible n'est masqué.
Leur exit publie `resource_exhausted/memory_budget`, sans étage, côté du budget
ni pic d'échec permettant de les attribuer. Ils ne définissent pas une frontière
universelle à 5 M sites : **Meadow 6,181 M et Marseille brut 6,709 M réussissent**.
Les séries Boreas manquent leur prise n50 réussie, donc B3 reste non évalué.

## Temps et mémoire effectivement publiés

[mesures.json](mesures.json) conserve tous les résumés et, pour chaque dernier
succès, P/C/G/queue, les fenêtres, les dates par ordre, les six postes C,
`recouvrement`, mémoire et coûts hors mur. En mode recouvert, **G est le temps
jusqu'au dernier calcul G et TMVR la queue après G** ; les fenêtres T/M/V/R sont
des sommes de tâches qui se chevauchent, ni des durées murales disjointes ni des
temps CPU. Les dates par ordre ne démontrent pas à elles seules la cause du
chemin critique. Il ne faut pas comparer la queue au TMVR séquentiel de L1r
comme s'il s'agissait du même poste.

Exemples d'une passe chaude : Boreas n10 sans sol appareil C/G/queue =
**1,331 / 2,928 / 2,952 s** ; même scène CPU C = **13,959 s**, mur **20,056 s** ;
Meadow appareil C/G/queue = **2,720 / 7,571 / 19,783 s**. Ce sont les composantes
des passes individuelles, pas une somme de médianes indépendantes.
Validation, empreinte et libération restent hors mur. Tous les CPU/RSS sont
renseignés ; CPU sur la voie appareil désigne le cumul du processus hôte.

Les contrôles de mémoire ferment usage≤pic, transitions P→C→tour, maximum des
pics = `pic_octets`, entrée résidente, épinglé≤pic hôte et capacité appareil≤pic
appareil, chacun dans son budget. La voie CPU conserve des capacités/pics
appareil et épinglé nuls. Pour Scion brut : pic actif hôte **79 198 502 707**,
pic appareil **87 152 692 652**, RSS hôte **81 545 334 784 octets**. Ces maxima
ne constituent pas un pic physique simultané obtenu par addition.

`core/buffer.hpp/.cpp` au pin définit **used/peak** pour les blocs vivants,
avec arrondi de classe ; **held** inclut aussi les blocs inactifs, reste borné
par la limite et peut provoquer leur éviction. La sonde publie used/peak mais
**pas held ni le pic inactif du cache**. Le plafond du cache 8 Gio est une
configuration, pas son occupation mesurée. Le RSS hôte inclut d'autres allocations
et n'est ni held, ni la VRAM, ni un relevé après libération. Les observations GPU
avant/après ne prouvent pas une isolation continue entre elles.

## Port et reproduction

[check.py](check.py) réutilise le lecteur indépendant L1r immuable pour la cohorte,
les issues, les codes, les statistiques et B1–B4. Son port est explicite et en
mémoire : commande 0, paramètre `schema`, hash du pilote actuel ; la validation
d'une ligne FULL utilise LF15437 épinglé, puis ajoute les bornes mémoire du plan.
Chaque flux est aussi relu par LF complet ; les deux lectures et le rapport
concordent. Huit corruptions ciblées du raccord nouveau sont refusées
(types, schéma, digest demandé, capacités/budget et date G) ; il ne s'agit pas
d'une nouvelle qualification générale du lecteur ni du moteur.

Les dépendances, six sources Git, plan, rapport, métadonnées et inventaire des
36 bruts sont hachés dans [capture.json](capture.json), vérifiés avant/après.
Le lecteur n'invente aucun code absent : ceux des processus viennent du rapport
épinglé. Son champ `qualification_campagne=false` sépare son admission JSON de
la qualification extérieure apportée par le reçu de provenance.

```sh
python check.py --repo DEPOT --returned DOSSIER_B1P --plan PLAN_B1P \
  --metadata ACTIVE_METADATA_JSON --check
python -O check.py --repo DEPOT --returned DOSSIER_B1P --plan PLAN_B1P \
  --metadata ACTIVE_METADATA_JSON --check
```

Aucun reçu historique, produit, registre ou note active modifié.
