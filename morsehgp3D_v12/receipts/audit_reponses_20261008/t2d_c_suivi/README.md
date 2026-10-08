# T2-d C : portes locales, admission partielle et variante de croissance

8 octobre 2026. Capture d'un **prototype non publié**, base déclarée
`902041f66`, sources `repo902` et variante séparée `repo902_rendu`.
Les 20 entrées et lignes utiles sont épinglées dans `capture.json` ; leurs
octets restent hors Git. Aucun moteur, compilation ou appel GCP lancé par
l'audit. Ce complément ne réécrit pas les
[budgets précédents](../t2d_c_budgets/README.md) ni les
[contre-exemples d'admission](../t2d_c_admission_reprise/README.md).

Les journaux désormais clos donnent :

| Construction locale | Sélection | Résultat |
| --- | ---: | --- |
| Release/u21, CUDA OFF | 706 | 705 Passed, un sentinel LiDAR Skipped |
| Module catalogue construit avec CUDA/u21 | 56 | 56 Passed |
| Module catalogue construit avec CUDA/u24 | 56 | 56 Passed |

`stream_staging`, `finish_budget`, `pipeline_budget`,
`pipeline_budget_reuse` et `device_open_budget` figurent parmi ces succès.
Sans GPU local, cette dernière porte vérifie l'indisponibilité ; aucune
reprise ni limite de mémoire **sur GPU** n'en découle. Le modèle FrontArray
et la croissance CUDA restent différents. Les fichiers CTest attestent
leur sélection et leur bilan ; la commande extérieure et son code de
sortie ne sont pas conservés ici. Les sources présentes après compilation
ne constituent pas un relevé rétroactif des dépendances du binaire.

`ful1_902.log` conserve neuf paires base/correctif, 18 codes 0 et neuf
empreintes identiques : ng00–02 à K5/K10 et trois synthétiques 8k/16k/32k
à K5. Le conducteur joue **CPU, une passe, trois fils**, puis écrase le
fichier `.out` à chaque cas. Les empreintes agrégées restent relisibles,
mais les lignes complètes des huit premiers cas ne sont pas conservées
par ce conducteur. Entrées non relues/rehachées. Aucune nouvelle mesure
GPU ou décision de vitesse n'est tirée de ces essais.

Le lecteur C `dae75c32…` ajoute huit refus ciblés : mur nul, sites nuls ou
différents de l'identité, mémoire utilisée supérieure au pic, pic global
discordant, étages séquentiels hors mur, T/M/V/R hors enveloppe TMVR,
tables+résolution hors G. Le contrôle synthétique positif passe ; les huit
corruptions passent l'ancien `0b68756f…` et sont refusées par le nouveau,
normal et `-O`. Il s'agit de validation structurelle, pas d'un oracle ou
d'une qualification de matériel. Le nouveau juge transmet le nombre de
sites de l'identité au lecteur FULL. Ces règles séquentielles ne sont pas
transférables au prototype A avec recouvrement.

La correction reste partielle : la lecture des passes catalogue en refus
et des raisons, le code FULL comparé à zéro sans exclure `False`, l'indice
`liberation.pass` et la cohorte supplémentaire ignorée restent dans les
corps déjà contre-jugés. Pas de seconde campagne des témoins inchangés.
Les anciens CTests ne sont pas attribués au nouveau lecteur, modifié après
leur exécution. `leaf_common.hpp`, `leaf_j3.hpp` et `simt.hpp` restent
identiques aux objets Git `902041f66` ; CPU live/SWAR non adoptés. Les
tableaux `positions` et `table` restent réservés séparément : proposition
de réemploi radix non adoptée.

La variante optionnelle `c436572d…`, **hors patch et plan principaux**,
rend le tableau avant sa croissance uniquement pour `keep=0`. Application
isolée du patch : octets exactement égaux au corps de la variante épinglée.
Lecture de vivacité :

- capacité cible calculée avant libération ; `release` synchronise le
  stream, libère, met pointeur/capacité à zéro et rend la réservation ;
- refus de réservation/allocation après cette étape : tableau vide, sans
  second propriétaire ; le neuf n'est adopté qu'après synchronisation ;
- `keep>0` garde l'ancien propriétaire jusqu'au succès ; les pointeurs de
  kernels sont empruntés dans l'appel, les résultats publiés sont possédés
  par le catalogue hôte ; les appels résidentiels sont séquentiels ;
- reprise : nuage, fault et parcours sont réinitialisés, compteurs de
  lots locaux neufs ; un refus ne publie pas de catalogue. À destruction,
  l'état et ses tableaux précèdent l'exécuteur et ses réservations.

Sous réussite des libérations CUDA, aucun double free ni usage après free
nouveau n'a été trouvé. `cudaFree` reste non vérifié dans ce corps ; une
faute pilote persistante relève du nettoyage best-effort, pas de la
garantie de reprise après simple refus de ressources. Le modèle abstrait
contrôle 192 parcours de propriété/allocation/refus/reprise/destruction ;
il ne simule ni pilote, concurrence réelle, ni calcul des kernels.

**La trajectoire des capacités après refus change.** Pour u32, ancienne
capacité 1024, besoin 1025 et limite 6000 octets : cible initiale
2560 éléments/10240 octets, refus après libération ; tentative suivante
depuis capacité zéro, cible 1025/4100 octets, admise. « Capacités
inchangées » décrit donc la première tentative, pas tous les retries.
Ce comportement n'est pas un résultat géométrique incorrect. La porte
utile avant adoption est une séquence réelle sur le même contexte GPU :
succès, croissance sous limite qui refuse, nouvelle entrée admissible,
identités complètes, budgets et réservations rendus à la destruction.
La porte existante à contextes neufs ne couvre pas cette séquence.

Relecture uniquement Python/métadonnées, avec la capture extérieure :

```sh
python check.py --evidence /chemin/capture --check
python -O check.py --evidence /chemin/capture --check
```

Les deux sorties correspondent à `results.json`. Aucun gain de pic réel,
latence, contrat FULL ou qualification CUDA n'est accordé par ce reçu.
