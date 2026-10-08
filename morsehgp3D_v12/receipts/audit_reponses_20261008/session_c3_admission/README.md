# MES-C3 : petits nuages, tour recouverte et cache par défaut

**Admission complète ; verdict « non tenu », C1/C2/C3 non tenus.** La Session épinglée à
`72f622a556130e25afdadbef84984619417286e2` demande **W4 et W48, pas W1**, CPU et appareil, K5 et K10.
La [provenance et la fermeture](../session_c3_provenance/README.md) sont contre-vérifiées séparément.
Ce reçu porte uniquement sur la commande MES-C ; les deux commandes appariées de la même Session ont leurs
propres cohortes et leur propre admission.

Même cohorte que MES-C2 : 147 nuages réguliers (132 réels, 15 synthétiques sains), trois tours par configuration,
premier passage de **chaque nuage** écarté. Douze cas difficiles séparés, deux passes par voie/K, à W48.
**56/56 processus joués, 48 succès et 8 refus, 3 608 passes complètes dont 2 392 chaudes.** Aucun contrôle
manquant, expiration, différence au rapport ou empreinte instable. Les huit refus restent les quasi-sphères de
3 000/10 000 sites, deux voies et deux K : `unsupported_degeneracy/wide_leaf`. Une passe refusée ne devient pas zéro.

Le plan ne transmet ni `--sequentiel` ni `--cache` : le produit joue donc la tour **recouverte** et le cache de
blocs hôte de **8 Gio**, défaut explicite de la sonde épinglée. Budgets hôte/appareil séparés de 64 Gio chacun.
L'ouverture CPU n'émet pas de ligne `open` ; ce comportement est prévu par le lecteur commun. La feuille reste
au défaut de la sonde, 24. Aucun de ces paramètres n'est déduit d'un chronométrage ultérieur.

## Temps FULL chauds

Chaque nuage porte la médiane de ses **deux passes chaudes** ; chaque cellule ci-dessous donne médiane puis
maximum de ces valeurs sur **147 nuages**, en ms. Ce maximum n'est pas le maximum des passes individuelles.

| K | fils | CPU : médiane / maximum | appareil : médiane / maximum |
| --- | ---: | ---: | ---: |
| 5 | 4 | 44,189 / 1 002,823 | 11,361 / 301,018 |
| 5 | 48 | **25,151 / 174,373** | **8,071 / 76,266** |
| 10 | 4 | 147,193 / 5 846,711 | 43,419 / 2 876,883 |
| 10 | 48 | **47,482 / 750,388** | **15,566 / 363,363** |

Sur les **132 réels seuls**, K5/W48 donne CPU **25,003 / 134,071 ms**, appareil **7,856 / 32,637 ms** ; K10/W48
CPU **46,420 / 515,627 ms**, appareil **14,571 / 199,354 ms**. Les vingt réels d'au plus 150 sites donnent, à K5,
CPU W4/W48 **6,075 / 10,884 ms**, appareil **4,051 / 4,262 ms**. W48 ne gagne donc pas sur cette petite sous-cohorte.
Une Session par configuration et deux chaudes par nuage ne fournissent pas une distribution inter-processus.

La régression des 132 réels CPU/K5/W48 donne une ordonnée à l'origine **14,902288 ms** (>2 ms) et une pente
**10,063622 µs/site** (>3,727217 µs/site). Ce sont des paramètres des moindres carrés, pas des coûts physiques
fixe/variable ni des plafonds par nuage. C3 est complet mais non tenu à cause des quatre refus K5.

## Mémoire et portée comparative

Maxima de `MemoryBudget::peak()` sur les passes chaudes régulières : hôte **1 899 340 308 octets**,
appareil **1 974 757 012 octets**, capacité épinglée **16 777 216 octets**. Le maximum RSS cumulatif processus est
**7 043 604 480 octets** (CPU/K10/W4). Ces mesures sont distinctes : le pic compté suit les réservations vivantes,
tandis que des blocs inactifs restent dans le cache physiquement retenu. Le RSS n'est ni le budget actif ni une
mémoire isolée par nuage ; les 147 entrées et des capacités réutilisables restent résidentes. Aucun dépassement
du budget annoncé n'est déduit du seul écart RSS/pic. Les maxima par configuration figurent dans `results.json`.

CPU·s dans les fenêtres des **294 passes chaudes** de chaque Session régulière :

| K | CPU W4 → W48 | appareil W4 → W48 |
| --- | ---: | ---: |
| 5 | 138,204 → 231,754 | 27,611 → 37,074 |
| 10 | 607,110 → 954,451 | 209,778 → 274,255 |

Ce n'est pas le CPU de tout le processus. Les fenêtres internes des tâches recouvertes ne sont pas des temps
CPU et ne s'additionnent pas au mur ; ce reçu ne les transforme pas en une partition temporelle exhaustive.

**1 176 comparaisons nuage/configuration** retrouvent les mêmes noms/sites/groupes et empreintes FUL1 que MES-C2,
K conservé. Les ratios sont seulement descriptifs : MES-C2 était séquentielle, plusieurs changements de produit
et de régime sont intervenus, dont A, B et le cache par défaut. Aucun gain causal isolé du cache, de A, de B ou
du pool, ni comparaison v11, n'est revendiqué. Les deux appariés du plan répondent séparément à la question du cache.

## Lecteur et rejeu

Le lecteur MES-C figé `55ca24fe` fournit les cohortes, températures, régressions et critères inchangés. L'adaptateur
visible de `check.py` modifie uniquement l'attendu en `schema=recouvert` et la continuité mémoire en `P,C,tour`.
Il vérifie aussi `parametres.schema`. Le lecteur FULL source `c3e9e0f4` est comparé à la proposition de gardes
d'horloges `15437e5f` : issues et résultats identiques sur tous les bruts. Dix fonctions de politique du pilote,
dont statistiques, critères et traitement des codes, sont identiques par AST à MES-C2. Les codes individuels
restent **inférés du rapport et du pilote épinglé**, non présentés comme des codes externes archivés.

Quatre contreflux issus d'un journal admis sont refusés : schéma séquentiel, tour hors mur, rupture de continuité
mémoire C→tour, trame étrangère. Aucun moteur n'est exécuté ; les JSONL sont des métadonnées de la Session close.
Ce n'est pas une nouvelle qualification universelle du lecteur FULL ni une preuve d'immuabilité temporelle de l'ELF.

```sh
python3 -B check.py DEPOT RETOUR_C3 PLAN_JSON RAPPORT_C2
python3 -B -O check.py DEPOT RETOUR_C3 PLAN_JSON RAPPORT_C2
```

`RETOUR_C3` contient `rapport_c.json`, `brut/` et les deux annexes de la commande MES-C. Inventaire de 59 fichiers,
pins Git, plan, lecteur, postimage de patch et rapport C2 sont vérifiés. Sorties identiques à `results.json`.
Aucun payload XYZ/IDs, compilation, moteur, contrôleur ou GCP utilisé par l'audit. Aucune modification produit.
Contrelecture statique indépendante favorable de latest_perf_e sur l'adaptateur et les frontières mémoire ;
aucun rejeu supplémentaire ni natif de sa part.
