# Intégration T2-d-A : garder le protocole distinct des métadonnées relues

Capture non commise du 8 octobre 2026, date exacte et HEAD dans `capture.json`. Le pilote **ea9023cf** est
identique octet pour octet à celui du [reçu schéma 902 publié](../t2d_a_schema902/README.md). Aucun correctif
d'admission de ce pilote n'est donc acquis par sa seule arrivée dans l'arbre d'intégration. Le README a évolué.
La capture est persistante hors dépôt ; aucune lecture de XYZ/IDs, compilation, exécution moteur ou GCP.

Trois compléments sont prouvés avec **`juger(..., verifier=True)`** : journaux JSON synthétiques distincts,
re-hachés, relecture complète et résumés reconstruits par le lecteur capturé. Le nominal reprend explicitement
les trois fabriques `rows/take/report` du reçu publié ; seuls les champs d'environnement sont mis au format du
producteur et `campagne_k5.fils=48` est ajouté. C'est un positif de lecteur, pas une qualification géométrique ni
une preuve du format final de la sonde encore en intégration.

| modification isolée | pilote capturé | proposition |
| --- | --- | --- |
| aucune, témoin complet | adopte | adopte |
| observation `gpu_apps=None` avant campagne | **adopte** | refuse |
| quatrième trame de campagne complète, journaux nouveaux | **adopte** | refuse |
| toutes les prises chronométrées après au schéma séquentiel | **adopte** | refuse |

Le dernier témoin conserve des étapes séquentielles cohérentes et toutes les identités FUL1 ; il n'affirme pas
une sortie FULL géométriquement fausse. Il montre que le juge peut adopter un **autre protocole** : aucune prise
chronométrée après ne comporte fenêtre ou recouvrement. `reverifier` ne reconstitue pas la commande depuis la
place de la prise dans la campagne ; il croit son propre `take.attendu`, qui peut annoncer « séquentiel ».
Le re-hachage d'un journal n'atteste ni sa cohorte ni ses paramètres attendus.

`campagne.patch` propose une garde **avant** la relecture brute : observation GPU connue et vide, exactement
ng00/ng01/ng02, nombre déclaré de tours, deux bras par tour, configurations dérivées de cette place (K5,
fils/passes déclarés, appareil, sans empreinte, avant séquentiel et après recouvert). Les naturels sont stricts,
et la comparaison JSON des métadonnées distingue booléens/flottants/entiers. Les trois défauts deviennent des
**refus bloquants**, pas de simples contrôles affichés. Le patch s'applique en copie temporaire uniquement ;
aucun produit vivant n'est modifié. Il ne change pas les cinq auto-tests statistiques à `verifier=False`.

La proposition reste ciblée : elle ne ferme pas encore la cohorte d'identité ni la Session informative de
37 trames, et n'ajoute pas un manifeste de commandes immuable au rapport. Ceux-ci doivent être dérivés du plan
et du manifeste de métadonnées figés, avec noms uniques, correspondance par K, W1 et prises après séquentielles
explicitement séparées. Les paramètres des cas effectivement en échec doivent également être liés à leur
place ; `valide=False` n'est ni une mesure nulle ni une exécution omissible. Aucun résultat réel A n'est invalidé
par ces doubles synthétiques.

Les autres résidus sont déjà prouvés au **même SHA**, sans nouveau rejeu redondant ici :

- `fin_ns = fin_g_ns + queue_ns` manque, alors que G=`fin_g_ns` et TMVR=`queue_ns` sont vérifiés ; les instants
  doivent rester dans l'enveloppe `tour_ns`, avec les bornes par ordre justifiées par leur émetteur.
- Le bloc `memoire_octets` peut manquer. Exiger les clés propres à chaque schéma, les couples d'entiers
  `used <= peak`, leur maximum égal à `pic_octets`, et les contraintes de pics successifs justifiées par
  `restart_peak`. Distinguer le budget partagé et le budget appareil séparé à partir de l'ouverture.
- Certaines métadonnées et la libération admettent encore booléens/entiers égaux ; `cpu_ns` absent est assimilé
  à null. Contrôler la présence des champs toujours émis avant leurs valeurs.

Le schéma recouvert garde un vrai **mur FULL** et une partition P/C/G/raccord/TMVR. Les fenêtres T/M/V/R sont des
sommes d'intervalles de tâches : **ni temps CPU ni partition du mur**. Ne pas leur appliquer aveuglément les
gardes séquentielles `T+M+V+R <= TMVR` ou `tables+resolution <= G`. Les corrections de fenêtre pré-passe et la
comparabilité des sources/routes sont suivies séparément par les autres auditeurs ; ce patch d'admission ne
les qualifie pas.

Rejeu Python normal et `-O`, sorties identiques à `results.json` :

```sh
python check.py DEPOT PILOTE_EPINGLE
python -O check.py DEPOT PILOTE_EPINGLE
```

La source non commise épinglée reste nécessaire ; le reçu ne la copie pas intégralement. Les fabriques de JSON
sont récupérées depuis leur commit Git historique épinglé. `SHA256SUMS` ferme ce lot, sans changement du registre.
