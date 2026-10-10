# Profil G de B3b : où reste le travail ?

10 octobre 2026. Les quatre journaux de profil de la [campagne B3b close](../session_b3b_admission/README.md)
sont recalculés séparément : deux trames, avant/après, quatre passes chacune,
passe 0 écartée. **Après = lot B3 complet `81b0883d1`, pas clés seules du produit
adopté.** Profil informatif de G isolé, u21/K5/48 fils ; instrumentation par
cycles, hors Session recouverte et hors latence FULL. Les parts comprennent
les attentes mémoire et le surcoût de l’instrumentation, non retranché.

| Poste, part des cycles sommés | ng00 avant | ng00 lot | 02/001606 avant | 02/001606 lot |
| --- | ---: | ---: | ---: | ---: |
| Trace | 20,11 % | 20,91 % | 22,02 % | 23,71 % |
| Sonde de population | 13,55 % | 14,32 % | 13,69 % | 14,98 % |
| Proposition | 9,88 % | 10,76 % | 9,61 % | 10,44 % |
| LEM-T1 | 25,27 % | 20,04 % | 27,45 % | 20,93 % |
| Census saturé | 11,83 % | 12,40 % | 8,88 % | 9,26 % |
| Census complet | 6,35 % | 6,61 % | 5,52 % | 5,72 % |

Les occurrences par ordre et passe sont identiques avant/après. LEM-T1 baisse
en temps-fil médian de 489,90 à 368,20 ms sur ng00 et de 784,57 à 569,42 ms sur
02/001606. La résolution instrumentée passe de 42,42 à 40,18 ms et de 61,60 à 58,82 ms.
Il ne faut pas convertir ces écarts en gains FULL ni les attribuer tous aux
seules clés. Le profil initial de la question du développeur reste historique.

Les premières sondes réussissent pour **75,23 % et 77,12 % des représentants**,
respectivement. C’est une raison de mesurer les optimisations du succès exact
de recherche et de matérialisation, sans supposer un gain proportionnel. K2
ne pèse que 6,70 % et 7,37 % des cycles du lot, K5 respectivement 52,03 % et 49,91 %.
Ces parts de temps-fil ne sont pas des bornes sur la latence du pipeline.
Une réduction valable à tous les ordres est préférable à une hypothèse de gain
fondée seulement sur les deux sites de K2.

Coquilles étendues : **354/1 205 734 cellules** pour ng00, **1 744/1 941 682**
pour 02/001606, ordres 2..5. Cela ne donne pas leur part des représentants ni
leur coût : mesurer ces deux quantités avant d’implanter la réduction locale
[9→2 démontrée](../g_travail_math/README.md). Aucun raccourci régulier ne découle
de ces effectifs.

`capture.json` épingle les quatre journaux et conserve toutes les sections,
ordres et agrégats utilisés. Le lecteur vérifie unicité des passes/ordres,
statut terminal, partition exacte des cycles, correspondance des comptes de
trace/sonde aux compteurs d’objet, puis égalité des occurrences avant/après.
Les médianes de parts sont calculées passe par passe ; ne pas sommer des
médianes en les présentant comme une décomposition additive exacte.

```sh
python3 -B check.py /chemin/journaux/profil
python3 -B -O check.py /chemin/journaux/profil
```

La première version du lecteur refusait les lignes finales `ordre`/`exit` ;
elle a été corrigée pour les valider explicitement avant cette capture.
Aucune erreur du moteur n’en est déduite. Aucun natif ni cloud exécuté ici.
