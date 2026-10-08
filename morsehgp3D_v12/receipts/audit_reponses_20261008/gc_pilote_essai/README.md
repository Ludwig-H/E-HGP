# Gc : essai local du pilote clos, refus d'effectif attendu

Le pilote T2-c du prototype `45976be8ddc489f14494937b13e43d0ca5fa0a55` publie son bilan final le 8 octobre à
**01:00:20 UTC**. Son jugement refuse les quatre leviers : les trois cohortes K5 comportent chacune deux tours
valides, contre dix exigés. C'est le résultat attendu de `--essai --processus 2`, une réussite de contrôle du
pilote, **aucune adoption de performance ni qualification G4**.

[capture.json](capture.json) épingle les sept artefacts relus, dont le rapport final, le journal de conduite,
le pilote et l'archive du bras avant (`45a69c21…`). Le corps testé est l'arbre de 359 sources `33410128…`, déjà
qualifié dans [gc_rebase_portes](../../audit_reponses_20261007/gc_rebase_portes/README.md). Aucun transfert vers
l'intégration produit en cours n'est fait ici.

[check.py](check.py) réexécute uniquement les lecteurs Python du pilote sur les fichiers existants :

- 30 prises K5 : trois trames, deux tours, cinq positions, six passes, W3 ;
- 38 prises informatives : K10/W3, K5/W1, uniformes et instrumentation ;
- **68 journaux, 308 passes** : schémas, codes, identités, diagnostics et résumés relus sans divergence ;
- jugement recalculé identique au JSON archivé, exactement trois refus d'effectif et quatre verdicts `refuse` ;
- hashes des journaux et des cinq chemins binaires distincts vérifiés avant/après ; `avant_bis` réutilise le
  binaire avant. Les six entrées de construction ne constituent donc pas six binaires indépendants.

La commande du développeur fixe `--fils 3 --jobs 3 --passes 6 --processus 2 --essai`, sous affinité CPU locale.
Le journal consigne aussi les cinq auto-tests Python du pilote ; l'auditeur ne les réexécute pas. Les tableaux
locaux produits et leurs intervalles ne sont pas repris comme chronos G4 ou preuve d'adoption. Le défaut CLI
reste huit processus alors que la règle en exige dix : la campagne réelle devra fixer explicitement
`--processus 10` tant que ce défaut n'est pas harmonisé.

**Le `pilote=0` du conducteur n'authentifie pas le statut externe.** `final_checks.sh` écrit
`echo "$(date -u +%H:%M:%S) pilote=$?"` : la substitution de `date` écrase ici le statut à publier. Le pilote
épinglé, lignes 671–679, fixe `code=3` dès que le jugement contient des refus. Ce code 3 est donc attendu selon
le corps et le jugement relus, mais il n'est pas archivé correctement par ce conducteur. Le même défaut touche
ses mentions `conf/build/rapide/lidar/mutants` : les bilans CTest et le rapport des mutants restent leurs preuves
propres : **675/675**, **6/6**, rapport JSON **18 mutants tués, témoin code 0**. Les reçus antérieurs demeurent
immuables ; seule la valeur probante du zéro shell externe est rectifiée ici. Le RAPPORT déclare un code 3
vérifié séparément pour le pilote ; sans trace de cette vérification, il n'est pas transformé ici en code observé.
La réparation du conducteur consiste à capturer immédiatement `code=$?`, puis à dater et publier `$code`.
Le lecteur reproduit le mécanisme avec la seule commande `false` : statut perdu 0, statut sauvegardé 1.
Aucun test natif n'est relancé pour reconstruire un statut historique.

```sh
python check.py --scratch "$GC"
python -O check.py --scratch "$GC"
```

`GC` désigne le scratch `v12_tour_Gc` épinglé. Lectures normal et `-O` identiques ; aucun natif, build ou calcul
LiDAR supplémentaire exécuté par l'auditeur. Sources, artefacts, journaux et binaires sont revérifiés en fermeture.
