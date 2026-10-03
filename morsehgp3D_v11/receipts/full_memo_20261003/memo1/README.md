# FULL mémo — capture memo1 close, calendrier incomplet

Source exécutée : `c2c3e0323afe25feadf9ed2c2ab85887ab1be3db`.
Matrice2475/2475 et ASan18 178/178 ; Clang absent autorisé. Les240 mutants
sont recoupés causalement dans les journaux :238 par code/ligne et2 refus de
construction attendus, aucun signal/délai. La configuration mutants utilise
32 créneaux et dure397,196s ; cela ne mesure pas isolément le gain12→32.

**17 succès/18 essais demandés**, aucune divergence observée. Seul
`uniform_u18_n32000`, u21, mode7, est omis avant lancement pour budget.
Les12 essais LiDAR sont tous présents. Rapport `complete=true` mais
`conforming=false`, mur485,980s. Le reçu conserve `failed_remote`, worker1,
DONE3. Arrêt ciblé, retrait des clés OSLogin/privée et libération du verrou
certifiés ; aucune erreur ni aucun avertissement de fermeture. Génération
exacte `2026-10-02T17:26:37.473-07:00`.

Archive originale unique576963octets, SHA256
`26d020f8b1df5fded608cd21297e0341af10fb024042c904150cf0af47ba2fe5`.
Les17 gros payloads ont été supprimés par le banc après contrôle : aucun
n'est réhaché à la lecture de cette capsule. Les12 résultats LiDAR mode3/7
ont les mêmes empreintes RAW par profil et sémantiques que `sweep2` r0.
Huit résumés Python sont réutilisés, neuf décodés ; les chaînes de réemploi
sont contrôlées et ne réutilisent pas les diagnostics, verdicts ou horloges.

Le calendrier contient18 processus frais : trois nuages LiDAR sans sol en
u21/u24, puis trois synthétiques8k/16k/32k en u21, chacun en modes3 et7,
K1..5 et W48. Le mode3 active cacheJ2 et tri indirect ; le mode7 ajoute un
mémo de65536 parties pour toute la construction FULL. La frontière catalogue
reste fixe. La réutilisation Python des résumés décodés est une autre option,
hors chrono moteur : chaque fichier courant est intégralement rehaché.

Le lecteur est **LIVE** : reçu brut local original, archive unique et reçus
LIVE historiques utilisés pour la comparaison `sweep2` restent obligatoires.
Il lit le tar avec `extractfile`, sans extraction ni exécution de binaire.
Les aides Python sont copiées depuis Git dans `source_c2/` et hachées par
`source_contract.json` ; aucun banc WIP n'est importé. Les anciennes aides de
transport/JUnit sont réutilisées dans les autres dossiers de reçus.

Lectures :

```sh
python3 -B morsehgp3D_v11/receipts/full_memo_20261003/memo1/check.py
python3 -B -O morsehgp3D_v11/receipts/full_memo_20261003/memo1/check.py
python3 -B morsehgp3D_v11/receipts/full_memo_20261003/memo1/check_selftest.py
python3 -B -O morsehgp3D_v11/receipts/full_memo_20261003/memo1/check_selftest.py
```

Code0 signifie cohérence des preuves, y compris pour une campagne incomplète
ou échouée. `conforming` reste un champ séparé. Le lecteur vérifie les portes
et leurs inventaires, profils/options CMake, configuration mutants32, causes
des mutants, calendriers18, omissions, échecs, identité des binaires et
entrées, diagnostics, budgets mémoire, temps dérivés, sorties et comparaisons.
Un réemploi de résumé doit renvoyer à un décodage antérieur réussi avec les
mêmes contexte, empreinte brute, taille et résumé. Un payload absent n'est
jamais présenté comme rehaché par ce lecteur ; ses empreintes restent celles
enregistrées par le banc. Un payload archivé est réellement rehaché et décodé.

Les résumés sémantiques vérifient structure, rationnels et verticales du
format ; les petites portes Fraction indépendantes qualifient la géométrie.
Il n'y a pas de scan géométrique Python indépendant de ces nuages entiers.
Les réservations sont celles des `Buffer`, avec les propriétaires retenus et
la table temporaire, sans piles OS, RSS ou Python. Aucun pic par ordre n'est
mesuré. Les timings par ordre sont disjoints mais non exhaustifs. Segmentation,
préparation de grille, hiérarchie sur les points, clustering et GPU sont hors
de ce banc.

## Mesures observées

Temps en secondes, un processus frais par ligne et mode, ordre3 puis7 fixe.
Les rapports sont descriptifs de cette capture, sans intervalle statistique.

