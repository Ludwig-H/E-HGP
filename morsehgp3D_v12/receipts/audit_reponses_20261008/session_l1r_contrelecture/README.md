# Contre-lecteur L1r : port explicite du schéma mémoire 902

Lecteur préparé et testé sur JSON synthétiques **avant admission de la seconde
exécution**. Source `ea62cd691`, pilote `457d0e6f`, commande **1** du plan `49892867` ;
la commande 0 récupère l'archive originale L1, qui reste hors de ce lecteur. Les noms
et nombres de sites déclarés sont ceux du manifeste L1 inchangé. Aucun moteur, GCP
ni donnée géométrique n'est exécuté ou lu.

Port explicite du [lecteur L1 figé](../session_l_contrelecture/README.md), SHA
`58cf7f1a…`. L'ancien fichier n'est pas modifié. Le diff des émetteurs 403→ea62 ajoute
uniquement `memoire_octets` et la capture des pics par étage ; le pilote, le plan,
l'indice de commande et les pins sont remplacés explicitement. Un brut 403 sans le
nouveau champ est refusé ; le nouveau schéma n'est pas admis par l'ancien lecteur.

Toutes les gardes antérieures restent : cohorte fermée, types hors bool, séquence
open/full/libération/exit, métadonnées commandées, partitions de durée, budgets
séparés, capacités appareil≤pic appareil et épinglé≤pic hôte. Refus K10, échec,
illisible et non joué restent distincts. CPU/RSS null sont conservés et signalés
comme contrôle indisponible. Les règles de froid/chaud, identité selon le seuil et
B1–B4 restent identiques. Les codes proviennent du rapport épinglé, sauf comparaison
externe explicite ; aucune qualification de campagne ne résulte du lecteur seul.

Nouvelles gardes, sur les clés exactes P/C/G/raccord/TMVR :

- Chaque valeur est une paire `[usage_final,pic]` de u64 hors bool, avec usage≤pic.
- `max(pics)=pic_octets` et `usage_précédent≤pic_suivant`.
- Chaque usage conserve au moins `16×sites` octets résidents.
- Le raccord vaut `[usage_G,usage_G]`, sans allocation budgétée dans ce raccord précis.

Ces dernières conditions sont liées **à cet émetteur séquentiel**, pas à toute future
API : `run_wall` marque puis redémarre le pic après chaque étape ; `restart_peak`
réinitialise le pic à l'usage courant. `Frame` conserve les quatre `Buffer` d'entrée
budgétés (16 octets par point) pendant toute la passe. Le nombre de sites ne dépasse
pas celui des points d'entrée. Le raccord remplit un `std::vector<ForestInput>` et un
`BallSource`, sans allocation dans `MemoryBudget`. Les sources de ces preuves sont
épinglées dans [capture.json](capture.json). Aucune somme des pics n'est une mémoire
simultanée ; RSS et capacité appareil restent des champs distincts.

[test_reader.py](test_reader.py) construit une cohorte entière synthétique, relue
indépendamment et comparée aux seules fonctions pures du pilote épinglé : **32
corruptions refusées**, deux null conservés, quatre issues dont refus K10 et non joué,
préfixe d'une passe froide avant refus conservé. Normal/−O concordants. Les sources
sont lues par Git ; aucune invocation du pilote, compilation ou sonde.

```sh
python3 -B test_reader.py --plan PLAN_L1R
python3 -B -O test_reader.py --plan PLAN_L1R
python3 -B reader.py --repo /workspaces/E-HGP --session L1r --plan PLAN_L1R --results DOSSIER_NOUVEAU_B
```

Les résultats réels, s'ils sont admis, seront dans un reçu séparé de celui-ci.
