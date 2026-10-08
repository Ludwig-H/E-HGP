# T2-d B — frontière des preuves locales du 8 octobre 2026

Contrelecture des journaux et sources du prototype B, base `902041f66`,
sans moteur, compilation, campagne GCP ni lecture des entrées par l'auditeur.
Les dates exactes et empreintes sont dans `capture.json` ; sources et traces
sont capturées hors dépôt. La source du prototype reste non livrée.

La campagne `ctest -LE long` se clôt à 05:03:09 UTC : **698 Passed et une
Skipped**, zéro échec, 618,60 secondes. La seule porte sautée est
`mhgp12_support_lidar_sentinel`, faute de données fournies à cette campagne.
Les 699 résultats enregistrés ne sont donc pas 699 exécutions réussies.
Le lecteur vérifie les identifiants uniques et le passage des portes
témoins du census, proposition (witnesses/random/routes/inventaire) et
échelles 8k/16k/32k. Le cache pointe sur `git8da4`, Release/u21, CUDA OFF ;
le journal extérieur conserve `build ok 04:52:50` puis `fin 0 05:03:09`.
Le `LastTest.log` courant a depuis été remplacé par un petit lancement :
c'est le journal clos `logs/ctest_lot9020.log` qui fait foi ici.

Le conducteur FUL1 ferme ses neuf paires à 05:09:35 UTC : ng00–02 à K5
et K10, uniformes 8k/16k/32k à K5 ; une passe CPU, trois fils. Les dix-huit
codes sont nuls, les neuf empreintes complètes sont des SHA-256 égaux,
les nombres de sites concordent. **Cette contrelecture porte sur le résumé
du conducteur** : celui-ci ne conserve pas les lignes JSON natives. Elle
ne reconstitue donc pas une admission indépendante de chaque sortie native.
Le conducteur ignore aussi les lignes non JSON ; il ne constitue pas un
juge strict de campagne. Aucun défaut d'empreinte n'est observé dans les
résultats présents ; aucun temps de cette passe unique n'est qualifié.

Le binaire de référence provient de la copie `8da450ab7`, celui du lot de
la base `902041f66` avec les leviers. Les 128 fichiers `src/` des deux bases
sont identiques octet pour octet ; leur sonde diffère par le schéma mémoire
ajouté entre ces versions. La sonde courante du lot est épinglée avec son
binaire et les deux caches. Cela explique la comparaison FUL1 sans
transférer un ancien schéma de mesure à la nouvelle sonde. Nous n'avons pas
de relevé intégral avant compilation attestant chaque binaire.

La campagne index est close à 05:22:09 UTC : **22 mutants TUE par code**,
témoin vert, aucun signal, délai ou échec de construction compté comme
mort. Le manifeste `0d1261bb…` et la liste ordonnée des 22 portes concordent
avec le rapport `d2007b9d…`. L'arbre de 385 fichiers actuellement capturé
reproduit exactement l'empreinte `845776ef…` enregistrée par le runner.
Son périmètre `COPIED` exclut `microbancs/` ; la retouche du pilote en cours
ne change donc pas cette égalité. Le runner qualifie d'abord les témoins
non mutés puis exige un verdict de porte ; en revanche il supprime les
constructions et sorties des mutants tués. Les verdicts sont recoupés avec
son journal global et son code extérieur, **pas rejugés sur des journaux
individuels absents**.

À la frontière de cette capture, num était actif (huit mutants demandés),
tour devait suivre (neuf demandés) et aucun de leurs rapports finaux n'était
présent. Leur réussite n'est pas anticipée. Il s'agit d'une qualification
locale partielle du lot CPU ; ni CUDA, ni gain G4, ni adoption des quatre
leviers ne découlent de ce reçu. Les arguments mathématiques des leviers
sont traités dans les reçus distincts du coordinateur.

Rejeu de la lecture, à partir de la capture extérieure de la passation :

```sh
python check.py --evidence /chemin/capture
python -O check.py --evidence /chemin/capture
```

Les sorties sont identiques au champ `result` de `capture.json` et ne
lancent aucun moteur. Sources et journaux complets restent hors dépôt ;
le reçu dépend de cette capture extérieure. Aucune donnée LiDAR n'est
incluse dans les fichiers du reçu.
