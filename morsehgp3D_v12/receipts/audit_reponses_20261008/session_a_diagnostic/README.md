# A : dernières étapes observées et résidu des horloges

**La fin observée dépend de la trame : R de l’ordre 5 est le dernier marqueur dans 43/45 passes ng02 et 94/111 passes des 37 trames.** Les ordres 1 et 2 ferment généralement le calcul G ; ils ne ferment pas généralement toute la forêt. Ce diagnostic corrige la lecture globale du [reçu développeur e022](../../g4_t2da_20261008/README.md), sans changer ses bruts ni son verdict d’adoption. Lire aussi [l’admission](../session_t2da_admission/README.md) et la [provenance](../session_a_provenance/README.md).

Source après **5f5c0c83f**, avant **27eca166b**. 36 journaux épinglés : 30 processus K5 ng00–02 (cinq par bras et trame, dix passes dont neuf chaudes), puis six Sessions de 37 trames (trois par bras, deux visites dont la seconde chaude). Tous les nombres ci-dessous concernent la voie GPU du catalogue, u21, 48 fils, FULL K1..5. Aucune compilation ni exécution moteur par l’audit.

## Ce que disent les fins par ordre

Le vecteur `fins_par_ordre_ns[k−1]` contient G/noyau T/M/V/R ; T ne marque pas la fin de l’historique. Le repère est `ouverture_ns + temps depuis p.start` : une somme de fenêtres internes, pas exactement le temps depuis l’entrée de `build_tower`. Pour chaque passe, on prend le maximum des 25 marqueurs, **avant** tout regroupement statistique. On ne recherche pas le maximum d’une table de médianes.

| Cohorte après A | Passes chaudes | Dernier marqueur observé |
|---|---:|---|
|ng00|45|R1 : 1 ; **R2 : 42** ; R3 : 1 ; R4 : 1|
|ng01|45|R1 : 1 ; **R2 : 29** ; V2 : 3 ; R3 : 6 ; R4 : 5 ; R5 : 1|
|ng02|45|R3 : 2 ; **R5 : 43**|
|37 trames, trois visites chaudes par trame|111|R1 : 1 ; R2 : 10 ; V2 : 1 ; R3 : 5 ; **R5 : 94**|

Une seconde lecture indépendante, [diagnostic37.py](diagnostic37.py), confirme les 111 passes et distingue les dernières fins de chaque phase : G finit sur l’ordre 1 dans 28 passes et sur l’ordre 2 dans 83 ; le noyau, M, V et R finissent sur l’ordre 5 dans 72, 80, 66 et 94 passes respectivement. [Résultat séparé](diagnostic37.json), mêmes bruts. Cette différence explique pourquoi « G finit sur 1/2 » ne signifie pas « toute la tour finit sur 1/2 ».

Ces marqueurs ne sont ni des durées d’exécution exclusives ni une trace de toutes les dépendances. `complete_step` enregistre M/V/R lors de la clôture de l’étape ; la tour effectue ensuite encore ses clôtures. La dernière fin enregistrée identifie un candidat à examiner, **pas une preuve causale du chemin critique**. Avancer le noyau d’ordre 5 pourrait avancer ses descendants R/M ; le seul fait qu’il se termine avant le retour FULL ne permet pas de rejeter ce levier. Inversement, aucune réduction de son temps ne garantit la même réduction du mur : les dépendances, les ressources et l’ordre des tâches interviennent.

Pour le développeur : séparer ng00/ng01 (souvent R2/V2) de ng02 et des 37 trames (souvent R5). Relever l’attente, le travail et les dépendances de R5 avant d’attribuer toute la queue à l’ordre de résolution de G. Une ablation déclarée du noyau ou de l’ordonnancement doit garder l’objet, les empreintes FULL, les budgets et le même chemin mesuré. Les présents marqueurs ne déterminent pas seuls quelle ablation gagnera.

## Les étapes affichées ne sont pas une partition exhaustive du mur

La sonde documente **P + C + G + raccord + TMVR ≤ mur**, pas une égalité. Ici `raccord=0`, `G=fin_g_ns`, `TMVR=queue_ns`, `fin_ns=fin_g_ns+queue_ns`. Pour chaque passe :

```
Δ = mur − (P + C + G + TMVR)
  = (tour_ns − fin_ns) + [mur − (P + C + tour_ns)].
```

| Après A | Δ médian / maximum, ms | tour_ns−fin_ns médian, ms | transitions médianes, ns |
|---|---:|---:|---:|
|ng00, 45 chaudes|2,387539 / 2,987859|2,387329|200|
|ng01, 45 chaudes|0,050000 / 2,645749|0,049910|190|
|ng02, 45 chaudes|1,784379 / 3,123659|1,784169|200|
|37 trames, 111 chaudes réunies|0,864610 / 6,443147|0,864260|330|

Chaque terme est calculé par passe avant agrégation ; les médianes des termes ne sont pas additionnées. `fin_ns` est enregistré après jonction du Pool et clôtures G/forêt/grand livre, mais avant la copie finale des diagnostics, le déplacement du résultat et la destruction de `SessionRun`. `tour_ns` englobe le retour de `build_tower` et le placement du résultat dans la sonde. Le résidu inclut donc ces opérations et les intervalles d’instrumentation ; il **ne mesure pas isolément la destruction des tampons**. Cette attribution demande une instrumentation ciblée. Le petit terme entre crochets comprend les transitions entre P, C et la tour.

Autre intervalle distinct : de la dernière fin par ordre à `fin_ns`, médiane 1,465 / 1,240 / 1,513 ms pour ng00–02, et 2,147 ms sur les 111 chaudes. Il ne faut ni l’ajouter deux fois à la queue ni le confondre avec Δ. G est lui-même un intervalle mural où la forêt travaille simultanément : sa hausse entre bras n’est pas une hausse démontrée du travail propre à G. Les `fenetres_ns` ne sont pas du CPU·s.

## Le seuil de 100 ms

Les médianes décisives ng00–02 sont inférieures à 100 ms ; **26/45, 45/45 et 35/45** passes chaudes le sont. Cela ne prouve pas un plafond de 100 ms : notamment, le maximum des cinq médianes processus de ng00 dépasse 100 ms (voir admission).

Sur les 37 trames, la médiane des 37 médianes est **160,567 ms** ; leur maximum est **319,740 ms** ; le maximum des 111 passes chaudes est **327,273 ms**. Seules 11/37 médianes et 32/111 passes sont sous 100 ms ; neuf trames ont leurs trois passes sous le seuil. L’échantillon de trois visites chaudes n’établit aucune garantie universelle. Le contrat reste non tenu.

## Rejeu et limites

```
python -B check.py DEPOT RETOUR_A > normal.json
python -B -O check.py DEPOT RETOUR_A > opt.json
cmp normal.json opt.json
cmp normal.json results.json
```

`capture.json` épingle trois sources, le document relu et 36 journaux ; aucune scène n’est ouverte. Le lecteur vérifie les deux bras, les régimes, les dix/74 passes, les libérations, les rotations par processus et les identités des horloges avant de calculer. L’admission complète et les empreintes sont celles du reçu lié, pas remplacées par ce diagnostic. Première version du harnais trop restrictive sur l’ordre inter-processus des 37 trames : corrigée pour les rotations annoncées de 0, 1, 2 ; aucune modification des bruts ni du produit. Les résultats ne simulent pas un nouvel ordonnancement et ne prédisent aucun gain. Contrelecture source/horloges et rejeu normal/−O indépendants par `perf_math`, sans écart ; contre-calcul des 37 trames par `latest_perf_e`.
