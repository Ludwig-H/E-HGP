# CUDA — juge de campagne à durcir avant adoption

**Le juge préparé peut annoncer « adopte » avec des preuves absentes ou incohérentes.** Contre-JSON sur le prototype
non publié : juge `eb2def75…`, pilote `8fc661c2…`, sources complètes épinglées dans `capture.json`. Le reçu porte sur
la future campagne d'intégration du catalogue appareil ; il n'invalide aucune mesure historique déjà publiée.
Cadre v12 hors registre, catalogue hybride, CPU de référence, FULL pi0, u21 ; `public_status=not_claimed`.

## Contre-preuves exécutées

`check.py` copie les deux scripts épinglés dans un dossier temporaire et appelle réellement `judge`, `parse_probe`
et `summary`. Son point de départ est le rapport conforme de l'auto-test officiel ; chaque mutation est indépendante.
Les temps sont inventés, aucun moteur, GPU, compilation ou GCP. Python normal et `-O` rendent exactement `result.json`.

| Mutation | Résultat capturé | Preuve manquante ou incohérente |
| --- | --- | --- |
| Aucune, puis toutes les prises K5 à 45 ms | adopte | Témoins conformes |
| Une médiane de processus à 45 ms + 1 ns | rejette | Le second critère temporel fonctionne, même si la médiane globale reste basse |
| Toutes les étapes supprimées | **adopte** | `stage_table` ignore les composantes absentes |
| Tous les identifiants de processus remplacés par zéro | **adopte** | Cinq lignes ne prouvent pas cinq prises distinctes |
| Comptes `balls`, `incidences`, `levels`, `ledger` retirés des identités CPU et appareil | **adopte** | L'égalité `None == None` tient lieu de preuve |
| Un seul digest gardé pour dix passes CPU | **adopte** | Le `set` conserve une empreinte conforme, sans vérifier le nombre de preuves |
| Durées totales et étapes remplacées par des entiers négatifs | **adopte** | Les types entiers sont vérifiés, pas la positivité |
| Mutants d'identité déclarés `critere=temps`, `timeout=true`, sans résultats | **adopte** | Le critère est cru depuis le rapport, pas fixé par identifiant |

Le contre-témoin supplémentaire du pilote change `rewritten_device` à la première des trois passes, puis laisse la
dernière correcte. **Les résumés produits sont identiques avant et après cette perte.** `summary` ne conserve les
diagnostics physiques et hybrides que pour la dernière passe ; le juge ne peut donc vérifier les autres. Cette
preuve porte sur une information effectivement supprimée en amont, pas seulement sur un rapport forgé.
Le petit flux utilisé ici contient seulement les lignes nécessaires à `summary` : c'est une sonde de perte
d'information, **pas** un témoin de conformité au format complet de la sonde native.

Les seuils sont comparés en nanosecondes avant arrondi d'affichage : le contrôle positif à 45 ms + 1 ns l'atteste.
Le défaut de cohorte reste distinct de ce calcul : `frame_stats` accepte au moins cinq enregistrements et ne valide
pas l'unicité de `process`. La règle de 45 valeurs demande une cohorte exactement définie, ou une règle explicite
pour davantage de prises ; le nombre de lignes ne remplace pas l'identité des prises.

## Corrections ciblées avant campagne qualifiante

1. Valider les formes et champs obligatoires avant comparaison : objets JSON, entiers non booléens et non négatifs,
   comptes et grand livre complets, neuf composantes temporelles présentes, empreintes SHA256 complètes et leur
   nombre attendu. Refuser une absence au lieu de comparer des valeurs `None`. Contrôler les métadonnées de la
   sonde réelle, notamment ses lignes `open`/`exit` et les indices/configurations de passes.
2. Conserver les diagnostics et la section `device` **dans chaque passe** du résumé. Juger chaque passe d'identité,
   les composantes appareil/hôte et les causes. La présence des octets et opérations de transfert ne doit pas être
   déduite du seul temps total ; les capacités/allocations rapportées restent des diagnostics à contrôler.
3. Exiger des identifiants de processus distincts et la cohorte attendue par trame/K. Valider toutes les étapes
   requises avant de calculer leur somme et les deux statistiques de décision. Fixer le critère de chaque mutant
   depuis le manifeste attendu ; un délai d'un mutant d'identité ne constitue pas sa mise à mort causale.
4. Rattacher le rapport aux journaux et empreintes réellement produits. Le pilote relève des hashes de binaires,
   mais le juge n'en exige pas la présence ; les source/entrée/commandes relèvent aussi de la session extérieure.
   Ne pas qualifier cette provenance sur la seule chaîne `environment.gpu` ou un booléen `build.ok`.

La sonde publie `exit` avec **`status` et `reason`**, pas un champ `code` ; l'ouverture appareil publie les mêmes
champs et `open_ns`. Le durcissement doit suivre ce format exact pour accepter une véritable sortie conforme.
Ce reçu fournit les contre-preuves, pas encore un remplacement complet du juge.

## Isolation, portes et limites de la prélecture

`gpu_quiet_before` est bien exigé vrai ; `gpu_quiet_after`, bien que relevé, n'est jamais jugé. Le témoin où il vaut
faux reste adopté. La docstring actuelle exige explicitement l'isolation **avant** les temps : le contrôle après
est une limite de couverture à compléter pour surveiller la campagne, et non une violation ajoutée rétroactivement
à cette règle. Deux observations ponctuelles ne prouvent pas à elles seules l'absence d'un concurrent entre elles.

La porte `device_open` impose une carte disponible au travers du pilote et compare les catalogues. Au corps
`4c0c773d…` capturé, elle ne demande toujours pas les nouveaux diagnostics ; la comparaison physique existe dans
`pipeline_witnesses` sur l'exécuteur Pool. Ajouter cette vérification au chemin réellement GPU, y compris deux appels
résidents successifs. Une sortie textuelle annonçant la voie appareil ne remplace pas ces assertions.

La correction des compteurs reste favorable par lecture dans
`../cuda_compteurs_reponse/README.md` : aucune régression de leur accumulation n'est déduite ici. Le nouveau champ
de `BatchStats` est alloué par `sizeof`, sa réduction reste bornée par le lot de `2^17` feuilles ; les réécritures
appareil et hôte restent distinctes des reprises CPU. La faiblesse porte sur la preuve conservée et jugée.

Rejeu, avec un dossier contenant les deux scripts correspondant aux hashes capturés :

```sh
python3 -B -S check.py --source-dir CHEMIN_BENCH > /tmp/cuda-judge-normal.json
python3 -B -S -O check.py --source-dir CHEMIN_BENCH > /tmp/cuda-judge-opt.json
cmp /tmp/cuda-judge-normal.json /tmp/cuda-judge-opt.json
cmp result.json /tmp/cuda-judge-normal.json
sha256sum -c SHA256SUMS
```

Aucune source produit ni aucun ancien reçu modifié. Les scripts refusent un corps différent de la capture ; leurs
résultats ne sont ni une exécution réelle de campagne ni une qualification CUDA.
