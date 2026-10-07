# MES-P : lecture provisoire avant la campagne 1/4/48 fils

7 octobre 2026. Analyseur et sonde v11 lus au pin `f601b36ace16bcc8f7ac9bc532e45ab5079f9667` ; le diff de
`pilote_p.py` est **non commis, en construction**, capturé séparément dans `pilote_p.pending.patch` et
`verification.json`. Aucun résultat de vitesse ni qualification de ce diff n'est annoncé.

**CST-0238 proposé : l'analyseur mélange silencieusement les nombres de fils.** Le pilote calcule ses ajustements
par `(K,fils)`, mais `analyse_p.py` regroupe les tableaux réels, les synthétiques et les ajustements par K/famille
sans filtrer ni afficher `fils`. Une campagne 1/4/48 serait ainsi résumée comme un seul régime.

Témoin au vrai CLI, valeurs synthétiques déclarées : deux nuages de 100 et 200 sites, K5, trois nombres de fils.
Pris séparément, les trois fichiers produisent 100,00 / 60,00 / 10,00 µs par site. Leur réunion produit **code 0**,
six prises dans une seule ligne et **56,67 µs par site**, sans aucune mention des fils. Cette pente n'est celle
d'aucun des trois régimes. Quatre appels par mode Python, normal/`-O` identiques. La campagne G historique, limitée
à 48 fils, n'est pas invalidée par ce témoin ; le défaut apparaît lorsque plusieurs valeurs de `fils` sont réunies.

Correction à faire avant d'utiliser l'analyse pour choisir le seuil CPU/GPU : grouper par `(K,fils,famille)` et
afficher les fils dans chaque ligne, y compris pour les échecs. La comparaison de fils doit porter sur les mêmes
nuages réussis, ou publier explicitement les différences de cohortes : le filtre actuel enlève indépendamment les
échecs/timeouts de chaque ajustement. Une expiration à un fil peut donc changer son panier sans changer celui à 48.

Le diff courant ajoute seulement `--exclure PREFIXE,...` et la liste des noms exclus dans le JSON. Cette traçabilité
est utile pour isoler la mesure du coût fixe sur les petits nuages LiDAR ; elle ne résout pas l'agrégation ci-dessus.
L'exclusion doit être annoncée comme un périmètre distinct ; elle ne qualifie pas la tenue sur les dégénérescences.
En lecture, une exclusion totale est actuellement rendue avec zéro prise et code 0 : afficher/refuser explicitement
la sélection vide éviterait une campagne inutile. Ce dernier point n'a pas reçu de numéro de constat ni de campagne
de tests : le pilote reste un chantier en cours.

**Périmètre du temps chaud.** `full_probe.cpp::full_pass()` démarre `full_clock` après `prepare_cloud()` et
`make_pool()`. Le pool est créé une fois par processus ; les passes suivantes refont `Cloud` hors chrono FULL.
`wall_ns` couvre index + domaine + forêt, avant l'export. La première passe n'est donc pas une latence froide
complète, et les suivantes ne mesurent pas encore la Session produit v12 intégrée. Le coût fixe ajusté de 5–7 ms ne
peut pas être attribué directement à la **création** du pool ; son travail de distribution pendant FULL reste bien
dans le chrono. Pour comparer 1/4/48, conserver exactement ce périmètre et publier séparément processus complet et
coûts de préparation si on veut en tirer un seuil produit.

**Expiration.** Le pilote utilise `subprocess.run(timeout=...)`, sans nouveau groupe de processus. Ce délai tue le
processus direct ; il n'est pas une garantie d'arrêt de tous les descendants d'un futur wrapper. La sonde v11
actuelle lance des fils dans le même processus : aucun processus enfant résiduel n'a été observé ou provoqué ici.
Les timeouts doivent rester des prises incomplètes distinctes ; aucune extrapolation de leurs passes déjà rendues
ne doit intégrer un résultat chaud qualifié.

Rejeu du seul constat reproductible :

```sh
python -B -S morsehgp3D_v12/receipts/audit_reprise_20261007/mes_p_provisoire/check.py
python -B -S -O morsehgp3D_v12/receipts/audit_reprise_20261007/mes_p_provisoire/check.py
```

`normal.json` conserve les six entrées synthétiques et les quatre sorties Markdown. Sources relues et hachées après
la capture ; aucun code produit modifié, aucune compilation native, aucun nuage réel, aucun GCP.
