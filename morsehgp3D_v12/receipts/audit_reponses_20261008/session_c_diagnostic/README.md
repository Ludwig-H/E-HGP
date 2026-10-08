# MES-C K5 : où se trouve le coût CPU

**Le catalogue porte l'essentiel de l'écart CPU/appareil dans cette campagne. La largeur de 48 fils pénalise tous les plus petits nuages réels observés.** Ce diagnostic relit les six Sessions K5 closes de MES-C, au Git **83ed7620d0b243343688a45473aa479240129725** : mêmes 147 nuages, dont **132 réels** et 15 synthétiques sains, trois tours par configuration. La première visite de chaque nuage est exclue ; restent **1 764 passes chaudes**, deux par nuage/configuration. Aucune nouvelle sonde n'est exécutée.

La [provenance et l'arrêt](../session_mes_c_provenance/README.md) et [l'admission de la campagne](../session_c_admission/README.md) sont distincts. Les refus des familles difficiles et l'expiration K10 ne font pas partie de ces six Sessions complètes ; ils restent des résultats de la campagne. Aucun critère MES-C n'est requalifié par ce diagnostic.

## Les 132 nuages réels

Chaque valeur ci-dessous est la médiane entre nuages de leur médiane des deux passes chaudes, en ms. **Les colonnes ne doivent pas être additionnées** : les médianes peuvent correspondre à des nuages différents.

| Catalogue | Fils | FULL | P | C | G | TMVR |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| CPU | 1 | 117,749 | 0,037 | 104,565 | 10,556 | 3,604 |
| CPU | 4 | 40,229 | 0,056 | 34,151 | 4,611 | 1,865 |
| CPU | 48 | 32,238 | 0,065 | 24,801 | 5,094 | 2,032 |
| GPU | 1 | 18,660 | 0,037 | 4,844 | 10,526 | 3,631 |
| GPU | 4 | 11,782 | 0,057 | 4,947 | 4,809 | 1,905 |
| GPU | 48 | 13,062 | 0,065 | 5,287 | 5,470 | 2,034 |

« GPU » désigne le catalogue sur appareil ; P, G et TMVR restent sur CPU. La part C/mur, calculée sur chaque passe avant agrégation, a pour médiane **87,30 %, 82,92 %, 75,99 %** sur CPU W1/4/48. P est faible ici ; cela ne prouve rien pour un autre régime ou une autre taille.

Le calcul **CPU moins appareil** est effectué sur chaque paire (nuage, tour chaud), puis agrégé. Toutes les 132 médianes FULL par nuage sont plus faibles avec le catalogue appareil à chacun des trois nombres de fils.

| Fils | Écart FULL médian | Écart C médian | Écart G médian | Écart TMVR médian |
| --- | ---: | ---: | ---: | ---: |
| 1 | 99,815 | 99,803 | +0,015 | −0,018 |
| 4 | 28,978 | 29,236 | −0,123 | −0,053 |
| 48 | 19,355 | 19,728 | −0,303 | −0,051 |

La décomposition **moyenne**, formée à partir des différences avant agrégation, est additive : à W48, écart FULL **27,665 ms = C 28,119 ms + hors C −0,454 ms**. C porte donc l'écart descriptif ; le reste le compense légèrement. Ce n'est pas une somme de médianes, ni une expérience randomisée établissant la cause des écarts aval.

Pour les **147 nuages** réunis, les médianes FULL CPU sont 136,207/45,835/32,647 ms et appareil 21,682/13,074/13,443 ms. Les écarts FULL appariés sont 97,815/28,440/19,129 ms. Les deux cohortes sont conservées séparément dans [results.json](results.json).

## Le changement de largeur dépend du nuage

Différence appariée **W48 moins W4**, médiane par nuage puis entre nuages réels, en ms ; positif signifie plus lent à W48.

| Sites | Nuages | Δ FULL CPU | CPU plus lents | Δ FULL appareil | Appareil plus lents |
| --- | ---: | ---: | ---: | ---: | ---: |
| 100–300 | 41 | +7,612 | 41/41 | +3,653 | 41/41 |
| 301–1 000 | 36 | −0,030 | 18/36 | +1,910 | 29/36 |
| 1 001–3 000 | 28 | −54,271 | 0/28 | −4,169 | 2/28 |
| 3 001–10 000 | 27 | −225,766 | 0/27 | −28,462 | 0/27 |

Sur les 132 réels, W48 ralentit le FULL de **59 cas CPU** et **72 cas appareil** par rapport à W4. Pour les 41 plus petits, l'écart médian appareil est surtout dans G (**+2,879 ms**), puis TMVR (+0,607 ms), alors que C n'augmente que de +0,176 ms. Conserver une grande largeur pour tout le pipeline n'est donc pas un remède universel au coût du catalogue CPU.

