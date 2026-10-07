# Audit des pilotes et preuves des microbancs M2, M3, M4

7 octobre 2026. Codex, volet preuve/mesure. Pin audité : `95247cf4baf2ebd0856c1ac75670f643d24daa6e`.
Cadre : `exploration_v12_hors_registre`, `cpu_reference`, `full_pi0`, `quantized_u21_input_only`,
`public_status=not_claimed`. Aucun GPU, GCP, nuage réel ou build moteur utilisé par ce reçu.
Les calculs et fichiers des injections sont synthétiques et temporaires. Les sources et documents non commis,
les ports déjà examinés par l'audit `00585c5bb` et les noyaux C++ examinés par les autres auditeurs sont hors périmètre.
Le dépôt ultérieur `26b53648c` (outils G4) reste à examiner dans une tranche suivante.

**Conclusion.** CST-0018 reste ouvert : le refus du banc n'interdit pas encore toute adoption dans M2 ; le pilote
M3/M4 peut déclarer conformes des exécutions sans preuve. CST-0213 précise un autre défaut : le paramètre de
réplication de M3 ne répète pas la mesure de résolution qui décide son seuil de gain. Les témoins ci-dessous ne
montrent aucune adoption historique erronée ni erreur géométrique des essais locaux publiés.

## CST-0018 — trois faux verdicts M2, prouvés dans le vrai `main`

Source : `microbancs/mes_m2_feuille/scripts/g4_leaf_bench.py`. Le script de preuve charge les octets Git au pin
ci-dessus. Il conserve `main`, `judge`, collecte, hachage et règles d'adoption ; seuls les appels externes
(construction, environnement, commandes) sont remplacés. Aucun temps injecté n'est une mesure réelle.
Une forme `j3`, cinq processus et quinze durées par processus suffisent ; le rapport variante/témoin vaut 0,1.

| Injection | Résultat observé | Cause précise |
| --- | --- | --- |
| `identity_host` rend code 1, `identity=false`, un écart d'émission | `adopte`, choix `j3`, aucune raison de refus | lignes 406–408 : code et résultat sont enregistrés sans être transmis au juge ni à `s.refusals` |
| Les cinq fichiers de sortie existent avant la prise ; le processus rend 0 sans écrire. Leur JSON désigne **un autre dump, K10/16**, alors que la commande demande K5/24 | `adopte`, choix `j3`, zéro fichier de prise nouvellement écrit | lignes 455–465 : présence du fichier et code 0/1 suffisent ; ni fraîcheur, ni dump/K/feuille, ni périmètre déclaré ne sont confrontés à la commande |
| `--configs 5:16 --forms witness,j3`, donc aucun cas des configurations décisionnelles `5:24,10:24` | `adopte`, choix `j3`, ratio global décisionnel `null` | lignes 234–270 : adoption par absence de motifs ; aucun cas décisionnel n'est exigé ; lignes 272–282 admettent ce choix sans ratio |

Le deuxième cas révèle aussi un chemin concret de production d'un tel reçu périmé : dans
`cuda/leaf_bench.cu:600–606`, le banc ne vérifie pas l'ouverture et l'écriture du fichier JSON avant de rendre 0.
Le pilote ne retire pas l'ancien fichier avant la prise. Le témoin prouve le défaut du contrôleur ; il ne prétend
pas avoir provoqué une panne disque sur GPU.

Contrôles : une erreur de `arena_selftest` et un processus de banc qui rend 2 provoquent bien `refuse` et aucun
choix. Le contrôle valide produit `adopte`. Les refus globaux des lignes 479–484 fonctionnent ; la faille est dans
les conditions qui ne les alimentent pas. Le défaut v11 consistant à ignorer indistinctement le refus global n'est
donc pas attribué tel quel à cette version.

Secondaire : `judge` accepte cinq listes de durées contenant `NaN`, puis adopte avec un ratio `NaN`
(lignes 242, 250–262). La comparaison `hi > seuil` ne rejette pas une valeur non finie. Cette entrée vient d'une
injection dans le lecteur de résultats, pas d'un chrono CUDA observé.

**À fermer avant campagne/adoption.** Propager les erreurs des preuves d'identité obligatoires ; enregistrer un
manifeste attendu des cas/formes/processus/répétitions ; refuser les cellules et cas décisionnels manquants ;
exiger des durées finies strictement positives. Donner un nom neuf à chaque résultat, puis vérifier schéma,
identité du cas et cardinalité des prises contre la commande. Les options exploratoires peuvent publier un résultat
partiel, mais ne doivent pas rendre le verdict contractuel d'adoption. Graver ces trois injections et les contrôles
de refus dans le juge unique.

## CST-0018 — M3/M4 conformes sans sortie, provenance non refermée

Source : `microbancs/mes_m3_m4_tour/pilote.py`. Deux exécutables synthétiques se contentent de `exit 0`, le mutant
de `exit 1`, sans écrire de JSON. Ils sont réellement lancés par `jouer` ; leurs octets et hashes sont déterministes.

- M3 joue cinq processus silencieux, rend code 0 et `routes_identiques_entre_processus=true` : cinq listes vides
  donnent la même signature (`m3`, lignes 292–302). La synthèse contient le cas avec zéro ordre.
- M4 rend code 0 avec zéro ordre et déclare le mutant tué sur le seul code 1 (`m4`, lignes 343–355). La synthèse
  M4 reste vide. Les portes du pilote emploient le même critère de code seul (lignes 217–223).

