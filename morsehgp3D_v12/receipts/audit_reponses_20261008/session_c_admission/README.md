# MES-C : résultats admis, objectifs non tenus

Session `v12.20261008.mesc`, source **83ed7620d**, archive de résultats **36675728…**, rapport **14f42133…**. Admission avec le [lecteur indépendant figé](../mes_c_contrelecture/README.md), SHA `55ca24fe…`. **Aucun écart entre notre recalcul et le rapport** ; aucune empreinte instable observée parmi les passes admises, à K égal. Ce contrôle ne remplace pas un oracle de géométrie.

La [provenance et la fermeture](../session_mes_c_provenance/README.md) sont vérifiées séparément : commande pilote code 0, arrêt ciblé certifié. Le code 0 du pilote signifie qu'il a rendu son rapport. **Le verdict de mesure est refusé**, à cause de la Session appareil/K10/1 fil expirée. Les trois critères C1/C2/C3 sont **non tenus** ; le défaut potentiel d'admission d'un C3 incomplet ne change pas cette campagne, dont tous les cas difficiles K5 ont été tentés.

## Temps chauds FULL

Chaque chiffre est la médiane des **147 médianes par nuage**, chacune formée de ses deux visites chaudes. La colonne maximum est le maximum de ces 147 médianes, pas le maximum d'une passe. Une seule Session/processus par configuration, sans intervalle de confiance entre processus.

| K | Voie | Fils | Médiane (ms) | Maximum (ms) |
|---|---|---:|---:|---:|
| 5 | CPU | 1 | 136,207 | 3 696,843 |
| 5 | CPU | 4 | 45,835 | 1 033,907 |
| 5 | CPU | 48 | 32,647 | 211,610 |
| 5 | appareil | 1 | 21,682 | 1 036,685 |
| 5 | appareil | 4 | 13,074 | 300,684 |
| 5 | appareil | 48 | 13,443 | 85,892 |
| 10 | CPU | 1 | 507,442 | 22 654,352 |
| 10 | CPU | 4 | 152,798 | 6 080,307 |
| 10 | CPU | 48 | 62,947 | 967,608 |

Sur les **132 nuages réels seuls**, à K5/48 fils : CPU médiane **32,238 ms**, maximum **159,216 ms** ; appareil médiane **13,062 ms**, maximum **56,781 ms**. Les autres 15 cas sont des familles synthétiques saines. Les tableaux par groupe, médianes CPU·ns et régressions sont dans [results.json](results.json). Ce sont les petits nuages MES-C ; ni les trames entières de K, ni une comparaison causale avec la v11.

C1/C2 sont jugés sur l'OLS réelle CPU/K5/48 fils : intercept **20,337 ms** contre 2 ms, pente **11,747 µs/site** contre 3,727 µs/site. L'intercept ajusté n'est pas une mesure physique isolée. La pente appareil de **3,605 µs/site** à 48 fils ne ferme pas C2 : le critère déclaré porte sur CPU. Ne pas substituer une configuration ou un groupe plus favorable après mesure.

## Couverture, refus et expiration

**34 processus tentés sur 60** : 29 réussis, quatre refus, une expiration ; 26 non joués. Neuf Sessions complètes de 441 passes et vingt cas difficiles réussis de deux passes donnent **4 009 passes complètes admises, dont 2 666 chaudes**. Les 24 cas difficiles K5 ont tous été tentés : `sphere_n3000` et `sphere_n10000` refusent `unsupported_degeneracy/wide_leaf` sur CPU et appareil ; les vingt autres réussissent. C3 est donc non tenu sur une cohorte K5 complète.

L'appareil K10/1 fil expire après **324,2 s**, délai résiduel de la limite globale de 2 050 s ; **ce n'est pas le délai de 120 s des cas difficiles**. Son brut contient 393 lignes FULL, 392 libérations, une ouverture et aucun exit ; la dernière FULL, passe 392, n'a pas sa libération. Aucune de ces lignes interrompues n'est transformée en passe admise ou en temps nul. Il n'y a donc **aucune statistique de Session appareil K10 complète**. Les deux autres Sessions appareil K10 et les 24 cas difficiles K10 sont explicitement non joués. Cette expiration ne permet pas de conclure que l'algorithme GPU échoue sur un nuage particulier.

## Portée des preuves

Les hashes des 34 JSONL et du rapport sont épinglés ; les bruts restent hors de ce reçu. Les codes par sous-processus sont inférés du pilote épinglé et de ses états publiés, comme annoncé avant les résultats ; ils ne sont pas présentés comme des codes externes archivés. L'archive de résultats et l'arrêt sont couverts par le reçu de provenance lié ci-dessus. Le lien exact du manifeste local de 159 cas au tar de données reste celui annoncé dans le reçu du lecteur ; cet audit n'a jamais lu ce tar, les XYZ, les IDs ni les classes sémantiques.

Le cache de compilation, les métadonnées publiées et les hash de sources/sonde sont contrôlés, sans recompiler le binaire historique. Les 64 Gio hôte et 64 Gio appareil sont des budgets séparés. Les pics sont ceux des budgets comptés ; le RSS est cumulatif par processus. Les premières visites sont exclues trame par trame. Aucune somme de médianes d'étages ni somme des sous-diagnostics C n'est utilisée comme mur.

```sh
python -B check.py --repo /chemin/du/depot --evidence /chemin/returned
python -B -O check.py --repo /chemin/du/depot --evidence /chemin/returned
```

Ces rejeux lisent les sources Git et les bruts épinglés, sans moteur ni GCP. Sorties identiques en normal et `-O`, conservées dans `results.json`.