## Lire correctement les sous-étapes C

À W48 sur les réels, écarts appariés CPU−appareil médians : parcours **8,390 ms**, feuilles **4,922 ms**, émission **1,972 ms**, fin d'étage **5,603 ms**. Les valeurs et les variations W1→4→48 sont publiées séparément ; ne pas additionner ces médianes.

Les frontières ne sont pas identiques opération par opération :

- CPU `catalogue.cpp:46–63` définit parcours comme le temps de `traverse` moins les fenêtres count/fill. `leaves.cpp:249–303` place les allocations par lot, la réduction des compteurs, les préfixes et les réservations en dehors de ces deux fenêtres : ils restent dans « parcours ». Ce champ n'est pas une mesure isolée du BFS.
- CPU `assemble.cpp:129–134` met rassemblement, copies de l'exécuteur et publication dans `fin_etage` ; ses champs transferts/publication sont nuls. L'appareil les sépare (`device_pipeline.hpp:148–173,230`). Une copie plus chère ne se compare donc pas par le seul champ `transferts`.
- Le catalogue appareil conserve ses capacités dans son contexte entre trames ; les tableaux de finition CPU sont locaux à l'appel. La Session et le Pool sont résidents des deux côtés, avec `cache=0` par défaut dans ce plan. L'écart C inclut cette différence de réemploi et les allocations ; il n'isole pas le débit arithmétique CPU contre GPU.

Le mur est celui de `full_probe.cpp:192–235` : P+C+G+raccord+TMVR, allocations de ces étapes comprises. Validation, empreinte FUL1 et destruction du résultat sont publiées hors mur. Les sous-C sont ici disjoints et leur somme ne dépasse C sur aucune des 1 764 passes utilisées ; le résidu est calculé avant agrégation, pas estimé par différence de médianes.

## Priorités concrètes proposées

1. **Qualifier une largeur adaptée au travail de chaque étape.** Sur CPU, W4 améliore chacun des 132 cas par rapport à W1, mais W48 dégrade tous les cas 100–300 sites. Il existe déjà un chemin direct pour au plus huit warps dans `PoolExecutor::launch`. En revanche `LeafStage::batch` appelle `parallel_for` pour count/fill et le Pool réveille tous ses workers pour tout appel non vide (`pool.cpp:88–115`). Mesurer le nombre de lots/chunks réellement utiles et les rendez-vous permettrait de tester une voie directe ou moins de workers sans modifier les identités ni les refus. Les classes de taille ci-dessus sont un diagnostic, pas un seuil produit déjà qualifié.
2. **Traiter C CPU en deux fronts.** À W1/W4, les feuilles dominent (74,064/19,989 ms de médiane réelle) ; les propositions CPU SWAR/live doivent être jugées dans ce périmètre. À W48, la finition et la fenêtre résiduelle « parcours » deviennent prioritaires. Séparer allocations/réductions/préfixes du parcours, puis mesurer grains de lancement et réemploi des tampons, permettra de choisir entre les pistes. Ces journaux n'isolent pas leur causalité et ne promettent aucun gain du seul réemploi radix.
3. **Garder G/TMVR dans l'ablation des petits cas.** Avec C sur appareil, le coût des fils est surtout aval sur les 41 plus petits. Optimiser uniquement les feuilles CPU ne règle pas cette partie ; préserver une mesure FULL englobante et les empreintes lors de chaque changement.

Ces priorités portent sur cette v12 empaquetée. Les frontières, caches, objets et campagnes v11 diffèrent : aucune cause d'une régression par rapport à v11 n'est établie ici. Il n'y a qu'un processus par configuration et deux visites chaudes par nuage, dans un ordre fixe, sans A/A ni répétition indépendante de processus. Ni accélération générale ni intervalle de confiance n'en découlent.

## Relecture

Le script réutilise explicitement le [lecteur MES-C figé](../mes_c_contrelecture/reader.py) au hash `55ca24fe…`, contrôle les six hashes JSONL, les paramètres et les empreintes de tous les nuages à K5. Les codes zéro restent inférés du pilote/rapport épinglés, comme dans l'admission. **66 médianes d'écarts** communes ont été recoupées avec le calcul indépendant de l'autre auditeur : concordance complète, sans recopier son script ni ses tables. Les résultats tiennent en normal et `-O`.

```sh
python -B check.py --repo /chemin/depot --bruts /chemin/brut --check
python -B -O check.py --repo /chemin/depot --bruts /chemin/brut --check
```

Les pins source et lignes utiles sont dans [capture.json](capture.json). Aucun bruit d'exécution, archive de résultats ou payload sous licence n'est dupliqué dans ce reçu.
