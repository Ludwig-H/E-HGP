# T2-d-C : scénario comptable de recouvrement, aucune mesure de A

Sur les trois trames FULL après C, le scénario à coûts inchangés place **deux médianes sur trois sous 100 ms** ; ng02 reste à **103,480 ms**, ou **104,759 ms** pour le maximum des médianes processus. Les murs réellement mesurés restent **153,861 / 122,664 / 155,638 ms**. Ce calcul prépare la lecture de A ; il ne prédit pas ses temps et ne démontre aucune impossibilité.

Sources avant `902041f66`, après `02b735d6b`, ancien pool identique : voir [provenance](../session_t2dc_provenance/README.md) et [admission indépendante](../session_t2dc_admission/README.md). Le pool `5b3362bbd` est absent de ces mesures. Aucun moteur, build, appel distant ou payload de points lu par ce reçu.

## Calcul et cohorte

Reprise de la formule du [modèle K](../session_k_recouvrement_modele/README.md), sur **18 processus**, 180 passes FULL dont **162 chaudes** : trois trames × deux bras × trois processus × neuf chaudes. La passe 0 est écartée séparément dans chacun des processus. K5, W48, u21, catalogue appareil, budget partagé. Les 37 autres trames de T2-d-C mesurent uniquement C : aucune composition avec leurs nouveaux temps et d'anciens G/TMVR. Leurs derniers FULL contractuels restent ceux de K, 241,31 ms en médiane des trames et 467,92 ms en maximum des médianes processus.

Pour **chaque passe**, avant toute agrégation :

```
delta = wall − (P + C + G + raccord + TMVR)
L     = P + C + raccord + max(G, TMVR)
Ldelta = L + delta = wall − min(G, TMVR)
```

Le lecteur réutilise les fonctions de vérification/empreinte du script K épinglé, reprend sa formule, puis recalcule indépendamment `max(P+C+raccord+G, P+C+raccord+TMVR)` et l'identité avec le mur. Il adapte seulement la cohorte ; le `main()` historique à cinq processus n'est jamais appelé. Les 18 journaux hachés sont rapprochés des lignes du rapport, et les agrégats FULL mesurés sont recoupés avec l'admission. Aucun journal, rapport volumineux ou chemin de commande privé n'est dupliqué.

`full_probe.cpp` est identique dans les deux commits : `run_wall`, lignes 192–242, chronomètre séquentiellement P, C, G, raccord et toute TMVR, verticales et registre compris. Validation, empreinte et destruction sont hors de ce mur déclaré. L'entrée est déjà chargée/quantifiée ; segmentation hors chaîne. Le résidu conservé inclut notamment les intervalles entre chronomètres ; il vaut ici au maximum **640 ns**. Les sous-étages T/M/V/R sont inclus dans TMVR, jamais additionnés une seconde fois.

## Résultats en ms

« Médiane » = médiane des 27 chaudes réunies par trame/bras. « Max médianes » = maximum des trois médianes calculées séparément par processus. Le maximum brut reste un diagnostic distinct. Toutes les valeurs du scénario ci-dessous **conservent delta**.

| Trame | Mur avant | Mur après | Scénario avant | Scénario après | Après : max médianes | Après : max brut | Après : passes <100 ms |
|---|---:|---:|---:|---:|---:|---:|---:|
| ng00 | 160,162 | 153,861 | 102,366 | **96,047** | 96,059 | 101,785 | 24/27 |
| ng01 | 128,006 | 122,664 | 84,315 | **78,671** | 79,188 | 80,472 | 27/27 |
| ng02 | 164,872 | 155,638 | 113,052 | **103,480** | 104,759 | 108,860 | 3/27 |

Le scénario passe de 1/3 à 2/3 trames sous 100 ms, tant en médiane qu'en maximum des médianes processus ; de 28/81 à 54/81 passes. **Aucune des 81 passes FULL réellement mesurées après C n'est sous 100 ms.** La médiane des trois médianes projetées vaut 96,047 ms, leur maximum des médianes processus 104,759 ms ; ce ne sont pas des statistiques sur plusieurs séquences ni sur les 37 trames contractuelles.

## Travail restant et priorité

Dans les **162 passes** avant/après, TMVR est plus long que G. Sous ce scénario seulement, raccourcir G sans changer TMVR ne réduit donc pas le maximum ; il reste cependant à réaliser leurs dépendances et leur recouvrement. Les médianes après C sont :

| Trame | P+C+raccord, calculé par passe | C | G | TMVR |
|---|---:|---:|---:|---:|
| ng00 | 30,102 | 28,305 | 57,956 | 65,688 |
| ng01 | 26,675 | 25,018 | 43,735 | 52,048 |
| ng02 | 29,826 | 27,903 | 52,298 | 73,676 |

Ces colonnes de médianes ne s'additionnent pas. Sur ng02, le manque positif à 100 ms calculé par passe puis agrégé vaut **3,480 ms en médiane**, **4,759 ms au maximum des médianes processus**, **8,860 ms au maximum brut**. Dans le scénario, agir sur C ou TMVR réduit ce manque tant que TMVR reste le chemin le plus long. Pour ng00, trois passes dépassent encore 100 ms malgré ses médianes favorables. Mesurer le recouvrement effectif et ces variations reste nécessaire.

L'appariement descriptif par trame/tour/indice chaud donne un écart médian C après−avant de **−7,030 / −5,935 / −9,658 ms**, contre **−6,517 / −5,946 / −10,340 ms** pour FULL. Ces différences sont calculées avant leur médiane ; elles ne sont ni une somme de médianes ni une attribution causale propre au pool ou à v11.

A change travail, allocation, calendrier, concurrence et contention. Le raccord lui-même dépend de G ; sa position dans cette formule ne constitue pas un ordonnancement exécutable. Le scénario idéal n'est donc ni une borne universelle du futur code ni une promesse. Son lecteur séquentiel ne doit pas être transféré aux futures fenêtres qui se recouvrent réellement.

## Rejeu léger

```sh
python -B check.py --repo DEPOT --returned RETOURNE > /tmp/t2dc-modele.json
python -B -O check.py --repo DEPOT --returned RETOURNE > /tmp/t2dc-modele-opt.json
cmp /tmp/t2dc-modele.json /tmp/t2dc-modele-opt.json
cmp /tmp/t2dc-modele.json results.json
```

`RETOURNE` est la capture de métadonnées fermée par la provenance. L'option `--admission DOSSIER` permet la contrelecture avant copie du reçu frère dans le dépôt. Normal et `-O` concordent ; les mêmes murs sont retrouvés dans l'admission. [Pins et 18 hashes](capture.json), [résultats exacts en ns](results.json), [lecteur](check.py), [fermeture](SHA256SUMS).
