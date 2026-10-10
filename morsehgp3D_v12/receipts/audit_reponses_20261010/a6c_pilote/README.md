# A6c : admission du pilote et diagnostic à compléter

10 octobre 2026. Pin `aa6338ee8b3d5daf6a821043c3bdd9d63ce85c66`.
[Lecteur indépendant](check.py), [résultats](results.json). Aucun moteur exécuté,
aucune donnée réelle chargée, aucun chrono A6c qualifié.

Le pilote conserve la fermeture de cohorte d'A6b : plan tiré de la commande et
du manifeste, prises attendues exactes, codes entiers nuls, identité A/A des
binaires, journaux uniques internes, empreinte puis relecture stricte de chaque
brut, résumé recalculé. Il ne présente pas le défaut B3 consistant à croire les
seuls résumés des identités. Le témoin Python produit **85 journaux artificiels**
(11 identités par bras, six tours de grandes, cinq tours de ng00–02) : nominal
adopté et **huit corruptions refusées**, avec `verifier=True` et le lecteur FULL
livré, sans simuler sa réponse. Les huit auto-tests officiels passent aussi.

Ce contrôle ne prouve ni l'identité géométrique d'une sortie native, ni un gain,
ni la provenance d'une future campagne. Le pilote écrit les stderr individuels
mais ne les référence ni ne les hache dans les prises ; le témoin est accepté
sans aucun `.err`. Les conserver dans l'archive et son manifeste reste à faire
par le lanceur. Épingler les exécutables réellement lancés avant/après toute la
campagne, ainsi que leurs sources et configurations.

**Le diagnostic annoncé aux lignes 48–52 du pilote est incomplet.**
`decision_chaine` rend uniquement `sites` et `chaine`. Les retards prédits et
les drapeaux 35 000–50 000 sites n'y figurent pas. Le tableau des grandes rend
des médianes de murs et de queues, pas les rapports appariés de toutes les
trames annoncées. La cohorte décisive chronométrée ne couvre que les 21 grandes
et ng00–02 ; les autres trames n'ont qu'une prise d'identité par bras.

Complément proposé, sans toucher à la règle ni aux seuils :

1. Ajouter le retard prédit par les coefficients épinglés, `proche_seuil` pour
   35 000 ≤ n ≤ 50 000 et la mention de réemploi de la cohorte de calibration.
   Conserver séparément les aliases ng00–02 et les noms v12set : un même nombre
   de sites ne prouve pas une entrée identique.
2. Sur ng00–02 et les grandes, publier par trame le nombre de tours et la moyenne
   géométrique des rapports **appariés**. Ne pas diviser deux médianes pour
   remplacer cette statistique. Afficher les bornes avec assez de décimales pour
   voir un seuil strict ; le JSON reste l'autorité.
3. Pour les autres trames, marquer « identité seulement, pas de comparaison
   chaude répétée ». Prévoir un banc distinct, figé avant mesure, de nouvelles
   trames de part et d'autre du seuil. Le laisser-un-dehors du modèle de retard
   ne constitue pas une validation indépendante du choix de politique.

Le changement source utilise le seuil dans la tour indépendamment de la voie
CPU/GPU et du nombre de fils. La campagne prévue G4/W48/K5 ne qualifie donc pas
un gain CPU, K10, petits nuages ou multi-millions ; ses identités K10 restent des
contrôles de sorties, pas des chronos contractuels.

```sh
python3 -B -S check.py /chemin/du/depot
python3 -O -B -S check.py /chemin/du/depot
```

Les sources sont relues depuis le commit, hachées dans le résultat ; journaux
artificiels et copies de scripts sont détruits après chaque contrôle. Ce reçu
est une prélecture reproductible du juge, pas une admission de session G4.