| Entrée | B | FULL3 | FULL7 | Forêt3 | Forêt7 | FULL3/7 |
|---|---:|---:|---:|---:|---:|---:|
| ng00,39885sites |21|18,642|14,546|15,308|11,285|1,282|
| ng00 |24|18,595|14,690|15,289|11,313|1,266|
| ng01,35551sites |21|13,313|10,069|11,191|7,919|1,322|
| ng01 |24|13,326|10,119|11,145|7,928|1,317|
| ng02,45845sites |21|15,686|12,082|12,782|9,209|1,298|
| ng02 |24|15,649|12,082|12,797|9,181|1,295|
| uniforme8000 |21|7,484|5,826|7,048|5,394|1,285|
| uniforme16000 |21|17,285|14,412|16,356|13,484|1,199|
| uniforme32000 |21|38,681|omis|36,684|omis|—|

Le contrat FULL≤200ms n'est pas acquis. Les données LiDAR sont les trois
trames0/100/200 de la même séquence08, avec le masque et la grille1mm figés ;
elles ne représentent pas trois séquences. u24 élargit l'arithmétique sur la
même entrée entière, sans ajouter de précision à cette entrée.

Le travail entier par mode est identique en u21/u24. Pour les trois LiDAR,
les totaux suivants cumulent les ordres1..5 ; les étapes en mode3 comprennent
les MEB qui seront évités par le mémo. Un hit de suffixe a déjà payé un
préfixe de descente ; un hit d'origine évite le premier MEB de la requête.

| Entrée | Étapes3 | Requêtes7 | Hits origine7 | Hits suffixe7 | Misses/étapes7 |
|---|---:|---:|---:|---:|---:|
| ng00 |6016334|4480149|1859694|619195|3181215|
| ng01 |4941487|3738985|1639555|540424|2471832|
| ng02 |6166870|4786862|2046980|637128|3176206|

À l'ordre5 seul :

| Entrée | Étapes3 | Requêtes7 | Hits origine7 | Hits suffixe7 | Misses/étapes7 |
|---|---:|---:|---:|---:|---:|
| ng00 |2465441|1691369|589341|286372|1415375|
| ng01 |1988914|1385712|516149|251487|1077488|
| ng02 |2445109|1755278|631318|296320|1366417|

La table native a65536 slots de192octets en u21,208 en u24 :12/13MiB.
Elle est détruite avant retour. Sur les huit paires complètes, réservations
retenues et pic global sont exactement identiques entre modes3/7 ; le pic
antérieur masque donc le coût temporaire de la table, qui est bien payé.
Pour les LiDAR, les pics vont de264666124 à352856536octets et les retenus
de197300288 à250645608octets. Aucun pic par ordre n'est inféré.

## Diamètre et mémo séparés

`sweep2` avait déjà classification au premier témoin et balayage vertical,
mais aucun raccourci par diamètre ni mémo. Comparer son travail au nouveau
mode3 mesure le changement de présentations MEB sur les parties ; les
paires de diamètre nouvellement calculées doivent aussi être comptées.
Les nombres sont identiques pour les deux profils.

| Entrée | Présentations sweep2 | Présentations mode3 | Rapport ancien/nouveau | Paires diamètre mode3 |
|---|---:|---:|---:|---:|
| ng00 |63994685|18297896|3,497|34861979|
| ng01 |51829002|14770741|3,509|28257897|
| ng02 |64239353|18301732|3,510|34909680|

Ce rapport de présentations n'est pas un rapport de temps. Les chronos
`sweep2` et `memo1` viennent de campagnes distinctes ; ils ne constituent pas
une ablation chronométrique isolée du diamètre. En revanche, les modes3/7
appariés utilisent le même exécutable et le même algorithme diamètre.

[`metrics.json`](metrics.json) contient les85 lignes par ordre, temps des
plateaux/verticales, hits/misses, présentations et paires payées, ainsi que
les six comparaisons historiques. `check.py` recalcule ce fichier depuis les
rapports validés ; les retenus de forêt dérivés supposent l'ABI qualifiée
ForestNode24/BirthEntry8/NodeIdx4 et ne sont pas la mémoire totale.

Lecteur et auto-test passent normalement et sous `-O` :18 témoins et
87 corruptions refusées, aucun appel natif. Le premier essai de compactage
avait supposé un champ supplémentaire `scope` présent dans le reçu brut ;
il s'est arrêté avant toute copie de reçu/archive. `capture_notes.json`
conserve cette erreur de lecture, sans modification du brut historique.
