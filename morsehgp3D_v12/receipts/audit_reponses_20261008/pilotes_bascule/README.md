# Bascule des pilotes MES-B, MES-C et MES-FULL

Contrelecture au commit **86d7e39d834cae86768d49a23d83bf53454ec2a3**, le 8 octobre 2026. Sources Git capturées, tests Python à sondes fictives seulement ; aucun moteur, compilation, GPU ou jeu licencié.

**Commandes et attentes sont raccordées aux deux modes.** Chaque pilote réinitialise son choix lors de `main` : défaut `recouvert`, ou `--sequentiel` transmis à la sonde avec attente `sequentiel`. Le choix sert aux Sessions et aux cas isolés. MES-B/C le publient dans `parametres.schema`, MES-FULL conserve l'option dans son rapport. Les trois portes officielles passent en normal et en −O : les deux modes, le retour au défaut après le mode séquentiel et le refus d'un schéma croisé sont exercés. Le nombre de contrôles de chaque porte est dans `capture.json` ; ces tests ne qualifient aucune voie native.

Les agrégations gardent leur objet : MES-C retire le premier tour **par nuage**, MES-B sélectionne sa passe chaude selon le plan, MES-FULL retire la première passe par processus ou le premier tour des Sessions. Les colonnes variables suivent le schéma ; les champs T/M/V/R du séquentiel ne sont pas inventés pour le recouvert. Dans ce dernier, `G` désigne l'intervalle depuis l'ouverture jusqu'au dernier calcul de G, et `TMVR` est la queue jusqu'à la fin ; G n'est ni la somme des travaux G ni leur temps CPU. Les sous-diagnostics C et les médianes d'étages ne sont pas additionnés.

## Défaut d'affichage distinct, correction d'une ligne

`MES-FULL.table` compte une colonne de trop pour son séparateur Markdown et ses prises absentes :

| Mode | En-tête / ligne renseignée | Séparateur / ligne absente |
|---|---:|---:|
| recouvert | 12 | 13 |
| séquentiel | 15 | 16 |

Cela rend la table Markdown structurellement incohérente ; les chiffres JSON, le jugement et les cellules des lignes renseignées ne changent pas. [tableau.patch](tableau.patch) remplace `columns = 11 + len(tail)` par `10 + len(tail)`. Le témoin vérifie les quatre lignes dans chaque mode ; après correction toutes ont le même nombre de colonnes et les en-têtes/valeurs sont identiques. Aucun correctif produit appliqué.

## Reproduction

```sh
python3 -B check.py DEPOT SNAPSHOT > lecture.json
python3 -B -O check.py DEPOT SNAPSHOT > lecture_O.json
cmp lecture.json lecture_O.json
# Optionnel : rejouer aussi les trois portes officielles avec leurs fausses sondes Python.
python3 -B check.py DEPOT SNAPSHOT --gates
python3 -B -O check.py DEPOT SNAPSHOT --gates
```

Les six exécutions officielles normal/−O ont été closes avec code 0 et sorties identiques par porte avant la rédaction du reçu. Le lecteur contrôle les 18 sources, leur égalité aux octets Git, l'application du patch dans un temporaire et les deux témoins de table ; `--gates` permet de reproduire les portes. Les sorties sont celles de `results.json`.

La [contrelecture des gardes LF](../lf_recouvert_gardes/README.md) reste distincte ; ce reçu ne la rejoue pas et ne prétend pas fermer tous les trous d'admission historiques. Il n'attribue aucun verdict à une campagne réelle avec le nouveau défaut.
