# Session J — contrelecture des preuves, sources et adoption de Gc

Reçu développeur [`g4_t2cj_20261008`](../../g4_t2cj_20261008/README.md), livré par `d8c6f7164` ; snapshot moteur `10050a96e`. Audit de l'archive, sans appel GCP, build ni exécution HGP. Les chiffres de G concernent le CPU du produit sur la VM G4, au profil u21.

## Provenance et fermeture

- Les **211 fichiers** de SHA256SUMS sont présents, conformes et versionnés. Les **344 fichiers** source/CMake/sondes/tests/référence/pilote pertinents du manifeste déployé sont identiques au Git `10050a96e`, avec ensemble complet vérifié.
- L'archive du bras avant `45a69c21…` a été relue localement : ses **338 fichiers** correspondants sont identiques au Git `4df326cc8` (Gc1, empreinte COBJ). Les deux ablations du rapport sont celles du pilote livré. Le plan JSON externe porte bien `bcd75ced…` ; le hash distinct du worker porte le `plan.sh` rendu par le lanceur, pas ce JSON.
- Quatre commandes terminées code0, sans erreur/avertissement/troncature déclaré. Le reçu porte `closure=stopped`, arrêt ciblé certifié, `observed_after.status=TERMINATED`, dernier arrêt **01:27:51.429 UTC**. C'est la vérification des preuves archivées, sans nouvelle interrogation du cloud.
- Les journaux confirment **675/675** portes de socle, **6/6** LiDAR et le passage du gate de campagne des **18 mutants**. Les détails individuels de cette dernière campagne ne sont pas réarchivés dans ce reçu : aucun nouveau motif de mise à mort n'est attribué.

Les 675 noms du socle sont un sous-ensemble exact des 701 portes locales Gc : seules les 26 `mhgp12_reference_diff_v10*`, conditionnées aux binaires v10 figés, manquent sur J. La sentinelle LiDAR, sautée localement, passe sur J. Les périmètres diffèrent ; aucune autre porte perdue dans cette comparaison.

## Relecture du pilote

Normal et `-O` donnent les mêmes résultats. **188 journaux**, tous admis, avec résumés identiques aux données brutes : 150 K5 décisifs, 18 K10, 12 W1, 6 uniformes et 2 prises profil. Le protocole K5 est bien 3 trames × 5 bras × 10 processus × 10 passes ; une première passe écartée par processus. Le calendrier déterministe du pilote place chaque bras deux fois à chaque position. Cela n'établit pas l'absence de tout effet de report entre processus.

Les verdicts de REGLE_T2C sont reproduits : lot, G-L7 et index/corrections **adoptés** ; G-L5 à la place de G-L7 **rejeté**, aucun refus. Les statistiques sont comparées à quatre ULP au maximum : une seule borne bootstrap diffère d'une ULP dans notre environnement (`0.6602344717643851` contre `…52`), sans changement de verdict. La conversion tuple→liste propre à JSON est normalisée.

Le digest atteste l'objet final de chaque processus, pas chaque passe chronométrée. Les chronos portent sur G et excluent la préparation du catalogue ; les diagnostics Gc ne doivent pas être additionnés comme phases indépendantes lorsque `orders_ns` les enveloppe. Le README développeur additionne les sessions I et J : **73–91 ms est une estimation issue de mesures séparées**, jamais un chrono intégré C+G, encore moins FULL. Le contrat de 100 ms reste non acquis. Les profils instrumentés et les données K10 informatives ne partagent pas le protocole décisif K5.

## Rejeu

Les murs ont aussi été recalculés directement depuis les JSONL par
[`replay_mesures.py`](replay_mesures.py), séparément des résumés du pilote ; résultat dans
[`mesures.json`](mesures.json). La médiane ci-dessous est celle des médianes par processus.

| G CPU W48, ms | ng00 | ng01 | ng02 |
| --- | ---: | ---: | ---: |
| K5 avant, médiane | 79,143 | 62,850 | 76,656 |
| K5 après, médiane | 53,063 | 42,113 | 48,198 |
| K5 après, maximum des 90 passes chaudes | 54,758 | 42,945 | 50,177 |
| K5 après, maximum des dix médianes de processus | 53,748 | 42,309 | 48,645 |
| K10 avant, médiane | 619,458 | 452,538 | 521,407 |
| K10 après, médiane | 440,898 | 321,281 | 354,650 |
| K10 après, maximum des six passes chaudes | 444,490 | 321,714 | 357,795 |

K10 comporte trois processus de trois passes, première exclue. La valeur ng00 443 ms du README développeur
correspond à une prise ; le tableau agrège les trois. La comparaison avant/après K10 de J est préférable au
rapprochement descriptif avec H. Aucun verdict d'adoption K10 distinct n'est déduit de cette petite cohorte.

Le profil ng00 donne 29,29 % de cycles au census complet+saturé à W1 (deux passes chaudes), 28,75 % à W48
(quatre). Il s'agit de sommes de cycles de travailleurs sur les ordres 2..5, sans soustraction du coût de lecture
du compteur. Le mur instrumenté W48 est 61,10 ms, contre 53,06 ms dans le bras produit : ces parts orientent
l'analyse, sans borner un gain mural. Elles ne reprennent pas le 31–35 % non défini dans le README développeur.

```
python CHEMIN/replay_preuves.py --repo DEPOT --check
python -O CHEMIN/replay_preuves.py --repo DEPOT --check
python CHEMIN/replay_mesures.py --repo DEPOT --check
python -O CHEMIN/replay_mesures.py --repo DEPOT --check
```
Option `--external DOSSIER_EXTERNE` : revérifier également l'archive source avant, le plan et la comparaison du journal CTest local (chemins relatifs et hashes dans `pins.json`). Ces fichiers externes, les bruts et les patches ne sont pas recopiés. Les sorties strictes et la fermeture restent intégralement relisibles depuis les fichiers versionnés.
