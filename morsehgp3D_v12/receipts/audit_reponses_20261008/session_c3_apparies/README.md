# MES-C3 — deux comparaisons appariées sur petits nuages

Audit Codex, 8 octobre 2026. **198 journaux / 1 836 passes admis**, mais les
deux campagnes appariées restent **refusées par leur contrôle A/A** sur
`p5000`. Ce refus reproduit exactement la règle et les rapports originels ;
aucun bras n'est adopté ou rejeté statistiquement. Les temps ci-dessous sont
des descriptions de prises conformes, pas une décision de cache sur les petits
nuages. Aucun moteur, build, appel GCP ni payload XYZ/IDs lu par ce reçu.

## Périmètre exécuté

Source `72f622a556130e25afdadbef84984619417286e2`, plan `61b2ed6f…` : commandes
1 et 2, respectivement appareil et CPU, **u21/K5/W48**, dix tours de dix passes.
Le binaire FULL de ce pin utilise par défaut un cache hôte de **8 Gio**.
Les bras sont donc `ref=[]`, `aa=[]`, `sans_cache=[--cache=0]` : deux prises
du défaut avec cache et une ablation sans cache. Aucun bras W1, ni choix de
seuil de parallélisation à partir de ces campagnes. Le texte historique du
pilote qui décrit le défaut « sans cache » ne décrit plus le source exécuté.

| Étiquette du plan | Entrées déclarées | XYZ (octets) | IDs (octets) |
|---|---:|---:|---:|
| p150 | 156 | 1 872 | 624 |
| p1000 | 1 013 | 12 156 | 4 052 |
| p5000 | 4 618 | 55 416 | 18 472 |

Les étiquettes ne sont pas des effectifs exacts. Les tailles et hashes déclarés
proviennent des métadonnées de [provenance C3](../session_c3_provenance/README.md),
pas des coordonnées ni d'une déduction depuis les noms. Les effectifs de sites
émis égalent ces nombres d'entrées ; les tailles seules ne sont pas une preuve
indépendante de l'absence de doublons géométriques.

Chaque voie comporte 9 prises d'identité de deux passes, puis 90 prises de
dix passes : **99 journaux, 918 passes, dont 810 chaudes décisives**. Toutes les
prises d'identité ont une empreinte FULL constante par cas et identique entre
les trois bras. Ces empreintes sont également identiques entre CPU et appareil.
Aucune Session de 37 trames ni autre cohorte informative n'était commandée.

## Rejeu et jugement

Les sources Python exécutées sont identiques à celles de M : pilote
`fe22a681…`, lecteur FULL `c3e9e0f4…`, auxiliaire `7d7f0661…`. Le rejeu applique
en copies temporaires la [composition d'admission](../apparie_livraison/composition.patch)
et les [gardes FULL](../lf_recouvert_gardes/proposition.patch). Il ferme d'abord
la cohorte dérivée du plan, les configurations, les ordres de rotation, les
hashes/noms de chaque journal et toutes les identités. Il reconstruit ensuite
chaque résumé de prise, les médianes, les ressources et le jugement. Les
deux vérifications d'environnement GPU sont admises pour la voie appareil.
Le CPU n'est pas soumis rétroactivement à une obligation d'isolation GPU.

La règle reste : moyenne géométrique des rapports appariés par tour ; 10 000
rééchantillonnages, graine `20261008`, mêmes quantiles ; A/A dans ±1,5 % sur
chaque cas. Les valeurs A/A sont :

| Voie | p150 | p1000 | p5000 |
|---|---:|---:|---:|
| Appareil | 1,0003371011 | 1,0015777559 | **0,9730011725240592** |
| CPU | 0,9922476693 | 0,9885907481 | **1,0154739474493801** |

Le dernier rapport CPU est strictement supérieur à **1,015**. L'arrondir à la
borne changerait à tort le verdict. Les deux cas A/A invalides suffisent à
refuser leurs campagnes ; les IC du bras `sans_cache` restent publiés à titre
de résultats calculés, sans adoption/rejet. Le rejeu indépendant, le juge
corrigé et les deux rapports concordent, **sans aucun écart d'ULP observé**.
La tolérance de rapprochement flottant héritée de M (au plus 2 ULP) ne touche
ni les seuils ni les égalités bruts/résumés.

Médiane des dix médianes chaudes de processus, en ms ; la première des dix
passes est exclue dans chaque processus :

| Voie / bras | p150 | p1000 | p5000 |
|---|---:|---:|---:|
| Appareil / ref 8 Gio | 4,850620 | 8,295735 | 15,714660 |
| Appareil / A/A 8 Gio | 4,850885 | 8,295195 | 14,847000 |
| Appareil / sans cache | 4,835990 | 8,260001 | 15,475195 |
| CPU / ref 8 Gio | 10,748150 | 27,620931 | 58,828761 |
| CPU / A/A 8 Gio | 10,564725 | 27,522641 | 59,837410 |
| CPU / sans cache | 10,712965 | 27,482701 | 58,420225 |

Le rapport de deux nombres de ce tableau n'est pas la moyenne géométrique des
rapports appariés utilisée par le juge. Les froides, CPU, étapes et ressources
complètes sont dans [results.json](results.json), sans somme des fenêtres
recouvertes. Les budgets sont actifs et partagés hôte/appareil ; les capacités
appareil/épinglées sont vérifiées contre leur pic. Le RSS est un maximum cumulé
du processus hôte, pas la VRAM ni le budget actif ; les blocs hôte inactifs du
cache restent hors de ce budget. Les capacités appareil sont nulles côté CPU.

La [provenance extérieure](../session_c3_provenance/README.md) qualifie archive,
sources, commandes et arrêt. Les ELF d'ouverture déclarés diffèrent entre ces
deux builds : `cd640e73…` appareil, `6e03dbb2…` CPU. Le pilote exécuté ne publie
pas d'empreinte ELF finale et aucune preuve extérieure de fermeture n'a été
retrouvée. Cela reste une limite distincte ; aucune substitution n'est observée
et aucun nouveau veto n'est ajouté après la campagne. Les codes internes des
prises restent ceux consignés par le pilote, différents des codes externes
des trois commandes de session.

## Reproduction

[capture.json](capture.json) épingle les quatre sources, les lecteurs, le plan,
les deux rapports et les inventaires exacts de journaux. Les métadonnées d'entrée
sont rapprochées du reçu de provenance sans lire leurs fichiers de coordonnées.
Le lecteur réutilise les seules fonctions de calcul et de validation épinglées
dans [session_m_apparie/check.py](../session_m_apparie/check.py) ; sa cohorte est
propre à C3. Normal et `-O` produisent exactement [results.json](results.json).

```sh
python check.py --repo DEPOT --returned DOSSIER_RETURNED_C3 --plan PLAN_C3_JSON \
  --provenance ../session_c3_provenance/capture.json --check
python -O check.py --repo DEPOT --returned DOSSIER_RETURNED_C3 --plan PLAN_C3_JSON \
  --provenance ../session_c3_provenance/capture.json --check
```

Aucun ancien reçu, rapport de campagne, registre ou fichier produit modifié.