Il n'existe pas de verdict « adopté » M3/M4 dans ce pilote : il construit des tableaux descriptifs et des contrôles
de codes. Ces résultats prouvent une validation de preuve insuffisante, **pas** un faux verdict d'adoption M3/M4.
Exiger les phases attendues, tous les ordres, les compteurs et identités, une fin explicite et la réponse géométrique
du mutant ; ne pas transformer un simple code de retour en preuve de la porte.

Pour le rattachement de la mesure, `construire` enregistre le hash de `libmhgp11.a` (ligne 193) ; `binaire` ne
contrôle que l'existence du chemin (lignes 200–204). Le pilote ne hache ni l'exécutable effectivement lancé ni ses
sources/paramètres au lancement et à la clôture. La liste `BINAIRES` (ligne 50) n'est jamais utilisée. Les étapes
peuvent être rejouées avec un autre dossier de construction en conservant les autres blocs du rapport chargé
(lignes 557–559). M3/M4 ne vérifient pas les hashes des vidages conservés par une étape `vider` précédente.
Le reçu local contient bien le hash de bibliothèque et les hashes de dumps, mais ne ferme pas le lien entre
chaque exécution et le binaire/source qui l'a produite. Complément de CST-0021 ; aucun remplacement historique
de binaire n'est démontré.

M2 hache davantage : binaires, sources locales et cinq sources v11. Il faut garder ce progrès et fermer aussi la
relation avec les dépendances transitives compilées et les entrées effectivement lues, pas seulement leurs chemins.

## CST-0213 — `--processus` ne répète pas le chrono qui décide M3

Le README M3/M4 §5.4 et le rapport local §6 demandent le rapport de **résolution totale**
`replique_v12/replique_v11` à K10, avec plusieurs processus, pour le seuil de −40 %. Le lancement G4 recommandé
utilise `--processus 3 ... vider m3 m4 rapport`.

Or `vider`, lignes 227–262, lance **une seule fois par cas** `mhgp12_vidage --chrono-resolution R`. Les répétitions
`R` sont dans ce processus ; `--processus` ne concerne que les autres microbancs M3 et M4 (lignes 287, 336).
Leur réplication ne réplique pas les temps de descente totale rangés dans `vidages`.

Témoin sur la fonction originale : `processus=5`, un cas synthétique, une commande de vidage observée. Un second
appel ajoute une seule commande et remplace le premier résultat : ratio synthétique 0,1 puis 0,2, une seule ligne
retenue. Le remplacement vient de `resultat[cas] = ...` (ligne 255), puis `synthese` ne dispose que du dernier
bloc (lignes 401–412). Aucun calcul de géométrie ni lecture de données n'est nécessaire à ce témoin de contrôle.

**À fermer.** Répéter et conserver par processus l'exécution contenant les trois bras de résolution ; rattacher chaque
prise aux mêmes traces et règles d'échauffement ; calculer ensuite la statistique prévue sur les rapports appariés.
Le minimum de passes au sein d'un processus (`vidage_v11.cpp:826–850`) doit rester identifié comme tel. Ne pas lui
attribuer le nombre de processus du microbanc des seules parties M3. Rejouer deux campagnes doit conserver leurs
reçus séparés ou leur historique, sans effacer une prise antérieure.

## Contrôle borné des reçus existants

`check_receipts.py` ne lit que les fichiers versionnés, jamais leurs chemins de données. Il recalcule :

- M2 : 18 lignes, deux formes × neuf cas, **2 748 544 feuilles par forme**, toutes résolues, aucun écart de compteur
  ni d'émission ; les **19 empreintes** du manifeste de sources correspondent aux sources publiées et au rapport
  déplacé dans le reçu.
- M3 : neuf cas, **29 878 990 parties**, identités déclarées vraies, codes 0.
- M4 : neuf cas, **17 497 207 naissances à K≥2** pour T6, aucun écart déclaré ; les naissances K1 sont exclues de
  ce compte de verticales. Les neuf hashes FULL déclarés correspondent aux références conservées par le pilote.
- **17 fichiers présents** correspondent aux SHA-256 des deux manifestes de reçus. Quatre entrées du manifeste
  M3/M4 sont absentes du dépôt au pin : `out/campagne_locale.log`, `_2.log`, `_3.log`, `_4.log`.
  Ce manque est conservé au JSON de contrôle ; aucune copie externe n'est récupérée.

Cette relecture confirme les comptes publiés et la fidélité des fichiers présents, pas le contenu géométrique des
dumps absents ni l'identité historique des exécutables. Les temps locaux restent hors adoption G4.
Les défauts des lecteurs natifs M2 et M4 sont traités par les autres volets (CST-0215 et CST-0214), sans doublon ici.

## Reproduction

Depuis la racine d'un dépôt possédant le pin :

```sh
python3 morsehgp3D_v12/receipts/audit_socle_microbancs_20261007/preuves/probe_pilotes.py
python3 -O morsehgp3D_v12/receipts/audit_socle_microbancs_20261007/preuves/probe_pilotes.py
python3 morsehgp3D_v12/receipts/audit_socle_microbancs_20261007/preuves/check_receipts.py
```

Les scripts refusent une source ou un reçu de travail différent des octets Git épinglés ; les marqueurs des
contre-exemples sont contrôlés explicitement, sans `assert` supprimable sous `-O`.
Les deux jeux de probes terminent avec code 0 et produisent des sorties identiques octet pour octet :
`probes.normal.json`, `probes.optimized.json`. `receipts_recomputed.json` porte les comptes, empreintes et absences.
Les temporaires sont détruits ; aucune archive de code, aucun dump réel, aucune donnée sous licence ne sont ajoutés.
`SHA256SUMS` ferme ce reçu. Aucun fichier produit ou canal d'audit modifié ; aucun commit ni push.
