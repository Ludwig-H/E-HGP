# CST-0009 : repli des feuilles non résolues

**Clôture proposée dans la portée d'origine** : le repli `unresolved` des feuilles est désormais distribué sur le
Pool et ses tampons de calcul/sortie sont comptés. Cette lecture du 8 octobre 2026 n'ajoute aucun test natif ni
mesure. Elle réutilise la [session I publiée](../../g4_t1bi_20261008/README.md).

Ancienne cause : v11 `ac081a06f`, `src/catalogue/single_pass_batch.cpp:142–165`, une boucle sur toutes les feuilles
non résolues appelle `enumerate_leaf` successivement, avec un seul espace de travail. Nouveau chemin livré par
T1-b (`8ba7d7287`, feuilles finales `c903774b1`), relu à `9c5656029` :

1. `device_pipeline.hpp:48–63` compacte puis rapatrie uniquement les feuilles non résolues et leurs sites dans des
   `Buffer`. `device_driver.hpp:69–81` conserve l'ordre des groupes de profondeur et appelle `LeafStage::consume`.
2. `leaves.cpp:17–19,252–303,306–311` découpe les groupes en lots d'au plus 16 384 feuilles. Les deux passes
   `count_body` et `fill_body` sont distribuées par `parallel_for(n,4,...)`. Les espaces de travail et compteurs
   sont privés par worker ; les positions d'écriture sont fixées par les comptes et préfixes, avant l'écriture.
   Le contrôle des issues précède la réservation des boules et incidences du lot.
3. Les espaces de travail, cases d'émission, comptes, préfixes et sorties sont des `Buffer` liés au budget commun.
   Chaque réservation est gardée ; le calcul des préfixes et des totaux contrôle ses débordements. Le raccord
   retient la première profondeur fautive avant d'admettre et écrire les sorties appareil (`device_pipeline.hpp:113–138`).

Les groupes successifs, préfixes, copies et assemblages ne sont pas tous parallèles ; une feuille seule demeure
une seule tâche. Il s'agit de supprimer l'obligation de rejouer **toutes** les feuilles dans une boucle sérielle,
pas de promettre une accélération proportionnelle au nombre de fils.

## Qualification déjà publiée

Sept sources déterminantes sont identiques entre le pin relu, le pin de la session `d2f39fe82` et son manifeste.
Le reçu et ses traces sont hachés dans [capture.json](capture.json).

- La porte `pipeline_witnesses`, passée dans la suite I, exerce les deux causes de reprise sur l'exécuteur Pool :
  étendue hors politique native et plus de 32 sites. Son témoin `coquille48` exige au moins huit reprises, exactement
  huit reprises de largeur, et au moins une réécriture hôte (`device_pipeline_test.cpp:94–104`).
- Le journal `device_open.log` atteste **la vraie voie GPU**, neuf témoins et **217 contrôles, zéro échec**. Le test
  utilise quatre workers hôte, compare CPU/appareil puis deux appels résidents successifs, y compris sur
  `coquille48`. Il compare empreinte de catalogue, grand livre, niveaux, table S* et diagnostics physiques
  (`device_support.hpp:99+`, `device_pipeline_test.cpp:158–182`). Le nombre de reprises de chaque témoin n'est pas
  imprimé dans ce journal ; le seuil huit vient de la porte précédente, pas d'une ligne GPU inventée.

Portée : raccord exact des feuilles CPU/GPU, distribution parallèle programmée et tampons comptés. Ces traces
n'enregistrent ni l'affectation de chaque feuille aux workers, ni un gain chronométrique isolé. Elles ne prouvent
pas une préadmission globale de toute la mémoire avant tout calcul, ni le contrat FULL. `finish_repair`, le tri
exact des chaînes de niveaux, est un autre mécanisme ; sa sérialisation n'est pas le défaut CST-0009.

[check.py](check.py) contrôle les dix hashes avant/après, les sept sources contre Git et le manifeste de session,
le corps v11 épinglé et les deux portes publiées. Lectures normal et `-O` identiques. Aucun artefact brut dupliqué,
aucun changement de produit ni du registre par cet auditeur.

```sh
python check.py --repo /workspaces/E-HGP
python -O check.py --repo /workspaces/E-HGP
```

Point distinct conservé ouvert : **CST-0008**. `MemoryBudget::admit` exige toujours un seul pilote d'étage ; les
tests de budget concurrent font une admission commune avant les tâches. Leurs succès et ceux du cache ne
qualifient pas plusieurs admissions d'étages concomitantes. Harmoniser ce contrat avec l'architecture avant
une future Session avec recouvrement ; aucune telle exécution n'est alléguée ici.
